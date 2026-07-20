/***************************************************************************
 *   Copyright (C) 2011 by Heiko Koehn - KoehnHeiko@googlemail.com         *
 *   Copyright (C) 2021 by Manuel Wegmann - wegmann.manuel@yahoo.com       *
 *   Copyright (C) 2022, 2026 by Stephen Lyons - slysven@virginmedia.com   *
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

#include <QCloseEvent>
#include <QFileDialog>
#include <QJsonDocument>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QProgressDialog>
#include <QSettings>
#include <QTimer>
#include <QVersionNumber>

using namespace std::chrono_literals;


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

    setupNavigationButtons();
    applyNavigationStyles();

    packageList->setSortingEnabled(true);

    mpPackageItemDelegate = new PackageItemDelegate(this);
    packageList->setItemDelegate(mpPackageItemDelegate);

    repositoryPackages = QJsonArray();
    if (!readPackageRepositoryFile()) {
        downloadRepositoryIndex();
    }

    pushButton_installRepo->setEnabled(false);
    mCurrentView = NavigationView::Installed;
    slot_setPackageList();

    setAttribute(Qt::WA_DeleteOnClose);
}

void dlgPackageManager::applyNavigationStyles()
{
    navigationFrame->setStyleSheet(qsl("QFrame#navigationFrame {"
                                       "  border: 1px solid palette(mid);"
                                       "  border-radius: 6px;"
                                       "  background-color: palette(window);"
                                       "}"
                                       "QPushButton {"
                                       "  border: none;"
                                       "  border-radius: 4px;"
                                       "  padding: 6px 16px;"
                                       "  background-color: transparent;"
                                       "}"
                                       "QPushButton:checked {"
                                       "  background-color: palette(highlight);"
                                       "  color: palette(highlighted-text);"
                                       "}"
                                       "QPushButton:hover:!checked {"
                                       "  background-color: palette(midlight);"
                                       "}"));
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

void dlgPackageManager::downloadIcon(const QString& packageName)
{
    QString iconPath;

    if (packageLookup.contains(packageName)) {
        QJsonObject packageObj = packageLookup.value(packageName);
        if (packageObj.contains(qsl("icon"))) {
            iconPath = packageObj[qsl("icon")].toString();
        } else {
            iconPath = qsl(":/icons/package-manager.png");
            QPixmap pixmap(iconPath);
            label_icon->setPixmap(pixmap.scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            return;
        }
    }

    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest request(QUrl(qsl("https://github.com/Mudlet/mudlet-package-repository/raw/refs/heads/main/") + iconPath));
    request.setTransferTimeout(10000);
    QNetworkReply* reply = manager->get(request);
    reply->setProperty("packageName", packageName);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        slot_onIconDownloaded(reply);
    });
}

void dlgPackageManager::downloadRepositoryIndex()
{
    const QString outputPath = mudlet::getMudletPath(enums::profileHomePath, mpHost->getName() + QDir::separator() + qsl("mpkg.packages.json"));
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest request(QUrl(qsl("https://raw.githubusercontent.com/Mudlet/mudlet-package-repository/refs/heads/main/packages/mpkg.packages.json")));
    request.setTransferTimeout(20000);
    QNetworkReply* reply = manager->get(request);
    QFile* file = new QFile(outputPath);

    if (!file->open(QIODevice::WriteOnly)) {
        file->deleteLater();
        reply->deleteLater();
        manager->deleteLater();
        return;
    }

    QObject::connect(reply, &QNetworkReply::readyRead, [file, reply]() {
        const QByteArray data = reply->readAll();
        if (file->write(data) != data.size()) {
            qWarning() << "dlgPackageManager::downloadRepositoryIndex() ERROR - failed to write downloaded data:" << file->errorString();
            reply->abort();
        }
    });

    QObject::connect(reply, &QNetworkReply::finished, [reply, file, manager, this]() {
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "dlgPackageManager::downloadRepositoryIndex() ERROR - network request failed:" << reply->errorString();
        } else {
            const QByteArray data = reply->readAll();
            if (!data.isEmpty() && file->write(data) != data.size()) {
                qWarning() << "dlgPackageManager::downloadRepositoryIndex() ERROR - failed to write final data:" << file->errorString();
            }
        }
        file->close();
        reply->deleteLater();
        file->deleteLater();
        manager->deleteLater();
        readPackageRepositoryFile();
        populatePackagesWithUpdates();
        updateUpdatesBadge();
    });
}

void dlgPackageManager::fillPackageDetails(const QString& name, const QString& title, const QString& author, const QString& version)
{
    const QFontMetrics metrics(label_packageName->font());
    const QString elidedText = metrics.elidedText(name, Qt::ElideRight, label_packageName->width());
    label_packageName->setText(elidedText);
    label_title->setText(title);
    label_author->setText(author);
    //: Package manager - label showing package version
    label_version->setText(tr("Version ") + version);
}

bool dlgPackageManager::hasNewerVersion(const QString& installed, const QString& repo) const
{
    const QVersionNumber installedVersion = QVersionNumber::fromString(installed);
    const QVersionNumber repoVersion = QVersionNumber::fromString(repo);
    return repoVersion > installedVersion;
}

void dlgPackageManager::populatePackagesWithUpdates()
{
    mPackagesWithUpdates.clear();

    for (const QString& packageName : std::as_const(mpHost->mInstalledPackages)) {
        if (!packageLookup.contains(packageName)) {
            continue;
        }

        const QJsonObject repoPackage = packageLookup.value(packageName);
        const QString repoVersion = repoPackage.value(qsl("version")).toString();
        const auto packageInfo = mpHost->mPackageInfo.value(packageName);
        const QString installedVersion = packageInfo.value(qsl("version"));

        if (!installedVersion.isEmpty() && !repoVersion.isEmpty() && hasNewerVersion(installedVersion, repoVersion)) {
            mPackagesWithUpdates.append(packageName);
        }
    }
}

bool dlgPackageManager::readPackageRepositoryFile()
{
    QFile file(mudlet::getMudletPath(enums::profileHomePath, mpHost->getName() + QDir::separator() + qsl("mpkg.packages.json")));
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QByteArray data = file.readAll();
    if (data.isEmpty()) {
        qWarning() << "Repository file is empty";
        return false;
    }

    const QJsonDocument doc(QJsonDocument::fromJson(data));
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON in repository file";
        return false;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("packages") || !obj["packages"].isArray()) {
        qWarning() << "Repository file corrupt: missing 'packages' array";
        return false;
    }

    repositoryPackages = obj[qsl("packages")].toArray();
    packageLookup.clear();
    for (const QJsonValue& val : std::as_const(repositoryPackages)) {
        QJsonObject pkg = val.toObject();
        packageLookup.insert(pkg["mpackage"].toString(), pkg);
    }

    populatePackagesWithUpdates();
    updateUpdatesBadge();

    return true;
}

void dlgPackageManager::resetPackageList()
{
    if (!mpHost) {
        return;
    }

    clearPackageDetails();
    mCurrentView = NavigationView::Installed;
    mpNavigationGroup->button(static_cast<int>(NavigationView::Installed))->setChecked(true);
    packageList->clear();

    for (int i = 0; i < mpHost->mInstalledPackages.size(); i++) {
        auto item = new QListWidgetItem();
        item->setText(mpHost->mInstalledPackages.at(i));

        auto packageInfo{mpHost->mPackageInfo.value(item->text())};
        const auto title = packageInfo.value(qsl("title"));
        if (!title.isEmpty()) {
            item->setData(Qt::UserRole, title);
        }
        const auto iconName = packageInfo.value(qsl("icon"));
        if (!iconName.isEmpty()) {
            const auto iconDir = mudlet::getMudletPath(enums::profileDataItemPath, mpHost->getName(), qsl("%1/.mudlet/Icon/%2").arg(mpHost->mInstalledPackages.at(i), iconName));
            item->setIcon(QIcon(iconDir));
        } else {
            QPixmap emptyPixmap(16, 16);
            emptyPixmap.fill(Qt::transparent);
            item->setIcon(QIcon(emptyPixmap));
        }
        packageList->addItem(item);
    }

    populatePackagesWithUpdates();
    updateUpdatesBadge();
    packageList->setCurrentRow(0);
}

void dlgPackageManager::setupNavigationButtons()
{
    mpNavigationGroup = new QButtonGroup(this);
    mpNavigationGroup->setExclusive(true);

    mpNavigationGroup->addButton(pushButton_explore, static_cast<int>(NavigationView::Explore));
    mpNavigationGroup->addButton(pushButton_installed, static_cast<int>(NavigationView::Installed));
    mpNavigationGroup->addButton(pushButton_updates, static_cast<int>(NavigationView::Updates));

    connect(mpNavigationGroup, &QButtonGroup::idClicked, this, &dlgPackageManager::slot_navigationButtonClicked);
}

void dlgPackageManager::slot_installPackageFromFile()
{
    QSettings& settings = *mudlet::getQSettings();
    QString lastDir = settings.value(qsl("lastFileDialogLocation"), QDir::homePath()).toString();

    //: Package manager - import packages from file dialog (multi-select enabled)
    //: Package manager - file filter for supported package types (mpackage, zip, xml)
    const QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Import Mudlet Package"), lastDir, tr("Mudlet Packages (*.mpackage *.zip *.xml)"));
    if (fileNames.isEmpty()) {
        return;
    }

    lastDir = QFileInfo(fileNames.first()).absolutePath();
    settings.setValue(qsl("lastFileDialogLocation"), lastDir);

    QStringList failedPackages;

    for (const QString& fileName : fileNames) {
        auto [success, errorMsg] = mpHost->installPackage(fileName, enums::PackageModuleType::Package);
        if (success) {
            mpHost->waitForProfileSave();
        } else {
            const QString baseName = QFileInfo(fileName).fileName();
            failedPackages << baseName;
            qWarning() << "dlgPackageManager::slot_installPackageFromFile() ERROR - failed to import" << baseName << ":" << errorMsg;
        }
    }

    resetPackageList();

    if (!failedPackages.isEmpty()) {
        //: Package manager - status message shown when some packages failed to import. %1 is a comma-separated list of package names
        showImportStatus(tr("Failed to import: %1").arg(failedPackages.join(qsl(", "))));
    }
}

void dlgPackageManager::slot_installPackageFromRepository()
{
    const QList<QListWidgetItem*> selected = packageList->selectedItems();
    if (selected.isEmpty()) {
        return;
    }

    //: Package manager - cancel button text for download progress dialog
    auto progress = new QProgressDialog(tr("Downloading packages..."), tr("Cancel"), 0, 0, this);
    progress->setWindowModality(Qt::WindowModal);
    progress->setAutoClose(true);
    progress->setMinimumDuration(0);
    progress->show();

    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    auto pendingDownloads = std::make_shared<QHash<QString, QString>>();
    auto remainingDownloads = std::make_shared<int>(selected.size());
    auto activeReplies = std::make_shared<QList<QNetworkReply*>>();
    auto cancelled = std::make_shared<bool>(false);
    bool repoError = false;

    QObject::connect(progress, &QProgressDialog::canceled, [activeReplies, pendingDownloads, manager, progress, cancelled]() {
        *cancelled = true;
        for (QNetworkReply* reply : *activeReplies) {
            if (reply) {
                reply->abort();
            }
        }
        for (const QString& filePath : pendingDownloads->values()) {
            QFile::remove(filePath);
        }
        pendingDownloads->clear();
        manager->deleteLater();
        progress->deleteLater();
    });

    for (QListWidgetItem* item : selected) {
        const QString packageName = item->text();

        QJsonObject foundObj;
        if (packageLookup.contains(packageName)) {
            foundObj = packageLookup.value(packageName);
        }

        if (foundObj.isEmpty()) {
            //: Package manager: package couldn't be downloaded
            QMessageBox::warning(this, tr("Installation Failed"), tr("Package '%1' not found in repository").arg(packageName));
            repoError = true;
            continue;
        }

        QString remoteFileName = foundObj.value("filename").toString();
        remoteFileName = QFileInfo(remoteFileName).fileName();
        if (remoteFileName.isEmpty()) {
            //: Package manager: package couldn't be downloaded
            QMessageBox::warning(this, tr("Installation Failed"), tr("Package '%1' not found in repository").arg(packageName));
            repoError = true;
            continue;
        }

        const QByteArray encoded = QUrl::toPercentEncoding(remoteFileName);
        const QString outDir = mudlet::getMudletPath(enums::profileHomePath, mpHost->getName());
        const QString outPath = outDir + QDir::separator() + remoteFileName;
        QNetworkRequest request(QUrl(qsl("https://github.com/Mudlet/mudlet-package-repository/raw/refs/heads/main/packages/%1").arg(QString::fromUtf8(encoded))));
        request.setTransferTimeout(30000);
        QNetworkReply* reply = manager->get(request);
        activeReplies->append(reply);

        QFile* file = new QFile(outPath);
        if (!file->open(QIODevice::WriteOnly)) {
            (*remainingDownloads.get())--;
            file->deleteLater();
            reply->deleteLater();
            continue;
        }

        QObject::connect(reply, &QNetworkReply::readyRead, [file, reply]() {
            const QByteArray data = reply->readAll();
            if (file->write(data) != data.size()) {
                qWarning() << "dlgPackageManager::slot_installMultiple() ERROR - failed to write downloaded data:" << file->errorString();
                reply->abort();
            }
        });

        pendingDownloads->insert(packageName, outPath);

        QObject::connect(reply, &QNetworkReply::finished, this, [reply, file, this, outPath, packageName, pendingDownloads, remainingDownloads, manager, progress, cancelled, activeReplies]() {
            const QByteArray data = reply->readAll();
            if (!data.isEmpty() && file->write(data) != data.size()) {
                qWarning() << "dlgPackageManager::slot_installMultiple() ERROR - failed to write final data:" << file->errorString();
            }
            file->close();
            reply->deleteLater();
            file->deleteLater();

            activeReplies->removeOne(reply);

            if (*cancelled) {
                QFile::remove(outPath);
                return;
            }

            if (reply->error() != QNetworkReply::NoError) {
                //: Package manager: network error, package couldn't be downloaded
                QMessageBox::warning(this, tr("Installation Failed"), tr("Package '%1' could not be downloaded due to a network error").arg(packageName));
                pendingDownloads->remove(packageName);
                (*remainingDownloads.get())--;
            }

            if (--(*remainingDownloads.get()) == 0) {
                for (auto it = pendingDownloads->begin(); it != pendingDownloads->end(); ++it) {
                    const QString& packageName = it.key();
                    const QString& filePath = it.value();

                    if (mpHost) {
                        // Uninstall existing package first if this is an update
                        if (mpHost->mInstalledPackages.contains(packageName)) {
                            mpHost->uninstallPackage(packageName, enums::PackageModuleType::Package);
                        }
                        mpHost->installPackage(filePath, enums::PackageModuleType::Package);
                    }
                    QFile::remove(filePath);
                }

                progress->reset();
                progress->close();
                progress->deleteLater();
                manager->deleteLater();

                resetPackageList();
            }
        });
    }

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

    QString packageName = pItem->text();

    if (mCurrentView == NavigationView::Installed) {
        auto packageInfo{mpHost->mPackageInfo.value(packageName)};
        if (packageInfo.isEmpty()) {
            packageDescription->clear();
            return;
        }

        if (packageLookup.contains(packageName)) {
            pushButton_website->show();
            pushButton_report->show();
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
            label_icon->setPixmap(QPixmap(iconDir).scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            QPixmap pixmap(":/icons/package-manager.png");
            label_icon->setPixmap(pixmap.scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }

        fillPackageDetails(packageName, packageInfo.value(qsl("title")), packageInfo.value(qsl("author")), packageInfo.value(qsl("version")));

    } else if (mCurrentView == NavigationView::Explore) {
        pushButton_website->show();
        pushButton_report->show();
        downloadIcon(packageName);

        if (packageLookup.contains(packageName)) {
            QJsonObject packageObj = packageLookup.value(packageName);
            fillPackageDetails(
                    packageObj.value(qsl("mpackage")).toString(), packageObj.value(qsl("title")).toString(), packageObj.value(qsl("author")).toString(), packageObj.value(qsl("version")).toString());
            packageDescription->setMarkdown(packageObj.value(qsl("description")).toString());
        }
    } else if (mCurrentView == NavigationView::Updates) {
        pushButton_website->show();
        pushButton_report->show();
        downloadIcon(packageName);

        if (packageLookup.contains(packageName)) {
            QJsonObject packageObj = packageLookup.value(packageName);
            const auto packageInfo = mpHost->mPackageInfo.value(packageName);
            const QString installedVersion = packageInfo.value(qsl("version"));
            const QString repoVersion = packageObj.value(qsl("version")).toString();

            fillPackageDetails(packageObj.value(qsl("mpackage")).toString(), packageObj.value(qsl("title")).toString(), packageObj.value(qsl("author")).toString(), repoVersion);

            //: Package manager - version update indicator showing old and new versions
            label_version->setText(tr("Version %1 → %2").arg(installedVersion, repoVersion));

            packageDescription->setMarkdown(packageObj.value(qsl("description")).toString());
        }
    }
}

void dlgPackageManager::slot_navigationButtonClicked(int buttonId)
{
    mCurrentView = static_cast<NavigationView>(buttonId);
    lineEdit_searchBar->clear();
    slot_setPackageList();
}

void dlgPackageManager::slot_onIconDownloaded(QNetworkReply* reply)
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

    for (const QString& package : std::as_const(removePackages)) {
        mpHost->uninstallPackage(package, enums::PackageModuleType::Package);
    }

    populatePackagesWithUpdates();
    updateUpdatesBadge();
}

void dlgPackageManager::slot_searchTextChanged(const QString& searchText)
{
    packageList->clear();

    if (mCurrentView == NavigationView::Installed) {
        for (const auto& value : std::as_const(mpHost->mPackageInfo)) {
            const QString name = value.value(qsl("mpackage"));
            const QString title = value.value(qsl("title"));
            const QString description = value.value(qsl("description"));
            const QString author = value.value(qsl("author"));

            if (name.contains(searchText, Qt::CaseInsensitive) || title.contains(searchText, Qt::CaseInsensitive) || description.contains(searchText, Qt::CaseInsensitive)
                || author.contains(searchText, Qt::CaseInsensitive)) {
                QListWidgetItem* item = new QListWidgetItem(name);
                if (!title.isEmpty()) {
                    item->setData(Qt::UserRole, title);
                }
                const auto iconName = value.value(qsl("icon"));
                if (!iconName.isEmpty()) {
                    const auto iconDir = mudlet::getMudletPath(enums::profileDataItemPath, mpHost->getName(), qsl("%1/.mudlet/Icon/%2").arg(name, iconName));
                    item->setIcon(QIcon(iconDir));
                } else {
                    QPixmap emptyPixmap(16, 16);
                    emptyPixmap.fill(Qt::transparent);
                    item->setIcon(QIcon(emptyPixmap));
                }
                packageList->addItem(item);
            }
        }
    } else if (mCurrentView == NavigationView::Explore) {
        for (const QJsonValue& value : std::as_const(repositoryPackages)) {
            const QJsonObject pkg = value.toObject();
            const QString name = pkg[qsl("mpackage")].toString();
            const QString title = pkg[qsl("title")].toString();
            const QString description = pkg[qsl("description")].toString();
            const QString author = pkg[qsl("author")].toString();

            if (name.contains(searchText, Qt::CaseInsensitive) || title.contains(searchText, Qt::CaseInsensitive) || description.contains(searchText, Qt::CaseInsensitive)
                || author.contains(searchText, Qt::CaseInsensitive)) {
                QListWidgetItem* item = new QListWidgetItem(name);
                if (!title.isEmpty()) {
                    item->setData(Qt::UserRole, title);
                }
                if (pkg.contains(qsl("icon"))) {
                    const QPixmap pixmap(pkg[qsl("icon")].toString());
                    item->setIcon(QIcon(pixmap));
                }
                packageList->addItem(item);
            }
        }
    } else if (mCurrentView == NavigationView::Updates) {
        for (const QString& packageName : std::as_const(mPackagesWithUpdates)) {
            const auto packageInfo = mpHost->mPackageInfo.value(packageName);
            const QString title = packageInfo.value(qsl("title"));
            const QString description = packageInfo.value(qsl("description"));
            const QString author = packageInfo.value(qsl("author"));

            if (packageName.contains(searchText, Qt::CaseInsensitive) || title.contains(searchText, Qt::CaseInsensitive) || description.contains(searchText, Qt::CaseInsensitive)
                || author.contains(searchText, Qt::CaseInsensitive)) {
                QListWidgetItem* item = new QListWidgetItem(packageName);
                if (!title.isEmpty()) {
                    item->setData(Qt::UserRole, title);
                }
                const auto iconName = packageInfo.value(qsl("icon"));
                if (!iconName.isEmpty()) {
                    const auto iconDir = mudlet::getMudletPath(enums::profileDataItemPath, mpHost->getName(), qsl("%1/.mudlet/Icon/%2").arg(packageName, iconName));
                    item->setIcon(QIcon(iconDir));
                } else {
                    QPixmap emptyPixmap(16, 16);
                    emptyPixmap.fill(Qt::transparent);
                    item->setIcon(QIcon(emptyPixmap));
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

    if (!lineEdit_searchBar->text().isEmpty()) {
        slot_searchTextChanged(lineEdit_searchBar->text());
        return;
    }

    packageList->clear();
    clearPackageDetails();

    if (mCurrentView == NavigationView::Installed) {
        for (int i = 0; i < mpHost->mInstalledPackages.size(); i++) {
            auto item = new QListWidgetItem();
            item->setText(mpHost->mInstalledPackages.at(i));
            const auto packageInfo{mpHost->mPackageInfo.value(item->text())};
            const auto iconName = packageInfo.value(qsl("icon"));
            const auto title = packageInfo.value(qsl("title"));
            if (!title.isEmpty()) {
                item->setData(Qt::UserRole, title);
            }
            if (!iconName.isEmpty()) {
                const auto iconDir = mudlet::getMudletPath(enums::profileDataItemPath, mpHost->getName(), qsl("%1/.mudlet/Icon/%2").arg(mpHost->mInstalledPackages.at(i), iconName));
                item->setIcon(QIcon(iconDir));
            } else {
                QPixmap emptyPixmap(16, 16);
                emptyPixmap.fill(Qt::transparent);
                item->setIcon(QIcon(emptyPixmap));
            }
            packageList->addItem(item);
        }
    } else if (mCurrentView == NavigationView::Explore) {
        if (!readPackageRepositoryFile()) {
            return;
        }
        for (const QJsonValue& packageVal : std::as_const(repositoryPackages)) {
            auto item = new QListWidgetItem();
            const QJsonObject packageObj = packageVal.toObject();
            const QString packageName = packageObj.value("mpackage").toString();
            const QString title = packageObj.value("title").toString();
            item->setText(packageName);
            if (!title.isEmpty()) {
                item->setData(Qt::UserRole, title);
            }
            packageList->addItem(item);
        }
    } else if (mCurrentView == NavigationView::Updates) {
        populatePackagesWithUpdates();

        if (mPackagesWithUpdates.isEmpty()) {
            //: Package manager - message shown in description area when no updates are available
            packageDescription->setPlainText(tr("All packages are up to date."));
        }

        for (const QString& packageName : std::as_const(mPackagesWithUpdates)) {
            auto item = new QListWidgetItem();
            item->setText(packageName);
            const auto packageInfo{mpHost->mPackageInfo.value(packageName)};
            const auto iconName = packageInfo.value(qsl("icon"));
            const auto title = packageInfo.value(qsl("title"));
            if (!title.isEmpty()) {
                item->setData(Qt::UserRole, title);
            }
            if (!iconName.isEmpty()) {
                const auto iconDir = mudlet::getMudletPath(enums::profileDataItemPath, mpHost->getName(), qsl("%1/.mudlet/Icon/%2").arg(packageName, iconName));
                item->setIcon(QIcon(iconDir));
            } else {
                QPixmap emptyPixmap(16, 16);
                emptyPixmap.fill(Qt::transparent);
                item->setIcon(QIcon(emptyPixmap));
            }
            packageList->addItem(item);
        }
    }

    packageList->setCurrentRow(0);
}

void dlgPackageManager::slot_toggleInstallRepoButton()
{
    if (mCurrentView == NavigationView::Explore || mCurrentView == NavigationView::Updates) {
        const QList selection = packageList->selectedItems();
        const int selectionCount = selection.size();
        pushButton_installRepo->setEnabled(selectionCount);

        if (mCurrentView == NavigationView::Updates) {
            if (selectionCount) {
                //: Message on button in package manager to update one or multiple (%n is the count) selected packages.
                pushButton_installRepo->setText(tr("Update (%n)", nullptr, selectionCount));
            } else {
                //: Message on button in package manager when there are no selected packages - button will also be disabled.
                pushButton_installRepo->setText(tr("Update"));
            }
            //: Tooltip for button in package manager when in Updates view
            pushButton_installRepo->setToolTip(tr("Update selected packages"));
        } else {
            if (selectionCount) {
                //: Message on button in package manager to install one or multiple (%n is the count) selected packages.
                pushButton_installRepo->setText(tr("Install (%n)", nullptr, selectionCount));
            } else {
                //: Message on button in package manager when there are no selected packages - button will also be disabled.
                pushButton_installRepo->setText(tr("Install"));
            }
            //: Tooltip for button in package manager when in Explore view
            pushButton_installRepo->setToolTip(tr("Install package from repository"));
        }
    } else {
        //: Message on button in package manager initially and when the view is the "Installed" one
        pushButton_installRepo->setText(tr("Install"));
        pushButton_installRepo->setEnabled(false);
        //: Tooltip for button in package manager when in Explore view
        pushButton_installRepo->setToolTip(tr("Install package from repository"));
    }
}

void dlgPackageManager::slot_toggleRemoveButton()
{
    if (mCurrentView == NavigationView::Installed) {
        const QList selection = packageList->selectedItems();
        const int selectionCount = selection.size();
        pushButton_remove->setEnabled(selectionCount);
        if (selectionCount) {
            //: Message on button in package manager to remove one or multiple (%n is the count) selected packages.
            pushButton_remove->setText(tr("Remove (%n)", nullptr, selectionCount));
        } else {
            //: Message on button in package manager when there are no selected packages - button will also be disabled.
            pushButton_remove->setText(tr("Remove"));
        }
    } else {
        //: Message on button in package manager initially and when the view is NOT the "Installed" one
        pushButton_remove->setText(tr("Remove"));
        pushButton_remove->setEnabled(false);
    }
}

void dlgPackageManager::showImportStatus(const QString& message)
{
    label_importStatus->setText(message);
    label_importStatus->setStyleSheet(qsl("QLabel { padding: 8px; }"));
    label_importStatus->show();
    QTimer::singleShot(4s, label_importStatus, &QWidget::hide);
}

void dlgPackageManager::updateUpdatesBadge()
{
    const int count = mPackagesWithUpdates.size();
    if (count) {
        //: Package manager - navigation button showing one or more available updates
        pushButton_updates->setText(tr("Updates (%1)").arg(count));
    } else {
        //: Package manager - navigation button for when there are no updates
        pushButton_updates->setText(tr("Updates"));
    }
}

void dlgPackageManager::closeEvent(QCloseEvent* event)
{
    if (mudlet::self() && !mudlet::self()->isGoingDown() && mpHost && !mpHost->isClosingDown()) {
        emit packageManagerClosing(mpHost->getName());
    }
    QDialog::closeEvent(event);
}
