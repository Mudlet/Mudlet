/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include "MudletVersion.h"

#include "utils.h"

#include <QDebug>
#include <QFile>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QUrl>

const QString& MudletVersion::build()
{
    // Deliberately not a namespace-scope constant: the Qt resource system is
    // only registered once main() runs, so reading the file any earlier would
    // silently yield an empty string and make every build look like a release.
    static const QString appBuild = [] {
        QFile gitShaFile(qsl(":/app-build.txt"));
        if (!gitShaFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "MudletVersion::build() failed to open app-build.txt for reading:" << gitShaFile.errorString();
            return QString();
        }
        return QString::fromUtf8(gitShaFile.readAll()).trimmed();
    }();
    return appBuild;
}

const QString& MudletVersion::scmVersion()
{
    static const QString version = qsl("Mudlet ") + QString(APP_VERSION) + build();
    return version;
}

bool MudletVersion::release()
{
    return build().isEmpty();
}

bool MudletVersion::publicTest()
{
    return build().startsWith(qsl("-ptb"));
}

bool MudletVersion::development()
{
    return !release() && !publicTest();
}

// Enable redirects and HTTPS support for a given url
void MudletVersion::setNetworkRequestDefaults(const QUrl& url, QNetworkRequest& request)
{
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    request.setRawHeader(QByteArray("User-Agent"), qsl("Mozilla/5.0 (Mudlet/%1%2)").arg(APP_VERSION, build()).toUtf8());
#if !defined(QT_NO_SSL)
    if (url.scheme() == qsl("https")) {
        const QSslConfiguration config(QSslConfiguration::defaultConfiguration());
        request.setSslConfiguration(config);
    }
#endif
}
