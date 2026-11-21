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

/*
 * What the connection dialog does with the profile it was asked to open, driven
 * against a stub server on a loopback port:
 *
 *  - the dialog takes itself off screen before the load starts, rather than
 *    sitting there through it collecting further clicks (PR #5313, issue #5303);
 *  - asking again for the profile that is already open reconnects it instead of
 *    loading it a second time (PR #7778, issue #7698);
 *  - a password changed for the open profile is the one it signs in with next,
 *    rather than waiting on disk until it is next opened;
 *  - a character name and password typed for the profile that is already open
 *    are what its next connection logs in with, a password erased there is not
 *    sent again, and a password box the dialog itself left empty changes nothing;
 *  - a password typed there outlives the profile's own saved-password lookup,
 *    while one the dialog filled in only stands in for a password the profile
 *    does not hold yet, and gives way to that lookup.
 *
 * Run with: ctest -R ConnectionDialogOpenProfileTest -V
 */

#include "AutoLoginDelaysTestHelper.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "TAppFrontend.h"
#include "TelnetServerStub.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include <QtTest/QtTest>

#include <QLineEdit>
#include <QListWidget>
#include <QPointer>
#include <QScopeGuard>
#include <QSignalSpy>
#include <chrono>

#include "GroupedTest.h"

using namespace std::chrono_literals;

class ConnectionDialogOpenProfileTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mXdgDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;

    const QString mProfileName = qsl("ConnDialogOpenProfile-Test");
    const QString mLocalhost = qsl("127.0.0.1");
    const QString mSavedName = qsl("savedname");
    const QString mSavedPassword = qsl("savedsecret");

    // written from a lambda on mudlet's own signal, so they outlive the case
    bool mDialogOnScreenDuringTheLoad = true;
    bool mDialogAliveDuringTheLoad = false;

    dlgConnectionProfiles* dialog() const { return mudlet::self()->mpConnectionDialog.data(); }

    // Each case here closes the dialog, which then deletes itself through the
    // event loop - asking for a new one before that has happened only raises
    // the dying one. One still on screen is the dialog this test started with,
    // which needs no reopening.
    bool reopenDialog() const
    {
        if (auto* pDialog = dialog(); pDialog && pDialog->isVisible()) {
            return true;
        }
        if (!QTest::qWaitFor(
                    []() {
                        return mudlet::self()->mpConnectionDialog.isNull();
                    },
                    5s)) {
            return false;
        }
        mudlet::self()->slot_showConnectionDialog();
        return QTest::qWaitFor(
                []() {
                    return !mudlet::self()->mpConnectionDialog.isNull() && mudlet::self()->mpConnectionDialog->isVisible();
                },
                5s);
    }

    // picks the test profile the way a click on it does. slot_itemClicked()
    // ignores a repeat of the profile it saw less than 100ms ago, and it keeps
    // that in function-local statics - so the debounce outlives the dialog
    // reopenDialog() throws away, and a click landing inside it would leave the
    // form as the rebuild blanked it
    bool selectTestProfile() const
    {
        QTest::qWait(300ms);
        auto* pDialog = dialog();
        if (!pDialog) {
            return false;
        }
        const auto items = pDialog->findData(*pDialog->listWidget_profiles, mProfileName, dlgConnectionProfiles::csmNameRole);
        if (items.isEmpty()) {
            return false;
        }
        pDialog->listWidget_profiles->setCurrentItem(items.first());
        pDialog->slot_itemClicked(items.first());
        return pDialog->profile_name_entry->text() == mProfileName;
    }

    // The case below reads what the dialog does about a profile that is
    // already open, and the case above it is what normally leaves one open.
    // The grouped runner also takes a single method name, so opening it here
    // is what lets that case run on its own.
    bool openTheTestProfile() const
    {
        if (auto* pHost = mudlet::self()->getActiveHost(); pHost && pHost->getName() == mProfileName) {
            return true;
        }
        if (!reopenDialog() || !selectTestProfile()) {
            return false;
        }
        dialog()->slot_load();
        return QTest::qWaitFor(
                [this]() {
                    auto* pHost = mudlet::self()->getActiveHost();
                    return pHost && pHost->getName() == mProfileName;
                },
                15s);
    }

    // Shows the test profile in the dialog with this login saved for it. The
    // password box is filled a turn of the event loop after the profile is
    // picked, so each case waits for that before typing.
    bool showTheTestProfileWithSavedLogin(const QString& name, const QString& password) const
    {
        if (!reopenDialog() || !MudletApp::writeProfileData(mProfileName, qsl("login"), name).first || !MudletApp::writeProfileData(mProfileName, qsl("password"), password).first
            || !selectTestProfile()) {
            return false;
        }
        const QPointer<dlgConnectionProfiles> pDialog = dialog();
        if (!password.isEmpty()
            && !QTest::qWaitFor(
                    [&pDialog, &password]() {
                        return pDialog && pDialog->character_password_entry->text() == password;
                    },
                    5s)) {
            return false;
        }
        return pDialog && pDialog->login_entry->text() == name;
    }

    // Typed, not setText(): the login box saves on textEdited, and loadProfile() takes the password box as the player's
    // only when isModified(), neither of which setText() gives
    static void typeOver(QLineEdit* pBox, const QString& text)
    {
        pBox->setFocus();
        pBox->selectAll();
        QTest::keyClick(pBox, Qt::Key_Backspace);
        QTest::keyClicks(pBox, text);
    }

    Host* theOpenTestProfileHolding(const QString& login, const QString& password) const
    {
        if (!openTheTestProfile()) {
            return nullptr;
        }
        auto* pHost = mudlet::self()->getActiveHost();
        if (!pHost || pHost->getName() != mProfileName) {
            return nullptr;
        }
        // The cases share the one open profile; each starts as one just opened, whose password nobody has given yet
        pHost->setLogin(login);
        pHost->setPass(password);
        pHost->mPassGiven = false;
        pHost->mSecuredPasswordPending = false;
        pHost->mSecuredPasswordUnreadable = false;
        return pHost;
    }

    bool sentSince(const qsizetype before, const QByteArray& line) const
    {
        return QTest::qWaitFor(
                [this, before, &line]() {
                    return mpServer->receivedSoFar().mid(before).contains(line);
                },
                10s);
    }

    bool connectFromTheDialog(const bool viaConnect = true) const
    {
        QSignalSpy connections(mpServer, &QTcpServer::newConnection);
        if (!connections.isValid()) {
            return false;
        }
        if (viaConnect) {
            dialog()->accept();
        } else {
            dialog()->slot_load();
        }
        return QTest::qWaitFor(
                [&connections]() {
                    return !connections.isEmpty();
                },
                15s);
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - cannot redirect the config dir for this test");
        }

        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(mXdgDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mXdgDir.path()))); // profiles/ = XDG opt-in
        qputenv("XDG_CONFIG_HOME", mXdgDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        // an OS-assigned port, so parallel test runs cannot collide on a fixed one
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->serverPort() != 0, "TelnetServerStub failed to bind a loopback port");

        mudlet::start();
        mudlet::self()->setupConfig();
        QVERIFY(MudletApp::getMudletPath(enums::profilesPath).startsWith(mXdgDir.path()));
        // a settings file that already holds something is how a returning
        // player is recognised, which keeps the first-run invitation - it hides
        // the games list - out of this test. It has to be written before
        // init(), which stamps this config dir with a first-launch date of its
        // own that would read as a brand new install
        MudletApp::getQSettings()->setValue(qsl("uiTourShown"), true);
        MudletApp::getQSettings()->sync();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        QVERIFY2(mudlet::self()->experiencedMudletPlayer(), "the first-run invitation would hide the games list these cases pick from");

        QVERIFY(QDir().mkpath(MudletApp::getMudletPath(enums::profileHomePath, mProfileName)));
        QVERIFY(MudletApp::writeProfileData(mProfileName, qsl("url"), mLocalhost).first);
        QVERIFY(MudletApp::writeProfileData(mProfileName, qsl("port"), QString::number(mpServer->serverPort())).first);

        mudlet::self()->startAutoLogin({});
        QVERIFY(QTest::qWaitFor(
                []() {
                    return mudlet::self()->mpConnectionDialog && mudlet::self()->mpConnectionDialog->isVisible();
                },
                5s));
    }

    void cleanupTestCase()
    {
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
        delete mpServer;
        mpServer = nullptr;
        delete mudlet::self();
    }

    // Loading a profile takes long enough for a second click to land on the
    // buttons of a dialog that is on its way out. PR #5313.
    void test_theDialogIsOffScreenBeforeTheProfileLoads()
    {
        QVERIFY2(dialog(), "No connection dialog to test against");
        QVERIFY(selectTestProfile());
        QVERIFY2(dialog()->isVisible(), "The dialog has to start out on screen for this case to mean anything");

        // stays true if the load never happens, which fails the case too
        mDialogOnScreenDuringTheLoad = true;
        const auto measurement = connect(mudlet::self(), &mudlet::signal_hostCreated, this, [this]() {
            mDialogAliveDuringTheLoad = (dialog() != nullptr);
            mDialogOnScreenDuringTheLoad = dialog() && dialog()->isVisible();
        });

        dialog()->slot_load(); // the Offline button
        disconnect(measurement);

        QVERIFY2(HostManager::self()->getHost(mProfileName), "The profile never loaded, so nothing was measured");
        QVERIFY2(mDialogAliveDuringTheLoad, "The dialog was already gone when the profile loaded, so its visibility was never measured");
        QVERIFY2(!mDialogOnScreenDuringTheLoad, "The dialog was still on screen while the profile loaded");
    }

    void test_aPasswordChangedForTheOpenProfileReachesIt()
    {
        QVERIFY2(openTheTestProfile(), "The test profile could not be opened, so there is nothing for the dialog to change");
        QVERIFY2(reopenDialog(), "No dialog to test against");
        QVERIFY(selectTestProfile());
        auto* pHost = HostManager::self()->getHost(mProfileName);
        QVERIFY(pHost);
        // the saved password is filled in on the next turn of the event loop, over anything typed before then
        QTest::qWait(100ms);

        dialog()->character_password_entry->selectAll();
        // spaces at either end are part of the password, as the saved copy keeps them
        QTest::keyClicks(dialog()->character_password_entry, qsl(" sesame "));
        // the dialog waits for typing to pause before it saves
        QVERIFY2(QTest::qWaitFor(
                         [pHost]() {
                             return pHost->getPass() == qsl(" sesame ");
                         },
                         5s),
                 "The open profile is still using its old password");
    }

    // With the profile open, the dialog has to leave it alone and just
    // reconnect it rather than run the load a second time. PR #7778.
    void test_askingForTheOpenProfileAgainReconnectsIt()
    {
        QVERIFY2(openTheTestProfile(), "The test profile could not be opened, so there is nothing for the dialog to be asked about twice");
        QVERIFY2(reopenDialog(), "No dialog to test against");
        QVERIFY(selectTestProfile());
        auto* pHost = mudlet::self()->getActiveHost();
        QVERIFY2(pHost && pHost->getName() == mProfileName, "The test profile is not the open one, so this case would take another path");

        QSignalSpy connections(mpServer, &QTcpServer::newConnection);
        QSignalSpy loads(dialog(), &dlgConnectionProfiles::signal_load_profile);
        // an unattached spy counts zero for ever, which is what this case reads as success
        QVERIFY(connections.isValid());
        QVERIFY(loads.isValid());

        dialog()->slot_load(); // the Offline button

        QVERIFY2(QTest::qWaitFor(
                         [&connections]() {
                             return !connections.isEmpty();
                         },
                         15s),
                 "Asking for the open profile again did not reconnect it");
        QCOMPARE(loads.count(), 0);
    }

    void test_aLoginTypedForTheOpenProfileIsSentAtItsNextConnection_data()
    {
        QTest::addColumn<bool>("viaConnect");
        QTest::addColumn<QString>("typedName");
        QTest::addColumn<QString>("typedPassword");

        QTest::newRow("Connect") << true << qsl("typedforconnect") << qsl("secretforconnect");
        // on the open profile this reconnects it
        QTest::newRow("Offline") << false << qsl("typedforoffline") << qsl("secretforoffline");
    }

    // The open profile read its login as it loaded, so what the dialog saves to
    // the profile's files does not reach it.
    void test_aLoginTypedForTheOpenProfileIsSentAtItsNextConnection()
    {
        QFETCH(bool, viaConnect);
        QFETCH(QString, typedName);
        QFETCH(QString, typedPassword);

        auto* pHost = theOpenTestProfileHolding(mSavedName, mSavedPassword);
        QVERIFY2(pHost, "The test profile could not be opened, so the dialog has no open profile to be asked about");
        QVERIFY2(showTheTestProfileWithSavedLogin(mSavedName, mSavedPassword), "The dialog did not show the profile's saved login");

        typeOver(dialog()->login_entry, typedName);
        typeOver(dialog()->character_password_entry, typedPassword);
        QCOMPARE(dialog()->login_entry->text(), typedName);
        QCOMPARE(dialog()->character_password_entry->text(), typedPassword);
        QVERIFY2(pHost->getLogin() == mSavedName && pHost->getPass() == mSavedPassword, "Typing alone reached the open profile, so pressing the button would prove nothing");

        const ScopedAutoLoginDelays delays(200, 200);
        const qsizetype before = mpServer->receivedSoFar().size();
        QVERIFY2(connectFromTheDialog(viaConnect), "The open profile was not connected, so no login could be sent");
        const QByteArray nameLine = typedName.toUtf8() + "\r\n";
        const QByteArray passwordLine = typedPassword.toUtf8() + "\r\n";
        const bool passwordSent = sentSince(before, passwordLine);
        const QByteArray sent = mpServer->receivedSoFar().mid(before);
        const QString sentText = QString::fromLatin1(sent.toPercentEncoding(" "));
        QVERIFY2(passwordSent, qPrintable(qsl("The game was not sent the password typed in the dialog, but: %1").arg(sentText)));
        QVERIFY2(sent.contains(nameLine), qPrintable(qsl("The game was not sent the name typed in the dialog, but: %1").arg(sentText)));
        QVERIFY2(sent.indexOf(nameLine) < sent.indexOf(passwordLine), "The password went out ahead of the name");
    }

    // An emptied box is the player taking the password away.
    void test_aPasswordErasedForTheOpenProfileIsNotSentAgain()
    {
        auto* pHost = theOpenTestProfileHolding(mSavedName, mSavedPassword);
        QVERIFY2(pHost, "The test profile could not be opened, so the dialog has no open profile to be asked about");
        QVERIFY2(showTheTestProfileWithSavedLogin(mSavedName, mSavedPassword), "The dialog did not show the profile's saved login");

        auto* pBox = dialog()->character_password_entry;
        typeOver(pBox, QString());
        QVERIFY2(pBox->isModified() && pBox->text().isEmpty(), "The box was not left edited and empty, so this case cannot cover it");
        // as when its own lookup failed, which an erased password makes moot
        pHost->mSecuredPasswordUnreadable = true;

        const ScopedAutoLoginDelays delays(200, 200);
        const qsizetype before = mpServer->receivedSoFar().size();
        QVERIFY2(connectFromTheDialog(), "The open profile was not connected, so the dialog was never asked");
        QVERIFY2(pHost->getPass().isEmpty(), "The open profile kept the password that was erased in the dialog");
        QCOMPARE(pHost->getLogin(), mSavedName);
        const QByteArray nameLine = mSavedName.toUtf8() + "\r\n";
        QVERIFY2(sentSince(before, nameLine), "The game was not sent the name, so the login step never ran");
        QVERIFY2(pHost->mSecuredPasswordUnreadable, "The player was warned that a saved password could not be read though they erased it");
    }

    void test_aPasswordTypedForTheOpenProfileOutlivesItsSavedPasswordLookup_data()
    {
        QTest::addColumn<QString>("typedPassword");

        QTest::newRow("changed") << qsl("typedsecret");
        QTest::newRow("erased") << QString();
    }

    // The keychain can answer the open profile's lookup after the dialog has
    // handed over what was typed, with the password saved before the edit.
    void test_aPasswordTypedForTheOpenProfileOutlivesItsSavedPasswordLookup()
    {
        QFETCH(QString, typedPassword);

        auto* pHost = theOpenTestProfileHolding(mSavedName, mSavedPassword);
        QVERIFY2(pHost, "The test profile could not be opened, so the dialog has no open profile to be asked about");
        QVERIFY2(showTheTestProfileWithSavedLogin(mSavedName, mSavedPassword), "The dialog did not show the profile's saved login");

        typeOver(dialog()->character_password_entry, typedPassword);
        pHost->mSecuredPasswordPending = true;
        QVERIFY2(connectFromTheDialog(), "The open profile was not connected, so the dialog was never asked");
        QCOMPARE(pHost->getPass(), typedPassword);
        // an erased one leaves nothing for the auto-login to wait for, as the lookup's answer will not be taken
        QCOMPARE(pHost->hasAutoLoginCredentials(), !typedPassword.isEmpty());

        pHost->securedPasswordAnswered(true, mSavedPassword, QString(), false);
        QCOMPARE(pHost->getPass(), typedPassword);
    }

    // A profile that could read no password of its own takes the one the dialog showed
    void test_aPasswordTheDialogFilledInGivesWayToTheOpenProfilesLookup()
    {
        const QString shownPassword = qsl("oldersecret");
        const QString lookedUpPassword = qsl("newersecret");

        // holding no password, with no lookup outstanding
        auto* pHost = theOpenTestProfileHolding(mSavedName, QString());
        QVERIFY2(pHost, "The test profile could not be opened, so the dialog has no open profile to be asked about");
        QVERIFY2(showTheTestProfileWithSavedLogin(mSavedName, shownPassword), "The dialog did not show the profile's saved login");
        QVERIFY2(!dialog()->character_password_entry->isModified(), "The box was typed in, so this case cannot cover one the dialog filled");

        QVERIFY2(connectFromTheDialog(), "The open profile was not connected, so the dialog was never asked");
        QCOMPARE(pHost->getPass(), shownPassword);

        pHost->securedPasswordAnswered(true, lookedUpPassword, QString(), false);
        QCOMPARE(pHost->getPass(), lookedUpPassword);
    }

    // The dialog's read can fall back to an older copy while the profile's own lookup is still waiting on the keychain
    void test_aPasswordTheDialogFilledInWaitsForTheOpenProfilesPendingLookup()
    {
        const QString shownPassword = qsl("oldersecret");
        const QString lookedUpPassword = qsl("newersecret");

        auto* pHost = theOpenTestProfileHolding(mSavedName, QString());
        QVERIFY2(pHost, "The test profile could not be opened, so the dialog has no open profile to be asked about");
        pHost->mSecuredPasswordPending = true;
        QVERIFY2(showTheTestProfileWithSavedLogin(mSavedName, shownPassword), "The dialog did not show the profile's saved login");
        QVERIFY2(!dialog()->character_password_entry->isModified(), "The box was typed in, so this case cannot cover one the dialog filled");

        // the password step long enough after the name for the lookup to answer first, on a loaded machine too
        const ScopedAutoLoginDelays delays(200, 3000);
        const qsizetype before = mpServer->receivedSoFar().size();
        QVERIFY2(connectFromTheDialog(), "The open profile was not connected, so the dialog was never asked");
        const QByteArray nameLine = mSavedName.toUtf8() + "\r\n";
        QVERIFY2(sentSince(before, nameLine), "The game was not sent the name, so the login step never ran");
        QVERIFY2(pHost->getPass().isEmpty(), "The dialog's copy was taken ahead of the open profile's pending lookup");

        pHost->securedPasswordAnswered(true, lookedUpPassword, QString(), false);
        QCOMPARE(pHost->getPass(), lookedUpPassword);
        const QByteArray passwordLine = lookedUpPassword.toUtf8() + "\r\n";
        QVERIFY2(sentSince(before, passwordLine), "The game was not sent the password the lookup brought");
        QVERIFY2(!mpServer->receivedSoFar().mid(before).contains(shownPassword.toUtf8()), "The game was sent the older copy the dialog showed");
    }

    // A lookup that ends with none, as when the player denies the keychain prompt, leaves the dialog's copy the best there is
    void test_aPasswordTheDialogFilledInStandsInForAPendingLookupThatBringsNone()
    {
        const QString shownPassword = qsl("oldersecret");

        auto* pHost = theOpenTestProfileHolding(mSavedName, QString());
        QVERIFY2(pHost, "The test profile could not be opened, so the dialog has no open profile to be asked about");
        pHost->mSecuredPasswordPending = true;
        QVERIFY2(showTheTestProfileWithSavedLogin(mSavedName, shownPassword), "The dialog did not show the profile's saved login");
        QVERIFY2(!dialog()->character_password_entry->isModified(), "The box was typed in, so this case cannot cover one the dialog filled");

        const ScopedAutoLoginDelays delays(200, 200);
        const qsizetype before = mpServer->receivedSoFar().size();
        QVERIFY2(connectFromTheDialog(), "The open profile was not connected, so the dialog was never asked");
        QVERIFY2(pHost->getPass().isEmpty(), "The dialog's copy was taken ahead of the open profile's pending lookup");
        QVERIFY2(sentSince(before, mSavedName.toUtf8() + "\r\n"), "The automatic login did not send the name");
        // A masked password prompt: the only proof a password sent late still has one to answer
        mpServer->sendRaw(QByteArrayLiteral("Password: \xff\xfb\x01"));
        QVERIFY2(QTest::qWaitFor(
                         [pHost]() {
                             return pHost->isRemoteEchoingActive();
                         },
                         5s),
                 "The game's password prompt did not mask input");
        // past the password step, which had nothing to send yet
        QTest::qWait(1000);

        pHost->securedPasswordAnswered(false, QString(), qsl("denied"), false);
        QCOMPARE(pHost->getPass(), shownPassword);
        QVERIFY2(sentSince(before, shownPassword.toUtf8() + "\r\n"), "The password the dialog showed was not sent once the lookup brought none");
    }

    // An emptied name box is the player turning the automatic login off
    void test_aNameErasedForTheOpenProfileIsNotSentAgain()
    {
        auto* pHost = theOpenTestProfileHolding(mSavedName, mSavedPassword);
        QVERIFY2(pHost, "The test profile could not be opened, so the dialog has no open profile to be asked about");
        QVERIFY2(showTheTestProfileWithSavedLogin(mSavedName, mSavedPassword), "The dialog did not show the profile's saved login");
        typeOver(dialog()->login_entry, QString());
        QVERIFY2(dialog()->login_entry->text().isEmpty(), "The name box was not left empty, so this case cannot cover it");

        const ScopedAutoLoginDelays delays(200, 200);
        const qsizetype before = mpServer->receivedSoFar().size();
        QVERIFY2(connectFromTheDialog(), "The open profile was not connected, so the dialog was never asked");
        QVERIFY2(pHost->getLogin().isEmpty(), "The open profile kept the name that was erased in the dialog");
        QTest::qWait(1s);
        const QByteArray sent = mpServer->receivedSoFar().mid(before);
        QVERIFY2(!sent.contains(mSavedName.toUtf8() + "\r\n") && !sent.contains(mSavedPassword.toUtf8() + "\r\n"),
                 qPrintable(qsl("The game was sent the erased login: %1").arg(QString::fromLatin1(sent.toPercentEncoding(" ")))));
    }

    // What the dialog falls back to, as a plaintext copy behind a locked keychain, can be older than what the profile holds
    void test_aPasswordTheDialogFilledInLeavesTheOneTheOpenProfileHolds()
    {
        const QString shownPassword = qsl("oldersecret");
        const QString heldPassword = qsl("heldsecret");

        auto* pHost = theOpenTestProfileHolding(mSavedName, heldPassword);
        QVERIFY2(pHost, "The test profile could not be opened, so the dialog has no open profile to be asked about");
        QVERIFY2(showTheTestProfileWithSavedLogin(mSavedName, shownPassword), "The dialog did not show the profile's saved login");
        QVERIFY2(!dialog()->character_password_entry->isModified(), "The box was typed in, so this case cannot cover one the dialog filled");

        QVERIFY2(connectFromTheDialog(), "The open profile was not connected, so the dialog was never asked");
        QCOMPARE(pHost->getPass(), heldPassword);
    }

    // The dialog cannot always show a saved password - a locked keychain leaves
    // the box empty - so one nobody touched says nothing about the open profile's.
    void test_aPasswordBoxLeftEmptyLeavesTheOpenProfilesPassword()
    {
        const QString heldPassword = qsl("heldsecret");

        auto* pHost = theOpenTestProfileHolding(qsl("heldname"), heldPassword);
        QVERIFY2(pHost, "The test profile could not be opened, so the dialog has no open profile to be asked about");
        QVERIFY2(showTheTestProfileWithSavedLogin(mSavedName, QString()), "The dialog did not show the profile's saved login");

        auto* pBox = dialog()->character_password_entry;
        QVERIFY2(!pBox->isModified() && pBox->text().isEmpty(), "The box was not left empty and untouched, so this case cannot cover it");

        QVERIFY2(connectFromTheDialog(), "The open profile was not connected, so the dialog was never asked");
        QCOMPARE(pHost->getPass(), heldPassword);
        QCOMPARE(pHost->getLogin(), mSavedName);
    }

    // A profile open in a tab behind another one connects without becoming the active one
    void test_aLoginTypedForAnOpenProfileThatIsNotActiveIsSentAtItsNextConnection()
    {
        const QString typedName = qsl("typedname");
        const QString typedPassword = qsl("typedsecret");
        const QString otherProfile = qsl("ConnDialogOpenProfile-Other");

        auto* pHost = theOpenTestProfileHolding(mSavedName, mSavedPassword);
        QVERIFY2(pHost, "The test profile could not be opened, so the dialog has no open profile to be asked about");
        QVERIFY(QDir().mkpath(MudletApp::getMudletPath(enums::profileHomePath, otherProfile)));
        QVERIFY2(TAppFrontend::instance()->openProfile(otherProfile, false), "A second profile could not be opened to put in front of the test one");
        Host* pOther = HostManager::self()->getHost(otherProfile);
        QVERIFY2(pOther, "The second profile opened but has no session");
        const auto backToTheTestProfile = qScopeGuard([pHost]() {
            mudlet::self()->activateProfile(pHost);
        });
        QVERIFY2(QTest::qWaitFor(
                         [pOther]() {
                             mudlet::self()->activateProfile(pOther);
                             return mudlet::self()->getActiveHost() == pOther;
                         },
                         5s),
                 "The second profile did not come in front of the test one");

        QVERIFY2(showTheTestProfileWithSavedLogin(mSavedName, mSavedPassword), "The dialog did not show the profile's saved login");
        typeOver(dialog()->login_entry, typedName);
        typeOver(dialog()->character_password_entry, typedPassword);

        const ScopedAutoLoginDelays delays(200, 200);
        const qsizetype before = mpServer->receivedSoFar().size();
        QVERIFY2(connectFromTheDialog(), "The open profile was not connected, so no login could be sent");
        const QByteArray nameLine = typedName.toUtf8() + "\r\n";
        const QByteArray passwordLine = typedPassword.toUtf8() + "\r\n";
        const bool passwordSent = sentSince(before, passwordLine);
        const QByteArray sent = mpServer->receivedSoFar().mid(before);
        const QString sentText = QString::fromLatin1(sent.toPercentEncoding(" "));
        QVERIFY2(passwordSent, qPrintable(qsl("The game was not sent the password typed in the dialog, but: %1").arg(sentText)));
        QVERIFY2(sent.contains(nameLine), qPrintable(qsl("The game was not sent the name typed in the dialog, but: %1").arg(sentText)));
    }
};

#include "ConnectionDialogOpenProfileTest.moc"
MUDLET_GROUPED_TEST_MAIN(ConnectionDialogOpenProfileTest)
