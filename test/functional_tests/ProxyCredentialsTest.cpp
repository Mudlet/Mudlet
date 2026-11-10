/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

// The proxy a profile connects through is built once and then kept, so every connection after
// the first is made with whatever that kept proxy holds.

#include <QDir>
#include <QNetworkProxy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <memory>

#include "CredentialManager.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "TBuffer.h"
#include "TConsoleModel.h"
#include "XMLexport.h"

#include "GroupedTest.h"

class ProxyCredentialsTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    // The main window owns the profile pool in the app; here nothing else would.
    std::unique_ptr<HostManager> mpHostManager;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("ProxyCredentials-Test");
    bool mInitialised = false;

    // Where XDG_CONFIG_HOME is ignored the credential root is the real one, so the stored password and
    // its key are removed; only once initTestCase() completed, as a skipped run wrote nothing there.
    void removeStoredCredentials()
    {
        if (!mInitialised) {
            return;
        }
        for (const QString& name : {MudletApp::sanitizeForPath(mHostname), mHostname}) {
            QDir(qsl("%1/profiles/%2").arg(MudletApp::credentialFilesRoot(), name)).removeRecursively();
        }
    }

    void save()
    {
        pugi::xml_document doc;
        XMLexport(mpHost).writeHost(mpHost, doc.append_child("HostPackage"));
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));

        QVERIFY2(!HostManager::self(), "A profile pool already exists, so this run is not headless.");
        mpHostManager = std::make_unique<HostManager>();
        QVERIFY2(HostManager::self()->addHost(mHostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        mpHost = HostManager::self()->getHost(mHostname);
        QVERIFY2(mpHost, "The profile is not in the pool.");
        mInitialised = true;
        removeStoredCredentials();
    }

    void cleanupTestCase()
    {
        removeStoredCredentials();
        mpHost = nullptr;
        mpHostManager.reset();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void clearedCredentialsAreNotOfferedToTheNextProxy()
    {
        mpHost->mUseProxy = true;
        mpHost->mProxyAddress = qsl("first-proxy.example");
        mpHost->mProxyPort = 1080;
        mpHost->mProxyUsername = qsl("alice");
        mpHost->mProxyPassword = qsl("secret");
        {
            const auto& proxy = mpHost->getConnectionProxy();
            QCOMPARE(proxy->user(), qsl("alice"));
            QCOMPARE(proxy->password(), qsl("secret"));
        }

        // A switch to a proxy that takes no credentials
        mpHost->mProxyAddress = qsl("second-proxy.example");
        mpHost->mProxyUsername.clear();
        mpHost->mProxyPassword.clear();
        const auto& proxy = mpHost->getConnectionProxy();
        QCOMPARE(proxy->hostName(), qsl("second-proxy.example"));
        QVERIFY2(proxy->user().isEmpty(), qPrintable(qsl("the cleared username \"%1\" is still offered").arg(proxy->user())));
        QVERIFY2(proxy->password().isEmpty(), "the cleared password is still offered");
    }

    // The saved profile never holds the password, so loading reads it from secure storage
    void clearedPasswordStaysClearedAfterSaving()
    {
        mpHost->setProxyPassword(qsl("secret"));
        save();
        QCOMPARE(CredentialManager::retrieveCredential(mHostname, qsl("proxy")), qsl("secret"));

        mpHost->setProxyPassword(QString());
        save();
        QVERIFY2(CredentialManager::retrieveCredential(mHostname, qsl("proxy")).isEmpty(), "the cleared password would be loaded again");
    }

    // A password that could not be read at load leaves the field as empty as one the player cleared
    void anUnreadablePasswordSurvivesASave()
    {
        QVERIFY(CredentialManager::storeCredential(mHostname, qsl("proxy"), qsl("secret")));
        const QString path = qsl("%1/profiles/%2/passwords/proxy").arg(MudletApp::credentialFilesRoot(), MudletApp::sanitizeForPath(mHostname));
        QFile file(path);
        QVERIFY2(file.open(QIODevice::WriteOnly | QIODevice::Truncate), qPrintable(path));
        file.write("not something that decrypts");
        file.close();
        CredentialManager::ReadOutcome outcome = CredentialManager::ReadOutcome::Found;
        QVERIFY(CredentialManager::retrieveCredential(mHostname, qsl("proxy"), &outcome).isEmpty());
        QCOMPARE(outcome, CredentialManager::ReadOutcome::Unreadable);

        // As loading leaves it after an unreadable read, then the preferences applying the empty field
        mpHost->mProxyPassword.clear();
        mpHost->setProxyPassword(QString());
        save();
        QVERIFY2(QFile::exists(path), "clearing an empty field deleted a stored password that only failed to load");
    }

    void aPasswordThatCannotBeDeletedIsReported()
    {
        // A directory where the stored password belongs, which QFile::remove() cannot delete even as root
        const QString path = qsl("%1/profiles/%2/passwords/proxy").arg(MudletApp::credentialFilesRoot(), MudletApp::sanitizeForPath(mHostname));
        QFile::remove(path);
        QVERIFY(QDir().mkpath(qsl("%1/blocker").arg(path)));

        mpHost->setProxyPassword(qsl("secret"));
        mpHost->setProxyPassword(QString());

        TBuffer& buffer = mpHost->mainConsoleModel().buffer;
        bool warned = false;
        for (int i = 0; i <= buffer.getLastLineNumber() && !warned; ++i) {
            warned = buffer.line(i).contains(qsl("proxy password could not be deleted"));
        }
        QDir(path).removeRecursively();
        QVERIFY2(warned, "the player was not told the cleared password will be loaded again");
    }
};

#include "ProxyCredentialsTest.moc"
MUDLET_GROUPED_TEST_MAIN(ProxyCredentialsTest)
