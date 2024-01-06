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

#include <QObject>

class QTcpSocket;

inline class TMCLan : public QObject {
    Q_OBJECT
    QTcpSocket* socket{};
    int sockfd{-1};

public:
    TMCLan(QObject* parent = nullptr);
    virtual ~TMCLan() { }
    struct tmcDev* tmcLanOpen(std::string_view);
    void tmcLanClose(struct tmcDev*);
    int tmcLanWrite(struct tmcDev*, std::string_view);
    int tmcLanRead(struct tmcDev*);
} tmcLan;

inline struct tmcDev* tmcLanOpen(const char* addr) { return tmcLan.tmcLanOpen(addr); }
inline void tmcLanClose(struct tmcDev* dev) { return tmcLan.tmcLanClose(dev); }
inline int tmcLanWrite(struct tmcDev* dev, const char* str) { return tmcLan.tmcLanWrite(dev, str); }
inline int tmcLanRead(struct tmcDev* dev) { return tmcLan.tmcLanRead(dev); }
