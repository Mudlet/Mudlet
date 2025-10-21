/***************************************************************************
 *   Copyright (C) 2011 by Heiko Koehn - KoehnHeiko@googlemail.com         *
 *   Copyright (C) 2021 by Manuel Wegmann - wegmann.manuel@yahoo.com       *
 *   Copyright (C) 2022 by Stephen Lyons - slysven@virginmedia.com         *
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


#include "dlgPackageManager.h"

#include "mudlet.h"

#include "pre_guard.h"
#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include "post_guard.h"


dlgPackageManager::dlgPackageManager(QWidget* parent, Host* pHost)
: QDialog(parent)
, mpHost(pHost)
{
    setupUi(this);
    connect(lineEdit_searchBar, &QLineEdit::textChanged, this, &dlgPackageManager::slot_searchTextChanged);
    connect(mpHost->mpConsole, &QWidget::destroyed, this, &dlgPackageManager::close);
    connect(packageList, &QListWidget::currentItemChanged, this, &dlgPackageManager::slot_itemChanged);
    connect(packageList, &QListWidget::itemSelectionChanged, this, &dlgPackageManager::slot_toggleInstallRepoButton);
    connect(packageList, &QListWidget::itemSelectionChanged, this, &dlgPackageManager::slot_toggleRemoveButton);
    connect(packageStatusList, &QListWidget::currentItemChanged, this, &dlgPackageManager::slot_setPackageList);
    connect(pushButton_installFile, &QAbstractButton::clicked, this, &dlgPackageManager::slot_installPackageFromFile);
    connect(pushButton_installRepo, &QAbstractButton::clicked, this, &dlgPackageManager::slot_installPackageFromRepository);    
    connect(pushButton_remove, &QAbstractButton::clicked, this, &dlgPackageManager::slot_removePackages);
    connect(pushButton_report, &QAbstractButton::clicked, this, &dlgPackageManager::slot_openBugWebsite);
    connect(pushButton_website, &QAbstractButton::clicked, this, &dlgPackageManager::slot_openPackageWebsite);

    //: Package manager - window title
    setWindowTitle(tr("Package Manager - %1").arg(mpHost->getName()));

    pushButton_website->setIcon(QIcon(qsl(":/icons/applications-internet.png")));
    pushButton_website->hide();
    pushButton_report->setIcon(QIcon(qsl(":/icons/flag-red.png")));    
    pushButton_report->hide();    

    packageStatusList->setSortingEnabled(false);
    //: Package manager - status item showing installed packages
    statusInstalled = new QListWidgetItem(tr("Installed") + QString(" (%1)").arg(mpHost->mInstalledPackages.size()), packageStatusList);
    statusUpdates = new QListWidgetItem(tr("Updates"), packageStatusList);
    //: Package manager - status item showing available packages
    statusAvailable = new QListWidgetItem(tr("Available") + QString(" (%1)").arg(repositoryPackages.size()), packageStatusList);
    packageStatusList->setCurrentItem(statusInstalled);

    packageList->setSortingEnabled(true);

    repositoryPackages = QJsonArray();
    if (!readPackageRepositoryFile()) {
        downloadRepositoryIndex();
    }

    pushButton_installRepo->setEnabled(false);
    if (packageList->count() >= 0) {
        packageList->setCurrentRow(0);
    }

    setAttribute(Qt::WA_DeleteOnClose);
}

dlgPackageManager::~dlgPackageManager()
{
}

void dlgPackageManager::clearPackageDetails()
{
    label_icon->clear();
    packageDescription->clear();
    label_packageName->clear();
    label_title->clear();
    label_author->clear();
    label_version->clear();
    pushButton_website->hide();
    pushButton_report->hide();
}

void dlgPackageManager::downloadIcon(const QString &packageName) 
{
    QString iconPath;

    for (const QJsonValue &value : repositoryPackages) {
        const QJsonObject packageObj = value.toObject();

        if (packageObj[qsl("mpackage")].toString() == packageName) {
            if (packageObj.contains(qsl("icon"))) {
                iconPath = packageObj[qsl("icon")].toString();
            } else {
                iconPath = qsl(":/icons/package-manager.png");
                QPixmap pixmap(iconPath);               
                label_icon->setPixmap(pixmap.scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                return;
            }
            break;
        }
    }

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkReply *reply = manager->get(QNetworkRequest(QUrl(qsl("https://github.com/Mudlet/mudlet-package-repository/raw/refs/heads/main/") + iconPath)));
    reply->setProperty("packageName", packageName);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){ slot_onIconDownloaded(reply); });
}

void dlgPackageManager::downloadRepositoryIndex() 
{
    const QUrl url = QUrl(qsl("https://raw.githubusercontent.com/Mudlet/mudlet-package-repository/refs/heads/main/packages/mpkg.packages.json"));
    const QString outputPath = mudlet::getMudletPath(enums::profileHomePath, mpHost->getName() + QDir::separator() + qsl("mpkg.packages.json"));
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkReply *reply = manager->get(QNetworkRequest(url));
    QFile *file = new QFile(outputPath);

    if (!file->open(QIODevice::WriteOnly)) {
        file->deleteLater();
        reply->deleteLater();
        manager->deleteLater();
        return;
    }

    QObject::connect(reply, &QNetworkReply::readyRead, [file, reply]() {
        file->write(reply->readAll());
    });

    QObject::connect(reply, &QNetworkReply::finished, [reply, file, manager, this]() {
        file->write(reply->readAll());
        file->close();
        reply->deleteLater();
        file->deleteLater();
        manager->deleteLater();
        readPackageRepositoryFile();
    });
}

void dlgPackageManager::fillPackageDetails(const QString &name, const QString &title, const QString &author, const QString &version)
{
    const QFontMetrics metrics(label_packageName->font());
    const QString elidedText = metrics.elidedText(name, Qt::ElideRight, label_packageName->width());
    label_packageName->setText(elidedText);
    label_title->setText(title);
    label_author->setText(author);
    //: Package manager - label showing package version
    label_version->setText(tr("Version ") + version);
}

bool dlgPackageManager::readPackageRepositoryFile() 
{
    //: Package manager - status item showing number of available packages
    statusAvailable->setText(tr("Available") + QString(" (%1)").arg(repositoryPackages.size()));

    QFile file(mudlet::getMudletPath(enums::profileHomePath, mpHost->getName() + QDir::separator() + qsl("mpkg.packages.json")));
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    const QByteArray data = file.readAll();
    const QJsonDocument doc(QJsonDocument::fromJson(data));

    if (!doc.isObject()) {
        return false;
    }

    QJsonObject obj = doc.object();
    repositoryPackages = obj[qsl("packages")].toArray();  
    //: Package manager - status item showing number of available packages
    statusAvailable->setText(tr("Available") + QString(" (%1)").arg(repositoryPackages.size()));
    return true;
}

void dlgPackageManager::resetPackageList()
{
    if (!mpHost) {
        return;
    }

    clearPackageDetails();
    packageStatusList->setCurrentItem(statusInstalled);
    packageList->clear();

    for (int i = 0; i < mpHost->mInstalledPackages.size(); i++) {
        auto item = new QListWidgetItem();
        item->setText(mpHost->mInstalledPackages.at(i));

        auto packageInfo{mpHost->mPackageInfo.value(item->text())};
        const auto iconName = packageInfo.value(qsl("icon"));
        if (!iconName.isEmpty()) {
            const auto iconDir = mudlet::getMudletPath(enums::profileDataItemPath, mpHost->getName(), qsl("%1/.mudlet/Icon/%2").arg(mpHost->mInstalledPackages.at(i), iconName));
            item->setIcon(QIcon(iconDir));
        } else {
            // for alignment purposes in the package list
            QPixmap emptyPixmap(16, 16);
            emptyPixmap.fill(Qt::transparent);
            item->setIcon(QIcon(emptyPixmap));   
        }     
        packageList->addItem(item);
    }

    //: Package manager - status item showing number of installed packages
    statusInstalled->setText(tr("Installed") + QString(" (%1)").arg(mpHost->mInstalledPackages.size()));    
}

void dlgPackageManager::slot_installPackageFromFile()
{
    QSettings& settings = *mudlet::getQSettings();
    QString lastDir = settings.value(qsl("lastFileDialogLocation"), QDir::homePath()).toString();

    //: Package manager - import package from file dialog
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Import Mudlet Package"), lastDir);
    if (fileName.isEmpty()) {
        return;
    }

    lastDir = QFileInfo(fileName).absolutePath();
    settings.setValue("lastFileDialogLocation", lastDir);

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        //: Package manager - error when attempting to read a file to import
        QMessageBox::warning(this, tr("Import Mudlet Package:"), tr("Cannot read file %1:\n%2.").arg(fileName.toHtmlEscaped(), file.errorString()));
        return;
    }

    mpHost->installPackage(fileName, enums::PackageModuleType::Package);
}

void dlgPackageManager::slot_installPackageFromRepository()
{ 
    const QList<QListWidgetItem*> selected = packageList->selectedItems();
    if (selected.isEmpty()) {
        return;
    }

    this->setEnabled(false);

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QHash<QString, QString> *pendingDownloads = new QHash<QString, QString>();
    int *remainingDownloads = new int(selected.size());
    bool repoError = false;

    for (QListWidgetItem* item : selected) {
        const QString packageName = item->text();

        QJsonObject foundObj;
        for (const QJsonValue &val : repositoryPackages) {
            QJsonObject obj = val.toObject();
            if (obj.value("mpackage").toString() == packageName) {
                foundObj = obj;
                break;
            }
        }

        if (foundObj.isEmpty()) {
            repoError = true;
            continue;
        }

        const QString remoteFileName = foundObj.value("filename").toString();
        if (remoteFileName.isEmpty()) {
            repoError = true;
            continue;
        }

        const QByteArray encoded = QUrl::toPercentEncoding(remoteFileName);
        const QUrl downloadUrl(qsl("https://github.com/Mudlet/mudlet-package-repository/raw/refs/heads/main/packages/%1").arg(QString::fromUtf8(encoded)));
        const QString outDir = mudlet::getMudletPath(enums::profileHomePath, mpHost->getName());
        const QString outPath = outDir + QDir::separator() + remoteFileName;

        QNetworkReply *reply = manager->get(QNetworkRequest(downloadUrl));

        QFile *file = new QFile(outPath);
        if (!file->open(QIODevice::WriteOnly)) {
            file->deleteLater();
            reply->deleteLater();
            continue;
        }

        QObject::connect(reply, &QNetworkReply::readyRead, [file, reply]() {
            file->write(reply->readAll());
        });

        pendingDownloads->insert(packageName, outPath);

        QObject::connect(reply, &QNetworkReply::finished, [reply, file, this, outPath, packageName, pendingDownloads, remainingDownloads, manager]() {
            file->write(reply->readAll());
            file->close();
            reply->deleteLater();
            file->deleteLater();

            if (reply->error() != QNetworkReply::NoError) {
                pendingDownloads->remove(packageName);
                (*remainingDownloads)--;
            }

            if (--(*remainingDownloads) == 0) {
                for (auto it = pendingDownloads->begin(); it != pendingDownloads->end(); ++it) {
                    const QString &pkgName = it.key();
                    const QString &filePath = it.value();
                    
                    if (mpHost) {
                        mpHost->installPackage(filePath, enums::PackageModuleType::Package);
                    }
                    QFile::remove(filePath);
                }

                this->setEnabled(true);
                delete pendingDownloads;
                delete remainingDownloads;
                manager->deleteLater();
            }
        });
    }

    // package listing must have been corrupted, re-download
    if (repoError) {
        downloadRepositoryIndex();
    }
}

void dlgPackageManager::slot_itemChanged(QListWidgetItem* pItem)
{
    if (!pItem) {
        return;
    }

    clearPackageDetails();

    const auto status = packageStatusList->currentItem();
    QString packageName = pItem->text();   

    if (status == statusInstalled) {
        auto packageInfo{mpHost->mPackageInfo.value(packageName)};
        if (packageInfo.isEmpty()) {
            packageDescription->clear();
            return;
        }

        // if the package is installed and in the repository, show the website link and report buttons
        for (const QJsonValue& packageVal : repositoryPackages) {
            const QJsonObject packageObj = packageVal.toObject();
            if (packageObj.value(qsl("mpackage")).toString() == packageName) {
                pushButton_website->show();
                pushButton_report->show();
                break;
            }
        }        

        QString description = packageInfo.value(qsl("description"));
        if (!description.isEmpty()) {
            QString packageDir = mudlet::self()->getMudletPath(enums::profileDataItemPath, mpHost->getName(), packageName);
            description.replace(QLatin1String("$packagePath"), packageDir);
            packageDescription->setMarkdown(description);
        }

        auto iconName = packageInfo.value(qsl("icon"));
        if (!iconName.isEmpty()) {
            const auto iconDir = mudlet::getMudletPath(enums::profileDataItemPath, mpHost->getName(), qsl("%1/.mudlet/Icon/%2").arg(packageName, iconName));
            label_icon->setPixmap(QPixmap(iconDir));
        }
        else {
            QPixmap pixmap(":/icons/package-manager.png");               
            label_icon->setPixmap(pixmap.scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }

        fillPackageDetails(packageName,
                           packageInfo.value(qsl("title")),
                           packageInfo.value(qsl("author")),
                           packageInfo.value(qsl("version")));

    } else if (status == statusAvailable) {
        pushButton_website->show();
        pushButton_report->show();
        downloadIcon(packageName);

        for (const QJsonValue& packageVal : repositoryPackages) {
            QJsonObject packageObj = packageVal.toObject();
            if (packageObj.value(qsl("mpackage")).toString() == packageName) {
                fillPackageDetails(packageObj.value(qsl("mpackage")).toString(),
                                   packageObj.value(qsl("title")).toString(),
                                   packageObj.value(qsl("author")).toString(),
                                   packageObj.value(qsl("version")).toString());
                packageDescription->setMarkdown(packageObj.value(qsl("description")).toString());                
                break;
            }
        }
    }
}

void dlgPackageManager::slot_onIconDownloaded(QNetworkReply *reply) 
{
    if (!reply) {
        return;
    }
    
    const QString requestedPackage = reply->property("packageName").toString();
    const QListWidgetItem* currentItem = packageList->currentItem();
    
    if (!currentItem || currentItem->text() != requestedPackage) {
        reply->deleteLater();
        reply->manager()->deleteLater();
        return;
    }

    if (reply->error() == QNetworkReply::NoError) {
        const QByteArray imageData = reply->readAll();
        QPixmap pixmap;
        pixmap.loadFromData(imageData);
        label_icon->setPixmap(pixmap.scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        const QPixmap pixmap(qsl(":/icons/mudlet.png"));               
        label_icon->setPixmap(pixmap.scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void dlgPackageManager::slot_openBugWebsite()
{
    const QListWidgetItem* currentItem = packageList->currentItem();
    if (!currentItem) {
        return;
    }

    mudlet::self()->openWebPage(qsl("https://github.com/Mudlet/mudlet-package-repository/issues/new?template=package-bug-or-issue.md&title=[Package%20Bug]%20") + currentItem->text());
}

void dlgPackageManager::slot_openPackageWebsite()
{
    const QListWidgetItem* currentItem = packageList->currentItem();
    if (!currentItem) {
        return;
    }

    mudlet::self()->openWebPage(qsl("https://packages.mudlet.org/packages#pkg-") + currentItem->text());
}

void dlgPackageManager::slot_removePackages()
{
    const QList<QListWidgetItem*> selectedItems = packageList->selectedItems();
    QStringList removePackages;

    for (QListWidgetItem* item : selectedItems) {
        removePackages << item->text();
    }

    for (const QString& package : removePackages) {
        mpHost->uninstallPackage(package, enums::PackageModuleType::Package);
    }
}

void dlgPackageManager::slot_searchTextChanged(const QString &searchText)
{
    const auto status = packageStatusList->currentItem();
    packageList->clear();

    if (status == statusInstalled) {
        for (const auto &value : mpHost->mPackageInfo) {
            const QString name = value.value(qsl("mpackage"));
            const QString title = value.value(qsl("title"));
            const QString description = value.value(qsl("description"));
            const QString author = value.value(qsl("author"));

            if (name.contains(searchText, Qt::CaseInsensitive) ||
                title.contains(searchText, Qt::CaseInsensitive) ||
                description.contains(searchText, Qt::CaseInsensitive) ||
                author.contains(searchText, Qt::CaseInsensitive)) {

                QListWidgetItem *item = new QListWidgetItem(name);
                const auto iconName = value.value(qsl("icon"));
                if (!iconName.isEmpty()) {
                    const auto iconDir = mudlet::getMudletPath(enums::profileDataItemPath, mpHost->getName(), qsl("%1/.mudlet/Icon/%2").arg(name, iconName));
                    item->setIcon(QIcon(iconDir));
                } else {
                    // for alignment purposes in the package list
                    QPixmap emptyPixmap(16, 16);
                    emptyPixmap.fill(Qt::transparent);
                    item->setIcon(QIcon(emptyPixmap));
                }
                packageList->addItem(item);
            }
        }
    }
    else if (status == statusAvailable) {
        for (const QJsonValue &value : repositoryPackages) {
            const QJsonObject pkg = value.toObject();
            const QString name = pkg[qsl("mpackage")].toString();
            const QString title = pkg[qsl("title")].toString();
            const QString description = pkg[qsl("description")].toString();
            const QString author = pkg[qsl("author")].toString();

            if (name.contains(searchText, Qt::CaseInsensitive) ||
                title.contains(searchText, Qt::CaseInsensitive) ||
                description.contains(searchText, Qt::CaseInsensitive) ||
                author.contains(searchText, Qt::CaseInsensitive)) {

                QListWidgetItem *item = new QListWidgetItem(name);
                if (pkg.contains(qsl("icon"))) {
                    const QPixmap pixmap(pkg[qsl("icon")].toString());
                    item->setIcon(QIcon(pixmap));
                }
                packageList->addItem(item);
            }
        }
    }
}

void dlgPackageManager::slot_setPackageList()
{
    if (!mpHost) {
        return;
    }

    if (lineEdit_searchBar->text().length() > 0) {
        slot_searchTextChanged(lineEdit_searchBar->text());
        return;
    }
    
    packageList->clear();
    clearPackageDetails();

    const auto status = packageStatusList->currentItem();
    
    if (status == statusInstalled) { 
        for (int i = 0; i < mpHost->mInstalledPackages.size(); i++) {
            auto item = new QListWidgetItem();
            item->setText(mpHost->mInstalledPackages.at(i));
            const auto packageInfo{mpHost->mPackageInfo.value(item->text())};
            const auto iconName = packageInfo.value(qsl("icon"));
            if (!iconName.isEmpty()) {
                const auto iconDir = mudlet::getMudletPath(enums::profileDataItemPath, mpHost->getName(), qsl("%1/.mudlet/Icon/%2").arg(mpHost->mInstalledPackages.at(i), iconName));
                item->setIcon(QIcon(iconDir));
            } else {
                // for alignment purposes in the package list
                QPixmap emptyPixmap(16, 16);
                emptyPixmap.fill(Qt::transparent);
                item->setIcon(QIcon(emptyPixmap));
            }
            packageList->addItem(item);
        }
    } else if (status == statusAvailable) {

        if (!readPackageRepositoryFile()) {
            return;
        }        
        for (const QJsonValue& packageVal : repositoryPackages) {
            auto item = new QListWidgetItem();
            const QJsonObject packageObj = packageVal.toObject();
            const QString packageName = packageObj.value("mpackage").toString();
            item->setText(packageName);
            packageList->addItem(item);
        }
    }
}

void dlgPackageManager::slot_toggleInstallRepoButton()
{
    const auto status = packageStatusList->currentItem();

    if (status == statusAvailable) {
        const QList selection = packageList->selectedItems();
        const int selectionCount = selection.size();
        pushButton_installRepo->setEnabled(selectionCount);
        if (selectionCount) {
            //: Message on button in package manager to install one or more (%n is the count of) selected package(s).
            pushButton_installRepo->setText(tr("Install (%n)", nullptr, selectionCount));
        } else {
            //: Message on button in package manager initially and when there is no packages to install
            pushButton_installRepo->setText(tr("Install"));
        }
    } else {
        //: Message on button in package manager initially and when there is no packages to install
        pushButton_installRepo->setText(tr("Install"));
        pushButton_installRepo->setEnabled(false);
    }
}

void dlgPackageManager::slot_toggleRemoveButton()
{
    const auto status = packageStatusList->currentItem();

    if (status == statusInstalled) {
        const QList selection = packageList->selectedItems();
        const int selectionCount = selection.size();
        pushButton_remove->setEnabled(selectionCount);
        if (selectionCount) {
            //: Message on button in package manager to remove one or more (%n is the count of) selected package(s).
            pushButton_remove->setText(tr("Remove (%n)", nullptr, selectionCount));
        } else {
            //: Message on button in package manager initially and when there is no packages to remove
            pushButton_remove->setText(tr("Remove"));
        }
    } else {
        //: Message on button in package manager initially and when there is no packages to remove
        pushButton_remove->setText(tr("Remove"));
        pushButton_remove->setEnabled(false);
    }
}

void dlgPackageManager::closeEvent(QCloseEvent* event)
{
    if (mpHost) {
        emit packageManagerClosing(mpHost->getName());
    }
    QDialog::closeEvent(event);
}
