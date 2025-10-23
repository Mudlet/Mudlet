/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016-2018, 2020-2023, 2025 by Stephen Lyons             *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2025 by Lecker Kebap - Leris@mudlet.org                 *
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


#include "dlgConnectionProfiles.h"


#include "Host.h"
#include "HostManager.h"
#include "LuaInterface.h"
#include "TGameDetails.h"
#include "XMLimport.h"
#include "mudlet.h"
#include "CredentialManager.h"
#include "SecureStringUtils.h"

#include "pre_guard.h"
#include <QtConcurrent>
#include <QtUiTools>
#include <QColorDialog>
#include <QDir>
#include <QRandomGenerator>
#include <QSettings>
#include <QSignalBlocker>
#include <QTime>
#include "post_guard.h"
#include <chrono>
#include <sstream>

using namespace std::chrono_literals;

dlgConnectionProfiles::dlgConnectionProfiles(QWidget* parent)
: QDialog(parent)
{
    setupUi(this);

    mDateTimeFormat = mudlet::self()->getUserLocale().dateTimeFormat();
    if (mDateTimeFormat.contains(QLatin1Char('t'))) {
        // There is a timezone identifier in there - which (apart from perhaps
        // the period around DST changes) we don't really need and which takes
        // up space:
        if (mDateTimeFormat.contains(QLatin1String(" t"))) {
            // Deal with the space if the time zone is appended to the end of
            // the string:
            mDateTimeFormat.remove(QLatin1String(" t"), Qt::CaseSensitive);
        } else {
            mDateTimeFormat.remove(QLatin1Char('t'), Qt::CaseSensitive);
        }
    }
    QPixmap holdPixmap;

    holdPixmap = notificationAreaIconLabelWarning->pixmap(Qt::ReturnByValue);
    holdPixmap.setDevicePixelRatio(5.3);
    notificationAreaIconLabelWarning->setPixmap(holdPixmap);

    holdPixmap = notificationAreaIconLabelError->pixmap(Qt::ReturnByValue);
    holdPixmap.setDevicePixelRatio(5.3);
    notificationAreaIconLabelError->setPixmap(holdPixmap);

    holdPixmap = notificationAreaIconLabelInformation->pixmap(Qt::ReturnByValue);
    holdPixmap.setDevicePixelRatio(5.3);
    notificationAreaIconLabelInformation->setPixmap(holdPixmap);

    // selection mode is important. if this is not set the selection behaviour is
    // undefined. this is an undocumented qt bug, as it only shows on certain OS
    // and certain architectures.

    listWidget_profiles->setSelectionMode(QAbstractItemView::SingleSelection);
    listWidget_profiles->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(listWidget_profiles, &QWidget::customContextMenuRequested, this, &dlgConnectionProfiles::slot_profileContextMenu);

    QAbstractButton* abort = dialog_buttonbox->button(QDialogButtonBox::Cancel);
    connect_button = dialog_buttonbox->addButton(tr("Connect"), QDialogButtonBox::AcceptRole);
    connect_button->setAccessibleDescription(btn_connOrLoad_disabled_accessDesc);
    offline_button = dialog_buttonbox->addButton(tr("Offline"), QDialogButtonBox::AcceptRole);
    offline_button->setAccessibleDescription(btn_connOrLoad_disabled_accessDesc);

    // Test and set if needed mudlet::mIsIconShownOnDialogButtonBoxes - if there
    // is already a Qt provided icon on a predefined button, this is probably
    // the first and best place to test this as the "Cancel" button is a built-
    // in dialog button which will have an icon if the current system style
    // settings suggest it:
    mudlet::self()->mShowIconsOnDialogs = !abort->icon().isNull();

    auto Welcome_text_template = tr("<p><center><big><b>Welcome to Mudlet!</b></big></center></p>"
                                    "<p><center><b>To get started, double-click on </b>Mudlet Tutorial<b> or select a game from the list.</b></center></p>"
                                    "<p>Want to play a game that’s not listed?</p>"
                                    "<p>Click %1 <span style=\" color:#555753;\">New</span>, then enter the <i>Profile Name</i>, <i>Server Address</i>, and <i>Port</i> in the required fields.</p>"
                                    "<p>Once you're ready, click %2 <span style=\" color:#555753;\">Connect</span> to begin your adventure.</p>"
                                    "<p>Have fun!</p><p align=\"right\"><span style=\" font-family:'Sans';\">The Mudlet Team </span>"
                                    "<img src=\":/icons/mudlet_main_16px.png\"/></p>",
                                    "Welcome message. Both %1 and %2 may be replaced by icons when this text is used.");

    auto pWelcome_document = new QTextDocument(this);

    mpCopyProfile = new QAction(tr("Copy"), this);
    mpCopyProfile->setObjectName(qsl("copyProfile"));
    auto copyProfileSettings = new QAction(tr("Copy settings only"), this);
    copyProfileSettings->setObjectName(qsl("copyProfileSettingsOnly"));

    copy_profile_toolbutton->addAction(mpCopyProfile);
    copy_profile_toolbutton->addAction(copyProfileSettings);
    copy_profile_toolbutton->setDefaultAction(mpCopyProfile);

    auto objectList = mpCopyProfile->associatedObjects();
    QList<QWidget*> widgetList;
    for (auto pObjectItem : objectList) {
        auto pWidgetItem = qobject_cast<QWidget*>(pObjectItem);
        if (pWidgetItem) {
            widgetList << pWidgetItem;
        }
    }

    Q_ASSERT_X(!widgetList.isEmpty(), "dlgConnectionProfiles::dlgConnectionProfiles(...)", "A QWidget for mpCopyProfile QAction not found.");
    widgetList.first()->setAccessibleName(tr("copy profile"));
    widgetList.first()->setAccessibleDescription(tr("copy the entire profile to new one that will require a different new name."));

    objectList = copyProfileSettings->associatedObjects();
    widgetList.clear();
    for (auto pObjectItem : objectList) {
        auto pWidgetItem = qobject_cast<QWidget*>(pObjectItem);
        if (pWidgetItem) {
            widgetList << pWidgetItem;
        }
    }

    Q_ASSERT_X(!widgetList.isEmpty(), "dlgConnectionProfiles::dlgConnectionProfiles(...)", "A QWidget for copyProfileSettings QAction not found.");
    widgetList.first()->setAccessibleName(tr("copy profile settings"));
    widgetList.first()->setAccessibleDescription(tr("copy the settings and some other parts of the profile to a new one that will require a different new name."));

    if (mudlet::self()->mShowIconsOnDialogs) {
        // Since I've switched to allowing the possibility of theme replacement
        // of icons we need a way to insert the current theme icons for
        // "dialog-ok-apply" and "edit-copy" into the help message - this is
        // awkward because Qt would normally expect to load them from a
        // resource file but this is no good in this case as we only use the
        // resource file if the icon is NOT supplied from the current theme.
        // We can fix this with a bit of fancy editing of the text - replacing a
        // particular sequence of characters with an image generated from the
        // actual icon in use.
        pWelcome_document->setHtml(qsl("<html><head/><body>%1</body></html>").arg(Welcome_text_template.arg(qsl("NEW_PROFILE_ICON"), qsl("CONNECT_PROFILE_ICON"))));

        // As we are repurposing the cancel to be a close button we do want to
        // change it anyhow:
        abort->setIcon(QIcon::fromTheme(qsl("dialog-close"), QIcon(qsl(":/icons/dialog-close.png"))));

        const QIcon icon_new(QIcon::fromTheme(qsl("document-new"), QIcon(qsl(":/icons/document-new.png"))));
        const QIcon icon_connect(QIcon::fromTheme(qsl("dialog-ok-apply"), QIcon(qsl(":/icons/preferences-web-browser-cache.png"))));

        offline_button->setIcon(QIcon(qsl(":/icons/mudlet_editor.png")));
        connect_button->setIcon(icon_connect);
        new_profile_button->setIcon(icon_new);
        remove_profile_button->setIcon(QIcon::fromTheme(qsl("edit-delete"), QIcon(qsl(":/icons/edit-delete.png"))));

        copy_profile_toolbutton->setIcon(QIcon::fromTheme(qsl("edit-copy"), QIcon(qsl(":/icons/edit-copy.png"))));
        copy_profile_toolbutton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        mpCopyProfile->setIcon(QIcon::fromTheme(qsl("edit-copy"), QIcon(qsl(":/icons/edit-copy.png"))));

        QTextCursor cursor = pWelcome_document->find(qsl("NEW_PROFILE_ICON"), 0, QTextDocument::FindWholeWords);
        // The indicated piece of marker text should be selected by the cursor
        Q_ASSERT_X(
                !cursor.isNull(), "dlgConnectionProfiles::dlgConnectionProfiles(...)", "NEW_PROFILE_ICON text marker not found in welcome_message text for when icons are shown on dialogue buttons");
        // Remove the marker:
        cursor.removeSelectedText();
        // Insert the current icon image into the same place:
        const QImage image_new(QPixmap(icon_new.pixmap(new_profile_button->iconSize())).toImage());
        cursor.insertImage(image_new);
        cursor.clearSelection();

        cursor = pWelcome_document->find(qsl("CONNECT_PROFILE_ICON"), 0, QTextDocument::FindWholeWords);
        Q_ASSERT_X(!cursor.isNull(),
                   "dlgConnectionProfiles::dlgConnectionProfiles(...)",
                   "CONNECT_PROFILE_ICON text marker not found in welcome_message text for when icons are shown on dialogue buttons");
        cursor.removeSelectedText();
        const QImage image_connect(QPixmap(icon_connect.pixmap(connect_button->iconSize())).toImage());
        cursor.insertImage(image_connect);
        cursor.clearSelection();
    } else {
        pWelcome_document->setHtml(qsl("<html><head/><body>%1</body></html>").arg(Welcome_text_template.arg(QString(), QString())));
    }

    welcome_message->setDocument(pWelcome_document);

    mpAction_revealPassword = new QAction(this);
    mpAction_revealPassword->setCheckable(true);
    mpAction_revealPassword->setObjectName(qsl("mpAction_revealPassword"));
    slot_togglePasswordVisibility(false);

    character_password_entry->addAction(mpAction_revealPassword, QLineEdit::TrailingPosition);
    if (mudlet::self()->storingPasswordsSecurely()) {
        character_password_entry->setToolTip(utils::richText(tr("Characters password, stored securely in the computer's credential manager")));
    } else {
        character_password_entry->setToolTip(utils::richText(tr("Characters password. Note that the password is not encrypted in storage")));
    }

    connect(mpAction_revealPassword, &QAction::triggered, this, &dlgConnectionProfiles::slot_togglePasswordVisibility);
    connect(offline_button, &QAbstractButton::clicked, this, &dlgConnectionProfiles::slot_load);
    connect(connect_button, &QAbstractButton::clicked, this, &dlgConnectionProfiles::accept);
    connect(abort, &QAbstractButton::clicked, this, &dlgConnectionProfiles::slot_cancel);
    connect(new_profile_button, &QAbstractButton::clicked, this, &dlgConnectionProfiles::slot_addProfile);
    connect(mpCopyProfile, &QAction::triggered, this, &dlgConnectionProfiles::slot_copyProfile);
    connect(copyProfileSettings, &QAction::triggered, this, &dlgConnectionProfiles::slot_copyOnlySettingsOfProfile);
    connect(remove_profile_button, &QAbstractButton::clicked, this, &dlgConnectionProfiles::slot_deleteProfile);
    connect(profile_name_entry, &QLineEdit::textChanged, this, &dlgConnectionProfiles::slot_updateName);
    connect(profile_name_entry, &QLineEdit::editingFinished, this, &dlgConnectionProfiles::slot_saveName);
    connect(host_name_entry, &QLineEdit::textChanged, this, &dlgConnectionProfiles::slot_updateUrl);
    connect(port_entry, &QLineEdit::textChanged, this, &dlgConnectionProfiles::slot_updatePort);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    connect(port_ssl_tsl, &QCheckBox::checkStateChanged, this, &dlgConnectionProfiles::slot_updateSslTslPort);
    connect(autologin_checkBox, &QCheckBox::checkStateChanged, this, &dlgConnectionProfiles::slot_updateAutoConnect);
    connect(auto_reconnect, &QCheckBox::checkStateChanged, this, &dlgConnectionProfiles::slot_updateAutoReconnect);
#else
    connect(port_ssl_tsl, &QCheckBox::stateChanged, this, &dlgConnectionProfiles::slot_updateSslTslPort);
    connect(autologin_checkBox, &QCheckBox::stateChanged, this, &dlgConnectionProfiles::slot_updateAutoConnect);
    connect(auto_reconnect, &QCheckBox::stateChanged, this, &dlgConnectionProfiles::slot_updateAutoReconnect);
#endif
    connect(login_entry, &QLineEdit::textEdited, this, &dlgConnectionProfiles::slot_updateLogin);
    // Use textChanged with timer debouncing to avoid saving on every keystroke
    connect(character_password_entry, &QLineEdit::textChanged, this, &dlgConnectionProfiles::slot_passwordTextChanged);

    // Listen for password migration completion to refresh the form
    connect(mudlet::self(), &mudlet::signal_passwordsMigratedToSecure, this, [this]() {
        // Refresh the current profile's password field after migration
        slot_itemClicked(listWidget_profiles->currentItem());
    });

    // Listen for character password migration completion to refresh the form
    connect(mudlet::self(), &mudlet::signal_characterPasswordsMigrated, this, [this]() {
        // Refresh the current profile's password field after migration
        slot_itemClicked(listWidget_profiles->currentItem());
    });

    connect(mud_description_textedit, &QPlainTextEdit::textChanged, this, &dlgConnectionProfiles::slot_updateDescription);
    connect(listWidget_profiles, &QListWidget::currentItemChanged, this, &dlgConnectionProfiles::slot_itemClicked);
    connect(listWidget_profiles, &QListWidget::itemDoubleClicked, this, &dlgConnectionProfiles::accept);

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    connect(discord_optin_checkBox, &QCheckBox::checkStateChanged, this, &dlgConnectionProfiles::slot_updateDiscordOptIn);
#else
    connect(discord_optin_checkBox, &QCheckBox::stateChanged, this, &dlgConnectionProfiles::slot_updateDiscordOptIn);
#endif

    // website_entry atm is only a label
    //connect(website_entry, SIGNAL(textEdited(const QString)), this, SLOT(slot_updateWebsite(const QString)));

    clearNotificationArea();

#if !defined(QT_NO_SSL)
    if (QSslSocket::supportsSsl()) {
        port_ssl_tsl->setEnabled(true);
    } else {
#endif
        port_ssl_tsl->setEnabled(false);
#if !defined(QT_NO_SSL)
    }
#endif

    mReadOnlyPalette.setColor(QPalette::Base, QColor(125, 125, 125, 25));
    mOKPalette.setColor(QPalette::Base, QColor(150, 255, 150, 50));
    mErrorPalette.setColor(QPalette::Base, QColor(255, 150, 150, 50));

    listWidget_profiles->setViewMode(QListView::IconMode);

    btn_load_enabled_accessDesc = tr("Click to load but not connect the selected profile.");
    btn_connect_enabled_accessDesc = tr("Click to load and connect the selected profile.");
    btn_connOrLoad_disabled_accessDesc = tr("Need to have a valid profile name, game server address and port before this button can be enabled.");
    item_profile_accessName = tr("Game name: %1");
    //: Some text to speech engines will spell out initials like MUD so stick to lower case if that is a better option
    item_profile_accessDesc = tr("Button to select a mud game to play, double-click it to connect and start playing it.");

    // Set up some initial black/white/greys:
    mCustomIconColors = {{QColor(0, 0, 0)}, {QColor(63, 63, 63)}, {QColor(128, 128, 128)}, {QColor(192, 192, 192)}, {QColor(255, 255, 255)}};

    // Add some color ones with evenly spaced hue
    for (quint16 i = 0; i < 360; i += 24) {
        mCustomIconColors.append(QColor::fromHsv(i, 255, 255));
        mCustomIconColors.append(QColor::fromHsv(i, 192, 255));
        mCustomIconColors.append(QColor::fromHsv(i, 128, 255));
    }

    mSearchTextTimer.setInterval(1s);
    mSearchTextTimer.setSingleShot(true);
    QCoreApplication::instance()->installEventFilter(this);
    connect(&mSearchTextTimer, &QTimer::timeout, this, &dlgConnectionProfiles::slot_reenableAllProfileItems);
}

