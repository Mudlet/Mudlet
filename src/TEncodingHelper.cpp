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

#include "TEncodingHelper.h"
#include "TTextCodec.h"
#include <QDebug>

bool TEncodingHelper::isCustomEncoding(const QByteArray& encoding)
{
    return encoding.startsWith("M_") ||
           encoding == "CP437" ||
           encoding == "CP667" ||
           encoding == "CP737" ||
           encoding == "CP869" ||
           encoding == "MEDIEVIA";
}

std::optional<QStringConverter::Encoding> TEncodingHelper::getQtEncoding(const QByteArray& encoding)
{
    auto enc = QStringConverter::encodingForName(encoding.constData());
    return enc;
}

QString TEncodingHelper::decode(const QByteArray& bytes, const QByteArray& encoding)
{
    if (encoding == "M_CP437" || encoding == "CP437") {
        return TTextCodec_437::toUnicode(bytes);
    }
    if (encoding == "M_CP667" || encoding == "CP667") {
        return TTextCodec_667::toUnicode(bytes);
    }
    if (encoding == "M_CP737" || encoding == "CP737") {
        return TTextCodec_737::toUnicode(bytes);
    }
    if (encoding == "M_CP869" || encoding == "CP869") {
        return TTextCodec_869::toUnicode(bytes);
    }
    if (encoding == "M_MEDIEVIA" || encoding == "MEDIEVIA") {
        return TTextCodec_medievia::toUnicode(bytes);
    }
    
    auto qtEnc = getQtEncoding(encoding);
    if (qtEnc) {
        QStringDecoder decoder(*qtEnc);
        return decoder.decode(bytes);
    }
    
    return QString::fromLatin1(bytes);
}

QByteArray TEncodingHelper::encode(const QString& str, const QByteArray& encoding)
{
    if (encoding == "M_CP437" || encoding == "CP437") {
        return TTextCodec_437::fromUnicode(str);
    }
    if (encoding == "M_CP667" || encoding == "CP667") {
        return TTextCodec_667::fromUnicode(str);
    }
    if (encoding == "M_CP737" || encoding == "CP737") {
        return TTextCodec_737::fromUnicode(str);
    }
    if (encoding == "M_CP869" || encoding == "CP869") {
        return TTextCodec_869::fromUnicode(str);
    }
    if (encoding == "M_MEDIEVIA" || encoding == "MEDIEVIA") {
        return TTextCodec_medievia::fromUnicode(str);
    }
    
    auto qtEnc = getQtEncoding(encoding);
    if (qtEnc) {
        QStringEncoder encoder(*qtEnc);
        return encoder.encode(str);
    }
    
    return str.toLatin1();
}

bool TEncodingHelper::canEncode(const QString& str, const QByteArray& encoding)
{
    if (encoding == "M_CP437" || encoding == "CP437") {
        return TTextCodec_437::canEncode(str);
    } else if (encoding == "M_CP667" || encoding == "CP667") {
        return TTextCodec_667::canEncode(str);
    } else if (encoding == "M_CP737" || encoding == "CP737") {
        return TTextCodec_737::canEncode(str);
    } else if (encoding == "M_CP869" || encoding == "CP869") {
        return TTextCodec_869::canEncode(str);
    } else if (encoding == "M_MEDIEVIA" || encoding == "MEDIEVIA") {
        return TTextCodec_medievia::canEncode(str);
    }
    
    auto qtEnc = getQtEncoding(encoding);
    if (qtEnc) {
        QStringEncoder encoder(*qtEnc);
        QByteArray encoded = encoder.encode(str);
        return encoder.hasError() == false;
    }
    
    // Unknown encoding - cannot encode
    return false;
}

bool TEncodingHelper::isEncodingAvailable(const QByteArray& encoding)
{
    if (isCustomEncoding(encoding)) {
        return true;
    }
    
    auto qtEnc = getQtEncoding(encoding);
    return qtEnc.has_value();
}

QList<QByteArray> TEncodingHelper::aliases(const QByteArray& encoding)
{
    if (encoding == "M_CP437" || encoding == "CP437") {
        return TTextCodec_437::aliases();
    } else if (encoding == "M_CP667" || encoding == "CP667") {
        return TTextCodec_667::aliases();
    } else if (encoding == "M_CP737" || encoding == "CP737") {
        return TTextCodec_737::aliases();
    } else if (encoding == "M_CP869" || encoding == "CP869") {
        return TTextCodec_869::aliases();
    } else if (encoding == "M_MEDIEVIA" || encoding == "MEDIEVIA") {
        return TTextCodec_medievia::aliases();
    }
    
    return {};
}
