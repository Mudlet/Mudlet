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


#include "TAccessibleTextEdit.h"
#include "TConsole.h"
#include "mudlet.h"

TTextEdit* TAccessibleTextEdit::textEdit() const
{
    return static_cast<TTextEdit*>(object());
}

QAccessible::State TAccessibleTextEdit::state() const
{
    QAccessible::State s = QAccessibleWidget::state();
    s.selectableText = true;
    s.multiLine = true;
    s.focusable = true;
    s.marqueed = true; // The object displays scrolling contents, e.g. a log view.

    return s;
}

bool TAccessibleTextEdit::offsetIsInvalid(int offset) const
{
    // The end offset is the first character which isn't part of the range, so
    // it may be equal to characterCount().
    return offset < 0 || offset > characterCount();
}

int TAccessibleTextEdit::lineForOffset(int offset, int *lengthSoFar = nullptr) const
{
    const QStringList& lineBuffer = textEdit()->mpBuffer->lineBuffer;
    int lengthSoFar_ = 0;

    // If the offset is just past the end of the contents, consider it to be on
    // the last character.
    if (offset == characterCount()) {
        offset -= 1;
    }

    for (int i = 0; i < lineBuffer.length(); i++) {
        // The text() method adds a '\n' to the end of every line, so account
        // for it with the '+ 1' below.
        lengthSoFar_ += lineBuffer[i].length() + 1;

        if (offset < lengthSoFar_) {
            if (lengthSoFar != nullptr) {
                *lengthSoFar = lengthSoFar_ - lineBuffer[i].length() - 1;
            }
            return i;
        }
    }

    if (lengthSoFar != nullptr) {
        // The text() method doesn't add a '\n' to the end of the last line.
        *lengthSoFar = lengthSoFar_ - 1;
    }
    return lineBuffer.length();
}

int TAccessibleTextEdit::columnForOffset(int offset) const
{
    int lengthSoFar;

    // If the offset is just past the end of the contents, consider it to be on
    // the last character.
    if (offset == characterCount()) {
        offset -= 1;
    }

    lineForOffset(offset, &lengthSoFar);

    return offset - lengthSoFar;
}

/*
 * Returns a selection. The size of the selection is returned in startOffset
 * and endOffset. If there is no selection both startOffset and endOffset are 0.
 *
 * The accessibility APIs support multiple selections. For most widgets though,
 * only one selection is supported with selectionIndex equal to 0.
 */
void TAccessibleTextEdit::selection(int selectionIndex, int *startOffset, int *endOffset) const
{
    int startLine = textEdit()->mpConsole->P_begin.y();
    int startColumn = textEdit()->mpConsole->P_begin.x();
    int endLine = textEdit()->mpConsole->P_end.y();
    int endColumn = textEdit()->mpConsole->P_end.x();

    if ((startLine == endLine) && (startColumn == endColumn)) {
        return;
    }

    *startOffset = startLine * textEdit()->getColumnCount() + startColumn;
    *endOffset = endLine * textEdit()->getColumnCount() + endColumn;
}

/*
 * Returns the number of selections in this text.
 */
