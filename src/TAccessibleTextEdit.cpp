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


#include <QTextBoundaryFinder>
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
    int lengthSoFar = 0;

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
    if (selectionIndex != 0) {
        *startOffset = *endOffset = 0;
        return;
    }

    TTextEdit* edit = textEdit();

    if (edit->mSelectedRegion == QRegion(0, 0, 0, 0)) {
        *startOffset = *endOffset = 0;
        return;
    }

    *startOffset = offsetForPosition(edit->mPA.y(), edit->mPA.x());
    *endOffset = offsetForPosition(edit->mPB.y(), edit->mPB.x());
}

/*
 * Returns the number of selections in this text.
 */
int TAccessibleTextEdit::selectionCount() const
{
    return textEdit()->mSelectedRegion != QRegion(0, 0, 0, 0);
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
    if (offsetIsInvalid(startOffset) || offsetIsInvalid(endOffset)) {
        return;
    }

    if (startOffset > endOffset) {
        std::swap(startOffset, endOffset);
    }

    TTextEdit *edit = textEdit();
    edit->mPA.setX(columnForOffset(startOffset));
    edit->mPA.setY(lineForOffset(startOffset));
    edit->mDragStart = edit->mPA;
    edit->mPB.setX(columnForOffset(endOffset - 1));
    edit->mPB.setY(lineForOffset(endOffset - 1));
    edit->mDragSelectionEnd = edit->mPB;

    textEdit()->highlightSelection();
}

/*
 * Clears the selection with index selectionIndex.
 */
void TAccessibleTextEdit::removeSelection(int selectionIndex)
{
    if (selectionIndex != 0) {
        return;
    }

    textEdit()->unHighlight();
    textEdit()->mSelectedRegion = QRegion(0, 0, 0, 0);
}

/*
 * Set the selection selectionIndex to the range from startOffset to endOffset.
 *
 * See also selection(), addSelection(), and removeSelection().
 */
void TAccessibleTextEdit::setSelection(int selectionIndex, int startOffset, int endOffset)
{
    if (selectionIndex != 0) {
        return;
    }

    addSelection(startOffset, endOffset);
}

/*
 * Convert from line, column coordinates to character offset.
 */
int TAccessibleTextEdit::offsetForPosition(int line, int column) const
{
    int ret = 0;

    for (int i = 0; i < line; i++) {
        // The text() method adds a '\n' to the end of every line, so account
        // for it with the '+ 1' below.
        ret += textEdit()->mpBuffer->line(i).length() + 1;
    }

    ret += column;

    return ret;
}

/*
 * Returns the current cursor position.
 *
 * See also setCursorPosition().
 */
