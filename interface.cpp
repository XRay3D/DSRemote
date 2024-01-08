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

void UiMainWindow::navDialChanged(int npos) {
    int mpr = 1;

    double val, lefttime, righttime, delayrange;

    if(navDial->isSliderDown() == true)
        navDialTimer->start(100);
    else
        navDialTimer->start(300);

    if(npos > 93)
        mpr = +64;
    else if(npos > 86)
        mpr = +32;
    else if(npos > 79)
        mpr = +16;
    else if(npos > 72)
        mpr = +8;
    else if(npos > 65)
        mpr = +4;
    else if(npos > 58)
        mpr = +2;
    else if(npos > 51)
        mpr = +1;
    else if(npos > 49)
        return;
    else if(npos > 42)
        mpr = -1;
    else if(npos > 35)
        mpr = -2;
    else if(npos > 28)
        mpr = -4;
    else if(npos > 21)
        mpr = -8;
    else if(npos > 14)
        mpr = -16;
    else if(npos > 7)
        mpr = -32;
    else
        mpr = -64;

    if(navDialFunc == NAV_DIAL_FUNC_HOLDOFF) {
        adjdialTimer->start(ADJDIAL_TIMER_IVAL_2);

        val = getStepSizeDivideBy1000(devParms.triggerholdoff);

        devParms.triggerholdoff += (val * mpr);

        if(devParms.modelSerie == 1) {
            if(devParms.triggerholdoff < 1.7e-8)
                devParms.triggerholdoff = 1.6e-8;
        } else {
            if(devParms.triggerholdoff < 1.01e-7)
                devParms.triggerholdoff = 1e-7;
        }

        if(devParms.triggerholdoff > 10)
            devParms.triggerholdoff = 10;
    } else if(devParms.timebasedelayenable) {
        val = devParms.timebasedelayoffset;

        if(val < 0)
            val *= -1;

        if(val < 2e-7)
            val = 2e-7;

        val = getStepSizeDivideBy1000(val);

        devParms.timebasedelayoffset += (val * mpr);

        lefttime = ((devParms.horDivisions / 2) * devParms.timebasescale) - devParms.timebaseoffset;

        righttime = ((devParms.horDivisions / 2) * devParms.timebasescale) + devParms.timebaseoffset;

        delayrange = (devParms.horDivisions / 2) * devParms.timebasedelayscale;

        if(devParms.timebasedelayoffset < -(lefttime - delayrange))
            devParms.timebasedelayoffset = -(lefttime - delayrange);

        if(devParms.timebasedelayoffset > (righttime - delayrange))
            devParms.timebasedelayoffset = (righttime - delayrange);
    }

    waveForm->update();
}

void UiMainWindow::navDialReleased() {
    char str[512];
    navDial->setSliderPosition(50);

    if(navDialFunc == NAV_DIAL_FUNC_HOLDOFF) {
        strlcpy(str, "Trigger holdoff: ", 512);

        convertToMetricSuffix(str + strlen(str), devParms.triggerholdoff, 2, 512);

        strlcat(str, "s", 512);

        statusLabel->setText(str);

        snprintf(str, 512, ":TRIG:HOLD %e", devParms.triggerholdoff);
        setCueCmd(str);
    } else if(devParms.timebasedelayenable) {
        strlcpy(str, "Delayed timebase position: ", 512);

        convertToMetricSuffix(str + strlen(str), devParms.timebasedelayoffset, 2, 512);

        strlcat(str, "s", 512);

        statusLabel->setText(str);

        snprintf(str, 512, ":TIM:DEL:OFFS %e", devParms.timebasedelayoffset);
        setCueCmd(str);
    }

    waveForm->update();
}

void UiMainWindow::adjDialChanged(int newPos) {
    static int oldPos = 50;
    int diff, dir;

    if(adjDialFunc == ADJ_DIAL_FUNC_NONE) return;

    adjdialTimer->start(ADJDIAL_TIMER_IVAL_2);

    diff = newPos - oldPos;

    if(diff < 0)
        diff *= -1;

    if(diff < 6)
        return;

    if(newPos > oldPos)
        if(diff < 12)
            dir = 0;
        else
            dir = 1;
    else if(diff < 12)
        dir = 1;
    else
        dir = 0;

    if(adjDialFunc == ADJ_DIAL_FUNC_HOLDOFF) {
        if(!dir) {
            if(devParms.triggerholdoff >= 10) {
                devParms.triggerholdoff = 10;
                oldPos = newPos;
                return;
            }

            devParms.triggerholdoff += getStepSizeDivideBy1000(devParms.triggerholdoff);
        } else {
            if(devParms.modelSerie == 1) {
                if(devParms.triggerholdoff < 1.7e-8) {
                    devParms.triggerholdoff = 1.6e-8;
                    oldPos = newPos;
                    return;
                }
            } else if(devParms.triggerholdoff <= 1.01e-7) {
                devParms.triggerholdoff = 1e-7;
                oldPos = newPos;
                return;
            }

            devParms.triggerholdoff -= getStepSizeDivideBy1000(devParms.triggerholdoff);
        }
    } else if(adjDialFunc == ADJ_DIAL_FUNC_ACQ_AVG) {
        if(!dir) {
            if(devParms.modelSerie == 6) {
                if(devParms.acquireaverages >= 8192) {
                    devParms.acquireaverages = 8192;
                    oldPos = newPos;
                    return;
                }
            } else if(devParms.acquireaverages >= 1024) {
                devParms.acquireaverages = 1024;
                oldPos = newPos;
                return;
            }

            devParms.acquireaverages *= 2;
        } else {
            if(devParms.acquireaverages <= 2) {
                devParms.acquireaverages = 2;
                oldPos = newPos;
                return;
            }

            devParms.acquireaverages /= 2;
        }
    }

    oldPos = newPos;
    waveForm->update();
}

void UiMainWindow::trigAdjustDialChanged(int newPos) {
    static int oldPos = 50;

    int diff, dir, chn;

    char str[512];
    if(devParms.activechannel < 0)
        return;

    chn = devParms.triggeredgesource;

    if((chn < 0) || (chn > 3))
        return;

    diff = newPos - oldPos;

    if(diff < 0)
        diff *= -1;

    if(diff < 6)
        return;

    if(newPos > oldPos)
        if(diff < 12)
            dir = 0;
        else
            dir = 1;
    else if(diff < 12)
        dir = 1;
    else
        dir = 0;

    if(dir) {
        if(devParms.triggeredgelevel[chn] <= (-6 * devParms.chan[chn].scale)) {
            devParms.triggeredgelevel[chn] = -6 * devParms.chan[chn].scale;

            oldPos = newPos;

            return;
        }

        devParms.triggeredgelevel[chn] -= devParms.chan[chn].scale / 50;
    } else {
        if(devParms.triggeredgelevel[chn] >= (6 * devParms.chan[chn].scale)) {
            devParms.triggeredgelevel[chn] = 6 * devParms.chan[chn].scale;

            oldPos = newPos;

            return;
        }

        devParms.triggeredgelevel[chn] += devParms.chan[chn].scale / 50;
    }

    strlcpy(str, "Trigger level: ", 512);

    convertToMetricSuffix(str + strlen(str), devParms.triggeredgelevel[chn], 2, 512);

    strlcat(str, devParms.chan[devParms.chan[chn].unit].unitstr, 512);

    statusLabel->setText(str);

    trigAdjDialTimer->start(TMC_DIAL_TIMER_DELAY);

    oldPos = newPos;

    waveForm->labelActive = LABEL_ACTIVE_TRIG;

    labelTimer->start(LABEL_TIMER_IVAL);

    waveForm->setTrigLineVisible();

    waveForm->update();
}

void UiMainWindow::horScaleDialChanged(int newPos) {
    static int oldPos = 50;

    int diff, dir;

    char str[512];
    diff = newPos - oldPos;

    if(diff < 0)
        diff *= -1;

    if(diff < 6)
        return;

    if(newPos > oldPos)
        if(diff < 12)
            dir = 0;
        else
            dir = 1;
    else if(diff < 12)
        dir = 1;
    else
        dir = 0;

    if(devParms.timebasedelayenable) {
        if(dir) {
            if(devParms.timebasedelayscale >= devParms.timebasescale / 2) {
                devParms.timebasedelayscale = devParms.timebasescale / 2;

                oldPos = newPos;

                return;
            }

            if(devParms.timebasedelayscale >= 0.1) {
                devParms.timebasedelayscale = 0.1;

                oldPos = newPos;

                return;
            }
        } else if(devParms.modelSerie == 1) {
            if(devParms.timebasedelayscale <= 5.001e-9) {
                devParms.timebasedelayscale = 5e-9;

                oldPos = newPos;

                return;
            }
        } else if(devParms.bandwidth == 1000) {
            if(devParms.timebasedelayscale <= 5.001e-10) {
                devParms.timebasedelayscale = 5e-10;

                oldPos = newPos;

                return;
            }
        } else if(devParms.timebasedelayscale <= 1.001e-9) {
            devParms.timebasedelayscale = 1e-9;

            oldPos = newPos;

            return;
        }

        if(dir)
            devParms.timebasedelayscale = roundUpStep125(devParms.timebasedelayscale, nullptr);
        else
            devParms.timebasedelayscale = roundDownStep125(devParms.timebasedelayscale, nullptr);

        devParms.currentScreenSf = 100.0 / devParms.timebasedelayscale;

        strlcpy(str, "Delayed timebase: ", 512);

        convertToMetricSuffix(str + strlen(str), devParms.timebasedelayscale, 2, 512);

        strlcat(str, "s", 512);

        statusLabel->setText(str);

        horScaleDialTimer->start(TMC_DIAL_TIMER_DELAY);

        oldPos = newPos;

        if(devParms.timebasedelayscale > 0.1000001)
            devParms.funcWrecEnable = 0;
    } else {
        if(dir) {
            if(devParms.timebasescale >= 10) {
                devParms.timebasescale = 10;

                oldPos = newPos;

                return;
            }
        } else if(devParms.modelSerie == 1) {
            if(devParms.timebasescale <= 5.001e-9) {
                devParms.timebasescale = 5e-9;

                oldPos = newPos;

                return;
            }
        } else if(devParms.bandwidth == 1000) {
            if(devParms.timebasescale <= 5.001e-10) {
                devParms.timebasescale = 5e-10;

                oldPos = newPos;

                return;
            }
        } else if(devParms.timebasescale <= 1.001e-9) {
            devParms.timebasescale = 1e-9;

            oldPos = newPos;

            return;
        }

        if(dir)
            devParms.timebasescale = roundUpStep125(devParms.timebasescale, nullptr);
        else
            devParms.timebasescale = roundDownStep125(devParms.timebasescale, nullptr);

        devParms.currentScreenSf = 100.0 / devParms.timebasescale;

        strlcpy(str, "Timebase: ", 512);

        convertToMetricSuffix(str + strlen(str), devParms.timebasescale, 2, 512 - strlen(str));

        strlcat(str, "s", 512);

        statusLabel->setText(str);

        horScaleDialTimer->start(TMC_DIAL_TIMER_DELAY);

        oldPos = newPos;

        if(devParms.timebasescale > 0.1000001)
            devParms.funcWrecEnable = 0;
    }

    waveForm->update();
}

void UiMainWindow::horPosDialChanged(int newPos) {
    static int oldPos = 50;

    int diff, dir;

    char str[512];
    if(devParms.activechannel < 0)
        return;

    diff = newPos - oldPos;

    if(diff < 0)
        diff *= -1;

    if(diff < 6)
        return;

    if(newPos > oldPos)
        if(diff < 12)
            dir = 0;
        else
            dir = 1;
    else if(diff < 12)
        dir = 1;
    else
        dir = 0;

    if(devParms.timebasedelayenable) {
        if(dir) {
            if(devParms.timebasedelayoffset >= (((devParms.horDivisions / 2) * devParms.timebasescale) + devParms.timebaseoffset - ((devParms.horDivisions / 2) * devParms.timebasedelayscale))) {
                oldPos = newPos;

                return;
            }

            devParms.timebasedelayoffset += (devParms.timebasedelayscale / 50);
        } else {
            if(devParms.timebasedelayoffset <= -(((devParms.horDivisions / 2) * devParms.timebasescale) - devParms.timebaseoffset - ((devParms.horDivisions / 2) * devParms.timebasedelayscale))) {
                oldPos = newPos;

                return;
            }

            devParms.timebasedelayoffset -= (devParms.timebasedelayscale / 50);
        }

        strlcpy(str, "Delayed timebase position: ", 512);

        convertToMetricSuffix(str + strlen(str), devParms.timebasedelayoffset, 2, 512);

        strlcat(str, "s", 512);

        statusLabel->setText(str);

        horPosDialTimer->start(TMC_DIAL_TIMER_DELAY);

        oldPos = newPos;
    } else {
        if(dir) {
            if(devParms.timebaseoffset >= 1) {
                devParms.timebaseoffset = 1;

                oldPos = newPos;

                return;
            }

            devParms.timebaseoffset += devParms.timebasescale / 50;
        } else {
            if(devParms.timebaseoffset <= -1) {
                devParms.timebaseoffset = -1;

                oldPos = newPos;

                return;
            }

            devParms.timebaseoffset -= devParms.timebasescale / 50;
        }

        strlcpy(str, "Horizontal position: ", 512);

        convertToMetricSuffix(str + strlen(str), devParms.timebaseoffset, 2, 512 - strlen(str));

        strlcat(str, "s", 512);

        statusLabel->setText(str);

        horPosDialTimer->start(TMC_DIAL_TIMER_DELAY);

        oldPos = newPos;
    }

    waveForm->update();
}

