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

#include <QRandomGenerator>
#include <QtNetwork/QTcpServer>

// A localhost port nothing listens on. A free port the system picked can be handed straight to a
// test ctest runs alongside, which then accepts the connection meant to be refused; systems only
// pick from 32768 up, so this takes one below that, and below the 20000 up the Lua specs pick from.
// Holding it bound without listening is no use: macOS drops connections to such a port.
class RefusedPort
{
public:
    bool reserve()
    {
        for (int attempt = 0; attempt < 100; ++attempt) {
            const auto candidate = static_cast<quint16>(QRandomGenerator::global()->bounded(scmFirst, scmLast));
            QTcpServer probe;
            if (probe.listen(QHostAddress::LocalHost, candidate)) {
                mPort = candidate;
                return true;
            }
        }
        return false;
    }
    quint16 port() const { return mPort; }

private:
    static constexpr int scmFirst = 10000;
    static constexpr int scmLast = 20000;
    quint16 mPort = 0;
};

#endif // REFUSED_PORT_H