int TAccessibleTextEdit::cursorPosition() const
{
    return offsetForPosition(textEdit()->mCaretLine, textEdit()->mCaretColumn);
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

bool TAccessibleTextEdit::lineIsVisible(int line) const
{
    TTextEdit* edit = textEdit();
    int topLine = edit->imageTopLine();

    return line >= topLine && line < topLine + edit->mScreenHeight;
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
    TTextEdit* edit = textEdit();
    int topLine = edit->imageTopLine();

    // Check whether the character is visible.
    if (!lineIsVisible(row)) {
        return QRect();
    }

    int fontWidth = edit->mFontWidth;
    int fontHeight = edit->mFontHeight;
    QPoint position = edit->mapToGlobal(QPoint(col * fontWidth, (row - topLine) * fontHeight));

    return QRect(position, QSize(fontWidth, fontHeight));
}

/*
 * Returns the offset of the character at the point in screen coordinates.
 */
int TAccessibleTextEdit::offsetAtPoint(const QPoint& point) const
{
    TTextEdit* edit = textEdit();
    QPoint local = edit->mapFromGlobal(point);
    int line = edit->imageTopLine() + local.y() / edit->mFontHeight;
    int column = local.x() / edit->mFontWidth;

    return offsetForPosition(line, column);
}

/*
 * Ensures that the text between startIndex and endIndex is visible.
 */
void TAccessibleTextEdit::scrollToSubstring(int startIndex, int endIndex)
{
    int startLine = lineForOffset(startIndex);
    int endLine = lineForOffset(endIndex);
    TTextEdit* edit = textEdit();

    if (!lineIsVisible(startLine)) {
        edit->scrollTo(startLine);
    }

    if (!lineIsVisible(endLine)) {
        edit->scrollTo(endLine);
    }
}

/*
 * Returns the text attributes at the position offset. In addition the
 * range of the attributes is returned in startOffset and endOffset.
 */
QString TAccessibleTextEdit::attributes(int offset, int *startOffset, int *endOffset) const
{
    // IAccessible2 defines -1 as length and -2 as cursor position.
    if (offset == -2) {
        offset = cursorPosition();
    }

    const int charCount = characterCount();

    // -1 doesn't make much sense here, but it's better to return something.
    // Screen readers may ask for text attributes at the cursor position which
    // may be equal to length.
    if (offset == -1 || offset == charCount) {
        offset = charCount - 1;
    }

    if (offset < 0 || offset > charCount) {
        *startOffset = -1;
        *endOffset = -1;
        return QString();
    }

    QString ret = QString();

    const QFont font = textEdit()->font();

    QString family = font.family();
    if (!family.isEmpty()) {
        family = family.replace('\\', QLatin1String("\\\\"));
        family = family.replace(':', QLatin1String("\\:"));
        family = family.replace(',', QLatin1String("\\,"));
        family = family.replace('=', QLatin1String("\\="));
        family = family.replace(';', QLatin1String("\\;"));
        family = family.replace('\"', QLatin1String("\\\""));
        ret += "font-family:" + QLatin1Char('"') + family + QLatin1Char('"') + ";";
    }

    const int fontSize = static_cast<int>(font.pointSize());
    if (fontSize) {
        ret += "font-size:" + QString::fromLatin1("%1pt").arg(fontSize) + ";";
    }

    const int line = lineForOffset(offset);
    const int column = columnForOffset(offset);
    const QFont::Style style = font.style();
    const TChar &charStyle = textEdit()->mpBuffer->buffer.at(line).at(column);
    // IAccessible2's text attributes don't support the overline attribute.
    const TChar::AttributeFlags attributes = charStyle.allDisplayAttributes();
    const bool isBold = (attributes & TChar::Bold) || font.weight() > QFont::Normal;
    const bool isItalics = (attributes & TChar::Italic) || style == QFont::StyleItalic;
    const bool isStrikeOut = attributes & TChar::StrikeOut;
    const bool isUnderline = attributes & TChar::Underline;
    const bool isReverse = attributes & TChar::Reverse;
    const bool caretIsHere = mudlet::self()->isCaretModeEnabled() && textEdit()->mCaretLine == line &&
        textEdit()->mCaretColumn == column;

    // Different weight values are not handled.
    if (isBold) {
        ret += "font-weight:bold;";
    }
    if (isItalics || style != QFont::StyleNormal) {
        ret += "font-style:" + QString::fromLatin1(isItalics ? "italic;" : "oblique;");
    }
    if (isStrikeOut) {
        ret += "text-line-through-type:single;";
    }
    if (isUnderline) {
        ret += "text-underline-type:single;";
    }

    QColor fgColor;
    QColor bgColor;
    if (isReverse != (charStyle.isSelected() != caretIsHere)) {
        fgColor = charStyle.background();
        bgColor = charStyle.foreground();
    } else {
        fgColor = charStyle.foreground();
        bgColor = charStyle.background();
    }
    ret += QString::fromLatin1("color:rgb(%1,%2,%3);").arg(fgColor.red()).arg(fgColor.green()).arg(fgColor.blue());
    ret += QString::fromLatin1("background-color:rgb(%1,%2,%3);").arg(bgColor.red()).arg(bgColor.green()).arg(bgColor.blue());

    return ret;
}

/*
 * Auxiliary function for textAtOffset() and textAfterOffset().
 */
QString TAccessibleTextEdit::textAroundOffset(TAccessibleTextEdit::TextOp op, int offset,
                                              QAccessible::TextBoundaryType boundaryType,
                                              int* startOffset, int* endOffset) const
{
    // There's no code overlap between the text*Offset() methods in this case, so
    // NoBoundary is always handled by the caller.
    Q_ASSERT_X(boundaryType != QAccessible::TextBoundaryType::NoBoundary,
               "TAccessibleTextEdit::textAroundOffset", "The caller should have handled NoBoundary.");

    if (boundaryType != QAccessible::TextBoundaryType::CharBoundary &&
        boundaryType != QAccessible::TextBoundaryType::WordBoundary &&
        boundaryType != QAccessible::TextBoundaryType::SentenceBoundary &&
        boundaryType != QAccessible::TextBoundaryType::LineBoundary) {
        *startOffset = *endOffset = -1;
        return QString();
    }

    if (offsetIsInvalid(offset)) {
        *startOffset = *endOffset = -1;
        return QString();
    }

    const QString contents = text(QAccessible::Value);

    if (boundaryType == QAccessible::TextBoundaryType::WordBoundary ||
        boundaryType == QAccessible::TextBoundaryType::SentenceBoundary) {
        QTextBoundaryFinder::BoundaryType type = boundaryType == QAccessible::TextBoundaryType::WordBoundary ?
            QTextBoundaryFinder::BoundaryType::Word : QTextBoundaryFinder::BoundaryType::Sentence;
        QTextBoundaryFinder finder = QTextBoundaryFinder(type, contents);
        int start = 0, end = 0;

        finder.setPosition(offset);

        if (op == TAccessibleTextEdit::BeforeOffset) {
            end = finder.toPreviousBoundary();
            start = finder.toPreviousBoundary();
        } else if (op == TAccessibleTextEdit::AtOffset) {
            start = finder.toPreviousBoundary();
            end = finder.toNextBoundary();
        } else {
            start = finder.toNextBoundary();
            end = finder.toNextBoundary();
        }

        if (start == -1 || end == -1) {
            // The documentation doesn't say what to put in startOffset and
            // endOffset in this case.
            *startOffset = *endOffset = 0;
            return QString();
        }

        *startOffset = start;
        *endOffset = end;

        return contents.mid(start, end - start);
    }

    QString ret;
    TBuffer *buffer = textEdit()->mpBuffer;

    if (boundaryType == QAccessible::TextBoundaryType::CharBoundary) {
        if (op == TAccessibleTextEdit::BeforeOffset) {
            offset -= 1;
        } else if (op == TAccessibleTextEdit::AfterOffset) {
            offset += 1;
        }

        if (offset < 0 || offset >= contents.length()) {
            // The documentation doesn't say what to put in startOffset and
            // endOffset in this case.
            *startOffset = *endOffset = 0;
            return QString();
        }

        int lineNum = lineForOffset(offset);
        QString line(buffer->line(lineNum));

        ret = QString(line[columnForOffset(offset)]);
        *startOffset = offset;
        *endOffset = offset + 1;
    } else {
        // LineBoundary

        int lineNum = lineForOffset(offset);

        if (op == TAccessibleTextEdit::BeforeOffset) {
            lineNum -= 1;
        } else if (op == TAccessibleTextEdit::AfterOffset) {
            lineNum += 1;
        }

        if (lineNum < 0 || lineNum > buffer->getLastLineNumber()) {
            // The documentation doesn't say what to put in startOffset and
            // endOffset in this case.
            *startOffset = *endOffset = 0;
            return QString();
        }

        QString line(buffer->line(lineNum));

        // Materialize the implicit '\n' at the end of every line.
        ret = line + "\n";

        const QStringList& lineBuffer = buffer->lineBuffer;
        *startOffset = 0;
        for (int i = 0; i < lineNum; i++) {
            // The text() method adds a '\n' to the end of every line, so account
            // for it with the '+ 1' below.
            *startOffset += lineBuffer[i].length() + 1;
        }

        *endOffset = *startOffset + ret.length();
    }

    return ret;
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
    // This is the simplest case to implement, so get it over with now.
    if (boundaryType == QAccessible::TextBoundaryType::NoBoundary) {
        *startOffset = offset;
        *endOffset = characterCount();

        return text(QAccessible::Value).mid(offset);
    }

    return textAroundOffset(TAccessibleTextEdit::AfterOffset, offset, boundaryType, startOffset, endOffset);
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
QString TAccessibleTextEdit::textAtOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                                         int *startOffset, int *endOffset) const
{
    // This is the simplest case to implement, so get it over with now.
    if (boundaryType == QAccessible::TextBoundaryType::NoBoundary) {
        // TODO: Confirm that this is indeed the expected behavior.
        *startOffset = 0;
        *endOffset = characterCount();

        return text(QAccessible::Value);
    }

    return textAroundOffset(TAccessibleTextEdit::AtOffset, offset, boundaryType, startOffset, endOffset);
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
    // This is the simplest case to implement, so get it over with now.
    if (boundaryType == QAccessible::TextBoundaryType::NoBoundary) {
        *startOffset = 0;
        *endOffset = offset;

        return text(QAccessible::Value).left(offset);
    }

    return textAroundOffset(TAccessibleTextEdit::BeforeOffset, offset, boundaryType, startOffset, endOffset);
}
