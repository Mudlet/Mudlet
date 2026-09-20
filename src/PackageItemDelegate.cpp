/***************************************************************************
 *   Copyright (C) 2025 by Vadim Peretokin - vperetokin@gmail.com         *
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

#include "PackageItemDelegate.h"

#include <QApplication>
#include <QFontMetrics>
#include <QPalette>

PackageItemDelegate::PackageItemDelegate(QObject* parent)
: QStyledItemDelegate(parent)
{
}

void PackageItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (!index.isValid()) {
        return;
    }

    painter->save();

    // Draw background and selection state
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    // Draw the background
    QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);

    // Get data from model
    QString packageName = index.data(Qt::DisplayRole).toString();
    QString description = index.data(Qt::UserRole).toString();
    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();

    // Calculate rects
    QRect iconRect = opt.rect.adjusted(cTextMargin, cTextMargin, 0, 0);
    iconRect.setSize(opt.decorationSize);

    int textLeft = iconRect.right() + cTextMargin;
    QRect textRect = opt.rect.adjusted(textLeft, cTextMargin, -cTextMargin, -cTextMargin);

    // Draw icon if available
    if (!icon.isNull()) {
        icon.paint(painter, iconRect, Qt::AlignLeft | Qt::AlignTop);
    }

    // Setup fonts
    QFont boldFont = opt.font;
    boldFont.setBold(true);
    QFont normalFont = opt.font;

    QFontMetrics boldMetrics(boldFont);
    QFontMetrics normalMetrics(normalFont);

    // Draw package name (bold)
    painter->setFont(boldFont);
    painter->setPen(opt.palette.color(QPalette::Text));

    QString elidedName = boldMetrics.elidedText(packageName, Qt::ElideRight, textRect.width());
    QRect nameRect = textRect;
    nameRect.setHeight(boldMetrics.height());
    painter->drawText(nameRect, Qt::AlignLeft | Qt::AlignTop, elidedName);

    // Draw description (gray, below name)
    if (!description.isEmpty()) {
        painter->setFont(normalFont);
        painter->setPen(opt.palette.color(QPalette::PlaceholderText));

        QRect descRect = textRect;
        descRect.setTop(nameRect.bottom() + cLineSpacing);

        QString elidedDesc = normalMetrics.elidedText(description, Qt::ElideRight, descRect.width());
        painter->drawText(descRect, Qt::AlignLeft | Qt::AlignTop, elidedDesc);
    }

    painter->restore();
}

QSize PackageItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (!index.isValid()) {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    // Calculate height needed for icon and two lines of text
    QFont boldFont = option.font;
    boldFont.setBold(true);
    QFont normalFont = option.font;

    QFontMetrics boldMetrics(boldFont);
    QFontMetrics normalMetrics(normalFont);

    int textHeight = boldMetrics.height() + cLineSpacing + normalMetrics.height();
    int iconHeight = option.decorationSize.height();

    int totalHeight = qMax(textHeight, iconHeight) + (cTextMargin * 2);

    return QSize(option.rect.width(), totalHeight);
}
