#ifndef MUDLET_DLGPACKAGEMANAGER_H
#define MUDLET_DLGPACKAGEMANAGER_H

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


#include "Host.h"

#include "pre_guard.h"
#include <QDialog>
#include <QTableWidget>
#include <QTextBrowser>
#include "post_guard.h"

class Host;
namespace Ui {
class package_manager;
}

class dlgPackageManager : public QDialog
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgPackageManager)
    explicit dlgPackageManager(QWidget* parent, Host*);
    ~dlgPackageManager();
    void resetPackageTable();

private slots:
    void slot_install_package();
    void slot_remove_packages();
    void slot_item_clicked(QTableWidgetItem*);
    void slot_toggle_remove_button();

private:
    Ui::package_manager* ui;
    Host* mpHost;
    QTableWidget* mPackageTable;
    QTableWidget* mDetailsTable;
    QTextBrowser* mDescription;
    QPushButton* mInstallButton;
    QPushButton* mRemoveButton;
    void fillAdditionalDetails(const QMap<QString, QString>&);
};

#endif
