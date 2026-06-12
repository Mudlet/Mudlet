/***************************************************************************
 *   Copyright (C) 2020 by Piotr Wilczynski - delwing@gmail.com            *
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

#ifndef MUDLET_MUDLETPROXYSTYLE_H
#define MUDLET_MUDLETPROXYSTYLE_H

#include <QProxyStyle>

// Applies Mudlet's application-wide style hint adjustments on top of whichever
// base style is in use; previously named AltFocusMenuBarDisable when it
// unconditionally suppressed the Alt key menu bar navigation that it now
// enables for screen reader users only
class MudletProxyStyle : public QProxyStyle
{
    Q_OBJECT

public:
    MudletProxyStyle();
    explicit MudletProxyStyle(const QString& style);
    int styleHint(StyleHint styleHint, const QStyleOption* opt, const QWidget* widget, QStyleHintReturn* returnData) const override;
};

#endif
