/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       *
 *   Copyright (C) 2014-2020 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2022 by Thiago Jung Bauermann - bauermann@kolabnow.com  *
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


#include "TAccessibleConsole.h"

TConsole* TAccessibleConsole::display() const
{
    return static_cast<TConsole*>(object());
}

QAccessibleInterface* TAccessibleConsole::childAt(int x, int y) const
{
    return 0;
}

int TAccessibleConsole::childCount() const
{
    return 0;
}

int TAccessibleConsole::indexOfChild(const QAccessibleInterface *child) const
{
    return -1;
}

QAccessible::Role TAccessibleConsole::role() const
{
    return QAccessible::StaticText;
}

QAccessible::State TAccessibleConsole::state() const
{
    QAccessible::State s = QAccessibleWidget::state();
    s.selectableText = true;
    s.multiLine = true;
    s.focusable = true;

    return s;
}

int TAccessibleConsole::lineForOffset(int offset) const
{
    return offset / display()->mLowerPane->getColumnCount();
}

int TAccessibleConsole::columnForOffset(int offset) const
{
    return offset % display()->mLowerPane->getColumnCount();
}

/*
 * Returns a selection. The size of the selection is returned in startOffset
 * and endOffset. If there is no selection both startOffset and endOffset are 0.
 *
 * The accessibility APIs support multiple selections. For most widgets though,
 * only one selection is supported with selectionIndex equal to 0.
 */
void TAccessibleConsole::selection(int selectionIndex, int *startOffset, int *endOffset) const
{
    int startLine = display()->P_begin.y();
    int startColumn = display()->P_begin.x();
    int endLine = display()->P_end.y();
    int endColumn = display()->P_end.x();

    if ((startLine == endLine) && (startColumn == endColumn)) {
        return;
    }

    *startOffset = startLine * display()->mLowerPane->getColumnCount() + startColumn;
    *endOffset = endLine * display()->mLowerPane->getColumnCount() + endColumn;
}

/*
 * Returns the number of selections in this text.
 */
int TAccessibleConsole::selectionCount() const
{
    int startLine = display()->P_begin.y();
    int startColumn = display()->P_begin.x();
    int endLine = display()->P_end.y();
    int endColumn = display()->P_end.x();

    int ret = ((startLine == endLine) && (startColumn == endColumn)) ? 0 : 1;

    return ret;
}

/*
 * Select the text from startOffset to endOffset. The startOffset is the
 * first character that will be selected. The endOffset is the first
 * character that will not be selected.
 *
 * When the object supports multiple selections (e.g. in a word processor),
 * this adds a new selection, otherwise it replaces the previous selection.
 *
 * The selection will be endOffset - startOffset characters long.
 */
void TAccessibleConsole::addSelection(int startOffset, int endOffset)
{
    qWarning() << QStringLiteral("Unsupported QAccessibleTextInterface::addSelection");
}

/*
 * Clears the selection with index selectionIndex.
 */
void TAccessibleConsole::removeSelection(int selectionIndex)
{
    qWarning() << QStringLiteral("Unsupported QAccessibleTextInterface::removeSelection");
}

/*
 * Set the selection selectionIndex to the range from startOffset to endOffset.
 *
 * See also selection(), addSelection(), and removeSelection().
 */
void TAccessibleConsole::setSelection(int selectionIndex, int startOffset, int endOffset)
{
    qWarning() << QStringLiteral("Unsupported QAccessibleTextInterface::setSelection");
}

/*
 * Returns the current cursor position.
 *
 * See also setCursorPosition().
 */
int TAccessibleConsole::cursorPosition() const
{
    int offset = display()->mLowerPane->getColumnCount() * display()->mUserCursor.y();
    int ret = offset + display()->mUserCursor.x();

    return ret;
}

/*
 * Moves the cursor to position.
 *
 * See also cursorPosition().
 */
void TAccessibleConsole::setCursorPosition(int position)
{
    qWarning() << QStringLiteral("Unsupported QAccessibleTextInterface::setCursorPosition");
}

/*
 * Returns the text from startOffset to endOffset. The startOffset is the
 * first character that will be returned. The endOffset is the first
 * character that will not be returned.
 */
QString TAccessibleConsole::text(int startOffset, int endOffset) const
{
    QString line(display()->buffer.line(display()->mUserCursor.y()));
    QString ret(line.mid(startOffset, endOffset - startOffset));

    return ret;
}

/*
 * Returns the length of the text (total size including spaces).
 */
int TAccessibleConsole::characterCount() const
{
    int ret = display()->getLineCount() * display()->mLowerPane->getColumnCount();

    return ret;
}

