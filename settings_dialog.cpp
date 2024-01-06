/*
***************************************************************************
*
* Author: Teunis van Beelen
*
* Copyright (C) 2015 - 2020 Teunis van Beelen
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

#include "settings_dialog.h"

UiSettingsWindow::UiSettingsWindow(QWidget* parnt) {
    int err;

    unsigned int ip_addr;

    char dev_str[256];

    QSettings settings;

    mainwindow = (UiMainWindow*)parnt;

    setMinimumSize(500, 500);
    setMaximumSize(500, 500);
    setWindowTitle("Settings");
    setModal(true);

    usbRadioButton = new QRadioButton("USB", this);
    usbRadioButton->setAutoExclusive(true);

    if(mainwindow->devParms.connectionType == 0)
        usbRadioButton->setChecked(true);

    lanIPRadioButton = new QRadioButton("LAN", this);
    lanIPRadioButton->setAutoExclusive(true);

    if(mainwindow->devParms.connectionType == 1)
        lanIPRadioButton->setChecked(true);

    hostnameLabel = new QLabel{this};

    hostnameLabel->setText("Hostname\n(overides IP-address)");
    hostnameLabel->setToolTip("Leave empty if you want to use the above IP-address");

    comboBox1 = new QComboBox{this};

    comboBox1->addItem("/dev/usbtmc");
    comboBox1->addItem("/dev/usbtmc0");
    comboBox1->addItem("/dev/usbtmc1");
    comboBox1->addItem("/dev/usbtmc2");
    comboBox1->addItem("/dev/usbtmc3");
    comboBox1->addItem("/dev/usbtmc4");
    comboBox1->addItem("/dev/usbtmc5");
    comboBox1->addItem("/dev/usbtmc6");
    comboBox1->addItem("/dev/usbtmc7");
    comboBox1->addItem("/dev/usbtmc8");
    comboBox1->addItem("/dev/usbtmc9");

    ipSpinbox1 = new QSpinBox{this};

    ipSpinbox1->setRange(0, 255);
    ipSpinbox1->setSingleStep(1);
    ipSpinbox1->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ipSpinbox1->setAlignment(Qt::AlignHCenter);

    ipSpinbox2 = new QSpinBox{this};

    ipSpinbox2->setRange(0, 255);
    ipSpinbox2->setSingleStep(1);
    ipSpinbox2->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ipSpinbox2->setAlignment(Qt::AlignHCenter);

    ipSpinbox3 = new QSpinBox{this};

    ipSpinbox3->setRange(0, 255);
    ipSpinbox3->setSingleStep(1);
    ipSpinbox3->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ipSpinbox3->setAlignment(Qt::AlignHCenter);

    ipSpinbox4 = new QSpinBox{this};

    ipSpinbox4->setRange(0, 255);
    ipSpinbox4->setSingleStep(1);
    ipSpinbox4->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ipSpinbox4->setAlignment(Qt::AlignHCenter);

    err = 1;

    if(settings.contains("connection/ip")) {
        if(strtoipaddr(&ip_addr, settings.value("connection/ip").toString().toLatin1().data())
            == 0) {
            ipSpinbox1->setValue((ip_addr >> 24) & 0xff);
            ipSpinbox2->setValue((ip_addr >> 16) & 0xff);
            ipSpinbox3->setValue((ip_addr >> 8) & 0xff);
            ipSpinbox4->setValue(ip_addr & 0xff);

            err = 0;
        }
    }

    if(err) {
        ipSpinbox1->setValue(192);
        ipSpinbox2->setValue(168);
        ipSpinbox3->setValue(1);
        ipSpinbox4->setValue(100);
    }

    if(settings.contains("connection/hostname"))
        strncpy(mainwindow->devParms.hostName,
            settings.value("connection/hostname").toString().toLatin1().data(),
            63);
    mainwindow->devParms.hostName[63] = 0;

    HostLineEdit = new QLineEdit{this};

    HostLineEdit->setMaxLength(63);
    HostLineEdit->setText(mainwindow->devParms.hostName);
    HostLineEdit->setToolTip("Leave empty if you want to use the above IP-address");

    refreshLabel = new QLabel{this};

    refreshLabel->setText("Screen update\ninterval");

    refreshSpinbox = new QSpinBox{this};

    refreshSpinbox->setSuffix(" mS");
    refreshSpinbox->setRange(50, 2000);
    refreshSpinbox->setSingleStep(10);
    refreshSpinbox->setValue(mainwindow->devParms.screenTimerIval);

    invScrShtLabel = new QLabel{this};

    invScrShtLabel->setText("Screenshot invert\n colors");

    invScrShtCheckbox = new QCheckBox{this};

    invScrShtCheckbox->setTristate(false);
    if(mainwindow->devParms.screenshotInv)
        invScrShtCheckbox->setCheckState(Qt::Checked);
    else
        invScrShtCheckbox->setCheckState(Qt::Unchecked);

    showfpsLabel = new QLabel{this};

    showfpsLabel->setText("Show frames\n per second");

    showfpsCheckbox = new QCheckBox{this};

    showfpsCheckbox->setTristate(false);
    if(mainwindow->devParms.showFps)
        showfpsCheckbox->setCheckState(Qt::Checked);
    else
        showfpsCheckbox->setCheckState(Qt::Unchecked);

    extendvertdivLabel = new QLabel{this};

    extendvertdivLabel->setText("Use extended\n vertical range");

    extendvertdivCheckbox = new QCheckBox{this};

    extendvertdivCheckbox->setTristate(false);
    if(mainwindow->devParms.useExtraVertDivisions)
        extendvertdivCheckbox->setCheckState(Qt::Checked);
    else
        extendvertdivCheckbox->setCheckState(Qt::Unchecked);

    applyButton = new QPushButton{this};

    applyButton->setText("Apply");

    cancelButton = new QPushButton{this};

    cancelButton->setText("Cancel");

    strlcpy(dev_str, settings.value("connection/device").toString().toLocal8Bit().data(), 256);

    if(!strcmp(dev_str, ""))
        strlcpy(dev_str, "/dev/usbtmc0", 256);

    comboBox1->setCurrentIndex(dev_str[11] - '0');

    if(mainwindow->devParms.connected) {
        usbRadioButton->setEnabled(false);
        lanIPRadioButton->setEnabled(false);
        ipSpinbox1->setEnabled(false);
        ipSpinbox2->setEnabled(false);
        ipSpinbox3->setEnabled(false);
        ipSpinbox4->setEnabled(false);
        comboBox1->setEnabled(false);
        applyButton->setEnabled(false);
    } else {
        QObject::connect(applyButton, &QPushButton::clicked, this, &UiSettingsWindow::applyButtonClicked);
    }

    auto& grid = *new QGridLayout{this};
    int R{};
    //                   rcrc                                                X    Y    W    H
    grid.addWidget(usbRadioButton, /*       */ (R), 0, 1, 1); // setGeometry(40,  20,  110, 25);
    grid.addWidget(comboBox1, /*            */ R++, 1, 1, 4); // setGeometry(180, 20,  110, 25);
    grid.addWidget(lanIPRadioButton, /*     */ (R), 0, 1, 1); // setGeometry(40,  70,  110, 25);
    grid.addWidget(ipSpinbox1, /*           */ (R), 1, 1, 1); // setGeometry(180, 70,  35,  25);
    grid.addWidget(ipSpinbox2, /*           */ (R), 2, 1, 1); // setGeometry(220, 70,  35,  25);
    grid.addWidget(ipSpinbox3, /*           */ (R), 3, 1, 1); // setGeometry(260, 70,  35,  25);
    grid.addWidget(ipSpinbox4, /*           */ R++, 4, 1, 1); // setGeometry(300, 70,  35,  25);
    grid.addWidget(hostnameLabel, /*        */ (R), 0, 1, 1); // setGeometry(40,  120, 120, 35);
    grid.addWidget(HostLineEdit, /*         */ R++, 1, 1, 4); // setGeometry(180, 120, 240, 25);
    grid.addWidget(refreshLabel, /*         */ (R), 0, 1, 1); // setGeometry(40,  170, 120, 35);
    grid.addWidget(refreshSpinbox, /*       */ R++, 1, 1, 4); // setGeometry(180, 170, 100, 25);
    grid.addWidget(invScrShtLabel, /*       */ (R), 0, 1, 1); // setGeometry(40,  220, 120, 35);
    grid.addWidget(invScrShtCheckbox, /*    */ R++, 1, 1, 4); // setGeometry(180, 220, 120, 35);
    grid.addWidget(showfpsLabel, /*         */ (R), 0, 1, 1); // setGeometry(40,  270, 120, 35);
    grid.addWidget(showfpsCheckbox, /*      */ R++, 1, 1, 4); // setGeometry(180, 270, 120, 35);
    grid.addWidget(extendvertdivLabel, /*   */ (R), 0, 1, 1); // setGeometry(40,  320, 120, 35);
    grid.addWidget(extendvertdivCheckbox, /**/ R++, 1, 1, 4); // setGeometry(180, 320, 120, 35);
    grid.addWidget(applyButton, /*          */ (R), 0, 1, 1); // setGeometry(40,  450, 100, 25);
    grid.addWidget(cancelButton, /*         */ (R), 1, 1, 4); // setGeometry(250, 450, 100, 25);

    QObject::connect(cancelButton, &QPushButton::clicked,
        this, &UiSettingsWindow::close);
    QObject::connect(refreshSpinbox, &QSpinBox::valueChanged,
        this, &UiSettingsWindow::refreshSpinboxChanged);
    QObject::connect(invScrShtCheckbox, &QCheckBox::stateChanged,
        this, &UiSettingsWindow::invScrShtCheckboxChanged);
    QObject::connect(showfpsCheckbox, &QCheckBox::stateChanged,
        this, &UiSettingsWindow::showfpsCheckboxChanged);
    QObject::connect(extendvertdivCheckbox, &QCheckBox::stateChanged,
        this, &UiSettingsWindow::extendvertdivCheckboxChanged);
    QObject::connect(HostLineEdit, &QLineEdit::textEdited,
        this, &UiSettingsWindow::hostnamechanged);

    exec();
}