void UiMainWindow::vertOffsetDialChanged(int newPos) {
    static int oldPos = 50;

    int diff, dir, chn;

    char str[512];
    double val;

    if(devParms.activechannel < 0)
        return;

    chn = devParms.activechannel;

    diff = newPos - oldPos;

    if(diff < 0)
        diff *= -1;

    if(diff < 6)
        return;

    if(newPos > oldPos)
        if(diff < 12)
            dir = 0;
        else
            dir = 1;
    else if(diff < 12)
        dir = 1;
    else
        dir = 0;

    val = roundUpStep125(devParms.chan[chn].scale, nullptr) / 100;

    if(dir) {
        if(devParms.chan[chn].offset <= -20) {
            devParms.chan[chn].offset = -20;

            oldPos = newPos;

            return;
        }

        devParms.chan[chn].offset -= val;
    } else {
        if(devParms.chan[chn].offset >= 20) {
            devParms.chan[chn].offset = 20;

            oldPos = newPos;

            return;
        }

        devParms.chan[chn].offset += val;
    }

    snprintf(str, 512, "Channel %i offset: ", chn + 1);

    convertToMetricSuffix(str + strlen(str), devParms.chan[chn].offset, 2, 512 - strlen(str));

    strlcat(str, devParms.chan[devParms.chan[chn].unit].unitstr, 512);

    statusLabel->setText(str);

    waveForm->labelActive = chn + 1;

    labelTimer->start(LABEL_TIMER_IVAL);

    vertOffsDialTimer->start(TMC_DIAL_TIMER_DELAY);

    oldPos = newPos;

    waveForm->update();
}

void UiMainWindow::vertScaleDialChanged(int newPos) {
    static int oldPos = 50;

    int diff, dir, chn;

    double val, ltmp;

    char str[512];
    if(devParms.activechannel < 0)
        return;

    chn = devParms.activechannel;

    diff = newPos - oldPos;

    if(diff < 0)
        diff *= -1;

    if(diff < 6)
        return;

    if(newPos > oldPos)
        if(diff < 12)
            dir = 0;
        else
            dir = 1;
    else if(diff < 12)
        dir = 1;
    else
        dir = 0;

    if(dir) {
        if(devParms.chan[chn].scale >= 20) {
            devParms.chan[chn].scale = 20;

            oldPos = newPos;

            return;
        }
    } else if(devParms.chan[chn].scale <= 1e-2) {
        devParms.chan[chn].scale = 1e-2;

        oldPos = newPos;

        return;
    }

    ltmp = devParms.chan[chn].scale;

    if(dir || devParms.chan[chn].vernier)
        val = roundUpStep125(devParms.chan[chn].scale, nullptr);
    else
        val = roundDownStep125(devParms.chan[chn].scale, nullptr);

    if(devParms.chan[chn].vernier) {
        val /= 100;

        if(dir)
            devParms.chan[chn].scale += val;
        else
            devParms.chan[chn].scale -= val;
    } else {
        devParms.chan[chn].scale = val;
    }

    ltmp /= devParms.chan[chn].scale;

    devParms.chan[chn].offset /= ltmp;

    snprintf(str, 512, "Channel %i scale: ", chn + 1);

    convertToMetricSuffix(str + strlen(str), devParms.chan[chn].scale, 2, 512 - strlen(str));

    strlcat(str, devParms.chan[devParms.chan[chn].unit].unitstr, 512);

    statusLabel->setText(str);

    snprintf(str, 512, ":CHAN%i:SCAL %e", chn + 1, devParms.chan[chn].scale);
    setCueCmd(str);

    oldPos = newPos;

    waveForm->update();
}

void UiMainWindow::acqButtonClicked() {
    int chn,
        chns_on = 0,
        dual = 0;

    QMenu menu,
        submenuacquisition,
        submenumemdepth;

    QList<QAction*> actionList;

    for(chn = 0; chn < MAX_CHNS; chn++)
        if(devParms.chan[chn].Display)
            chns_on++;

    if((devParms.chan[0].Display && devParms.chan[1].Display)
        || (devParms.chan[2].Display && devParms.chan[3].Display))
        dual = 1;

    submenuacquisition.setTitle("Mode");
    submenuacquisition.addAction("Normal", this, &UiMainWindow::setAcqNormal);
    submenuacquisition.addAction("Average", this, &UiMainWindow::setAcqAverage);
    submenuacquisition.addAction("Peak Detect", this, &UiMainWindow::setAcqPeak);
    submenuacquisition.addAction("High Resolution", this, &UiMainWindow::setAcqHres);
    actionList = submenuacquisition.actions();
    if(devParms.acquiretype == 0) {
        actionList[0]->setCheckable(true);
        actionList[0]->setChecked(true);
    } else if(devParms.acquiretype == 1) {
        actionList[1]->setCheckable(true);
        actionList[1]->setChecked(true);
    } else if(devParms.acquiretype == 2) {
        actionList[2]->setCheckable(true);
        actionList[2]->setChecked(true);
    } else if(devParms.acquiretype == 3) {
        actionList[3]->setCheckable(true);
        actionList[3]->setChecked(true);
    }
    menu.addMenu(&submenuacquisition);

    submenumemdepth.setTitle("Mem Depth");
    submenumemdepth.addAction("Auto", this, &UiMainWindow::setMemDepthAuto);
    if(devParms.modelSerie == 6 || devParms.modelSerie == 4) {
        if(!dual) {
            submenumemdepth.addAction("14K", this, &UiMainWindow::setMemDepth14k);
            submenumemdepth.addAction("140K", this, &UiMainWindow::setMemDepth140k);
            submenumemdepth.addAction("1.4M", this, &UiMainWindow::setMemDepth1400k);
            submenumemdepth.addAction("14M", this, &UiMainWindow::setMemDepth14M);
            submenumemdepth.addAction("140M", this, &UiMainWindow::setMemDepth140M);
        } else {
            submenumemdepth.addAction("7K", this, &UiMainWindow::setMemDepth7k);
            submenumemdepth.addAction("70K", this, &UiMainWindow::setMemDepth70k);
            submenumemdepth.addAction("700K", this, &UiMainWindow::setMemDepth700k);
            submenumemdepth.addAction("7M", this, &UiMainWindow::setMemDepth7M);
            submenumemdepth.addAction("70M", this, &UiMainWindow::setMemDepth70M);
        }
    } else if(devParms.modelSerie == 2) {
        if(chns_on < 2) {
            submenumemdepth.addAction("14K", this, &UiMainWindow::setMemDepth14k);
            submenumemdepth.addAction("140K", this, &UiMainWindow::setMemDepth140k);
            submenumemdepth.addAction("1.4M", this, &UiMainWindow::setMemDepth1400k);
            submenumemdepth.addAction("14M", this, &UiMainWindow::setMemDepth14M);
            submenumemdepth.addAction("56M", this, &UiMainWindow::setMemDepth56M);
        } else {
            submenumemdepth.addAction("7K", this, &UiMainWindow::setMemDepth7k);
            submenumemdepth.addAction("70K", this, &UiMainWindow::setMemDepth70k);
            submenumemdepth.addAction("700K", this, &UiMainWindow::setMemDepth700k);
            submenumemdepth.addAction("7M", this, &UiMainWindow::setMemDepth7M);
            submenumemdepth.addAction("28M", this, &UiMainWindow::setMemDepth28M);
        }
    } else if(devParms.modelSerie == 1) {
        if(chns_on < 2) {
            submenumemdepth.addAction("12K", this, &UiMainWindow::setMemDepth12k);
            submenumemdepth.addAction("120K", this, &UiMainWindow::setMemDepth120k);
            submenumemdepth.addAction("1.2M", this, &UiMainWindow::setMemDepth1200k);
            submenumemdepth.addAction("12M", this, &UiMainWindow::setMemDepth12M);
            submenumemdepth.addAction("24M", this, &UiMainWindow::setMemDepth24M);
        } else if(chns_on < 3) {
            submenumemdepth.addAction("6K", this, &UiMainWindow::setMemDepth6k);
            submenumemdepth.addAction("60K", this, &UiMainWindow::setMemDepth60k);
            submenumemdepth.addAction("600K", this, &UiMainWindow::setMemDepth600k);
            submenumemdepth.addAction("6M", this, &UiMainWindow::setMemDepth6M);
            submenumemdepth.addAction("12M", this, &UiMainWindow::setMemDepth12M);
        } else {
            submenumemdepth.addAction("3K", this, &UiMainWindow::setMemDepth3k);
            submenumemdepth.addAction("30K", this, &UiMainWindow::setMemDepth30k);
            submenumemdepth.addAction("300K", this, &UiMainWindow::setMemDepth300k);
            submenumemdepth.addAction("3M", this, &UiMainWindow::setMemDepth3M);
            submenumemdepth.addAction("6M", this, &UiMainWindow::setMemDepth6M);
        }
    }
    actionList = submenumemdepth.actions();
    if(devParms.acquirememdepth == 0) {
        actionList[0]->setCheckable(true);
        actionList[0]->setChecked(true);
    }
    if(devParms.modelSerie != 1) {
        if((devParms.acquirememdepth == 14000) || (devParms.acquirememdepth == 7000)) {
            actionList[1]->setCheckable(true);
            actionList[1]->setChecked(true);
        } else if((devParms.acquirememdepth == 140000) || (devParms.acquirememdepth == 70000)) {
            actionList[2]->setCheckable(true);
            actionList[2]->setChecked(true);
        } else if((devParms.acquirememdepth == 1400000) || (devParms.acquirememdepth == 700000)) {
            actionList[3]->setCheckable(true);
            actionList[3]->setChecked(true);
        } else if((devParms.acquirememdepth == 14000000) || (devParms.acquirememdepth == 7000000)) {
            actionList[4]->setCheckable(true);
            actionList[4]->setChecked(true);
        } else if((devParms.acquirememdepth == 140000000) || (devParms.acquirememdepth == 70000000) || (devParms.acquirememdepth == 56000000) || (devParms.acquirememdepth == 28000000)) {
            actionList[5]->setCheckable(true);
            actionList[5]->setChecked(true);
        }
    } else if((devParms.acquirememdepth == 12000) || (devParms.acquirememdepth == 6000) || (devParms.acquirememdepth == 3000)) {
        actionList[1]->setCheckable(true);
        actionList[1]->setChecked(true);
    } else if((devParms.acquirememdepth == 120000) || (devParms.acquirememdepth == 60000) || (devParms.acquirememdepth == 30000)) {
        actionList[2]->setCheckable(true);
        actionList[2]->setChecked(true);
    } else if((devParms.acquirememdepth == 1200000) || (devParms.acquirememdepth == 600000) || (devParms.acquirememdepth == 300000)) {
        actionList[3]->setCheckable(true);
        actionList[3]->setChecked(true);
    } else if((devParms.acquirememdepth == 12000000) || (devParms.acquirememdepth == 6000000) || (devParms.acquirememdepth == 3000000)) {
        actionList[4]->setCheckable(true);
        actionList[4]->setChecked(true);
    } else if((devParms.acquirememdepth == 24000000) || (devParms.acquirememdepth == 12000000) || (devParms.acquirememdepth == 6000000)) {
        actionList[5]->setCheckable(true);
        actionList[5]->setChecked(true);
    }
    menu.addMenu(&submenumemdepth);

    menu.exec(acqButton->mapToGlobal(QPoint(0, 0)));
}

void UiMainWindow::setMemDepth(int mdepth) {
    char str[512];
    QMessageBox msgBox;

    if(devParms.triggerstatus == 5) // Trigger status is STOP?
    {
        msgBox.setIcon(QMessageBox::NoIcon);
        msgBox.setText("Can not set memory depth when in STOP mode.\n");
        msgBox.exec();
        return;
    }

    if(mdepth <= 0) {
        statusLabel->setText("Memory depth: auto");
        setCueCmd(":ACQ:MDEP AUTO");

        devParms.timebaseoffset = 0;

        usleep(20000);
        setCueCmd(":TIM:OFFS 0");

        return;
    }

    strlcpy(str, "Memory depth: ", 512);

    convertToMetricSuffix(str + strlen(str), mdepth, 0, 512 - strlen(str));

    statusLabel->setText(str);

    snprintf(str, 512, ":ACQ:MDEP %i", mdepth);
    setCueCmd(str);

    devParms.timebaseoffset = 0;

    usleep(20000);
    setCueCmd(":TIM:OFFS 0");
}

void UiMainWindow::setMemDepthAuto() {
    setMemDepth(0);
}

void UiMainWindow::setMemDepth12k() {
    setMemDepth(12000);
}

void UiMainWindow::setMemDepth120k() {
    setMemDepth(120000);
}

void UiMainWindow::setMemDepth1200k() {
    setMemDepth(1200000);
}

void UiMainWindow::setMemDepth12M() {
    setMemDepth(12000000);
}

void UiMainWindow::setMemDepth24M() {
    setMemDepth(24000000);
}

void UiMainWindow::setMemDepth3k() {
    setMemDepth(3000);
}

void UiMainWindow::setMemDepth30k() {
    setMemDepth(30000);
}

void UiMainWindow::setMemDepth300k() {
    setMemDepth(300000);
}

void UiMainWindow::setMemDepth3M() {
    setMemDepth(3000000);
}

void UiMainWindow::setMemDepth6M() {
    setMemDepth(6000000);
}

void UiMainWindow::setMemDepth6k() {
    setMemDepth(6000);
}

void UiMainWindow::setMemDepth60k() {
    setMemDepth(60000);
}

void UiMainWindow::setMemDepth600k() {
    setMemDepth(600000);
}

void UiMainWindow::setMemDepth7k() {
    setMemDepth(7000);
}

void UiMainWindow::setMemDepth70k() {
    setMemDepth(70000);
}

void UiMainWindow::setMemDepth700k() {
    setMemDepth(700000);
}

void UiMainWindow::setMemDepth7M() {
    setMemDepth(7000000);
}

void UiMainWindow::setMemDepth70M() {
    setMemDepth(70000000);
}

void UiMainWindow::setMemDepth14k() {
    setMemDepth(14000);
}

void UiMainWindow::setMemDepth140k() {
    setMemDepth(140000);
}

void UiMainWindow::setMemDepth1400k() {
    setMemDepth(1400000);
}

void UiMainWindow::setMemDepth14M() {
    setMemDepth(14000000);
}

void UiMainWindow::setMemDepth140M() {
    setMemDepth(140000000);
}

void UiMainWindow::setMemDepth28M() {
    setMemDepth(28000000);
}

void UiMainWindow::setMemDepth56M() {
    setMemDepth(56000000);
}

void UiMainWindow::setAcqNormal() {
    if(devParms.acquiretype == 0)
        return;

    devParms.acquiretype = 0;

    statusLabel->setText("Acquire: normal");
    setCueCmd(":ACQ:TYPE NORM");
}

void UiMainWindow::setAcqPeak() {
    if(devParms.acquiretype == 2)
        return;

    devParms.acquiretype = 2;

    statusLabel->setText("Acquire: peak");
    setCueCmd(":ACQ:TYPE PEAK");
}

