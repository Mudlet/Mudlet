#ifndef MUDLET_DLGPACKAGEMANAGER_H
#define MUDLET_DLGPACKAGEMANAGER_H

/***************************************************************************
 *   Copyright (C) 2011 by Heiko Koehn - KoehnHeiko@googlemail.com         *
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
    void fillAdditionalDetails(const QMap<QString, QString>&);

    Ui::package_manager* ui = nullptr;
    Host* mpHost = nullptr;
    QTableWidget* mPackageTable = nullptr;
    QTableWidget* mDetailsTable = nullptr;
    QTextBrowser* mDescription = nullptr;
    QPushButton* mInstallButton = nullptr;
    QPushButton* mRemoveButton = nullptr;
};

#endif
