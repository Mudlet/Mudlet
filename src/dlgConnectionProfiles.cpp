/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016-2018, 2020 by Stephen Lyons                        *
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
#include "XMLimport.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QtUiTools>
#include <QSettings>
#include <sstream>
#include "post_guard.h"

dlgConnectionProfiles::dlgConnectionProfiles(QWidget * parent)
: QDialog(parent)
, validName()
, validUrl()
, validPort()
, mProfileList(QStringList())
, offline_button(nullptr)
, connect_button(nullptr)
, delete_profile_lineedit(nullptr)
, delete_button(nullptr)
, mDefaultGames({"3Kingdoms", "3Scapes", "Aardwolf", "Achaea", "Aetolia",
                 "Avalon.de", "BatMUD", "Clessidra", "Fierymud", "Imperian", "Luminari",
                 "Lusternia", "Materia Magica", "Midnight Sun 2", "Realms of Despair",
                 "Reinos de Leyenda", "StickMUD", "WoTMUD", "ZombieMUD", "Carrion Fields"})
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

    holdPixmap = *(this->notificationAreaIconLabelWarning->pixmap());
    holdPixmap.setDevicePixelRatio(5.3);
    this->notificationAreaIconLabelWarning->setPixmap(holdPixmap);

    holdPixmap = *(this->notificationAreaIconLabelError->pixmap());
    holdPixmap.setDevicePixelRatio(5.3);
    this->notificationAreaIconLabelError->setPixmap(holdPixmap);

    holdPixmap = *(this->notificationAreaIconLabelInformation->pixmap());
    holdPixmap.setDevicePixelRatio(5.3);
    this->notificationAreaIconLabelInformation->setPixmap(holdPixmap);

    // selection mode is important. if this is not set the selection behaviour is
    // undefined. this is an undocumented qt bug, as it only shows on certain OS
    // and certain architectures.

    profiles_tree_widget->setSelectionMode(QAbstractItemView::SingleSelection);
    profiles_tree_widget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(profiles_tree_widget, &QWidget::customContextMenuRequested, this, &dlgConnectionProfiles::slot_profile_menu);

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

    auto copyProfile = new QAction(tr("Copy"), this);
    copyProfile->setObjectName(QStringLiteral("copyProfile"));
    auto copyProfileSettings = new QAction(tr("Copy settings only"), this);
    copyProfileSettings->setObjectName(QStringLiteral("copyProfileSettingsOnly"));

    copy_profile_toolbutton->addAction(copyProfile);
    copy_profile_toolbutton->addAction(copyProfileSettings);
    copy_profile_toolbutton->setDefaultAction(copyProfile);
    auto widgetList = copyProfile->associatedWidgets();
    Q_ASSERT_X(widgetList.count(), "dlgConnectionProfiles::dlgConnectionProfiles(...)", "A QWidget for copyProfile QAction not found.");
    widgetList.first()->setAccessibleName(tr("copy profile"));
    widgetList.first()->setAccessibleDescription(tr("copy the entire profile to new one that will require a different new name."));

    widgetList = copyProfileSettings->associatedWidgets();
    Q_ASSERT_X(widgetList.count(), "dlgConnectionProfiles::dlgConnectionProfiles(...)", "A QWidget for copyProfileSettings QAction not found.");
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
        pWelcome_document->setHtml(QStringLiteral("<html><head/><body>%1</body></html>")
                                   .arg(Welcome_text_template.arg(QStringLiteral("NEW_PROFILE_ICON"),
                                                                  QStringLiteral("CONNECT_PROFILE_ICON"))));

        // As we are repurposing the cancel to be a close button we do want to
        // change it anyhow:
        abort->setIcon(QIcon::fromTheme(QStringLiteral("dialog-close"), QIcon(QStringLiteral(":/icons/dialog-close.png"))));

        QIcon icon_new(QIcon::fromTheme(QStringLiteral("document-new"), QIcon(QStringLiteral(":/icons/document-new.png"))));
        QIcon icon_connect(QIcon::fromTheme(QStringLiteral("dialog-ok-apply"), QIcon(QStringLiteral(":/icons/preferences-web-browser-cache.png"))));

        offline_button->setIcon(QIcon(QStringLiteral(":/icons/mudlet_editor.png")));
        connect_button->setIcon(icon_connect);
        new_profile_button->setIcon(icon_new);
        remove_profile_button->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete"), QIcon(QStringLiteral(":/icons/edit-delete.png"))));

        copy_profile_toolbutton->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/edit-copy.png"))));
        copy_profile_toolbutton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        copyProfile->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/edit-copy.png"))));

        QTextCursor cursor = pWelcome_document->find(QStringLiteral("NEW_PROFILE_ICON"), 0, QTextDocument::FindWholeWords);
        // The indicated piece of marker text should be selected by the cursor
        Q_ASSERT_X(!cursor.isNull(), "dlgConnectionProfiles::dlgConnectionProfiles(...)",
                   "NEW_PROFILE_ICON text marker not found in welcome_message text for when icons are shown on dialogue buttons");
        // Remove the marker:
        cursor.removeSelectedText();
        // Insert the current icon image into the same place:
        QImage image_new(QPixmap(icon_new.pixmap(new_profile_button->iconSize())).toImage());
        cursor.insertImage(image_new);
        cursor.clearSelection();

        cursor = pWelcome_document->find(QStringLiteral("CONNECT_PROFILE_ICON"), 0, QTextDocument::FindWholeWords);
        Q_ASSERT_X(!cursor.isNull(), "dlgConnectionProfiles::dlgConnectionProfiles(...)",
                   "CONNECT_PROFILE_ICON text marker not found in welcome_message text for when icons are shown on dialogue buttons");
        cursor.removeSelectedText();
        QImage image_connect(QPixmap(icon_connect.pixmap(connect_button->iconSize())).toImage());
        cursor.insertImage(image_connect);
        cursor.clearSelection();
    } else {
        pWelcome_document->setHtml(QStringLiteral("<html><head/><body>%1</body></html>")
                                   .arg(Welcome_text_template.arg(QString(), QString())));
    }

    welcome_message->setDocument(pWelcome_document);

    mpAction_revealPassword = new QAction(this);
    mpAction_revealPassword->setCheckable(true);
    mpAction_revealPassword->setObjectName(QStringLiteral("mpAction_revealPassword"));
    slot_togglePasswordVisibility(false);

    character_password_entry->addAction(mpAction_revealPassword, QLineEdit::TrailingPosition);
    if (mudlet::self()->storingPasswordsSecurely()) {
        character_password_entry->setToolTip(tr("Characters password, stored securely in the computer's credential manager"));
    } else {
        character_password_entry->setToolTip(tr("Characters password. Note that the password isn't encrypted in storage"));
    }

    connect(mpAction_revealPassword, &QAction::triggered, this, &dlgConnectionProfiles::slot_togglePasswordVisibility);
    connect(offline_button, &QAbstractButton::clicked, this, &dlgConnectionProfiles::slot_load);
    connect(connect_button, &QAbstractButton::clicked, this, &dlgConnectionProfiles::accept);
    connect(abort, &QAbstractButton::clicked, this, &dlgConnectionProfiles::slot_cancel);
    connect(new_profile_button, &QAbstractButton::clicked, this, &dlgConnectionProfiles::slot_addProfile);
    connect(copyProfile, &QAction::triggered, this, &dlgConnectionProfiles::slot_copy_profile);
    connect(copyProfileSettings, &QAction::triggered, this, &dlgConnectionProfiles::slot_copy_profilesettings_only);
    connect(remove_profile_button, &QAbstractButton::clicked, this, &dlgConnectionProfiles::slot_deleteProfile);
    connect(profile_name_entry, &QLineEdit::textEdited, this, &dlgConnectionProfiles::slot_update_name);
    connect(profile_name_entry, &QLineEdit::editingFinished, this, &dlgConnectionProfiles::slot_save_name);
    connect(host_name_entry, &QLineEdit::textChanged, this, &dlgConnectionProfiles::slot_update_url);
    connect(port_entry, &QLineEdit::textChanged, this, &dlgConnectionProfiles::slot_update_port);
    connect(port_ssl_tsl, &QCheckBox::stateChanged, this, &dlgConnectionProfiles::slot_update_SSL_TSL_port);
    connect(autologin_checkBox, &QCheckBox::stateChanged, this, &dlgConnectionProfiles::slot_update_autologin);
    connect(auto_reconnect, &QCheckBox::stateChanged, this, &dlgConnectionProfiles::slot_update_autoreconnect);
    connect(login_entry, &QLineEdit::textEdited, this, &dlgConnectionProfiles::slot_update_login);
    connect(character_password_entry, &QLineEdit::textEdited, this, &dlgConnectionProfiles::slot_update_pass);
    connect(mud_description_textedit, &QPlainTextEdit::textChanged, this, &dlgConnectionProfiles::slot_update_description);
    connect(profiles_tree_widget, &QListWidget::currentItemChanged, this, &dlgConnectionProfiles::slot_item_clicked);
    connect(profiles_tree_widget, &QListWidget::itemDoubleClicked, this, &dlgConnectionProfiles::accept);

    connect(discord_optin_checkBox, &QCheckBox::stateChanged, this, &dlgConnectionProfiles::slot_update_discord_optin);

    // website_entry atm is only a label
    //connect(website_entry, SIGNAL(textEdited(const QString)), this, SLOT(slot_update_website(const QString)));

    notificationArea->hide();
    notificationAreaIconLabelWarning->hide();
    notificationAreaIconLabelError->hide();
    notificationAreaIconLabelInformation->hide();
    notificationAreaMessageBox->hide();

#if !defined(QT_NO_SSL)
    if (QSslSocket::supportsSsl()) {
        port_ssl_tsl->setEnabled(true);
    } else {
#endif
        port_ssl_tsl->setEnabled(false);
#if !defined(QT_NO_SSL)
    }
#endif

    mRegularPalette.setColor(QPalette::Text, QColor(0, 0, 192));
    mRegularPalette.setColor(QPalette::Highlight, QColor(0, 0, 192));
    mRegularPalette.setColor(QPalette::HighlightedText, QColor(Qt::white));
    mRegularPalette.setColor(QPalette::Base, QColor(Qt::white));

    mReadOnlyPalette.setColor(QPalette::Base, QColor(212, 212, 212));
    mReadOnlyPalette.setColor(QPalette::Text, QColor(0, 0, 192));
    mReadOnlyPalette.setColor(QPalette::Highlight, QColor(0, 0, 192));
    mReadOnlyPalette.setColor(QPalette::HighlightedText, QColor(Qt::white));

    mOKPalette.setColor(QPalette::Text, QColor(0, 0, 192));
    mOKPalette.setColor(QPalette::Highlight, QColor(0, 0, 192));
    mOKPalette.setColor(QPalette::HighlightedText, QColor(Qt::white));
    mOKPalette.setColor(QPalette::Base, QColor(235, 255, 235));

    mErrorPalette.setColor(QPalette::Text, QColor(0, 0, 192));
    mErrorPalette.setColor(QPalette::Highlight, QColor(0, 0, 192));
    mErrorPalette.setColor(QPalette::HighlightedText, QColor(Qt::white));
    mErrorPalette.setColor(QPalette::Base, QColor(255, 235, 235));

    profiles_tree_widget->setViewMode(QListView::IconMode);

    btn_load_enabled_accessDesc = tr("Click to load but not connect the selected profile.");
    btn_connect_enabled_accessDesc = tr("Click to load and connect the selected profile.");
    btn_connOrLoad_disabled_accessDesc = tr("Need to have a valid profile name, game server address and port before this button can be enabled.");
    item_profile_accessName = tr("Game name: %1");
    item_profile_accessDesc = tr("Button to select a mud game to play, double-click it to connect and start playing it.",
                                 // Intentional comment to separate arguments
                                 "Some text to speech engines will spell out initials like MUD so stick to lower case if that is a better option");
}

// the dialog can be accepted by pressing Enter on an qlineedit; this is a safeguard against it
// accepting invalid data
void dlgConnectionProfiles::accept()
{
    if (validName && validUrl && validPort) {
        slot_connectToServer();
        QDialog::accept();
    }
}

void dlgConnectionProfiles::slot_update_description()
{
    QListWidgetItem* pItem = profiles_tree_widget->currentItem();

    if (pItem) {
        QString description = mud_description_textedit->toPlainText();
        writeProfileData(pItem->data(csmNameRole).toString(), QStringLiteral("description"), description);

        // don't display custom profile descriptions as a tooltip, as passwords could be stored in there
    }
}

