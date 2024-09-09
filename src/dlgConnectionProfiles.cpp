/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016-2018, 2020-2023 by Stephen Lyons                   *
 *                                               - slysven@virginmedia.com *
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

#include "pre_guard.h"
#include <QtConcurrent>
#include <QtUiTools>
#include <QColorDialog>
#include <QDir>
#include <QRandomGenerator>
#include <QSettings>
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

#if (QT_VERSION) >= (QT_VERSION_CHECK(5, 15, 0))
    holdPixmap = notificationAreaIconLabelWarning->pixmap(Qt::ReturnByValue);
#else
    holdPixmap = *(this->notificationAreaIconLabelWarning->pixmap());
#endif
    holdPixmap.setDevicePixelRatio(5.3);
    notificationAreaIconLabelWarning->setPixmap(holdPixmap);

#if (QT_VERSION) >= (QT_VERSION_CHECK(5, 15, 0))
    holdPixmap = notificationAreaIconLabelError->pixmap(Qt::ReturnByValue);
#else
    holdPixmap = *(this->notificationAreaIconLabelError->pixmap());
#endif
    holdPixmap.setDevicePixelRatio(5.3);
    notificationAreaIconLabelError->setPixmap(holdPixmap);

#if (QT_VERSION) >= (QT_VERSION_CHECK(5, 15, 0))
    holdPixmap = notificationAreaIconLabelInformation->pixmap(Qt::ReturnByValue);
#else
    holdPixmap = *(notificationAreaIconLabelInformation->pixmap());
#endif
    holdPixmap.setDevicePixelRatio(5.3);
    notificationAreaIconLabelInformation->setPixmap(holdPixmap);

    // selection mode is important. if this is not set the selection behaviour is
    // undefined. this is an undocumented qt bug, as it only shows on certain OS
    // and certain architectures.

    profiles_tree_widget->setSelectionMode(QAbstractItemView::SingleSelection);
    profiles_tree_widget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(profiles_tree_widget, &QWidget::customContextMenuRequested, this, &dlgConnectionProfiles::slot_profileContextMenu);

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
                                    "<p><center><b>Click on one of the games on the list to play.</b></center></p>"
                                    "<p>To play a game not in the list, click on %1 "
                                    "<span style=\" color:#555753;\">New</span>, fill in the <i>Profile Name</i>, "
                                    "<i>Server address</i>, and <i>Port</i> fields in the <i>Required </i> area.</p>"
                                    "<p>After that, click %2 <span style=\" color:#555753;\">Connect</span> "
                                    "to play.</p>"
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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto widgetList = mpCopyProfile->associatedWidgets();
#else
    // QAction::associatedWidgets() has been deprecated in Qt 6
    auto objectList = mpCopyProfile->associatedObjects();
    QList<QWidget*> widgetList;
    for (auto pObjectItem : objectList) {
        auto pWidgetItem = qobject_cast<QWidget*>(pObjectItem);
        if (pWidgetItem) {
            widgetList << pWidgetItem;
        }
    }
#endif
    Q_ASSERT_X(!widgetList.isEmpty(), "dlgConnectionProfiles::dlgConnectionProfiles(...)", "A QWidget for mpCopyProfile QAction not found.");
    widgetList.first()->setAccessibleName(tr("copy profile"));
    widgetList.first()->setAccessibleDescription(tr("copy the entire profile to new one that will require a different new name."));

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    widgetList = copyProfileSettings->associatedWidgets();
#else
    objectList = copyProfileSettings->associatedObjects();
    widgetList.clear();
    for (auto pObjectItem : objectList) {
        auto pWidgetItem = qobject_cast<QWidget*>(pObjectItem);
        if (pWidgetItem) {
            widgetList << pWidgetItem;
        }
    }
