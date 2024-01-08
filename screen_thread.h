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

#pragma once

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <QObject>
#include <QThread>

#include "connection.h"
#include "global.h"
#include "tmc_dev.h"
#include "utils.h"

#include "third_party/kiss_fft/kiss_fftr.h"

class ScreenThread : public QThread {
    Q_OBJECT

public:
    ScreenThread();
    ~ScreenThread();

    int hBusy;

    void setDevice(struct tmcDev*);

    void setParams(struct DeviceSettings*);
    void getParams(struct DeviceSettings*);

private:
    struct {
        int connected;
        int modelserie;
        int chanDisplay[MAX_CHNS];
        double chanScale[MAX_CHNS];
        int triggerstatus;
        int triggersweep;
        double samplerate;
        int memdepth;
        int countersrc;
        double counterfreq;
        int waveBufsz;
        short* waveBuf[MAX_CHNS];
        int errorStat;
        int errorLine;
        int cmdCueIdxIn;
        int cmdCueIdxOut;
        int result;
        int job;

        double triggerEdgeLevel;
        double timebaseDelayOffset;
        double timebaseDelayScale;

        int mathFftSrc;
        int mathFft;
        int mathFftUnit;
        double mathFftHscale;
        double mathFftHcenter;
        double* fftBufIn;
        double* fftBufOut;
        int fftBufsz;
        kiss_fftr_cfg kCfg;
        kiss_fft_cpx* kissFftBuf;

        int currentScreenSf;

        int funcWrecEnable;
        int funcWrecFmax;
        int funcWrecOperate;
        int funcWrepFmax;
        int funcWplayOperate;
        int funcWplayFcur;

        double xorigin[MAX_CHNS];

        char debugStr[1024];
    } params;

    struct tmcDev* device;

    struct DeviceSettings* deviceParms;

    void run();

    int get_devicestatus();
};
