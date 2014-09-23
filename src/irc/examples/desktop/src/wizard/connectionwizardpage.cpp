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

#include "connectionwizardpage.h"
#include <QCompleter>
#include <QSettings>

ConnectionWizardPage::ConnectionWizardPage(QWidget* parent) : QWizardPage(parent)
{
    ui.setupUi(this);
    setPixmap(QWizard::LogoPixmap, QPixmap(":/resources/oxygen/64x64/actions/save_all.png"));

    QSettings settings;
    QStringList names = settings.value("connectionNames").toStringList();

    QCompleter* completer = new QCompleter(names, ui.lineEditName);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    ui.lineEditName->setCompleter(completer);
}

QString ConnectionWizardPage::connectionName() const
{
    return ui.lineEditName->text();
}

void ConnectionWizardPage::setConnectionName(const QString& name)
{
    ui.lineEditName->setText(name);
}
