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

#define SAV_MEM_BSZ (250000)

void UiMainWindow::saveScreenshot() {
    int n;

    char str[512], opath[MAX_PATHLEN];

    QPainter painter;

    QPainterPath path;

    if(device == nullptr)
        return;

    scrnTimer->stop();

    scrnThread->wait();

    tmcWrite(":DISP:DATA?");

    SaveDataThread get_data_thrd(0);

    QMessageBox w_msg_box;
    w_msg_box.setIcon(QMessageBox::NoIcon);
    w_msg_box.setText("Downloading data...");
    w_msg_box.setStandardButtons(QMessageBox::Abort);

    connect(&get_data_thrd, SIGNAL(finished()), &w_msg_box, SLOT(accept()));

    get_data_thrd.start();

    if(w_msg_box.exec() != QDialog::Accepted) {
        disconnect(&get_data_thrd, 0, 0, 0);
        strlcpy(str, "Aborted by user.", 512);
        goto OUT_ERROR;
    }

    disconnect(&get_data_thrd, 0, 0, 0);

    n = get_data_thrd.get_num_bytes_rcvd();
    if(n < 0) {
        strlcpy(str, "Can not read from device.", 512);
        goto OUT_ERROR;
    }

    if(device->sz != SCRN_SHOT_BMP_SZ) {
        strlcpy(str, "Error, bitmap has wrong filesize\n", 512);
        goto OUT_ERROR;
    }

    if(strncmp(device->buf, "BM", 2)) {
        strlcpy(str, "Error, file is not a bitmap\n", 512);
        goto OUT_ERROR;
    }

    memcpy(devParms.screenshotBuf, device->buf, SCRN_SHOT_BMP_SZ);

    screenXpm.loadFromData((uchar*)(devParms.screenshotBuf), SCRN_SHOT_BMP_SZ);

    if(devParms.modelSerie == 1) {
        painter.begin(&screenXpm);
        painter.fillRect(0, 0, 80, 29, Qt::black);
        painter.setPen(Qt::white);
        painter.drawText(5, 8, 65, 20, Qt::AlignCenter, devParms.modelName);
        painter.end();
    } else if(devParms.modelSerie == 6) {
        painter.begin(&screenXpm);
        painter.fillRect(0, 0, 95, 29, QColor(48, 48, 48));
        path.addRoundedRect(5, 5, 85, 20, 3, 3);
        painter.fillPath(path, Qt::black);
        painter.setPen(Qt::white);
        painter.drawText(5, 5, 85, 20, Qt::AlignCenter, devParms.modelName);
        painter.end();
    }

    if(devParms.screenshotInv)
        screenXpm.invertPixels(QImage::InvertRgb);

    opath[0] = 0;
    if(recentSaveDir[0] != 0) {
        strlcpy(opath, recentSaveDir, MAX_PATHLEN);
        strlcat(opath, "/", MAX_PATHLEN);
    }
    strlcat(opath, "screenshot.png", MAX_PATHLEN);

    strlcpy(opath,
        QFileDialog::getSaveFileName(this, "Save file", opath, "PNG files (*.png *.PNG)")
            .toLocal8Bit()
            .data(),
        MAX_PATHLEN);

    if(!strcmp(opath, "")) {
        scrnTimer->start(devParms.screenTimerIval);

        return;
    }

    getDirectoryFromPath(recentSaveDir, opath, MAX_PATHLEN);

    if(screenXpm.save(QString::fromLocal8Bit(opath), "PNG", 50) == false) {
        strlcpy(str, "Could not save file (unknown error)", 512);
        goto OUT_ERROR;
    }

    scrnTimer->start(devParms.screenTimerIval);

    return;

OUT_ERROR:

    if(get_data_thrd.isFinished() != true) {
        connect(&get_data_thrd, SIGNAL(finished()), &w_msg_box, SLOT(accept()));
        w_msg_box.setText("Waiting for thread to finish, please wait...");
        w_msg_box.exec();
    }

    scrnTimer->start(devParms.screenTimerIval);

    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText(str);
    msgBox.exec();
}

