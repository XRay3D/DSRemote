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

#include <pthread.h>

#include "third_party/kiss_fft/kiss_fftr.h"
#define PROGRAM_NAME    "DSRemote"
#define PROGRAM_VERSION "0.37_2006141810"

enum {
    // clang-format off
    MAX_PATHLEN =              1024,

    MAX_CHNS =                 4,
    MAX_DIG_CHNS =             16,

    ADJDIAL_TIMER_IVAL_1 =     4000,
    ADJDIAL_TIMER_IVAL_2 =     2000,

    SCRN_SHOT_BMP_SZ =         1152054,

    WAVFRM_MAX_BUFSZ =         (1024 * 1024 * 2),

    FFT_MAX_BUFSZ =            4096,

    ADJ_DIAL_FUNC_NONE =       0,
    ADJ_DIAL_FUNC_HOLDOFF =    1,
    ADJ_DIAL_FUNC_ACQ_AVG =    2,

    NAV_DIAL_FUNC_NONE =       0,
    NAV_DIAL_FUNC_HOLDOFF =    1,

    LABEL_TIMER_IVAL =         1500,

    LABEL_ACTIVE_NONE =        0,
    LABEL_ACTIVE_CHAN1 =       1,
    LABEL_ACTIVE_CHAN2 =       2,
    LABEL_ACTIVE_CHAN3 =       3,
    LABEL_ACTIVE_CHAN4 =       4,
    LABEL_ACTIVE_TRIG =        5,
    LABEL_ACTIVE_FFT =         6,

    TMC_GDS_DELAY =            10000, // us

    TMC_CMD_CUE_SZ =           1024,

    TMC_THRD_RESULT_NONE =     0,
    TMC_THRD_RESULT_SCRN =     1,
    TMC_THRD_RESULT_CMD =      2,

    TMC_THRD_JOB_NONE =        0,
    TMC_THRD_JOB_TRIGEDGELEV = 1,
    TMC_THRD_JOB_TIMDELAY =    2,
    TMC_THRD_JOB_FFTHZDIV =    3,

    TMC_DIAL_TIMER_DELAY =     300,

    DECODE_MODE_TAB_PAR =      0,
    DECODE_MODE_TAB_UART =     1,
    DECODE_MODE_TAB_SPI =      2,
    DECODE_MODE_TAB_I2C =      3,

    DECODE_MODE_PAR =          0,
    DECODE_MODE_UART =         1,
    DECODE_MODE_SPI =          2,
    DECODE_MODE_I2C =          3,

    DECODE_MAX_CHARS =         512,
    // clang-format on
};

enum CHAN {
    OFF,
    CH_AN_1,
    CH_AN_2,
    CH_AN_3,
    CH_AN_4,
    EXT,
    EXT5,
    AC,
    ACL = AC,
};

enum CHD {
    CH_DI_0,
    CH_DI_1,
    CH_DI_2,
    CH_DI_3,
    CH_DI_4,
    CH_DI_5,
    CH_DI_6,
    CH_DI_7,
    CH_DI_8,
    CH_DI_9,
    CH_DI_10,
    CH_DI_11,
    CH_DI_12,
    CH_DI_13,
    CH_DI_14,
    CH_DI_15,
};

struct WaveformPreamble {
    int format;
    int type;
    int points;
    int count;
    double xIncrement[MAX_CHNS];
    double xOrigin[MAX_CHNS];
    double xReference[MAX_CHNS];
    double yIncrement[MAX_CHNS];
    double yOrigin[MAX_CHNS];
    int yReference[MAX_CHNS];
};

enum class Coup { // Coupling
    GND,
    DC,
    AC
};

