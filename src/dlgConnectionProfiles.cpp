/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn                                     *
 *   KoehnHeiko@googlemail.com                                             *
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
#include <QtUiTools>
#include "dlgConnectionProfiles.h"
#include "Host.h"
#include "HostManager.h"
#include "mudlet.h"
#include "XMLimport.h"
#include <QFileDialog>
#include <QPainter>
#include "LuaInterface.h"

#define _DEBUG_

dlgConnectionProfiles::dlgConnectionProfiles(QWidget * parent) : QDialog(parent)
{
    setupUi( this );

    // selection mode is important. if this is not set the selection behaviour is
    // undefined. this is an undocumented qt bug, as it only shows on certain OS
    // and certain architectures.

    profiles_tree_widget->setSelectionMode( QAbstractItemView::SingleSelection );

    QAbstractButton * abort = dialog_buttonbox->button( QDialogButtonBox::Cancel );
    abort->setIcon(QIcon(":/icons/dialog-close.png"));
    connect_button = dialog_buttonbox->addButton(tr("Connect"), QDialogButtonBox::AcceptRole);
    connect_button->setIcon(QIcon(":/icons/dialog-ok-apply.png"));

    connect( connect_button, SIGNAL(clicked()), this, SLOT(accept()));
    connect( abort, SIGNAL(clicked()), this, SLOT(slot_cancel()));
    connect( new_profile_button, SIGNAL( clicked() ), this, SLOT( slot_addProfile() ) );
    connect( copy_profile_button, SIGNAL( clicked() ), this, SLOT( slot_copy_profile() ) );
    connect( remove_profile_button, SIGNAL( clicked() ), this, SLOT( slot_deleteProfile() ) );
    connect( profile_name_entry, SIGNAL(textEdited(const QString)), this, SLOT(slot_update_name(const QString)));
    connect( profile_name_entry, SIGNAL(editingFinished()), this, SLOT(slot_save_name()));
    connect( host_name_entry, SIGNAL(textChanged(const QString)), this, SLOT(slot_update_url(const QString)));
    connect( port_entry, SIGNAL(textChanged(const QString)), this, SLOT(slot_update_port(const QString)));
    connect( autologin_checkBox, SIGNAL(stateChanged( int )), this, SLOT(slot_update_autologin(int)));
    connect( login_entry, SIGNAL(textEdited(const QString)), this, SLOT(slot_update_login(const QString)));
    connect( character_password_entry, SIGNAL(textEdited(const QString)), this, SLOT(slot_update_pass(const QString)));
    connect( mud_description_textedit, SIGNAL(textChanged()), this, SLOT(slot_update_description()));
    connect( this, SIGNAL( update() ), this, SLOT( slot_update() ) );
    connect( profiles_tree_widget, SIGNAL( currentItemChanged( QListWidgetItem *, QListWidgetItem * ) ), this, SLOT( slot_item_clicked( QListWidgetItem * )));
    connect( profiles_tree_widget, SIGNAL( itemDoubleClicked( QListWidgetItem * ) ), this, SLOT ( accept() ) );

    // website_entry atm is only a label
    //connect( website_entry, SIGNAL(textEdited(const QString)), this, SLOT(slot_update_website(const QString)));

    profile_name_entry->setReadOnly(true);
    notificationArea->hide();
    notificationAreaIconLabelWarning->hide();
    notificationAreaIconLabelError->hide();
    notificationAreaIconLabelInformation->hide();
    notificationAreaMessageBox->hide();

    mRegularPalette.setColor(QPalette::Text,QColor(0,0,192));
    mRegularPalette.setColor(QPalette::Highlight,QColor(0,0,192));
    mRegularPalette.setColor(QPalette::HighlightedText, QColor(255,255,255));
    mRegularPalette.setColor(QPalette::Base,QColor(255,255,255));

    mReadOnlyPalette.setColor(QPalette::Base,QColor(212,212,212));
    mReadOnlyPalette.setColor(QPalette::Text,QColor(0,0,192));
    mReadOnlyPalette.setColor(QPalette::Highlight,QColor(0,0,192));
    mReadOnlyPalette.setColor(QPalette::HighlightedText, QColor(255,255,255));

    mOKPalette.setColor(QPalette::Text,QColor(0,0,192));
    mOKPalette.setColor(QPalette::Highlight,QColor(0,0,192));
    mOKPalette.setColor(QPalette::HighlightedText, QColor(255,255,255));
    mOKPalette.setColor(QPalette::Base,QColor(235,255,235));

    mErrorPalette.setColor(QPalette::Text,QColor(0,0,192));
    mErrorPalette.setColor(QPalette::Highlight,QColor(0,0,192));
    mErrorPalette.setColor(QPalette::HighlightedText, QColor(255,255,255));
    mErrorPalette.setColor(QPalette::Base,QColor(255,235,235));

    // need to resize because the intro & error boxes get hidden
    resize(minimumSize());

    profiles_tree_widget->setViewMode(QListView::IconMode);
}

// the dialog can be accepted by pressing Enter on an qlineedit; this is a safeguard against it
// accepting invalid data
void dlgConnectionProfiles::accept()
{
    if (validateConnect())
    {
        slot_connectToServer();
        QDialog::accept();
    }
}

void dlgConnectionProfiles::slot_update_description()
{
    QListWidgetItem * pItem = profiles_tree_widget->currentItem();

    if( pItem )
    {
        QString profile = pItem->text();
        QString desc = mud_description_textedit->toPlainText();
        writeProfileData( profile, "description", desc );
    }
}

void dlgConnectionProfiles::slot_update_website( const QString url )
{
    QListWidgetItem * pItem = profiles_tree_widget->currentItem();
    if( pItem )
    {
        QString profile = pItem->text();
        writeProfileData( profile, "website", url );
    }
}

void dlgConnectionProfiles::slot_update_pass( const QString pass )
{
    QListWidgetItem * pItem = profiles_tree_widget->currentItem();
    if( pItem )
    {
        QString profile = pItem->text();
        writeProfileData( profile, "password", pass );
    }
}

void dlgConnectionProfiles::slot_update_login( const QString login )
{
    QListWidgetItem * pItem = profiles_tree_widget->currentItem();
    if( pItem )
    {
        QString profile = pItem->text();
        writeProfileData( profile, "login", login );
    }
}

