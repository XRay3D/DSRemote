/*
***************************************************************************
*
* Author: Teunis van Beelen
*
* Copyright (C) 2014 - 2020 Teunis van Beelen
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

#include "tled.h"

Led::Led(QWidget* p)
    : QWidget(p) {
    OnColor = QColor(Qt::green);
    OffColor = QColor(Qt::darkGreen);
    value = false;
}

void Led::paintEvent(QPaintEvent*) {
    QPainter paint{this};
    drawLed(&paint);
}

void Led::drawLed(QPainter* painter) {
    painter->save();

    const double w = width() - 1;
    const double h = height() - 1;
    const double m = std::min(w, h);
    const double n = m / 2;
    const double o = m / 3;
    const QColor color = value ? OnColor : OffColor;
    painter->setRenderHint(QPainter::Antialiasing);
    painter->translate(.5, .5);
    QRadialGradient gradient{n, n, n, o, o};
    if(value) gradient.setColorAt(0.001, Qt::white);
    gradient.setColorAt(0.9, color);
    gradient.setColorAt(1, Qt::black);
    painter->setBrush(gradient);
    painter->drawEllipse(0, 0, m, m);

    painter->restore();
}

void Led::setOnColor(QColor newColor) {
    OnColor = newColor;
    update();
}

void Led::setOffColor(QColor newColor) {
    OffColor = newColor;
    update();
}

void Led::setValue(bool newValue) {
    value = newValue;
    update();
}