struct Channel {
    int bwlimit;   // 20, 250 or 0MHz (off)
    Coup coupling; // 0=GND, 1=DC, 2=AC
    bool Display;  // 0=off, 1=on
    int impedance; // 0=1MOhm, 1=50Ohm
    int invert;    // 0=normal, 1=inverted
    int unit;      // 0=V, 1=W, 2=A, 3=U
    char unitstr[2];
    double offset; // expressed in volts
    double probe;  // Probe attenuation ratio e.g. 10:1
    double scale;
    int vernier;                // Vernier 1=on, 0=off (fine adjustment of vertical scale)
    double mathDecodeThreshold; // 0: threshold of decode channel 1 (SPI:MISO for modelserie 6)
                                // 1: threshold of decode channel 2 (SPI:MOSI for modelserie 6)
                                // 2: threshold of decode channel 3 (SPI:SCLK for modelserie 6)
                                // 3: threshold of decode channel 4 (SPI:SS for modelserie 6)
                                // (-4 x VerticalScale - VerticalOffset) to
                                // (4 x VerticalScale - VerticalOffset)
    double yinc;
    int yor;
    double xorigin;
};

struct LA {
    enum Digital {
        D0,
        D1,
        D2,
        D3,
        D4,
        D5,
        D6,
        D7,
        D8,
        D9,
        D10,
        D11,
        D12,
        D13,
        D14,
        D15,
        Count
    };
    enum Group {
        GROUP1 = D15 + 1,
        GROUP2,
        GROUP3,
        GROUP4
    };
    enum Pod { // POD1 (D0 to D7) and POD2 (D8 to D15).
        POD1,
        POD2
    };

    enum Size {
        SMALl,
        LARGe
    };

    int ACTive; // Digital | Group

    bool AUTosort; // 0: waveforms on the screen are D15 to D0 from top to bottom
                   // 1: waveforms on the screen are D0 to D15 from top to bottom
    struct DIGital {
        bool DISPlay; // Num 0 to 15 {{1|ON}|{0|OFF}}

        int POSition; // Num 0 to 15 When the waveform display mode is SMALl: 0 to 15
                      // When the waveform display mode is LARGe: 0 to 7
                      // For the waveform display mode, refer to the :LA:SIZE command

        char LABel[4]{}; // It can include English uppercase letters (A to Z)
                         // and numbers (0 to 9).
                         // It cannot exceed 4 characters.

    } DIGital[Count];

    int DISPlay;           // Digital | Group | Pod
    bool PODDISPlay[2];    // Num 1 | 2 {{1|ON}|{0|OFF}}
    float PODTHReshold[2]; // Num 1 | 2 -15.0V to +15.0V
    Size SIZE;             // SMALl: up to 16 waveforms can be displayed on
                           // the screen (0 to 15 from top to bottom).
                           // LARGe: up to 8 waveforms can be displayed on
                           // the screen (0 to 7 from top to bottom).
                           // The LARGe display mode is only available when
                           // the number of channels currently turned on is no more than 8.
    bool STATe;            //{{1|ON}|{0|OFF}}
    double TCALibrate;     // -100ns to 100ns
};

struct DeviceSettings {
    Channel chan[MAX_CHNS];
    short* waveBuf[MAX_CHNS];

    int connected;
    int connectionType; // 0=USB, 1=LAN
    char modelName[128];
    char serialNum[128];
    char softwVers[128];
    int modelSerie;            // 1=DS1000, 2=DS2000, etc.
    int horDivisions;          // number of horizontal divisions, 12 or 14
    int vertDivisions;         // number of vertical divisions, 8 or 10
    int useExtraVertDivisions; // If 1: use 10 vertical divisions instead of 8, DS1000Z only

    char hostName[128];

    int screenTimerIval;

    int channelCnt; // Device has 2 or 4 channels
    int bandwidth;  // Bandwidth in MHz

    int activechannel; // Last pressed channel button (used to know at which channel to apply scale change)

    double timebaseoffset; // Main timebase offset in Sec
                           // MemDepth/SamplingRate to 1s (when TimeScale < 20ms)
                           // MemDepth/SamplingRate to 10xTimeScale (when TimeScale >=20ms)
    double timebasescale;  // Main timebase scale in Sec/div, 500pSec to 50Sec