void UiMainWindow::setAcqHres() {
    if(devParms.acquiretype == 3)
        return;

    devParms.acquiretype = 3;

    statusLabel->setText("Acquire: high resolution");
    setCueCmd(":ACQ:TYPE HRES");
}

void UiMainWindow::setAcqAverage() {
    adjDialFunc = ADJ_DIAL_FUNC_ACQ_AVG;

    adjDialLabel->setText("Averages");

    adjDialLabel->setStyleSheet("background: #66FF99; font: 7pt;");

    adjdialTimer->start(ADJDIAL_TIMER_IVAL_1);

    if(devParms.acquiretype == 1)
        return;

    devParms.acquiretype = 1;

    statusLabel->setText("Acquire: average");
    setCueCmd(":ACQ:TYPE AVER");
}

void UiMainWindow::cursButtonClicked() {
}

void UiMainWindow::saveButtonClicked() {
    QMenu menu;

    menu.addAction("Save screen waveform", this, &UiMainWindow::saveScreenWaveform);
    menu.addAction("Wave Inspector", this, &UiMainWindow::getDeepMemoryWaveform);
    menu.addAction("Save screenshot", this, &UiMainWindow::saveScreenshot);
    menu.addAction("Factory", this, &UiMainWindow::setToFactory);

    menu.exec(saveButton->mapToGlobal(QPoint(0, 0)));
}

void UiMainWindow::dispButtonClicked() {
    QMenu menu,
        submenutype,
        submenugrid,
        submenugrading;

    QList<QAction*> actionList;

    submenutype.setTitle("Type");
    submenutype.addAction("Vectors", this, &UiMainWindow::setGridTypeVectors);
    submenutype.addAction("Dots", this, &UiMainWindow::setGridTypeDots);
    actionList = submenutype.actions();
    if(devParms.displaytype == 0) {
        actionList[0]->setCheckable(true);
        actionList[0]->setChecked(true);
    } else {
        actionList[1]->setCheckable(true);
        actionList[1]->setChecked(true);
    }
    menu.addMenu(&submenutype);

    submenugrid.setTitle("Grid");
    submenugrid.addAction("Full", this, &UiMainWindow::setGridFull);
    submenugrid.addAction("Half", this, &UiMainWindow::setGridHalf);
    submenugrid.addAction("None", this, &UiMainWindow::setGridNone);
    actionList = submenugrid.actions();
    if(devParms.displaygrid == 2) {
        actionList[0]->setCheckable(true);
        actionList[0]->setChecked(true);
    } else if(devParms.displaygrid == 1) {
        actionList[1]->setCheckable(true);
        actionList[1]->setChecked(true);
    } else if(devParms.displaygrid == 0) {
        actionList[2]->setCheckable(true);
        actionList[2]->setChecked(true);
    }
    menu.addMenu(&submenugrid);

    submenugrading.setTitle("Persistence");
    submenugrading.addAction("Minimum", this, &UiMainWindow::setGradingMin);
    //   submenugrading.addAction("0.05",     this, &UI_Mainwindow::set_grading_005);
    submenugrading.addAction("0.1", this, &UiMainWindow::setGradingX01);
    submenugrading.addAction("0.2", this, &UiMainWindow::setGradingX02);
    submenugrading.addAction("0.5", this, &UiMainWindow::setGradingX05);
    submenugrading.addAction("1", this, &UiMainWindow::setGradingX1);
    submenugrading.addAction("2", this, &UiMainWindow::setGradingX2);
    submenugrading.addAction("5", this, &UiMainWindow::setGradingX5);
    //   submenugrading.addAction("10",       this, &UI_Mainwindow::set_grading_10);
    //   submenugrading.addAction("20",       this, &UI_Mainwindow::set_grading_20);
    submenugrading.addAction("Infinite", this, &UiMainWindow::setGradingInf);
    actionList = submenugrading.actions();
    if(devParms.displaygrading == 0) {
        actionList[0]->setCheckable(true);
        actionList[0]->setChecked(true);
    } else if(devParms.displaygrading == 1) {
        actionList[1]->setCheckable(true);
        actionList[1]->setChecked(true);
    } else if(devParms.displaygrading == 2) {
        actionList[2]->setCheckable(true);
        actionList[2]->setChecked(true);
    } else if(devParms.displaygrading == 5) {
        actionList[3]->setCheckable(true);
        actionList[3]->setChecked(true);
    } else if(devParms.displaygrading == 10) {
        actionList[4]->setCheckable(true);
        actionList[4]->setChecked(true);
    } else if(devParms.displaygrading == 20) {
        actionList[5]->setCheckable(true);
        actionList[5]->setChecked(true);
    } else if(devParms.displaygrading == 50) {
        actionList[6]->setCheckable(true);
        actionList[6]->setChecked(true);
    } else if(devParms.displaygrading == 10000) {
        actionList[7]->setCheckable(true);
        actionList[7]->setChecked(true);
    }
    menu.addMenu(&submenugrading);

    menu.exec(dispButton->mapToGlobal(QPoint(0, 0)));
}

void UiMainWindow::setGridTypeVectors() {
    if(!devParms.displaytype)
        return;

    devParms.displaytype = 0;

    statusLabel->setText("Display type: vectors");
    setCueCmd(":DISP:TYPE VECT");
}

void UiMainWindow::setGridTypeDots() {
    if(devParms.displaytype)
        return;

    devParms.displaytype = 1;

    statusLabel->setText("Display type: dots");
    setCueCmd(":DISP:TYPE DOTS");
}

void UiMainWindow::setGridFull() {
    if(devParms.displaygrid == 2)
        return;

    devParms.displaygrid = 2;

    statusLabel->setText("Display grid: full");
    setCueCmd(":DISP:GRID FULL");
}

void UiMainWindow::setGridHalf() {
    if(devParms.displaygrid == 1)
        return;

    devParms.displaygrid = 1;

    statusLabel->setText("Display grid: half");
    setCueCmd(":DISP:GRID HALF");
}

void UiMainWindow::setGridNone() {
    if(devParms.displaygrid == 0)
        return;

    devParms.displaygrid = 0;

    statusLabel->setText("Display grid: none");
    setCueCmd(":DISP:GRID NONE");
}

void UiMainWindow::setGradingMin() {
    if(devParms.displaygrading == 0)
        return;

    devParms.displaygrading = 0;

    statusLabel->setText("Display grading: Minimum");
    setCueCmd(":DISP:GRAD:TIME MIN");
}

void UiMainWindow::setGradingX005() {
    statusLabel->setText("Display grading: 0.05 Sec.");
    setCueCmd(":DISP:GRAD:TIME 0.05");
}

void UiMainWindow::setGradingX01() {
    if(devParms.displaygrading == 1)
        return;

    devParms.displaygrading = 1;

    statusLabel->setText("Display grading: 0.1 Sec.");
    setCueCmd(":DISP:GRAD:TIME 0.1");
}

void UiMainWindow::setGradingX02() {
    if(devParms.displaygrading == 2)
        return;

    devParms.displaygrading = 2;

    statusLabel->setText("Display grading: 0.2 Sec.");
    setCueCmd(":DISP:GRAD:TIME 0.2");
}

void UiMainWindow::setGradingX05() {
    if(devParms.displaygrading == 5)
        return;

    devParms.displaygrading = 5;

    statusLabel->setText("Display grading: 0.5 Sec.");
    setCueCmd(":DISP:GRAD:TIME 0.5");
}

void UiMainWindow::setGradingX1() {
    if(devParms.displaygrading == 10)
        return;

    devParms.displaygrading = 10;

    statusLabel->setText("Display grading: 1 Sec.");
    setCueCmd(":DISP:GRAD:TIME 1");
}

void UiMainWindow::setGradingX2() {
    if(devParms.displaygrading == 20)
        return;

    devParms.displaygrading = 20;

    statusLabel->setText("Display grading: 2 Sec.");
    setCueCmd(":DISP:GRAD:TIME 2");
}

void UiMainWindow::setGradingX5() {
    if(devParms.displaygrading == 50)
        return;

    devParms.displaygrading = 50;

    statusLabel->setText("Display grading: 5 Sec.");
    setCueCmd(":DISP:GRAD:TIME 5");
}

void UiMainWindow::setGradingX10() {
    statusLabel->setText("Display grading: 10 Sec.");
    setCueCmd(":DISP:GRAD:TIME 10");
}

void UiMainWindow::setGradingX20() {
    statusLabel->setText("Display grading: 20 Sec.");
    setCueCmd(":DISP:GRAD:TIME 20");
}

void UiMainWindow::setGradingInf() {
    if(devParms.displaygrading == 10000)
        return;

    devParms.displaygrading = 10000;

    statusLabel->setText("Display grading: Infinite");
    setCueCmd(":DISP:GRAD:TIME INF");
}

void UiMainWindow::utilButtonClicked() {
    QMenu menu;

    menu.addAction("Record", this, &UiMainWindow::showPlaybackWindow);

    menu.exec(utilButton->mapToGlobal(QPoint(0, 0)));
}

void UiMainWindow::showPlaybackWindow() {
    UI_playback_window w{this};
}

void UiMainWindow::playpauseButtonClicked() {
    if(devParms.funcWrecEnable == 0) return;
    if(devParms.funcWrecOperate) return;
    if(devParms.funcHasRecord == 0) return;
    if(devParms.funcWplayOperate == 1) {
        devParms.funcWplayOperate = 2;

        statusLabel->setText("Replay paused");
        setCueCmd(":FUNC:WREP:OPER PAUS");
    } else {
        if((devParms.modelSerie != 1) && (devParms.funcWrecEnable == 1)) {
            setCueCmd(":FUNC:WRM PLAY");

            devParms.funcWrecEnable = 2;
        }

        devParms.funcWplayOperate = 1;

        devParms.funcWplayFcur = 0;

        statusLabel->setText("Replay on");
        setCueCmd(":FUNC:WREP:OPER PLAY");
    }
}

void UiMainWindow::stopButtonClicked() {
    if(devParms.funcWrecEnable == 0) return;

    if(devParms.funcWrecOperate) {
        statusLabel->setText("Record off");
        setCueCmd(":FUNC:WREC:OPER STOP");
    }

    if(devParms.funcWplayOperate) {
        statusLabel->setText("Replay off");
        setCueCmd(":FUNC:WREP:OPER STOP");
    }
}

void UiMainWindow::recordButtonClicked() {
    if(devParms.funcWrecEnable == 0) return;

    if(devParms.funcWplayOperate) return;

    if(devParms.funcWrecOperate) return;

    if(devParms.funcWrecEnable == 2) // DS6000 series play mode
    {
        setCueCmd(":FUNC:WRM REC");

        devParms.funcWrecEnable = 1; // DS6000 series record mode
    }

    statusLabel->setText("Record on");

    if(devParms.modelSerie != 1) setCueCmd(":FUNC:WREC:OPER REC");
    else setCueCmd(":FUNC:WREC:OPER RUN");

    devParms.funcHasRecord = 1;
}

void UiMainWindow::helpButtonClicked() {
    showHowtoOperate();
}

void UiMainWindow::showHowtoOperate() {
    QMessageBox msgBox;
    msgBox.setStandardButtons(QMessageBox::Close);
    msgBox.setText(
        "Use the mousewheel to change the dials. In order to simulate a push on a dial,"
        "click on it with the right mouse button.\n"
        "To toggle the delayed timebase, right-click on the timebase dial.\n"
        "To set the horizontal position to zero, right-click on the horizontal position dial.\n"
        "To set the vertical offset to zero, right-click on the vertical position dial.\n\n"
        "In addition of using the dials to change the scale and offset of the traces and the trigger position,"
        "you can use the mouse to drag the colored arrows aside of the plot.\n\n"
        "Keyboard shortcuts:\n"
        "PageUp: move traces 12 (or 14) divisions to the right.\n"
        "PageDn: move traces 12 (or 14) divisions to the left.\n"
        "Arrow left: move traces 1 division to the right.\n"
        "Arrow right: move traces 1 division to the left.\n"
        "Arrow up: move active trace 1 division up.\n"
        "Arrow down: move active trace 1 division down.\n"
        "Zoom In (decrease timebase): Ctl+\n"
        "Zoom Out (increase timebase): Ctl-\n"
        "Increase vertical scale: -\n"
        "Decrease vertical scale: +\n"
        "Increase vertical scale for all active channels: Shift-\n"
        "Decrease vertical scale for all active channels: Shift+\n"
        "Press '1' to select or deselect channel 1\n"
        "Press '2' to select or deselect channel 2, etc.\n"
        "Press 'c' to center the horizontal position.\n"
        "Press 't' to center the trigger position.\n"
        "Press 'f' to toggle FFT.\n"
        "Press 'ctrl+p' to save a screenshot.\n");

    msgBox.exec();
}

void UiMainWindow::showAboutDialog() {
    UiAboutwindow aboutwindow;
}

void UiMainWindow::vertScaleDialClicked(QPoint) {
    int chn;

    char str[512];
    if(devParms.activechannel < 0)
        return;

    chn = devParms.activechannel;

    if(devParms.chan[chn].vernier) {
        devParms.chan[chn].vernier = 0;

        snprintf(str, 512, "Channel %i vernier: off", chn + 1);

        statusLabel->setText(str);

        snprintf(str, 512, ":CHAN%i:VERN 0", chn + 1);
        setCueCmd(str);
    } else {
        devParms.chan[chn].vernier = 1;

        snprintf(str, 512, "Channel %i vernier: on", chn + 1);

        statusLabel->setText(str);

        snprintf(str, 512, ":CHAN%i:VERN 1", chn + 1);
        setCueCmd(str);
    }
}

void UiMainWindow::ch1ButtonClicked() {
    if(devParms.chan[0].Display) {
        if(devParms.activechannel == 0) {
            devParms.chan[0].Display = 0;
            statusLabel->setText("Channel 1 off");
            setCueCmd(":CHAN1:DISP 0");
            ch1Button->setStyleSheet(defStylesh);
            devParms.activechannel = -1;
            for(int i{}; i < MAX_CHNS; i++) {
                if(devParms.chan[i].Display) {
                    devParms.activechannel = i;
                    break;
                }
            }
        } else {
            devParms.activechannel = 0;
        }
    } else {
        devParms.chan[0].Display = 1;
        statusLabel->setText("Channel 1 on");
        setCueCmd(":CHAN1:DISP 1");
        ch1Button->setStyleSheet("background: #FFFF33; color: black;");
        devParms.activechannel = 0;
    }
}

