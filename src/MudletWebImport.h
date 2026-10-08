#ifndef MUDLET_MUDLETWEBIMPORT_H
#define MUDLET_MUDLETWEBIMPORT_H

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

#include <QCoreApplication>
#include <QStringList>

// Adds the profiles in a .zip - Mudlet Web's "Export profiles…" download, or
// "Export to Mudlet Web" from another computer - as new profiles: one Mudlet
// profile folder per profile, each given a name no existing profile has.
class MudletWebImport
{
    Q_DECLARE_TR_FUNCTIONS(MudletWebImport)

public:
    struct Result
    {
        // as named in the profiles folder, which can differ from the archive's
        QStringList profiles;
        QStringList warnings;
        // why nothing was imported
        QString error;
    };

    // Blocks, and touches nothing but the file system, so it can run off the main thread
    static Result importArchive(const QString& archivePathFileName, const QString& profilesPath);

    static QString locateModuleFile(const QString& profileHome, const QString& module, const QString& filePath);
};

#endif // MUDLET_MUDLETWEBIMPORT_H
