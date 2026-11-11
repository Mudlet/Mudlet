#ifndef MUDLET_GRIPSPLITTER_H
#define MUDLET_GRIPSPLITTER_H

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

#include "utils.h"

#include <QPointer>
#include <QSplitter>
#include <QSplitterHandle>

class QMouseEvent;
class QPainter;

namespace uiDesign {

struct ThemeTokens;

class GripSplitterHandle : public QSplitterHandle
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(GripSplitterHandle)
    GripSplitterHandle(const Qt::Orientation orientation, QSplitter* pParent);

    // The content is made transparent to the mouse, so a drag anywhere on it
    // still resizes.
    void setContent(QWidget* pContent);

    void setResizes(const bool resizes);
    [[nodiscard]] bool resizes() const { return mResizes; }

    // The content takes no mouse events, so the handle detects clicks on pPiece
    // itself: a press that never travels startDragDistance() emits clicked().
    void setClickable(QWidget* pPiece);

    [[nodiscard]] QSize sizeHint() const override;

signals:
    void clicked(QWidget* pPiece);

protected:
    bool event(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintSeam(QPainter& painter, const ThemeTokens& tokens) const;
    void resizeEvent(QResizeEvent* event) override;
    void enterEvent(TEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    [[nodiscard]] bool overClickable(const QPoint& point) const;
    void placeCursor(const QPoint& point);

    QPointer<QWidget> mpContent;
    QPointer<QWidget> mpClickable;
    QPoint mPressedAt;
    // A drag that wandered off and came back releases where it started, which
    // the release point alone would read as a click
    bool mDragged = false;
    bool mHovered = false;
    bool mResizes = true;
};

class GripSplitter : public QSplitter
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(GripSplitter)
    // Public because QSplitter::restoreState() also restores the serialized
    // handle width, so whoever restores has to set this again.
    static constexpr int scmHandleThickness = 9;

    explicit GripSplitter(QWidget* pParent = nullptr);
    GripSplitter(const Qt::Orientation orientation, QWidget* pParent = nullptr);

    // The handle above the widget at `index` exists only once that widget does,
    // so call this after the panes have been added.
    void setHeaderHandle(const int index, QWidget* pContent);

    // Returns false where there is no handle above the widget at `index`.
    bool setHandleResizes(const int index, const bool resizes);
    [[nodiscard]] bool handleResizes(const int index) const;

protected:
    QSplitterHandle* createHandle() override;
};

} // namespace uiDesign

#endif // MUDLET_GRIPSPLITTER_H
