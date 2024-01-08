/*
***************************************************************************
*
* Author: Teunis van Beelen
*
* Copyright (C) 2015 - 2020 Teunis van Beelen
*
* Email: teuniz@protonmail.com
*
***************************************************************************
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
***************************************************************************
*/

#include <QTcpSocket>
#include <QThread>
#include <arpa/inet.h>
#include <locale.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "tmc_dev.h"
#include "tmclan.h"
#include "utils.h"

using namespace std::literals;
enum {
    TMC_LAN_TIMEOUT = 5000,

    MAX_CMD_LEN = 255,
    MAX_RESP_LEN = 1024 * 1024 * 2,
};

TMCLan::TMCLan(QObject* parent)
    : QObject{parent}
    , socket{new QTcpSocket{this}} {
    connect(socket, &QAbstractSocket::hostFound, [] { qWarning("hostFound"); });
    connect(socket, &QAbstractSocket::connected, [this] { sockfd = 0; qWarning("connected"); });
    connect(socket, &QAbstractSocket::disconnected, [this] { sockfd = -1; qWarning("disconnected"); });
    //    connect(socket, &QAbstractSocket::stateChanged, [](QAbstractSocket::SocketState state) { qDebug() << "stateChanged" << state; });
    //    connect(socket, qOverload<QAbstractSocket::SocketError>(&QAbstractSocket::error), [](QAbstractSocket::SocketError error) {
    //        qDebug() << "error" << error;
    //    });
    //    connect(socket, &QAbstractSocket::proxyAuthenticationRequired, [](const QNetworkProxy& proxy, QAuthenticator* authenticator) {
    //        qDebug() << "proxyAuthenticationRequired" << &proxy << authenticator;
    //    });

    // connect(this, &DigitalOsc::Write, socket, qOverload<const QByteArray&>(&QIODevice::write), Qt::QueuedConnection);
    // connect(socket, &QIODevice::readyRead, [this] {
    //     data = socket->readAll();
    //     semaphore.release();
    // });

    // connect(thrd, &QThread::finished, socket, &QObject::deleteLater);
    // thrd->start();
    // qDebug() << __FUNCTION__ << thrd;
}

struct tmcDev* TMCLan::tmcLanOpen(std::string_view hostOrIp) {
    qWarning(__PRETTY_FUNCTION__);

    moveToThread(QThread::currentThread());
    socket = new QTcpSocket{/*QThread::currentThread()*/};
    connect(socket, &QAbstractSocket::hostFound, [] { qInfo("hostFound"); });
    connect(socket, &QAbstractSocket::connected, [this] { sockfd = 0; qInfo("connected"); });
    connect(socket, &QAbstractSocket::disconnected, [this] { sockfd = -1; qInfo("disconnected"); });
    // connect(socket, &QIODevice::readyRead, [this] { qInfo("readyRead: %lld", socket->bytesAvailable()); });
    // connect(socket, &QIODevice::bytesWritten, [this](qint64 bytes) { qInfo("bytesWritten: %rlld", bytes); });

    socket->connectToHost(hostOrIp.data(), 5555);

    if(0) {
        int ctr{10};
        while(!socket->canReadLine() && ctr--)
            socket->waitForReadyRead(100);
        if(!socket->canReadLine()) {
            socket->disconnectFromHost();
            delete socket;
            socket = nullptr;
            return nullptr;
        }
    }

    if(!socket->waitForConnected())
        return nullptr;

    socket->write("*IDN?\n");
    socket->waitForBytesWritten();
    socket->waitForReadyRead();
    qWarning() << socket->readAll();

    tmcDev* tmcDevice{};

    tmcDevice = new(std::nothrow) tmcDev; //(struct tmcDev*)calloc(1, sizeof(struct tmcDev));
    if(tmcDevice == nullptr) {
        socket->disconnectFromHost();
        return nullptr;
    }

    tmcDevice->hdrBuf = new(std::nothrow) char[MAX_RESP_LEN + 1024]; //  (char*)calloc(1, MAX_RESP_LEN + 1024);
    if(tmcDevice->hdrBuf == nullptr) {
        socket->disconnectFromHost();
        delete tmcDevice;
        tmcDevice = nullptr;
        return nullptr;
    }

    tmcDevice->buf = tmcDevice->hdrBuf;

    return tmcDevice;
}

