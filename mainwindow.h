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

#pragma once

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QDesktopServices>
#include <QDial>
#include <QDockWidget>
#include <QEventLoop>
#include <QFileDialog>
#include <QFont>
#include <QGroupBox>
#include <QImage>
#include <QKeySequence>
#include <QLabel>
#include <QLibrary>
#include <QList>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QObject>
#include <QPalette>
#include <QPixmap>
#include <QPoint>
#include <QProcess>
#include <QProgressDialog>
#include <QRect>
#include <QSettings>
#include <QSplashScreen>
#include <QStatusBar>
#include <QString>
#include <QTimer>
#include <QToolBar>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>
#include <QtGlobal>
#include <QtGui>

#include <locale.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "about_dialog.h"
#include "connection.h"
#include "decode_dialog.h"
#include "dial.h"
#include "edflib.h"
#include "global.h"
#include "lan_connect_thread.h"
#include "playback_dialog.h"
#include "read_settings_thread.h"
#include "save_data_thread.h"
#include "screen_thread.h"
#include "settings_dialog.h"
#include "signalcurve.h"
#include "tled.h"
#include "tmc_dev.h"
#include "ui_controlpanel.h"
#include "utils.h"
#include "wave_dialog.h"

#include "third_party/kiss_fft/kiss_fftr.h"

class SignalCurve;

class UiMainWindow : public QMainWindow, public Ui::ControlPanel {
    Q_OBJECT

public:
    UiMainWindow();
    ~UiMainWindow();

    char recentDir[MAX_PATHLEN], recentSaveDir[MAX_PATHLEN];

    void readSettings(void);
    void writeSettings(void);
    void setCueCmd(const char*);
    void setCueCmd(const char*, char*);
    void serialDecoder(struct DeviceSettings*);
    void saveWaveInspectorBufferToEdf(struct DeviceSettings*);

    struct DeviceSettings devParms;

    QLabel* statusLabel;

    QFont* appfont;

    int adjDialFunc,
        navDialFunc;

    QTimer
        *scrnTimer,
        *labelTimer;

    ScreenThread* scrnThread;

private:
    QMenuBar* menubar;

    QMenu
        *devicemenu,
        *settingsmenu,
        *helpmenu;

    QTimer
        *adjdialTimer,
        *navDialTimer,
        *horPosDialTimer,
        *trigAdjDialTimer,
        *vertOffsDialTimer,
        *horScaleDialTimer,
        *vertScaleDialTimer,
        *testTimer;

    QStatusBar* statusBar;

    QDockWidget* dockPanelRight;
    QWidget* DPRwidget;

    std::array<QPushButton*, MAX_CHNS> chButtons;

    QString defStylesh;

    QImage screenXpm;

    QAction
        *formerPageAct,
        *shiftPageLeftAct,
        *shiftPageRightAct,
        *nextPageAct,
        *zoomInAct,
        *zoomOutAct,
        *chanScalePlusAct,
        *chanScalePlusAllChannelsAct,
        *chanScaleMinusAct,
        *chanScaleMinusAllChannelsAct,
        *shiftTraceUpAct,
        *shiftTraceDownAct,
        *selectChan1Act,
        *selectChan2Act,
        *selectChan3Act,
        *selectChan4Act,
        *toggleFftAct,
        *centerTriggerAct,
        *centerPositionAct,
        *saveScreenshotAct;

    struct tmcDev* device;

    SignalCurve* waveForm;

    int parsePreamble(char*, int, struct WaveformPreamble*, int);
    int getMetricFactor(double);
    void getDeviceModel(const char*);
    double getStepSizeDivideBy1000(double);
    inline unsigned char reverseBitOrder8(unsigned char);
    inline unsigned int reverseBitOrder32(unsigned int);
    int getDeviceSettings(int delay = 0);

private slots:

    void scrnTimerHandler();
    void screenUpdate();
    void adjdialTimerHandler();
    void labelTimerHandler();
    void testTimerHandler();
    void horPosDialTimerHandler();
    void trigAdjDialTimerHandler();
    void vertOffsDialTimerHandler();
    void horScaleDialTimerHandler();
    void vertScaleDialTimerHandler();

    void showAboutDialog();
    void showHowtoOperate();
    void openConnection();
    void closeConnection();
    void openSettingsDialog();
    void saveScreenWaveform();
    void getDeepMemoryWaveform();
    void saveScreenshot();

    void adjDialChanged(int);
    void trigAdjustDialChanged(int);
    void horScaleDialChanged(int);
    void horPosDialChanged(int);
    void vertOffsetDialChanged(int);
    void vertScaleDialChanged(int);

    void ch1ButtonClicked();
    void ch2ButtonClicked();
    void ch3ButtonClicked();
    void ch4ButtonClicked();
    void clearButtonClicked();
    void autoButtonClicked();
    void runButtonClicked();
    void singleButtonClicked();
    void horMenuButtonClicked();
    void trigModeButtonClicked();
    void trigMenuButtonClicked();
    void trigForceButtonClicked();
    void trig50pctButtonClicked();
    void acqButtonClicked();
    void cursButtonClicked();
    void saveButtonClicked();
    void dispButtonClicked();
    void utilButtonClicked();
    void helpButtonClicked();
    void measureButtonClicked();

