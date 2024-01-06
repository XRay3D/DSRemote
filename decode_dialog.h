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

#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QObject>
#include <QSpinBox>
#include <QTabWidget>
#include <QWidget>
#include <QtGlobal>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "mainwindow.h"
#include "utils.h"

class UiMainWindow;

class UiDecoderWindow : public QDialog {
    Q_OBJECT

public:
    UiDecoderWindow(QWidget* parent);

    UiMainWindow* mainWindow;

private:
    QWidget *tabPar,
        *tabUart,
        *tabSpi,
        *tabIic;

    QTabWidget* tabHolder;

    QLabel *spiClkSrcLabel,
        *spiMosiSrcLabel,
        *spiMisoSrcLabel,
        *spiCsSrcLabel,
        *spiSelectLabel,
        *spiModeLabel,
        *spiTimeoutLabel,
        *spiPolarityLabel,
        *spiEdgeLabel,
        *spiEndianLabel,
        *spiWidthLabel,
        *uartTxSrcLabel,
        *uartRxSrcLabel,
        *uartPolarityLabel,
        *uartEndianLabel,
        *uartBaudLabel,
        *uartWidthLabel,
        *uartStopLabel,
        *uartParityLabel,
        *threshold1Label,
        *threshold2Label,
        *threshold3Label,
        *threshold4Label,
        *tracePosLabel,
        *formatLabel;

    QComboBox *spiClkSrcCombobox,
        *spiMosiSrcCombobox,
        *spiMisoSrcCombobox,
        *spiCsSrcCombobox,
        *spiSelectCombobox,
        *spiModeCombobox,
        *spiPolarityCombobox,
        *spiEdgeCombobox,
        *spiEndianCombobox,
        *uartTxSrcCombobox,
        *uartRxSrcCombobox,
        *uartPolarityCombobox,
        *uartEndianCombobox,
        *uartWidthCombobox,
        *uartStopCombobox,
        *uartParityCombobox,
        *thresholdAutoCombobox,
        *formatCombobox;

    QSpinBox *spiWidthSpinbox,
        *uartBaudSpinbox,
        *tracePosSpinbox;

    QDoubleSpinBox *spiTimeoutDspinbox,
        *threshold1Dspinbox,
        *threshold2Dspinbox,
        *threshold3Dspinbox,
        *threshold4Dspinbox;

    QPushButton *closeButton,
        *toggleDecodeButton;

    struct DeviceSettings* devParms;

private slots:

    void thresholdAutoClicked(int);
    void threshold1DspinboxChanged();
    void threshold2DspinboxChanged();
    void threshold3DspinboxChanged();
    void threshold4DspinboxChanged();
    void tabHolderIndexChanged(int);
    void srcComboboxClicked(int);
    void spiSelectComboboxClicked(int);
    void spiModeComboboxClicked(int);
    void spiPolarityComboboxClicked(int);
    void spiEdgeComboboxClicked(int);
    void spiEndianComboboxClicked(int);
    void spiWidthSpinboxChanged();
    void spiTimeoutDspinboxChanged();
    void uartBaudSpinboxChanged();
    void uartPolarityComboboxClicked(int);
    void uartEndianComboboxClicked(int);
    void uartWidthComboboxClicked(int);
    void uartStopComboboxClicked(int);
    void uartParityComboboxClicked(int);
    void tracePosSpinboxChanged();
    void toggleDecode();
    void formatComboboxClicked(int);
};
