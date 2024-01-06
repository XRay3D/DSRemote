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

#include <QApplication>
#include <QDialog>
#include <QGridLayout>
#include <QMenu>
#include <QMenuBar>
#include <QObject>
#include <QSlider>

#include "global.h"
#include "mainwindow.h"
#include "wave_view.h"

class UiMainWindow;

class WaveCurve;

class UiWaveWindow : public QDialog {
    Q_OBJECT

public:
    UiWaveWindow(struct DeviceSettings*, short* wBuf[MAX_CHNS], QWidget* parent = 0);
    ~UiWaveWindow();

    void set_wavslider(void);

private:
    struct DeviceSettings* devParms;

    UiMainWindow* mainwindow;

    QMenuBar* menubar;

    QMenu *savemenu,
        *helpmenu;

    QGridLayout* g_layout;

    WaveCurve* wavcurve;

    QSlider* wavslider;

    QAction *former_page_act,
        *shift_page_left_act,
        *shift_page_right_act,
        *next_page_act,
        *zoom_in_act,
        *zoom_out_act,
        *center_position_act,
        *center_trigger_act;

private slots:

    void wavslider_value_changed(int);

    void former_page();
    void shift_page_left();
    void shift_page_right();
    void next_page();
    void zoom_in();
    void zoom_out();
    void center_position();
    void center_trigger();

    void save_wi_Buffer_to_edf();
};
