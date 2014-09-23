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

#ifndef COMPLETER_H
#define COMPLETER_H

#include <QObject>
#include <QStringList>
class AbstractSessionItem;

class Completer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject* modelItem READ modelItem WRITE setModelItem)

public:
    explicit Completer(QObject *parent = 0);

    QObject* modelItem() const;
    void setModelItem(QObject* item);

public slots:
    void complete(const QString& text, int selStart, int selEnd);

signals:
    void completed(const QString& text, int selStart, int selEnd);

private:
    static QString findWord(const QString& text, int selStart, int selEnd, int* wordStart, int* wordEnd);

    AbstractSessionItem* m_item;
    QStringList m_candidates;
    int m_current;
};

#endif // COMPLETER_H
