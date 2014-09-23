/*
* Copyright (C) 2008-2012 J-P Nurmi <jpnurmi@gmail.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#include "historylineedit.h"
#include <QKeyEvent>

HistoryLineEdit::HistoryLineEdit(QWidget* parent)
    : FancyLineEdit(parent), index(0)
{
}

// QLineEdit doesn't like chars like '-', '/'..
// so we'll do it by hand
void HistoryLineEdit::cursorWordBackward(bool mark)
{
    // skip trailing whitespace
    while (cursorPosition() > 0 && text().at(cursorPosition() - 1).isSpace())
        cursorBackward(false, 1);

    // find previous whitespace
    int steps = cursorPosition();
    int idx = text().lastIndexOf(QRegExp("\\s"), steps - 1);

    // move cursor
    if (idx != -1)
        steps -= idx + 1;
    cursorBackward(mark, steps);
}

// QLineEdit::insert() emits textEdited()
// so here's the workaround
void HistoryLineEdit::insert(const QString& text)
{
    QString tmp = FancyLineEdit::text();
    int pos = cursorPosition();
    int len = selectedText().length();
    tmp.replace(pos, len, text);
    setText(tmp);
    setCursorPosition(pos + text.length() + 1);
}

void HistoryLineEdit::goBackward()
{
    if (index > 0)
        setText(history.value(--index));
}

void HistoryLineEdit::goForward()
{
    if (index < history.count())
        setText(history.value(++index));
    if (text().isEmpty())
        setText(input);
}

void HistoryLineEdit::clearHistory()
{
    index = 0;
    input.clear();
    history.clear();
}

void HistoryLineEdit::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            if (!text().isEmpty())
            {
                input.clear();
                history.append(text());
                index = history.count();
            }
            event->accept();
            break;
        case Qt::Key_Up:
            if (!text().isEmpty() && !history.contains(text()))
                input = text();
            goBackward();
            event->accept();
            return;
        case Qt::Key_Down:
            goForward();
            event->accept();
            return;
        default:
            break;
    }
    FancyLineEdit::keyPressEvent(event);
}
