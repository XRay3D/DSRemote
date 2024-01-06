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

#include "decode_dialog.h"

UiDecoderWindow::UiDecoderWindow(QWidget* wParent) {
    mainWindow = (UiMainWindow*)wParent;

    devParms = &mainWindow->devParms;

    setWindowTitle("Decode");

    setMinimumSize(690, 525);
    setMaximumSize(690, 525);

    tabHolder = new QTabWidget{this};
    tabHolder->setGeometry(0, 0, 250, 455);

    tabPar = new QWidget;
    tabUart = new QWidget;
    tabSpi = new QWidget;
    tabIic = new QWidget;

    spiClkSrcLabel = new QLabel(tabSpi);
    spiClkSrcLabel->setGeometry(10, 20, 100, 25);
    spiClkSrcLabel->setText("Clock");

    spiClkSrcCombobox = new QComboBox(tabSpi);
    spiClkSrcCombobox->setGeometry(130, 20, 100, 25);
    spiClkSrcCombobox->addItem("Ch. 1");
    spiClkSrcCombobox->addItem("Ch. 2");
    if(devParms->channelCnt == 4) {
        spiClkSrcCombobox->addItem("Ch. 3");
        spiClkSrcCombobox->addItem("Ch. 4");
    }
    spiClkSrcCombobox->setCurrentIndex(devParms->mathDecodeSpiClk);

    threshold1Label = new QLabel{this};
    threshold1Label->setGeometry(270, 42, 100, 25);
    threshold1Label->setText("Threshold");

    threshold1Dspinbox = new QDoubleSpinBox{this};
    threshold1Dspinbox->setGeometry(370, 42, 100, 25);
    threshold1Dspinbox->setDecimals(3);
    threshold1Dspinbox->setRange(-20.0, 20.0);

    thresholdAutoCombobox = new QComboBox{this};
    thresholdAutoCombobox->setGeometry(510, 42, 140, 25);
    thresholdAutoCombobox->addItem("Manual threshold");
    thresholdAutoCombobox->addItem("Auto threshold");
    if(devParms->modelSerie != 1) {
        devParms->mathDecodeThresholdAuto = 0;
        thresholdAutoCombobox->setCurrentIndex(0);
        thresholdAutoCombobox->setEnabled(false);
    } else {
        thresholdAutoCombobox->setCurrentIndex(devParms->mathDecodeThresholdAuto);
    }

    spiMosiSrcLabel = new QLabel(tabSpi);
    spiMosiSrcLabel->setGeometry(10, 55, 100, 25);
    spiMosiSrcLabel->setText("MOSI");

    spiMosiSrcCombobox = new QComboBox(tabSpi);
    spiMosiSrcCombobox->setGeometry(130, 55, 100, 25);
    spiMosiSrcCombobox->addItem("Off");
    spiMosiSrcCombobox->addItem("Ch. 1");
    spiMosiSrcCombobox->addItem("Ch. 2");
    if(devParms->channelCnt == 4) {
        spiMosiSrcCombobox->addItem("Ch. 3");
        spiMosiSrcCombobox->addItem("Ch. 4");
    }
    spiMosiSrcCombobox->setCurrentIndex(devParms->mathDecodeSpiMosi);

    threshold2Label = new QLabel{this};
    threshold2Label->setGeometry(270, 77, 100, 25);
    threshold2Label->setText("Threshold");

    threshold2Dspinbox = new QDoubleSpinBox{this};
    threshold2Dspinbox->setGeometry(370, 77, 100, 25);
    threshold2Dspinbox->setDecimals(3);
    threshold2Dspinbox->setRange(-20.0, 20.0);

    spiMisoSrcLabel = new QLabel(tabSpi);
    spiMisoSrcLabel->setGeometry(10, 90, 100, 25);
    spiMisoSrcLabel->setText("MISO");

    spiMisoSrcCombobox = new QComboBox(tabSpi);
    spiMisoSrcCombobox->setGeometry(130, 90, 100, 25);
    spiMisoSrcCombobox->addItem("Off");
    spiMisoSrcCombobox->addItem("Ch. 1");
    spiMisoSrcCombobox->addItem("Ch. 2");
    if(devParms->channelCnt == 4) {
        spiMisoSrcCombobox->addItem("Ch. 3");
        spiMisoSrcCombobox->addItem("Ch. 4");
    }
    spiMisoSrcCombobox->setCurrentIndex(devParms->mathDecodeSpiMiso);

    threshold3Label = new QLabel{this};
    threshold3Label->setGeometry(270, 112, 100, 25);
    threshold3Label->setText("Threshold");

    threshold3Dspinbox = new QDoubleSpinBox{this};
    threshold3Dspinbox->setGeometry(370, 112, 100, 25);
    threshold3Dspinbox->setDecimals(3);
    threshold3Dspinbox->setRange(-20.0, 20.0);

    spiCsSrcLabel = new QLabel(tabSpi);
    spiCsSrcLabel->setGeometry(10, 125, 100, 25);
    spiCsSrcLabel->setText("CS");

    spiCsSrcCombobox = new QComboBox(tabSpi);
    spiCsSrcCombobox->setGeometry(130, 125, 100, 25);
    spiCsSrcCombobox->addItem("Off");
    spiCsSrcCombobox->addItem("Ch. 1");
    spiCsSrcCombobox->addItem("Ch. 2");
    if(devParms->channelCnt == 4) {
        spiCsSrcCombobox->addItem("Ch. 3");
        spiCsSrcCombobox->addItem("Ch. 4");
    }
    if(devParms->mathDecodeSpiMode)
        spiCsSrcCombobox->setCurrentIndex(devParms->mathDecodeSpiCs);

    threshold4Label = new QLabel{this};
    threshold4Label->setGeometry(270, 147, 100, 25);
    threshold4Label->setText("Threshold");

    threshold4Dspinbox = new QDoubleSpinBox{this};
    threshold4Dspinbox->setGeometry(370, 147, 100, 25);
    threshold4Dspinbox->setDecimals(3);
    threshold4Dspinbox->setRange(-20.0, 20.0);

    spiSelectLabel = new QLabel(tabSpi);
    spiSelectLabel->setGeometry(10, 160, 100, 25);
    spiSelectLabel->setText("CS active level");

    spiSelectCombobox = new QComboBox(tabSpi);
    spiSelectCombobox->setGeometry(130, 160, 100, 25);
    spiSelectCombobox->addItem("Low");
    spiSelectCombobox->addItem("High");
    spiSelectCombobox->setCurrentIndex(devParms->mathDecodeSpiSelect);

    formatLabel = new QLabel{this};
    formatLabel->setGeometry(270, 182, 100, 25);
    formatLabel->setText("Format");

    formatCombobox = new QComboBox{this};
    formatCombobox->setGeometry(370, 182, 100, 25);
    formatCombobox->addItem("Hexadecimal");
    formatCombobox->addItem("ASCII");
    formatCombobox->addItem("Decimal");
    formatCombobox->addItem("Binary");
    if(devParms->modelSerie == 1)
        formatCombobox->addItem("Line");
    formatCombobox->setCurrentIndex(devParms->mathDecodeFormat);

    spiModeLabel = new QLabel(tabSpi);
    spiModeLabel->setGeometry(10, 195, 100, 25);
    spiModeLabel->setText("Mode");

    spiModeCombobox = new QComboBox(tabSpi);
    spiModeCombobox->setGeometry(130, 195, 100, 25);
    spiModeCombobox->addItem("Timeout");
    spiModeCombobox->addItem("CS");
    spiModeCombobox->setCurrentIndex(devParms->mathDecodeSpiMode);

    spiTimeoutLabel = new QLabel(tabSpi);
    spiTimeoutLabel->setGeometry(10, 230, 100, 25);
    spiTimeoutLabel->setText("Timeout");

    spiTimeoutDspinbox = new QDoubleSpinBox(tabSpi);
    spiTimeoutDspinbox->setGeometry(110, 230, 120, 25);
    spiTimeoutDspinbox->setSuffix(" Sec.");
    spiTimeoutDspinbox->setDecimals(7);
    if(devParms->modelSerie != 1) {
        spiTimeoutDspinbox->setValue(0.0);
        spiTimeoutDspinbox->setEnabled(false);
    } else {
        spiTimeoutDspinbox->setRange(1e-7, 1.0);
        spiTimeoutDspinbox->setValue(devParms->mathDecodeSpiTimeout);
    }

    spiPolarityLabel = new QLabel(tabSpi);
    spiPolarityLabel->setGeometry(10, 265, 100, 25);
    spiPolarityLabel->setText("Polarity");

    spiPolarityCombobox = new QComboBox(tabSpi);
    spiPolarityCombobox->setGeometry(130, 265, 100, 25);
    spiPolarityCombobox->addItem("Negative");
    spiPolarityCombobox->addItem("Positive");
    spiPolarityCombobox->setCurrentIndex(devParms->mathDecodeSpiPol);

    spiEdgeLabel = new QLabel(tabSpi);
    spiEdgeLabel->setGeometry(10, 300, 100, 25);
    spiEdgeLabel->setText("Edge");

    spiEdgeCombobox = new QComboBox(tabSpi);
    spiEdgeCombobox->setGeometry(130, 300, 100, 25);
    spiEdgeCombobox->addItem("Falling");
    spiEdgeCombobox->addItem("Rising");
    spiEdgeCombobox->setCurrentIndex(devParms->mathDecodeSpiEdge);

    spiEndianLabel = new QLabel(tabSpi);
    spiEndianLabel->setGeometry(10, 335, 100, 25);
    spiEndianLabel->setText("Endian");

    spiEndianCombobox = new QComboBox(tabSpi);
    spiEndianCombobox->setGeometry(130, 335, 100, 25);
    spiEndianCombobox->addItem("LSB");
    spiEndianCombobox->addItem("MSB");
    spiEndianCombobox->setCurrentIndex(devParms->mathDecodeSpiEnd);

    spiWidthLabel = new QLabel(tabSpi);
    spiWidthLabel->setGeometry(10, 370, 100, 25);
    spiWidthLabel->setText("Width");

    spiWidthSpinbox = new QSpinBox(tabSpi);
    spiWidthSpinbox->setGeometry(130, 370, 100, 25);
    spiWidthSpinbox->setSuffix(" bits");
    spiWidthSpinbox->setRange(8, 32);
    spiWidthSpinbox->setValue(devParms->mathDecodeSpiWidth);

    tracePosLabel = new QLabel{this};
    tracePosLabel->setGeometry(270, 392, 100, 25);
    tracePosLabel->setText("Vertical position");

    tracePosSpinbox = new QSpinBox{this};
    tracePosSpinbox->setGeometry(370, 392, 100, 25);
    if(devParms->modelSerie != 1)
        tracePosSpinbox->setRange(-163, 143);
    else
        tracePosSpinbox->setRange(50, 350);
    tracePosSpinbox->setValue(devParms->mathDecodePos);

    uartTxSrcLabel = new QLabel(tabUart);
    uartTxSrcLabel->setGeometry(10, 20, 100, 25);
    uartTxSrcLabel->setText("TX");

    uartTxSrcCombobox = new QComboBox(tabUart);
    uartTxSrcCombobox->setGeometry(130, 20, 100, 25);
    uartTxSrcCombobox->addItem("Off");
    uartTxSrcCombobox->addItem("Ch. 1");
    uartTxSrcCombobox->addItem("Ch. 2");
    if(devParms->channelCnt == 4) {
        uartTxSrcCombobox->addItem("Ch. 3");
        uartTxSrcCombobox->addItem("Ch. 4");
    }
    uartTxSrcCombobox->setCurrentIndex(devParms->mathDecodeUartTx);

    uartRxSrcLabel = new QLabel(tabUart);
    uartRxSrcLabel->setGeometry(10, 55, 100, 25);
    uartRxSrcLabel->setText("RX");

    uartRxSrcCombobox = new QComboBox(tabUart);
    uartRxSrcCombobox->setGeometry(130, 55, 100, 25);
    uartRxSrcCombobox->addItem("Off");
    uartRxSrcCombobox->addItem("Ch. 1");
    uartRxSrcCombobox->addItem("Ch. 2");
    if(devParms->channelCnt == 4) {
        uartRxSrcCombobox->addItem("Ch. 3");
        uartRxSrcCombobox->addItem("Ch. 4");
    }
    uartRxSrcCombobox->setCurrentIndex(devParms->mathDecodeUartRx);

    uartPolarityLabel = new QLabel(tabUart);
    uartPolarityLabel->setGeometry(10, 90, 100, 25);
    uartPolarityLabel->setText("Polarity");

    uartPolarityCombobox = new QComboBox(tabUart);
    uartPolarityCombobox->setGeometry(130, 90, 100, 25);
    uartPolarityCombobox->addItem("Negative");
    uartPolarityCombobox->addItem("Positive");
    uartPolarityCombobox->setCurrentIndex(devParms->mathDecodeUartPol);

    uartEndianLabel = new QLabel(tabUart);
    uartEndianLabel->setGeometry(10, 125, 100, 25);
    uartEndianLabel->setText("Endian");

    uartEndianCombobox = new QComboBox(tabUart);
    uartEndianCombobox->setGeometry(130, 125, 100, 25);
    uartEndianCombobox->addItem("LSB");
    uartEndianCombobox->addItem("MSB");
    uartEndianCombobox->setCurrentIndex(devParms->mathDecodeUartEnd);

    uartBaudLabel = new QLabel(tabUart);
    uartBaudLabel->setGeometry(10, 160, 100, 25);
    uartBaudLabel->setText("Baudrate");

    uartBaudSpinbox = new QSpinBox(tabUart);
    uartBaudSpinbox->setGeometry(130, 160, 100, 25);
    uartBaudSpinbox->setRange(110, 20000000);
    uartBaudSpinbox->setValue(devParms->mathDecodeUartBaud);

    uartWidthLabel = new QLabel(tabUart);
    uartWidthLabel->setGeometry(10, 195, 100, 25);
    uartWidthLabel->setText("Data bits");

    uartWidthCombobox = new QComboBox(tabUart);
    uartWidthCombobox->setGeometry(130, 195, 100, 25);
    uartWidthCombobox->addItem("5");
    uartWidthCombobox->addItem("6");
    uartWidthCombobox->addItem("7");
    uartWidthCombobox->addItem("8");
    if((devParms->mathDecodeUartWidth >= 5) && (devParms->mathDecodeUartWidth <= 8))
        uartWidthCombobox->setCurrentIndex(devParms->mathDecodeUartWidth - 5);

    uartStopLabel = new QLabel(tabUart);
    uartStopLabel->setGeometry(10, 230, 100, 25);
    uartStopLabel->setText("Stop bits");

    uartStopCombobox = new QComboBox(tabUart);
    uartStopCombobox->setGeometry(130, 230, 100, 25);
    uartStopCombobox->addItem("1");
    uartStopCombobox->addItem("1.5");
    uartStopCombobox->addItem("2");
    uartStopCombobox->setCurrentIndex(devParms->mathDecodeUartStop);

    uartParityLabel = new QLabel(tabUart);
    uartParityLabel->setGeometry(10, 265, 100, 25);
    uartParityLabel->setText("Parity");

    uartParityCombobox = new QComboBox(tabUart);
    uartParityCombobox->setGeometry(130, 265, 100, 25);
    uartParityCombobox->addItem("None");
    uartParityCombobox->addItem("Odd");
    uartParityCombobox->addItem("Even");
    uartParityCombobox->setCurrentIndex(devParms->mathDecodeUartPar);

    thresholdAutoClicked(devParms->mathDecodeThresholdAuto);

    toggleDecodeButton = new QPushButton{this};
    toggleDecodeButton->setGeometry(20, 475, 100, 25);
    if(devParms->mathDecodeDisplay == 1)
        toggleDecodeButton->setText("Stop Decoding");
    else
        toggleDecodeButton->setText("Start Decoding");
    toggleDecodeButton->setAutoDefault(false);
    toggleDecodeButton->setDefault(false);

    closeButton = new QPushButton{this};
    closeButton->setGeometry(570, 475, 100, 25);
    closeButton->setText("Close");
    closeButton->setAutoDefault(false);
    closeButton->setDefault(false);

    tabHolder->addTab(tabPar, "Parallel");
    tabHolder->addTab(tabUart, "UART");
    tabHolder->addTab(tabSpi, "SPI");
    tabHolder->addTab(tabIic, "I2C");

    tabHolder->setCurrentIndex(devParms->mathDecodeMode);
    tabHolderIndexChanged(devParms->mathDecodeMode);

    connect(thresholdAutoCombobox,
        SIGNAL(currentIndexChanged(int)),
        this,
        SLOT(thresholdAutoClicked(int)));

    connect(uartTxSrcCombobox,
        SIGNAL(currentIndexChanged(int)),
        this,
        SLOT(srcComboboxClicked(int)));
    connect(uartRxSrcCombobox,
        SIGNAL(currentIndexChanged(int)),
        this,
        SLOT(srcComboboxClicked(int)));
    connect(uartPolarityCombobox,
        SIGNAL(currentIndexChanged(int)),
        this,
        SLOT(uartPolarityComboboxClicked(int)));
    connect(uartEndianCombobox,
        SIGNAL(currentIndexChanged(int)),
        this,
        SLOT(uartEndianComboboxClicked(int)));
    connect(uartWidthCombobox,
        SIGNAL(currentIndexChanged(int)),
        this,
        SLOT(uartWidthComboboxClicked(int)));
    connect(uartStopCombobox,
        SIGNAL(currentIndexChanged(int)),
        this,
        SLOT(uartStopComboboxClicked(int)));
    connect(uartParityCombobox,
        SIGNAL(currentIndexChanged(int)),
        this,
        SLOT(uartParityComboboxClicked(int)));

    connect(spiClkSrcCombobox,
        SIGNAL(currentIndexChanged(int)),
        this,
        SLOT(srcComboboxClicked(int)));
    connect(spiMosiSrcCombobox,
        SIGNAL(currentIndexChanged(int)),
        this,
        SLOT(srcComboboxClicked(int)));
    connect(spiMisoSrcCombobox,
        SIGNAL(currentIndexChanged(int)),
        this,
        SLOT(srcComboboxClicked(int)));
    connect(spiCsSrcCombobox,
        SIGNAL(currentIndexChanged(int)),
        this,
        SLOT(srcComboboxClicked(int)));

    connect(threshold1Dspinbox,
        SIGNAL(editingFinished()),
        this,
        SLOT(threshold1DspinboxChanged()));
    connect(threshold2Dspinbox,
        SIGNAL(editingFinished()),
        this,
        SLOT(threshold2DspinboxChanged()));
    connect(threshold3Dspinbox,
        SIGNAL(editingFinished()),
        this,
        SLOT(threshold3DspinboxChanged()));
    connect(threshold4Dspinbox,
        SIGNAL(editingFinished()),
        this,
        SLOT(threshold4DspinboxChanged()));

    connect(uartBaudSpinbox, SIGNAL(editingFinished()), this, SLOT(uartBaudSpinboxChanged()));
    connect(spiWidthSpinbox, SIGNAL(editingFinished()), this, SLOT(spiWidthSpinboxChanged()));
    connect(spiTimeoutDspinbox,
        SIGNAL(editingFinished()),
        this,
        SLOT(spiTimeoutDspinboxChanged()));
    connect(tracePosSpinbox, SIGNAL(editingFinished()), this, SLOT(tracePosSpinboxChanged()));

    connect(tabHolder, SIGNAL(currentChanged(int)), this, SLOT(tabHolderIndexChanged(int)));

    connect(spiSelectCombobox,
        SIGNAL(currentIndexChanged(int)),
        this,
        SLOT(spiSelectComboboxClicked(int)));
    connect(spiModeCombobox,
        SIGNAL(currentIndexChanged(int)),
        this,
        SLOT(spiModeComboboxClicked(int)));
    connect(spiPolarityCombobox,
        SIGNAL(currentIndexChanged(int)),
        this,
        SLOT(spiPolarityComboboxClicked(int)));
    connect(spiEdgeCombobox,
        SIGNAL(currentIndexChanged(int)),
        this,
        SLOT(spiEdgeComboboxClicked(int)));
    connect(spiEndianCombobox,
        SIGNAL(currentIndexChanged(int)),
        this,
        SLOT(spiEndianComboboxClicked(int)));
    connect(formatCombobox,
        SIGNAL(currentIndexChanged(int)),
        this,
        SLOT(formatComboboxClicked(int)));

    connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(toggleDecodeButton, SIGNAL(clicked()), this, SLOT(toggleDecode()));

    exec();
}

