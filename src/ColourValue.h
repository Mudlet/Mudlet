#ifndef MUDLET_COLOURVALUE_H
#define MUDLET_COLOURVALUE_H

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
#include <QLineEdit>
#include <QPointer>
#include <QString>

class QAbstractButton;
class QSpinBox;

namespace uiDesign {

struct ThemeTokens;

// A row at a form field's height, or at a colour well's for a card holding columns of colours
enum class ColourRowSize { Normal, Compact };

// A field that shows a colour and takes one typed as #RGB or #RRGGBB, with a swatch of the colour at its leading
// edge, beside a button that opens the colours window. Drawn by the window's inputStyleSheet(); the swatch is
// painted from themeTokens() each time, so an appearance change asks nothing of the owner for it.
class ColourValue : public QLineEdit
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(ColourValue)
    explicit ColourValue(QWidget* pParent = nullptr, const ColourRowSize size = ColourRowSize::Normal);

    // An invalid colour leaves the field blank, as clear() does
    void setColour(const QColor& colour);
    // No colour of its own: the swatch shows the one the item is drawn with, under the default word
    void setDefault(const QColor& drawnWith);
    // For a selection whose items do not agree on one colour: no code, no word and no swatch
    void clear();
    // What the field says while the item has no colour of its own. Only where the row has a Reset: emptying the
    // field then asks for the default as well, where without a word it puts the colour back.
    void setDefaultWord(const QString& word);
    // For a colour that may be see-through, a box for its opacity, kept in step with this field, for the owner to
    // lay out right after it. Made on the first call.
    QSpinBox* opacityBox();
    // The row's Choose. A click on the swatch presses it, and while it is available the swatch shows it can be
    // clicked: a pointing hand, a ring in the accent and Choose's tooltip under the pointer.
    void setChooseButton(QAbstractButton* pChoose);
    [[nodiscard]] QAbstractButton* chooseButton() const { return mpChoose; }

    // What the swatch shows; invalid while the field is blank
    [[nodiscard]] QColor colour() const { return mColour; }
    [[nodiscard]] bool showsDefault() const { return mShows == Shows::Default; }
    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

signals:
    // Typed, or set in the opacity box; the field already shows it
    void colourEntered(const QColor& colour);
    // The field was emptied where it has a default word
    void defaultEntered();

protected:
    bool event(QEvent* pEvent) override;
    bool eventFilter(QObject* pWatched, QEvent* pEvent) override;
    void keyPressEvent(QKeyEvent* pEvent) override;
    void focusOutEvent(QFocusEvent* pEvent) override;
    void changeEvent(QEvent* pEvent) override;
    void paintEvent(QPaintEvent* pEvent) override;
    void mousePressEvent(QMouseEvent* pEvent) override;
    void mouseReleaseEvent(QMouseEvent* pEvent) override;
    void mouseDoubleClickEvent(QMouseEvent* pEvent) override;
    void mouseMoveEvent(QMouseEvent* pEvent) override;
    void leaveEvent(QEvent* pEvent) override;

private:
    enum class Shows { Nothing, Default, Own };

    [[nodiscard]] QString shownCode() const;
    [[nodiscard]] bool edited() const;
    [[nodiscard]] bool opacityEdited() const;
    void commit();
    void revert();
    void showOpacity();
    void takeOpacity(const int percent);
    void keepTextClearOfSwatch();
    [[nodiscard]] QRect swatchRect() const;
    // The swatch and the inset round it: the whole square at the field's leading edge
    [[nodiscard]] QRect swatchTarget() const;
    [[nodiscard]] bool swatchClickable() const;
    [[nodiscard]] bool onSwatch(const QPoint& position) const;
    void hoverSwatch(const bool over);

    const ColourRowSize mSize;
    QColor mColour;
    Shows mShows = Shows::Nothing;
    QString mDefaultWord;
    QPointer<QSpinBox> mpOpacity;
    QPointer<QAbstractButton> mpChoose;
    bool mSwatchHovered = false;
    bool mSwatchPressed = false;
};

enum class ColourButton { Choose, Reset };

// The icon-only button beside a ColourValue, square at the height of the row it stands in; its words belong in its
// tooltip and accessible name. The glyph's ink does not follow an appearance change by itself, so call this again on
// every one.
void setColourButtonGlyph(QAbstractButton* pButton, const ColourButton kind, const ThemeTokens& tokens, const ColourRowSize size = ColourRowSize::Normal);

} // namespace uiDesign

#endif // MUDLET_COLOURVALUE_H
