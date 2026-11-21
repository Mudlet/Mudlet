/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include "SidebarItemDelegate.h"

#include "uiDesign.h"

#include <QFontMetrics>
#include <QListWidget>
#include <QStyle>
#include <QStyleOptionViewItem>

namespace uiDesign {

SidebarItemDelegate::SidebarItemDelegate(QListWidget* pList)
: QStyledItemDelegate(pList)
, mpList(pList)
{
}

void SidebarItemDelegate::initStyleOption(QStyleOptionViewItem* pOption, const QModelIndex& index) const
{
    QStyledItemDelegate::initStyleOption(pOption, index);
    // Not in the sheet: a stylesheet font on ::item never reaches the painter.
    // sidebarRowWidth() measures every row in this bold.
    if (pOption->state & QStyle::State_Selected) {
        pOption->font.setBold(true);
        pOption->fontMetrics = QFontMetrics(pOption->font);
    }
    if (!mpList->property(scmProp_rail).toBool()) {
        return;
    }
    pOption->text.clear();
    pOption->features &= ~QStyleOptionViewItem::HasDisplay;
    pOption->decorationAlignment = Qt::AlignCenter;
}

} // namespace uiDesign
