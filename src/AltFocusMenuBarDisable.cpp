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

#include <QScopedValueRollback>

AltFocusMenuBarDisable::AltFocusMenuBarDisable()
{
    setObjectName(baseStyle()->objectName());
}

AltFocusMenuBarDisable::AltFocusMenuBarDisable(const QString& style)
: QProxyStyle(QStyleFactory::create(style))
{
}

int AltFocusMenuBarDisable::styleHint(StyleHint styleHint, const QStyleOption* opt, const QWidget* widget, QStyleHintReturn* returnData) const
{
    if (styleHint == QStyle::SH_MenuBar_AltKeyNavigation) {
        return 0;
    }
    if (styleHint == QStyle::SH_ItemView_ActivateItemOnSingleClick) {
        return 0;
    }

    return QProxyStyle::styleHint(styleHint, opt, widget, returnData);
}

void AltFocusMenuBarDisable::drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
    if (element == CE_DockWidgetTitle && !baseStyle()->name().compare(QLatin1String("fusion"), Qt::CaseInsensitive)) {
        const QScopedValueRollback measuring(mMeasuringDockTitle, true);
        QProxyStyle::drawControl(element, option, painter, widget);
        return;
    }
    QProxyStyle::drawControl(element, option, painter, widget);
}

QIcon AltFocusMenuBarDisable::standardIcon(StandardPixmap standardIcon, const QStyleOption* option, const QWidget* widget) const
{
    if (!mMeasuringDockTitle || (standardIcon != SP_TitleBarCloseButton && standardIcon != SP_TitleBarNormalButton)) {
        return QProxyStyle::standardIcon(standardIcon, option, widget);
    }
    QIcon& kept = standardIcon == SP_TitleBarCloseButton ? mDockTitleCloseIcon : mDockTitleNormalIcon;
    if (kept.isNull()) {
        kept = QProxyStyle::standardIcon(standardIcon, option, widget);
    }
    return kept;
}