void dlgConnectionProfiles::slot_update_url( const QString url )
{
    if (url == "")
    {
        validUrl = false;
        connect_button->setDisabled(true);
        return;
    }

    QListWidgetItem * pItem = profiles_tree_widget->currentItem();

    if( pItem )
    {
        QString profile = pItem->text();
        QUrl check;
        check.setHost( url );
        if( check.isValid() )
        {
            host_name_entry->setPalette( mOKPalette );
            notificationArea->hide();
            notificationAreaIconLabelWarning->hide();
            notificationAreaIconLabelError->hide();
            notificationAreaIconLabelInformation->hide();
            notificationAreaMessageBox->hide();
            validUrl = true;
            validateConnect();
            writeProfileData( profile, "url", url );
        }
        else
        {
            host_name_entry->setPalette( mErrorPalette );
            notificationArea->show();
            notificationAreaIconLabelWarning->hide();
            notificationAreaIconLabelError->show();
            notificationAreaIconLabelInformation->hide();
            notificationAreaMessageBox->show();
            notificationAreaMessageBox->setText( QString("Please enter the URL or IP address of the MUD server.\n\n")+check.errorString() );
            validUrl = false;
            connect_button->setDisabled(true);
        }
    }
}

void dlgConnectionProfiles::slot_update_autologin( int state )
{
    QListWidgetItem * pItem = profiles_tree_widget->currentItem();
    if( ! pItem )
        return;
    QString profile = pItem->text();
    writeProfileData( profile, "autologin", QString::number( state ) );
}

void dlgConnectionProfiles::slot_update_port( const QString ignoreBlank )
{
    QString port = port_entry->text().trimmed();

    if (ignoreBlank == "")
    {
        validPort = false;
        connect_button->setDisabled(true);
        return;
    }

    if( port.indexOf(QRegExp("^\\d+$"), 0) == -1 )
    {
        QString val = port;
        val.chop( 1 );
        port_entry->setText( val );
        notificationArea->show();
        notificationAreaIconLabelWarning->hide();
        notificationAreaIconLabelError->show();
        notificationAreaIconLabelInformation->hide();
        notificationAreaMessageBox->setText( tr("You have to enter a number. Other characters are not permitted.") );
        notificationAreaMessageBox->show();
        validPort = false;
        connect_button->setDisabled(true);
        return;
    }
    QListWidgetItem * pItem = profiles_tree_widget->currentItem();

    if( pItem )
    {
        QString profile = pItem->text();
        bool ok;
        int num = port.trimmed().toInt(&ok);
        if( num < 65536 && ok)
        {
            port_entry->setPalette( mOKPalette );
            notificationArea->hide();
            notificationAreaIconLabelWarning->hide();
            notificationAreaIconLabelError->hide();
            notificationAreaIconLabelInformation->hide();
            notificationAreaMessageBox->hide();
            validPort = true;
            validateConnect();
            writeProfileData( profile, "port", port );
        }
        else
        {
            notificationArea->show();
            notificationAreaIconLabelWarning->hide();
            notificationAreaIconLabelError->show();
            notificationAreaIconLabelInformation->hide();
            notificationAreaMessageBox->setText( tr("Port number must be above zero and below 65535.") );
            notificationAreaMessageBox->show();
            validPort = false;
            connect_button->setDisabled(true);
            port_entry->setPalette( mErrorPalette );
        }
    }
}

void dlgConnectionProfiles::slot_update_name( const QString _n )
{
    QString name = profile_name_entry->text().trimmed();
    QListWidgetItem * pItem = profiles_tree_widget->currentItem();

    const QString allowedChars = ". _0123456789-#&aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ";
    bool __error = false;
    for( int __i=0; __i<name.size(); __i++ )
    {
        if( ! allowedChars.contains( name[__i] ) )
        {
            name.replace( name[__i], "" );
            __i=-1;
            __error = true;
        }
    }
    if( __error )
    {
        profile_name_entry->setText( name );
        notificationArea->show();
        notificationAreaIconLabelWarning->show();
        notificationAreaIconLabelError->hide();
        notificationAreaIconLabelInformation->hide();
        notificationAreaMessageBox->show();
        notificationAreaMessageBox->setText( tr("This character is not permitted. Use one of the following: %1\n").arg(allowedChars) );
        return;
    }

    // see if there is an edit that already uses a similar name
    if( pItem->text() != name && mProfileList.contains( name ) )
    {
        notificationArea->show();
        notificationAreaIconLabelWarning->hide();
        notificationAreaIconLabelError->show();
        notificationAreaIconLabelInformation->hide();
        notificationAreaMessageBox->show();
        notificationAreaMessageBox->setText( tr("This profile name is already in use.") );
        validName = false;
        connect_button->setDisabled(true);
    }
    else
    {
        notificationArea->hide();
        notificationAreaIconLabelWarning->hide();
        notificationAreaIconLabelError->hide();
        notificationAreaIconLabelInformation->hide();
        validName = true;
        validateConnect();
    }

}

