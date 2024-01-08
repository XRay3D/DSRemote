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

#include "mainwindow.h"
// #include "interface.cpp"
// #include "mainwindowConstr.cpp"
// #include "saveData.cpp"
// #include "serialDecoder.cpp"
// #include "timerHandlers.cpp"

void UiMainWindow::openSettingsDialog() {
    UiSettingsWindow settings{this};
}

void UiMainWindow::openConnection() {
    int i, j, n, len;
    char str[4096] = {""}, devStr[256] = {""}, respStr[2048] = {""},
         *ptr;
    QSettings settings;
    QMessageBox msgBox;
    LanConnectThread lanCnThrd;
    if(device != nullptr)
        return;
    if(devParms.connected)
        return;
    strlcpy(str, settings.value("connection/type", "USB").toString().toLatin1().data(), 4096);
    if(!strcmp(str, "LAN"))
        devParms.connectionType = 1;
    else
        devParms.connectionType = 0;
    if(devParms.connectionType == 0) // USB
    {
        strlcpy(devStr,
            settings.value("connection/device", "/dev/usbtmc0").toString().toLatin1().data(),
            256);
        if(!strcmp(devStr, "")) {
            strlcpy(devStr, "/dev/usbtmc0", 256);
            settings.setValue("connection/device", QByteArray{devStr});
        }

        device = tmcOpenUsb(devStr);
        if(device == nullptr) {
            snprintf(str, 4096, "Can not open device %s", devStr);
            goto OC_OUT_ERROR;
        }
    }

    if(devParms.connectionType == 1) // LAN
    {
        strlcpy(devParms.hostName,
            settings.value("connection/hostname", "").toString().toLatin1().data(),
            128);
        if(strlen(devParms.hostName)) {
            strlcpy(devStr, devParms.hostName, 256);
        } else {
            strlcpy(devStr,
                settings.value("connection/ip", "192.168.1.100").toString().toLatin1().data(),
                256);
            if(!strcmp(devStr, "")) {
                snprintf(str, 4096, "No IP address or hostname set");
                goto OC_OUT_ERROR;
            }

            len = strlen(devStr);
            if(len < 7) {
                snprintf(str, 4096, "No IP address set");
                goto OC_OUT_ERROR;
            }

            int cf = 0;
            for(int i{}; i < len; i++) {
                if(devStr[i] == '.')
                    cf = 0;
                if(devStr[i] == '0') {
                    if(cf == 0) {
                        if((devStr[i + 1] != 0) && (devStr[i + 1] != '.')) {
                            for(j = i; j < len; j++)
                                devStr[j] = devStr[j + 1];
                            i--;
                            len--;
                        }
                    }
                } else {
                    if(devStr[i] != '.')
                        cf = 1;
                }
            }
        }

        statusLabel->setText("Trying to connect...");
        snprintf(str, 4096, "Trying to connect to %s", devStr);
        msgBox.setIcon(QMessageBox::NoIcon);
        msgBox.setText(str);
        msgBox.addButton("Abort", QMessageBox::RejectRole);
        lanCnThrd.setDeviceAddress(devStr);
        connect(&lanCnThrd, &QThread::finished, &msgBox, &QMessageBox::accept);
        lanCnThrd.start();
        if(msgBox.exec() != QDialog::Accepted) {
            statusLabel->setText("Connection aborted");
            lanCnThrd.terminate();
            lanCnThrd.wait(20000);
            snprintf(str, 4096, "Connection aborted");
            disconnect(&lanCnThrd, 0, 0, 0);
            goto OC_OUT_ERROR;
        }

        disconnect(&lanCnThrd, 0, 0, 0);
        device = lanCnThrd.getDevice();
        if(device == nullptr) {
            statusLabel->setText("Connection failed");
            snprintf(str, 4096, "Can not open connection to %s", devStr);
            goto OC_OUT_ERROR;
        }
    }

    if(tmcWrite("*IDN?") != 5)
    //  if(tmcWrite("*IDN?;:SYST:ERR?") != 16)  // This is a fix for the broken *IDN? command in older fw version
    {
        snprintf(str, 4096, "Can not write to device %s", devStr);
        goto OC_OUT_ERROR;
    }

    n = tmcRead();
    if(n < 0) {
        snprintf(str, 4096, "Can not read from device %s", devStr);
        goto OC_OUT_ERROR;
    }

    devParms.channelCnt = 0;
    devParms.bandwidth = 0;
    devParms.modelName[0] = 0;
    strlcpy(respStr, device->buf, 1024);
    ptr = strtok(respStr, ",");
    if(ptr == nullptr) {
        snprintf(str,
            1024,
            "Received an unknown identification string from device:\n\n%s\n ",
            device->buf);
        goto OC_OUT_ERROR;
    }

    if(strcmp(ptr, "RIGOL TECHNOLOGIES")) {
        snprintf(str,
            1024,
            "Received an unknown identification string from device:\n\n%s\n ",
            device->buf);
        goto OC_OUT_ERROR;
    }

    ptr = strtok(nullptr, ",");
    if(ptr == nullptr) {
        snprintf(str,
            1024,
            "Received an unknown identification string from device:\n\n%s\n ",
            device->buf);
        goto OC_OUT_ERROR;
    }

    getDeviceModel(ptr);
    if((!devParms.channelCnt) || (!devParms.bandwidth)) {
        snprintf(str,
            1024,
            "Received an unknown identification string from device:\n\n%s\n ",
            device->buf);
        goto OC_OUT_ERROR;
    }

    ptr = strtok(nullptr, ",");
    if(ptr == nullptr) {
        snprintf(str,
            1024,
            "Received an unknown identification string from device:\n\n%s\n ",
            device->buf);
        goto OC_OUT_ERROR;
    }

    strlcpy(devParms.serialNum, ptr, 128);
    ptr = strtok(nullptr, ",");
    if(ptr == nullptr) {
        snprintf(str,
            1024,
            "Received an unknown identification string from device:\n\n%s\n ",
            device->buf);
        goto OC_OUT_ERROR;
    }

    strlcpy(devParms.softwVers, ptr, 128);
    for(int i{};; i++) {
        if(devParms.softwVers[i] == 0)
            break;
        if(devParms.softwVers[i] == ';') {
            devParms.softwVers[i] = 0;
            break;
        }
    }

    if((devParms.modelSerie != 6) && (devParms.modelSerie != 1)) {
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("Unsupported device detected.");
        msgBox.setInformativeText("This software has not been tested with your device.\n"
                                  "It has been tested with the DS6000 and DS1000Z series only.\n"
                                  "If you continue, it's likely that the program will not work "
                                  "correctly at some points.\n"
                                  "\nDo you want to continue?\n");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        int ret = msgBox.exec();
        if(ret == QMessageBox::No) {
            statusLabel->setText("Disconnected");
            devParms.connected = 0;
            closeConnection();
            return;
        }
    }

    if(getDeviceSettings()) {
        strlcpy(str, "Can not read device settings", 4096);
        goto OC_OUT_ERROR;
    }

    if(devParms.timebasedelayenable)
        devParms.currentScreenSf = 100.0 / devParms.timebasedelayscale;
    else
        devParms.currentScreenSf = 100.0 / devParms.timebasescale;
    if((devParms.modelSerie == 1) || (devParms.modelSerie == 2))
        trig50pctButton->setEnabled(false);

    if(devParms.channelCnt < 4) {
        ch4Button->setEnabled(false);
        ch4Button->setVisible(false);
    }
    if(devParms.channelCnt < 3) {
        ch3Button->setEnabled(false);
        ch3Button->setVisible(false);
    }
    if(devParms.channelCnt < 2) {
        ch2Button->setEnabled(false);
        ch2Button->setVisible(false);
    }

    devParms.cmdCueIdxIn = 0;
    devParms.cmdCueIdxOut = 0;
    devParms.funcHasRecord = 0;
    devParms.fftBufsz = devParms.horDivisions * 50;
    if(devParms.kCfg != nullptr)
        free(devParms.kCfg);
    devParms.kCfg = kiss_fftr_alloc(devParms.fftBufsz * 2, 0, nullptr, nullptr);
    connect(adjDial, &Dial::valueChanged,
        this, &UiMainWindow::adjDialChanged);
    connect(trigAdjustDial, &Dial::valueChanged,
        this, &UiMainWindow::trigAdjustDialChanged);
    connect(horScaleDial, &Dial::valueChanged,
        this, &UiMainWindow::horScaleDialChanged);
    connect(horPosDial, &Dial::valueChanged,
        this, &UiMainWindow::horPosDialChanged);
    connect(vertOffsetDial, &Dial::valueChanged,
        this, &UiMainWindow::vertOffsetDialChanged);
    connect(vertScaleDial, &Dial::valueChanged,
        this, &UiMainWindow::vertScaleDialChanged);
    connect(navDial, &Dial::valueChanged,
        this, &UiMainWindow::navDialChanged);
    connect(ch1Button, &QPushButton::clicked,
        this, &UiMainWindow::ch1ButtonClicked);
    connect(ch2Button, &QPushButton::clicked,
        this, &UiMainWindow::ch2ButtonClicked);
    connect(ch3Button, &QPushButton::clicked,
        this, &UiMainWindow::ch3ButtonClicked);
    connect(ch4Button, &QPushButton::clicked,
        this, &UiMainWindow::ch4ButtonClicked);
    connect(chanMenuButton, &QPushButton::clicked,
        this, &UiMainWindow::chanMenu);
    connect(mathMenuButton, &QPushButton::clicked,
        this, &UiMainWindow::mathMenu);
    connect(waveForm, &SignalCurve::chan1Clicked,
        this, &UiMainWindow::ch1ButtonClicked);
    connect(waveForm, &SignalCurve::chan2Clicked,
        this, &UiMainWindow::ch2ButtonClicked);
    connect(waveForm, &SignalCurve::chan3Clicked,
        this, &UiMainWindow::ch3ButtonClicked);
    connect(waveForm, &SignalCurve::chan4Clicked,
        this, &UiMainWindow::ch4ButtonClicked);
    connect(clearButton, &QPushButton::clicked,
        this, &UiMainWindow::clearButtonClicked);
    connect(autoButton, &QPushButton::clicked,
        this, &UiMainWindow::autoButtonClicked);
    connect(runButton, &QPushButton::clicked,
        this, &UiMainWindow::runButtonClicked);
    connect(singleButton, &QPushButton::clicked,
        this, &UiMainWindow::singleButtonClicked);
    connect(horMenuButton, &QPushButton::clicked,
        this, &UiMainWindow::horMenuButtonClicked);
    connect(trigModeButton, &QPushButton::clicked,
        this, &UiMainWindow::trigModeButtonClicked);
    connect(trigMenuButton, &QPushButton::clicked,
        this, &UiMainWindow::trigMenuButtonClicked);
    connect(trigForceButton, &QPushButton::clicked,
        this, &UiMainWindow::trigForceButtonClicked);
    connect(trig50pctButton, &QPushButton::clicked,
        this, &UiMainWindow::trig50pctButtonClicked);
    connect(acqButton, &QPushButton::clicked,
        this, &UiMainWindow::acqButtonClicked);
    connect(cursButton, &QPushButton::clicked,
        this, &UiMainWindow::cursButtonClicked);
    connect(saveButton, &QPushButton::clicked,
        this, &UiMainWindow::saveButtonClicked);
    connect(dispButton, &QPushButton::clicked,
        this, &UiMainWindow::dispButtonClicked);
    connect(utilButton, &QPushButton::clicked,
        this, &UiMainWindow::utilButtonClicked);
    connect(helpButton, &QPushButton::clicked,
        this, &UiMainWindow::helpButtonClicked);
    connect(measureButton, &QPushButton::clicked,
        this, &UiMainWindow::measureButtonClicked);
    connect(selectChan1Act, &QAction::triggered,
        this, &UiMainWindow::ch1ButtonClicked);
    connect(selectChan2Act, &QAction::triggered,
        this, &UiMainWindow::ch2ButtonClicked);
    connect(selectChan3Act, &QAction::triggered,
        this, &UiMainWindow::ch3ButtonClicked);
    connect(selectChan4Act, &QAction::triggered,
        this, &UiMainWindow::ch4ButtonClicked);
    connect(toggleFftAct, &QAction::triggered,
        this, &UiMainWindow::toggleFft);
    connect(horPosDial, &Dial::customContextMenuRequested,
        this, &UiMainWindow::horPosDialClicked);
    connect(vertOffsetDial, &Dial::customContextMenuRequested,
        this, &UiMainWindow::vertOffsetDialClicked);
    connect(horScaleDial, &Dial::customContextMenuRequested,
        this, &UiMainWindow::horScaleDialClicked);
    connect(vertScaleDial, &Dial::customContextMenuRequested,
        this, &UiMainWindow::vertScaleDialClicked);
    connect(trigAdjustDial, &Dial::customContextMenuRequested,
        this, &UiMainWindow::trigAdjustDialClicked);
    connect(adjDial, &Dial::customContextMenuRequested,
        this, &UiMainWindow::adjustDialClicked);
    connect(playpauseButton, &QPushButton::clicked,
        this, &UiMainWindow::playpauseButtonClicked);
    connect(stopButton, &QPushButton::clicked,
        this, &UiMainWindow::stopButtonClicked);
    connect(recordButton, &QPushButton::clicked,
        this, &UiMainWindow::recordButtonClicked);
    snprintf(str,
        4096,
        PROGRAM_NAME " " PROGRAM_VERSION "   %s   %s   %s",
        devParms.serialNum,
        devParms.softwVers,
        devStr); //   sprintf(str, PROGRAM_NAME " " PROGRAM_VERSION "   %s   %s",
    //           devParms.softwvers, devStr);
    setWindowTitle(str);
    statusLabel->setText("Connected");
    scrnThread->setDevice(device);
    devParms.connected = 1;
    //  testTimer->start(2000);
    DPRwidget->setEnabled(true);
    devParms.screenupdatesOn = 1;
    scrnThread->hBusy = 0;
    scrnTimer->start(devParms.screenTimerIval);
    return;
OC_OUT_ERROR:

    statusLabel->setText("Disconnected");
    devParms.connected = 0;
    QMessageBox mesgbox;
    mesgbox.setIcon(QMessageBox::Critical);
    mesgbox.setText(str);
    mesgbox.exec();
    closeConnection();
}

