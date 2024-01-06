/*
***************************************************************************
*
* Author: Teunis van Beelen
*
* Copyright (C) 2016 - 2020 Teunis van Beelen
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

#include "read_settings_thread.h"

ReadSettingsThread::ReadSettingsThread() {
    device = nullptr;
    errStr[0] = 0;
    errNum = -1;
    devParms = nullptr;
    delay = 0;
}

void ReadSettingsThread::setDelay(int val) {
    delay = val;
}

int ReadSettingsThread::getErrorNum(void) {
    return errNum;
}

void ReadSettingsThread::getErrorStr(char* dest, int sz) {
    strlcpy(dest, errStr, sz);
}

void ReadSettingsThread::setDevice(struct tmcDev* dev) {
    device = dev;
}

void ReadSettingsThread::setDevparmPtr(struct DeviceSettings* devp) {
    devParms = devp;
}

void ReadSettingsThread::run() {
    int chn, line = 0;

    char str[512] = "";

    errNum = -1;

    if(device == nullptr)
        return;

    if(devParms == nullptr)
        return;

    devParms->activechannel = -1;

    if(delay > 0)
        sleep(delay);

    for(chn = 0; chn < devParms->channelCnt; chn++) {
        if(chn < 4) {
            snprintf(str, 512, ":CHAN%i:BWL?", chn + 1);

            usleep(TMC_GDS_DELAY);

            if(tmcWrite(str) != 11) {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }

            if(tmcRead() < 1) {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }

            if(!strcmp(device->buf, "20M")) {
                devParms->chanbwlimit[chn] = 20;
            } else if(!strcmp(device->buf, "250M")) {
                devParms->chanbwlimit[chn] = 250;
            } else if(!strcmp(device->buf, "OFF")) {
                devParms->chanbwlimit[chn] = 0;
            } else {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }

            snprintf(str, 512, ":CHAN%i:COUP?", chn + 1);

            usleep(TMC_GDS_DELAY);

            if(tmcWrite(str) != 12) {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }

            if(tmcRead() < 1) {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }

            if(!strcmp(device->buf, "AC")) {
                devParms->chancoupling[chn] = 2;
            } else if(!strcmp(device->buf, "DC")) {
                devParms->chancoupling[chn] = 1;
            } else if(!strcmp(device->buf, "GND")) {
                devParms->chancoupling[chn] = 0;
            } else {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }

            snprintf(str, 512, ":CHAN%i:DISP?", chn + 1);
            usleep(TMC_GDS_DELAY);

            if(tmcWrite(str) != 12) {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }

            if(tmcRead() < 1) {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }

            if(!strcmp(device->buf, "0")) {
                devParms->chanDisplay[chn] = 0;
            } else if(!strcmp(device->buf, "1")) {
                devParms->chanDisplay[chn] = 1;

                if(devParms->activechannel == -1)
                    devParms->activechannel = chn;
            } else {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }

            if(devParms->modelSerie != 1) {
                snprintf(str, 512, ":CHAN%i:IMP?", chn + 1);

                usleep(TMC_GDS_DELAY);

                if(tmcWrite(str) != 11) {
                    line = __LINE__;
                    goto GDS_OUT_ERROR;
                }

                if(tmcRead() < 1) {
                    line = __LINE__;
                    goto GDS_OUT_ERROR;
                }

                if(!strcmp(device->buf, "OMEG")) {
                    devParms->chanimpedance[chn] = 0;
                } else if(!strcmp(device->buf, "FIFT")) {
                    devParms->chanimpedance[chn] = 1;
                } else {
                    line = __LINE__;
                    goto GDS_OUT_ERROR;
                }
            }

            snprintf(str, 512, ":CHAN%i:INV?", chn + 1);

            usleep(TMC_GDS_DELAY);

            if(tmcWrite(str) != 11) {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }

            if(tmcRead() < 1) {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }

            if(!strcmp(device->buf, "0")) {
                devParms->chaninvert[chn] = 0;
            } else if(!strcmp(device->buf, "1")) {
                devParms->chaninvert[chn] = 1;
            } else {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }

            snprintf(str, 512, ":CHAN%i:OFFS?", chn + 1);

            usleep(TMC_GDS_DELAY);

            if(tmcWrite(str) != 12) {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }

            if(tmcRead() < 1) {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }

            devParms->chanoffset[chn] = atof(device->buf);

            snprintf(str, 512, ":CHAN%i:PROB?", chn + 1);

            usleep(TMC_GDS_DELAY);

            if(tmcWrite(str) != 12) {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }

            if(tmcRead() < 1) {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }

            devParms->chanprobe[chn] = atof(device->buf);

            snprintf(str, 512, ":CHAN%i:UNIT?", chn + 1);

            usleep(TMC_GDS_DELAY);

            if(tmcWrite(str) != 12) {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }

            if(tmcRead() < 1) {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }

            if(!strcmp(device->buf, "VOLT"))
                devParms->chanunit[chn] = 0;
            else if(!strcmp(device->buf, "WATT"))
                devParms->chanunit[chn] = 1;
            else if(!strcmp(device->buf, "AMP"))
                devParms->chanunit[chn] = 2;
            else if(!strcmp(device->buf, "UNKN"))
                devParms->chanunit[chn] = 3;
            else
                devParms->chanunit[chn] = 0;

            snprintf(str, 512, ":CHAN%i:SCAL?", chn + 1);

            usleep(TMC_GDS_DELAY);

            if(tmcWrite(str) != 12) {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }

            if(tmcRead() < 1) {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }

            devParms->chanscale[chn] = atof(device->buf);

            snprintf(str, 512, ":CHAN%i:VERN?", chn + 1);

            usleep(TMC_GDS_DELAY);

            if(tmcWrite(str) != 12) {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }

            if(tmcRead() < 1) {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }

            if(!strcmp(device->buf, "0")) {
                devParms->chanvernier[chn] = 0;
            } else if(!strcmp(device->buf, "1")) {
                devParms->chanvernier[chn] = 1;
            } else {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }
        } else {
            snprintf(str, 512, ":LA:DIG%i:DISP?", chn - 4);
            usleep(TMC_GDS_DELAY);

            if(tmcWrite(str) != strnlen(str, 512)) {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }

            if(tmcRead() < 1) {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }

            if(!strcmp(device->buf, "0")) {
                devParms->chanDisplay[chn] = 0;
            } else if(!strcmp(device->buf, "1")) {
                devParms->chanDisplay[chn] = 1;

                if(devParms->activechannel == -1)
                    devParms->activechannel = chn;
            } else {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }
        }
    }

    usleep(TMC_GDS_DELAY);

    if(tmcWrite(":TIM:OFFS?") != 10) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    devParms->timebaseoffset = atof(device->buf);

    usleep(TMC_GDS_DELAY);

    if(tmcWrite(":TIM:SCAL?") != 10) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    devParms->timebasescale = atof(device->buf);

    usleep(TMC_GDS_DELAY);

    if(tmcWrite(":TIM:DEL:ENAB?") != 14) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "0")) {
        devParms->timebasedelayenable = 0;
    } else if(!strcmp(device->buf, "1")) {
        devParms->timebasedelayenable = 1;
    } else {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    usleep(TMC_GDS_DELAY);

    if(tmcWrite(":TIM:DEL:OFFS?") != 14) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    devParms->timebasedelayoffset = atof(device->buf);

    usleep(TMC_GDS_DELAY);

    if(tmcWrite(":TIM:DEL:SCAL?") != 14) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    devParms->timebasedelayscale = atof(device->buf);

    if(devParms->modelSerie != 1) {
        usleep(TMC_GDS_DELAY);

        if(tmcWrite(":TIM:HREF:MODE?") != 15) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(!strcmp(device->buf, "CENT")) {
            devParms->timebasehrefmode = 0;
        } else if(!strcmp(device->buf, "TPOS")) {
            devParms->timebasehrefmode = 1;
        } else if(!strcmp(device->buf, "USER")) {
            devParms->timebasehrefmode = 2;
        } else {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        usleep(TMC_GDS_DELAY);

        if(tmcWrite(":TIM:HREF:POS?") != 14) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        devParms->timebasehrefpos = atoi(device->buf);
    }

    usleep(TMC_GDS_DELAY);

    if(tmcWrite(":TIM:MODE?") != 10) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "MAIN")) {
        devParms->timebasemode = 0;
    } else if(!strcmp(device->buf, "XY")) {
        devParms->timebasemode = 1;
    } else if(!strcmp(device->buf, "ROLL")) {
        devParms->timebasemode = 2;
    } else {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(devParms->modelSerie != 1) {
        usleep(TMC_GDS_DELAY);

        if(tmcWrite(":TIM:VERN?") != 10) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(!strcmp(device->buf, "0")) {
            devParms->timebasevernier = 0;
        } else if(!strcmp(device->buf, "1")) {
            devParms->timebasevernier = 1;
        } else {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    }

    if((devParms->modelSerie != 1) && (devParms->modelSerie != 2)) {
        usleep(TMC_GDS_DELAY);

        if(tmcWrite(":TIM:XY1:DISP?") != 14) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(!strcmp(device->buf, "0")) {
            devParms->timebasexy1display = 0;
        } else if(!strcmp(device->buf, "1")) {
            devParms->timebasexy1display = 1;
        } else {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        usleep(TMC_GDS_DELAY);

        if(tmcWrite(":TIM:XY2:DISP?") != 14) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(!strcmp(device->buf, "0")) {
            devParms->timebasexy2display = 0;
        } else if(!strcmp(device->buf, "1")) {
            devParms->timebasexy2display = 1;
        } else {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    }

    usleep(TMC_GDS_DELAY);

    if(tmcWrite(":TRIG:COUP?") != 11) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "AC")) {
        devParms->triggercoupling = 0;
    } else if(!strcmp(device->buf, "DC")) {
        devParms->triggercoupling = 1;
    } else if(!strcmp(device->buf, "LFR")) {
        devParms->triggercoupling = 2;
    } else if(!strcmp(device->buf, "HFR")) {
        devParms->triggercoupling = 3;
    } else {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    usleep(TMC_GDS_DELAY);

    if(tmcWrite(":TRIG:SWE?") != 10) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "AUTO")) {
        devParms->triggersweep = 0;
    } else if(!strcmp(device->buf, "NORM")) {
        devParms->triggersweep = 1;
    } else if(!strcmp(device->buf, "SING")) {
        devParms->triggersweep = 2;
    } else {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    usleep(TMC_GDS_DELAY);

    if(tmcWrite(":TRIG:MODE?") != 11) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "EDGE")) {
        devParms->triggermode = 0;
    } else if(!strcmp(device->buf, "PULS")) {
        devParms->triggermode = 1;
    } else if(!strcmp(device->buf, "SLOP")) {
        devParms->triggermode = 2;
    } else if(!strcmp(device->buf, "VID")) {
        devParms->triggermode = 3;
    } else if(!strcmp(device->buf, "PATT")) {
        devParms->triggermode = 4;
    } else if(!strcmp(device->buf, "RS232")) {
        devParms->triggermode = 5;
    } else if(!strcmp(device->buf, "IIC")) {
        devParms->triggermode = 6;
    } else if(!strcmp(device->buf, "SPI")) {
        devParms->triggermode = 7;
    } else if(!strcmp(device->buf, "CAN")) {
        devParms->triggermode = 8;
    } else if(!strcmp(device->buf, "USB")) {
        devParms->triggermode = 9;
    } else {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    usleep(TMC_GDS_DELAY);

    if(tmcWrite(":TRIG:STAT?") != 11) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "TD")) {
        devParms->triggerstatus = 0;
    } else if(!strcmp(device->buf, "WAIT")) {
        devParms->triggerstatus = 1;
    } else if(!strcmp(device->buf, "RUN")) {
        devParms->triggerstatus = 2;
    } else if(!strcmp(device->buf, "AUTO")) {
        devParms->triggerstatus = 3;
    } else if(!strcmp(device->buf, "FIN")) {
        devParms->triggerstatus = 4;
    } else if(!strcmp(device->buf, "STOP")) {
        devParms->triggerstatus = 5;
    } else {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    usleep(TMC_GDS_DELAY);

    if(tmcWrite(":TRIG:EDG:SLOP?") != 15) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "POS")) {
        devParms->triggeredgeslope = 0;
    } else if(!strcmp(device->buf, "NEG")) {
        devParms->triggeredgeslope = 1;
    } else if(!strcmp(device->buf, "RFAL")) {
        devParms->triggeredgeslope = 2;
    } else {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    usleep(TMC_GDS_DELAY);

    if(tmcWrite(":TRIG:EDG:SOUR?") != 15) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "CHAN1")) {
        devParms->triggeredgesource = 0;
    } else if(!strcmp(device->buf, "CHAN2")) {
        devParms->triggeredgesource = 1;
    } else if(!strcmp(device->buf, "CHAN3")) {
        devParms->triggeredgesource = 2;
    } else if(!strcmp(device->buf, "CHAN4")) {
        devParms->triggeredgesource = 3;
    } else if(!strcmp(device->buf, "EXT")) {
        devParms->triggeredgesource = 4;
    } else if(!strcmp(device->buf, "EXT5")) {
        devParms->triggeredgesource = 5;
    } // DS1000Z: "AC", DS6000: "ACL" !!
    else if((!strcmp(device->buf, "AC")) || (!strcmp(device->buf, "ACL"))) {
        devParms->triggeredgesource = 6;
    } else {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    for(chn = 0; chn < /*devParms->channelCnt*/ 4; chn++) { // FIXME
        snprintf(str, 512, ":TRIG:EDG:SOUR CHAN%i", chn + 1);

        usleep(TMC_GDS_DELAY);

        if(tmcWrite(str) != 20) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        usleep(TMC_GDS_DELAY);

        if(tmcWrite(":TRIG:EDG:LEV?") != 14) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        devParms->triggeredgelevel[chn] = atof(device->buf);
    }

    if(devParms->triggeredgesource < 4) {
        snprintf(str, 512, ":TRIG:EDG:SOUR CHAN%i", devParms->triggeredgesource + 1);

        usleep(TMC_GDS_DELAY);

        if(tmcWrite(str) != 20) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    }

    if(devParms->triggeredgesource == 4) {
        usleep(TMC_GDS_DELAY);

        if(tmcWrite(":TRIG:EDG:SOUR EXT") != 18) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    }

    if(devParms->triggeredgesource == 5) {
        usleep(TMC_GDS_DELAY);

        if(tmcWrite(":TRIG:EDG:SOUR EXT5") != 19) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    }

    if(devParms->triggeredgesource == 6) {
        usleep(TMC_GDS_DELAY);

        if(tmcWrite(":TRIG:EDG:SOUR AC") != 17) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    }

    usleep(TMC_GDS_DELAY);

    if(tmcWrite(":TRIG:HOLD?") != 11) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    devParms->triggerholdoff = atof(device->buf);

    usleep(TMC_GDS_DELAY);

    if(tmcWrite(":ACQ:SRAT?") != 10) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    devParms->samplerate = atof(device->buf);

    usleep(TMC_GDS_DELAY);

    if(tmcWrite(":DISP:GRID?") != 11) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "NONE")) {
        devParms->displaygrid = 0;
    } else if(!strcmp(device->buf, "HALF")) {
        devParms->displaygrid = 1;
    } else if(!strcmp(device->buf, "FULL")) {
        devParms->displaygrid = 2;
    } else {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    usleep(TMC_GDS_DELAY);

    if(tmcWrite(":MEAS:COUN:SOUR?") != 16) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "OFF")) {
        devParms->countersrc = 0;
    } else if(!strcmp(device->buf, "CHAN1")) {
        devParms->countersrc = 1;
    } else if(!strcmp(device->buf, "CHAN2")) {
        devParms->countersrc = 2;
    } else if(!strcmp(device->buf, "CHAN3")) {
        devParms->countersrc = 3;
    } else if(!strcmp(device->buf, "CHAN4")) {
        devParms->countersrc = 4;
    } else {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    usleep(TMC_GDS_DELAY);

    if(tmcWrite(":DISP:TYPE?") != 11) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "VECT")) {
        devParms->displaytype = 0;
    } else if(!strcmp(device->buf, "DOTS")) {
        devParms->displaytype = 1;
    } else {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    usleep(TMC_GDS_DELAY);

    if(tmcWrite(":ACQ:TYPE?") != 10) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "NORM")) {
        devParms->acquiretype = 0;
    } else if(!strcmp(device->buf, "AVER")) {
        devParms->acquiretype = 1;
    } else if(!strcmp(device->buf, "PEAK")) {
        devParms->acquiretype = 2;
    } else if(!strcmp(device->buf, "HRES")) {
        devParms->acquiretype = 3;
    } else {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    usleep(TMC_GDS_DELAY);

    if(tmcWrite(":ACQ:AVER?") != 10) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    devParms->acquireaverages = atoi(device->buf);

    usleep(TMC_GDS_DELAY);

    if(tmcWrite(":DISP:GRAD:TIME?") != 16) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "MIN")) {
        devParms->displaygrading = 0;
    } else if(!strcmp(device->buf, "0.1")) {
        devParms->displaygrading = 1;
    } else if(!strcmp(device->buf, "0.2")) {
        devParms->displaygrading = 2;
    } else if(!strcmp(device->buf, "0.5")) {
        devParms->displaygrading = 5;
    } else if(!strcmp(device->buf, "1")) {
        devParms->displaygrading = 10;
    } else if(!strcmp(device->buf, "2")) {
        devParms->displaygrading = 20;
    } else if(!strcmp(device->buf, "5")) {
        devParms->displaygrading = 50;
    } else if(!strcmp(device->buf, "INF")) {
        devParms->displaygrading = 10000;
    } else {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":CALC:FFT:SPL?") != 14) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    } else if(tmcWrite(":MATH:FFT:SPL?") != 14) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    devParms->mathFftSplit = atoi(device->buf);

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":CALC:MODE?") != 11) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(!strcmp(device->buf, "FFT"))
            devParms->mathFft = 1;
        else
            devParms->mathFft = 0;
    } else {
        if(tmcWrite(":MATH:DISP?") != 11) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        devParms->mathFft = atoi(device->buf);

        if(devParms->mathFft == 1) {
            usleep(TMC_GDS_DELAY);

            if(tmcWrite(":MATH:OPER?") != 11) {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }

            if(tmcRead() < 1) {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }

            if(!strcmp(device->buf, "FFT"))
                devParms->mathFft = 1;
            else
                devParms->mathFft = 0;
        }
    }

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":CALC:FFT:VSM?") != 14) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    } else if(tmcWrite(":MATH:FFT:UNIT?") != 15) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "VRMS")) {
        devParms->fftVScale = 0.5;

        devParms->fftVOffset = -2.0;

        devParms->mathFftUnit = 0;
    } else {
        devParms->fftVScale = 10.0;

        devParms->fftVOffset = 20.0;

        devParms->mathFftUnit = 1;
    }

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":CALC:FFT:SOUR?") != 15) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    } else if(tmcWrite(":MATH:FFT:SOUR?") != 15) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "CHAN1"))
        devParms->mathFftSrc = 0;
    else if(!strcmp(device->buf, "CHAN2"))
        devParms->mathFftSrc = 1;
    else if(!strcmp(device->buf, "CHAN3"))
        devParms->mathFftSrc = 2;
    else if(!strcmp(device->buf, "CHAN4"))
        devParms->mathFftSrc = 3;
    else
        devParms->mathFftSrc = 0;

    usleep(TMC_GDS_DELAY);

    devParms->currentScreenSf = 100.0 / devParms->timebasescale;

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":CALC:FFT:HSP?") != 14) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        devParms->mathFftHscale = atof(device->buf);

        //     if(tmc_write(":CALC:FFT:HSC?") != 14)
        //     {
        //       line = __LINE__;
        //       goto GDS_OUT_ERROR;
        //     }
        //
        //     if(tmc_read() < 1)
        //     {
        //       line = __LINE__;
        //       goto GDS_OUT_ERROR;
        //     }
        //
        //     switch(atoi(device->buf))
        //     {
        // //       case  0: devParms->math_fft_hscale = devParms->current_screen_sf / 80.0;
        // //                break;
        //       case  1: devParms->math_fft_hscale = devParms->current_screen_sf / 40.0;
        //                break;
        //       case  2: devParms->math_fft_hscale = devParms->current_screen_sf / 80.0;
        //                break;
        //       case  3: devParms->math_fft_hscale = devParms->current_screen_sf / 200.0;
        //                break;
        //       default: devParms->math_fft_hscale = devParms->current_screen_sf / 40.0;
        //                break;
        //     }
    } else {
        if(tmcWrite(":MATH:FFT:HSC?") != 14) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        devParms->mathFftHscale = atof(device->buf);
    }

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":CALC:FFT:HCEN?") != 15) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    } else if(tmcWrite(":MATH:FFT:HCEN?") != 15) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    devParms->mathFftHcenter = atof(device->buf);

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":CALC:FFT:VOFF?") != 15) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        devParms->fftVOffset = atof(device->buf);
    } else {
        if(tmcWrite(":MATH:OFFS?") != 11) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        devParms->fftVOffset = atof(device->buf);
    }

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":CALC:FFT:VSC?") != 14) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(devParms->mathFftUnit == 1)
            devParms->fftVScale = atof(device->buf);
        else
            devParms->fftVScale = atof(device->buf) * devParms->chanscale[devParms->mathFftSrc];
    } else {
        if(tmcWrite(":MATH:SCAL?") != 11) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        devParms->fftVScale = atof(device->buf);
    }

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":BUS1:MODE?") != 11) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    } else if(tmcWrite(":DEC1:MODE?") != 11) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "PAR"))
        devParms->mathDecodeMode = 0;
    else if(!strcmp(device->buf, "UART"))
        devParms->mathDecodeMode = 1;
    else if(!strcmp(device->buf, "RS232"))
        devParms->mathDecodeMode = 1;
    else if(!strcmp(device->buf, "SPI"))
        devParms->mathDecodeMode = 2;
    else if(!strcmp(device->buf, "IIC"))
        devParms->mathDecodeMode = 3;

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":BUS1:DISP?") != 11) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    } else if(tmcWrite(":DEC1:DISP?") != 11) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    devParms->mathDecodeDisplay = atoi(device->buf);

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":BUS1:FORM?") != 11) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    } else if(tmcWrite(":DEC1:FORM?") != 11) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "HEX"))
        devParms->mathDecodeFormat = 0;
    else if(!strcmp(device->buf, "ASC"))
        devParms->mathDecodeFormat = 1;
    else if(!strcmp(device->buf, "DEC"))
        devParms->mathDecodeFormat = 2;
    else if(!strcmp(device->buf, "BIN"))
        devParms->mathDecodeFormat = 3;
    else if(!strcmp(device->buf, "LINE"))
        devParms->mathDecodeFormat = 4;

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":BUS1:SPI:OFFS?") != 15) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    } else if(tmcWrite(":DEC1:POS?") != 10) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    devParms->mathDecodePos = atoi(device->buf);

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":BUS1:SPI:MISO:THR?") != 19) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    } else if(tmcWrite(":DEC1:THRE:CHAN1?") != 17) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    devParms->mathDecodeThreshold[0] = atof(device->buf);

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":BUS1:SPI:MOSI:THR?") != 19) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    } else if(tmcWrite(":DEC1:THRE:CHAN2?") != 17) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    devParms->mathDecodeThreshold[1] = atof(device->buf);

    if(devParms->channelCnt == 4) {
        usleep(TMC_GDS_DELAY);

        if(devParms->modelSerie != 1) {
            if(tmcWrite(":BUS1:SPI:SCLK:THR?") != 19) {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }
        } else if(tmcWrite(":DEC1:THRE:CHAN3?") != 17) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        devParms->mathDecodeThreshold[2] = atof(device->buf);

        usleep(TMC_GDS_DELAY);

        if(devParms->modelSerie != 1) {
            if(tmcWrite(":BUS1:SPI:SS:THR?") != 17) {
                line = __LINE__;
                goto GDS_OUT_ERROR;
            }
        } else if(tmcWrite(":DEC1:THRE:CHAN4?") != 17) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        devParms->mathDecodeThreshold[3] = atof(device->buf);
    }

    if(devParms->modelSerie != 1) {
        usleep(TMC_GDS_DELAY);

        if(tmcWrite(":BUS1:RS232:TTHR?") != 17) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        //    devParms->math_decode_threshold_uart_tx = atof(device->buf);
        devParms->mathDecodeThresholdUartTx = atof(device->buf)
            * 10.0; // hack for firmware bug!

        usleep(TMC_GDS_DELAY);

        if(tmcWrite(":BUS1:RS232:RTHR?") != 17) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        //    devParms->math_decode_threshold_uart_rx = atof(device->buf);
        devParms->mathDecodeThresholdUartRx = atof(device->buf)
            * 10.0; // hack for firmware bug!
    }

    if(devParms->modelSerie == 1) {
        usleep(TMC_GDS_DELAY);

        if(tmcWrite(":DEC1:THRE:AUTO?") != 16) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        devParms->mathDecodeThresholdAuto = atoi(device->buf);
    }

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":BUS1:RS232:RX?") != 15) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    } else if(tmcWrite(":DEC1:UART:RX?") != 14) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "CHAN1"))
        devParms->mathDecodeUartRx = 1;
    else if(!strcmp(device->buf, "CHAN2"))
        devParms->mathDecodeUartRx = 2;
    else if(!strcmp(device->buf, "CHAN3"))
        devParms->mathDecodeUartRx = 3;
    else if(!strcmp(device->buf, "CHAN4"))
        devParms->mathDecodeUartRx = 4;
    else if(!strcmp(device->buf, "OFF"))
        devParms->mathDecodeUartRx = 0;
    else
        devParms->mathDecodeUartRx = 0;

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":BUS1:RS232:TX?") != 15) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    } else if(tmcWrite(":DEC1:UART:TX?") != 14) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "CHAN1"))
        devParms->mathDecodeUartTx = 1;
    else if(!strcmp(device->buf, "CHAN2"))
        devParms->mathDecodeUartTx = 2;
    else if(!strcmp(device->buf, "CHAN3"))
        devParms->mathDecodeUartTx = 3;
    else if(!strcmp(device->buf, "CHAN4"))
        devParms->mathDecodeUartTx = 4;
    else if(!strcmp(device->buf, "OFF"))
        devParms->mathDecodeUartTx = 0;
    else
        devParms->mathDecodeUartTx = 0;

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":BUS1:RS232:POL?") != 16) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    } else if(tmcWrite(":DEC1:UART:POL?") != 15) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "POS"))
        devParms->mathDecodeUartPol = 1;
    else if(!strcmp(device->buf, "NEG"))
        devParms->mathDecodeUartPol = 0;

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":BUS1:RS232:END?") != 16) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    } else if(tmcWrite(":DEC1:UART:END?") != 15) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "MSB"))
        devParms->mathDecodeUartPol = 1;
    else if(!strcmp(device->buf, "LSB"))
        devParms->mathDecodeUartPol = 0;

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":BUS1:RS232:BAUD?") != 17) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    } else if(tmcWrite(":DEC1:UART:BAUD?") != 16) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    // FIXME  DEC1:UART:BAUD? can return also "USER" instead of a number!
    devParms->mathDecodeUartBaud = atoi(device->buf);

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":BUS1:RS232:DBIT?") != 17) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    } else if(tmcWrite(":DEC1:UART:WIDT?") != 16) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    devParms->mathDecodeUartWidth = atoi(device->buf);

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":BUS1:RS232:SBIT?") != 17) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    } else if(tmcWrite(":DEC1:UART:STOP?") != 16) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "1"))
        devParms->mathDecodeUartStop = 0;
    else if(!strcmp(device->buf, "1.5"))
        devParms->mathDecodeUartStop = 1;
    else if(!strcmp(device->buf, "2"))
        devParms->mathDecodeUartStop = 2;

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":BUS1:RS232:PAR?") != 16) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    } else if(tmcWrite(":DEC1:UART:PAR?") != 15) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "ODD"))
        devParms->mathDecodeUartPar = 1;
    else if(!strcmp(device->buf, "EVEN"))
        devParms->mathDecodeUartPar = 2;
    else if(!strcmp(device->buf, "NONE"))
        devParms->mathDecodeUartPar = 0;
    else
        devParms->mathDecodeUartPar = 0;

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":BUS1:SPI:SCLK:SOUR?") != 20) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    } else if(tmcWrite(":DEC1:SPI:CLK?") != 14) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "CHAN1"))
        devParms->mathDecodeSpiClk = 0;
    else if(!strcmp(device->buf, "CHAN2"))
        devParms->mathDecodeSpiClk = 1;
    else if(!strcmp(device->buf, "CHAN3"))
        devParms->mathDecodeSpiClk = 2;
    else if(!strcmp(device->buf, "CHAN4"))
        devParms->mathDecodeSpiClk = 3;

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":BUS1:SPI:MISO:SOUR?") != 20) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    } else if(tmcWrite(":DEC1:SPI:MISO?") != 15) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "CHAN1"))
        devParms->mathDecodeSpiMiso = 1;
    else if(!strcmp(device->buf, "CHAN2"))
        devParms->mathDecodeSpiMiso = 2;
    else if(!strcmp(device->buf, "CHAN3"))
        devParms->mathDecodeSpiMiso = 3;
    else if(!strcmp(device->buf, "CHAN4"))
        devParms->mathDecodeSpiMiso = 4;
    else if(!strcmp(device->buf, "OFF"))
        devParms->mathDecodeSpiMiso = 0;
    else
        devParms->mathDecodeSpiMiso = 0;

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":BUS1:SPI:MOSI:SOUR?") != 20) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    } else if(tmcWrite(":DEC1:SPI:MOSI?") != 15) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "CHAN1"))
        devParms->mathDecodeSpiMosi = 1;
    else if(!strcmp(device->buf, "CHAN2"))
        devParms->mathDecodeSpiMosi = 2;
    else if(!strcmp(device->buf, "CHAN3"))
        devParms->mathDecodeSpiMosi = 3;
    else if(!strcmp(device->buf, "CHAN4"))
        devParms->mathDecodeSpiMosi = 4;
    else if(!strcmp(device->buf, "OFF"))
        devParms->mathDecodeSpiMosi = 0;
    else
        devParms->mathDecodeSpiMosi = 0;

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":BUS1:SPI:SS:SOUR?") != 18) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    } else if(tmcWrite(":DEC1:SPI:CS?") != 13) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "CHAN1"))
        devParms->mathDecodeSpiCs = 1;
    else if(!strcmp(device->buf, "CHAN2"))
        devParms->mathDecodeSpiCs = 2;
    else if(!strcmp(device->buf, "CHAN3"))
        devParms->mathDecodeSpiCs = 3;
    else if(!strcmp(device->buf, "CHAN4"))
        devParms->mathDecodeSpiCs = 4;
    else if(!strcmp(device->buf, "OFF"))
        devParms->mathDecodeSpiCs = 0;
    else
        devParms->mathDecodeSpiCs = 0;

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":BUS1:SPI:SS:POL?") != 17) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    } else if(tmcWrite(":DEC1:SPI:SEL?") != 14) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "NCS"))
        devParms->mathDecodeSpiSelect = 0;
    else if(!strcmp(device->buf, "CS"))
        devParms->mathDecodeSpiSelect = 1;
    else if(!strcmp(device->buf, "NEG"))
        devParms->mathDecodeSpiSelect = 0;
    else if(!strcmp(device->buf, "POS"))
        devParms->mathDecodeSpiSelect = 1;

    if(devParms->modelSerie == 1) {
        usleep(TMC_GDS_DELAY);

        if(tmcWrite(":DEC1:SPI:MODE?") != 15) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(!strcmp(device->buf, "TIM"))
            devParms->mathDecodeSpiMode = 0;
        else if(!strcmp(device->buf, "CS"))
            devParms->mathDecodeSpiMode = 1;
    }

    if(devParms->modelSerie == 1) {
        usleep(TMC_GDS_DELAY);

        if(tmcWrite(":DEC1:SPI:TIM?") != 14) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        devParms->mathDecodeSpiTimeout = atof(device->buf);
    }

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":BUS1:SPI:MOSI:POL?") != 19) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    } else if(tmcWrite(":DEC1:SPI:POL?") != 14) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "NEG"))
        devParms->mathDecodeSpiPol = 0;
    else if(!strcmp(device->buf, "POS"))
        devParms->mathDecodeSpiPol = 1;

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":BUS1:SPI:SCLK:SLOP?") != 20) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    } else if(tmcWrite(":DEC1:SPI:EDGE?") != 15) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "NEG"))
        devParms->mathDecodeSpiEdge = 0;
    else if(!strcmp(device->buf, "POS"))
        devParms->mathDecodeSpiEdge = 1;
    else if(!strcmp(device->buf, "FALL"))
        devParms->mathDecodeSpiEdge = 0;
    else if(!strcmp(device->buf, "RISE"))
        devParms->mathDecodeSpiEdge = 1;

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":BUS1:SPI:DBIT?") != 15) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    } else if(tmcWrite(":DEC1:SPI:WIDT?") != 15) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    devParms->mathDecodeSpiWidth = atoi(device->buf);

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie != 1) {
        if(tmcWrite(":BUS1:SPI:END?") != 14) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    } else if(tmcWrite(":DEC1:SPI:END?") != 14) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto GDS_OUT_ERROR;
    }

    if(!strcmp(device->buf, "LSB"))
        devParms->mathDecodeSpiEnd = 0;
    else if(!strcmp(device->buf, "MSB"))
        devParms->mathDecodeSpiEnd = 1;

    usleep(TMC_GDS_DELAY);

    if(devParms->modelSerie == 1) {
        if(tmcWrite(":FUNC:WREC:ENAB?") != 16) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(!strcmp(device->buf, "1")) {
            devParms->funcWrecEnable = 1;
        } else if(!strcmp(device->buf, "0")) {
            devParms->funcWrecEnable = 0;
        } else {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    } else {
        if(tmcWrite(":FUNC:WRM?") != 10) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(!strcmp(device->buf, "REC")) {
            devParms->funcWrecEnable = 1;
        } else if(!strcmp(device->buf, "PLAY")) {
            devParms->funcWrecEnable = 2;
        } else if(!strcmp(device->buf, "OFF")) {
            devParms->funcWrecEnable = 0;
        } else {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }
    }

    if(devParms->funcWrecEnable) {
        usleep(TMC_GDS_DELAY);

        if(tmcWrite(":FUNC:WREC:FEND?") != 16) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        devParms->funcWrecFend = atoi(device->buf);

        usleep(TMC_GDS_DELAY);

        if(tmcWrite(":FUNC:WREC:FMAX?") != 16) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        devParms->funcWrecFmax = atoi(device->buf);

        usleep(TMC_GDS_DELAY);

        if(tmcWrite(":FUNC:WREC:FINT?") != 16) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        devParms->funcWrecFintval = atof(device->buf);

        usleep(TMC_GDS_DELAY);

        if(tmcWrite(":FUNC:WREP:FST?") != 15) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        devParms->funcWplayFstart = atoi(device->buf);

        usleep(TMC_GDS_DELAY);

        if(tmcWrite(":FUNC:WREP:FEND?") != 16) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        devParms->funcWplayFend = atoi(device->buf);

        usleep(TMC_GDS_DELAY);

        if(tmcWrite(":FUNC:WREP:FMAX?") != 16) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        devParms->funcWplayFmax = atoi(device->buf);

        usleep(TMC_GDS_DELAY);

        if(tmcWrite(":FUNC:WREP:FINT?") != 16) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        devParms->funcWplayFintval = atof(device->buf);

        usleep(TMC_GDS_DELAY);

        if(tmcWrite(":FUNC:WREP:FCUR?") != 16) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto GDS_OUT_ERROR;
        }

        devParms->funcWplayFcur = atoi(device->buf);
    }

    errNum = 0;

    return;

GDS_OUT_ERROR:

    snprintf(errStr,
        4096,
        "An error occurred while reading settings from device.\n"
        "Command sent: %s\n"
        "Received: %s\n"
        "File %s line %i",
        str,
        device->buf,
        __FILE__,
        line);

    errNum = -1;

    return;
}