dlgConnectionProfiles::~dlgConnectionProfiles()
{
    // Clear any pending operation flags
    mKeychainOperationInProgress = false;
    mPendingProfileLoad.clear();

    QCoreApplication::instance()->removeEventFilter(this);
}

// the dialog can be accepted by pressing Enter on an qlineedit; this is a safeguard against it
// accepting invalid data
void dlgConnectionProfiles::accept()
{
    if (validName && validUrl && validPort) {
        setVisible(false);
        // This is needed to make the above take effect as fast as possible:
        qApp->processEvents();

        // Check if keychain authentication is pending - if so, wait for it
        ensurePasswordLoadedThenConnect(true);
    }
}

void dlgConnectionProfiles::slot_load()
{
    setVisible(false);
    // This is needed to make the above take effect as fast as possible:
    qApp->processEvents();

    // Check if keychain authentication is pending - if so, wait for it
    ensurePasswordLoadedThenConnect(false);
}

void dlgConnectionProfiles::ensurePasswordLoadedThenConnect(bool alsoConnect)
{
    const QString profile_name = profile_name_entry->text().trimmed();

    if (profile_name.isEmpty()) {
        QDialog::accept();
        return;
    }

    // Check if we have any pending keychain operations for this profile
    if (hasPendingKeychainOperation(profile_name)) {
        // Queue the profile loading until keychain completes
        mPendingConnect = alsoConnect;
        mPendingProfileLoad = profile_name;
        return; // Will be handled by keychain callback
    }

    // No pending keychain operations, proceed immediately
    loadProfile(alsoConnect);
    QDialog::accept();
}

bool dlgConnectionProfiles::hasPendingKeychainOperation(const QString& profile_name) const
{
    Q_UNUSED(profile_name)
    // Simply check if we have a keychain operation in progress
    return mKeychainOperationInProgress;
}

void dlgConnectionProfiles::slot_updateDescription()
{
    QListWidgetItem* pItem = listWidget_profiles->currentItem();

    if (pItem) {
        const QString description = mud_description_textedit->toPlainText();
        writeProfileData(pItem->data(csmNameRole).toString(), qsl("description"), description);

        // don't display custom profile descriptions as a tooltip, as passwords could be stored in there
    }
}

void dlgConnectionProfiles::indicatePackagesInstallOnConnect(QStringList packages)
{
    if (!packages.length()) {
        return;
    }

    QWidget widget;
    QGroupBox* packageGroupBox = new QGroupBox("Select and load a profile to install the following package(s) into:", this);
    QVBoxLayout* packageInfoLayout = new QVBoxLayout(packageGroupBox);

    packageInfoLayout->setContentsMargins(8, 8, 8, 8);
    packageGroupBox->setStyleSheet("QGroupBox:title { padding-left: 8px; }");

    for (const QString &package : packages) {
        QFileInfo fileInfo(package);
        QString packageName = fileInfo.baseName();
        QLabel *packageLabel = new QLabel(packageName);
        packageInfoLayout->addWidget(packageLabel);
    }

    layout()->addWidget(packageGroupBox);
}

// Not used:
//void dlgConnectionProfiles::slot_updateWebsite(const QString& url)
//{
//    QListWidgetItem* pItem = listWidget_profiles->currentItem();
//    if (pItem) {
//        writeProfileData(pItem->data(csmNameRole).toString(), qsl("website"), url);
//    }
//}

void dlgConnectionProfiles::slot_updatePassword(const QString& pass)
{
    QListWidgetItem* pItem = listWidget_profiles->currentItem();
    if (!pItem) {
        return;
    }

    const QString profileName = pItem->data(csmNameRole).toString();

    if (mudlet::self()->storingPasswordsSecurely()) {
        if (pass.trimmed().isEmpty()) {
            // If password is empty, remove it from secure storage
            deleteSecurePassword(profileName);
        } else {
            // Store the password securely
            writeSecurePassword(profileName, pass);
        }
    } else {
        auto result = mudlet::self()->writeProfileData(profileName, qsl("password"), pass);
        if (!result.first) {
            qWarning().noquote().nospace() << "dlgConnectionProfiles::slot_updatePassword() ERROR - failed to save password for profile \"" << profileName << "\": " << result.second;
        }
    }
}

void dlgConnectionProfiles::writeSecurePassword(const QString& profile, const QString& pass) const
{
    // Validate that we have a password to store
    if (pass.trimmed().isEmpty()) {
        qDebug() << "dlgConnectionProfiles: Skipping storage of empty password for profile" << profile;
        return;
    }

    // Use async API for QtKeychain integration with file fallback
    auto* credManager = new CredentialManager();

    credManager->storeCredential(profile, "character", pass,
        [credManager, profile](bool success, const QString& errorMessage) {
            if (success) {
                qDebug() << "dlgConnectionProfiles: Successfully stored password for profile" << profile;
            } else {
                qWarning() << "dlgConnectionProfiles: Failed to store password for profile" << profile << ":" << errorMessage;
            }

            // Clean up the credential manager
            credManager->deleteLater();
        });
}

void dlgConnectionProfiles::deleteSecurePassword(const QString& profile) const
{
    // Use async API for QtKeychain integration with file fallback
    auto* credManager = new CredentialManager();

    credManager->removeCredential(profile, "character",
        [credManager, profile](bool success, const QString& errorMessage) {
            if (success) {
                qDebug() << "dlgConnectionProfiles: Successfully removed password for profile" << profile;
            } else {
                qWarning() << "dlgConnectionProfiles: Failed to remove password for profile" << profile << ":" << errorMessage;
            }

            // Clean up the credential manager
            credManager->deleteLater();
        });
}

void dlgConnectionProfiles::slot_updateLogin(const QString& login)
{
    QListWidgetItem* pItem = listWidget_profiles->currentItem();
    if (pItem) {
        const QString profileName = pItem->data(csmNameRole).toString();
        auto result = mudlet::self()->writeProfileData(profileName, qsl("login"), login);
        if (!result.first) {
            qWarning().noquote().nospace() << "dlgConnectionProfiles::slot_updateLogin() ERROR - failed to save character name for profile \"" << profileName << "\": " << result.second;
            // Could optionally show user notification here
        }
    }
}

void dlgConnectionProfiles::slot_updateUrl(const QString& url)
{
    if (url.isEmpty()) {
        validUrl = false;
        offline_button->setEnabled(false);
        connect_button->setEnabled(false);
        offline_button->setAccessibleDescription(btn_connOrLoad_disabled_accessDesc);
        connect_button->setAccessibleDescription(btn_connOrLoad_disabled_accessDesc);
        return;
    }

    if (validateProfile()) {
        QListWidgetItem* pItem = listWidget_profiles->currentItem();
        if (!pItem) {
            return;
        }
        writeProfileData(pItem->data(csmNameRole).toString(), qsl("url"), host_name_entry->text());
    }
}

