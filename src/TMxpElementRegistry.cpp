/***************************************************************************
 *   Copyright (C) 2020 by Gustavo Sousa - gustavocms@gmail.com            *
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

#include "TMxpElementRegistry.h"
void TMxpElementRegistry::registerElement(const TMxpElement& element)
{
    mMXP_Elements[element.name.toUpper()] = element;
}
bool TMxpElementRegistry::containsElement(const QString& name) const
{
    return mMXP_Elements.contains(name.toUpper());
}

TMxpElement TMxpElementRegistry::getElement(const QString& name) const
{
    return mMXP_Elements[name.toUpper()];
}
void TMxpElementRegistry::unregisterElement(const QString& name)
{
    mMXP_Elements.remove(name.toUpper());
}

bool TMxpElementRegistry::isOpenElement(const QString& name) const
{
    const auto it = mMXP_Elements.constFind(name.toUpper());
    if (it != mMXP_Elements.constEnd()) {
        return it->open;
    }
    return false;
}

bool TMxpElementRegistry::hasElementWithPrefix(const QString& prefix) const
{
    const QString upper = prefix.toUpper();
    for (auto it = mMXP_Elements.constBegin(); it != mMXP_Elements.constEnd(); ++it) {
        if (it.key().startsWith(upper)) {
            return true;
        }
    }
    return false;
}

bool TMxpElementRegistry::hasOpenElementWithPrefix(const QString& prefix) const
{
    const QString upper = prefix.toUpper();
    for (auto it = mMXP_Elements.constBegin(); it != mMXP_Elements.constEnd(); ++it) {
        if (it.key().startsWith(upper) && it.value().open) {
            return true;
        }
    }
    return false;
}