void dlgConnectionProfiles::slot_save_name()
{
    QListWidgetItem * pItem = profiles_tree_widget->currentItem();
    QString name = profile_name_entry->text().trimmed();

    if (notificationAreaIconLabelError->isVisible() || name == "")
        return;

    validName = true;
    if( pItem )
    {
        mCurrentProfileEditName = pItem->text();
        int row = mProfileList.indexOf( mCurrentProfileEditName );
        if( ( row >= 0 ) && ( row < mProfileList.size() ) )
        {
            mProfileList[row] = name;
        }
        else
            mProfileList << name;

        // don't do anything if this was just a normal click, and not an edit of any sort
        if (mCurrentProfileEditName == name)
            return;

        pItem->setText( name );

        QDir previouspath(QDir::homePath()+"/.config/mudlet/profiles/"+mCurrentProfileEditName);
        QDir dir;

        if (previouspath.exists())
        {
            QDir parentpath(QDir::homePath()+"/.config/mudlet/profiles/");
            if (! parentpath.rename( mCurrentProfileEditName, name ) )
            {
                notificationArea->show();
                notificationAreaIconLabelWarning->show();
                notificationAreaIconLabelError->hide();
                notificationAreaIconLabelInformation->hide();
                notificationAreaMessageBox->show();
                notificationAreaMessageBox->setText( tr("Couldn't rename your profile data on the computer." ));
            }
        } else if (! dir.mkpath(QDir::homePath()+"/.config/mudlet/profiles/"+name) )
        {
            notificationArea->show();
            notificationAreaIconLabelWarning->show();
            notificationAreaIconLabelError->hide();
            notificationAreaIconLabelInformation->hide();
            notificationAreaMessageBox->show();
            notificationAreaMessageBox->setText( tr("Couldn't create the new profile folder on your computer." ));
        }

        // code stolen from fillout_form, should be moved to it's own function
        QFont font("Bitstream Vera Sans Mono", 1 );//mDisplayFont( QFont("Monospace", 10, QFont::Courier ) )
        QString sList = name;
        QString s = name;
        pItem->setFont(font);
        pItem->setForeground(QColor(255,255,255,255));
        profiles_tree_widget->addItem( pItem );
        QPixmap pb( 120, 30 );
        pb.fill(QColor(0,0,0,0));
        uint hash = qHash( sList );
        QLinearGradient shade(0, 0, 120, 30);
        int i = row;
        uint i1 = hash%255;
        uint i2 = (hash+i)%255;
        uint i3 = (i*hash)%255;
        uint i4 = (3*hash)%255;
        uint i5 = (hash)%255;
        uint i6 = (hash/i)%255;
        shade.setColorAt( 1, QColor(i1, i2, i3,255) );
        shade.setColorAt( 0, QColor(i4, i5, i6,255) );
        QBrush br( shade );
        QPainter pt(&pb);
        pt.setCompositionMode(QPainter::CompositionMode_SourceOver);
        pt.fillRect(QRect(0,0,120,30), shade);
        QPixmap pg( ":/icons/mudlet_main_32px.png");
        pt.drawPixmap( QRect(5,5, 20, 20 ), pg );

        QFont _font;
        QImage _pm( 90, 30, QImage::Format_ARGB32_Premultiplied	);
        QPainter _pt( &_pm );
        _pt.setCompositionMode(QPainter::CompositionMode_SourceOver);
        int fs=30;
        for( ; fs>1; fs-- )
        {
            _pt.eraseRect( QRect( 0, 0, 90, 30 ) );
            _pt.fillRect(QRect(0,0,90,30), QColor(255,0,0,10));
            _font = QFont("DejaVu Sans", fs, QFont::Helvetica);
            _pt.setFont( _font );
            QRect _r;
            if( (i1+i2+i3+i4+i5+i6)/6 < 100 )
                _pt.setPen( QColor(255,255,255,255) );
            else
                _pt.setPen( QColor(0,0,0,255));
            _pt.drawText(QRect(0,0, 90, 30), Qt::AlignHCenter|Qt::AlignVCenter|Qt::TextWordWrap, s, &_r );
            /*if( QFontMetrics( _font ).boundingRect( s ).width() <= 80
            && QFontMetrics( _font ).boundingRect( s ).height() <= 30 )*/
            if( _r.width() <= 90 && _r.height() <= 30 )
            {
                break;
            }

        }
        pt.setFont( _font );
        QRect _r;
        if( (i1+i2+i3+i4+i5+i6)/6 < 100 )
            pt.setPen( QColor(255,255,255,255) );
        else
            pt.setPen( QColor(0,0,0,255));
        pt.drawText( QRect(30,0, 90, 30), Qt::AlignHCenter|Qt::AlignVCenter|Qt::TextWordWrap, s, &_r );
        QIcon mi = QIcon( pb );
        pItem->setIcon( mi );
    }

}

void dlgConnectionProfiles::slot_addProfile()
{
    profile_name_entry->setReadOnly(false);
    fillout_form();
    welcome_message->hide();

    requiredArea->show();
    informationalArea->show();
    optionalArea->show();

    QStringList newname;
    mUnsavedProfileName = tr("new profile name");

    QListWidgetItem * pItem = new QListWidgetItem( mUnsavedProfileName);
    if( ! pItem )
    {
        return;
    }

    profiles_tree_widget->setSelectionMode( QAbstractItemView::SingleSelection );
    profiles_tree_widget->addItem( pItem );

    // insert newest entry on top of the list as the general sorting
    // is always newest item first -> fillout->form() filters
    // this is more practical for the user as they use the same profile most of the time

    profiles_tree_widget->setItemSelected(profiles_tree_widget->currentItem(), false); // Unselect previous item
    profiles_tree_widget->setCurrentItem( pItem );
    profiles_tree_widget->setItemSelected( pItem, true );

    profile_name_entry->setText( mUnsavedProfileName );
    profile_name_entry->setFocus();
    profile_name_entry->selectAll();
    profile_name_entry->setReadOnly( false );
    host_name_entry->setReadOnly( false );
    port_entry->setReadOnly( false );

    validName = false;
    validUrl = false;
    validPort = false;
    connect_button->setDisabled(true);
}

void dlgConnectionProfiles::slot_deleteprofile_check( const QString text )
{
    QString profile = profiles_tree_widget->currentItem()->text();
    if (profile != text)
        delete_button->setDisabled(true);
    else
    {
        delete_button->setEnabled(true);
        delete_button->setFocus();
    }
}

void dlgConnectionProfiles::slot_reallyDeleteProfile()
{
    QString profile = profiles_tree_widget->currentItem()->text();
// N/U:     int currentRow = profiles_tree_widget->currentIndex().row();
    QDir dir( QDir::homePath()+"/.config/mudlet/profiles/"+profile );
    removeDir( dir.path(), dir.path() );
    fillout_form();
    profiles_tree_widget->setFocus();
}

void dlgConnectionProfiles::slot_deleteProfile()
{
    if( ! profiles_tree_widget->currentItem() )
        return;

    QString profile = profiles_tree_widget->currentItem()->text();
    if( profile.size() > 1 ) return;

    QUiLoader loader;

    QFile file(":/ui/delete_profile_confirmation.ui");
    file.open(QFile::ReadOnly);

    QDialog *delete_profile_dialog = dynamic_cast<QDialog *>(loader.load(&file, this));
    file.close();

    if (!delete_profile_dialog)
        return;

    delete_profile_lineedit = delete_profile_dialog->findChild<QLineEdit*>("delete_profile_lineedit");
    delete_button = delete_profile_dialog->findChild<QPushButton*>("delete_button");
    QPushButton * cancel_button = delete_profile_dialog->findChild<QPushButton*>("cancel_button");

    if (!delete_profile_lineedit || !delete_button || !cancel_button) return;

    connect(delete_profile_lineedit, SIGNAL(textChanged(const QString)), this, SLOT(slot_deleteprofile_check(const QString)));
    connect(delete_profile_dialog, SIGNAL(accepted()), this, SLOT(slot_reallyDeleteProfile()));

    #if QT_VERSION >= 0x040700
    delete_profile_lineedit->setPlaceholderText(profile);
    #endif
    cancel_button->setFocus();
    delete_button->setDisabled(true);
    delete_profile_dialog->setWindowTitle("Deleting '"+profile+"'");

    delete_profile_dialog->show();
    delete_profile_dialog->raise();
}