void dlgConnectionProfiles::slot_updateAutoConnect(int state)
{
    QListWidgetItem* pItem = listWidget_profiles->currentItem();
    if (!pItem) {
        return;
    }
    writeProfileData(pItem->data(csmNameRole).toString(), qsl("autologin"), QString::number(state));
}

void dlgConnectionProfiles::slot_updateAutoReconnect(int state)
{
    QListWidgetItem* pItem = listWidget_profiles->currentItem();
    if (!pItem) {
        return;
    }
    writeProfileData(pItem->data(csmNameRole).toString(), qsl("autoreconnect"), QString::number(state));
}

// This gets called when the QCheckBox that it is connect-ed to gets its
// checked state set programmatically AS WELL as when the user clicks on it:
void dlgConnectionProfiles::slot_updateDiscordOptIn(int state)
{
    QListWidgetItem* pItem = listWidget_profiles->currentItem();
    if (!pItem) {
        return;
    }
    writeProfileData(pItem->data(csmNameRole).toString(), qsl("discordserveroptin"), QString::number(state));

    // in case the user is already connected, pull up stored GMCP data
    auto& hostManager = mudlet::self()->getHostManager();
    auto pHost = hostManager.getHost(profile_name_entry->text());
    if (!pHost) {
        return;
    }

    if (state == Qt::Checked) {
        pHost->mDiscordDisableServerSide = false;
        pHost->mTelnet.requestDiscordInfo();
    } else {
        pHost->mDiscordDisableServerSide = true;
        pHost->clearDiscordData();
    }
}

void dlgConnectionProfiles::slot_updatePort(const QString& ignoreBlank)
{
    const QString port = port_entry->text().trimmed();

    if (ignoreBlank.isEmpty()) {
        validPort = false;
        if (offline_button) {
            offline_button->setEnabled(false);
            offline_button->setAccessibleDescription(btn_connOrLoad_disabled_accessDesc);
        }
        if (connect_button) {
            connect_button->setEnabled(false);
            connect_button->setAccessibleDescription(btn_connOrLoad_disabled_accessDesc);
        }
        return;
    }

    if (validateProfile()) {
        QListWidgetItem* pItem = listWidget_profiles->currentItem();
        if (!pItem) {
            return;
        }
        writeProfileData(pItem->data(csmNameRole).toString(), qsl("port"), port);
    }
}

void dlgConnectionProfiles::slot_updateSslTslPort(int state)
{
    if (validateProfile()) {
        QListWidgetItem* pItem = listWidget_profiles->currentItem();
        if (!pItem) {
            return;
        }
        writeProfileData(pItem->data(csmNameRole).toString(), qsl("ssl_tsl"), QString::number(state));
    }
}

void dlgConnectionProfiles::slot_updateName(const QString& newName)
{
    Q_UNUSED(newName)
    validateProfile();
}

void dlgConnectionProfiles::slot_saveName()
{
    QListWidgetItem* pItem = listWidget_profiles->currentItem();
    const QString newProfileName = profile_name_entry->text().trimmed();
    const QString newProfileHost = host_name_entry->text().trimmed();
    const QString newProfilePort = port_entry->text().trimmed();
    const int newProfileSslTsl = port_ssl_tsl->isChecked() * 2;

    validateProfile();
    if (!validName || newProfileName.isEmpty() || !pItem) {
        return;
    }

    const QString currentProfileEditName = pItem->data(csmNameRole).toString();
    // don't do anything if this was just a normal click, and not an edit of any sort
    if (currentProfileEditName == newProfileName) {
        return;
    }

    // Check for orphaned keychain entries when creating a new profile with a name
    // that doesn't exist as a directory but might have keychain entries from
    // a previously deleted profile (deleted outside Mudlet interface)
    if (mudlet::self()->storingPasswordsSecurely() &&
        currentProfileEditName == tr("new profile name") &&
        !QDir(mudlet::getMudletPath(enums::profileHomePath, newProfileName)).exists()) {

        // Check if there are orphaned keychain entries for this profile name
        auto* credManager = new CredentialManager(this);
        credManager->retrievePassword(newProfileName, "character",
            [this, credManager, newProfileName, pItem, newProfileHost, newProfilePort, newProfileSslTsl]
            (bool foundCharacterEntry, const QString& characterPassword, const QString& errorMessage) {
                Q_UNUSED(characterPassword)
                Q_UNUSED(errorMessage)

                credManager->retrievePassword(newProfileName, "proxy",
                    [this, credManager, newProfileName, pItem, newProfileHost, newProfilePort, newProfileSslTsl, foundCharacterEntry]
                    (bool foundProxyEntry, const QString& proxyPassword, const QString& errorMessage) {
                        Q_UNUSED(proxyPassword)
                        Q_UNUSED(errorMessage)

                        // If we found any orphaned entries, clean them up
                        if (foundCharacterEntry || foundProxyEntry) {
                            if (foundCharacterEntry) {
                                credManager->removeCredential(newProfileName, "character",
                                    [newProfileName](bool success, const QString& errorMessage) {
                                        if (!success) {
                                            qWarning() << "dlgConnectionProfiles: Failed to clean up orphaned character password for" << newProfileName << ":" << errorMessage;
                                        }
                                    });
                            }

                            if (foundProxyEntry) {
                                credManager->removeCredential(newProfileName, "proxy",
                                    [newProfileName](bool success, const QString& errorMessage) {
                                        if (!success) {
                                            qWarning() << "dlgConnectionProfiles: Failed to clean up orphaned proxy password for" << newProfileName << ":" << errorMessage;
                                        }
                                    });
                            }
                        }

                        credManager->deleteLater();

                        // Continue with normal profile creation flow
                        continueProfileSave(pItem, newProfileName, newProfileHost, newProfilePort, newProfileSslTsl);
                    });
            });

        return; // Exit here - continueProfileSave will be called from the callback
    }

    if (mudlet::self()->storingPasswordsSecurely()) {
        migrateSecuredPassword(currentProfileEditName, newProfileName);
    }

    continueProfileSave(pItem, newProfileName, newProfileHost, newProfilePort, newProfileSslTsl);
}

void dlgConnectionProfiles::continueProfileSave(QListWidgetItem* pItem, const QString& newProfileName,
                                               const QString& newProfileHost, const QString& newProfilePort,
                                               const int newProfileSslTsl)
{
    const QString currentProfileEditName = pItem->data(csmNameRole).toString();
    setItemName(pItem, newProfileName);

    const QDir currentPath(mudlet::getMudletPath(enums::profileHomePath, currentProfileEditName));
    const QDir dir;

    if (currentPath.exists()) {
        // CHECKME: previous code specified a path ending in a '/'
        QDir parentpath(mudlet::getMudletPath(enums::profilesPath));
        if (!parentpath.rename(currentProfileEditName, newProfileName)) {
            notificationArea->show();
            notificationAreaIconLabelWarning->show();
            notificationAreaIconLabelError->hide();
            notificationAreaIconLabelInformation->hide();
            notificationAreaMessageBox->show();
            notificationAreaMessageBox->setText(tr("Could not rename your profile data on the computer."));
        }
    } else if (!dir.mkpath(mudlet::getMudletPath(enums::profileHomePath, newProfileName))) {
        notificationArea->show();
        notificationAreaIconLabelWarning->show();
        notificationAreaIconLabelError->hide();
        notificationAreaIconLabelInformation->hide();
        notificationAreaMessageBox->show();
        notificationAreaMessageBox->setText(tr("Could not create the new profile folder on your computer."));
    }

    if (!newProfileHost.isEmpty()) {
        slot_updateUrl(newProfileHost);
    }

    if (!newProfilePort.isEmpty()) {
        slot_updatePort(newProfilePort);
    }

    slot_updateSslTslPort(newProfileSslTsl);

    // if this was a previously deleted profile, restore it
    auto& settings = *mudlet::self()->mpSettings;
    auto deletedDefaultMuds = settings.value(qsl("deletedDefaultMuds"), QStringList()).toStringList();
    if (deletedDefaultMuds.contains(newProfileName)) {
        deletedDefaultMuds.removeOne(newProfileName);
        settings.setValue(qsl("deletedDefaultMuds"), deletedDefaultMuds);
        // run fillout_form to re-create the default profile icon and description
        fillout_form();
        // and re-select the profile since focus is lost
        auto pRestoredItems = findData(*listWidget_profiles, newProfileName, csmNameRole);
        Q_ASSERT_X(pRestoredItems.count() == 1, "dlgConnectionProfiles::continueProfileSave", "Couldn't find exactly 1 restored profile to select");

        // As we are using QAbstractItemView::SingleSelection this will
        // automatically unselect the previous item:
        listWidget_profiles->setCurrentItem(pRestoredItems.first());
        slot_itemClicked(pRestoredItems.first());
    } else {
        setItemName(pItem, newProfileName);
        pItem->setIcon(customIcon(newProfileName, std::nullopt));
    }
}

void dlgConnectionProfiles::slot_addProfile()
{
    profile_name_entry->setReadOnly(false);
    // while normally handled by fillout_form, due to it's asynchronous nature it is better UX to reset it here
    character_password_entry->setText(QString());
    fillout_form();
    welcome_message->hide();

    informationArea->show();
    tabWidget_connectionInfo->show();

    const QString newname = tr("new profile name");

    auto pItem = new (std::nothrow) QListWidgetItem();
    if (!pItem) {
        return;
    }
    setItemName(pItem, newname);

    listWidget_profiles->addItem(pItem);

    // insert newest entry on top of the list as the general sorting
    // is always newest item first -> fillout->form() filters
    // this is more practical for the user as they use the same profile most of the time

    // As we are using QAbstractItemView::SingleSelection this will
    // automatically unselect the previous item:
    listWidget_profiles->setCurrentItem(pItem);

    profile_name_entry->setText(newname);
    profile_name_entry->setFocus();
    profile_name_entry->selectAll();
    profile_name_entry->setReadOnly(false);
    host_name_entry->setReadOnly(false);
    port_entry->setReadOnly(false);

    validName = false;
    validUrl = false;
    validPort = false;
    offline_button->setEnabled(false);
    offline_button->setAccessibleDescription(btn_connOrLoad_disabled_accessDesc);
    connect_button->setEnabled(false);
    connect_button->setAccessibleDescription(btn_connOrLoad_disabled_accessDesc);
}

// enables the deletion button once the correct text (profile name) is entered
void dlgConnectionProfiles::slot_deleteProfileCheck(const QString& text)
{
    const QString profile = listWidget_profiles->currentItem()->data(csmNameRole).toString();
    if (profile != text) {
        delete_button->setEnabled(false);
    } else {
        delete_button->setEnabled(true);
        delete_button->setFocus();
    }
}

// actually performs the deletion once the correct text has been entered
void dlgConnectionProfiles::slot_reallyDeleteProfile()
{
    const QString profile = listWidget_profiles->currentItem()->data(csmNameRole).toString();
    reallyDeleteProfile(profile);
}

void dlgConnectionProfiles::reallyDeleteProfile(const QString& profile)
{
    QDir dir(mudlet::getMudletPath(enums::profileHomePath, profile));
    dir.removeRecursively();

    // Clean up keychain entries for the deleted profile
    if (mudlet::self()->storingPasswordsSecurely()) {
        auto* credManager = new CredentialManager(this);

        // Clean up character password entry
        credManager->removeCredential(profile, "character",
            [profile](bool success, const QString& errorMessage) {
                if (!success) {
                    qWarning() << "dlgConnectionProfiles: Failed to clean up character password for deleted profile" << profile << ":" << errorMessage;
                }
            });

        // Clean up proxy password entry (if any)
        credManager->removeCredential(profile, "proxy",
            [credManager, profile](bool success, const QString& errorMessage) {
                if (!success) {
                    qWarning() << "dlgConnectionProfiles: Failed to clean up proxy password for deleted profile" << profile << ":" << errorMessage;
                }

                // Clean up the credential manager after both operations
                credManager->deleteLater();
            });
    }

    // record the deleted default profile so it does not get re-created in the future
    auto& settings = *mudlet::self()->mpSettings;
    auto deletedDefaultMuds = settings.value(qsl("deletedDefaultMuds"), QStringList()).toStringList();
    if (!deletedDefaultMuds.contains(profile)) {
        deletedDefaultMuds.append(profile);
    }
    settings.setValue(qsl("deletedDefaultMuds"), deletedDefaultMuds);

    fillout_form();
    listWidget_profiles->setFocus();
}

