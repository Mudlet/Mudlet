#ifndef MUDLET_EDITORTREEDELEGATE_H
#define MUDLET_EDITORTREEDELEGATE_H

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

#include "EditorTreeRowDelegate.h"
#include "TTreeWidget.h"

#include <QColor>
#include <QPixmap>
#include <QPointer>

class Host;

namespace uiDesign {

class EditorTreeDelegate : public EditorTreeRowDelegate
{
    Q_OBJECT

public:
    enum class DotState { Off = 0, WantedOn = 1, Running = 2 };

    enum class RowMark { None = 0, Folder = 1, Filter = 2, OffsetTimer = 3, Error = 4, NewFolder = 5, NewItem = 6, Package = 7 };

    // Folders view (TreeType::None) only: a row names its TreeType in this role beside the id,
    // and a row with neither is drawn as a group of folders, with no switch
    static constexpr int scmRole_rowTreeType = Qt::UserRole + 40;

    EditorTreeDelegate(TTreeWidget* pTree, const TreeType treeType, Host* pHost);

    // Must match the accessible description the editor gives an unsaved item's or folder's row
    void setNewDescriptions(const QString& item, const QString& folder);
    // Takes effect at the next restyle()
    void setColouredMarks(const bool coloured) { mColouredMarks = coloured; }

    void initStyleOption(QStyleOptionViewItem* pOption, const QModelIndex& index) const override;

    void paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    struct ItemState
    {
        bool known = false;
        // A group of folders has no switch, but keeps the dot's room so its mark lines up with its siblings'
        bool dotSlot = false;
        DotState dot = DotState::Off;
        RowMark mark = RowMark::None;
    };

    // Public for tests, which assert against the state rather than against the drawn pictures
    [[nodiscard]] ItemState stateOf(const QModelIndex& index) const;
    // Rows anywhere under index whose own switch is off; a closed group says how many at its edge
    [[nodiscard]] int switchedOffInside(const QModelIndex& index) const;
    // What hovering the row's dot says, and nothing for a row without a switch
    [[nodiscard]] QString dotToolTip(const QModelIndex& index) const;

    bool helpEvent(QHelpEvent* pEvent, QAbstractItemView* pView, const QStyleOptionViewItem& option, const QModelIndex& index) override;

signals:
    // No argument: the row is made the tree's current one first, and the slot reads that
    void toggleRequested();

protected:
    void restyleInks(const ThemeTokens& tokens) override;
    void clearGlyphs() const override;
    [[nodiscard]] bool carriesDot(const QModelIndex& index) const override;
    void dotPressed() override;
    // One deeper in the Folders view: it has no heading, so its top rows stand where an item tree's first items do
    [[nodiscard]] int rowLevelOf(const QModelIndex& index) const override;

private:
    static constexpr int scmRowMarkCount = 8;
    static constexpr int scmDotStateCount = 3;

    void paintTypeLabel(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    void paintRowWithOffCount(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index, const int offInside) const;
    [[nodiscard]] QPixmap dotGlyph(const DotState state, const bool broken) const;
    [[nodiscard]] QPixmap markGlyph(const RowMark mark, const bool selected) const;
    [[nodiscard]] Decoration decorationFor(const ItemState& state, const int level, const ChevronState chevron, const bool selected) const;

    TreeType mTreeType = TreeType::None;
    QPointer<Host> mpHost;
    QColor mRunningDot;
    QColor mQuietDot;
    QColor mNotRunningText;
    QColor mMarkInk;
    QColor mErrorMarkInk;
    QColor mSelectedMarkInk;
    bool mColouredMarks = false;
    // Left invalid unless the marks are coloured
    QColor mPackageInk;
    QColor mPackageFill;
    QString mNewItemDescription;
    QString mNewFolderDescription;

    // Cached because laying a tree out asks every one of its rows for a size
    mutable QPixmap mDotGlyphs[2 * scmDotStateCount];
    mutable QPixmap mMarkGlyphs[2 * scmRowMarkCount];
};

} // namespace uiDesign

#endif // MUDLET_EDITORTREEDELEGATE_H
