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
#include "mudlet.h"

#include <QFileDialog>
#include <QScrollBar>
#include <QMessageBox>


dlgPackageManager::dlgPackageManager(QWidget* parent, Host* pHost)
: QDialog(parent)
, ui(new Ui::package_manager)
, mpHost(pHost)
{
    ui->setupUi(this);
    mPackageTable = ui->packageTable;
    mInstallButton = ui->installButton;
    mRemoveButton = ui->removeButton;
    mDetailsTable = ui->additionalDetails;
    mDescription = ui->packageDescription;
    resetPackageTable();
    connect(mPackageTable, &QTableWidget::itemClicked, this, &dlgPackageManager::slot_item_clicked);
    connect(mInstallButton, &QAbstractButton::clicked, this, &dlgPackageManager::slot_install_package);
    connect(mRemoveButton, &QAbstractButton::clicked, this, &dlgPackageManager::slot_remove_packages);
    connect(mpHost->mpConsole, &QWidget::destroyed, this, &dlgPackageManager::close);
    connect(mPackageTable, &QTableWidget::currentItemChanged, this, &dlgPackageManager::slot_item_clicked);
    connect(mPackageTable, &QTableWidget::itemSelectionChanged, this, &dlgPackageManager::slot_toggle_remove_button);

    setWindowTitle(tr("Package Manager (experimental) - %1").arg(mpHost->getName()));
    mDetailsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mDetailsTable->setFocusPolicy(Qt::NoFocus);
    mDetailsTable->setSelectionMode(QAbstractItemView::NoSelection);
    mPackageTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mPackageTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mPackageTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mDetailsTable->hide();
    ui->detailsLabel->hide();
    mDescription->hide();
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
        mPackageTable->insertRow(i);
        auto packageName = new QTableWidgetItem();
        auto shortDescription = new QTableWidgetItem();
        packageName->setTextAlignment(Qt::AlignCenter);
        QFont nameFont;
        nameFont.setBold(true);
        packageName->setFont(nameFont);
        shortDescription->setTextAlignment(Qt::AlignCenter);
        packageName->setText(mpHost->mInstalledPackages.at(i));
        auto packageInfo{mpHost->mPackageInfo.value(packageName->text())};
        auto iconName = packageInfo.value(QStringLiteral("icon"));
        auto iconDir = iconName.isEmpty() ? QStringLiteral(":/icons/mudlet.png")
                                          : mudlet::getMudletPath(mudlet::profileDataItemPath, mpHost->getName(), QStringLiteral("%1/.mudlet/Icon/%2").arg(packageName->text(), iconName));
        packageName->setIcon(QIcon(iconDir));
        auto title = packageInfo.value(QStringLiteral("title"));
        shortDescription->setText(title);
        mPackageTable->setItem(i, 0, packageName);
        mPackageTable->setItem(i, 1, shortDescription);
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
}

void dlgPackageManager::slot_remove_packages()
{
    QModelIndexList selection = mPackageTable->selectionModel()->selectedRows();
    QStringList removePackages;
    for (int i = 0; i < selection.count(); i++) {
        QModelIndex index = selection.at(i);
        auto package = mPackageTable->item(index.row(), 0);
        removePackages << package->text();
    }

    for (int i = 0; i < removePackages.size(); i++) {
        mpHost->uninstallPackage(removePackages.at(i), 0);
    }

    mDetailsTable->hide();
    ui->detailsLabel->hide();
    mDescription->hide();
}

void dlgPackageManager::slot_item_clicked(QTableWidgetItem* pItem)
{
    if (!pItem) {
        return;
    }

    //clear details Table
    for (int i = mDetailsTable->rowCount() - 1; i >= 0; --i) {
        mDetailsTable->removeRow(i);
    }
    QString packageName = mPackageTable->item(pItem->row(), 0)->text();
    auto packageInfo{mpHost->mPackageInfo.value(packageName)};
    if (packageInfo.isEmpty()) {
        mDescription->clear();
        mDetailsTable->hide();
        ui->detailsLabel->hide();
        mDescription->hide();
        return;
    }
    packageInfo.remove(QStringLiteral("mpackage"));
    packageInfo.remove(QStringLiteral("icon"));
    packageInfo.remove(QStringLiteral("title"));

    QString description = packageInfo.take(QStringLiteral("description"));
    if (description.isEmpty()) {
        mDescription->hide();
    } else {
        mDescription->show();
        QString packageDir = mudlet::self()->getMudletPath(mudlet::profileDataItemPath, mpHost->getName(), packageName);
        description.replace(QLatin1String("$packagePath"), packageDir);
#if (QT_VERSION) >= (QT_VERSION_CHECK(5, 14, 0))
        mDescription->setMarkdown(description);
#else
        mDescription->setText(description);
#endif
    }

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
        mDetailsTable->insertRow(counter);
        mDetailsTable->setCellWidget(counter, 0, info);
        mDetailsTable->setCellWidget(counter++, 1, value);
        info->setText(labelText.at(i));
        info->setAlignment(Qt::AlignLeft);
        value->setText(valueText);
        value->setTextInteractionFlags(Qt::TextSelectableByMouse);
        value->setAlignment(Qt::AlignLeft);
    }

    if (!packageInfo.isEmpty()) {
        fillAdditionalDetails(packageInfo);
    }
    mDetailsTable->resizeColumnsToContents();
    mDetailsTable->resizeRowsToContents();
    mDetailsTable->horizontalHeader()->resizeSection(0, mDetailsTable->horizontalHeader()->sectionSize(0) + 10);
    if (mDetailsTable->rowCount() == 0) {
        mDetailsTable->hide();
        ui->detailsLabel->hide();
    } else {
        mDetailsTable->show();
        ui->detailsLabel->show();
    }
    int maxHeight = mDetailsTable->rowCount() * mDetailsTable->rowHeight(0);
    mDetailsTable->setMaximumHeight(maxHeight);
    mDetailsTable->verticalScrollBar()->hide();
    mPackageTable->scrollToItem(pItem);
    mPackageTable->selectRow(pItem->row());
}

void dlgPackageManager::fillAdditionalDetails(const QMap<QString, QString>& packageInfo)
{
    QMap<QString, QString>::const_iterator iter = packageInfo.constBegin();
    int counter = mDetailsTable->rowCount();
    while (iter != packageInfo.constEnd()) {
        QLabel* info = new QLabel();
        QLabel* value = new QLabel();
        info->setEnabled(false);
        mDetailsTable->insertRow(counter);
        mDetailsTable->setCellWidget(counter, 0, info);
        mDetailsTable->setCellWidget(counter++, 1, value);
        info->setText(iter.key());
        info->setAlignment(Qt::AlignLeft);
        value->setText(iter.value());
        value->setOpenExternalLinks(true);
        value->setTextInteractionFlags(Qt::TextSelectableByMouse|Qt::LinksAccessibleByMouse);
        value->setAlignment(Qt::AlignLeft);
        ++iter;
    }
}

void dlgPackageManager::slot_toggle_remove_button()
{
    QModelIndexList selection = mPackageTable->selectionModel()->selectedRows();
    int selectionCount = selection.count();
    bool haveSelection = selectionCount != 0;

    mRemoveButton->setEnabled(haveSelection);
    if (selectionCount > 1) {
        // let the translations decide whenever it should be 'Remove package', 'Remove packages', or whatever is language-appropriate
        mRemoveButton->setText(tr("Remove packages", "Button in package manager to remove selected package(s)", selectionCount));
    }
}