void UiMainWindow::closeConnection() {
    DPRwidget->setEnabled(false);
    testTimer->stop();
    scrnTimer->stop();
    adjdialTimer->stop();
    devParms.connected = 0;
    if(scrnThread->wait(5000) == false) {
        scrnThread->terminate();
        scrnThread->wait(5000);
        pthread_mutex_trylock(&devParms.mutexx);
        pthread_mutex_unlock(&devParms.mutexx);
        scrnThread->hBusy = 0;
    }

    devParms.screenupdatesOn = 0;
    setWindowTitle(PROGRAM_NAME " " PROGRAM_VERSION);
    strlcpy(devParms.modelName, "-----", 128);
    adjDialFunc = ADJ_DIAL_FUNC_NONE;
    navDialFunc = NAV_DIAL_FUNC_NONE;
    disconnect(adjDial, &Dial::valueChanged,
        this, &UiMainWindow::adjDialChanged);
    disconnect(trigAdjustDial, &Dial::valueChanged,
        this, &UiMainWindow::trigAdjustDialChanged);
    disconnect(horScaleDial, &Dial::valueChanged,
        this, &UiMainWindow::horScaleDialChanged);
    disconnect(horPosDial, &Dial::valueChanged,
        this, &UiMainWindow::horPosDialChanged);
    disconnect(vertOffsetDial, &Dial::valueChanged,
        this, &UiMainWindow::vertOffsetDialChanged);
    disconnect(vertScaleDial, &Dial::valueChanged,
        this, &UiMainWindow::vertScaleDialChanged);
    disconnect(navDial, &Dial::valueChanged,
        this, &UiMainWindow::navDialChanged);
    disconnect(ch1Button, &QPushButton::clicked,
        this, &UiMainWindow::ch1ButtonClicked);
    disconnect(ch2Button, &QPushButton::clicked,
        this, &UiMainWindow::ch2ButtonClicked);
    disconnect(ch3Button, &QPushButton::clicked,
        this, &UiMainWindow::ch3ButtonClicked);
    disconnect(ch4Button, &QPushButton::clicked,
        this, &UiMainWindow::ch4ButtonClicked);
    disconnect(chanMenuButton, &QPushButton::clicked,
        this, &UiMainWindow::chanMenu);
    disconnect(mathMenuButton, &QPushButton::clicked,
        this, &UiMainWindow::mathMenu);
    disconnect(waveForm, &SignalCurve::chan1Clicked,
        this, &UiMainWindow::ch1ButtonClicked);
    disconnect(waveForm, &SignalCurve::chan2Clicked,
        this, &UiMainWindow::ch2ButtonClicked);
    disconnect(waveForm, &SignalCurve::chan3Clicked,
        this, &UiMainWindow::ch3ButtonClicked);
    disconnect(waveForm, &SignalCurve::chan4Clicked,
        this, &UiMainWindow::ch4ButtonClicked);
    disconnect(clearButton, &QPushButton::clicked,
        this, &UiMainWindow::clearButtonClicked);
    disconnect(autoButton, &QPushButton::clicked,
        this, &UiMainWindow::autoButtonClicked);
    disconnect(runButton, &QPushButton::clicked,
        this, &UiMainWindow::runButtonClicked);
    disconnect(singleButton, &QPushButton::clicked,
        this, &UiMainWindow::singleButtonClicked);
    disconnect(horMenuButton, &QPushButton::clicked,
        this, &UiMainWindow::horMenuButtonClicked);
    disconnect(trigModeButton, &QPushButton::clicked,
        this, &UiMainWindow::trigModeButtonClicked);
    disconnect(trigMenuButton, &QPushButton::clicked,
        this, &UiMainWindow::trigMenuButtonClicked);
    disconnect(trigForceButton, &QPushButton::clicked,
        this, &UiMainWindow::trigForceButtonClicked);
    disconnect(trig50pctButton, &QPushButton::clicked,
        this, &UiMainWindow::trig50pctButtonClicked);
    disconnect(acqButton, &QPushButton::clicked,
        this, &UiMainWindow::acqButtonClicked);
    disconnect(cursButton, &QPushButton::clicked,
        this, &UiMainWindow::cursButtonClicked);
    disconnect(saveButton, &QPushButton::clicked,
        this, &UiMainWindow::saveButtonClicked);
    disconnect(dispButton, &QPushButton::clicked,
        this, &UiMainWindow::dispButtonClicked);
    disconnect(utilButton, &QPushButton::clicked,
        this, &UiMainWindow::utilButtonClicked);
    disconnect(helpButton, &QPushButton::clicked,
        this, &UiMainWindow::helpButtonClicked);
    disconnect(measureButton, &QPushButton::clicked,
        this, &UiMainWindow::measureButtonClicked);
    disconnect(selectChan1Act, &QAction::triggered,
        this, &UiMainWindow::ch1ButtonClicked);
    disconnect(selectChan2Act, &QAction::triggered,
        this, &UiMainWindow::ch2ButtonClicked);
    disconnect(selectChan3Act, &QAction::triggered,
        this, &UiMainWindow::ch3ButtonClicked);
    disconnect(selectChan4Act, &QAction::triggered,
        this, &UiMainWindow::ch4ButtonClicked);
    disconnect(toggleFftAct, &QAction::triggered,
        this, &UiMainWindow::toggleFft);
    disconnect(horPosDial, &Dial::customContextMenuRequested,
        this, &UiMainWindow::horPosDialClicked);
    disconnect(vertOffsetDial, &Dial::customContextMenuRequested,
        this, &UiMainWindow::vertOffsetDialClicked);
    disconnect(horScaleDial, &Dial::customContextMenuRequested,
        this, &UiMainWindow::horScaleDialClicked);
    disconnect(vertScaleDial, &Dial::customContextMenuRequested,
        this, &UiMainWindow::vertScaleDialClicked);
    disconnect(trigAdjustDial, &Dial::customContextMenuRequested,
        this, &UiMainWindow::trigAdjustDialClicked);
    disconnect(adjDial, &Dial::customContextMenuRequested,
        this, &UiMainWindow::adjustDialClicked);
    disconnect(playpauseButton, &QPushButton::clicked,
        this, &UiMainWindow::playpauseButtonClicked);
    disconnect(stopButton, &QPushButton::clicked,
        this, &UiMainWindow::stopButtonClicked);
    disconnect(recordButton, &QPushButton::clicked,
        this, &UiMainWindow::recordButtonClicked);
    scrnThread->setDevice(nullptr);
    devParms.mathFft = 0;
    devParms.mathFftSplit = 0;
    waveForm->clear();
    tmcClose();
    device = nullptr;
    if(devParms.kCfg != nullptr) {
        free(devParms.kCfg);
        devParms.kCfg = nullptr;
    }

    statusLabel->setText("Disconnected");
    printf("Disconnected from device\n");
}

