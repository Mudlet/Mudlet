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
#ifdef Q_CC_MSVC
#include <ircsession.h>
#else
#include "irc/include/ircsession.h"
#endif
#include "post_guard.h"


class dlgIRC : public QMainWindow, public Ui::irc_dlg
{
    Q_OBJECT

public:
    dlgIRC();
    Irc::Session* session;
    QString mNick;

public slots:
    void irc_gotMsg(QString, QString, QString);
    void irc_gotMsg2(QString a, QStringList c);
    void irc_gotMsg3(QString a, uint code, QStringList c);
    void anchorClicked(const QUrl& link);
    void slot_joined(QString, QString);
    void slot_parted(QString, QString, QString);
    void sendMsg();
};

#endif // MUDLET_DLGIRC_H
