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

#ifndef SEARCHEDITOR_H
#define SEARCHEDITOR_H

#include "historylineedit.h"
class QTextEdit;

class SearchEditor : public HistoryLineEdit
{
    Q_OBJECT

public:
    SearchEditor(QWidget* parent = 0);

    QTextEdit* textEdit() const;
    void setTextEdit(QTextEdit* textEdit);

public slots:
    void find();
    void findNext();
    void findPrevious();

protected slots:
    void find(const QString& text, bool forward = false, bool backward = false);

private:
    struct Private
    {
        QTextEdit* textEdit;
    } d;
};

#endif // SEARCHEDITOR_H
