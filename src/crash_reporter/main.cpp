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
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSettings>
#include <QCoreApplication>
#include <cstdlib>

#include "crashReporter.h"
#include "sentry.h"
#include "sentry_options.h"
extern "C" {
#include "sentry_transport.h"
}

// Setting "autoSendCrashReports" is expected to be stored there:
// Windows: in the registry at HKEY_CURRENT_USER\Software\mudlet\CrashReporter
// Linux: in a file at ~/.config/Mudlet/CrashReporter.conf
// macOS: in a file at ~/Library/Preferences/com.Mudlet.CrashReporter.plist
int main(int argc, char *argv[])
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
        sendCrashReport(argv[1]);
    } else {
        showCrashDialogAndSend(argv[1], settings);
    }
    return 0;
}

void showCrashDialogAndSend(const char *envelopePath, QSettings &settings) {
    TCrashSendOption result = createCrashDialog();

    if (result == AlwaysSend) {
        settings.setValue("autoSendCrashReports", static_cast<int>(AlwaysSend));
        sendCrashReport(envelopePath);
    } else if (result == SendThisTime) {
        sendCrashReport(envelopePath);
    }
}

TCrashSendOption createCrashDialog() {
    QDialog dialog;
    dialog.setWindowTitle(QCoreApplication::translate("CrashReporter", "Mudlet Crash"));

    QVBoxLayout *vLayout = new QVBoxLayout(&dialog);
    QLabel *label = new QLabel(QCoreApplication::translate(
        "CrashReporter",
        "<div align='center'><b>Mudlet has encountered a problem.</b><br><br>"
        "You can choose to send a crash report to help us improve the application.</div>"));
    label->setAlignment(Qt::AlignCenter);
    vLayout->addWidget(label);

    QHBoxLayout *hLayout = new QHBoxLayout();
    QPushButton *sendBtn   = new QPushButton(QCoreApplication::translate("CrashReporter", "Send this time"));
    QPushButton *alwaysBtn = new QPushButton(QCoreApplication::translate("CrashReporter", "Always send"));
    QPushButton *dontBtn   = new QPushButton(QCoreApplication::translate("CrashReporter", "Don't send"));

    hLayout->addStretch();
    hLayout->addWidget(sendBtn);
    hLayout->addWidget(alwaysBtn);
    hLayout->addWidget(dontBtn);
    hLayout->addStretch();
    vLayout->addLayout(hLayout);

    QObject::connect(sendBtn,   &QPushButton::clicked, [&dialog](){ dialog.done(static_cast<int>(SendThisTime)); });
    QObject::connect(alwaysBtn, &QPushButton::clicked, [&dialog](){ dialog.done(static_cast<int>(AlwaysSend)); });
    QObject::connect(dontBtn,   &QPushButton::clicked, [&dialog](){ dialog.done(static_cast<int>(DontSend)); });

    sendBtn->setDefault(true);

    return static_cast<TCrashSendOption>(dialog.exec());
}

void sendCrashReport(const char *envelopePath)
{
    sentry_options_t*  opts = sentry_options_new();
    sentry_envelope_t* envelope = sentry_envelope_read_from_file(envelopePath);
    const char*        effectiveDsn = nullptr;
    const char*        sentry_dsn_from_environment = std::getenv("SENTRY_DSN");

    if (!opts || !envelope) {
        return;
    }
    if (sentry_dsn_from_environment && sentry_dsn_from_environment[0]) {
        effectiveDsn = sentry_dsn_from_environment;
    } else if (SENTRY_DSN && SENTRY_DSN[0]) {
        effectiveDsn = SENTRY_DSN;
    } else {
        return;
    }
    sentry_options_set_dsn(opts, effectiveDsn);
    sentry__transport_startup(opts->transport, opts);
    sentry__transport_send_envelope(opts->transport, envelope);
    sentry__transport_shutdown(opts->transport, 30 * 1000);
}
