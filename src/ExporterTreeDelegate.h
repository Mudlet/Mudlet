#ifndef MUDLET_EXPORTERTREEDELEGATE_H
#define MUDLET_EXPORTERTREEDELEGATE_H

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

#include "uiDesign.h"

#include <QColor>
#include <QStyledItemDelegate>

// Paints only the accent bar; see uiDesign::paintAccentBar() for why it is not a border-left
class ExporterTreeDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(ExporterTreeDelegate)
    explicit ExporterTreeDelegate(QObject* pParent = nullptr);

    void restyle(const uiDesign::ThemeTokens& tokens);

    void paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QColor mAccentBar;
};

#endif // MUDLET_EXPORTERTREEDELEGATE_H