void UiMainWindow::closeEvent(QCloseEvent* clEvent) {
    devParms.connected = 0;
    testTimer->stop();
    scrnTimer->stop();
    adjdialTimer->stop();
    scrnThread->wait(5000);
    scrnThread->terminate();
    scrnThread->wait(5000);
    devParms.screenupdatesOn = 0;
    scrnThread->setDevice(nullptr);
    tmcClose();
    device = nullptr;
    clEvent->accept();
}

int UiMainWindow::getDeviceSettings(int delay) {
    char str[4096]{};
    statusLabel->setText("Reading instrument settings...");
    ReadSettingsThread rdSetThrd;
    rdSetThrd.setDevice(device);
    rdSetThrd.setDelay(delay);
    rdSetThrd.setDevparmPtr(&devParms);
    rdSetThrd.start();
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::NoIcon);
    msgBox.setText("Reading instrument settings...");
    msgBox.addButton("Abort", QMessageBox::RejectRole);
    connect(&rdSetThrd, &QThread::finished, &msgBox, &QMessageBox::accept);
    if(msgBox.exec() != QDialog::Accepted) {
        statusLabel->setText("Reading settings aborted");
        rdSetThrd.terminate();
        rdSetThrd.wait(20000);
        snprintf(str, 4096, "Reading settings aborted");
        disconnect(&rdSetThrd, 0, 0, 0);
        return -1;
    }

    disconnect(&rdSetThrd, 0, 0, 0);
    if(rdSetThrd.getErrorNum() != 0) {
        statusLabel->setText("Error while reading settings");
        rdSetThrd.getErrorStr(str, 4096);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(str);
        msgBox.exec();
        strlcpy(str, "Can not read settings from device", 4096);
        return -1;
    }

    for(int chn{}; chn < /*devParms.channelCnt*/ 4; chn++) // FIXME
        if(devParms.chan[chn].Display == 1)
            chButtons.at(chn)->setStyleSheet(std::array{
                "background: #FFFF33; color: black;",
                "background: #33FFFF; color: black;",
                "background: #FF33FF; color: black;",
                "background: #0080FF; color: black;"}[chn]);

        else
            chButtons.at(chn)->setStyleSheet(defStylesh);

    if(devParms.triggersweep == 0) {
        trigModeAutoLed->setValue(true);
        trigModeNormLed->setValue(false);
        trigModeSingLed->setValue(false);
    } else if(devParms.triggersweep == 1) {
        trigModeAutoLed->setValue(false);
        trigModeNormLed->setValue(true);
        trigModeSingLed->setValue(false);
    } else if(devParms.triggersweep == 2) {
        trigModeAutoLed->setValue(false);
        trigModeNormLed->setValue(false);
        trigModeSingLed->setValue(true);
    }

    updateLabels();
    return 0;
}

int UiMainWindow::parsePreamble(char* str, int sz, struct WaveformPreamble* wfp, int chn) {
    char* ptr;
    if(sz < 19)
        return -1;
    ptr = strtok(str, ",");
    if(ptr == nullptr)
        return -1;
    wfp->format = atoi(ptr);
    ptr = strtok(nullptr, ",");
    if(ptr == nullptr)
        return -1;
    wfp->type = atoi(ptr);
    ptr = strtok(nullptr, ",");
    if(ptr == nullptr)
        return -1;
    wfp->points = atoi(ptr);
    ptr = strtok(nullptr, ",");
    if(ptr == nullptr)
        return -1;
    wfp->count = atoi(ptr);
    ptr = strtok(nullptr, ",");
    if(ptr == nullptr)
        return -1;
    wfp->xIncrement[chn] = atof(ptr);
    ptr = strtok(nullptr, ",");
    if(ptr == nullptr)
        return -1;
    wfp->xOrigin[chn] = atof(ptr);
    ptr = strtok(nullptr, ",");
    if(ptr == nullptr)
        return -1;
    wfp->xReference[chn] = atof(ptr);
    ptr = strtok(nullptr, ",");
    if(ptr == nullptr)
        return -1;
    wfp->yIncrement[chn] = atof(ptr);
    ptr = strtok(nullptr, ",");
    if(ptr == nullptr)
        return -1;
    wfp->yOrigin[chn] = atof(ptr);
    ptr = strtok(nullptr, ",");
    if(ptr == nullptr)
        return -1;
    wfp->yReference[chn] = atoi(ptr);
    ptr = strtok(nullptr, ",");
    if(ptr != nullptr)
        return -1;
    return 0;
}

int UiMainWindow::getMetricFactor(double value) {
    int suffix = 0;
    if(value < 0)
        value *= -1;
    if(value >= 1e12 && value < 1e15)
        suffix = 12;
    else if(value >= 1e9 && value < 1e12)
        suffix = 9;
    else if(value >= 1e6 && value < 1e9)
        suffix = 6;
    else if(value >= 1e3 && value < 1e6)
        suffix = 3;
    else if(value >= 1e-3 && value < 1)
        suffix = -3;
    else if(value >= 1e-6 && value < 1e-3)
        suffix = -6;
    else if(value >= 1e-9 && value < 1e-6)
        suffix = -9;
    else if(value >= 1e-12 && value < 1e-9)
        suffix = -12;
    return suffix;
}

double UiMainWindow::getStepSizeDivideBy1000(double val) {
    int exp = 0;
    if(val < 1e-9)
        return 1e-9;
    while(val < 1) {
        val *= 10;
        exp--;
    }

    while(val >= 10) {
        val /= 10;
        exp++;
    }

    val = exp10(exp - 2);
    if((val < 1e-13) && (val > -1e-13))
        return 0;
    return val;
}

