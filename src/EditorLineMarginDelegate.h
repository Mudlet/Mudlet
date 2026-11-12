#ifndef MUDLET_EDITORLINEMARGINDELEGATE_H
#define MUDLET_EDITORLINEMARGINDELEGATE_H

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

#include "edbee/views/components/textmargincomponent.h"

#include <QColor>
#include <QPixmap>
#include <QRectF>
#include <string>

namespace uiDesign {

// Marks the line a compile stopped on among the code pane's line numbers, with the trees' mark for a broken
// item and the ink of the note on the code heading. The room for the mark is kept whether a line is marked or
// not, so the code does not move sideways when a save fails.
class EditorLineMarginDelegate : public edbee::TextMarginComponentDelegate
{
public:
    EditorLineMarginDelegate() = default;

    // Counted from one, as Lua and the heading count; zero marks nothing
    void setErrorLine(int line);
    int errorLine() const { return mErrorLine; }
    // The line drawn marked, counted from zero: past the end of the text, as for a block left open, it is the last
    size_t markedLine();
    QColor errorInk();
    // Where the mark was last painted, in the margin's own coordinates
    QRectF markRect() const { return mMarkRect; }

    QString lineText(size_t line) override;
    int widthBeforeLineNumber() override;
    void renderAfter(QPainter* painter, size_t startLine, size_t endLine, int width) override;

private:
    int markSize();
    QPixmap markGlyph(const QColor& ink, int size, qreal ratio);

    int mErrorLine = 0;
    QRectF mMarkRect;
    QPixmap mMarkGlyph;
    QString mMarkGlyphKey;
};

} // namespace uiDesign

#endif // MUDLET_EDITORLINEMARGINDELEGATE_H
