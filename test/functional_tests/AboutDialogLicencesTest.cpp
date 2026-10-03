/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers - mudlet@mudlet.org           *
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

/*
 * The About dialog is where Mudlet discharges the obligations the components it
 * bundles put on it, and where it credits its Patreon supporters.
 *
 * Run with: ctest -R AboutDialogLicencesTest -V
 */

#include <QDir>
#include <QTemporaryDir>
#include <QTextBrowser>
#include <QtTest/QtTest>

#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "dlgAboutDialog.h"
#include "mudlet.h"

#include "GroupedTest.h"

class AboutDialogLicencesTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    dlgAboutDialog* mpDialog = nullptr;

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own. Sharing the developer's
        // ~/.config/mudlet means sharing a profile list, so a second copy of
        // this test running at the same time is told the name it types is
        // already in use and never gets an enabled Connect button. Since #9712
        // the opt-in that makes setupConfig() adopt a directory is
        // $XDG_CONFIG_HOME/mudlet/profiles, not the mudlet directory alone.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();

        mpDialog = new dlgAboutDialog(mudlet::self());
    }

    void cleanupTestCase()
    {
        delete mpDialog;
        mpDialog = nullptr;
        if (mudlet::self()) {
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // Lucide's icons are under the ISC licence, which only permits Mudlet to
    // ship them while the permission notice travels with them; a one-line
    // mention of the project does not discharge that (#10961). The notice is
    // the assertion that holds under translation - the header around it,
    // lucide.dev included, is inside a tr().
    void test_theThirdPartyTabCarriesTheLucideLicence()
    {
        const QString shown = mpDialog->textBrowser_license_3rdparty->toPlainText();
        QVERIFY2(shown.contains(qsl("lucide.dev")), "the third party tab does not credit Lucide at all");
        QVERIFY2(shown.contains(qsl("Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted")),
                 "the third party tab credits Lucide without carrying the ISC permission notice that lets Mudlet ship it");
    }

    // The supporters are named alongside a link to the page that lets anyone
    // join them, and a QTextBrowser swallows a link it is not told to hand to
    // the browser, so the only thing the link did was nothing (#2927).
    void test_theSupportersTabHandsItsLinksToTheBrowser()
    {
        QVERIFY2(mpDialog->textBrowser_supporters->toHtml().contains(qsl("patreon.com")), "the supporters tab carries no link at all, so the property below has nothing to matter to");
        QVERIFY2(mpDialog->textBrowser_supporters->openExternalLinks(), "a link on the supporters tab goes nowhere");
    }
};

#include "AboutDialogLicencesTest.moc"
MUDLET_GROUPED_TEST_MAIN(AboutDialogLicencesTest)
