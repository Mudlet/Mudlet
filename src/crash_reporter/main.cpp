/***************************************************************************
 *   Copyright (C) 2025 by Nicolas Keita - nicolaskeita2@gmail.com         *
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

#include <QApplication>
#include <QDebug>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSettings>
#include <QCoreApplication>
#include <cstdlib>

#include "crashReporter.h"

static QString configuredDsn()
{
    const char* dsnFromEnvironment = std::getenv("SENTRY_DSN");
    if (dsnFromEnvironment && dsnFromEnvironment[0]) {
        return QString::fromUtf8(dsnFromEnvironment);
    }
    if (SENTRY_DSN && SENTRY_DSN[0]) {
        return QString::fromUtf8(SENTRY_DSN);
    }
    return QString();
}

// Setting "autoSendCrashReports" is expected to be stored there:
// Windows: in the registry at HKEY_CURRENT_USER\Software\mudlet\CrashReporter
// Linux: in a file at ~/.config/Mudlet/CrashReporter.conf
// macOS: in a file at ~/Library/Preferences/com.Mudlet.CrashReporter.plist
int main(int argc, char* argv[])
{
    if (argc < 2) {
        qWarning() << "Error: This program requires the path to a .envelope file as an argument.";
        qWarning() << "Usage:" << argv[0] << "<path_to_envelope>";
        return 1;
    }

    QApplication app(argc, argv);
    QSettings settings("Mudlet", "CrashReporter");
    QVariant storedOption = settings.value("autoSendCrashReports", QVariant());

    if (storedOption.isValid() && storedOption.toInt() == AlwaysSend) {
        sendCrashReport(argv[1], configuredDsn());
    } else {
        showCrashDialogAndSend(argv[1], settings);
    }
    return 0;
}

void showCrashDialogAndSend(const char* envelopePath, QSettings& settings)
{
    TCrashSendOption result = createCrashDialog();

    if (result == AlwaysSend) {
        settings.setValue("autoSendCrashReports", static_cast<int>(AlwaysSend));
        sendCrashReport(envelopePath, configuredDsn());
    } else if (result == SendThisTime) {
        sendCrashReport(envelopePath, configuredDsn());
    }
}

TCrashSendOption createCrashDialog()
{
    QDialog dialog;
    dialog.setWindowTitle(QCoreApplication::translate("CrashReporter", "Mudlet Crash"));

    QVBoxLayout* vLayout = new QVBoxLayout(&dialog);
    QLabel* label = new QLabel(QCoreApplication::translate("CrashReporter",
                                                           "<div align='center'><b>Mudlet has encountered a problem.</b><br><br>"
                                                           "You can choose to send a crash report to help us improve the application.</div>"));
    label->setAlignment(Qt::AlignCenter);
    vLayout->addWidget(label);

    QHBoxLayout* hLayout = new QHBoxLayout();
    QPushButton* sendBtn = new QPushButton(QCoreApplication::translate("CrashReporter", "Send this time"));
    QPushButton* alwaysBtn = new QPushButton(QCoreApplication::translate("CrashReporter", "Always send"));
    QPushButton* dontBtn = new QPushButton(QCoreApplication::translate("CrashReporter", "Don't send"));

    hLayout->addStretch();
    hLayout->addWidget(sendBtn);
    hLayout->addWidget(alwaysBtn);
    hLayout->addWidget(dontBtn);
    hLayout->addStretch();
    vLayout->addLayout(hLayout);

    QObject::connect(sendBtn, &QPushButton::clicked, [&dialog]() {
        dialog.done(static_cast<int>(SendThisTime));
    });
    QObject::connect(alwaysBtn, &QPushButton::clicked, [&dialog]() {
        dialog.done(static_cast<int>(AlwaysSend));
    });
    QObject::connect(dontBtn, &QPushButton::clicked, [&dialog]() {
        dialog.done(static_cast<int>(DontSend));
    });

    sendBtn->setDefault(true);

    return static_cast<TCrashSendOption>(dialog.exec());
}
