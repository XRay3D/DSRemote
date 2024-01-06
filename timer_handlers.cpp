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

void UiMainWindow::testTimerHandler() {
    printf("scrUpdateCntr is: %u\n", waveForm->scrUpdateCntr);

    waveForm->scrUpdateCntr = 0;
}

void UiMainWindow::labelTimerHandler() {
    waveForm->labelActive = LABEL_ACTIVE_NONE;
}

void UiMainWindow::navDialTimerHandler() {
    if(navDial->isSliderDown() == true) {
        navDialChanged(navDial->value());
    } else {
        navDial->setSliderPosition(50);

        navDialFunc = NAV_DIAL_FUNC_NONE;

        if(adjdialTimer->isActive() == false)
            adjDialFunc = ADJ_DIAL_FUNC_NONE;
    }
}

void UiMainWindow::adjdialTimerHandler() {
    char str[512];

    adjdialTimer->stop();

    adjDialLabel->setStyleSheet(defStylesh);

    adjDialLabel->setStyleSheet("font: 7pt;");

    adjDialLabel->setText("");

    if(adjDialFunc == ADJ_DIAL_FUNC_HOLDOFF) {
        strlcpy(str, "Trigger holdoff: ", 512);

        convertToMetricSuffix(str + strlen(str), devParms.triggerholdoff, 2, 512 - strlen(str));

        strlcat(str, "s", 512);

        statusLabel->setText(str);

        snprintf(str, 512, ":TRIG:HOLD %e", devParms.triggerholdoff);

        setCueCmd(str);

        if((devParms.modelSerie == 2) || (devParms.modelSerie == 6)) {
            usleep(20000);

            setCueCmd(":DISP:CLE");
        } else {
            usleep(20000);

            setCueCmd(":CLE");
        }
    } else if(adjDialFunc == ADJ_DIAL_FUNC_ACQ_AVG) {
        snprintf(str, 512, "Acquire averages: %i", devParms.acquireaverages);

        statusLabel->setText(str);

        snprintf(str, 512, ":ACQ:AVER %i", devParms.acquireaverages);

        setCueCmd(str);
    }

    adjDialFunc = ADJ_DIAL_FUNC_NONE;

    if(navDialTimer->isActive() == false)
        navDialFunc = NAV_DIAL_FUNC_NONE;
}

void UiMainWindow::scrnTimerHandler() {
    if(pthread_mutex_trylock(&devParms.mutexx))
        return;

    scrnThread->setParams(&devParms);

    scrnThread->start();
}

void UiMainWindow::horPosDialTimerHandler() {
    char str[512];

    if(devParms.timebasedelayenable)
        snprintf(str, 512, ":TIM:DEL:OFFS %e", devParms.timebasedelayoffset);
    else
        snprintf(str, 512, ":TIM:OFFS %e", devParms.timebaseoffset);

    setCueCmd(str);
}

void UiMainWindow::trigAdjDialTimerHandler() {
    int chn;

    char str[512];

    chn = devParms.triggeredgesource;

    if((chn < 0) || (chn > 3))
        return;

    snprintf(str, 512, ":TRIG:EDG:LEV %e", devParms.triggeredgelevel[chn]);

    setCueCmd(str);
}

void UiMainWindow::vertOffsDialTimerHandler() {
    int chn;

    char str[512];

    if(devParms.activechannel < 0)
        return;

    chn = devParms.activechannel;

    snprintf(str, 512, ":CHAN%i:OFFS %e", chn + 1, devParms.chanoffset[chn]);

    setCueCmd(str);
}

void UiMainWindow::horScaleDialTimerHandler() {
    char str[512];

    if(devParms.timebasedelayenable)
        snprintf(str, 512, ":TIM:DEL:SCAL %e", devParms.timebasedelayscale);
    else
        snprintf(str, 512, ":TIM:SCAL %e", devParms.timebasescale);

    setCueCmd(str);
}

void UiMainWindow::vertScaleDialTimerHandler() {
    int chn;

    char str[512];

    if(devParms.activechannel < 0)
        return;

    chn = devParms.activechannel;

    snprintf(str, 512, ":CHAN%i:SCAL %e", chn + 1, devParms.chanscale[chn]);

    setCueCmd(str);
}
