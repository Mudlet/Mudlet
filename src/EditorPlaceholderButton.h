#ifndef MUDLET_EDITORPLACEHOLDERBUTTON_H
#define MUDLET_EDITORPLACEHOLDERBUTTON_H

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

#include <QColor>
#include <QMargins>
#include <QToolButton>

namespace uiDesign {

// The frame is painted because Qt strokes a stylesheet "1px dashed" border as
// squares as long as the gaps. The rest is still the stylesheet's, so
// setFrameMargins() takes the margins the sheet gives the button.
class PlaceholderButton : public QToolButton
{
public:
    Q_DISABLE_COPY(PlaceholderButton)
    explicit PlaceholderButton(QWidget* pParent);

    void setFrameColors(const QColor& resting, const QColor& active, const QColor& disabled);
    void setFrameMargins(const QMargins& margins);
    void setFrameRadius(const qreal radius);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QColor mRestingColor;
    QColor mActiveColor;
    QColor mDisabledColor;
    QMargins mFrameMargins;
    qreal mFrameRadius;
};

} // namespace uiDesign

#endif // MUDLET_EDITORPLACEHOLDERBUTTON_H
