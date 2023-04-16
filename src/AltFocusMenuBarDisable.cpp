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

#include "AltFocusMenuBarDisable.h"

AltFocusMenuBarDisable::AltFocusMenuBarDisable()
{
    setObjectName(baseStyle()->objectName());
}

AltFocusMenuBarDisable::AltFocusMenuBarDisable(const QString &style)
: QProxyStyle(QStyleFactory::create(style))
{}

int AltFocusMenuBarDisable::styleHint(StyleHint styleHint, const QStyleOption *opt, const QWidget *widget, QStyleHintReturn *returnData) const
{
    if (styleHint == QStyle::SH_MenuBar_AltKeyNavigation) {
        return 0;
    }

    return QProxyStyle::styleHint(styleHint, opt, widget, returnData);
}