#endif
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
    connect(profile_name_entry, &QLineEdit::textEdited, this, &dlgConnectionProfiles::slot_updateName);
    connect(profile_name_entry, &QLineEdit::editingFinished, this, &dlgConnectionProfiles::slot_saveName);
    connect(host_name_entry, &QLineEdit::textChanged, this, &dlgConnectionProfiles::slot_updateUrl);
    connect(port_entry, &QLineEdit::textChanged, this, &dlgConnectionProfiles::slot_updatePort);
    connect(port_ssl_tsl, &QCheckBox::stateChanged, this, &dlgConnectionProfiles::slot_updateSslTslPort);
    connect(autologin_checkBox, &QCheckBox::stateChanged, this, &dlgConnectionProfiles::slot_updateAutoConnect);
    connect(auto_reconnect, &QCheckBox::stateChanged, this, &dlgConnectionProfiles::slot_updateAutoReconnect);
    connect(login_entry, &QLineEdit::textEdited, this, &dlgConnectionProfiles::slot_updateLogin);
    connect(character_password_entry, &QLineEdit::textEdited, this, &dlgConnectionProfiles::slot_updatePassword);
    connect(mud_description_textedit, &QPlainTextEdit::textChanged, this, &dlgConnectionProfiles::slot_updateDescription);
    connect(profiles_tree_widget, &QListWidget::currentItemChanged, this, &dlgConnectionProfiles::slot_itemClicked);
    connect(profiles_tree_widget, &QListWidget::itemDoubleClicked, this, &dlgConnectionProfiles::accept);

    connect(discord_optin_checkBox, &QCheckBox::stateChanged, this, &dlgConnectionProfiles::slot_updateDiscordOptIn);

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

    profiles_tree_widget->setViewMode(QListView::IconMode);

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
        loadProfile(true);
        QDialog::accept();
    }
}

void dlgConnectionProfiles::slot_updateDescription()
{
    QListWidgetItem* pItem = profiles_tree_widget->currentItem();

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
//    QListWidgetItem* pItem = profiles_tree_widget->currentItem();
//    if (pItem) {
//        writeProfileData(pItem->data(csmNameRole).toString(), qsl("website"), url);
//    }
//}

void dlgConnectionProfiles::slot_updatePassword(const QString& pass)
{
    QListWidgetItem* pItem = profiles_tree_widget->currentItem();
    if (!pItem) {
        return;
    }

    if (mudlet::self()->storingPasswordsSecurely()) {
        writeSecurePassword(pItem->data(csmNameRole).toString(), pass);
    } else {
        writeProfileData(pItem->data(csmNameRole).toString(), qsl("password"), pass);
    }
}

void dlgConnectionProfiles::writeSecurePassword(const QString& profile, const QString& pass) const
{
    auto* job = new QKeychain::WritePasswordJob(qsl("Mudlet profile"));
    job->setAutoDelete(false);
    job->setInsecureFallback(false);

    job->setKey(profile);
    job->setTextData(pass);
    job->setProperty("profile", profile);

    connect(job, &QKeychain::WritePasswordJob::finished, this, &dlgConnectionProfiles::slot_passwordSaved);

    job->start();
}

void dlgConnectionProfiles::deleteSecurePassword(const QString& profile) const
{
    auto* job = new QKeychain::DeletePasswordJob(qsl("Mudlet profile"));
    job->setAutoDelete(false);
    job->setInsecureFallback(false);

    job->setKey(profile);
    job->setProperty("profile", profile);

    connect(job, &QKeychain::WritePasswordJob::finished, this, &dlgConnectionProfiles::slot_passwordDeleted);

    job->start();
}

void dlgConnectionProfiles::slot_updateLogin(const QString& login)
{
    QListWidgetItem* pItem = profiles_tree_widget->currentItem();
    if (pItem) {
        writeProfileData(pItem->data(csmNameRole).toString(), qsl("login"), login);
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
        QListWidgetItem* pItem = profiles_tree_widget->currentItem();
        if (!pItem) {
            return;
        }
        writeProfileData(pItem->data(csmNameRole).toString(), qsl("url"), host_name_entry->text());
    }
}

void dlgConnectionProfiles::slot_updateAutoConnect(int state)
{
    QListWidgetItem* pItem = profiles_tree_widget->currentItem();
    if (!pItem) {
        return;
    }
    writeProfileData(pItem->data(csmNameRole).toString(), qsl("autologin"), QString::number(state));
}

void dlgConnectionProfiles::slot_updateAutoReconnect(int state)
{
    QListWidgetItem* pItem = profiles_tree_widget->currentItem();
    if (!pItem) {
        return;
    }
    writeProfileData(pItem->data(csmNameRole).toString(), qsl("autoreconnect"), QString::number(state));
}

// This gets called when the QCheckBox that it is connect-ed to gets its
// checked state set programmatically AS WELL as when the user clicks on it:
void dlgConnectionProfiles::slot_updateDiscordOptIn(int state)
{
    QListWidgetItem* pItem = profiles_tree_widget->currentItem();
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
        QListWidgetItem* pItem = profiles_tree_widget->currentItem();
        if (!pItem) {
            return;
        }
        writeProfileData(pItem->data(csmNameRole).toString(), qsl("port"), port);
    }
}

