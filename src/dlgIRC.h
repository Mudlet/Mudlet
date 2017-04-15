#ifndef MUDLET_DLGIRC_H
#define MUDLET_DLGIRC_H

/***************************************************************************
 *   Copyright (C) 2010-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
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


// clang-format off
#include "pre_guard.h"
// clang-format on
#include "Host.h"
#include "ui_irc.h"

#include <QtCore>
#include <QTimer>

#ifdef Q_CC_MSVC
#include <irc.h>
#include <ircsession.h>
#include <irccommand.h>
#include <ircmessage.h>
#else
#include "irc/include/irc.h"
#include "irc/include/ircsession.h"
#include "irc/include/irccommand.h"
#include "irc/include/ircmessage.h"
#endif
// clang-format: off
#include "post_guard.h"
// clang-format: on

class Host;

class dlgIRC : public QMainWindow, public Ui::irc_dlg
{
    Q_OBJECT

public:
    dlgIRC( Host* );
    IrcSession* session;
    QTimer* rc_timer;
    QString mNick;
    QString mChannel;
    QString mHostName;
    quint16 mHostPort;
    bool mUseIrcClient;
    bool mIrcConnected;
    bool mIrcWelcomed;
    bool ircReconnectCalled;
    bool ircReconnectActive;
    int mIrcSocketState;

    bool mIsOper;
    QString mIrcModes;
    QStringList mChannels;
    QStringList mChanOpList;
    QMap< QString, QString > mChUserLists;
        
    void setNick( QString );
    void setMainChannel( QString );
    void setServerName( QString );
    void setServerPort( quint16 );

    void saveNick( QString );
    void saveMainChannel( QString );
    void saveServerName( QString );
    void saveServerPort( quint16 );
    void saveSessionConfigs();

    QString loadNick();
    QString loadMainChannel();
    QString loadServerName();
    quint16 loadServerPort();

    QString getNick();
    QString getMainChannel();
    QString getServerName();
    quint16 getServerPort();
    
    bool    isValidChannel( QString );
    bool    ircJoin( QString );
    bool    ircPart( QString );
    QString ircGetNicks( QString );
    void    ircOper( QString, QString );
    bool    ircIsOper();
    void    ircOp( QString, QString );
    void    ircDeOp( QString, QString );
    bool    ircIsChanOp( QString );
    void    ircTopic( QString, QString );
    void    ircMode( QString, QString );
    void    ircWhoIs( QString );
    void    ircWhoWas( QString );
    

public slots:
    void onMessageReceived(IrcMessage*);
    void anchorClicked(const QUrl& link);
    void sendMsg();
    
    void onConnected();
    void onDisconnected();
    void hardReconnect();
    void reconnect();
    void onSocketError( QAbstractSocket::SocketError );
    void onSocketStateChanged( QAbstractSocket::SocketState );
    
    void reconnect_timeout();

private:
    unsigned long ircReconnectMax;
    unsigned long ircReconnectCur;
    unsigned long ircReconnectWait;
    
    QPointer<Host> mpHost;
    
    QStringList eventGetNicksFor;
    QStringList eventWhoisFor;
    QStringList eventWhowasFor;
    QStringList ircWhoisActiveFor;
    QStringList ircWhowasActiveFor;
    QStringList eventTopicActiveFor;
    QStringList eventModeActiveFor;
    int eventOperCount;
    
    void irc_gotMsg( QString, QString, QString );
    void irc_gotMsg2( QString a, QStringList c );
    void irc_gotMsg3( QString a, uint code, QStringList c );
    
    void slot_welcomed();
    
    void slot_joined(QString, QString);
    void slot_parted(QString, QString, QString);
    
    void dlgWriteMsg(QString, QString);

};

#endif // MUDLET_DLGIRC_H