// called when the 'delete' button is pressed, raises a dialog to confirm deletion
// if this profile has been used
void dlgConnectionProfiles::slot_deleteProfile()
{
    if (!listWidget_profiles->currentItem()) {
        return;
    }

    const QString profile = listWidget_profiles->currentItem()->data(csmNameRole).toString();
    const QStringList& onlyShownPredefinedProfiles{mudlet::self()->mOnlyShownPredefinedProfiles};
    if (!onlyShownPredefinedProfiles.isEmpty() && onlyShownPredefinedProfiles.contains(profile)) {
        // Do NOT allow deletion of the prioritised predefined MUD:
        return;
    }

    const QDir profileDirContents(mudlet::getMudletPath(enums::profileXmlFilesPath, profile));
    if (!profileDirContents.exists() || profileDirContents.isEmpty()) {
        // shortcut - don't show profile deletion confirmation if there is no data to delete
        reallyDeleteProfile(profile);
        return;
    }

    QUiLoader loader;

    QFile file(qsl(":/ui/delete_profile_confirmation.ui"));
    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "dlgConnectionProfiles: failed to open UI file for reading:" << file.errorString();
        return;
    }

    auto* delete_profile_dialog = dynamic_cast<QDialog*>(loader.load(&file, this));
    file.close();

    if (!delete_profile_dialog) {
        return;
    }

    delete_profile_lineedit = delete_profile_dialog->findChild<QLineEdit*>(qsl("delete_profile_lineedit"));
    delete_button = delete_profile_dialog->findChild<QPushButton*>(qsl("delete_button"));
    auto* cancel_button = delete_profile_dialog->findChild<QPushButton*>(qsl("cancel_button"));

    if (!delete_profile_lineedit || !delete_button || !cancel_button) {
        return;
    }

    connect(delete_profile_lineedit, &QLineEdit::textChanged, this, &dlgConnectionProfiles::slot_deleteProfileCheck);
    connect(delete_profile_dialog, &QDialog::accepted, this, &dlgConnectionProfiles::slot_reallyDeleteProfile);

    delete_profile_lineedit->setPlaceholderText(profile);
    delete_profile_lineedit->setFocus();
    delete_button->setEnabled(false);
    delete_profile_dialog->setWindowTitle(tr("Deleting '%1'").arg(profile));
    delete_profile_dialog->setAttribute(Qt::WA_DeleteOnClose);

    delete_profile_dialog->show();
    delete_profile_dialog->raise();
}

QString dlgConnectionProfiles::readProfileData(const QString& profile, const QString& item) const
{
    QFile file(mudlet::getMudletPath(enums::profileDataItemPath, profile, item));
    const bool success = file.open(QIODevice::ReadOnly);
    QString ret;
    if (success) {
        QDataStream ifs(&file);
        if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
            ifs.setVersion(mudlet::scmQDataStreamFormat_5_12);
        }
        ifs >> ret;
        file.close();
    }

    return ret;
}

QPair<bool, QString> dlgConnectionProfiles::writeProfileData(const QString& profile, const QString& item, const QString& what)
{
    QSaveFile file(mudlet::getMudletPath(enums::profileDataItemPath, profile, item));
    if (file.open(QIODevice::WriteOnly | QIODevice::Unbuffered)) {
        QDataStream ofs(&file);
        if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
            ofs.setVersion(mudlet::scmQDataStreamFormat_5_12);
        }
        ofs << what;
        if (!file.commit()) {
            qDebug().noquote().nospace() << "dlgConnectionProfiles::writeProfileData(...) ERROR - writing profile: \"" << profile << "\", item: \"" << item << "\", reason: \"" << file.errorString() << "\".";
        }
    }

    if (file.error() == QFileDevice::NoError) {
        return {true, QString()};
    } else {
        return {false, file.errorString()};
    }
}

QString dlgConnectionProfiles::getDescription(const QString& profile_name) const
{
    QString profileDesc = readProfileData(profile_name, qsl("description"));

    if (profileDesc.isEmpty()) {
        auto itDetails = TGameDetails::findGame(profile_name);
        if (itDetails != TGameDetails::scmDefaultGames.constEnd()) {
            if (!(*itDetails).description.isEmpty()) {
                return (*itDetails).description;
            }
        }
    }

    return profileDesc;
}

void dlgConnectionProfiles::slot_itemClicked(QListWidgetItem* pItem)
{
    if (!pItem) {
        qDebug() << "dlgConnectionProfiles::slot_itemClicked() called with null item";
        return;
    }

    const QString profile_name = pItem->data(csmNameRole).toString();

    // Prevent rapid duplicate clicks on the same profile
    static QString lastProfileClicked;
    static QTime lastClickTime;
    
    if (profile_name == lastProfileClicked && lastClickTime.isValid() && lastClickTime.msecsTo(QTime::currentTime()) < 100) {
        return;
    }
    
    lastProfileClicked = profile_name;
    lastClickTime = QTime::currentTime();

    slot_togglePasswordVisibility(false);

    profile_name_entry->setText(profile_name);

    QString host_url = readProfileData(profile_name, qsl("url"));
    if (host_url.isEmpty()) {
        // Host to connect to, see below for port
        auto it = TGameDetails::findGame(profile_name);
        if (it != TGameDetails::scmDefaultGames.end()) {
            host_url = (*it).hostUrl;
        }
    }
    host_name_entry->setText(host_url);

    QString host_port = readProfileData(profile_name, qsl("port"));
    QString val = readProfileData(profile_name, qsl("ssl_tsl"));
    if (val.toInt() == Qt::Checked) {
        port_ssl_tsl->setChecked(true);
    } else {
        port_ssl_tsl->setChecked(false);
    }

    if (host_port.isEmpty()) {
        auto it = TGameDetails::findGame(profile_name);
        if (it != TGameDetails::scmDefaultGames.end()) {
            host_port = QString::number((*it).port);
            port_ssl_tsl->setChecked((*it).tlsEnabled);
        }
    }

    port_entry->setText(host_port);

    // if we're currently copying a profile, don't blank and re-load the password,
    // because there isn't one in storage yet. It'll be copied over into the widget
    // by the copy method
    if (!mCopyingProfile) {
        character_password_entry->setText(QString());
        // Schedule password loading asynchronously to avoid event loop issues
        auto* timer = new QTimer(this);
        timer->setSingleShot(true);
        timer->setProperty("profileName", profile_name);
        connect(timer, &QTimer::timeout, this, &dlgConnectionProfiles::slot_loadPasswordAsync);
        timer->start(0);
    }

    val = readProfileData(profile_name, qsl("login"));
    login_entry->setText(val);

    val = readProfileData(profile_name, qsl("autologin"));
    if (val.toInt() == Qt::Checked) {
        autologin_checkBox->setChecked(true);
    } else {
        autologin_checkBox->setChecked(false);
    }

    val = readProfileData(profile_name, qsl("autoreconnect"));
    if (!val.isEmpty() && val.toInt() == Qt::Checked) {
        auto_reconnect->setChecked(true);
    } else {
        auto_reconnect->setChecked(false);
    }

    mDiscordApplicationId = readProfileData(profile_name, qsl("discordApplicationId"));
    mDiscordInviteURL = readProfileData(profile_name, qsl("discordInviteURL"));

    // val will be null if this is the first time the profile has been read
    // since an update to a Mudlet version supporting Discord - so a toint()
    // will return 0 - which just happens to be Qt::Unchecked() but let's not
    // rely on that...
    val = readProfileData(profile_name, qsl("discordserveroptin"));
    if ((!val.isEmpty()) && val.toInt() == Qt::Checked) {
        discord_optin_checkBox->setChecked(true);
    } else {
        discord_optin_checkBox->setChecked(false);
    }

    updateDiscordStatus();

    mud_description_textedit->setPlainText(getDescription(profile_name));

    val = readProfileData(profile_name, qsl("website"));
    if (val.isEmpty()) {
        auto it = TGameDetails::findGame(profile_name);
        if (it != TGameDetails::scmDefaultGames.end()) {
            val = (*it).websiteInfo;
        }
        website_entry->setVisible(!val.isEmpty());
    } else {
        website_entry->show();
    }
    website_entry->setText(val);

    profile_history->clear();

    QDir dir(mudlet::getMudletPath(enums::profileXmlFilesPath, profile_name));
    dir.setSorting(QDir::Time);
    const QStringList entries = dir.entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Time);

    for (const auto& entry : entries) {
        const QRegularExpression rx(qsl("(\\d+)\\-(\\d+)\\-(\\d+)#(\\d+)\\-(\\d+)\\-(\\d+).xml"));
        const QRegularExpressionMatch match = rx.match(entry);

        if (match.capturedStart() != -1) {
            QString day;
            const QString month = match.captured(2);
            QString year;
            const QString hour = match.captured(4);
            const QString minute = match.captured(5);
            const QString second = match.captured(6);
            if (match.captured(1).toInt() > 31 && match.captured(3).toInt() >= 1 && match.captured(3).toInt() <= 31) {
                // I have been experimenting with code that puts the year first
                // which is actually quite useful - this accommodates such cases
                // as well... - SlySven
                year = match.captured(1);
                day = match.captured(3);
            } else {
                day = match.captured(1);
                year = match.captured(3);
            }

            QDateTime datetime;
            datetime.setTime(QTime(hour.toInt(), minute.toInt(), second.toInt()));
            datetime.setDate(QDate(year.toInt(), month.toInt(), day.toInt()));
            profile_history->addItem(mudlet::self()->getUserLocale().toString(datetime, mDateTimeFormat), QVariant(entry));
        } else if (entry == QLatin1String("autosave.xml")) {
            const QFileInfo fileInfo(dir, entry);
            auto lastModified = fileInfo.lastModified();
            profile_history->addItem(QIcon::fromTheme(qsl("document-save"), QIcon(qsl(":/icons/document-save.png"))),
                                     mudlet::self()->getUserLocale().toString(lastModified, mDateTimeFormat),
                                     QVariant(entry));
        } else if (entry.endsWith(QLatin1String(".xml"), Qt::CaseInsensitive)) {
            profile_history->addItem(entry, QVariant(entry)); // if it has a custom name, use it as it is
        }
    }

    profile_history->setEnabled(static_cast<bool>(profile_history->count()));

    const QString profileLoadedMessage = tr("This profile is currently loaded - close it before changing the connection parameters.");

    if (mudlet::self()->getHostManager().getHost(profile_name)) {
        remove_profile_button->setEnabled(false);
        remove_profile_button->setToolTip(utils::richText(tr("A profile that is in use cannot be removed")));
        connect_button->setEnabled(false);
        offline_button->setEnabled(false);

        profile_name_entry->setReadOnly(true);
        host_name_entry->setReadOnly(true);
        port_entry->setReadOnly(true);

        profile_name_entry->setFocusPolicy(Qt::NoFocus);
        host_name_entry->setFocusPolicy(Qt::NoFocus);
        port_entry->setFocusPolicy(Qt::NoFocus);

        profile_name_entry->setPalette(mReadOnlyPalette);
        host_name_entry->setPalette(mReadOnlyPalette);
        port_entry->setPalette(mReadOnlyPalette);

        notificationArea->show();
        notificationAreaIconLabelWarning->hide();
        notificationAreaIconLabelError->hide();
        notificationAreaIconLabelInformation->show();
        notificationAreaMessageBox->show();
        notificationAreaMessageBox->setText(profileLoadedMessage);
    } else {
        profile_name_entry->setReadOnly(false);
        host_name_entry->setReadOnly(false);
        port_entry->setReadOnly(false);

        profile_name_entry->setFocusPolicy(Qt::StrongFocus);
        host_name_entry->setFocusPolicy(Qt::StrongFocus);
        port_entry->setFocusPolicy(Qt::StrongFocus);

        profile_name_entry->setPalette(mRegularPalette);
        host_name_entry->setPalette(mRegularPalette);
        port_entry->setPalette(mRegularPalette);

        if (notificationAreaMessageBox->text() == profileLoadedMessage) {
            clearNotificationArea();
        }
        remove_profile_button->setEnabled(true);
        remove_profile_button->setToolTip(QString());
    }
}