void dlgConnectionProfiles::slot_updateSslTslPort(int state)
{
    if (validateProfile()) {
        QListWidgetItem* pItem = profiles_tree_widget->currentItem();
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
    QListWidgetItem* pItem = profiles_tree_widget->currentItem();
    const QString newProfileName = profile_name_entry->text().trimmed();
    const QString newProfileHost = host_name_entry->text().trimmed();
    const QString newProfilePort = port_entry->text().trimmed();
    const int newProfileSslTsl = port_ssl_tsl->isChecked() * 2;

    validateProfile();
    if (!validName || newProfileName.isEmpty() || !pItem) {
        return;
    }

    const QString currentProfileEditName = pItem->data(csmNameRole).toString();
    const int row = mProfileList.indexOf(currentProfileEditName);
    if ((row >= 0) && (row < mProfileList.size())) {
        mProfileList[row] = newProfileName;
    } else {
        mProfileList << newProfileName;
    }

    // don't do anything if this was just a normal click, and not an edit of any sort
    if (currentProfileEditName == newProfileName) {
        return;
    }

    if (mudlet::self()->storingPasswordsSecurely()) {
        migrateSecuredPassword(currentProfileEditName, newProfileName);
    }

    setItemName(pItem, newProfileName);

    const QDir currentPath(mudlet::getMudletPath(mudlet::profileHomePath, currentProfileEditName));
    const QDir dir;

    if (currentPath.exists()) {
        // CHECKME: previous code specified a path ending in a '/'
        QDir parentpath(mudlet::getMudletPath(mudlet::profilesPath));
        if (!parentpath.rename(currentProfileEditName, newProfileName)) {
            notificationArea->show();
            notificationAreaIconLabelWarning->show();
            notificationAreaIconLabelError->hide();
            notificationAreaIconLabelInformation->hide();
            notificationAreaMessageBox->show();
            notificationAreaMessageBox->setText(tr("Could not rename your profile data on the computer."));
        }
    } else if (!dir.mkpath(mudlet::getMudletPath(mudlet::profileHomePath, newProfileName))) {
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
        auto pRestoredItems = findData(*profiles_tree_widget, newProfileName, csmNameRole);
        Q_ASSERT_X(pRestoredItems.count() == 1, "dlgConnectionProfiles::slot_saveName", "Couldn't find exactly 1 restored profile to select");

        // As we are using QAbstractItemView::SingleSelection this will
        // automatically unselect the previous item:
        profiles_tree_widget->setCurrentItem(pRestoredItems.first());
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

    informationalArea->show();
    tabWidget_connectionInfo->show();

    const QString newname = tr("new profile name");

    auto pItem = new (std::nothrow) QListWidgetItem();
    if (!pItem) {
        return;
    }
    setItemName(pItem, newname);

    profiles_tree_widget->addItem(pItem);

    // insert newest entry on top of the list as the general sorting
    // is always newest item first -> fillout->form() filters
    // this is more practical for the user as they use the same profile most of the time

    // As we are using QAbstractItemView::SingleSelection this will
    // automatically unselect the previous item:
    profiles_tree_widget->setCurrentItem(pItem);

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
    const QString profile = profiles_tree_widget->currentItem()->data(csmNameRole).toString();
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
    const QString profile = profiles_tree_widget->currentItem()->data(csmNameRole).toString();
    reallyDeleteProfile(profile);
}

void dlgConnectionProfiles::reallyDeleteProfile(const QString& profile)
{
    QDir dir(mudlet::getMudletPath(mudlet::profileHomePath, profile));
    dir.removeRecursively();

    // record the deleted default profile so it does not get re-created in the future
    auto& settings = *mudlet::self()->mpSettings;
    auto deletedDefaultMuds = settings.value(qsl("deletedDefaultMuds"), QStringList()).toStringList();
    if (!deletedDefaultMuds.contains(profile)) {
        deletedDefaultMuds.append(profile);
    }
    settings.setValue(qsl("deletedDefaultMuds"), deletedDefaultMuds);

    fillout_form();
    profiles_tree_widget->setFocus();
}

// called when the 'delete' button is pressed, raises a dialog to confirm deletion
// if this profile has been used
void dlgConnectionProfiles::slot_deleteProfile()
{
    if (!profiles_tree_widget->currentItem()) {
        return;
    }

    const QString profile = profiles_tree_widget->currentItem()->data(csmNameRole).toString();
    const QStringList& onlyShownPredefinedProfiles{mudlet::self()->mOnlyShownPredefinedProfiles};
    if (!onlyShownPredefinedProfiles.isEmpty() && onlyShownPredefinedProfiles.contains(profile)) {
        // Do NOT allow deletion of the prioritised predefined MUD:
        return;
    }

    const QDir profileDirContents(mudlet::getMudletPath(mudlet::profileXmlFilesPath, profile));
    if (!profileDirContents.exists() || profileDirContents.isEmpty()) {
        // shortcut - don't show profile deletion confirmation if there is no data to delete
        reallyDeleteProfile(profile);
        return;
    }

    QUiLoader loader;

    QFile file(qsl(":/ui/delete_profile_confirmation.ui"));
    file.open(QFile::ReadOnly);

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
    QFile file(mudlet::getMudletPath(mudlet::profileDataItemPath, profile, item));
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
    QSaveFile file(mudlet::getMudletPath(mudlet::profileDataItemPath, profile, item));
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
    auto itDetails = TGameDetails::findGame(profile_name);
    if (itDetails != TGameDetails::scmDefaultGames.constEnd()) {
        if (!(*itDetails).description.isEmpty()) {
            return (*itDetails).description;
        }
    }

    // Else, if there isn't a predefined text, return whatever the user might
    // have stored:
    return readProfileData(profile_name, qsl("description"));
}

void dlgConnectionProfiles::slot_itemClicked(QListWidgetItem* pItem)
{
    if (!pItem) {
        return;
    }

    slot_togglePasswordVisibility(false);

    const QString profile_name = pItem->data(csmNameRole).toString();

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
        if (mudlet::self()->storingPasswordsSecurely()) {
            loadSecuredPassword(profile_name, [this, profile_name](const QString& password) {
                if (!password.isEmpty()) {
                    character_password_entry->setText(password);
                } else {
                    character_password_entry->setText(readProfileData(profile_name, qsl("password")));
                }
            });

        } else {
            character_password_entry->setText(readProfileData(profile_name, qsl("password")));
        }
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

    QDir dir(mudlet::getMudletPath(mudlet::profileXmlFilesPath, profile_name));
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
        } else {
            profile_history->addItem(entry, QVariant(entry)); // if it has a custom name, use it as it is
        }
    }

    profile_history->setEnabled(static_cast<bool>(profile_history->count()));

    const QString profileLoadedMessage = tr("This profile is currently loaded - close it before changing the connection parameters.");

    if (mudlet::self()->getHostManager().getHost(profile_name)) {
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
    profiles_tree_widget->clear();
    profile_name_entry->clear();
    host_name_entry->clear();
    port_entry->clear();

    mProfileList = QDir(mudlet::getMudletPath(mudlet::profilesPath)).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    if (mProfileList.isEmpty()) {
        welcome_message->show();
        tabWidget_connectionInfo->hide();
        informationalArea->hide();

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
        informationalArea->show();
    }

    profiles_tree_widget->setIconSize(QSize(120, 30));
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

            profiles_tree_widget->addItem(pItem);
            description = getDescription(qsl("mudlet.org"));
            if (!description.isEmpty()) {
                pItem->setToolTip(utils::richText(description));
            }
        }
#endif
    } else {
        pItem = new QListWidgetItem();
        for (const QString& onlyShownPredefinedProfile : onlyShownPredefinedProfiles) {
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

    for (int i = 0; i < profiles_tree_widget->count(); i++) {
        const auto profile = profiles_tree_widget->item(i);
        const auto profileName = profile->data(csmNameRole).toString();
        if (profileName == qsl("Mudlet self-test")) {
            test_profile_row = i;
        }
        const auto fileinfo = QFileInfo(mudlet::getMudletPath(mudlet::profileXmlFilesPath, profileName));
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
            if (profiles_tree_widget->count() > 1) {
                while (toselectRow == -1 || toselectRow == test_profile_row) {
                    toselectRow = QRandomGenerator::global()->bounded(profiles_tree_widget->count());
                }
            }
        } else if (predefined_profile_row >= 0) {
            // If the user is starting one of a MUD's "dedicated" Mudlet versions then
            // select the first of THAT/THOSE predefined one(s) on first launch:
            toselectRow = predefined_profile_row;
        }
    }

    if (toselectRow != -1) {
        profiles_tree_widget->setCurrentRow(toselectRow);
    }

    updateDiscordStatus();
}

