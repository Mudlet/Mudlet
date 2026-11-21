#ifndef MUDLET_SEARCHRESULTDELEGATE_H
#define MUDLET_SEARCHRESULTDELEGATE_H

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
#include <QFont>
#include <QHash>
#include <QPixmap>
#include <QPointer>
#include <QStyledItemDelegate>

class QTreeWidget;

// Forward-declared: EditorCommand.h, which defines it, pulls in the undo stack.
namespace EditorViewTypes {
enum class EditorViewType : int;
}
using EditorViewTypes::EditorViewType;

namespace uiDesign {

// A delegate rather than a widget per row: a search of a large profile is
// thousands of rows.
class SearchResultDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    enum RowKind { ItemRow = 0, MatchRow = 1, NoticeRow = 2 };

    // Clear of dlgTriggerEditor's private navigation roles, which start at
    // Qt::UserRole, so one added there needs nothing here.
    enum Role {
        // Must equal dlgTriggerEditor::ItemRole
        ViewTypeRole = Qt::UserRole + 2,
        RowKindRole = Qt::UserRole + 32,
        TitleRole = Qt::UserRole + 33,
        TypeLabelRole = Qt::UserRole + 34,
        WhereRole = Qt::UserRole + 35,
        SnippetRole = Qt::UserRole + 36,
        // QChar offsets into the snippet, not the item: tabs are widened before
        // the line is shown
        MatchStartRole = Qt::UserRole + 37,
        MatchLengthRole = Qt::UserRole + 38
    };

    Q_DISABLE_COPY(SearchResultDelegate)
    explicit SearchResultDelegate(QTreeWidget* pTree);

    void restyle();

    void paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    [[nodiscard]] QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    [[nodiscard]] QPixmap typeGlyph(const EditorViewType viewType, const bool selected) const;
    [[nodiscard]] static QFont scaledFont(const QFont& base, const qreal factor);
    void paintItemRow(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index, const QRect& contentRect) const;
    void paintMatchRow(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index, const QRect& contentRect) const;
    void paintNoticeRow(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index, const QRect& contentRect) const;

    QPointer<QTreeWidget> mpTree;
    QColor mTitleColor;
    QColor mMutedColor;
    QColor mChipBorder;
    QColor mSelectedText;
    QColor mMarkerColor;
    QColor mMarkerText;

    QFont mNameFont;
    QFont mChipFont;
    int mItemRowHeight = 0;
    int mMatchRowHeight = 0;

    mutable QHash<int, QPixmap> mTypeGlyphs;
    mutable qreal mTypeGlyphRatio = 0.0;
};

} // namespace uiDesign

#endif // MUDLET_SEARCHRESULTDELEGATE_H