void dlgConnectionProfiles::updateDiscordStatus()
{
    auto discordLoaded = mudlet::self()->mDiscord.libraryLoaded();

    if (!discordLoaded) {
        discord_optin_checkBox->setEnabled(false);
        discord_optin_checkBox->setChecked(false);
        discord_optin_checkBox->setToolTip(utils::richText(tr("Discord integration not available on this platform")));
    } else if (mDiscordApplicationId.isEmpty() && !mudlet::self()->mDiscord.gameIntegrationSupported(host_name_entry->text().trimmed()).first) {
        // Disable discord support if it is not recognised by name and a
        // Application Id has not been previously entered:
        discord_optin_checkBox->setEnabled(false);
        discord_optin_checkBox->setChecked(false);
        discord_optin_checkBox->setToolTip(utils::richText(tr("Discord integration not supported by game")));
    } else {
        discord_optin_checkBox->setEnabled(true);
        discord_optin_checkBox->setToolTip(utils::richText(tr("Check to enable Discord integration")));
    }
}

// (re-)creates the dialogs profile list
void dlgConnectionProfiles::fillout_form()
{
    listWidget_profiles->clear();
    profile_name_entry->clear();
    host_name_entry->clear();
    port_entry->clear();

    mProfileList = QDir(mudlet::getMudletPath(enums::profilesPath)).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    if (mProfileList.isEmpty()) {
        welcome_message->show();
        tabWidget_connectionInfo->hide();
        informationArea->hide();

// collapse the width as the default is too big and set the height to a reasonable default
// to fit all of the 'Welcome' message
#if defined(Q_OS_MACOS)
        // macOS requires 15px more width to get 3 columns of MUD listings in
        resize(minimumSize().width() + 15, 300);
#else
        resize(minimumSize().width(), 300);
#endif
    } else {
        welcome_message->hide();

        tabWidget_connectionInfo->show();
        informationArea->show();
    }

    listWidget_profiles->setIconSize(QSize(120, 30));
    QString description;
    QListWidgetItem* pItem;

    auto& settings = *mudlet::self()->mpSettings;
    auto deletedDefaultMuds = settings.value(qsl("deletedDefaultMuds"), QStringList()).toStringList();
    const QStringList& onlyShownPredefinedProfiles{mudlet::self()->mOnlyShownPredefinedProfiles};
    if (onlyShownPredefinedProfiles.isEmpty()) {
        const auto defaultGames = TGameDetails::keys();
        for (auto& game : defaultGames) {
            if (!deletedDefaultMuds.contains(game)) {
                pItem = new QListWidgetItem();
                auto details = TGameDetails::findGame(game);
                setupMudProfile(pItem, game, (*details).description, (*details).icon);
            }
        }

#if defined(QT_DEBUG)
        const QString mudServer = qsl("Mudlet self-test");
        if (!deletedDefaultMuds.contains(mudServer) && !mProfileList.contains(mudServer)) {
            mProfileList.append(mudServer);
            pItem = new QListWidgetItem();
            // Can't use setupMudProfile(...) here as we do not set the icon in the same way:
            setItemName(pItem, mudServer);

            listWidget_profiles->addItem(pItem);
            description = getDescription(qsl("mudlet.org"));
            if (!description.isEmpty()) {
                pItem->setToolTip(utils::richText(description));
            }
        }
#endif
    } else {
        for (const QString& onlyShownPredefinedProfile : onlyShownPredefinedProfiles) {
            pItem = new QListWidgetItem();
            auto details = TGameDetails::findGame(onlyShownPredefinedProfile);
            setupMudProfile(pItem, onlyShownPredefinedProfile, (*details).description, (*details).icon);
        }
    }

    setProfileIcon();

    QDateTime test_date;
    QString toselectProfileName;
    int toselectRow = -1;
    int test_profile_row = -1;
    int predefined_profile_row = -1;
    bool firstMudletLaunch = true;

    for (int i = 0; i < listWidget_profiles->count(); i++) {
        const auto profile = listWidget_profiles->item(i);
        const auto profileName = profile->data(csmNameRole).toString();
        if (profileName == qsl("Mudlet self-test")) {
            test_profile_row = i;
        }
        const auto fileinfo = QFileInfo(mudlet::getMudletPath(enums::profileXmlFilesPath, profileName));
        if (fileinfo.exists()) {
            firstMudletLaunch = false;
            const QDateTime profile_lastRead = fileinfo.lastModified();
            // Since Qt 5.x null QTimes and QDateTimes are invalid - and might not
            // work as expected - so test for validity of the test_date value as well
            if ((!test_date.isValid()) || profile_lastRead > test_date) {
                test_date = profile_lastRead;
                toselectProfileName = profileName;
                toselectRow = i;
            }
        }
        if (!onlyShownPredefinedProfiles.isEmpty() && profileName == onlyShownPredefinedProfiles.first()) {
            predefined_profile_row = i;
        }
    }

    if (firstMudletLaunch) {
        if (onlyShownPredefinedProfiles.isEmpty()) {
            // Select a random pre-defined profile to give all MUDs a fair go first time
            // make sure not to select the test_profile though
            if (listWidget_profiles->count() > 1) {
                while (toselectRow == -1 || toselectRow == test_profile_row) {
                    toselectRow = QRandomGenerator::global()->bounded(listWidget_profiles->count());
                }
            }
        } else if (predefined_profile_row >= 0) {
            // If the user is starting one of a MUD's "dedicated" Mudlet versions then
            // select the first of THAT/THOSE predefined one(s) on first launch:
            toselectRow = predefined_profile_row;
        }
    }

    if (toselectRow != -1) {
        listWidget_profiles->setCurrentRow(toselectRow);
    }

    updateDiscordStatus();
}

void dlgConnectionProfiles::setProfileIcon() const
{
    const QStringList defaultGames = TGameDetails::keys();

    for (const QString& profileName : mProfileList) {
        if (profileName.isEmpty()) {
            continue;
        }

        if (hasCustomIcon(profileName)) {
            loadCustomProfile(profileName);
        } else {
            // mProfileList is derived from a filesystem directory, but MacOS is not
            // necessarily case preserving for file names so any tests on them
            // should be case insensitive
            // skip creating icons for default MUDs as they are already created above
            if (defaultGames.contains(profileName, Qt::CaseInsensitive)) {
                continue;
            }

            // This will instantiate a new QListWidgetItem for the profile:
            generateCustomProfile(profileName);
        }
    }
}

bool dlgConnectionProfiles::hasCustomIcon(const QString& profileName) const
{
    return QFileInfo::exists(mudlet::getMudletPath(enums::profileDataItemPath, profileName, qsl("profileicon")));
}

void dlgConnectionProfiles::loadCustomProfile(const QString& profileName) const
{
    auto pItem = new QListWidgetItem();
    setItemName(pItem, profileName);

    setCustomIcon(profileName, pItem);
    auto description = getDescription(profileName);
    if (!description.isEmpty()) {
        pItem->setToolTip(utils::richText(description));
    }
    listWidget_profiles->addItem(pItem);
}

void dlgConnectionProfiles::setCustomIcon(const QString& profileName, QListWidgetItem* profile) const
{
    auto profileIconPath = mudlet::getMudletPath(enums::profileDataItemPath, profileName, qsl("profileicon"));
    auto icon = QIcon(QPixmap(profileIconPath).scaled(QSize(120, 30), Qt::IgnoreAspectRatio, Qt::SmoothTransformation).copy());
    profile->setIcon(icon);
}

// When a profile is renamed, migrate password storage to the new profile
void dlgConnectionProfiles::migrateSecuredPassword(const QString& oldProfile, const QString& newProfile)
{
    const auto& password = character_password_entry->text().trimmed();

    deleteSecurePassword(oldProfile);

    // Only store the password if it's not empty
    if (!password.isEmpty()) {
        writeSecurePassword(newProfile, password);
    }
}

template <typename L>
void dlgConnectionProfiles::loadSecuredPassword(const QString& profile, L callback)
{
    // Use async API for QtKeychain integration with file fallback
    auto* credManager = new CredentialManager();

    credManager->retrievePassword(profile, "character",
        [credManager, callback = std::move(callback)](bool success, const QString& password, const QString& errorMessage) {
            if (success) {
                callback(password);
                QString passwordCopy = password; // Make a copy for secure clearing
                SecureStringUtils::secureStringClear(passwordCopy);
            } else {
                if (!errorMessage.isEmpty()) {
                    qDebug() << "dlgConnectionProfiles: Failed to retrieve password:" << errorMessage;
                }
                callback(QString()); // Call with empty string on failure
            }

            // Clean up the credential manager
            credManager->deleteLater();
        });
}

std::optional<QColor> getCustomColor(const QString& profileName)
{
    auto profileColorPath = mudlet::getMudletPath(enums::profileDataItemPath, profileName, qsl("profilecolor"));
    if (QFileInfo::exists(profileColorPath)) {
        QFile file(profileColorPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return std::nullopt;
        }

        QTextStream in(&file);
        const QString colorString = in.readLine();
        QColor color(colorString);
        if (color.isValid()) {
            return {color};
        }
    }
    return std::nullopt;
}

void dlgConnectionProfiles::generateCustomProfile(const QString& profileName) const
{
    auto pItem = new QListWidgetItem();
    setItemName(pItem, profileName);
    pItem->setIcon(customIcon(profileName, getCustomColor(profileName)));
    listWidget_profiles->addItem(pItem);
}

void dlgConnectionProfiles::slot_profileContextMenu(QPoint pos)
{
    const QPoint globalPos = listWidget_profiles->mapToGlobal(pos);
    auto profileName = listWidget_profiles->currentItem()->data(csmNameRole).toString();

    QMenu menu;
    if (hasCustomIcon(profileName)) {
        //: Reset the custom picture for this profile in the connection dialog and show the default one instead
        menu.addAction(tr("Reset icon"), this, &dlgConnectionProfiles::slot_resetCustomIcon);
    } else {
        menu.addAction(QIcon(":/icons/mudlet_main_16px.png"),
                       //: Set a custom picture to show for the profile in the connection dialog
                       tr("Set custom icon"),
                       this,
                       &dlgConnectionProfiles::slot_setCustomIcon);
        menu.addAction(QIcon(":/icons/mudlet_main_16px.png"),
                       //: Set a custom color to show for the profile in the connection dialog
                       tr("Set custom color"),
                       this,
                       &dlgConnectionProfiles::slot_setCustomColor);
    }

    menu.exec(globalPos);
}

