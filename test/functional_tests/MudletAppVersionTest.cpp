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

// The version identity is what Mudlet calls itself to a game server, to the
// package repository and to the updater, and it used to live on the main
// window - so it could only be asked once a window existed. This binary never
// makes one, which is the point: every assertion below runs with
// mudlet::self() still null.
//
// The resource read is checked against the file rather than only for
// self-consistency. An empty :/app-build.txt and an unregistered resource
// collection are indistinguishable from buildSuffix()'s side, and both read as an
// official release - which is the arm that turns the updater loose and picks
// the release splash screen.

#include <QFile>
#include <QNetworkRequest>
#include <QUrl>
#include <QtTest/QtTest>

#include "MudletApp.h"
#include "mudlet.h"
#include "utils.h"

#include "GroupedTest.h"

class MudletAppVersionTest : public QObject
{
    Q_OBJECT

private:
    // Read straight off the resource, so the expectations below are not simply
    // whatever MudletApp happened to compute.
    static QString suffixFromResource()
    {
        QFile buildFile(qsl(":/app-build.txt"));
        if (!buildFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString();
        }
        return QString::fromUtf8(buildFile.readAll()).trimmed();
    }

private slots:
    void buildComesFromTheResource()
    {
        QVERIFY2(mudlet::self() == nullptr, "the version identity has to be readable before a main window exists");

        QFile buildFile(qsl(":/app-build.txt"));
        QVERIFY2(buildFile.open(QIODevice::ReadOnly | QIODevice::Text), qUtf8Printable(qsl("cannot open :/app-build.txt: %1").arg(buildFile.errorString())));

        QCOMPARE(MudletApp::buildSuffix(), QString::fromUtf8(buildFile.readAll()).trimmed());
    }

    void scmVersionSpellsOutTheBuild()
    {
        // Against the independent read of the resource, not against buildSuffix():
        // comparing it with what the implementation itself returns restates the code
        QCOMPARE(MudletApp::scmVersion(), qsl("Mudlet ") + QString(APP_VERSION) + suffixFromResource());
    }

    void flagsFollowTheBuild()
    {
        const QString suffix = suffixFromResource();

        // Each against the resource, never against another of the three: comparing
        // development() with !release() && !publicTest() is what the body does, so
        // it holds however badly any of them is broken
        QCOMPARE(MudletApp::release(), suffix.isEmpty());
        QCOMPARE(MudletApp::publicTest(), suffix.startsWith(qsl("-ptb")));
        QCOMPARE(MudletApp::development(), !suffix.isEmpty() && !suffix.startsWith(qsl("-ptb")));
    }

    void networkRequestsCarryTheUserAgent()
    {
        const QUrl url(qsl("https://www.mudlet.org/"));
        QNetworkRequest request(url);
        MudletApp::setNetworkRequestDefaults(url, request);

        QCOMPARE(request.rawHeader(QByteArray("User-Agent")), (qsl("Mozilla/5.0 (Mudlet/") + QString(APP_VERSION) + suffixFromResource() + qsl(")")).toUtf8());
        QCOMPARE(request.attribute(QNetworkRequest::RedirectPolicyAttribute).value<QNetworkRequest::RedirectPolicy>(), QNetworkRequest::NoLessSafeRedirectPolicy);
    }
};

MUDLET_GROUPED_TEST_MAIN(MudletAppVersionTest)

#include "MudletAppVersionTest.moc"
