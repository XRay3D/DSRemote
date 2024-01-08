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

#include "screen_thread.h"

#define SPECT_LOG_MINIMUM     (0.00000001)
#define SPECT_LOG_MINIMUM_LOG (-80)

void ScreenThread::setDevice(struct tmcDev* tmdev) {
    params.cmdCueIdxIn = 0;
    params.cmdCueIdxOut = 0;
    params.connected = 0;
    device = tmdev;
}

ScreenThread::ScreenThread() {
    device = nullptr;
    hBusy = 0;
    for(int i{}; i < MAX_CHNS; i++)
        params.waveBuf[i] = (short*)malloc(WAVFRM_MAX_BUFSZ);
    params.cmdCueIdxIn = 0;
    params.cmdCueIdxOut = 0;
    params.connected = 0;
}

ScreenThread::~ScreenThread() {
    for(int i{}; i < MAX_CHNS; i++)
        free(params.waveBuf[i]);
}

void ScreenThread::setParams(struct DeviceSettings* dev_parms) {
    deviceParms = dev_parms;
    params.connected = deviceParms->connected;
    params.modelserie = deviceParms->modelSerie;
    for(int i{}; i < MAX_CHNS; ++i) {
        params.chanDisplay[i] = deviceParms->chan[i].Display;
        params.chanScale[i] = deviceParms->chan[i].scale;
    }
    params.countersrc = deviceParms->countersrc;
    params.cmdCueIdxIn = deviceParms->cmdCueIdxIn;
    params.mathFftSrc = deviceParms->mathFftSrc;
    params.mathFft = deviceParms->mathFft;
    params.mathFftUnit = deviceParms->mathFftUnit;
    params.fftBufIn = deviceParms->fftBufIn;
    params.fftBufOut = deviceParms->fftBufOut;
    params.fftBufsz = deviceParms->fftBufsz;
    params.kCfg = deviceParms->kCfg;
    params.kissFftBuf = deviceParms->kissFftBuf;
    params.currentScreenSf = deviceParms->currentScreenSf;
    params.debugStr[0] = 0;
    params.funcWrecEnable = deviceParms->funcWrecEnable;
    params.funcWrecOperate = deviceParms->funcWrecOperate;
    params.funcWplayOperate = deviceParms->funcWplayOperate;
    params.funcWplayFcur = deviceParms->funcWplayFcur;
    params.funcWrecFmax = deviceParms->funcWrecFmax;
    params.funcWrepFmax = deviceParms->funcWplayFmax;
}

void ScreenThread::getParams(struct DeviceSettings* dev_parms) {
    dev_parms->connected = params.connected;
    dev_parms->triggerstatus = params.triggerstatus;
    dev_parms->triggersweep = params.triggersweep;
    dev_parms->samplerate = params.samplerate;
    dev_parms->acquirememdepth = params.memdepth;
    dev_parms->counterfreq = params.counterfreq;
    dev_parms->waveBufsz = params.waveBufsz;
    for(int i{}; i < MAX_CHNS; i++) {
        if(params.chanDisplay[i]) {
            memcpy(dev_parms->waveBuf[i], params.waveBuf[i], params.waveBufsz * sizeof(short));
            dev_parms->chan[i].xorigin = params.xorigin[i];
        }
    }
    dev_parms->threadErrorStat = params.errorStat;
    dev_parms->threadErrorLine = params.errorLine;
    dev_parms->cmdCueIdxOut = params.cmdCueIdxOut;
    dev_parms->threadResult = params.result;
    dev_parms->threadJob = params.job;
    if(dev_parms->threadJob == TMC_THRD_JOB_TRIGEDGELEV)
        dev_parms->threadValue = params.triggerEdgeLevel;
    if(dev_parms->threadJob == TMC_THRD_JOB_TIMDELAY) {
        dev_parms->timebasedelayoffset = params.timebaseDelayOffset;
        dev_parms->timebasedelayscale = params.timebaseDelayScale;
    }
    if(dev_parms->threadJob == TMC_THRD_JOB_FFTHZDIV) {
        dev_parms->mathFftHscale = params.mathFftHscale;
        dev_parms->mathFftHcenter = params.mathFftHcenter;
    }
    if(dev_parms->funcWrecEnable) {
        dev_parms->funcWrecOperate = params.funcWrecOperate;
        dev_parms->funcWplayOperate = params.funcWplayOperate;
        deviceParms->funcWplayFcur = params.funcWplayFcur;
        deviceParms->funcWrecFmax = params.funcWrecFmax;
        deviceParms->funcWplayFmax = params.funcWrepFmax;
    }
    if(params.debugStr[0]) {
        params.debugStr[1023] = 0;

        printf("params.debug_str: ->%s<-\n", params.debugStr);
    }
}

