#ifndef MUDLET_ALPHACOLOURWELL_H
#define MUDLET_ALPHACOLOURWELL_H

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

#include <QColor>
#include <QIcon>
#include <QObject>

class QPushButton;

namespace uiDesign {

// A colour well whose colour may be partly see-through. It has a plain well's
// frame - the same hairline, corner, height and width - and inside it a field:
// a square swatch at the leading edge with the field showing round it, the
// colour over a checkerboard so any transparency shows the board through it
// and a hairline of its own over that, and at the trailing edge the button's
// own words saying how opaque the colour is, a step smaller in the fixed-pitch
// face and of one length, so a column of them lines up. It is drawn at
// whatever size the well has.
//
// The field and the swatch are painted under the button's own drawing rather
// than handed to it as an icon: a push button's size hint follows its icon,
// and an icon sized to the button would have the button ask for more room each
// time it was given some. The words are the button's text, drawn by its
// stylesheet at the trailing edge, with the well never narrower than the
// widest of them needs.
class AlphaColourWell : public QObject
{
public:
    Q_DISABLE_COPY(AlphaColourWell)
    // The one on pButton, made the first time it is asked for; the button owns it
    static AlphaColourWell* on(QPushButton* pButton);
    // The one already made on pButton, or null
    static AlphaColourWell* find(const QPushButton* pButton);

    // An invalid colour clears the well's words and its sheet and paints
    // nothing, leaving the button as the window's own sheet draws it
    void setColour(const QColor& colour);
    [[nodiscard]] QColor colour() const { return mColour; }

protected:
    bool eventFilter(QObject* pWatched, QEvent* pEvent) override;

private:
    explicit AlphaColourWell(QPushButton* pButton);
    // Inside the hairline: what a plain well's background fills
    [[nodiscard]] QRect inside() const;
    // The words and the sheet, which follow the colour, the language, the
    // font and the appearance
    void applyLook();
    void regenerate();

    QPushButton* mpButton = nullptr;
    QColor mColour;
    QIcon mIcon;
    // Set by anything that changes what is painted - the size, the screen, the
    // style, the colour, whether the well is available - and cleared when the
    // picture is drawn again for it
    bool mStale = true;
};

} // namespace uiDesign

#endif // MUDLET_ALPHACOLOURWELL_H
