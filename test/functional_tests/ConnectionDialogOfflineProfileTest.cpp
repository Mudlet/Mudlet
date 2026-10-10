/***************************************************************************
 *   Copyright (C) 2026 by Michael Conley - sousesider@gmail.com           *
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
 * Whether the connection dialog offers to open a profile that has no game server
 * address, and says why Connect is unavailable. Loading one was never the broken
 * half - "--profile <name> --offline" against a profile with no url already
 * worked - so what is measured here is what the dialog makes reachable, not the
 * load path behind the button: the dialog creates such profiles itself, and
 * dimming Offline left them listed under My games and unreachable, with nothing
 * said, as a disabled button cannot show its own tooltip. See issue #10756.
 *
 * Also that showing a game from the catalog writes nothing into its folder, and
 * that what the folder then lacks still comes from the catalog wherever the
 * profile's details are read, copied or renamed.
 *
 * Run with: ctest -R ConnectionDialogOfflineProfileTest -V
 */

#include "PortableModeTestHelper.h"
#include "MudletInstanceCoordinator.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "TGameDetails.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include <QtTest/QtTest>

#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>

#include "GroupedTest.h"

using namespace std::chrono_literals;

class ConnectionDialogOfflineProfileTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mXdgDir;
    QByteArray mSavedXdg;

    // A folder and nothing else, which is what the dialog's own New button
    // leaves behind when the name is committed and the address never filled in
    const QString mAddresslessProfile = qsl("Addressless-ConnDialogOffline");
    // A game from the catalog with an empty folder, whose details all come from the catalog
    const QString mCatalogProfile = qsl("Achaea");
    // A catalog game the catalog turns TLS on for
    const QString mSecureCatalogProfile = qsl("StickMUD");
    // More catalog games with empty folders, one per case that changes what is in its folder
    const QString mPortEditedSecureCatalogProfile = qsl("Icesus");
    const QString mCopiedSecureCatalogProfile = qsl("Accursed Lands");
    const QString mRenamedCatalogProfile = qsl("Aardwolf");
    const QString mAddressSavedCatalogProfile = qsl("Slothmud");
    const QString mAutoLoginSecureCatalogProfile = qsl("MorgenGrauen");
    // A catalog game with no folder at all
    const QString mUnopenedCatalogProfile = qsl("3Scapes");
    // Not from the catalog, with every detail saved
    const QString mSavedProfile = qsl("Saved-ConnDialogOffline");
    // Every checkbox the dialog saves ticked, and no address: showing a profile after this one
    // changes each box, and validation along the way finds no address to connect to
    const QString mTickedProfile = qsl("Ticked-ConnDialogOffline");

    dlgConnectionProfiles* dialog() const { return mudlet::self()->mpConnectionDialog.data(); }

    void selectTestProfile() { selectProfile(mAddresslessProfile); }

    void selectProfile(const QString& name)
    {
        auto* pDialog = dialog();
        const auto items = pDialog->findData(*pDialog->listWidget_profiles, name, dlgConnectionProfiles::csmNameRole);
        QVERIFY2(!items.isEmpty(), qPrintable(qsl("%1 is missing from the games list").arg(name)));
        pDialog->listWidget_profiles->setCurrentItem(items.first());
    }

    QString saved(const QString& profile, const QString& item) const { return MudletApp::readProfileData(profile, item); }

    bool savedAtAll(const QString& profile, const QString& item) const { return QFileInfo::exists(MudletApp::getMudletPath(enums::profileDataItemPath, profile, item)); }

    bool waitForTheCopy() const
    {
        return QTest::qWaitFor(
                [this]() {
                    return !dialog()->mCopyingProfile;
                },
                10s);
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
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles/%2").arg(mXdgDir.path(), mAddresslessProfile)));
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles/%2").arg(mXdgDir.path(), mCatalogProfile)));
        for (const QString& name : {mSecureCatalogProfile,
                                    mPortEditedSecureCatalogProfile,
                                    mCopiedSecureCatalogProfile,
                                    mRenamedCatalogProfile,
                                    mAddressSavedCatalogProfile,
                                    mAutoLoginSecureCatalogProfile,
                                    mSavedProfile,
                                    mTickedProfile}) {
            QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles/%2").arg(mXdgDir.path(), name)));
        }
        qputenv("XDG_CONFIG_HOME", mXdgDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        QVERIFY(MudletApp::getMudletPath(enums::profilesPath).startsWith(mXdgDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        mudlet::self()->startAutoLogin({});
        QVERIFY(QTest::qWaitFor(
                []() {
                    return mudlet::self()->mpConnectionDialog && mudlet::self()->mpConnectionDialog->isVisible();
                },
                5s));
        // only now, as a profile set to log in on its own would have been opened instead of the dialog
        for (const QString& item : {qsl("ssl_tsl"), qsl("autologin"), qsl("autoreconnect")}) {
            QVERIFY(MudletApp::writeProfileData(mTickedProfile, item, QString::number(Qt::Checked)).first);
        }
        QVERIFY(MudletApp::writeProfileData(mSavedProfile, qsl("url"), qsl("example.org")).first);
        QVERIFY(MudletApp::writeProfileData(mSavedProfile, qsl("port"), qsl("4000")).first);
        QVERIFY(MudletApp::writeProfileData(mSavedProfile, qsl("ssl_tsl"), QString::number(Qt::Checked)).first);
    }

    void cleanupTestCase()
    {
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
        delete mudlet::self();
    }

    void test_offlineIsOfferedForAProfileWithNoServerAddress()
    {
        QVERIFY2(dialog(), "No connection dialog to test against");
        selectTestProfile();

        QVERIFY2(dialog()->host_name_entry->text().trimmed().isEmpty(), "The test profile has an address after all, so this proves nothing");
        QVERIFY2(dialog()->offline_button->isEnabled(), "A profile with no server address cannot be opened offline");
        QVERIFY2(!dialog()->connect_button->isEnabled(), "Connect was offered for a profile with nowhere to connect to");
    }

    // The reason lived only in the disabled button's tooltip, and Qt does not
    // deliver a tooltip to a disabled widget - so the dialog said nothing at all
    void test_theMissingAddressIsExplainedWhereItCanBeRead()
    {
        QVERIFY2(dialog(), "No connection dialog to test against");
        selectTestProfile();

        QVERIFY2(dialog()->notificationArea->isVisible(), "Nothing on screen says why this profile cannot be connected to");
        QVERIFY2(!dialog()->notificationAreaMessageBox->text().trimmed().isEmpty(), "The notification area is showing, but with no text in it");
    }

    // Typing an address and taking it away again goes through a different path
    // from selecting the profile, and that one dimmed both buttons by hand
    void test_clearingTheAddressLeavesOfflineAvailable()
    {
        QVERIFY2(dialog(), "No connection dialog to test against");
        selectTestProfile();

        dialog()->host_name_entry->setText(qsl("localhost"));
        QVERIFY2(dialog()->connect_button->isEnabled(), "An address was entered and Connect stayed dimmed");
        QVERIFY2(dialog()->offline_button->isEnabled(), "An address was entered and Offline stayed dimmed");

        dialog()->host_name_entry->clear();
        QVERIFY2(!dialog()->connect_button->isEnabled(), "Connect stayed available with the address taken away");
        QVERIFY2(dialog()->offline_button->isEnabled(), "Taking the address away took the Offline button with it");
    }

    // Showing a profile is not editing it, so the catalog's details must not be written into the
    // folder, where they would outlive any later change to the catalog
    void test_selectingACatalogGameWritesNothingIntoItsFolder()
    {
        QVERIFY2(dialog(), "No connection dialog to test against");
        selectProfile(mTickedProfile);
        QVERIFY2(dialog()->autologin_checkBox->isChecked() && dialog()->auto_reconnect->isChecked() && dialog()->port_ssl_tsl->isChecked(),
                 "The profile shown first has a box unticked, so showing the game next changes nothing in it and this proves nothing");
        QVERIFY2(!dialog()->connect_button->isEnabled(), "The profile shown first can be connected to, so Connect being offered next proves nothing");
        selectProfile(mCatalogProfile);

        QVERIFY2(!dialog()->host_name_entry->text().isEmpty(), "The catalog gave the game no address, so this proves nothing");
        QVERIFY2(!dialog()->port_entry->text().isEmpty(), "The catalog gave the game no port, so this proves nothing");
        for (const QString& item : {qsl("url"), qsl("port"), qsl("ssl_tsl"), qsl("description"), qsl("autologin"), qsl("autoreconnect")}) {
            QVERIFY2(!QFileInfo::exists(MudletApp::getMudletPath(enums::profileDataItemPath, mCatalogProfile, item)), qPrintable(qsl("selecting the game wrote its %1 file").arg(item)));
        }
        QVERIFY2(dialog()->connect_button->isEnabled(), "the catalog's details were not validated once the form was filled in");
    }

    // Editing a catalog game's port saves only the port, and that must not cost it the catalog's Secure
    void test_savingOnlyThePortOfASecureCatalogGameKeepsItSecure()
    {
        QVERIFY2(dialog(), "No connection dialog to test against");
        const auto game = TGameDetails::findGame(mPortEditedSecureCatalogProfile);
        QVERIFY2(game != TGameDetails::scmDefaultGames.end() && (*game).tlsEnabled, "The game is not one the catalog turns TLS on for, so this proves nothing");
        selectTestProfile();
        selectProfile(mPortEditedSecureCatalogProfile);
        QVERIFY2(dialog()->port_ssl_tsl->isChecked(), "the catalog turns TLS on for the game, and showing it showed Secure off");

        const QString editedPort = QString::number((*game).port + 1);
        dialog()->port_entry->setText(editedPort);
        QVERIFY2(saved(mPortEditedSecureCatalogProfile, qsl("port")) == editedPort, "editing the port did not save it, so this proves nothing");
        QVERIFY2(!savedAtAll(mPortEditedSecureCatalogProfile, qsl("ssl_tsl")), "editing the port saved Secure as well, so this proves nothing");
        selectTestProfile();
        selectProfile(mPortEditedSecureCatalogProfile);
        QVERIFY2(dialog()->port_ssl_tsl->isChecked(), "once its port was saved the game was shown with Secure off, so it would connect in plain text");

        dialog()->slot_copyProfile();
        QVERIFY2(waitForTheCopy(), "the copy never finished");
        const QString copy = mPortEditedSecureCatalogProfile + qsl("1");
        QCOMPARE(saved(copy, qsl("port")), editedPort);
        QCOMPARE(saved(copy, qsl("ssl_tsl")), QString::number(Qt::Checked));
        selectTestProfile();
        selectProfile(copy);
        QVERIFY2(dialog()->port_ssl_tsl->isChecked(), "the copy of the game was shown with Secure off");
    }

    // The player's own Secure choice is kept in ssl_tsl, which the catalog's default must not override
    void test_aSavedSecureChoiceBeatsTheCatalogs()
    {
        QVERIFY2(dialog(), "No connection dialog to test against");
        QVERIFY(MudletApp::writeProfileData(mSecureCatalogProfile, qsl("ssl_tsl"), QString::number(Qt::Unchecked)).first);
        selectTestProfile();
        selectProfile(mSecureCatalogProfile);

        QVERIFY2(!dialog()->port_entry->text().isEmpty(), "The catalog gave the game no port, so this proves nothing");
        QVERIFY2(!dialog()->port_ssl_tsl->isChecked(), "the catalog's TLS default replaced the Secure choice saved for the profile");
    }

    // A copy's name is not in the catalog, so it has to be given the details the original only had from there
    void test_aCopyOfACatalogGameKeepsItsDetails()
    {
        QVERIFY2(dialog(), "No connection dialog to test against");
        const auto game = TGameDetails::findGame(mSecureCatalogProfile);
        QVERIFY(game != TGameDetails::scmDefaultGames.end());
        QVERIFY(MudletApp::writeProfileData(mSecureCatalogProfile, qsl("ssl_tsl"), QString::number(Qt::Unchecked)).first);
        selectTestProfile();
        selectProfile(mSecureCatalogProfile);

        dialog()->slot_copyProfile();
        const QString copy = mSecureCatalogProfile + qsl("1");
        QVERIFY2(waitForTheCopy(), "the copy never finished");
        QCOMPARE(saved(copy, qsl("url")), (*game).hostUrl);
        QCOMPARE(saved(copy, qsl("port")), QString::number((*game).port));
        QVERIFY2(saved(copy, qsl("ssl_tsl")) == QString::number(Qt::Unchecked), "the copy did not keep the TLS the player had turned off");
        QCOMPARE(saved(copy, qsl("description")), (*game).description);

        selectProfile(mSecureCatalogProfile);
        dialog()->slot_copyOnlySettingsOfProfile();
        const QString settingsCopy = mSecureCatalogProfile + qsl("2");
        QCOMPARE(saved(settingsCopy, qsl("url")), (*game).hostUrl);
        QCOMPARE(saved(settingsCopy, qsl("port")), QString::number((*game).port));
        QVERIFY2(saved(settingsCopy, qsl("ssl_tsl")) == QString::number(Qt::Unchecked), "the settings copy did not keep the TLS the player had turned off");
        QCOMPARE(saved(settingsCopy, qsl("description")), (*game).description);
    }

    // What the copy is given comes from the catalog, not from a form that may hold an edit refused as invalid
    void test_aCopyOfAnUntouchedCatalogGameGetsTheCatalogsDetails()
    {
        QVERIFY2(dialog(), "No connection dialog to test against");
        const auto game = TGameDetails::findGame(mCopiedSecureCatalogProfile);
        QVERIFY2(game != TGameDetails::scmDefaultGames.end() && (*game).tlsEnabled && !(*game).websiteInfo.isEmpty(), "The game is not one this case can tell anything from");
        selectTestProfile();
        selectProfile(mCopiedSecureCatalogProfile);
        dialog()->port_entry->setText(qsl("70000"));
        QVERIFY2(!savedAtAll(mCopiedSecureCatalogProfile, qsl("port")), "the out of range port was saved, so this proves nothing");

        dialog()->slot_copyProfile();
        QVERIFY2(waitForTheCopy(), "the copy never finished");
        const QString copy = mCopiedSecureCatalogProfile + qsl("1");
        QCOMPARE(saved(copy, qsl("url")), (*game).hostUrl);
        QCOMPARE(saved(copy, qsl("port")), QString::number((*game).port));
        QCOMPARE(saved(copy, qsl("ssl_tsl")), QString::number(Qt::Checked));
        QCOMPARE(saved(copy, qsl("description")), (*game).description);
        QCOMPARE(saved(copy, qsl("website")), (*game).websiteInfo);
    }

    // ssl_tsl is one of the connection details a settings copy carries over, with the address and port
    void test_aSettingsCopyKeepsTheSecureChoice()
    {
        QVERIFY2(dialog(), "No connection dialog to test against");
        selectTestProfile();
        selectProfile(mSavedProfile);
        QVERIFY2(dialog()->port_ssl_tsl->isChecked(), "Secure is not shown ticked for the profile, so this proves nothing");

        dialog()->slot_copyOnlySettingsOfProfile();
        const QString settingsCopy = mSavedProfile + qsl("1");
        QCOMPARE(saved(settingsCopy, qsl("port")), qsl("4000"));
        QCOMPARE(saved(settingsCopy, qsl("ssl_tsl")), QString::number(Qt::Checked));
    }

    // A renamed catalog game's new name is not in the catalog either
    void test_aRenamedCatalogGameKeepsItsDetails()
    {
        QVERIFY2(dialog(), "No connection dialog to test against");
        const auto game = TGameDetails::findGame(mRenamedCatalogProfile);
        QVERIFY(game != TGameDetails::scmDefaultGames.end());
        selectTestProfile();
        selectProfile(mRenamedCatalogProfile);

        const QString renamed = mRenamedCatalogProfile + qsl("-renamed");
        dialog()->profile_name_entry->setText(renamed);
        dialog()->slot_saveName();
        QVERIFY2(QDir(MudletApp::getMudletPath(enums::profileHomePath, renamed)).exists(), "the game's folder was not renamed");
        QCOMPARE(saved(renamed, qsl("url")), (*game).hostUrl);
        QCOMPARE(saved(renamed, qsl("port")), QString::number((*game).port));
        QCOMPARE(saved(renamed, qsl("description")), (*game).description);
        QCOMPARE(saved(renamed, qsl("website")), (*game).websiteInfo);
        selectTestProfile();
        selectProfile(renamed);
        QCOMPARE(dialog()->mud_description_textedit->toPlainText(), (*game).description);
    }

    // A telnet:// link has to find a catalog game whose folder holds none of its details
    void test_aTelnetLinkFindsACatalogGameByTheCatalogsAddress()
    {
        auto* pMudlet = mudlet::self();
        const auto game = TGameDetails::findGame(mCatalogProfile);
        QVERIFY(game != TGameDetails::scmDefaultGames.end());
        QVERIFY2(!savedAtAll(mCatalogProfile, qsl("url")) && !savedAtAll(mCatalogProfile, qsl("port")), "The game's folder holds its address, so this proves nothing");
        QCOMPARE(pMudlet->findMatchingProfile((*game).hostUrl, (*game).port), mCatalogProfile);

        // with no folder there is no profile to open, so the link has to make one
        const auto unopened = TGameDetails::findGame(mUnopenedCatalogProfile);
        QVERIFY(unopened != TGameDetails::scmDefaultGames.end());
        QVERIFY(!QDir(MudletApp::getMudletPath(enums::profileHomePath, mUnopenedCatalogProfile)).exists());
        QVERIFY2(pMudlet->findMatchingProfile((*unopened).hostUrl, (*unopened).port).isEmpty(), "a link was matched to a catalog game that has no profile");

        // an address the player saved replaces the catalog's
        const auto addressSaved = TGameDetails::findGame(mAddressSavedCatalogProfile);
        QVERIFY(addressSaved != TGameDetails::scmDefaultGames.end());
        QVERIFY(MudletApp::writeProfileData(mAddressSavedCatalogProfile, qsl("url"), qsl("elsewhere.example")).first);
        QVERIFY2(pMudlet->findMatchingProfile((*addressSaved).hostUrl, (*addressSaved).port).isEmpty(), "the catalog's address was matched although the player saved another");
        QCOMPARE(pMudlet->findMatchingProfile(qsl("elsewhere.example"), (*addressSaved).port), mAddressSavedCatalogProfile);
    }

    // Opening a profile without the dialog, as autologin and --profile do, keeps the saved Secure
    // choice over the catalog's just as the dialog does. Last, as it loads the profile
    void test_openingWithoutTheDialogKeepsTheSavedSecureChoice()
    {
        const auto game = TGameDetails::findGame(mAutoLoginSecureCatalogProfile);
        QVERIFY2(game != TGameDetails::scmDefaultGames.end() && (*game).tlsEnabled, "The game is not one the catalog turns TLS on for, so this proves nothing");
        QVERIFY(MudletApp::writeProfileData(mAutoLoginSecureCatalogProfile, qsl("ssl_tsl"), QString::number(Qt::Unchecked)).first);

        mudlet::self()->doAutoLogin(mAutoLoginSecureCatalogProfile, true);
        Host* pHost = HostManager::self()->getHost(mAutoLoginSecureCatalogProfile);
        QVERIFY2(pHost, "the profile was not opened");
        QVERIFY2(!pHost->mSslTsl, "the catalog's TLS default replaced the Secure choice saved for the profile");
    }
};

MUDLET_GROUPED_TEST_MAIN(ConnectionDialogOfflineProfileTest)
#include "ConnectionDialogOfflineProfileTest.moc"
