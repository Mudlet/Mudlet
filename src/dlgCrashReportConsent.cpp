/***************************************************************************
 *   Copyright (C) 2022-2025 by The Mudlet Team                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "dlgCrashReportConsent.h"
#include "mudlet.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QSettings>
#include <QTextEdit>

dlgCrashReportConsent::dlgCrashReportConsent(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle(tr("Crash Reporting"));
    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *label = new QLabel(tr("Mudlet crashed during the last session. "
                                  "Would you like to send a crash report to help us fix the issue? "
                                  "You can also add optional comments below."));
    label->setWordWrap(true);
    layout->addWidget(label);

    mFeedbackTextEdit = new QTextEdit;
    mFeedbackTextEdit->setPlaceholderText(tr("You can add any details about what you were doing when Mudlet crashed here."));
    layout->addWidget(mFeedbackTextEdit);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::No);
    buttonBox->button(QDialogButtonBox::Yes)->setText(tr("Send Report"));
    buttonBox->button(QDialogButtonBox::No)->setText(tr("Don't Send"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);

    setLayout(layout);
    resize(400, 300);
}

dlgCrashReportConsent::~dlgCrashReportConsent()
{
}

QString dlgCrashReportConsent::getFeedback() const
{
    return mFeedbackTextEdit->toPlainText();
}