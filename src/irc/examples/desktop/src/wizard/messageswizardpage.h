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

#ifndef MESSAGESWIZARDPAGE_H
#define MESSAGESWIZARDPAGE_H

#include "ui_messageswizardpage.h"
#include "settings.h"

class MessagesWizardPage : public QWizardPage
{
    Q_OBJECT

public:
    MessagesWizardPage(QWidget* parent = 0);

    QHash<int, bool> messages() const;
    void setMessages(const QHash<int, bool>& messages);

    QHash<int, bool> highlights() const;
    void setHighlights(const QHash<int, bool>& highlights);

private:
    Ui::MessagesWizardPage ui;
};

#endif // MESSAGESWIZARDPAGE_H