void UiMainWindow::getDeepMemoryWaveform(void) {
    int i, k, n = 0, chn, chns = 0, bytes_rcvd = 0, mempnts, yref[MAX_CHNS], empty_Buf;

    char str[512];

    short* wavBuf[MAX_CHNS];

    QEventLoop ev_loop;

    QMessageBox wi_msg_box;

    SaveDataThread get_data_thrd(0);

    if(device == nullptr)
        return;

    scrnTimer->stop();

    scrnThread->wait();

    for(int i{}; i < MAX_CHNS; i++)
        wavBuf[i] = nullptr;

    mempnts = devParms.acquirememdepth;

    QProgressDialog progress("Downloading data...", "Abort", 0, mempnts, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);

    connect(&get_data_thrd, SIGNAL(finished()), &ev_loop, SLOT(quit()));

    statusLabel->setText("Downloading data...");

    for(int i{}; i < MAX_CHNS; i++) {
        if(!devParms.chan[i].Display)
            continue;

        chns++;
    }

    if(!chns) {
        strlcpy(str, "No active channels.", 512);
        goto OUT_ERROR;
    }

    if(mempnts < 1) {
        strlcpy(str, "Can not download waveform when memory depth is set to \"Auto\".", 512);
        goto OUT_ERROR;
    }

    for(int i{}; i < MAX_CHNS; i++) {
        if(!devParms.chan[i].Display) // Download data only when channel is switched on
            continue;

        wavBuf[i] = (short*)malloc(mempnts * sizeof(short));
        if(wavBuf[i] == nullptr) {
            snprintf(str, 512, "Malloc error.  line %i file %s", __LINE__, __FILE__);
            goto OUT_ERROR;
        }
    }

    tmcWrite(":STOP");

    usleep(20000);

    for(chn = 0; chn < MAX_CHNS; chn++) {
        if(!devParms.chan[chn].Display) // Download data only when channel is switched on
            continue;

        snprintf(str, 512, "Downloading channel %i waveform data...", chn + 1);
        progress.setLabelText(str);

        snprintf(str, 512, ":WAV:SOUR CHAN%i", chn + 1);

        tmcWrite(str);

        tmcWrite(":WAV:FORM BYTE");

        usleep(20000);

        tmcWrite(":WAV:MODE RAW");

        usleep(20000);

        tmcWrite(":WAV:YINC?");

        usleep(20000);

        tmcRead();

        devParms.chan[chn].yinc = atof(device->buf);

        if(devParms.chan[chn].yinc < 1e-6) {
            snprintf(str,
                512,
                "Error, parameter \"YINC\" out of range: %e  line %i file %s",
                devParms.chan[chn].yinc,
                __LINE__,
                __FILE__);
            goto OUT_ERROR;
        }

        usleep(20000);

        tmcWrite(":WAV:YREF?");

        usleep(20000);

        tmcRead();

        yref[chn] = atoi(device->buf);

        if((yref[chn] < 1) || (yref[chn] > 255)) {
            snprintf(str,
                512,
                "Error, parameter \"YREF\" out of range: %i  line %i file %s",
                yref[chn],
                __LINE__,
                __FILE__);
            goto OUT_ERROR;
        }

        usleep(20000);

        tmcWrite(":WAV:YOR?");

        usleep(20000);

        tmcRead();

        devParms.chan[chn].yor = atoi(device->buf);

        if((devParms.chan[chn].yor < -255) || (devParms.chan[chn].yor > 255)) {
            snprintf(str,
                512,
                "Error, parameter \"YOR\" out of range: %i  line %i file %s",
                devParms.chan[chn].yor,
                __LINE__,
                __FILE__);
            goto OUT_ERROR;
        }

        empty_Buf = 0;

        for(bytes_rcvd = 0; bytes_rcvd < mempnts;) {
            progress.setValue(bytes_rcvd);

            if(progress.wasCanceled()) {
                strlcpy(str, "Canceled", 512);
                goto OUT_ERROR;
            }

            snprintf(str, 512, ":WAV:STAR %i", bytes_rcvd + 1);

            usleep(20000);

            tmcWrite(str);

            if((bytes_rcvd + SAV_MEM_BSZ) > mempnts)
                snprintf(str, 512, ":WAV:STOP %i", mempnts);
            else
                snprintf(str, 512, ":WAV:STOP %i", bytes_rcvd + SAV_MEM_BSZ);

            usleep(20000);

            tmcWrite(str);

            usleep(20000);

            tmcWrite(":WAV:DATA?");

            get_data_thrd.start();

            ev_loop.exec();

            n = get_data_thrd.get_num_bytes_rcvd();
            if(n < 0) {
                snprintf(str, 512, "Can not read from device.  line %i file %s", __LINE__, __FILE__);
                goto OUT_ERROR;
            }

            if(n == 0) {
                snprintf(str, 512, "No waveform data available.");
                goto OUT_ERROR;
            }

            printf("received %i bytes, total %i bytes\n", n, n + bytes_rcvd);

            if(n > SAV_MEM_BSZ) {
                snprintf(str,
                    512,
                    "Datablock too big for buffer: %i  line %i file %s",
                    n,
                    __LINE__,
                    __FILE__);
                goto OUT_ERROR;
            }

            if(n < 1) {
                if(empty_Buf++ > 100)
                    break;
            } else {
                empty_Buf = 0;
            }

            for(k = 0; k < n; k++) {
                if((bytes_rcvd + k) >= mempnts)
                    break;

                wavBuf[chn][bytes_rcvd + k]
                    = ((int)(((unsigned char*)device->buf)[k]) - yref[chn] - devParms.chan[chn].yor)
                    << 5;
            }

            bytes_rcvd += n;

            if(bytes_rcvd >= mempnts)
                break;
        }

        if(bytes_rcvd < mempnts) {
            snprintf(str, 512, "Download error.  line %i file %s", __LINE__, __FILE__);
            goto OUT_ERROR;
        }
    }

    progress.reset();

    for(chn = 0; chn < MAX_CHNS; chn++) {
        if(!devParms.chan[chn].Display)
            continue;

        snprintf(str, 512, ":WAV:SOUR CHAN%i", chn + 1);

        usleep(20000);

        tmcWrite(str);

        usleep(20000);

        tmcWrite(":WAV:MODE NORM");

        usleep(20000);

        tmcWrite(":WAV:STAR 1");

        if(devParms.modelSerie == 1) {
            usleep(20000);

            tmcWrite(":WAV:STOP 1200");
        } else {
            usleep(20000);

            tmcWrite(":WAV:STOP 1400");

            usleep(20000);

            tmcWrite(":WAV:POIN 1400");
        }
    }

    if(bytes_rcvd < mempnts) {
        snprintf(str, 512, "Download error.  line %i file %s", __LINE__, __FILE__);
        goto OUT_ERROR;
    } else {
        statusLabel->setText("Downloading finished");
    }

    new UiWaveWindow(&devParms, wavBuf, this);

    disconnect(&get_data_thrd, 0, 0, 0);

    scrnTimer->start(devParms.screenTimerIval);

    return;

OUT_ERROR:

    disconnect(&get_data_thrd, 0, 0, 0);

    progress.reset();

    statusLabel->setText("Downloading aborted");

    if(get_data_thrd.isRunning() == true) {
        QMessageBox w_msg_box;
        w_msg_box.setIcon(QMessageBox::NoIcon);
        w_msg_box.setText("Waiting for thread to finish, please wait...");
        w_msg_box.setStandardButtons(QMessageBox::Abort);

        connect(&get_data_thrd, SIGNAL(finished()), &w_msg_box, SLOT(accept()));

        w_msg_box.exec();
    }

    if(progress.wasCanceled() == false) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(str);
        msgBox.exec();
    }

    for(chn = 0; chn < MAX_CHNS; chn++) {
        if(!devParms.chan[chn].Display)
            continue;

        snprintf(str, 512, ":WAV:SOUR CHAN%i", chn + 1);

        tmcWrite(str);

        tmcWrite(":WAV:MODE NORM");

        tmcWrite(":WAV:STAR 1");

        if(devParms.modelSerie == 1) {
            tmcWrite(":WAV:STOP 1200");
        } else {
            tmcWrite(":WAV:STOP 1400");

            tmcWrite(":WAV:POIN 1400");
        }
    }

    for(chn = 0; chn < MAX_CHNS; chn++) {
        free(wavBuf[chn]);
        wavBuf[chn] = nullptr;
    }

    scrnTimer->start(devParms.screenTimerIval);
}