void UiMainWindow::getDeviceModel(const char* str) {
    devParms.channelCnt = 0;
    devParms.bandwidth = 0;
    devParms.horDivisions = 14;
    devParms.vertDivisions = 8;
    if(!strcmp(str, "DS6104")) {
        devParms.channelCnt = 4;
        devParms.bandwidth = 1000;
        devParms.modelSerie = 6;
    } else if(!strcmp(str, "DS6064")) {
        devParms.channelCnt = 4;
        devParms.bandwidth = 600;
        devParms.modelSerie = 6;
    } else if(!strcmp(str, "DS6102")) {
        devParms.channelCnt = 2;
        devParms.bandwidth = 1000;
        devParms.modelSerie = 6;
    } else if(!strcmp(str, "DS6062")) {
        devParms.channelCnt = 2;
        devParms.bandwidth = 600;
        devParms.modelSerie = 6;
    } else if(!strcmp(str, "DS4012")) {
        devParms.channelCnt = 2;
        devParms.bandwidth = 100;
        devParms.modelSerie = 4;
    } else if(!strcmp(str, "DS4014")) {
        devParms.channelCnt = 4;
        devParms.bandwidth = 100;
        devParms.modelSerie = 4;
    } else if(!strcmp(str, "DS4022")) {
        devParms.channelCnt = 2;
        devParms.bandwidth = 200;
        devParms.modelSerie = 4;
    } else if(!strcmp(str, "DS4024")) {
        devParms.channelCnt = 4;
        devParms.bandwidth = 200;
        devParms.modelSerie = 4;
    } else if(!strcmp(str, "DS4032")) {
        devParms.channelCnt = 2;
        devParms.bandwidth = 350;
        devParms.modelSerie = 4;
    } else if(!strcmp(str, "DS4034")) {
        devParms.channelCnt = 4;
        devParms.bandwidth = 350;
        devParms.modelSerie = 4;
    } else if(!strcmp(str, "DS4052")) {
        devParms.channelCnt = 2;
        devParms.bandwidth = 500;
        devParms.modelSerie = 4;
    } else if(!strcmp(str, "DS4054")) {
        devParms.channelCnt = 4;
        devParms.bandwidth = 500;
        devParms.modelSerie = 4;
    } else if(!strcmp(str, "MSO4012")) {
        devParms.channelCnt = 2;
        devParms.bandwidth = 100;
        devParms.modelSerie = 4;
    } else if(!strcmp(str, "MSO4024")) {
        devParms.channelCnt = 4;
        devParms.bandwidth = 200;
        devParms.modelSerie = 4;
    } else if(!strcmp(str, "DS2072A")) {
        devParms.channelCnt = 2;
        devParms.bandwidth = 70;
        devParms.modelSerie = 2;
    } else if(!strcmp(str, "DS2072A-S")) {
        devParms.channelCnt = 2;
        devParms.bandwidth = 70;
        devParms.modelSerie = 2;
    } else if(!strcmp(str, "DS2102")) {
        devParms.channelCnt = 2;
        devParms.bandwidth = 100;
        devParms.modelSerie = 2;
    } else if(!strcmp(str, "DS2102A")) {
        devParms.channelCnt = 2;
        devParms.bandwidth = 100;
        devParms.modelSerie = 2;
    } else if(!strcmp(str, "DS2102A-S")) {
        devParms.channelCnt = 2;
        devParms.bandwidth = 100;
        devParms.modelSerie = 2;
    } else if(!strcmp(str, "DS2202")) {
        devParms.channelCnt = 2;
        devParms.bandwidth = 200;
        devParms.modelSerie = 2;
    } else if(!strcmp(str, "DS2202A")) {
        devParms.channelCnt = 2;
        devParms.bandwidth = 200;
        devParms.modelSerie = 2;
    } else if(!strcmp(str, "DS2202A-S")) {
        devParms.channelCnt = 2;
        devParms.bandwidth = 200;
        devParms.modelSerie = 2;
    } else if(!strcmp(str, "DS2302")) {
        devParms.channelCnt = 2;
        devParms.bandwidth = 300;
        devParms.modelSerie = 2;
    } else if(!strcmp(str, "DS2302A")) {
        devParms.channelCnt = 2;
        devParms.bandwidth = 300;
        devParms.modelSerie = 2;
    } else if(!strcmp(str, "DS2302A-S")) {
        devParms.channelCnt = 2;
        devParms.bandwidth = 300;
        devParms.modelSerie = 2;
    } else if(!strcmp(str, "DS1054Z")) {
        devParms.channelCnt = 4;
        devParms.bandwidth = 50;
        devParms.modelSerie = 1;
    } else if(!strcmp(str, "DS1074Z")) {
        devParms.channelCnt = 4;
        devParms.bandwidth = 70;
        devParms.modelSerie = 1;
    } else if(!strcmp(str, "DS1074Z-S")) {
        devParms.channelCnt = 4;
        devParms.bandwidth = 70;
        devParms.modelSerie = 1;
    } else if(!strcmp(str, "DS1074Z Plus")) {
        devParms.channelCnt = 4;
        devParms.bandwidth = 70;
        devParms.modelSerie = 1;
    } else if(!strcmp(str, "DS1074Z-S Plus")) {
        devParms.channelCnt = 4;
        devParms.bandwidth = 70;
        devParms.modelSerie = 1;
    } else if(!strcmp(str, "DS1104Z")) {
        devParms.channelCnt = 4;
        devParms.bandwidth = 100;
        devParms.modelSerie = 1;
    } else if(!strcmp(str, "DS1104Z-S")) {
        devParms.channelCnt = 4;
        devParms.bandwidth = 100;
        devParms.modelSerie = 1;
    } else if(!strcmp(str, "DS1104Z Plus")) { // FIXME
        devParms.channelCnt = 4;
        devParms.bandwidth = 100;
        devParms.modelSerie = 1;
    } else if(!strcmp(str, "DS1104Z-S Plus")) {
        devParms.channelCnt = 4;
        devParms.bandwidth = 100;
        devParms.modelSerie = 1;
    } else if(!strcmp(str, "MSO1074Z")) {
        devParms.channelCnt = 4;
        devParms.bandwidth = 70;
        devParms.modelSerie = 1;
    } else if(!strcmp(str, "MSO1104Z")) {
        devParms.channelCnt = 4;
        devParms.bandwidth = 100;
        devParms.modelSerie = 1;
    } else if(!strcmp(str, "MSO1074Z-S")) {
        devParms.channelCnt = 4;
        devParms.bandwidth = 70;
        devParms.modelSerie = 1;
    } else if(!strcmp(str, "MSO1104Z-S")) {
        devParms.channelCnt = 4;
        devParms.bandwidth = 100;
        devParms.modelSerie = 1;
    } else if(!strcmp(str, "DS1202Z-E")) {
        devParms.channelCnt = 2;
        devParms.bandwidth = 200;
        devParms.modelSerie = 1;
    } else if(!strcmp(str, "DS1102Z-E")) {
        devParms.channelCnt = 2;
        devParms.bandwidth = 100;
        devParms.modelSerie = 1;
    }

    if(devParms.modelSerie == 1) {
        if(devParms.useExtraVertDivisions)
            devParms.vertDivisions = 10;
        devParms.horDivisions = 12;
    }

    if(devParms.channelCnt && devParms.bandwidth && devParms.modelSerie)
        strlcpy(devParms.modelName, str, 128);
}

void UiMainWindow::formerPage() {
    char str[512];
    if(device == nullptr)
        return;
    if(!devParms.connected)
        return;
    if(devParms.activechannel < 0)
        return;
    if(devParms.timebasedelayenable) {
        const auto timebasedelayoffset = (((devParms.horDivisions / 2) * devParms.timebasescale)
            - devParms.timebaseoffset
            - ((devParms.horDivisions / 2) * devParms.timebasedelayscale));
        if(devParms.timebasedelayoffset <= -timebasedelayoffset)
            return;
        devParms.timebasedelayoffset -= devParms.timebasedelayscale * devParms.horDivisions;
        if(devParms.timebasedelayoffset <= -timebasedelayoffset)
            devParms.timebasedelayoffset = -timebasedelayoffset;
        strlcpy(str, "Delayed timebase position: ", 512);
        convertToMetricSuffix(str + strlen(str),
            devParms.timebasedelayoffset,
            2,
            512 - strlen(str));
        strlcat(str, "s", 512);
        statusLabel->setText(str);
        horPosDialTimer->start(TMC_DIAL_TIMER_DELAY);
    } else {
        devParms.timebaseoffset -= devParms.timebasescale * devParms.horDivisions;
        if(devParms.acquirememdepth > 10) {
            if(devParms.timebaseoffset
                <= -(((double)devParms.acquirememdepth / devParms.samplerate) / 2))
                devParms.timebaseoffset = -(
                    ((double)devParms.acquirememdepth / devParms.samplerate) / 2);
        } else {
            if(devParms.timebaseoffset <= -((devParms.horDivisions / 2) * devParms.timebasescale))
                devParms.timebaseoffset = -((devParms.horDivisions / 2) * devParms.timebasescale);
        }

        strlcpy(str, "Horizontal position: ", 512);
        convertToMetricSuffix(str + strlen(str), devParms.timebaseoffset, 2, 512 - strlen(str));
        strlcat(str, "s", 512);
        statusLabel->setText(str);
        horPosDialTimer->start(TMC_DIAL_TIMER_DELAY);
    }

    waveForm->update();
}

