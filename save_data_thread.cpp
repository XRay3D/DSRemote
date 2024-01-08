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

#include "save_data_thread.h"

SaveDataThread::SaveDataThread(int job_s)
    : job{job_s}
    , hdl{-1}
    , err_num{-1}
    , err_str{0}
    , n_bytes_rcvd{-1}
    , devParms{nullptr}
    , datrecs{0}
    , smps_per_record{0} {
}

int SaveDataThread::get_error_num(void) {
    return err_num;
}

void SaveDataThread::get_error_str(char* dest, int sz) {
    strlcpy(dest, err_str, sz);
}

int SaveDataThread::get_num_bytes_rcvd(void) {
    return n_bytes_rcvd;
}

void SaveDataThread::run() {
    err_str[0] = 0;

    switch(job) {
    case 0:
        read_data();
        break;
    case 1:
        save_memory_edf_file();
        break;
    default:
        err_num = -4;
        break;
    }
}

void SaveDataThread::read_data(void) {
    msleep(100);
    n_bytes_rcvd = tmcRead();
    err_num = 0;
}

void SaveDataThread::init_save_memory_edf_file(
    struct DeviceSettings* devp, int hdl_s, int records, int smpls, short** wav) {
    datrecs = records;
    devParms = devp;
    smps_per_record = smpls;
    wavBuf = wav;
    hdl = hdl_s;
}

void SaveDataThread::save_memory_edf_file(void) {
    if(devParms == nullptr) {
        strlcpy(err_str, "save_memory_edf_file(): Invalid devParms pointer.", 4096);
        err_num = 1;
        msleep(200);
        return;
    }

    if(hdl < 0) {
        strlcpy(err_str, "save_memory_edf_file(): Invalid handel.", 4096);
        err_num = 2;
        msleep(200);
        return;
    }

    msleep(100);

    for(int i{}; i < datrecs; i++) {
        for(int chn{}; chn < MAX_CHNS; chn++) {
            if(!devParms->chan[chn].Display)
                continue;

            if(edfwrite_digital_short_samples(hdl, wavBuf[chn] + (i * smps_per_record))) {
                strlcpy(err_str, "A file write error occurred.", 4096);
                err_num = 3;
                return;
            }
        }
    }

    err_num = 0;
}