void UiMainWindow::saveWaveInspectorBufferToEdf(struct DeviceSettings* d_parms) {
    int i, j, chn, chns = 0, hdl = -1, mempnts, smps_per_record, datrecs = 1, ret_stat;

    char str[512], opath[MAX_PATHLEN];

    long long rec_len = 0LL, datrecduration;

    QMessageBox wi_msg_box;

    SaveDataThread sav_data_thrd(1);

    mempnts = d_parms->acquirememdepth;

    smps_per_record = mempnts;

    for(int i{}; i < MAX_CHNS; i++) {
        if(!d_parms->chan[i].Display)
            continue;

        chns++;
    }

    if(!chns) {
        strlcpy(str, "No active channels.", 512);
        goto OUT_ERROR;
    }

    while(smps_per_record >= (5000000 / chns)) {
        smps_per_record /= 2;

        datrecs *= 2;
    }

    rec_len = (EDFLIB_TIME_DIMENSION * (long long)mempnts) / d_parms->samplerate;

    if(rec_len < 100) {
        strlcpy(str,
            "Can not save waveforms shorter than 10 uSec.\n"
            "Select a higher memory depth or a higher timebase.",
            512);
        goto OUT_ERROR;
    }

    opath[0] = 0;
    if(recentSaveDir[0] != 0) {
        strlcpy(opath, recentSaveDir, MAX_PATHLEN);
        strlcat(opath, "/", MAX_PATHLEN);
    }
    strlcat(opath, "waveform.edf", MAX_PATHLEN);

    strlcpy(opath,
        QFileDialog::getSaveFileName(this, "Save file", opath, "EDF files (*.edf *.EDF)")
            .toLocal8Bit()
            .data(),
        MAX_PATHLEN);

    if(!strcmp(opath, ""))
        goto OUT_NORMAL;

    getDirectoryFromPath(recentSaveDir, opath, MAX_PATHLEN);

    hdl = edfopen_file_writeonly(opath, EDFLIB_FILETYPE_EDFPLUS, chns);
    if(hdl < 0) {
        strlcpy(str, "Can not create EDF file.", 512);
        goto OUT_ERROR;
    }

    statusLabel->setText("Saving EDF file...");

    datrecduration = (rec_len / 10LL) / datrecs;

    //  printf("rec_len: %lli   datrecs: %i   datrecduration: %lli\n", rec_len, datrecs, datrecduration);

    if(datrecduration < 10000LL) {
        if(edf_set_micro_datarecord_duration(hdl, datrecduration)) {
            snprintf(str, 512, "Can not set datarecord duration of EDF file: %lli", datrecduration);
            printf("\ndebug line %i: rec_len: %lli   datrecs: %i   datrecduration: %lli\n",
                __LINE__,
                rec_len,
                datrecs,
                datrecduration);
            goto OUT_ERROR;
        }
    } else if(edf_set_datarecord_duration(hdl, datrecduration / 10LL)) {
        snprintf(str, 512, "Can not set datarecord duration of EDF file: %lli", datrecduration);
        printf("\ndebug line %i: rec_len: %lli   datrecs: %i   datrecduration: %lli\n",
            __LINE__,
            rec_len,
            datrecs,
            datrecduration);
        goto OUT_ERROR;
    }

    j = 0;

    for(chn = 0; chn < MAX_CHNS; chn++) {
        if(!d_parms->chan[chn].Display)
            continue;

        edf_set_samplefrequency(hdl, j, smps_per_record);
        edf_set_digital_maximum(hdl, j, 32767);
        edf_set_digital_minimum(hdl, j, -32768);
        if(d_parms->chan[chn].scale > 2) {
            edf_set_physical_maximum(hdl, j, d_parms->chan[chn].yinc * 32767.0 / 32.0);
            edf_set_physical_minimum(hdl, j, d_parms->chan[chn].yinc * -32768.0 / 32.0);
            edf_set_physical_dimension(hdl, j, "V");
        } else {
            edf_set_physical_maximum(hdl, j, 1000.0 * d_parms->chan[chn].yinc * 32767.0 / 32.0);
            edf_set_physical_minimum(hdl, j, 1000.0 * d_parms->chan[chn].yinc * -32768.0 / 32.0);
            edf_set_physical_dimension(hdl, j, "mV");
        }
        snprintf(str, 512, "CHAN%i", chn + 1);
        edf_set_label(hdl, j, str);

        j++;
    }

    edf_set_equipment(hdl, d_parms->modelName);

    //  printf("datrecs: %i    smps_per_record: %i\n", datrecs, smps_per_record);

    sav_data_thrd.init_save_memory_edf_file(d_parms, hdl, datrecs, smps_per_record, d_parms->waveBuf);

    wi_msg_box.setIcon(QMessageBox::NoIcon);
    wi_msg_box.setText("Saving EDF file ...");
    wi_msg_box.setStandardButtons(QMessageBox::Abort);

    connect(&sav_data_thrd, SIGNAL(finished()), &wi_msg_box, SLOT(accept()));

    sav_data_thrd.start();

    ret_stat = wi_msg_box.exec();

    if(ret_stat != QDialog::Accepted) {
        strlcpy(str, "Saving EDF file aborted.", 512);
        goto OUT_ERROR;
    }

    if(sav_data_thrd.get_error_num()) {
        sav_data_thrd.get_error_str(str, 512);
        goto OUT_ERROR;
    }

OUT_NORMAL:

    disconnect(&sav_data_thrd, 0, 0, 0);

    if(hdl >= 0)
        edfclose_file(hdl);

    if(!strcmp(opath, ""))
        statusLabel->setText("Save file canceled.");
    else
        statusLabel->setText("Saved memory buffer to EDF file.");

    return;

OUT_ERROR:

    disconnect(&sav_data_thrd, 0, 0, 0);

    statusLabel->setText("Saving file aborted.");

    if(hdl >= 0)
        edfclose_file(hdl);

    wi_msg_box.setIcon(QMessageBox::Critical);
    wi_msg_box.setText(str);
    wi_msg_box.setStandardButtons(QMessageBox::Ok);
    wi_msg_box.exec();
}

