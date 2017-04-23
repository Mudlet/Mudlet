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


#include "pre_guard.h"
#include "ui_irc.h"
#include <Irc>
#include <IrcCommand>
#include <IrcConnection>
#include <IrcMessage>
#include "post_guard.h"


class dlgIRC : public QMainWindow, public Ui::irc_dlg
{
    Q_OBJECT

    Q_DISABLE_COPY(dlgIRC)

public:
    dlgIRC();

    IrcConnection* connection;

public slots:
    void onMessageReceived(IrcMessage*);
    void anchorClicked(const QUrl& link);
    void sendMsg();
    void onConnected();

private:
    void irc_gotMsg(QString, QString, QString);
    void irc_gotMsg2(QString a, QStringList c);
    void irc_gotMsg3(QString a, uint code, QStringList c);
    void slot_joined(QString, QString);
    void slot_parted(QString, QString, QString);

    QString mNick;
};

#endif // MUDLET_DLGIRC_H
