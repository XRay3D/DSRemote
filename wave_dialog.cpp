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

#include "wave_dialog.h"

UiWaveWindow::UiWaveWindow(struct DeviceSettings* p_devParms,
    short* wBuf[MAX_CHNS],
    QWidget* parnt) {

    mainwindow = (UiMainWindow*)parnt;

    setMinimumSize(840, 655);
    setWindowTitle("Wave Inspector");
    setWindowIcon(QIcon(":/images/r_dsremote.png"));

    devParms = (struct DeviceSettings*)calloc(1, sizeof(struct DeviceSettings));
    if(devParms == nullptr)
        printf("Malloc error! file: %s  line: %i", __FILE__, __LINE__);
    else
        *devParms = *p_devParms;

    for(int i{}; i < MAX_CHNS; i++)
        devParms->waveBuf[i] = wBuf[i];

    devParms->waveBufsz = devParms->acquirememdepth;

    if(devParms->timebasedelayenable)
        devParms->timebasescale = devParms->timebasedelayscale;

    devParms->timebasedelayenable = 0;

    devParms->viewerCenterPosition = 0;

    devParms->waveMemViewEnabled = 1;

    if(devParms->mathDecodeDisplay)
        mainwindow->serialDecoder(devParms);

    wavcurve = new WaveCurve;
    wavcurve->setBackgroundColor(Qt::black);
    wavcurve->setSignalColor1(Qt::yellow);
    wavcurve->setSignalColor2(Qt::cyan);
    wavcurve->setSignalColor3(Qt::magenta);
    wavcurve->setSignalColor4(QColor(0, 128, 255));
    wavcurve->setRasterColor(Qt::darkGray);
    wavcurve->setBorderSize(40);
    wavcurve->setDeviceParameters(devParms);

    wavslider = new QSlider;
    wavslider->setOrientation(Qt::Horizontal);
    set_wavslider();

    devParms->waveMemViewSampleStart = wavslider->value();

    menubar = new QMenuBar{this};

    savemenu = new QMenu{this};
    savemenu->setTitle("Save");
    savemenu->addAction("Save to EDF file", this, SLOT(save_wi_Buffer_to_edf()));
    menubar->addMenu(savemenu);

    helpmenu = new QMenu{this};
    helpmenu->setTitle("Help");
    helpmenu->addAction("How to operate", mainwindow, SLOT(helpButtonClicked()));
    helpmenu->addAction("About", mainwindow, SLOT(show_about_dialog()));
    menubar->addMenu(helpmenu);

    g_layout = new QGridLayout{this};
    g_layout->setMenuBar(menubar);
    g_layout->addWidget(wavcurve, 0, 0);
    g_layout->addWidget(wavslider, 1, 0);

    former_page_act = new QAction{this};
    former_page_act->setShortcut(QKeySequence::MoveToPreviousPage);
    connect(former_page_act, SIGNAL(triggered()), this, SLOT(former_page()));
    addAction(former_page_act);

    shift_page_left_act = new QAction{this};
    shift_page_left_act->setShortcut(QKeySequence::MoveToPreviousChar);
    connect(shift_page_left_act, SIGNAL(triggered()), this, SLOT(shift_page_left()));
    addAction(shift_page_left_act);

    center_position_act = new QAction{this};
    center_position_act->setShortcut(QKeySequence("c"));
    connect(center_position_act, SIGNAL(triggered()), this, SLOT(center_position()));
    addAction(center_position_act);

    center_trigger_act = new QAction{this};
    center_trigger_act->setShortcut(QKeySequence("t"));
    connect(center_trigger_act, SIGNAL(triggered()), this, SLOT(center_trigger()));
    addAction(center_trigger_act);

    shift_page_right_act = new QAction{this};
    shift_page_right_act->setShortcut(QKeySequence::MoveToNextChar);
    connect(shift_page_right_act, SIGNAL(triggered()), this, SLOT(shift_page_right()));
    addAction(shift_page_right_act);

    next_page_act = new QAction{this};
    next_page_act->setShortcut(QKeySequence::MoveToNextPage);
    connect(next_page_act, SIGNAL(triggered()), this, SLOT(next_page()));
    addAction(next_page_act);

    zoom_in_act = new QAction{this};
    zoom_in_act->setShortcut(QKeySequence::ZoomIn);
    connect(zoom_in_act, SIGNAL(triggered()), this, SLOT(zoom_in()));
    addAction(zoom_in_act);

    zoom_out_act = new QAction{this};
    zoom_out_act->setShortcut(QKeySequence::ZoomOut);
    connect(zoom_out_act, SIGNAL(triggered()), this, SLOT(zoom_out()));
    addAction(zoom_out_act);

    connect(wavslider, SIGNAL(sliderMoved(int)), this, SLOT(wavslider_value_changed(int)));

    show();
}

