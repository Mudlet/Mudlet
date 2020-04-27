#ifndef TTEXTCODEC_H
#define TTEXTCODEC_H
/***************************************************************************
 *   Copyright (C) 2020 by Stephen Lyons - slysven@virginmedia.com         *
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
 *   This class is entirely concerned with providing some codecs on        *
 *   platforms that do not come with a Qt provided QTextCodec for the      *
 *   encodings - which seems to be the Windows AppVeyor CI at this time.   *
 ***************************************************************************/

#include "pre_guard.h"
#include <QTextCodec>
#include <QVector>
#include "post_guard.h"


class TTextCodec_437 : private QTextCodec
{
public:
    TTextCodec_437() = default;
    ~TTextCodec_437() = default;

    QByteArray name() const override;
    QList<QByteArray> aliases() const override;
    int mibEnum() const override;
    QString convertToUnicode(const char *in, int length, ConverterState *state) const override;
    QByteArray convertFromUnicode(const QChar *in, int length, ConverterState *state) const override;

private:
    static const QVector<QChar> CptoUnicode;
};

class TTextCodec_667 : private QTextCodec
{
public:
    TTextCodec_667() = default;
    ~TTextCodec_667() = default;

    QByteArray name() const override;
    QList<QByteArray> aliases() const override;
    int mibEnum() const override;
    QString convertToUnicode(const char *in, int length, ConverterState *state) const override;
    QByteArray convertFromUnicode(const QChar *in, int length, ConverterState *state) const override;

private:
    static const QVector<QChar> CptoUnicode;
};

class TTextCodec_737 : private QTextCodec
{
public:
    TTextCodec_737() = default;
    ~TTextCodec_737() = default;

    QByteArray name() const override;
    QList<QByteArray> aliases() const override;
    int mibEnum() const override;
    QString convertToUnicode(const char *in, int length, ConverterState *state) const override;
    QByteArray convertFromUnicode(const QChar *in, int length, ConverterState *state) const override;

private:
    static const QVector<QChar> CptoUnicode;
};

class TTextCodec_869 : private QTextCodec
{
public:
    TTextCodec_869() = default;
    ~TTextCodec_869() = default;

    QByteArray name() const override;
    QList<QByteArray> aliases() const override;
    int mibEnum() const override;
    QString convertToUnicode(const char *in, int length, ConverterState *state) const override;
    QByteArray convertFromUnicode(const QChar *in, int length, ConverterState *state) const override;

private:
    static const QVector<QChar> CptoUnicode;
};
#endif // TTEXTCODEC_H
