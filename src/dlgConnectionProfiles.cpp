/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016-2018 by Stephen Lyons - slysven@virginmedia.com    *
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
#include "post_guard.h"

dlgConnectionProfiles::dlgConnectionProfiles(QWidget* parent)
: QDialog(parent)
, mProfileList(QStringList())
, connect_button(Q_NULLPTR)
, delete_profile_lineedit(Q_NULLPTR)
, delete_button(Q_NULLPTR)
, validName()
, validUrl()
, validPort()
, mCurrentQSettings(Q_NULLPTR)
{
    setupUi(this);

    // selection mode is important. if this is not set the selection behaviour is
    // undefined. this is an undocumented qt bug, as it only shows on certain OS
    // and certain architectures.

    profiles_tree_widget->setSelectionMode(QAbstractItemView::SingleSelection);

    QAbstractButton* abort = dialog_buttonbox->button(QDialogButtonBox::Cancel);
    connect_button = dialog_buttonbox->addButton(tr("Connect"), QDialogButtonBox::AcceptRole);

    // Test and set if needed mudlet::mIsIconShownOnDialogButtonBoxes - if there
    // is already a Qt provided icon on a predefined button, this is probably
    // the first and best place to test this as the "Cancel" button is a built-
    // in dialog button which will have an icon if the current system style
    // settings suggest it:
    mudlet::self()->mShowIconsOnDialogs = !abort->icon().isNull();

    auto Welcome_text_template = tr("<p><center><big><b>Welcome to Mudlet!</b><bold></center></p>"
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
        QIcon icon_connect(QIcon::fromTheme(QStringLiteral("dialog-ok-apply"), QIcon(QStringLiteral(":/icons/dialog-ok-apply.png"))));

        connect_button->setIcon(QIcon(QStringLiteral(":/icons/preferences-web-browser-cache.png")));
        new_profile_button->setIcon(icon_new);
        copy_profile_button->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/edit-copy.png"))));
        remove_profile_button->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete"), QIcon(QStringLiteral(":/icons/edit-delete.png"))));

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

    connect(connect_button, &QAbstractButton::clicked, this, &dlgConnectionProfiles::accept);
    connect(abort, &QAbstractButton::clicked, this, &dlgConnectionProfiles::slot_cancel);
    connect(new_profile_button, &QAbstractButton::clicked, this, &dlgConnectionProfiles::slot_addProfile);
    connect(copy_profile_button, &QAbstractButton::clicked, this, &dlgConnectionProfiles::slot_copy_profile);
    connect(remove_profile_button, &QAbstractButton::clicked, this, &dlgConnectionProfiles::slot_deleteProfile);
    connect(profile_name_entry, &QLineEdit::textEdited, this, &dlgConnectionProfiles::slot_update_name);
    connect(profile_name_entry, &QLineEdit::editingFinished, this, &dlgConnectionProfiles::slot_save_name);
    connect(host_name_entry, &QLineEdit::textChanged, this, &dlgConnectionProfiles::slot_update_url);
    connect(port_entry, &QLineEdit::textChanged, this, &dlgConnectionProfiles::slot_update_port);
    connect(port_ssl_tsl, &QCheckBox::stateChanged, this, &dlgConnectionProfiles::slot_update_SSL_TSL_port);
    connect(auto_reconnect, &QCheckBox::stateChanged, this, &dlgConnectionProfiles::slot_update_autoreconnect);
    connect(profiles_tree_widget, &QListWidget::currentItemChanged, this, &dlgConnectionProfiles::slot_item_clicked);
    connect(profiles_tree_widget, &QListWidget::itemDoubleClicked, this, &dlgConnectionProfiles::accept);

    connect(discord_optin_checkBox, &QCheckBox::stateChanged, this, &dlgConnectionProfiles::slot_update_discord_optin);

    notificationArea->hide();
    notificationAreaIconLabelWarning->hide();
    notificationAreaIconLabelError->hide();
    notificationAreaIconLabelInformation->hide();
    notificationAreaMessageBox->hide();

#if !defined(QT_NO_SSL)
    if (QSslSocket::supportsSsl()) {
        port_ssl_tsl->setEnabled(true);
    } else
#endif
    {
        port_ssl_tsl->setEnabled(false);
    }

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

void dlgConnectionProfiles::closeEvent(QCloseEvent* event)
{
    QListWidgetItem* pItem = profiles_tree_widget->currentItem();
    if (pItem) {
        QString profile = pItem->text();
        // Save the currently selected profile's data.
        writeProfileData(profile);
    }

    event->accept();
}

void dlgConnectionProfiles::slot_update_url(const QString& url)
{
    if (url.isEmpty()) {
        validUrl = false;
        connect_button->setDisabled(true);
        return;
    }

    if (validateProfile()) {
        QListWidgetItem* pItem = profiles_tree_widget->currentItem();
        if (!pItem) {
            return;
        }
        QString profile = pItem->text();
    }
}

void dlgConnectionProfiles::slot_update_autoreconnect(int state)
{
    QListWidgetItem* pItem = profiles_tree_widget->currentItem();
    if (!pItem) {
        return;
    }
    QString profile = pItem->text();
}

// This gets called when the QCheckBox that it is connect-ed to gets it's
// checked state set programatically AS WELL as when the user clicks on it:
void dlgConnectionProfiles::slot_update_discord_optin(int state)
{
    QListWidgetItem* pItem = profiles_tree_widget->currentItem();
    if (!pItem) {
        return;
    }
    QString profile = pItem->text();

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

void dlgConnectionProfiles::slot_update_port(const QString ignoreBlank)
{
    QString port = port_entry->text().trimmed();

    if (ignoreBlank.isEmpty()) {
        validPort = false;
        if (connect_button) {
            connect_button->setDisabled(true);
        }
        return;
    }

    if (validateProfile()) {
        QListWidgetItem* pItem = profiles_tree_widget->currentItem();
        if (!pItem) {
            return;
        }
        QString profile = pItem->text();
    }
}

void dlgConnectionProfiles::slot_update_SSL_TSL_port(int state)
{
    if (validateProfile()) {
        QListWidgetItem* pItem = profiles_tree_widget->currentItem();
        if (!pItem) {
            return;
        }
        QString profile = pItem->text();
    }
}

void dlgConnectionProfiles::slot_update_name(const QString newName)
{
    validateProfile();
}

void dlgConnectionProfiles::slot_save_name()
{
    QListWidgetItem* pItem = profiles_tree_widget->currentItem();
    QString newProfileName = profile_name_entry->text().trimmed();

    if (!validName || newProfileName.isEmpty()) {
        return;
    }

    if (pItem) {
        mCurrentQSettings = getProfileSettings(newProfileName);
        QString currentProfileEditName = pItem->text();
        int row = mProfileList.indexOf(currentProfileEditName); // This returns -1 if currentProfileEditName not present!
        if ((row >= 0) && (row < mProfileList.size())) {
            mProfileList[row] = newProfileName;
        } else {
            mProfileList << newProfileName;
        }

        // don't do anything if this was just a normal click, and not an edit of any sort
        if (currentProfileEditName == newProfileName) {
            return;
        }

        pItem->setText(newProfileName);

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
            auto newProfileIcon = profiles_tree_widget->findItems(newProfileName, Qt::MatchExactly).first();
            profiles_tree_widget->setCurrentItem(newProfileIcon);
            slot_item_clicked(newProfileIcon);
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
                /*if( QFontMetrics( _font ).boundingRect( s ).width() <= 80
                   && QFontMetrics( _font ).boundingRect( s ).height() <= 30 )*/
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
}

void dlgConnectionProfiles::slot_addProfile()
{
    writeProfileData();

    QString newname = tr("new profile name");
    auto pItem = new QListWidgetItem(newname);
    if (!pItem) {
        return;
    }

    profile_name_entry->setReadOnly(false);
    welcome_message->hide();

    requiredArea->show();
    informationalArea->show();
    optionalArea->show();


    profiles_tree_widget->setSelectionMode(QAbstractItemView::SingleSelection);
    profiles_tree_widget->addItem(pItem);

    // insert newest entry on top of the list as the general sorting
    // is always newest item first -> fillout->form() filters
    // this is more practical for the user as they use the same profile most of the time

    profiles_tree_widget->setItemSelected(profiles_tree_widget->currentItem(), false); // Unselect previous item
    profiles_tree_widget->setCurrentItem(pItem);
    profiles_tree_widget->setItemSelected(pItem, true);

    profile_name_entry->setFocus();
    profile_name_entry->selectAll();
    profile_name_entry->setReadOnly(false);
    host_name_entry->setReadOnly(false);
    port_entry->setReadOnly(false);

    validName = false;
    validUrl = false;
    validPort = false;
    connect_button->setDisabled(true);
}

// enables the deletion button once the correct text (profile name) is entered
void dlgConnectionProfiles::slot_deleteprofile_check(const QString text)
{
    QString profile = profiles_tree_widget->currentItem()->text();
    if (profile != text) {
        delete_button->setDisabled(true);
    } else {
        delete_button->setEnabled(true);
        delete_button->setFocus();
    }
}

// actually performs the deletion once the correct text has been entered
void dlgConnectionProfiles::slot_reallyDeleteProfile()
{
    QString profile = profiles_tree_widget->currentItem()->text();
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

    QString profile = profiles_tree_widget->currentItem()->text();

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
    delete_button->setDisabled(true);
    delete_profile_dialog->setWindowTitle(tr("Deleting '%1'").arg(profile));

    delete_profile_dialog->show();
    delete_profile_dialog->raise();
}

QPointer<QSettings> dlgConnectionProfiles::getProfileSettings(const QString& profile)
{
    return new QSettings(mudlet::getMudletPath(mudlet::profileDataItemPath, profile, QStringLiteral("profile.ini")), QSettings::IniFormat);
}

QString dlgConnectionProfiles::readProfileData(const QString& profile, const QString& item)
{
    // Read profile data from a config file in the INI format inside the
    // profile's home path.
    if (!mCurrentQSettings) {
        mCurrentQSettings = getProfileSettings(profile);
    }

    if (mCurrentQSettings->contains(item)) {
        return mCurrentQSettings->value(item).toString();
    } else {
        // For backwards compatibility's sake, if the INI file does not contain
        // the item, read the item from a separate item file in the profile's
        // data item path.
        QFile file(mudlet::getMudletPath(mudlet::profileDataItemPath, profile, item));

        bool success = file.open(QIODevice::ReadOnly);
        QString ret;
        if (success) {
            QDataStream ifs(&file);
            ifs >> ret;
            file.close();
        }

        return ret;
    }
}

// Stores profile data in an INI file within the profile's home path.
QPair<bool, QString> dlgConnectionProfiles::writeProfileData(const QString& profile)
{
    if (!mCurrentQSettings && !profile.isEmpty()) {
        mCurrentQSettings = getProfileSettings(profile);
    }

    if (!mCurrentQSettings) {
        return qMakePair(true, QString());
    }

    if (mCurrentQSettings) {
        mCurrentQSettings->setValue(QStringLiteral("url"), host_name_entry->text());
        mCurrentQSettings->setValue(QStringLiteral("port"), port_entry->text().trimmed());
        mCurrentQSettings->setValue(QStringLiteral("login"), login_entry->text());
        mCurrentQSettings->setValue(QStringLiteral("password"), character_password_entry->text());
        mCurrentQSettings->setValue(QStringLiteral("autologin"), QString::number(autologin_checkBox->checkState()));
        mCurrentQSettings->setValue(QStringLiteral("description"), mud_description_textedit->toPlainText());
        mCurrentQSettings->setValue(QStringLiteral("website"), website_entry->text());
        mCurrentQSettings->setValue(QStringLiteral("autoreconnect"), QString::number(auto_reconnect->checkState()));
        mCurrentQSettings->setValue(QStringLiteral("discordserveroptin"), QString::number(discord_optin_checkBox->checkState()));
        mCurrentQSettings->setValue(QStringLiteral("ssl_tsl"), QString::number(port_ssl_tsl->checkState()));
    }

    if (mCurrentQSettings->status() == QSettings::NoError) {
        return qMakePair(true, QString());
    } else {
        return qMakePair(false, QString::number(mCurrentQSettings->status()));
    }
}

// Use the URL so we can use the same descriptions for user generated copies of
// predefined MUDs - but also need the port number to disambiguate the 3K ones!
QString dlgConnectionProfiles::getDescription(const QString& hostUrl, const quint16 port, const QString& profile_name)
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
    } else if (mCurrentQSettings) {
        return readProfileData(profile_name, "description");
    } else {
        return QString();
    }
}

void dlgConnectionProfiles::slot_item_clicked(QListWidgetItem* pItem)
{
    if (!pItem) {
        return;
    }


    QString profile_name = pItem->text();

    profile_name_entry->setText(profile_name);

    QString profile = profile_name;

    // Save the previously selected profile's data before setting
    // mCurrentQSettings to the newly clicked profile's INI file.
    if (mCurrentQSettings) {
        writeProfileData();
    }

    // Set mCurrentQSettings to the currently selected profile's INI file.
    mCurrentQSettings = getProfileSettings(profile);

    QString host_url = readProfileData(profile, QStringLiteral("url"));
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
        if (profile_name == QStringLiteral("Reinos de Leyenda")) {
            host_url = QStringLiteral("reinosdeleyenda.es");
        }
    }
    host_name_entry->setText(host_url);


    QString host_port = readProfileData(profile, QStringLiteral("port"));
    QString val = readProfileData(profile, QStringLiteral("ssl_tsl"));
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
            host_port = QStringLiteral("7680");
        }
        if (profile_name == QStringLiteral("Reinos de Leyenda")) {
            //host_url = QStringLiteral("reinosdeleyenda.es");
            host_port = QStringLiteral("23");
            port_ssl_tsl->setChecked(false);
        }
    }

    port_entry->setText(host_port);

    val = readProfileData(profile, QStringLiteral("password"));
    character_password_entry->setText(val);

    val = readProfileData(profile, QStringLiteral("login"));
    login_entry->setText(val);

    val = readProfileData(profile, QStringLiteral("autologin"));
    if (val.toInt() == Qt::Checked) {
        autologin_checkBox->setChecked(true);
    } else {
        autologin_checkBox->setChecked(false);
    }

    val = readProfileData(profile, QStringLiteral("autoreconnect"));
    if (!val.isEmpty() && val.toInt() == Qt::Checked) {
        auto_reconnect->setChecked(true);
    } else {
        auto_reconnect->setChecked(false);
    }

    mDiscordApplicationId = readProfileData(profile, QStringLiteral("discordApplicationId"));

    // val will be null if this is the first time the profile has been read
    // since an update to a Mudlet version supporting Discord - so a toint()
    // will return 0 - which just happens to be Qt::Unchecked() but lets not
    // rely on that...
    val = readProfileData(profile, QStringLiteral("discordserveroptin"));
    if ((!val.isEmpty()) && val.toInt() == Qt::Checked) {
        discord_optin_checkBox->setChecked(true);
    } else {
        discord_optin_checkBox->setChecked(false);
    }

    updateDiscordStatus();

    mud_description_textedit->setPlainText(getDescription(host_url, host_port.toUInt(), profile_name));

    val = readProfileData(profile, QStringLiteral("website"));
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

    for (const auto entry : entries) {
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

            //readableEntries << datetime.toString(Qt::SystemLocaleLongDate);
            //profile_history->addItem(datetime.toString(Qt::SystemLocaleShortDate), QVariant(entry));
            profile_history->addItem(datetime.toString(Qt::SystemLocaleLongDate), QVariant(entry));
        } else if (entry == QLatin1String("autosave.xml")) {
            QFileInfo fileInfo(dir, entry);
            auto lastModified = fileInfo.lastModified();
            profile_history->addItem(QIcon::fromTheme(QStringLiteral("document-save"), QIcon(QStringLiteral(":/icons/document-save.png"))), lastModified.toString(Qt::SystemLocaleLongDate), QVariant(entry));
        } else {
            profile_history->addItem(entry, QVariant(entry)); // if it has a custom name, use it as it is
        }
    }

    profile_history->setEnabled(static_cast<bool>(profile_history->count()));

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
        notificationAreaMessageBox->setText(tr("This profile is currently loaded - you will need to disconnect before changing the connection parameters."));
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

        if (notificationAreaMessageBox->text() == tr("This profile is currently loaded - you will need to disconnect before changing the connection parameters.")) {
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
        discord_optin_checkBox->setDisabled(true);
        discord_optin_checkBox->setChecked(false);
        discord_optin_checkBox->setToolTip(tr("Discord integration not available on this platform"));
    } else if (mDiscordApplicationId.isEmpty() && !mudlet::self()->mDiscord.gameIntegrationSupported(host_name_entry->text().trimmed()).first) {
        // Disable discord support if it is not recognised by name and a
        // Application Id has not been previously entered:
        discord_optin_checkBox->setDisabled(true);
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

    mProfileList = QDir(mudlet::getMudletPath(mudlet::profilesPath)).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    // if only the default_host is present it means no profiles have yet been created
    if (mProfileList.isEmpty() || (mProfileList.size() == 1 && mProfileList.at(0) == QStringLiteral("default_host"))) {
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
    QListWidgetItem* pM;
    QIcon mi;

    auto &settings = *mudlet::self()->mpSettings;
    auto deletedDefaultMuds = settings.value(QStringLiteral("deletedDefaultMuds"), QStringList()).toStringList();

    mudServer = QStringLiteral("Avalon.de");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pM = new QListWidgetItem(mudServer);
        pM->setFont(font);
        pM->setForeground(QColor(Qt::white));
        profiles_tree_widget->addItem(pM);
        QPixmap p(QStringLiteral(":/icons/avalon.png"));
        mi = QIcon(p.scaled(QSize(120, 30)));
        pM->setIcon(mi);
        description = getDescription(QStringLiteral("avalon.mud.de"), 0, mudServer);
        if (!description.isEmpty()) {
            pM->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("Achaea");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pM = new QListWidgetItem(mudServer);
        pM->setFont(font);
        pM->setForeground(QColor(Qt::white));
        profiles_tree_widget->addItem(pM);
        mi = QIcon(QStringLiteral(":/icons/achaea_120_30.png"));
        pM->setIcon(mi);
        description = getDescription(QStringLiteral("achaea.com"), 0, mudServer);
        if (!description.isEmpty()) {
            pM->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("3Kingdoms");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pM = new QListWidgetItem(mudServer);
        pM->setFont(font);
        pM->setForeground(QColor(Qt::white));
        profiles_tree_widget->addItem(pM);
        QPixmap pd(QStringLiteral(":/icons/3klogo.png"));
        QPixmap pd1 = pd.scaled(QSize(120, 30), Qt::IgnoreAspectRatio, Qt::SmoothTransformation).copy();
        QIcon mi5(pd1);
        pM->setIcon(mi5);
        description = getDescription(QStringLiteral("3k.org"), 3000, mudServer);
        if (!description.isEmpty()) {
            pM->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("3Scapes");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pM = new QListWidgetItem(mudServer);
        pM->setFont(font);
        pM->setForeground(QColor(Qt::white));
        profiles_tree_widget->addItem(pM);
        QPixmap pc(QStringLiteral(":/icons/3slogo.png"));
        QPixmap pc1 = pc.scaled(QSize(120, 30), Qt::IgnoreAspectRatio, Qt::SmoothTransformation).copy();
        QIcon mi4(pc1);
        pM->setIcon(mi4);
        description = getDescription(QStringLiteral("3k.org"), 3200, mudServer);
        if (!description.isEmpty()) {
            pM->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("Lusternia");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pM = new QListWidgetItem(mudServer);
        pM->setFont(font);
        pM->setForeground(QColor(Qt::white));
        profiles_tree_widget->addItem(pM);
        mi = QIcon(QStringLiteral(":/icons/lusternia_120_30.png"));
        pM->setIcon(mi);
        description = getDescription(QStringLiteral("lusternia.com"), 0, mudServer);
        if (!description.isEmpty()) {
            pM->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("BatMUD");
    if (!deletedDefaultMuds.contains(mudServer)) {
        QPixmap pb(QStringLiteral(":/icons/batmud_mud.png"));
        QPixmap pb1 = pb.scaled(QSize(120, 30)).copy();
        mi = QIcon(pb1);
        pM = new QListWidgetItem(mudServer);
        pM->setFont(font);
        pM->setForeground(QColor(Qt::white));
        profiles_tree_widget->addItem(pM);
        pM->setIcon(mi);
        description = getDescription(QStringLiteral("batmud.bat.org"), 0, mudServer);
        if (!description.isEmpty()) {
            pM->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("God Wars II");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pM = new QListWidgetItem(mudServer);
        pM->setFont(font);
        pM->setForeground(QColor(Qt::white));
        profiles_tree_widget->addItem(pM);
        mi = QIcon(QStringLiteral(":/icons/gw2.png"));
        pM->setIcon(mi);
        description = getDescription(QStringLiteral("godwars2.org"), 0, mudServer);
        if (!description.isEmpty()) {
            pM->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("Slothmud");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pM = new QListWidgetItem(mudServer);
        pM->setFont(font);
        pM->setForeground(QColor(Qt::white));
        profiles_tree_widget->addItem(pM);
        mi = QIcon(QStringLiteral(":/icons/Slothmud.png"));
        pM->setIcon(mi);
        description = getDescription(QStringLiteral("slothmud.org"), 0, mudServer);
        if (!description.isEmpty()) {
            pM->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("Aardwolf");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pM = new QListWidgetItem(mudServer);
        pM->setFont(font);
        pM->setForeground(QColor(Qt::white));
        profiles_tree_widget->addItem(pM);
        mi = QIcon(QStringLiteral(":/icons/aardwolf_mud.png"));
        pM->setIcon(mi);
        description = getDescription(QStringLiteral("aardmud.org"), 0, mudServer);
        if (!description.isEmpty()) {
            pM->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("Materia Magica");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pM = new QListWidgetItem(mudServer);
        pM->setFont(font);
        pM->setForeground(QColor(Qt::white));
        profiles_tree_widget->addItem(pM);
        mi = QIcon(QStringLiteral(":/materiaMagicaIcon"));
        pM->setIcon(mi);
        description = getDescription(QStringLiteral("materiamagica.com"), 0, mudServer);
        if (!description.isEmpty()) {
            pM->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("Realms of Despair");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pM = new QListWidgetItem(mudServer);
        pM->setFont(font);
        pM->setForeground(QColor(Qt::white));
        profiles_tree_widget->addItem(pM);
        mi = QIcon(QStringLiteral(":/icons/120x30RoDLogo.png"));
        pM->setIcon(mi);
        description = getDescription(QStringLiteral("realmsofdespair.com"), 0, mudServer);
        if (!description.isEmpty()) {
            pM->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("ZombieMUD");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pM = new QListWidgetItem(mudServer);
        pM->setFont(font);
        pM->setForeground(QColor(Qt::white));
        profiles_tree_widget->addItem(pM);
        mi = QIcon(QStringLiteral(":/icons/zombiemud.png"));
        pM->setIcon(mi);
        description = getDescription(QStringLiteral("zombiemud.org"), 0, mudServer);
        if (!description.isEmpty()) {
            pM->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("Aetolia");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pM = new QListWidgetItem(mudServer);
        pM->setFont(font);
        pM->setForeground(QColor(Qt::white));
        profiles_tree_widget->addItem(pM);
        mi = QIcon(QStringLiteral(":/icons/aetolia_120_30.png"));
        pM->setIcon(mi);
        description = getDescription(QStringLiteral("aetolia.com"), 0, mudServer);
        if (!description.isEmpty()) {
            pM->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("Imperian");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pM = new QListWidgetItem(mudServer);
        pM->setFont(font);
        pM->setForeground(QColor(Qt::white));
        profiles_tree_widget->addItem(pM);
        mi = QIcon(QStringLiteral(":/icons/imperian_120_30.png"));
        pM->setIcon(mi);
        description = getDescription(QStringLiteral("imperian.com"), 0, mudServer);
        if (!description.isEmpty()) {
            pM->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("WoTMUD");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pM = new QListWidgetItem(mudServer);
        pM->setFont(font);
        pM->setForeground(QColor(Qt::white));
        profiles_tree_widget->addItem(pM);
        mi = QIcon(QPixmap(QStringLiteral(":/icons/wotmudicon.png")).scaled(QSize(120, 30), Qt::IgnoreAspectRatio,
                                                                            Qt::SmoothTransformation).copy());
        pM->setIcon(mi);
        description = getDescription(QStringLiteral("game.wotmud.org"), 0, mudServer);
        if (!description.isEmpty()) {
            pM->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("Midnight Sun 2");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pM = new QListWidgetItem(mudServer);
        pM->setFont(font);
        pM->setForeground(QColor(Qt::white));
        profiles_tree_widget->addItem(pM);
        mi = QIcon(QPixmap(QStringLiteral(":/icons/midnightsun2.png")).scaled(QSize(120, 30), Qt::IgnoreAspectRatio,
                                                                              Qt::SmoothTransformation).copy());
        pM->setIcon(mi);
        description = getDescription(QStringLiteral("midnightsun2.org"), 0, mudServer);
        if (!description.isEmpty()) {
            pM->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("Luminari");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pM = new QListWidgetItem(mudServer);
        pM->setFont(font);
        pM->setForeground(QColor(Qt::white));
        profiles_tree_widget->addItem(pM);
        mi = QIcon(QPixmap(QStringLiteral(":/icons/luminari_icon.png")).scaled(QSize(120, 30), Qt::IgnoreAspectRatio,
                                                                              Qt::SmoothTransformation).copy());
        pM->setIcon(mi);
        description = getDescription(QStringLiteral("luminarimud.com"), 0, mudServer);
        if (!description.isEmpty()) {
            pM->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("StickMUD");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pM = new QListWidgetItem(mudServer);
        pM->setFont(font);
        pM->setForeground(QColor(Qt::white));
        profiles_tree_widget->addItem(pM);
        mi = QIcon(QPixmap(QStringLiteral(":/icons/stickmud_icon.jpg")).scaled(QSize(120, 30), Qt::IgnoreAspectRatio,
                                                                              Qt::SmoothTransformation).copy());
        pM->setIcon(mi);
        description = getDescription(QStringLiteral("stickmud.com"), 0, mudServer);
        if (!description.isEmpty()) {
            pM->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    mudServer = QStringLiteral("Reinos de Leyenda");
    if (!deletedDefaultMuds.contains(mudServer)) {
        pM = new QListWidgetItem(mudServer);
        pM->setFont(font);
        pM->setForeground(QColor(Qt::white));
        profiles_tree_widget->addItem(pM);
        mi = QIcon(QPixmap(QStringLiteral(":/icons/reinosdeleyenda_mud.png")).scaled(QSize(120, 30), Qt::IgnoreAspectRatio, Qt::SmoothTransformation).copy());
        pM->setIcon(mi);
        description = getDescription(QStringLiteral("reinosdeleyenda.es"), 0, mudServer);
        if (!description.isEmpty()) {
            pM->setToolTip(QLatin1String("<html><head/><body><p>") % description % QLatin1String("</p></body></html>"));
        }
    }

    for (int i = 0; i < mProfileList.size(); i++) {
        QString s = mProfileList.at(i);
        if (s.isEmpty()) {
            continue;
        }

        auto pItem = new QListWidgetItem(mProfileList.at(i));

        // mProfileList is derived from a filesystem directory, but MacOS is not
        // necessarily case preserving for file names so any tests on them
        // should be case insensitive
        // skip creating icons for default MUDs as they are already created above
        if ((!mProfileList.at(i).compare(QStringLiteral("Avalon.de"), Qt::CaseInsensitive)) || (!mProfileList.at(i).compare(QStringLiteral("BatMUD"), Qt::CaseInsensitive))
            || (!mProfileList.at(i).compare(QStringLiteral("Materia Magica"), Qt::CaseInsensitive))
            || (!mProfileList.at(i).compare(QStringLiteral("Aardwolf"), Qt::CaseInsensitive))
            || (!mProfileList.at(i).compare(QStringLiteral("Achaea"), Qt::CaseInsensitive))
            || (!mProfileList.at(i).compare(QStringLiteral("Aetolia"), Qt::CaseInsensitive))
            || (!mProfileList.at(i).compare(QStringLiteral("Lusternia"), Qt::CaseInsensitive))
            || (!mProfileList.at(i).compare(QStringLiteral("Imperian"), Qt::CaseInsensitive))
            || (!mProfileList.at(i).compare(QStringLiteral("Realms of Despair"), Qt::CaseInsensitive))
            || (!mProfileList.at(i).compare(QStringLiteral("ZombieMUD"), Qt::CaseInsensitive))
            || (!mProfileList.at(i).compare(QStringLiteral("3Scapes"), Qt::CaseInsensitive))
            || (!mProfileList.at(i).compare(QStringLiteral("3Kingdoms"), Qt::CaseInsensitive))
            || (!mProfileList.at(i).compare(QStringLiteral("Midnight Sun 2"), Qt::CaseInsensitive))
            || (!mProfileList.at(i).compare(QStringLiteral("Luminari"), Qt::CaseInsensitive))
            || (!mProfileList.at(i).compare(QStringLiteral("StickMUD"), Qt::CaseInsensitive))
            || (!mProfileList.at(i).compare(QStringLiteral("Reinos de Leyenda"), Qt::CaseInsensitive))
            || (!mProfileList.at(i).compare(QStringLiteral("WoTMUD"), Qt::CaseInsensitive))) {
            delete pItem;
            continue;
        }

        pItem->setFont(font);
        pItem->setForeground(QColor(Qt::white));
        profiles_tree_widget->addItem(pItem);
        QPixmap pb(120, 30);
        pb.fill(Qt::transparent);
        uint hash = qHash(mProfileList.at(i));
        QLinearGradient shade(0, 0, 120, 30);
        quint8 i1 = hash % 255;
        quint8 i2 = (hash + i) % 255;
        quint8 i3 = (i * hash) % 255;
        quint8 i4 = (3 * hash) % 255;
        quint8 i5 = (hash) % 255;
        quint8 i6 = (hash / (i + 2)) % 255; // In the other place where this is used i might be -1 or 0
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
            /*if( QFontMetrics( _font ).boundingRect( s ).width() <= 80
			   && QFontMetrics( _font ).boundingRect( s ).height() <= 30 )*/
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
        mi = QIcon(pb);
        pItem->setIcon(mi);
    }

    QDateTime test_date;
    QString toselectProfileName;
    int toselectRow = -1;

    for (int i = 0; i < profiles_tree_widget->count(); i++) {
        auto profile = profiles_tree_widget->item(i);
        auto profileName = profile->text();

        QDateTime profile_lastRead = QFileInfo(mudlet::getMudletPath(mudlet::profileXmlFilesPath, profileName)).lastModified();
        // Since Qt 5.x null QTimes and QDateTimes are invalid - and might not
        // work as expected - so test for validity of the test_date value as well
        if ((!test_date.isValid()) || profile_lastRead > test_date) {
            test_date = profile_lastRead;
            toselectProfileName = profileName;
            toselectRow = i;
        }
    }

    if (toselectRow != -1 && toselectProfileName == QStringLiteral("default_host") && profiles_tree_widget->count() > 1) {
        // if the last profile read is default_host, it means the user hasn't created
        // any profiles yet since default_host profile cannot actually be used. In this case,
        // select a random pre-defined profile to give all MUDs a fair go

        // make sure not to actually select the default_host though
        auto default_host_row = toselectRow;
        while (toselectRow == default_host_row) {
            toselectRow = qrand() % profiles_tree_widget->count();
        }
    }

    profiles_tree_widget->setCurrentRow(toselectRow);

    updateDiscordStatus();
}

void dlgConnectionProfiles::slot_cancel()
{
    // QDialog::Rejected is the enum value (= 0) return value for a "cancelled"
    // outcome...
    QListWidgetItem* pItem = profiles_tree_widget->currentItem();
    if (pItem) {
        QString profile = pItem->text();
        // Save the currently selected profile's data.
        writeProfileData(profile);
    }
    QDialog::done(QDialog::Rejected);
}

void dlgConnectionProfiles::slot_copy_profile()
{
    QString profile_name = profile_name_entry->text().trimmed();
    QString oldname = profile_name;

    if (profile_name.isEmpty()) {
        return;
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

    auto pItem = new QListWidgetItem(profile_name);
    if (!pItem) {
        return;
    }

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

    // copy the folder on-disk
    QDir dir(mudlet::getMudletPath(mudlet::profileHomePath, oldname));
    if (!dir.exists()) {
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
}

void dlgConnectionProfiles::slot_connectToServer()
{
    QString profile_name = profile_name_entry->text().trimmed();

    if (profile_name.isEmpty()) {
        return;
    }

    HostManager & hostManager = mudlet::self()->getHostManager();
    Host* pHost = hostManager.getHost(profile_name);
    if (pHost) {
        pHost->mTelnet.connectIt(pHost->getUrl(), pHost->getPort());
        QDialog::accept();
        return;
    }
    // load an old profile if there is any
    // PLACEMARKER: Host creation (3) - normal case
    if (hostManager.addHost(profile_name, port_entry->text().trimmed(), QString(), QString())) {
        pHost = hostManager.getHost(profile_name);
        if (!pHost) {
            return;
        }
    } else {
        return;
    }

    QString folder(mudlet::getMudletPath(mudlet::profileXmlFilesPath, profile_name));
    QDir dir(folder);
    dir.setSorting(QDir::Time);
    QStringList entries = dir.entryList(QDir::Files, QDir::Time);
    bool needsGenericPackagesInstall = false;
    LuaInterface* lI = pHost->getLuaInterface();
    lI->getVars(true);
    if (!entries.isEmpty()) {
        QFile file(QStringLiteral("%1%2").arg(folder, profile_history->itemData(profile_history->currentIndex()).toString()));
        file.open(QFile::ReadOnly | QFile::Text);
        XMLimport importer(pHost);
        qDebug() << "[LOADING PROFILE]:" << file.fileName();
        importer.importPackage(&file, nullptr); // TODO: Missing false return value handler
        pHost->refreshPackageFonts();
        file.close();
    } else {
        needsGenericPackagesInstall = true;
    }

    // overwrite the generic profile with user supplied name, url and login information
    if (pHost) {
        pHost->setName(profile_name);
        mCurrentQSettings = getProfileSettings(profile_name);

        if (host_name_entry->text().trimmed().size() > 0) {
            pHost->setUrl(host_name_entry->text().trimmed());
        } else {
            mCurrentQSettings->setValue(QStringLiteral("url"), pHost->getUrl());
        }

        if (port_entry->text().trimmed().size() > 0) {
            pHost->setPort(port_entry->text().trimmed().toInt());
        } else {
            mCurrentQSettings->setValue(QStringLiteral("port"), QString::number(pHost->getPort()));
        }

        pHost->mSslTsl = port_ssl_tsl->isChecked();

        if (character_password_entry->text().trimmed().size() > 0) {
            pHost->setPass(character_password_entry->text().trimmed());
        } else {
            mCurrentQSettings->setValue(QStringLiteral("password"), pHost->getPass());
        }

        if (login_entry->text().trimmed().size() > 0) {
            pHost->setLogin(login_entry->text().trimmed());
        } else {
            mCurrentQSettings->setValue(QStringLiteral("login"), pHost->getLogin());
        }

        // This settings also need to be configured, note that the only time not to
        // save the setting is on profile loading:
        pHost->mTelnet.setEncoding(readProfileData(profile_name, QLatin1String("encoding")), false);
        // Needed to ensure setting is correct on start-up:
        pHost->setWideAmbiguousEAsianGlyphs(pHost->getWideAmbiguousEAsianGlyphsControlState());
        pHost->setAutoReconnect(auto_reconnect->isChecked());

        // This also writes the value out to the profile's base directory:
        mudlet::self()->mDiscord.setApplicationID(pHost, mDiscordApplicationId);
    }

    if (needsGenericPackagesInstall) {
        //install generic mapper script
        if (pHost->getUrl().contains(QStringLiteral("aetolia.com"), Qt::CaseInsensitive) || pHost->getUrl().contains(QStringLiteral("achaea.com"), Qt::CaseInsensitive)
            || pHost->getUrl().contains(QStringLiteral("lusternia.com"), Qt::CaseInsensitive) || pHost->getUrl().contains(QStringLiteral("imperian.com"), Qt::CaseInsensitive)) {
            mudlet::self()->packagesToInstallList.append(QStringLiteral(":/mudlet-mapper.xml"));
        } else if (pHost->getUrl().contains(QStringLiteral("3scapes.org"), Qt::CaseInsensitive) || pHost->getUrl().contains(QStringLiteral("3k.org"), Qt::CaseInsensitive)) {
            mudlet::self()->packagesToInstallList.append(QStringLiteral(":/3k-mapper.xml"));
        }

        mudlet::self()->packagesToInstallList.append(QStringLiteral(":/deleteOldProfiles.xml"));
        mudlet::self()->packagesToInstallList.append(QStringLiteral(":/echo.xml"));
        mudlet::self()->packagesToInstallList.append(QStringLiteral(":/run-lua-code-v4.xml"));
        mudlet::self()->packagesToInstallList.append(QStringLiteral(":/send-text-to-all-games.xml"));
    }

    emit mudlet::self()->signal_hostCreated(pHost, hostManager.getHostCount());
    emit signal_establish_connection(profile_name, 0);
}

bool dlgConnectionProfiles::validateProfile()
{
    bool valid = true;

    notificationArea->hide();
    notificationAreaIconLabelWarning->hide();
    notificationAreaIconLabelError->hide();
    notificationAreaIconLabelInformation->hide();
    notificationAreaMessageBox->setText(tr(""));

    QListWidgetItem* pItem = profiles_tree_widget->currentItem();

    if (pItem) {
        QString name = profile_name_entry->text().trimmed();
        const QString allowedChars = QStringLiteral(". _0123456789-#&aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ");

        for (int i = 0; i < name.size(); ++i) {
            if (!allowedChars.contains(name.at(i))) {
                notificationAreaIconLabelWarning->show();
                notificationAreaMessageBox->setText(
                        QString("%1\n%2").arg(notificationAreaMessageBox->text(), tr("The %1 character is not permitted. Use one of the following:\n\"%2\".\n").arg(name.at(i), allowedChars)));
                name.replace(name.at(i--), QString());
                profile_name_entry->setText(name);
                validName = false;
                valid = false;
                break;
            }
        }

        // see if there is an edit that already uses a similar name
        if (pItem->text() != name && mProfileList.contains(name)) {
            notificationAreaIconLabelError->show();
            notificationAreaMessageBox->setText(QString("%1\n%2").arg(notificationAreaMessageBox->text(), tr("This profile name is already in use.")));
            validName = false;
            valid = false;
        } else {
            pItem->setText(name);
        }

        QString port = port_entry->text().trimmed();
        if (!port.isEmpty() && (port.indexOf(QRegularExpression(QStringLiteral("^\\d+$")), 0) == -1)) {
            QString val = port;
            val.chop(1);
            port_entry->setText(val);
            notificationAreaIconLabelError->show();
            notificationAreaMessageBox->setText(QString("%1\n%2").arg(notificationAreaMessageBox->text(), tr("You have to enter a number. Other characters are not permitted.")));
            port_entry->setPalette(mErrorPalette);
            validPort = false;
            valid = false;
        }

        bool ok;
        int num = port.trimmed().toInt(&ok);
        if (!port.isEmpty() && (num > 65536 && ok)) {
            notificationAreaIconLabelError->show();
            notificationAreaMessageBox->setText(QString("%1\n%2").arg(notificationAreaMessageBox->text(), tr("Port number must be above zero and below 65535.\r\n")));
            port_entry->setPalette(mErrorPalette);
            validPort = false;
            valid = false;
        }

#if defined(QT_NO_SSL)
        port_ssl_tsl->setEnabled(false);
        port_ssl_tsl->setToolTip(tr("Mudlet is not configured for secure connections."));
        if (port_ssl_tsl->isChecked()) {
            notificationAreaIconLabelError->show();
            notificationAreaMessageBox->setText(QString("%1\n%2").arg(notificationAreaMessageBox->text(), tr("Mudlet is not configured for secure connections.\n\n")));
            port_ssl_tsl->setEnabled(true);
            validPort = false;
            valid = false;
        }
#else
        if (!QSslSocket::supportsSsl()) {
            if (port_ssl_tsl->isChecked()) {
                notificationAreaIconLabelError->show();
                notificationAreaMessageBox->setText(QString("%1\n%2").arg(notificationAreaMessageBox->text(), tr("Mudlet can not load support for secure connections.\n\n")));
                validPort = false;
                valid = false;
            }
        } else {
            port_ssl_tsl->setEnabled(true);
            port_ssl_tsl->setToolTip("");
        }
#endif
        QUrl check;
        QString url = host_name_entry->text().trimmed();
        check.setHost(url);
        if (!check.isValid()) {
            notificationAreaIconLabelError->show();
            notificationAreaMessageBox->setText(QString("%1\n%2").arg(notificationAreaMessageBox->text(), tr("Please enter the URL or IP address of the Game server.\n\n%1").arg(check.errorString())));
            host_name_entry->setPalette(mErrorPalette);
            validUrl = false;
            valid = false;
        }

        if (url.indexOf(QRegularExpression(QStringLiteral("^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$")), 0) != -1) {
            if (port_ssl_tsl->isChecked()) {
                notificationAreaIconLabelError->show();
                notificationAreaMessageBox->setText(QString("%1\n%2").arg(notificationAreaMessageBox->text(), tr("SSL connections require the URL of the Game server.\n\n%1").arg(check.errorString())));
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

            if (connect_button) {
                connect_button->setEnabled(true);
                connect_button->setToolTip(QString());
            }

            return true;

        } else {
            notificationArea->show();
            notificationAreaMessageBox->show();
            if (connect_button) {
                connect_button->setDisabled(true);
                connect_button->setToolTip(
                        QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(tr("Please set a valid profile name, game server address and the game port before connecting.")));
            }
            return false;
        }
    }
    return false;
}

// credit: http://www.qtcentre.org/archive/index.php/t-23469.html
void dlgConnectionProfiles::copyFolder(QString sourceFolder, QString destFolder)
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