UiWaveWindow::~UiWaveWindow() {
    for(int i{}; i < MAX_CHNS; i++)
        free(devParms->waveBuf[i]);
    free(devParms);
}

void UiWaveWindow::save_wi_Buffer_to_edf() {
    mainwindow->saveWaveInspectorBufferToEdf(devParms);
}

void UiWaveWindow::wavslider_value_changed(int val) {
    devParms->waveMemViewSampleStart = val;

    int samples_per_div = devParms->samplerate * devParms->timebasescale;

    devParms->viewerCenterPosition
        = (double)(((devParms->waveBufsz - (devParms->horDivisions * samples_per_div)) / 2)
              - devParms->waveMemViewSampleStart)
        / devParms->samplerate * -1.0;

    devParms->viewerCenterPosition = roundTo3digits(devParms->viewerCenterPosition);

    wavcurve->update();
}

void UiWaveWindow::set_wavslider(void) {
    int samples_per_div = devParms->samplerate * devParms->timebasescale;

    wavslider->setRange(0, devParms->waveBufsz - (devParms->horDivisions * samples_per_div));

    devParms->waveMemViewSampleStart
        = ((devParms->waveBufsz - (devParms->horDivisions * samples_per_div)) / 2)
        + devParms->samplerate * devParms->viewerCenterPosition;

    wavslider->setValue(devParms->waveMemViewSampleStart);
}

void UiWaveWindow::former_page() {
    devParms->viewerCenterPosition -= devParms->timebasescale * devParms->horDivisions;

    if(devParms->viewerCenterPosition
        <= ((((double)devParms->acquirememdepth / devParms->samplerate)
                - (devParms->timebasescale * devParms->horDivisions))
            / -2))
        devParms->viewerCenterPosition = (((double)devParms->acquirememdepth
                                              / devParms->samplerate)
                                             - (devParms->timebasescale * devParms->horDivisions))
            / -2;

    set_wavslider();

    wavcurve->update();
}

void UiWaveWindow::next_page() {
    devParms->viewerCenterPosition += devParms->timebasescale * devParms->horDivisions;

    if(devParms->viewerCenterPosition
        >= ((((double)devParms->acquirememdepth / devParms->samplerate)
                - (devParms->timebasescale * devParms->horDivisions))
            / 2))
        devParms->viewerCenterPosition = (((double)devParms->acquirememdepth
                                              / devParms->samplerate)
                                             - (devParms->timebasescale * devParms->horDivisions))
            / 2;

    set_wavslider();

    wavcurve->update();
}

void UiWaveWindow::shift_page_left() {
    devParms->viewerCenterPosition -= devParms->timebasescale;

    if(devParms->viewerCenterPosition
        <= ((((double)devParms->acquirememdepth / devParms->samplerate)
                - (devParms->timebasescale * devParms->horDivisions))
            / -2))
        devParms->viewerCenterPosition = (((double)devParms->acquirememdepth
                                              / devParms->samplerate)
                                             - (devParms->timebasescale * devParms->horDivisions))
            / -2;

    set_wavslider();

    wavcurve->update();
}