void UiMainWindow::ch2ButtonClicked() {
    if(devParms.channelCnt < 2)
        return;

    if(devParms.chan[1].Display) {
        if(devParms.activechannel == 1) {
            devParms.chan[1].Display = 0;
            statusLabel->setText("Channel 2 off");
            setCueCmd(":CHAN2:DISP 0");
            ch2Button->setStyleSheet(defStylesh);
            devParms.activechannel = -1;
            for(int i{}; i < MAX_CHNS; i++) {
                if(devParms.chan[i].Display) {
                    devParms.activechannel = i;

                    break;
                }
            }
        } else {
            devParms.activechannel = 1;
        }
    } else {
        devParms.chan[1].Display = 1;
        statusLabel->setText("Channel 2 on");
        setCueCmd(":CHAN2:DISP 1");
        ch2Button->setStyleSheet("background: #33FFFF; color: black;");
        devParms.activechannel = 1;
    }
}

void UiMainWindow::ch3ButtonClicked() {
    if(devParms.channelCnt < 3)
        return;

    if(devParms.chan[2].Display) {
        if(devParms.activechannel == 2) {
            devParms.chan[2].Display = 0;
            statusLabel->setText("Channel 3 off");
            setCueCmd(":CHAN3:DISP 0");
            ch3Button->setStyleSheet(defStylesh);
            devParms.activechannel = -1;
            for(int i{}; i < MAX_CHNS; i++) {
                if(devParms.chan[i].Display) {
                    devParms.activechannel = i;

                    break;
                }
            }
        } else {
            devParms.activechannel = 2;
        }
    } else {
        devParms.chan[2].Display = 1;
        statusLabel->setText("Channel 3 on");
        setCueCmd(":CHAN3:DISP 1");
        ch3Button->setStyleSheet("background: #FF33FF; color: black;");
        devParms.activechannel = 2;
    }
}

void UiMainWindow::ch4ButtonClicked() {
    if(devParms.channelCnt < 4)
        return;

    if(devParms.chan[3].Display) {
        if(devParms.activechannel == 3) {
            devParms.chan[3].Display = 0;
            statusLabel->setText("Channel 4 off");
            setCueCmd(":CHAN4:DISP 0");
            ch4Button->setStyleSheet(defStylesh);
            devParms.activechannel = -1;
            for(int i{}; i < MAX_CHNS; i++) {
                if(devParms.chan[i].Display) {
                    devParms.activechannel = i;

                    break;
                }
            }
        } else {
            devParms.activechannel = 3;
        }
    } else {
        devParms.chan[3].Display = 1;
        statusLabel->setText("Channel 4 on");
        setCueCmd(":CHAN4:DISP 1");
        ch4Button->setStyleSheet("background: #0066CC; color: black;");
        devParms.activechannel = 3;
    }
}

void UiMainWindow::chanMenu() {
    QMenu menu,
        submenubwl,
        submenucoupling,
        submenuinvert,
        submenuprobe,
        submenuunit;

    QList<QAction*> actionList;

    if((devParms.activechannel < 0) || (devParms.activechannel > MAX_CHNS))
        return;

    submenucoupling.setTitle("Coupling");
    submenucoupling.addAction("AC", this, &UiMainWindow::chanCouplingAc);
    submenucoupling.addAction("DC", this, &UiMainWindow::chanCouplingDc);
    submenucoupling.addAction("GND", this, &UiMainWindow::chanCouplingGnd);
    actionList = submenucoupling.actions();
    if(devParms.chan[devParms.activechannel].coupling == Coup::GND) {
        actionList[2]->setCheckable(true);
        actionList[2]->setChecked(true);
    } else if(devParms.chan[devParms.activechannel].coupling == Coup::DC) {
        actionList[1]->setCheckable(true);
        actionList[1]->setChecked(true);
    } else if(devParms.chan[devParms.activechannel].coupling == Coup::AC) {
        actionList[0]->setCheckable(true);
        actionList[0]->setChecked(true);
    }
    menu.addMenu(&submenucoupling);

    submenubwl.setTitle("BWL");
    submenubwl.addAction("Off", this, &UiMainWindow::chanBwlOff);
    submenubwl.addAction("20MHz", this, &UiMainWindow::chanBwl20MHz);
    if(devParms.modelSerie == 4) {
        if(devParms.bandwidth >= 200)
            submenubwl.addAction("100MHz", this, &UiMainWindow::chanBwl100MHz);

        if(devParms.bandwidth >= 300)
            submenubwl.addAction("200MHz", this, &UiMainWindow::chanBwl200MHz);
    }
    if(devParms.modelSerie == 6)
        submenubwl.addAction("250MHz", this, &UiMainWindow::chanBwl250MHz);
    actionList = submenubwl.actions();
    if(devParms.chan[devParms.activechannel].bwlimit == 0) {
        actionList[0]->setCheckable(true);
        actionList[0]->setChecked(true);
    } else if(devParms.chan[devParms.activechannel].bwlimit == 20) {
        actionList[1]->setCheckable(true);
        actionList[1]->setChecked(true);
    } else if(devParms.modelSerie == 4) {
        if(devParms.chan[devParms.activechannel].bwlimit == 100) {
            actionList[2]->setCheckable(true);
            actionList[2]->setChecked(true);
        }

        if(devParms.chan[devParms.activechannel].bwlimit == 200) {
            actionList[3]->setCheckable(true);
            actionList[3]->setChecked(true);
        }
    } else if(devParms.modelSerie == 6) {
        if(devParms.chan[devParms.activechannel].bwlimit == 250) {
            actionList[2]->setCheckable(true);
            actionList[2]->setChecked(true);
        }
    }
    menu.addMenu(&submenubwl);

    submenuprobe.setTitle("Probe");
    if(devParms.modelSerie != 6) {
        submenuprobe.addAction("0.01X", this, &UiMainWindow::chanProbeX001);
        submenuprobe.addAction("0.02X", this, &UiMainWindow::chanProbeX002);
        submenuprobe.addAction("0.05X", this, &UiMainWindow::chanProbeX005);
    }
    submenuprobe.addAction("0.1X", this, &UiMainWindow::chanProbeX01);
    if(devParms.modelSerie != 6) {
        submenuprobe.addAction("0.2X", this, &UiMainWindow::chanProbeX02);
        submenuprobe.addAction("0.5X", this, &UiMainWindow::chanProbeX05);
    }
    submenuprobe.addAction("1X", this, &UiMainWindow::chanProbeX1);
    if(devParms.modelSerie != 6) {
        submenuprobe.addAction("2X", this, &UiMainWindow::chanProbeX2);
        submenuprobe.addAction("5X", this, &UiMainWindow::chanProbeX5);
    }
    submenuprobe.addAction("10X", this, &UiMainWindow::chanProbeX10);
    if(devParms.modelSerie != 6) {
        submenuprobe.addAction("20X", this, &UiMainWindow::chanProbeX20);
        submenuprobe.addAction("50X", this, &UiMainWindow::chanProbeX50);
    }
    submenuprobe.addAction("100X", this, &UiMainWindow::chanProbeX100);
    if(devParms.modelSerie != 6) {
        submenuprobe.addAction("200X", this, &UiMainWindow::chanProbeX200);
        submenuprobe.addAction("500X", this, &UiMainWindow::chanProbeX500);
        submenuprobe.addAction("1000X", this, &UiMainWindow::chanProbeX1000);
    }
    actionList = submenuprobe.actions();
    if(devParms.modelSerie != 6) {
        if(!dblcmp(devParms.chan[devParms.activechannel].probe, 0.01)) {
            actionList[0]->setCheckable(true);
            actionList[0]->setChecked(true);
        } else if(!dblcmp(devParms.chan[devParms.activechannel].probe, 0.02)) {
            actionList[1]->setCheckable(true);
            actionList[1]->setChecked(true);
        } else if(!dblcmp(devParms.chan[devParms.activechannel].probe, 0.05)) {
            actionList[2]->setCheckable(true);
            actionList[2]->setChecked(true);
        } else if(!dblcmp(devParms.chan[devParms.activechannel].probe, 0.1)) {
            actionList[3]->setCheckable(true);
            actionList[3]->setChecked(true);
        } else if(!dblcmp(devParms.chan[devParms.activechannel].probe, 0.2)) {
            actionList[4]->setCheckable(true);
            actionList[4]->setChecked(true);
        } else if(!dblcmp(devParms.chan[devParms.activechannel].probe, 0.5)) {
            actionList[5]->setCheckable(true);
            actionList[5]->setChecked(true);
        } else if(!dblcmp(devParms.chan[devParms.activechannel].probe, 1)) {
            actionList[6]->setCheckable(true);
            actionList[6]->setChecked(true);
        } else if(!dblcmp(devParms.chan[devParms.activechannel].probe, 2)) {
            actionList[7]->setCheckable(true);
            actionList[7]->setChecked(true);
        } else if(!dblcmp(devParms.chan[devParms.activechannel].probe, 5)) {
            actionList[8]->setCheckable(true);
            actionList[8]->setChecked(true);
        } else if(!dblcmp(devParms.chan[devParms.activechannel].probe, 10)) {
            actionList[9]->setCheckable(true);
            actionList[9]->setChecked(true);
        } else if(!dblcmp(devParms.chan[devParms.activechannel].probe, 20)) {
            actionList[10]->setCheckable(true);
            actionList[10]->setChecked(true);
        } else if(!dblcmp(devParms.chan[devParms.activechannel].probe, 50)) {
            actionList[11]->setCheckable(true);
            actionList[11]->setChecked(true);
        } else if(!dblcmp(devParms.chan[devParms.activechannel].probe, 100)) {
            actionList[12]->setCheckable(true);
            actionList[12]->setChecked(true);
        } else if(!dblcmp(devParms.chan[devParms.activechannel].probe, 200)) {
            actionList[13]->setCheckable(true);
            actionList[13]->setChecked(true);
        } else if(!dblcmp(devParms.chan[devParms.activechannel].probe, 500)) {
            actionList[14]->setCheckable(true);
            actionList[14]->setChecked(true);
        } else if(!dblcmp(devParms.chan[devParms.activechannel].probe, 1000)) {
            actionList[15]->setCheckable(true);
            actionList[15]->setChecked(true);
        }
    } else if(!dblcmp(devParms.chan[devParms.activechannel].probe, 0.1)) {
        actionList[0]->setCheckable(true);
        actionList[0]->setChecked(true);
    } else if(!dblcmp(devParms.chan[devParms.activechannel].probe, 1)) {
        actionList[1]->setCheckable(true);
        actionList[1]->setChecked(true);
    } else if(!dblcmp(devParms.chan[devParms.activechannel].probe, 10)) {
        actionList[2]->setCheckable(true);
        actionList[2]->setChecked(true);
    } else if(!dblcmp(devParms.chan[devParms.activechannel].probe, 100)) {
        actionList[3]->setCheckable(true);
        actionList[3]->setChecked(true);
    }
    menu.addMenu(&submenuprobe);

    submenuinvert.setTitle("Invert");
    submenuinvert.addAction("On", this, &UiMainWindow::chanInvertOn);
    submenuinvert.addAction("Off", this, &UiMainWindow::chanInvertOff);
    actionList = submenuinvert.actions();
    if(devParms.chan[devParms.activechannel].invert == 1) {
        actionList[0]->setCheckable(true);
        actionList[0]->setChecked(true);
    } else {
        actionList[1]->setCheckable(true);
        actionList[1]->setChecked(true);
    }
    menu.addMenu(&submenuinvert);

    submenuunit.setTitle("Unit");
    submenuunit.addAction("Volt", this, &UiMainWindow::chanUnitV);
    submenuunit.addAction("Watt", this, &UiMainWindow::chanUnitW);
    submenuunit.addAction("Ampere", this, &UiMainWindow::chanUnitA);
    submenuunit.addAction("Unknown", this, &UiMainWindow::chanUnitU);
    actionList = submenuunit.actions();
    actionList[devParms.chan[devParms.activechannel].unit]->setCheckable(true);
    actionList[devParms.chan[devParms.activechannel].unit]->setChecked(true);
    menu.addMenu(&submenuunit);

    menu.exec(chanMenuButton->mapToGlobal(QPoint(0, 0)));
}

