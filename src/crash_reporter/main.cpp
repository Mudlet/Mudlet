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
#include <QEventLoop>
#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>
#include <QSettings>
#include <QCoreApplication>
#include <QTimer>
#include <QUrl>
#include <cstdlib>

#include "crashReporter.h"

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
        sendCrashReport(argv[1]);
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
        sendCrashReport(envelopePath);
    } else if (result == SendThisTime) {
        sendCrashReport(envelopePath);
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

void sendCrashReport(const char* envelopePath)
{
    const char* dsnFromEnvironment = std::getenv("SENTRY_DSN");
    QString dsn;
    if (dsnFromEnvironment && dsnFromEnvironment[0]) {
        dsn = QString::fromUtf8(dsnFromEnvironment);
    } else if (SENTRY_DSN && SENTRY_DSN[0]) {
        dsn = QString::fromUtf8(SENTRY_DSN);
    } else {
        return;
    }

    // A DSN is <scheme>://<public key>@<host>/<optional path>/<project id>, and
    // the envelope endpoint is that host and path with /api/<project id>/envelope/
    // appended: https://develop.sentry.dev/sdk/overview/#parsing-the-dsn
    const QUrl parsedDsn(dsn);
    if (!parsedDsn.isValid() || parsedDsn.userName().isEmpty() || parsedDsn.host().isEmpty()) {
        return;
    }
    QStringList segments = parsedDsn.path().split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (segments.isEmpty()) {
        return;
    }
    const QString projectId = segments.takeLast();
    const QString pathPrefix = segments.isEmpty() ? QString() : QLatin1Char('/') + segments.join(QLatin1Char('/'));

    QUrl endpoint;
    endpoint.setScheme(parsedDsn.scheme());
    endpoint.setHost(parsedDsn.host());
    if (parsedDsn.port() != -1) {
        endpoint.setPort(parsedDsn.port());
    }
    endpoint.setPath(QStringLiteral("%1/api/%2/envelope/").arg(pathPrefix, projectId));

    QFile envelope(QString::fromUtf8(envelopePath));
    if (!envelope.open(QIODevice::ReadOnly)) {
        return;
    }
    const QByteArray payload = envelope.readAll();
    envelope.close();
    if (payload.isEmpty()) {
        return;
    }

    QNetworkRequest request(endpoint);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-sentry-envelope");
    // Store endpoint auth: sentry_key is the DSN's public key, and sentry_version
    // has been 7 since the DSN format settled.
    // https://develop.sentry.dev/sdk/overview/#authentication
    request.setRawHeader("X-Sentry-Auth", QStringLiteral("Sentry sentry_version=7, sentry_client=mudlet-crash-reporter/1.0, sentry_key=%1").arg(parsedDsn.userName()).toUtf8());

    // The reply is parented to the manager, so it goes with it at end of scope.
    QNetworkAccessManager manager;
    QEventLoop loop;
    QNetworkReply* reply = manager.post(request, payload);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    timeout.start(30 * 1000);

    loop.exec();

    if (!reply->isFinished()) {
        reply->abort();
        qWarning() << "Crash report upload timed out.";
    } else if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Crash report upload failed:" << reply->errorString();
    }
}
