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

#pragma once

#include <QFont>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <QtGlobal>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "utils.h"
#include "wave_dialog.h"

class UiWaveWindow;

class WaveCurve : public QWidget {
    Q_OBJECT

public:
    WaveCurve(QWidget* parent = 0);

    QSize sizeHint() const { return minimumSizeHint(); }
    QSize minimumSizeHint() const { return QSize(30, 10); }

    void setSignalColor1(QColor);
    void setSignalColor2(QColor);
    void setSignalColor3(QColor);
    void setSignalColor4(QColor);
    void setTraceWidth(int);
    void setBackgroundColor(QColor);
    void setRasterColor(QColor);
    void setTextColor(QColor);
    void setBorderSize(int);
    void setDeviceParameters(struct DeviceSettings*);

private slots:

private:
    QColor SignalColor[MAX_CHNS], BackgroundColor, RasterColor, TextColor;

    QFont smallfont;

    double v_sense;

    int bufSize,
        borderSize,
        traceWidth,
        w, h, oldW,
        chanArrowPos[MAX_CHNS],
        trigLevelArrowPos,
        trigPosArrowPos,
        useMoveEvents,
        mouseX,
        mouseY,
        mouseOldX,
        mouseOldY;

    void drawArrow(QPainter*, int, int, int, QColor, char);
    void drawSmallTriggerArrow(QPainter*, int, int, int, QColor);
    void drawTrigCenterArrow(QPainter*, int, int);
    void drawTopLabels(QPainter*);
    void drawChanLabel(QPainter*, int, int, int);
    void paintLabel(QPainter*, int, int, int, int, const char*, QColor);
    void drawDecoder(QPainter*, int, int);
    int asciiDecodeControlChar(char, char*, int);

    struct DeviceSettings* devParms;

    UiWaveWindow* wavedialog;

protected:
    void paintEvent(QPaintEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void mouseMoveEvent(QMouseEvent*);
};
