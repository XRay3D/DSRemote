/*
***************************************************************************
*
* Author: Teunis van Beelen
*
* Copyright (C) 2010 - 2020 Teunis van Beelen
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

#include "signalcurve.h"

#include "time.h"

SignalCurve::SignalCurve(QWidget* w_parent)
    : QWidget(w_parent) {

    mainwindow = (UiMainWindow*)w_parent;

    setAttribute(Qt::WA_OpaquePaintEvent);
    for(auto&& color: SignalColor)
        color = Qt::blue;

    traceWidth = 0;
    BackgroundColor = Qt::gray;
    RasterColor = Qt::darkGray;
    TextColor = Qt::black;

    smallFont.setFamily("Arial");
    smallFont.setPixelSize(8);

    trigLineTimer = new QTimer{this};
    trigLineTimer->setSingleShot(true);

    trigStatTimer = new QTimer{this};

#if QT_VERSION >= 0x050000
    trigLineTimer->setTimerType(Qt::PreciseTimer);
    trigStatTimer->setTimerType(Qt::PreciseTimer);
#endif

    bufSize = 0;
    borderSize = 60;

    vSense = 1;

    mouseX = 0;
    mouseY = 0;
    mouseOldX = 0;
    mouseOldY = 0;

    labelActive = LABEL_ACTIVE_NONE;

    for(int i{}; i < MAX_CHNS; i++) {
        chan[i].ArrowMoving = 0;
        chan[i].ArrowPos = 127;
        chan[i].TmpYPixelOffset = 0;
        chan[i].TmpOldYPixelOffset = 0;
    }

    trigLevelArrowMoving = 0;
    trigPosArrowMoving = 0;
    fftArrowPos = 0;
    fftArrowMoving = 0;
    trigLineVisible = 0;
    trigStatFlash = 0;
    trigLevelArrowPos = 127;
    trigPosArrowPos = 100;
    useMoveEvents = 0;
    updatesEnabled = true;
    oldW = 10000;
    devParms = nullptr;
    device = nullptr;

    connect(trigLineTimer, SIGNAL(timeout()), this, SLOT(trig_lineTimer_handler()));
    connect(trigStatTimer, SIGNAL(timeout()), this, SLOT(trig_statTimer_handler()));
}

void SignalCurve::clear() {
    bufSize = 0;

    update();
}

void SignalCurve::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
}

void SignalCurve::setUpdatesEnabled(bool enabled) {
    updatesEnabled = enabled;
}

void SignalCurve::paintEvent(QPaintEvent*) {
    if(updatesEnabled == true) {
        QPainter paint{this};

        smallFont.setPixelSize(devParms->fontSize);

        paint.setFont(smallFont);

        drawWidget(&paint, width(), height());

        oldW = width();
    }
}

void SignalCurve::drawWidget(QPainter* painter, int curve_w, int curve_h) {
    int i, chn, tmp, rot = 1, small_rulers, curve_w_backup, curve_h_backup, w_trace_offset,
                     chns_done;

    char str[1024];

    double h_step = 0.0, step, step2;

    //  clk_start = clock();

    if(devParms == nullptr)
        return;

    curve_w_backup = curve_w;

    curve_h_backup = curve_h;

    small_rulers = 5 * devParms->horDivisions;

    painter->fillRect(0, 0, curve_w, curve_h, BackgroundColor);

    if((curve_w < ((borderSize * 2) + 5)) || (curve_h < ((borderSize * 2) + 5)))
        return;

    painter->fillRect(0, 0, curve_w, 30, QColor(32, 32, 32));

    drawTopLabels(painter);

    if((devParms->acquirememdepth > 1000) && !devParms->timebasedelayenable)
        tmp = 405
            - ((devParms->timebaseoffset / (devParms->acquirememdepth / devParms->samplerate))
                * 233);
    else
        tmp = 405
            - ((devParms->timebaseoffset
                   / ((double)devParms->timebasescale * (double)devParms->horDivisions))
                * 233);

    if(tmp < 289) {
        tmp = 284;

        rot = 2;
    } else if(tmp > 521) {
        tmp = 526;

        rot = 0;
    }

    if((rot == 0) || (rot == 2))
        drawSmallTriggerArrow(painter, tmp, 11, rot, QColor(255, 128, 0));
    else
        drawSmallTriggerArrow(painter, tmp, 16, rot, QColor(255, 128, 0));

    painter->fillRect(0, curve_h - 30, curve_w, curve_h, QColor(32, 32, 32));

    for(int i{}; i < devParms->channelCnt; i++)
        drawChanLabel(painter, 8 + (i * 130), curve_h - 25, i);

    if(devParms->connected && devParms->showFps)
        drawfpsLabel(painter, curve_w - 80, curve_h - 11);

    /////////////////////////////////// translate coordinates, draw and fill a rectangle ///////////////////////////////////////////

    painter->translate(borderSize, borderSize);

    curve_w -= (borderSize * 2);

    curve_h -= (borderSize * 2);

    if(devParms->mathFft && devParms->mathFftSplit)
        curve_h /= 3;

    /////////////////////////////////// draw the rasters ///////////////////////////////////////////

    painter->setPen(RasterColor);

    painter->drawRect(0, 0, curve_w - 1, curve_h - 1);

    if((devParms->mathFft == 0) || (devParms->mathFftSplit == 0)) {
        if(devParms->displaygrid) {
            painter->setPen(QPen(QBrush(RasterColor, Qt::SolidPattern),
                traceWidth,
                Qt::DotLine,
                Qt::SquareCap,
                Qt::BevelJoin));

            if(devParms->displaygrid == 2) {
                step = (double)curve_w / (double)devParms->horDivisions;

                for(i = 1; i < devParms->horDivisions; i++)
                    painter->drawLine(step * i, curve_h - 1, step * i, 0);

                step = curve_h / (double)devParms->vertDivisions;

                for(i = 1; i < devParms->vertDivisions; i++)
                    painter->drawLine(0, step * i, curve_w - 1, step * i);
            } else {
                painter->drawLine(curve_w / 2, curve_h - 1, curve_w / 2, 0);

                painter->drawLine(0, curve_h / 2, curve_w - 1, curve_h / 2);
            }
        }

        painter->setPen(RasterColor);

        step = (double)curve_w / (double)small_rulers;

        for(i = 1; i < small_rulers; i++) {
            step2 = step * i;

            if(devParms->displaygrid)
                painter->drawLine(step2, curve_h / 2 + 2, step2, curve_h / 2 - 2);

            if(i % 5) {
                painter->drawLine(step2, curve_h - 1, step2, curve_h - 5);

                painter->drawLine(step2, 0, step2, 4);
            } else {
                painter->drawLine(step2, curve_h - 1, step2, curve_h - 9);

                painter->drawLine(step2, 0, step2, 8);
            }
        }

        step = curve_h / (5.0 * devParms->vertDivisions);

        for(i = 1; i < (5 * devParms->vertDivisions); i++) {
            step2 = step * i;

            if(devParms->displaygrid)
                painter->drawLine(curve_w / 2 + 2, step2, curve_w / 2 - 2, step2);

            if(i % 5) {
                painter->drawLine(curve_w - 1, step2, curve_w - 5, step2);

                painter->drawLine(0, step2, 4, step2);
            } else {
                painter->drawLine(curve_w - 1, step2, curve_w - 9, step2);

                painter->drawLine(0, step2, 8, step2);
            }
        }
    } // if((devParms->math_fft == 0) || (devParms->math_fft_split == 0))
    else {
        painter->drawLine(curve_w / 2, curve_h - 1, curve_w / 2, 0);

        painter->drawLine(0, curve_h / 2, curve_w - 1, curve_h / 2);
    }

    /////////////////////////////////// draw the arrows ///////////////////////////////////////////

    if(devParms->modelSerie == 6)
        vSense = -((double)curve_h / 256.0);
    else if(devParms->modelSerie == 4)
        vSense = -((double)curve_h / (32.0 * devParms->vertDivisions));
    else
        vSense = -((double)curve_h / (25.0 * devParms->vertDivisions));

    drawTrigCenterArrow(painter, curve_w / 2, 0);

    for(chn = 0; chn < devParms->channelCnt; chn++) {
        if(!devParms->chan[chn].Display)
            continue;

        if(chan[chn].ArrowMoving) {
            drawArrow(painter, 0, chan[chn].ArrowPos, 0, SignalColor[chn], '1' + chn);
        } else {
            chan[chn].ArrowPos = (curve_h / 2)
                - (devParms->chan[chn].offset
                    / ((devParms->chan[chn].scale * devParms->vertDivisions)
                        / curve_h));

            if(chan[chn].ArrowPos < 0) {
                chan[chn].ArrowPos = -1;
                drawArrow(painter, -6, chan[chn].ArrowPos, 3, SignalColor[chn], '1' + chn);
            } else if(chan[chn].ArrowPos > curve_h) {
                chan[chn].ArrowPos = curve_h + 1;
                drawArrow(painter, -6, chan[chn].ArrowPos, 1, SignalColor[chn], '1' + chn);
            } else {
                drawArrow(painter, 0, chan[chn].ArrowPos, 0, SignalColor[chn], '1' + chn);
            }
        }
    }

    /////////////////////////////////// FFT: draw the curve ///////////////////////////////////////////

    if((devParms->mathFft == 1) && (devParms->mathFftSplit == 0))
        drawFFT(painter, curve_h_backup, curve_w_backup);

    /////////////////////////////////// draw the curve ///////////////////////////////////////////

    if(bufSize > 32) {
        painter->setClipping(true);
        painter->setClipRegion(QRegion(0, 0, curve_w, curve_h), Qt::ReplaceClip);

        h_step = (double)curve_w / (devParms->horDivisions * 100);

        for(chn = 0, chns_done = 0; chn <= devParms->channelCnt; chn++) {
            if(chns_done)
                break;

            if(chn == devParms->activechannel)
                continue;

            if(chn == devParms->channelCnt) {
                chn = devParms->activechannel;

                chns_done = 1;
            }

            if(!devParms->chan[chn].Display)
                continue;

            w_trace_offset = (curve_w / 2.0)
                - (((devParms->timebaseoffset - devParms->chan[chn].xorigin)
                       / devParms->timebasescale)
                    * ((double)curve_w / (double)(devParms->horDivisions)));

            painter->setPen(QPen(QBrush(SignalColor[chn], Qt::SolidPattern),
                traceWidth,
                Qt::SolidLine,
                Qt::SquareCap,
                Qt::BevelJoin));

            for(int i{}; i < bufSize; i++) {
                if(bufSize < (curve_w / 2)) {
                    painter->drawLine(i * h_step + w_trace_offset,
                        (devParms->waveBuf[chn][i] * vSense) + (curve_h / 2)
                            - chan[chn].TmpYPixelOffset,
                        (i + 1) * h_step + w_trace_offset,
                        (devParms->waveBuf[chn][i] * vSense) + (curve_h / 2)
                            - chan[chn].TmpYPixelOffset);
                    if(i)
                        painter->drawLine(i * h_step + w_trace_offset,
                            (devParms->waveBuf[chn][i - 1] * vSense) + (curve_h / 2)
                                - chan[chn].TmpYPixelOffset,
                            i * h_step + w_trace_offset,
                            (devParms->waveBuf[chn][i] * vSense) + (curve_h / 2)
                                - chan[chn].TmpYPixelOffset);
                } else if(i < (bufSize - 1)) {
                    if(devParms->displaytype)
                        painter->drawPoint(i * h_step + w_trace_offset,
                            (devParms->waveBuf[chn][i] * vSense) + (curve_h / 2)
                                - chan[chn].TmpYPixelOffset);
                    else
                        painter->drawLine(i * h_step + w_trace_offset,
                            (devParms->waveBuf[chn][i] * vSense) + (curve_h / 2)
                                - chan[chn].TmpYPixelOffset,
                            (i + 1) * h_step + w_trace_offset,
                            (devParms->waveBuf[chn][i + 1] * vSense)
                                + (curve_h / 2) - chan[chn].TmpYPixelOffset);
                }
            }
        }

        painter->setClipping(false);
    }

    /////////////////////////////////// draw the decoder ///////////////////////////////////////////

    if(devParms->mathDecodeDisplay)
        draw_decoder(painter, curve_w, curve_h);

    /////////////////////////////////// draw the trigger arrows ///////////////////////////////////////////

    if(trigLevelArrowMoving) {
        drawArrow(painter, curve_w, trigLevelArrowPos, 2, QColor(255, 128, 0), 'T');

        painter->setPen(QPen(QBrush(QColor(255, 128, 0), Qt::SolidPattern),
            traceWidth,
            Qt::DashDotLine,
            Qt::SquareCap,
            Qt::BevelJoin));

        painter->drawLine(1, trigLevelArrowPos, curve_w - 2, trigLevelArrowPos);
    } else {
        if(devParms->triggeredgesource < 4) {
            trigLevelArrowPos = (curve_h / 2)
                - ((devParms->triggeredgelevel[devParms->triggeredgesource]
                       + devParms->chan[devParms->triggeredgesource].offset)
                    / ((devParms->chan[devParms->triggeredgesource].scale
                           * devParms->vertDivisions)
                        / curve_h));

            if(trigLevelArrowPos < 0) {
                trigLevelArrowPos = -1;

                drawArrow(painter, curve_w + 6, trigLevelArrowPos, 3, QColor(255, 128, 0), 'T');
            } else if(trigLevelArrowPos > curve_h) {
                trigLevelArrowPos = curve_h + 1;

                drawArrow(painter, curve_w + 6, trigLevelArrowPos, 1, QColor(255, 128, 0), 'T');
            } else {
                drawArrow(painter, curve_w, trigLevelArrowPos, 2, QColor(255, 128, 0), 'T');
            }
        }

        if(trigLineVisible) {
            painter->setPen(QPen(QBrush(QColor(255, 128, 0), Qt::SolidPattern),
                traceWidth,
                Qt::DashDotLine,
                Qt::SquareCap,
                Qt::BevelJoin));

            painter->drawLine(1, trigLevelArrowPos, curve_w - 2, trigLevelArrowPos);
        }
    }

    if(trigPosArrowMoving) {
        drawArrow(painter, trigPosArrowPos, 27, 1, QColor(255, 128, 0), 'T');
    } else {
        if(devParms->timebasedelayenable)
            trigPosArrowPos = (curve_w / 2)
                - ((devParms->timebasedelayoffset
                       / ((double)devParms->timebasedelayscale
                           * (double)devParms->horDivisions))
                    * curve_w);
        else
            trigPosArrowPos = (curve_w / 2)
                - ((devParms->timebaseoffset
                       / ((double)devParms->timebasescale
                           * (double)devParms->horDivisions))
                    * curve_w);

        if(trigPosArrowPos < 0) {
            trigPosArrowPos = -1;

            drawArrow(painter, trigPosArrowPos, 18, 2, QColor(255, 128, 0), 'T');
        } else if(trigPosArrowPos > curve_w) {
            trigPosArrowPos = curve_w + 1;

            drawArrow(painter, trigPosArrowPos, 18, 0, QColor(255, 128, 0), 'T');
        } else {
            drawArrow(painter, trigPosArrowPos, 27, 1, QColor(255, 128, 0), 'T');
        }
    }

    if(devParms->countersrc)
        paintCounterLabel(painter, curve_w - 180, 6);

    if(devParms->funcWrecEnable)
        paintPlaybackLabel(painter, curve_w - 180, 40);

    if((mainwindow->adjDialFunc == ADJ_DIAL_FUNC_HOLDOFF)
        || (mainwindow->navDialFunc == NAV_DIAL_FUNC_HOLDOFF)) {
        convertToMetricSuffix(str, devParms->triggerholdoff, 2, 1024);

        strlcat(str, "S", 1024);

        paintLabel(painter, curve_w - 110, 5, 100, 20, str, Qt::white);
    } else if(mainwindow->adjDialFunc == ADJ_DIAL_FUNC_ACQ_AVG) {
        snprintf(str, 1024, "%i", devParms->acquireaverages);

        paintLabel(painter, curve_w - 110, 5, 100, 20, str, Qt::white);
    }

    if(labelActive == LABEL_ACTIVE_TRIG) {
        convertToMetricSuffix(str,
            devParms->triggeredgelevel[devParms->triggeredgesource],
            2,
            1024);

        strlcat(str, devParms->chan[devParms->chan[devParms->triggeredgesource].unit].unitstr, 1024);

        paintLabel(painter, curve_w - 120, curve_h - 50, 100, 20, str, QColor(255, 128, 0));
    } else if(labelActive == LABEL_ACTIVE_CHAN1) {
        convertToMetricSuffix(str, devParms->chan[0].offset, 2, 1024);

        strlcat(str, devParms->chan[devParms->chan[0].unit].unitstr, 1024);

        paintLabel(painter, 20, curve_h - 50, 100, 20, str, SignalColor[0]);
    } else if(labelActive == LABEL_ACTIVE_CHAN2) {
        convertToMetricSuffix(str, devParms->chan[1].offset, 2, 1024);

        strlcat(str, devParms->chan[devParms->chan[1].unit].unitstr, 1024);

        paintLabel(painter, 20, curve_h - 50, 100, 20, str, SignalColor[1]);
    } else if(labelActive == LABEL_ACTIVE_CHAN3) {
        convertToMetricSuffix(str, devParms->chan[2].offset, 2, 1024);

        strlcat(str, devParms->chan[devParms->chan[2].unit].unitstr, 1024);

        paintLabel(painter, 20, curve_h - 50, 100, 20, str, SignalColor[2]);
    } else if(labelActive == LABEL_ACTIVE_CHAN4) {
        convertToMetricSuffix(str, devParms->chan[3].offset, 2, 1024);

        strlcat(str, devParms->chan[devParms->chan[3].unit].unitstr, 1024);

        paintLabel(painter, 20, curve_h - 50, 100, 20, str, SignalColor[3]);
    }

    if((devParms->mathFft == 1) && (devParms->mathFftSplit == 1)) {
        drawFFT(painter, curve_h_backup, curve_w_backup);

        if(labelActive == LABEL_ACTIVE_FFT) {
            if(devParms->mathFftUnit == 0) {
                strlcpy(str, "POS: ", 1024);

                convertToMetricSuffix(str + strlen(str), devParms->fftVOffset, 1, 1024);

                strlcat(str, "V", 1024);
            } else {
                snprintf(str, 1024, "POS: %.1fdB", devParms->fftVOffset);
            }

            paintLabel(painter, 20, curve_h * 1.85 - 50.0, 100, 20, str, QColor(128, 64, 255));
        }
    }

    //   clk_end = clock();
    //
    //   cpu_time_used += ((double) (clk_end - clk_start)) / CLOCKS_PER_SEC;
    //
    //   scrUpdateCntr++;
    //
    //   if(!(scrUpdateCntr % 50))
    //   {
    //     printf("CPU time used: %f\n", cpu_time_used / 50);
    //
    //     cpu_time_used = 0;
    //   }
}

void SignalCurve::drawFFT(QPainter* painter, int curve_h_b, int curve_w_b) {
    int i, smallRulers, curveW, curveH;

    char str[1024];

    double hStep = 0.0, step, step2, fftHOffset = 0.0;

    smallRulers = 5 * devParms->horDivisions;

    /////////////////////////////////// FFT: translate coordinates, draw and fill a rectangle ///////////////////////////////////////////

    if(devParms->mathFftSplit == 0) {
        curveW = curve_w_b - (borderSize * 2);

        curveH = curve_h_b - (borderSize * 2);
    } else {
        curveH = curve_h_b - (borderSize * 2);

        curveH /= 3;

        painter->resetTransform();

        painter->translate(borderSize, borderSize + curveH + 15);

        curveW = curve_w_b - (borderSize * 2);

        curveH = curve_h_b - (borderSize * 2);

        curveH *= 0.64;

        /////////////////////////////////// FFT: draw the rasters ///////////////////////////////////////////

        painter->setPen(RasterColor);

        painter->drawRect(0, 0, curveW - 1, curveH - 1);

        if(devParms->displaygrid) {
            painter->setPen(QPen(QBrush(RasterColor, Qt::SolidPattern),
                traceWidth,
                Qt::DotLine,
                Qt::SquareCap,
                Qt::BevelJoin));

            if(devParms->displaygrid == 2) {
                step = (double)curveW / (double)devParms->horDivisions;

                for(i = 1; i < devParms->horDivisions; i++)
                    painter->drawLine(step * i, curveH - 1, step * i, 0);

                step = curveH / 8.0;

                for(i = 1; i < 8; i++)
                    painter->drawLine(0, step * i, curveW - 1, step * i);
            } else {
                painter->drawLine(curveW / 2, curveH - 1, curveW / 2, 0);

                painter->drawLine(0, curveH / 2, curveW - 1, curveH / 2);
            }
        }

        painter->setPen(RasterColor);

        step = (double)curveW / (double)smallRulers;

        for(i = 1; i < smallRulers; i++) {
            step2 = step * i;

            if(devParms->displaygrid)
                painter->drawLine(step2, curveH / 2 + 2, step2, curveH / 2 - 2);

            if(i % 5) {
                painter->drawLine(step2, curveH - 1, step2, curveH - 5);

                painter->drawLine(step2, 0, step2, 4);
            } else {
                painter->drawLine(step2, curveH - 1, step2, curveH - 9);

                painter->drawLine(step2, 0, step2, 8);
            }
        }

        step = curveH / 40.0;

        for(i = 1; i < 40; i++) {
            step2 = step * i;

            if(devParms->displaygrid)
                painter->drawLine(curveW / 2 + 2, step2, curveW / 2 - 2, step2);

            if(i % 5) {
                painter->drawLine(curveW - 1, step2, curveW - 5, step2);

                painter->drawLine(0, step2, 4, step2);
            } else {
                painter->drawLine(curveW - 1, step2, curveW - 9, step2);

                painter->drawLine(0, step2, 8, step2);
            }
        }

        /////////////////////////////////// FFT: draw the arrow ///////////////////////////////////////////

        fftArrowPos = (curveH / 2.0)
            - (((double)curveH / (8.0 * devParms->fftVScale))
                * devParms->fftVOffset);

        drawArrow(painter, 0, fftArrowPos, 0, QColor(128, 64, 255), 'M');
    }

    /////////////////////////////////// FFT: draw the curve ///////////////////////////////////////////

    if(bufSize < 32)
        return;

    if((devParms->fftBufsz > 32) && devParms->chan[devParms->mathFftSrc].Display) {
        painter->setClipping(true);
        painter->setClipRegion(QRegion(0, 0, curveW, curveH), Qt::ReplaceClip);

        hStep = (double)curveW / (double)devParms->fftBufsz;

        if(devParms->timebasedelayenable)
            hStep *= (100.0 / devParms->timebasedelayscale) / devParms->mathFftHscale;
        else
            hStep *= (100.0 / devParms->timebasescale) / devParms->mathFftHscale;

        if(devParms->modelSerie != 1)
            hStep /= 28.0;
        else
            hStep /= 24.0;

        fftVSense = (double)curveH / (-8.0 * devParms->fftVScale);

        fftVOffset = (curveH / 2.0) + (fftVSense * devParms->fftVOffset);

        fftHOffset = (curveW / 2)
            - ((devParms->mathFftHcenter / devParms->mathFftHscale) * curveW
                / devParms->horDivisions);

        //     fft_smpls_onscreen = (double)devParms->fftBufsz * ((devParms->math_fft_hscale * devParms->hordivisions) / (double)devParms->current_screen_sf);

        painter->setPen(QPen(QBrush(QColor(128, 64, 255), Qt::SolidPattern),
            traceWidth,
            Qt::SolidLine,
            Qt::SquareCap,
            Qt::BevelJoin));

        for(int i{}; i < (devParms->fftBufsz - 1); i++) {
            //       if(fft_smpls_onscreen < (curve_w / 2))
            //       {
            //         painter->drawLine(i * h_step + fft_h_offset, (devParms->fftBuf_out[i] * fft_v_sense) + fft_v_offset, (i + 1) * h_step + fft_h_offset, (devParms->fftBuf_out[i] * fft_v_sense) + fft_v_offset);
            //         if(i)
            //         {
            //           painter->drawLine(i * h_step + fft_h_offset, (devParms->fftBuf_out[i - 1] * fft_v_sense) + fft_v_offset, i * h_step + fft_h_offset, (devParms->fftBuf_out[i] * fft_v_sense) + fft_v_offset);
            //         }
            //       }
            //       else
            //       {
            //         if(i < (devParms->fftBufsz - 1))
            //         {
            painter->drawLine(i * hStep + fftHOffset,
                (devParms->fftBufOut[i] * fftVSense) + fftVOffset,
                (i + 1) * hStep + fftHOffset,
                (devParms->fftBufOut[i + 1] * fftVSense) + fftVOffset);
            //          }
            //       }
        }

        snprintf(str, 1024, "FFT:  CH%i  ", devParms->mathFftSrc + 1);

        convertToMetricSuffix(str + strlen(str), devParms->fftVScale, 2, 1024 - strlen(str));

        if(devParms->mathFftUnit == 0)
            strlcat(str, "V/Div   Center ", 1024);
        else
            strlcat(str, "dBV/Div   Center ", 1024);

        convertToMetricSuffix(str + strlen(str),
            devParms->mathFftHcenter,
            1,
            1024 - strlen(str));

        strlcat(str, "Hz   ", 1024);

        convertToMetricSuffix(str + strlen(str),
            devParms->mathFftHscale,
            2,
            1024 - strlen(str));

        strlcat(str, "Hz/Div   ", 1024);

        if(devParms->timebasedelayenable)
            convertToMetricSuffix(str + strlen(str),
                100.0 / devParms->timebasedelayscale,
                0,
                1024 - strlen(str));
        else
            convertToMetricSuffix(str + strlen(str),
                100.0 / devParms->timebasescale,
                0,
                1024 - strlen(str));

        strlcat(str, "Sa/s", 1024);

        painter->drawText(15, 30, str);

        painter->setClipping(false);
    } else {
        painter->setPen(QPen(QBrush(QColor(128, 64, 255), Qt::SolidPattern),
            traceWidth,
            Qt::SolidLine,
            Qt::SquareCap,
            Qt::BevelJoin));

        snprintf(str, 1024, "FFT: CH%i Data Invalid!", devParms->mathFftSrc + 1);

        painter->drawText(15, 30, str);
    }
}

void SignalCurve::drawCurve(struct DeviceSettings* devp, struct tmcDev* dev) {
    devParms = devp;

    device = dev;

    bufSize = devParms->waveBufsz;

    update();
}

void SignalCurve::setDeviceParameters(struct DeviceSettings* devp) {
    devParms = devp;
}

void SignalCurve::drawTopLabels(QPainter* painter) {
    int i, x1, tmp;

    char str[512];

    double dtmp1, dtmp2;

    QPainterPath path;

    if(devParms == nullptr)
        return;

    path.addRoundedRect(5, 5, 70, 20, 3, 3);

    painter->fillPath(path, Qt::black);

    painter->setPen(Qt::white);

    painter->drawText(5, 5, 70, 20, Qt::AlignCenter, devParms->modelName);

    //////////////// triggerstatus ///////////////////////////////

    path = QPainterPath();

    path.addRoundedRect(80, 5, 35, 20, 3, 3);

    if((devParms->triggerstatus == 1) || (devParms->triggerstatus == 3)) {
        if(!trigStatFlash) {
            trigStatFlash = 1;

            trigStatTimer->start(1000);
        }
    } else if(trigStatFlash) {
        trigStatFlash = 0;

        trigStatTimer->stop();
    }

    if(trigStatFlash == 2) {
        painter->fillPath(path, Qt::green);

        painter->setPen(Qt::black);
    } else {
        painter->fillPath(path, Qt::black);

        painter->setPen(Qt::green);
    }

    switch(devParms->triggerstatus) {
    case 0:
        painter->drawText(80, 5, 35, 20, Qt::AlignCenter, "T'D");
        break;
    case 1:
        painter->drawText(80, 5, 35, 20, Qt::AlignCenter, "WAIT");
        break;
    case 2:
        painter->drawText(80, 5, 35, 20, Qt::AlignCenter, "RUN");
        break;
    case 3:
        painter->drawText(80, 5, 35, 20, Qt::AlignCenter, "AUTO");
        break;
    case 4:
        painter->drawText(80, 5, 35, 20, Qt::AlignCenter, "FIN");
        break;
    case 5:
        painter->setPen(Qt::red);
        painter->drawText(80, 5, 35, 20, Qt::AlignCenter, "STOP");
        break;
    }

    //////////////// horizontal ///////////////////////////////

    path = QPainterPath();

    path.addRoundedRect(140, 5, 70, 20, 3, 3);

    painter->fillPath(path, Qt::black);

    painter->setPen(Qt::white);

    painter->drawText(125, 20, "H");

    if(devParms->timebasedelayenable)
        convertToMetricSuffix(str, devParms->timebasedelayscale, 1, 512);
    else
        convertToMetricSuffix(str, devParms->timebasescale, 1, 512);

    removeTrailingZeros(str);

    strlcat(str, "s", 512);

    painter->drawText(140, 5, 70, 20, Qt::AlignCenter, str);

    //////////////// samplerate ///////////////////////////////

    painter->setPen(Qt::gray);

    convertToMetricSuffix(str, devParms->samplerate, 0, 512);

    strlcat(str, "Sa/s", 512);

    painter->drawText(200, -1, 85, 20, Qt::AlignCenter, str);

    if(devParms->acquirememdepth) {
        convertToMetricSuffix(str, devParms->acquirememdepth, 1, 512);

        removeTrailingZeros(str);

        strlcat(str, "pts", 512);

        painter->drawText(200, 14, 85, 20, Qt::AlignCenter, str);
    } else {
        painter->drawText(200, 14, 85, 20, Qt::AlignCenter, "AUTO");
    }

    //////////////// memory position ///////////////////////////////

    path = QPainterPath();

    path.addRoundedRect(285, 5, 240, 20, 3, 3);

    painter->fillPath(path, Qt::black);

    painter->setPen(Qt::gray);

    if(devParms->timebasedelayenable) {
        dtmp1 = devParms->timebasedelayscale / devParms->timebasescale;

        dtmp2 = (devParms->timebaseoffset - devParms->timebasedelayoffset)
            / ((devParms->horDivisions / 2) * devParms->timebasescale);

        tmp = (116 - (dtmp1 * 116)) - (dtmp2 * 116);

        if(tmp > 0)
            painter->fillRect(288, 16, tmp, 8, QColor(64, 160, 255));

        x1 = (116 - (dtmp1 * 116)) + (dtmp2 * 116);

        if(x1 > 0)
            painter->fillRect(288 + 233 - x1, 16, x1, 8, QColor(64, 160, 255));
    } else if(devParms->acquirememdepth > 1000) {
        dtmp1 = (devParms->horDivisions * devParms->timebasescale)
            / (devParms->acquirememdepth / devParms->samplerate);

        dtmp2 = devParms->timebaseoffset / dtmp1;

        tmp = (116 - (dtmp1 * 116)) - (dtmp2 * 116);

        if(tmp > 0)
            painter->fillRect(288, 16, tmp, 8, QColor(64, 160, 255));

        x1 = (116 - (dtmp1 * 116)) + (dtmp2 * 116);

        if(x1 > 0)
            painter->fillRect(288 + 233 - x1, 16, x1, 8, QColor(64, 160, 255));
    }

    painter->drawRect(288, 16, 233, 8);

    painter->setPen(Qt::white);

    painter->drawLine(289, 20, 291, 22);

    for(int i{}; i < 19; i++) {
        painter->drawLine((i * 12) + 291, 22, (i * 12) + 293, 22);

        painter->drawLine((i * 12) + 297, 18, (i * 12) + 299, 18);

        painter->drawLine((i * 12) + 294, 21, (i * 12) + 296, 19);

        painter->drawLine((i * 12) + 300, 19, (i * 12) + 302, 21);
    }

    painter->drawLine(519, 22, 520, 22);

    //////////////// delay ///////////////////////////////

    path = QPainterPath();

    path.addRoundedRect(570, 5, 85, 20, 3, 3);

    painter->fillPath(path, Qt::black);

    painter->setPen(QColor(255, 128, 0));

    painter->drawText(555, 20, "D");

    if(devParms->timebasedelayenable)
        convertToMetricSuffix(str, devParms->timebasedelayoffset, 4, 512);
    else
        convertToMetricSuffix(str, devParms->timebaseoffset, 4, 512);

    strlcat(str, "s", 512);

    painter->drawText(570, 5, 85, 20, Qt::AlignCenter, str);

    //////////////// trigger ///////////////////////////////

    path = QPainterPath();

    path.addRoundedRect(685, 5, 125, 20, 3, 3);

    painter->fillPath(path, Qt::black);

    painter->setPen(Qt::gray);

    painter->drawText(670, 20, "T");

    convertToMetricSuffix(str, devParms->triggeredgelevel[devParms->triggeredgesource], 2, 512);

    strlcat(str, devParms->chan[devParms->chan[devParms->triggeredgesource].unit].unitstr, 512);

    if(devParms->triggeredgesource < 4) {
        painter->setPen(SignalColor[devParms->triggeredgesource]);
    } else {
        switch(devParms->triggeredgesource) {
        case 4:
        case 5:
            painter->setPen(Qt::green);
            break;
        case 6:
            painter->setPen(QColor(255, 64, 0));
            break;
        }
    }

    if(devParms->triggeredgesource != 6)
        painter->drawText(735, 5, 85, 20, Qt::AlignCenter, str);

    path = QPainterPath();

    path.addRoundedRect(725, 7, 15, 15, 3, 3);

    if(devParms->triggeredgesource < 4) {
        painter->fillPath(path, SignalColor[devParms->triggeredgesource]);

        snprintf(str, 512, "%i", devParms->triggeredgesource + 1);
    } else {
        switch(devParms->triggeredgesource) {
        case 4:
        case 5:
            painter->fillPath(path, Qt::green);
            strlcpy(str, "E", 512);
            break;
        case 6:
            painter->fillPath(path, QColor(255, 64, 0));
            strlcpy(str, "AC", 512);
            break;
        }
    }

    if(devParms->triggeredgeslope == 0) {
        painter->drawLine(705, 8, 710, 8);
        painter->drawLine(705, 8, 705, 20);
        painter->drawLine(700, 20, 705, 20);
        painter->drawLine(701, 15, 705, 11);
        painter->drawLine(709, 15, 705, 11);
    }

    if(devParms->triggeredgeslope == 1) {
        painter->drawLine(700, 8, 705, 8);
        painter->drawLine(705, 8, 705, 20);
        painter->drawLine(705, 20, 710, 20);
        painter->drawLine(701, 12, 705, 16);
        painter->drawLine(709, 12, 705, 16);
    }

    if(devParms->triggeredgeslope == 2) {
        painter->drawLine(702, 8, 702, 18);
        painter->drawLine(700, 10, 702, 8);
        painter->drawLine(704, 10, 702, 8);
        painter->drawLine(708, 8, 708, 18);
        painter->drawLine(706, 16, 708, 18);
        painter->drawLine(710, 16, 708, 18);
    }

    painter->setPen(Qt::black);

    painter->drawText(725, 8, 15, 15, Qt::AlignCenter, str);
}

void SignalCurve::drawfpsLabel(QPainter* painter, int xpos, int ypos) {
    char str[512];

    static struct timespec tp1, tp2;

    painter->setPen(Qt::red);

    clock_gettime(CLOCK_REALTIME, &tp1);

    if(tp1.tv_nsec >= tp2.tv_nsec)
        snprintf(str,
            512,
            "%04.1f fps",
            1.0 / ((tp1.tv_sec - tp2.tv_sec) + ((tp1.tv_nsec - tp2.tv_nsec) / 1e9)));
    else
        snprintf(str,
            512,
            "%04.1f fps",
            1.0
                / ((tp1.tv_sec - tp2.tv_sec - 1)
                    + ((tp1.tv_nsec - tp2.tv_nsec + 1000000000) / 1e9)));

    painter->drawText(xpos, ypos, str);

    tp2 = tp1;
}

void SignalCurve::drawChanLabel(QPainter* painter, int xpos, int ypos, int chn) {
    QPainterPath path;

    char str1[4], str2[512];

    if(devParms == nullptr)
        return;

    str1[0] = '1' + chn;
    str1[1] = 0;

    convertToMetricSuffix(str2, devParms->chan[chn].scale, 2, 512);

    strlcat(str2, devParms->chan[devParms->chan[chn].unit].unitstr, 512);

    if(devParms->chan[chn].bwlimit)
        strlcat(str2, " B", 512);

    if(devParms->chan[chn].Display) {
        if(chn == devParms->activechannel) {
            path.addRoundedRect(xpos, ypos, 20, 20, 3, 3);

            painter->fillPath(path, SignalColor[chn]);

            painter->setPen(Qt::black);

            painter->drawText(xpos + 6, ypos + 15, str1);

            if(devParms->chan[chn].invert)
                painter->drawLine(xpos + 6, ypos + 3, xpos + 14, ypos + 3);

            path = QPainterPath();
            path.addRoundedRect(xpos + 25, ypos, 90, 20, 3, 3);
            painter->fillPath(path, Qt::black);
            painter->setPen(SignalColor[chn]);
            painter->drawRoundedRect(xpos + 25, ypos, 90, 20, 3, 3);
            painter->drawText(xpos + 35, ypos + 1, 90, 20, Qt::AlignCenter, str2);
            if(devParms->chan[chn].coupling == Coup::GND) {
                QLine lines[]{
                    {xpos + 33, ypos + 6,  xpos + 33, ypos + 10},
                    {xpos + 28, ypos + 10, xpos + 38, ypos + 10},
                    {xpos + 30, ypos + 12, xpos + 36, ypos + 12},
                    {xpos + 32, ypos + 14, xpos + 34, ypos + 14},
                };
                painter->drawLines(lines, 4);
            } else if(devParms->chan[chn].coupling == Coup::DC) {
                QLine lines[]{
                    {xpos + 28, ypos + 8,  xpos + 38, ypos + 8 },
                    {xpos + 28, ypos + 12, xpos + 30, ypos + 12},
                    {xpos + 32, ypos + 12, xpos + 34, ypos + 12},
                    {xpos + 36, ypos + 12, xpos + 38, ypos + 12},
                };
                painter->drawLines(lines, 4);
            } else if(devParms->chan[chn].coupling == Coup::AC) {
                painter->drawArc(xpos + 30, ypos + 8, 5, 5, 10 * 16, 160 * 16);
                painter->drawArc(xpos + 35, ypos + 8, 5, 5, -10 * 16, -160 * 16);
            }
        } else {
            path.addRoundedRect(xpos, ypos, 20, 20, 3, 3);

            path.addRoundedRect(xpos + 25, ypos, 85, 20, 3, 3);

            painter->fillPath(path, Qt::black);

            painter->setPen(SignalColor[chn]);

            painter->drawText(xpos + 6, ypos + 15, str1);

            if(devParms->chan[chn].invert)
                painter->drawLine(xpos + 6, ypos + 3, xpos + 14, ypos + 3);

            painter->drawText(xpos + 35, ypos + 1, 90, 20, Qt::AlignCenter, str2);

            if(devParms->chan[chn].coupling == Coup::GND) {
                QLine lines[]{
                    {xpos + 33, ypos + 6,  xpos + 33, ypos + 10},
                    {xpos + 28, ypos + 10, xpos + 38, ypos + 10},
                    {xpos + 30, ypos + 12, xpos + 36, ypos + 12},
                    {xpos + 32, ypos + 14, xpos + 34, ypos + 14},
                };
                painter->drawLines(lines, 4);
            } else if(devParms->chan[chn].coupling == Coup::DC) {
                QLine lines[]{
                    {xpos + 28, ypos + 8,  xpos + 38, ypos + 8 },
                    {xpos + 28, ypos + 12, xpos + 30, ypos + 12},
                    {xpos + 32, ypos + 12, xpos + 34, ypos + 12},
                    {xpos + 36, ypos + 12, xpos + 38, ypos + 12},
                };
                painter->drawLines(lines, 4);
            } else if(devParms->chan[chn].coupling == Coup::AC) {
                painter->drawArc(xpos + 30, ypos + 8, 5, 5, 10 * 16, 160 * 16);
                painter->drawArc(xpos + 35, ypos + 8, 5, 5, -10 * 16, -160 * 16);
            }
        }
    } else {
        path.addRoundedRect(xpos, ypos, 20, 20, 3, 3);
        path.addRoundedRect(xpos + 25, ypos, 85, 20, 3, 3);
        painter->fillPath(path, Qt::black);

        painter->setPen(QColor(48, 48, 48));

        painter->drawText(xpos + 6, ypos + 15, str1);
        painter->drawText(xpos + 30, ypos + 1, 85, 20, Qt::AlignCenter, str2);

        if(devParms->chan[chn].bwlimit)
            painter->drawText(xpos + 90, ypos + 1, 20, 20, Qt::AlignCenter, "B");

        if(devParms->chan[chn].coupling == Coup::GND) {
            QLine lines[]{
                {xpos + 33, ypos + 6,  xpos + 33, ypos + 10},
                {xpos + 28, ypos + 10, xpos + 38, ypos + 10},
                {xpos + 30, ypos + 12, xpos + 36, ypos + 12},
                {xpos + 32, ypos + 14, xpos + 34, ypos + 14},
            };
            painter->drawLines(lines, 4);
        } else if(devParms->chan[chn].coupling == Coup::DC) {
            QLine lines[]{
                {xpos + 28, ypos + 8,  xpos + 38, ypos + 8 },
                {xpos + 28, ypos + 12, xpos + 30, ypos + 12},
                {xpos + 32, ypos + 12, xpos + 34, ypos + 12},
                {xpos + 36, ypos + 12, xpos + 38, ypos + 12},
            };
            painter->drawLines(lines, 4);
        } else if(devParms->chan[chn].coupling == Coup::AC) {
            painter->drawArc(xpos + 30, ypos + 8, 5, 5, 10 * 16, 160 * 16);
            painter->drawArc(xpos + 35, ypos + 8, 5, 5, -10 * 16, -160 * 16);
        }
    }
}

void SignalCurve::drawArrow(QPainter* painter, int xpos, int ypos, int rot, QColor color, char ch) {
    QPainterPath path;

    char str[4];

    str[0] = ch;
    str[1] = 0;

    if(rot == 0) {
        path.moveTo(xpos - 20, ypos + 6);
        path.lineTo(xpos - 7, ypos + 6);
        path.lineTo(xpos, ypos);
        path.lineTo(xpos - 7, ypos - 6);
        path.lineTo(xpos - 20, ypos - 6);
        path.lineTo(xpos - 20, ypos + 6);
        painter->fillPath(path, color);

        painter->setPen(Qt::black);

        painter->drawText(xpos - 17, ypos + 4, str);
    } else if(rot == 1) {
        path.moveTo(xpos + 6, ypos - 20);
        path.lineTo(xpos + 6, ypos - 7);
        path.lineTo(xpos, ypos);
        path.lineTo(xpos - 6, ypos - 7);
        path.lineTo(xpos - 6, ypos - 20);
        path.lineTo(xpos + 6, ypos - 20);
        painter->fillPath(path, color);

        painter->setPen(Qt::black);

        painter->drawText(xpos - 3, ypos - 7, str);
    } else if(rot == 2) {
        path.moveTo(xpos + 20, ypos + 6);
        path.lineTo(xpos + 7, ypos + 6);
        path.lineTo(xpos, ypos);
        path.lineTo(xpos + 7, ypos - 6);
        path.lineTo(xpos + 20, ypos - 6);
        path.lineTo(xpos + 20, ypos + 6);
        painter->fillPath(path, color);

        painter->setPen(Qt::black);

        painter->drawText(xpos + 9, ypos + 4, str);
    } else if(rot == 3) {
        path.moveTo(xpos + 6, ypos + 20);
        path.lineTo(xpos + 6, ypos + 7);
        path.lineTo(xpos, ypos);
        path.lineTo(xpos - 6, ypos + 7);
        path.lineTo(xpos - 6, ypos + 20);
        path.lineTo(xpos + 6, ypos + 20);
        painter->fillPath(path, color);

        painter->setPen(Qt::black);

        painter->drawText(xpos - 3, ypos + 16, str);
    }
}

void SignalCurve::drawSmallTriggerArrow(QPainter* painter, int xpos, int ypos, int rot, QColor color) {
    QPainterPath path;

    if(rot == 0) {
        path.moveTo(xpos - 13, ypos - 5);
        path.lineTo(xpos - 5, ypos - 5);
        path.lineTo(xpos, ypos);
        path.lineTo(xpos - 5, ypos + 5);
        path.lineTo(xpos - 13, ypos + 5);
        path.lineTo(xpos - 13, ypos - 5);

        painter->fillPath(path, color);

        painter->setPen(Qt::black);

        painter->drawLine(xpos - 10, ypos - 4, xpos - 6, ypos - 4);

        painter->drawLine(xpos - 8, ypos - 4, xpos - 8, ypos + 4);
    } else if(rot == 1) {
        path.moveTo(xpos + 5, ypos - 10);
        path.lineTo(xpos + 5, ypos - 5);
        path.lineTo(xpos, ypos);
        path.lineTo(xpos - 4, ypos - 5);
        path.lineTo(xpos - 4, ypos - 10);
        path.lineTo(xpos + 5, ypos - 10);

        painter->fillPath(path, color);

        painter->setPen(Qt::black);

        painter->drawLine(xpos - 2, ypos - 8, xpos + 2, ypos - 8);

        painter->drawLine(xpos, ypos - 8, xpos, ypos - 3);
    } else if(rot == 2) {
        path.moveTo(xpos + 12, ypos - 5);
        path.lineTo(xpos + 5, ypos - 5);
        path.lineTo(xpos, ypos);
        path.lineTo(xpos + 5, ypos + 5);
        path.lineTo(xpos + 12, ypos + 5);
        path.lineTo(xpos + 12, ypos - 5);

        painter->fillPath(path, color);

        painter->setPen(Qt::black);

        painter->drawLine(xpos + 9, ypos - 4, xpos + 5, ypos - 4);

        painter->drawLine(xpos + 7, ypos - 4, xpos + 7, ypos + 4);
    }
}

void SignalCurve::drawTrigCenterArrow(QPainter* painter, int xpos, int ypos) {
    QPainterPath path;

    path.moveTo(xpos + 7, ypos);
    path.lineTo(xpos - 6, ypos);
    path.lineTo(xpos, ypos + 7);
    path.lineTo(xpos + 7, ypos);
    painter->fillPath(path, QColor(255, 128, 0));
}

void SignalCurve::setSignalColor1(QColor newColor) {
    SignalColor[0] = newColor;
    update();
}

void SignalCurve::setSignalColor2(QColor newColor) {
    SignalColor[1] = newColor;
    update();
}

void SignalCurve::setSignalColor3(QColor newColor) {
    SignalColor[2] = newColor;
    update();
}

void SignalCurve::setSignalColor4(QColor newColor) {
    SignalColor[3] = newColor;
    update();
}

void SignalCurve::setTraceWidth(int tr_width) {
    traceWidth = tr_width;
    if(traceWidth < 0)
        traceWidth = 0;
    update();
}

void SignalCurve::setBackgroundColor(QColor newColor) {
    BackgroundColor = newColor;
    update();
}

void SignalCurve::setRasterColor(QColor newColor) {
    RasterColor = newColor;
    update();
}

void SignalCurve::setBorderSize(int newsize) {
    borderSize = newsize;
    if(borderSize < 0)
        borderSize = 0;
    update();
}

void SignalCurve::mousePressEvent(QMouseEvent* press_event) {
    int chn, m_x, m_y;

    setFocus(Qt::MouseFocusReason);

    w = width() - (2 * borderSize);
    h = height() - (2 * borderSize);

    m_x = press_event->x() - borderSize;
    m_y = press_event->y() - borderSize;

    if(devParms == nullptr)
        return;

    if(!devParms->connected)
        return;

    if(devParms->mathFft && devParms->mathFftSplit) {
        m_y -= ((h / 3) + 15);

        if((m_x > -26) && (m_x < 0) && (m_y > (fftArrowPos - 7)) && (m_y < (fftArrowPos + 7))) {
            fftArrowMoving = 1;
            useMoveEvents = 1;
            setMouseTracking(true);
            mouseOldX = m_x;
            mouseOldY = m_y;
            mainwindow->scrnTimer->stop();
        }

        return;
    }

    //  printf("m_x: %i   m_y: %i   trig_pos_arrow_pos: %i\n",m_x, m_y, trig_pos_arrow_pos);

    if(press_event->button() == Qt::LeftButton) {
        if(m_y > (h + 12)) {
            //      printf("m_x is: %i   m_y is: %i\n", m_x, m_y);

            m_x += borderSize;

            if((m_x > 8) && (m_x < 118)) {
                emit chan1Clicked();
            } else if((m_x > 133) && (m_x < 243)) {
                emit chan2Clicked();
            } else if((m_x > 258) && (m_x < 368)) {
                if(devParms->channelCnt > 2)
                    emit chan3Clicked();
            } else if((m_x > 383) && (m_x < 493)) {
                if(devParms->channelCnt > 3)
                    emit chan4Clicked();
            }

            return;
        }

        if(((m_x > (trigPosArrowPos - 8)) && (m_x < (trigPosArrowPos + 8)) && (m_y > 5)
               && (m_y < 24))
            || ((trigPosArrowPos > w) && (m_x > (trigPosArrowPos - 24))
                && (m_x <= trigPosArrowPos) && (m_y > 9) && (m_y < 26))
            || ((trigPosArrowPos < 0) && (m_x < 24) && (m_x >= 0) && (m_y > 9) && (m_y < 26))) {
            trigPosArrowMoving = 1;
            useMoveEvents = 1;
            setMouseTracking(true);
            mouseOldX = m_x;
            mouseOldY = m_y;
        } else if(((m_x > w) && (m_x < (w + 26)) && (m_y > (trigLevelArrowPos - 7))
                      && (m_y < (trigLevelArrowPos + 7)))
            || ((trigLevelArrowPos < 0) && (m_x >= w) && (m_x < (w + 18)) && (m_y >= 0)
                && (m_y < 22))
            || ((trigLevelArrowPos > h) && (m_x >= w) && (m_x < (w + 18)) && (m_y <= h)
                && (m_y > (h - 22)))) {
            trigLevelArrowMoving = 1;
            useMoveEvents = 1;
            trigLineVisible = 1;
            setMouseTracking(true);
            mouseOldX = m_x;
            mouseOldY = m_y;
        } else {
            for(chn = 0; chn < devParms->channelCnt; chn++) {
                if(!devParms->chan[chn].Display)
                    continue;

                if((m_x > -26 && (m_x < 0) && (m_y > (chan[chn].ArrowPos - 7))
                       && (m_y < (chan[chn].ArrowPos + 7)))
                    || ((chan[chn].ArrowPos < 0) && (m_x > -18) && (m_x <= 0) && (m_y >= 0)
                        && (m_y < 22))
                    || ((chan[chn].ArrowPos > h) && (m_x > -18) && (m_x <= 0) && (m_y <= h)
                        && (m_y > (h - 22)))) {
                    chan[chn].ArrowMoving = 1;
                    devParms->activechannel = chn;
                    useMoveEvents = 1;
                    setMouseTracking(true);
                    mouseOldX = m_x;
                    mouseOldY = m_y;
                    chan[chn].TmpOldYPixelOffset = m_y;

                    break;
                }
            }
        }
    }

    if(useMoveEvents)
        mainwindow->scrnTimer->stop();
}

void SignalCurve::mouseReleaseEvent(QMouseEvent* release_event) {
    int chn, tmp;

    char str[512];

    double lefttime, righttime, delayrange;

    w = width() - (2 * borderSize);
    h = height() - (2 * borderSize);

    mouseX = release_event->x() - borderSize;
    mouseY = release_event->y() - borderSize;

    if(devParms == nullptr)
        return;

    if(!devParms->connected)
        return;

    if(devParms->mathFft && devParms->mathFftSplit) {
        useMoveEvents = 0;
        setMouseTracking(false);

        if(fftArrowMoving) {
            fftArrowMoving = 0;

            if(devParms->screenupdatesOn == 1)
                mainwindow->scrnTimer->start(devParms->screenTimerIval);

            if(devParms->fftVScale > 9.0)
                devParms->fftVOffset = nearbyint(devParms->fftVOffset);
            else
                devParms->fftVOffset = nearbyint(devParms->fftVOffset * 10.0) / 10.0;

            if(devParms->modelSerie == 1) {
                snprintf(str, 512, ":MATH:OFFS %e", devParms->fftVOffset);

                mainwindow->setCueCmd(str);
            }

            if(devParms->mathFftUnit == 0) {
                strlcpy(str, "FFT position: ", 512);

                convertToMetricSuffix(str + strlen(str),
                    devParms->fftVOffset,
                    1,
                    512 - strlen(str));

                strlcat(str, "V/Div", 512);
            } else {
                snprintf(str, 512, "FFT position: %+.0fdB", devParms->fftVOffset);
            }

            mainwindow->statusLabel->setText(str);

            update();
        }

        return;
    }

    if(trigPosArrowMoving) {
        trigPosArrowPos = mouseX;

        if(trigPosArrowPos < 0)
            trigPosArrowPos = 0;

        if(trigPosArrowPos > w)
            trigPosArrowPos = w;

        //    printf("w is %i   trig_pos_arrow_pos is %i\n", w, trig_pos_arrow_pos);

        if(devParms->timebasedelayenable) {
            devParms->timebasedelayoffset = ((devParms->timebasedelayscale
                                                 * (double)devParms->horDivisions)
                                                / w)
                * ((w / 2) - trigPosArrowPos);

            tmp = devParms->timebasedelayoffset / (devParms->timebasedelayscale / 50);

            devParms->timebasedelayoffset = (devParms->timebasedelayscale / 50) * tmp;

            lefttime = ((devParms->horDivisions / 2) * devParms->timebasescale)
                - devParms->timebaseoffset;

            righttime = ((devParms->horDivisions / 2) * devParms->timebasescale)
                + devParms->timebaseoffset;

            delayrange = (devParms->horDivisions / 2) * devParms->timebasedelayscale;

            if(devParms->timebasedelayoffset < -(lefttime - delayrange))
                devParms->timebasedelayoffset = -(lefttime - delayrange);

            if(devParms->timebasedelayoffset > (righttime - delayrange))
                devParms->timebasedelayoffset = (righttime - delayrange);

            strlcpy(str, "Delayed timebase position: ", 512);

            convertToMetricSuffix(str + strlen(str),
                devParms->timebasedelayoffset,
                2,
                512 - strlen(str));

            strlcat(str, "s", 512);

            mainwindow->statusLabel->setText(str);

            snprintf(str, 512, ":TIM:DEL:OFFS %e", devParms->timebasedelayoffset);

            mainwindow->setCueCmd(str);
        } else {
            devParms->timebaseoffset = ((devParms->timebasescale * (double)devParms->horDivisions)
                                           / w)
                * ((w / 2) - trigPosArrowPos);

            tmp = devParms->timebaseoffset / (devParms->timebasescale / 50);

            devParms->timebaseoffset = (devParms->timebasescale / 50) * tmp;

            strlcpy(str, "Horizontal position: ", 512);

            convertToMetricSuffix(str + strlen(str),
                devParms->timebaseoffset,
                2,
                512 - strlen(str));

            strlcat(str, "s", 512);

            mainwindow->statusLabel->setText(str);

            snprintf(str, 512, ":TIM:OFFS %e", devParms->timebaseoffset);

            mainwindow->setCueCmd(str);
        }
    } else if(trigLevelArrowMoving) {
        if(devParms->triggeredgesource > 3)
            return;

        trigLevelArrowPos = mouseY;

        if(trigLevelArrowPos < 0)
            trigLevelArrowPos = 0;

        if(trigLevelArrowPos > h)
            trigLevelArrowPos = h;

        //       printf("chan[chn] is: %e   chanscale[chn].offset is %e   trig_level_arrow_pos is: %i   v_sense is: %e\n",
        //              devParms->chan[chn], devParms->chanscale[chn].offset, trig_level_arrow_pos, v_sense);

        devParms->triggeredgelevel[devParms->triggeredgesource]
            = (((h / 2) - trigLevelArrowPos)
                  * ((devParms->chan[devParms->triggeredgesource].scale * devParms->vertDivisions) / h))
            - devParms->chan[devParms->triggeredgesource].offset;

        tmp = devParms->triggeredgelevel[devParms->triggeredgesource]
            / (devParms->chan[devParms->triggeredgesource].scale / 50);

        devParms->triggeredgelevel[devParms->triggeredgesource]
            = (devParms->chan[devParms->triggeredgesource].scale / 50) * tmp;

        snprintf(str, 512, "Trigger level: ");

        convertToMetricSuffix(str + strlen(str),
            devParms->triggeredgelevel[devParms->triggeredgesource],
            2,
            512 - strlen(str));

        strlcat(str, devParms->chan[devParms->chan[devParms->triggeredgesource].unit].unitstr, 512);

        mainwindow->statusLabel->setText(str);

        snprintf(str,
            512,
            ":TRIG:EDG:LEV %e",
            devParms->triggeredgelevel[devParms->triggeredgesource]);

        mainwindow->setCueCmd(str);

        trigLineTimer->start(1300);
    } else {
        for(chn = 0; chn < devParms->channelCnt; chn++) {
            if(!devParms->chan[chn].Display)
                continue;

            if(chan[chn].ArrowMoving) {
                chan[chn].ArrowPos = mouseY;

                if(chan[chn].ArrowPos < 0)
                    chan[chn].ArrowPos = 0;

                if(chan[chn].ArrowPos > h)
                    chan[chn].ArrowPos = h;

                //       printf("chan[chn] is: %e   chanscale[chn] is %e   chan_arrow_pos[chn].offset is: %i   v_sense is: %e\n",
                //              devParms->chan[chn], devParms->chanscale[chn], chan_arrow_pos[chn].offset, v_sense);

                devParms->chan[chn].offset = ((h / 2) - chan[chn].ArrowPos)
                    * ((devParms->chan[chn].scale * devParms->vertDivisions)
                        / h);

                tmp = devParms->chan[chn].offset / (devParms->chan[chn].scale / 50);

                devParms->chan[chn].offset = (devParms->chan[chn].scale / 50) * tmp;

                snprintf(str, 512, "Channel %i offset: ", chn + 1);

                convertToMetricSuffix(str + strlen(str),
                    devParms->chan[chn].offset,
                    3,
                    512 - strlen(str));

                strlcat(str, devParms->chan[devParms->chan[chn].unit].unitstr, 512);

                mainwindow->statusLabel->setText(str);

                snprintf(str, 512, ":CHAN%i:OFFS %e", chn + 1, devParms->chan[chn].offset);

                mainwindow->setCueCmd(str);

                devParms->activechannel = chn;

                chan[chn].TmpYPixelOffset = 0;

                chan[chn].TmpOldYPixelOffset = 0;

                break;
            }
        }
    }

    for(chn = 0; chn < MAX_CHNS; chn++)
        chan[chn].ArrowMoving = 0;
    trigLevelArrowMoving = 0;
    trigPosArrowMoving = 0;
    useMoveEvents = 0;
    setMouseTracking(false);

    if(devParms->screenupdatesOn == 1)
        mainwindow->scrnTimer->start(devParms->screenTimerIval);

    update();
}

void SignalCurve::mouseMoveEvent(QMouseEvent* move_event) {
    int chn, h_fft, a_pos;

    double dtmp;

    if(!useMoveEvents)
        return;

    mouseX = move_event->x() - borderSize;
    mouseY = move_event->y() - borderSize;

    if(devParms == nullptr)
        return;

    if(!devParms->connected)
        return;

    if(devParms->mathFft && devParms->mathFftSplit) {
        mouseY -= ((h / 3) + 15);

        h_fft = h * 0.64;

        if(fftArrowMoving) {
            a_pos = mouseY;

            if(a_pos < 0)
                a_pos = 0;

            if(a_pos > (h * 0.64))
                a_pos = (h * 0.64);

            devParms->fftVOffset = ((h_fft / 2) - a_pos) * (devParms->fftVScale * 8.0 / h_fft);

            labelActive = LABEL_ACTIVE_FFT;

            mainwindow->labelTimer->start(LABEL_TIMER_IVAL);
        }

        update();

        return;
    }

    if(trigPosArrowMoving) {
        trigPosArrowPos = mouseX;

        if(trigPosArrowPos < 0)
            trigPosArrowPos = 0;

        if(trigPosArrowPos > w)
            trigPosArrowPos = w;
    } else if(trigLevelArrowMoving) {
        trigLevelArrowPos = mouseY;

        if(trigLevelArrowPos < 0)
            trigLevelArrowPos = 0;

        if(trigLevelArrowPos > h)
            trigLevelArrowPos = h;

        devParms->triggeredgelevel[devParms->triggeredgesource]
            = (((h / 2) - trigLevelArrowPos)
                  * ((devParms->chan[devParms->triggeredgesource].scale * devParms->vertDivisions) / h))
            - devParms->chan[devParms->triggeredgesource].offset;

        dtmp = devParms->triggeredgelevel[devParms->triggeredgesource]
            / (devParms->chan[devParms->triggeredgesource].scale / 50);

        devParms->triggeredgelevel[devParms->triggeredgesource]
            = (devParms->chan[devParms->triggeredgesource].scale / 50) * dtmp;

        labelActive = LABEL_ACTIVE_TRIG;

        mainwindow->labelTimer->start(LABEL_TIMER_IVAL);
    } else {
        for(chn = 0; chn < devParms->channelCnt; chn++) {
            if(!devParms->chan[chn].Display)
                continue;

            if(chan[chn].ArrowMoving) {
                chan[chn].ArrowPos = mouseY;

                if(chan[chn].ArrowPos < 0)
                    chan[chn].ArrowPos = 0;

                if(chan[chn].ArrowPos > h)
                    chan[chn].ArrowPos = h;

                devParms->chan[chn].offset = ((h / 2) - chan[chn].ArrowPos)
                    * ((devParms->chan[chn].scale * devParms->vertDivisions)
                        / h);

                //          chan[chn] = (h / 2) - chan_arrow_pos[chn]._tmp_y_pixel_offset;

                chan[chn].TmpYPixelOffset = chan[chn].TmpOldYPixelOffset - chan[chn].ArrowPos;

                dtmp = devParms->chan[chn].offset / (devParms->chan[chn].scale / 50);

                devParms->chan[chn].offset = (devParms->chan[chn].scale / 50) * dtmp;

                labelActive = chn + 1;

                mainwindow->labelTimer->start(LABEL_TIMER_IVAL);

                break;
            }
        }
    }

    update();
}

void SignalCurve::trig_lineTimer_handler() {
    trigLineVisible = 0;
}

void SignalCurve::setTrigLineVisible(void) {
    trigLineVisible = 1;

    trigLineTimer->start(1300);
}

void SignalCurve::trig_statTimer_handler() {
    if(!trigStatFlash) {
        trigStatTimer->stop();

        return;
    }

    if(trigStatFlash == 1)
        trigStatFlash = 2;
    else
        trigStatFlash = 1;
}

void SignalCurve::paintLabel(
    QPainter* painter, int xpos, int ypos, int xw, int yh, const char* str, QColor color) {
    QPainterPath path;

    path.addRoundedRect(xpos, ypos, xw, yh, 3, 3);

    painter->fillPath(path, Qt::black);

    painter->setPen(color);

    painter->drawRoundedRect(xpos, ypos, xw, yh, 3, 3);

    painter->drawText(xpos, ypos, xw, yh, Qt::AlignCenter, str);
}

void SignalCurve::paintCounterLabel(QPainter* painter, int xpos, int ypos) {

    char str[512];

    QPainterPath path;

    path.addRoundedRect(xpos, ypos, 175, 20, 3, 3);

    painter->fillPath(path, Qt::black);

    painter->setPen(Qt::darkGray);

    painter->drawRoundedRect(xpos, ypos, 175, 20, 3, 3);

    path = QPainterPath();

    path.addRoundedRect(xpos + 4, ypos + 3, 14, 14, 3, 3);

    painter->fillPath(path, SignalColor[devParms->countersrc - 1]);

    painter->setPen(Qt::black);

    painter->drawLine(xpos + 7, ypos + 6, xpos + 15, ypos + 6);

    painter->drawLine(xpos + 11, ypos + 6, xpos + 11, ypos + 14);

    painter->setPen(Qt::white);

    if((devParms->counterfreq < 15) || (devParms->counterfreq > 1.1e9)) {
        strlcpy(str, "< 15 Hz", 512);
    } else {
        convertToMetricSuffix(str, devParms->counterfreq, 5, 512);

        strlcat(str, "Hz", 512);
    }
    int i{};
    for(; i < 3; i++) {
        painter->drawLine(xpos + 22 + (i * 14), ypos + 14, xpos + 29 + (i * 14), ypos + 14);
        painter->drawLine(xpos + 29 + (i * 14), ypos + 14, xpos + 29 + (i * 14), ypos + 7);
        painter->drawLine(xpos + 29 + (i * 14), ypos + 7, xpos + 36 + (i * 14), ypos + 7);
        painter->drawLine(xpos + 36 + (i * 14), ypos + 7, xpos + 36 + (i * 14), ypos + 14);
    }
    painter->drawLine(xpos + 22 + (i * 14), ypos + 14, xpos + 29 + (i * 14), ypos + 14);

    painter->drawText(xpos + 75, ypos, 100, 20, Qt::AlignCenter, str);
}

void SignalCurve::paintPlaybackLabel(QPainter* painter, int xpos, int ypos) {
    char str[512];

    QPainterPath path;

    path.addRoundedRect(xpos, ypos, 175, 20, 3, 3);

    painter->fillPath(path, Qt::black);

    painter->setPen(Qt::darkGray);

    painter->drawRoundedRect(xpos, ypos, 175, 20, 3, 3);

    if(devParms->funcWrecOperate || !devParms->funcHasRecord) {
        painter->fillRect(xpos + 5, ypos + 5, 10, 10, Qt::red);

        painter->setPen(Qt::red);

        snprintf(str, 512, "%i/%i", 0, devParms->funcWrecFend);

        painter->drawText(xpos + 30, ypos, 120, 20, Qt::AlignCenter, str);
    } else {
        painter->fillRect(xpos + 5, ypos + 5, 10, 10, Qt::green);

        painter->setPen(Qt::green);

        snprintf(str, 512, "%i/%i", devParms->funcWplayFcur, devParms->funcWrecFend);

        painter->drawText(xpos + 30, ypos, 120, 20, Qt::AlignCenter, str);
    }
}

bool SignalCurve::hasMoveEvent(void) {
    if(useMoveEvents)
        return true;

    return false;
}

void SignalCurve::draw_decoder(QPainter* painter, int dw, int dh) {
    int j, k, cell_width, base_line, line_h_uart_tx = 0, line_h_uart_rx = 0, line_h_spi_mosi = 0,
                                     line_h_spi_miso = 0, spi_chars = 1, pixel_per_bit = 1;

    double pix_per_smpl;

    char str[512];

    if(devParms->modelSerie != 1)
        base_line = (dh / 2) - (((double)dh / 400.0) * devParms->mathDecodePos);
    else
        base_line = ((double)dh / 400.0) * devParms->mathDecodePos;

    pix_per_smpl = (double)dw / (devParms->horDivisions * 100);

    switch(devParms->mathDecodeFormat) {
    case 0:
        cell_width = 40; // hex
        break;
    case 1:
        cell_width = 30; // ASCII
        break;
    case 2:
        cell_width = 30; // decimal;
        break;
    case 3:
        cell_width = 70; // binary
        break;
    default:
        cell_width = 70; // line
        break;
    }

    if(devParms->mathDecodeMode == DECODE_MODE_UART) {
        if(devParms->timebasedelayenable)
            pixel_per_bit = ((double)dw / 12.0 / devParms->timebasedelayscale)
                / (double)devParms->mathDecodeUartBaud;
        else
            pixel_per_bit = ((double)dw / 12.0 / devParms->timebasescale)
                / (double)devParms->mathDecodeUartBaud;

        cell_width = pixel_per_bit * devParms->mathDecodeUartWidth;

        painter->setPen(Qt::green);

        if(devParms->mathDecodeUartTx && devParms->mathDecodeUartRx) {
            line_h_uart_tx = base_line - 5;

            line_h_uart_rx = base_line + 45;

            painter->drawLine(0, line_h_uart_tx, dw, line_h_uart_tx);

            painter->drawLine(0, line_h_uart_rx, dw, line_h_uart_rx);
        } else if(devParms->mathDecodeUartTx) {
            line_h_uart_tx = base_line;

            painter->drawLine(0, line_h_uart_tx, dw, line_h_uart_tx);
        } else if(devParms->mathDecodeUartRx) {
            line_h_uart_rx = base_line;

            painter->drawLine(0, line_h_uart_rx, dw, line_h_uart_rx);
        }

        if(devParms->mathDecodeUartTx) {
            for(int i{}; i < devParms->mathDecodeUartTxNval; i++) {
                painter->fillRect(devParms->mathDecodeUartTxValPos[i] * pix_per_smpl,
                    line_h_uart_tx - 13,
                    cell_width,
                    26,
                    Qt::black);

                painter->drawRect(devParms->mathDecodeUartTxValPos[i] * pix_per_smpl,
                    line_h_uart_tx - 13,
                    cell_width,
                    26);
            }
        }

        if(devParms->mathDecodeUartRx) {
            for(int i{}; i < devParms->mathDecodeUartRxNval; i++) {
                painter->fillRect(devParms->mathDecodeUartRxValPos[i] * pix_per_smpl,
                    line_h_uart_rx - 13,
                    cell_width,
                    26,
                    Qt::black);

                painter->drawRect(devParms->mathDecodeUartRxValPos[i] * pix_per_smpl,
                    line_h_uart_rx - 13,
                    cell_width,
                    26);
            }
        }

        painter->setPen(Qt::white);

        if(devParms->mathDecodeUartTx) {
            switch(devParms->mathDecodeFormat) {
            case 0:
                painter->drawText(5, line_h_uart_tx - 35, 65, 30, Qt::AlignCenter, "Tx[HEX]");
                break;
            case 1:
                painter->drawText(5, line_h_uart_tx - 35, 65, 30, Qt::AlignCenter, "Tx[ASC]");
                break;
            case 2:
                painter->drawText(5, line_h_uart_tx - 35, 65, 30, Qt::AlignCenter, "Tx[DEC]");
                break;
            case 3:
                painter->drawText(5, line_h_uart_tx - 35, 65, 30, Qt::AlignCenter, "Tx[BIN]");
                break;
            case 4:
                painter->drawText(5, line_h_uart_tx - 35, 65, 30, Qt::AlignCenter, "Tx[LINE]");
                break;
            default:
                painter->drawText(5, line_h_uart_tx - 35, 65, 30, Qt::AlignCenter, "Tx[\?\?\?]");
                break;
            }

            for(int i{}; i < devParms->mathDecodeUartTxNval; i++) {
                if(devParms->mathDecodeFormat == 0) // hex
                {
                    snprintf(str, 512, "%02X", devParms->mathDecodeUartTxVal[i]);

                    painter->drawText(devParms->mathDecodeUartTxValPos[i] * pix_per_smpl,
                        line_h_uart_tx - 13,
                        cell_width,
                        30,
                        Qt::AlignCenter,
                        str);
                } else if(devParms->mathDecodeFormat == 1) // ASCII
                {
                    ascii_decode_control_char(devParms->mathDecodeUartTxVal[i], str, 512);

                    painter->drawText(devParms->mathDecodeUartTxValPos[i] * pix_per_smpl,
                        line_h_uart_tx - 13,
                        cell_width,
                        30,
                        Qt::AlignCenter,
                        str);
                } else if(devParms->mathDecodeFormat == 2) // decimal
                {
                    snprintf(str, 512, "%u", (unsigned int)devParms->mathDecodeUartTxVal[i]);

                    painter->drawText(devParms->mathDecodeUartTxValPos[i] * pix_per_smpl,
                        line_h_uart_tx - 13,
                        cell_width,
                        30,
                        Qt::AlignCenter,
                        str);
                } else if(devParms->mathDecodeFormat == 3) // binary
                {
                    for(j = 0; j < devParms->mathDecodeUartWidth; j++)
                        str[devParms->mathDecodeUartWidth - 1 - j]
                            = ((devParms->mathDecodeUartTxVal[i] >> j) & 1) + '0';

                    str[j] = 0;

                    painter->drawText(devParms->mathDecodeUartTxValPos[i] * pix_per_smpl,
                        line_h_uart_tx - 13,
                        cell_width,
                        30,
                        Qt::AlignCenter,
                        str);
                } else if(devParms->mathDecodeFormat == 4) // line
                {
                    for(j = 0; j < devParms->mathDecodeUartWidth; j++)
                        str[j] = ((devParms->mathDecodeUartTxVal[i] >> j) & 1) + '0';

                    str[j] = 0;

                    painter->drawText(devParms->mathDecodeUartTxValPos[i] * pix_per_smpl,
                        line_h_uart_tx - 13,
                        cell_width,
                        30,
                        Qt::AlignCenter,
                        str);
                }

                if(devParms->mathDecodeUartTxErr[i]) {
                    painter->setPen(Qt::red);

                    painter->drawText(devParms->mathDecodeUartTxValPos[i] * pix_per_smpl
                            + cell_width,
                        line_h_uart_tx - 13,
                        25,
                        25,
                        Qt::AlignCenter,
                        "?");

                    painter->setPen(Qt::white);
                }
            }
        }

        if(devParms->mathDecodeUartRx) {
            switch(devParms->mathDecodeFormat) {
            case 0:
                painter->drawText(5, line_h_uart_rx - 35, 65, 30, Qt::AlignCenter, "Rx[HEX]");
                break;
            case 1:
                painter->drawText(5, line_h_uart_rx - 35, 65, 30, Qt::AlignCenter, "Rx[ASC]");
                break;
            case 2:
                painter->drawText(5, line_h_uart_rx - 35, 65, 30, Qt::AlignCenter, "Rx[DEC]");
                break;
            case 3:
                painter->drawText(5, line_h_uart_rx - 35, 65, 30, Qt::AlignCenter, "Rx[BIN]");
                break;
            case 4:
                painter->drawText(5, line_h_uart_rx - 35, 65, 30, Qt::AlignCenter, "Rx[LINE]");
                break;
            default:
                painter->drawText(5, line_h_uart_rx - 35, 65, 30, Qt::AlignCenter, "Rx[\?\?\?]");
                break;
            }

            for(int i{}; i < devParms->mathDecodeUartRxNval; i++) {
                if(devParms->mathDecodeFormat == 0) // hex
                {
                    snprintf(str, 512, "%02X", devParms->mathDecodeUartRxVal[i]);

                    painter->drawText(devParms->mathDecodeUartRxValPos[i] * pix_per_smpl,
                        line_h_uart_rx - 13,
                        cell_width,
                        30,
                        Qt::AlignCenter,
                        str);
                } else if(devParms->mathDecodeFormat == 1) // ASCII
                {
                    ascii_decode_control_char(devParms->mathDecodeUartRxVal[i], str, 512);

                    painter->drawText(devParms->mathDecodeUartRxValPos[i] * pix_per_smpl,
                        line_h_uart_rx - 13,
                        cell_width,
                        30,
                        Qt::AlignCenter,
                        str);
                } else if(devParms->mathDecodeFormat == 2) // decimal
                {
                    snprintf(str, 512, "%u", (unsigned int)devParms->mathDecodeUartRxVal[i]);

                    painter->drawText(devParms->mathDecodeUartRxValPos[i] * pix_per_smpl,
                        line_h_uart_rx - 13,
                        cell_width,
                        30,
                        Qt::AlignCenter,
                        str);
                } else if(devParms->mathDecodeFormat == 3) // binary
                {
                    for(j = 0; j < devParms->mathDecodeUartWidth; j++)
                        str[devParms->mathDecodeUartWidth - 1 - j]
                            = ((devParms->mathDecodeUartRxVal[i] >> j) & 1) + '0';

                    str[j] = 0;

                    painter->drawText(devParms->mathDecodeUartRxValPos[i] * pix_per_smpl,
                        line_h_uart_rx - 13,
                        cell_width,
                        30,
                        Qt::AlignCenter,
                        str);
                } else if(devParms->mathDecodeFormat == 4) // line
                {
                    for(j = 0; j < devParms->mathDecodeUartWidth; j++)
                        str[j] = ((devParms->mathDecodeUartRxVal[i] >> j) & 1) + '0';

                    str[j] = 0;

                    painter->drawText(devParms->mathDecodeUartRxValPos[i] * pix_per_smpl,
                        line_h_uart_rx - 13,
                        cell_width,
                        30,
                        Qt::AlignCenter,
                        str);
                }

                if(devParms->mathDecodeUartRxErr[i]) {
                    painter->setPen(Qt::red);

                    painter->drawText(devParms->mathDecodeUartRxValPos[i] * pix_per_smpl
                            + cell_width,
                        line_h_uart_rx - 13,
                        25,
                        25,
                        Qt::AlignCenter,
                        "?");

                    painter->setPen(Qt::white);
                }
            }
        }
    }

    if(devParms->mathDecodeMode == DECODE_MODE_SPI) {
        painter->setPen(Qt::green);

        if(devParms->mathDecodeSpiWidth > 24)
            spi_chars = 4;
        else if(devParms->mathDecodeSpiWidth > 16)
            spi_chars = 3;
        else if(devParms->mathDecodeSpiWidth > 8)
            spi_chars = 2;
        else
            spi_chars = 1;

        cell_width *= spi_chars;

        if(devParms->mathDecodeSpiMosi && devParms->mathDecodeSpiMiso) {
            line_h_spi_mosi = base_line - 5;

            line_h_spi_miso = base_line + 45;

            painter->drawLine(0, line_h_spi_mosi, dw, line_h_spi_mosi);

            painter->drawLine(0, line_h_spi_miso, dw, line_h_spi_miso);
        } else if(devParms->mathDecodeSpiMosi) {
            line_h_spi_mosi = base_line;

            painter->drawLine(0, line_h_spi_mosi, dw, line_h_spi_mosi);
        } else if(devParms->mathDecodeSpiMiso) {
            line_h_spi_miso = base_line;

            painter->drawLine(0, line_h_spi_miso, dw, line_h_spi_miso);
        }

        if(devParms->mathDecodeSpiMosi) {
            for(int i{}; i < devParms->mathDecodeSpiMosiNval; i++) {
                cell_width = (devParms->mathDecodeSpiMosiValPosEnd[i]
                                 - devParms->mathDecodeSpiMosiValPos[i])
                    * pix_per_smpl;

                painter->fillRect(devParms->mathDecodeSpiMosiValPos[i] * pix_per_smpl,
                    line_h_spi_mosi - 13,
                    cell_width,
                    26,
                    Qt::black);

                painter->drawRect(devParms->mathDecodeSpiMosiValPos[i] * pix_per_smpl,
                    line_h_spi_mosi - 13,
                    cell_width,
                    26);
            }
        }

        if(devParms->mathDecodeSpiMiso) {
            for(int i{}; i < devParms->mathDecodeSpiMisoNval; i++) {
                cell_width = (devParms->mathDecodeSpiMisoValPosEnd[i]
                                 - devParms->mathDecodeSpiMisoValPos[i])
                    * pix_per_smpl;

                painter->fillRect(devParms->mathDecodeSpiMisoValPos[i] * pix_per_smpl,
                    line_h_spi_miso - 13,
                    cell_width,
                    26,
                    Qt::black);

                painter->drawRect(devParms->mathDecodeSpiMisoValPos[i] * pix_per_smpl,
                    line_h_spi_miso - 13,
                    cell_width,
                    26);
            }
        }

        painter->setPen(Qt::white);

        if(devParms->mathDecodeSpiMosi) {
            switch(devParms->mathDecodeFormat) {
            case 0:
                painter->drawText(5, line_h_spi_mosi - 35, 80, 30, Qt::AlignCenter, "Mosi[HEX]");
                break;
            case 1:
                painter->drawText(5, line_h_spi_mosi - 35, 80, 30, Qt::AlignCenter, "Mosi[ASC]");
                break;
            case 2:
                painter->drawText(5, line_h_spi_mosi - 35, 80, 30, Qt::AlignCenter, "Mosi[DEC]");
                break;
            case 3:
                painter->drawText(5, line_h_spi_mosi - 35, 80, 30, Qt::AlignCenter, "Mosi[BIN]");
                break;
            case 4:
                painter->drawText(5, line_h_spi_mosi - 35, 80, 30, Qt::AlignCenter, "Mosi[LINE]");
                break;
            default:
                painter->drawText(5, line_h_spi_mosi - 35, 80, 30, Qt::AlignCenter, "Mosi[\?\?\?]");
                break;
            }

            for(int i{}; i < devParms->mathDecodeSpiMosiNval; i++) {
                if(devParms->mathDecodeFormat == 0) // hex
                {
                    switch(spi_chars) {
                    case 1:
                        snprintf(str, 512, "%02X", devParms->mathDecodeSpiMosiVal[i]);
                        break;
                    case 2:
                        snprintf(str, 512, "%04X", devParms->mathDecodeSpiMosiVal[i]);
                        break;
                    case 3:
                        snprintf(str, 512, "%06X", devParms->mathDecodeSpiMosiVal[i]);
                        break;
                    case 4:
                        snprintf(str, 512, "%08X", devParms->mathDecodeSpiMosiVal[i]);
                        break;
                    }

                    painter->drawText(devParms->mathDecodeSpiMosiValPos[i] * pix_per_smpl,
                        line_h_spi_mosi - 13,
                        cell_width,
                        30,
                        Qt::AlignCenter,
                        str);
                } else if(devParms->mathDecodeFormat == 1) // ASCII
                {
                    for(k = 0, j = 0; k < spi_chars; k++)
                        j += ascii_decode_control_char(devParms->mathDecodeSpiMosiVal[i]
                                >> (k * 8),
                            str + j,
                            512 - j);

                    painter->drawText(devParms->mathDecodeSpiMosiValPos[i] * pix_per_smpl,
                        line_h_spi_mosi - 13,
                        cell_width,
                        30,
                        Qt::AlignCenter,
                        str);
                } else if(devParms->mathDecodeFormat == 2) // decimal
                {
                    snprintf(str, 512, "%u", devParms->mathDecodeSpiMosiVal[i]);

                    painter->drawText(devParms->mathDecodeSpiMosiValPos[i] * pix_per_smpl,
                        line_h_spi_mosi - 13,
                        cell_width,
                        30,
                        Qt::AlignCenter,
                        str);
                } else if(devParms->mathDecodeFormat == 3) // binary
                {
                    for(j = 0; j < devParms->mathDecodeSpiWidth; j++)
                        str[devParms->mathDecodeSpiWidth - 1 - j]
                            = ((devParms->mathDecodeSpiMosiVal[i] >> j) & 1) + '0';

                    str[j] = 0;

                    painter->drawText(devParms->mathDecodeSpiMosiValPos[i] * pix_per_smpl,
                        line_h_spi_mosi - 13,
                        cell_width,
                        30,
                        Qt::AlignCenter,
                        str);
                } else if(devParms->mathDecodeFormat == 4) // line
                {
                    for(j = 0; j < devParms->mathDecodeSpiWidth; j++)
                        str[j] = ((devParms->mathDecodeSpiMosiVal[i] >> j) & 1) + '0';

                    str[devParms->mathDecodeSpiWidth] = 0;

                    painter->drawText(devParms->mathDecodeSpiMosiValPos[i] * pix_per_smpl,
                        line_h_spi_mosi - 13,
                        cell_width,
                        30,
                        Qt::AlignCenter,
                        str);
                }
            }
        }

        if(devParms->mathDecodeSpiMiso) {
            switch(devParms->mathDecodeFormat) {
            case 0:
                painter->drawText(5, line_h_spi_miso - 35, 80, 30, Qt::AlignCenter, "Miso[HEX]");
                break;
            case 1:
                painter->drawText(5, line_h_spi_miso - 35, 80, 30, Qt::AlignCenter, "Miso[HEX]");
                break;
            case 2:
                painter->drawText(5, line_h_spi_miso - 35, 80, 30, Qt::AlignCenter, "Miso[DEC]");
                break;
            case 3:
                painter->drawText(5, line_h_spi_miso - 35, 80, 30, Qt::AlignCenter, "Miso[BIN]");
                break;
            case 4:
                painter->drawText(5, line_h_spi_miso - 35, 80, 30, Qt::AlignCenter, "Miso[LINE]");
                break;
            default:
                painter->drawText(5, line_h_spi_miso - 35, 80, 30, Qt::AlignCenter, "Miso[\?\?\?]");
                break;
            }

            for(int i{}; i < devParms->mathDecodeSpiMisoNval; i++) {
                if(devParms->mathDecodeFormat == 0) // hex
                {
                    switch(spi_chars) {
                    case 1:
                        snprintf(str, 512, "%02X", devParms->mathDecodeSpiMisoVal[i]);
                        break;
                    case 2:
                        snprintf(str, 512, "%04X", devParms->mathDecodeSpiMisoVal[i]);
                        break;
                    case 3:
                        snprintf(str, 512, "%06X", devParms->mathDecodeSpiMisoVal[i]);
                        break;
                    case 4:
                        snprintf(str, 512, "%08X", devParms->mathDecodeSpiMisoVal[i]);
                        break;
                    }

                    painter->drawText(devParms->mathDecodeSpiMisoValPos[i] * pix_per_smpl,
                        line_h_spi_miso - 13,
                        cell_width,
                        30,
                        Qt::AlignCenter,
                        str);
                } else if(devParms->mathDecodeFormat == 1) // ASCII
                {
                    for(k = 0, j = 0; k < spi_chars; k++)
                        j += ascii_decode_control_char(devParms->mathDecodeSpiMisoVal[i]
                                >> (k * 8),
                            str + j,
                            512 - j);

                    painter->drawText(devParms->mathDecodeSpiMisoValPos[i] * pix_per_smpl,
                        line_h_spi_miso - 13,
                        cell_width,
                        30,
                        Qt::AlignCenter,
                        str);
                } else if(devParms->mathDecodeFormat == 2) // decimal
                {
                    snprintf(str, 512, "%u", devParms->mathDecodeSpiMisoVal[i]);

                    painter->drawText(devParms->mathDecodeSpiMisoValPos[i] * pix_per_smpl,
                        line_h_spi_miso - 13,
                        cell_width,
                        30,
                        Qt::AlignCenter,
                        str);
                } else if(devParms->mathDecodeFormat == 3) // binary
                {
                    for(j = 0; j < devParms->mathDecodeSpiWidth; j++)
                        str[devParms->mathDecodeSpiWidth - 1 - j]
                            = ((devParms->mathDecodeSpiMisoVal[i] >> j) & 1) + '0';

                    str[j] = 0;

                    painter->drawText(devParms->mathDecodeSpiMisoValPos[i] * pix_per_smpl,
                        line_h_spi_miso - 13,
                        cell_width,
                        30,
                        Qt::AlignCenter,
                        str);
                } else if(devParms->mathDecodeFormat == 4) // line
                {
                    for(j = 0; j < devParms->mathDecodeSpiWidth; j++)
                        str[j] = ((devParms->mathDecodeSpiMisoVal[i] >> j) & 1) + '0';

                    str[devParms->mathDecodeSpiWidth] = 0;

                    painter->drawText(devParms->mathDecodeSpiMisoValPos[i] * pix_per_smpl,
                        line_h_spi_miso - 13,
                        cell_width,
                        30,
                        Qt::AlignCenter,
                        str);
                }
            }
        }
    }
}

int SignalCurve::ascii_decode_control_char(char ch, char* str, int sz) {
    if((ch > 32) && (ch < 127)) {
        str[0] = ch;

        str[1] = 0;

        return 1;
    }

    switch(ch) {
    case 0:
        strlcpy(str, "nullptr", sz);
        break;
    case 1:
        strlcpy(str, "SOH", sz);
        break;
    case 2:
        strlcpy(str, "STX", sz);
        break;
    case 3:
        strlcpy(str, "ETX", sz);
        break;
    case 4:
        strlcpy(str, "EOT", sz);
        break;
    case 5:
        strlcpy(str, "ENQ", sz);
        break;
    case 6:
        strlcpy(str, "ACK", sz);
        break;
    case 7:
        strlcpy(str, "BEL", sz);
        break;
    case 8:
        strlcpy(str, "BS", sz);
        break;
    case 9:
        strlcpy(str, "HT", sz);
        break;
    case 10:
        strlcpy(str, "LF", sz);
        break;
    case 11:
        strlcpy(str, "VT", sz);
        break;
    case 12:
        strlcpy(str, "FF", sz);
        break;
    case 13:
        strlcpy(str, "CR", sz);
        break;
    case 14:
        strlcpy(str, "SO", sz);
        break;
    case 15:
        strlcpy(str, "SI", sz);
        break;
    case 16:
        strlcpy(str, "DLE", sz);
        break;
    case 17:
        strlcpy(str, "DC1", sz);
        break;
    case 18:
        strlcpy(str, "DC2", sz);
        break;
    case 19:
        strlcpy(str, "DC3", sz);
        break;
    case 20:
        strlcpy(str, "DC4", sz);
        break;
    case 21:
        strlcpy(str, "NAK", sz);
        break;
    case 22:
        strlcpy(str, "SYN", sz);
        break;
    case 23:
        strlcpy(str, "ETB", sz);
        break;
    case 24:
        strlcpy(str, "CAN", sz);
        break;
    case 25:
        strlcpy(str, "EM", sz);
        break;
    case 26:
        strlcpy(str, "SUB", sz);
        break;
    case 27:
        strlcpy(str, "ESC", sz);
        break;
    case 28:
        strlcpy(str, "FS", sz);
        break;
    case 29:
        strlcpy(str, "GS", sz);
        break;
    case 30:
        strlcpy(str, "RS", sz);
        break;
    case 31:
        strlcpy(str, "US", sz);
        break;
    case 32:
        strlcpy(str, "SP", sz);
        break;
    case 127:
        strlcpy(str, "DEL", sz);
        break;
    default:
        strlcpy(str, ".", sz);
        break;
    }

    return strlen(str);
}
