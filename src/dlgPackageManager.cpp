/***************************************************************************
 *   Copyright (C) 2021 by Manuel Wegmann - wegmann.manuel@yahoo.com       *
 *   Copyright (C) 2011 by Heiko Koehn - KoehnHeiko@googlemail.com         *
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
#include "ui_package_manager.h"

#include <QFileDialog>
#include <QMessageBox>


dlgPackageManager::dlgPackageManager(QWidget* parent, Host* pHost)
: QDialog(parent)
, ui(new Ui::package_manager)
, mpHost(pHost)
{
    ui->setupUi(this);
    mPackageList = ui->packageList;
    mUninstallButton = ui->uninstallButton;
    mInstallButton = ui->installButton;
    mPackageList->addItems(mpHost->mInstalledPackages);
    connect(mUninstallButton, &QAbstractButton::clicked, this, &dlgPackageManager::slot_uninstall_package);
    connect(mInstallButton, &QAbstractButton::clicked, this, &dlgPackageManager::slot_install_package);
    connect(mpHost->mpConsole, &QWidget::destroyed, this, &dlgPackageManager::close);
    setWindowTitle(tr("Package Manager - %1").arg(mpHost->getName()));
    setAttribute(Qt::WA_DeleteOnClose);
}

dlgPackageManager::~dlgPackageManager()
{
    mpHost->mpPackageManager = nullptr;
    delete ui;
}

void dlgPackageManager::slot_install_package()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Import Mudlet Package"), QDir::currentPath());
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Import Mudlet Package:"), tr("Cannot read file %1:\n%2.").arg(fileName, file.errorString()));
        return;
    }

    mpHost->installPackage(fileName, 0);
    mPackageList->clear();
    mPackageList->addItems(mpHost->mInstalledPackages);
}

void dlgPackageManager::slot_uninstall_package()
{
    auto selectedPackages = mPackageList->selectedItems();
    if (!selectedPackages.empty()) {
        for (auto package : selectedPackages) {
            mpHost->uninstallPackage(package->text(), 0);
        }
    }
    mPackageList->clear();
    mPackageList->addItems(mpHost->mInstalledPackages);
}
