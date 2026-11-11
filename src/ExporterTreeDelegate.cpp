/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers - mudlet@mudlet.org           *
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

#include "ExporterTreeDelegate.h"

#include <QModelIndex>
#include <QPainter>
#include <QStyleOptionViewItem>

ExporterTreeDelegate::ExporterTreeDelegate(QObject* pParent)
: QStyledItemDelegate(pParent)
{
    restyle(uiDesign::themeTokens());
}

void ExporterTreeDelegate::restyle(const uiDesign::ThemeTokens& tokens)
{
    mAccentBar = tokens.accent;
}

void ExporterTreeDelegate::paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyledItemDelegate::paint(pPainter, option, index);

    if (index.column() != 0 || !(option.state & QStyle::State_Selected) || option.rect.isEmpty()) {
        return;
    }

    uiDesign::paintAccentBar(pPainter, option.rect, mAccentBar);
}
