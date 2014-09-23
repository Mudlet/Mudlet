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

#include "addviewdialog.h"
#include "session.h"
#include <QDialogButtonBox>
#include <QRegExpValidator>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QRegExp>
#include <QLabel>

AddViewDialog::AddViewDialog(Session* session, QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("Add view"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    d.session = session;

    d.viewLabel = new QLabel(this);
    d.viewEdit = new QLineEdit("#", this);
    d.viewEdit->setFocus();
    d.viewEdit->setValidator(new QRegExpValidator(QRegExp("\\S+"), d.viewEdit));
    connect(d.viewEdit, SIGNAL(textEdited(QString)), this, SLOT(updateUi()));

    QLabel* subLabel = new QLabel(this);
    subLabel->setText(tr("<small>%1 supports channel types: %2<small>").arg(session->network()).arg(session->channelTypes()));
    subLabel->setAlignment(Qt::AlignRight);
    subLabel->setDisabled(true);

    d.passLabel = new QLabel(this);
    d.passLabel->setText("Password:");
    d.passEdit = new QLineEdit(this);
    d.passEdit->setPlaceholderText(tr("Optional..."));

    d.buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(d.buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(d.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(d.viewLabel);
    layout->addWidget(d.viewEdit);
    layout->addWidget(subLabel);
    layout->addSpacing(9);
    layout->addWidget(d.passLabel);
    layout->addWidget(d.passEdit);
    layout->addSpacing(9);
    layout->addStretch();
    layout->addWidget(d.buttonBox);

    updateUi();
}

QString AddViewDialog::view() const
{
    return d.viewEdit->text();
}

QString AddViewDialog::password() const
{
    return d.passEdit->text();
}

Session* AddViewDialog::session() const
{
    return d.session;
}

void AddViewDialog::updateUi()
{
    bool valid = false;
    bool channel = !view().isEmpty() && d.session->channelTypes().contains(view().at(0));
    if (channel)
    {
        valid = view().length() > 1;
        d.viewLabel->setText(tr("Join channel:"));
    }
    else
    {
        valid = !view().isEmpty();
        d.viewLabel->setText(tr("Open query:"));
    }

    d.passLabel->setEnabled(channel);
    d.passEdit->setEnabled(channel);
    d.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
}
