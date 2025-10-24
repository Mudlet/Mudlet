#ifndef MUDLET_DLGCONNECTIONPROFILES_H
#define MUDLET_DLGCONNECTIONPROFILES_H

/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016, 2020-2022 by Stephen Lyons                        *
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

#include "ui_connection_profiles.h"
#include <optional>
#include <QTimer>
#include <QKeyEvent>
#include <pugixml.hpp>

class QDir;

class dlgConnectionProfiles : public QDialog, public Ui::connection_profiles
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgConnectionProfiles)
    explicit dlgConnectionProfiles(QWidget* parent = nullptr);
    ~dlgConnectionProfiles();

    void fillout_form();
    QPair<bool, QString> writeProfileData(const QString& profile, const QString& item, const QString& what);
    QString readProfileData(const QString& profile, const QString& item) const;
    void accept() override;
    QList<QListWidgetItem*> findData(const QListWidget& listWidget, const QVariant& what, const int role = Qt::UserRole) const;
    QList<int> findProfilesBeginningWith(const QString&) const;
    static const int csmNameRole{Qt::UserRole};

    QString btn_connect_enabled_accessDesc;
    QString btn_load_enabled_accessDesc;
    QString btn_connOrLoad_disabled_accessDesc;
    QString item_profile_accessName;
    QString item_profile_accessDesc;

signals:
    void signal_load_profile(QString profile_name, bool alsoConnect);

public slots:
    void slot_updateName(const QString&);
    void slot_saveName();
    void slot_updateUrl(const QString&);
    void slot_updatePort(const QString&);
    void slot_updateSslTslPort(int state);
    void slot_updateLogin(const QString&);
    void slot_updatePassword(const QString&);
// Not used:    void slot_updateWebsite(const QString&);
    void slot_deleteProfileCheck(const QString&);
    void slot_updateDescription();

    void slot_itemClicked(QListWidgetItem*);
    void slot_addProfile();
    void slot_deleteProfile();
    void slot_reallyDeleteProfile();

    void slot_updateAutoConnect(int state);
    void slot_updateAutoReconnect(int state);
    void slot_updateDiscordOptIn(int state);
    void slot_load();
    void slot_cancel();
    void slot_copyProfile();
    void slot_copyOnlySettingsOfProfile();
    void indicatePackagesInstallOnConnect(QStringList packages);


protected:
    bool eventFilter(QObject*, QEvent*) override;
    void loadPasswordFromSettings(const QString& profile_name);
    void ensurePasswordLoadedThenConnect(bool alsoConnect);
    bool hasPendingKeychainOperation(const QString& profile_name) const;


private:
    static bool copyFolder(const QString& sourceFolder, const QString& destFolder);
    QString getDescription(const QString& profile_name) const;
    bool validateConnect();
    void updateDiscordStatus();
    bool validateProfile();
    void loadProfile(bool alsoConnect);
    void copyProfileSettingsOnly(const QString& oldname, const QString& newname);
    bool extractSettingsFromProfile(pugi::xml_document& newProfile, const QString& copySettingsFrom);
    void saveProfileCopy(const QDir& newProfiledir, const pugi::xml_document& newProfileXml) const;
    bool copyProfileWidget(QString& profile_name, QString& oldname, QListWidgetItem*& pItem) const;
    bool hasCustomIcon(const QString&) const;
    void setProfileIcon() const;
    void loadCustomProfile(const QString&) const;
    void generateCustomProfile(const QString&) const;
    void setCustomIcon(const QString&, QListWidgetItem*) const;
    template <typename L>
    void loadSecuredPassword(const QString& profile, L callback);
    void migrateSecuredPassword(const QString& oldProfile, const QString& newProfile);
    void writeSecurePassword(const QString& profile, const QString& pass) const;
    void deleteSecurePassword(const QString& profile) const;
    void setupMudProfile(QListWidgetItem*, const QString& mudServer, const QString& serverDescription, const QString& iconFileName);
    void reallyDeleteProfile(const QString& profile);
    void continueProfileSave(QListWidgetItem* pItem, const QString& newProfileName,
                           const QString& newProfileHost, const QString& newProfilePort,
                           const int newProfileSslTsl);
    void setItemName(QListWidgetItem*, const QString&) const;
    QIcon customIcon(const QString&, const std::optional<QColor>&) const;
    void addLetterToProfileSearch(const int);
    inline void clearNotificationArea();
    void loadPasswordAsync(const QString& profileName);

    // split into 3 properties so each one can be checked individually
    // important for creation of a folder on disk, for example: name has
    // to be valid, but other properties don't have to be
    bool validName = false;
    bool validUrl = false;
    bool validPort = false;

    QStringList mProfileList;
    QPalette mRegularPalette;
    QPalette mOKPalette;
    QPalette mErrorPalette;
    QPalette mReadOnlyPalette;
    QAction* mpCopyProfile = nullptr;
    QPushButton* offline_button = nullptr;
    QPushButton* connect_button = nullptr;
    QLineEdit* delete_profile_lineedit = nullptr;
    QPushButton* delete_button  = nullptr;
    QString mDiscordApplicationId;
    QString mDiscordInviteURL;
    QAction* mpAction_revealPassword;
    // true for the duration of the 'Copy profile' action
    bool mCopyingProfile = false;
    QString mDateTimeFormat;
    QVector<QColor> mCustomIconColors;
    QTimer mSearchTextTimer;
    QString mSearchText;
    QTimer* mPasswordSaveTimer = nullptr;

    // Async connection handling
    QString mPendingProfileLoad;  // Profile name waiting for password load
    bool mPendingConnect = false; // Whether to connect (true) or just load (false)
    bool mKeychainOperationInProgress = false;  // Track if keychain op is active


private slots:
    void slot_profileContextMenu(QPoint pos);
    void slot_setCustomIcon();
    void slot_setCustomColor();
    void slot_resetCustomIcon();
    void slot_togglePasswordVisibility(const bool);
    void slot_reenableAllProfileItems();
    void slot_loadPasswordAsync();
    void slot_passwordTextChanged();
};


#endif // MUDLET_DLGCONNECTIONPROFILES_H
