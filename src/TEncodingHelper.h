/***************************************************************************
 *   Copyright (C) 2025 by Mike Conley - mike.conley@stickmud.com          *
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

#ifndef TENCODINGHELPER_H
#define TENCODINGHELPER_H

/***************************************************************************
 *   This class provides helper functions for text encoding/decoding       *
 *   using both Qt6's QStringConverter and custom codecs.                  *
 ***************************************************************************/

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringConverter>
#include <optional>

class TEncodingHelper
{
public:
    static QString decode(const QByteArray& bytes, const QByteArray& encoding);
    static QByteArray encode(const QString& str, const QByteArray& encoding);
    static bool canEncode(const QString& str, const QByteArray& encoding);
    static bool isEncodingAvailable(const QByteArray& encoding);
    static QList<QByteArray> aliases(const QByteArray& encoding);
    
private:
    static bool isCustomEncoding(const QByteArray& encoding);
    static std::optional<QStringConverter::Encoding> getQtEncoding(const QByteArray& encoding);
};

#endif // TENCODINGHELPER_H