    void horizontalDelayedToggle();
    void horizontalDelayedOn();
    void horizontalDelayedOff();

    void counterOff();
    void counterCh1();
    void counterCh2();
    void counterCh3();
    void counterCh4();

    void triggerSourceCh1();
    void triggerSourceCh2();
    void triggerSourceCh3();
    void triggerSourceCh4();
    void triggerSourceExt();
    void triggerSourceExt5();
    void triggerSourceAcl();
    void triggerCouplingDc();
    void triggerCouplingAc();
    void triggerCouplingLfReject();
    void triggerCouplingHfReject();
    void triggerSlopePos();
    void triggerSlopeNeg();
    void triggerSlopeRfal();
    void triggerSettingHoldOff();

    void horPosDialClicked(QPoint);
    void vertOffsetDialClicked(QPoint);
    void horScaleDialClicked(QPoint);
    void vertScaleDialClicked(QPoint);
    void trigAdjustDialClicked(QPoint);
    void adjustDialClicked(QPoint);

    void navDialReleased();
    void navDialTimerHandler();
    void navDialChanged(int);

    void setGridTypeVectors();
    void setGridTypeDots();

    void setGridFull();
    void setGridHalf();
    void setGridNone();

    void setGradingMin();
    void setGradingX005();
    void setGradingX01();
    void setGradingX02();
    void setGradingX05();
    void setGradingX1();
    void setGradingX2();
    void setGradingX5();
    void setGradingX10();
    void setGradingX20();
    void setGradingInf();

    void chanCouplingAc();
    void chanCouplingDc();
    void chanCouplingGnd();
    void chanBwlOff();
    void chanBwl20MHz();
    void chanBwl100MHz();
    void chanBwl200MHz();
    void chanBwl250MHz();
    void chanInvertOn();
    void chanInvertOff();
    void chanProbeX001();
    void chanProbeX002();
    void chanProbeX005();
    void chanProbeX01();
    void chanProbeX02();
    void chanProbeX05();
    void chanProbeX1();
    void chanProbeX2();
    void chanProbeX5();
    void chanProbeX10();
    void chanProbeX20();
    void chanProbeX50();
    void chanProbeX100();
    void chanProbeX200();
    void chanProbeX500();
    void chanProbeX1000();

    void chanUnitV();
    void chanUnitW();
    void chanUnitA();
    void chanUnitU();

    void chanMenu();
    void mathMenu();

    void setAcqNormal();
    void setAcqAverage();
    void setAcqPeak();
    void setAcqHres();

    void setMemDepth(int);
    void setMemDepthAuto();
    void setMemDepth3k();
    void setMemDepth6k();
    void setMemDepth7k();
    void setMemDepth12k();
    void setMemDepth14k();
    void setMemDepth30k();
    void setMemDepth60k();
    void setMemDepth70k();
    void setMemDepth120k();
    void setMemDepth140k();
    void setMemDepth300k();
    void setMemDepth600k();
    void setMemDepth700k();
    void setMemDepth1200k();
    void setMemDepth1400k();
    void setMemDepth12M();
    void setMemDepth24M();
    void setMemDepth6M();
    void setMemDepth3M();
    void setMemDepth7M();
    void setMemDepth70M();
    void setMemDepth14M();
    void setMemDepth140M();
    void setMemDepth28M();
    void setMemDepth56M();

    void formerPage();
    void shiftPageLeft();
    void shiftPageRight();
    void nextPage();
    void zoomIn();
    void zoomOut();
    void chanScalePlus();
    void chanScalePlusAll();
    void chanScaleMinusAll();
    void chanScaleMinus();
    void shiftTraceUp();
    void shiftTraceDown();
    void centerTrigger();

    void setToFactory();

    void toggleFft();
    void toggleFftSplit();
    void toggleFftUnit();
    void selectFftCh1();
    void selectFftCh2();
    void selectFftCh3();
    void selectFftCh4();
    void selectFftHzdiv20();
    void selectFftHzdiv40();
    void selectFftHzdiv80();
    void selectFftHzdiv100();
    void selectFftHzdiv200();
    void setFftHzdiv(double);
    void selectFftCtr5();
    void selectFftCtr6();
    void selectFftCtr7();
    void selectFftCtr8();
    void selectFftCtr9();
    void selectFftCtr10();
    void selectFftCtr11();
    void selectFftCtr12();
    void selectFftVScale1();
    void selectFftVScale2();
    void selectFftVScale5();
    void selectFftVScale10();
    void selectFftVScale20();
    void setFftVScale();
    void selectFftVOffsetp4();
    void selectFftVOffsetp3();
    void selectFftVOffsetp2();
    void selectFftVOffsetp1();
    void selectFftVOffset0();
    void selectFftVOffsetm1();
    void selectFftVOffsetm2();
    void selectFftVOffsetm3();
    void selectFftVOffsetm4();
    void setFftVOffset();

    void showDecodeWindow();

    void showPlaybackWindow();
    void playpauseButtonClicked();
    void stopButtonClicked();
    void recordButtonClicked();

    void updateLabels();

protected:
    void closeEvent(QCloseEvent*);
};