QString dlgConnectionProfiles::readProfileData( QString profile, QString item )
{
    QString filepath = QDir::homePath()+"/.config/mudlet/profiles/"+profile+"/"+item;
    QFile file( filepath );
    bool success = file.open( QIODevice::ReadOnly );
    QString ret;
    if ( success ) {
        QDataStream ifs( & file );
        ifs >> ret;
        file.close();
    }

    return ret;
}

QStringList dlgConnectionProfiles::readProfileHistory( QString profile, QString item )
{
    QFile file( QDir::homePath()+"/.config/mudlet/profiles/"+profile+"/"+item );
    file.open( QIODevice::ReadOnly );
    QDataStream ifs( & file );
    QString ret;
    QStringList historyList;
    while( ifs.status() == QDataStream::Ok )
    {
        ifs >> ret;
        historyList << ret;
    }
    file.close();
    return historyList;
}

void dlgConnectionProfiles::writeProfileData( QString profile, QString item, QString what )
{
    QFile file( QDir::homePath()+"/.config/mudlet/profiles/"+profile+"/"+item );
    file.open( QIODevice::WriteOnly | QIODevice::Unbuffered );
    QDataStream ofs( & file );
    ofs << what;
    file.close();
}


void dlgConnectionProfiles::slot_item_clicked(QListWidgetItem *pItem)
{
    if( !pItem )
        return;

    profile_name_entry->setReadOnly(true);

    QString profile_name = pItem->text();

    profile_name_entry->setText( profile_name );

    QString profile = profile_name;

    QString item = "url";
    QString val = readProfileData( profile, item );
    if( val.size() < 1 )
    {
        if( profile_name == "Avalon.de" )
            val = "avalon.mud.de";
        if( profile_name == "God Wars II" )
            val = "godwars2.org";
        if( profile_name == "Materia Magica" )
            val = "materiamagica.com";
        if( profile_name == "BatMUD" )
            val = "batmud.bat.org";
        if( profile_name == "Aardwolf" )
            val = "aardmud.org";
        if( profile_name == "Achaea" )
            val = "achaea.com";
        if( profile_name == "Aetolia" )
            val = "aetolia.com";
        if( profile_name == "Midkemia" )
            val = "midkemiaonline.com";
        if( profile_name == "Lusternia" )
            val = "lusternia.com";
        if( profile_name == "Imperian" )
            val = "imperian.com";
        if( profile_name == "Realms of Despair" )
            val = "realmsofdespair.com";
        if( profile_name == "ZombieMUD" )
            val = "zombiemud.org";
        if( profile_name == "3Scapes")
            val = "3k.org";
        if( profile_name == "3Kingdoms")
            val = "3k.org";
        if( profile_name == "Slothmud")
            val = "slothmud.org";
    }
    host_name_entry->setText( val );
    item = "port";
    val = readProfileData( profile, item );
    if( val.size() < 1 )
    {
        if( profile_name == "Avalon.de" )
            val = "23";
        if( profile_name == "God Wars II" )
            val = "3000";
        if( profile_name == "Materia Magica" )
            val = "23";
        if( profile_name == "BatMUD" )
            val = "23";
        if( profile_name == "Aardwolf" )
            val = "4000";
        if( profile_name == "Achaea" )
            val = "23";
        if( profile_name == "Aetolia" )
            val = "23";
        if( profile_name == "Midkemia" )
            val = "23";
        if( profile_name == "Lusternia" )
            val = "23";
        if( profile_name == "Imperian" )
            val = "23";
        if( profile_name == "Realms of Despair" )
            val = "4000";
        if( profile_name == "ZombieMUD" )
            val = "23";
        if( profile_name == "3Scapes")
            val = "3200";
        if( profile_name == "3Kingdoms")
            val = "3000";
        if( profile_name == "Slothmud")
            val = "6101";
    }
    port_entry->setText( val );
    item = "password";
    val = readProfileData( profile, item );
    character_password_entry->setText( val );
    item = "login";
    val = readProfileData( profile, item );
    login_entry->setText( val );
    item = "autologin";
    val = readProfileData( profile, item );
    if( val.toInt() == Qt::Checked )
    {
        autologin_checkBox->setChecked( true );
    }
    else
    {
        autologin_checkBox->setChecked( false );
    }
    item = "description";
    if( profile_name == "Realms of Despair" )
        val = "The Realms of Despair is the original SMAUG MUD and is FREE to play. We have an active Roleplaying community, an active player-killing (deadly) community, and a very active peaceful community. Players can choose from 13 classes (including a deadly-only class) and 13 races. Character appearances are customizable on creation and we have a vast collection of equipment that is level, gender, class, race and alignment specific. We boast well over 150 original, exclusive areas, with a total of over 20,000 rooms. Mob killing, or 'running' is one of our most popular activities, with monster difficulties varying from easy one-player kills to difficult group kills. We have four deadly-only Clans, twelve peaceful-only Guilds, eight Orders, and fourteen Role-playing Nations that players can join to interact more closely with other players. We have two mortal councils that actively work toward helping players: The Symposium hears ideas for changes, and the Newbie Council assists new players. Our team of Immortals are always willing to answer questions and to help out however necessary. Best of all, playing the Realms of Despair is totally FREE!";
    else if( profile_name == "ZombieMUD" )
        val = "Since 1994, ZombieMUD has been on-line and bringing orc-butchering fun to the masses from our home base in Oulu, Finland. We're a pretty friendly bunch, with players logging in from all over the globe to test their skill in our medieval role-playing environment. With 15 separate guilds and 41 races to choose from, as a player the only limitation to your achievements on the game is your own imagination and will to succeed.";
    else if( profile_name == "God Wars II" )
        val = "God Wars II is a fast and furious combat mud, designed to test player skill in terms of pre-battle preparation and on-the-spot reflexes, as well as the ability to adapt quickly to new situations. Take on the role of a godlike supernatural being in a fight for supremacy.\n\nRoomless world. Manual combat. Endless possibilities.";
    else if( profile_name == "3Scapes")
        val = "3Scapes is an alternative dimension to 3Kingdoms, similar in many respects, but unique and twisted in so many ways.  3Scapes offers a faster pace of play, along with an assortment of new guilds, features, and areas.";
    else if ( profile_name == "3Kingdoms")
        val = "Simple enough to learn, yet complex enough to challenge you for years, 3Kingdoms is a colossal adventure through which many years of active and continued development by its dedicated coding staff.  Based around the mighty town of Pinnacle, three main realms beckon the player to explore. These kingdoms are known as: Fantasy, a vast medieval realm full of orcs, elves, dragons, and a myriad of other creatures; Science, a post-apocalyptic, war-torn world set in the not-so-distant future; and Chaos, a transient realm where the enormous realities of Fantasy and Science collide to produce creatures so bizarre that they have yet to be categorized.  During their exploration of the realms, players have the opportunity to join any of well over a dozen different guilds, which grant special, unique powers to the player, furthering their abilities as they explore the vast expanses of each realm. Add in the comprehensive skill system that 3K offers and you are able to extensively customize your characters.";
    else if( profile_name == "Slothmud" )
        val = "SlothMUD... the ultimate in DIKUMUD! The most active, intricate, exciting FREE MUD of its kind. This text based multiplayer free online rpg game and is enjoyed continuously by players worldwide. With over 27,500 uniquely described rooms, 9,300 distinct creatures, 14,200 characters, and 87,100 pieces of equipment, charms, trinkets and other items, our online rpg world is absolutely enormous and ready to explore.";
    else
        val = readProfileData( profile, item );
    mud_description_textedit->clear();
    mud_description_textedit->insertPlainText( val );
    item = "website";
    val = readProfileData( profile, item );
    if( val.size() < 1 )
    {
        if( profile_name == "Avalon.de" )
            val = "<center><a href='http://avalon.mud.de'>http://avalon.mud.de</a></center>";
        if( profile_name == "God Wars II" )
            val = "<center><a href='http://www.godwars2.org'>http://www.godwars2.org</a></center>";
        if( profile_name == "Materia Magica" )
            val = val = "<center><a href='http://www.materiamagica.com'>http://www.materiamagica.com</a></center>";
        if( profile_name == "BatMUD" )
            val = val = "<center><a href='http://www.bat.org'>http://www.bat.org</a></center>";
        if( profile_name == "Aardwolf" )
            val = "<center><a href='http://www.aardwolf.com/'>http://www.aardwolf.com</a></center>";;
        if( profile_name == "Achaea" )
            val = "<center><a href='http://www.achaea.com/'>http://www.achaea.com</a></center>";
        if( profile_name == "Realms of Despair" )
            val = "<center><a href='http://www.realmsofdespair.com/'>http://www.realmsofdespair.com</a></center>";
        if( profile_name == "ZombieMUD" )
            val = "<center><a href='http://www.zombiemud.org/'>http://www.zombiemud.org</a></center>";
        if( profile_name == "Aetolia" )
            val = "<center><a href='http://www.aetolia.com/'>http://www.aetolia.com</a></center>";;
        if( profile_name == "Midkemia" )
            val = "<center><a href='http://www.midkemiaonline.com/'>http://www.midkemiaonline.com</a></center>";;
        if( profile_name == "Lusternia" )
            val = "<center><a href='http://www.lusternia.com/'>http://www.lusternia.com</a></center>";;
        if( profile_name == "Imperian" )
            val = "<center><a href='http://www.imperian.com/'>http://www.imperian.com</a></center>";;
        if( profile_name == "3Scapes" )
            val = "<center><a href='http://www.3scapes.org/'>http://www.3scapes.org</a></center>";;
        if( profile_name == "3Kingdoms" )
            val = "<center><a href='http://www.3k.org/'>http://www.3k.org</a></center>";;
        if( profile_name == "Slothmud" )
            val = "<center><a href='http://www.slothmud.org/'>http://www.slothmud.org/</a></center>";
    }
    website_entry->setText( val );

    profile_history->clear();

    QString folder = QDir::homePath()+"/.config/mudlet/profiles/"+profile_name+"/current/";
    QDir dir( folder );
    dir.setSorting(QDir::Time);
    QStringList entries = dir.entryList( QDir::Files, QDir::Time );

    for (int i = 0; i < entries.size(); ++i)
    {
        QRegExp rx("(\\d+)\\-(\\d+)\\-(\\d+)#(\\d+)\\-(\\d+)\\-(\\d+).xml");
        if( rx.indexIn(entries.at(i)) != -1 )
        {
            QString day = rx.cap(1);
            QString month = rx.cap(2);
            QString year = rx.cap(3);
            QString hour = rx.cap(4);
            QString minute = rx.cap(5);
            QString second = rx.cap(6);

            QDateTime datetime;
            datetime.setTime(QTime (hour.toInt(), minute.toInt(), second.toInt()));
            datetime.setDate(QDate (year.toInt(), month.toInt(), day.toInt()));

            //readableEntries << datetime.toString(Qt::SystemLocaleLongDate);
            //profile_history->addItem(datetime.toString(Qt::SystemLocaleShortDate), QVariant(entries.at(i)));
            profile_history->addItem(datetime.toString(Qt::SystemLocaleLongDate), QVariant(entries.at(i)));
        }
        else
            profile_history->addItem(entries.at(i), QVariant(entries.at(i))); // if it has a custom name, use it as it is

    }

    if( profile_history->count() == 0 )
        profile_history->setDisabled(true);
    else
        profile_history->setEnabled(true);

    QStringList loadedProfiles = HostManager::self()->getHostList();
    if( loadedProfiles.contains( profile_name ) )
    {
        profile_name_entry->setReadOnly( true );
        host_name_entry->setReadOnly( true );
        port_entry->setReadOnly( true );

        profile_name_entry->setFocusPolicy( Qt::NoFocus );
        host_name_entry->setFocusPolicy( Qt::NoFocus );
        port_entry->setFocusPolicy( Qt::NoFocus );

        profile_name_entry->setPalette( mReadOnlyPalette );
        host_name_entry->setPalette( mReadOnlyPalette );
        port_entry->setPalette( mReadOnlyPalette );

        notificationArea->show();
        notificationAreaIconLabelWarning->hide();
        notificationAreaIconLabelError->hide();
        notificationAreaIconLabelInformation->show();
        notificationAreaMessageBox->show();
        notificationAreaMessageBox->setText(tr("This profile is currently loaded - you'll need to disconnect before changing the connection parameters."));
    }
    else
    {
        profile_name_entry->setReadOnly( false );
        host_name_entry->setReadOnly( false );
        port_entry->setReadOnly( false );

        profile_name_entry->setFocusPolicy( Qt::StrongFocus );
        host_name_entry->setFocusPolicy( Qt::StrongFocus );
        port_entry->setFocusPolicy( Qt::StrongFocus );

        profile_name_entry->setPalette( mRegularPalette );
        host_name_entry->setPalette( mRegularPalette );
        port_entry->setPalette( mRegularPalette );

        if (notificationAreaMessageBox->text() == tr("This profile is currently loaded - you'll need to disconnect before changing the connection parameters."))
        {
            notificationArea->hide();
            notificationAreaIconLabelWarning->hide();
            notificationAreaIconLabelError->hide();
            notificationAreaIconLabelInformation->hide();
            notificationAreaMessageBox->hide();
            notificationAreaMessageBox->setText(tr(""));
        }
    }
    profile_name_entry->setReadOnly(true);

}