void dlgConnectionProfiles::slot_update_website(const QString &url)
{
    QListWidgetItem* pItem = profiles_tree_widget->currentItem();
    if (pItem) {
        writeProfileData(pItem->data(csmNameRole).toString(), QStringLiteral("website"), url);
    }
}

void dlgConnectionProfiles::slot_update_pass(const QString &pass)
{
    QListWidgetItem* pItem = profiles_tree_widget->currentItem();
    if (!pItem) {
        return;
    }

    if (mudlet::self()->storingPasswordsSecurely()) {
        writeSecurePassword(pItem->data(csmNameRole).toString(), pass);
    } else {
        writeProfileData(pItem->data(csmNameRole).toString(), QStringLiteral("password"), pass);
    }
}

void dlgConnectionProfiles::writeSecurePassword(const QString& profile, const QString& pass) const
{
    auto *job = new QKeychain::WritePasswordJob(QStringLiteral("Mudlet profile"));
    job->setAutoDelete(false);
    job->setInsecureFallback(false);

    job->setKey(profile);
    job->setTextData(pass);
    job->setProperty("profile", profile);

    connect(job, &QKeychain::WritePasswordJob::finished, this, &dlgConnectionProfiles::slot_password_saved);

    job->start();
}

void dlgConnectionProfiles::deleteSecurePassword(const QString& profile) const
{
    auto *job = new QKeychain::DeletePasswordJob(QStringLiteral("Mudlet profile"));
    job->setAutoDelete(false);
    job->setInsecureFallback(false);

    job->setKey(profile);
    job->setProperty("profile", profile);

    connect(job, &QKeychain::WritePasswordJob::finished, this, &dlgConnectionProfiles::slot_password_deleted);

    job->start();
}

void dlgConnectionProfiles::slot_update_login(const QString &login)
{
    QListWidgetItem* pItem = profiles_tree_widget->currentItem();
    if (pItem) {
        writeProfileData(pItem->data(csmNameRole).toString(), QStringLiteral("login"), login);
    }
}

void dlgConnectionProfiles::slot_update_url(const QString& url)
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
        writeProfileData(pItem->data(csmNameRole).toString(), QStringLiteral("url"), host_name_entry->text());
    }
}

void dlgConnectionProfiles::slot_update_autologin(int state)
{
    QListWidgetItem* pItem = profiles_tree_widget->currentItem();
    if (!pItem) {
        return;
    }
    writeProfileData(pItem->data(csmNameRole).toString(), QStringLiteral("autologin"), QString::number(state));
}

void dlgConnectionProfiles::slot_update_autoreconnect(int state)
{
    QListWidgetItem* pItem = profiles_tree_widget->currentItem();
    if (!pItem) {
        return;
    }
    writeProfileData(pItem->data(csmNameRole).toString(), QStringLiteral("autoreconnect"), QString::number(state));
}

// This gets called when the QCheckBox that it is connect-ed to gets it's
// checked state set programatically AS WELL as when the user clicks on it:
void dlgConnectionProfiles::slot_update_discord_optin(int state)
{
    QListWidgetItem* pItem = profiles_tree_widget->currentItem();
    if (!pItem) {
        return;
    }
    writeProfileData(pItem->data(csmNameRole).toString(), QStringLiteral("discordserveroptin"), QString::number(state));

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

void dlgConnectionProfiles::slot_update_port(const QString& ignoreBlank)
{
    QString port = port_entry->text().trimmed();

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
        writeProfileData(pItem->data(csmNameRole).toString(), QStringLiteral("port"), port);
    }
}

void dlgConnectionProfiles::slot_update_SSL_TSL_port(int state)
{
    if (validateProfile()) {
        QListWidgetItem* pItem = profiles_tree_widget->currentItem();
        if (!pItem) {
            return;
        }
        writeProfileData(pItem->data(csmNameRole).toString(), QStringLiteral("ssl_tsl"), QString::number(state));
    }
}

void dlgConnectionProfiles::slot_update_name(const QString& newName)
{
    Q_UNUSED(newName)
    validateProfile();
}

void dlgConnectionProfiles::slot_save_name()
{
    QListWidgetItem* pItem = profiles_tree_widget->currentItem();
    QString newProfileName = profile_name_entry->text().trimmed();

    if (!validName || newProfileName.isEmpty() || !pItem) {
        return;
    }

    QString currentProfileEditName = pItem->data(csmNameRole).toString();
    int row = mProfileList.indexOf(currentProfileEditName);
    if ((row >= 0) && (row < mProfileList.size())) {
        mProfileList[row] = newProfileName;
    } else {
        mProfileList << newProfileName;
    }

    // don't do anything if this was just a normal click, and not an edit of any sort
    if (currentProfileEditName == newProfileName) {
        return;
    }

    migrateSecuredPassword(currentProfileEditName, newProfileName);

    pItem->setData(csmNameRole, newProfileName);

    QDir currentPath(mudlet::getMudletPath(mudlet::profileHomePath, currentProfileEditName));
    QDir dir;

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

    // if this was a previously deleted profile, restore it
    auto& settings = *mudlet::self()->mpSettings;
    auto deletedDefaultMuds = settings.value(QStringLiteral("deletedDefaultMuds"), QStringList()).toStringList();
    if (deletedDefaultMuds.contains(newProfileName)) {
        deletedDefaultMuds.removeOne(newProfileName);
        settings.setValue(QStringLiteral("deletedDefaultMuds"), deletedDefaultMuds);
        // run fillout_form to re-create the default profile icon and description
        fillout_form();
        // and re-select the profile since focus is lost
        auto pRestoredItems = findData(*profiles_tree_widget, newProfileName, csmNameRole);
        Q_ASSERT_X(pRestoredItems.count() < 1, "dlgConnectionProfiles::slot_save_name", "no previously deleted Mud found with matching name when trying to restore one");
        Q_ASSERT_X(pRestoredItems.count() > 1, "dlgConnectionProfiles::slot_save_name", "multiple deleted Muds found with matching name when trying to restore one");

        profiles_tree_widget->setCurrentItem(pRestoredItems.first());
        slot_item_clicked(pRestoredItems.first());
    } else {
        // code stolen from fillout_form, should be moved to its own function
        QFont font(QStringLiteral("Bitstream Vera Sans Mono"), 1, QFont::Normal);
        // Some uses of QFont have a third argument such as QFont::Helvetica or
        // QFont::Courier but that is not a valid value for that argument - it
        // is a font weight and typically only QFont::Normal or QFont::Bold is
        // correct there (or a number 0 to 99, the two given are 50 and 75
        // respectively)

        QString sList = newProfileName;
        QString s = newProfileName;
        pItem->setFont(font);
        pItem->setForeground(QColor(Qt::white));
        profiles_tree_widget->addItem(pItem);
        QPixmap pb(120, 30);
        pb.fill(Qt::transparent);
        uint hash = qHash(sList);
        QLinearGradient shade(0, 0, 120, 30);
        int i = row;
        quint8 i1 = hash % 255;
        quint8 i2 = (hash + i) % 255;
        quint8 i3 = (i * hash) % 255;
        quint8 i4 = (3 * hash) % 255;
        quint8 i5 = (hash) % 255;
        quint8 i6 = (hash / (i + 2)) % 255; // Under some corner cases i might be -1 or 0
        shade.setColorAt(1, QColor(i1, i2, i3, 255));
        shade.setColorAt(0, QColor(i4, i5, i6, 255));

        QPainter pt(&pb);
        pt.setCompositionMode(QPainter::CompositionMode_SourceOver);
        pt.fillRect(QRect(0, 0, 120, 30), shade);
        QPixmap pg(QStringLiteral(":/icons/mudlet_main_32px.png"));
        pt.drawPixmap(QRect(5, 5, 20, 20), pg);

        QFont _font;
        QImage _pm(90, 30, QImage::Format_ARGB32_Premultiplied);
        QPainter _pt(&_pm);
        _pt.setCompositionMode(QPainter::CompositionMode_SourceOver);
        int fs = 30;
        for (; fs > 1; fs--) {
            _pt.eraseRect(QRect(0, 0, 90, 30));
            _pt.fillRect(QRect(0, 0, 90, 30), QColor(255, 0, 0, 10));
            _font = QFont(QStringLiteral("DejaVu Sans"), fs, QFont::Normal);
            _pt.setFont(_font);
            QRect _r;
            if ((i1 + i2 + i3 + i4 + i5 + i6) / 6 < 100) {
                _pt.setPen(QColor(Qt::white));
            } else {
                _pt.setPen(QColor(Qt::black));
            }
            _pt.drawText(QRect(0, 0, 90, 30), Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextWordWrap, s, &_r);
            /*
             * if (QFontMetrics(_font).boundingRect(s).width() <= 80
             *     && QFontMetrics(_font).boundingRect( s ).height() <= 30)
             */
            if (_r.width() <= 90 && _r.height() <= 30) {
                break;
            }
        }
        pt.setFont(_font);
        QRect _r;
        if ((i1 + i2 + i3 + i4 + i5 + i6) / 6 < 100) {
            pt.setPen(QColor(Qt::white));
        } else {
            pt.setPen(QColor(Qt::black));
        }
        pt.drawText(QRect(30, 0, 90, 30), Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextWordWrap, s, &_r);
        QIcon mi = QIcon(pb);
        pItem->setIcon(mi);
    }
}

void dlgConnectionProfiles::slot_addProfile()
{
    profile_name_entry->setReadOnly(false);
    fillout_form();
    welcome_message->hide();

    requiredArea->show();
    informationalArea->show();
    optionalArea->show();

    QString newname = tr("new profile name");

    auto pItem = new QListWidgetItem();
    if (!pItem) {
        return;
    }
    pItem->setData(csmNameRole, newname);

    profiles_tree_widget->setSelectionMode(QAbstractItemView::SingleSelection);
    profiles_tree_widget->addItem(pItem);

    // insert newest entry on top of the list as the general sorting
    // is always newest item first -> fillout->form() filters
    // this is more practical for the user as they use the same profile most of the time

    profiles_tree_widget->setItemSelected(profiles_tree_widget->currentItem(), false); // Unselect previous item
    profiles_tree_widget->setCurrentItem(pItem);
    profiles_tree_widget->setItemSelected(pItem, true);

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
void dlgConnectionProfiles::slot_deleteprofile_check(const QString& text)
{
    QString profile = profiles_tree_widget->currentItem()->data(csmNameRole).toString();
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
    QString profile = profiles_tree_widget->currentItem()->data(csmNameRole).toString();
    QDir dir(mudlet::getMudletPath(mudlet::profileHomePath, profile));
    dir.removeRecursively();

    // record the deleted default profile so it does not get re-created in the future
    auto& settings = *mudlet::self()->mpSettings;
    auto deletedDefaultMuds = settings.value(QStringLiteral("deletedDefaultMuds"), QStringList()).toStringList();
    if (!deletedDefaultMuds.contains(profile)) {
        deletedDefaultMuds.append(profile);
    }
    settings.setValue(QStringLiteral("deletedDefaultMuds"), deletedDefaultMuds);

    fillout_form();
    profiles_tree_widget->setFocus();
}

// called when the 'delete' button is pressed, raises a dialog to confirm deletion
void dlgConnectionProfiles::slot_deleteProfile()
{
    if (!profiles_tree_widget->currentItem()) {
        return;
    }

    QString profile = profiles_tree_widget->currentItem()->data(csmNameRole).toString();

    QUiLoader loader;

    QFile file(QStringLiteral(":/ui/delete_profile_confirmation.ui"));
    file.open(QFile::ReadOnly);

    auto * delete_profile_dialog = dynamic_cast<QDialog*>(loader.load(&file, this));
    file.close();

    if (!delete_profile_dialog) {
        return;
    }

    delete_profile_lineedit = delete_profile_dialog->findChild<QLineEdit*>(QStringLiteral("delete_profile_lineedit"));
    delete_button = delete_profile_dialog->findChild<QPushButton*>(QStringLiteral("delete_button"));
    auto * cancel_button = delete_profile_dialog->findChild<QPushButton*>(QStringLiteral("cancel_button"));

    if (!delete_profile_lineedit || !delete_button || !cancel_button) {
        return;
    }

    connect(delete_profile_lineedit, &QLineEdit::textChanged, this, &dlgConnectionProfiles::slot_deleteprofile_check);
    connect(delete_profile_dialog, &QDialog::accepted, this, &dlgConnectionProfiles::slot_reallyDeleteProfile);

    delete_profile_lineedit->setPlaceholderText(profile);
    delete_profile_lineedit->setFocus();
    delete_button->setEnabled(false);
    delete_profile_dialog->setWindowTitle(tr("Deleting '%1'").arg(profile));

    delete_profile_dialog->show();
    delete_profile_dialog->raise();
}

QString dlgConnectionProfiles::readProfileData(const QString& profile, const QString& item) const
{
    QFile file(mudlet::getMudletPath(mudlet::profileDataItemPath, profile, item));
    bool success = file.open(QIODevice::ReadOnly);
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
    auto f = mudlet::getMudletPath(mudlet::profileDataItemPath, profile, item);
    QFile file(f);
    if (file.open(QIODevice::WriteOnly | QIODevice::Unbuffered)) {
        QDataStream ofs(&file);
        if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
            ofs.setVersion(mudlet::scmQDataStreamFormat_5_12);
        }
        ofs << what;
        file.close();
    }

    if (file.error() == QFile::NoError) {
        return qMakePair(true, QString());
    } else {
        return qMakePair(false, file.errorString());
    }
}