void UiDecoderWindow::uartPolarityComboboxClicked(int idx) {
    devParms->mathDecodeUartPol = idx;

    if(devParms->modelSerie != 1)
        if(idx == 0)
            mainWindow->setCueCmd(":BUS1:RS232:POL NEG");
        else
            mainWindow->setCueCmd(":BUS1:RS232:POL POS");
    else if(idx == 0)
        mainWindow->setCueCmd(":DEC1:UART:POL NEG");
    else
        mainWindow->setCueCmd(":DEC1:UART:POL POS");
}

void UiDecoderWindow::uartEndianComboboxClicked(int idx) {
    devParms->mathDecodeUartEnd = idx;

    if(devParms->modelSerie != 1)
        if(idx == 0)
            mainWindow->setCueCmd(":BUS1:RS232:END LSB");
        else
            mainWindow->setCueCmd(":BUS1:RS232:END MSB");
    else if(idx == 0)
        mainWindow->setCueCmd(":DEC1:UART:END LSB");
    else
        mainWindow->setCueCmd(":DEC1:UART:END MSB");
}

void UiDecoderWindow::uartBaudSpinboxChanged() {
    char str[512];

    devParms->mathDecodeUartBaud = uartBaudSpinbox->value();

    if(devParms->modelSerie != 1)
        snprintf(str, 512, ":BUS1:RS232:BAUD %i", devParms->mathDecodeUartBaud);
    else
        snprintf(str, 512, ":DEC1:UART:BAUD %i", devParms->mathDecodeUartBaud);

    mainWindow->setCueCmd(str);
}