void UiMainWindow::nextPage() {
    char str[512];
    if(device == nullptr)
        return;
    if(!devParms.connected)
        return;
    if(devParms.activechannel < 0)
        return;
    if(devParms.timebasedelayenable) {
        const auto timebasedelayoffset = (((devParms.horDivisions / 2) * devParms.timebasescale)
            + devParms.timebaseoffset
            - ((devParms.horDivisions / 2) * devParms.timebasedelayscale));
        if(devParms.timebasedelayoffset >= timebasedelayoffset)
            return;
        devParms.timebasedelayoffset += devParms.timebasedelayscale * devParms.horDivisions;
        if(devParms.timebasedelayoffset >= timebasedelayoffset)
            devParms.timebasedelayoffset = timebasedelayoffset;
        strlcpy(str, "Delayed timebase position: ", 512);
        convertToMetricSuffix(str + strlen(str),
            devParms.timebasedelayoffset,
            2,
            512 - strlen(str));
        strlcat(str, "s", 512);
        statusLabel->setText(str);
        horPosDialTimer->start(TMC_DIAL_TIMER_DELAY);
    } else {
        devParms.timebaseoffset += devParms.timebasescale * devParms.horDivisions;
        if(devParms.acquirememdepth > 10) {
            if(devParms.timebaseoffset >= (((double)devParms.acquirememdepth / devParms.samplerate) / 2))
                devParms.timebaseoffset = (((double)devParms.acquirememdepth / devParms.samplerate) / 2);
        } else {
            if(devParms.timebaseoffset >= (devParms.horDivisions / 2) * devParms.timebasescale)
                devParms.timebaseoffset = (devParms.horDivisions / 2) * devParms.timebasescale;
        }

        strlcpy(str, "Horizontal position: ", 512);
        convertToMetricSuffix(str + strlen(str), devParms.timebaseoffset, 2, 512 - strlen(str));
        strlcat(str, "s", 512);
        statusLabel->setText(str);
        horPosDialTimer->start(TMC_DIAL_TIMER_DELAY);
    }

    waveForm->update();
}

void UiMainWindow::shiftPageLeft() {
    char str[512];
    if(device == nullptr)
        return;
    if(!devParms.connected)
        return;
    if(devParms.mathFft && devParms.mathFftSplit) {
        devParms.mathFftHcenter -= devParms.mathFftHscale;
        if(devParms.mathFftHcenter <= 0.0)
            devParms.mathFftHcenter = 0.0;
        if(devParms.modelSerie != 1)
            snprintf(str, 512, ":CALC:FFT:HCEN %e", devParms.mathFftHcenter);
        else
            snprintf(str, 512, ":MATH:FFT:HCEN %e", devParms.mathFftHcenter);
        setCueCmd(str);
        strlcpy(str, "FFT center: ", 512);
        convertToMetricSuffix(str + strlen(str), devParms.mathFftHcenter, 0, 512 - strlen(str));
        strlcat(str, "Hz", 512);
        statusLabel->setText(str);
        waveForm->update();
        return;
    }

    if(devParms.activechannel < 0)
        return;
    if(devParms.timebasedelayenable) {
        const auto timebasedelayoffset = (((devParms.horDivisions / 2) * devParms.timebasescale)
            - devParms.timebaseoffset
            - ((devParms.horDivisions / 2) * devParms.timebasedelayscale));
        if(devParms.timebasedelayoffset <= -timebasedelayoffset)
            return;
        devParms.timebasedelayoffset -= devParms.timebasedelayscale;
        if(devParms.timebasedelayoffset <= -timebasedelayoffset)
            devParms.timebasedelayoffset = -timebasedelayoffset;
        strlcpy(str, "Delayed timebase position: ", 512);
        convertToMetricSuffix(str + strlen(str),
            devParms.timebasedelayoffset,
            2,
            512 - strlen(str));
        strlcat(str, "s", 512);
        statusLabel->setText(str);
        horPosDialTimer->start(TMC_DIAL_TIMER_DELAY);
    } else {
        devParms.timebaseoffset -= devParms.timebasescale;
        if(devParms.acquirememdepth > 10) {
            if(devParms.timebaseoffset
                <= -(((double)devParms.acquirememdepth / devParms.samplerate) / 2))
                devParms.timebaseoffset = -(
                    ((double)devParms.acquirememdepth / devParms.samplerate) / 2);
        } else {
            if(devParms.timebaseoffset <= -((devParms.horDivisions / 2) * devParms.timebasescale))
                devParms.timebaseoffset = -((devParms.horDivisions / 2) * devParms.timebasescale);
        }

        strlcpy(str, "Horizontal position: ", 512);
        convertToMetricSuffix(str + strlen(str), devParms.timebaseoffset, 2, 512 - strlen(str));
        strlcat(str, "s", 512);
        statusLabel->setText(str);
        horPosDialTimer->start(TMC_DIAL_TIMER_DELAY);
    }

    waveForm->update();
}

void UiMainWindow::shiftPageRight() {
    char str[512];
    if(device == nullptr)
        return;
    if(!devParms.connected)
        return;
    if(devParms.mathFft && devParms.mathFftSplit) {
        devParms.mathFftHcenter += devParms.mathFftHscale;
        if(devParms.mathFftHcenter >= (devParms.currentScreenSf * 0.4))
            devParms.mathFftHcenter = devParms.currentScreenSf * 0.4;
        if(devParms.modelSerie != 1)
            snprintf(str, 512, ":CALC:FFT:HCEN %e", devParms.mathFftHcenter);
        else
            snprintf(str, 512, ":MATH:FFT:HCEN %e", devParms.mathFftHcenter);
        setCueCmd(str);
        strlcpy(str, "FFT center: ", 512);
        convertToMetricSuffix(str + strlen(str), devParms.mathFftHcenter, 0, 512 - strlen(str));
        strlcat(str, "Hz", 512);
        statusLabel->setText(str);
        waveForm->update();
        return;
    }

    if(devParms.activechannel < 0)
        return;
    if(devParms.timebasedelayenable) {
        const auto timebasedelayoffset = (((devParms.horDivisions / 2) * devParms.timebasescale)
            + devParms.timebaseoffset
            - ((devParms.horDivisions / 2) * devParms.timebasedelayscale));
        if(devParms.timebasedelayoffset >= timebasedelayoffset)
            return;
        devParms.timebasedelayoffset += devParms.timebasedelayscale;
        if(devParms.timebasedelayoffset >= timebasedelayoffset)
            devParms.timebasedelayoffset = timebasedelayoffset;
        strlcpy(str, "Delayed timebase position: ", 512);
        convertToMetricSuffix(str + strlen(str),
            devParms.timebasedelayoffset,
            2,
            512 - strlen(str));
        strlcat(str, "s", 512);
        statusLabel->setText(str);
        horPosDialTimer->start(TMC_DIAL_TIMER_DELAY);
    } else {
        devParms.timebaseoffset += devParms.timebasescale;
        if(devParms.acquirememdepth > 10) {
            if(devParms.timebaseoffset >= (((double)devParms.acquirememdepth / devParms.samplerate) / 2))
                devParms.timebaseoffset = (((double)devParms.acquirememdepth / devParms.samplerate) / 2);
        } else {
            if(devParms.timebaseoffset >= (devParms.horDivisions / 2) * devParms.timebasescale)
                devParms.timebaseoffset = (devParms.horDivisions / 2) * devParms.timebasescale;
        }

        strlcpy(str, "Horizontal position: ", 512);
        convertToMetricSuffix(str + strlen(str), devParms.timebaseoffset, 2, 512 - strlen(str));
        strlcat(str, "s", 512);
        statusLabel->setText(str);
        horPosDialTimer->start(TMC_DIAL_TIMER_DELAY);
    }

    waveForm->update();
}

void UiMainWindow::centerTrigger() {
    char str[512];
    if(device == nullptr)
        return;
    if(!devParms.connected)
        return;
    if(devParms.mathFft && devParms.mathFftSplit) {
        devParms.mathFftHcenter = 0;
        if(devParms.modelSerie != 1)
            snprintf(str, 512, ":CALC:FFT:HCEN %e", devParms.mathFftHcenter);
        else
            snprintf(str, 512, ":MATH:FFT:HCEN %e", devParms.mathFftHcenter);
        setCueCmd(str);
        strlcpy(str, "FFT center: ", 512);
        convertToMetricSuffix(str + strlen(str), devParms.mathFftHcenter, 0, 512 - strlen(str));
        strlcat(str, "Hz", 512);
        statusLabel->setText(str);
        waveForm->update();
        return;
    }

    if(devParms.activechannel < 0)
        return;
    if(devParms.timebasedelayenable) {
        devParms.timebasedelayoffset = 0;
        strlcpy(str, "Delayed timebase position: ", 512);
        convertToMetricSuffix(str + strlen(str),
            devParms.timebasedelayoffset,
            2,
            512 - strlen(str));
        strlcat(str, "s", 512);
        statusLabel->setText(str);
        horPosDialTimer->start(TMC_DIAL_TIMER_DELAY);
    } else {
        devParms.timebaseoffset = 0;
        strlcpy(str, "Horizontal position: ", 512);
        convertToMetricSuffix(str + strlen(str), devParms.timebaseoffset, 2, 512 - strlen(str));
        strlcat(str, "s", 512);
        statusLabel->setText(str);
        horPosDialTimer->start(TMC_DIAL_TIMER_DELAY);
    }

    waveForm->update();
}