// (re-)creates the dialogs profile list
void dlgConnectionProfiles::fillout_form()
{
    profiles_tree_widget->clear();
    profile_name_entry->clear();
    host_name_entry->clear();
    port_entry->clear();

    mProfileList = QDir(QDir::homePath()+"/.config/mudlet/profiles").entryList(QDir::Dirs, QDir::Name);

    if( mProfileList.size() < 3 )
    {
        welcome_message->show();
        requiredArea->hide();
        informationalArea->hide();
        optionalArea->hide();
        resize( minimumSize() );
    }
    else
    {
        welcome_message->hide();

        requiredArea->show();
        informationalArea->show();
        optionalArea->show();
    }

    profiles_tree_widget->setIconSize(QSize(120,30));
    QFont font("Bitstream Vera Sans Mono", 1 );//mDisplayFont( QFont("Monospace", 10, QFont::Courier ) )
    QString muds;
    QListWidgetItem * pM;
    QIcon mi;


    muds = "Avalon.de";
    pM = new QListWidgetItem( muds );
    pM->setFont(font);
    pM->setForeground(QColor(255,255,255));
    profiles_tree_widget->addItem( pM );
    QPixmap p(":/icons/avalon.png");
    mi = QIcon( p.scaled(QSize(120,30)) );
    pM->setIcon(mi);
    muds.clear();

    muds = "Achaea";
    pM = new QListWidgetItem( muds );
    pM->setFont(font);
    pM->setForeground(QColor(255,255,255));
    profiles_tree_widget->addItem( pM );
    mi = QIcon( ":/icons/achaea_120_30.png" );
    pM->setIcon(mi);
    muds.clear();

    muds = "3Kingdoms";
    pM = new QListWidgetItem( muds );
    pM->setFont(font);
    pM->setForeground(QColor(255,255,255));
    profiles_tree_widget->addItem(pM);
    QPixmap pd(":/icons/3klogo.png");
    QPixmap pd1 = pd.scaled(QSize(120,30),Qt::IgnoreAspectRatio, Qt::SmoothTransformation).copy();
    QIcon mi5(pd1);
    pM->setIcon(mi5);

    muds = "3Scapes";
    pM = new QListWidgetItem( muds );
    pM->setFont(font);
    pM->setForeground(QColor(255,255,255));
    profiles_tree_widget->addItem(pM);
    QPixmap pc(":/icons/3slogo.png");
    QPixmap pc1 = pc.scaled(QSize(120,30),Qt::IgnoreAspectRatio, Qt::SmoothTransformation).copy();
    QIcon mi4(pc1);
    pM->setIcon(mi4);
    muds.clear();

    muds = "Midkemia";
    pM = new QListWidgetItem( muds );
    pM->setFont(font);
    pM->setForeground(QColor(255,255,255));
    profiles_tree_widget->addItem( pM );
    mi = QIcon( ":/icons/midkemia_120_30.png" );
    pM->setIcon(mi);
    muds.clear();

    muds = "Lusternia";
    pM = new QListWidgetItem( muds );
    pM->setFont(font);
    pM->setForeground(QColor(255,255,255));
    profiles_tree_widget->addItem( pM );
    mi = QIcon( ":/icons/lusternia_120_30.png" );
    pM->setIcon(mi);
    muds.clear();

    muds = "BatMUD";
    QPixmap pb(":/icons/batmud_mud.png");
    QPixmap pb1 = pb.scaled(QSize(120,30)).copy();
    mi = QIcon( pb1 );
    pM = new QListWidgetItem( muds );
    pM->setFont(font);
    pM->setForeground(QColor(255,255,255));
    profiles_tree_widget->addItem( pM );
    pM->setIcon(mi);
    muds.clear();

    muds = "God Wars II";
    pM = new QListWidgetItem( muds );
    pM->setFont(font);
    pM->setForeground(QColor(255,255,255));
    profiles_tree_widget->addItem( pM );
    mi = QIcon( ":/icons/gw2.png" );
    pM->setIcon(mi);
    muds.clear();

    muds = "Slothmud";
    pM = new QListWidgetItem( muds );
    pM->setFont(font);
    pM->setForeground(QColor(255,255,255));
    profiles_tree_widget->addItem( pM );
    mi = QIcon( ":/icons/Slothmud.png" );
    pM->setIcon(mi);
    muds.clear();

    muds = "Aardwolf";
    pM = new QListWidgetItem( muds );
    pM->setFont(font);
    pM->setForeground(QColor(255,255,255));
    profiles_tree_widget->addItem( pM );
    mi = QIcon(":/icons/aardwolf_mud.png");
    pM->setIcon(mi);
    muds.clear();

    muds = "Materia Magica";
    pM = new QListWidgetItem( muds );
    pM->setFont(font);
    pM->setForeground(QColor(255,255,255));
    profiles_tree_widget->addItem( pM );
    mi = QIcon( ":/materiaMagicaIcon" );
    pM->setIcon(mi);
    muds.clear();

    muds = "Realms of Despair";
    pM = new QListWidgetItem( muds );
    pM->setFont(font);
    pM->setForeground(QColor(255,255,255));
    profiles_tree_widget->addItem( pM );
    mi = QIcon( ":/icons/120x30RoDLogo.png" );
    pM->setIcon(mi);
    muds.clear();

    muds = "ZombieMUD";
    pM = new QListWidgetItem( muds );
    pM->setFont(font);
    pM->setForeground(QColor(255,255,255));
    profiles_tree_widget->addItem( pM );
    mi = QIcon( ":/icons/zombiemud.png" );
    pM->setIcon(mi);
    muds.clear();

    muds = "Aetolia";
    pM = new QListWidgetItem( muds );
    pM->setFont(font);
    pM->setForeground(QColor(255,255,255));
    profiles_tree_widget->addItem( pM );
    mi = QIcon( ":/icons/aetolia_120_30.png" );
    pM->setIcon(mi);
    muds.clear();

    muds = "Imperian";
    pM = new QListWidgetItem( muds );
    pM->setFont(font);
    pM->setForeground(QColor(255,255,255));
    profiles_tree_widget->addItem( pM );
    mi = QIcon( ":/icons/imperian_120_30.png" );
    pM->setIcon(mi);
    muds.clear();


    QDateTime test_date;
    QListWidgetItem * toselect = 0;

    muds.clear();
    for( int i=0; i<mProfileList.size(); i++ )
    {
        QString s = mProfileList[i];
        if( s.size() < 1 )
            continue;
        if( (mProfileList[i] == ".") || (mProfileList[i] == ".." ) )
            continue;

        if( mProfileList[i] == "Avalon.de" )
            continue;
        if( mProfileList[i] == "BatMUD" )
            continue;
        if( mProfileList[i] == "Materia Magica" )
            continue;
        if( mProfileList[i] == "Aardwolf" )
            continue;
        if( mProfileList[i] == "Achaea" )
            continue;
        if( mProfileList[i] == "Aetolia" )
            continue;
        if( mProfileList[i] == "Midkemia" )
            continue;
        if( mProfileList[i] == "Lusternia" )
            continue;
        if( mProfileList[i] == "Imperian" )
            continue;
        if( mProfileList[i] == "Realms of Despair" )
            continue;
        if( mProfileList[i] == "ZombieMUD" )
            continue;
        if( mProfileList[i] == "3Scapes" )
            continue;
        if( mProfileList[i] == "3Kingdoms" )
            continue;
        QString sList;
        sList = mProfileList[i];
        QListWidgetItem * pItem = new QListWidgetItem( sList );
        pItem->setFont(font);
        pItem->setForeground(QColor(255,255,255,255));
        profiles_tree_widget->addItem( pItem );
        QPixmap pb( 120, 30 );
        pb.fill(QColor(0,0,0,0));
        uint hash = qHash( sList );
        QLinearGradient shade(0, 0, 120, 30);
        uint i1 = hash%255;
        uint i2 = (hash+i)%255;
        uint i3 = (i*hash)%255;
        uint i4 = (3*hash)%255;
        uint i5 = (hash)%255;
        uint i6 = (hash/i)%255;
        shade.setColorAt( 1, QColor(i1, i2, i3,255) );
        shade.setColorAt( 0, QColor(i4, i5, i6,255) );
        QBrush br( shade );
        QPainter pt(&pb);
        pt.setCompositionMode(QPainter::CompositionMode_SourceOver);
        pt.fillRect(QRect(0,0,120,30), shade);
        QPixmap pg( ":/icons/mudlet_main_32px.png");
        pt.drawPixmap( QRect(5,5, 20, 20 ), pg );

        QFont _font;
        QImage _pm( 90, 30, QImage::Format_ARGB32_Premultiplied	);
        QPainter _pt( &_pm );
        _pt.setCompositionMode(QPainter::CompositionMode_SourceOver);
        int fs=30;
        for( ; fs>1; fs-- )
        {
            _pt.eraseRect( QRect( 0, 0, 90, 30 ) );
            _pt.fillRect(QRect(0,0,90,30), QColor(255,0,0,10));
            _font = QFont("DejaVu Sans", fs, QFont::Helvetica);
            _pt.setFont( _font );
            QRect _r;
            if( (i1+i2+i3+i4+i5+i6)/6 < 100 )
                _pt.setPen( QColor(255,255,255,255) );
            else
                _pt.setPen( QColor(0,0,0,255));
            _pt.drawText(QRect(0,0, 90, 30), Qt::AlignHCenter|Qt::AlignVCenter|Qt::TextWordWrap, s, &_r );
            /*if( QFontMetrics( _font ).boundingRect( s ).width() <= 80
            && QFontMetrics( _font ).boundingRect( s ).height() <= 30 )*/
            if( _r.width() <= 90 && _r.height() <= 30 )
            {
                break;
            }

        }
        pt.setFont( _font );
        QRect _r;
        if( (i1+i2+i3+i4+i5+i6)/6 < 100 )
            pt.setPen( QColor(255,255,255,255) );
        else
            pt.setPen( QColor(0,0,0,255));
        pt.drawText( QRect(30,0, 90, 30), Qt::AlignHCenter|Qt::AlignVCenter|Qt::TextWordWrap, s, &_r );
        mi = QIcon( pb );
        pItem->setIcon( mi );

        QDateTime profile_lastRead = QFileInfo(QDir::homePath()+"/.config/mudlet/profiles/"+mProfileList[i]+"/Host.dat").lastRead();
        if (profile_lastRead > test_date)
        {
            test_date = profile_lastRead;
            toselect = pItem;
        }
    }

    if( toselect )
        profiles_tree_widget->setCurrentItem( toselect );
    profile_name_entry->setReadOnly(true);
}

