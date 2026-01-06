#ifndef MUDLET_DLGPACKAGEMANAGER_H
#define MUDLET_DLGPACKAGEMANAGER_H

/***************************************************************************
 *   Copyright (C) 2011 by Heiko Koehn - KoehnHeiko@googlemail.com         *
 *   Copyright (C) 2021 by Manuel Wegmann - wegmann.manuel@yahoo.com       *
 *   Copyright (C) 2022 by Stephen Lyons - slysven@virginmedia.com         *
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


#include "PackageItemDelegate.h"

#include "ui_package_manager.h"
#include <QDialog>
#include <QJsonArray>
#include <QListWidget>
#include <QTextBrowser>

class Host;
class QNetworkReply;


class dlgPackageManager : public QDialog, public Ui::package_manager
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgPackageManager)
    explicit dlgPackageManager(QWidget* parent, Host*);
    bool readPackageRepositoryFile();
    void resetPackageList();

signals:
    void packageManagerClosing(const QString& profileName);

private slots:
    void slot_installPackageFromFile();
    void slot_installPackageFromRepository();
    void slot_itemChanged(QListWidgetItem*);
    void slot_openBugWebsite();
    void slot_openPackageWebsite();
    void slot_onIconDownloaded(QNetworkReply *reply);
    void slot_removePackages();
    void slot_searchTextChanged(const QString &searchText);
    void slot_setPackageList();
    void slot_toggleInstallRepoButton();
    void slot_toggleRemoveButton();

private:
    void clearPackageDetails();
    void closeEvent(QCloseEvent* event) override;
    void downloadIcon(const QString &packageName);
    void downloadRepositoryIndex();    
    void fillPackageDetails(const QString &name, const QString &title, const QString &author, const QString &version);

    Host* mpHost = nullptr;
    PackageItemDelegate* mpPackageItemDelegate = nullptr;
    QHash<QString, QJsonObject> packageLookup;
    QJsonArray repositoryPackages;
    QListWidgetItem* statusAvailable;
    QListWidgetItem* statusInstalled;
};

#endif