    int timebasedelayenable;    // 1=on, 0=off
    double timebasedelayoffset; //
    double timebasedelayscale;  //  (1 x 50 / sample rate) x 1 / 40  in seconds
    int timebasehrefmode;       // 0=center, 1=tpos, 2=user
    int timebasehrefpos;
    int timebasemode;       // 0=MAIN, 1=XY, 2=ROLL
    int timebasevernier;    // Vernier 1=on, 0=off (fine adjustment of timebase)
    int timebasexy1display; // XY mode for channel 1 & 2,  1=on, 0=off
    int timebasexy2display; // XY mode for channel 3 & 4,  1=on, 0=off

    int triggercoupling;                               // 0=AC, 1=DC, 2=LFReject, 3=HFReject
    double triggeredgelevel[7 /* FIXME CHAN::COUNT*/]; // Trigger level
    int triggeredgeslope;                              // 0=POS, 1=NEG, 2= RFAL
    CHAN triggeredgesource;                            // 0=chan1, 1=chan2, 2=chan3, 3=chan4, 4=ext, 5=ext5, 6=acl
    double triggerholdoff;                             // min. is 16nSec or 100nSec depends on series
    int triggermode;                                   // 0=edge, 1=pulse, 2=slope, 3=video, 4=pattern, 5=rs232,
                                                       // 6=i2c, 7=spi, 8=can, 9=usb
    int triggerstatus;                                 // 0=td, 1=wait, 2=run, 3=auto, 4=fin, 5=stop
    int triggersweep;                                  // 0=auto, 1=normal, 2=single

    int displaygrid;    // 0=none, 1=half, 2=full
    int displaytype;    // 0=vectors, 1=dots
    int displaygrading; // 0=minimum, 1=0.1, 2=0.2, 5=0.5, 1=10, 2=20, 5=50, 10000=infinite

    double samplerate;   // Samplefrequency
    int acquiretype;     // 0=normal, 1=average, 2=peak, 3=highres
    int acquireaverages; // 2, 4, 8, 16, 32, 64, etc. to 8192
    int acquirememdepth; // Number of waveform points that the oscilloscope can
                         // store in a single trigger sample. 0=AUTO

    int countersrc;     // 0=off, 1=ch1, 2=ch2, 3=ch3, 4=ch4
    double counterfreq; // Value of frequency counter

    int mathDecodeDisplay; // 0=off, 1=on
    int mathDecodeMode;    // 0=par, 1=uart, 2=spi, 3=iic
    int mathDecodeFormat;  // 0=hex, 1=ascii, 2=dec, 3=bin, 4=line
    int mathDecodePos;     // vertical position of the decode trace,
                           // the screen is divided into 400 parts vertically which
                           // are marked as 0 to 400 from top to bottom respectively
                           // the range of <pos> is from 50 to 350

    double mathDecodeThresholdUartTx; // threshold of RS232:TX for modelserie 6
    double mathDecodeThresholdUartRx; // threshold of RS232:RX for modelserie 6

    int mathDecodeThresholdAuto; // 0=off, 1=on

    CHAN mathDecodeSpiClk;                               // channel - 1
    CHAN mathDecodeSpiMiso;                              // channel (0=off)
    CHAN mathDecodeSpiMosi;                              // channel (0=off)
    CHAN mathDecodeSpiCs;                                // channel (0=off)
    int mathDecodeSpiSelect;                             // select cs level, 0=low, 1=high
    int mathDecodeSpiMode;                               // frame mode, 0=timeout, 1=cs (chip select)
    double mathDecodeSpiTimeout;                         // timeout
    int mathDecodeSpiPol;                                // polarity of serial data line, 0=negative, 1=positive
    int mathDecodeSpiEdge;                               // edge, 0=falling edge of clock, 1=rising edge of clock
    int mathDecodeSpiEnd;                                // endian, 0=lsb, 1=msb
    int mathDecodeSpiWidth;                              // databits, 8-32
    int mathDecodeSpiMosiNval;                           // number of decoded characters
    unsigned int mathDecodeSpiMosiVal[DECODE_MAX_CHARS]; // array with decoded characters
    int mathDecodeSpiMosiValPos[DECODE_MAX_CHARS];       // array with position of the decoded characters
    int mathDecodeSpiMosiValPosEnd[DECODE_MAX_CHARS];    // array with endposition of the decoded characters
    int mathDecodeSpiMisoNval;                           // number of decoded characters
    unsigned int mathDecodeSpiMisoVal[DECODE_MAX_CHARS]; // array with decoded characters
    int mathDecodeSpiMisoValPos[DECODE_MAX_CHARS];       // array with position of the decoded characters
    int mathDecodeSpiMisoValPosEnd[DECODE_MAX_CHARS];    // array with endposition of the decoded characters