void dlgConnectionProfiles::slot_setCustomIcon()
{
    auto profileName = listWidget_profiles->currentItem()->data(csmNameRole).toString();

    QSettings& settings = *mudlet::getQSettings();
    QString lastDir = settings.value("lastFileDialogLocation", QDir::homePath()).toString();

    const QString imageLocation = QFileDialog::getOpenFileName(
            this, tr("Select custom image for profile (should be 120x30)"), lastDir, tr("Images (%1)").arg(qsl("*.png *.gif *.jpg")));
    if (imageLocation.isEmpty()) {
        return;
    }

    lastDir = QFileInfo(imageLocation).absolutePath();
    settings.setValue("lastFileDialogLocation", lastDir);

    const bool success = mudlet::self()->setProfileIcon(profileName, imageLocation).first;
    if (!success) {
        return;
    }

    auto icon = QIcon(QPixmap(imageLocation).scaled(QSize(120, 30), Qt::IgnoreAspectRatio, Qt::SmoothTransformation).copy());
    listWidget_profiles->currentItem()->setIcon(icon);
}
void dlgConnectionProfiles::slot_setCustomColor()
{
    auto profileName = listWidget_profiles->currentItem()->data(csmNameRole).toString();
    QColor color = QColorDialog::getColor(getCustomColor(profileName).value_or(QColor(255, 255, 255)));
    if (color.isValid()) {
        auto profileColorPath = mudlet::getMudletPath(enums::profileDataItemPath, profileName, qsl("profilecolor"));
        QSaveFile file(profileColorPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qWarning() << "dlgConnectionProfiles: failed to open profile color file for writing:" << file.errorString();
            return;
        }
        auto colorName = color.name();
        file.write(colorName.toUtf8(), colorName.length());
        if (!file.commit()) {
            qDebug() << "dlgConnectionProfiles::slot_setCustomColor: error saving custom icon color: " << file.errorString();
        }
        listWidget_profiles->currentItem()->setIcon(customIcon(profileName, {color}));
    }
}
void dlgConnectionProfiles::slot_resetCustomIcon()
{
    auto profileName = listWidget_profiles->currentItem()->data(csmNameRole).toString();

    const bool success = mudlet::self()->resetProfileIcon(profileName).first;
    if (!success) {
        return;
    }

    auto currentRow = listWidget_profiles->currentRow();
    fillout_form();
    listWidget_profiles->setCurrentRow(currentRow);
}

void dlgConnectionProfiles::slot_cancel()
{
    // QDialog::Rejected is the enum value (= 0) return value for a "cancelled"
    // outcome...
    QDialog::done(QDialog::Rejected);
}

void dlgConnectionProfiles::slot_copyProfile()
{
    mCopyingProfile = true;

    QString profile_name;
    QString oldname;
    QListWidgetItem* pItem;
    const auto oldPassword = character_password_entry->text();

    if (!copyProfileWidget(profile_name, oldname, pItem)) {
        mCopyingProfile = false;
        return;
    }

    // copy the folder on-disk
    const QDir dir(mudlet::getMudletPath(enums::profileHomePath, oldname));
    if (!dir.exists()) {
        mCopyingProfile = false;
        return;
    }

    QApplication::setOverrideCursor(Qt::BusyCursor);
    mpCopyProfile->setText(tr("Copying..."));
    mpCopyProfile->setEnabled(false);
    auto future = QtConcurrent::run(dlgConnectionProfiles::copyFolder, mudlet::getMudletPath(enums::profileHomePath, oldname), mudlet::getMudletPath(enums::profileHomePath, profile_name));
    auto watcher = new QFutureWatcher<bool>;
    connect(watcher, &QFutureWatcher<bool>::finished, this, [=, this]() {
        mProfileList << profile_name;
        slot_itemClicked(pItem);
        // Clear the Discord optin on the copied profile - just because the source
        // one may have had it enabled does not mean we can assume the new one would
        // want it set:
        discord_optin_checkBox->setChecked(false);

        // restore the password, which won't be copied by the disk copy if stored in the credential manager
        // Temporarily block textChanged signal to avoid triggering save on programmatic setText
        {
            const QSignalBlocker blocker(character_password_entry);
            character_password_entry->setText(oldPassword);
        }
        
        if (mudlet::self()->storingPasswordsSecurely() && !oldPassword.trimmed().isEmpty()) {
            writeSecurePassword(profile_name, oldPassword);
        }
        mCopyingProfile = false;
        mpCopyProfile->setText(tr("Copy"));
        mpCopyProfile->setEnabled(true);
        QApplication::restoreOverrideCursor();
        validateProfile();
    });
    watcher->setFuture(future);
}

void dlgConnectionProfiles::slot_copyOnlySettingsOfProfile()
{
    QString profile_name;
    QString oldname;
    QListWidgetItem* pItem;
    if (!copyProfileWidget(profile_name, oldname, pItem)) {
        return;
    }

    const QDir newProfileDir(mudlet::getMudletPath(enums::profileHomePath, profile_name));
    newProfileDir.mkpath(newProfileDir.path());
    if (!newProfileDir.exists()) {
        return;
    }

    // copy relevant profile files
    for (const QString& file : {qsl("url"), qsl("port"), qsl("password"), qsl("login"), qsl("description")}) {
        auto filePath = qsl("%1/%2").arg(mudlet::getMudletPath(enums::profileHomePath, oldname), file);
        auto newFilePath = qsl("%1/%2").arg(mudlet::getMudletPath(enums::profileHomePath, profile_name), file);
        QFile::copy(filePath, newFilePath);
    }

    copyProfileSettingsOnly(oldname, profile_name);

    mProfileList << profile_name;
    slot_itemClicked(pItem);
    // Clear the Discord optin on the copied profile - just because the source
    // one may have had it enabled does not mean we can assume the new one would
    // want it set:
    discord_optin_checkBox->setChecked(false);
}

bool dlgConnectionProfiles::copyProfileWidget(QString& profile_name, QString& oldname, QListWidgetItem*& pItem) const
{
    profile_name = profile_name_entry->text().trimmed();
    oldname = profile_name;
    if (profile_name.isEmpty()) {
        return false;
    }

    // prepend n+1 to end of the profile name
    if (profile_name.at(profile_name.size() - 1).isDigit()) {
        int i = 1;
        do {
            profile_name = profile_name.left(profile_name.size() - 1) + QString::number(profile_name.at(profile_name.size() - 1).digitValue() + i++);
        } while (mProfileList.contains(profile_name));
    } else {
        int i = 1;
        QString profile_name2;
        do {
            profile_name2 = profile_name + QString::number(i++);
        } while (mProfileList.contains(profile_name2));
        profile_name = profile_name2;
    }

    pItem = new (std::nothrow) QListWidgetItem();
    if (!pItem) {
        return false;
    }
    setItemName(pItem, profile_name);

    // add the new widget in
    listWidget_profiles->addItem(pItem);
    pItem->setIcon(customIcon(profile_name, std::nullopt));
    listWidget_profiles->setCurrentItem(pItem);

    profile_name_entry->setText(profile_name);
    profile_name_entry->setFocus();
    profile_name_entry->selectAll();
    profile_name_entry->setReadOnly(false);
    host_name_entry->setReadOnly(false);
    port_entry->setReadOnly(false);

    return true;
}

void dlgConnectionProfiles::copyProfileSettingsOnly(const QString& oldname, const QString& newname)
{
    const QDir oldProfiledir(mudlet::getMudletPath(enums::profileXmlFilesPath, oldname));
    const QDir newProfiledir(mudlet::getMudletPath(enums::profileXmlFilesPath, newname));
    newProfiledir.mkpath(newProfiledir.absolutePath());
    QStringList entries = oldProfiledir.entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Time);
    if (entries.empty()) {
        return;
    }

    auto copySettingsFromFile = oldProfiledir.absoluteFilePath(entries.first());

    pugi::xml_document newProfileXml;
    if (extractSettingsFromProfile(newProfileXml, copySettingsFromFile)) {
        saveProfileCopy(newProfiledir, newProfileXml);
    }
}

bool dlgConnectionProfiles::extractSettingsFromProfile(pugi::xml_document& newProfile, const QString& copySettingsFrom)
{
    pugi::xml_document oldProfile;
    pugi::xml_parse_result const result = oldProfile.load_file(copySettingsFrom.toUtf8().constData());
    if (!result) {
        qWarning() << "dlgConnectionProfiles::copyProfileSettingsOnly() ERROR: couldn't parse" << copySettingsFrom;
        qWarning() << "Parse error: " << result.description() << ", character pos= " << result.offset;
        return false;
    }

    // write header
    auto declaration = newProfile.prepend_child(pugi::node_declaration);
    declaration.append_attribute("version") = "1.0";
    declaration.append_attribute("encoding") = "UTF-8";
    newProfile.append_child(pugi::node_doctype).set_value("MudletPackage");

    // copy /MudletPackage attributes
    auto mudletPackage = newProfile.append_child("MudletPackage");
    const auto attributeNodes = oldProfile.select_nodes("/MudletPackage/attribute::*");
    for (pugi::xpath_node_set::const_iterator it = attributeNodes.begin(); it != attributeNodes.end(); ++it) {
        auto node = *it;
        mudletPackage.append_attribute(node.attribute().name()) = node.attribute().value();
    }

    // remove installed packages/modules
    const auto hostPackageResults = oldProfile.select_nodes("/MudletPackage/HostPackage");
    pugi::xml_node const hostPackage = hostPackageResults.first().node();
    auto host = hostPackage.child("Host");
    host.remove_child("mInstalledPackages");
    host.remove_child("mInstalledModules");

    // copy in the /Mudlet/HostPackage
    mudletPackage.append_copy(hostPackage);
    return true;
}

// save profile using Qt's API's which handle non-ASCII characters in Windows paths fine
void dlgConnectionProfiles::saveProfileCopy(const QDir& newProfiledir, const pugi::xml_document& newProfileXml) const
{
    QSaveFile file(newProfiledir.absoluteFilePath(qsl("Copied profile (settings only).xml")));
    if (!file.open(QFile::WriteOnly)) {
        qDebug() << "dlgConnectionProfiles::copyProfileSettingsOnly ERROR - couldn't create new profile file:" << file.fileName() << "-" << file.errorString();
        return;
    }

    std::stringstream saveStringStream(std::ios::out);
    newProfileXml.save(saveStringStream);
    std::string output(saveStringStream.str());
    file.write(output.data());
    if (!file.commit()) {
        qDebug() << "dlgConnectionProfiles::saveProfileCopy: error copying profile: " << file.errorString();
    }
}