void UiMainWindow::mathMenu() {
    char str[512];
    double val;

    QMenu menu,
        submenufft,
        submenufftctr,
        submenuffthzdiv,
        submenufftsrc,
        submenufftvscale,
        submenufftoffset;

    QList<QAction*> actionList;

    if((devParms.activechannel < 0) || (devParms.activechannel > MAX_CHNS))
        return;

    if(devParms.timebasedelayenable)
        val = 100.0 / devParms.timebasedelayscale;
    else
        val = 100.0 / devParms.timebasescale;

    submenufftctr.setTitle("Center");
    convertToMetricSuffix(str, devParms.mathFftHscale * 5.0, 1, 512);
    strlcat(str, "Hz", 512);
    submenufftctr.addAction(str, this, &UiMainWindow::selectFftCtr5);
    convertToMetricSuffix(str, devParms.mathFftHscale * 6.0, 1, 512);
    strlcat(str, "Hz", 512);
    submenufftctr.addAction(str, this, &UiMainWindow::selectFftCtr6);
    convertToMetricSuffix(str, devParms.mathFftHscale * 7.0, 1, 512);
    strlcat(str, "Hz", 512);
    submenufftctr.addAction(str, this, &UiMainWindow::selectFftCtr7);
    convertToMetricSuffix(str, devParms.mathFftHscale * 8.0, 1, 512);
    strlcat(str, "Hz", 512);
    submenufftctr.addAction(str, this, &UiMainWindow::selectFftCtr8);
    if((devParms.mathFftHscale * 9.0) < (val * 0.40001)) {
        convertToMetricSuffix(str, devParms.mathFftHscale * 9.0, 1, 512);
        strlcat(str, "Hz", 512);
        submenufftctr.addAction(str, this, &UiMainWindow::selectFftCtr9);
        if((devParms.mathFftHscale * 10.0) < (val * 0.40001)) {
            convertToMetricSuffix(str, devParms.mathFftHscale * 10.0, 1, 512);
            strlcat(str, "Hz", 512);
            submenufftctr.addAction(str, this, &UiMainWindow::selectFftCtr10);
            if((devParms.mathFftHscale * 11.0) < (val * 0.40001)) {
                convertToMetricSuffix(str, devParms.mathFftHscale * 11.0, 1, 512);
                strlcat(str, "Hz", 512);
                submenufftctr.addAction(str, this, &UiMainWindow::selectFftCtr11);
                if((devParms.mathFftHscale * 12.0) < (val * 0.40001)) {
                    convertToMetricSuffix(str, devParms.mathFftHscale * 12.0, 1, 512);
                    strlcat(str, "Hz", 512);
                    submenufftctr.addAction(str, this, &UiMainWindow::selectFftCtr12);
                }
            }
        }
    }

    submenuffthzdiv.setTitle("Hz/Div");
    //   if(devParms.modelserie == 6)
    //   {
    //     convertToMetricSuffix(str, val / 40.0 , 2);
    //     strlcat(str, "Hz/Div", 512);
    //     submenuffthzdiv.addAction(str, this, &UIMainWindow::selectFftHzdiv40);
    //     convertToMetricSuffix(str, val / 80.0 , 2);
    //     strlcat(str, "Hz/Div", 512);
    //     submenuffthzdiv.addAction(str, this, &UIMainWindow::selectFftHzdiv80);
    //     convertToMetricSuffix(str, val / 200.0 , 2);
    //     strlcat(str, "Hz/Div", 512);
    //     submenuffthzdiv.addAction(str, this, &UIMainWindow::selectFftHzdiv200);
    //   }
    //   else
    //   {
    convertToMetricSuffix(str, val / 20.0, 2, 512);
    strlcat(str, "Hz/Div", 512);
    submenuffthzdiv.addAction(str, this, &UiMainWindow::selectFftHzdiv20);
    convertToMetricSuffix(str, val / 40.0, 2, 512);
    strlcat(str, "Hz/Div", 512);
    submenuffthzdiv.addAction(str, this, &UiMainWindow::selectFftHzdiv40);
    convertToMetricSuffix(str, val / 100.0, 2, 512);
    strlcat(str, "Hz/Div", 512);
    submenuffthzdiv.addAction(str, this, &UiMainWindow::selectFftHzdiv100);
    convertToMetricSuffix(str, val / 200.0, 2, 512);
    strlcat(str, "Hz/Div", 512);
    submenuffthzdiv.addAction(str, this, &UiMainWindow::selectFftHzdiv200);
    //  }

    submenufftoffset.setTitle("Offset");
    if(devParms.mathFftUnit == 0) {
        convertToMetricSuffix(str, devParms.fftVScale * 4.0, 1, 512);
        strlcat(str, "V", 512);
        submenufftoffset.addAction(str, this, &UiMainWindow::selectFftVOffsetp4);
        convertToMetricSuffix(str, devParms.fftVScale * 3.0, 1, 512);
        strlcat(str, "V", 512);
        submenufftoffset.addAction(str, this, &UiMainWindow::selectFftVOffsetp3);
        convertToMetricSuffix(str, devParms.fftVScale * 2.0, 1, 512);
        strlcat(str, "V", 512);
        submenufftoffset.addAction(str, this, &UiMainWindow::selectFftVOffsetp2);
        convertToMetricSuffix(str, devParms.fftVScale, 1, 512);
        strlcat(str, "V", 512);
        submenufftoffset.addAction(str, this, &UiMainWindow::selectFftVOffsetp1);
        strlcpy(str, "0V", 512);
        submenufftoffset.addAction(str, this, &UiMainWindow::selectFftVOffset0);
        convertToMetricSuffix(str, devParms.fftVScale * -1.0, 1, 512);
        strlcat(str, "V", 512);
        submenufftoffset.addAction(str, this, &UiMainWindow::selectFftVOffsetm1);
        convertToMetricSuffix(str, devParms.fftVScale * -2.0, 1, 512);
        strlcat(str, "V", 512);
        submenufftoffset.addAction(str, this, &UiMainWindow::selectFftVOffsetm2);
        convertToMetricSuffix(str, devParms.fftVScale * -3.0, 1, 512);
        strlcat(str, "V", 512);
        submenufftoffset.addAction(str, this, &UiMainWindow::selectFftVOffsetm3);
        convertToMetricSuffix(str, devParms.fftVScale * -4.0, 1, 512);
        strlcat(str, "V", 512);
        submenufftoffset.addAction(str, this, &UiMainWindow::selectFftVOffsetm4);
    } else {
        snprintf(str, 512, "%+.0fdB", devParms.fftVScale * 4.0);
        submenufftoffset.addAction(str, this, &UiMainWindow::selectFftVOffsetp4);
        snprintf(str, 512, "%+.0fdB", devParms.fftVScale * 3.0);
        submenufftoffset.addAction(str, this, &UiMainWindow::selectFftVOffsetp3);
        snprintf(str, 512, "%+.0fdB", devParms.fftVScale * 2.0);
        submenufftoffset.addAction(str, this, &UiMainWindow::selectFftVOffsetp2);
        snprintf(str, 512, "%+.0fdB", devParms.fftVScale);
        submenufftoffset.addAction(str, this, &UiMainWindow::selectFftVOffsetp1);
        strlcpy(str, "0dB", 512);
        submenufftoffset.addAction(str, this, &UiMainWindow::selectFftVOffset0);
        snprintf(str, 512, "%.0fdB", devParms.fftVScale * -1.0);
        submenufftoffset.addAction(str, this, &UiMainWindow::selectFftVOffsetm1);
        snprintf(str, 512, "%.0fdB", devParms.fftVScale * -2.0);
        submenufftoffset.addAction(str, this, &UiMainWindow::selectFftVOffsetm2);
        snprintf(str, 512, "%.0fdB", devParms.fftVScale * -3.0);
        submenufftoffset.addAction(str, this, &UiMainWindow::selectFftVOffsetm3);
        snprintf(str, 512, "%.0fdB", devParms.fftVScale * -4.0);
        submenufftoffset.addAction(str, this, &UiMainWindow::selectFftVOffsetm4);
    }

    submenufftvscale.setTitle("Scale");
    if(devParms.mathFftUnit == 0) {
        submenufftvscale.addAction("1V/Div", this, &UiMainWindow::selectFftVScale1);
        submenufftvscale.addAction("2V/Div", this, &UiMainWindow::selectFftVScale2);
        submenufftvscale.addAction("5V/Div", this, &UiMainWindow::selectFftVScale5);
        submenufftvscale.addAction("10V/Div", this, &UiMainWindow::selectFftVScale10);
        submenufftvscale.addAction("20V/Div", this, &UiMainWindow::selectFftVScale20);
    } else {
        submenufftvscale.addAction("1dB/Div", this, &UiMainWindow::selectFftVScale1);
        submenufftvscale.addAction("2dB/Div", this, &UiMainWindow::selectFftVScale2);
        submenufftvscale.addAction("5dB/Div", this, &UiMainWindow::selectFftVScale5);
        submenufftvscale.addAction("10dB/Div", this, &UiMainWindow::selectFftVScale10);
        submenufftvscale.addAction("20dB/Div", this, &UiMainWindow::selectFftVScale20);
    }

    submenufftsrc.setTitle("Source");
    submenufftsrc.addAction("CH1", this, &UiMainWindow::selectFftCh1);
    submenufftsrc.addAction("CH2", this, &UiMainWindow::selectFftCh2);
    if(devParms.channelCnt > 2) {
        submenufftsrc.addAction("CH3", this, &UiMainWindow::selectFftCh3);
        submenufftsrc.addAction("CH4", this, &UiMainWindow::selectFftCh4);
    }
    actionList = submenufftsrc.actions();
    if(devParms.mathFftSrc == 0) {
        actionList[0]->setCheckable(true);
        actionList[0]->setChecked(true);
    } else if(devParms.mathFftSrc == 1) {
        actionList[1]->setCheckable(true);
        actionList[1]->setChecked(true);
    } else if(devParms.mathFftSrc == 2) {
        actionList[2]->setCheckable(true);
        actionList[2]->setChecked(true);
    } else if(devParms.mathFftSrc == 3) {
        actionList[3]->setCheckable(true);
        actionList[3]->setChecked(true);
    }

    submenufft.setTitle("FFT");
    submenufft.addAction("On", this, &UiMainWindow::toggleFft);
    submenufft.addAction("Off", this, &UiMainWindow::toggleFft);
    submenufft.addAction("Full", this, &UiMainWindow::toggleFftSplit);
    submenufft.addAction("Half", this, &UiMainWindow::toggleFftSplit);
    submenufft.addAction("Vrms", this, &UiMainWindow::toggleFftUnit);
    submenufft.addAction("dB/dBm", this, &UiMainWindow::toggleFftUnit);
    submenufft.addMenu(&submenufftsrc);
    submenufft.addMenu(&submenufftctr);
    submenufft.addMenu(&submenuffthzdiv);
    submenufft.addMenu(&submenufftoffset);
    submenufft.addMenu(&submenufftvscale);
    actionList = submenufft.actions();
    if(devParms.mathFft == 1) {
        actionList[0]->setCheckable(true);
        actionList[0]->setChecked(true);
    } else {
        actionList[1]->setCheckable(true);
        actionList[1]->setChecked(true);
    }
    if(devParms.mathFftSplit == 0) {
        actionList[2]->setCheckable(true);
        actionList[2]->setChecked(true);
    } else {
        actionList[3]->setCheckable(true);
        actionList[3]->setChecked(true);
    }
    if(devParms.mathFftUnit == 0) {
        actionList[4]->setCheckable(true);
        actionList[4]->setChecked(true);
    } else {
        actionList[5]->setCheckable(true);
        actionList[5]->setChecked(true);
    }

    menu.addMenu(&submenufft);

    menu.addAction("Decode", this, &UiMainWindow::showDecodeWindow);

    menu.exec(mathMenuButton->mapToGlobal(QPoint(0, 0)));
}

void UiMainWindow::chanCouplingAc() {
    char str[512];
    devParms.chan[devParms.activechannel].coupling = Coup::AC;
    snprintf(str, 512, "Channel %i coupling: AC", devParms.activechannel + 1);
    statusLabel->setText(str);
    snprintf(str, 512, ":CHAN%i:COUP AC", devParms.activechannel + 1);
    setCueCmd(str);
    updateLabels();
}

void UiMainWindow::chanCouplingDc() {
    char str[512];
    devParms.chan[devParms.activechannel].coupling = Coup::DC;
    snprintf(str, 512, "Channel %i coupling: DC", devParms.activechannel + 1);
    statusLabel->setText(str);
    snprintf(str, 512, ":CHAN%i:COUP DC", devParms.activechannel + 1);
    setCueCmd(str);
    updateLabels();
}

void UiMainWindow::chanCouplingGnd() {
    char str[512];
    devParms.chan[devParms.activechannel].coupling = Coup::GND;
    snprintf(str, 512, "Channel %i coupling: GND", devParms.activechannel + 1);
    statusLabel->setText(str);
    snprintf(str, 512, ":CHAN%i:COUP GND", devParms.activechannel + 1);
    setCueCmd(str);
    updateLabels();
}

void UiMainWindow::chanUnitV() {
    char str[512];
    devParms.chan[devParms.activechannel].unit = 0;
    snprintf(str, 512, "Channel %i units: Volt", devParms.activechannel + 1);
    statusLabel->setText(str);
    snprintf(str, 512, ":CHAN%i:UNIT VOLT", devParms.activechannel + 1);
    setCueCmd(str);
}

void UiMainWindow::chanUnitW() {
    char str[512];
    devParms.chan[devParms.activechannel].unit = 1;

    snprintf(str, 512, "Channel %i units: Watt", devParms.activechannel + 1);

    statusLabel->setText(str);

    snprintf(str, 512, ":CHAN%i:UNIT WATT", devParms.activechannel + 1);
    setCueCmd(str);
}

void UiMainWindow::chanUnitA() {
    char str[512];
    devParms.chan[devParms.activechannel].unit = 2;

    snprintf(str, 512, "Channel %i units: Ampere", devParms.activechannel + 1);

    statusLabel->setText(str);

    snprintf(str, 512, ":CHAN%i:UNIT AMP", devParms.activechannel + 1);
    setCueCmd(str);
}

void UiMainWindow::chanUnitU() {
    char str[512];
    devParms.chan[devParms.activechannel].unit = 3;

    snprintf(str, 512, "Channel %i units: Unknown", devParms.activechannel + 1);

    statusLabel->setText(str);

    snprintf(str, 512, ":CHAN%i:UNIT UNKN", devParms.activechannel + 1);
    setCueCmd(str);
}

void UiMainWindow::chanProbeX001() {
    char str[512];
    devParms.chan[devParms.activechannel].scale /= devParms.chan[devParms.activechannel].probe;

    devParms.chan[devParms.activechannel].probe = 0.01;

    devParms.chan[devParms.activechannel].scale *= devParms.chan[devParms.activechannel].probe;

    snprintf(str, 512, "Channel %i probe: 0.01X", devParms.activechannel + 1);

    statusLabel->setText(str);

    snprintf(str, 512, ":CHAN%i:PROB %e", devParms.activechannel + 1, devParms.chan[devParms.activechannel].probe);
    setCueCmd(str);
}

void UiMainWindow::chanProbeX002() {
    char str[512];
    devParms.chan[devParms.activechannel].scale /= devParms.chan[devParms.activechannel].probe;

    devParms.chan[devParms.activechannel].probe = 0.02;

    devParms.chan[devParms.activechannel].scale *= devParms.chan[devParms.activechannel].probe;

    snprintf(str, 512, "Channel %i probe: 0.02X", devParms.activechannel + 1);

    statusLabel->setText(str);

    snprintf(str, 512, ":CHAN%i:PROB %e", devParms.activechannel + 1, devParms.chan[devParms.activechannel].probe);
    setCueCmd(str);
}

void UiMainWindow::chanProbeX005() {
    char str[512];
    devParms.chan[devParms.activechannel].scale /= devParms.chan[devParms.activechannel].probe;

    devParms.chan[devParms.activechannel].probe = 0.05;

    devParms.chan[devParms.activechannel].scale *= devParms.chan[devParms.activechannel].probe;

    snprintf(str, 512, "Channel %i probe: 0.05X", devParms.activechannel + 1);

    statusLabel->setText(str);

    snprintf(str, 512, ":CHAN%i:PROB %e", devParms.activechannel + 1, devParms.chan[devParms.activechannel].probe);
    setCueCmd(str);
}

