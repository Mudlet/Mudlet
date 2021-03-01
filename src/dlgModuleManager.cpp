/***************************************************************************
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


#include "dlgModuleManager.h"
#include "ui_module_manager.h"

#include <QFileDialog>
#include <QMessageBox>


dlgModuleManager::dlgModuleManager(QWidget* parent, Host* pHost)
: QDialog(parent)
, ui(new Ui::module_manager)
, mpHost(pHost)
{
    ui->setupUi(this);


    mModuleTable = ui->moduleTable;
    mModuleUninstallButton = ui->uninstallButton;
    mModuleInstallButton = ui->installButton;
    mModuleHelpButton = ui->helpButton;

    layoutModules();
    connect(mModuleUninstallButton, &QAbstractButton::clicked, this, &mudlet::slot_uninstall_module);
    connect(mModuleInstallButton, &QAbstractButton::clicked, this, &mudlet::slot_install_module);
    connect(mModuleHelpButton, &QAbstractButton::clicked, this, &mudlet::slot_help_module);
    connect(mModuleTable, &QTableWidget::itemClicked, this, &mudlet::slot_module_clicked);
    connect(mModuleTable, &QTableWidget::itemChanged, this, &mudlet::slot_module_changed);
    connect(this, &QObject::destroyed, this, &mudlet::slot_module_manager_destroyed);
    setWindowTitle(tr("Module Manager - %1").arg(mpHost->getName()));
    setAttribute(Qt::WA_DeleteOnClose);
}

dlgModuleManager::~dlgModuleManager()
{
    delete ui;
}