void UiMainWindow::zoomIn() {
    char str[512];
    if(device == nullptr)
        return;
    if(!devParms.connected)
        return;
    if(devParms.activechannel < 0)
        return;
    if(devParms.mathFft && devParms.mathFftSplit) {
        if(!dblcmp(devParms.mathFftHscale, devParms.currentScreenSf / 200.0))
            return;
        if(!dblcmp(devParms.mathFftHscale, devParms.currentScreenSf / 20.0))
            devParms.mathFftHscale = devParms.currentScreenSf / 40.0;
        else if(!dblcmp(devParms.mathFftHscale, devParms.currentScreenSf / 40.0))
            devParms.mathFftHscale = devParms.currentScreenSf / 100.0;
        else
            devParms.mathFftHscale = devParms.currentScreenSf / 200.0;
        if(devParms.modelSerie != 1)
            snprintf(str, 512, ":CALC:FFT:HSP %e", devParms.mathFftHscale);
        else
            snprintf(str, 512, ":MATH:FFT:HSC %e", devParms.mathFftHscale);
        setCueCmd(str);
        strlcpy(str, "FFT scale: ", 512);
        convertToMetricSuffix(str + strlen(str), devParms.mathFftHscale, 0, 512 - strlen(str));
        strlcat(str, "Hz/Div", 512);
        statusLabel->setText(str);
        waveForm->update();
        return;
    }

    if(devParms.activechannel < 0)
        return;
    if(devParms.timebasedelayenable) {
        if(devParms.modelSerie == 1) {
            if(devParms.timebasescale <= 5.001e-9) {
                devParms.timebasescale = 5e-9;
                return;
            }
        } else if(devParms.bandwidth == 1000) {
            if(devParms.timebasedelayscale <= 5.001e-10) {
                devParms.timebasedelayscale = 5e-10;
                return;
            }
        } else if(devParms.timebasedelayscale <= 1.001e-9) {
            devParms.timebasedelayscale = 1e-9;
            return;
        }

        devParms.timebasedelayscale = roundDownStep125(devParms.timebasedelayscale, nullptr);
        devParms.currentScreenSf = 100.0 / devParms.timebasedelayscale;
        strlcpy(str, "Delayed timebase: ", 512);
        convertToMetricSuffix(str + strlen(str),
            devParms.timebasedelayscale,
            2,
            512 - strlen(str));
        strlcat(str, "s", 512);
        statusLabel->setText(str);
        snprintf(str, 512, ":TIM:DEL:SCAL %e", devParms.timebasedelayscale);
        setCueCmd(str);
    } else {
        if(devParms.modelSerie == 1) {
            if(devParms.timebasescale <= 5.001e-9) {
                devParms.timebasescale = 5e-9;
                return;
            }
        } else if(devParms.bandwidth == 1000) {
            if(devParms.timebasescale <= 5.001e-10) {
                devParms.timebasescale = 5e-10;
                return;
            }
        } else if(devParms.timebasescale <= 1.001e-9) {
            devParms.timebasescale = 1e-9;
            return;
        }

        devParms.timebasescale = roundDownStep125(devParms.timebasescale, nullptr);
        devParms.currentScreenSf = 100.0 / devParms.timebasescale;
        strlcpy(str, "Timebase: ", 512);
        convertToMetricSuffix(str + strlen(str), devParms.timebasescale, 2, 512 - strlen(str));
        strlcat(str, "s", 512);
        statusLabel->setText(str);
        snprintf(str, 512, ":TIM:SCAL %e", devParms.timebasescale);
        setCueCmd(str);
    }

    waveForm->update();
}

void UiMainWindow::zoomOut() {
    char str[512];
    if(device == nullptr)
        return;
    if(!devParms.connected)
        return;
    if(devParms.activechannel < 0)
        return;
    if(devParms.mathFft && devParms.mathFftSplit) {
        if(!dblcmp(devParms.mathFftHscale, devParms.currentScreenSf / 20.0))
            return;
        if(!dblcmp(devParms.mathFftHscale, devParms.currentScreenSf / 200.0))
            devParms.mathFftHscale = devParms.currentScreenSf / 100.0;
        else if(!dblcmp(devParms.mathFftHscale, devParms.currentScreenSf / 100.0))
            devParms.mathFftHscale = devParms.currentScreenSf / 40.0;
        else
            devParms.mathFftHscale = devParms.currentScreenSf / 20.0;
        if(devParms.modelSerie != 1)
            snprintf(str, 512, ":CALC:FFT:HSP %e", devParms.mathFftHscale);
        else
            snprintf(str, 512, ":MATH:FFT:HSC %e", devParms.mathFftHscale);
        setCueCmd(str);
        strlcpy(str, "FFT scale: ", 512);
        convertToMetricSuffix(str + strlen(str), devParms.mathFftHscale, 0, 512 - strlen(str));
        strlcat(str, "Hz/Div", 512);
        statusLabel->setText(str);
        waveForm->update();
        return;
    }

    if(devParms.timebasedelayenable) {
        if(devParms.timebasedelayscale >= devParms.timebasescale / 2) {
            devParms.timebasedelayscale = devParms.timebasescale / 2;
            return;
        }

        if(devParms.timebasedelayscale >= 0.1) {
            devParms.timebasedelayscale = 0.1;
            return;
        }

        devParms.timebasedelayscale = roundUpStep125(devParms.timebasedelayscale, nullptr);
        devParms.currentScreenSf = 100.0 / devParms.timebasedelayscale;
        strlcpy(str, "Delayed timebase: ", 512);
        convertToMetricSuffix(str + strlen(str),
            devParms.timebasedelayscale,
            2,
            512 - strlen(str));
        strlcat(str, "s", 512);
        statusLabel->setText(str);
        snprintf(str, 512, ":TIM:DEL:SCAL %e", devParms.timebasedelayscale);
        setCueCmd(str);
        if(devParms.timebasedelayscale > 0.1000001)
            devParms.funcWrecEnable = 0;
    } else {
        if(devParms.timebasescale >= 10) {
            devParms.timebasescale = 10;
            return;
        }

        devParms.timebasescale = roundUpStep125(devParms.timebasescale, nullptr);
        devParms.currentScreenSf = 100.0 / devParms.timebasescale;
        strlcpy(str, "Timebase: ", 512);
        convertToMetricSuffix(str + strlen(str), devParms.timebasescale, 2, 512 - strlen(str));
        strlcat(str, "s", 512);
        statusLabel->setText(str);
        snprintf(str, 512, ":TIM:SCAL %e", devParms.timebasescale);
        setCueCmd(str);
        if(devParms.timebasescale > 0.1000001)
            devParms.funcWrecEnable = 0;
    }

    waveForm->update();
}

void UiMainWindow::chanScalePlus() {
    int chn;
    double val, ltmp;
    char str[512];
    if(device == nullptr)
        return;
    if(!devParms.connected)
        return;
    if(devParms.activechannel < 0)
        return;
    if(devParms.mathFft && devParms.mathFftSplit) {
        devParms.fftVScale = roundUpStep125(devParms.fftVScale, nullptr);
        if(devParms.mathFftUnit == 0) {
            if(devParms.fftVScale > 50.0)
                devParms.fftVScale = 50.0;
        } else {
            if(devParms.fftVScale > 20.0)
                devParms.fftVScale = 20.0;
        }

        if(devParms.fftVOffset > (devParms.fftVScale * 4.0))
            devParms.fftVOffset = (devParms.fftVScale * 4.0);
        if(devParms.fftVOffset < (devParms.fftVScale * -4.0))
            devParms.fftVOffset = (devParms.fftVScale * -4.0);
        if(devParms.modelSerie != 1) {
            if(devParms.mathFftUnit == 1) {
                snprintf(str, 512, ":CALC:FFT:VSC %e", devParms.fftVScale);
                setCueCmd(str);
            } else {
                snprintf(str,
                    512,
                    ":CALC:FFT:VSC %e",
                    devParms.fftVScale / devParms.chan[devParms.mathFftSrc].scale);
                setCueCmd(str);
            }
        } else {
            snprintf(str, 512, ":MATH:SCAL %e", devParms.fftVScale);
            setCueCmd(str);
        }

        if(devParms.mathFftUnit == 0) {
            strlcpy(str, "FFT scale: ", 512);
            convertToMetricSuffix(str + strlen(str), devParms.fftVScale, 1, 512 - strlen(str));
            strlcat(str, "V/Div", 512);
        } else {
            snprintf(str, 512, "FFT scale: %.1fdB/Div", devParms.fftVScale);
        }

        statusLabel->setText(str);
        waveForm->update();
        return;
    }

    chn = devParms.activechannel;
    if(devParms.chan[chn].scale >= 20) {
        devParms.chan[chn].scale = 20;
        return;
    }

    ltmp = devParms.chan[chn].scale;
    val = roundUpStep125(devParms.chan[chn].scale, nullptr);
    if(devParms.chan[chn].vernier) {
        val /= 100;
        devParms.chan[chn].scale += val;
    } else {
        devParms.chan[chn].scale = val;
    }

    ltmp /= val;
    devParms.chan[chn].offset /= ltmp;
    snprintf(str, 512, "Channel %i scale: ", chn + 1);
    convertToMetricSuffix(str + strlen(str), devParms.chan[chn].scale, 2, 512 - strlen(str));
    strlcat(str, "V", 512);
    statusLabel->setText(str);
    snprintf(str, 512, ":CHAN%i:SCAL %e", chn + 1, devParms.chan[chn].scale);
    setCueCmd(str);
    waveForm->update();
}