/*
 * Returns the position and size of the character at position offset in
 * screen coordinates.
 */
QRect TAccessibleConsole::characterRect(int offset) const
{
    int row = lineForOffset(offset);
    int col = offset - row * display()->mLowerPane->getColumnCount();
    int fontWidth = QFontMetrics(display()->mDisplayFont).averageCharWidth();
    int fontHeight = QFontMetrics(display()->mDisplayFont).height();
    QPoint position = QPoint(col * fontWidth , row * fontHeight);
    position = display()->mapToGlobal(position);

    return QRect(position, QSize(fontWidth, fontHeight));
}

/*
 * Returns the offset of the character at the point in screen coordinates.
 */
int TAccessibleConsole::offsetAtPoint(const QPoint &point) const
{
    qWarning() << QStringLiteral("Unsupported QAccessibleTextInterface::offsetAtPoint");

    return 0;
}

/*
 * Ensures that the text between startIndex and endIndex is visible.
 */
void TAccessibleConsole::scrollToSubstring(int startIndex, int endIndex)
{
    qWarning() << QStringLiteral("Unsupported QAccessibleTextInterface::scrollToSubstring");
}

/*
 * Returns the text attributes at the position offset. In addition the
 * range of the attributes is returned in startOffset and endOffset.
 */
QString TAccessibleConsole::attributes(int offset, int *startOffset, int *endOffset) const
{
    qWarning() << QStringLiteral("Unsupported QAccessibleTextInterface::attributes");

    return QString();
}

/*
 * Returns the text item of type boundaryType that is right after offset
 * offset and sets startOffset and endOffset values to the start and end
 * positions of that item; returns an empty string if there is no such an
 * item. Sets startOffset and endOffset values to -1 on error.
 *
 * This default implementation is provided for small text edits. A word
 * processor or text editor should provide their own efficient
 * implementations. This function makes no distinction between
 * paragraphs and lines.
 *
 * Note: this function can not take the cursor position into account. By
 * convention an offset of -2 means that this function should use the
 * cursor position as offset. Thus an offset of -2 must be converted to
 * the cursor position before calling this function. An offset of -1 is
 * used for the text length and custom implementations of this function
 * have to return the result as if the length was passed in as offset.
 */
QString TAccessibleConsole::textAfterOffset(int offset, QAccessible::TextBoundaryType boundaryType, int *startOffset, int *endOffset) const
{
    qWarning() << QStringLiteral("Unsupported QAccessibleTextInterface::textAfterOffset");

    return QString();
}

/*
 * Returns the text item of type boundaryType at offset offset and sets
 * startOffset and endOffset values to the start and end positions of that
 * item; returns an empty string if there is no such an item. Sets
 * startOffset and endOffset values to -1 on error.
 *
 * This default implementation is provided for small text edits. A word
 * processor or text editor should provide their own efficient
 * implementations. This function makes no distinction between paragraphs
 * and lines.
 *
 * Note: this function can not take the cursor position into account. By
 * convention an offset of -2 means that this function should use the
 * cursor position as offset. Thus an offset of -2 must be converted to
 * the cursor position before calling this function. An offset of -1 is
 * used for the text length and custom implementations of this function
 * have to return the result as if the length was passed in as offset.
 */
QString TAccessibleConsole::textAtOffset(int offset, QAccessible::TextBoundaryType boundaryType, int *startOffset, int *endOffset) const
{
    qWarning() << QStringLiteral("Unsupported QAccessibleTextInterface::textAtOffset");

    return QString();
}

/*
 * Returns the text item of type boundaryType that is close to offset
 * offset and sets startOffset and endOffset values to the start and end
 * positions of that item; returns an empty string if there is no such an
 * item. Sets startOffset and endOffset values to -1 on error.
 *
 * This default implementation is provided for small text edits. A word
 * processor or text editor should provide their own efficient
 * implementations. This function makes no distinction between paragraphs
 * and lines.
 *
 * Note: this function can not take the cursor position into account. By
 * convention an offset of -2 means that this function should use the
 * cursor position as offset. Thus an offset of -2 must be converted to
 * the cursor position before calling this function. An offset of -1 is
 * used for the text length and custom implementations of this function
 * have to return the result as if the length was passed in as offset.
 */
QString TAccessibleConsole::textBeforeOffset(int offset, QAccessible::TextBoundaryType boundaryType, int *startOffset, int *endOffset) const
{
    qWarning() << QStringLiteral("Unsupported QAccessibleTextInterface::textBeforeOffset");

    return QString();
}
