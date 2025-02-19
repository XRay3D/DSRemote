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

#include <QApplication>
#include <QDialog>
#include <QHBoxLayout>
#include <QObject>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QtGlobal>

#include "global.h"
#include "mainwindow.h"

class UiAboutwindow : public QObject {
    Q_OBJECT

public:
    UiAboutwindow();

private:
    QDialog* AboutDialog;

    QPushButton* pushButton1;

    QTextEdit* textedit1;

    QHBoxLayout* hlayout1;

    QVBoxLayout* vlayout1;
};
