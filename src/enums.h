/***************************************************************************
*   Copyright (C) 2025 by Vadim Peretokin - vperetokin@gmail.com           *
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

#ifndef ENUMS_H
#define ENUMS_H

#include <QObject>

// helper Qt class to declare enums and flags throughout Mudlet. This class should not include any other files
// keep compile times down
class enums : public QObject {
    Q_OBJECT
public:
    enum Appearance {
        systemSetting = 0,
        light = 1,
        dark = 2
    };

    enum controlsVisibilityFlag {
        visibleNever = 0,
        visibleOnlyWithoutLoadedProfile = 0x1,
        visibleMaskNormally = 0x2,
        visibleAlways = 0x3
    };
    Q_DECLARE_FLAGS(controlsVisibility, controlsVisibilityFlag)

    enum class PackageModuleType {
        Package = 0,         // Regular package installation
        ModuleFromUI = 1,    // First-time module installation via UI
        ModuleSync = 2,      // Module sync operation
        ModuleFromScript = 3 // Module installation from script
    };
    Q_ENUM(PackageModuleType)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(enums::controlsVisibility)

#endif //ENUMS_H