    CHAN mathDecodeUartTx;                               // channel (0=off)
    CHAN mathDecodeUartRx;                               // channel (0=off)
    int mathDecodeUartPol;                               // polarity, 0=negative, 1=positive
    int mathDecodeUartEnd;                               // endian, 0=lsb, 1=msb
    int mathDecodeUartBaud;                              // baudrate
    int mathDecodeUartWidth;                             // databits, 5-8
    int mathDecodeUartStop;                              // stopbits, 0=1, 1=1.5, 2=2
    int mathDecodeUartPar;                               // parity, 0=none, 1=odd, 2=even
    int mathDecodeUartTxNval;                            // number of decoded characters
    unsigned char mathDecodeUartTxVal[DECODE_MAX_CHARS]; // array with decoded characters
    int mathDecodeUartTxValPos[DECODE_MAX_CHARS];        // array with position of the decoded characters
    int mathDecodeUartTxErr[DECODE_MAX_CHARS];           // array with protocol errors, non zero means an error
    int mathDecodeUartRxNval;                            // number of decoded characters
    unsigned char mathDecodeUartRxVal[DECODE_MAX_CHARS]; // array with decoded characters
    int mathDecodeUartRxValPos[DECODE_MAX_CHARS];        // array with position of the decoded characters
    int mathDecodeUartRxErr[DECODE_MAX_CHARS];           // array with protocol errors, non zero means an error

    char* screenshotBuf;
    int waveBufsz;

    int screenshotInv; // 0=normal, 1=inverted colors

    int screenupdatesOn;

    pthread_mutex_t mutexx;

    int threadErrorStat;
    int threadErrorLine;
    int threadResult;
    int threadJob;
    double threadValue;

    struct WaveformPreamble preamble;

    char cmdCue[TMC_CMD_CUE_SZ][128];
    char* cmdCueResp[TMC_CMD_CUE_SZ];
    int cmdCueIdxIn;
    int cmdCueIdxOut;

    int mathFftSrc;   // 0=ch1, 1=ch2, 2=ch3, 3=ch4
    int mathFft;      // 0=off, 1=on
    int mathFftSplit; // 0=off, 1=on
    int mathFftUnit;  // 0=VRMS, 1=DB
    double* fftBufIn;
    double* fftBufOut;
    int fftBufsz;
    kiss_fftr_cfg kCfg;
    kiss_fft_cpx* kissFftBuf;
    double mathFftHscale;
    double mathFftHcenter;
    double fftVScale;
    double fftVOffset;

    int currentScreenSf;

    int showFps;

    int fontSize;

    // below here is use for the wave inspector
    int waveMemViewSampleStart;
    int waveMemViewEnabled;

    double viewerCenterPosition;

    int funcWrecEnable;
    int funcWrecFend;
    int funcWrecFmax;
    double funcWrecFintval;
    int funcWrecPrompt;
    int funcWrecOperate;
    int funcWplayFstart;
    int funcWplayFend;
    int funcWplayFmax;
    double funcWplayFintval;
    int funcWplayMode;
    int funcWplayDir;
    int funcWplayOperate;
    int funcWplayFcur;
    int funcHasRecord;

    LA la;
};