void UiMainWindow::chanScalePlusAll() {
    int chn;
    double val, ltmp;
    char str[512];
    if(device == nullptr)
        return;
    if(!devParms.connected)
        return;
    if(devParms.activechannel < 0)
        return;
    if(devParms.mathFft && devParms.mathFftSplit)
        return;
    for(chn = 0; chn < devParms.channelCnt; chn++) {
        if(!devParms.chan[chn].Display)
            continue;
        if(devParms.chan[chn].scale >= 20) {
            devParms.chan[chn].scale = 20;
            return;
        }

        ltmp = devParms.chan[chn].scale;
        val = roundUpStep125(devParms.chan[chn].scale, nullptr);
        if(devParms.chan[chn].vernier) {
            val /= 100;
            devParms.chan[chn].scale += val;
        } else {
            devParms.chan[chn].scale = val;
        }

        ltmp /= val;
        devParms.chan[chn].offset /= ltmp;
        snprintf(str, 512, "Channel %i scale: ", chn + 1);
        convertToMetricSuffix(str + strlen(str), devParms.chan[chn].scale, 2, 512 - strlen(str));
        strlcat(str, "V", 512);
        statusLabel->setText(str);
        snprintf(str, 512, ":CHAN%i:SCAL %e", chn + 1, devParms.chan[chn].scale);
        setCueCmd(str);
    }

    waveForm->update();
}

void UiMainWindow::chanScaleMinus() {
    int chn;
    double val, ltmp;
    char str[512];
    if(device == nullptr)
        return;
    if(!devParms.connected)
        return;
    if(devParms.activechannel < 0)
        return;
    if(devParms.mathFft && devParms.mathFftSplit) {
        devParms.fftVScale = roundDownStep125(devParms.fftVScale, nullptr);
        if(devParms.mathFftUnit == 0) {
            if(devParms.fftVScale < 1.0)
                devParms.fftVScale = 1.0;
        } else {
            if(devParms.fftVScale < 1.0)
                devParms.fftVScale = 1.0;
        }

        if(devParms.fftVOffset > (devParms.fftVScale * 4.0))
            devParms.fftVOffset = (devParms.fftVScale * 4.0);
        if(devParms.fftVOffset < (devParms.fftVScale * -4.0))
            devParms.fftVOffset = (devParms.fftVScale * -4.0);
        if(devParms.modelSerie != 1) {
            if(devParms.mathFftUnit == 1) {
                snprintf(str, 512, ":CALC:FFT:VSC %e", devParms.fftVScale);
                setCueCmd(str);
            } else {
                snprintf(str,
                    512,
                    ":CALC:FFT:VSC %e",
                    devParms.fftVScale / devParms.chan[devParms.mathFftSrc].scale);
                setCueCmd(str);
            }
        } else {
            snprintf(str, 512, ":MATH:SCAL %e", devParms.fftVScale);
            setCueCmd(str);
        }

        if(devParms.mathFftUnit == 0) {
            strlcpy(str, "FFT scale: ", 512);
            convertToMetricSuffix(str + strlen(str), devParms.fftVScale, 1, 512 - strlen(str));
            strlcat(str, "V/Div", 512);
        } else {
            snprintf(str, 512, "FFT scale: %.1fdB/Div", devParms.fftVScale);
        }

        statusLabel->setText(str);
        waveForm->update();
        return;
    }

    chn = devParms.activechannel;
    if(devParms.chan[chn].scale <= 1e-2) {
        devParms.chan[chn].scale = 1e-2;
        return;
    }

    ltmp = devParms.chan[chn].scale;
    if(devParms.chan[chn].vernier)
        val = roundUpStep125(devParms.chan[chn].scale, nullptr);
    else
        val = roundDownStep125(devParms.chan[chn].scale, nullptr);
    if(devParms.chan[chn].vernier) {
        val /= 100;
        devParms.chan[chn].scale -= val;
    } else {
        devParms.chan[chn].scale = val;
    }

    ltmp /= val;
    devParms.chan[chn].offset /= ltmp;
    snprintf(str, 512, "Channel %i scale: ", chn + 1);
    convertToMetricSuffix(str + strlen(str), devParms.chan[chn].scale, 2, 512 - strlen(str));
    strlcat(str, "V", 512);
    statusLabel->setText(str);
    snprintf(str, 512, ":CHAN%i:SCAL %e", chn + 1, devParms.chan[chn].scale);
    setCueCmd(str);
    waveForm->update();
}

void UiMainWindow::chanScaleMinusAll() {
    int chn;
    double val, ltmp;
    char str[512];
    if(device == nullptr)
        return;
    if(!devParms.connected)
        return;
    if(devParms.activechannel < 0)
        return;
    if(devParms.mathFft && devParms.mathFftSplit)
        return;
    for(chn = 0; chn < devParms.channelCnt; chn++) {
        if(!devParms.chan[chn].Display)
            continue;
        if(devParms.chan[chn].scale <= 1e-2) {
            devParms.chan[chn].scale = 1e-2;
            return;
        }

        ltmp = devParms.chan[chn].scale;
        if(devParms.chan[chn].vernier)
            val = roundUpStep125(devParms.chan[chn].scale, nullptr);
        else
            val = roundDownStep125(devParms.chan[chn].scale, nullptr);
        if(devParms.chan[chn].vernier) {
            val /= 100;
            devParms.chan[chn].scale -= val;
        } else {
            devParms.chan[chn].scale = val;
        }

        ltmp /= val;
        devParms.chan[chn].offset /= ltmp;
        snprintf(str, 512, "Channel %i scale: ", chn + 1);
        convertToMetricSuffix(str + strlen(str), devParms.chan[chn].scale, 2, 512 - strlen(str));
        strlcat(str, "V", 512);
        statusLabel->setText(str);
        snprintf(str, 512, ":CHAN%i:SCAL %e", chn + 1, devParms.chan[chn].scale);
        setCueCmd(str);
    }

    waveForm->update();
}

void UiMainWindow::shiftTraceUp() {
    int chn;
    char str[512];
    if(device == nullptr)
        return;
    if(!devParms.connected)
        return;
    if(devParms.activechannel < 0)
        return;
    if(devParms.mathFft && devParms.mathFftSplit) {
        devParms.fftVOffset += devParms.fftVScale;
        if(devParms.fftVOffset > (devParms.fftVScale * 4.0))
            devParms.fftVOffset = (devParms.fftVScale * 4.0);
        if(devParms.mathFftUnit && (devParms.fftVScale > 9.9))
            devParms.fftVOffset = nearbyint(devParms.fftVOffset);
        if(devParms.modelSerie != 1) {
            snprintf(str, 512, ":CALC:FFT:VOFF %e", devParms.fftVOffset);
            setCueCmd(str);
        } else {
            snprintf(str, 512, ":MATH:OFFS %e", devParms.fftVOffset);
            setCueCmd(str);
        }

        if(devParms.mathFftUnit == 0) {
            strlcpy(str, "FFT position: ", 512);
            convertToMetricSuffix(str + strlen(str), devParms.fftVOffset, 1, 512 - strlen(str));
            strlcat(str, "V", 512);
        } else {
            snprintf(str, 512, "FFT position: %+.0fdB", devParms.fftVOffset);
        }

        statusLabel->setText(str);
        waveForm->labelActive = LABEL_ACTIVE_FFT;
        labelTimer->start(LABEL_TIMER_IVAL);
        waveForm->update();
        return;
    }

    chn = devParms.activechannel;
    if(devParms.chan[chn].offset >= 20) {
        devParms.chan[chn].offset = 20;
        return;
    }

    devParms.chan[chn].offset += devParms.chan[chn].scale;
    snprintf(str, 512, "Channel %i offset: ", chn + 1);
    convertToMetricSuffix(str + strlen(str), devParms.chan[chn].offset, 2, 512 - strlen(str));
    strlcat(str, devParms.chan[devParms.chan[chn].unit].unitstr, 512);
    statusLabel->setText(str);
    waveForm->labelActive = chn + 1;
    labelTimer->start(LABEL_TIMER_IVAL);
    vertOffsDialTimer->start(TMC_DIAL_TIMER_DELAY);
    waveForm->update();
}