void UiMainWindow::chanProbeX01() {
    char str[512];
    devParms.chan[devParms.activechannel].scale /= devParms.chan[devParms.activechannel].probe;

    devParms.chan[devParms.activechannel].probe = 0.1;

    devParms.chan[devParms.activechannel].scale *= devParms.chan[devParms.activechannel].probe;

    snprintf(str, 512, "Channel %i probe: 0.1X", devParms.activechannel + 1);

    statusLabel->setText(str);

    snprintf(str, 512, ":CHAN%i:PROB %e", devParms.activechannel + 1, devParms.chan[devParms.activechannel].probe);
    setCueCmd(str);
}

void UiMainWindow::chanProbeX02() {
    char str[512];
    devParms.chan[devParms.activechannel].scale /= devParms.chan[devParms.activechannel].probe;

    devParms.chan[devParms.activechannel].probe = 0.2;

    devParms.chan[devParms.activechannel].scale *= devParms.chan[devParms.activechannel].probe;

    snprintf(str, 512, "Channel %i probe: 0.2X", devParms.activechannel + 1);

    statusLabel->setText(str);

    snprintf(str, 512, ":CHAN%i:PROB %e", devParms.activechannel + 1, devParms.chan[devParms.activechannel].probe);
    setCueCmd(str);
}

void UiMainWindow::chanProbeX05() {
    char str[512];
    devParms.chan[devParms.activechannel].scale /= devParms.chan[devParms.activechannel].probe;

    devParms.chan[devParms.activechannel].probe = 0.5;

    devParms.chan[devParms.activechannel].scale *= devParms.chan[devParms.activechannel].probe;

    snprintf(str, 512, "Channel %i probe: 0.5X", devParms.activechannel + 1);

    statusLabel->setText(str);

    snprintf(str, 512, ":CHAN%i:PROB %e", devParms.activechannel + 1, devParms.chan[devParms.activechannel].probe);
    setCueCmd(str);
}

void UiMainWindow::chanProbeX1() {
    char str[512];
    devParms.chan[devParms.activechannel].scale /= devParms.chan[devParms.activechannel].probe;

    devParms.chan[devParms.activechannel].probe = 1;

    devParms.chan[devParms.activechannel].scale *= devParms.chan[devParms.activechannel].probe;

    snprintf(str, 512, "Channel %i probe: 1X", devParms.activechannel + 1);

    statusLabel->setText(str);

    snprintf(str, 512, ":CHAN%i:PROB %e", devParms.activechannel + 1, devParms.chan[devParms.activechannel].probe);
    setCueCmd(str);
}

void UiMainWindow::chanProbeX2() {
    char str[512];
    devParms.chan[devParms.activechannel].scale /= devParms.chan[devParms.activechannel].probe;

    devParms.chan[devParms.activechannel].probe = 2;

    devParms.chan[devParms.activechannel].scale *= devParms.chan[devParms.activechannel].probe;

    snprintf(str, 512, "Channel %i probe: 2X", devParms.activechannel + 1);

    statusLabel->setText(str);

    snprintf(str, 512, ":CHAN%i:PROB %e", devParms.activechannel + 1, devParms.chan[devParms.activechannel].probe);
    setCueCmd(str);
}

void UiMainWindow::chanProbeX5() {
    char str[512];
    devParms.chan[devParms.activechannel].scale /= devParms.chan[devParms.activechannel].probe;

    devParms.chan[devParms.activechannel].probe = 5;

    devParms.chan[devParms.activechannel].scale *= devParms.chan[devParms.activechannel].probe;

    snprintf(str, 512, "Channel %i probe: 5X", devParms.activechannel + 1);

    statusLabel->setText(str);

    snprintf(str, 512, ":CHAN%i:PROB %e", devParms.activechannel + 1, devParms.chan[devParms.activechannel].probe);
    setCueCmd(str);
}

void UiMainWindow::chanProbeX10() {
    char str[512];
    devParms.chan[devParms.activechannel].scale /= devParms.chan[devParms.activechannel].probe;

    devParms.chan[devParms.activechannel].probe = 10;

    devParms.chan[devParms.activechannel].scale *= devParms.chan[devParms.activechannel].probe;

    snprintf(str, 512, "Channel %i probe: 10X", devParms.activechannel + 1);

    statusLabel->setText(str);

    snprintf(str, 512, ":CHAN%i:PROB %e", devParms.activechannel + 1, devParms.chan[devParms.activechannel].probe);
    setCueCmd(str);
}

void UiMainWindow::chanProbeX20() {
    char str[512];
    devParms.chan[devParms.activechannel].scale /= devParms.chan[devParms.activechannel].probe;

    devParms.chan[devParms.activechannel].probe = 20;

    devParms.chan[devParms.activechannel].scale *= devParms.chan[devParms.activechannel].probe;

    snprintf(str, 512, "Channel %i probe: 20X", devParms.activechannel + 1);

    statusLabel->setText(str);

    snprintf(str, 512, ":CHAN%i:PROB %e", devParms.activechannel + 1, devParms.chan[devParms.activechannel].probe);
    setCueCmd(str);
}

void UiMainWindow::chanProbeX50() {
    char str[512];
    devParms.chan[devParms.activechannel].scale /= devParms.chan[devParms.activechannel].probe;

    devParms.chan[devParms.activechannel].probe = 50;

    devParms.chan[devParms.activechannel].scale *= devParms.chan[devParms.activechannel].probe;

    snprintf(str, 512, "Channel %i probe: 50X", devParms.activechannel + 1);

    statusLabel->setText(str);

    snprintf(str, 512, ":CHAN%i:PROB %e", devParms.activechannel + 1, devParms.chan[devParms.activechannel].probe);
    setCueCmd(str);
}

void UiMainWindow::chanProbeX100() {
    char str[512];
    devParms.chan[devParms.activechannel].scale /= devParms.chan[devParms.activechannel].probe;

    devParms.chan[devParms.activechannel].probe = 100;

    devParms.chan[devParms.activechannel].scale *= devParms.chan[devParms.activechannel].probe;

    snprintf(str, 512, "Channel %i probe: 100X", devParms.activechannel + 1);

    statusLabel->setText(str);

    snprintf(str, 512, ":CHAN%i:PROB %e", devParms.activechannel + 1, devParms.chan[devParms.activechannel].probe);
    setCueCmd(str);
}

void UiMainWindow::chanProbeX200() {
    char str[512];
    devParms.chan[devParms.activechannel].scale /= devParms.chan[devParms.activechannel].probe;

    devParms.chan[devParms.activechannel].probe = 200;

    devParms.chan[devParms.activechannel].scale *= devParms.chan[devParms.activechannel].probe;

    snprintf(str, 512, "Channel %i probe: 200X", devParms.activechannel + 1);

    statusLabel->setText(str);

    snprintf(str, 512, ":CHAN%i:PROB %e", devParms.activechannel + 1, devParms.chan[devParms.activechannel].probe);
    setCueCmd(str);
}

void UiMainWindow::chanProbeX500() {
    char str[512];
    devParms.chan[devParms.activechannel].scale /= devParms.chan[devParms.activechannel].probe;

    devParms.chan[devParms.activechannel].probe = 500;

    devParms.chan[devParms.activechannel].scale *= devParms.chan[devParms.activechannel].probe;

    snprintf(str, 512, "Channel %i probe: 500X", devParms.activechannel + 1);

    statusLabel->setText(str);

    snprintf(str, 512, ":CHAN%i:PROB %e", devParms.activechannel + 1, devParms.chan[devParms.activechannel].probe);
    setCueCmd(str);
}

void UiMainWindow::chanProbeX1000() {
    char str[512];
    devParms.chan[devParms.activechannel].scale /= devParms.chan[devParms.activechannel].probe;

    devParms.chan[devParms.activechannel].probe = 1000;

    devParms.chan[devParms.activechannel].scale *= devParms.chan[devParms.activechannel].probe;

    snprintf(str, 512, "Channel %i probe: 1000X", devParms.activechannel + 1);

    statusLabel->setText(str);

    snprintf(str, 512, ":CHAN%i:PROB %e", devParms.activechannel + 1, devParms.chan[devParms.activechannel].probe);
    setCueCmd(str);
}

void UiMainWindow::chanBwlOff() {
    char str[512];
    devParms.chan[devParms.activechannel].bwlimit = 0;

    snprintf(str, 512, "Channel %i bandwidth limit: Off", devParms.activechannel + 1);

    statusLabel->setText(str);

    snprintf(str, 512, ":CHAN%i:BWL OFF", devParms.activechannel + 1);
    setCueCmd(str);

    updateLabels();
}

void UiMainWindow::chanBwl20MHz() {
    char str[512];
    devParms.chan[devParms.activechannel].bwlimit = 20;

    snprintf(str, 512, "Channel %i bandwidth limit: 20MHz", devParms.activechannel + 1);

    statusLabel->setText(str);

    snprintf(str, 512, ":CHAN%i:BWL 20M", devParms.activechannel + 1);
    setCueCmd(str);

    updateLabels();
}

void UiMainWindow::chanBwl100MHz() {
    char str[512];
    devParms.chan[devParms.activechannel].bwlimit = 100;

    snprintf(str, 512, "Channel %i bandwidth limit: 100MHz", devParms.activechannel + 1);

    statusLabel->setText(str);

    snprintf(str, 512, ":CHAN%i:BWL 100M", devParms.activechannel + 1);
    setCueCmd(str);

    updateLabels();
}

void UiMainWindow::chanBwl200MHz() {
    char str[512];
    devParms.chan[devParms.activechannel].bwlimit = 200;

    snprintf(str, 512, "Channel %i bandwidth limit: 200MHz", devParms.activechannel + 1);

    statusLabel->setText(str);

    snprintf(str, 512, ":CHAN%i:BWL 200M", devParms.activechannel + 1);
    setCueCmd(str);

    updateLabels();
}

void UiMainWindow::chanBwl250MHz() {
    char str[512];
    devParms.chan[devParms.activechannel].bwlimit = 250;

    snprintf(str, 512, "Channel %i bandwidth limit: 250MHz", devParms.activechannel + 1);

    statusLabel->setText(str);

    snprintf(str, 512, ":CHAN%i:BWL 250M", devParms.activechannel + 1);
    setCueCmd(str);

    updateLabels();
}

void UiMainWindow::updateLabels() {
    int chn;

    char str[512];
    for(chn = 0; chn < devParms.channelCnt; chn++) {
        str[0] = 0;

        if(devParms.chan[chn].coupling == Coup::AC)
            strlcat(str, "AC", 512);

        if(devParms.chan[chn].impedance)
            strlcat(str, " 50", 512);

        if(devParms.chan[chn].bwlimit)
            strlcat(str, " BW", 512);

        switch(chn) {
        case 0:
            ch1InputLabel->setText(str);
            break;
        case 1:
            ch2InputLabel->setText(str);
            break;
        case 2:
            ch3InputLabel->setText(str);
            break;
        case 3:
            ch4InputLabel->setText(str);
            break;
        }
    }
}

void UiMainWindow::chanInvertOn() {
    char str[512];
    if(!devParms.chan[devParms.activechannel].invert)
        devParms.triggeredgelevel[devParms.activechannel] *= -1;

    devParms.chan[devParms.activechannel].invert = 1;

    snprintf(str, 512, "Channel %i inverted: On", devParms.activechannel + 1);

    statusLabel->setText(str);

    snprintf(str, 512, ":CHAN%i:INV 1", devParms.activechannel + 1);
    setCueCmd(str);
}

void UiMainWindow::chanInvertOff() {
    char str[512];
    if(devParms.chan[devParms.activechannel].invert)
        devParms.triggeredgelevel[devParms.activechannel] *= -1;

    devParms.chan[devParms.activechannel].invert = 0;

    snprintf(str, 512, "Channel %i inverted: Off", devParms.activechannel + 1);

    statusLabel->setText(str);

    snprintf(str, 512, ":CHAN%i:INV 0", devParms.activechannel + 1);
    setCueCmd(str);
}

void UiMainWindow::vertOffsetDialClicked(QPoint) {
    //   QMenu menu;
    //
    //   menu.addAction("Zero", this, &UIMainWindow::verticalPositionZero);
    //
    //   menu.exec(vertOffsetDial->mapToGlobal(QPoint(0,0)));
    int chn;

    char str[512];
    if(devParms.activechannel < 0)
        return;

    chn = devParms.activechannel;

    devParms.chan[chn].offset = 0;

    snprintf(str, 512, "Channel %i offset: ", chn + 1);

    convertToMetricSuffix(str + strlen(str), devParms.chan[chn].offset, 2, 512 - strlen(str));

    strlcat(str, devParms.chan[devParms.chan[chn].unit].unitstr, 512);

    statusLabel->setText(str);

    snprintf(str, 512, ":CHAN%i:OFFS %e", chn + 1, devParms.chan[chn].offset);
    setCueCmd(str);
}

void UiMainWindow::clearButtonClicked() {
    statusLabel->setText("Display cleared");
    setCueCmd(":DISP:CLE");

    waveForm->clear();
}

void UiMainWindow::autoButtonClicked() {
    if((device == nullptr) || (!devParms.connected))
        return;

    scrnTimer->stop();

    scrnThread->wait();

    statusLabel->setText("Auto settings");

    tmcWrite(":AUT");

    getDeviceSettings(7);

    scrnTimer->start(devParms.screenTimerIval);
}

void UiMainWindow::runButtonClicked() {
    if(devParms.triggerstatus == 5) {
        statusLabel->setText("Trigger: run");
        setCueCmd(":RUN");
    } else {
        statusLabel->setText("Trigger: stop");
        setCueCmd(":STOP");
    }
}

void UiMainWindow::singleButtonClicked() {
    statusLabel->setText("Trigger: single");
    setCueCmd(":SING");
}

void UiMainWindow::adjustDialClicked(QPoint) {
    if(adjDialFunc == ADJ_DIAL_FUNC_HOLDOFF) {
        if(devParms.modelSerie == 1) {
            devParms.triggerholdoff = 1.6e-8;

            statusLabel->setText("Holdoff: 16ns");
        } else {
            devParms.triggerholdoff = 1e-7;

            statusLabel->setText("Holdoff: 100ns");
        }
    }
}

