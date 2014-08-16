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

#ifndef LINEEDITOR_H
#define LINEEDITOR_H

#include "historylineedit.h"
class Completer;

class LineEditor : public HistoryLineEdit
{
    Q_OBJECT

public:
    LineEditor(QWidget* parent = 0);

    Completer* completer() const;

signals:
    void send(const QString& text);
    void typed(const QString& text);

private slots:
    void onSend();

private:
    struct Private
    {
        Completer* completer;
    } d;
};

#endif // LINEEDITOR_H
