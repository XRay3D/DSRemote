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

#include "dial.h"

#include <math.h>

Dial::Dial(QWidget* wParent)
    : QWidget(wParent)
    , fgColor{QColor(100, 100, 100)}
    , bgColor{Qt::lightGray}
    , notchVisible{false}
    , wrap{false}
    , dmin{0}
    , dmax{2}
    , step{1}
    , sliderDown{false}
    , dval{1}
    , dvalOld{1}
    , enabled{true}
    , mouseGradOld{180}
    , dialGrad{180}
    , degrPerStep{1} {
}

void Dial::paintEvent(QPaintEvent*) {
    int w, h, r0, horA, horB, vertA, vertB, gr;

    double r1, r2, r3, r4, dtmp;

    QPainter p{this};
    p.save();

    w = width();
    h = height();

    r0 = w;
    if(r0 > h)
        r0 = h;

    r0 /= 2;

    p.setRenderHint(QPainter::Antialiasing);

    if(enabled == true) {
        QRadialGradient gradient((w / 2) - (r0 * 0.5), (h / 2) - (r0 * 0.5), r0);
        gradient.setColorAt(0.001, QColor(250, 250, 250));
        gradient.setColorAt(0.9, QColor(228, 228, 228));
        gradient.setColorAt(1, QColor(215, 215, 215));
        p.setBrush(gradient);
        drawFillCirc(w / 2, h / 2, r0 * 0.8, &p);

        p.setPen(fgColor);
    } else {
        p.setPen(fgColor);

        drawCirc(w / 2, h / 2, r0 * 0.8, &p);
    }

    if(notchVisible == true) {
        r1 = r0;
        r2 = r0 * 0.85;
        r3 = r0 * 0.98;
        r4 = r0 * 0.9;

        if(r0 < 30)
            step = 20;
        else
            step = 12;

        for(gr = 0; gr < 360; gr += step) {
            if(wrap == false) {
                if((gr > 240) && (gr < 300))
                    continue;
            }
            if(gr % 60) {
                vertA = r3 * sin(gr * std::numbers::pi / 180.0);
                horA = r3 * cos(gr * std::numbers::pi / 180.0);

                vertB = r4 * sin(gr * std::numbers::pi / 180.0);
                horB = r4 * cos(gr * std::numbers::pi / 180.0);
            } else {
                vertA = r1 * sin(gr * std::numbers::pi / 180.0);
                horA = r1 * cos(gr * std::numbers::pi / 180.0);

                vertB = r2 * sin(gr * std::numbers::pi / 180.0);
                horB = r2 * cos(gr * std::numbers::pi / 180.0);
            }

            p.drawLine((w / 2) - horB, (h / 2) - vertB, (w / 2) - horA, (h / 2) - vertA);
        }
    }

    dtmp = dialGrad - 90;
    if(dtmp < 0)
        dtmp += 360;

    vertA = r0 * 0.55 * sin(dtmp * std::numbers::pi / 180.0);
    horA = r0 * 0.55 * cos(dtmp * std::numbers::pi / 180.0);

    if(enabled == true)
        p.setBrush(QBrush(QColor(210, 210, 210)));
    else
        p.setBrush(QBrush(QColor(220, 220, 220)));

    drawFillCirc(w / 2 - horA, h / 2 - vertA, r0 * 0.15, &p);

    p.restore();
}

void Dial::mousePressEvent(QMouseEvent* pressEvent) {
    if(pressEvent->button() == Qt::LeftButton) {
        sliderDown = true;
        setMouseTracking(true);

        mouseGradOld = polarToDegr(pressEvent->x() - (width() / 2),
            pressEvent->y() - (height() / 2));
    }

    pressEvent->accept();
}

void Dial::mouseReleaseEvent(QMouseEvent* releaseEvent) {
    if(releaseEvent->button() == Qt::LeftButton) {
        setMouseTracking(false);
        sliderDown = false;

        emit sliderReleased();
    }

    releaseEvent->accept();
}

void Dial::mouseMoveEvent(QMouseEvent* moveEvent) {
    double grNew, grDiff;

    if(sliderDown == false)
        return;

    grNew = polarToDegr(moveEvent->x() - (width() / 2), moveEvent->y() - (height() / 2));

    grDiff = grNew - mouseGradOld;

    mouseGradOld = grNew;

    dialGrad += grDiff;

    processRotation();

    moveEvent->accept();
}

bool Dial::isSliderDown(void) {
    return sliderDown;
}

void Dial::setWrapping(bool wr) {
    wrap = wr;
    update();
}

void Dial::setNotchesVisible(bool vis) {
    notchVisible = vis;
    update();
}