void UiMainWindow::horMenuButtonClicked() {
    QMenu menu,
        submenudelayed;

    QList<QAction*> actionList;

    submenudelayed.setTitle("Delayed");
    submenudelayed.addAction("On", this, &UiMainWindow::horizontalDelayedOn);
    submenudelayed.addAction("Off", this, &UiMainWindow::horizontalDelayedOff);
    actionList = submenudelayed.actions();
    if(devParms.timebasedelayenable == 1) {
        actionList[0]->setCheckable(true);
        actionList[0]->setChecked(true);
    } else {
        actionList[1]->setCheckable(true);
        actionList[1]->setChecked(true);
    }
    menu.addMenu(&submenudelayed);

    menu.exec(horMenuButton->mapToGlobal(QPoint(0, 0)));
}

void UiMainWindow::horizontalDelayedOn() {
    if(devParms.timebasedelayenable)
        return;

    devParms.timebasedelayenable = 1;

    statusLabel->setText("Delayed timebase enabled");
    setCueCmd(":TIM:DEL:ENAB 1");

    devParms.timebasedelayoffset = devParms.timebaseoffset;
}

void UiMainWindow::horizontalDelayedOff() {
    if(!devParms.timebasedelayenable)
        return;

    devParms.timebasedelayenable = 0;

    statusLabel->setText("Delayed timebase disabled");
    setCueCmd(":TIM:DEL:ENAB 0");
}

void UiMainWindow::horizontalDelayedToggle() {
    if(devParms.timebasedelayenable) {
        devParms.timebasedelayenable = 0;

        statusLabel->setText("Delayed timebase disabled");
        setCueCmd(":TIM:DEL:ENAB 0");
    } else {
        devParms.timebasedelayenable = 1;

        statusLabel->setText("Delayed timebase enabled");
        setCueCmd(":TIM:DEL:ENAB 1");
    }
}

void UiMainWindow::horPosDialClicked(QPoint) {
    char str[512];
    if(devParms.timebasedelayenable) {
        devParms.timebasedelayoffset = devParms.timebaseoffset;

        strlcpy(str, "Delayed timebase position: ", 512);

        convertToMetricSuffix(str + strlen(str), devParms.timebasedelayoffset, 2, 512 - strlen(str));

        strlcat(str, "s", 512);

        statusLabel->setText(str);

        snprintf(str, 512, ":TIM:DEL:OFFS %e", devParms.timebasedelayoffset);
        setCueCmd(str);
    } else {
        devParms.timebaseoffset = 0;

        strlcpy(str, "Horizontal position: ", 512);

        convertToMetricSuffix(str + strlen(str), devParms.timebaseoffset, 2, 512 - strlen(str));

        strlcat(str, "s", 512);

        statusLabel->setText(str);

        snprintf(str, 512, ":TIM:OFFS %e", devParms.timebaseoffset);
        setCueCmd(str);
    }
}

void UiMainWindow::horScaleDialClicked(QPoint) {
    horizontalDelayedToggle();
}

void UiMainWindow::measureButtonClicked() {

    QMenu menu,
        submenucounter;

    QList<QAction*> actionList;

    submenucounter.setTitle("Counter");
    submenucounter.addAction("OFF", this, &UiMainWindow::counterOff);
    submenucounter.addAction("CH1", this, &UiMainWindow::counterCh1);
    if(devParms.channelCnt > 1)
        submenucounter.addAction("CH2", this, &UiMainWindow::counterCh2);
    if(devParms.channelCnt > 2)
        submenucounter.addAction("CH3", this, &UiMainWindow::counterCh3);
    if(devParms.channelCnt > 3)
        submenucounter.addAction("CH4", this, &UiMainWindow::counterCh4);
    actionList = submenucounter.actions();
    for(int i{}; i < 5; i++) {
        if(devParms.countersrc == i) {
            actionList[i]->setCheckable(true);
            actionList[i]->setChecked(true);

            break;
        }
    }
    menu.addMenu(&submenucounter);

    menu.exec(measureButton->mapToGlobal(QPoint(0, 0)));
}

void UiMainWindow::counterOff() {
    devParms.countersrc = 0;

    statusLabel->setText("Freq. counter off");
    setCueCmd(":MEAS:COUN:SOUR OFF");
}

void UiMainWindow::counterCh1() {
    devParms.countersrc = 1;

    statusLabel->setText("Freq. counter channel 1");
    setCueCmd(":MEAS:COUN:SOUR CHAN1");
}

void UiMainWindow::counterCh2() {
    devParms.countersrc = 2;

    statusLabel->setText("Freq. counter channel 2");
    setCueCmd(":MEAS:COUN:SOUR CHAN2");
}

void UiMainWindow::counterCh3() {
    devParms.countersrc = 3;

    statusLabel->setText("Freq. counter channel 3");
    setCueCmd(":MEAS:COUN:SOUR CHAN3");
}

void UiMainWindow::counterCh4() {
    devParms.countersrc = 4;

    statusLabel->setText("Freq. counter channel 4");
    setCueCmd(":MEAS:COUN:SOUR CHAN4");
}

void UiMainWindow::trigModeButtonClicked() {
    devParms.triggersweep++;

    devParms.triggersweep %= 3;

    switch(devParms.triggersweep) {
    case 0:
        trigModeAutoLed->setValue(true);
        trigModeSingLed->setValue(false);
        statusLabel->setText("Trigger auto");
        setCueCmd(":TRIG:SWE AUTO");
        break;
    case 1:
        trigModeNormLed->setValue(true);
        trigModeAutoLed->setValue(false);
        statusLabel->setText("Trigger norm");
        setCueCmd(":TRIG:SWE NORM");
        break;
    case 2:
        trigModeSingLed->setValue(true);
        trigModeNormLed->setValue(false);
        statusLabel->setText("Trigger single");
        setCueCmd(":TRIG:SWE SING");
        break;
    }
}

void UiMainWindow::trigMenuButtonClicked() {

    char str[512];
    QMenu menu,
        submenusource,
        submenuslope,
        submenucoupling,
        submenusetting;

    QList<QAction*> actionList;

    submenusource.setTitle("Source");
    submenusource.addAction("CH1", this, &UiMainWindow::triggerSourceCh1);
    if(devParms.channelCnt > 1)
        submenusource.addAction("CH2", this, &UiMainWindow::triggerSourceCh2);
    if(devParms.channelCnt > 2)
        submenusource.addAction("CH3", this, &UiMainWindow::triggerSourceCh3);
    if(devParms.channelCnt > 3)
        submenusource.addAction("CH4", this, &UiMainWindow::triggerSourceCh4);
    if(devParms.modelSerie != 1) {
        submenusource.addAction("EXT", this, &UiMainWindow::triggerSourceExt);
        if(devParms.modelSerie != 2)
            submenusource.addAction("EXT/ 5", this, &UiMainWindow::triggerSourceExt5);
    }
    submenusource.addAction("AC Line", this, &UiMainWindow::triggerSourceAcl);
    actionList = submenusource.actions();
    if(devParms.modelSerie == 6 || devParms.modelSerie == 4) {
        for(int i{}; i < 7; i++) {
            if(devParms.triggeredgesource == i) {
                actionList[i]->setCheckable(true);
                actionList[i]->setChecked(true);

                break;
            }
        }
    } else {
        if(devParms.modelSerie == 1) {
            for(int i{}; i < 4; i++) {
                if(devParms.triggeredgesource == i) {
                    actionList[i]->setCheckable(true);
                    actionList[i]->setChecked(true);

                    break;
                }
            }
        }

        if(devParms.modelSerie == 2) {
            for(int i{}; i < 5; i++) {
                if(devParms.triggeredgesource == i) {
                    actionList[i]->setCheckable(true);
                    actionList[i]->setChecked(true);

                    break;
                }
            }
        }

        if(devParms.triggeredgesource == 6) {
            actionList[4]->setCheckable(true);
            actionList[4]->setChecked(true);
        }
    }
    menu.addMenu(&submenusource);

    submenucoupling.setTitle("Coupling");
    submenucoupling.addAction("AC", this, &UiMainWindow::triggerCouplingAc);
    submenucoupling.addAction("DC", this, &UiMainWindow::triggerCouplingDc);
    submenucoupling.addAction("LF reject", this, &UiMainWindow::triggerCouplingLfReject);
    submenucoupling.addAction("HF reject", this, &UiMainWindow::triggerCouplingHfReject);
    actionList = submenucoupling.actions();
    for(int i{}; i < 4; i++) {
        if(devParms.triggercoupling == i) {
            actionList[i]->setCheckable(true);
            actionList[i]->setChecked(true);

            break;
        }
    }
    menu.addMenu(&submenucoupling);

    submenuslope.setTitle("Slope");
    submenuslope.addAction("Positive", this, &UiMainWindow::triggerSlopePos);
    submenuslope.addAction("Negative", this, &UiMainWindow::triggerSlopeNeg);
    submenuslope.addAction("Rise/Fal", this, &UiMainWindow::triggerSlopeRfal);
    actionList = submenuslope.actions();
    for(int i{}; i < 3; i++) {
        if(devParms.triggeredgeslope == i) {
            actionList[i]->setCheckable(true);
            actionList[i]->setChecked(true);

            break;
        }
    }
    menu.addMenu(&submenuslope);

    submenusetting.setTitle("Setting");
    snprintf(str, 512, "Holdoff ");
    convertToMetricSuffix(str + strlen(str), devParms.triggerholdoff, 3, 512 - strlen(str));
    strlcat(str, "S", 512);
    submenusetting.addAction(str, this, &UiMainWindow::triggerSettingHoldOff);
    menu.addMenu(&submenusetting);

    menu.exec(trigMenuButton->mapToGlobal(QPoint(0, 0)));
}

void UiMainWindow::triggerSourceCh1() {
    devParms.triggeredgesource = CHAN::CH_AN_1;
    statusLabel->setText("Trigger source channel 1");
    setCueCmd(":TRIG:EDG:SOUR CHAN1");
}

void UiMainWindow::triggerSourceCh2() {
    devParms.triggeredgesource = CHAN::CH_AN_2;
    statusLabel->setText("Trigger source channel 2");
    setCueCmd(":TRIG:EDG:SOUR CHAN2");
}

void UiMainWindow::triggerSourceCh3() {
    devParms.triggeredgesource = CHAN::CH_AN_3;
    statusLabel->setText("Trigger source channel 3");
    setCueCmd(":TRIG:EDG:SOUR CHAN3");
}

void UiMainWindow::triggerSourceCh4() {
    devParms.triggeredgesource = CHAN::CH_AN_4;
    statusLabel->setText("Trigger source channel 4");
    setCueCmd(":TRIG:EDG:SOUR CHAN4");
}

void UiMainWindow::triggerSourceExt() {
    devParms.triggeredgesource = CHAN::EXT;
    statusLabel->setText("Trigger source extern");
    setCueCmd(":TRIG:EDG:SOUR EXT");
}

void UiMainWindow::triggerSourceExt5() {
    devParms.triggeredgesource = CHAN::EXT5;
    statusLabel->setText("Trigger source extern 5");
    setCueCmd(":TRIG:EDG:SOUR EXT5");
}

void UiMainWindow::triggerSourceAcl() {
    devParms.triggeredgesource = CHAN::ACL;
    statusLabel->setText("Trigger source AC powerline");
    if(devParms.modelSerie != 1) setCueCmd(":TRIG:EDG:SOUR ACL");
    else setCueCmd(":TRIG:EDG:SOUR AC");
}

void UiMainWindow::triggerCouplingAc() {
    devParms.triggercoupling = 0;
    statusLabel->setText("Trigger coupling AC");
    setCueCmd(":TRIG:COUP AC");
}

void UiMainWindow::triggerCouplingDc() {
    devParms.triggercoupling = 1;
    statusLabel->setText("Trigger coupling DC");
    setCueCmd(":TRIG:COUP DC");
}

void UiMainWindow::triggerCouplingLfReject() {
    devParms.triggercoupling = 2;
    statusLabel->setText("Trigger LF reject");
    setCueCmd(":TRIG:COUP LFR");
}

void UiMainWindow::triggerCouplingHfReject() {
    devParms.triggercoupling = 3;
    statusLabel->setText("Trigger HF reject");
    setCueCmd(":TRIG:COUP HFR");
}

void UiMainWindow::triggerSlopePos() {
    devParms.triggeredgeslope = 0;
    statusLabel->setText("Trigger edge positive");
    setCueCmd(":TRIG:EDG:SLOP POS");
}

void UiMainWindow::triggerSlopeNeg() {
    devParms.triggeredgeslope = 1;
    statusLabel->setText("Trigger edge negative");
    setCueCmd(":TRIG:EDG:SLOP NEG");
}

void UiMainWindow::triggerSlopeRfal() {
    devParms.triggeredgeslope = 2;
    statusLabel->setText("Trigger edge positive /negative");
    setCueCmd(":TRIG:EDG:SLOP RFAL");
}

void UiMainWindow::triggerSettingHoldOff() {
    navDialFunc = NAV_DIAL_FUNC_HOLDOFF;
    adjDialFunc = ADJ_DIAL_FUNC_HOLDOFF;
    adjDialLabel->setText("Holdoff");
    adjDialLabel->setStyleSheet("background: #66FF99; font: 7pt;");
    adjdialTimer->start(ADJDIAL_TIMER_IVAL_1);
}

void UiMainWindow::trigForceButtonClicked() {
    statusLabel->setText("Trigger force");
    setCueCmd(":TFOR");
}

void UiMainWindow::trig50pctButtonClicked() {
    statusLabel->setText("Trigger 50%");
    setCueCmd(":TLHA");
    waveForm->setTrigLineVisible();
}

void UiMainWindow::trigAdjustDialClicked(QPoint) {
    char str[512];
    devParms.triggeredgelevel[devParms.triggeredgesource] = 0;
    strlcpy(str, "Trigger level: ", 512);
    convertToMetricSuffix(str + strlen(str), devParms.triggeredgelevel[devParms.triggeredgesource], 2, 512 - strlen(str));
    strlcat(str, devParms.chan[devParms.chan[devParms.triggeredgesource].unit].unitstr, 512);
    statusLabel->setText(str);
    snprintf(str, 512, ":TRIG:EDG:LEV %e", devParms.triggeredgelevel[devParms.triggeredgesource]);
    setCueCmd(str);
}

void UiMainWindow::toggleFft() {
    if(devParms.mathFft == 1) {
        devParms.mathFft = 0;

        if(devParms.modelSerie != 1) setCueCmd(":CALC:MODE OFF");
        else setCueCmd(":MATH:DISP OFF");

        statusLabel->setText("Math display off");
    } else {
        if(devParms.modelSerie != 1) {
            setCueCmd(":CALC:MODE FFT");
        } else {
            setCueCmd(":MATH:OPER FFT");
            setCueCmd(":MATH:DISP ON");
        }

        devParms.mathFft = 1;

        statusLabel->setText("FFT on");
    }
}

