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

#include "mainwindow.h"

UiMainWindow::UiMainWindow() {
    int i, pxw;

    char str[1024];

    QPixmap pxm(500, 100);

    QPainter p_aint(&pxm);

    setMinimumSize(1170, 630);
    setWindowTitle(PROGRAM_NAME " " PROGRAM_VERSION);
    setWindowIcon(QIcon(":/images/r_dsremote.png"));

    appfont = new QFont;

    appfont->setFamily("Noto Sans");

    for(i = 20; i > 7; i--) {
        appfont->setPixelSize(i);

        p_aint.setFont(*appfont);

        pxw = p_aint
                  .boundingRect(0,
                      0,
                      500,
                      100,
                      Qt::AlignLeft | Qt::TextSingleLine,
                      "ABCDEFGHIJKLMNOPQRSTUVWXYZ")
                  .width();

        //    printf("i is: %i    width is: %i\n", i, pxw);

        if(pxw < 203)
            break;
    }
    pxw = i;

    appfont->setPixelSize(pxw);

    QApplication::setFont(*appfont);

    snprintf(str, 1024, "font: %ipx;", pxw);
    setStyleSheet(str);

    setlocale(LC_NUMERIC, "C");

    QCoreApplication::setOrganizationName("TvB");
    QCoreApplication::setOrganizationDomain("teuniz.net");
    QCoreApplication::setApplicationName(PROGRAM_NAME);

    QSettings settings;

    memset(&devParms, 0, sizeof(struct DeviceSettings));

    devParms.fontSize = pxw;

    devParms.screenshotBuf = (char*)malloc(WAVFRM_MAX_BUFSZ);

    for(int i{}; i < MAX_CHNS; i++) {
        devParms.waveBuf[i] = (short*)malloc(WAVFRM_MAX_BUFSZ * sizeof(short));

        devParms.chan[i].scale = 1;
    }

    strlcpy(devParms.chan[0].unitstr, "V", 1024);
    strlcpy(devParms.chan[1].unitstr, "W", 1024);
    strlcpy(devParms.chan[2].unitstr, "A", 1024);
    strlcpy(devParms.chan[3].unitstr, "U", 1024);

    devParms.fftBufIn = new double[FFT_MAX_BUFSZ];
    devParms.fftBufOut = new double[FFT_MAX_BUFSZ];
    devParms.kissFftBuf = new kiss_fft_cpx[FFT_MAX_BUFSZ];

    devParms.kCfg = nullptr;

    devParms.screenTimerIval = settings.value("gui/refresh", 50).toInt();

    if((devParms.screenTimerIval < 50) || (devParms.screenTimerIval > 2000)) {
        devParms.screenTimerIval = 50;

        settings.setValue("gui/refresh", devParms.screenTimerIval);
    }

    devParms.screenshotInv = settings.value("screenshot/inverted", 0).toInt();

    if(devParms.screenshotInv) {
        devParms.screenshotInv = 1;

        settings.setValue("screenshot/inverted", devParms.screenshotInv);
    } else {
        devParms.screenshotInv = 0;

        settings.setValue("screenshot/inverted", devParms.screenshotInv);
    }

    devParms.showFps = settings.value("gui/show_fps", 0).toInt();

    if(devParms.showFps) {
        devParms.showFps = 1;

        settings.setValue("gui/show_fps", devParms.showFps);
    } else {
        devParms.showFps = 0;

        settings.setValue("gui/show_fps", devParms.showFps);
    }

    devParms.displaygrid = 2;

    devParms.channelCnt = 4;

    devParms.modelSerie = 1;

    devParms.timebasescale = 0.001;

    devParms.horDivisions = 14;

    devParms.vertDivisions = 8;

    devParms.useExtraVertDivisions = settings.value("gui/use_extra_vertdivisions", 1).toInt();

    if(devParms.useExtraVertDivisions) {
        devParms.useExtraVertDivisions = 1;

        settings.setValue("gui/use_extra_vertdivisions", devParms.useExtraVertDivisions);
    } else {
        devParms.useExtraVertDivisions = 0;

        settings.setValue("gui/use_extra_vertdivisions", devParms.useExtraVertDivisions);
    }

    devParms.currentScreenSf = 1;

    devParms.fftVScale = 10.0;

    devParms.fftVOffset = 20.0;

    strlcpy(devParms.modelName, "-----", 128);

    pthread_mutex_init(&devParms.mutexx, nullptr);

    scrnThread = new ScreenThread;
    scrnThread->setObjectName("scrnThread");
    scrnThread->setDevice(nullptr);

    menubar = menuBar();

    devicemenu = new QMenu{this};
    devicemenu->setObjectName("devicemenu");
    devicemenu->setTitle("Device");
    devicemenu->addAction("Connect", this, &UiMainWindow::openConnection);
    devicemenu->addAction("Disconnect", this, &UiMainWindow::closeConnection);
    devicemenu->addAction("Exit", this, &UiMainWindow::close, QKeySequence::Quit);
    menubar->addMenu(devicemenu);

    settingsmenu = new QMenu{this};
    settingsmenu->setObjectName("settingsmenu");
    settingsmenu->setTitle("Settings");
    settingsmenu->addAction("Settings", this, &UiMainWindow::openSettingsDialog);
    menubar->addMenu(settingsmenu);

    helpmenu = new QMenu{this};
    helpmenu->setObjectName("helpmenu");
    helpmenu->setTitle("Help");
    helpmenu->addAction("How to operate", this, &UiMainWindow::helpButtonClicked);
    helpmenu->addAction("About", this, &UiMainWindow::showAboutDialog);
    menubar->addMenu(helpmenu);

    statusLabel = new QLabel;
    statusLabel->setObjectName("statusLabel");

    waveForm = new SignalCurve{this};
    waveForm->setObjectName("waveForm");
    waveForm->setBackgroundColor(Qt::black);
    waveForm->setSignalColor1(Qt::yellow);
    waveForm->setSignalColor2(Qt::cyan);
    waveForm->setSignalColor3(Qt::magenta);
    waveForm->setSignalColor4(QColor(0, 128, 255));
    waveForm->setRasterColor(Qt::darkGray);
    waveForm->setBorderSize(40);
    waveForm->setDeviceParameters(&devParms);

    setCentralWidget(waveForm);

    statusBar = new QStatusBar;
    statusBar->setObjectName("statusBar");
    setStatusBar(statusBar);
    statusBar->addPermanentWidget(statusLabel, 100);
    statusLabel->setText("Disconnected");

    DPRwidget = new QWidget;
    DPRwidget->setObjectName("DPRwidget");

    setupUi(DPRwidget);

    {
        QSizePolicy sizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        // sizePolicy.setHeightForWidth(adjDialLabel->sizePolicy().hasHeightForWidth());
        for(int ch{}; ch < 16; ++ch) {
            auto rb = new QRadioButton{this};
            auto cbx = new QCheckBox{QStringLiteral("D%1").arg(ch), this};
            rb->setSizePolicy(sizePolicy);
            cbx->setSizePolicy(sizePolicy);
            gridLogic->addWidget(rb, ch + 1, 0);
            gridLogic->addWidget(cbx, ch + 1, 1);
        }
    }

    chButtons = {
        ch1Button,
        ch2Button,
        ch3Button,
        ch4Button,
    };

    gridLayout_11->setAlignment(trigModeAutoLed, Qt::AlignCenter);
    gridLayout_11->setAlignment(trigModeNormLed, Qt::AlignCenter);
    gridLayout_11->setAlignment(trigModeSingLed, Qt::AlignCenter);

    // adjDial->setMaximum(100);
    // adjDial->setMinimum(0);
    // adjDial->setSingleStep(1);
    // adjDial->setWrapping(true);
    adjDial->setContextMenuPolicy(Qt::CustomContextMenu);

    // navDial->setMaximum(100);
    // navDial->setMinimum(0);
    // navDial->setSingleStep(1);
    // navDial->setWrapping(false);
    navDial->setValue(50);
    navDial->setContextMenuPolicy(Qt::CustomContextMenu);

    // horScaleDial->setMaximum(100);
    // horScaleDial->setMinimum(0);
    // horScaleDial->setSingleStep(1);
    // horScaleDial->setWrapping(true);
    // horScaleDial->setNotchesVisible(true);
    horScaleDial->setContextMenuPolicy(Qt::CustomContextMenu);

    // horPosDial->setMaximum(100);
    // horPosDial->setMinimum(0);
    // horPosDial->setSingleStep(1);
    // horPosDial->setWrapping(true);
    // horPosDial->setNotchesVisible(true);
    horPosDial->setContextMenuPolicy(Qt::CustomContextMenu);

    // vertOffsetDial->setMaximum(100);
    // vertOffsetDial->setMinimum(0);
    // vertOffsetDial->setSingleStep(1);
    // vertOffsetDial->setWrapping(true);
    // vertOffsetDial->setNotchesVisible(true);
    vertOffsetDial->setContextMenuPolicy(Qt::CustomContextMenu);

    // vertScaleDial->setMaximum(100);
    // vertScaleDial->setMinimum(0);
    // vertScaleDial->setSingleStep(1);
    // vertScaleDial->setWrapping(true);
    // vertScaleDial->setNotchesVisible(true);
    vertScaleDial->setContextMenuPolicy(Qt::CustomContextMenu);

    // trigAdjustDial->setMaximum(100);
    // trigAdjustDial->setMinimum(0);
    // trigAdjustDial->setSingleStep(1);
    // trigAdjustDial->setWrapping(true);
    // trigAdjustDial->setNotchesVisible(true);
    trigAdjustDial->setContextMenuPolicy(Qt::CustomContextMenu);

    defStylesh = clearButton->styleSheet();

    dockPanelRight = new QDockWidget{this};
    dockPanelRight->setObjectName("dockPanelRight");
    dockPanelRight->setFeatures(QDockWidget::DockWidgetFloatable);
    dockPanelRight->setAllowedAreas(Qt::RightDockWidgetArea);
    dockPanelRight->setWidget(DPRwidget);
    dockPanelRight->setMinimumWidth(350);
    dockPanelRight->setMinimumHeight(400);
    addDockWidget(Qt::RightDockWidgetArea, dockPanelRight);

    formerPageAct = new QAction{this};
    formerPageAct->setObjectName("formerPageAct");
    formerPageAct->setShortcut(QKeySequence::MoveToPreviousPage);
    connect(formerPageAct, &QAction::triggered, this, &UiMainWindow::formerPage);
    addAction(formerPageAct);

    shiftTraceUpAct = new QAction{this};
    shiftTraceUpAct->setObjectName("shiftTraceUpAct");
    shiftTraceUpAct->setShortcut(QKeySequence::MoveToPreviousLine);
    connect(shiftTraceUpAct, &QAction::triggered, this, &UiMainWindow::shiftTraceUp);
    addAction(shiftTraceUpAct);

    shiftTraceDownAct = new QAction{this};
    shiftTraceDownAct->setObjectName("shiftTraceDownAct");
    shiftTraceDownAct->setShortcut(QKeySequence::MoveToNextLine);
    connect(shiftTraceDownAct, &QAction::triggered, this, &UiMainWindow::shiftTraceDown);
    addAction(shiftTraceDownAct);

    shiftPageLeftAct = new QAction{this};
    shiftPageLeftAct->setObjectName("shiftPageLeftAct");
    shiftPageLeftAct->setShortcut(QKeySequence::MoveToPreviousChar);
    connect(shiftPageLeftAct, &QAction::triggered, this, &UiMainWindow::shiftPageLeft);
    addAction(shiftPageLeftAct);

    centerPositionAct = new QAction{this};
    centerPositionAct->setObjectName("centerPositionAct");
    centerPositionAct->setShortcut(QKeySequence("c"));
    connect(centerPositionAct, &QAction::triggered, this, &UiMainWindow::centerTrigger);
    addAction(centerPositionAct);

    centerTriggerAct = new QAction{this};
    centerTriggerAct->setObjectName("centerTriggerAct");
    centerTriggerAct->setShortcut(QKeySequence("t"));
    connect(centerTriggerAct, &QAction::triggered, this, &UiMainWindow::centerTrigger);
    addAction(centerTriggerAct);

    shiftPageRightAct = new QAction{this};
    shiftPageRightAct->setObjectName("shiftPageRightAct");
    shiftPageRightAct->setShortcut(QKeySequence::MoveToNextChar);
    connect(shiftPageRightAct, &QAction::triggered, this, &UiMainWindow::shiftPageRight);
    addAction(shiftPageRightAct);

    nextPageAct = new QAction{this};
    nextPageAct->setObjectName("nextPageAct");
    nextPageAct->setShortcut(QKeySequence::MoveToNextPage);
    connect(nextPageAct, &QAction::triggered, this, &UiMainWindow::nextPage);
    addAction(nextPageAct);

    zoomInAct = new QAction{this};
    zoomInAct->setObjectName("zoomInAct");
    zoomInAct->setShortcut(QKeySequence::ZoomIn);
    connect(zoomInAct, &QAction::triggered, this, &UiMainWindow::zoomIn);
    addAction(zoomInAct);

    zoomOutAct = new QAction{this};
    zoomOutAct->setObjectName("zoomOutAct");
    zoomOutAct->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOutAct, &QAction::triggered, this, &UiMainWindow::zoomOut);
    addAction(zoomOutAct);

    chanScalePlusAct = new QAction{this};
    chanScalePlusAct->setObjectName("chanScalePlusAct");
    chanScalePlusAct->setShortcut(Qt::Key_Minus);
    connect(chanScalePlusAct, &QAction::triggered, this, &UiMainWindow::chanScalePlus);
    addAction(chanScalePlusAct);

    chanScalePlusAllChannelsAct = new QAction{this};
    chanScalePlusAllChannelsAct->setObjectName("chanScalePlusAllChannelsAct");
    chanScalePlusAllChannelsAct->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Minus));
    connect(chanScalePlusAllChannelsAct, &QAction::triggered,
        this, &UiMainWindow::chanScalePlusAll);
    addAction(chanScalePlusAllChannelsAct);

    chanScaleMinusAct = new QAction{this};
    chanScaleMinusAct->setObjectName("chanScaleMinusAct");
    chanScaleMinusAct->setShortcut(Qt::Key_Plus);
    connect(chanScaleMinusAct, &QAction::triggered, this, &UiMainWindow::chanScaleMinus);
    addAction(chanScaleMinusAct);

    chanScaleMinusAllChannelsAct = new QAction{this};
    chanScaleMinusAllChannelsAct->setObjectName("chanScaleMinusAllChannelsAct");
    chanScaleMinusAllChannelsAct->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Plus));
    connect(chanScaleMinusAllChannelsAct, &QAction::triggered,
        this, &UiMainWindow::chanScaleMinusAll);
    addAction(chanScaleMinusAllChannelsAct);

    selectChan1Act = new QAction{this};
    selectChan1Act->setObjectName("selectChan1Act");
    selectChan1Act->setShortcut(QKeySequence("1"));
    addAction(selectChan1Act);

    selectChan2Act = new QAction{this};
    selectChan2Act->setObjectName("selectChan2Act");
    selectChan2Act->setShortcut(QKeySequence("2"));
    addAction(selectChan2Act);

    selectChan3Act = new QAction{this};
    selectChan3Act->setObjectName("selectChan3Act");
    selectChan3Act->setShortcut(QKeySequence("3"));
    addAction(selectChan3Act);

    selectChan4Act = new QAction{this};
    selectChan4Act->setObjectName("selectChan4Act");
    selectChan4Act->setShortcut(QKeySequence("4"));
    addAction(selectChan4Act);

    toggleFftAct = new QAction{this};
    toggleFftAct->setObjectName("toggleFftAct");
    toggleFftAct->setShortcut(QKeySequence("f"));
    addAction(toggleFftAct);

    saveScreenshotAct = new QAction{this};
    saveScreenshotAct->setObjectName("saveScreenshotAct");
    saveScreenshotAct->setShortcut(QKeySequence::Print);
    connect(saveScreenshotAct, &QAction::triggered, this, &UiMainWindow::saveScreenshot);
    addAction(saveScreenshotAct);

    DPRwidget->setEnabled(false);

    recentDir[0] = 0;

    recentSaveDir[0] = 0;

    device = nullptr;

    strlcpy(recentSaveDir,
        settings.value("path/savedir").toString().toLocal8Bit().data(),
        MAX_PATHLEN);

    strlcpy(str, settings.value("connection/type").toString().toLatin1().data(), 1024);

    if(!strcmp(str, "LAN"))
        devParms.connectionType = 1;
    else
        devParms.connectionType = 0;

    adjDialFunc = ADJ_DIAL_FUNC_NONE;
    navDialFunc = NAV_DIAL_FUNC_NONE;

    adjdialTimer = new QTimer{this};
    labelTimer = new QTimer{this};
    scrnTimer = new QTimer{this};
    testTimer = new QTimer{this};

    horPosDialTimer = new QTimer{this};
    horPosDialTimer->setSingleShot(true);

    horScaleDialTimer = new QTimer{this};
    horScaleDialTimer->setSingleShot(true);

    navDialTimer = new QTimer{this};
    navDialTimer->setSingleShot(true);

    trigAdjDialTimer = new QTimer{this};
    trigAdjDialTimer->setSingleShot(true);

    vertOffsDialTimer = new QTimer{this};
    vertOffsDialTimer->setSingleShot(true);

    vertScaleDialTimer = new QTimer{this};
    vertScaleDialTimer->setSingleShot(true);

