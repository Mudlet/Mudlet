#ifndef MUDLET_DLGMODULEMANAGER_H
#define MUDLET_DLGMODULEMANAGER_H

/***************************************************************************
 *   Copyright (C) 2021 by Manuel Wegmann - wegmann.manuel@yahoo.com       *
 *   Copyright (C) 2011 by Chris Mitchell                                  *
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


#include "Host.h"

#include "pre_guard.h"
#include "QTableWidget"
#include <QDialog>
#include "post_guard.h"

class Host;
namespace Ui {
class module_manager;
}

class dlgModuleManager : public QDialog
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgModuleManager)
    explicit dlgModuleManager(QWidget* parent, Host*);
    ~dlgModuleManager();
     void layoutModules();
     QTableWidget* mModuleTable;

private slots:
    void slot_install_module();
    void slot_uninstall_module();
    void slot_help_module();
    void slot_module_clicked(QTableWidgetItem*);
    void slot_module_changed(QTableWidgetItem*);

private:
    Ui::module_manager* ui;
    Host* mpHost;
    QPushButton* mModuleUninstallButton;
    QPushButton* mModuleInstallButton;
    QPushButton* mModuleHelpButton;
};

#endif