void UiMainWindow::shiftTraceDown() {
    int chn;
    char str[512];
    if(device == nullptr)
        return;
    if(!devParms.connected)
        return;
    if(devParms.activechannel < 0)
        return;
    if(devParms.mathFft && devParms.mathFftSplit) {
        devParms.fftVOffset -= devParms.fftVScale;
        if(devParms.fftVOffset < (devParms.fftVScale * -4.0))
            devParms.fftVOffset = (devParms.fftVScale * -4.0);
        if(devParms.mathFftUnit) {
            if(devParms.fftVScale > 9.0)
                devParms.fftVOffset = nearbyint(devParms.fftVOffset);
            else
                devParms.fftVOffset = nearbyint(devParms.fftVOffset * 10.0) / 10.0;
        }

        if(devParms.modelSerie != 1) {
            snprintf(str, 512, ":CALC:FFT:VOFF %e", devParms.fftVOffset);
            setCueCmd(str);
        } else {
            snprintf(str, 512, ":MATH:OFFS %e", devParms.fftVOffset);
            setCueCmd(str);
        }

        if(devParms.mathFftUnit == 0) {
            strlcpy(str, "FFT position: ", 512);
            convertToMetricSuffix(str + strlen(str), devParms.fftVOffset, 1, 512 - strlen(str));
            strlcat(str, "V", 512);
        } else {
            snprintf(str, 512, "FFT position: %+.0fdB", devParms.fftVOffset);
        }

        statusLabel->setText(str);
        waveForm->labelActive = LABEL_ACTIVE_FFT;
        labelTimer->start(LABEL_TIMER_IVAL);
        waveForm->update();
        return;
    }

    chn = devParms.activechannel;
    if(devParms.chan[chn].offset <= -20) {
        devParms.chan[chn].offset = -20;
        return;
    }

    devParms.chan[chn].offset -= devParms.chan[chn].scale;
    snprintf(str, 512, "Channel %i offset: ", chn + 1);
    convertToMetricSuffix(str + strlen(str), devParms.chan[chn].offset, 2, 512 - strlen(str));
    strlcat(str, devParms.chan[devParms.chan[chn].unit].unitstr, 512);
    statusLabel->setText(str);
    waveForm->labelActive = chn + 1;
    labelTimer->start(LABEL_TIMER_IVAL);
    vertOffsDialTimer->start(TMC_DIAL_TIMER_DELAY);
    waveForm->update();
}

void UiMainWindow::setToFactory() {

    char str[512];
    if((device == nullptr) || (!devParms.connected))
        return;
    QMessageBox msgBox2;
    msgBox2.setText("Do you want to reset the instrument to the factory settings?");
    msgBox2.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
    msgBox2.setDefaultButton(QMessageBox::No);
    if(msgBox2.exec() != QMessageBox::Yes)
        return;
    scrnTimer->stop();
    scrnThread->wait();
    tmcWrite("*RST");
    devParms.timebasescale = 1e-6;
    devParms.timebaseoffset = 0;
    devParms.timebasedelayenable = 0;
    devParms.timebasedelayoffset = 0;
    for(int i{}; i < MAX_CHNS; i++) {
        devParms.chan[i].scale = 1;
        devParms.chan[i].offset = 0;
        devParms.chan[i].Display = 0;
        devParms.chan[i].coupling = Coup::DC;
        devParms.chan[i].bwlimit = 0;
        devParms.chan[i].probe = 10;
        devParms.chan[i].invert = 0;
        devParms.chan[i].vernier = 0;
        devParms.triggeredgelevel[i] = 0;
    }

    devParms.chan[0].Display = 1;
    devParms.activechannel = 0;
    devParms.acquiretype = 0;
    devParms.acquirememdepth = 0;
    devParms.triggermode = 0;
    devParms.triggeredgesource = CHAN::OFF;
    devParms.triggeredgeslope = 0;
    devParms.triggerstatus = 3;
    devParms.triggercoupling = 1;
    if(devParms.modelSerie == 1)
        devParms.triggerholdoff = 1.6e-8;
    else
        devParms.triggerholdoff = 1e-7;
    devParms.displaytype = 0;
    devParms.displaygrid = 2;
    devParms.countersrc = 0;
    devParms.funcWrecEnable = 0;
    statusLabel->setText("Reset to factory settings");
    waveForm->update();
    QMessageBox msgBox;
    msgBox.setText("Resetting the instrument to the factory settings.\n"
                   "Please wait...");
    QTimer tRst1;
    tRst1.setSingleShot(true);
    tRst1.setTimerType(Qt::PreciseTimer);
    connect(&tRst1, &QTimer::timeout, &msgBox, &QMessageBox::accept);
    tRst1.start(9000);
    msgBox.exec();
    if(devParms.modelSerie == 6) {
        for(int i{}; i < MAX_CHNS; i++) {
            snprintf(str, 512, ":CHAN%i:SCAL 1", i + 1);
            tmcWrite(str);
            usleep(20000);
        }
    }

    scrnTimer->start(devParms.screenTimerIval);
}

// this function is called when ScreenThread has finished
void UiMainWindow::screenUpdate() {
    int i, chns = 0;
    char str[512];
    if(device == nullptr) {
        pthread_mutex_unlock(&devParms.mutexx);
        return;
    }

    if(!devParms.connected) {
        pthread_mutex_unlock(&devParms.mutexx);
        return;
    }

    if(!devParms.screenupdatesOn) {
        pthread_mutex_unlock(&devParms.mutexx);
        return;
    }

    scrnThread->getParams(&devParms);
    if(devParms.threadErrorStat) {
        scrnTimer->stop();
        snprintf(str,
            512,
            "An error occurred while reading screen data from device.\n"
            "File ScreenThread.cpp line %i",
            devParms.threadErrorLine);
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(str);
        msgBox.exec();
        pthread_mutex_unlock(&devParms.mutexx);
        closeConnection();
        return;
    }

    if(devParms.threadResult == TMC_THRD_RESULT_NONE) {
        pthread_mutex_unlock(&devParms.mutexx);
        return;
    }

    if(devParms.threadResult == TMC_THRD_RESULT_CMD) {
        if(devParms.threadJob == TMC_THRD_JOB_TRIGEDGELEV) {
            devParms.triggeredgelevel[devParms.triggeredgesource] = devParms.threadValue;
            //      waveForm->setTrigLineVisible();
        }

        pthread_mutex_unlock(&devParms.mutexx);
        return;
    }

    if(scrnTimer->isActive() == false) {
        pthread_mutex_unlock(&devParms.mutexx);
        return;
    }

    runButton->setStyleSheet(defStylesh);
    singleButton->setStyleSheet(defStylesh);
    if(devParms.triggerstatus == 0)
        runButton->setStyleSheet("background: #66FF99; color: black;");
    else if(devParms.triggerstatus == 1)
        singleButton->setStyleSheet("background: #FF9966; color: black;");
    else if(devParms.triggerstatus == 2)
        runButton->setStyleSheet("background: #66FF99; color: black;");
    else if(devParms.triggerstatus == 3)
        runButton->setStyleSheet("background: #66FF99; color: black;");
    else if(devParms.triggerstatus == 5)
        runButton->setStyleSheet("background: #FF0066; color: black;");
    if(devParms.triggersweep == 0) {
        trigModeAutoLed->setValue(true);
        trigModeNormLed->setValue(false);
        trigModeSingLed->setValue(false);
    } else if(devParms.triggersweep == 1) {
        trigModeAutoLed->setValue(false);
        trigModeNormLed->setValue(true);
        trigModeSingLed->setValue(false);
    } else if(devParms.triggersweep == 2) {
        trigModeAutoLed->setValue(false);
        trigModeNormLed->setValue(false);
        trigModeSingLed->setValue(true);
    }

    if(waveForm->hasMoveEvent() == true) {
        pthread_mutex_unlock(&devParms.mutexx);
        return;
    }

    for(int i{}; i < MAX_CHNS; i++) {
        if(!devParms.chan[i].Display) // Display data only when channel is switched on
            continue;
        chns++;
    }

    if(!chns) {
        waveForm->clear();
        pthread_mutex_unlock(&devParms.mutexx);
        return;
    }

    //  if(devParms.triggerstatus != 1)  // Don't plot waveform data when triggerstatus is "wait"
    if(1) {
        if(devParms.mathDecodeDisplay)
            serialDecoder(&devParms);
        waveForm->drawCurve(&devParms, device);
    } else // trigger status is "wait"
    {
        waveForm->update();
    }

    pthread_mutex_unlock(&devParms.mutexx);
}

void UiMainWindow::setCueCmd(const char* str) {
    strlcpy(devParms.cmdCue[devParms.cmdCueIdxIn], str, 128);
    devParms.cmdCueResp[devParms.cmdCueIdxIn] = nullptr;
    devParms.cmdCueIdxIn++;
    devParms.cmdCueIdxIn %= TMC_CMD_CUE_SZ;
    scrnTimerHandler();
}

void UiMainWindow::setCueCmd(const char* str, char* ptr) {
    strlcpy(devParms.cmdCue[devParms.cmdCueIdxIn], str, 128);
    ptr[0] = 0;
    devParms.cmdCueResp[devParms.cmdCueIdxIn] = ptr;
    devParms.cmdCueIdxIn++;
    devParms.cmdCueIdxIn %= TMC_CMD_CUE_SZ;
    scrnTimerHandler();
}
