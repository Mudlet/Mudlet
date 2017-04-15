/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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


#include "dlgIRC.h"

#include "Host.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QDebug>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QScrollBar>
#include <QTime>
#include <QTimer>
#include <QRegExp>
#include <QString>
#include "post_guard.h"


dlgIRC::dlgIRC( Host * pH ) : mpHost( pH )
{
    setupUi(this);
    
    ircReconnectCur = 0;
    ircReconnectMax = 1000000;
    ircReconnectWait = 8000;
    
    mIsOper = false;
    eventOperCount = 0;
    mIrcConnected = false;
    ircReconnectCalled = false;
    ircReconnectActive = false;

    rc_timer = new QTimer(this);
    rc_timer->setSingleShot( true );
    connect(rc_timer, SIGNAL(timeout()), this, SLOT(reconnect_timeout()) );

    session = new IrcSession(this);
    
    irc->setOpenExternalLinks ( true );
    setUnifiedTitleAndToolBarOnMac( true );
    
    connect( irc, SIGNAL(anchorClicked(QUrl)), this, SLOT(anchorClicked(QUrl)));
    connect( session, SIGNAL(messageReceived(IrcMessage*)), this, SLOT(onMessageReceived(IrcMessage*)));
    connect( lineEdit, SIGNAL(returnPressed()), this, SLOT(sendMsg()));
    
    connect( session, SIGNAL(connected()), this, SLOT(onConnected()));
    connect( session, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
    
    connect( session, SIGNAL(socketError(QAbstractSocket::SocketError)), this, SLOT(onSocketError(QAbstractSocket::SocketError)));
    connect( session, SIGNAL(socketStateChanged(QAbstractSocket::SocketState)), this, SLOT(onSocketStateChanged( QAbstractSocket::SocketState )) );
    
    // these are all processed via messageReceived now:
    //connect( session, SIGNAL(welcomed()), this, SLOT(onWelcomed()));
    //connect( session, SIGNAL(msgJoined(const QString &, const QString &)), this, SLOT(slot_joined(QString, QString)));
    //connect( session, SIGNAL(msgParted(const QString &, const QString &, const QString &)), this, SLOT(slot_parted(QString, QString, QString)));
    //connect( session, SIGNAL(msgNoticeReceived(const QString &, const QString &, const QString &)), this, SLOT(irc_gotMsg(QString, QString, QString)));
    //connect( session, SIGNAL(msgUnknownMessageReceived(const QString &, const QStringList &)), this, SLOT(irc_gotMsg2(QString, QStringList)));
    //connect( session, SIGNAL(msgNumericMessageReceived(const QString &, uint, const QStringList &)), this, SLOT(irc_gotMsg3(QString, uint, QStringList)));
    
    
    /* old slot binds * /
    connect( irc, SIGNAL(anchorClicked(QUrl)), this, SLOT(anchorClicked(QUrl)));
    connect( session, SIGNAL(msgMessageReceived(const QString &, const QString &, const QString &)), this, SLOT(irc_gotMsg(QString, QString, QString)));
    connect( session, SIGNAL(msgNoticeReceived(const QString &, const QString &, const QString &)), this, SLOT(irc_gotMsg(QString, QString, QString)));
    connect( session, SIGNAL(msgUnknownMessageReceived(const QString &, const QStringList &)), this, SLOT(irc_gotMsg2(QString, QStringList)));
    connect( session, SIGNAL(msgNumericMessageReceived(const QString &, uint, const QStringList &)), this, SLOT(irc_gotMsg3(QString, uint, QStringList)));
    connect( lineEdit, SIGNAL(returnPressed()), this, SLOT(sendMsg()));
    connect( session, SIGNAL(msgJoined(const QString &, const QString &)), this, SLOT(slot_joined(QString, QString)));
    connect( session, SIGNAL(msgParted(const QString &, const QString &, const QString &)), this, SLOT(slot_parted(QString, QString, QString)));
    
    connect( session, SIGNAL(connected()), this, SLOT(onConnected()));
    connect( session, SIGNAL(welcomed()), this, SLOT(onWelcomed()));
    connect( session, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
    
    connect( session, SIGNAL(socketError(QAbstractSocket::SocketError)), this, SLOT(onSocketError(QAbstractSocket::SocketError)));
    connect( session, SIGNAL(socketStateChanged(QAbstractSocket::SocketState)), this, SLOT(onSocketStateChanged( QAbstractSocket::SocketState )) );
    /* TODO: Remove this block when done porting. */

    /** /
    QStringList chans;
    chans << "#mudlet";
    session->setAutoJoinChannels( chans );

    QFile file( QDir::homePath()+"/.config/mudlet/irc_nick" );
    file.open( QIODevice::ReadOnly );
    QDataStream ifs( & file );
    QString nick;
    ifs >> nick;
    file.close();

    if( nick.isEmpty() )
    {
        nick = tr("Mudlet%1").arg(QString::number(rand()%10000));
        QFile file( QDir::homePath()+"/.config/mudlet/irc_nick" );
        file.open( QIODevice::WriteOnly | QIODevice::Unbuffered );
        QDataStream ofs( & file );
        ofs << nick;
        file.close();
    }

    session->setNick(nick);
    mNick = nick;
    /**/
    session->setUserName("mudlet");
    session->setRealName(mudlet::self()->version);

    setNick( loadNick() );
    setMainChannel( loadMainChannel() );
    setServerName( loadServerName() );
    setServerPort( loadServerPort() );
    
    /* Check if options allow for IRC */
    if( mNick == "DisableMudletIRC" ) {
        qDebug()<<"dlgIRC() IRC Nickname was pre-set to 'DisableMudletIRC' and so IRC will not start automatically.";

        irc_gotMsg( "", "", "IRC auto-start is disabled because 'DisableMudletIRC' was configured in your profile as the nickname." );
        irc_gotMsg( "", "", "You can manually start irc again with /startirc and use /nick to set a nick for this session." );
        
        mUseIrcClient = false;
        return;  // Return here to prevent starting the session.
    } else {
        mUseIrcClient = true;
    }
    
    // set ircReconnectCalled to true preventing calls before the client has fully started the first connection.
    ircReconnectCalled = true;
    
    //session->connectToServer( mHostName, mHostPort );
    session->open();
}


QString dlgIRC::loadNick() {
    Host * pHost = mpHost;
    
    QString nnick, nick; nnick = nick = tr("Mudlet%1").arg(QString::number(rand()%1000));
    QFile file( QDir::homePath()+"/.config/mudlet/profiles/" + pHost->getName() + "/irc_nick" );

    if( file.exists() ) {
        file.open( QIODevice::ReadOnly );
        QDataStream ifs( & file );
        ifs >> nick;
        file.close();
        
        if( nick.trimmed().isEmpty() ) {
            nick = nnick;
        }
        
    } else {
        saveNick( nick );
    }

    return nick;
}

QString dlgIRC::loadMainChannel() {
    Host * pHost = mpHost;
    
    QFile file2( QDir::homePath()+"/.config/mudlet/profiles/" + pHost->getName() + "/irc_channel" );
    QString nchan, chan; chan = nchan = tr("#mudlet%1").arg(QString::number(rand()%1000));

    if( file2.exists() ) {
        file2.open( QIODevice::ReadOnly );
        QDataStream ifs1( & file2 );
        ifs1 >> chan;
        file2.close();
        
        if( ! isValidChannel( chan ) ) {
            chan = nchan;
        }
        
    } else {
        saveMainChannel( chan );
    }

    return chan;
}

QString dlgIRC::loadServerName() {
    Host * pHost = mpHost;
    QFile file4( QDir::homePath()+"/.config/mudlet/profiles/" + pHost->getName() + "/irc_server_name" );
    QString serverName = "localhost";
    
    qDebug() << QDir::homePath()+"/.config/mudlet/profiles/" + pHost->getName() + "/irc_server_name";
    
    if( file4.exists() ) {
        file4.open( QIODevice::ReadOnly );
        QDataStream ifs2( & file4 );
        ifs2 >> serverName;
        file4.close();
        
        if( serverName.trimmed().isEmpty() ) {
            serverName = "localhost";
        }
        
        qDebug()<<"dlgIRC::loadServerName() Loaded serverName="<<serverName;
    } else {
        qDebug()<<"dlgIRC::loadServerName() Just saved serverName="<<serverName;
        saveServerName( serverName );
    }

    return serverName;
}

quint16 dlgIRC::loadServerPort() {
    Host * pHost = mpHost;
    
    QFile file6( QDir::homePath()+"/.config/mudlet/profiles/" + pHost->getName() + "/irc_server_port" );
    quint16 serverPort = 6667;

    if( file6.exists() ) {
        file6.open( QIODevice::ReadOnly );
        QDataStream ifs3( & file6 );
        ifs3 >> serverPort;
        
        qDebug()<<"dlgIRC::loadServerPort() - Loaded Port: "<<serverPort ; 
        
        if( serverPort > 65535 || serverPort < 1 ) {
            serverPort = 6667;
            qDebug()<<"dlgIRC::loadServerPort() - Reset Port to 6667, irc_server_port did not look like a valid port number.";
        }
        
        file6.close();
    } else {
        saveServerPort( serverPort );
    }

    return serverPort;
}

QString dlgIRC::getNick() {
    return mNick;
}

QString dlgIRC::getMainChannel() {
    return mChannel;
}

QString dlgIRC::getServerName() {
    qDebug()<<"dlgIRC::getServerName() returned "<<mHostName;
    return mHostName;
}

quint16 dlgIRC::getServerPort() {
    return mHostPort;
}

void dlgIRC::setNick( QString nick ) {
    QString sNick = session->nickName();
    qDebug()<<"dlgIRC::setNick() new_nick:"<< nick << " old_nick:" << mNick << " session_nick:" << sNick;
    
    if( nick == mNick && nick == sNick) {
        qDebug()<<"dlgIRC::setNick() returned no need to update.";
        return;
    }
    
    session->setNickName(nick);
    mNick = nick;
    
    if( mIrcConnected ) {
        session->sendCommand( IrcCommand::createNames( mChannel ) );
        //session->cmdNames( mChannel );
    }
}

void dlgIRC::setMainChannel( QString channel ) {
    /**
     * This function sets the "main" irc channel for the chat window.
     * TODO:  maybe add a boolean to control parting old mChannel.
     **/
    
    qDebug() << "dlgIRC::setMainChannel() channel="<<channel;
    if( channel == mChannel ) {
        return;
    }
    
    QString oldChan = mChannel;
    mChannel = channel;
    
    if( mIrcConnected ) {
        if( oldChan.size() > 1 ) {
            ircPart( oldChan );
        }
        ircJoin( channel );
    }
    
    
}

void dlgIRC::setServerName( QString serverName ) {
    qDebug()<<"dlgIRC::setServerName() serverName="<<serverName << " old serverName="<<mHostName;
    if( serverName == mHostName ) {
        qDebug()<<"dlgIRC::setServerName() no need to update, host isn't changed.";
        return;
    }
    
    mHostName = serverName;
    session->setHost(serverName);
}

void dlgIRC::setServerPort( quint16 serverPort ) {
    if( serverPort == mHostPort ) {
        return;
    }
    
    if( serverPort > 65535 || serverPort < 1 ) {
        serverPort = 6667;
        qDebug()<<"dlgIRC::setServerPort() - Reset Port to 6667, irc_server_port did not look like a valid port number.";
    }
    
    mHostPort = serverPort;
    session->setPort(serverPort);
}

void dlgIRC::saveNick( QString nick ) {
    Host * pHost = mpHost;
    
    QFile file1( QDir::homePath()+"/.config/mudlet/profiles/" + pHost->getName() + "/irc_nick" );
    file1.open( QIODevice::WriteOnly | QIODevice::Unbuffered );
    QDataStream ofs( & file1 );
    ofs << nick;
    file1.close();

    setNick( nick );
}

void dlgIRC::saveMainChannel( QString channelName ) {
    Host * pHost = mpHost;
    
    QFile file3( QDir::homePath()+"/.config/mudlet/profiles/" + pHost->getName() + "/irc_channel" );
    file3.open( QIODevice::WriteOnly | QIODevice::Unbuffered );
    QDataStream ofs1( & file3 );
    ofs1 << channelName;
    file3.close();

    setMainChannel( channelName );
}

void dlgIRC::saveServerName( QString hostname ) {
    qDebug()<<"dlgIRC::saveServerName() New server: "<< hostname;
    Host * pHost = mpHost;
    
    QFile file5( QDir::homePath()+"/.config/mudlet/profiles/" + pHost->getName() + "/irc_server_name" );
    file5.open( QIODevice::WriteOnly | QIODevice::Unbuffered );
    QDataStream ofs2( & file5 );
    ofs2 << hostname;
    file5.close();

    setServerName( hostname );
}

void dlgIRC::saveServerPort( quint16 portnumber ) {
    if( portnumber > 65535 || portnumber < 1 ) {
        portnumber = 6667;
        qDebug()<<"dlgIRC::saveServerPort() - Reset Port to 6667, irc_server_port did not look like a valid port number.";
    }
    
    Host * pHost = mpHost;
    QString strPortNum = QString::number( portnumber );
    
    qDebug()<<"dlgIRC::saveServerPort() New port: "<< portnumber << " As Str: "<< strPortNum;
    
    QFile file7( QDir::homePath()+"/.config/mudlet/profiles/" + pHost->getName() + "/irc_server_port" );
    file7.open( QIODevice::WriteOnly | QIODevice::Unbuffered );
    QDataStream ofs3( & file7 );
    ofs3 << portnumber;
    file7.close();

    setServerPort( portnumber );
}

void dlgIRC::saveSessionConfigs() {
    saveServerName( mHostName );
    saveServerPort( mHostPort );
    saveMainChannel( mChannel );
    saveNick( mNick );
}


void dlgIRC::sendMsg()
{
    QString txt = lineEdit->text();
    lineEdit->clear();
    
    if( txt.isEmpty() ) {
        return;
    }
    
    qDebug() << "dlgIRC::sendMsg() msg="<< txt << " Sstate="<< mIrcSocketState;

    if( ! mIrcConnected ) {
        if( txt.startsWith("/startirc") ) {

            QString msg = QString("<font color=#aa00aa>Manually Starting IRC Client</font>");
            dlgWriteMsg( msg, "" );

            session->close();
            session->open();
            //session->disconnectFromServer();
            //session->connectToServer( mHostName, mHostPort );
            mUseIrcClient = true;
            return;
        }
        
        this->reconnect();
        return;
    }

    if( txt.startsWith("/stopirc") && mIrcConnected ) {

        QString msg = QString("<font color=#aa00aa>Manually Stoping IRC Client</font>");
        dlgWriteMsg( msg, "" );
        
        ircReconnectCalled = false;
        session->close();
        //session->disconnectFromServer();
        mIrcConnected = false;
        mUseIrcClient = false;
        return;
    }
    
    if( txt.startsWith("/joinserver ") ) {
        txt.replace( "/joinserver ", "" );
        txt.replace( ":", " ");
        
        if( txt.size() > 1 ) {
            QStringList hpL = txt.split( " " ); 
            QString newHost = hpL[0];
            bool ok = true;
            int newPort = 6667;
            if( hpL.size() >= 2 ) {
                newPort = hpL[1].toInt( &ok );
            }
            
            qDebug() << "dlgIRC::sendMsg( '/joinserver' )  host: "<< newHost << " port: " << newPort;
            
            if( newHost != mHostName || newPort != mHostPort ) {
                if( ok ) {
                    setServerPort( newPort );
                }
                setServerName( newHost );
                
                reconnect();
            }
        }
        
        return;
    }
    
    if( txt.startsWith("/setchan ") ) {
        txt.replace("/setchan ", "");
        
        int _i = txt.indexOf(" ");
        if( _i != -1 ) {
            txt = txt.left( _i );
        }
        
        if( isValidChannel( txt ) ) {
            dlgWriteMsg( "<font color=#aa00aa>Changing Main Channel...</font>", "" );
            setMainChannel( txt );
        }
        
        return;
    }
    
    if( txt.startsWith("/join ") ) {
        txt.replace( "/join ", "" );
        int _i = txt.indexOf(" ");
        QStringList jChans;
        
        if( _i != -1 ) {
            jChans = txt.split(" ");
        } else {
            jChans.append( txt );
        }
        
        for( int i=0; i < jChans.size(); i++ ) {
            ircJoin( jChans[i] );
        }
        
        return;
    }
    
    if( txt.startsWith("/part ") ) {
          txt.replace( "/part ", "" );
          int _i = txt.indexOf(" ");
          QStringList pChans;
          
        if( _i != -1 ) {
            pChans = txt.split(" ");
        } else {
            pChans.append( txt );
        }
        
        for( int i=0; i < pChans.size(); i++ ) {
            ircPart( pChans[i] );
        }
          
          return;
    }
    
    if( txt.startsWith("/listusers") || txt.startsWith("/listnicks") ) {
        txt.replace( "/listusers", "" );
        txt.replace( "/listnicks", "" );
        
        QString lookupChan = mChannel;
        if( txt.size() > 1 ) {
            txt = txt.mid( 1 );
            int _i = txt.indexOf(" ");
            if( _i != -1 ) {
                txt = txt.left( _i );
            }
            
            if( txt.size() > 0 ) {
                lookupChan = txt;
            }
        }
        
        if( isValidChannel( lookupChan ) ) {
            dlgWriteMsg( QString("<font color=#aa00aa>Listing Users in Channel: </font>%1").arg(lookupChan), "" );
            
            //session->cmdNames( lookupChan );
            session->sendCommand( IrcCommand::createNames( lookupChan ) );
        } else {
            dlgWriteMsg( QString("<font color=#C90C0C>Channel '%1' does not appear to be a valid channel name.</font>").arg(lookupChan), "" );
        }
        return;
    }
    
    if( txt.startsWith("/listchans") ) {
        QString hi = QString( "<font color=#1737EA>%1</font>" ).arg( mChannel );
        QString msg = QString( "<font color=#aa00aa>Currently joined in these channels: </font><br><font color=#7B17ED> %1</font>" )
                              .arg( mChannels.join(" &nbsp;") );
        msg.replace( mChannel, hi );
        dlgWriteMsg( msg, "" );
        return;
    }

    if( txt.startsWith("/whois ") ) {
        txt.replace( "/whois ", "" );
        
        if( txt.size() > 1 ) {
            int _i = txt.indexOf(" ");
            if( _i != -1 ) {
                txt = txt.left( _i );
            }
            
            ircWhoisActiveFor.append( txt );
            IrcCommand *cmd = IrcCommand::createWhois( txt );
            session->sendCommand( cmd );
            //session->cmdWhois( txt );
        }
        
        return;
    }
    
    if( txt.startsWith("/whowas ") ) {
        txt.replace( "/whowas ", "" );
        
        if( txt.size() > 1 ) {
            int _i = txt.indexOf(" ");
            if( _i != -1 ) {
                txt = txt.left( _i );
            }
            
            ircWhowasActiveFor.append( txt );
            IrcCommand *cmd = IrcCommand::createWhowas( txt );
            session->sendCommand( cmd );
            //QString cmdStr = QString("WHOWAS %1 %1").arg( txt );
            //session->sendRaw( cmdStr );
        }
        
        return;
    }
    
    if( txt.startsWith("/oper ") ) {
        txt.replace("/oper ", "");
        QStringList oParts = txt.split(" ");
        
        if( oParts.size() == 2 ) {
            QString cmdStr = QString( "OPER %1 %2" ).arg( oParts[0] ).arg( oParts[1] );
            session->sendRaw( cmdStr );
            session->sendCommand( IrcCommand::createNames( mChannel ) );
            //session->cmdNames( mChannel );
            //qDebug() << " OPER CMD:  " << cmdStr ;
        } else {
            dlgWriteMsg( QString("<font color=#C90C0C>Command /oper requires a username and password.</font>"), "" );
        }
        
        return;
    }
    
    if( txt.startsWith("/op ") ) {
        txt.replace( "/op ", "" );
        txt = txt.trimmed();
        
        if( txt.size() > 1 ) {
            txt = txt.trimmed();
            QStringList oL = txt.split(" ");
            QString nick = oL[0];
            QString chan = mChannel;
            
            if( oL.size() >= 2 ) {
                if( isValidChannel( oL[1] ) ) {
                    chan = oL[1];
                }
            }
            
            //session->mode( chan, QString("+o %1").arg( nick ) );
            session->sendCommand( IrcCommand::createMode( nick, "+o" ) );
            if( chan != mChannel ) {
                //session->cmdNames( chan );
                session->sendCommand( IrcCommand::createNames( chan ) );
            }
            session->sendCommand( IrcCommand::createNames( mChannel ) );
            //session->cmdNames( mChannel );
        } else {
            dlgWriteMsg( "<font color=#C90C0C>OP command requires a nick name.</font>", "" );
        }
        
        return;
    }
    
    if( txt.startsWith("/deop ") ) {
        txt.replace( "/deop ", "" );
        txt = txt.trimmed();
        
        if( txt.size() > 1 ) {
            txt = txt.trimmed();
            QStringList oL = txt.split(" ");
            QString nick = oL[0];
            QString chan = mChannel;
            
            if( oL.size() >= 2 ) {
                if( isValidChannel( oL[1] ) ) {
                    chan = oL[1];
                }
            }
            
            //session->mode( chan, QString("-o %1").arg( nick ) );
            session->sendCommand( IrcCommand::createMode( nick, "-o" ) );
            if( chan != mChannel ) {
                //session->cmdNames( chan );
                session->sendCommand( IrcCommand::createNames( chan ) );
            }
            //session->cmdNames( mChannel );
            session->sendCommand( IrcCommand::createNames( mChannel ) );
        } else {
            dlgWriteMsg( "<font color=#C90C0C>DEOP command requires a nick name.</font>", "" );
        }
        
        return;
    }
    
    if( txt.startsWith("/mode") ) {
        txt = txt.trimmed();
        txt.replace( "/mode", "" );
        QString target = mNick;
        QString mStr = "";
        
        // check for a target and mode strings.
        if( txt.size() > 2 ) {
            txt = txt.trimmed();
            QStringList mParts = txt.split(" ");
            
            // Test the target is valid or use default of mNick.
            if( mParts[0].indexOf("+") == -1 && mParts[0].indexOf("-") == -1 ) {
                target = mParts[0];
                mParts.removeAt(0);
            }
            
            // If any text remains, use it as the mode string.
            if( mParts.size() >= 1 ) {
                mStr = mParts.join(" ");
            }
        }
        
        qDebug() << "dlgIRC::sendMsg( '/mode' ) To: "<< target << "  str: " << mStr ; 
        
        //session->mode( target, mStr );
        session->sendCommand( IrcCommand::createMode( target, mStr ) );
        
        return;
    }
    
    if( txt.startsWith("/topic") ) {
        txt = txt.trimmed();
        txt.replace( "/topic", "" );
        QString target = mChannel;
        QString mStr = "";
        
        // check for a target and topic message to set.
        if( txt.size() > 3 ) {
            txt = txt.trimmed();
            QStringList mParts = txt.split(" ");
            
            // Test the target is valid
            if( isValidChannel( mParts[0] ) ) {
                target = mParts[0];
                mParts.removeAt(0);
            }
            
            // If any text remains, use it to set the topic.
            if( mParts.size() >= 1 ) {
                mStr = mParts.join(" ");
            }
        }
        
        qDebug() << "dlgIRC::sendMsg( '/topic' ) To: "<< target << "  str: " << mStr ; 
        
        //session->topic( target, mStr );
        session->sendCommand( IrcCommand::createTopic(target, mStr ) );
        
        return;
    }
    
    if( txt.startsWith("/help") ) {
        
        dlgWriteMsg( "IRC Client Help ", "" );
        dlgWriteMsg( "Commands: ", "" );
        dlgWriteMsg( "&nbsp; /stopirc <font color=#5B828C><i>- Manually Stop & disable the IRC Client functions of Mudlet.</i></font>", "" );
        dlgWriteMsg( "&nbsp; /startirc <font color=#5B828C><i>- Manually Start a Stopped IRC Client.</i></font>", "" );
        dlgWriteMsg( "&nbsp; /joinserver &#60;host&#62;[:&#60;port&#62;] <font color=#5B828C><i>- Manually connect to a new server, without changing the saved IRC settings.</i></font>", "" );
        dlgWriteMsg( "&nbsp; /setchan &#60;channel&#62; <font color=#5B828C><i>- Manually set a new Default/Main IRC Channel, without changing the saved IRC settings.</i></font>", "" );
        dlgWriteMsg( "&nbsp; /join &#60;channel&#62; <font color=#5B828C><i>- Joins a new channel, in addition to the default/main channel.</i></font>", "" );
        dlgWriteMsg( "&nbsp; /part &#60;channel&#62; <font color=#5B828C><i>- Parts the given channel if the client has joined it.  Will close the IRC session if parting the last channel.</i></font>", "" );
        dlgWriteMsg( "&nbsp; /nick &#60;new nick&#62; <font color=#5B828C><i>- Manually change the IRC Client Nickname, without changing the saved IRC settings.</i></font>", "" );
        dlgWriteMsg( "&nbsp; /msg &#60;target&#62; &#60;message txt&#62; <font color=#5B828C><i>- Send a message to another channel or private message to a user.</i></font>", "" );
        dlgWriteMsg( "&nbsp; /listusers [channel] <font color=#5B828C><i>- Update the user nick list for given channel. Channel is optional and defaults to the main channel.</i></font>", "" );
        dlgWriteMsg( "&nbsp; /listchans <font color=#5B828C><i>- Prints a list of all currently joined channels.</i></font>", "" );
        dlgWriteMsg( "&nbsp; /whois &#60;nick&#62; <font color=#5B828C><i>- Request WHOIS info about a given Nick from the IRC Server.</i></font>", "" );
        dlgWriteMsg( "&nbsp; /whowas &#60;nick&#62; <font color=#5B828C><i>- Request WHOWAS info about a given Nick from the IRC Server.</i></font>", "" );
        dlgWriteMsg( "&nbsp; /topic [channel] [topic text] <font color=#5B828C><i>- Manually review the topic or set a new topic on the given channel. Main channel is used if no arguments are given.</i></font>", "" );
        dlgWriteMsg( "&nbsp; /mode [modes] <font color=#5B828C><i>- Get or set the mode(s) of the IRC client.</i></font>", "" );
        dlgWriteMsg( "&nbsp; /mode &#60;channel&#62; [modes] <font color=#5B828C><i>- Get or set mode(s) on a channel. Must have CHANOP or OPER to set modes.</i></font>", "" );
        dlgWriteMsg( "&nbsp; /oper &#60;user&#62; &#60;password&#62; <font color=#5B828C><i>- Send an OPER command to authenticate as an IRC Server Operator.</i></font>", "" );
        dlgWriteMsg( "&nbsp; /op &#60;nick&#62; [channel] <font color=#5B828C><i>- Grant CHANOP to given Nick in the given Channel. Main channel is used if none is given. Must have CHANOP or OPER to use this.</i></font>", "" );
        dlgWriteMsg( "&nbsp; /deop &#60;nick&#62; [channel] <font color=#5B828C><i>- Revoke CHANOP from given Nick in the given Channel. Main channel is used if none is given. Must have CHANOP or OPER to use this.</i></font>", "" );
        dlgWriteMsg( "  ", "" );
        
        return;
    }
    
    if( txt.startsWith("/nick ") ) {
        txt.replace("/nick ", "" );
        
        if( txt.size() > 0 ) {
            int _i = txt.indexOf(" ");
            if( _i != -1 ) {
                txt = txt.left( _i );
            }
            
            session->setNickName( txt );
            //session->setNick( txt );
            mNick = txt;
            session->sendCommand( IrcCommand::createNames( mChannel ) );
            //session->cmdNames( mChannel );
            
            QString nickOutStr = QString( "<font color=#aa00aa>Changing your nick to: </font>%1"  ).arg(mNick);
            dlgWriteMsg( nickOutStr, "" );
        }
        return;
    }
    
    /* Legacy code* /
    if( txt.startsWith("/nick ") )
    {
        txt.replace("/nick ", "" );
        session->setNick( txt );
        mNick = txt;
        session->cmdNames("#mudlet");
        return;
    } /**/
    
    if( txt.startsWith( "/msg ") )
    {
        if( txt.size() < 5 ) return;
        txt = txt.mid(5);
        int _i = txt.indexOf(" ");
        if( _i == -1 || _i < 1 || txt.size()<_i+2 ) return;
        QString r = txt.mid(0, _i);
        QString m = txt.mid( _i+1 );
        
        qDebug()<<"dlgIRC::sendMsg( '/msg' ) To:"<< r <<" Msg:"<< m;
        
        IrcCommand *cmd = IrcCommand::createMessage( r, m );
        session->sendCommand( cmd );
        IrcMessage* msg = IrcMessage::fromCommand( session->nickName(), cmd, session );
        onMessageReceived( msg );
        session->sendCommand( IrcCommand::createNames( mChannel ) );
        
        //session->cmdMessage( r, m );
        //session->cmdNames(mChannel);
        return;
    }

    //session->cmdMessage(mChannel, txt);
    //session->cmdNames( mChannel );
    
    session->sendCommand( IrcCommand::createMessage(mChannel, txt) );
    session->sendCommand( IrcCommand::createNames(mChannel) );
}

void dlgIRC::onConnected() {
    qDebug()<<"dlgIRC::onConnected() -- Connection should be ready now.";
    ircReconnectCalled = false;
    mIrcConnected = true;
    ircReconnectCur = 0;
    
    dlgWriteMsg( "<font color=#aa00aa>Connected to Server...</font>", "" );
    
    // This should be done after the server welcomes us...
    // might need to set that up.
    ircJoin( mChannel );
    //setNick( mNick );
}

void dlgIRC::onDisconnected(){
    mIrcConnected = false;
    mIsOper = false;
    mChannels.clear();
    qDebug()<<"dlgIRC::onDisconnected() :: reconnect_pending:"<< ircReconnectCalled ;
    
    dlgWriteMsg( "<font color=#aa00aa>Disconnected from Server.</font>", "" );
    
    // if reconnect was called, we can now reconnect cleanly.
    if( ircReconnectCalled ) {
        ircReconnectCalled = false;
        reconnect();
    }
}

void dlgIRC::hardReconnect() {
    QString msg = QString("<font color=#aa00aa>Manually Reconnecting IRC Client</font>");
    dlgWriteMsg( msg, "" );

    session->close();
    session->open();
    //session->disconnectFromServer();
    //session->connectToServer( mHostName, mHostPort );
}

void dlgIRC::reconnect() {

    ircReconnectCur++;
    
    qDebug() << "dlgIRC::reconnect() - useIRC:" << mUseIrcClient << " connected: " << mIrcConnected << "rc_called: " << ircReconnectCalled << " rc_timer_active: " << ircReconnectActive << " Sstate: " << mIrcSocketState ;
    // Don't reconnect if IRC client is disabled or if its already reconnecting.
    if( ! mUseIrcClient ) {
        qDebug()<<"dlgIRC::reconnect() skipping attempt #"<< ircReconnectCur << " IRC Client is currently disabled.";
        return;
    }
    
    if( ircReconnectCalled ) {
        qDebug()<<"dlgIRC::reconnect() skipping attempt #"<< ircReconnectCur << " waiting for a previous attempt.";
        return;
    }
    
    ircReconnectCalled = true;
    
    QString msg = QString("<font color=#aa00aa>Trying to Reconnect...</font>");
    dlgWriteMsg( msg, "" );

    // Need to make sure the current connection state is 0 before trying to reconnect.
    if( mIrcSocketState != QAbstractSocket::UnconnectedState ) {
        qDebug()<<"dlgIRC::reconnect() -- closing connection.";
        session->close();
        //session->disconnectFromServer();
    } else {
        qDebug()<<"dlgIRC::reconnect() -- opening new connection.";
        
        mChannels.clear();
        
        session->open();
        //session->connectToServer();
        // finalize the reconnect, only if the reconnect timer isn't active.
        if( ! ircReconnectActive ) {
            ircReconnectCalled = false;
        }
    }
}
 
void dlgIRC::reconnect_timeout() {
    if( mIrcConnected ) {
        return;
    }

    reconnect();
    ircReconnectActive = false;

    if( ! mIrcConnected ) {
        rc_timer->start( ircReconnectWait );
    }
}

void dlgIRC::onSocketStateChanged( QAbstractSocket::SocketState sockState ) {

    mIrcSocketState = sockState;

    qDebug()<< "dlgIRC::onSocketStateChanged() " << sockState << " StateNum: "<< mIrcSocketState;

    if( sockState == QAbstractSocket::ClosingState || sockState == QAbstractSocket::UnconnectedState ) {
        mIrcConnected = false;
    }
}

void dlgIRC::onSocketError( QAbstractSocket::SocketError sockErr ) {

    mIrcConnected = false;
    ircReconnectCalled = false;

    qDebug() << "dlgIRC::onSocketError() - " << sockErr ;

    // Attempt to reconnect after a short wait.
    if( ! rc_timer->isActive() && ! ircReconnectActive ) {
        qDebug() << "dlgIRC::onSocketError() Reconnect Timer #"<< ircReconnectCur <<" Started";
        ircReconnectCur = ircReconnectCur + 1;
        ircReconnectActive = true;
        rc_timer->start( ircReconnectWait );
    }
}

bool dlgIRC::ircJoin( QString jChan ) {
    if( isValidChannel(jChan) ) {
        int idxChan = mChannels.indexOf( jChan );
        
        // if the jChan exists already we should abort this join.
        if( idxChan != -1 ) {
            dlgWriteMsg( QString("<font color=#C90C0C>You are already in the channel named '%1'</font>").arg(jChan), "" );
            return false;
        }
        
        QString msg = QString("<font color=#aa00aa>Joining channel %1</font>").arg(jChan);
        dlgWriteMsg( msg, "" );
        session->sendCommand( IrcCommand::createJoin( jChan ) );
        mChannels.append( jChan );
        return true;
    } else {
        dlgWriteMsg( QString("<font color=#C90C0C>Could not join channel.<br> '%1' does not appear to be a valid channel name.</font>").arg(jChan), "" );
    }
    
    return false;
}

bool dlgIRC::ircPart( QString pChan ) {
    if( isValidChannel(pChan) ) {
        int idxChan = mChannels.indexOf( pChan );
        
        // Handle directly /part'ing the main channel.
        if( pChan == mChannel ) {
            int chSize = mChannels.size();
            // If we have a channel to switch to, do that.
            if( chSize > 1 ) {
                QStringList chListTmp = mChannels;
                chListTmp.removeAll( mChannel );
                dlgWriteMsg( QString("<font color=#aa00aa>Switching Main Channel to: %1</font>").arg( chListTmp[0] ), "" );
                setMainChannel( chListTmp[0] );
                return true;
            } 
            else // Otherwise close the connection (??)
            {
                dlgWriteMsg( "<font color=#aa00aa>You've left every channel. Closing IRC Session now.</font>", "" );
                dlgWriteMsg( "<font color=#aa00aa>Use /startirc to reconnect again. </font>", "" );
                dlgWriteMsg( "<font color=#aa00aa>Or use /joinserver (servername[:port]) [channel]. </font>", "" );
                
                mChannels.removeOne( pChan );
                
                session->close();
                //session->disconnectFromServer();
                ircReconnectCalled = false;
                mIrcConnected = false;
                mUseIrcClient = false;
                return true;
            }
        }
        
        // Regular /part handling.
        if( idxChan != -1 ) {
            QString msg = QString("<font color=#aa00aa>Leaving channel %1</font>").arg(pChan);
            dlgWriteMsg( msg, "" );
            session->sendCommand( IrcCommand::createPart( pChan ) );
            mChannels.removeOne( pChan );
            
            return true;
        }
            
        if( idxChan == -1 ) {
            dlgWriteMsg( QString("<font color=#C90C0C>Could not leave channel '%1' because you never joined it.</font>").arg(pChan), "" );
        }
    } else {
        dlgWriteMsg( QString("<font color=#C90C0C>Could not leave channel.<br> '%1' does not appear to be a valid channel name.</font>").arg(pChan), ""  );
    }
    
    return false;
}

/** ircGetNicks( QString channel ) 
 *  
 *  When called, preforms a nickist update and triggers the postIrcStatusMessage event process.
 *   if the channel has previously been joined/examined a cached list of nicks are returned.
 *
 **/
QString dlgIRC::ircGetNicks( QString lookupChan ) {
    QString rStr = "";
    
    if( isValidChannel( lookupChan ) && mIrcConnected ) {
        // If we already have the name list, pass it back.
        rStr = mChUserLists.value( lookupChan, "" );
        
        // Start the event process.
        eventGetNicksFor.append( lookupChan );
        session->sendCommand( IrcCommand::createNames( lookupChan ) );
        //session->cmdNames( lookupChan );
    }
    
    return rStr;
}

void dlgIRC::ircWhoIs( QString nick ) {
    if( !mIrcConnected ) {
        return;
    }
    
    eventWhoisFor.append( nick );
    
    IrcCommand *cmd = IrcCommand::createWhois( nick );
    session->sendCommand( cmd );
    //session->cmdWhois( nick );
}

void dlgIRC::ircWhoWas( QString nick ) {
    if( !mIrcConnected ) {
        return;
    }
    
    eventWhowasFor.append( nick );
    
    IrcCommand *cmd = IrcCommand::createWhowas( nick );
    session->sendCommand( cmd );
    //QString cmdStr = QString("WHOWAS %1 %1").arg( nick );
    //session->sendRaw( cmdStr );
}

// Returns True if already an OP, Triggers sysIrcStatusMessage.
void dlgIRC::ircOper( QString user, QString pass ) {
    if( !mIrcConnected ) {
        return;
    }
    
    eventOperCount++;
    
    QString cmdStr = QString( "OPER %1 %2" ).arg( user ).arg( pass );
    session->sendRaw( cmdStr );
}

// returns boolean true if the client has IRC Operator status.
bool dlgIRC::ircIsOper() {
    return mIsOper;
}

// used to give chanop
void dlgIRC::ircOp( QString nick, QString chan = "" ) {
    if( !mIrcConnected ) {
        return;
    }
    
    if( chan == "" ) {
        chan = mChannel;
    }
    
    //session->mode( chan, QString("+o %1").arg( nick ) );
    session->sendCommand( IrcCommand::createMode( nick, "+o" ) );
    if( chan != mChannel ) {
        //session->cmdNames( chan );
        session->sendCommand( IrcCommand::createNames( chan ) );
    }
    session->sendCommand( IrcCommand::createNames( mChannel ) );
    //session->cmdNames( mChannel );
}

void dlgIRC::ircDeOp( QString nick, QString chan = "" ) {
    if( !mIrcConnected ) {
        return;
    }
    
    if( chan == "" ) {
        chan = mChannel;
    }
    
    //session->mode( chan, QString("+o %1").arg( nick ) );
    session->sendCommand( IrcCommand::createMode( nick, "-o" ) );
    if( chan != mChannel ) {
        //session->cmdNames( chan );
        session->sendCommand( IrcCommand::createNames( chan ) );
    }
    session->sendCommand( IrcCommand::createNames( mChannel ) );
    //session->cmdNames( mChannel );
}

// returns boolean true if the client has CHANOP in given channel.
bool dlgIRC::ircIsChanOp( QString chan = "" ) {
    if( !mIrcConnected ) {
        return false;
    }
    
    if( chan == "" ) {
        chan = mChannel;
    }
    
    if( mChanOpList.indexOf( chan ) != -1 ) {
        return true;
    } 
    
    return false;
}

void dlgIRC::ircTopic( QString target, QString newTopic = "" ) {
    if( !mIrcConnected ) {
        return;
    }
    
    eventTopicActiveFor.append( target );
    
    //session->topic( target, newTopic );
    session->sendCommand( IrcCommand::createTopic(target, newTopic ) );
    
    // get new topic if we set one.  This ensures the event is raised.
    if( newTopic.size() > 0 ) {
        //session->topic( target );
        session->sendCommand( IrcCommand::createTopic(target ) );
    }
}

void dlgIRC::ircMode( QString target, QString newModes = "" ) {
    if( !mIrcConnected ) {
        return;
    }
    
    eventModeActiveFor.append( target );
    
    //session->mode( target, newModes );
    session->sendCommand( IrcCommand::createMode( target, newModes ) );
    
    // get new modes if we set them.  This ensures the event is raised.
    if( newModes.size() > 0 ) {
        //session->mode( target );
        session->sendCommand( IrcCommand::createMode( target, newModes ) );
    }
}

void dlgIRC::onMessageReceived( IrcMessage * msg ) 
{
    switch( msg->type() )
    {
    case IrcMessage::Type::Join: {
        IrcJoinMessage *rmsg = static_cast<IrcJoinMessage*>(msg);
        slot_joined( rmsg->sender().name(), rmsg->channel() );
        break;
        }
    case IrcMessage::Type::Notice: {
        IrcNoticeMessage *rmsg = static_cast<IrcNoticeMessage*>(msg);
        irc_gotMsg( rmsg->sender().name(), rmsg->target(), rmsg->message() );
        break;
        }
    case IrcMessage::Type::Private: {
        IrcPrivateMessage *rmsg = static_cast<IrcPrivateMessage*>(msg);
        irc_gotMsg( rmsg->sender().name(), rmsg->target(), rmsg->message() );
        break;
        }
    case IrcMessage::Type::Numeric: {
        IrcNumericMessage *rmsg = static_cast<IrcNumericMessage*>(msg);
        irc_gotMsg3( rmsg->sender().name(), rmsg->code(), rmsg->parameters() );
        break;
        }
    case IrcMessage::Type::Part: {
        IrcPartMessage *rmsg = static_cast<IrcPartMessage*>(msg);
        slot_parted( rmsg->sender().name(), rmsg->channel(), rmsg->reason() );
        break;
        }
    case IrcMessage::Type::Unknown:
        irc_gotMsg2( msg->sender().name(), msg->parameters() );
        break;
    default:
        break;
    }
    /*
    Nick 	IrcNickMessage
    Quit 	IrcQuitMessage
    Topic 	IrcTopicMessage
    Invite 	IrcInviteMessage
    Kick 	IrcKickMessage
    Mode 	IrcModeMessage
    Ping 	IrcPingMessage
    Pong 	IrcPongMessage
    Error 	IrcErrorMessage
    */
}

void dlgIRC::irc_gotMsg( QString a, QString b, QString c )
{
    qDebug()<<"dlgIRC::irc_gotMsg() a<"<<a<<"> b<"<<b<<">"<<" c<"<<c<<">";
    mudlet::self()->getHostManager()->postIrcMessage( a, b, c );
    c.replace("<","&#60;");
    c.replace(">","&#62;");
    //const QString _t = QTime::currentTime().toString();

    QRegExp rx("(http://[^ ]*)");
    QStringList list;
    int pos = 0;

    while( (pos = rx.indexIn(c, pos)) != -1)
    {
        QString _l = "<a href=";
        _l.append( rx.cap(1) );
        _l.append(" >");
        c.insert(pos,_l);
        pos += (rx.matchedLength()*2)+9;
        c.insert(pos,"< /a>");
        pos += 5;
    }

    const QString msg = c;
    const QString n = a;
    QString t;
    if( b == mNick )
        t = tr("<font color=#ff0000>%1</font><font color=#000000>: %2</font>").arg(n).arg(msg);
    else if( a == mNick )
        t = tr("%1<font color=#004400>: %2</font>").arg(n).arg(msg);
    else
        t = tr("<font color=#0B78E5>%1</font><font color=#000000>: %2</font>").arg(n).arg(msg);
    
    dlgWriteMsg( t, b );
    
    /* Legacy code * /
    if( b == a )
        t = tr("<font color=#a5a5a5>[%1] </font>msg from <font color=#ff0000>%2</font><font color=#ff0000>: %3</font>").arg(_t).arg(n).arg(msg);
    else if( a == mNick )
        t = tr("<font color=#a5a5a5>[%1] </font><font color=#00aaaa>%2</font><font color=#004400>: %3</font>").arg(_t).arg(n).arg(msg);
    else
        t = tr("<font color=#a5a5a5>[%1] </font><font color=#0000ff>%2</font><font color=#000000>: %3</font>").arg(_t).arg(n).arg(msg);


    QString hi = QString("<font color=#aa00aa>%1</font>").arg(mNick);
    t.replace(mNick, hi);
    QTextCursor cur = irc->textCursor();
    cur.movePosition(QTextCursor::End);
    cur.insertBlock();
    cur.insertHtml(t);
    irc->verticalScrollBar()->triggerAction(QScrollBar::SliderToMaximum);
    /**/
}

void dlgIRC::irc_gotMsg2( QString a, QStringList c )
{
    qDebug() << "dlgIRC::irc_gotMsg2() a="<< a << " c="<< c;
    QString m = c.join(" ");
    m.replace("<","&#60;");
    m.replace(">","&#62;");
    //const QString _t = QTime::currentTime().toString();

    QRegExp rx("(http://[^ ]*)");
    QStringList list;

    int pos = 0;

    while( (pos = rx.indexIn(m, pos)) != -1)
    {
        QString _l = "<a href=";
        _l.append( rx.cap(1) );
        _l.append(" >");
        m.insert(pos,_l);
        pos += (rx.matchedLength()*2)+9;
        m.insert(pos,"< /a>");
        pos += 5;
    }
    
    const QString msg = m;
    
    QString t = tr("<font color=#5B828C>%1</font>").arg(msg);

    dlgWriteMsg( t, a );

    /* Legacy Code * /
    const QString msg = m;
    const QString n = a;
    QString t;
    if( msg.contains( mNick ) )
        t = tr("<font color=#a5a5a5>[%1] </font><font color=#ff0000>%2</font><font color=#000000>: %3</font>").arg(_t).arg(n).arg(msg);
    else
        t = tr("<font color=#a5a5a5>[%1] </font><font color=#0000ff>%2</font><font color=#000000>: %3</font>").arg(_t).arg(n).arg(msg);

    //QString t = tr("<font color=#aaaaaa>[%1] </font><font color=#ff0000>%2</font><font color=#000000>: %3</font>").arg(_t).arg(n).arg(msg);
    QTextCursor cur = irc->textCursor();
    cur.movePosition(QTextCursor::End);
    cur.insertBlock();
    cur.insertHtml(t);
    irc->verticalScrollBar()->triggerAction(QScrollBar::SliderToMaximum);
    /**/
}

void dlgIRC::irc_gotMsg3( QString a, uint code, QStringList c )
{
    qDebug()<<"dlgIRC::irc_gotMsg3() a=" << a <<" code="<< code <<" list="<< c;
    if( code == Irc::RPL_TOPIC && c.size()>2 )
    {
        // remove the nick and channel names from the message list for printing.
        QString chan = c[1];
        QStringList l = c;
        l.replaceInStrings( l[1], "" );
        l.replaceInStrings( l[0], "" );
        
        QString m = l.join(" ");
        QString mc = m;
        m.replace("<","&#60;");
        m.replace(">","&#62;");

        QRegExp rx("(http://[^ ]*)");
        int pos = 0;
        while( (pos = rx.indexIn(m, pos)) != -1)
        {
            QString _l = "<a href=";
            _l.append( rx.cap(1) );
            _l.append(" >");
            m.insert(pos,_l);
            pos += (rx.matchedLength()*2)+9;
            m.insert(pos,"< /a>");
            pos += 5;
        }

        QString msg = m;
        QString n = a;
        msg.replace( n, "" );
        QString t = tr("<font color=#0000ff>TOPIC:</font><font color=#00aa00> %1</font>").arg(msg);
        dlgWriteMsg( t, chan );
        
        if( eventTopicActiveFor.indexOf( chan ) != -1 ) {
            QString codeStr = QString::number( code );
            mudlet::self()->getActiveHost()->postIrcStatusMessage( chan, mc, codeStr );
        }
    }
    else if( code == Irc::RPL_NAMREPLY && c.size()>=3 )
    {
        QString nicks = c[3];
        QString chan = c[2];
        QStringList nList = nicks.split(" ");
        QString opNick = QString( "@%1" ).arg( mNick );
        nickList->clear();
        for( int i=0; i<nList.size(); i++ )
        {
            nickList->addItem( nList[i] );
            
            // Check for client having OP in this channel.
            if( nList[i] == opNick ) {
                if( mChanOpList.indexOf( chan ) != -1 ) {
                    mChanOpList.append( chan ); 
                }
            } else if( nList[i] == mNick ) {
                if( mChanOpList.indexOf( chan ) != -1 ) {
                    mChanOpList.removeOne( chan );
                }
            }
        }
        
        mChUserLists.insert( chan, nicks );
        if( eventGetNicksFor.size() != 0 ) {
            QString codeStr = QString::number( code );
            mudlet::self()->getActiveHost()->postIrcStatusMessage( chan, nicks, codeStr );
        }
    }
    else if( code == Irc::ERR_NICKNAMEINUSE )
    {
        QString m = c.join(" ");
        if( m.contains( "name is already in use" ) )
        {
            mNick.append("_");
            session->setNickName( mNick );
            //session->setNick( mNick );
            
            QString msg = QString("<font color=#aa00aa>Nickname already in use, changing it to: </font>%1 ").arg(mNick);
            dlgWriteMsg( msg, "" );
        }
    }
    else if( code == Irc::ERR_ERRONEUSNICKNAME )
    {
        QString badChars = "~!@#$%&*()+=:\\/\"'<>,.?";
        QString regExStr = QString( "[%1]{1}" ).arg( QRegExp::escape(badChars) );
        QRegExp re = QRegExp( regExStr );
        QString newNick = mNick;
        
        qDebug() << "BadChars: " << badChars << " RegExp: " << regExStr << "  Nick: " << mNick;
        
        if( re.indexIn( mNick ) == -1 ) {
            newNick = mNick.left( (mNick.length()-1) );
            
            if( newNick.size() == 0 ) {
                newNick = tr("Mudlet%1").arg(QString::number(rand()%1000));
            }
        } else {
            newNick = newNick.replace( re, "_" );
        }

        QString msg = QString("<font color=#aa00aa>Erroneus Nick: </font>%1 <font color=#aa00aa>Trying: </font>%2").arg(mNick).arg(newNick);
        dlgWriteMsg( msg, "" );
        setNick( newNick );

        //mNick = newNick;
        //session->setNickName( newNick );
        //session->sendCommand( IrcCommand::createNames( mChannel ) );        
    }
    else if( code == Irc::RPL_WHOISUSER || 
             code == Irc::RPL_WHOISSERVER || 
             code == Irc::RPL_WHOISOPERATOR || 
             code == Irc::RPL_WHOISCHANOP || 
             code == Irc::RPL_WHOISIDLE ||
             code == Irc::RPL_WHOISCHANNELS ||
             code == Irc::RPL_WHOISVIRT ||
             code == Irc::RPL_WHOWASUSER ||
             code == Irc::RPL_WHOISHOST ||
             code == Irc::RPL_WHOISMODES ) 
    {
        c.removeAt( 0 ); // Remove the irc client nick.
        QString whoisNick = c[0];
        c.removeAt( 0 ); // Remove the whois nick.
        
        if( ircWhoisActiveFor.indexOf( whoisNick ) != -1 ||
            ircWhowasActiveFor.indexOf( whoisNick ) != -1 ) 
        {
            irc_gotMsg2( whoisNick, c );
        }
        
        QString codeStr = QString::number( code );
        QString line = c.join(" ");
        
        if( eventWhoisFor.indexOf( whoisNick ) != -1 ) {
            mudlet::self()->getActiveHost()->postIrcStatusMessage( whoisNick, line, codeStr );
        }
        
        if( eventWhowasFor.indexOf( whoisNick ) != -1 &&
            (code == Irc::RPL_WHOISSERVER || code == Irc::RPL_WHOWASUSER) ) 
        {
            mudlet::self()->getActiveHost()->postIrcStatusMessage( whoisNick, line, codeStr );
        }
    }
    else if( code == Irc::RPL_ENDOFWHOIS ||
             code == Irc::ERR_NOSUCHSERVER )
    {
        c.removeAt( 0 ); // Remove the irc client nick.
        QString whoisNick = c[0];
        c.removeAt( 0 );
        
        if( ircWhoisActiveFor.indexOf( whoisNick ) != -1 ) {
            irc_gotMsg2( whoisNick, c );
            ircWhoisActiveFor.removeOne( whoisNick );
        }
        
        if( eventWhoisFor.indexOf( whoisNick ) != -1 ) {
            QString codeStr = QString::number( code );
            QString line = c.join(" ");
            mudlet::self()->getActiveHost()->postIrcStatusMessage( whoisNick, line, codeStr );
        
            eventWhoisFor.removeOne( whoisNick );
        }
    }
    else if( code == Irc::RPL_ENDOFWHOWAS ||
             code == Irc::ERR_WASNOSUCHNICK ) 
    {
        c.removeAt( 0 ); // Remove the irc client nick.
        QString whoisNick = c[0];
        c.removeAt( 0 );
        
        if( ircWhowasActiveFor.indexOf( whoisNick ) != -1 ) {
            irc_gotMsg2( whoisNick, c );
            ircWhowasActiveFor.removeOne( whoisNick );
        }
        
        if( eventWhowasFor.indexOf( whoisNick ) != -1 ) {
            QString codeStr = QString::number( code );
            QString line = c.join(" ");
            mudlet::self()->getActiveHost()->postIrcStatusMessage( whoisNick, line, codeStr );
            
            eventWhowasFor.removeOne( whoisNick );
        }
    }
    else if( code == Irc::RPL_ENDOFNAMES ) 
    {
        if( eventGetNicksFor.size() != 0 ) {
            c.removeAt( 0 ); // Remove the irc client nick.
            QString chan = c[0];
            c.removeAt( 0 ); // Remove the channel name.
            QString codeStr = QString::number( code );
            QString line = c.join(" ");
            mudlet::self()->getActiveHost()->postIrcStatusMessage( chan, line, codeStr );
            
            eventGetNicksFor.removeOne( chan );
        }
    }
    else if( code == Irc::RPL_UMODEIS || 
             code == Irc::RPL_CHANNELMODEIS || 
             code == Irc::RPL_CREATIONTIME ) 
    {
        QString mType = "User Modes";
        QString chan = c[0];  // channel defaults to nick.
        c.removeAt(0); // Remove Nick
        
        if( code == Irc::RPL_CHANNELMODEIS ) {
            mType = "Channel Modes";
            chan = c[0];
            c.removeAt(0); // Remove Channel Name
        }
        
        QString tStamp = "";
        if( code == Irc::RPL_CREATIONTIME  ) {
            mType = "Created On";
            chan = c[0];
            c.removeAt(0); // Remove Channel Name
            tStamp = c[0];
            
            // Format the timestamp as a date
            bool ok;
            uint ts = c[0].toUInt( &ok );
            if( ok ) {
                QDateTime dt = QDateTime::fromTime_t( ts );
                // Desired format: Fri Oct 14 12:53:07 2016
                QString dateStr = dt.toString( "ddd MMM dd hh:mm:ss yyyy" );
                c.replaceInStrings( c[0], dateStr );
            }
        }
        
        QString modeMsg = c.join(" ");
        
        if( eventModeActiveFor.indexOf( chan ) != -1 ) {
            QString sMM = modeMsg;
            if( code == Irc::RPL_CREATIONTIME ) {
                sMM = tStamp;
            }
            
            QString codeStr = QString::number( code );
            mudlet::self()->getActiveHost()->postIrcStatusMessage( chan, sMM, codeStr  );
            eventModeActiveFor.removeOne( chan );
        }
        
        modeMsg = QString( "<font color=#5B828C>%1: %2</font>" ).arg( mType ).arg( modeMsg );

        dlgWriteMsg( modeMsg, chan );
        
        qDebug() << "Mode event count: " << eventModeActiveFor.size() ;
    }
    else if( code == Irc::RPL_YOUREOPER ) 
    {
        mIsOper = true;
        
        dlgWriteMsg( c[1], c[0] );
        
        if( eventOperCount > 0 ) {
            QString name = c[0];
            c.removeAt( 0 ); // Remove the irc client nick.
            QString codeStr = QString::number( code );
            QString line = c.join(" ");
            mudlet::self()->getActiveHost()->postIrcStatusMessage( name, line, codeStr );
            
            eventOperCount--;
        }
        
        //session->cmdNames( mChannel );
        session->sendCommand( IrcCommand::createNames( mChannel ) );
    }
    else if( code == Irc::ERR_CHANOPRIVSNEEDED ) 
    {
        QString nick = c[0];
        QString chan = c[1];
        
        if( mChanOpList.indexOf( chan ) != -1 ) {
            mChanOpList.removeOne( chan ); 
        }
    }
    else if( code == Irc::RPL_TOPICWHOTIME ) 
    {
        c.removeAt(0); // remove nick
        QString chan = c[0];
        c.removeAt(0); // remove chan
        QString who = c[0];
        c.removeAt(0); // remove who-nick
        QString when = "";
        
        // Format the timestamp as a date
        bool ok;
        uint ts = c[0].toUInt( &ok );
        if( ok ) {
            QDateTime dt = QDateTime::fromTime_t( ts );
            // Desired format: Fri Oct 14 12:53:07 2016
            when = dt.toString( "ddd MMM dd hh:mm:ss yyyy" );
        }
        
        if( eventTopicActiveFor.indexOf( chan ) != -1 ) {
            QString codeStr = QString::number( code );
            QString tStr = QString("%1 %2").arg( who ).arg( c[0] );
            mudlet::self()->getActiveHost()->postIrcStatusMessage( chan, tStr, codeStr );
            eventTopicActiveFor.removeOne( chan );
        }
        
        QString Msg = QString( "<font color=#5B828C>Topic Set by: %1  On: %2</font>" ).arg( who ).arg( when );
        dlgWriteMsg( Msg, chan );
        return;
    }
    else if( code == Irc::RPL_NOTOPIC )
    {
        c.removeAt(0); // remove nick
        QString chan = c[0];
        c.removeAt(0); // remove chan
        QString tStr = c[0];
        
        if( eventTopicActiveFor.indexOf( chan ) != -1 ) {
            QString codeStr = QString::number( code );
            mudlet::self()->getActiveHost()->postIrcStatusMessage( chan, tStr, codeStr );
            eventTopicActiveFor.removeOne( chan );
        }
        
        QString Msg = QString( "<font color=#5B828C>%1</font>" ).arg( tStr );
        dlgWriteMsg( Msg, chan );
        return;
    }
    else
    {
        irc_gotMsg2("", c.replaceInStrings(mNick, "") );
    }
}

void dlgIRC::slot_joined(QString nick, QString chan )
{
    QString t = tr("<font color=#008800> %1 has joined the channel.</font>").arg(nick);
    dlgWriteMsg( t, chan );
    nickList->addItem( nick );
}

void dlgIRC::slot_parted(QString nick, QString chan, QString msg )
{
    QString t = tr("<font color=#888800> %1 has left the channel. \"%2\"</font>").arg(nick).arg(msg);
    dlgWriteMsg( t, chan );
    nickList->clear();
    //session->cmdNames( mChannel );
    session->sendCommand( IrcCommand::createNames( mChannel ) );
}

bool dlgIRC::isValidChannel( QString chan ) {
    
    if( (chan.startsWith("#") || 
         chan.startsWith("&") || 
         chan.startsWith("+") ) && 
         chan.size() > 1 && 
         chan.indexOf(" ") == -1 ) 
    {
        return true;
    }
    
    return false;
}

void dlgIRC::dlgWriteMsg(QString msg, QString chan) {
    
    // Highlight bots nick name in messages.
    QString hi = QString("<font color=#C4761D>%1</font>").arg(mNick);
    msg.replace(mNick, hi);
    
    // Format the Channel name, if there is one.
    if( chan.size() > 0 ) {
        if( chan == mNick ) {
            chan = "PrvMsg";
        }
        chan = QString("[%1]").arg( chan );
    }
    
    // Add current timestamp and channel name to the message text.
    QString _t = QTime::currentTime().toString();
    msg = QString("<font color=#686868>[%1]%2</font> %3").arg( _t ).arg(chan).arg( msg );
    
    // Update the chat box.
    QTextCursor cur = irc->textCursor();
    cur.movePosition(QTextCursor::End);
    cur.insertBlock();
    cur.insertHtml(msg);
    irc->verticalScrollBar()->triggerAction(QScrollBar::SliderToMaximum);
    
    //nickList->clear();
    //session->cmdNames( mChannel );
}


void dlgIRC::anchorClicked(const QUrl& link)
{
    QDesktopServices::openUrl(link);
}

