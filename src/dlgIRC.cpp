

/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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
#include <QDebug>
#include <QDesktopServices>
#include <QScrollBar>
//#define IRC_SHARED
#include "dlgIRC.h"
//#include "irc/include/ircsession.h"

//#include <ircsession.h>
#include "mudlet.h"

dlgIRC::dlgIRC()
{
    setupUi(this);
    session = new Irc::Session(this);
    irc->setOpenExternalLinks ( true );
    setUnifiedTitleAndToolBarOnMac( true );
    connect( irc, SIGNAL(anchorClicked(QUrl)), this, SLOT(anchorClicked(QUrl)));
    connect( session, SIGNAL(msgMessageReceived(const QString &, const QString &, const QString &)), this, SLOT(irc_gotMsg(QString, QString, QString)));
    connect( session, SIGNAL(msgNoticeReceived(const QString &, const QString &, const QString &)), this, SLOT(irc_gotMsg(QString, QString, QString)));
    connect( session, SIGNAL(msgUnknownMessageReceived(const QString &, const QStringList &)), this, SLOT(irc_gotMsg2(QString, QStringList)));
    connect( session, SIGNAL(msgNumericMessageReceived(const QString &, uint, const QStringList &)), this, SLOT(irc_gotMsg3(QString, uint, QStringList)));
    connect( lineEdit, SIGNAL(returnPressed()), this, SLOT(sendMsg()));
    connect( session, SIGNAL(msgJoined(const QString &, const QString &)), this, SLOT(slot_joined(QString, QString)));
    connect( session, SIGNAL(msgParted(const QString &, const QString &, const QString &)), this, SLOT(slot_parted(QString, QString, QString)));

    QStringList chans;
    chans << "#mudlet";
    session->setAutoJoinChannels( chans );
    QString nick = tr("Mudlet%1").arg(QString::number(rand()%10000));
    session->setNick(nick);
    session->setIdent("mudlet");
    session->setRealName(mudlet::self()->version);
    session->connectToServer("irc.freenode.net", 6667);
}

void dlgIRC::sendMsg()
{
    QString txt = lineEdit->text();
    lineEdit->clear();
    session->cmdMessage("#mudlet", txt);
    session->cmdNames( "#mudlet" );
}

#include <QTime>
#include <QString>

void dlgIRC::irc_gotMsg( QString a, QString b, QString c )
{
    const QString _t = QTime::currentTime().toString();

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
    QString t = tr("<font color=#0000ff>[%1] </font><font color=#ff0000>%2</font><font color=#000000>: %3</font>").arg(_t).arg(n).arg(msg);
    QTextCursor cur = irc->textCursor();
    cur.movePosition(QTextCursor::End);
    cur.insertBlock();
    cur.insertHtml(t);
    irc->verticalScrollBar()->triggerAction(QScrollBar::SliderToMaximum);
}

void dlgIRC::irc_gotMsg2( QString a, QStringList c )
{
    QString m = c.join(" ");
    const QString _t = QTime::currentTime().toString();

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
    const QString n = a;
    QString t = tr("<font color=#0000ff>[%1] </font><font color=#ff0000>%2</font><font color=#000000>: %3</font>").arg(_t).arg(n).arg(msg);
    QTextCursor cur = irc->textCursor();
    cur.movePosition(QTextCursor::End);
    cur.insertBlock();
    cur.insertHtml(t);
    irc->verticalScrollBar()->triggerAction(QScrollBar::SliderToMaximum);
}
void dlgIRC::irc_gotMsg3( QString a, uint code, QStringList c )
{
    qDebug()<<"code="<<code<<" list="<<c;
    if( code == 332 && c.size()>2 )
    {
        QString m = c.join(" ");
        const QString _t = QTime::currentTime().toString();

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
        const QString n = a;
        QString t = tr("<font color=#0000ff>[%1] (%2) </font><font color=#ff0000>%3</font><font color=#000000>: %4</font>").arg(_t).arg(code).arg(n).arg(msg);
        QTextCursor cur = irc->textCursor();
        cur.movePosition(QTextCursor::End);
        cur.insertBlock();
        cur.insertHtml(t);
    }
    if( code == 353 && c.size()>=3 )
    {
        QString nicks = c[3];
        QStringList nList = nicks.split(" ");
        nickList->clear();
        for( int i=0; i<nList.size(); i++ )
        {
            nickList->addItem( nList[i]);
        }
    }
    irc->verticalScrollBar()->triggerAction(QScrollBar::SliderToMaximum);
}

void dlgIRC::slot_joined(QString nick, QString chan )
{
    const QString _t = QTime::currentTime().toString();
    QString t = tr("<font color=#008800>[%1] %2 has joined the channel.</font>").arg(_t).arg(nick);
    QTextCursor cur = irc->textCursor();
    cur.movePosition(QTextCursor::End);
    cur.insertBlock();
    cur.insertHtml(t);
    irc->verticalScrollBar()->triggerAction(QScrollBar::SliderToMaximum);
    nickList->addItem( nick );
}

void dlgIRC::slot_parted(QString nick, QString chan, QString msg )
{
    const QString _t = QTime::currentTime().toString();
    QString t = tr("<font color=#888800>[%1] %2 has left the channel.</font>").arg(_t).arg(nick);
    QTextCursor cur = irc->textCursor();
    cur.movePosition(QTextCursor::End);
    cur.insertBlock();
    cur.insertHtml(t);
    irc->verticalScrollBar()->triggerAction(QScrollBar::SliderToMaximum);
    nickList->clear();
    session->cmdNames( "#mudlet" );
}


void dlgIRC::anchorClicked(const QUrl& link)
{
    QDesktopServices::openUrl(link);
}

