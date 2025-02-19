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

#include <errno.h>
#include <fcntl.h>
#include <linux/usb/tmc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tmc_dev.h"
#include "utils.h"

#define MAX_CMD_LEN  (255)
#define MAX_RESP_LEN (1024 * 1024 * 2)

struct tmcDev* tmcDevOpen(const char* device) {
    struct tmcDev* dev;

    dev = (struct tmcDev*)calloc(1, sizeof(struct tmcDev));
    if(dev == NULL)
        return NULL;

    dev->hdrBuf = (char*)calloc(1, MAX_RESP_LEN + 1024);
    if(dev->hdrBuf == NULL) {
        free(dev);

        return NULL;
    }

    dev->buf = dev->hdrBuf;

    dev->fd = open(device, O_RDWR);

    if(dev->fd == -1) {
        free(dev->hdrBuf);

        free(dev);

        return NULL;
    }

    return dev;
}

void tmcDevClose(struct tmcDev* dev) {
    if(dev == NULL)
        return;

    close(dev->fd);

    free(dev->hdrBuf);

    free(dev);
}

int tmcDevWrite(struct tmcDev* dev, const char* cmd) {
    int i, n, len, qry = 0;

    char buf[MAX_CMD_LEN + 16],
        str[256];

    if(dev == NULL)
        return -1;

    len = strlen(cmd);

    if(len > MAX_CMD_LEN) {
        printf("tmcDev error: command too long\n");

        return -1;
    }

    if(len < 2) {
        printf("tmcDev error: command too short\n");

        return -1;
    }

    if(cmd[len - 1] == '?')
        qry = 1;

    strlcpy(buf, cmd, MAX_CMD_LEN + 16);

    strlcat(buf, "\n", MAX_CMD_LEN + 16);

    if(!(!strncmp(buf, ":TRIG:STAT?", 11) ||  /* don't print these commands to the console */
           !strncmp(buf, ":TRIG:SWE?", 10) || /* because they are used repeatedly */
           !strncmp(buf, ":WAV:DATA?", 10) || !strncmp(buf, ":WAV:MODE NORM", 14) || !strncmp(buf, ":WAV:FORM BYTE", 14) || !strncmp(buf, ":WAV:SOUR CHAN", 14) || !strncmp(buf, ":TRIG:SWE?", 10) || !strncmp(buf, ":ACQ:SRAT?", 10) || !strncmp(buf, ":ACQ:MDEP?", 10) || !strncmp(buf, ":MEAS:COUN:VAL?", 15) || !strncmp(buf, ":FUNC:WREC:OPER?", 16) || !strncmp(buf, ":FUNC:WREP:OPER?", 16) || !strncmp(buf, ":FUNC:WREP:FMAX?", 16) || !strncmp(buf, ":FUNC:WREC:FMAX?", 16) || !strncmp(buf, ":FUNC:WREP:FCUR?", 16) || !strncmp(buf, ":WAV:XOR?", 9)))
        printf("tmc_dev write: %s", buf);

    if((!strncmp(buf, "*RST", 4)) || (!strncmp(buf, ":AUT", 4)))
        qry = 1;

    n = write(dev->fd, buf, strlen(buf));

    if(n != (len + 1)) {
        printf("tmcDev error: device write error");

        return -1;
    }

    if(!qry) {
        for(i = 0; i < 20; i++) {
            usleep(25000);

            n = write(dev->fd, "*OPC?\n", 6);

            if(n < 0) {
                printf("tmcDev error: device write error");

                return -1;
            }

            n = read(dev->fd, str, 128);

            if(n < 0) {
                printf("tmcDev error: device read error");

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
 * Reading from the file descriptor blocks,
 * there is a timeout of 5000 milli-Sec.
 */
int tmcDevRead(struct tmcDev* dev) {
    int size, size2, len;

    char blockhdr[32];

    if(dev == NULL)
        return -1;

    dev->hdrBuf[0] = 0;

    dev->sz = 0;

    size = read(dev->fd, dev->hdrBuf, MAX_RESP_LEN);

    if((size < 2) || (size > MAX_RESP_LEN)) {
        dev->hdrBuf[0] = 0;

        dev->buf[0] = 0;

        return -1;
    }

    dev->hdrBuf[size] = 0;

    if(dev->hdrBuf[0] != '#') {
        if(dev->hdrBuf[size - 1] == '\n')
            dev->hdrBuf[--size] = 0;

        dev->buf = dev->hdrBuf;

        dev->sz = size;

        return dev->sz;
    }

    strncpy(blockhdr, dev->hdrBuf, 16);

    len = blockhdr[1] - '0';

    if((len < 1) || (len > 9)) {
        blockhdr[31] = 0;

        return -1;
    }

    blockhdr[len + 2] = 0;

    size2 = atoi(blockhdr + 2);

    while(size < size2 && size < MAX_RESP_LEN) // we did not get all the data
    {
        ssize_t read_size = read(dev->fd, &dev->hdrBuf[size], MAX_RESP_LEN - size);
        if(read_size < 1) // timeout or error occurred
        {
            blockhdr[31] = 0;

            return -1;
        }

        size += read_size;
    }

    size--; // remove the last character

    if(size < size2) {
        blockhdr[31] = 0;

        return -1;
    }

    dev->buf = dev->hdrBuf + len + 2;

    dev->sz = size2;

    return dev->sz;
}
