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

#ifndef CONNECTIONWIZARDPAGE_H
#define CONNECTIONWIZARDPAGE_H

#include "ui_connectionwizardpage.h"

class ConnectionWizardPage : public QWizardPage
{
    Q_OBJECT

public:
    ConnectionWizardPage(QWidget* parent = 0);

    QString connectionName() const;
    void setConnectionName(const QString& name);

private:
    Ui::ConnectionWizardPage ui;
};

#endif // CONNECTIONWIZARDPAGE_H