#if QT_VERSION >= 0x050000
    scrnTimer->setTimerType(Qt::PreciseTimer);
    adjdialTimer->setTimerType(Qt::PreciseTimer);
    navDialTimer->setTimerType(Qt::PreciseTimer);
    labelTimer->setTimerType(Qt::PreciseTimer);
    testTimer->setTimerType(Qt::PreciseTimer);
    horPosDialTimer->setTimerType(Qt::PreciseTimer);
    trigAdjDialTimer->setTimerType(Qt::PreciseTimer);
    vertOffsDialTimer->setTimerType(Qt::PreciseTimer);
    horScaleDialTimer->setTimerType(Qt::PreciseTimer);
    vertScaleDialTimer->setTimerType(Qt::PreciseTimer);
#endif

    // clang-format off
    connect(scrnTimer,          &QTimer::timeout,      this, &UiMainWindow::scrnTimerHandler);
    connect(scrnThread,         &QThread::finished,    this, &UiMainWindow::screenUpdate);
    connect(adjdialTimer,       &QTimer::timeout,      this, &UiMainWindow::adjdialTimerHandler);
    connect(navDial,            &Dial::sliderReleased, this, &UiMainWindow::navDialReleased);
    connect(navDialTimer,       &QTimer::timeout,      this, &UiMainWindow::navDialTimerHandler);
    connect(labelTimer,         &QTimer::timeout,      this, &UiMainWindow::labelTimerHandler);
    connect(testTimer,          &QTimer::timeout,      this, &UiMainWindow::testTimerHandler);
    connect(horPosDialTimer,    &QTimer::timeout,      this, &UiMainWindow::horPosDialTimerHandler);
    connect(trigAdjDialTimer,   &QTimer::timeout,      this, &UiMainWindow::trigAdjDialTimerHandler);
    connect(vertOffsDialTimer,  &QTimer::timeout,      this, &UiMainWindow::vertOffsDialTimerHandler);
    connect(horScaleDialTimer,  &QTimer::timeout,      this, &UiMainWindow::horScaleDialTimerHandler);
    connect(vertScaleDialTimer, &QTimer::timeout,      this, &UiMainWindow::vertScaleDialTimerHandler);
    // clang-format on

    ///// TEST /////////////////////////////////////
    //   DPRwidget->setEnabled(true);
    //
    //   connect(adjDial,          SIGNAL(valueChanged(int)), this, &UiMainwindow::adjDialChanged(int)));
    //   connect(trigAdjustDial,   SIGNAL(valueChanged(int)), this, &UiMainwindow::trigAdjustDialChanged(int)));
    //   connect(horScaleDial,     SIGNAL(valueChanged(int)), this, &UiMainwindow::horScaleDialChanged(int)));
    //   connect(horPosDial,       SIGNAL(valueChanged(int)), this, &UiMainwindow::horPosDialChanged(int)));
    //   connect(vertOffsetDial,   SIGNAL(valueChanged(int)), this, &UiMainwindow::vertOffsetDialChanged(int)));
    //   connect(vertScaleDial,    SIGNAL(valueChanged(int)), this, &UiMainwindow::vertScaleDialChanged(int)));
    //   connect(navDial,          SIGNAL(valueChanged(int)), this, &UiMainwindow::navDialChanged(int)));
    //
    //   connect(ch1Button,        SIGNAL(clicked()),     this, &UiMainwindow::ch1ButtonClicked);
    //   connect(ch2Button,        SIGNAL(clicked()),     this, &UiMainwindow::ch2ButtonClicked);
    //   connect(ch3Button,        SIGNAL(clicked()),     this, &UiMainwindow::ch3ButtonClicked);
    //   connect(ch4Button,        SIGNAL(clicked()),     this, &UiMainwindow::ch4ButtonClicked);
    //   connect(clearButton,      SIGNAL(clicked()),     this, &UiMainwindow::clearButtonClicked);
    //   connect(autoButton,       SIGNAL(clicked()),     this, &UiMainwindow::autoButtonClicked);
    //   connect(runButton,        SIGNAL(clicked()),     this, &UiMainwindow::runButtonClicked);
    //   connect(singleButton,     SIGNAL(clicked()),     this, &UiMainwindow::singleButtonClicked);
    //   connect(horMenuButton,    SIGNAL(clicked()),     this, &UiMainwindow::horMenuButtonClicked);
    //   connect(trigModeButton,   SIGNAL(clicked()),     this, &UiMainwindow::trigModeButtonClicked);
    //   connect(trigMenuButton,   SIGNAL(clicked()),     this, &UiMainwindow::trigMenuButtonClicked);
    //   connect(trigForceButton,  SIGNAL(clicked()),     this, &UiMainwindow::trigForceButtonClicked);
    //   connect(trig50pctButton,  SIGNAL(clicked()),     this, &UiMainwindow::trig50pctButtonClicked);
    //   connect(acqButton,        SIGNAL(clicked()),     this, &UiMainwindow::acqButtonClicked);
    //   connect(cursButton,       SIGNAL(clicked()),     this, &UiMainwindow::cursButtonClicked);
    //   connect(saveButton,       SIGNAL(clicked()),     this, &UiMainwindow::saveButtonClicked);
    //   connect(dispButton,       SIGNAL(clicked()),     this, &UiMainwindow::dispButtonClicked);
    //   connect(utilButton,       SIGNAL(clicked()),     this, &UiMainwindow::utilButtonClicked);
    //   connect(helpButton,       SIGNAL(clicked()),     this, &UiMainwindow::helpButtonClicked);
    //
    //   connect(horPosDial, SIGNAL(customContextMenuRequested(QPoint)), this, &UiMainwindow::horPosDialClicked(QPoint)));
    //   connect(vertOffsetDial, SIGNAL(customContextMenuRequested(QPoint)), this, &UiMainwindow::vertOffsetDialClicked(QPoint)));
    //   connect(horScaleDial,   SIGNAL(customContextMenuRequested(QPoint)), this, &UiMainwindow::horScaleDialClicked(QPoint)));
    //   connect(vertScaleDial,  SIGNAL(customContextMenuRequested(QPoint)), this, &UiMainwindow::vertScaleDialClicked(QPoint)));
    //   connect(trigAdjustDial, SIGNAL(customContextMenuRequested(QPoint)), this, &UiMainwindow::trigAdjustDialClicked(QPoint)));
    //   connect(adjDial,        SIGNAL(customContextMenuRequested(QPoint)), this, &UiMainwindow::adjustDialClicked(QPoint)));
    ///// TEST /////////////////////////////////////

    show();

    openConnection();
    //  UIDecoderWindow w{this};
}

UiMainWindow::~UiMainWindow() {
    QSettings settings;

    settings.setValue("path/savedir", QByteArray{recentSaveDir});

    delete scrnThread;
    delete appfont;
    pthread_mutex_destroy(&devParms.mutexx);

    free(devParms.screenshotBuf);

    for(int i{}; i < MAX_CHNS; i++)
        free(devParms.waveBuf[i]);

    delete[] devParms.fftBufIn;
    delete[] devParms.fftBufOut;
    delete[] devParms.kissFftBuf;
}
