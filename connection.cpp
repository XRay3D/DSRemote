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

#include "connection.h"
#include "tmc_dev.h"
#include "tmclan.h"

int tmcConnectionType;

struct tmcDev* tmcDevice;

struct tmcDev* tmcOpenUsb(const char* device) {
    tmcConnectionType = 0;
    tmcDevice = tmcDevOpen(device);
    return tmcDevice;
}

struct tmcDev* tmcOpenLan(const char* address) {
    tmcConnectionType = 1;
    tmcDevice = tmcLanOpen(address);
    return tmcDevice;
}

void tmcClose(void) {
    if(tmcConnectionType == 0)
        tmcDevClose(tmcDevice);
    else
        tmcLanClose(tmcDevice);
    tmcDevice = nullptr;
}

int tmcWrite(const char* cmd) {
    if(tmcConnectionType == 0)
        return tmcDevWrite(tmcDevice, cmd);
    else
        return tmcLanWrite(tmcDevice, cmd);
    return -1;
}

int tmcRead(void) {
    if(tmcConnectionType == 0)
        return tmcDevRead(tmcDevice);
    else
        return tmcLanRead(tmcDevice);
    return -1;
}