// Use the URL so we can use the same descriptions for user generated copies of
// predefined MUDs - but also need the port number to disambiguate the 3K ones!
QString dlgConnectionProfiles::getDescription(const QString& hostUrl, const quint16 port, const QString& profile_name) const
{
    if (hostUrl == QLatin1String("realmsofdespair.com")) {
        return QLatin1String(
                "The Realms of Despair is the original SMAUG MUD and is FREE to play. We have an active Roleplaying community, an active player-killing (deadly) community, and a very active "
                "peaceful community. Players can choose from 13 classes (including a deadly-only class) and 13 races. Character appearances are customizable on creation and we have a vast "
                "collection of equipment that is level, gender, class, race and alignment specific. We boast well over 150 original, exclusive areas, with a total of over 20,000 rooms. Mob killing, "
                "or 'running' is one of our most popular activities, with monster difficulties varying from easy one-player kills to difficult group kills. We have four deadly-only Clans, twelve "
                "peaceful-only Guilds, eight Orders, and fourteen Role-playing Nations that players can join to interact more closely with other players. We have two mortal councils that actively "
                "work toward helping players: The Symposium hears ideas for changes, and the Newbie Council assists new players. Our team of Immortals are always willing to answer questions and to "
                "help out however necessary. Best of all, playing the Realms of Despair is totally FREE!");
    } else if (hostUrl == QLatin1String("zombiemud.org")) {
        return QLatin1String(
                "Since 1994, ZombieMUD has been on-line and bringing orc-butchering fun to the masses from our home base in Oulu, Finland. We're a pretty friendly bunch, with players logging in "
                "from all over the globe to test their skill in our medieval role-playing environment. With 15 separate guilds and 41 races to choose from, as a player the only limitation to your "
                "achievements on the game is your own imagination and will to succeed.");
    } else if (hostUrl == QLatin1String("carrionfields.net")) {
        return QLatin1String(
                "Carrion Fields is a unique blend of high-caliber roleplay and complex, hardcore player-versus-player combat that has been running continuously, and 100% free, for over 25 years.\n\nChoose from among 21 races, 17 highly customizable classes, and several cabals and religions to suit your playstyle and the story you want to tell. Our massive, original world is full of secrets and envied limited objects that take skill to acquire and great care to keep.\n\nWe like to think of ourselves as the Dark Souls of MUDs, with a community that is supportive of new players - unforgiving though our world may be. Join us for a real challenge and real rewards: adrenalin-pumping battles, memorable quests run by our volunteer immortal staff, and stories that will stick with you for a lifetime.");
    } else if (hostUrl == QLatin1String("godwars2.org")) {
        return QLatin1String(
                "God Wars II is a fast and furious combat mud, designed to test player skill in terms of pre-battle preparation and on-the-spot reflexes, as well as the ability to adapt quickly to "
                "new situations. Take on the role of a godlike supernatural being in a fight for supremacy.\n\nRoomless world. Manual combat. Endless possibilities.");
    } else if (hostUrl == QLatin1String("3k.org")) {
        if (port == 3200) {
            return QLatin1String(
                    "3Scapes is an alternative dimension to 3Kingdoms, similar in many respects, but unique and twisted in so many ways.  3Scapes offers a faster pace of play, along with "
                    "an assortment "
                    "of new guilds, features, and areas.");
        } else { // port==3000
            return QLatin1String(
                    "Simple enough to learn, yet complex enough to challenge you for years, 3Kingdoms is a colossal adventure through which many years of active and continued development by its "
                    "dedicated coding staff.  Based around the mighty town of Pinnacle, three main realms beckon the player to explore. These kingdoms are known as: Fantasy, a vast medieval realm "
                    "full "
                    "of orcs, elves, dragons, and a myriad of other creatures; Science, a post-apocalyptic, war-torn world set in the not-so-distant future; and Chaos, a transient realm where the "
                    "enormous realities of Fantasy and Science collide to produce creatures so bizarre that they have yet to be categorized.  During their exploration of the realms, players have the "
                    "opportunity to join any of well over a dozen different guilds, which grant special, unique powers to the player, furthering their abilities as they explore the vast expanses of "
                    "each realm. Add in the comprehensive skill system that 3K offers and you are able to extensively customize your characters.");
        }
    } else if (hostUrl == QLatin1String("slothmud.org")) {
        return QLatin1String(
                "SlothMUD... the ultimate in DIKUMUD! The most active, intricate, exciting FREE MUD of its kind. This text based multiplayer free online rpg game and is enjoyed continuously by "
                "players worldwide. With over 27,500 uniquely described rooms, 9,300 distinct creatures, 14,200 characters, and 87,100 pieces of equipment, charms, trinkets and other items, our "
                "online rpg world is absolutely enormous and ready to explore.");
    } else if (hostUrl == QLatin1String("game.wotmud.org")) {
        return QLatin1String(
                "WoTMUD is the most popular on-line game based on the late Robert Jordan's epic Wheel of Time fantasy novels.\n"
                "Not only totally FREE to play since it started in 1993 it was officially sanctioned by the Author himself.\n"
                "Explore a World very like that of Rand al'Thor's; from the Blight in the North down to the Isle of Madmen far, far south.\n"
                "Wander around in any of the towns from the books such as Caemlyn, Tar Valon or Tear, or start your adventure in the Two Rivers area, not YET the home of the Dragon Reborn.\n"
                "Will you join one of the Clans working for the triumph of the Light over the creatures and minions of the Dark One; or will you be one of the returning invaders in the South West, "
                "descendants of Artur Hawkwing's long-thought lost Armies; or just maybe you are skilled enough to be a hideous Trolloc, creature of the Dark, who like Humans - but only as a source "
                "of sustenance.\n"
                "Very definitely a Player Verses Player (PvP) world but with strong Role Playing (RP) too; nowhere is totally safe but some parts are much more dangerous than others - once you "
                "enter you may never leave...");
    } else if (hostUrl == QStringLiteral("midnightsun2.org")) {
        return QLatin1String(
                "Midnight Sun is a medieval fantasy LPmud that has been around since 1991. We are a non-PK, hack-and-slash game, cooperative rather than competitive in nature, and with a strong "
                "sense of community.");
    } else if (hostUrl == QStringLiteral("mudlet.org")) {
        return QLatin1String(
                "This isn't a game profile, but a special one for testing Mudlet itself using Busted. You can also use it as a starting point to create automated tests for your own profiles!");
    } else if (hostUrl == QStringLiteral("luminarimud.com")) {
        return QLatin1String("Luminari is a deep, engaging game set in the world of the Luminari - A place where magic is entwined with the fabric of reality and the forces of evil and destruction "
                             "are rising from a long slumber to again wreak havoc on the realm.  The gameplay of Luminari will be familiar to anyone who has played Dungeons and Dragons, Pathfinder "
                             "or any of the many RPG systems based on the d20 ruleset.");
    } else if (hostUrl == QStringLiteral("stickmud.com")) {
        return QStringLiteral("StickMUD is a free, medieval fantasy game with a graphical user interface and a depth of features. You are welcomed into the game world with maps and dashboards to complement your imagination. Newbies escape quickly into game play with minimal study time. Awaken under the wondrous Mallorn Tree in the center of Newbie Park and learn by playing. Challenge non-player characters to gain experience, advance level and maximize your stats. Between battles, sit on the enchanted bench under the Tree to rapidly heal and reduce wait time. Signs in the park present game features such as races, clans and guilds. Read up on teasers about the adventures on the path ahead like dragons, castles and sailing. Upon maturing to level 5, join a guild and learn the ways of a Bard, Fighter, Mage, Necromancer, Ninja, Thief, Healer or Priest. Train skills in both craft and combat aligned with your guild. Participate in frequent game-wide events to earn points exchanged for gold, experience or skill training. Heroes and villains alike are invited! Role play is optional and player vs. player combat is allowed in much of the game. StickMUD was born in Finland in June 1991 and is now hosted in Canada. Our diverse community of players and active game engineers are ready to welcome new players like you to one of the best text-based multi-player games ever!");
    } else if (hostUrl == QStringLiteral("reinosdeleyenda.es")) {
        return QStringLiteral(
                "The oldest Spanish free mud with more than 20 years of running history.\n\n"
                "Reinos de Leyenda takes place in the ever changing world of Eirea, ravaged by the mischiefs of the gods after "
                "more than a thousand years of contempt and hideous war amongst their zealous mortal pawns.\n\n"
                "History is written on a day per day basis, taking into consideration the players' choices "
                "to decide the irreversible aftermath of this everlasting struggle.\n\n"
                "This is a PvP MUD which allows the player to set how high are the stakes: the more you risk losing upon death, the more glory to be earned by your heroism. RP, while "
                "not enforced, is rewarded with non-PvP oriented perks and unique treasure.\n\n"
                "A powerful character customization system allows you to choose your deity –or fully disregard the gods– and join one of the player-run realms that govern the land "
                "to explore a breathing world, delve into the secrets of the oceans, shape your legacy, craft forgotten marvels for you –or your allies– and fight for faith, glory or coin.");
        /**
                 * Translation to the following text to Spanish as per request from SlyVen on PR #1505.
                 * -- begin translation --
                 * El mud Español gratis con más de 20 años de historia.
                 *
                 * Reinos de Leyenda toma lugar en el siempre cambiante mundo de Eirea, devastado por las intrigas de los dioses tras más de un millar de años de desprecio y cruenta guerra entre sus fanáticos peones mortales.
                 *
                 * La historia se escribe día a día, tomando en consideración las elecciones de los jugadores para decidir las consecuencias irreversibles de este conflicto imperecedero.
                 *
                 * Éste es un MUD con PvP que permite al jugador establecer cuánto quiere arriesgar al morir: a más riesgo, más gloria ganará por sus heroicidades. La interpretación (Rol) no está obligada, pero si recompensada
                 * con habilidades especiales -no orientadas al combate- y tesoros únicos.
                 *
                 * El detallado creador del juego te permitirá elegir tu deidad -o renegar completamente de los dioses- y unirte a uno de los reinos que los jugadores se encargan de gobernar para explorar un mundo viviente, sumergirte en los misterios del océano,
                 * dar forma a tu legado, forjar maravillas olvidadas para ti -o tus aliados- y luchar por fe, gloria o dinero.
                 * -- end translation --
                 */
    } else if (hostUrl == QStringLiteral("mud.clessidra.it")) {
        return QStringLiteral(
                "Clessidra is the first all italian MUD ever created! On Clessidra you may find only original Areas, all in italian! Many features make Clessidra one of the best, or the best, MUD in Italy : Advanced travel mode, fight one to one versus your friend, or enemy, The Arena and its fight, the Mortal Challenge, the intelligent MOBs and their Quest and fighting style, a random automatic mission assignament and for you and your friends you must try the advanced Clan system that allows wars and conquest. A mercenary system to help playing when few players are online, a crafting system to create special object and a graphical user interface to help newbie and expert players have a better experince. A MUD that evolves with new challenge, new rules, new skills!");
        /**
                 * Original Italian
                 * -- begin translation --
                 * Clessidra e' il primo MUD completamente in italiano mai creato. Su Clessidra potrete trovare solo aree originali ed in italiano. Molte caratteristiche rendono Clessidra uno dei migliori, se non il migliore, MUD in Italia : Avanzati sistemi di spostamento, sfide uno-contro-uno contro gli amici, o i nemici, L'arena e i combattimenti, Le sfide all'ultimo sangue e i MOB intelligenti con le loro Quest e tecniche di combattimento, un sistema di assegnazione di missioni casuali e un avanzatissimo sistema di Clan che permettera' guerre e conquiste. Disponibilità di mercenari in caso di poca utenza, sistema di produzione/mercato per ottenere esclusivi oggetti, un interfaccia grafica per aiutarti a giocare, sia per i novizi che gli esperti. Un MUD che si evolve di continuo.
                 * -- end translation --
                 */
    } else if (hostUrl == QStringLiteral("fierymud.org")) {
        return QStringLiteral(
                "The original vision of FieryMUD was to create a challanging MUD for advanced players. This new reborne Fiery is a hope to bring back the goals of the past by inflicting certain death on unsuspecting players. FieryMUD will continue to grow and change through the coming years and those players who seek challenge and possess imagination will come in search of what the 3D world fails to offer them.");
    } else {
        return readProfileData(profile_name, QStringLiteral("description"));
    }
}

