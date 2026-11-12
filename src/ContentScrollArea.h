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

#ifndef MUDLET_CONTENTSCROLLAREA_H
#define MUDLET_CONTENTSCROLLAREA_H

#include <QScrollArea>

namespace uiDesign {

// QScrollArea caches its widget's sizeHint() from the first ask, and its
// minimumSizeHint() reserves both scroll bars even when neither shows; under a
// form held to its contents that sizes it for an earlier item and a bar too tall.
class ContentScrollArea : public QScrollArea
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(ContentScrollArea)
    explicit ContentScrollArea(QWidget* pParent = nullptr);

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

protected:
    // Passes the widget's LayoutRequest up as updateGeometry(), or the layout
    // holding this area keeps the hint it cached
    bool eventFilter(QObject* pWatched, QEvent* pEvent) override;
};

} // namespace uiDesign

#endif // MUDLET_CONTENTSCROLLAREA_H
