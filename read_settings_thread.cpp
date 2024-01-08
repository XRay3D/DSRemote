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

#include "read_settings_thread.h"
#include <unordered_map>

using namespace std::literals;

template <typename K = std::string_view, typename V = int>
struct Pair {
    using Key = K;
    using Val = V;
    Key key;
    Val val;
};

template <size_t N, typename Key = std::string_view, typename Val = int>
struct Map : std::array<Pair<Key, Val>, N> {
    using Pair_ = Pair<Key, Val>;
    using Array = std::array<Pair_, N>;
    using Array::Array;

    template <typename Pair, typename... Pairs>
    consteval Map(Pair&& pair, Pairs&&... pairs)
        : Array{std::forward<Pair>(pair), std::forward<Pairs>(pairs)...} {
        for(size_t i{}; i < N; ++i)
            for(size_t j = i + 1; j < N; ++j)
                if(Array::at(i).key == Array::at(j).key)
                    throw "Two sane keys!!";
    }

    Val default_{};

    constexpr const Val& operator[](const Key& key) const {
        auto it = std::ranges::find(*this, key, &Pair_::key);
        return it == Array::end() ? default_ : it->val;
    }
    constexpr const Val& at(const Key& key) const {
        auto it = std::ranges::find(*this, key, &Pair_::key);
        if(it == Array::end()) throw std::range_error{__PRETTY_FUNCTION__};
        return it->val;
    }
};

template <typename K = std::string_view, typename V = int, typename... Pairs>
Map(Pair<K, V>&& pair, Pairs&&... pairs) -> Map<sizeof...(Pairs) + 1, K, V>;

ReadSettingsThread::ReadSettingsThread() {
    device = nullptr;
    errStr[0] = 0;
    errNum = -1;
    devParms = nullptr;
    delay = 0;
}

void ReadSettingsThread::setDelay(int val) {
    delay = val;
}

int ReadSettingsThread::getErrorNum(void) {
    return errNum;
}

void ReadSettingsThread::getErrorStr(char* dest, int sz) {
    strlcpy(dest, errStr, sz);
}

void ReadSettingsThread::setDevice(struct tmcDev* dev) {
    device = dev;
}

void ReadSettingsThread::setDevparmPtr(struct DeviceSettings* devp) {
    devParms = devp;
}

