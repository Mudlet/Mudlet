/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include "MudletSettings.h"

#include "MudletPaths.h"
#include "utils.h"

#include <QCoreApplication>
#include <QPointer>
#include <QSettings>

namespace {
QPointer<QSettings> smpSettings;
QString smInterfaceLanguage;
} // namespace

QSettings* MudletSettings::getQSettings()
{
    if (smpSettings) {
        return smpSettings;
    }
    const QString configRoot = MudletPaths::getMudletPath(enums::mainPath);
    // Callers guard on null until setupConfig() has settled the root; a root of
    // "" would otherwise put the file at /Mudlet.ini
    if (configRoot.isEmpty()) {
        return nullptr;
    }
    // parented to the application, not the main window: the window deletes
    // itself on close and the Updater keeps using this QSettings past that point.
    smpSettings = new QSettings(qsl("%1/Mudlet.ini").arg(configRoot), QSettings::IniFormat, QCoreApplication::instance());
    return smpSettings;
}

void MudletSettings::reset()
{
    delete smpSettings;
}

const QString& MudletSettings::getInterfaceLanguage()
{
    return smInterfaceLanguage;
}

void MudletSettings::setInterfaceLanguage(const QString& language)
{
    smInterfaceLanguage = language;
}
