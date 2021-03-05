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
#include "TLuaInterpreter.h"
#include "mudlet.h"

#include <QFileDialog>
#include <QMessageBox>


dlgPackageManager::dlgPackageManager(QWidget* parent, Host* pHost)
: QDialog(parent)
, ui(new Ui::package_manager)
, mpHost(pHost)
{
    ui->setupUi(this);
    mPackageTable = ui->packageTable;
    mInstallButton = ui->installButton;
    fillItems();
    connect(mPackageTable, &QTableWidget::itemClicked, this, &dlgPackageManager::slot_item_clicked);
    connect(mInstallButton, &QAbstractButton::clicked, this, &dlgPackageManager::slot_install_package);
    connect(mpHost->mpConsole, &QWidget::destroyed, this, &dlgPackageManager::close);
    connect(mPackageTable, &QTableWidget::currentItemChanged, this, &dlgPackageManager::slot_item_clicked);
    setWindowTitle(tr("Package Manager - %1").arg(mpHost->getName()));
    setAttribute(Qt::WA_DeleteOnClose);
}

dlgPackageManager::~dlgPackageManager()
{
    mpHost->mpPackageManager = nullptr;
    delete ui;
}

void dlgPackageManager::fillItems()
{
    for (int i =  mPackageTable->rowCount() - 1; i >= 0; --i) {
        mPackageTable->removeRow(i);
    }
    const QString& iconPath = QStringLiteral(":/icons/mudlet.png");
    mPackageTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    for (int i = 0; i < mpHost->mInstalledPackages.size(); i++) {
        QPushButton* remove_btn = new QPushButton();
        connect(remove_btn, &QPushButton::clicked, [=] {slot_uninstall_package(i);});
        remove_btn ->setText(QStringLiteral("Remove"));
        remove_btn ->setStyleSheet(QStringLiteral("QPushButton {padding: 10px; }"));
        mPackageTable->insertRow(i);
        auto packageName = new QTableWidgetItem();
        auto shortDescription = new QTableWidgetItem();
        packageName->setTextAlignment(Qt::AlignCenter);
        QFont nameFont;
        nameFont.setBold(true);
        packageName->setFont(nameFont);
        shortDescription->setTextAlignment(Qt::AlignCenter);
        packageName->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        shortDescription->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        packageName->setIcon(QIcon(iconPath));
        packageName->setText(mpHost->mInstalledPackages.at(i));
        auto description = mpHost->mLuaInterpreter.getPackageInfo(packageName->text(), QStringLiteral("description"));
        shortDescription->setText(description);
        mPackageTable->setItem(i, 0, packageName);
        mPackageTable->setItem(i, 1, shortDescription);
        mPackageTable->setCellWidget(i, 2, remove_btn);
    }
    mPackageTable->resizeColumnsToContents();
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
    fillItems();
}

void dlgPackageManager::slot_uninstall_package(int index)
{
    auto package = mPackageTable->item(index, 0);
    mpHost->uninstallPackage(package->text(), 0);
    fillItems();
}

void dlgPackageManager::slot_item_clicked(QTableWidgetItem* pItem)
{
    if (!pItem) {
        return;
    }
    QString packageName = ui->packageTable->item(pItem->row(), 0)->text();
    QString description = mpHost->mLuaInterpreter.getPackageInfo(packageName, QStringLiteral("description"));
    QString packageDir = mudlet::getMudletPath(mudlet::profileHomePath, mpHost->getName(), QStringLiteral("/%1").arg(packageName));
    description.replace(QLatin1String("$packagePath"), packageDir);
    ui->packageDescription->setText(description);
    ui->Author->setText(mpHost->mLuaInterpreter.getPackageInfo(packageName, QStringLiteral("author")));
    ui->Version->setText(mpHost->mLuaInterpreter.getPackageInfo(packageName, QStringLiteral("version")));
    ui->Created->setText(mpHost->mLuaInterpreter.getPackageInfo(packageName, QStringLiteral("created")));
    ui->Dependencies->setText(mpHost->mLuaInterpreter.getPackageInfo(packageName, QStringLiteral("dependencies")));
}
