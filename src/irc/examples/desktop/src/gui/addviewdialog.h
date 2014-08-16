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

#ifndef ADDVIEWDIALOG_H
#define ADDVIEWDIALOG_H

#include <QDialog>

class Session;
class QLabel;
class QLineEdit;
class QDialogButtonBox;

class AddViewDialog : public QDialog
{
    Q_OBJECT

public:
    AddViewDialog(Session* session, QWidget* parent = 0);

    QString view() const;
    QString password() const;
    Session* session() const;

private slots:
    void updateUi();

private:
    struct Private
    {
        Session* session;
        QLabel* viewLabel;
        QLabel* passLabel;
        QLineEdit* viewEdit;
        QLineEdit* passEdit;
        QDialogButtonBox* buttonBox;
    } d;
};

#endif // ADDVIEWDIALOG_H