void dlgConnectionProfiles::slot_cancel()
{
    QDialog::done( 0 );
}

void dlgConnectionProfiles::slot_copy_profile()
{
    QString profile_name = profile_name_entry->text().trimmed();
    QString oldname = profile_name;

    if( profile_name == "")
        return;

    // prepend n+1 to end of the profile name
    if (profile_name[profile_name.size()-1].isDigit())
    {
        int i=1;
        do {
            profile_name = profile_name.left(profile_name.size()-1) + QString::number(profile_name[profile_name.size()-1].digitValue() + i++);
        } while (mProfileList.contains(profile_name));
    } else {
        int i=1;
        QString profile_name2;
        do {
            profile_name2 = profile_name + QString::number(i++);
        } while (mProfileList.contains(profile_name2));
        profile_name = profile_name2;
    }

    QListWidgetItem * pItem = new QListWidgetItem( profile_name );
    if( ! pItem )
    {
        return;
    }

    // add the new widget in
    profiles_tree_widget->setSelectionMode( QAbstractItemView::SingleSelection );
    profiles_tree_widget->addItem( pItem );
    profiles_tree_widget->setItemSelected(profiles_tree_widget->currentItem(), false); // Unselect previous item
    profiles_tree_widget->setCurrentItem( pItem );
    profiles_tree_widget->setItemSelected( pItem, true );

    profile_name_entry->setText( profile_name );
    profile_name_entry->setFocus();
    profile_name_entry->selectAll();
    profile_name_entry->setReadOnly( false );
    host_name_entry->setReadOnly( false );
    port_entry->setReadOnly( false );

    // copy the folder on-disk
    QDir dir(QDir::homePath()+"/.config/mudlet/profiles/"+oldname);
    if (!dir.exists())
        return;

    copyFolder(QDir::homePath()+"/.config/mudlet/profiles/"+oldname, QDir::homePath()+"/.config/mudlet/profiles/"+profile_name);
    mProfileList << profile_name;
    slot_item_clicked(pItem);
}

