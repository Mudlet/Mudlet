/***************************************************************************
 *   Copyright (C) 2021 by Manuel Wegmann - wegmann.manuel@yahoo.com       *
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

#include "DarkTheme.h"
#include "MudletProxyStyle.h"

#include <QApplication>
#include <QPainter>
#include <QStyleOption>
#include <QWidget>

//DarkTheme only works with Fusion style
DarkTheme::DarkTheme()
: DarkTheme(new MudletProxyStyle(qsl("Fusion")))
{
}

DarkTheme::DarkTheme(QStyle* style)
: QProxyStyle(style)
{
}

void DarkTheme::polish(QPalette& palette)
{
    // modify palette to dark
    palette.setColor(QPalette::Window, QColor(53, 53, 53));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Base, QColor(25, 25, 25));
    palette.setColor(QPalette::AlternateBase, QColor(40, 40, 40));
    palette.setColor(QPalette::ToolTipBase, QColor(53, 53, 53));
    palette.setColor(QPalette::ToolTipText, Qt::white);
    palette.setColor(QPalette::Text, Qt::white);
    // Before Qt 6.10 Fusion polishes with the light palette even in dark mode, leaving this at #646464:
    // 2.9:1 against Base, under the 4.5:1 AA floor. Only needed for Qt 6.8/6.9.
    QColor placeholderText(palette.color(QPalette::Text));
    placeholderText.setAlphaF(0.5f);
    palette.setColor(QPalette::PlaceholderText, placeholderText);
    palette.setColor(QPalette::Dark, QColor(35, 35, 35));
    palette.setColor(QPalette::Light, QColor(75, 75, 75));
    palette.setColor(QPalette::Midlight, QColor(64, 64, 64));
    palette.setColor(QPalette::Mid, QColor(45, 45, 45));
    palette.setColor(QPalette::Shadow, QColor(20, 20, 20));
    palette.setColor(QPalette::Button, QColor(53, 53, 53));
    palette.setColor(QPalette::ButtonText, Qt::white);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Link, QColor(42, 130, 218));
    palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(127, 127, 127));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(127, 127, 127));
    palette.setColor(QPalette::Disabled, QPalette::Light, QColor(53, 53, 53));
    palette.setColor(QPalette::Disabled, QPalette::Midlight, QColor(53, 53, 53));
    palette.setColor(QPalette::Disabled, QPalette::Mid, QColor(42, 42, 42));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(127, 127, 127));
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(127, 127, 127));
    palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));

    QProxyStyle::polish(palette);
}

QIcon DarkTheme::standardIcon(StandardPixmap standardIcon, const QStyleOption* option, const QWidget* widget) const
{
    const QIcon icon = QProxyStyle::standardIcon(standardIcon, option, widget);
    if (icon.isNull() || (standardIcon != SP_ToolBarHorizontalExtensionButton && standardIcon != SP_ToolBarVerticalExtensionButton)) {
        return icon;
    }

    // Fusion's toolbar overflow chevrons are near-black images whatever the
    // palette, so on the dark Window colour they cannot be seen
    const QPalette& palette = option ? option->palette : (widget ? widget->palette() : QApplication::palette());
    const auto tinted = [](QPixmap pixmap, const QColor& colour) {
        QPainter painter(&pixmap);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(pixmap.rect(), colour);
        painter.end();
        return pixmap;
    };
    QIcon recoloured;
    const QList<QSize> sizes = icon.availableSizes();
    for (const QSize& size : sizes) {
        const QPixmap pixmap = icon.pixmap(size, 1.0);
        // Filling through an opaque pixmap would leave a solid square
        if (!pixmap.hasAlphaChannel()) {
            return icon;
        }
        recoloured.addPixmap(tinted(pixmap, palette.color(QPalette::Active, QPalette::WindowText)));
        // Qt would otherwise lighten the white glyph, leaving it looking enabled
        recoloured.addPixmap(tinted(pixmap, palette.color(QPalette::Disabled, QPalette::WindowText)), QIcon::Disabled);
    }
    return recoloured.isNull() ? icon : recoloured;
}

void DarkTheme::drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
    const auto* item = element == PE_IndicatorItemViewItemCheck ? qstyleoption_cast<const QStyleOptionViewItem*>(option) : nullptr;
    if (!item || item->state.testFlag(State_Selected) || item->backgroundBrush.style() != Qt::NoBrush) {
        QProxyStyle::drawPrimitive(element, option, painter, widget);
        return;
    }

    // Fusion's outline, Window.darker(140).lighter(110), is 1.2:1 on an item view's Base; this Window puts it halfway to
    // WindowText (Text is the item's own foreground). Highlight and item backgrounds show the original well enough.
    const QColor base = item->palette.color(QPalette::Base);
    const QColor text = item->palette.color(QPalette::WindowText);
    QStyleOptionViewItem check(*item);
    check.palette.setColor(QPalette::Window, QColor((base.red() + text.red()) / 2, (base.green() + text.green()) / 2, (base.blue() + text.blue()) / 2).lighter(127));
    QProxyStyle::drawPrimitive(element, &check, painter, widget);
}
