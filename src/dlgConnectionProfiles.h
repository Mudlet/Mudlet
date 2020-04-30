#ifndef MUDLET_DLGCONNECTIONPROFILES_H
#define MUDLET_DLGCONNECTIONPROFILES_H

/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016, 2020 by Stephen Lyons - slysven@virginmedia.com   *
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

#include "pre_guard.h"
#include "ui_connection_profiles.h"
#include "QDir"
#include <pugixml.hpp>
#if defined(INCLUDE_OWN_QT5_KEYCHAIN)
#include <../3rdparty/qtkeychain/keychain.h>
#else
#include <qt5keychain/keychain.h>
#endif
#include "post_guard.h"

class dlgConnectionProfiles : public QDialog, public Ui::connection_profiles
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgConnectionProfiles)
    dlgConnectionProfiles(QWidget* parent = nullptr);
    void fillout_form();
    QPair<bool, QString> writeProfileData(const QString& profile, const QString& item, const QString& what);
    QString readProfileData(const QString& profile, const QString& item) const;
    void accept() override;

signals:
    void signal_load_profile(QString profile_name, bool alsoConnect);

public slots:
    void slot_update_name(const QString&);
    void slot_save_name();
    void slot_update_url(const QString &);
    void slot_update_port(const QString&);
    void slot_update_SSL_TSL_port(int state);
    void slot_update_login(const QString &);
    void slot_update_pass(const QString &);
    void slot_update_website(const QString &);
    void slot_deleteprofile_check(const QString&);
    void slot_update_description();

    void slot_item_clicked(QListWidgetItem*);
    void slot_addProfile();
    void slot_deleteProfile();
    void slot_reallyDeleteProfile();

    void slot_update_autologin(int state);
    void slot_update_autoreconnect(int state);
    void slot_update_discord_optin(int state);
    void slot_connectToServer();
    void slot_load();
    void slot_cancel();
    void slot_copy_profile();
    void slot_copy_profilesettings_only();

private:
    void copyFolder(const QString& sourceFolder, const QString& destFolder);
    QString getDescription(const QString& hostUrl, quint16 port, const QString& profile_name) const;
    bool validateConnect();
    void updateDiscordStatus();
    bool validateProfile();
    void loadProfile(bool alsoConnect);
    void copyProfileSettingsOnly(const QString& oldname, const QString& newname);
    bool extractSettingsFromProfile(pugi::xml_document& newProfile, const QString& copySettingsFrom);
    void saveProfileCopy(const QDir& newProfiledir, const pugi::xml_document& newProfileXml) const;
    bool copyProfileWidget(QString& profile_name, QString& oldname, QListWidgetItem*& pItem) const;
    bool hasCustomIcon(const QString& profileName) const;
    void setProfileIcon(const QFont& font) const;
    void loadCustomProfile(const QFont& font, const QString& profileName) const;
    void generateCustomProfile(const QFont& font, int i, const QString& profileName) const;
    void setCustomIcon(const QString& profileName, QListWidgetItem* profile) const;
    template <typename L>
    void loadSecuredPassword(const QString& profile, L callback);
    void migrateSecuredPassword(const QString& oldProfile, const QString& newProfile);
    void writeSecurePassword(const QString& profile, const QString& pass) const;
    void deleteSecurePassword(const QString& profile) const;

    // split into 3 properties so each one can be checked individually
    // important for creation of a folder on disk, for example: name has
    // to be valid, but other properties don't have to be
    bool validName;
    bool validUrl;
    bool validPort;

    QStringList mProfileList;
    QPalette mRegularPalette;
    QPalette mOKPalette;
    QPalette mErrorPalette;
    QPalette mReadOnlyPalette;
    QPushButton* offline_button;
    QPushButton* connect_button;
    QLineEdit* delete_profile_lineedit;
    QPushButton* delete_button;
    QString mDiscordApplicationId;
    const QStringList mDefaultGames;
    QAction* mpAction_revealPassword;
    // true for the duration of the 'Copy profile' action
    bool mCopyingProfile {};


private slots:
    void slot_profile_menu(QPoint pos);
    void slot_set_custom_icon();
    void slot_reset_custom_icon();
    void slot_togglePasswordVisibility(const bool);
    void slot_password_saved(QKeychain::Job* job);
    void slot_password_deleted(QKeychain::Job* job);
};


#endif // MUDLET_DLGCONNECTIONPROFILES_H