void dlgConnectionProfiles::loadProfile(bool alsoConnect)
{
    const QString profile_name = profile_name_entry->text().trimmed();

    if (profile_name.isEmpty()) {
        return;
    }

    // Check if the host already exists before calling mudlet::loadProfile()
    Host* pHostBeforeLoad = mudlet::self()->getHostManager().getHost(profile_name);
    bool hostExistedBefore = (pHostBeforeLoad != nullptr);

    Host *pHost = mudlet::self()->loadProfile(profile_name, alsoConnect, profile_history->currentData().toString());

    // overwrite the generic profile with user supplied name, url and login information
    if (pHost) {

        Host* pActiveHost = mudlet::self()->getActiveHost();

        if (pActiveHost && pActiveHost->getName() == profile_name) {
            // Skip reconnect if mudlet::loadProfile already connected for existing hosts
            if (alsoConnect && hostExistedBefore) {
                QDialog::accept();
                return;
            }
            // Reconnect to the active profile
            pActiveHost->mTelnet.reconnect();
            QDialog::accept();
            return;
        }

        // Skip signal emission if mudlet::loadProfile already handled the connection
        if (alsoConnect && hostExistedBefore) {
            QDialog::accept();
            return;
        }

        pHost->setName(profile_name);

        if (!host_name_entry->text().trimmed().isEmpty()) {
            pHost->setUrl(host_name_entry->text().trimmed());
        } else {
            slot_updateUrl(pHost->getUrl());
        }

        if (!port_entry->text().trimmed().isEmpty()) {
            pHost->setPort(port_entry->text().trimmed().toInt());
        } else {
            slot_updatePort(QString::number(pHost->getPort()));
        }

        pHost->mSslTsl = port_ssl_tsl->isChecked();

        if (!character_password_entry->text().trimmed().isEmpty()) {
            pHost->setPass(character_password_entry->text().trimmed());
        } else {
            slot_updatePassword(pHost->getPass());
        }

        if (!login_entry->text().trimmed().isEmpty()) {
            pHost->setLogin(login_entry->text().trimmed());
        } else {
            slot_updateLogin(pHost->getLogin());
        }

        // This settings also need to be configured, note that the only time not to
        // save the setting is on profile loading:
        pHost->mTelnet.setEncoding(readProfileData(profile_name, qsl("encoding")).toUtf8(), false);
        // Needed to ensure setting is correct on start-up:
        pHost->setWideAmbiguousEAsianGlyphs(pHost->getWideAmbiguousEAsianGlyphsControlState());
        pHost->setAutoReconnect(auto_reconnect->isChecked());

        // This also writes the value out to the profile's base directory:
        mudlet::self()->mDiscord.setApplicationID(pHost, mDiscordApplicationId);
    }

    emit signal_load_profile(profile_name, alsoConnect);
    QDialog::accept();
}

bool dlgConnectionProfiles::validateProfile()
{
    bool valid = true;

    // don't validate url duplication during copy, as information will already exist when we try to set it
    if (mCopyingProfile) {
        return true;
    }

    validName = true, validPort = true, validUrl = true;

    clearNotificationArea();

    QListWidgetItem* pItem = listWidget_profiles->currentItem();

    if (pItem) {
        QString name = profile_name_entry->text().trimmed();
        const QString allowedChars = qsl(". _0123456789-#&aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ");

        for (int i = 0; i < name.size(); ++i) {
            if (!allowedChars.contains(name.at(i))) {
                notificationAreaIconLabelWarning->show();
                notificationAreaMessageBox->setText(
                        qsl("%1\n%2\n%3\n").arg(notificationAreaMessageBox->text(), tr("The %1 character is not permitted. Use one of the following:").arg(name.at(i)), allowedChars));
                name.replace(name.at(i--), QString());
                profile_name_entry->setText(name);
                validName = false;
                valid = false;
                break;
            }
        }

        // see if there is an edit that already uses a similar name
        if ((QString::compare(pItem->data(csmNameRole).toString(), name, Qt::CaseInsensitive) != 0) && mProfileList.contains(name, Qt::CaseInsensitive)) {
            notificationAreaIconLabelError->show();
            notificationAreaMessageBox->setText(qsl("%1\n%2").arg(notificationAreaMessageBox->text(), tr("This profile name is already in use.")));
            validName = false;
            valid = false;
        }

        const QString port = port_entry->text().trimmed();
        if (!port.isEmpty() && (port.indexOf(QRegularExpression(qsl("^\\d+$")), 0) == -1)) {
            QString val = port;
            val.chop(1);
            port_entry->setText(val);
            notificationAreaIconLabelError->show();
            notificationAreaMessageBox->setText(qsl("%1\n%2").arg(notificationAreaMessageBox->text(), tr("You have to enter a number. Other characters are not permitted.")));
            port_entry->setPalette(mErrorPalette);
            validPort = false;
            valid = false;
        }

        bool ok;
        const int num = port.trimmed().toInt(&ok);
        if (!port.isEmpty() && (num > 65536 && ok)) {
            notificationAreaIconLabelError->show();
            notificationAreaMessageBox->setText(qsl("%1\n%2\n\n").arg(notificationAreaMessageBox->text(), tr("Port number must be above zero and below 65535.")));
            port_entry->setPalette(mErrorPalette);
            validPort = false;
            valid = false;
        }

#if defined(QT_NO_SSL)
        port_ssl_tsl->setEnabled(false);
        port_ssl_tsl->setToolTip(utils::richText(tr("Mudlet is not configured for secure connections.")));
        if (port_ssl_tsl->isChecked()) {
            notificationAreaIconLabelError->show();
            notificationAreaMessageBox->setText(qsl("%1\n%2\n\n").arg(notificationAreaMessageBox->text(), tr("Mudlet is not configured for secure connections.")));
            port_ssl_tsl->setEnabled(true);
            validPort = false;
            valid = false;
        }
#else
        if (!QSslSocket::supportsSsl()) {
            if (port_ssl_tsl->isChecked()) {
                notificationAreaIconLabelError->show();
                notificationAreaMessageBox->setText(qsl("%1\n%2\n\n").arg(notificationAreaMessageBox->text(), tr("Mudlet can not load support for secure connections.")));
                validPort = false;
                valid = false;
            }
        } else {
            port_ssl_tsl->setEnabled(true);
            port_ssl_tsl->setToolTip(QString());
        }
#endif

        QUrl check;
        const QString url = host_name_entry->text().trimmed();
        check.setHost(url);

        if (url.isEmpty()) {
            host_name_entry->setPalette(mErrorPalette);
            validUrl = false;
            valid = false;
        }

        if (!check.isValid()) {
            notificationAreaIconLabelError->show();
            notificationAreaMessageBox->setText(qsl("%1\n%2\n\n%3").arg(notificationAreaMessageBox->text(), tr("Please enter the URL or IP address of the Game server."), check.errorString()));
            host_name_entry->setPalette(mErrorPalette);
            validUrl = false;
            valid = false;
        }

        if (url.indexOf(QRegularExpression(qsl("^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$")), 0) != -1) {
            if (port_ssl_tsl->isChecked()) {
                notificationAreaIconLabelError->show();
                notificationAreaMessageBox->setText(
                        qsl("%1\n%2\n\n%3").arg(notificationAreaMessageBox->text(), tr("Please enter the URL or IP address of the Game server."), check.errorString()));
                host_name_entry->setPalette(mErrorPalette);
                validUrl = false;
                valid = false;
            }
        }

        if (url.indexOf(QRegularExpression(qsl("^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$")), 0) != -1) {
            if (port_ssl_tsl->isChecked()) {
                notificationAreaIconLabelError->show();
                notificationAreaMessageBox->setText(
                        qsl("%1\n%2\n\n%3").arg(notificationAreaMessageBox->text(), tr("SSL connections require the URL of the Game server."), check.errorString()));
                host_name_entry->setPalette(mErrorPalette);
                validUrl = false;
                valid = false;
            }
        }

        if (valid) {
            port_entry->setPalette(mOKPalette);
            host_name_entry->setPalette(mOKPalette);
            clearNotificationArea();
            validName = true;
            validPort = true;
            validUrl = true;

            if (offline_button) {
                offline_button->setEnabled(true);
                offline_button->setToolTip(utils::richText(tr("Load profile without connecting.")));
                offline_button->setAccessibleDescription(btn_load_enabled_accessDesc);
            }
            if (connect_button) {
                connect_button->setEnabled(true);
                connect_button->setToolTip(QString());
                connect_button->setAccessibleDescription(btn_connect_enabled_accessDesc);
            }
            return true;
        } else {
            if (!notificationAreaMessageBox->text().isEmpty()) {
                notificationArea->show();
                notificationAreaMessageBox->show();
            }
            if (offline_button) {
                offline_button->setEnabled(false);
                offline_button->setToolTip(utils::richText(tr("Please set a valid profile name, game server address and the game port before loading.")));
                offline_button->setAccessibleDescription(btn_connOrLoad_disabled_accessDesc);
            }
            if (connect_button) {
                connect_button->setEnabled(false);
                connect_button->setToolTip(utils::richText(tr("Please set a valid profile name, game server address and the game port before connecting.")));
                connect_button->setAccessibleDescription(btn_connOrLoad_disabled_accessDesc);
            }
            return false;
        }
    }
    return false;
}

// credit: http://www.qtcentre.org/archive/index.php/t-23469.html
bool dlgConnectionProfiles::copyFolder(const QString& sourceFolder, const QString& destFolder)
{
    const QDir sourceDir(sourceFolder);
    if (!sourceDir.exists()) {
        return false;
    }

    const QDir destDir(destFolder);
    if (!destDir.exists()) {
        destDir.mkdir(destFolder);
    }
    QStringList files = sourceDir.entryList(QDir::Files);
    for (const QString& file : files) {
        const QString srcName = sourceFolder + QDir::separator() + file;
        const QString destName = destFolder + QDir::separator() + file;
        QFile::copy(srcName, destName);
    }
    files.clear();
    files = sourceDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    for (const QString& file : files) {
        const QString srcName = sourceFolder + QDir::separator() + file;
        const QString destName = destFolder + QDir::separator() + file;
        copyFolder(srcName, destName);
    }
    return true;
}

// As it is wired to the triggered() signal it is only called that way when
// the user clicks on the action, and not when setChecked() is used.
void dlgConnectionProfiles::slot_togglePasswordVisibility(const bool showPassword)
{
    if (mpAction_revealPassword->isChecked() != showPassword) {
        // This will only be reached and needed by a call NOT prompted by the
        // user clicking on the icon - i.e. either when a different profile is
        // selected or when called from the constructor:
        mpAction_revealPassword->setChecked(showPassword);
    }

    if (mpAction_revealPassword->isChecked()) {
        character_password_entry->setEchoMode(QLineEdit::Normal);
        // In practice I could not get the icon to change based upon supplying
        // different QPixmaps for the QIcon for different states - so let's do it
        // directly:
        mpAction_revealPassword->setIcon(QIcon::fromTheme(qsl("password-show-on"), QIcon(qsl(":/icons/password-show-on.png"))));
        mpAction_revealPassword->setToolTip(utils::richText(tr("Click to hide the password; it will also hide if another profile is selected.")));
    } else {
        character_password_entry->setEchoMode(QLineEdit::Password);
        mpAction_revealPassword->setIcon(QIcon::fromTheme(qsl("password-show-off"), QIcon(qsl(":/icons/password-show-off.png"))));
        mpAction_revealPassword->setToolTip(utils::richText(tr("Click to reveal the password for this profile.")));
    }
}

QList<QListWidgetItem*> dlgConnectionProfiles::findData(const QListWidget& listWidget, const QVariant& what, const int role) const
{
    QList<QListWidgetItem*> results;
    for (int index = 0, total = listWidget.count(); index < total; ++index) {
        if (listWidget.item(index)->data(role) == what) {
            results.append(listWidget.item(index));
        }
    }
    return results;
}

QList<int> dlgConnectionProfiles::findProfilesBeginningWith(const QString& what) const
{
    QList<int> results;
    for (int index = 0, total = listWidget_profiles->count(); index < total; ++index) {
        if (listWidget_profiles->item(index)->data(csmNameRole).toString().startsWith(what, Qt::CaseInsensitive)) {
            results.append(index);
        }
    }
    return results;
}

void dlgConnectionProfiles::setItemName(QListWidgetItem* pI, const QString& name) const
{
    if (!pI) {
        // Avoid any problems should the supplied argument be a nullptr:
        return;
    }

    pI->setData(csmNameRole, name);
    pI->setData(Qt::AccessibleTextRole, item_profile_accessName.arg(name));
    pI->setData(Qt::AccessibleDescriptionRole, item_profile_accessDesc);
}