void UiDecoderWindow::uartWidthComboboxClicked(int idx) {
    char str[512];

    idx += 5;

    devParms->mathDecodeUartWidth = idx;

    if(devParms->modelSerie != 1)
        snprintf(str, 512, ":BUS1:RS232:DBIT %i", idx);
    else
        snprintf(str, 512, ":DEC1:UART:WIDT %i", idx);

    mainWindow->setCueCmd(str);
}

void UiDecoderWindow::uartStopComboboxClicked(int idx) {
    devParms->mathDecodeUartStop = idx;

    if(devParms->modelSerie != 1) {
        if(idx == 0)
            mainWindow->setCueCmd(":BUS1:RS232:SBIT 1");
        else if(idx == 1)
            mainWindow->setCueCmd(":BUS1:RS232:SBIT 1.5");
        else if(idx == 2)
            mainWindow->setCueCmd(":BUS1:RS232:SBIT 2");
    } else {
        if(idx == 0)
            mainWindow->setCueCmd(":DEC1:UART:STOP 1");
        else if(idx == 1)
            mainWindow->setCueCmd(":DEC1:UART:STOP 1.5");
        else if(idx == 2)
            mainWindow->setCueCmd(":DEC1:UART:STOP 2");
    }
}

void UiDecoderWindow::uartParityComboboxClicked(int idx) {
    devParms->mathDecodeUartPar = idx;

    if(devParms->modelSerie != 1) {
        if(idx == 0)
            mainWindow->setCueCmd(":BUS1:RS232:PAR NONE");
        else if(idx == 1)
            mainWindow->setCueCmd(":BUS1:RS232:PAR ODD");
        else if(idx == 2)
            mainWindow->setCueCmd(":BUS1:RS232:PAR EVEN");
    } else {
        if(idx == 0)
            mainWindow->setCueCmd(":DEC1:UART:PAR NONE");
        else if(idx == 1)
            mainWindow->setCueCmd(":DEC1:UART:PAR ODD");
        else if(idx == 2)
            mainWindow->setCueCmd(":DEC1:UART:PAR EVEN");
    }
}

