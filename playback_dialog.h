/*
***************************************************************************
*
* Author: Teunis van Beelen
*
* Copyright (C) 2017 - 2020 Teunis van Beelen
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

#include <QApplication>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QMessageBox>
#include <QObject>
#include <QSpinBox>
#include <QTimer>
#include <QWidget>
#include <QtGlobal>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "mainwindow.h"
#include "utils.h"

class UiMainWindow;

class UI_playback_window : public QDialog {
    Q_OBJECT

public:
    UI_playback_window(QWidget* parent);

    UiMainWindow* mainwindow;

private:
    QLabel *rec_fend_label,
        *rec_fint_label,
        *rep_fstart_label,
        *rep_fend_label,
        *rep_fint_label;

    QSpinBox *rec_fend_spinbox,
        *rep_fstart_spinbox,
        *rep_fend_spinbox;

    QDoubleSpinBox *rec_fint_spinbox,
        *rep_fint_spinbox;

    QPushButton *close_button,
        *toggle_playback_button;

    QTimer* t1;

    struct DeviceSettings* devParms;

    char rec_fmax_resp[128], rec_fend_resp[128], rec_fint_resp[128], rep_fstart_resp[128],
        rep_fend_resp[128], rep_fint_resp[128], rep_fmax_resp[128];

private slots:

    void toggle_playback();
    void t1_func();
    void rec_fend_spinbox_changed(int);
    void rec_fint_spinbox_changed(double);
    void rep_fstart_spinbox_changed(int);
    void rep_fend_spinbox_changed(int);
    void rep_fint_spinbox_changed(double);
};
