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

#ifndef MUDLET_ALTFOCUSMENUBARDISABLE_H
#define MUDLET_ALTFOCUSMENUBARDISABLE_H

#include <QIcon>
#include <QProxyStyle>
#include <QStyleFactory>

class AltFocusMenuBarDisable : public QProxyStyle
{
    Q_OBJECT

public:
    AltFocusMenuBarDisable();
    explicit AltFocusMenuBarDisable(const QString& style);
    int styleHint(StyleHint styleHint, const QStyleOption* opt, const QWidget* widget, QStyleHintReturn* returnData) const override;
    void drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = nullptr) const override;
    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption* option = nullptr, const QWidget* widget = nullptr) const override;

private:
    // Fusion loads each title bar icon from six embedded PNGs on every call, and paints a dock
    // widget's title - each time any part of the dock is painted - by asking two of them for
    // their actualSize(). That answer never depends on what the icon was asked before, which
    // pixmap()'s does, so only the title paint shares an icon.
    mutable bool mMeasuringDockTitle = false;
    mutable QIcon mDockTitleCloseIcon;
    mutable QIcon mDockTitleNormalIcon;
};

#endif