void dlgConnectionProfiles::slot_item_clicked(QListWidgetItem* pItem)
{
    if (!pItem) {
        return;
    }

    slot_togglePasswordVisibility(false);

    const QString profile_name = pItem->data(csmNameRole).toString();

    profile_name_entry->setText(profile_name);

    QString host_url = readProfileData(profile_name, QStringLiteral("url"));
    if (host_url.isEmpty()) {
        // Host to connect to, see below for port
        if (profile_name == QStringLiteral("Avalon.de")) {
            host_url = QStringLiteral("avalon.mud.de");
        }
        if (profile_name == QStringLiteral("God Wars II")) {
            host_url = QStringLiteral("godwars2.org");
        }
        if (profile_name == QStringLiteral("Materia Magica")) {
            host_url = QStringLiteral("materiamagica.com");
        }
        if (profile_name == QStringLiteral("BatMUD")) {
            host_url = QStringLiteral("batmud.bat.org");
        }
        if (profile_name == QStringLiteral("Aardwolf")) {
            host_url = QStringLiteral("aardmud.org");
        }
        if (profile_name == QStringLiteral("Achaea")) {
            host_url = QStringLiteral("achaea.com");
        }
        if (profile_name == QStringLiteral("Aetolia")) {
            host_url = QStringLiteral("aetolia.com");
        }
        if (profile_name == QStringLiteral("Lusternia")) {
            host_url = QStringLiteral("lusternia.com");
        }
        if (profile_name == QStringLiteral("Imperian")) {
            host_url = QStringLiteral("imperian.com");
        }
        if (profile_name == QStringLiteral("Realms of Despair")) {
            host_url = QStringLiteral("realmsofdespair.com");
        }
        if (profile_name == QStringLiteral("ZombieMUD")) {
            host_url = QStringLiteral("zombiemud.org");
        }
        if (profile_name == QStringLiteral("Carrion Fields")) {
            host_url = QStringLiteral("carrionfields.net");
        }
        if (profile_name == QStringLiteral("3Scapes")) {
            host_url = QStringLiteral("3k.org");
        }
        if (profile_name == QStringLiteral("3Kingdoms")) {
            host_url = QStringLiteral("3k.org");
        }
        if (profile_name == QStringLiteral("Slothmud")) {
            host_url = QStringLiteral("slothmud.org");
        }
        if (profile_name == QStringLiteral("WoTMUD")) {
            host_url = QStringLiteral("game.wotmud.org");
        }
        if (profile_name == QStringLiteral("Midnight Sun 2")) {
            host_url = QStringLiteral("midnightsun2.org");
        }
        if (profile_name == QStringLiteral("Luminari")) {
            host_url = QStringLiteral("luminarimud.com");
        }
        if (profile_name == QStringLiteral("StickMUD")) {
            host_url = QStringLiteral("stickmud.com");
        }
        if (profile_name == QStringLiteral("Clessidra")) {
            host_url = QStringLiteral("mud.clessidra.it");
        }
        if (profile_name == QStringLiteral("Reinos de Leyenda")) {
            host_url = QStringLiteral("reinosdeleyenda.es");
        }
        if (profile_name == QStringLiteral("Fierymud")) {
            host_url = QStringLiteral("fierymud.org");
        }
        if (profile_name == QStringLiteral("Mudlet self-test")) {
            host_url = QStringLiteral("mudlet.org");
        }
    }
    host_name_entry->setText(host_url);

    QString host_port = readProfileData(profile_name, QStringLiteral("port"));
    QString val = readProfileData(profile_name, QStringLiteral("ssl_tsl"));
    if (val.toInt() == Qt::Checked) {
        port_ssl_tsl->setChecked(true);
    } else {
        port_ssl_tsl->setChecked(false);
    }

    if (host_port.isEmpty()) {
        if (profile_name == QStringLiteral("Avalon.de")) {
            //host_url = QStringLiteral("avalon.mud.de");
            host_port = QStringLiteral("23");
            port_ssl_tsl->setChecked(false);
        }
        if (profile_name == QStringLiteral("God Wars II")) {
            //host_url = QStringLiteral("godwars2.org");
            host_port = QStringLiteral("3000");
            port_ssl_tsl->setChecked(false);
        }
        if (profile_name == QStringLiteral("Materia Magica")) {
            //host_url = QStringLiteral("materiamagica.com");
            host_port = QStringLiteral("23");
            port_ssl_tsl->setChecked(false);
        }
        if (profile_name == QStringLiteral("BatMUD")) {
            //host_url = QStringLiteral("batmud.bat.org");
            host_port = QStringLiteral("23");
            port_ssl_tsl->setChecked(false);
        }
        if (profile_name == QStringLiteral("Aardwolf")) {
            //host_url = QStringLiteral("aardmud.org");
            host_port = QStringLiteral("4000");
            port_ssl_tsl->setChecked(false);
        }
        if (profile_name == QStringLiteral("Achaea")) {
            //host_url = QStringLiteral("achaea.com");
            host_port = QStringLiteral("23");
            port_ssl_tsl->setChecked(false);
        }
        if (profile_name == QStringLiteral("Aetolia")) {
            //host_url = QStringLiteral("aetolia.com");
            host_port = QStringLiteral("23");
            port_ssl_tsl->setChecked(false);
        }
        if (profile_name == QStringLiteral("Lusternia")) {
            //host_url = QStringLiteral("lusternia.com");
            host_port = QStringLiteral("23");
            port_ssl_tsl->setChecked(false);
        }
        if (profile_name == QStringLiteral("Imperian")) {
            //host_url = QStringLiteral("imperian.com");
            host_port = QStringLiteral("23");
            port_ssl_tsl->setChecked(false);
        }
        if (profile_name == QStringLiteral("Realms of Despair")) {
            //host_url = QStringLiteral("realmsofdespair.com");
            host_port = QStringLiteral("4000");
            port_ssl_tsl->setChecked(false);
        }
        if (profile_name == QStringLiteral("ZombieMUD")) {
            //host_url = QStringLiteral("zombiemud.org");
            host_port = QStringLiteral("23");
            port_ssl_tsl->setChecked(false);
        }
        if (profile_name == QStringLiteral("Carrion Fields")) {
            host_port = QStringLiteral("4449");
            port_ssl_tsl->setChecked(false);
        }
        if (profile_name == QStringLiteral("3Scapes")) {
            //host_url = QStringLiteral("3k.org");
            host_port = QStringLiteral("3200");
            port_ssl_tsl->setChecked(false);
        }
        if (profile_name == QStringLiteral("3Kingdoms")) {
            //host_url = QStringLiteral("3k.org");
            host_port = QStringLiteral("3000");
            port_ssl_tsl->setChecked(false);
        }
        if (profile_name == QStringLiteral("Slothmud")) {
            //host_url = QStringLiteral("slothmud.org");
            host_port = QStringLiteral("6101");
            port_ssl_tsl->setChecked(false);
        }
        if (profile_name == QStringLiteral("WoTMUD")) {
            //host_url = QStringLiteral("game.wotmud.org");
            host_port = QStringLiteral("2224");
            port_ssl_tsl->setChecked(false);
        }
        if (profile_name == QStringLiteral("Midnight Sun 2")) {
            //host_url = QStringLiteral("midnightsun2.org");
            host_port = QStringLiteral("3000");
            port_ssl_tsl->setChecked(false);
        }
        if (profile_name == QStringLiteral("Luminari")) {
            //host_url = QStringLiteral("luminarimud.com");
            host_port = QStringLiteral("4100");
            port_ssl_tsl->setChecked(false);
        }
        if (profile_name == QStringLiteral("StickMUD")) {
            host_port = QStringLiteral("8680");
            port_ssl_tsl->setChecked(true);
        }
        if (profile_name == QStringLiteral("Clessidra")) {
            host_port = QStringLiteral("4000");
        }
        if (profile_name == QStringLiteral("Reinos de Leyenda")) {
            //host_url = QStringLiteral("reinosdeleyenda.es");
            host_port = QStringLiteral("23");
            port_ssl_tsl->setChecked(false);
        }
        if (profile_name == QStringLiteral("Fierymud")) {
            host_port = QStringLiteral("4000");
        }
        if (profile_name == QStringLiteral("Mudlet self-test")) {
            host_port = QStringLiteral("23");
            port_ssl_tsl->setChecked(false);
        }
    }

    port_entry->setText(host_port);

    // if we're currently copying a profile, don't blank and re-load the password,
    // because there isn't one in storage yet. It'll be copied over into the widget
    // by the copy method
    if (!mCopyingProfile) {
        character_password_entry->setText(QString());
        loadSecuredPassword(profile_name, [this, profile_name](const QString& password) {
            if (!password.isEmpty()) {
                character_password_entry->setText(password);
            } else {
                character_password_entry->setText(readProfileData(profile_name, QStringLiteral("password")));
            }
        });
    }

    val = readProfileData(profile_name, QStringLiteral("login"));
    login_entry->setText(val);

    val = readProfileData(profile_name, QStringLiteral("autologin"));
    if (val.toInt() == Qt::Checked) {
        autologin_checkBox->setChecked(true);
    } else {
        autologin_checkBox->setChecked(false);
    }

    val = readProfileData(profile_name, QStringLiteral("autoreconnect"));
    if (!val.isEmpty() && val.toInt() == Qt::Checked) {
        auto_reconnect->setChecked(true);
    } else {
        auto_reconnect->setChecked(false);
    }

    mDiscordApplicationId = readProfileData(profile_name, QStringLiteral("discordApplicationId"));

    // val will be null if this is the first time the profile has been read
    // since an update to a Mudlet version supporting Discord - so a toint()
    // will return 0 - which just happens to be Qt::Unchecked() but lets not
    // rely on that...
    val = readProfileData(profile_name, QStringLiteral("discordserveroptin"));
    if ((!val.isEmpty()) && val.toInt() == Qt::Checked) {
        discord_optin_checkBox->setChecked(true);
    } else {
        discord_optin_checkBox->setChecked(false);
    }

    updateDiscordStatus();

    mud_description_textedit->setPlainText(getDescription(host_url, host_port.toUInt(), profile_name));

    val = readProfileData(profile_name, QStringLiteral("website"));
    if (val.isEmpty()) {
        if (profile_name == QStringLiteral("Avalon.de")) {
            val = QStringLiteral("<center><a href='http://avalon.mud.de'>http://avalon.mud.de</a></center>");
        }
        if (profile_name == QStringLiteral("God Wars II")) {
            val = QStringLiteral("<center><a href='http://www.godwars2.org'>http://www.godwars2.org</a></center>");
        }
        if (profile_name == QStringLiteral("Materia Magica")) {
            val = QStringLiteral("<center><a href='http://www.materiamagica.com'>http://www.materiamagica.com</a></center>");
        }
        if (profile_name == QStringLiteral("BatMUD")) {
            val = QStringLiteral("<center><a href='http://www.bat.org'>http://www.bat.org</a></center>");
        }
        if (profile_name == QStringLiteral("Aardwolf")) {
            val = QStringLiteral("<center><a href='http://www.aardwolf.com/'>http://www.aardwolf.com</a></center>");
        }
        if (profile_name == QStringLiteral("Achaea")) {
            val = QStringLiteral("<center><a href='http://www.achaea.com/'>http://www.achaea.com</a></center>");
        }
        if (profile_name == QStringLiteral("Realms of Despair")) {
            val = QStringLiteral("<center><a href='http://www.realmsofdespair.com/'>http://www.realmsofdespair.com</a></center>");
        }
        if (profile_name == QStringLiteral("ZombieMUD")) {
            val = QStringLiteral("<center><a href='http://www.zombiemud.org/'>http://www.zombiemud.org</a></center>");
        }
        if (profile_name == QStringLiteral("Carrion Fields")) {
            val = QStringLiteral("<center><a href='http://www.carrionfields.net'>www.carrionfields.net</a></center>");
        }
        if (profile_name == QStringLiteral("Aetolia")) {
            val = QStringLiteral("<center><a href='http://www.aetolia.com/'>http://www.aetolia.com</a></center>");
        }
        if (profile_name == QStringLiteral("Lusternia")) {
            val = QStringLiteral("<center><a href='http://www.lusternia.com/'>http://www.lusternia.com</a></center>");
        }
        if (profile_name == QStringLiteral("Imperian")) {
            val = QStringLiteral("<center><a href='http://www.imperian.com/'>http://www.imperian.com</a></center>");
        }
        if (profile_name == QStringLiteral("3Scapes")) {
            val = QStringLiteral("<center><a href='http://www.3scapes.org/'>http://www.3scapes.org</a></center>");
        }
        if (profile_name == QStringLiteral("3Kingdoms")) {
            val = QStringLiteral("<center><a href='http://www.3k.org/'>http://www.3k.org</a></center>");
        }
        if (profile_name == QStringLiteral("Slothmud")) {
            val = QStringLiteral("<center><a href='http://www.slothmud.org/'>http://www.slothmud.org/</a></center>");
        }
        if (profile_name == QStringLiteral("WoTMUD")) {
            val = QStringLiteral("<center><a href='http://www.wotmud.org/'>Main website</a></center>\n"
                                 "<center><a href='http://www.wotmod.org/'>Forums</a></center>");
        }
        if (profile_name == QStringLiteral("Midnight Sun 2")) {
            val = QStringLiteral("<center><a href='http://midnightsun2.org/'>http://midnightsun2.org/</a></center>");
        }
        if (profile_name == QStringLiteral("Luminari")) {
            val = QStringLiteral("<center><a href='http://www.luminarimud.com/'>http://www.luminarimud.com/</a></center>");
        }
        if (profile_name == QStringLiteral("StickMUD")) {
            val = QStringLiteral("<center><a href='http://www.stickmud.com/'>stickmud.com</a></center>");
        }
        if (profile_name == QStringLiteral("Clessidra")) {
            val = QStringLiteral("<center><a href='http://www.clessidra.it/'>http://www.clessidra.it</a></center>");
        }
        if (profile_name == QStringLiteral("Fierymud")) {
            val = QStringLiteral("<center><a href='https://www.fierymud.org/'>https://www.fierymud.org</a></center>");
        }
        if (profile_name == QStringLiteral("Reinos de Leyenda")) {
            val = QStringLiteral("<center><a href='https://www.reinosdeleyenda.es/'>Main website</a></center>\n"
                                 "<center><a href='https://www.reinosdeleyenda.es/foro/'>Forums</a></center>\n"
                                 "<center><a href='https://wiki.reinosdeleyenda.es/'>Wiki</a></center>\n"
                                 );
        }
    }
    website_entry->setText(val);

    profile_history->clear();

    QDir dir(mudlet::getMudletPath(mudlet::profileXmlFilesPath, profile_name));
    dir.setSorting(QDir::Time);
    QStringList entries = dir.entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Time);

    for (const auto& entry : entries) {
        QRegularExpression rx(QStringLiteral("(\\d+)\\-(\\d+)\\-(\\d+)#(\\d+)\\-(\\d+)\\-(\\d+).xml"));
        QRegularExpressionMatch match = rx.match(entry);

        if (match.capturedStart() != -1) {
            QString day;
            QString month = match.captured(2);
            QString year;
            QString hour = match.captured(4);
            QString minute = match.captured(5);
            QString second = match.captured(6);
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
            QFileInfo fileInfo(dir, entry);
            auto lastModified = fileInfo.lastModified();
            profile_history->addItem(QIcon::fromTheme(QStringLiteral("document-save"), QIcon(QStringLiteral(":/icons/document-save.png"))), mudlet::self()->getUserLocale().toString(lastModified, mDateTimeFormat), QVariant(entry));
        } else {
            profile_history->addItem(entry, QVariant(entry)); // if it has a custom name, use it as it is
        }
    }

    profile_history->setEnabled(static_cast<bool>(profile_history->count()));

    const QString profileLoadedMessage = tr("This profile is currently loaded - close it before changing the connection parameters.");

    QStringList loadedProfiles = mudlet::self()->getHostManager().getHostList();
    if (loadedProfiles.contains(profile_name)) {
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
            notificationArea->hide();
            notificationAreaIconLabelWarning->hide();
            notificationAreaIconLabelError->hide();
            notificationAreaIconLabelInformation->hide();
            notificationAreaMessageBox->hide();
            notificationAreaMessageBox->setText(QString());
        }
    }
}