void UiDecoderWindow::spiWidthSpinboxChanged() {
    char str[512];

    devParms->mathDecodeSpiWidth = spiWidthSpinbox->value();

    if(devParms->modelSerie != 1)
        snprintf(str, 512, ":BUS1:SPI:DBIT %i", devParms->mathDecodeSpiWidth);
    else
        snprintf(str, 512, ":DEC1:SPI:WIDT %i", devParms->mathDecodeSpiWidth);

    mainWindow->setCueCmd(str);
}

void UiDecoderWindow::spiTimeoutDspinboxChanged() {
    char str[512];

    devParms->mathDecodeSpiTimeout = spiTimeoutDspinbox->value();

    if(devParms->modelSerie == 1) {
        snprintf(str, 512, ":DEC1:SPI:TIM %e", devParms->mathDecodeSpiTimeout);

        mainWindow->setCueCmd(str);
    }
}

void UiDecoderWindow::tracePosSpinboxChanged() {
    char str[512];

    devParms->mathDecodePos = tracePosSpinbox->value();

    if(devParms->modelSerie != 1) {
        if(devParms->mathDecodeMode == DECODE_MODE_SPI)
            snprintf(str, 512, ":BUS1:SPI:OFFS %i", devParms->mathDecodePos);
        else if(devParms->mathDecodeMode == DECODE_MODE_UART)
            snprintf(str, 512, ":BUS1:RS232:OFFS %i", devParms->mathDecodePos);
        else if(devParms->mathDecodeMode == DECODE_MODE_I2C)
            snprintf(str, 512, ":BUS1:IIC:OFFS %i", devParms->mathDecodePos);
        else if(devParms->mathDecodeMode == DECODE_MODE_PAR)
            snprintf(str, 512, ":BUS1:PARA:OFFS %i", devParms->mathDecodePos);
    } else {
        snprintf(str, 512, ":DEC1:POS %i", devParms->mathDecodePos);
    }

    mainWindow->setCueCmd(str);
}