void dlgConnectionProfiles::slot_connectToServer()
{
    QString profile_name = profile_name_entry->text().trimmed();

    if( profile_name.size() < 1 )
        return;

    Host * pOH = HostManager::self()->getHost( profile_name );
    if( pOH )
    {
        pOH->mTelnet.connectIt( pOH->getUrl(), pOH->getPort() );
        QDialog::accept();
        return;
    }
    // load an old profile if there is any
    HostManager::self()->addHost( profile_name, port_entry->text().trimmed(), "", "" );
    Host * pHost = HostManager::self()->getHost( profile_name );

    if( ! pHost ) return;

    QString folder = QDir::homePath()+"/.config/mudlet/profiles/"+profile_name+"/current/";
    QDir dir( folder );
    dir.setSorting(QDir::Time);
    QStringList entries = dir.entryList( QDir::Files, QDir::Time );
    bool needsGenericPackagesInstall = false;
    LuaInterface * lI = pHost->getLuaInterface();
    lI->getVars( true );
    if( entries.size() > 0 )
    {
        QFile file(folder+"/"+profile_history->itemData(profile_history->currentIndex()).toString());   //entries[0]);
        file.open(QFile::ReadOnly | QFile::Text);
        XMLimport importer( pHost );
        qDebug()<<"[LOADING PROFILE]:"<<file.fileName();
        importer.importPackage( & file, 0);
    }
    else
    {
        needsGenericPackagesInstall = true;
    }

    // overwrite the generic profile with user supplied name, url and login information
    if( pHost )
    {
        pHost->setName( profile_name );

        if( host_name_entry->text().trimmed().size() > 0 )
            pHost->setUrl( host_name_entry->text().trimmed() );
        else
            slot_update_url( pHost->getUrl() );

        if( port_entry->text().trimmed().size() > 0 )
            pHost->setPort( port_entry->text().trimmed().toInt() );
        else
            slot_update_port( QString::number( pHost->getPort() ) );

        if( character_password_entry->text().trimmed().size() > 0 )
            pHost->setPass( character_password_entry->text().trimmed() );
        else
            slot_update_pass( pHost->getPass() );

        if( login_entry->text().trimmed().size() > 0 )
            pHost->setLogin( login_entry->text().trimmed() );
        else
            slot_update_login( pHost->getLogin() );
    }

    if( needsGenericPackagesInstall )
    {
        //install generic mapper script
        if( pHost->getUrl().toLower().contains( "aetolia.com" ) ||
            pHost->getUrl().toLower().contains( "achaea.com" ) ||
            pHost->getUrl().toLower().contains( "lusternia.com" ) ||
            pHost->getUrl().toLower().contains( "midkemiaonline.com" ) ||
            pHost->getUrl().toLower().contains( "imperian.com" ) )
        {
           mudlet::self()->packagesToInstallList.append(":/mudlet-mapper.xml");
        }
        else if( pHost->getUrl().toLower().contains("3scapes.org") ||
                 pHost->getUrl().toLower().contains("3k.org"))
        {
            mudlet::self()->packagesToInstallList.append(":/3k-mapper.xml");
        }

        mudlet::self()->packagesToInstallList.append(":/deleteOldProfiles.xml");
        mudlet::self()->packagesToInstallList.append(":/echo.xml");
        mudlet::self()->packagesToInstallList.append(":/run-lua-code-v4.xml");

    }

    emit signal_establish_connection( profile_name, 0 );
}

