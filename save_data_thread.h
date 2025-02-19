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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <QObject>
#include <QThread>

#include "connection.h"
#include "edflib.h"
#include "global.h"
#include "tmc_dev.h"
#include "utils.h"

class SaveDataThread : public QThread {
    Q_OBJECT

public:
    SaveDataThread(int);

    int get_error_num(void);
    void get_error_str(char*, int);
    int get_num_bytes_rcvd(void);
    void init_save_memory_edf_file(struct DeviceSettings* devp, int, int, int, short** wav);

private:
    int job, err_num, n_bytes_rcvd, hdl, datrecs, smps_per_record;

    char err_str[4096];

    struct DeviceSettings* devParms;

    short** wavBuf;

    void run();

    void read_data(void);
    void save_memory_edf_file(void);
};