void UiDecoderWindow::spiSelectComboboxClicked(int idx) {
    devParms->mathDecodeSpiSelect = idx;

    if(devParms->modelSerie != 1)
        if(idx == 0)
            mainWindow->setCueCmd(":BUS1:SPI:SS:POL NEG");
        else
            mainWindow->setCueCmd(":BUS1:SPI:SS:POL POS");
    else if(idx == 0)
        mainWindow->setCueCmd(":DEC1:SPI:SEL NCS");
    else
        mainWindow->setCueCmd(":DEC1:SPI:SEL CS");
}

void UiDecoderWindow::spiModeComboboxClicked(int idx) {
    char str[512];

    devParms->mathDecodeSpiMode = idx;

    if(devParms->modelSerie == 1) {
        if(idx == 0) {
            mainWindow->setCueCmd(":DEC1:SPI:MODE TIM");

            spiCsSrcCombobox->setCurrentIndex(0);

            devParms->mathDecodeSpiCs = 0;
        } else {
            mainWindow->setCueCmd(":DEC1:SPI:MODE CS");

            if(spiCsSrcCombobox->currentIndex() > 0) {
                snprintf(str, 512, ":DEC1:SPI:CS CHAN%i", spiCsSrcCombobox->currentIndex());

                mainWindow->setCueCmd(str);
            }
        }
    }
}

void UiDecoderWindow::spiPolarityComboboxClicked(int idx) {
    devParms->mathDecodeSpiPol = idx;

    if(devParms->modelSerie != 1) {
        if(idx == 0) {
            mainWindow->setCueCmd(":BUS1:SPI:MISO:POL NEG");
            mainWindow->setCueCmd(":BUS1:SPI:MOSI:POL NEG");
        } else {
            mainWindow->setCueCmd(":BUS1:SPI:MISO:POL POS");
            mainWindow->setCueCmd(":BUS1:SPI:MOSI:POL POS");
        }
    } else {
        if(idx == 0)
            mainWindow->setCueCmd(":DEC1:SPI:POL NEG");
        else
            mainWindow->setCueCmd(":DEC1:SPI:POL POS");
    }
}

void UiDecoderWindow::spiEdgeComboboxClicked(int idx) {
    devParms->mathDecodeSpiEdge = idx;

    if(devParms->modelSerie != 1)
        if(idx == 0)
            mainWindow->setCueCmd(":BUS1:SPI:SCLK:SLOP NEG");
        else
            mainWindow->setCueCmd(":BUS1:SPI:SCLK:SLOP POS");
    else if(idx == 0)
        mainWindow->setCueCmd(":DEC1:SPI:EDGE FALL");
    else
        mainWindow->setCueCmd(":DEC1:SPI:EDGE RISE");
}

void UiDecoderWindow::spiEndianComboboxClicked(int idx) {
    devParms->mathDecodeSpiEnd = idx;

    if(devParms->modelSerie != 1)
        if(idx == 0)
            mainWindow->setCueCmd(":BUS1:SPI:END LSB");
        else
            mainWindow->setCueCmd(":BUS1:SPI:END MSB");
    else if(idx == 0)
        mainWindow->setCueCmd(":DEC1:SPI:END LSB");
    else
        mainWindow->setCueCmd(":DEC1:SPI:END MSB");
}

void UiDecoderWindow::formatComboboxClicked(int idx) {
    devParms->mathDecodeFormat = idx;

    if(devParms->modelSerie != 1) {
        if(idx == 0)
            mainWindow->setCueCmd(":BUS1:FORM HEX");
        else if(idx == 1)
            mainWindow->setCueCmd(":BUS1:FORM ASC");
        else if(idx == 2)
            mainWindow->setCueCmd(":BUS1:FORM DEC");
        else if(idx == 3)
            mainWindow->setCueCmd(":BUS1:FORM BIN");
    } else {
        if(idx == 0)
            mainWindow->setCueCmd(":DEC1:FORM HEX");
        else if(idx == 1)
            mainWindow->setCueCmd(":DEC1:FORM ASC");
        else if(idx == 2)
            mainWindow->setCueCmd(":DEC1:FORM DEC");
        else if(idx == 3)
            mainWindow->setCueCmd(":DEC1:FORM BIN");
        else if(idx == 4)
            mainWindow->setCueCmd(":DEC1:FORM LINE");
    }
}