void UiWaveWindow::shift_page_right() {
    devParms->viewerCenterPosition += devParms->timebasescale;

    if(devParms->viewerCenterPosition
        >= ((((double)devParms->acquirememdepth / devParms->samplerate)
                - (devParms->timebasescale * devParms->horDivisions))
            / 2))
        devParms->viewerCenterPosition = (((double)devParms->acquirememdepth
                                              / devParms->samplerate)
                                             - (devParms->timebasescale * devParms->horDivisions))
            / 2;

    set_wavslider();

    wavcurve->update();
}

void UiWaveWindow::center_position() {
    devParms->viewerCenterPosition = 0;

    set_wavslider();

    wavcurve->update();
}

void UiWaveWindow::center_trigger() {
    devParms->viewerCenterPosition = -devParms->timebaseoffset;

    if(devParms->viewerCenterPosition
        <= ((((double)devParms->acquirememdepth / devParms->samplerate)
                - (devParms->timebasescale * devParms->horDivisions))
            / -2))
        devParms->viewerCenterPosition = (((double)devParms->acquirememdepth
                                              / devParms->samplerate)
                                             - (devParms->timebasescale * devParms->horDivisions))
            / -2;

    if(devParms->viewerCenterPosition
        >= ((((double)devParms->acquirememdepth / devParms->samplerate)
                - (devParms->timebasescale * devParms->horDivisions))
            / 2))
        devParms->viewerCenterPosition = (((double)devParms->acquirememdepth
                                              / devParms->samplerate)
                                             - (devParms->timebasescale * devParms->horDivisions))
            / 2;

    set_wavslider();

    wavcurve->update();
}

void UiWaveWindow::zoom_in() {
    devParms->timebasescale = roundDownStep125(devParms->timebasescale, nullptr);

    if(devParms->timebasescale <= 1.001e-9)
        devParms->timebasescale = 1e-9;

    if(devParms->viewerCenterPosition
        <= ((((double)devParms->acquirememdepth / devParms->samplerate)
                - (devParms->timebasescale * devParms->horDivisions))
            / -2))
        devParms->viewerCenterPosition = (((double)devParms->acquirememdepth
                                              / devParms->samplerate)
                                             - (devParms->timebasescale * devParms->horDivisions))
            / -2;

    if(devParms->viewerCenterPosition
        >= ((((double)devParms->acquirememdepth / devParms->samplerate)
                - (devParms->timebasescale * devParms->horDivisions))
            / 2))
        devParms->viewerCenterPosition = (((double)devParms->acquirememdepth
                                              / devParms->samplerate)
                                             - (devParms->timebasescale * devParms->horDivisions))
            / 2;

    set_wavslider();

    wavcurve->update();
}

void UiWaveWindow::zoom_out() {
    double dtmp = roundUpStep125(devParms->timebasescale, nullptr);

    if(dtmp >= ((double)devParms->acquirememdepth / devParms->samplerate)
            / (double)devParms->horDivisions)
        return;

    devParms->timebasescale = roundUpStep125(devParms->timebasescale, nullptr);

    if(devParms->viewerCenterPosition
        <= ((((double)devParms->acquirememdepth / devParms->samplerate)
                - (devParms->timebasescale * devParms->horDivisions))
            / -2))
        devParms->viewerCenterPosition = (((double)devParms->acquirememdepth
                                              / devParms->samplerate)
                                             - (devParms->timebasescale * devParms->horDivisions))
            / -2;

    if(devParms->viewerCenterPosition
        >= ((((double)devParms->acquirememdepth / devParms->samplerate)
                - (devParms->timebasescale * devParms->horDivisions))
            / 2))
        devParms->viewerCenterPosition = (((double)devParms->acquirememdepth
                                              / devParms->samplerate)
                                             - (devParms->timebasescale * devParms->horDivisions))
            / 2;

    set_wavslider();

    wavcurve->update();
}
