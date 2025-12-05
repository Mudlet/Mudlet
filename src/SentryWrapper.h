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

#ifndef SENTRY_WRAPPER_H
#define SENTRY_WRAPPER_H

#ifdef WITH_SENTRY
#include <QtCore/qscopeguard.h>
#include "sentry.h"
#endif

#include <string>

void        initSentry();
std::string makeExecutablePath(const std::string& dir, const std::string& name);
std::string getExeDir();
void        crashIfRequested();

#endif // SENTRY_WRAPPER_H