void TMCLan::tmcLanClose(struct tmcDev* tmcDevice) {
    if(socket) {
        socket->disconnectFromHost();
        delete socket;
        socket = nullptr;
    }

    if(tmcDevice != nullptr) {
        delete[] tmcDevice->hdrBuf;
        delete tmcDevice;
        tmcDevice = nullptr;
    }
}
qint64 avg, ctr;
int TMCLan::tmcLanWrite(struct tmcDev* tmcDevice __attribute__((unused)), std::string_view cmd) {
    if(sockfd == -1) return -1;

    // struct name {
    //     QElapsedTimer t;
    //     name() { t.start(); }
    //     ~name() { qCritical("%lld us", (avg += t.nsecsElapsed() / 1000) / ++ctr); }
    // } name;

    int len = cmd.size();
    if(len > MAX_CMD_LEN) {
        qCritical("tmcLan error: command too long\n");
        return -1;
    }
    if(len < 2) {
        qCritical("tmcLan error: command too short\n");
        return -1;
    }

    bool qry = cmd.back() == '?';

    std::string buf{cmd};
    buf += '\n';
    char /*buf[MAX_CMD_LEN + 16]{},*/ str[256]{};
    // strlcpy(buf, cmd.data(), MAX_CMD_LEN + 16);
    // strlcat(buf, "\n", MAX_CMD_LEN + 16);

    if(!(buf.starts_with(":ACQ:MDEP?")
           || buf.starts_with(":ACQ:SRAT?")
           || buf.starts_with(":FUNC:WREC:FMAX?")
           || buf.starts_with(":FUNC:WREC:OPER?")
           || buf.starts_with(":FUNC:WREP:FCUR?")
           || buf.starts_with(":FUNC:WREP:FMAX?")
           || buf.starts_with(":FUNC:WREP:OPER?")
           || buf.starts_with(":MEAS:COUN:VAL?")
           || buf.starts_with(":TRIG:STAT?") /* don't print these commands to the console */
           || buf.starts_with(":TRIG:SWE?")
           || buf.starts_with(":TRIG:SWE?") /* because they are used repeatedly */
           || buf.starts_with(":WAV:DATA?")
           || buf.starts_with(":WAV:FORM BYTE")
           || buf.starts_with(":WAV:MODE NORM")
           || buf.starts_with(":WAV:SOUR CHAN")
           || buf.starts_with(":WAV:XOR?")))
        qCritical("tmcLan write: %s", buf.data());

    if(std::string_view{buf}.starts_with("*RST"))
        qry = 1;

    int n = socket->write(buf.data(), buf.size());
    // socket->waitForBytesWritten();

    if(n != (len + 1)) {
        qCritical("tmcLan error: device write error");
        return -1;
    }

    if(!qry) {
        for(int i{}; i < 20; i++) {
            usleep(25000);

            if(socket->write("*OPC?\n") != 6) {
                qCritical("tmcLan error: device write error");
                return -1;
            }

            socket->waitForReadyRead();
            n = socket->read(str, 128);

            if(n < 0) {
                qCritical("tmcLan error: device read error");
                return -1;
            }

            if(n == 2) {
                if(str[0] == '1')
                    break;
            }
        }
    }

    return len;
}

/*
 * TMC Blockheader ::= #NXXXXXX: is used to describe
 * the length of the data stream, wherein, # is the start denoter of
 * the data stream; N is less than or equal to 9; the N figures
 * followed N represents the length of the data stream in bytes.
 * For example, #9001152054. Wherein, N is 9 and 001152054
 * represents that the data stream contains 1152054 bytes
 * effective data.
 */
int TMCLan::tmcLanRead(struct tmcDev* tmcDevice) {
    int n, size, size2, len;

    char blockhdr[32];

    if(sockfd == -1)
        return -1;

    tmcDevice->hdrBuf[0] = 0;
    tmcDevice->sz = 0;

    size = 0;

    while(1) {
        socket->waitForReadyRead();
        n = socket->read(tmcDevice->hdrBuf + size, MAX_RESP_LEN - size);

        if(n < 1)
            return -2;

        size += n;

        if(tmcDevice->hdrBuf[size - 1] == '\n')
            break;
    }

    if((size < 2) || (size > MAX_RESP_LEN)) {
        tmcDevice->hdrBuf[0] = 0;
        tmcDevice->buf[0] = 0;
        return -3;
    }

    tmcDevice->hdrBuf[size] = 0;

    if(tmcDevice->hdrBuf[0] != '#') {
        if(tmcDevice->hdrBuf[size - 1] == '\n')
            tmcDevice->hdrBuf[--size] = 0;

        tmcDevice->buf = tmcDevice->hdrBuf;
        tmcDevice->sz = size;
        return tmcDevice->sz;
    }

    strncpy(blockhdr, tmcDevice->hdrBuf, 16);

    len = blockhdr[1] - '0';

    if((len < 1) || (len > 9)) {
        blockhdr[31] = 0;
        return -1;
    }

    blockhdr[len + 2] = 0;
    size2 = atoi(blockhdr + 2);
    size--; // remove the last character

    if(size < size2) {
        blockhdr[31] = 0;
        return -1;
    }

    tmcDevice->buf = tmcDevice->hdrBuf + len + 2;
    tmcDevice->sz = size2;
    return tmcDevice->sz;
}
