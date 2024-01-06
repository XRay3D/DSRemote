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

#ifdef __cplusplus
extern "C" {
#endif

struct tmcDev {
    int fd;
    char* hdrBuf;
    char* buf;
    int sz;
};

struct tmcDev* tmcDevOpen(const char*);
void tmcDevClose(struct tmcDev*);
int tmcDevWrite(struct tmcDev*, const char*);
int tmcDevRead(struct tmcDev*);

#ifdef __cplusplus
} /* extern "C" */
#endif
