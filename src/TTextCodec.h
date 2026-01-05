#ifndef TTEXTCODEC_H
#define TTEXTCODEC_H
/***************************************************************************
 *   Copyright (C) 2020, 2024 by Stephen Lyons - slysven@virginmedia.com   *
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

/***************************************************************************
 *   This class provides custom text encodings for character sets not      *
 *   available through Qt's standard QStringConverter or for special       *
 *   cases. These are standalone converter classes that handle conversion  *
 *   between byte arrays and Unicode strings.                              *
 ***************************************************************************/

#include <QByteArray>
#include <QList>
#include <QString>
#include <QVector>


class TTextCodec_437
{
public:
    TTextCodec_437() = default;
    ~TTextCodec_437() = default;

    static QByteArray name();
    static QList<QByteArray> aliases();
    static int mibEnum();
    static QString toUnicode(const QByteArray& bytes);
    static QByteArray fromUnicode(const QString& str);
    static bool canEncode(const QString& str);

private:
    static const QVector<QChar> CptoUnicode;
};

class TTextCodec_667
{
public:
    TTextCodec_667() = default;
    ~TTextCodec_667() = default;

    static QByteArray name();
    static QList<QByteArray> aliases();
    static int mibEnum();
    static QString toUnicode(const QByteArray& bytes);
    static QByteArray fromUnicode(const QString& str);
    static bool canEncode(const QString& str);

private:
    static const QVector<QChar> CptoUnicode;
};

class TTextCodec_737
{
public:
    TTextCodec_737() = default;
    ~TTextCodec_737() = default;

    static QByteArray name();
    static QList<QByteArray> aliases();
    static int mibEnum();
    static QString toUnicode(const QByteArray& bytes);
    static QByteArray fromUnicode(const QString& str);
    static bool canEncode(const QString& str);

private:
    static const QVector<QChar> CptoUnicode;
};

class TTextCodec_869
{
public:
    TTextCodec_869() = default;
    ~TTextCodec_869() = default;

    static QByteArray name();
    static QList<QByteArray> aliases();
    static int mibEnum();
    static QString toUnicode(const QByteArray& bytes);
    static QByteArray fromUnicode(const QString& str);
    static bool canEncode(const QString& str);

private:
    static const QVector<QChar> CptoUnicode;
};

class TTextCodec_medievia
{
public:
    TTextCodec_medievia() = default;
    ~TTextCodec_medievia() = default;

    static QByteArray name();
    static QList<QByteArray> aliases();
    static int mibEnum();
    static QString toUnicode(const QByteArray& bytes);
    static QByteArray fromUnicode(const QString& str);
    static bool canEncode(const QString& str);

private:
    static const QVector<QChar> CptoUnicode;
};

#endif // TTEXTCODEC_H
