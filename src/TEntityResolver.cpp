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
#include "utils.h"

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
        {qsl("&tab;"), qsl("\t")},
        {qsl("&newline;"), qsl("\n")},
        {qsl("&excl;"), qsl("!")},
        {qsl("&quot;"), qsl("\"")},
        {qsl("&num;"), qsl("#")},
        {qsl("&dollar;"), qsl("$")},
        {qsl("&percnt;"), qsl("%")},
        {qsl("&amp;"), qsl("&")},
        {qsl("&apos;"), qsl("'")},
        {qsl("&lpar;"), qsl("(")},
        {qsl("&rpar;"), qsl(")")},
        {qsl("&ast;"), qsl("*")},
        {qsl("&plus;"), qsl("+")},
        {qsl("&comma;"), qsl(",")},
        {qsl("&period;"), qsl(".")},
        {qsl("&sol;"), qsl("/")},
        {qsl("&colon;"), qsl(":")},
        {qsl("&semi;"), qsl(";")},
        {qsl("&lt;"), qsl("<")},
        {qsl("&equals;"), qsl("=")},
        {qsl("&gt;"), qsl(">")},
        {qsl("&quest;"), qsl("?")},
        {qsl("&commat;"), qsl("@")},
        {qsl("&lsqb;"), qsl("[")},
        {qsl("&bsol;"), qsl("\\")},
        {qsl("&rsqb;"), qsl("]")},
        {qsl("&hat;"), qsl("^")},
        {qsl("&lowbar;"), qsl("_")},
        {qsl("&grave;"), qsl("`")},
        {qsl("&lcub;"), qsl("{")},
        {qsl("&verbar;"), qsl("|")},
        {qsl("&rcub;"), qsl("}")},
        {qsl("&nbsp;"), qsl(" ")},
        {qsl("&iexcl;"), qsl("¡")},
        {qsl("&cent;"), qsl("¢")},
        {qsl("&pound;"), qsl("£")},
        {qsl("&curren;"), qsl("¤")},
        {qsl("&yen;"), qsl("¥")},
        {qsl("&brvbar;"), qsl("¦")},
        {qsl("&sect;"), qsl("§")},
        {qsl("&dot;"), qsl("¨")},
        {qsl("&copy;"), qsl("©")},
        {qsl("&ordf;"), qsl("ª")},
        {qsl("&laquo;"), qsl("«")},
        {qsl("&not;"), qsl("¬")},
        {qsl("&shy;"), qsl("­")},
        {qsl("&reg;"), qsl("®")},
        {qsl("&macr;"), qsl("¯")},
        {qsl("&deg;"), qsl("°")},
        {qsl("&plusmn;"), qsl("±")},
        {qsl("&sup2;"), qsl("²")},
        {qsl("&sup3;"), qsl("³")},
        {qsl("&acute;"), qsl("´")},
        {qsl("&micro;"), qsl("µ")},
        {qsl("&para;"), qsl("¶")},
        {qsl("&middot;"), qsl("·")},
        {qsl("&cedil;"), qsl("¸")},
        {qsl("&sup1;"), qsl("¹")},
        {qsl("&ordm;"), qsl("º")},
        {qsl("&raquo;"), qsl("»")},
        {qsl("&frac14;"), qsl("¼")},
        {qsl("&frac12;"), qsl("½")},
        {qsl("&frac34;"), qsl("¾")},
        {qsl("&iquest;"), qsl("¿")}
};
// clang-format on
