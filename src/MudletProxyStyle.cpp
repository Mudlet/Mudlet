/***************************************************************************
 *   Copyright (C) 2020 by Piotr Wilczynski - delwing@gmail.com            *
 *   Copyright (C) 2022 by Stephen Lyons - slysven@virginmedia.com         *
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

#include "MudletProxyStyle.h"

#include <QAccessible>
#include <QStyleFactory>

MudletProxyStyle::MudletProxyStyle()
{
    setObjectName(baseStyle()->objectName());
}

MudletProxyStyle::MudletProxyStyle(const QString& style)
: QProxyStyle(QStyleFactory::create(style))
{
}

int MudletProxyStyle::styleHint(StyleHint styleHint, const QStyleOption* opt, const QWidget* widget, QStyleHintReturn* returnData) const
{
    if (styleHint == QStyle::SH_MenuBar_AltKeyNavigation) {
        // Screen reader users expect a lone Alt tap to focus the menu bar
        // (Mudlet/Mudlet#6145), but for everyone else an accidental Alt tap
        // stealing focus from the command line is a nuisance when using
        // Alt-based keybindings (Mudlet/Mudlet#4280), so only enable the hint
        // while assistive technology is in use. QMenuBar re-reads this hint on
        // every Alt key event, so the gate applies live, on each keypress
        return QAccessible::isActive() ? 1 : 0;
    }
    if (styleHint == QStyle::SH_ItemView_ActivateItemOnSingleClick) {
        return 0;
    }

    return QProxyStyle::styleHint(styleHint, opt, widget, returnData);
}