void dlgConnectionProfiles::setupMudProfile(QListWidgetItem* pItem, const QString& mudServer, const QString& serverDescription, const QString& iconFileName)
{
    setItemName(pItem, mudServer);

    listWidget_profiles->addItem(pItem);
    if (!hasCustomIcon(mudServer)) {
        const QPixmap pixmap(iconFileName);
        if (pixmap.isNull()) {
            qWarning() << mudServer << "doesn't have a valid icon";
            return;
        }
        if (pixmap.width() != 120) {
            pItem->setIcon(pixmap.scaled(QSize(120, 30), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        } else {
            pItem->setIcon(QIcon(iconFileName));
        }
    } else {
        setCustomIcon(mudServer, pItem);
    }
    if (!serverDescription.isEmpty()) {
        pItem->setToolTip(utils::richText(serverDescription));
    }
}

QIcon dlgConnectionProfiles::customIcon(const QString& text, const std::optional<QColor>& backgroundColor) const
{
    QPixmap background(120, 30);

    const QColor color = backgroundColor.value_or(mCustomIconColors.at(static_cast<int>((qHash(text) * 8131) % mCustomIconColors.count())));
    background.fill(color);

    // Set to one larger than wanted so that do loop can contain the decrementor
    int fontSize = 30;
    QFont font(qsl("Bitstream Vera Sans Mono"), fontSize, QFont::Normal);
    // For an icon of size 120x30 allow 89x29 for the text:
    const QRect textRectangle(0, 0, 89, 29);
    QRect testRect;
    // Really long names will be drawn very small (font size 6) with the ends clipped off:
    do {
        font.setPointSize(--fontSize);
        const QFontMetrics metrics(font);
        testRect = metrics.boundingRect(textRectangle, Qt::AlignCenter | Qt::TextWordWrap, text);
    } while (fontSize > 6 && !textRectangle.contains(testRect));

    { // Enclosed in braces to limit lifespan of QPainter:
        QPainter painter(&background);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        const QPixmap pixmap(qsl(":/icons/mudlet_main_32px.png"));
        painter.drawPixmap(QRect(5, 5, 20, 20), pixmap);
        if (color.lightness() > 127) {
            painter.setPen(Qt::black);
        } else {
            painter.setPen(Qt::white);
        }
        painter.setFont(font);
        painter.drawText(QRect(30, 0, 90, 30), Qt::AlignCenter | Qt::TextWordWrap, text);
    }
    return QIcon(background);
}

void dlgConnectionProfiles::clearNotificationArea()
{
    notificationArea->hide();
    notificationAreaIconLabelWarning->hide();
    notificationAreaIconLabelError->hide();
    notificationAreaIconLabelInformation->hide();
    notificationAreaMessageBox->clear();
}

void dlgConnectionProfiles::slot_reenableAllProfileItems()
{
    for (int i = 0, total = listWidget_profiles->count(); i < total; ++i) {
        listWidget_profiles->item(i)->setFlags(listWidget_profiles->item(i)->flags() | Qt::ItemIsEnabled);
    }
}

bool dlgConnectionProfiles::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == listWidget_profiles && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        switch (keyEvent->key()) {
            // Process all the keys that could be used in a profile name
            // fortunately we limit this to a sub-set of ASCII because we also use
            // it for a directory name - based on "allowedChars" list in
            // validateProfile() i.e.:
            // ". _0123456789-#&aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ"
        default:
            // For other keys handle them as normal:
            return QObject::eventFilter(obj, event);

        case Qt::Key_Escape:
            // Clear the search:
            mSearchText.clear();
            slot_reenableAllProfileItems();
            // Eat (filter) this event so it goes no further:
            return true;

        case Qt::Key_Period:
        case Qt::Key_Space:
        case Qt::Key_Underscore:
        case Qt::Key_0:
        case Qt::Key_1:
        case Qt::Key_2:
        case Qt::Key_3:
        case Qt::Key_4:
        case Qt::Key_5:
        case Qt::Key_6:
        case Qt::Key_7:
        case Qt::Key_8:
        case Qt::Key_9:
        case Qt::Key_Minus:
        case Qt::Key_NumberSign:
        case Qt::Key_Ampersand:
        case Qt::Key_A:
        case Qt::Key_B:
        case Qt::Key_C:
        case Qt::Key_D:
        case Qt::Key_E:
        case Qt::Key_F:
        case Qt::Key_G:
        case Qt::Key_H:
        case Qt::Key_I:
        case Qt::Key_J:
        case Qt::Key_K:
        case Qt::Key_L:
        case Qt::Key_M:
        case Qt::Key_N:
        case Qt::Key_O:
        case Qt::Key_P:
        case Qt::Key_Q:
        case Qt::Key_R:
        case Qt::Key_S:
        case Qt::Key_T:
        case Qt::Key_U:
        case Qt::Key_V:
        case Qt::Key_W:
        case Qt::Key_X:
        case Qt::Key_Y:
        case Qt::Key_Z:
            if (keyEvent->modifiers() & ~(Qt::ShiftModifier)) {
                // There is a modifier in play OTHER than the shift one so treat
                // it as normal:
                return QObject::eventFilter(obj, event);
            }

            if (!mSearchTextTimer.isActive()) {
                // Too long since the last keypress so forget any previously
                // entered keypresses:
                mSearchText.clear();
            }
            mSearchTextTimer.stop();
            addLetterToProfileSearch(keyEvent->key());
            // Restart the timeout for another keypress:
            mSearchTextTimer.start();
            // Eat (filter) this event so it goes no further:
            return true;
        }
    }

    // standard event processing
    return QObject::eventFilter(obj, event);
}

void dlgConnectionProfiles::addLetterToProfileSearch(const int key)
{
    if ((key < 0) || (key > 128)) {
        // out of range of normal ASCII keys
        return;
    }

    // As it happens the values for key correspond to those of the corresponding
    // ASCII (upper-case for letters) character codes
    mSearchText.append(QLatin1Char(static_cast<unsigned char>(key)));
    auto indexes = findProfilesBeginningWith(mSearchText);

    if (indexes.isEmpty()) {
        // No matches at all so clearing search term and reset all profiles to
        // be enabled:
        mSearchText.clear();
        slot_reenableAllProfileItems();
        return;
    }

    for (int i = 0, total = listWidget_profiles->count(); i < total; ++i) {
        auto flags = listWidget_profiles->item(i)->flags();
        if (indexes.isEmpty() || !indexes.contains(i)) {
            flags &= ~Qt::ItemIsEnabled;
        } else {
            flags |= Qt::ItemIsEnabled;
        }
        listWidget_profiles->item(i)->setFlags(flags);
    }

    listWidget_profiles->setCurrentRow(indexes.first());
}

void dlgConnectionProfiles::slot_loadPasswordAsync()
{
    if (!sender()) {
        return;
    }

    // Get the profile name from the timer's property
    QTimer* timer = qobject_cast<QTimer*>(sender());
    if (!timer) {
        return;
    }

    const QString profile_name = timer->property("profileName").toString();

    // Prevent duplicate password loading operations for the same profile
    if (mKeychainOperationInProgress) {
        return;
    }

    if (profile_name.isEmpty()) {
        return;
    }

    // Clean up the timer
    timer->deleteLater();

    // Check if this dialog is still valid and the profile is still selected
    if (listWidget_profiles->currentItem() == nullptr) {
        return;
    }

    const QString currentProfileName = listWidget_profiles->currentItem()->data(csmNameRole).toString();

    if (currentProfileName != profile_name) {
        // Selection has changed, ignore this async load
        return;
    }

    // If secure storage is enabled, try keychain first, then fallback to QSettings
    if (mudlet::self()->storingPasswordsSecurely()) {
        mKeychainOperationInProgress = true;
        auto* credManager = new CredentialManager(this);
        credManager->retrievePassword(profile_name, "character",
            [this, credManager, profile_name](bool success, const QString& retrievedPassword, const QString& errorMessage) {
                // Clear the operation flag first
                mKeychainOperationInProgress = false;

                // Check if profile selection has changed while we were waiting
                if (listWidget_profiles->currentItem() &&
                    listWidget_profiles->currentItem()->data(csmNameRole).toString() == profile_name) {

                    if (success) {
                        // Keychain operation succeeded - set the password (even if empty)
                        // Temporarily block textChanged signal to avoid triggering save on programmatic setText
                        {
                            const QSignalBlocker blocker(character_password_entry);
                            character_password_entry->setText(retrievedPassword);
                        }
                        
                        if (retrievedPassword.isEmpty()) {
                            qDebug() << "dlgConnectionProfiles: Keychain returned empty password for" << profile_name;
                        } else {
                            qDebug() << "dlgConnectionProfiles: Successfully loaded password from keychain for" << profile_name;
                        }
                    } else {
                        // Fallback to QSettings only if keychain operation failed
                        loadPasswordFromSettings(profile_name);
                        qDebug() << "dlgConnectionProfiles: Keychain failed for" << profile_name << ", using file fallback:" << errorMessage;
                    }
                }

                // Check if there's a pending connection waiting for this password load
                // (do this regardless of profile selection state to avoid hanging)
                if (!mPendingProfileLoad.isEmpty() && mPendingProfileLoad == profile_name) {
                    qDebug() << "dlgConnectionProfiles: Password load completed, proceeding with pending connection for" << profile_name;

                    // Clear pending state
                    QString profileToLoad = mPendingProfileLoad;
                    bool shouldConnect = mPendingConnect;
                    mPendingProfileLoad.clear();

                    // Proceed with the connection
                    loadProfile(shouldConnect);
                    QDialog::accept();
                }

                credManager->deleteLater();
            });
    } else {
        // Secure storage disabled, use QSettings directly
        loadPasswordFromSettings(profile_name);

        // Check if there's a pending connection waiting
        if (!mPendingProfileLoad.isEmpty() && mPendingProfileLoad == profile_name) {
            qDebug() << "dlgConnectionProfiles: Password loaded from settings, proceeding with pending connection for" << profile_name;

            // Clear pending state
            QString profileToLoad = mPendingProfileLoad;
            bool shouldConnect = mPendingConnect;
            mPendingProfileLoad.clear();

            // Proceed with the connection
            loadProfile(shouldConnect);
            QDialog::accept();
        }
    }
}

void dlgConnectionProfiles::loadPasswordFromSettings(const QString& profile_name)
{
    auto& settings = *mudlet::self()->mpSettings;
    settings.beginGroup(qsl("profiles/%1").arg(profile_name));

    // Get password and handle migration
    const QString password = settings.value(qsl("password"), QString()).toString();
    const QString oldPassword = settings.value(qsl("login"), QString()).toString();

    // Temporarily block textChanged signal to avoid triggering save on programmatic setText
    {
        const QSignalBlocker blocker(character_password_entry);
        
        if (!password.isEmpty()) {
            character_password_entry->setText(password);
        } else if (!oldPassword.isEmpty()) {
            // Migrate old password
            character_password_entry->setText(oldPassword);
            settings.setValue(qsl("password"), oldPassword);
            settings.remove(qsl("login"));
        } else {
            character_password_entry->setText(QString());
        }
    }

    settings.endGroup();
}

void dlgConnectionProfiles::slot_passwordTextChanged()
{
    // Cancel any pending password save
    if (mPasswordSaveTimer) {
        mPasswordSaveTimer->stop();
    } else {
        mPasswordSaveTimer = new QTimer(this);
        mPasswordSaveTimer->setSingleShot(true);
        mPasswordSaveTimer->setInterval(500); // 500ms debounce
        connect(mPasswordSaveTimer, &QTimer::timeout, this, [this]() {
            QListWidgetItem* pItem = listWidget_profiles->currentItem();
            if (pItem) {
                slot_updatePassword(character_password_entry->text());
            }
        });
    }
    mPasswordSaveTimer->start();
}
