#ifndef MUDLET_DLGCONNECTIONPROFILES_H
#define MUDLET_DLGCONNECTIONPROFILES_H

/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Stephen Lyons - slysven@virginmedia.com         *
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
#include "post_guard.h"

class dlgConnectionProfiles : public QDialog, public Ui::connection_profiles
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgConnectionProfiles)
    dlgConnectionProfiles(QWidget* parent = nullptr);
    void fillout_form();
    QPair<bool, QString> writeProfileData(const QString& profile, const QString& item, const QString& what);
    QString readProfileData(QString, QString);
    void accept() override;

signals:
    void signal_establish_connection(QString profile_name, int historyVersion);

public slots:
    void slot_update_name(QString);
    void slot_save_name();
    void slot_update_url(const QString &);
    void slot_update_port(QString);
    void slot_update_login(const QString &);
    void slot_update_pass(const QString &);
    void slot_update_website(const QString &);
    void slot_deleteprofile_check(QString);
    void slot_update_description();

    void slot_item_clicked(QListWidgetItem*);
    void slot_addProfile();
    void slot_deleteProfile();
    void slot_reallyDeleteProfile();

    void slot_update_autologin(int state);
    void slot_update_discord_optin(int state);
    void slot_connectToServer();
    void slot_cancel();
    void slot_copy_profile();

private:
    void copyFolder(QString sourceFolder, QString destFolder);
    QString getDescription(const QString& hostUrl, quint16 port, const QString& profile_name);

    bool validName;
    bool validUrl;
    bool validPort;
    bool validateConnect();

    QStringList mProfileList;
    QPalette mRegularPalette;
    QPalette mOKPalette;
    QPalette mErrorPalette;
    QPalette mReadOnlyPalette;
    QPushButton* connect_button;
    QLineEdit* delete_profile_lineedit;
    QPushButton* delete_button;
    void updateDiscordStatus();
};

#endif // MUDLET_DLGCONNECTIONPROFILES_H