//     tmc_write(":WAV:PRE?");
//
//     n = tmc_read();
//
//     if(n < 0)
//     {
//       strlcpy(str, "Can not read from device.");
//       goto OUT_ERROR;
//     }

//     printf("waveform preamble: %s\n", device->buf);

//     if(parse_preamble(device->buf, device->sz, &wfp, i))
//     {
//       strlcpy(str, "Preamble parsing error.");
//       goto OUT_ERROR;
//     }

//     printf("waveform preamble:\n"
//            "format: %i\n"
//            "type: %i\n"
//            "points: %i\n"
//            "count: %i\n"
//            "xincrement: %e\n"
//            "xorigin: %e\n"
//            "xreference: %e\n"
//            "yincrement: %e\n"
//            "yorigin: %e\n"
//            "yreference: %e\n",
//            wfp.format, wfp.type, wfp.points, wfp.count,
//            wfp.xincrement[i], wfp.xorigin[i], wfp.xreference[i],
//            wfp.yincrement[i], wfp.yorigin[i], wfp.yreference[i]);

//     rec_len = wfp.xincrement[i] * wfp.points;

void UiMainWindow::saveScreenWaveform() {
    int i, j, n = 0, chn, chns = 0, hdl = -1, yref[MAX_CHNS];

    char str[512], opath[MAX_PATHLEN];

    short* wavBuf[MAX_CHNS];

    long long rec_len = 0LL;

    if(device == nullptr)
        return;

    for(int i{}; i < MAX_CHNS; i++)
        wavBuf[i] = nullptr;

    SaveDataThread get_data_thrd(0);

    QMessageBox w_msg_box;
    w_msg_box.setIcon(QMessageBox::NoIcon);
    w_msg_box.setText("Downloading data...");
    w_msg_box.setStandardButtons(QMessageBox::Abort);

    scrnTimer->stop();

    scrnThread->wait();

    if(devParms.timebasedelayenable)
        rec_len = EDFLIB_TIME_DIMENSION * devParms.timebasedelayscale * devParms.horDivisions;
    else
        rec_len = EDFLIB_TIME_DIMENSION * devParms.timebasescale * devParms.horDivisions;

    if(rec_len < 10LL) {
        strlcpy(str, "Can not save waveforms when timebase < 1uSec.", 512);
        goto OUT_ERROR;
    }

    for(chn = 0; chn < MAX_CHNS; chn++) {
        if(!devParms.chan[chn].Display) // Download data only when channel is switched on
            continue;

        wavBuf[chn] = (short*)malloc(WAVFRM_MAX_BUFSZ * sizeof(short));
        if(wavBuf[chn] == nullptr) {
            strlcpy(str, "Malloc error.", 512);
            goto OUT_ERROR;
        }

        chns++;
    }

    if(!chns) {
        strlcpy(str, "No active channels.", 512);
        goto OUT_ERROR;
    }

    for(chn = 0; chn < MAX_CHNS; chn++) {
        if(!devParms.chan[chn].Display) // Download data only when channel is switched on
            continue;

        usleep(20000);

        snprintf(str, 512, ":WAV:SOUR CHAN%i", chn + 1);

        tmcWrite(str);

        usleep(20000);

        tmcWrite(":WAV:FORM BYTE");

        usleep(20000);

        tmcWrite(":WAV:MODE NORM");

        usleep(20000);

        tmcWrite(":WAV:YINC?");

        usleep(20000);

        tmcRead();

        devParms.chan[chn].yinc = atof(device->buf);

        if(devParms.chan[chn].yinc < 1e-6) {
            snprintf(str,
                512,
                "Error, parameter \"YINC\" out of range: %e  line %i file %s",
                devParms.chan[chn].yinc,
                __LINE__,
                __FILE__);
            goto OUT_ERROR;
        }

        usleep(20000);

        tmcWrite(":WAV:YREF?");

        usleep(20000);

        tmcRead();

        yref[chn] = atoi(device->buf);

        if((yref[chn] < 1) || (yref[chn] > 255)) {
            snprintf(str,
                512,
                "Error, parameter \"YREF\" out of range: %i  line %i file %s",
                yref[chn],
                __LINE__,
                __FILE__);
            goto OUT_ERROR;
        }

        usleep(20000);

        tmcWrite(":WAV:YOR?");

        usleep(20000);

        tmcRead();

        devParms.chan[chn].yor = atoi(device->buf);

        if((devParms.chan[chn].yor < -255) || (devParms.chan[chn].yor > 255)) {
            snprintf(str,
                512,
                "Error, parameter \"YOR\" out of range: %i  line %i file %s",
                devParms.chan[chn].yor,
                __LINE__,
                __FILE__);
            goto OUT_ERROR;
        }

        usleep(20000);

        tmcWrite(":WAV:DATA?");

        connect(&get_data_thrd, SIGNAL(finished()), &w_msg_box, SLOT(accept()));

        get_data_thrd.start();

        if(w_msg_box.exec() != QDialog::Accepted) {
            disconnect(&get_data_thrd, 0, 0, 0);
            strlcpy(str, "Aborted by user.", 512);
            goto OUT_ERROR;
        }

        disconnect(&get_data_thrd, 0, 0, 0);

        n = get_data_thrd.get_num_bytes_rcvd();
        if(n < 0) {
            strlcpy(str, "Can not read from device.", 512);
            goto OUT_ERROR;
        }

        if(n > WAVFRM_MAX_BUFSZ) {
            snprintf(str, 512, "Datablock too big for buffer: %i", n);
            goto OUT_ERROR;
        }

        if(n < 16) {
            strlcpy(str, "Not enough data in buffer.", 512);
            goto OUT_ERROR;
        }

        for(int i{}; i < n; i++)
            wavBuf[chn][i]
                = ((int)(((unsigned char*)device->buf)[i]) - yref[chn] - devParms.chan[chn].yor) << 5;
    }

    opath[0] = 0;
    if(recentSaveDir[0] != 0) {
        strlcpy(opath, recentSaveDir, MAX_PATHLEN);
        strlcat(opath, "/", MAX_PATHLEN);
    }
    strlcat(opath, "waveform.edf", MAX_PATHLEN);

    strlcpy(opath,
        QFileDialog::getSaveFileName(this, "Save file", opath, "EDF files (*.edf *.EDF)")
            .toLocal8Bit()
            .data(),
        MAX_PATHLEN);

    if(!strcmp(opath, ""))
        goto OUT_NORMAL;

    getDirectoryFromPath(recentSaveDir, opath, MAX_PATHLEN);

    hdl = edfopen_file_writeonly(opath, EDFLIB_FILETYPE_EDFPLUS, chns);
    if(hdl < 0) {
        strlcpy(str, "Can not create EDF file.", 512);
        goto OUT_ERROR;
    }

    if(edf_set_datarecord_duration(hdl, rec_len / 100LL)) {
        snprintf(str, 512, "Can not set datarecord duration of EDF file: %lli", rec_len / 100LL);
        goto OUT_ERROR;
    }

    j = 0;

    for(chn = 0; chn < MAX_CHNS; chn++) {
        if(!devParms.chan[chn].Display)
            continue;

        edf_set_samplefrequency(hdl, j, n);
        edf_set_digital_maximum(hdl, j, 32767);
        edf_set_digital_minimum(hdl, j, -32768);
        if(devParms.chan[chn].scale > 2) {
            edf_set_physical_maximum(hdl, j, devParms.chan[chn].yinc * 32767.0 / 32.0);
            edf_set_physical_minimum(hdl, j, devParms.chan[chn].yinc * -32768.0 / 32.0);
            edf_set_physical_dimension(hdl, j, "V");
        } else {
            edf_set_physical_maximum(hdl, j, 1000.0 * devParms.chan[chn].yinc * 32767.0 / 32.0);
            edf_set_physical_minimum(hdl, j, 1000.0 * devParms.chan[chn].yinc * -32768.0 / 32.0);
            edf_set_physical_dimension(hdl, j, "mV");
        }
        snprintf(str, 512, "CHAN%i", chn + 1);
        edf_set_label(hdl, j, str);

        j++;
    }

    edf_set_equipment(hdl, devParms.modelName);

    for(chn = 0; chn < MAX_CHNS; chn++) {
        if(!devParms.chan[chn].Display)
            continue;

        if(edfwrite_digital_short_samples(hdl, wavBuf[chn])) {
            strlcpy(str, "A write error occurred.", 512);
            goto OUT_ERROR;
        }
    }

OUT_NORMAL:

    if(hdl >= 0)
        edfclose_file(hdl);

    for(chn = 0; chn < MAX_CHNS; chn++) {
        free(wavBuf[chn]);
        wavBuf[chn] = nullptr;
    }

    scrnTimer->start(devParms.screenTimerIval);

    return;

OUT_ERROR:

    if(get_data_thrd.isFinished() != true) {
        connect(&get_data_thrd, SIGNAL(finished()), &w_msg_box, SLOT(accept()));
        w_msg_box.setText("Waiting for thread to finish, please wait...");
        w_msg_box.exec();
    }

    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText(str);
    msgBox.exec();

    if(hdl >= 0)
        edfclose_file(hdl);

    for(chn = 0; chn < MAX_CHNS; chn++) {
        free(wavBuf[chn]);
        wavBuf[chn] = nullptr;
    }

    scrnTimer->start(devParms.screenTimerIval);
}