void ReadSettingsThread::run() {
    errNum = -1;
    if(device == nullptr) return;
    if(devParms == nullptr) return;

    devParms->activechannel = -1;

    if(delay > 0) sleep(delay);

    int errLine{};
    try {
        errLine = [this] {
            auto writeRead = []<size_t N>(const char(&str)[N], int len = N - 1) {
                return (tmcWrite(str) != len || tmcRead() < 1);
            };

            char str[512] = "";
            for(int chn = 0; chn < devParms->channelCnt; chn++) {
                usleep(TMC_GDS_DELAY);
                snprintf(str, 512, ":CHAN%i:BWL?", chn + 1);
                if(writeRead(str, 11)) return __LINE__;
                constexpr Map bwl{
                    Pair{"20M"sv,  20 },
                    Pair{"250M"sv, 250},
                    Pair{"OFF"sv,  0  },
                };
                devParms->chan[chn].bwlimit = bwl.at(device->buf);

                usleep(TMC_GDS_DELAY);
                snprintf(str, 512, ":CHAN%i:COUP?", chn + 1);
                if(writeRead(str, 12)) return __LINE__;
                constexpr Map coup{
                    Pair{"AC"sv,  Coup::AC },
                    Pair{"DC"sv,  Coup::DC },
                    Pair{"GND"sv, Coup::GND},
                };
                devParms->chan[chn].coupling = coup.at(device->buf);

                usleep(TMC_GDS_DELAY);
                snprintf(str, 512, ":CHAN%i:DISP?", chn + 1);
                if(writeRead(str, 12)) return __LINE__;
                if(!strcmp(device->buf, "0")) devParms->chan[chn].Display = 0;
                else if(!strcmp(device->buf, "1")) {
                    devParms->chan[chn].Display = 1;
                    if(devParms->activechannel == -1)
                        devParms->activechannel = chn;
                } else return __LINE__;

                if(devParms->modelSerie != 1) {
                    usleep(TMC_GDS_DELAY);
                    snprintf(str, 512, ":CHAN%i:IMP?", chn + 1);
                    if(writeRead(str, 11)) return __LINE__;
                    if(!strcmp(device->buf, "OMEG")) devParms->chan[chn].impedance = 0;
                    else if(!strcmp(device->buf, "FIFT")) devParms->chan[chn].impedance = 1;
                    else return __LINE__;
                }

                usleep(TMC_GDS_DELAY);
                snprintf(str, 512, ":CHAN%i:INV?", chn + 1);
                if(writeRead(str, 11)) return __LINE__;
                if(!strcmp(device->buf, "0")) devParms->chan[chn].invert = 0;
                else if(!strcmp(device->buf, "1")) devParms->chan[chn].invert = 1;
                else return __LINE__;

                usleep(TMC_GDS_DELAY);
                snprintf(str, 512, ":CHAN%i:OFFS?", chn + 1);
                if(writeRead(str, 12)) return __LINE__;
                devParms->chan[chn].offset = atof(device->buf);

                usleep(TMC_GDS_DELAY);
                snprintf(str, 512, ":CHAN%i:PROB?", chn + 1);
                if(writeRead(str, 12)) return __LINE__;
                devParms->chan[chn].probe = atof(device->buf);

                usleep(TMC_GDS_DELAY);
                snprintf(str, 512, ":CHAN%i:UNIT?", chn + 1);
                if(writeRead(str, 12)) return __LINE__;
                constexpr Map unit{
                    Pair{"VOLT"sv, 0},
                    Pair{"WATT"sv, 1},
                    Pair{"AMP"sv,  2},
                    Pair{"UNKN"sv, 3},
                };
                devParms->chan[chn].unit = unit[device->buf];

                usleep(TMC_GDS_DELAY);
                snprintf(str, 512, ":CHAN%i:SCAL?", chn + 1);
                if(writeRead(str, 12)) return __LINE__;
                devParms->chan[chn].scale = atof(device->buf);

                usleep(TMC_GDS_DELAY);
                snprintf(str, 512, ":CHAN%i:VERN?", chn + 1);
                if(writeRead(str, 12)) return __LINE__;
                if(!strcmp(device->buf, "0")) devParms->chan[chn].vernier = 0;
                else if(!strcmp(device->buf, "1")) devParms->chan[chn].vernier = 1;
                else return __LINE__;

            } // for

            usleep(TMC_GDS_DELAY);
            if(writeRead(":TIM:OFFS?", 10)) return __LINE__;
            devParms->timebaseoffset = atof(device->buf);

            usleep(TMC_GDS_DELAY);
            if(writeRead(":TIM:SCAL?", 10)) return __LINE__;
            devParms->timebasescale = atof(device->buf);

            usleep(TMC_GDS_DELAY);
            if(writeRead(":TIM:DEL:ENAB?", 14)) return __LINE__;
            if(!strcmp(device->buf, "0")) devParms->timebasedelayenable = 0;
            else if(!strcmp(device->buf, "1")) devParms->timebasedelayenable = 1;
            else return __LINE__;

            usleep(TMC_GDS_DELAY);
            if(writeRead(":TIM:DEL:OFFS?", 14)) return __LINE__;
            devParms->timebasedelayoffset = atof(device->buf);

            usleep(TMC_GDS_DELAY);
            if(writeRead(":TIM:DEL:SCAL?", 14)) return __LINE__;
            devParms->timebasedelayscale = atof(device->buf);

            if(devParms->modelSerie != 1) {
                usleep(TMC_GDS_DELAY);
                if(writeRead(":TIM:HREF:MODE?", 15)) return __LINE__;
                if(!strcmp(device->buf, "CENT")) devParms->timebasehrefmode = 0;
                else if(!strcmp(device->buf, "TPOS")) devParms->timebasehrefmode = 1;
                else if(!strcmp(device->buf, "USER")) devParms->timebasehrefmode = 2;
                else return __LINE__;

                usleep(TMC_GDS_DELAY);
                if(writeRead(":TIM:HREF:POS?", 14)) return __LINE__;
                devParms->timebasehrefpos = atoi(device->buf);
            }

            usleep(TMC_GDS_DELAY);
            if(writeRead(":TIM:MODE?", 10)) return __LINE__;
            if(!strcmp(device->buf, "MAIN")) devParms->timebasemode = 0;
            else if(!strcmp(device->buf, "XY")) devParms->timebasemode = 1;
            else if(!strcmp(device->buf, "ROLL")) devParms->timebasemode = 2;
            else return __LINE__;

            if(devParms->modelSerie != 1) {
                usleep(TMC_GDS_DELAY);
                if(writeRead(":TIM:VERN?", 10)) return __LINE__;
                if(!strcmp(device->buf, "0")) devParms->timebasevernier = 0;
                else if(!strcmp(device->buf, "1")) devParms->timebasevernier = 1;
                else return __LINE__;
            }

            if((devParms->modelSerie != 1) && (devParms->modelSerie != 2)) {
                usleep(TMC_GDS_DELAY);
                if(writeRead(":TIM:XY1:DISP?", 14)) return __LINE__;
                if(!strcmp(device->buf, "0")) devParms->timebasexy1display = 0;
                else if(!strcmp(device->buf, "1")) devParms->timebasexy1display = 1;
                else return __LINE__;

                usleep(TMC_GDS_DELAY);
                if(writeRead(":TIM:XY2:DISP?", 14)) return __LINE__;
                if(!strcmp(device->buf, "0")) devParms->timebasexy2display = 0;
                else if(!strcmp(device->buf, "1")) devParms->timebasexy2display = 1;
                else return __LINE__;
            }

            usleep(TMC_GDS_DELAY);
            if(writeRead(":TRIG:COUP?", 11)) return __LINE__;
            constexpr Map trigCoup{
                Pair{"AC"sv,  0},
                Pair{"DC"sv,  1},
                Pair{"LFR"sv, 2},
                Pair{"HFR"sv, 3},
            };
            devParms->triggercoupling = trigCoup.at(device->buf);

            usleep(TMC_GDS_DELAY);
            if(writeRead(":TRIG:SWE?", 10)) return __LINE__;
            constexpr Map trigSweep{
                Pair{"AUTO"sv, 0},
                Pair{"NORM"sv, 1},
                Pair{"SING"sv, 2},
            };
            devParms->triggersweep = trigSweep.at(device->buf);

            usleep(TMC_GDS_DELAY);
            if(writeRead(":TRIG:MODE?", 11)) return __LINE__;
            constexpr Map trigMode{
                Pair{"EDGE"sv,  0},
                Pair{"PULS"sv,  1},
                Pair{"SLOP"sv,  2},
                Pair{"VID"sv,   3},
                Pair{"PATT"sv,  4},
                Pair{"RS232"sv, 5},
                Pair{"IIC"sv,   6},
                Pair{"SPI"sv,   7},
                Pair{"CAN"sv,   8},
                Pair{"USB"sv,   9},
            };
            devParms->triggermode = trigMode.at(device->buf);

            usleep(TMC_GDS_DELAY);
            if(writeRead(":TRIG:STAT?", 11)) return __LINE__;
            constexpr Map trigStatus{
                Pair{"TD"sv,   0},
                Pair{"WAIT"sv, 1},
                Pair{"RUN"sv,  2},
                Pair{"AUTO"sv, 3},
                Pair{"FIN"sv,  4},
                Pair{"STOP"sv, 5},
            };
            devParms->triggerstatus = trigStatus.at(device->buf);

            usleep(TMC_GDS_DELAY);
            if(writeRead(":TRIG:EDG:SLOP?", 15)) return __LINE__;
            constexpr Map trigEdgeSlope{
                Pair{"POS"sv,  0},
                Pair{"NEG"sv,  1},
                Pair{"RFAL"sv, 2},
            };
            devParms->triggeredgeslope = trigEdgeSlope.at(device->buf);

            usleep(TMC_GDS_DELAY);
            constexpr Map chan{
                Pair{"CHAN1"sv, CH_AN_1},
                Pair{"CHAN2"sv, CH_AN_2},
                Pair{"CHAN3"sv, CH_AN_3},
                Pair{"CHAN4"sv, CH_AN_4},
                Pair{"EXT"sv,   EXT    },
                Pair{"EXT5"sv,  EXT5   },
                Pair{"AC"sv,    AC     },
                Pair{"ACL"sv,   ACL    },
            };

            // const std::unordered_map<std::string_view, CHD> chd{
            //     {"D0"sv,  CH_DI_0 },
            //     {"D1"sv,  CH_DI_1 },
            //     {"D2"sv,  CH_DI_2 },
            //     {"D3"sv,  CH_DI_3 },
            //     {"D4"sv,  CH_DI_4 },
            //     {"D5"sv,  CH_DI_5 },
            //     {"D6"sv,  CH_DI_6 },
            //     {"D7"sv,  CH_DI_7 },
            //     {"D8"sv,  CH_DI_8 },
            //     {"D9"sv,  CH_DI_9 },
            //     {"D10"sv, CH_DI_10},
            //     {"D11"sv, CH_DI_11},
            //     {"D12"sv, CH_DI_12},
            //     {"D13"sv, CH_DI_13},
            //     {"D14"sv, CH_DI_14},
            //     {"D15"sv, CH_DI_15},
            // };

            if(writeRead(":TRIG:EDG:SOUR?", 15)) return __LINE__;
            devParms->triggeredgesource = chan.at(device->buf);

            for(int chn = 0; chn < devParms->channelCnt; chn++) {
                usleep(TMC_GDS_DELAY);
                snprintf(str, 512, ":TRIG:EDG:SOUR CHAN%i", chn + 1);
                if(tmcWrite(str) != 20) return __LINE__;

                usleep(TMC_GDS_DELAY);
                if(writeRead(":TRIG:EDG:LEV?", 14)) return __LINE__;
                devParms->triggeredgelevel[chn] = atof(device->buf);
            }

            if(devParms->triggeredgesource < 4) {
                snprintf(str, 512, ":TRIG:EDG:SOUR CHAN%i", devParms->triggeredgesource + 1);
                usleep(TMC_GDS_DELAY);
                if(tmcWrite(str) != 20) return __LINE__;
            }

            if(devParms->triggeredgesource == 4) {
                usleep(TMC_GDS_DELAY);
                if(tmcWrite(":TRIG:EDG:SOUR EXT")) return __LINE__;
            }

            if(devParms->triggeredgesource == 5) {
                usleep(TMC_GDS_DELAY);
                if(tmcWrite(":TRIG:EDG:SOUR EXT5")) return __LINE__;
            }

            if(devParms->triggeredgesource == 6) {
                usleep(TMC_GDS_DELAY);
                if(tmcWrite(":TRIG:EDG:SOUR AC")) return __LINE__;
            }

            usleep(TMC_GDS_DELAY);
            if(writeRead(":TRIG:HOLD?", 11)) return __LINE__;
            devParms->triggerholdoff = atof(device->buf);

            usleep(TMC_GDS_DELAY);
            if(writeRead(":ACQ:SRAT?", 10)) return __LINE__;
            devParms->samplerate = atof(device->buf);

            usleep(TMC_GDS_DELAY);
            if(writeRead(":DISP:GRID?", 11)) return __LINE__;
            if(!strcmp(device->buf, "NONE")) devParms->displaygrid = 0;
            else if(!strcmp(device->buf, "HALF")) devParms->displaygrid = 1;
            else if(!strcmp(device->buf, "FULL")) devParms->displaygrid = 2;
            else return __LINE__;

            usleep(TMC_GDS_DELAY);
            if(writeRead(":MEAS:COUN:SOUR?", 16)) return __LINE__;
            devParms->countersrc = Map{
                Pair{"OFF"sv,   0},
                Pair{"CHAN1"sv, 1},
                Pair{"CHAN2"sv, 2},
                Pair{"CHAN3"sv, 3},
                Pair{"CHAN4"sv, 4}
            }.at(device->buf);

            usleep(TMC_GDS_DELAY);
            if(writeRead(":DISP:TYPE?", 11)) return __LINE__;
            constexpr Map dispType{
                Pair{"VECT"sv, 0},
                Pair{"DOTS"sv, 1},
            };
            devParms->displaytype = dispType.at(device->buf);

            usleep(TMC_GDS_DELAY);
            if(writeRead(":ACQ:TYPE?", 10)) return __LINE__;
            devParms->acquiretype = Map{
                Pair{"NORM"sv, 0},
                Pair{"AVER"sv, 1},
                Pair{"PEAK"sv, 2},
                Pair{"HRES"sv, 3}
            }.at(device->buf);

            usleep(TMC_GDS_DELAY);
            if(writeRead(":ACQ:AVER?", 10)) return __LINE__;
            devParms->acquireaverages = atoi(device->buf);

            usleep(TMC_GDS_DELAY);
            if(writeRead(":DISP:GRAD:TIME?", 16)) return __LINE__;
            devParms->displaygrading = Map{
                Pair{"MIN"sv, 0    },
                Pair{"0.1"sv, 1    },
                Pair{"0.2"sv, 2    },
                Pair{"0.5"sv, 5    },
                Pair{"1"sv,   10   },
                Pair{"2"sv,   20   },
                Pair{"5"sv,   50   },
                Pair{"INF"sv, 10000}
            }.at(device->buf);

            usleep(TMC_GDS_DELAY);
            if(devParms->modelSerie != 1) {
                if(writeRead(":CALC:FFT:SPL?", 14)) return __LINE__;
            } else if(writeRead(":MATH:FFT:SPL?", 14)) return __LINE__;
            devParms->mathFftSplit = atoi(device->buf);

            usleep(TMC_GDS_DELAY);

            if(devParms->modelSerie != 1) {
                if(writeRead(":CALC:MODE?", 11)) return __LINE__;
                if(!strcmp(device->buf, "FFT")) devParms->mathFft = 1;
                else devParms->mathFft = 0;
            } else {
                if(writeRead(":MATH:DISP?", 11)) return __LINE__;
                devParms->mathFft = atoi(device->buf);

                if(devParms->mathFft == 1) {
                    usleep(TMC_GDS_DELAY);

                    if(writeRead(":MATH:OPER?", 11)) return __LINE__;
                    if(!strcmp(device->buf, "FFT")) devParms->mathFft = 1;
                    else devParms->mathFft = 0;
                }
            }

            usleep(TMC_GDS_DELAY);
            if(devParms->modelSerie != 1) {
                if(writeRead(":CALC:FFT:VSM?", 14)) return __LINE__;
            } else if(writeRead(":MATH:FFT:UNIT?", 15)) return __LINE__;
            if(!strcmp(device->buf, "VRMS")) {
                devParms->fftVScale = 0.5;
                devParms->fftVOffset = -2.0;
                devParms->mathFftUnit = 0;
            } else {
                devParms->fftVScale = 10.0;
                devParms->fftVOffset = 20.0;
                devParms->mathFftUnit = 1;
            }

            usleep(TMC_GDS_DELAY);
            if(devParms->modelSerie != 1) {
                if(writeRead(":CALC:FFT:SOUR?", 15)) return __LINE__;
            } else if(writeRead(":MATH:FFT:SOUR?", 15)) return __LINE__;
            devParms->mathFftSrc = Map{
                Pair{"CHAN1"sv, 0},
                Pair{"CHAN2"sv, 1},
                Pair{"CHAN3"sv, 2},
                Pair{"CHAN4"sv, 3}
            }[device->buf];

            usleep(TMC_GDS_DELAY);
            devParms->currentScreenSf = 100.0 / devParms->timebasescale;
            if(devParms->modelSerie != 1) {
                if(writeRead(":CALC:FFT:HSP?", 14)) return __LINE__;
                devParms->mathFftHscale = atof(device->buf);

                //     if(tmc_write(":CALC:FFT:HSC?") != 14)
                //     {            //       return  __LINE__;
                //                   //     }
                //
                //     if(tmc_read() < 1)
                //     {            //       return  __LINE__;
                //                   //     }
                //
                //     switch(atoi(device->buf))
                //     {
                // //       case  0: devParms->math_fft_hscale = devParms->current_screen_sf / 80.0;
                // //                break;
                //       case  1: devParms->math_fft_hscale = devParms->current_screen_sf / 40.0;
                //                break;
                //       case  2: devParms->math_fft_hscale = devParms->current_screen_sf / 80.0;
                //                break;
                //       case  3: devParms->math_fft_hscale = devParms->current_screen_sf / 200.0;
                //                break;
                //       default: devParms->math_fft_hscale = devParms->current_screen_sf / 40.0;
                //                break;
                //     }
            } else {
                if(writeRead(":MATH:FFT:HSC?", 14)) return __LINE__;
                devParms->mathFftHscale = atof(device->buf);
            }

            usleep(TMC_GDS_DELAY);
            if(devParms->modelSerie != 1) {
                if(writeRead(":CALC:FFT:HCEN?", 15)) return __LINE__;
            } else if(writeRead(":MATH:FFT:HCEN?", 15)) return __LINE__;
            devParms->mathFftHcenter = atof(device->buf);

            usleep(TMC_GDS_DELAY);
            if(devParms->modelSerie != 1) {
                if(writeRead(":CALC:FFT:VOFF?", 15)) return __LINE__;
            } else if(writeRead(":MATH:OFFS?", 11)) return __LINE__;
            devParms->fftVOffset = atof(device->buf);

            usleep(TMC_GDS_DELAY);
            if(devParms->modelSerie != 1) {
                if(writeRead(":CALC:FFT:VSC?", 14)) return __LINE__;
                if(devParms->mathFftUnit == 1)
                    devParms->fftVScale = atof(device->buf);
                else
                    devParms->fftVScale = atof(device->buf) * devParms->chan[devParms->mathFftSrc].scale;
            } else {
                if(writeRead(":MATH:SCAL?", 11)) return __LINE__;
                devParms->fftVScale = atof(device->buf);
            }

            usleep(TMC_GDS_DELAY);

            if(devParms->modelSerie != 1) {
                if(writeRead(":BUS1:MODE?", 11)) return __LINE__;
            } else if(writeRead(":DEC1:MODE?", 11)) return __LINE__;
            devParms->mathDecodeMode = Map{
                Pair{"PAR"sv,   0},
                Pair{"UART"sv,  1},
                Pair{"RS232"sv, 1},
                Pair{"SPI"sv,   2},
                Pair{"IIC"sv,   3}
            }[device->buf];

            usleep(TMC_GDS_DELAY);
            if(devParms->modelSerie != 1) {
                if(writeRead(":BUS1:DISP?", 11)) return __LINE__;
            } else if(writeRead(":DEC1:DISP?", 11)) return __LINE__;
            devParms->mathDecodeDisplay = atoi(device->buf);

            usleep(TMC_GDS_DELAY);
            if(devParms->modelSerie != 1) {
                if(writeRead(":BUS1:FORM?", 11)) return __LINE__;
            } else if(writeRead(":DEC1:FORM?", 11)) return __LINE__;
            devParms->mathDecodeFormat = Map{
                Pair{"HEX"sv,  0},
                Pair{"ASC"sv,  1},
                Pair{"DEC"sv,  2},
                Pair{"BIN"sv,  3},
                Pair{"LINE"sv, 4}
            }[device->buf];

            usleep(TMC_GDS_DELAY);
            if(devParms->modelSerie != 1) {
                if(writeRead(":BUS1:SPI:OFFS?", 15)) return __LINE__;
            } else if(writeRead(":DEC1:POS?", 10)) return __LINE__;
            devParms->mathDecodePos = atoi(device->buf);

            usleep(TMC_GDS_DELAY);
            if(devParms->modelSerie != 1) {
                if(writeRead(":BUS1:SPI:MISO:THR?", 19)) return __LINE__;
            } else if(writeRead(":DEC1:THRE:CHAN1?", 17)) return __LINE__;
            devParms->chan[0].mathDecodeThreshold = atof(device->buf);

            usleep(TMC_GDS_DELAY);
            if(devParms->modelSerie != 1) {
                if(writeRead(":BUS1:SPI:MOSI:THR?", 19)) return __LINE__;
            } else if(writeRead(":DEC1:THRE:CHAN2?", 17)) return __LINE__;
            devParms->chan[1].mathDecodeThreshold = atof(device->buf);

            if(devParms->channelCnt == 4) {
                usleep(TMC_GDS_DELAY);
                if(devParms->modelSerie != 1) {
                    if(writeRead(":BUS1:SPI:SCLK:THR?", 19)) return __LINE__;
                } else if(writeRead(":DEC1:THRE:CHAN3?", 17)) return __LINE__;
                devParms->chan[2].mathDecodeThreshold = atof(device->buf);

                usleep(TMC_GDS_DELAY);
                if(devParms->modelSerie != 1) {
                    if(writeRead(":BUS1:SPI:SS:THR?", 17)) return __LINE__;
                } else if(writeRead(":DEC1:THRE:CHAN4?", 17)) return __LINE__;
                devParms->chan[3].mathDecodeThreshold = atof(device->buf);
            }

            if(devParms->modelSerie != 1) {
                usleep(TMC_GDS_DELAY);
                if(writeRead(":BUS1:RS232:TTHR?", 17)) return __LINE__;
                //    devParms->math_decode_threshold_uart_tx = atof(device->buf);
                devParms->mathDecodeThresholdUartTx = atof(device->buf) * 10.0; // hack for firmware bug!

                usleep(TMC_GDS_DELAY);
                if(writeRead(":BUS1:RS232:RTHR?", 17)) return __LINE__;
                //    devParms->math_decode_threshold_uart_rx = atof(device->buf);
                devParms->mathDecodeThresholdUartRx = atof(device->buf) * 10.0; // hack for firmware bug!
            }

            if(devParms->modelSerie == 1) {
                usleep(TMC_GDS_DELAY);
                if(writeRead(":DEC1:THRE:AUTO?", 16)) return __LINE__;
                devParms->mathDecodeThresholdAuto = atoi(device->buf);
            }

            usleep(TMC_GDS_DELAY);
            constexpr Map decoder{
                Pair{"CHAN1"sv, CH_AN_1},
                Pair{"CHAN2"sv, CH_AN_2},
                Pair{"CHAN3"sv, CH_AN_3},
                Pair{"CHAN4"sv, CH_AN_4},
                Pair{"OFF"sv,   OFF    },
            };

            if(devParms->modelSerie != 1) {
                if(writeRead(":BUS1:RS232:RX?", 15)) return __LINE__;
            } else if(writeRead(":DEC1:UART:RX?", 14)) return __LINE__;
            devParms->mathDecodeUartRx = decoder[device->buf];

            usleep(TMC_GDS_DELAY);
            if(devParms->modelSerie != 1) {
                if(writeRead(":BUS1:RS232:TX?", 15)) return __LINE__;
            } else if(writeRead(":DEC1:UART:TX?", 14)) return __LINE__;
            devParms->mathDecodeUartTx = decoder[device->buf];

            usleep(TMC_GDS_DELAY);
            if(devParms->modelSerie != 1) {
                if(writeRead(":BUS1:RS232:POL?", 16)) return __LINE__;
            } else if(writeRead(":DEC1:UART:POL?", 15)) return __LINE__;
            devParms->mathDecodeUartPol = Map{
                Pair{"POS"sv, 1},
                Pair{"NEG"sv, 0}
            }[device->buf];

            usleep(TMC_GDS_DELAY);
            if(devParms->modelSerie != 1) {
                if(writeRead(":BUS1:RS232:END?", 16)) return __LINE__;
            } else if(writeRead(":DEC1:UART:END?", 15)) return __LINE__;
            devParms->mathDecodeUartPol = Map{
                Pair{"MSB"sv, 1},
                Pair{"LSB"sv, 0}
            }[device->buf];

            usleep(TMC_GDS_DELAY);
            if(devParms->modelSerie != 1) {
                if(writeRead(":BUS1:RS232:BAUD?", 17)) return __LINE__;
            } else if(writeRead(":DEC1:UART:BAUD?", 16)) return __LINE__;
            // FIXME  DEC1:UART:BAUD? can return also "USER" instead of a number!
            devParms->mathDecodeUartBaud = atoi(device->buf);

            usleep(TMC_GDS_DELAY);
            if(devParms->modelSerie != 1) {
                if(writeRead(":BUS1:RS232:DBIT?", 17)) return __LINE__;
            } else if(writeRead(":DEC1:UART:WIDT?", 16)) return __LINE__;
            devParms->mathDecodeUartWidth = atoi(device->buf);

            usleep(TMC_GDS_DELAY);
            if(devParms->modelSerie != 1) {
                if(writeRead(":BUS1:RS232:SBIT?", 17)) return __LINE__;
            } else if(writeRead(":DEC1:UART:STOP?", 16)) return __LINE__;
            devParms->mathDecodeUartStop = Map{
                Pair{"1"sv,   0},
                Pair{"1.5"sv, 1},
                Pair{"2"sv,   2}
            }[device->buf];

            usleep(TMC_GDS_DELAY);

            if(devParms->modelSerie != 1) {
                if(writeRead(":BUS1:RS232:PAR?", 16)) return __LINE__;
            } else if(writeRead(":DEC1:UART:PAR?", 15)) return __LINE__;
            devParms->mathDecodeUartPar = Map{
                Pair{"ODD"sv,  1},
                Pair{"EVEN"sv, 2},
                Pair{"NONE"sv, 0}
            }[device->buf];

            usleep(TMC_GDS_DELAY);

            if(devParms->modelSerie != 1) {
                if(writeRead(":BUS1:SPI:SCLK:SOUR?", 20)) return __LINE__;
            } else if(writeRead(":DEC1:SPI:CLK?", 14)) return __LINE__;
            devParms->mathDecodeSpiClk = decoder[device->buf];

            usleep(TMC_GDS_DELAY);
            if(devParms->modelSerie != 1) {
                if(writeRead(":BUS1:SPI:MISO:SOUR?", 20)) return __LINE__;
            } else if(writeRead(":DEC1:SPI:MISO?", 15)) return __LINE__;
            devParms->mathDecodeSpiMiso = decoder[device->buf];

            usleep(TMC_GDS_DELAY);
            if(devParms->modelSerie != 1) {
                if(writeRead(":BUS1:SPI:MOSI:SOUR?", 20)) return __LINE__;
            } else if(writeRead(":DEC1:SPI:MOSI?", 15)) return __LINE__;
            devParms->mathDecodeSpiMosi = decoder[device->buf];

            usleep(TMC_GDS_DELAY);
            if(devParms->modelSerie != 1) {
                if(writeRead(":BUS1:SPI:SS:SOUR?", 18)) return __LINE__;
            } else if(writeRead(":DEC1:SPI:CS?", 13)) return __LINE__;
            devParms->mathDecodeSpiCs = decoder[device->buf];

            usleep(TMC_GDS_DELAY);
            if(devParms->modelSerie != 1) {
                if(writeRead(":BUS1:SPI:SS:POL?", 17)) return __LINE__;
            } else if(writeRead(":DEC1:SPI:SEL?", 14)) return __LINE__;
            devParms->mathDecodeSpiSelect = Map{
                Pair{"NCS"sv, 0},
                Pair{"CS"sv,  1},
                Pair{"NEG"sv, 0},
                Pair{"POS"sv, 1}
            }[device->buf];

            if(devParms->modelSerie == 1) {
                usleep(TMC_GDS_DELAY);
                if(writeRead(":DEC1:SPI:MODE?", 15)) return __LINE__;
                devParms->mathDecodeSpiMode = Map{
                    Pair{"TIM"sv, 0},
                    Pair{"CS"sv,  1},
                }[device->buf];
            }

            if(devParms->modelSerie == 1) {
                usleep(TMC_GDS_DELAY);
                if(writeRead(":DEC1:SPI:TIM?", 14)) return __LINE__;
                devParms->mathDecodeSpiTimeout = atof(device->buf);
            }

            usleep(TMC_GDS_DELAY);

            if(devParms->modelSerie != 1) {
                if(writeRead(":BUS1:SPI:MOSI:POL?", 19)) return __LINE__;
            } else if(writeRead(":DEC1:SPI:POL?", 14)) return __LINE__;
            devParms->mathDecodeSpiPol = Map{
                Pair{"POS"sv, 1},
                Pair{"NEG"sv, 0}
            }[device->buf];

            usleep(TMC_GDS_DELAY);
            if(devParms->modelSerie != 1) {
                if(writeRead(":BUS1:SPI:SCLK:SLOP?", 20)) return __LINE__;
            } else if(writeRead(":DEC1:SPI:EDGE?", 15)) return __LINE__;
            devParms->mathDecodeSpiEdge = Map{
                Pair{"NEG"sv,  0},
                Pair{"POS"sv,  1},
                Pair{"FALL"sv, 0},
                Pair{"RISE"sv, 1}
            }[device->buf];

            usleep(TMC_GDS_DELAY);
            if(devParms->modelSerie != 1) {
                if(writeRead(":BUS1:SPI:DBIT?", 15)) return __LINE__;
            } else if(writeRead(":DEC1:SPI:WIDT?", 15)) return __LINE__;
            devParms->mathDecodeSpiWidth = atoi(device->buf);

            usleep(TMC_GDS_DELAY);
            if(devParms->modelSerie != 1) {
                if(writeRead(":BUS1:SPI:END?", 14)) return __LINE__;
            } else if(writeRead(":DEC1:SPI:END?", 14)) return __LINE__;
            devParms->mathDecodeSpiEnd = Map{
                Pair{"LSB"sv, 0},
                Pair{"MSB"sv, 1}
            }[device->buf];

            usleep(TMC_GDS_DELAY);
            if(devParms->modelSerie == 1) {
                if(writeRead(":FUNC:WREC:ENAB?", 16)) return __LINE__;
                constexpr Map wrec{
                    Pair{"0"sv, 0},
                    Pair{"1"sv, 1},
                };
                devParms->funcWrecEnable = wrec[device->buf];
            } else {
                if(writeRead(":FUNC:WRM?", 10)) return __LINE__;
                devParms->funcWrecEnable = Map{
                    Pair{"REC"sv,  1},
                    Pair{"PLAY"sv, 2},
                    Pair{"OFF"sv,  0}
                }.at(device->buf);
            }

            if(devParms->funcWrecEnable) {
                usleep(TMC_GDS_DELAY);
                if(writeRead(":FUNC:WREC:FEND?", 16)) return __LINE__;
                devParms->funcWrecFend = atoi(device->buf);

                usleep(TMC_GDS_DELAY);
                if(writeRead(":FUNC:WREC:FMAX?", 16)) return __LINE__;
                devParms->funcWrecFmax = atoi(device->buf);

                usleep(TMC_GDS_DELAY);
                if(writeRead(":FUNC:WREC:FINT?", 16)) return __LINE__;
                devParms->funcWrecFintval = atof(device->buf);

                usleep(TMC_GDS_DELAY);
                if(writeRead(":FUNC:WREP:FST?", 15)) return __LINE__;
                devParms->funcWplayFstart = atoi(device->buf);

                usleep(TMC_GDS_DELAY);
                if(writeRead(":FUNC:WREP:FEND?", 16)) return __LINE__;
                devParms->funcWplayFend = atoi(device->buf);

                usleep(TMC_GDS_DELAY);
                if(writeRead(":FUNC:WREP:FMAX?", 16)) return __LINE__;
                devParms->funcWplayFmax = atoi(device->buf);

                usleep(TMC_GDS_DELAY);
                if(writeRead(":FUNC:WREP:FINT?", 16)) return __LINE__;
                devParms->funcWplayFintval = atof(device->buf);

                usleep(TMC_GDS_DELAY);
                if(writeRead(":FUNC:WREP:FCUR?", 16)) return __LINE__;
                devParms->funcWplayFcur = atoi(device->buf);
            }

            usleep(TMC_GDS_DELAY);
            if(writeRead(":LA:STATe?", 10)) return __LINE__;
            devParms->la.STATe = atoi(device->buf);

            if(devParms->la.STATe) {
                usleep(TMC_GDS_DELAY);
                if(writeRead(":LA:ACTive?", 11)) return __LINE__;
                constexpr Map active{
                    Pair{"D0"sv,     int(LA::D0)    },
                    Pair{"D1"sv,     int(LA::D1)    },
                    Pair{"D2"sv,     int(LA::D2)    },
                    Pair{"D3"sv,     int(LA::D3)    },
                    Pair{"D4"sv,     int(LA::D4)    },
                    Pair{"D5"sv,     int(LA::D5)    },
                    Pair{"D6"sv,     int(LA::D6)    },
                    Pair{"D7"sv,     int(LA::D7)    },
                    Pair{"D8"sv,     int(LA::D8)    },
                    Pair{"D9"sv,     int(LA::D9)    },
                    Pair{"D10"sv,    int(LA::D10)   },
                    Pair{"D11"sv,    int(LA::D11)   },
                    Pair{"D12"sv,    int(LA::D12)   },
                    Pair{"D13"sv,    int(LA::D13)   },
                    Pair{"D14"sv,    int(LA::D14)   },
                    Pair{"D15"sv,    int(LA::D15)   },
                    Pair{"GROUP1"sv, int(LA::GROUP1)},
                    Pair{"GROUP2"sv, int(LA::GROUP2)},
                    Pair{"GROUP3"sv, int(LA::GROUP3)},
                    Pair{"GROUP4"sv, int(LA::GROUP4)}
                };
                devParms->la.ACTive = active[device->buf];

                usleep(TMC_GDS_DELAY);
                // if(writeRead(":LA:DISPlay?",12) ) return __LINE__;
                //                 // devParms->la.DISPlay;

                usleep(TMC_GDS_DELAY);
                if(writeRead(":LA:SIZE?", 9)) return __LINE__;
                constexpr Map size{
                    Pair{"LARGe"sv, LA::Size::LARGe},
                    Pair{"SMALl"sv, LA::Size::SMALl},
                };
                devParms->la.SIZE = size[device->buf];

                usleep(TMC_GDS_DELAY);
                if(writeRead(":LA:TCALibrate?", 15)) return __LINE__;
                devParms->la.TCALibrate = atof(device->buf);

                for(int digCh = 0; digCh < 16; ++digCh) {
                    usleep(TMC_GDS_DELAY);
                    int len = snprintf(str, 512, ":LA:DIGital%i:DISPlay?", digCh);
                    str[len] = 0;
                    if(writeRead(str, len)) return __LINE__;
                    devParms->la.DIGital[digCh].DISPlay = atoi(device->buf);

                    usleep(TMC_GDS_DELAY);
                    len = snprintf(str, 512, ":LA:DIGital%i:POSition?", digCh);
                    str[len] = 0;
                    if(writeRead(str, len)) return __LINE__;
                    devParms->la.DIGital[digCh].POSition = atoi(device->buf);

                    usleep(TMC_GDS_DELAY);
                    len = snprintf(str, 512, ":LA:DIGital%i:LABel?", digCh);
                    str[len] = 0;
                    if(writeRead(str, len)) return __LINE__;
                    strncat(devParms->la.DIGital[digCh].LABel, device->buf, 4);
                }

                usleep(TMC_GDS_DELAY);
                if(writeRead(":LA:POD1:THReshold?", 19)) return __LINE__;
                devParms->la.PODTHReshold[0] = atof(device->buf);
                usleep(TMC_GDS_DELAY);

                if(writeRead(":LA:POD2:THReshold?", 19)) return __LINE__;
                devParms->la.PODTHReshold[1] = atof(device->buf);
                usleep(TMC_GDS_DELAY);

                // :LA:POD<n>:DISPlay
            }

            errNum = 0;
            return 0;
        }();
    } catch(const std::exception* ex) {
        strncat(device->buf, ex->what(), 512);
        errLine = __LINE__;
    }

    if(errLine > 0) {
        char str[512]{};

        snprintf(errStr,
            4096,
            "An error occurred while reading settings from device.\n"
            "Command sent: %s\n"
            "Received: %s\n"
            "File %s line %i",
            str, device->buf, __FILE__, errLine);
        errNum = -1;
    }
}
