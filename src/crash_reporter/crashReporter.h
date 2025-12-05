/***************************************************************************
 *   Copyright (C) 2025 by Nicolas Keita - nicolaskeita2@gmail.com         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef CRASH_REPORTER_H
#define CRASH_REPORTER_H

#include <QSettings>

enum TCrashSendOption {
    AlwaysSend   = 1,
    SendThisTime = 2,
    DontSend     = 3
};

void sendCrashReport(const char *envelopePath);
void showCrashDialogAndSend(const char *envelopePath, QSettings &settings);
TCrashSendOption createCrashDialog();

#endif // CRASH_REPORTER_H
