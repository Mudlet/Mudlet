#ifndef MUDLET_MAPCOLOURPREVIEW_H
#define MUDLET_MAPCOLOURPREVIEW_H

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

#include <QPointer>
#include <QWidget>

class Host;

namespace uiDesign {

// A small map drawn in the profile's map colours: rooms in the environment colours joined by links on the grid, a
// room on the level below and one above, two rooms on one spot and the info box over its corner. Shows only; the
// colours are read from the Host at every paint, so whoever changes one calls update().
class MapColourPreview : public QWidget
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(MapColourPreview)
    explicit MapColourPreview(QWidget* pParent = nullptr);

    void setHost(Host* pHost);
    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

protected:
    void changeEvent(QEvent* pEvent) override;
    void paintEvent(QPaintEvent* pEvent) override;

private:
    void retranslate();

    QPointer<Host> mpHost;
};

} // namespace uiDesign

#endif // MUDLET_MAPCOLOURPREVIEW_H
