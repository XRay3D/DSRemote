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

#include "wave_view.h"

WaveCurve::WaveCurve(QWidget* w_parent)
    : QWidget(w_parent) {
    wavedialog = (UiWaveWindow*)w_parent;

    setAttribute(Qt::WA_OpaquePaintEvent);

    SignalColor[0] = Qt::blue;
    SignalColor[1] = Qt::blue;
    SignalColor[2] = Qt::blue;
    SignalColor[3] = Qt::blue;
    traceWidth = 0;
    BackgroundColor = Qt::gray;
    RasterColor = Qt::darkGray;
    TextColor = Qt::black;

    smallfont.setFamily("Arial");
    smallfont.setPixelSize(8);

    bufSize = 0;
    borderSize = 60;

    v_sense = 1;

    mouseX = 0;
    mouseY = 0;
    mouseOldX = 0;
    mouseOldY = 0;

    useMoveEvents = 0;

    oldW = 10000;

    devParms = nullptr;
}

void WaveCurve::paintEvent(QPaintEvent*) {
    int i, chn, small_rulers, h_trace_offset, w_trace_offset, curve_w, curve_h, sample_range,
        sample_start, sample_end, t_pos;

    double h_step = 0.0, samples_per_div, step, step2;

    if(devParms == nullptr)
        return;

    QPainter paint{this};

    QPainter* painter = &paint;

    smallfont.setPixelSize(devParms->fontSize);

    painter->setFont(smallfont);

    curve_w = width();

    curve_h = height();

    bufSize = devParms->waveBufsz;

    small_rulers = 5 * devParms->horDivisions;

    painter->fillRect(0, 0, curve_w, curve_h, BackgroundColor);

    if((curve_w < ((borderSize * 2) + 5)) || (curve_h < ((borderSize * 2) + 5)))
        return;

    painter->fillRect(0, 0, curve_w, 30, QColor(32, 32, 32));

    samples_per_div = devParms->samplerate * devParms->timebasescale;

    drawTopLabels(painter);

    t_pos = 408
        - ((devParms->timebaseoffset
               / ((double)devParms->acquirememdepth / devParms->samplerate))
            * 233);

    drawSmallTriggerArrow(painter, t_pos, 16, 1, QColor(255, 128, 0));

    painter->fillRect(0, curve_h - 30, curve_w, curve_h, QColor(32, 32, 32));

    for(int i{}; i < devParms->channelCnt; i++)
        drawChanLabel(painter, 8 + (i * 130), curve_h - 25, i);

    /////////////////////////////////// translate coordinates, draw and fill a rectangle ///////////////////////////////////////////

    painter->translate(borderSize, borderSize);

    curve_w -= (borderSize * 2);

    curve_h -= (borderSize * 2);

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
    } else {
        painter->drawLine(curve_w / 2, curve_h - 1, curve_w / 2, 0);

        painter->drawLine(0, curve_h / 2, curve_w - 1, curve_h / 2);
    }

    /////////////////////////////////// draw the arrows ///////////////////////////////////////////

    drawTrigCenterArrow(painter, curve_w / 2, 0);

    for(chn = 0; chn < devParms->channelCnt; chn++) {
        if(!devParms->chanDisplay[chn])
            continue;

        v_sense = ((double)curve_h
                      / ((devParms->chanscale[chn] * devParms->vertDivisions) / devParms->yinc[chn]))
            / -32.0;

        chanArrowPos[chn] = (curve_h / 2)
            - (devParms->chanoffset[chn]
                / ((devParms->chanscale[chn] * devParms->vertDivisions) / curve_h));

        if(chanArrowPos[chn] < 0) {
            chanArrowPos[chn] = -1;

            drawArrow(painter, -6, chanArrowPos[chn], 3, SignalColor[chn], '1' + chn);
        } else if(chanArrowPos[chn] > curve_h) {
            chanArrowPos[chn] = curve_h + 1;

            drawArrow(painter, -6, chanArrowPos[chn], 1, SignalColor[chn], '1' + chn);
        } else {
            drawArrow(painter, 0, chanArrowPos[chn], 0, SignalColor[chn], '1' + chn);
        }
    }

    /////////////////////////////////// draw the curve ///////////////////////////////////////////

    if(bufSize > 32) {
        painter->setClipping(true);
        painter->setClipRegion(QRegion(0, 0, curve_w, curve_h), Qt::ReplaceClip);

        h_step = (double)curve_w / (devParms->horDivisions * samples_per_div);

        sample_start = devParms->waveMemViewSampleStart;

        sample_end = devParms->horDivisions * samples_per_div + sample_start;

        if(sample_end > bufSize)
            sample_end = bufSize;

        sample_range = sample_end - sample_start;

        w_trace_offset = 0;

        //     if(bufsize != (devParms->hordivisions * samples_per_div))
        //     {
        //       if(devParms->timebaseoffset < 0)
        //       {
        //         w_trace_offset = curve_w - ((double)curve_w * ((double)bufsize / (double)(devParms->hordivisions * samples_per_div)));
        //       }
        //     }

        for(chn = 0; chn < devParms->channelCnt; chn++) {
            if(!devParms->chanDisplay[chn])
                continue;

            v_sense = ((double)curve_h
                          / ((devParms->chanscale[chn] * devParms->vertDivisions)
                              / devParms->yinc[chn]))
                / -32.0;

            h_trace_offset = curve_h / 2;

            h_trace_offset += (devParms->yor[chn] * v_sense * 32.0);

            painter->setPen(QPen(QBrush(SignalColor[chn], Qt::SolidPattern),
                traceWidth,
                Qt::SolidLine,
                Qt::SquareCap,
                Qt::BevelJoin));

            for(int i{}; i < sample_range; i++) {
                if(sample_range < (curve_w / 2)) {
                    if(devParms->displaytype) {
                        painter->drawPoint(i * h_step + w_trace_offset,
                            (devParms->waveBuf[chn][i + sample_start] * v_sense)
                                + h_trace_offset);
                    } else {
                        painter->drawLine(i * h_step + w_trace_offset,
                            (devParms->waveBuf[chn][i + sample_start] * v_sense)
                                + h_trace_offset,
                            (i + 1) * h_step + w_trace_offset,
                            (devParms->waveBuf[chn][i + sample_start] * v_sense)
                                + h_trace_offset);
                        if(i)
                            painter->drawLine(i * h_step + w_trace_offset,
                                (devParms->waveBuf[chn][i - 1 + sample_start]
                                    * v_sense)
                                    + h_trace_offset,
                                i * h_step + w_trace_offset,
                                (devParms->waveBuf[chn][i + sample_start] * v_sense)
                                    + h_trace_offset);
                    }
                } else if(i < (bufSize - 1)) {
                    if(devParms->displaytype)
                        painter->drawPoint(i * h_step + w_trace_offset,
                            (devParms->waveBuf[chn][i + sample_start] * v_sense)
                                + h_trace_offset);
                    else
                        painter->drawLine(i * h_step + w_trace_offset,
                            (devParms->waveBuf[chn][i + sample_start] * v_sense)
                                + h_trace_offset,
                            (i + 1) * h_step + w_trace_offset,
                            (devParms->waveBuf[chn][i + 1 + sample_start]
                                * v_sense)
                                + h_trace_offset);
                }
            }
        }

        painter->setClipping(false);
    }

    /////////////////////////////////// draw the decoder ///////////////////////////////////////////

    if(devParms->mathDecodeDisplay)
        drawDecoder(painter, curve_w, curve_h);

    /////////////////////////////////// draw the trigger arrows ///////////////////////////////////////////

    if(devParms->triggeredgesource < 4) {
        trigLevelArrowPos = (curve_h / 2)
            - ((devParms->triggeredgelevel[devParms->triggeredgesource]
                   + devParms->chanoffset[devParms->triggeredgesource])
                / ((devParms->chanscale[devParms->triggeredgesource]
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

    trigPosArrowPos = (curve_w / 2)
        - (((devParms->timebaseoffset + devParms->viewerCenterPosition)
               / (devParms->timebasescale * (double)devParms->horDivisions))
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

void WaveCurve::setDeviceParameters(struct DeviceSettings* devp) {
    devParms = devp;
}

void WaveCurve::drawTopLabels(QPainter* painter) {

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

    painter->fillPath(path, Qt::black);

    painter->setPen(Qt::green);

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

    dtmp1 = (devParms->horDivisions * devParms->timebasescale)
        / (devParms->acquirememdepth / devParms->samplerate);

    dtmp2 = devParms->viewerCenterPosition / (devParms->acquirememdepth / devParms->samplerate);

    painter->fillRect(288, 16, 233, 8, QColor(64, 160, 255));

    painter->fillRect(288 + 119 + ((233.0 * dtmp2) - (116.0 * dtmp1)), 16, 233 * dtmp1, 8, Qt::black);

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

    convertToMetricSuffix(str,
        devParms->timebaseoffset + devParms->viewerCenterPosition,
        4,
        512);

    strlcat(str, "s", 512);

    painter->drawText(570, 5, 85, 20, Qt::AlignCenter, str);

    //////////////// trigger ///////////////////////////////

    path = QPainterPath();

    path.addRoundedRect(685, 5, 125, 20, 3, 3);

    painter->fillPath(path, Qt::black);

    painter->setPen(Qt::gray);

    painter->drawText(670, 20, "T");

    convertToMetricSuffix(str, devParms->triggeredgelevel[devParms->triggeredgesource], 2, 512);

    strlcat(str, devParms->chanunitstr[devParms->chanunit[devParms->triggeredgesource]], 512);

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

void WaveCurve::drawArrow(QPainter* painter, int xpos, int ypos, int rot, QColor color, char ch) {
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

void WaveCurve::drawSmallTriggerArrow(QPainter* painter, int xpos, int ypos, int rot, QColor color) {
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

void WaveCurve::drawTrigCenterArrow(QPainter* painter, int xpos, int ypos) {
    QPainterPath path;

    path.moveTo(xpos + 7, ypos);
    path.lineTo(xpos - 6, ypos);
    path.lineTo(xpos, ypos + 7);
    path.lineTo(xpos + 7, ypos);
    painter->fillPath(path, QColor(255, 128, 0));
}

void WaveCurve::setSignalColor1(QColor newColor) {
    SignalColor[0] = newColor;
    update();
}

void WaveCurve::setSignalColor2(QColor newColor) {
    SignalColor[1] = newColor;
    update();
}

void WaveCurve::setSignalColor3(QColor newColor) {
    SignalColor[2] = newColor;
    update();
}

void WaveCurve::setSignalColor4(QColor newColor) {
    SignalColor[3] = newColor;
    update();
}

void WaveCurve::setTraceWidth(int tr_width) {
    traceWidth = tr_width;
    if(traceWidth < 0)
        traceWidth = 0;
    update();
}

void WaveCurve::setBackgroundColor(QColor newColor) {
    BackgroundColor = newColor;
    update();
}

void WaveCurve::setRasterColor(QColor newColor) {
    RasterColor = newColor;
    update();
}

void WaveCurve::setBorderSize(int newsize) {
    borderSize = newsize;
    if(borderSize < 0)
        borderSize = 0;
    update();
}

void WaveCurve::mousePressEvent(QMouseEvent*) // press_event)
{
    //   int m_x,
    //       m_y;

    setFocus(Qt::MouseFocusReason);

    w = width() - (2 * borderSize);
    h = height() - (2 * borderSize);

    //   m_x = press_event->x() - bordersize;
    //   m_y = press_event->y() - bordersize;

    if(devParms == nullptr)
        return;
}

void WaveCurve::mouseReleaseEvent(QMouseEvent* release_event) {
    w = width() - (2 * borderSize);
    h = height() - (2 * borderSize);

    mouseX = release_event->x() - borderSize;
    mouseY = release_event->y() - borderSize;

    if(devParms == nullptr)
        return;

    useMoveEvents = 0;
    setMouseTracking(false);

    update();
}

void WaveCurve::mouseMoveEvent(QMouseEvent* move_event) {
    if(!useMoveEvents)
        return;

    mouseX = move_event->x() - borderSize;
    mouseY = move_event->y() - borderSize;

    if(devParms == nullptr)
        return;

    if(!devParms->connected)
        return;

    update();
}

void WaveCurve::paintLabel(
    QPainter* painter, int xpos, int ypos, int xw, int yh, const char* str, QColor color) {
    QPainterPath path;

    path.addRoundedRect(xpos, ypos, xw, yh, 3, 3);

    painter->fillPath(path, Qt::black);

    painter->setPen(color);

    painter->drawRoundedRect(xpos, ypos, xw, yh, 3, 3);

    painter->drawText(xpos, ypos, xw, yh, Qt::AlignCenter, str);
}

void WaveCurve::drawDecoder(QPainter* painter, int dw, int dh) {
    int i, j, k, cell_width, base_line, line_h_uart_tx = 0, line_h_uart_rx = 0, line_h_spi_mosi = 0,
                                        line_h_spi_miso = 0, spi_chars = 1, pixel_per_bit = 1,
                                        samples_per_div, sample_start, sample_end;

    double pix_per_smpl;

    char str[512];

    painter->setClipping(true);
    painter->setClipRegion(QRegion(0, 0, dw, dh), Qt::ReplaceClip);

    samples_per_div = devParms->samplerate * devParms->timebasescale;

    sample_start = devParms->waveMemViewSampleStart;

    sample_end = devParms->horDivisions * samples_per_div + sample_start;

    if(sample_end > bufSize)
        sample_end = bufSize;

    if(devParms->modelSerie == 6 || devParms->modelSerie == 4)
        base_line = (dh / 2) - (((double)dh / 400.0) * devParms->mathDecodePos);
    else
        base_line = ((double)dh / 400.0) * devParms->mathDecodePos;

    pix_per_smpl = (double)dw / (devParms->horDivisions * samples_per_div);

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
        pixel_per_bit = ((double)dw / devParms->horDivisions / devParms->timebasescale)
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
                if((devParms->mathDecodeUartTxValPos[i] >= sample_start)
                    && (devParms->mathDecodeUartTxValPos[i] < sample_end)) {
                    painter->fillRect((devParms->mathDecodeUartTxValPos[i] - sample_start)
                            * pix_per_smpl,
                        line_h_uart_tx - 13,
                        cell_width,
                        26,
                        Qt::black);

                    painter->drawRect((devParms->mathDecodeUartTxValPos[i] - sample_start)
                            * pix_per_smpl,
                        line_h_uart_tx - 13,
                        cell_width,
                        26);
                }
            }
        }

        if(devParms->mathDecodeUartRx) {
            for(int i{}; i < devParms->mathDecodeUartRxNval; i++) {
                if((devParms->mathDecodeUartRxValPos[i] >= sample_start)
                    && (devParms->mathDecodeUartRxValPos[i] < sample_end)) {
                    painter->fillRect((devParms->mathDecodeUartRxValPos[i] - sample_start)
                            * pix_per_smpl,
                        line_h_uart_rx - 13,
                        cell_width,
                        26,
                        Qt::black);

                    painter->drawRect((devParms->mathDecodeUartRxValPos[i] - sample_start)
                            * pix_per_smpl,
                        line_h_uart_rx - 13,
                        cell_width,
                        26);
                }
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
                if((devParms->mathDecodeUartTxValPos[i] >= sample_start)
                    && (devParms->mathDecodeUartTxValPos[i] < sample_end)) {
                    if(devParms->mathDecodeFormat == 0) // hex
                    {
                        snprintf(str, 512, "%02X", devParms->mathDecodeUartTxVal[i]);

                        painter->drawText((devParms->mathDecodeUartTxValPos[i] - sample_start)
                                * pix_per_smpl,
                            line_h_uart_tx - 13,
                            cell_width,
                            30,
                            Qt::AlignCenter,
                            str);
                    } else if(devParms->mathDecodeFormat == 1) // ASCII
                    {
                        asciiDecodeControlChar(devParms->mathDecodeUartTxVal[i], str, 512);

                        painter->drawText((devParms->mathDecodeUartTxValPos[i] - sample_start)
                                * pix_per_smpl,
                            line_h_uart_tx - 13,
                            cell_width,
                            30,
                            Qt::AlignCenter,
                            str);
                    } else if(devParms->mathDecodeFormat == 2) // decimal
                    {
                        snprintf(str,
                            512,
                            "%u",
                            (unsigned int)devParms->mathDecodeUartTxVal[i]);

                        painter->drawText((devParms->mathDecodeUartTxValPos[i] - sample_start)
                                * pix_per_smpl,
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

                        painter->drawText((devParms->mathDecodeUartTxValPos[i] - sample_start)
                                * pix_per_smpl,
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

                        painter->drawText((devParms->mathDecodeUartTxValPos[i] - sample_start)
                                * pix_per_smpl,
                            line_h_uart_tx - 13,
                            cell_width,
                            30,
                            Qt::AlignCenter,
                            str);
                    }

                    if(devParms->mathDecodeUartTxErr[i]) {
                        painter->setPen(Qt::red);

                        painter->drawText((devParms->mathDecodeUartTxValPos[i] - sample_start)
                                    * pix_per_smpl
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
                if((devParms->mathDecodeUartRxValPos[i] >= sample_start)
                    && (devParms->mathDecodeUartRxValPos[i] < sample_end)) {
                    if(devParms->mathDecodeFormat == 0) // hex
                    {
                        snprintf(str, 512, "%02X", devParms->mathDecodeUartRxVal[i]);

                        painter->drawText((devParms->mathDecodeUartRxValPos[i] - sample_start)
                                * pix_per_smpl,
                            line_h_uart_rx - 13,
                            cell_width,
                            30,
                            Qt::AlignCenter,
                            str);
                    } else if(devParms->mathDecodeFormat == 1) // ASCII
                    {
                        asciiDecodeControlChar(devParms->mathDecodeUartRxVal[i], str, 512);

                        painter->drawText((devParms->mathDecodeUartRxValPos[i] - sample_start)
                                * pix_per_smpl,
                            line_h_uart_rx - 13,
                            cell_width,
                            30,
                            Qt::AlignCenter,
                            str);
                    } else if(devParms->mathDecodeFormat == 2) // decimal
                    {
                        snprintf(str,
                            512,
                            "%u",
                            (unsigned int)devParms->mathDecodeUartRxVal[i]);

                        painter->drawText((devParms->mathDecodeUartRxValPos[i] - sample_start)
                                * pix_per_smpl,
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

                        painter->drawText((devParms->mathDecodeUartRxValPos[i] - sample_start)
                                * pix_per_smpl,
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

                        painter->drawText((devParms->mathDecodeUartRxValPos[i] - sample_start)
                                * pix_per_smpl,
                            line_h_uart_rx - 13,
                            cell_width,
                            30,
                            Qt::AlignCenter,
                            str);
                    }

                    if(devParms->mathDecodeUartRxErr[i]) {
                        painter->setPen(Qt::red);

                        painter->drawText((devParms->mathDecodeUartRxValPos[i] - sample_start)
                                    * pix_per_smpl
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

                painter->fillRect((devParms->mathDecodeSpiMosiValPos[i] - sample_start)
                        * pix_per_smpl,
                    line_h_spi_mosi - 13,
                    cell_width,
                    26,
                    Qt::black);

                painter->drawRect((devParms->mathDecodeSpiMosiValPos[i] - sample_start)
                        * pix_per_smpl,
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

                painter->fillRect((devParms->mathDecodeSpiMisoValPos[i] - sample_start)
                        * pix_per_smpl,
                    line_h_spi_miso - 13,
                    cell_width,
                    26,
                    Qt::black);

                painter->drawRect((devParms->mathDecodeSpiMisoValPos[i] - sample_start)
                        * pix_per_smpl,
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

                    painter->drawText((devParms->mathDecodeSpiMosiValPos[i] - sample_start)
                            * pix_per_smpl,
                        line_h_spi_mosi - 13,
                        cell_width,
                        30,
                        Qt::AlignCenter,
                        str);
                } else if(devParms->mathDecodeFormat == 1) // ASCII
                {
                    for(k = 0, j = 0; k < spi_chars; k++)
                        j += asciiDecodeControlChar(devParms->mathDecodeSpiMosiVal[i]
                                >> (k * 8),
                            str + j,
                            512 - j);

                    painter->drawText((devParms->mathDecodeSpiMosiValPos[i] - sample_start)
                            * pix_per_smpl,
                        line_h_spi_mosi - 13,
                        cell_width,
                        30,
                        Qt::AlignCenter,
                        str);
                } else if(devParms->mathDecodeFormat == 2) // decimal
                {
                    snprintf(str, 512, "%u", devParms->mathDecodeSpiMosiVal[i]);

                    painter->drawText((devParms->mathDecodeSpiMosiValPos[i] - sample_start)
                            * pix_per_smpl,
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

                    painter->drawText((devParms->mathDecodeSpiMosiValPos[i] - sample_start)
                            * pix_per_smpl,
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

                    painter->drawText((devParms->mathDecodeSpiMosiValPos[i] - sample_start)
                            * pix_per_smpl,
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

                    painter->drawText((devParms->mathDecodeSpiMisoValPos[i] - sample_start)
                            * pix_per_smpl,
                        line_h_spi_miso - 13,
                        cell_width,
                        30,
                        Qt::AlignCenter,
                        str);
                } else if(devParms->mathDecodeFormat == 1) // ASCII
                {
                    for(k = 0, j = 0; k < spi_chars; k++)
                        j += asciiDecodeControlChar(devParms->mathDecodeSpiMisoVal[i]
                                >> (k * 8),
                            str + j,
                            512 - j);

                    painter->drawText((devParms->mathDecodeSpiMisoValPos[i] - sample_start)
                            * pix_per_smpl,
                        line_h_spi_miso - 13,
                        cell_width,
                        30,
                        Qt::AlignCenter,
                        str);
                } else if(devParms->mathDecodeFormat == 2) // decimal
                {
                    snprintf(str, 512, "%u", devParms->mathDecodeSpiMisoVal[i]);

                    painter->drawText((devParms->mathDecodeSpiMisoValPos[i] - sample_start)
                            * pix_per_smpl,
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

                    painter->drawText((devParms->mathDecodeSpiMisoValPos[i] - sample_start)
                            * pix_per_smpl,
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

                    painter->drawText((devParms->mathDecodeSpiMisoValPos[i] - sample_start)
                            * pix_per_smpl,
                        line_h_spi_miso - 13,
                        cell_width,
                        30,
                        Qt::AlignCenter,
                        str);
                }
            }
        }
    }

    painter->setClipping(false);
}

int WaveCurve::asciiDecodeControlChar(char ch, char* str, int sz) {
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

void WaveCurve::drawChanLabel(QPainter* painter, int xpos, int ypos, int chn) {
    QPainterPath path;

    char str1[4], str2[512];

    if(devParms == nullptr)
        return;

    str1[0] = '1' + chn;
    str1[1] = 0;

    convertToMetricSuffix(str2, devParms->chanscale[chn], 2, 512);

    strlcat(str2, devParms->chanunitstr[devParms->chanunit[chn]], 512);

    if(devParms->chanbwlimit[chn])
        strlcat(str2, " B", 512);

    if(devParms->chanDisplay[chn]) {
        path.addRoundedRect(xpos, ypos, 20, 20, 3, 3);

        path.addRoundedRect(xpos + 25, ypos, 85, 20, 3, 3);

        painter->fillPath(path, Qt::black);

        painter->setPen(SignalColor[chn]);

        painter->drawText(xpos + 6, ypos + 15, str1);

        if(devParms->chaninvert[chn])
            painter->drawLine(xpos + 6, ypos + 3, xpos + 14, ypos + 3);

        painter->drawText(xpos + 35, ypos + 1, 90, 20, Qt::AlignCenter, str2);

        if(devParms->chancoupling[chn] == 0) {
            painter->drawLine(xpos + 33, ypos + 6, xpos + 33, ypos + 10);

            painter->drawLine(xpos + 28, ypos + 10, xpos + 38, ypos + 10);

            painter->drawLine(xpos + 30, ypos + 12, xpos + 36, ypos + 12);

            painter->drawLine(xpos + 32, ypos + 14, xpos + 34, ypos + 14);
        } else if(devParms->chancoupling[chn] == 1) {
            painter->drawLine(xpos + 28, ypos + 8, xpos + 38, ypos + 8);

            painter->drawLine(xpos + 28, ypos + 12, xpos + 30, ypos + 12);

            painter->drawLine(xpos + 32, ypos + 12, xpos + 34, ypos + 12);

            painter->drawLine(xpos + 36, ypos + 12, xpos + 38, ypos + 12);
        } else if(devParms->chancoupling[chn] == 2) {
            painter->drawArc(xpos + 30, ypos + 8, 5, 5, 10 * 16, 160 * 16);

            painter->drawArc(xpos + 35, ypos + 8, 5, 5, -10 * 16, -160 * 16);
        }
    } else {
        path.addRoundedRect(xpos, ypos, 20, 20, 3, 3);

        path.addRoundedRect(xpos + 25, ypos, 85, 20, 3, 3);

        painter->fillPath(path, Qt::black);

        painter->setPen(QColor(48, 48, 48));

        painter->drawText(xpos + 6, ypos + 15, str1);

        painter->drawText(xpos + 30, ypos + 1, 85, 20, Qt::AlignCenter, str2);

        if(devParms->chanbwlimit[chn])
            painter->drawText(xpos + 90, ypos + 1, 20, 20, Qt::AlignCenter, "B");

        if(devParms->chancoupling[chn] == 0) {
            painter->drawLine(xpos + 33, ypos + 6, xpos + 33, ypos + 10);

            painter->drawLine(xpos + 28, ypos + 10, xpos + 38, ypos + 10);

            painter->drawLine(xpos + 30, ypos + 12, xpos + 36, ypos + 12);

            painter->drawLine(xpos + 32, ypos + 14, xpos + 34, ypos + 14);
        } else if(devParms->chancoupling[chn] == 1) {
            painter->drawLine(xpos + 28, ypos + 8, xpos + 38, ypos + 8);

            painter->drawLine(xpos + 28, ypos + 12, xpos + 30, ypos + 12);

            painter->drawLine(xpos + 32, ypos + 12, xpos + 34, ypos + 12);

            painter->drawLine(xpos + 36, ypos + 12, xpos + 38, ypos + 12);
        } else if(devParms->chancoupling[chn] == 2) {
            painter->drawArc(xpos + 30, ypos + 8, 5, 5, 10 * 16, 160 * 16);

            painter->drawArc(xpos + 35, ypos + 8, 5, 5, -10 * 16, -160 * 16);
        }
    }
}