void dlgConnectionProfiles::slot_chose_history()
{
    QString profile_name = profile_name_entry->text().trimmed();
    if( profile_name.size() < 1 )
    {
        QMessageBox::warning(this, tr("Browse Profile History:"),
                             tr("You have not selected a profile yet.\nWhich profile history do you want to browse?\nPlease select a profile first."));
        return;
    }
    QString fileName = QFileDialog::getOpenFileName(this, tr("Chose Mudlet Profile"),
                                                    QDir::homePath()+"/.config/mudlet/profiles/"+profile_name,
                                                    tr("*.xml"));

    if( fileName.isEmpty() ) return;

    QFile file(fileName);
    if( ! file.open(QFile::ReadOnly | QFile::Text) )
    {
        QMessageBox::warning(this, tr("Import Mudlet Package:"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return;
    }

    HostManager::self()->addHost( profile_name, port_entry->text().trimmed(), "", "" );
    Host * pHost = HostManager::self()->getHost( profile_name );
    if( ! pHost ) return;
    XMLimport importer( pHost );
    importer.importPackage( & file );

    emit signal_establish_connection( profile_name, -1 );
    QDialog::accept();
}

void dlgConnectionProfiles::slot_update()
{
    update();
}

bool dlgConnectionProfiles::validateConnect()
{
    if (validName && validUrl && validPort)
    {
        connect_button->setEnabled(true);
        connect_button->setToolTip("");
        return true;
    }
    else if (!validName)
        slot_update_name(profile_name_entry->text());
    else if (!validUrl)
        slot_update_url("");
    else if (!validPort)
        slot_update_port("");

    connect_button->setDisabled(true);
    connect_button->setToolTip("Please set a valid profile name, game server address and the game port before connecting.");
    return false;
}


// credit: http://www.qtcentre.org/archive/index.php/t-23469.html
void dlgConnectionProfiles::copyFolder(QString sourceFolder, QString destFolder)
{
    QDir sourceDir(sourceFolder);
    if(!sourceDir.exists())
        return;

    QDir destDir(destFolder);
    if(!destDir.exists())
    {
        destDir.mkdir(destFolder);
    }
    QStringList files = sourceDir.entryList(QDir::Files);
    for(int i = 0; i< files.count(); i++)
    {
        QString srcName = sourceFolder + QDir::separator() + files[i];
        QString destName = destFolder + QDir::separator() + files[i];
        QFile::copy(srcName, destName);
    }
    files.clear();
    files = sourceDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    for(int i = 0; i< files.count(); i++)
    {
        QString srcName = sourceFolder + QDir::separator() + files[i];
        QString destName = destFolder + QDir::separator() + files[i];
        copyFolder(srcName, destName);
    }
}

// credit: http://john.nachtimwald.com/2010/06/08/qt-remove-directory-and-its-contents/
bool dlgConnectionProfiles::removeDir( const QString dirName, QString originalPath )
{
    bool result = true;
    QDir dir(dirName);
    if( dir.exists( dirName ) )
    {
        Q_FOREACH( QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst))
        {
            // prevent recursion outside of the original branch
            if( info.isDir() && info.absoluteFilePath().startsWith( originalPath ) )
            {
                result = removeDir( info.absoluteFilePath(), originalPath );
            }
            else
            {
                result = QFile::remove( info.absoluteFilePath() );
            }

            if( !result )
            {
                return result;
            }
        }
        result = dir.rmdir( dirName );
    }

    return result;
}
