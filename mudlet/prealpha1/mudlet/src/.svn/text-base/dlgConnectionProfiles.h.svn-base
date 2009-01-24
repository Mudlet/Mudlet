/***************************************************************************
 *   Copyright (C) 2008 by Heiko Koehn   *
 *   KoehnHeiko@googlemail.com   *
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

#ifndef DLGCONNECTION_PROFILES_H
#define DLGCONNECTION_PROFILES_H

#include "ui_connection_profiles.h"


class dlgConnectionProfiles : public QDialog , public Ui::profile_dialog
{
Q_OBJECT

public:
   
         dlgConnectionProfiles(QWidget * parent = 0);
    void fillout_form();
    
signals:
     
    void signal_establish_connection( QString profile_name );
    void accept();
    void update();
    
public slots:
      
    void slot_item_changed( QTreeWidgetItem *, QTreeWidgetItem * );
    void slot_update();
    void slot_addProfile();
    void slot_deleteProfile();
    void slot_connection_dlg_finnished();
    void slot_showmudlist_clicked ( bool checked );
    void slot_finished ( int f );
private:
    QString active_profile;
    void save();
};

#endif

