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

#include "userwizardpage.h"
#include <QRegExpValidator>
#include <QCompleter>
#include <QSettings>

UserWizardPage::UserWizardPage(QWidget* parent) : QWizardPage(parent)
{
    ui.setupUi(this);
    setPixmap(QWizard::LogoPixmap, QPixmap(":/resources/oxygen/64x64/actions/user.png"));
    connect(ui.lineEditNick, SIGNAL(textChanged(QString)), this, SIGNAL(completeChanged()));

    QSettings settings;
    QStringList nicks = settings.value("nicks").toStringList();
    QStringList names = settings.value("realNames").toStringList();

    QCompleter* nickCompleter = new QCompleter(nicks, ui.lineEditNick);
    nickCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    ui.lineEditNick->setCompleter(nickCompleter);
    QRegExpValidator* validator = new QRegExpValidator(ui.lineEditNick);
    validator->setRegExp(QRegExp("\\S+"));
    ui.lineEditNick->setValidator(validator);

    QCompleter* nameCompleter = new QCompleter(names, ui.lineEditName);
    nameCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    ui.lineEditName->setCompleter(nameCompleter);
}

UserWizardPage::~UserWizardPage()
{
    QSettings settings;
    QStringList nicks = settings.value("nicks").toStringList();
    QStringList names = settings.value("realNames").toStringList();
    if (!nicks.contains(nickName(), Qt::CaseInsensitive))
        settings.setValue("nicks", nicks << nickName());
    if (!names.contains(realName(), Qt::CaseInsensitive))
        settings.setValue("realNames", names << realName());
}

QString UserWizardPage::nickName() const
{
    return ui.lineEditNick->text();
}

void UserWizardPage::setNickName(const QString& nickName)
{
    ui.lineEditNick->setText(nickName);
}

QString UserWizardPage::realName() const
{
    return ui.lineEditName->text();
}

void UserWizardPage::setRealName(const QString& realName)
{
    ui.lineEditName->setText(realName);
}

bool UserWizardPage::isComplete() const
{
    return !ui.lineEditNick->text().isEmpty();
}
