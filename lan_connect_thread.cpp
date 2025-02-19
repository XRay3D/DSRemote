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

#include "lan_connect_thread.h"

LanConnectThread::LanConnectThread() {
    device = nullptr;
    devStr[0] = 0;
}

void LanConnectThread::run() {
    msleep(300);
    if(devStr[0] == 0)
        return;
    device = tmcOpenLan(devStr);
}

struct tmcDev* LanConnectThread::getDevice(void) {
    return device;
}

void LanConnectThread::setDeviceAddress(const char* addr) {
    strlcpy(devStr, addr, 64);
}
