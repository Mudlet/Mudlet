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

#include "completer.h"
#include "abstractsessionitem.h"
#include <QTextBoundaryFinder>

Completer::Completer(QObject *parent) : QObject(parent), m_item(0), m_current(0)
{
}

QObject* Completer::modelItem() const
{
    return m_item;
}

void Completer::setModelItem(QObject* item)
{
    m_item = static_cast<AbstractSessionItem*>(item);
}

void Completer::complete(const QString& text, int selStart, int selEnd)
{
    if (text.isEmpty())
        return;

    int wordStart = 0, wordEnd = 0;
    QString word = findWord(text, selStart, selEnd, &wordStart, &wordEnd);
    QString prefix;
    if (wordStart > 0)
        prefix = text.mid(0, wordStart);

    QStringList candidates = m_item->completions(prefix, word);
    if (m_candidates != candidates)
    {
        m_current = 0;
        m_candidates = candidates;
    }
    else
    {
        if (!m_candidates.isEmpty())
            m_current = (m_current + 1) % m_candidates.count();
        else
            m_current = 0;
    }

    if (!m_candidates.isEmpty())
    {
        QString replace = m_candidates.value(m_current);

        if (word == "/")
            ++wordStart;

        QString completion = text;
        completion.replace(wordStart, selEnd - wordStart, replace);
        emit completed(completion, selStart, wordStart + replace.length());
    }
}

QString Completer::findWord(const QString& text, int selStart, int selEnd, int* wordStart, int* wordEnd)
{
    QTextBoundaryFinder finder(QTextBoundaryFinder::Word, text);
    finder.setPosition(selStart);
    if (!finder.isAtBoundary() || finder.boundaryReasons() & QTextBoundaryFinder::EndWord)
        finder.toPreviousBoundary();

    *wordStart = finder.position();
    *wordEnd = (selStart == selEnd) ? finder.toNextBoundary() : selStart;
    return text.mid(*wordStart, *wordEnd - *wordStart);
}