void UiSettingsWindow::applyButtonClicked() {
    char dev_str[256];

    QSettings settings;

    if(mainwindow->devParms.connected)
        close();

    strlcpy(dev_str, comboBox1->currentText().toLatin1().data(), 256);

    // dev_str[11] = '0' + comboBox1->currentIndex();

    if(usbRadioButton->isChecked() == true) {
        settings.setValue("connection/type", "USB");

        mainwindow->devParms.connectionType = 0;
    } else {
        settings.setValue("connection/type", "LAN");

        mainwindow->devParms.connectionType = 1;
    }

    settings.setValue("connection/device", QByteArray{dev_str});

    snprintf(dev_str,
        256,
        "%i.%i.%i.%i",
        ipSpinbox1->value(),
        ipSpinbox2->value(),
        ipSpinbox3->value(),
        ipSpinbox4->value());

    settings.setValue("connection/ip", QByteArray{dev_str});

    strncpy(mainwindow->devParms.hostName, HostLineEdit->text().toLatin1().data(), 63);
    mainwindow->devParms.hostName[63] = 0;

    settings.setValue("connection/hostname", QByteArray{mainwindow->devParms.hostName});

    if(invScrShtCheckbox->checkState() == Qt::Checked) {
        mainwindow->devParms.screenshotInv = 1;

        settings.setValue("screenshot/inverted", 1);
    } else {
        mainwindow->devParms.screenshotInv = 0;

        settings.setValue("screenshot/inverted", 0);
    }

    if(showfpsCheckbox->checkState() == Qt::Checked) {
        mainwindow->devParms.showFps = 1;

        settings.setValue("gui/show_fps", 1);
    } else {
        mainwindow->devParms.showFps = 0;

        settings.setValue("gui/show_fps", 0);
    }

    close();
}

