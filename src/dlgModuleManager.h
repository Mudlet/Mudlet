#ifndef MUDLET_DLGMODULEMANAGER_H
#define MUDLET_DLGMODULEMANAGER_H

/***************************************************************************
 *   Copyright (C) 2011 by Chris Mitchell                                  *
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


#include "Host.h"

#include "ui_module_manager.h"
#include <QDialog>
#include <QCloseEvent>

class dlgModuleManager : public QDialog, public Ui::module_manager
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgModuleManager)
    explicit dlgModuleManager(QWidget* parent, Host*);
    ~dlgModuleManager() override;

    void layoutModules();

private slots:
    void slot_installModule();
    void slot_uninstallModule();
    void slot_helpModule();
    void slot_moduleClicked(QTableWidgetItem*);
    void slot_moduleChanged(QTableWidgetItem*);

signals:
    void moduleManagerClosing(const QString& profileName);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    Host* mpHost = nullptr;
};

#endif
