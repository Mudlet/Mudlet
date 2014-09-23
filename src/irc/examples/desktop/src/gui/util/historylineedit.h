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

#ifndef HISTORYLINEEDIT_H
#define HISTORYLINEEDIT_H

#include "fancylineedit.h"

class HistoryLineEdit : public Utils::FancyLineEdit
{
    Q_OBJECT

public:
    explicit HistoryLineEdit(QWidget* parent = 0);

    void cursorWordBackward(bool mark);
    void insert(const QString& text);

public slots:
    void goBackward();
    void goForward();
    void clearHistory();

protected:
    void keyPressEvent(QKeyEvent* event);

private:
    int index;
    QString input;
    QStringList history;
};

#endif // HISTORYLINEEDIT_H
