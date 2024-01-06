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

#include "playback_dialog.h"

UI_playback_window::UI_playback_window(QWidget* w_parent) {
    mainwindow = (UiMainWindow*)w_parent;

    devParms = &mainwindow->devParms;

    mainwindow->setCueCmd(":FUNC:WREC:FMAX?", rec_fmax_resp);
    mainwindow->setCueCmd(":FUNC:WREC:FEND?", rec_fend_resp);
    if(devParms->modelSerie == 1) {
        mainwindow->setCueCmd(":FUNC:WREC:FINT?", rec_fint_resp);
        mainwindow->setCueCmd(":FUNC:WREP:FINT?", rep_fint_resp);
    } else {
        mainwindow->setCueCmd(":FUNC:WREC:INT?", rec_fint_resp);
        mainwindow->setCueCmd(":FUNC:WREP:INT?", rep_fint_resp);
    }
    mainwindow->setCueCmd(":FUNC:WREP:FST?", rep_fstart_resp);
    mainwindow->setCueCmd(":FUNC:WREP:FEND?", rep_fend_resp);
    mainwindow->setCueCmd(":FUNC:WREP:FMAX?", rep_fmax_resp);

    setWindowTitle("Record/Playback");

    setMinimumSize(420, 300);
    setMaximumSize(420, 300);

    rec_fend_label = new QLabel{this};
    rec_fend_label->setGeometry(20, 20, 150, 25);
    rec_fend_label->setText("Recording Length");

    rec_fend_spinbox = new QSpinBox{this};
    rec_fend_spinbox->setGeometry(200, 20, 140, 25);
    rec_fend_spinbox->setSuffix(" frames");
    if(!devParms->funcWrecEnable)
        rec_fend_spinbox->setEnabled(false);

    rec_fint_label = new QLabel{this};
    rec_fint_label->setGeometry(20, 65, 150, 25);
    rec_fint_label->setText("Recording Interval");

    rec_fint_spinbox = new QDoubleSpinBox{this};
    rec_fint_spinbox->setGeometry(200, 65, 140, 25);
    rec_fint_spinbox->setDecimals(7);
    rec_fint_spinbox->setRange(1e-7, 10);
    rec_fint_spinbox->setSuffix(" Sec.");
    if(!devParms->funcWrecEnable)
        rec_fint_spinbox->setEnabled(false);

    rep_fstart_label = new QLabel{this};
    rep_fstart_label->setGeometry(20, 110, 150, 25);
    rep_fstart_label->setText("Playback Start Frame");

    rep_fstart_spinbox = new QSpinBox{this};
    rep_fstart_spinbox->setGeometry(200, 110, 140, 25);
    if(!devParms->funcHasRecord || !devParms->funcWrecEnable)
        rep_fstart_spinbox->setEnabled(false);

    rep_fend_label = new QLabel{this};
    rep_fend_label->setGeometry(20, 155, 150, 25);
    rep_fend_label->setText("Playback End Frame");

    rep_fend_spinbox = new QSpinBox{this};
    rep_fend_spinbox->setGeometry(200, 155, 140, 25);
    if(!devParms->funcHasRecord || !devParms->funcWrecEnable)
        rep_fend_spinbox->setEnabled(false);

    rep_fint_label = new QLabel{this};
    rep_fint_label->setGeometry(20, 200, 150, 25);
    rep_fint_label->setText("Playback Interval");

    rep_fint_spinbox = new QDoubleSpinBox{this};
    rep_fint_spinbox->setGeometry(200, 200, 140, 25);
    rep_fint_spinbox->setDecimals(7);
    rep_fint_spinbox->setRange(1e-7, 10);
    rep_fint_spinbox->setSuffix(" Sec.");
    if(!devParms->funcHasRecord || !devParms->funcWrecEnable)
        rep_fint_spinbox->setEnabled(false);

    toggle_playback_button = new QPushButton{this};
    toggle_playback_button->setGeometry(20, 255, 100, 25);
    if(!devParms->funcWrecEnable)
        toggle_playback_button->setText("Enable");
    else
        toggle_playback_button->setText("Disable");
    toggle_playback_button->setAutoDefault(false);
    toggle_playback_button->setDefault(false);
    toggle_playback_button->setEnabled(false);

    close_button = new QPushButton{this};
    close_button->setGeometry(300, 255, 100, 25);
    close_button->setText("Close");
    close_button->setAutoDefault(false);
    close_button->setDefault(false);

    t1 = new QTimer{this};

    connect(close_button, SIGNAL(clicked()), this, SLOT(close()));
    connect(toggle_playback_button, SIGNAL(clicked()), this, SLOT(toggle_playback()));
    connect(t1, SIGNAL(timeout()), this, SLOT(t1_func()));

    t1->start(100);

    exec();
}