void UiDecoderWindow::tabHolderIndexChanged(int idx) {
    devParms->mathDecodeMode = idx;

    thresholdAutoClicked(devParms->mathDecodeThresholdAuto);

    if(devParms->modelSerie != 1) {
        if(idx == DECODE_MODE_TAB_PAR)
            mainWindow->setCueCmd(":BUS1:MODE PAR");
        else if(idx == DECODE_MODE_TAB_UART)
            mainWindow->setCueCmd(":BUS1:MODE RS232");
        else if(idx == DECODE_MODE_TAB_SPI)
            mainWindow->setCueCmd(":BUS1:MODE SPI");
        else if(idx == DECODE_MODE_TAB_I2C)
            mainWindow->setCueCmd(":BUS1:MODE IIC");
    } else {
        if(idx == DECODE_MODE_TAB_PAR)
            mainWindow->setCueCmd(":DEC1:MODE PAR");
        else if(idx == DECODE_MODE_TAB_UART)
            mainWindow->setCueCmd(":DEC1:MODE UART");
        else if(idx == DECODE_MODE_TAB_SPI)
            mainWindow->setCueCmd(":DEC1:MODE SPI");
        else if(idx == DECODE_MODE_TAB_I2C)
            mainWindow->setCueCmd(":DEC1:MODE IIC");
    }
}

void UiDecoderWindow::threshold1DspinboxChanged() {
    char str[512];

    if(tabHolder->currentIndex() == DECODE_MODE_TAB_UART) {
        if(devParms->modelSerie != 1) {
            devParms->mathDecodeThresholdUartTx = threshold1Dspinbox->value();

            snprintf(str, 512, ":BUS1:RS232:TTHR %e", devParms->mathDecodeThresholdUartTx);

            mainWindow->setCueCmd(str);
        } else if(uartTxSrcCombobox->currentIndex() > 0) {
            devParms->mathDecodeThreshold[uartTxSrcCombobox->currentIndex() - 1]
                = threshold1Dspinbox->value();

            snprintf(str,
                512,
                ":DEC1:THRE:CHAN%i %e",
                uartTxSrcCombobox->currentIndex(),
                devParms->mathDecodeThreshold[uartTxSrcCombobox->currentIndex() - 1]);

            mainWindow->setCueCmd(str);
        }
    } else if(tabHolder->currentIndex() == DECODE_MODE_TAB_SPI) {
        if(devParms->modelSerie != 1) {
            devParms->mathDecodeThreshold[2] = threshold1Dspinbox->value();

            snprintf(str, 512, ":BUS1:SPI:SCLK:THR %e", devParms->mathDecodeThreshold[2]);

            mainWindow->setCueCmd(str);
        } else {
            devParms->mathDecodeThreshold[spiClkSrcCombobox->currentIndex()]
                = threshold1Dspinbox->value();

            snprintf(str,
                512,
                ":DEC1:THRE:CHAN%i %e",
                spiClkSrcCombobox->currentIndex() + 1,
                devParms->mathDecodeThreshold[spiClkSrcCombobox->currentIndex()]);

            mainWindow->setCueCmd(str);
        }
    }

    thresholdAutoClicked(devParms->mathDecodeThresholdAuto);
}

void UiDecoderWindow::threshold2DspinboxChanged() {
    char str[512];

    if(tabHolder->currentIndex() == DECODE_MODE_TAB_UART) {
        if(devParms->modelSerie != 1) {
            devParms->mathDecodeThresholdUartRx = threshold2Dspinbox->value();

            snprintf(str, 512, ":BUS1:RS232:RTHR %e", devParms->mathDecodeThresholdUartRx);

            mainWindow->setCueCmd(str);
        } else if(uartRxSrcCombobox->currentIndex() > 0) {
            devParms->mathDecodeThreshold[uartRxSrcCombobox->currentIndex() - 1]
                = threshold2Dspinbox->value();

            snprintf(str,
                512,
                ":DEC1:THRE:CHAN%i %e",
                uartRxSrcCombobox->currentIndex(),
                devParms->mathDecodeThreshold[uartRxSrcCombobox->currentIndex() - 1]);

            mainWindow->setCueCmd(str);
        }
    } else if(tabHolder->currentIndex() == DECODE_MODE_TAB_SPI) {
        if(devParms->modelSerie != 1) {
            devParms->mathDecodeThreshold[1] = threshold2Dspinbox->value();

            snprintf(str, 512, ":BUS1:SPI:MOSI:THR %e", devParms->mathDecodeThreshold[1]);

            mainWindow->setCueCmd(str);
        } else if(spiMosiSrcCombobox->currentIndex() > 0) {
            devParms->mathDecodeThreshold[spiMosiSrcCombobox->currentIndex() - 1]
                = threshold2Dspinbox->value();

            snprintf(str,
                512,
                ":DEC1:THRE:CHAN%i %e",
                spiMosiSrcCombobox->currentIndex(),
                devParms->mathDecodeThreshold[spiMosiSrcCombobox->currentIndex() - 1]);

            mainWindow->setCueCmd(str);
        }
    }

    thresholdAutoClicked(devParms->mathDecodeThresholdAuto);
}

