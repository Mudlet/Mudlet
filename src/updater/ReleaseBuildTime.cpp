/***************************************************************************
 *   Copyright (C) 2017 by Philipp Medien - hello@dblsqd.com               *
 *   Copyright (C) 2026 by Vadim Peretokin - vperetokin@gmail.com          *
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

// Kept apart from Release.cpp so it can be built without the precompiled header
// and its relaxed ccache settings, which would let ccache hand back an object
// carrying some earlier build's __DATE__ and __TIME__.

#include "Release.h"

#include "../utils.h"

#include <QCoreApplication>
#include <QLocale>

namespace dblsqd {

dblsqd::Release Release::getCurrentRelease()
{
    // embed build time so public test releases, which cannot be compared via semver, can be compared via datetime
    QString buildDateTime = QString(__DATE__) + " " + QString(__TIME__);
    // locale-independent datetime parsing (C locale matches __DATE__'s English format)
    QDateTime date = QLocale::c().toDateTime(buildDateTime.simplified(), qsl("MMM d yyyy hh:mm:ss"));

    return dblsqd::Release(QCoreApplication::applicationVersion(), date);
}

} // namespace dblsqd
