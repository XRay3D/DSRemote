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
#pragma once

#include <QBrush>
#include <QColor>
#include <QMouseEvent>
#include <QObject>
#include <QPainter>
#include <QRadialGradient>
#include <QWidget>
#include <QtGlobal>

class Dial : public QWidget {
    Q_OBJECT

public:
    Dial(QWidget* parent = 0);

    QSize sizeHint() const { return minimumSizeHint(); }
    QSize minimumSizeHint() const { return QSize(80, 80); }

    void setFgColor(QColor);
    void setBgColor(QColor);
    QColor getFgColor(void);
    QColor getBgColor(void);
    void setWrapping(bool);
    void setEnabled(bool);
    void setNotchesVisible(bool);
    void setMaximum(int);
    void setMinimum(int);
    void setValue(int);
    int value(void);
    void setSingleStep(int);
    bool isSliderDown(void);
    void setSliderPosition(int);

private:
    int dval, dvalOld, dmin, dmax, step;

    double dialGrad, mouseGradOld, degrPerStep;

    bool wrap, notchVisible, sliderDown, enabled;

    QColor fgColor, bgColor;

    void drawCirc(int, int, int, QPainter*);
    void drawFillCirc(int, int, int, QPainter*);
    double polarToDegr(double, double);
    void processRotation(void);

public slots:

protected:
    void paintEvent(QPaintEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void wheelEvent(QWheelEvent*);

signals:
    void valueChanged(int);
    void sliderReleased();
};