void UiDecoderWindow::threshold3DspinboxChanged() {
    char str[512];

    if(tabHolder->currentIndex() == DECODE_MODE_TAB_UART) {
    } else if(tabHolder->currentIndex() == DECODE_MODE_TAB_SPI) {
        if(devParms->modelSerie != 1) {
            devParms->mathDecodeThreshold[0] = threshold3Dspinbox->value();

            snprintf(str, 512, ":BUS1:SPI:MISO:THR %e", devParms->mathDecodeThreshold[0]);

            mainWindow->setCueCmd(str);
        } else if(spiMisoSrcCombobox->currentIndex() > 0) {
            devParms->mathDecodeThreshold[spiMisoSrcCombobox->currentIndex() - 1]
                = threshold3Dspinbox->value();

            snprintf(str,
                512,
                ":DEC1:THRE:CHAN%i %e",
                spiMisoSrcCombobox->currentIndex(),
                devParms->mathDecodeThreshold[spiMisoSrcCombobox->currentIndex() - 1]);

            mainWindow->setCueCmd(str);
        }
    }

    thresholdAutoClicked(devParms->mathDecodeThresholdAuto);
}

void UiDecoderWindow::threshold4DspinboxChanged() {
    char str[512];

    if(tabHolder->currentIndex() == DECODE_MODE_TAB_UART) {
    } else if(tabHolder->currentIndex() == DECODE_MODE_TAB_SPI) {
        if(devParms->modelSerie != 1) {
            devParms->mathDecodeThreshold[3] = threshold4Dspinbox->value();

            snprintf(str, 512, ":BUS1:SPI:SS:THR %e", devParms->mathDecodeThreshold[3]);

            mainWindow->setCueCmd(str);
        } else if(spiCsSrcCombobox->currentIndex() > 0) {
            devParms->mathDecodeThreshold[spiCsSrcCombobox->currentIndex() - 1]
                = threshold4Dspinbox->value();

            snprintf(str,
                512,
                ":DEC1:THRE:CHAN%i %e",
                spiCsSrcCombobox->currentIndex(),
                devParms->mathDecodeThreshold[spiCsSrcCombobox->currentIndex() - 1]);

            mainWindow->setCueCmd(str);
        }
    }

    thresholdAutoClicked(devParms->mathDecodeThresholdAuto);
}

void UiDecoderWindow::srcComboboxClicked(int) {
    devParms->mathDecodeSpiClk = spiClkSrcCombobox->currentIndex();
    devParms->mathDecodeSpiMosi = spiMosiSrcCombobox->currentIndex();
    devParms->mathDecodeSpiMiso = spiMisoSrcCombobox->currentIndex();
    devParms->mathDecodeSpiCs = spiCsSrcCombobox->currentIndex();
    devParms->mathDecodeUartTx = uartTxSrcCombobox->currentIndex();
    devParms->mathDecodeUartRx = uartRxSrcCombobox->currentIndex();
    devParms->mathDecodeSpiMode = spiModeCombobox->currentIndex();
    devParms->mathDecodeSpiSelect = spiSelectCombobox->currentIndex();

    thresholdAutoClicked(devParms->mathDecodeThresholdAuto);
}