void UI_playback_window::t1_func() {
    if((rep_fint_resp[0] == 0) || (rep_fmax_resp[0] == 0) || (rep_fend_resp[0] == 0)
        || (rep_fstart_resp[0] == 0) || (rec_fend_resp[0] == 0) || (rec_fmax_resp[0] == 0)
        || (rec_fint_resp[0] == 0))
        return;

    t1->stop();

    devParms->funcWrecFmax = atoi(rec_fmax_resp);

    devParms->funcWrecFend = atoi(rec_fend_resp);

    devParms->funcWrecFintval = atof(rec_fint_resp);

    devParms->funcWplayFstart = atoi(rep_fstart_resp);

    devParms->funcWplayFend = atoi(rep_fend_resp);

    devParms->funcWplayFmax = atoi(rep_fmax_resp);

    devParms->funcWplayFintval = atof(rep_fint_resp);

    rec_fend_spinbox->setRange(1, devParms->funcWrecFmax);
    rec_fend_spinbox->setValue(devParms->funcWrecFend);
    rec_fint_spinbox->setValue(devParms->funcWrecFintval);
    rep_fstart_spinbox->setValue(devParms->funcWplayFstart);
    rep_fstart_spinbox->setRange(1, devParms->funcWrecFend);
    rep_fend_spinbox->setRange(1, devParms->funcWrecFend);
    rep_fend_spinbox->setValue(devParms->funcWplayFend);
    rep_fint_spinbox->setValue(devParms->funcWplayFintval);

    connect(rec_fend_spinbox, SIGNAL(valueChanged(int)), this, SLOT(rec_fend_spinbox_changed(int)));
    connect(rec_fint_spinbox,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(rec_fint_spinbox_changed(double)));
    connect(rep_fstart_spinbox,
        SIGNAL(valueChanged(int)),
        this,
        SLOT(rep_fstart_spinbox_changed(int)));
    connect(rep_fend_spinbox, SIGNAL(valueChanged(int)), this, SLOT(rep_fend_spinbox_changed(int)));
    connect(rep_fint_spinbox,
        SIGNAL(valueChanged(double)),
        this,
        SLOT(rep_fint_spinbox_changed(double)));

    toggle_playback_button->setEnabled(true);
}

void UI_playback_window::toggle_playback() {
    QMessageBox msgBox;
    msgBox.setText("Timebase scale must be <= 100mS.");

    if(!devParms->funcWrecEnable) {
        if(devParms->timebasedelayenable) {
            if(devParms->timebasedelayscale > 0.1000001) {
                msgBox.exec();
                return;
            }
        } else if(devParms->timebasescale > 0.1000001) {
            msgBox.exec();
            return;
        }

        devParms->funcWrecEnable = 1;

        toggle_playback_button->setText("Disable");

        rec_fend_spinbox->setEnabled(true);

        rec_fint_spinbox->setEnabled(true);

        mainwindow->statusLabel->setText("Recording enabled");

        if(devParms->modelSerie == 1)
            mainwindow->setCueCmd(":FUNC:WREC:ENAB ON");
        else
            mainwindow->setCueCmd(":FUNC:WRM REC");
    } else {
        devParms->funcWrecEnable = 0;

        devParms->funcHasRecord = 0;

        rec_fend_spinbox->setEnabled(false);

        rec_fint_spinbox->setEnabled(false);

        rep_fstart_spinbox->setEnabled(false);

        rep_fend_spinbox->setEnabled(false);

        rep_fint_spinbox->setEnabled(false);

        mainwindow->statusLabel->setText("Recording disabled");

        toggle_playback_button->setText("Enable");

        if(devParms->modelSerie == 1)
            mainwindow->setCueCmd(":FUNC:WREC:ENAB OFF");
        else
            mainwindow->setCueCmd(":FUNC:WRM OFF");
    }
}

void UI_playback_window::rec_fend_spinbox_changed(int fend) {
    char str[512];

    snprintf(str, 512, "Recording frame end: %i", fend);

    mainwindow->statusLabel->setText(str);

    snprintf(str, 512, ":FUNC:WREC:FEND %i", fend);

    mainwindow->setCueCmd(str);
}

void UI_playback_window::rec_fint_spinbox_changed(double fint) {
    char str[512];

    strlcpy(str, "Recording frame interval: ", 512);

    convertToMetricSuffix(str + strlen(str), 3, fint, 512);

    strlcat(str, "S", 512);

    mainwindow->statusLabel->setText(str);

    if(devParms->modelSerie == 1)
        snprintf(str, 512, ":FUNC:WREC:FINT %e", fint);
    else
        snprintf(str, 512, ":FUNC:WREC:INT %e", fint);

    mainwindow->setCueCmd(str);
}

void UI_playback_window::rep_fstart_spinbox_changed(int fstart) {
    char str[512];

    snprintf(str, 512, "Playback frame start: %i", fstart);

    mainwindow->statusLabel->setText(str);

    snprintf(str, 512, ":FUNC:WREP:FST %i", fstart);

    mainwindow->setCueCmd(str);
}

void UI_playback_window::rep_fend_spinbox_changed(int fend) {
    char str[512];

    snprintf(str, 512, "Playback frame end: %i", fend);

    mainwindow->statusLabel->setText(str);

    snprintf(str, 512, ":FUNC:WREP:FEND %i", fend);

    mainwindow->setCueCmd(str);
}

void UI_playback_window::rep_fint_spinbox_changed(double fint) {
    char str[512];

    strlcpy(str, "Playback frame interval: ", 512);

    convertToMetricSuffix(str + strlen(str), 3, fint, 512);

    strlcat(str, "S", 512);

    mainwindow->statusLabel->setText(str);

    if(devParms->modelSerie == 1)
        snprintf(str, 512, ":FUNC:WREP:FINT %e", fint);
    else
        snprintf(str, 512, ":FUNC:WREP:INT %e", fint);

    mainwindow->setCueCmd(str);
}
