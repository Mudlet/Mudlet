#ifndef MUDLET_EDITORTREEROWDELEGATE_H
#define MUDLET_EDITORTREEROWDELEGATE_H

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
#include <QHash>
#include <QIcon>
#include <QPersistentModelIndex>
#include <QPixmap>
#include <QSize>
#include <QStyledItemDelegate>

class TTreeWidget;

namespace uiDesign {

struct ThemeTokens;

// The row machinery EditorTreeDelegate and VariableTreeDelegate share: the view indents nothing and
// the decoration draws depth, chevron and the marks at the leading edge, because QTreeView paints its
// indented branch cells itself in the style's selection colour, out of any stylesheet rule's reach,
// so a nested row's selection could not otherwise be one pill
class EditorTreeRowDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(EditorTreeRowDelegate)

    // Must be called on a theme change: the cached glyphs keep the old inks until then
    void restyle();

    bool editorEvent(QEvent* pEvent, QAbstractItemModel* pModel, const QStyleOptionViewItem& option, const QModelIndex& index) override;

    // On the viewport: the view reaches editorEvent() only through edit(), which gives up on an
    // invalid index, so the rest of a press that leaves its row would never arrive there
    bool eventFilter(QObject* pWatched, QEvent* pEvent) override;

    // Viewport coordinates, in the option the view hands editorEvent()
    [[nodiscard]] QRect dotHitRect(const QStyleOptionViewItem& option, const QModelIndex& index) const;
    [[nodiscard]] QRect chevronHitRect(const QStyleOptionViewItem& option, const QModelIndex& index) const;

    // For tests: rebuilds what QAbstractItemView hands editorEvent(), which it makes privately from
    // initViewItemOption(), the row's rectangle and whether the row is current
    [[nodiscard]] QStyleOptionViewItem rowOption(const QModelIndex& index) const;

protected:
    enum class ChevronState { None = 0, Closed = 1, Open = 2 };

    struct Decoration
    {
        QIcon icon;
        QSize size;
    };

    explicit EditorTreeRowDelegate(TTreeWidget* pTree);

    // The derived class's own inks, read before the caches are cleared
    virtual void restyleInks(const ThemeTokens& tokens) = 0;
    virtual void clearGlyphs() const = 0;
    // Whether the row carries the mark at the dot's place: an item's switch, a variable's kept square
    [[nodiscard]] virtual bool carriesDot(const QModelIndex& index) const = 0;
    // A press landed on that mark; the row is the tree's current one by now
    virtual void dotPressed() = 0;
    [[nodiscard]] virtual int rowLevelOf(const QModelIndex& index) const;

    [[nodiscard]] ChevronState chevronOf(const QModelIndex& index, const int level) const;
    [[nodiscard]] QPixmap chevronGlyph(const ChevronState state) const;
    // Call before reading any glyph cache, not while filling one: a screen change clears what a lookup would return
    void syncGlyphRatio() const;
    // The leading edge composed once rather than per paint and per measure, and kept under the key the
    // caller looked it up by; a dot slot keeps the dot's room whether or not a dot is drawn in it
    [[nodiscard]] Decoration composeDecoration(const int key, const int level, const ChevronState chevron, const bool dotSlot, const QPixmap& dot, const QPixmap& mark) const;

    TTreeWidget* mpTree = nullptr;
    int mIndentStep = 0;
    QColor mChevronInk;
    QColor mAccentBar;
    mutable qreal mGlyphRatio = 0.0;
    // Keyed on the leading edge's inputs alone, so a tree of any size fills it with a handful of entries
    mutable QHash<int, Decoration> mDecorations;

private:
    void clearCaches() const;

    // Indexed by ChevronState, so slot 0 (None) stays empty
    mutable QPixmap mChevronGlyphs[3];
    bool mPressAnswered = false;
    QPersistentModelIndex mLastDotPressIndex;
};

} // namespace uiDesign

#endif // MUDLET_EDITORTREEROWDELEGATE_H
