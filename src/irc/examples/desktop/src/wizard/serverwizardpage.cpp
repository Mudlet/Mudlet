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

#include "serverwizardpage.h"
#include <QCompleter>
#include <QSettings>

ServerWizardPage::ServerWizardPage(QWidget* parent) : QWizardPage(parent)
{
    ui.setupUi(this);
    setPixmap(QWizard::LogoPixmap, QPixmap(":/resources/oxygen/64x64/actions/network.png"));
    connect(ui.lineEditHost, SIGNAL(textChanged(QString)), this, SIGNAL(completeChanged()));
    connect(ui.spinBoxPort, SIGNAL(valueChanged(int)), this, SIGNAL(completeChanged()));

    QSettings settings;
    QStringList hosts = settings.value("hosts").toStringList();

    QCompleter* hostCompleter = new QCompleter(hosts, ui.lineEditHost);
    hostCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    ui.lineEditHost->setCompleter(hostCompleter);
}

ServerWizardPage::~ServerWizardPage()
{
    QSettings settings;
    QStringList hosts = settings.value("hosts").toStringList();
    if (!hosts.contains(hostName(), Qt::CaseInsensitive))
        settings.setValue("hosts", hosts << hostName());
}

QString ServerWizardPage::hostName() const
{
    return ui.lineEditHost->text();
}

void ServerWizardPage::setHostName(const QString& hostName)
{
    ui.lineEditHost->setText(hostName);
}

quint16 ServerWizardPage::port() const
{
    return ui.spinBoxPort->value();
}

void ServerWizardPage::setPort(quint16 port)
{
    ui.spinBoxPort->setValue(port);
}

bool ServerWizardPage::isSecure() const
{
    return ui.checkBoxSecure->isChecked();
}

void ServerWizardPage::setSecure(bool secure)
{
    ui.checkBoxSecure->setChecked(secure);
}

QString ServerWizardPage::password() const
{
    return ui.lineEditPass->text();
}

void ServerWizardPage::setPassword(const QString& password)
{
    ui.lineEditPass->setText(password);
}

bool ServerWizardPage::isComplete() const
{
    return !ui.lineEditHost->text().isEmpty() && ui.spinBoxPort->hasAcceptableInput();
}
