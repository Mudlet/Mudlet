#ifndef MUDLET_TACCESSIBLECONSOLE_H
#define MUDLET_TACCESSIBLECONSOLE_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       *
 *   Copyright (C) 2014-2020, 2022 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2022 by Thiago Jung Bauermann - bauermann@kolabnow.com  *
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

#include "TConsole.h"

#include <QAccessibleInterface>
#include <QAccessibleWidget>

class TAccessibleConsole : public QAccessibleWidget
{
public:
    explicit TAccessibleConsole(QWidget* pWidget)
    : QAccessibleWidget(pWidget, QAccessible::Pane)
    {
        Q_ASSERT(isValid());
    }

    static QAccessibleInterface* consoleFactory(const QString &classname, QObject *object)
    {
        // The original identifier "interface" was no good as it is an existing
        // macro (on MSYS2/Mingw-w64) and breaks compilation on that platform in
        // an obsecure way:
        QAccessibleInterface* pInterface = nullptr;

        if (classname == QLatin1String("TConsole") && object && object->isWidgetType()) {
            pInterface = new TAccessibleConsole(static_cast<QWidget *>(object));
        }

        return pInterface;
    }
};

#endif // MUDLET_TACCESSIBLECONSOLE_H