void Dial::setMaximum(int m) {
    dmax = m;
    if(dmax < 2)
        dmax = 2;
    if(dmin >= dmax)
        dmin = dmax - 1;
    if(dmin < 0)
        dmin = 0;
    if(wrap == true)
        degrPerStep = 360.0 / ((double)(dmax - dmin) / (double)step);
    else
        degrPerStep = 300.0 / ((double)(dmax - dmin) / (double)step);

    dval = (dmax + dmin) / 2;

    dvalOld = dval;

    update();
}

void Dial::setMinimum(int m) {
    dmin = m;
    if(dmin >= dmax)
        dmax = dmin + 1;
    if(dmax < 2)
        dmax = 2;
    if(dmin >= dmax)
        dmin = dmax - 1;
    if(dmin < 0)
        dmin = 0;
    if(wrap == true)
        degrPerStep = 360.0 / ((double)(dmax - dmin) / (double)step);
    else
        degrPerStep = 300.0 / ((double)(dmax - dmin) / (double)step);

    dval = (dmax + dmin) / 2;

    dvalOld = dval;

    update();
}

void Dial::setValue(int v) {
    dval = v;

    if(dval > dmax)
        dval = dmax;
    if(dval < dmin)
        dval = dmin;

    if(wrap == true)
        dialGrad = ((double)(dval - dmin) / (double)(dmax - dmin)) * 360.0;
    else
        dialGrad = (((double)(dval - dmin) / (double)(dmax - dmin)) * 300.0) + 30.0;

    dvalOld = dval;

    update();
}

void Dial::setSliderPosition(int v) {
    dval = v;

    if(dval > dmax)
        dval = dmax;
    if(dval < dmin)
        dval = dmin;

    if(wrap == true)
        dialGrad = ((double)(dval - dmin) / (double)(dmax - dmin)) * 360.0;
    else
        dialGrad = (((double)(dval - dmin) / (double)(dmax - dmin)) * 300.0) + 30.0;

    dvalOld = dval;

    update();
}

int Dial::value(void) {
    return dval;
}

void Dial::setSingleStep(int s) {
    step = s;
    if(step > dmax)
        step = dmax;
    if(step < 1)
        step = 1;
    if(wrap == true)
        degrPerStep = 360.0 / ((double)(dmax - dmin) / (double)step);
    else
        degrPerStep = 300.0 / ((double)(dmax - dmin) / (double)step);
    update();
}

void Dial::setFgColor(QColor newColor) {
    fgColor = newColor;
    update();
}

void Dial::setBgColor(QColor newColor) {
    bgColor = newColor;
    update();
}

QColor Dial::getFgColor(void) { return fgColor; }

QColor Dial::getBgColor(void) { return bgColor; }

void Dial::drawCirc(int px, int py, int r, QPainter* p) {
    p->drawArc(px - r, py - r, r * 2, r * 2, 0, 5760);
}

void Dial::drawFillCirc(int px, int py, int r, QPainter* p) {
    p->drawEllipse(px - r, py - r, r * 2, r * 2);
}

void Dial::setEnabled(bool enab) {
    enabled = enab;
    update();
}

double Dial::polarToDegr(double px, double py) {
    int quad = 0;

    double gr;

    if(px < 0) {
        quad += 2;

        px *= -1;
    }

    if(py < 0) {
        quad += 1;

        py *= -1;
    }

    if(px < 0.01)
        px = 0.01;
    if(py < 0.01)
        py = 0.01;

    gr = atan(py / px) * 180.0 / std::numbers::pi;

    switch(quad) {
    case 0:
        return gr += 270;
    case 1:
        return gr = 270 - gr;
    case 2:
        return gr = 90 - gr;
    case 3:
        return gr += 90;
    }
    return gr;
}

void Dial::wheelEvent(QWheelEvent* wheelEvent) {
    dialGrad += (wheelEvent->angleDelta /*delta*/ ().y() / 8);

    processRotation();

    wheelEvent->accept();
}

void Dial::processRotation(void) {
    if(dialGrad > 360)
        dialGrad -= 360;

    if(dialGrad < 0)
        dialGrad += 360;

    if(wrap == false) {
        if(dialGrad > 330)
            dialGrad = 330;
        else if(dialGrad < 30)
            dialGrad = 30;
    }

    if(wrap == false)
        dval = (dialGrad - 30) * ((double)(dmax - dmin) / 300.0);
    else
        dval = dialGrad * ((double)(dmax - dmin) / 360.0);

    if(dval == dvalOld)
        return;

    dvalOld = dval;

    emit valueChanged(dval);

    update();
}
