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
#pragma once

#include <QFont>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QWidget>
#include <QtGlobal>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "connection.h"
#include "global.h"
#include "mainwindow.h"
#include "tmc_dev.h"
#include "utils.h"

class UiMainWindow;

class SignalCurve : public QWidget {
    Q_OBJECT

public:
    SignalCurve(QWidget* parent = 0);

    QSize sizeHint() const { return minimumSizeHint(); }
    QSize minimumSizeHint() const { return QSize(30, 10); }

    int labelActive, trigLineVisible;

    unsigned int scrUpdateCntr;

    void setSignalColor1(QColor);
    void setSignalColor2(QColor);
    void setSignalColor3(QColor);
    void setSignalColor4(QColor);
    void setTraceWidth(int);
    void setBackgroundColor(QColor);
    void setRasterColor(QColor);
    void setTextColor(QColor);
    void setBorderSize(int);
    void drawCurve(struct DeviceSettings*, struct tmcDev*);
    void clear();
    void setUpdatesEnabled(bool);
    void setTrigLineVisible(void);
    void setDeviceParameters(struct DeviceSettings*);
    bool hasMoveEvent(void);

signals:
    void chan1Clicked();
    void chan2Clicked();
    void chan3Clicked();
    void chan4Clicked();

private slots:

    void trig_lineTimer_handler();
    void trig_statTimer_handler();

private:
    QColor SignalColor[MAX_CHNS], digColor[MAX_DIG_CHNS], BackgroundColor, RasterColor, TextColor;

    QTimer *trigLineTimer,
        *trigStatTimer;

    QFont smallFont;

    double vSense, fftVSense, fftVOffset;

    int bufSize;
    int borderSize;
    int traceWidth;
    int w;
    int h;
    int oldW;
    int updatesEnabled;
    struct Channel {
        int ArrowMoving;
        int ArrowPos;
        int TmpOldYPixelOffset;
        int TmpYPixelOffset;
    } chan[MAX_CHNS];
    int trigLevelArrowMoving;
    int trigPosArrowMoving;
    int useMoveEvents;
    int trigLevelArrowPos;
    int trigPosArrowPos;
    int trigStatFlash;
    int fftArrowPos;
    int fftArrowMoving;
    int mouseX;
    int mouseY;
    int mouseOldX;
    int mouseOldY;

    // clock_t clkStart, clkEnd;
    // double cpuTimeUsed;

    void drawWidget(QPainter*, int, int);
    void drawArrow(QPainter*, int, int, int, QColor, char);
    void drawSmallTriggerArrow(QPainter*, int, int, int, QColor);
    void drawTrigCenterArrow(QPainter*, int, int);
    void drawChanLabel(QPainter*, int, int, int);
    void drawTopLabels(QPainter*);
    void paintLabel(QPainter*, int, int, int, int, const char*, QColor);
    void paintCounterLabel(QPainter*, int, int);
    void paintPlaybackLabel(QPainter*, int, int);
    void drawFFT(QPainter*, int, int);
    void drawfpsLabel(QPainter*, int, int);
    void draw_decoder(QPainter*, int, int);
    int ascii_decode_control_char(char, char*, int);

    struct DeviceSettings* devParms;

    struct tmcDev* device;

    UiMainWindow* mainwindow;

protected:
    void paintEvent(QPaintEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void resizeEvent(QResizeEvent*);
};