int ScreenThread::get_devicestatus() {
    int line;

    usleep(TMC_GDS_DELAY);

    if(tmcWrite(":TRIG:STAT?") != 11) {
        line = __LINE__;
        goto OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto OUT_ERROR;
    }

    if(!strcmp(device->buf, "TD")) {
        params.triggerstatus = 0;
    } else if(!strcmp(device->buf, "WAIT")) {
        params.triggerstatus = 1;
    } else if(!strcmp(device->buf, "RUN")) {
        params.triggerstatus = 2;
    } else if(!strcmp(device->buf, "AUTO")) {
        params.triggerstatus = 3;
    } else if(!strcmp(device->buf, "FIN")) {
        params.triggerstatus = 4;
    } else if(!strcmp(device->buf, "STOP")) {
        params.triggerstatus = 5;
    } else {
        line = __LINE__;
        goto OUT_ERROR;
    }

    usleep(TMC_GDS_DELAY);

    if(tmcWrite(":TRIG:SWE?") != 10) {
        line = __LINE__;
        goto OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto OUT_ERROR;
    }

    if(!strcmp(device->buf, "AUTO")) {
        params.triggersweep = 0;
    } else if(!strcmp(device->buf, "NORM")) {
        params.triggersweep = 1;
    } else if(!strcmp(device->buf, "SING")) {
        params.triggersweep = 2;
    } else {
        line = __LINE__;
        goto OUT_ERROR;
    }

    usleep(TMC_GDS_DELAY);

    if(tmcWrite(":ACQ:SRAT?") != 10) {
        line = __LINE__;
        goto OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto OUT_ERROR;
    }

    params.samplerate = atof(device->buf);

    usleep(TMC_GDS_DELAY);

    if(tmcWrite(":ACQ:MDEP?") != 10) {
        line = __LINE__;
        goto OUT_ERROR;
    }

    if(tmcRead() < 1) {
        line = __LINE__;
        goto OUT_ERROR;
    }

    params.memdepth = atoi(device->buf);

    if(params.countersrc) {
        usleep(TMC_GDS_DELAY);

        if(tmcWrite(":MEAS:COUN:VAL?") != 15) {
            line = __LINE__;
            goto OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto OUT_ERROR;
        }

        params.counterfreq = atof(device->buf);
    }

    if(params.funcWrecEnable) {
        usleep(TMC_GDS_DELAY);

        if(tmcWrite(":FUNC:WREC:OPER?") != 16) {
            line = __LINE__;
            goto OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto OUT_ERROR;
        }

        if(params.modelserie == 6) {
            if(!strcmp(device->buf, "REC")) {
                params.funcWrecOperate = 1;
            } else if(!strcmp(device->buf, "STOP")) {
                params.funcWrecOperate = 0;
            } else {
                line = __LINE__;
                goto OUT_ERROR;
            }
        } else if(!strcmp(device->buf, "RUN")) {
            params.funcWrecOperate = 1;
        } else if(!strcmp(device->buf, "STOP")) {
            params.funcWrecOperate = 0;
        } else {
            line = __LINE__;
            goto OUT_ERROR;
        }

        usleep(TMC_GDS_DELAY);

        if(tmcWrite(":FUNC:WREP:OPER?") != 16) {
            line = __LINE__;
            goto OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto OUT_ERROR;
        }

        if(!strcmp(device->buf, "PLAY")) {
            params.funcWplayOperate = 1;
        } else if(!strcmp(device->buf, "STOP")) {
            params.funcWplayOperate = 0;
        } else if(!strcmp(device->buf, "PAUS")) {
            params.funcWplayOperate = 2;
        } else {
            line = __LINE__;
            goto OUT_ERROR;
        }

        usleep(TMC_GDS_DELAY);

        if(tmcWrite(":FUNC:WREP:FCUR?") != 16) {
            line = __LINE__;
            goto OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto OUT_ERROR;
        }

        params.funcWplayFcur = atoi(device->buf);

        usleep(TMC_GDS_DELAY);

        if(tmcWrite(":FUNC:WREC:FMAX?") != 16) {
            line = __LINE__;
            goto OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto OUT_ERROR;
        }

        params.funcWrecFmax = atoi(device->buf);

        usleep(TMC_GDS_DELAY);

        if(tmcWrite(":FUNC:WREP:FMAX?") != 16) {
            line = __LINE__;
            goto OUT_ERROR;
        }

        if(tmcRead() < 1) {
            line = __LINE__;
            goto OUT_ERROR;
        }

        params.funcWrepFmax = atoi(device->buf);
    }

    params.debugStr[0] = 0;

    return 0;

OUT_ERROR:

    strlcpy(params.debugStr, device->hdrBuf, 1024);

    params.errorLine = line;

    return -1;
}

