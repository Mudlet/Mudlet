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
    resetPackageTable();
    connect(mPackageTable, &QTableWidget::itemClicked, this, &dlgPackageManager::slot_item_clicked);
    connect(mInstallButton, &QAbstractButton::clicked, this, &dlgPackageManager::slot_install_package);
    connect(mpHost->mpConsole, &QWidget::destroyed, this, &dlgPackageManager::close);
    connect(mPackageTable, &QTableWidget::currentItemChanged, this, &dlgPackageManager::slot_item_clicked);
    setWindowTitle(tr("Package Manager - %1").arg(mpHost->getName()));
    ui->additionalDetails->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->additionalDetails->setFocusPolicy(Qt::NoFocus);
    ui->additionalDetails->setSelectionMode(QAbstractItemView::NoSelection);
    mPackageTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mPackageTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mPackageTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    setAttribute(Qt::WA_DeleteOnClose);
}

dlgPackageManager::~dlgPackageManager()
{
    mpHost->mpPackageManager = nullptr;
    delete ui;
}

void dlgPackageManager::resetPackageTable()
{
    if (!mpHost) {
        return;
    }
    for (int i =  mPackageTable->rowCount() - 1; i >= 0; --i) {
        mPackageTable->removeRow(i);
    }

    mPackageTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    for (int i = 0; i < mpHost->mInstalledPackages.size(); i++) {
        QPushButton* remove_btn = new QPushButton();
        connect(remove_btn, &QPushButton::clicked, [=] { slot_uninstall_package(i); });
        remove_btn->setText(QStringLiteral("  Remove  "));
        remove_btn->setStyleSheet("text-align: center; margin-top:10%; margin-bottom:10%;");
        mPackageTable->insertRow(i);
        auto packageName = new QTableWidgetItem();
        auto shortDescription = new QTableWidgetItem();
        packageName->setTextAlignment(Qt::AlignCenter);
        QFont nameFont;
        nameFont.setBold(true);
        packageName->setFont(nameFont);
        shortDescription->setTextAlignment(Qt::AlignCenter);
        packageName->setText(mpHost->mInstalledPackages.at(i));
        auto packageInfo{mpHost->mLuaInterpreter.getPackageInfo(packageName->text())};
        auto iconName = packageInfo.value(QStringLiteral("icon"));
        auto iconDir = iconName.isEmpty() ? QStringLiteral(":/icons/mudlet.png")
                                          : mudlet::getMudletPath(mudlet::profileDataItemPath, mpHost->getName(), QStringLiteral("%1/Icon/%2").arg(packageName->text(), iconName));
        packageName->setIcon(QIcon(iconDir));
        auto title = packageInfo.value(QStringLiteral("title"));
        shortDescription->setText(title);
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
    //resetPackageTable();
}

void dlgPackageManager::slot_uninstall_package(int index)
{
    auto package = mPackageTable->item(index, 0);
    mpHost->uninstallPackage(package->text(), 0);
    //resetPackageTable();
}

void dlgPackageManager::slot_item_clicked(QTableWidgetItem* pItem)
{
    if (!pItem) {
        return;
    }

    //clear details Table
    for (int i = ui->additionalDetails->rowCount() - 1; i >= 0; --i) {
        ui->additionalDetails->removeRow(i);
    }
    QString packageName = ui->packageTable->item(pItem->row(), 0)->text();
    auto packageInfo{mpHost->mLuaInterpreter.getPackageInfo(packageName)};
    if (packageInfo.isEmpty()) {
        ui->packageDescription->clear();
        return;
    }
    packageInfo.remove(QStringLiteral("mpackage"));
    packageInfo.remove(QStringLiteral("icon"));
    packageInfo.remove(QStringLiteral("title"));

    QString description = packageInfo.take(QStringLiteral("description"));
    QString packageDir = mudlet::self()->getMudletPath(mudlet::profileDataItemPath, mpHost->getName(), packageName);
    description.replace(QLatin1String("$packagePath"), packageDir);
#if (QT_VERSION) >= (QT_VERSION_CHECK(5, 14, 0))
    ui->packageDescription->setMarkdown(description);
#else
    ui->packageDescription->setText(description);
#endif

    QStringList labelText, details;
    labelText << tr("Author") << tr("Version") << tr("Created") << tr("Dependencies");
    details << QStringLiteral("author") << QStringLiteral("version") << QStringLiteral("created") << QStringLiteral("dependencies");
    int counter = 0;
    for (int i = 0; i < details.size(); i++) {
        QString valueText{packageInfo.take(details.at(i))};
        if (valueText.isEmpty()) {
            continue;
        }
        QLabel* info = new QLabel();
        QLabel* value = new QLabel();
        info->setEnabled(false);
        ui->additionalDetails->insertRow(counter);
        ui->additionalDetails->setCellWidget(counter, 0, info);
        ui->additionalDetails->setCellWidget(counter++, 1, value);
        info->setText(labelText.at(i));
        value->setText(valueText);
        value->setTextInteractionFlags(Qt::TextSelectableByMouse);
        value->setAlignment(Qt::AlignCenter);
    }

    if (!packageInfo.isEmpty()) {
        fillAdditionalDetails(packageInfo);
    }
    ui->additionalDetails->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}


void dlgPackageManager::fillAdditionalDetails(const QMap<QString, QString>& packageInfo)
{
    QMap<QString, QString>::const_iterator iter = packageInfo.constBegin();
    int counter = ui->additionalDetails->rowCount();
    while (iter != packageInfo.constEnd()) {
        QLabel* info = new QLabel();
        QLabel* value = new QLabel();
        info->setEnabled(false);
        ui->additionalDetails->insertRow(counter);
        ui->additionalDetails->setCellWidget(counter, 0, info);
        ui->additionalDetails->setCellWidget(counter++, 1, value);
        info->setText(iter.key());
        value->setText(iter.value());
        value->setOpenExternalLinks(true);
        value->setTextInteractionFlags(Qt::TextSelectableByMouse|Qt::LinksAccessibleByMouse);
        value->setAlignment(Qt::AlignCenter);
        ++iter;
    }
}