void dlgConnectionProfiles::updateDiscordStatus()
{
    auto discordLoaded = mudlet::self()->mDiscord.libraryLoaded();

    if (!discordLoaded) {
        discord_optin_checkBox->setEnabled(false);
        discord_optin_checkBox->setChecked(false);
        discord_optin_checkBox->setToolTip(tr("Discord integration not available on this platform"));
    } else if (mDiscordApplicationId.isEmpty() && !mudlet::self()->mDiscord.gameIntegrationSupported(host_name_entry->text().trimmed()).first) {
        // Disable discord support if it is not recognised by name and a
        // Application Id has not been previously entered:
        discord_optin_checkBox->setEnabled(false);
        discord_optin_checkBox->setChecked(false);
        discord_optin_checkBox->setToolTip(tr("Discord integration not supported by game"));
    } else {
        discord_optin_checkBox->setEnabled(true);
        discord_optin_checkBox->setToolTip(tr("Check to enable Discord integration"));
    }
}

// (re-)creates the dialogs profile list
void dlgConnectionProfiles::fillout_form()
{
    profiles_tree_widget->clear();
    profile_name_entry->clear();
    host_name_entry->clear();
    port_entry->clear();

    mProfileList = QDir(mudlet::getMudletPath(mudlet::profilesPath)).entryList(QDir::Dirs | QDir::NoDotAndDotDot,
                                                                               QDir::Name);

    if (mProfileList.isEmpty()) {
        welcome_message->show();
        requiredArea->hide();
        informationalArea->hide();
        optionalArea->hide();

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

        requiredArea->show();
        informationalArea->show();
        optionalArea->show();
    }

    profiles_tree_widget->setIconSize(QSize(120, 30));
    QFont font(QStringLiteral("Bitstream Vera Sans Mono"), 1, QFont::Normal);
    // This (and setting the font color to white on a white background for an
    // unselected widget) is a hack that minimises - but does not remove the
    // QString assigned as the "name" of the QListWidgetItem in the constructor
    // we use - unfortunately we currently need that text programmatically at
    // present to identify each item - more work is needed, and is plausable, to
    // completely resolve this. -Slysven
    QString mudServer, description;
    QListWidgetItem* pItem;
    QIcon mi;

    auto& settings = *mudlet::self()->mpSettings;
    auto deletedDefaultMuds = settings.value(QStringLiteral("deletedDefaultMuds"), QStringList()).toStringList();

    mudServer = QStringLiteral("Avalon.de");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pItem = new QListWidgetItem();
        pItem->setData(csmNameRole, mudServer);
        pItem->setData(Qt::AccessibleTextRole, item_profile_accessName.arg(mudServer));
        pItem->setData(Qt::AccessibleDescriptionRole, item_profile_accessDesc);

        profiles_tree_widget->addItem(pItem);
        if (!hasCustomIcon(mudServer)) {
            QPixmap p(QStringLiteral(":/icons/avalon.png"));
            mi = QIcon(p.scaled(QSize(120, 30)));
            pItem->setIcon(mi);
        } else {
            setCustomIcon(mudServer, pItem);
        }
        description = getDescription(QStringLiteral("avalon.mud.de"), 0, mudServer);
        if (!description.isEmpty()) {
            pItem->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("Achaea");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pItem = new QListWidgetItem();
        pItem->setData(csmNameRole, mudServer);
        pItem->setData(Qt::AccessibleTextRole, item_profile_accessName.arg(mudServer));
        pItem->setData(Qt::AccessibleDescriptionRole, item_profile_accessDesc);

        profiles_tree_widget->addItem(pItem);
        if (!hasCustomIcon(mudServer)) {
            mi = QIcon(QStringLiteral(":/icons/achaea_120_30.png"));
            pItem->setIcon(mi);
        } else {
            setCustomIcon(mudServer, pItem);
        }
        description = getDescription(QStringLiteral("achaea.com"), 0, mudServer);
        if (!description.isEmpty()) {
            pItem->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("3Kingdoms");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pItem = new QListWidgetItem();
        pItem->setData(csmNameRole, mudServer);
        pItem->setData(Qt::AccessibleTextRole, item_profile_accessName.arg(mudServer));
        pItem->setData(Qt::AccessibleDescriptionRole, item_profile_accessDesc);

        profiles_tree_widget->addItem(pItem);
        if (!hasCustomIcon(mudServer)) {
            QPixmap pc(QStringLiteral(":/icons/3klogo.png"));
            QPixmap pc1 = pc.scaled(QSize(120, 30), Qt::IgnoreAspectRatio, Qt::SmoothTransformation).copy();
            mi = QIcon(pc1);
            pItem->setIcon(mi);
        } else {
            setCustomIcon(mudServer, pItem);
        }
        description = getDescription(QStringLiteral("3k.org"), 3000, mudServer);
        if (!description.isEmpty()) {
            pItem->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("3Scapes");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pItem = new QListWidgetItem();
        pItem->setData(csmNameRole, mudServer);
        pItem->setData(Qt::AccessibleTextRole, item_profile_accessName.arg(mudServer));
        pItem->setData(Qt::AccessibleDescriptionRole, item_profile_accessDesc);

        profiles_tree_widget->addItem(pItem);
        if (!hasCustomIcon(mudServer)) {
            QPixmap pc(QStringLiteral(":/icons/3slogo.png"));
            QPixmap pc1 = pc.scaled(QSize(120, 30), Qt::IgnoreAspectRatio, Qt::SmoothTransformation).copy();
            mi = QIcon(pc1);
            pItem->setIcon(mi);
        } else {
            setCustomIcon(mudServer, pItem);
        }
        description = getDescription(QStringLiteral("3k.org"), 3200, mudServer);
        if (!description.isEmpty()) {
            pItem->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("Lusternia");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pItem = new QListWidgetItem();
        pItem->setData(csmNameRole, mudServer);
        pItem->setData(Qt::AccessibleTextRole, item_profile_accessName.arg(mudServer));
        pItem->setData(Qt::AccessibleDescriptionRole, item_profile_accessDesc);

        profiles_tree_widget->addItem(pItem);
        if (!hasCustomIcon(mudServer)) {
            mi = QIcon(QStringLiteral(":/icons/lusternia_120_30.png"));
            pItem->setIcon(mi);
        } else {
            setCustomIcon(mudServer, pItem);
        }
        description = getDescription(QStringLiteral("lusternia.com"), 0, mudServer);
        if (!description.isEmpty()) {
            pItem->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("BatMUD");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pItem = new QListWidgetItem();
        pItem->setData(csmNameRole, mudServer);
        pItem->setData(Qt::AccessibleTextRole, item_profile_accessName.arg(mudServer));
        pItem->setData(Qt::AccessibleDescriptionRole, item_profile_accessDesc);

        if (!hasCustomIcon(mudServer)) {
            mi = QIcon(QStringLiteral(":/icons/batmud_mud.png"));
            pItem->setIcon(mi);
        } else {
            setCustomIcon(mudServer, pItem);
        }
        profiles_tree_widget->addItem(pItem);
        description = getDescription(QStringLiteral("batmud.bat.org"), 0, mudServer);
        if (!description.isEmpty()) {
            pItem->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("God Wars II");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pItem = new QListWidgetItem();
        pItem->setData(csmNameRole, mudServer);
        pItem->setData(Qt::AccessibleTextRole, item_profile_accessName.arg(mudServer));
        pItem->setData(Qt::AccessibleDescriptionRole, item_profile_accessDesc);

        profiles_tree_widget->addItem(pItem);
        if (!hasCustomIcon(mudServer)) {
            mi = QIcon(QStringLiteral(":/icons/gw2.png"));
            pItem->setIcon(mi);
        } else {
            setCustomIcon(mudServer, pItem);
        }
        description = getDescription(QStringLiteral("godwars2.org"), 0, mudServer);
        if (!description.isEmpty()) {
            pItem->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("Slothmud");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pItem = new QListWidgetItem();
        pItem->setData(csmNameRole, mudServer);
        pItem->setData(Qt::AccessibleTextRole, item_profile_accessName.arg(mudServer));
        pItem->setData(Qt::AccessibleDescriptionRole, item_profile_accessDesc);

        profiles_tree_widget->addItem(pItem);
        if (!hasCustomIcon(mudServer)) {
            mi = QIcon(QStringLiteral(":/icons/Slothmud.png"));
            pItem->setIcon(mi);
        } else {
            setCustomIcon(mudServer, pItem);
        }
        description = getDescription(QStringLiteral("slothmud.org"), 0, mudServer);
        if (!description.isEmpty()) {
            pItem->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("Aardwolf");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pItem = new QListWidgetItem();
        pItem->setData(csmNameRole, mudServer);
        pItem->setData(Qt::AccessibleTextRole, item_profile_accessName.arg(mudServer));
        pItem->setData(Qt::AccessibleDescriptionRole, item_profile_accessDesc);

        profiles_tree_widget->addItem(pItem);
        if (!hasCustomIcon(mudServer)) {
            mi = QIcon(QStringLiteral(":/icons/aardwolf_mud.png"));
            pItem->setIcon(mi);
        } else {
            setCustomIcon(mudServer, pItem);
        }
        description = getDescription(QStringLiteral("aardmud.org"), 0, mudServer);
        if (!description.isEmpty()) {
            pItem->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("Materia Magica");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pItem = new QListWidgetItem();
        pItem->setData(csmNameRole, mudServer);
        pItem->setData(Qt::AccessibleTextRole, item_profile_accessName.arg(mudServer));
        pItem->setData(Qt::AccessibleDescriptionRole, item_profile_accessDesc);

        profiles_tree_widget->addItem(pItem);
        if (!hasCustomIcon(mudServer)) {
            mi = QIcon(QStringLiteral(":/materiaMagicaIcon"));
            pItem->setIcon(mi);
        } else {
            setCustomIcon(mudServer, pItem);
        }
        description = getDescription(QStringLiteral("materiamagica.com"), 0, mudServer);
        if (!description.isEmpty()) {
            pItem->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("Realms of Despair");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pItem = new QListWidgetItem();
        pItem->setData(csmNameRole, mudServer);
        pItem->setData(Qt::AccessibleTextRole, item_profile_accessName.arg(mudServer));
        pItem->setData(Qt::AccessibleDescriptionRole, item_profile_accessDesc);

        profiles_tree_widget->addItem(pItem);
        if (!hasCustomIcon(mudServer)) {
            mi = QIcon(QStringLiteral(":/icons/120x30RoDLogo.png"));
            pItem->setIcon(mi);
        } else {
            setCustomIcon(mudServer, pItem);
        }
        description = getDescription(QStringLiteral("realmsofdespair.com"), 0, mudServer);
        if (!description.isEmpty()) {
            pItem->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("ZombieMUD");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pItem = new QListWidgetItem();
        pItem->setData(csmNameRole, mudServer);
        pItem->setData(Qt::AccessibleTextRole, item_profile_accessName.arg(mudServer));
        pItem->setData(Qt::AccessibleDescriptionRole, item_profile_accessDesc);

        profiles_tree_widget->addItem(pItem);
        if (!hasCustomIcon(mudServer)) {
            mi = QIcon(QStringLiteral(":/icons/zombiemud.png"));
            pItem->setIcon(mi);
        } else {
            setCustomIcon(mudServer, pItem);
        }
        description = getDescription(QStringLiteral("zombiemud.org"), 0, mudServer);
        if (!description.isEmpty()) {
            pItem->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("Aetolia");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pItem = new QListWidgetItem();
        pItem->setData(csmNameRole, mudServer);
        pItem->setData(Qt::AccessibleTextRole, item_profile_accessName.arg(mudServer));
        pItem->setData(Qt::AccessibleDescriptionRole, item_profile_accessDesc);

        profiles_tree_widget->addItem(pItem);
        if (!hasCustomIcon(mudServer)) {
            mi = QIcon(QStringLiteral(":/icons/aetolia_120_30.png"));
            pItem->setIcon(mi);
        } else {
            setCustomIcon(mudServer, pItem);
        }
        description = getDescription(QStringLiteral("aetolia.com"), 0, mudServer);
        if (!description.isEmpty()) {
            pItem->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("Imperian");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pItem = new QListWidgetItem();
        pItem->setData(csmNameRole, mudServer);
        pItem->setData(Qt::AccessibleTextRole, item_profile_accessName.arg(mudServer));
        pItem->setData(Qt::AccessibleDescriptionRole, item_profile_accessDesc);

        profiles_tree_widget->addItem(pItem);
        if (!hasCustomIcon(mudServer)) {
            mi = QIcon(QStringLiteral(":/icons/imperian_120_30.png"));
            pItem->setIcon(mi);
        } else {
            setCustomIcon(mudServer, pItem);
        }
        description = getDescription(QStringLiteral("imperian.com"), 0, mudServer);
        if (!description.isEmpty()) {
            pItem->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("WoTMUD");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pItem = new QListWidgetItem();
        pItem->setData(csmNameRole, mudServer);
        pItem->setData(Qt::AccessibleTextRole, item_profile_accessName.arg(mudServer));
        pItem->setData(Qt::AccessibleDescriptionRole, item_profile_accessDesc);

        profiles_tree_widget->addItem(pItem);
        if (!hasCustomIcon(mudServer)) {
            mi = QIcon(QPixmap(QStringLiteral(":/icons/wotmudicon.png")).scaled(QSize(120, 30), Qt::IgnoreAspectRatio,
                                                                                Qt::SmoothTransformation).copy());
            pItem->setIcon(mi);
        } else {
            setCustomIcon(mudServer, pItem);
        }
        description = getDescription(QStringLiteral("game.wotmud.org"), 0, mudServer);
        if (!description.isEmpty()) {
            pItem->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("Midnight Sun 2");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pItem = new QListWidgetItem();
        pItem->setData(csmNameRole, mudServer);
        pItem->setData(Qt::AccessibleTextRole, item_profile_accessName.arg(mudServer));
        pItem->setData(Qt::AccessibleDescriptionRole, item_profile_accessDesc);

        profiles_tree_widget->addItem(pItem);
        if (!hasCustomIcon(mudServer)) {
            mi = QIcon(QPixmap(QStringLiteral(":/icons/midnightsun2.png")).scaled(QSize(120, 30), Qt::IgnoreAspectRatio,
                                                                                  Qt::SmoothTransformation).copy());
            pItem->setIcon(mi);
        } else {
            setCustomIcon(mudServer, pItem);
        }
        description = getDescription(QStringLiteral("midnightsun2.org"), 0, mudServer);
        if (!description.isEmpty()) {
            pItem->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("Luminari");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pItem = new QListWidgetItem();
        pItem->setData(csmNameRole, mudServer);
        pItem->setData(Qt::AccessibleTextRole, item_profile_accessName.arg(mudServer));
        pItem->setData(Qt::AccessibleDescriptionRole, item_profile_accessDesc);

        profiles_tree_widget->addItem(pItem);
        if (!hasCustomIcon(mudServer)) {
            mi = QIcon(
                    QPixmap(QStringLiteral(":/icons/luminari_icon.png")).scaled(QSize(120, 30), Qt::IgnoreAspectRatio,
                                                                                Qt::SmoothTransformation).copy());
            pItem->setIcon(mi);
        } else {
            setCustomIcon(mudServer, pItem);
        }
        description = getDescription(QStringLiteral("luminarimud.com"), 0, mudServer);
        if (!description.isEmpty()) {
            pItem->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("StickMUD");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pItem = new QListWidgetItem();
        pItem->setData(csmNameRole, mudServer);
        pItem->setData(Qt::AccessibleTextRole, item_profile_accessName.arg(mudServer));
        pItem->setData(Qt::AccessibleDescriptionRole, item_profile_accessDesc);

        profiles_tree_widget->addItem(pItem);
        if (!hasCustomIcon(mudServer)) {
            mi = QIcon(
                    QPixmap(QStringLiteral(":/icons/stickmud_icon.jpg")).scaled(QSize(120, 30), Qt::IgnoreAspectRatio,
                                                                                Qt::SmoothTransformation).copy());
            pItem->setIcon(mi);
        } else {
            setCustomIcon(mudServer, pItem);
        }
        description = getDescription(QStringLiteral("stickmud.com"), 0, mudServer);
        if (!description.isEmpty()) {
            pItem->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("Clessidra");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pItem = new QListWidgetItem();
        pItem->setData(csmNameRole, mudServer);
        pItem->setData(Qt::AccessibleTextRole, item_profile_accessName.arg(mudServer));
        pItem->setData(Qt::AccessibleDescriptionRole, item_profile_accessDesc);

        profiles_tree_widget->addItem(pItem);
        if (!hasCustomIcon(mudServer)) {
            mi = QIcon(QPixmap(QStringLiteral(":/icons/clessidra.jpg")).scaled(QSize(120, 30), Qt::IgnoreAspectRatio,
                                                                               Qt::SmoothTransformation).copy());
            pItem->setIcon(mi);
        } else {
            setCustomIcon(mudServer, pItem);
        }
        description = getDescription(QStringLiteral("clessidra.it"), 0, mudServer);
        if (!description.isEmpty()) {
            pItem->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("Reinos de Leyenda");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pItem = new QListWidgetItem();
        pItem->setData(csmNameRole, mudServer);
        pItem->setData(Qt::AccessibleTextRole, item_profile_accessName.arg(mudServer));
        pItem->setData(Qt::AccessibleDescriptionRole, item_profile_accessDesc);

        if (!hasCustomIcon(mudServer)) {
            profiles_tree_widget->addItem(pItem);
            mi = QIcon(QPixmap(QStringLiteral(":/icons/reinosdeleyenda_mud.png")).scaled(QSize(120, 30),
                                                                                         Qt::IgnoreAspectRatio,
                                                                                         Qt::SmoothTransformation).copy());
            pItem->setIcon(mi);
        } else {
            setCustomIcon(mudServer, pItem);
        }
        description = getDescription(QStringLiteral("reinosdeleyenda.es"), 0, mudServer);
        if (!description.isEmpty()) {
            pItem->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }


    mudServer = QStringLiteral("Fierymud");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pItem = new QListWidgetItem();
        pItem->setData(csmNameRole, mudServer);
        pItem->setData(Qt::AccessibleTextRole, item_profile_accessName.arg(mudServer));
        pItem->setData(Qt::AccessibleDescriptionRole, item_profile_accessDesc);

        profiles_tree_widget->addItem(pItem);
        if (!hasCustomIcon(mudServer)) {
            mi = QIcon(QStringLiteral(":/icons/fiery_mud.png"));
            pItem->setIcon(mi);
        } else {
            setCustomIcon(mudServer, pItem);
        }
        description = getDescription(QStringLiteral("fierymud.org"), 0, mudServer);
        if (!description.isEmpty()) {
            pItem->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("Carrion Fields");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pItem = new QListWidgetItem();
        pItem->setData(csmNameRole, mudServer);
        pItem->setData(Qt::AccessibleTextRole, item_profile_accessName.arg(mudServer));
        pItem->setData(Qt::AccessibleDescriptionRole, item_profile_accessDesc);

        profiles_tree_widget->addItem(pItem);
        if (!hasCustomIcon(mudServer)) {
            mi = QIcon(QStringLiteral(":/icons/carrionfields.png"));
            pItem->setIcon(mi);
        } else {
            setCustomIcon(mudServer, pItem);
        }
        description = getDescription(QStringLiteral("carrionfields.net"), 0, mudServer);
        if (!description.isEmpty()) {
            pItem->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }


#if defined(QT_DEBUG)
    mudServer = QStringLiteral("Mudlet self-test");
    if (!deletedDefaultMuds.contains(mudServer) && !mProfileList.contains(mudServer)) {
        mProfileList.append(mudServer);
        pItem = new QListWidgetItem();
        pItem->setData(csmNameRole, mudServer);
        pItem->setData(Qt::AccessibleTextRole, item_profile_accessName.arg(mudServer));
        pItem->setData(Qt::AccessibleDescriptionRole, item_profile_accessDesc);

        profiles_tree_widget->addItem(pItem);
        description = getDescription(QStringLiteral("mudlet.org"), 0, mudServer);
        if (!description.isEmpty()) {
            pItem->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }
#endif

    setProfileIcon();

    QDateTime test_date;
    QString toselectProfileName;
    int toselectRow = -1;
    int test_profile_row = -1;
    bool firstMudletLaunch = true;

    for (int i = 0; i < profiles_tree_widget->count(); i++) {
        const auto profile = profiles_tree_widget->item(i);
        const auto profileName = profile->data(csmNameRole).toString();
        if (profileName == QStringLiteral("Mudlet self-test")) {
            test_profile_row = i;
        }

        const auto fileinfo = QFileInfo(
                    mudlet::getMudletPath(mudlet::profileXmlFilesPath, profileName));

        if (fileinfo.exists()) {
            firstMudletLaunch = false;
            QDateTime profile_lastRead = fileinfo.lastModified();
            // Since Qt 5.x null QTimes and QDateTimes are invalid - and might not
            // work as expected - so test for validity of the test_date value as well
            if ((!test_date.isValid()) || profile_lastRead > test_date) {
                test_date = profile_lastRead;
                toselectProfileName = profileName;
                toselectRow = i;
            }
        }
    }

    if (firstMudletLaunch) {
        // Select a random pre-defined profile to give all MUDs a fair go first time
        // make sure not to select the test_profile though
        if (profiles_tree_widget->count() > 1) {
            while (toselectRow == -1 || toselectRow == test_profile_row) {
                toselectRow = qrand() % profiles_tree_widget->count();
            }
        }
    }

    if (toselectRow != -1) {
        profiles_tree_widget->setCurrentRow(toselectRow);
    }

    updateDiscordStatus();
}

void dlgConnectionProfiles::setProfileIcon() const
{
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
            if (mDefaultGames.contains(profileName, Qt::CaseInsensitive)) {
                continue;
            }

            generateCustomProfile(i, profileName);
        }
    }
}

bool dlgConnectionProfiles::hasCustomIcon(const QString& profileName) const
{
    return QFileInfo::exists(mudlet::getMudletPath(mudlet::profileDataItemPath, profileName, QStringLiteral("profileicon")));
}

void dlgConnectionProfiles::loadCustomProfile(const QString& profileName) const
{
    auto pItem = new QListWidgetItem();
    pItem->setData(csmNameRole, profileName);
    pItem->setData(Qt::AccessibleTextRole, item_profile_accessName.arg(profileName));
    pItem->setData(Qt::AccessibleDescriptionRole, item_profile_accessDesc);

    setCustomIcon(profileName, pItem);
    auto description = getDescription(profileName, 0, profileName);
    if (!description.isEmpty()) {
        pItem->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
    }
    profiles_tree_widget->addItem(pItem);
}

void dlgConnectionProfiles::setCustomIcon(const QString& profileName, QListWidgetItem* profile) const {
    auto profileIconPath = mudlet::getMudletPath(mudlet::profileDataItemPath, profileName, QStringLiteral("profileicon"));
    auto icon = QIcon(QPixmap(profileIconPath).scaled(QSize(120, 30), Qt::IgnoreAspectRatio, Qt::SmoothTransformation).copy());
    profile->setIcon(icon);
}

// When a profile is renamed, migrate password storage to the new profile
void dlgConnectionProfiles::migrateSecuredPassword(const QString &oldProfile, const QString &newProfile)
{
    const auto& password = character_password_entry->text().trimmed();

    deleteSecurePassword(oldProfile);
    writeSecurePassword(newProfile, password);
}

template <typename L>
void dlgConnectionProfiles::loadSecuredPassword(const QString &profile, L callback)
{
    // character_password_entry

    auto *job = new QKeychain::ReadPasswordJob(QStringLiteral("Mudlet profile"));
    job->setAutoDelete(false);
    job->setInsecureFallback(false);

    job->setKey(profile);

    connect(job, &QKeychain::ReadPasswordJob::finished, this, [=](QKeychain::Job* job) {
        if (job->error()) {
            const auto error = job->errorString();
            if (error != QStringLiteral("Entry not found") && error != QStringLiteral("No match")) {
            qDebug() << "dlgConnectionProfiles::loadSecuredPassword ERROR: couldn't retrieve secure password for" << profile << ", error is:" << error;
            }
        }

        auto readJob = static_cast<QKeychain::ReadPasswordJob*>(job);
        callback(readJob->textData());

        job->deleteLater();
    });

    job->start();
}

void dlgConnectionProfiles::generateCustomProfile(int i, const QString& profileName) const
{
    auto pItem = new QListWidgetItem();
    pItem->setData(csmNameRole, profileName);
    pItem->setData(Qt::AccessibleTextRole, item_profile_accessName.arg(profileName));
    pItem->setData(Qt::AccessibleDescriptionRole, item_profile_accessDesc);

    profiles_tree_widget->addItem(pItem);
    QPixmap background(120, 30);
    background.fill(Qt::transparent);
    uint hash = qHash(profileName);
    QLinearGradient shade(0, 0, 120, 30);
    quint8 i1 = hash % 255;
    quint8 i2 = (hash + i) % 255;
    quint8 i3 = (i * hash) % 255;
    quint8 i4 = (3 * hash) % 255;
    quint8 i5 = (hash) % 255;
    quint8 i6 = (hash / (i + 2)) % 255; // In the other place where this is used i might be -1 or 0
    shade.setColorAt(1, QColor(i1, i2, i3, 255));
    shade.setColorAt(0, QColor(i4, i5, i6, 255));
    QPainter pt(&background);
    pt.setCompositionMode(QPainter::CompositionMode_SourceOver);
    pt.fillRect(QRect(0, 0, 120, 30), shade);
    QPixmap pg(QStringLiteral(":/icons/mudlet_main_32px.png"));
    pt.drawPixmap(QRect(5, 5, 20, 20), pg);

    QFont _font;
    QImage _pm(90, 30, QImage::Format_ARGB32_Premultiplied);
    QPainter _pt(&_pm);
    _pt.setCompositionMode(QPainter::CompositionMode_SourceOver);
    for (int fs = 30; fs > 1; fs--) {
        _pt.eraseRect(QRect(0, 0, 90, 30));
        _pt.fillRect(QRect(0, 0, 90, 30), QColor(255, 0, 0, 10));
        _font = QFont(QStringLiteral("DejaVu Sans"), fs, QFont::Normal);
        _pt.setFont(_font);
        QRect _r;
        if ((i1 + i2 + i3 + i4 + i5 + i6) / 6 < 100) {
            _pt.setPen(QColor(Qt::white));
        } else {
            _pt.setPen(QColor(Qt::black));
        }
        _pt.drawText(QRect(0, 0, 90, 30), Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextWordWrap, profileName, &_r);
        if (_r.width() <= 90 && _r.height() <= 30) {
            break;
        }
    }
    pt.setFont(_font);
    QRect _r;
    if ((i1 + i2 + i3 + i4 + i5 + i6) / 6 < 100) {
        pt.setPen(QColor(Qt::white));
    } else {
        pt.setPen(QColor(Qt::black));
    }
    pt.drawText(QRect(30, 0, 90, 30), Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextWordWrap, profileName, &_r);
    pItem->setIcon(QIcon(background));
}

void dlgConnectionProfiles::slot_profile_menu(QPoint pos)
{
    QPoint globalPos = profiles_tree_widget->mapToGlobal(pos);
    auto profileName = profiles_tree_widget->currentItem()->text();

    QMenu menu;
    if (hasCustomIcon(profileName)) {
        menu.addAction(tr("Reset icon", "Reset the custom picture for this profile in the connection dialog and show the default one instead"), this, &dlgConnectionProfiles::slot_reset_custom_icon);
    } else {
        menu.addAction(QIcon(":/icons/mudlet_main_16px.png"), tr("Set custom icon", "Set a custom picture to show for the profile in the connection dialog"), this,  &dlgConnectionProfiles::slot_set_custom_icon);
    }

    menu.exec(globalPos);
}

void dlgConnectionProfiles::slot_set_custom_icon()
{
    auto profileName = profiles_tree_widget->currentItem()->text();

    QString imageLocation = QFileDialog::getOpenFileName(this, tr("Select custom image for profile (should be 120x30)"),
                                                    QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
                                                    tr("Images (%1)").arg(QStringLiteral("*.png *.gif *.jpg")));
    if (imageLocation.isEmpty()) {
        return;
    }

    bool success = mudlet::self()->setProfileIcon(profileName, imageLocation).first;
    if (!success) {
        return;
    }

    auto icon = QIcon(QPixmap(imageLocation).scaled(QSize(120, 30), Qt::IgnoreAspectRatio, Qt::SmoothTransformation).copy());
    profiles_tree_widget->currentItem()->setIcon(icon);
}

void dlgConnectionProfiles::slot_reset_custom_icon()
{
    auto profileName = profiles_tree_widget->currentItem()->text();

    bool success = mudlet::self()->resetProfileIcon(profileName).first;
    if (!success) {
        return;
    }

    auto currentRow = profiles_tree_widget->currentRow();
    fillout_form();
    profiles_tree_widget->setCurrentRow(currentRow);
}

void dlgConnectionProfiles::slot_password_saved(QKeychain::Job *job)
{
    if (job->error()) {
        qWarning() << "dlgConnectionProfiles::slot_password_saved ERROR: couldn't save password for" << job->property("profile") << "; error was:" << job->errorString();
    }

    job->deleteLater();
}

void dlgConnectionProfiles::slot_password_deleted(QKeychain::Job *job)
{
    if (job->error()) {
        qWarning() << "dlgConnectionProfiles::slot_password_deleted ERROR: couldn't delete password for" << job->property("profile") << "; error was:" << job->errorString();
    }

    job->deleteLater();
}

void dlgConnectionProfiles::slot_cancel()
{
    // QDialog::Rejected is the enum value (= 0) return value for a "cancelled"
    // outcome...
    QDialog::done(QDialog::Rejected);
}

void dlgConnectionProfiles::slot_copy_profile()
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
    QDir dir(mudlet::getMudletPath(mudlet::profileHomePath, oldname));
    if (!dir.exists()) {
        mCopyingProfile = false;
        return;
    }

    copyFolder(mudlet::getMudletPath(mudlet::profileHomePath, oldname),
               mudlet::getMudletPath(mudlet::profileHomePath, profile_name));
    mProfileList << profile_name;
    slot_item_clicked(pItem);
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
}

void dlgConnectionProfiles::slot_copy_profilesettings_only()
{
    QString profile_name;
    QString oldname;
    QListWidgetItem* pItem;
    if (!copyProfileWidget(profile_name, oldname, pItem)) {
        return;
    }

    QDir newProfileDir(mudlet::getMudletPath(mudlet::profileHomePath, profile_name));
    newProfileDir.mkpath(newProfileDir.path());
    if (!newProfileDir.exists()) {
        return;
    }

    // copy relevant profile files
    for (QString file : {"url", "port", "password", "login", "description"}) {
        auto filePath = QStringLiteral("%1/%2").arg(mudlet::getMudletPath(mudlet::profileHomePath, oldname), file);
        auto newFilePath = QStringLiteral("%1/%2").arg(mudlet::getMudletPath(mudlet::profileHomePath, profile_name), file);
        QFile::copy(filePath, newFilePath);
    }

    copyProfileSettingsOnly(oldname, profile_name);

    mProfileList << profile_name;
    slot_item_clicked(pItem);
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

    pItem = new QListWidgetItem();
    if (!pItem) {
        return false;
    }
    pItem->setData(csmNameRole, profile_name);
    pItem->setData(Qt::AccessibleTextRole, item_profile_accessName.arg(profile_name));
    pItem->setData(Qt::AccessibleDescriptionRole, item_profile_accessDesc);

    // add the new widget in
    profiles_tree_widget->setSelectionMode(QAbstractItemView::SingleSelection);
    profiles_tree_widget->addItem(pItem);
    profiles_tree_widget->setItemSelected(profiles_tree_widget->currentItem(), false); // Unselect previous item
    profiles_tree_widget->setCurrentItem(pItem);
    profiles_tree_widget->setItemSelected(pItem, true);

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
    QDir oldProfiledir(mudlet::getMudletPath(mudlet::profileXmlFilesPath, oldname));
    QDir newProfiledir(mudlet::getMudletPath(mudlet::profileXmlFilesPath, newname));
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
    pugi::xml_parse_result result = oldProfile.load_file(copySettingsFrom.toUtf8().constData());
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
    pugi::xml_node hostPackage = hostPackageResults.first().node();
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
    QFile file(newProfiledir.absoluteFilePath(QStringLiteral("Copied profile (settings only).xml")));
    if (!file.open(QFile::WriteOnly)) {
        qDebug() << "dlgConnectionProfiles::copyProfileSettingsOnly ERROR - couldn't create new profile file:" << file.fileName() << "-" << file.errorString();
        return;
    }

    std::stringstream saveStringStream(std::ios::out);
    newProfileXml.save(saveStringStream);
    std::string output(saveStringStream.str());
    file.write(output.data());
    file.close();
}

void dlgConnectionProfiles::slot_load()
{
    loadProfile(false);
    QDialog::accept();
}
void dlgConnectionProfiles::slot_connectToServer()
{
    loadProfile(true);
}
void dlgConnectionProfiles::loadProfile(bool alsoConnect)
{
    QString profile_name = profile_name_entry->text().trimmed();

    if (profile_name.isEmpty()) {
        return;
    }

    HostManager & hostManager = mudlet::self()->getHostManager();
    Host* pHost = hostManager.getHost(profile_name);
    if (pHost) {
        if (alsoConnect) {
            pHost->mTelnet.connectIt(pHost->getUrl(), pHost->getPort());
        }
        QDialog::accept();
        return;
    }
    // load an old profile if there is any
    // PLACEMARKER: Host creation (1) - normal case
    if (hostManager.addHost(profile_name, port_entry->text().trimmed(), QString(), QString())) {
        pHost = hostManager.getHost(profile_name);
        if (!pHost) {
            return ;
        }
    } else {
        return;
    }

    QString folder(mudlet::getMudletPath(mudlet::profileXmlFilesPath, profile_name));
    QDir dir(folder);
    dir.setSorting(QDir::Time);
    QStringList entries = dir.entryList(QDir::Files, QDir::Time);
    bool preInstallPackages = false;
    mudlet::self()->hideMudletsVariables(pHost);
    if (entries.isEmpty()) {
        preInstallPackages = true;
    } else {
        QFile file(QStringLiteral("%1%2").arg(folder, profile_history->itemData(profile_history->currentIndex()).toString()));
        file.open(QFile::ReadOnly | QFile::Text);
        XMLimport importer(pHost);
        qDebug() << "[LOADING PROFILE]:" << file.fileName();
        importer.importPackage(&file, nullptr); // TODO: Missing false return value handler
        pHost->refreshPackageFonts();

        // Is this a new profile created through 'copy profile (settings only)'? install default packages into it
        if (entries.size() == 1 && entries.first() == QLatin1String("Copied profile (settings only).xml")) {
            preInstallPackages = true;
        }
    }

    // overwrite the generic profile with user supplied name, url and login information
    if (pHost) {
        pHost->setName(profile_name);

        if (host_name_entry->text().trimmed().size() > 0) {
            pHost->setUrl(host_name_entry->text().trimmed());
        } else {
            slot_update_url(pHost->getUrl());
        }

        if (port_entry->text().trimmed().size() > 0) {
            pHost->setPort(port_entry->text().trimmed().toInt());
        } else {
            slot_update_port(QString::number(pHost->getPort()));
        }

        pHost->mSslTsl = port_ssl_tsl->isChecked();

        if (character_password_entry->text().trimmed().size() > 0) {
            pHost->setPass(character_password_entry->text().trimmed());
        } else {
            slot_update_pass(pHost->getPass());
        }

        if (login_entry->text().trimmed().size() > 0) {
            pHost->setLogin(login_entry->text().trimmed());
        } else {
            slot_update_login(pHost->getLogin());
        }

        // This settings also need to be configured, note that the only time not to
        // save the setting is on profile loading:
        pHost->mTelnet.setEncoding(readProfileData(profile_name, QLatin1String("encoding")).toLatin1(), false);
        // Needed to ensure setting is correct on start-up:
        pHost->setWideAmbiguousEAsianGlyphs(pHost->getWideAmbiguousEAsianGlyphsControlState());
        pHost->setAutoReconnect(auto_reconnect->isChecked());

        // This also writes the value out to the profile's base directory:
        mudlet::self()->mDiscord.setApplicationID(pHost, mDiscordApplicationId);
    }

    if (preInstallPackages) {
        auto gameUrl = pHost->getUrl().toLower();
        const QHash<QString, QStringList> defaultScripts = {
                {QStringLiteral(":/run-lua-code-v4.xml"), {QStringLiteral("*")}},
                {QStringLiteral(":/echo.xml"), {QStringLiteral("*")}},
                {QStringLiteral(":/send-text-to-all-games.xml"), {QStringLiteral("*")}},
                {QStringLiteral(":/deleteOldProfiles.xml"), {QStringLiteral("*")}},
                {QStringLiteral(":/CF-loader.xml"), {QStringLiteral("carrionfields.net")}},
                {QStringLiteral(":/run-tests.xml"), {QStringLiteral("mudlet.org")}},
                {QStringLiteral(":/mudlet-mapper.xml"),
                 {QStringLiteral("aetolia.com"), QStringLiteral("achaea.com"), QStringLiteral("lusternia.com"), QStringLiteral("imperian.com"), QStringLiteral("starmourn.com")}},
        };

        QHashIterator<QString, QStringList> i(defaultScripts);
        while (i.hasNext()) {
            i.next();
            if (i.value().first() == QLatin1String("*") || i.value().contains(gameUrl)) {
                mudlet::self()->packagesToInstallList.append(i.key());
            }
        }

        if (!mudlet::self()->packagesToInstallList.contains(QStringLiteral(":/mudlet-mapper.xml"))) {
            mudlet::self()->packagesToInstallList.append(QStringLiteral(":/mudlet-lua/lua/generic-mapper/generic_mapper.xml"));
        }
    }

    emit mudlet::self()->signal_hostCreated(pHost, hostManager.getHostCount());
    emit signal_load_profile(profile_name, alsoConnect);
}

bool dlgConnectionProfiles::validateProfile()
{
    bool valid = true;

    notificationArea->hide();
    notificationAreaIconLabelWarning->hide();
    notificationAreaIconLabelError->hide();
    notificationAreaIconLabelInformation->hide();
    notificationAreaMessageBox->clear();

    QListWidgetItem* pItem = profiles_tree_widget->currentItem();

    if (pItem) {
        QString name = profile_name_entry->text().trimmed();
        const QString allowedChars = QStringLiteral(". _0123456789-#&aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ");

        for (int i = 0; i < name.size(); ++i) {
            if (!allowedChars.contains(name.at(i))) {
                notificationAreaIconLabelWarning->show();
                notificationAreaMessageBox->setText(
                    QStringLiteral("%1\n%2\n%3\n").arg(
                        notificationAreaMessageBox->text(),
                        tr("The %1 character is not permitted. Use one of the following:").arg(name.at(i)),
                        allowedChars));
                name.replace(name.at(i--), QString());
                profile_name_entry->setText(name);
                validName = false;
                valid = false;
                break;
            }
        }

        // see if there is an edit that already uses a similar name
        if (pItem->data(csmNameRole).toString() != name && mProfileList.contains(name)) {
            notificationAreaIconLabelError->show();
            notificationAreaMessageBox->setText(
                QStringLiteral("%1\n%2").arg(
                    notificationAreaMessageBox->text(),
                    tr("This profile name is already in use.")));
            validName = false;
            valid = false;
        }

        QString port = port_entry->text().trimmed();
        if (!port.isEmpty() && (port.indexOf(QRegularExpression(QStringLiteral("^\\d+$")), 0) == -1)) {
            QString val = port;
            val.chop(1);
            port_entry->setText(val);
            notificationAreaIconLabelError->show();
            notificationAreaMessageBox->setText(
                QStringLiteral("%1\n%2").arg(
                    notificationAreaMessageBox->text(),
                    tr("You have to enter a number. Other characters are not permitted.")));
            port_entry->setPalette(mErrorPalette);
            validPort = false;
            valid = false;
        }

        bool ok;
        int num = port.trimmed().toInt(&ok);
        if (!port.isEmpty() && (num > 65536 && ok)) {
            notificationAreaIconLabelError->show();
            notificationAreaMessageBox->setText(
                QStringLiteral("%1\n%2\n\n").arg(
                    notificationAreaMessageBox->text(),
                    tr("Port number must be above zero and below 65535.")));
            port_entry->setPalette(mErrorPalette);
            validPort = false;
            valid = false;
        }

#if defined(QT_NO_SSL)
        port_ssl_tsl->setEnabled(false);
        port_ssl_tsl->setToolTip(tr("Mudlet is not configured for secure connections."));
        if (port_ssl_tsl->isChecked()) {
            notificationAreaIconLabelError->show();
            notificationAreaMessageBox->setText(
                QStringLiteral("%1\n%2\n\n").arg(
                    notificationAreaMessageBox->text(),
                    tr("Mudlet is not configured for secure connections.")));
            port_ssl_tsl->setEnabled(true);
            validPort = false;
            valid = false;
        }
#else
        if (!QSslSocket::supportsSsl()) {
            if (port_ssl_tsl->isChecked()) {
                notificationAreaIconLabelError->show();
                notificationAreaMessageBox->setText(
                    QStringLiteral("%1\n%2\n\n").arg(
                        notificationAreaMessageBox->text(),
                        tr("Mudlet can not load support for secure connections.")));
                validPort = false;
                valid = false;
            }
        } else {
            port_ssl_tsl->setEnabled(true);
            port_ssl_tsl->setToolTip(QString());
        }
#endif
        QUrl check;
        QString url = host_name_entry->text().trimmed();
        check.setHost(url);
        if (!check.isValid()) {
            notificationAreaIconLabelError->show();
            notificationAreaMessageBox->setText(
                QStringLiteral("%1\n%2\n\n%3").arg(
                    notificationAreaMessageBox->text(),
                    tr("Please enter the URL or IP address of the Game server."),
                    check.errorString()));
            host_name_entry->setPalette(mErrorPalette);
            validUrl = false;
            valid = false;
        }

        if (url.indexOf(QRegularExpression(QStringLiteral("^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$")), 0) != -1) {
            if (port_ssl_tsl->isChecked()) {
                notificationAreaIconLabelError->show();
                notificationAreaMessageBox->setText(
                    QStringLiteral("%1\n%2\n\n%3").arg(
                        notificationAreaMessageBox->text(),
                        tr("SSL connections require the URL of the Game server."),
                        check.errorString()));
                host_name_entry->setPalette(mErrorPalette);
                validUrl = false;
                valid = false;
            }
        }

        if (valid) {
            port_entry->setPalette(mOKPalette);
            host_name_entry->setPalette(mOKPalette);
            notificationArea->hide();
            notificationAreaIconLabelWarning->hide();
            notificationAreaIconLabelError->hide();
            notificationAreaIconLabelInformation->hide();
            notificationAreaMessageBox->hide();
            validName = true;
            validPort = true;
            validUrl = true;

            if (offline_button) {
                offline_button->setEnabled(true);
                offline_button->setToolTip(tr("<p>Load profile without connecting.</p>"));
                offline_button->setAccessibleDescription(btn_load_enabled_accessDesc);
            }
            if (connect_button) {
                connect_button->setEnabled(true);
                connect_button->setToolTip(QString());
                connect_button->setAccessibleDescription(btn_connect_enabled_accessDesc);
            }
            return true;
        } else {
            notificationArea->show();
            notificationAreaMessageBox->show();
            if (offline_button) {
                offline_button->setEnabled(false);
                offline_button->setToolTip(tr("<p>Please set a valid profile name, game server address and the game port before loading.</p>"));
                offline_button->setAccessibleDescription(btn_connOrLoad_disabled_accessDesc);
            }
            if (connect_button) {
                connect_button->setEnabled(false);
                connect_button->setToolTip(tr("<p>Please set a valid profile name, game server address and the game port before connecting.</p>"));
                connect_button->setAccessibleDescription(btn_connOrLoad_disabled_accessDesc);
            }
            return false;
        }
    }
    return false;
}

// credit: http://www.qtcentre.org/archive/index.php/t-23469.html
void dlgConnectionProfiles::copyFolder(const QString& sourceFolder, const QString& destFolder)
{
    QDir sourceDir(sourceFolder);
    if (!sourceDir.exists()) {
        return;
    }

    QDir destDir(destFolder);
    if (!destDir.exists()) {
        destDir.mkdir(destFolder);
    }
    QStringList files = sourceDir.entryList(QDir::Files);
    for (int i = 0; i < files.count(); i++) {
        QString srcName = sourceFolder + QDir::separator() + files[i];
        QString destName = destFolder + QDir::separator() + files[i];
        QFile::copy(srcName, destName);
    }
    files.clear();
    files = sourceDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    for (int i = 0; i < files.count(); i++) {
        QString srcName = sourceFolder + QDir::separator() + files[i];
        QString destName = destFolder + QDir::separator() + files[i];
        copyFolder(srcName, destName);
    }
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
        // different QPixmaps for the QIcon for different states - so lets do it
        // directly:
        mpAction_revealPassword->setIcon(QIcon::fromTheme(QStringLiteral("password-show-on"), QIcon(QStringLiteral(":/icons/password-show-on.png"))));
        mpAction_revealPassword->setToolTip(tr("<p>Click to hide the password; it will also hide if another profile is selected.</p>"));
    } else {
        character_password_entry->setEchoMode(QLineEdit::Password);
        mpAction_revealPassword->setIcon(QIcon::fromTheme(QStringLiteral("password-show-off"), QIcon(QStringLiteral(":/icons/password-show-off.png"))));
        mpAction_revealPassword->setToolTip(tr("<p>Click to reveal the password for this profile.</p>"));
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