void ScreenThread::run() {
    int n = 0, chns = 0, line, cmd_sent = 0;

    char str[512];

    double y_incr, binsz;

    params.errorStat = 0;

    params.result = TMC_THRD_RESULT_NONE;

    params.job = TMC_THRD_JOB_NONE;

    params.waveBufsz = 0;

    if(device == nullptr)
        return;

    if(hBusy)
        return;

    if(!params.connected) {
        hBusy = 0;

        return;
    }

    hBusy = 1;

    while(params.cmdCueIdxOut != params.cmdCueIdxIn) {
        usleep(TMC_GDS_DELAY);

        tmcWrite(deviceParms->cmdCue[params.cmdCueIdxOut]);

        if(deviceParms->cmdCueResp[params.cmdCueIdxOut] != nullptr) {
            usleep(TMC_GDS_DELAY);

            if(tmcRead() < 1) {
                printf("Can not read from device.\n");
                line = __LINE__;
                goto OUT_ERROR;
            }

            strlcpy(deviceParms->cmdCueResp[params.cmdCueIdxOut], device->buf, 128);
        }

        if((!strncmp(deviceParms->cmdCue[params.cmdCueIdxOut], ":TLHA", 5))
            || ((!strncmp(deviceParms->cmdCue[params.cmdCueIdxOut], ":CHAN", 5))
                && (!strncmp(deviceParms->cmdCue[params.cmdCueIdxOut] + 6, ":SCAL ", 6)))) {
            usleep(TMC_GDS_DELAY);

            if(tmcWrite(":TRIG:EDG:LEV?") != 14) {
                printf("Can not write to device.\n");
                line = __LINE__;
                goto OUT_ERROR;
            }

            if(tmcRead() < 1) {
                printf("Can not read from device.\n");
                line = __LINE__;
                goto OUT_ERROR;
            }

            params.triggerEdgeLevel = atof(device->buf);

            params.job = TMC_THRD_JOB_TRIGEDGELEV;
        } else if(!strncmp(deviceParms->cmdCue[params.cmdCueIdxOut], ":TIM:DEL:ENAB 1", 15)) {
            usleep(TMC_GDS_DELAY);

            if(tmcWrite(":TIM:DEL:OFFS?") != 14) {
                printf("Can not write to device.\n");
                line = __LINE__;
                goto OUT_ERROR;
            }

            if(tmcRead() < 1) {
                printf("Can not read from device.\n");
                line = __LINE__;
                goto OUT_ERROR;
            }

            params.timebaseDelayOffset = atof(device->buf);

            usleep(TMC_GDS_DELAY);

            if(tmcWrite(":TIM:DEL:SCAL?") != 14) {
                printf("Can not write to device.\n");
                line = __LINE__;
                goto OUT_ERROR;
            }

            if(tmcRead() < 1) {
                printf("Can not read from device.\n");
                line = __LINE__;
                goto OUT_ERROR;
            }

            params.timebaseDelayScale = atof(device->buf);

            params.job = TMC_THRD_JOB_TIMDELAY;
        }

        if(params.mathFft) {
            if((!strncmp(deviceParms->cmdCue[params.cmdCueIdxOut], ":TIM:SCAL ", 10))
                || (!strncmp(deviceParms->cmdCue[params.cmdCueIdxOut], ":MATH:OPER FFT", 14))
                || (!strncmp(deviceParms->cmdCue[params.cmdCueIdxOut], ":CALC:MODE FFT", 14))) {
                usleep(TMC_GDS_DELAY * 10);

                if(params.modelserie != 1) {
                    if(tmcWrite(":CALC:FFT:HSP?") != 14) {
                        line = __LINE__;
                        goto OUT_ERROR;
                    }
                } else if(tmcWrite(":MATH:FFT:HSC?") != 14) {
                    printf("Can not write to device.\n");
                    line = __LINE__;
                    goto OUT_ERROR;
                }

                if(tmcRead() < 1) {
                    printf("Can not read from device.\n");
                    line = __LINE__;
                    goto OUT_ERROR;
                }

                params.mathFftHscale = atof(device->buf);

                usleep(TMC_GDS_DELAY);

                if(params.modelserie != 1) {
                    if(tmcWrite(":CALC:FFT:HCEN?") != 15) {
                        line = __LINE__;
                        goto OUT_ERROR;
                    }
                } else if(tmcWrite(":MATH:FFT:HCEN?") != 15) {
                    printf("Can not write to device.\n");
                    line = __LINE__;
                    goto OUT_ERROR;
                }

                if(tmcRead() < 1) {
                    printf("Can not read from device.\n");
                    line = __LINE__;
                    goto OUT_ERROR;
                }

                params.mathFftHcenter = atof(device->buf);

                params.job = TMC_THRD_JOB_FFTHZDIV;
            }
        }

        params.cmdCueIdxOut++;

        params.cmdCueIdxOut %= TMC_CMD_CUE_SZ;

        cmd_sent = 1;
    }

    if(cmd_sent) {
        hBusy = 0;

        params.result = TMC_THRD_RESULT_CMD;

        return;
    }

    params.errorStat = get_devicestatus();

    if(params.errorStat) {
        hBusy = 0;

        return;
    }

    if(!params.connected) {
        hBusy = 0;

        return;
    }

    for(int i{}; i < MAX_CHNS; i++) {
        if(!params.chanDisplay[i]) // Download data only when channel is switched on
            continue;

        chns++;
    }

    if(!chns) {
        hBusy = 0;

        return;
    }

    params.result = TMC_THRD_RESULT_SCRN;

    // struct waveform_preamble wfp;

    //  if(params.triggerstatus != 1)  // Don't download waveform data when triggerstatus is "wait"
    if(1) {
        for(int channel{}; channel < MAX_CHNS; channel++) {
            if(!params.chanDisplay[channel]) // Download data only when channel is switched on
                continue;

            ///////////////////////////////////////////////////////////

            //     tmc_write(":WAV:PRE?");
            //
            //     n = tmc_read();
            //
            //     if(n < 0)
            //     {
            //       strlcpy(str, "Can not read from device.", 512);
            //       goto OUT_ERROR;
            //     }
            //
            //     printf("waveform preamble: %s\n", device->buf);
            //
            //     if(parse_preamble(device->buf, device->sz, &wfp, i))
            //     {
            //       strlcpy(str, "Preamble parsing error.", 512);
            //       goto OUT_ERROR;
            //     }
            //
            //     printf("waveform preamble:\n"
            //            "format: %i\n"
            //            "type: %i\n"
            //            "points: %i\n"
            //            "count: %i\n"
            //            "xincrement: %e\n"
            //            "xorigin: %e\n"
            //            "xreference: %e\n"
            //            "yincrement: %e\n"
            //            "yorigin: %e\n"
            //            "yreference: %i\n",
            //            wfp.format, wfp.type, wfp.points, wfp.count,
            //            wfp.xincrement[i], wfp.xorigin[i], wfp.xreference[i],
            //            wfp.yincrement[i], wfp.yorigin[i], wfp.yreference[i]);
            //
            //     printf("chan[] is %e\n", params.chanoffset[i].offset);

            //     rec_len = wfp.xincrement[i] * wfp.points;

            ///////////////////////////////////////////////////////////
            if(channel < 4)
                snprintf(str, 512, ":WAV:SOUR CHAN%i", channel + 1);
            else
                snprintf(str, 512, ":WAV:SOUR D%i", channel - 4);

            if(tmcWrite(str) != strnlen(str, 512)) { // FIXME
                printf("Can not write to device.\n");
                line = __LINE__;
                goto OUT_ERROR;
            }

            if(tmcWrite(":WAV:FORM BYTE") != 14) {
                printf("Can not write to device.\n");
                line = __LINE__;
                goto OUT_ERROR;
            }

            if(tmcWrite(":WAV:MODE NORM") != 14) {
                printf("Can not write to device.\n");
                line = __LINE__;
                goto OUT_ERROR;
            }

            if(tmcWrite(":WAV:XOR?") != 9) {
                printf("Can not write to device.\n");
                line = __LINE__;
                goto OUT_ERROR;
            }

            if(tmcRead() < 1) {
                printf("Can not read from device.\n");
                line = __LINE__;
                goto OUT_ERROR;
            }

            params.xorigin[channel] = atof(device->buf);

            if(tmcWrite(":WAV:DATA?") != 10) {
                printf("Can not write to device.\n");
                line = __LINE__;
                goto OUT_ERROR;
            }

            n = tmcRead();

            if(n < 0) {
                printf("Can not read from device.\n");
                line = __LINE__;
                goto OUT_ERROR;
            }

            if(n > WAVFRM_MAX_BUFSZ) {
                printf("Datablock too big for buffer.\n");
                line = __LINE__;
                goto OUT_ERROR;
            }

            if(n < 32)
                n = 0;

            for(int j{}; j < n; j++)
                params.waveBuf[channel][j] = channel < 4
                    ? (int)(((unsigned char*)device->buf)[j]) - 127
                    : device->buf[j];

            if((n == (params.fftBufsz * 2)) && (params.mathFft == 1)
                && (channel == params.mathFftSrc)) {
                if(params.modelserie == 6)
                    y_incr = params.chanScale[channel] / 32.0;
                else
                    y_incr = params.chanScale[channel] / 25.0;

                binsz = (double)params.currentScreenSf / (params.fftBufsz * 2.0);

                for(int j{}; j < n; j++)
                    params.fftBufIn[j] = params.waveBuf[channel][j] * y_incr;

                kiss_fftr(params.kCfg, params.fftBufIn, params.kissFftBuf);

                for(int k{}; k < params.fftBufsz; k++) {
                    params.fftBufOut[k] = (((params.kissFftBuf[k].r * params.kissFftBuf[k].r)
                                               + (params.kissFftBuf[k].i * params.kissFftBuf[k].i))
                        / params.fftBufsz);

                    params.fftBufOut[k] /= params.currentScreenSf;

                    if(k == 0) // DC!
                        params.fftBufOut[k] /= 2.0;

                    params.fftBufOut[k] *= binsz;

                    if(params.mathFftUnit) // dBm
                    {
                        if(params.fftBufOut[k] < SPECT_LOG_MINIMUM)
                            params.fftBufOut[k] = SPECT_LOG_MINIMUM;
                        // convert to deciBel's, not to Bel's!
                        params.fftBufOut[k] = log10(params.fftBufOut[k]) * 10.0;

                        if(params.fftBufOut[k] < SPECT_LOG_MINIMUM_LOG)
                            params.fftBufOut[k] = SPECT_LOG_MINIMUM_LOG;
                    } else // Vrms
                    {
                        params.fftBufOut[k] = sqrt(params.fftBufOut[k]);
                    }
                }
            }
        }

        params.waveBufsz = n;
    } else // triggerstatus is "wait"
    {
        params.waveBufsz = 0;
    }

    hBusy = 0;

    return;

OUT_ERROR:

    params.result = TMC_THRD_RESULT_NONE;

    snprintf(str,
        512,
        "An error occurred while reading screen data from device.\n"
        "File %s line %i",
        __FILE__,
        line);

    hBusy = 0;

    params.errorLine = line;

    params.errorStat = -1;

    return;
}
