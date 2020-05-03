/***************************************************************************
 *   Copyright (C) 2020 by Gustavo Sousa - gustavocms@gmail.com            *
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

#include "TEntityResolver.h"

QString TEntityResolver::getResolution(const QString& entity) const
{
    if (entity.front() != '&' || entity.back() != ';') {
        return entity;
    }

    auto ptr = mEntititesMap.find(entity.toLower());
    if (ptr != mEntititesMap.end()) {
        return *ptr;
    }

    auto stdPtr = scmStandardEntites.find(entity.toLower());
    if (stdPtr != scmStandardEntites.end()) {
        return *stdPtr;
    }


    return entity[1] == '#' ? resolveCode(entity.mid(2, entity.size() - 3)) : entity;
}

bool TEntityResolver::registerEntity(const QString& entity, const QString& str)
{
    if (entity.front() != '&' || entity.back() != ';') {
        return false;
    }


    mEntititesMap[entity.toLower()] = str;
    return true;
}

bool TEntityResolver::unregisterEntity(const QString & entity){
    return mEntititesMap.remove(entity) > 0;
}

QString TEntityResolver::resolveCode(const QString& entityValue)
{
    return entityValue.front() == 'x' ? resolveCode(entityValue.mid(1), 16) : resolveCode(entityValue, 10);
}

QString TEntityResolver::resolveCode(const QString& entityValue, int base)
{
    bool isNum = false;
    ushort code = entityValue.toUShort(&isNum, base);
    return isNum ? resolveCode(code) : entityValue;
}

QString TEntityResolver::resolveCode(ushort val)
{
    return QString(QChar(val));
}

QString TEntityResolver::interpolate(const QString& input, std::function<QString(const QString&)> resolver)
{
    QString output;
    QString entity;

    for (const auto& ch : input) {
        if (ch == ';' && !entity.isEmpty()) {
            entity.append(ch);
            output.append(resolver(entity));
            entity.clear();
        } else if (ch == '&' || !entity.isEmpty()) {
            entity.append(ch);
        } else {
            output.append(ch);
        }
    }

    output.append(entity);
    return output;
}

QString TEntityResolver::interpolate(const QString& input) const
{
    return interpolate(input, [this](const QString& it) { return getResolution(it); });
}

const TEntityResolver TEntityResolver::scmDefaultResolver = TEntityResolver();

// clang-format off
const QHash<QString, QString> TEntityResolver::scmStandardEntites = {
        {QStringLiteral("&tab;"), QStringLiteral("\t")},
        {QStringLiteral("&newline;"), QStringLiteral("\n")},
        {QStringLiteral("&excl;"), QStringLiteral("!")},
        {QStringLiteral("&quot;"), QStringLiteral("\"")},
        {QStringLiteral("&num;"), QStringLiteral("#")},
        {QStringLiteral("&dollar;"), QStringLiteral("$")},
        {QStringLiteral("&percnt;"), QStringLiteral("%")},
        {QStringLiteral("&amp;"), QStringLiteral("&")},
        {QStringLiteral("&apos;"), QStringLiteral("'")},
        {QStringLiteral("&lpar;"), QStringLiteral("(")},
        {QStringLiteral("&rpar;"), QStringLiteral(")")},
        {QStringLiteral("&ast;"), QStringLiteral("*")},
        {QStringLiteral("&plus;"), QStringLiteral("+")},
        {QStringLiteral("&comma;"), QStringLiteral(",")},
        {QStringLiteral("&period;"), QStringLiteral(".")},
        {QStringLiteral("&sol;"), QStringLiteral("/")},
        {QStringLiteral("&colon;"), QStringLiteral(":")},
        {QStringLiteral("&semi;"), QStringLiteral(";")},
        {QStringLiteral("&lt;"), QStringLiteral("<")},
        {QStringLiteral("&equals;"), QStringLiteral("=")},
        {QStringLiteral("&gt;"), QStringLiteral(">")},
        {QStringLiteral("&quest;"), QStringLiteral("?")},
        {QStringLiteral("&commat;"), QStringLiteral("@")},
        {QStringLiteral("&lsqb;"), QStringLiteral("[")},
        {QStringLiteral("&bsol;"), QStringLiteral("\\")},
        {QStringLiteral("&rsqb;"), QStringLiteral("]")},
        {QStringLiteral("&hat;"), QStringLiteral("^")},
        {QStringLiteral("&lowbar;"), QStringLiteral("_")},
        {QStringLiteral("&grave;"), QStringLiteral("`")},
        {QStringLiteral("&lcub;"), QStringLiteral("{")},
        {QStringLiteral("&verbar;"), QStringLiteral("|")},
        {QStringLiteral("&rcub;"), QStringLiteral("}")},
        {QStringLiteral("&nbsp;"), QStringLiteral(" ")},
        {QStringLiteral("&iexcl;"), QStringLiteral("¡")},
        {QStringLiteral("&cent;"), QStringLiteral("¢")},
        {QStringLiteral("&pound;"), QStringLiteral("£")},
        {QStringLiteral("&curren;"), QStringLiteral("¤")},
        {QStringLiteral("&yen;"), QStringLiteral("¥")},
        {QStringLiteral("&brvbar;"), QStringLiteral("¦")},
        {QStringLiteral("&sect;"), QStringLiteral("§")},
        {QStringLiteral("&dot;"), QStringLiteral("¨")},
        {QStringLiteral("&copy;"), QStringLiteral("©")},
        {QStringLiteral("&ordf;"), QStringLiteral("ª")},
        {QStringLiteral("&laquo;"), QStringLiteral("«")},
        {QStringLiteral("&not;"), QStringLiteral("¬")},
        {QStringLiteral("&shy;"), QStringLiteral("­")},
        {QStringLiteral("&reg;"), QStringLiteral("®")},
        {QStringLiteral("&macr;"), QStringLiteral("¯")},
        {QStringLiteral("&deg;"), QStringLiteral("°")},
        {QStringLiteral("&plusmn;"), QStringLiteral("±")},
        {QStringLiteral("&sup2;"), QStringLiteral("²")},
        {QStringLiteral("&sup3;"), QStringLiteral("³")},
        {QStringLiteral("&acute;"), QStringLiteral("´")},
        {QStringLiteral("&micro;"), QStringLiteral("µ")},
        {QStringLiteral("&para;"), QStringLiteral("¶")},
        {QStringLiteral("&middot;"), QStringLiteral("·")},
        {QStringLiteral("&cedil;"), QStringLiteral("¸")},
        {QStringLiteral("&sup1;"), QStringLiteral("¹")},
        {QStringLiteral("&ordm;"), QStringLiteral("º")},
        {QStringLiteral("&raquo;"), QStringLiteral("»")},
        {QStringLiteral("&frac14;"), QStringLiteral("¼")},
        {QStringLiteral("&frac12;"), QStringLiteral("½")},
        {QStringLiteral("&frac34;"), QStringLiteral("¾")},
        {QStringLiteral("&iquest;"), QStringLiteral("¿")}
};
// clang-format on
