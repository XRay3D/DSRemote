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

#pragma once
#pragma once

#include <QPainter>
#include <QWidget>

class Led : public QWidget {
    Q_OBJECT

public:
    Led(QWidget* parent = 0);

    bool getValue() const { return value; }
    QColor getOnColor() const { return OnColor; }
    QColor getOffColor() const { return OffColor; }
    QSize sizeHint() const { return minimumSizeHint(); }
    QSize minimumSizeHint() const { return QSize(20, 20); }

public slots:
    void setValue(bool);
    void setOnColor(QColor);
    void setOffColor(QColor);

protected:
    void paintEvent(QPaintEvent*);
    QColor OnColor, OffColor;
    bool value;
    void drawLed(QPainter* painter);
};
