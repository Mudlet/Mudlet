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
 * Drives the real connection dialog against profile folders created outside
 * of Mudlet, the way a file manager makes them (e.g. copying a profile to
 * "test (2)"). Guards the on-disk exemption in
 * dlgConnectionProfiles::validateProfile(): selecting such a folder must keep
 * its name intact (no silent character stripping, which used to rename the
 * folder on disk and lose its stored password) and must leave the
 * Connect/Offline buttons enabled - while a freshly typed name must still
 * have disallowed characters filtered out.
 *
 * Run with: ctest -R ProfileFolderNameTest -V
 */

#include "MudletInstanceCoordinator.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include <QtTest/QtTest>

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();

static void initializeQRCResources()
{
#ifdef INCLUDE_VARIABLE_SPLASH_SCREEN
    qInitResources_additional_splash_screens();
#endif
#ifdef INCLUDE_FONTS
    qInitResources_mudlet_fonts_common();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    qInitResources_mudlet_fonts_posix();
#endif
#endif
    qInitResources_mudlet();
    qInitResources_qm();
}

class ProfileFolderNameTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mXdgDir;
    QByteArray mSavedXdg;

    // The name a file manager typically produces when copying a folder; every
    // character is in the allowed set now that parentheses are permitted:
    const QString mCopiedName = qsl("test (2)");
    // Contains a character that is NOT in the allowed set, so only the
    // on-disk exemption lets it through unmangled:
    const QString mForeignName = qsl("café");
    // Trailing whitespace: the entered name arrives trimmed, so the exemption
    // has to match against the trimmed folder name to cover this one:
    const QString mPaddedName = qsl("padded café ");

    // setupConfig() consults portable.txt before the XDG logic; skip rather
    // than run against an unexpected config dir (see ConfigDirOverrideTest).
    bool portableMarkerPresent() const
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    void makeExternalProfileFolder(const QString& name) const
    {
        QVERIFY(QDir().mkpath(mudlet::getMudletPath(enums::profileHomePath, name)));
        // A folder copied by a file manager carries the original's connection
        // data files with it:
        QVERIFY(mudlet::self()->writeProfileData(name, qsl("url"), qsl("mudlet.org")).first);
        QVERIFY(mudlet::self()->writeProfileData(name, qsl("port"), qsl("23")).first);
    }

    // The Connect and Offline buttons are the only AcceptRole buttons in the
    // dialog's button box (they are private members of the dialog itself):
    QList<QAbstractButton*> acceptButtons(dlgConnectionProfiles* dialog) const
    {
        QList<QAbstractButton*> buttons;
        for (auto* button : dialog->dialog_buttonbox->buttons()) {
            if (dialog->dialog_buttonbox->buttonRole(button) == QDialogButtonBox::AcceptRole) {
                buttons << button;
            }
        }
        return buttons;
    }

    dlgConnectionProfiles* selectProfile(const QString& name)
    {
        auto* dialog = mudlet::self()->mpConnectionDialog.data();
        if (!dialog) {
            return nullptr;
        }
        const auto items = dialog->findData(*dialog->listWidget_profiles, name, dlgConnectionProfiles::csmNameRole);
        if (items.isEmpty()) {
            return nullptr;
        }
        dialog->listWidget_profiles->setCurrentItem(items.first());
        dialog->slot_itemClicked(items.first());
        return dialog;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - cannot redirect the config dir for this test");
        }
        initializeQRCResources();

        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(mXdgDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet").arg(mXdgDir.path()))); // empty dir = XDG opt-in
        qputenv("XDG_CONFIG_HOME", mXdgDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        // never touch the user's real profiles:
        QVERIFY(mudlet::getMudletPath(enums::profilesPath).startsWith(mXdgDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        makeExternalProfileFolder(mCopiedName);
        makeExternalProfileFolder(mForeignName);
        makeExternalProfileFolder(mPaddedName);

        mudlet::self()->startAutoLogin({});
        QVERIFY(QTest::qWaitFor(
                []() {
                    return mudlet::self()->mpConnectionDialog != nullptr;
                },
                5000));
    }

    void cleanupTestCase()
    {
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
        delete mudlet::self();
    }

    void test_copiedFolderWithParenthesesIsUsable()
    {
        auto* dialog = selectProfile(mCopiedName);
        QVERIFY2(dialog, "profile folder with parentheses was not listed in the dialog");

        QCOMPARE(dialog->profile_name_entry->text(), mCopiedName);
        const auto buttons = acceptButtons(dialog);
        QCOMPARE(buttons.size(), 2);
        for (auto* button : buttons) {
            QVERIFY2(button->isEnabled(), qPrintable(qsl("'%1' button is disabled for profile '%2'").arg(button->text(), mCopiedName)));
        }
        // the folder must not have been renamed behind the user's back:
        QVERIFY(QDir(mudlet::getMudletPath(enums::profileHomePath, mCopiedName)).exists());
    }

    void test_folderWithDisallowedCharacterIsNotMangled()
    {
        auto* dialog = selectProfile(mForeignName);
        QVERIFY2(dialog, "profile folder with a disallowed character was not listed in the dialog");

        QCOMPARE(dialog->profile_name_entry->text(), mForeignName);
        const auto buttons = acceptButtons(dialog);
        QCOMPARE(buttons.size(), 2);
        for (auto* button : buttons) {
            QVERIFY2(button->isEnabled(), qPrintable(qsl("'%1' button is disabled for profile '%2'").arg(button->text(), mForeignName)));
        }
        QVERIFY(QDir(mudlet::getMudletPath(enums::profileHomePath, mForeignName)).exists());
    }

    void test_folderWithTrailingWhitespaceIsNotMangled()
    {
        auto* dialog = selectProfile(mPaddedName);
        QVERIFY2(dialog, "profile folder with trailing whitespace was not listed in the dialog");

        QCOMPARE(dialog->profile_name_entry->text(), mPaddedName);
        const auto buttons = acceptButtons(dialog);
        QCOMPARE(buttons.size(), 2);
        for (auto* button : buttons) {
            QVERIFY2(button->isEnabled(), qPrintable(qsl("'%1' button is disabled for profile '%2'").arg(button->text(), mPaddedName)));
        }
        QVERIFY(QDir(mudlet::getMudletPath(enums::profileHomePath, mPaddedName)).exists());
    }

    // The exemption must not disable validation of names the user types:
    void test_editedNameIsStillValidated()
    {
        auto* dialog = selectProfile(mCopiedName);
        QVERIFY(dialog);

        // setText() drives the same textChanged path as typing does
        dialog->profile_name_entry->setText(qsl("test |2"));
        QVERIFY(!dialog->profile_name_entry->text().contains(QLatin1Char('|')));

        // restore the on-disk selection so no rename can be left pending
        dialog->profile_name_entry->setText(mCopiedName);
        QCOMPARE(dialog->profile_name_entry->text(), mCopiedName);
    }
};

QTEST_MAIN(ProfileFolderNameTest)
#include "ProfileFolderNameTest.moc"