void UiDecoderWindow::thresholdAutoClicked(int thrAuto) {
    char str[512];

    devParms->mathDecodeThresholdAuto = thrAuto;

    if(devParms->modelSerie == 1) {
        if(thrAuto == 0)
            mainWindow->setCueCmd(":DEC1:THRE:AUTO 0");
        else
            mainWindow->setCueCmd(":DEC1:THRE:AUTO 1");
    }

    if(tabHolder->currentIndex() == DECODE_MODE_TAB_UART) {
        if(uartTxSrcCombobox->currentIndex() > 0) {
            if(devParms->modelSerie != 1) {
                snprintf(str, 512, ":BUS1:RS232:TX CHAN%i", uartTxSrcCombobox->currentIndex());

                mainWindow->setCueCmd(str);
            } else {
                snprintf(str, 512, ":DEC1:UART:TX CHAN%i", uartTxSrcCombobox->currentIndex());

                mainWindow->setCueCmd(str);
            }
        } else {
            if(devParms->modelSerie != 1)
                mainWindow->setCueCmd(":BUS1:RS232:TX OFF");
            else
                mainWindow->setCueCmd(":DEC1:UART:TX OFF");
        }

        if(uartRxSrcCombobox->currentIndex() > 0) {
            if(devParms->modelSerie != 1) {
                snprintf(str, 512, ":BUS1:RS232:RX CHAN%i", uartRxSrcCombobox->currentIndex());

                mainWindow->setCueCmd(str);
            } else {
                snprintf(str, 512, ":DEC1:UART:RX CHAN%i", uartRxSrcCombobox->currentIndex());

                mainWindow->setCueCmd(str);
            }
        } else {
            if(devParms->modelSerie != 1)
                mainWindow->setCueCmd(":BUS1:RS232:RX OFF");
            else
                mainWindow->setCueCmd(":DEC1:UART:RX OFF");
        }

        if(thrAuto == 0) {
            threshold1Dspinbox->setEnabled(true);
            threshold2Dspinbox->setEnabled(true);

            if(devParms->modelSerie != 1) {
                threshold1Dspinbox->setValue(devParms->mathDecodeThresholdUartTx);
                threshold2Dspinbox->setValue(devParms->mathDecodeThresholdUartRx);
            } else {
                threshold1Dspinbox->setValue(
                    devParms->mathDecodeThreshold[uartTxSrcCombobox->currentIndex() - 1]);
                threshold2Dspinbox->setValue(
                    devParms->mathDecodeThreshold[uartRxSrcCombobox->currentIndex() - 1]);
            }
        } else {
            threshold1Dspinbox->setValue(0.0);
            threshold2Dspinbox->setValue(0.0);

            threshold1Dspinbox->setEnabled(false);
            threshold2Dspinbox->setEnabled(false);
        }

        threshold1Label->setVisible(true);
        threshold2Label->setVisible(true);
        threshold3Label->setVisible(false);
        threshold4Label->setVisible(false);

        threshold1Dspinbox->setVisible(true);
        threshold2Dspinbox->setVisible(true);
        threshold3Dspinbox->setVisible(false);
        threshold4Dspinbox->setVisible(false);
    } else if(tabHolder->currentIndex() == DECODE_MODE_TAB_SPI) {
        if(devParms->modelSerie != 1) {
            snprintf(str,
                512,
                ":BUS1:SPI:SCLK:SOUR CHAN%i",
                spiClkSrcCombobox->currentIndex() + 1);

            mainWindow->setCueCmd(str);
        } else {
            snprintf(str, 512, ":DEC1:SPI:CLK CHAN%i", spiClkSrcCombobox->currentIndex() + 1);

            mainWindow->setCueCmd(str);
        }

        if(spiMosiSrcCombobox->currentIndex() > 0) {
            if(devParms->modelSerie != 1) {
                snprintf(str,
                    512,
                    ":BUS1:SPI:MOSI:SOUR CHAN%i",
                    spiMosiSrcCombobox->currentIndex());

                mainWindow->setCueCmd(str);
            } else {
                snprintf(str, 512, ":DEC1:SPI:MOSI CHAN%i", spiMosiSrcCombobox->currentIndex());

                mainWindow->setCueCmd(str);
            }
        } else {
            if(devParms->modelSerie != 1)
                mainWindow->setCueCmd(":BUS1:SPI:MOSI:SOUR OFF");
            else
                mainWindow->setCueCmd(":DEC1:SPI:MOSI OFF");
        }

        if(spiMisoSrcCombobox->currentIndex() > 0) {
            if(devParms->modelSerie != 1) {
                snprintf(str,
                    512,
                    ":BUS1:SPI:MISO:SOUR CHAN%i",
                    spiMisoSrcCombobox->currentIndex());

                mainWindow->setCueCmd(str);
            } else {
                snprintf(str, 512, ":DEC1:SPI:MISO CHAN%i", spiMisoSrcCombobox->currentIndex());

                mainWindow->setCueCmd(str);
            }
        } else {
            if(devParms->modelSerie != 1)
                mainWindow->setCueCmd(":BUS1:SPI:MISO:SOUR OFF");
            else
                mainWindow->setCueCmd(":DEC1:SPI:MISO OFF");
        }

        if(devParms->modelSerie == 1) {
            if(spiModeCombobox->currentIndex() == 0)
                mainWindow->setCueCmd(":DEC1:SPI:MODE TIM");
            else
                mainWindow->setCueCmd(":DEC1:SPI:MODE CS");
        }

        if(spiCsSrcCombobox->currentIndex() > 0) {
            if(devParms->modelSerie != 1) {
                snprintf(str, 512, ":BUS1:SPI:SS:SOUR CHAN%i", spiCsSrcCombobox->currentIndex());

                mainWindow->setCueCmd(str);
            } else if(devParms->mathDecodeSpiMode) {
                snprintf(str, 512, ":DEC1:SPI:CS CHAN%i", spiCsSrcCombobox->currentIndex());

                mainWindow->setCueCmd(str);
            }
        } else {
            if(devParms->modelSerie != 1)
                mainWindow->setCueCmd(":BUS1:SPI:SS:SOUR OFF");
        }

        if(spiSelectCombobox->currentIndex() == 0) {
            if(devParms->modelSerie != 1)
                mainWindow->setCueCmd(":BUS1:SPI:SS:POL NEG");
            else if(devParms->mathDecodeSpiMode)
                mainWindow->setCueCmd(":DEC1:SPI:SEL NCS");
        } else {
            if(devParms->modelSerie != 1)
                mainWindow->setCueCmd(":BUS1:SPI:SS:POL POS");
            else
                mainWindow->setCueCmd(":DEC1:SPI:SEL CS");
        }

        if(thrAuto == 0) {
            threshold1Dspinbox->setEnabled(true);
            threshold2Dspinbox->setEnabled(true);
            threshold3Dspinbox->setEnabled(true);
            threshold4Dspinbox->setEnabled(true);

            if(devParms->modelSerie != 1) {
                threshold1Dspinbox->setValue(devParms->mathDecodeThreshold[2]);
                threshold2Dspinbox->setValue(devParms->mathDecodeThreshold[1]);
                threshold3Dspinbox->setValue(devParms->mathDecodeThreshold[0]);
                threshold4Dspinbox->setValue(devParms->mathDecodeThreshold[3]);
            } else {
                threshold1Dspinbox->setValue(
                    devParms->mathDecodeThreshold[spiClkSrcCombobox->currentIndex()]);
                threshold2Dspinbox->setValue(
                    devParms->mathDecodeThreshold[spiMosiSrcCombobox->currentIndex() - 1]);
                threshold3Dspinbox->setValue(
                    devParms->mathDecodeThreshold[spiMisoSrcCombobox->currentIndex() - 1]);
                threshold4Dspinbox->setValue(
                    devParms->mathDecodeThreshold[spiCsSrcCombobox->currentIndex() - 1]);
            }
        } else {
            threshold1Dspinbox->setValue(0.0);
            threshold2Dspinbox->setValue(0.0);
            threshold3Dspinbox->setValue(0.0);
            threshold4Dspinbox->setValue(0.0);

            threshold1Dspinbox->setEnabled(false);
            threshold2Dspinbox->setEnabled(false);
            threshold3Dspinbox->setEnabled(false);
            threshold4Dspinbox->setEnabled(false);
        }

        threshold1Label->setVisible(true);
        threshold2Label->setVisible(true);
        threshold3Label->setVisible(true);
        threshold4Label->setVisible(true);

        threshold1Dspinbox->setVisible(true);
        threshold2Dspinbox->setVisible(true);
        threshold3Dspinbox->setVisible(true);
        threshold4Dspinbox->setVisible(true);
    }
}

void UiDecoderWindow::toggleDecode() {
    if(devParms->mathDecodeDisplay == 1) {
        devParms->mathDecodeDisplay = 0;

        if(devParms->modelSerie != 1)
            mainWindow->setCueCmd(":BUS1:DISP OFF");
        else
            mainWindow->setCueCmd(":DEC1:DISP OFF");

        toggleDecodeButton->setText("Start Decoding");

        mainWindow->statusLabel->setText("Decode off");
    } else {
        devParms->mathDecodeDisplay = 1;

        if(devParms->modelSerie != 1)
            mainWindow->setCueCmd(":BUS1:DISP ON");
        else
            mainWindow->setCueCmd(":DEC1:DISP ON");

        toggleDecodeButton->setText("Stop Decoding");

        mainWindow->statusLabel->setText("Decode on");
    }
}
