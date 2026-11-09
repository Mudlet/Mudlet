#ifndef MUDLET_FLOWLAYOUT_H
#define MUDLET_FLOWLAYOUT_H

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

#include <QLayout>
#include <QList>
#include <QPointer>
#include <QStyle>

namespace uiDesign {

// Wraps rather than squeezing items or widening the window as a box layout would.
// The height depends on the width, so a holder must ask heightForWidth(); see ChipRow::sizeHint().
class FlowLayout : public QLayout
{
public:
    // WidestItem keeps a row inside a column from widening it by its item count
    enum class Width {
        OneLine,
        WidestItem,
    };

    enum class LineAlignment {
        Top,
        Centre,
    };

    Q_DISABLE_COPY(FlowLayout)
    // A spacing of -1 takes the style's layout spacing
    explicit FlowLayout(QWidget* pParent = nullptr, const int horizontalSpacing = -1, const int verticalSpacing = -1);
    ~FlowLayout() override;

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

    void setWidthPolicy(const Width policy);
    void setLineAlignment(const LineAlignment alignment);
    // Takes the spare room at the end of its line, but still wraps by its own width
    void setStretchWidget(QWidget* pWidget);

    [[nodiscard]] int horizontalSpacing() const;
    [[nodiscard]] int verticalSpacing() const;

private:
    // Measuring and placing share one walk, so a height never disagrees with the geometry given
    int runItems(const QRect& rect, const bool place) const;
    [[nodiscard]] int styleSpacing(const QStyle::PixelMetric metric) const;

    QList<QLayoutItem*> mItems;
    int mHorizontalSpacing = -1;
    int mVerticalSpacing = -1;
    Width mWidthPolicy = Width::OneLine;
    LineAlignment mLineAlignment = LineAlignment::Top;
    QPointer<QWidget> mpStretchWidget;
    // One layout pass asks heightForWidth() two or three times with the same width
    mutable int mCachedWidth = -1;
    mutable int mCachedHeight = 0;
};

} // namespace uiDesign

#endif // MUDLET_FLOWLAYOUT_H