void dlgConnectionProfiles::setProfileIcon() const
{
    const QStringList defaultGames = TGameDetails::keys();

    for (int i = 0; i < mProfileList.size(); i++) {
        const QString& profileName = mProfileList.at(i);
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
    return QFileInfo::exists(mudlet::getMudletPath(mudlet::profileDataItemPath, profileName, qsl("profileicon")));
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
    profiles_tree_widget->addItem(pItem);
}

void dlgConnectionProfiles::setCustomIcon(const QString& profileName, QListWidgetItem* profile) const
{
    auto profileIconPath = mudlet::getMudletPath(mudlet::profileDataItemPath, profileName, qsl("profileicon"));
    auto icon = QIcon(QPixmap(profileIconPath).scaled(QSize(120, 30), Qt::IgnoreAspectRatio, Qt::SmoothTransformation).copy());
    profile->setIcon(icon);
}

// When a profile is renamed, migrate password storage to the new profile
void dlgConnectionProfiles::migrateSecuredPassword(const QString& oldProfile, const QString& newProfile)
{
    const auto& password = character_password_entry->text().trimmed();

    deleteSecurePassword(oldProfile);
    writeSecurePassword(newProfile, password);
}

template <typename L>
void dlgConnectionProfiles::loadSecuredPassword(const QString& profile, L callback)
{
    // character_password_entry

    auto* job = new QKeychain::ReadPasswordJob(qsl("Mudlet profile"));
    job->setAutoDelete(false);
    job->setInsecureFallback(false);

    job->setKey(profile);

    connect(job, &QKeychain::ReadPasswordJob::finished, this, [=](QKeychain::Job* task) {
        if (task->error()) {
            const auto error = task->errorString();
            if (error != qsl("Entry not found") && error != qsl("No match")) {
                qDebug().nospace().noquote() << "dlgConnectionProfiles::loadSecuredPassword() ERROR - could not retrieve secure password for \"" << profile << "\", error is: " << error << ".";
            }

        }

        auto readJob = static_cast<QKeychain::ReadPasswordJob*>(task);
        callback(readJob->textData());

        task->deleteLater();
    });

    job->start();
}

std::optional<QColor> getCustomColor(const QString& profileName)
{
    auto profileColorPath = mudlet::getMudletPath(mudlet::profileDataItemPath, profileName, qsl("profilecolor"));
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
    profiles_tree_widget->addItem(pItem);
}

void dlgConnectionProfiles::slot_profileContextMenu(QPoint pos)
{
    const QPoint globalPos = profiles_tree_widget->mapToGlobal(pos);
    auto profileName = profiles_tree_widget->currentItem()->data(csmNameRole).toString();

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
    auto profileName = profiles_tree_widget->currentItem()->data(csmNameRole).toString();

    const QString imageLocation = QFileDialog::getOpenFileName(
            this, tr("Select custom image for profile (should be 120x30)"), QStandardPaths::writableLocation(QStandardPaths::HomeLocation), tr("Images (%1)").arg(qsl("*.png *.gif *.jpg")));
    if (imageLocation.isEmpty()) {
        return;
    }

    const bool success = mudlet::self()->setProfileIcon(profileName, imageLocation).first;
    if (!success) {
        return;
    }

    auto icon = QIcon(QPixmap(imageLocation).scaled(QSize(120, 30), Qt::IgnoreAspectRatio, Qt::SmoothTransformation).copy());
    profiles_tree_widget->currentItem()->setIcon(icon);
}
void dlgConnectionProfiles::slot_setCustomColor()
{
    auto profileName = profiles_tree_widget->currentItem()->data(csmNameRole).toString();
    QColor color = QColorDialog::getColor(getCustomColor(profileName).value_or(QColor(255, 255, 255)));
    if (color.isValid()) {
        auto profileColorPath = mudlet::getMudletPath(mudlet::profileDataItemPath, profileName, qsl("profilecolor"));
        QSaveFile file(profileColorPath);
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        auto colorName = color.name();
        file.write(colorName.toUtf8(), colorName.length());
        if (!file.commit()) {
            qDebug() << "dlgConnectionProfiles::slot_setCustomColor: error saving custom icon color: " << file.errorString();
        }
        profiles_tree_widget->currentItem()->setIcon(customIcon(profileName, {color}));
    }
}
void dlgConnectionProfiles::slot_resetCustomIcon()
{
    auto profileName = profiles_tree_widget->currentItem()->data(csmNameRole).toString();

    const bool success = mudlet::self()->resetProfileIcon(profileName).first;
    if (!success) {
        return;
    }

    auto currentRow = profiles_tree_widget->currentRow();
    fillout_form();
    profiles_tree_widget->setCurrentRow(currentRow);
}

void dlgConnectionProfiles::slot_passwordSaved(QKeychain::Job* job)
{
    if (job->error()) {
        qWarning().nospace().noquote() << "dlgslot_passwordSaved:slot_passwordSaved(...) ERROR - could not save password for \"" << job->property("profile").toString() << "\"; error was: \"" << job->errorString() << "\".";
    }

    job->deleteLater();
}

void dlgConnectionProfiles::slot_passwordDeleted(QKeychain::Job* job)
{
    if (job->error()) {
        qWarning() << "dlgConnectionProfiles::slot_passwordDeleted(...) ERROR - could not delete password for: \"" << job->property("profile").toString() << "\"; error was: \"" << job->errorString() << "\".";
    }

    job->deleteLater();
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
    const QDir dir(mudlet::getMudletPath(mudlet::profileHomePath, oldname));
    if (!dir.exists()) {
        mCopyingProfile = false;
        return;
    }

    QApplication::setOverrideCursor(Qt::BusyCursor);
    mpCopyProfile->setText(tr("Copying..."));
    mpCopyProfile->setEnabled(false);
    auto future = QtConcurrent::run(dlgConnectionProfiles::copyFolder, mudlet::getMudletPath(mudlet::profileHomePath, oldname), mudlet::getMudletPath(mudlet::profileHomePath, profile_name));
    auto watcher = new QFutureWatcher<bool>;
    connect(watcher, &QFutureWatcher<bool>::finished, this, [=]() {
        mProfileList << profile_name;
        slot_itemClicked(pItem);
        // Clear the Discord optin on the copied profile - just because the source
        // one may have had it enabled does not mean we can assume the new one would
        // want it set:
        discord_optin_checkBox->setChecked(false);

        // restore the password, which won't be copied by the disk copy if stored in the credential manager
        character_password_entry->setText(oldPassword);
        if (mudlet::self()->storingPasswordsSecurely()) {
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

    const QDir newProfileDir(mudlet::getMudletPath(mudlet::profileHomePath, profile_name));
    newProfileDir.mkpath(newProfileDir.path());
    if (!newProfileDir.exists()) {
        return;
    }

    // copy relevant profile files
    for (const QString file : {"url", "port", "password", "login", "description"}) {
        auto filePath = qsl("%1/%2").arg(mudlet::getMudletPath(mudlet::profileHomePath, oldname), file);
        auto newFilePath = qsl("%1/%2").arg(mudlet::getMudletPath(mudlet::profileHomePath, profile_name), file);
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
    profiles_tree_widget->addItem(pItem);
    pItem->setIcon(customIcon(profile_name, std::nullopt));
    profiles_tree_widget->setCurrentItem(pItem);

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
    const QDir oldProfiledir(mudlet::getMudletPath(mudlet::profileXmlFilesPath, oldname));
    const QDir newProfiledir(mudlet::getMudletPath(mudlet::profileXmlFilesPath, newname));
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

void dlgConnectionProfiles::slot_load()
{
    setVisible(false);
    // This is needed to make the above take effect as fast as possible:
    qApp->processEvents();
    loadProfile(false);
    QDialog::accept();
}

void dlgConnectionProfiles::loadProfile(bool alsoConnect)
{
    const QString profile_name = profile_name_entry->text().trimmed();

    if (profile_name.isEmpty()) {
        return;
    }

    Host *pHost = mudlet::self()->loadProfile(profile_name, alsoConnect);

    // overwrite the generic profile with user supplied name, url and login information
    if (pHost) {
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

    QListWidgetItem* pItem = profiles_tree_widget->currentItem();

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
    for (int i = 0; i < files.count(); i++) {
        const QString srcName = sourceFolder + QDir::separator() + files[i];
        const QString destName = destFolder + QDir::separator() + files[i];
        QFile::copy(srcName, destName);
    }
    files.clear();
    files = sourceDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    for (int i = 0; i < files.count(); i++) {
        const QString srcName = sourceFolder + QDir::separator() + files[i];
        const QString destName = destFolder + QDir::separator() + files[i];
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
    for (int index = 0, total = profiles_tree_widget->count(); index < total; ++index) {
        if (profiles_tree_widget->item(index)->data(csmNameRole).toString().startsWith(what, Qt::CaseInsensitive)) {
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
    pItem = new QListWidgetItem();
    setItemName(pItem, mudServer);

    profiles_tree_widget->addItem(pItem);
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
    for (int i = 0, total = profiles_tree_widget->count(); i < total; ++i) {
        profiles_tree_widget->item(i)->setFlags(profiles_tree_widget->item(i)->flags() | Qt::ItemIsEnabled);
    }
}

bool dlgConnectionProfiles::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == profiles_tree_widget && event->type() == QEvent::KeyPress) {
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

    for (int i = 0, total = profiles_tree_widget->count(); i < total; ++i) {
        auto flags = profiles_tree_widget->item(i)->flags();
        if (indexes.isEmpty() || !indexes.contains(i)) {
            flags &= ~Qt::ItemIsEnabled;
        } else {
            flags |= Qt::ItemIsEnabled;
        }
        profiles_tree_widget->item(i)->setFlags(flags);
    }

    profiles_tree_widget->setCurrentRow(indexes.first());
}
