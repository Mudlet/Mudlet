#ifndef MUDLET_PAIRGRIDLAYOUT_H
#define MUDLET_PAIRGRIDLAYOUT_H

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

#include <QLayout>
#include <QList>

namespace uiDesign {

// Pairs of a label and the control it names, two to a row the way a form grid
// lays them - label, control, label, control - with every control stretched to
// the one width, so the controls line up down the column whichever side they
// stand on. Where two pairs no longer fit, it lays one pair to a row rather
// than widening whatever holds it; a pair never splits. A row can instead
// carry one widget across the whole width (a divider, a sentence), and the
// last row a footer: a widget under each half.
//
// The height therefore depends on the width, so this answers heightForWidth()
// and the widget carrying it has to say so in its size policy.
//
// Read down rather than across, the pairs between two full-width rows are two
// columns, the first half of them (or as many as setLeadingPairs() says) leading
// and the rest trailing, each column as wide as its own controls need; one pair
// to a row, the trailing column's pairs follow the leading column's whole rather
// than taking turns with them.
class PairGridLayout : public QLayout
{
public:
    enum class ReadingOrder { AcrossRows, DownColumns };
    // Automatic decides from the width the layout is given. A caller deciding for itself, from a width of its own and
    // with a margin against flipping, gives the answer here; sizeHint().width() is what two pairs to a row need.
    enum class PairsPerRow { Automatic, Two, One };

    Q_DISABLE_COPY(PairGridLayout)
    // A spacing of -1 takes the style's own idea of what goes between two
    // controls, as a QGridLayout the caller says nothing about does
    explicit PairGridLayout(QWidget* pParent = nullptr, const int horizontalSpacing = -1, const int verticalSpacing = -1);
    ~PairGridLayout() override;

    void addPair(QWidget* pLabel, QWidget* pControl);
    void addSpanningWidget(QWidget* pWidget);
    // Under the leading pair and the trailing one; either may be null
    void addFooter(QWidget* pLeading, QWidget* pTrailing);
    void setReadingOrder(const ReadingOrder order);
    // Read down, how many of a block's pairs the leading column holds; -1 gives it the first half
    void setLeadingPairs(const qsizetype pairs);
    void setPairsPerRow(const PairsPerRow pairsPerRow);
    [[nodiscard]] PairsPerRow pairsPerRow() const { return mPairsPerRow; }
    // Between the leading pair and the trailing one, two to a row; -1 takes the horizontal spacing
    void setPairSpacing(const int spacing);

    // Takes the item as a row across the whole width
    void addItem(QLayoutItem* pItem) override;
    [[nodiscard]] int count() const override;
    [[nodiscard]] QLayoutItem* itemAt(int index) const override;
    QLayoutItem* takeAt(int index) override;
    [[nodiscard]] Qt::Orientations expandingDirections() const override;
    [[nodiscard]] bool hasHeightForWidth() const override;
    [[nodiscard]] int heightForWidth(int width) const override;
    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSize() const override;
    void setGeometry(const QRect& rect) override;
    void invalidate() override;

    [[nodiscard]] int horizontalSpacing() const;
    [[nodiscard]] int verticalSpacing() const;
    [[nodiscard]] int pairSpacing() const;

private:
    enum class RowKind {
        Pair,
        Spanning,
        Footer,
    };
    struct Row
    {
        RowKind kind = RowKind::Spanning;
        QLayoutItem* pFirst = nullptr;
        QLayoutItem* pSecond = nullptr;
    };
    // What the columns come to: the labels and the controls of either side.
    // Read across, every control is given the one width.
    struct Columns
    {
        int leadingLabel = 0;
        int trailingLabel = 0;
        int leadingControl = 0;
        int trailingControl = 0;
    };

    void addRow(const RowKind kind, QWidget* pFirst, QWidget* pSecond);
    // The shown pairs between two rows of any other kind, in the order added
    [[nodiscard]] QList<QList<const Row*>> pairBlocks() const;
    // How many of a block's pairs lead, two pairs to a row
    [[nodiscard]] qsizetype leadingCount(const qsizetype pairs) const;
    // The columns the pairs alone come to
    [[nodiscard]] Columns pairColumns(const bool twoPairs) const;
    [[nodiscard]] bool footersFit(const Columns& columns, const bool twoPairs) const;
    // ...and widened where a footer needs more room than they leave it
    [[nodiscard]] Columns narrowestColumns(const bool twoPairs) const;
    [[nodiscard]] int narrowestContentWidth(const bool twoPairs) const;
    // As FlowLayout's runItems(): laying the rows out and measuring what that
    // comes to are the same walk
    int runRows(const QRect& rect, const bool place) const;
    [[nodiscard]] int styleSpacing(const Qt::Orientation orientation) const;

    QList<QLayoutItem*> mItems;
    QList<Row> mRows;
    ReadingOrder mReadingOrder = ReadingOrder::AcrossRows;
    qsizetype mLeadingPairs = -1;
    PairsPerRow mPairsPerRow = PairsPerRow::Automatic;
    int mHorizontalSpacing = -1;
    int mVerticalSpacing = -1;
    int mPairSpacing = -1;
    mutable int mCachedWidth = -1;
    mutable int mCachedHeight = 0;
};

} // namespace uiDesign

#endif // MUDLET_PAIRGRIDLAYOUT_H