void UiMainWindow::toggleFftSplit() {
    QMessageBox msgBox;

    if(devParms.mathFftSplit == 1) {
        if(devParms.vertDivisions == 10) {
            msgBox.setIcon(QMessageBox::NoIcon);
            msgBox.setText("Can not set FFT to fullscreen when extended vertical range is set.\n"
                           "Uncheck \"Use extended vertical range\" checkbox in the settings menu first.");
            msgBox.exec();
            return;
        }

        devParms.mathFftSplit = 0;
        setCueCmd(":MATH:FFT:SPL OFF");

        statusLabel->setText("FFT fullscreen");
    } else {
        setCueCmd(":MATH:FFT:SPL ON");

        devParms.mathFftSplit = 1;

        statusLabel->setText("FFT splitscreen");
    }
}

void UiMainWindow::toggleFftUnit() {
    char str[512];
    if(devParms.mathFftUnit == 1) {
        devParms.fftVScale = 1.0;

        devParms.fftVOffset = 0.0;

        devParms.mathFftUnit = 0;

        if(devParms.modelSerie != 1) {
            setCueCmd(":CALC:FFT:VSM VRMS");
        } else {
            setCueCmd(":MATH:FFT:UNIT VRMS");

            snprintf(str, 512, ":MATH:OFFS %e", devParms.fftVOffset);
            setCueCmd(str);

            snprintf(str, 512, ":MATH:SCAL %e", devParms.fftVScale);
            setCueCmd(str);

            snprintf(str, 512, ":MATH:OFFS %e", devParms.fftVOffset);
            setCueCmd(str);
        }

        statusLabel->setText("FFT unit: Vrms");
    } else {
        devParms.fftVScale = 10.0;

        devParms.fftVOffset = 20.0;

        devParms.mathFftUnit = 1;

        if(devParms.modelSerie != 1) {
            setCueCmd(":CALC:FFT:VSM DBVR");
        } else {
            setCueCmd(":MATH:FFT:UNIT DB");

            snprintf(str, 512, ":MATH:OFFS %e", devParms.fftVOffset);
            setCueCmd(str);

            snprintf(str, 512, ":MATH:SCAL %e", devParms.fftVScale);
            setCueCmd(str);

            snprintf(str, 512, ":MATH:OFFS %e", devParms.fftVOffset);
            setCueCmd(str);
        }

        statusLabel->setText("FFT unit: dB");
    }
}

void UiMainWindow::selectFftCh1() {
    if(devParms.modelSerie == 1) setCueCmd(":MATH:FFT:SOUR CHAN1");
    else setCueCmd(":CALC:FFT:SOUR CHAN1");

    devParms.mathFftSrc = 0;

    statusLabel->setText("FFT source: CH1");
}

void UiMainWindow::selectFftCh2() {
    if(devParms.modelSerie == 1) setCueCmd(":MATH:FFT:SOUR CHAN2");
    else setCueCmd(":CALC:FFT:SOUR CHAN2");

    devParms.mathFftSrc = 1;

    statusLabel->setText("FFT source: CH2");
}

void UiMainWindow::selectFftCh3() {
    if(devParms.modelSerie == 1) setCueCmd(":MATH:FFT:SOUR CHAN3");
    else setCueCmd(":CALC:FFT:SOUR CHAN3");

    devParms.mathFftSrc = 2;

    statusLabel->setText("FFT source: CH3");
}

void UiMainWindow::selectFftCh4() {
    if(devParms.modelSerie == 1) setCueCmd(":MATH:FFT:SOUR CHAN4");
    else setCueCmd(":CALC:FFT:SOUR CHAN4");

    devParms.mathFftSrc = 3;

    statusLabel->setText("FFT source: CH4");
}

void UiMainWindow::selectFftHzdiv20() {
    setFftHzdiv(20.0);
}

void UiMainWindow::selectFftHzdiv40() {
    setFftHzdiv(40.0);
}

void UiMainWindow::selectFftHzdiv80() {
    setFftHzdiv(80.0);
}

void UiMainWindow::selectFftHzdiv100() {
    setFftHzdiv(100.0);
}

void UiMainWindow::selectFftHzdiv200() {
    setFftHzdiv(200.0);
}

void UiMainWindow::setFftHzdiv(double val) {
    char str[512];
    if(devParms.timebasedelayenable)
        devParms.mathFftHscale = (100.0 / devParms.timebasedelayscale) / val;
    else
        devParms.mathFftHscale = (100.0 / devParms.timebasescale) / val;

    if(devParms.modelSerie != 1)
        snprintf(str, 512, ":CALC:FFT:HSP %e", devParms.mathFftHscale);
    else
        snprintf(str, 512, ":MATH:FFT:HSC %e", devParms.mathFftHscale);
    setCueCmd(str);

    strlcpy(str, "FFT scale: ", 512);

    convertToMetricSuffix(str + strlen(str), devParms.mathFftHscale, 2, 512 - strlen(str));

    strlcat(str, "Hz/Div", 512);

    statusLabel->setText(str);
}

void UiMainWindow::selectFftCtr5() {
    char str[512];
    if(devParms.modelSerie != 1)
        snprintf(str, 512, ":CALC:FFT:HCEN %e", devParms.mathFftHscale * 5.0);
    else
        snprintf(str, 512, ":MATH:FFT:HCEN %e", devParms.mathFftHscale * 5.0);
    setCueCmd(str);

    devParms.mathFftHcenter = devParms.mathFftHscale * 5.0;

    strlcpy(str, "FFT center: ", 512);

    convertToMetricSuffix(str + strlen(str), devParms.mathFftHscale * 5.0, 1, 512 - strlen(str));

    strlcat(str, "Hz", 512);

    statusLabel->setText(str);
}

void UiMainWindow::selectFftCtr6() {
    char str[512];
    if(devParms.modelSerie != 1)
        snprintf(str, 512, ":CALC:FFT:HCEN %e", devParms.mathFftHscale * 6.0);
    else
        snprintf(str, 512, ":MATH:FFT:HCEN %e", devParms.mathFftHscale * 6.0);
    setCueCmd(str);

    devParms.mathFftHcenter = devParms.mathFftHscale * 6.0;

    strlcpy(str, "FFT center: ", 512);

    convertToMetricSuffix(str + strlen(str), devParms.mathFftHscale * 6.0, 1, 512 - strlen(str));

    strlcat(str, "Hz", 512);

    statusLabel->setText(str);
}

void UiMainWindow::selectFftCtr7() {
    char str[512];
    if(devParms.modelSerie != 1)
        snprintf(str, 512, ":CALC:FFT:HCEN %e", devParms.mathFftHscale * 7.0);
    else
        snprintf(str, 512, ":MATH:FFT:HCEN %e", devParms.mathFftHscale * 7.0);
    setCueCmd(str);

    devParms.mathFftHcenter = devParms.mathFftHscale * 7.0;

    strlcpy(str, "FFT center: ", 512);

    convertToMetricSuffix(str + strlen(str), devParms.mathFftHscale * 7.0, 1, 512 - strlen(str));

    strlcat(str, "Hz", 512);

    statusLabel->setText(str);
}

void UiMainWindow::selectFftCtr8() {
    char str[512];
    if(devParms.modelSerie != 1)
        snprintf(str, 512, ":CALC:FFT:HCEN %e", devParms.mathFftHscale * 8.0);
    else
        snprintf(str, 512, ":MATH:FFT:HCEN %e", devParms.mathFftHscale * 8.0);
    setCueCmd(str);

    devParms.mathFftHcenter = devParms.mathFftHscale * 8.0;

    strlcpy(str, "FFT center: ", 512);

    convertToMetricSuffix(str + strlen(str), devParms.mathFftHscale * 8.0, 1, 512 - strlen(str));

    strlcat(str, "Hz", 512);

    statusLabel->setText(str);
}

void UiMainWindow::selectFftCtr9() {
    char str[512];
    if(devParms.modelSerie != 1)
        snprintf(str, 512, ":CALC:FFT:HCEN %e", devParms.mathFftHscale * 9.0);
    else
        snprintf(str, 512, ":MATH:FFT:HCEN %e", devParms.mathFftHscale * 9.0);
    setCueCmd(str);

    devParms.mathFftHcenter = devParms.mathFftHscale * 9.0;

    strlcpy(str, "FFT center: ", 512);

    convertToMetricSuffix(str + strlen(str), devParms.mathFftHscale * 9.0, 1, 512 - strlen(str));

    strlcat(str, "Hz", 512);

    statusLabel->setText(str);
}

void UiMainWindow::selectFftCtr10() {
    char str[512];
    if(devParms.modelSerie != 1)
        snprintf(str, 512, ":CALC:FFT:HCEN %e", devParms.mathFftHscale * 10.0);
    else
        snprintf(str, 512, ":MATH:FFT:HCEN %e", devParms.mathFftHscale * 10.0);
    setCueCmd(str);

    devParms.mathFftHcenter = devParms.mathFftHscale * 10.0;

    strlcpy(str, "FFT center: ", 512);

    convertToMetricSuffix(str + strlen(str), devParms.mathFftHscale * 10.0, 1, 512 - strlen(str));

    strlcat(str, "Hz", 512);

    statusLabel->setText(str);
}

void UiMainWindow::selectFftCtr11() {
    char str[512];
    if(devParms.modelSerie != 1)
        snprintf(str, 512, ":CALC:FFT:HCEN %e", devParms.mathFftHscale * 11.0);
    else
        snprintf(str, 512, ":MATH:FFT:HCEN %e", devParms.mathFftHscale * 11.0);
    setCueCmd(str);

    devParms.mathFftHcenter = devParms.mathFftHscale * 11.0;

    strlcpy(str, "FFT center: ", 512);

    convertToMetricSuffix(str + strlen(str), devParms.mathFftHscale * 11.0, 1, 512 - strlen(str));

    strlcat(str, "Hz", 512);

    statusLabel->setText(str);
}

void UiMainWindow::selectFftCtr12() {
    char str[512];
    if(devParms.modelSerie != 1)
        snprintf(str, 512, ":CALC:FFT:HCEN %e", devParms.mathFftHscale * 12.0);
    else
        snprintf(str, 512, ":MATH:FFT:HCEN %e", devParms.mathFftHscale * 12.0);
    setCueCmd(str);

    devParms.mathFftHcenter = devParms.mathFftHscale * 12.0;

    strlcpy(str, "FFT center: ", 512);

    convertToMetricSuffix(str + strlen(str), devParms.mathFftHscale * 12.0, 1, 512 - strlen(str));

    strlcat(str, "Hz", 512);

    statusLabel->setText(str);
}

void UiMainWindow::selectFftVScale1() {
    devParms.fftVScale = 1.0;

    setFftVScale();
}

void UiMainWindow::selectFftVScale2() {
    devParms.fftVScale = 2.0;

    setFftVScale();
}

void UiMainWindow::selectFftVScale5() {
    devParms.fftVScale = 5.0;

    setFftVScale();
}

void UiMainWindow::selectFftVScale10() {
    devParms.fftVScale = 10.0;

    setFftVScale();
}

void UiMainWindow::selectFftVScale20() {
    devParms.fftVScale = 20.0;

    setFftVScale();
}

void UiMainWindow::setFftVScale() {
    char str[512];
    if(device == nullptr)
        return;

    if(!devParms.connected)
        return;

    if(devParms.activechannel < 0)
        return;

    if(devParms.fftVOffset > (devParms.fftVScale * 4.0))
        devParms.fftVOffset = (devParms.fftVScale * 4.0);

    if(devParms.fftVOffset < (devParms.fftVScale * -4.0))
        devParms.fftVOffset = (devParms.fftVScale * -4.0);

    if(devParms.modelSerie != 1) {
        if(devParms.mathFftUnit == 1) {
            snprintf(str, 512, ":CALC:FFT:VSC %e", devParms.fftVScale);
            setCueCmd(str);
        } else {
            snprintf(str, 512, ":CALC:FFT:VSC %e", devParms.fftVScale / devParms.chan[devParms.mathFftSrc].scale);
            setCueCmd(str);
        }
    } else {
        snprintf(str, 512, ":MATH:SCAL %e", devParms.fftVScale);
        setCueCmd(str);
    }

    if(devParms.mathFftUnit == 0) {
        strlcpy(str, "FFT scale: ", 512);

        convertToMetricSuffix(str + strlen(str), devParms.fftVScale, 1, 512 - strlen(str));

        strlcat(str, "V", 512);
    } else {
        snprintf(str, 512, "FFT scale: %+.1fdB/Div", devParms.fftVScale);
    }

    statusLabel->setText(str);

    waveForm->update();
}

void UiMainWindow::selectFftVOffsetp4() {
    devParms.fftVOffset = devParms.fftVScale * 4.0;

    setFftVOffset();
}

void UiMainWindow::selectFftVOffsetp3() {
    devParms.fftVOffset = devParms.fftVScale * 3.0;

    setFftVOffset();
}

void UiMainWindow::selectFftVOffsetp2() {
    devParms.fftVOffset = devParms.fftVScale * 2.0;

    setFftVOffset();
}

void UiMainWindow::selectFftVOffsetp1() {
    devParms.fftVOffset = devParms.fftVScale;

    setFftVOffset();
}

void UiMainWindow::selectFftVOffset0() {
    devParms.fftVOffset = 0.0;

    setFftVOffset();
}

void UiMainWindow::selectFftVOffsetm1() {
    devParms.fftVOffset = devParms.fftVScale * -1.0;

    setFftVOffset();
}

void UiMainWindow::selectFftVOffsetm2() {
    devParms.fftVOffset = devParms.fftVScale * -2.0;

    setFftVOffset();
}

void UiMainWindow::selectFftVOffsetm3() {
    devParms.fftVOffset = devParms.fftVScale * -3.0;

    setFftVOffset();
}

void UiMainWindow::selectFftVOffsetm4() {
    devParms.fftVOffset = devParms.fftVScale * -4.0;

    setFftVOffset();
}

void UiMainWindow::setFftVOffset() {
    char str[512];
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
}

void UiMainWindow::showDecodeWindow() {
    UiDecoderWindow w{this};
}