void UiSettingsWindow::refreshSpinboxChanged(int value) {
    QSettings settings;

    mainwindow->devParms.screenTimerIval = value;

    settings.setValue("gui/refresh", value);

    if(mainwindow->scrnTimer->isActive())
        mainwindow->scrnTimer->start(value);
}

void UiSettingsWindow::invScrShtCheckboxChanged(int state) {
    QSettings settings;

    if(state == Qt::Checked)
        mainwindow->devParms.screenshotInv = 1;
    else
        mainwindow->devParms.screenshotInv = 0;

    settings.setValue("screenshot/inverted", mainwindow->devParms.screenshotInv);
}

void UiSettingsWindow::showfpsCheckboxChanged(int state) {
    QSettings settings;

    if(state == Qt::Checked)
        mainwindow->devParms.showFps = 1;
    else
        mainwindow->devParms.showFps = 0;

    settings.setValue("gui/show_fps", mainwindow->devParms.showFps);
}

void UiSettingsWindow::extendvertdivCheckboxChanged(int state) {
    QSettings settings;

    QMessageBox msgBox;

    if(state == Qt::Checked) {
        if((mainwindow->devParms.connected == 1) && (mainwindow->devParms.mathFft == 1)
            && (mainwindow->devParms.mathFftSplit == 0)) {
            msgBox.setIcon(QMessageBox::NoIcon);
            msgBox.setText("Can not use extended vertical range when FFT is fullscreen.\n"
                           "Set FFT to \"half\" first (splitscreen mode).");
            msgBox.exec();
            extendvertdivCheckbox->setCheckState(Qt::Unchecked);
            return;
        }

        mainwindow->devParms.useExtraVertDivisions = 1;
    } else {
        mainwindow->devParms.useExtraVertDivisions = 0;
    }

    settings.setValue("gui/use_extra_vertdivisions", mainwindow->devParms.useExtraVertDivisions);

    if(mainwindow->devParms.modelSerie == 1) {
        if(mainwindow->devParms.useExtraVertDivisions == 1)
            mainwindow->devParms.vertDivisions = 10;
        else
            mainwindow->devParms.vertDivisions = 8;
    }
}

void UiSettingsWindow::hostnamechanged(QString qstr) {
    int i, j, len, trunc = 0;

    char str[128];

    strncpy(str, qstr.toLatin1().data(), 63);

    str[63] = 0;

    len = strlen(str);

    for(int i{}; i < len; i++) {
        if(((str[i] < '0') && (str[i] != '-') && (str[i] != '.'))
            || ((str[i] > '9') && (str[i] < 'A')) || ((str[i] > 'Z') && (str[i] < 'a'))
            || (str[i] > 'z')) {
            for(j = i; j < len; j++)
                str[j] = str[j + 1];

            len--;

            i--;

            trunc = 1;
        }
    }

    for(int i{}; i < len; i++) {
        if((str[i] >= 'A') && (str[i] <= 'Z')) {
            str[i] += 32;

            trunc = 1;
        }
    }

    if(trunc)
        HostLineEdit->setText(str);
}
