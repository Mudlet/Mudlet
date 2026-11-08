/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vperetokin@hey.com            *
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

#ifndef REFUSED_PORT_H
#define REFUSED_PORT_H

#include <QtNetwork/QTcpSocket>

// A localhost port that refuses connections for as long as this lives. A port taken from a
// server that then closes is free again, and ctest runs other tests at the same time: one of
// them can be given it and accept the connection meant to fail. Bound without listening, it
// refuses connections and the system hands it to no one else.
class RefusedPort
{
public:
    bool reserve() { return mSocket.bind(QHostAddress::LocalHost, 0, QAbstractSocket::DontShareAddress); }
    quint16 port() const { return mSocket.localPort(); }

private:
    QTcpSocket mSocket;
};

#endif // REFUSED_PORT_H