int TAccessibleTextEdit::selectionCount() const
{
    int startLine = textEdit()->mpConsole->P_begin.y();
    int startColumn = textEdit()->mpConsole->P_begin.x();
    int endLine = textEdit()->mpConsole->P_end.y();
    int endColumn = textEdit()->mpConsole->P_end.x();

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
void TAccessibleTextEdit::addSelection(int startOffset, int endOffset)
{
    qWarning("Unsupported TAccessibleTextEdit::addSelection");
}

/*
 * Clears the selection with index selectionIndex.
 */
void TAccessibleTextEdit::removeSelection(int selectionIndex)
{
    qWarning("Unsupported TAccessibleTextEdit::removeSelection");
}

/*
 * Set the selection selectionIndex to the range from startOffset to endOffset.
 *
 * See also selection(), addSelection(), and removeSelection().
 */
void TAccessibleTextEdit::setSelection(int selectionIndex, int startOffset, int endOffset)
{
    qWarning("Unsupported TAccessibleTextEdit::setSelection");
}

/*
 * Returns the current cursor position.
 *
 * See also setCursorPosition().
 */
int TAccessibleTextEdit::cursorPosition() const
{
    int ret = 0;
    int line = textEdit()->mCaretLine;
    int column = textEdit()->mCaretColumn;

    for (int i = 0; i < line; i++) {
        // The text() method adds a '\n' to the end of every line, so account
        // for it with the '+ 1' below.
        ret += textEdit()->mpBuffer->line(i).length() + 1;
    }

    ret += column;

    return ret;
}

/*
 * Moves the cursor to position.
 *
 * See also cursorPosition().
 */
void TAccessibleTextEdit::setCursorPosition(int position)
{
    if (offsetIsInvalid(position)) {
        return;
    }

    int line = lineForOffset(position);
    int column = columnForOffset(position);

    textEdit()->setCaretPosition(line, column);
}

QString TAccessibleTextEdit::text(QAccessible::Text t) const
{
    if (t != QAccessible::Value) {
        return QAccessibleWidget::text(t);
    }

    return textEdit()->mpBuffer->lineBuffer.join('\n');
}

/*
 * Returns the text from startOffset to endOffset. The startOffset is the
 * first character that will be returned. The endOffset is the first
 * character that will not be returned.
 */
QString TAccessibleTextEdit::text(int startOffset, int endOffset) const
{
    if (offsetIsInvalid(startOffset) || offsetIsInvalid(endOffset)) {
        return QString();
    }

    QString ret = text(QAccessible::Value).mid(startOffset, endOffset - startOffset);

    return ret;
}

/*
 * Returns the length of the text (total size including spaces).
 */
int TAccessibleTextEdit::characterCount() const
{
    return text(QAccessible::Value).length();
}

/*
 * Returns the position and size of the character at position offset in
 * screen coordinates.
 */
QRect TAccessibleTextEdit::characterRect(int offset) const
{
    if (offsetIsInvalid(offset)) {
        return QRect();
    }

    int row = lineForOffset(offset);
    int col = columnForOffset(offset);
    int fontWidth = QFontMetrics(textEdit()->mDisplayFont).averageCharWidth();
    int fontHeight = QFontMetrics(textEdit()->mDisplayFont).height();
    QPoint position = QPoint(col * fontWidth , row * fontHeight);
    position = textEdit()->mapToGlobal(position);

    return QRect(position, QSize(fontWidth, fontHeight));
}

/*
 * Returns the offset of the character at the point in screen coordinates.
 */
int TAccessibleTextEdit::offsetAtPoint(const QPoint &point) const
{
    qWarning("Unsupported TAccessibleTextEdit::offsetAtPoint");

    return 0;
}

/*
 * Ensures that the text between startIndex and endIndex is visible.
 */
void TAccessibleTextEdit::scrollToSubstring(int startIndex, int endIndex)
{
    qWarning("Unsupported TAccessibleTextEdit::scrollToSubstring");
}

/*
 * Returns the text attributes at the position offset. In addition the
 * range of the attributes is returned in startOffset and endOffset.
 */
QString TAccessibleTextEdit::attributes(int offset, int *startOffset, int *endOffset) const
{
    qWarning("Unsupported TAccessibleTextEdit::attributes");

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
QString TAccessibleTextEdit::textAfterOffset(int offset, QAccessible::TextBoundaryType boundaryType, int *startOffset, int *endOffset) const
{
    qWarning("Unsupported TAccessibleTextEdit::textAfterOffset");

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
QString TAccessibleTextEdit::textAtOffset(int offset, QAccessible::TextBoundaryType boundaryType, int *startOffset, int *endOffset) const
{
    qWarning("Unsupported TAccessibleTextEdit::textAtOffset");

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
QString TAccessibleTextEdit::textBeforeOffset(int offset, QAccessible::TextBoundaryType boundaryType, int *startOffset, int *endOffset) const
{
    qWarning("Unsupported TAccessibleTextEdit::textBeforeOffset");

    return QString();
}
