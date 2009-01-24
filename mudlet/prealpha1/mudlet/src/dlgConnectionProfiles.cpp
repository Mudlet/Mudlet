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

#include <QInputDialog>
#include <QMessageBox>

#include "dlgConnectionProfiles.h"
#include "Host.h"
#include "HostManager.h"

dlgConnectionProfiles::dlgConnectionProfiles(QWidget * parent) : QDialog(parent)
{
    setupUi( this );
    active_profile.clear();
    active_item = NULL;
    QPushButton *connect_button = dialog_buttonbox->addButton(tr("Connect"), QDialogButtonBox::AcceptRole);
    connect_button->setIcon(QIcon(":/dialog-ok-apply.png"));
    connect( new_profile_button, SIGNAL( pressed() ), this, SLOT( slot_addProfile() ) );
    connect( remove_profile_button, SIGNAL( pressed() ), this, SLOT( slot_deleteProfile() ) );
    connect( mud_info_groupbox, SIGNAL ( clicked ( bool ) ), this, SLOT ( slot_showmudlist_clicked ( bool ) )) ;

    connect( this, SIGNAL( update() ), this, SLOT( slot_update() ) );
    connect( profiles_tree_widget, SIGNAL( currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), SLOT( slot_item_changed(QTreeWidgetItem *, QTreeWidgetItem *) ) );
    connect( profiles_tree_widget, SIGNAL( itemDoubleClicked( QTreeWidgetItem *, int ) ), this, SLOT ( slot_connection_dlg_finnished() ) );
    connect( this, SIGNAL (accept()), this, SLOT (slot_connection_dlg_finnished()));
    connect( this, SIGNAL (finished(int)), this, SLOT (slot_finished(int)));
}

void dlgConnectionProfiles::slot_showmudlist_clicked ( bool checked )
{
    if (checked)
        mud_treewidget->show();
    else
        mud_treewidget->hide();
}

void dlgConnectionProfiles::slot_addProfile()
{
    QString newname = tr("new");

    if (HostManager::self()->getHost( newname )) {
        int inc=2;
        QString trick;
        do
        {
            trick = newname + "_" + QString::number(inc++);
        } while (HostManager::self()->getHost( trick ));
        newname = trick;
    }

    do
    {
        newname = QInputDialog::getText(this, tr("Profile name"), tr("Enter profile name"), QLineEdit::Normal, newname);
        if (HostManager::self()->getHost( newname ))
            QMessageBox::information( this, tr("Profile name exist"), tr("This profile name is already taken"));
    } while (!newname.isEmpty() && HostManager::self()->getHost( newname ));

    if (newname.isEmpty())
        return;

    QStringList sList;
    sList << newname;
    QTreeWidgetItem * pItem = new QTreeWidgetItem( (QTreeWidgetItem *)0, sList);
    profiles_tree_widget->insertTopLevelItem( profiles_tree_widget->topLevelItemCount(), pItem );    
    HostManager::self()->addHost( newname, QString("23"), QString(""), QString("") );
    //Host * pHost = HostManager::self()->getHost( newname );

    profiles_tree_widget->setItemSelected(profiles_tree_widget->currentItem(), false); // Unselect previous item
    profiles_tree_widget->setCurrentItem(pItem);
    profiles_tree_widget->setItemSelected(pItem, true);
    slot_item_changed(pItem, NULL);
    profiles_tree_widget->sortByColumn(0, Qt::AscendingOrder);

    welcome_message->hide();
    basic_info_groupbox->show();
    autologin_groupbox->show();
    mud_info_groupbox->show();
}

void dlgConnectionProfiles::slot_deleteProfile()
{
    if( ! profiles_tree_widget->currentItem() ) return;

    QString profile = profiles_tree_widget->currentItem()->text( 0 );
    if ( QMessageBox::question(this, tr("Confirmation"), tr("Are you sure you want to delete %1 ?").arg( profile ), QMessageBox::Yes|QMessageBox::No, QMessageBox::No) == QMessageBox::No )
        return;
    
    HostManager::self()->deleteHost( profile );
    profiles_tree_widget->takeTopLevelItem( profiles_tree_widget->currentIndex().row() );
    QStringList hostList = HostManager::self()->getHostList();
    if( hostList.size() > 0 )
        slot_item_changed(profiles_tree_widget->currentItem(), NULL);
    else {
        active_profile.clear();
        active_item = NULL;
        welcome_message->show();
        basic_info_groupbox->hide();
        autologin_groupbox->hide();
        mud_info_groupbox->hide();
    }
    profiles_tree_widget->setFocus();
}

void dlgConnectionProfiles::slot_item_changed(QTreeWidgetItem *pItem, QTreeWidgetItem *previousItem) {
    if (pItem)
    {
        save();
        QString profile_name = pItem->text( 0 );
        active_profile = profile_name;
        active_item = pItem;
        Host * pHost = HostManager::self()->getHost( profile_name );
        profile_name_entry->setText( profile_name );
        profile_name_entry->setCursorPosition(0);
        host_name_entry->setText( pHost->getUrl() );        
        character_password_entry->setText( pHost->getPass() );
        login_entry->setText( pHost->getLogin() );
        port_entry->setText( QString::number(pHost->getPort()) );;
    }
}

void dlgConnectionProfiles::fillout_form()
{
    QStringList hostList = HostManager::self()->getHostList();
    qDebug()<<"hostList="<<hostList<<"hostList.size()="<<hostList.size();
    if( hostList.size() < 1 ) 
    {
        welcome_message->show();
        basic_info_groupbox->hide();
        autologin_groupbox->hide();
        mud_info_groupbox->hide();
    }
    else
    {
        welcome_message->hide();
        basic_info_groupbox->show();
        autologin_groupbox->show();
        mud_info_groupbox->show();
    }
    for( int i=0; i<hostList.size(); i++ )
    {
        QString s = hostList[i];
        if( s.size() < 1 ) continue;
        //Host * pHost = HostManager::self()->getHost( hostList[i] );
        QStringList sList;
        sList << hostList[i];
        QTreeWidgetItem * pItem = new QTreeWidgetItem( (QTreeWidgetItem *)0, sList);
        profiles_tree_widget->insertTopLevelItem( profiles_tree_widget->topLevelItemCount(), pItem );    
    }
    profiles_tree_widget->sortByColumn(0, Qt::AscendingOrder);
}

void dlgConnectionProfiles::slot_connection_dlg_finnished()
{
    QDialog::accept();
}

void dlgConnectionProfiles::save()
{
    if (active_profile.isEmpty())
        return;

    QString profile_name = profile_name_entry->text().trimmed();
    QString url = host_name_entry->text().trimmed();
    QString pass = character_password_entry->text().trimmed();
    QString login = login_entry->text().trimmed();
    int port = port_entry->text().trimmed().toInt();
    QString website = website_entry->text().trimmed();

    if (Host *pHost = HostManager::self()->getHost( active_profile )) {
        pHost->setName( profile_name );
        pHost->setUrl( url );
        pHost->setPass( pass );
        pHost->setLogin( login );
        pHost->setPort( port );
    }

    if ( profile_name != active_profile ) {
        HostManager::self()->renameHost(active_profile);
        active_item->setText( 0, profile_name);
    }
}

void dlgConnectionProfiles::slot_finished(int f)
{
    save();
    if (f == 1)
    {
        QString profile_name = profile_name_entry->text().trimmed();
        if( profile_name.size() < 1 ) return;
        emit signal_establish_connection( profile_name );
    }
}

void dlgConnectionProfiles::slot_update()
{
    update();      
}

