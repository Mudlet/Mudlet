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

QString TEntityResolver::getResolution(const QString& entity, bool resolveCustomEntities, TEntityType *entityType) const
{
    if (entity.front() != '&' || entity.back() != ';') {
        if (entityType) {
            *entityType = ENTITY_TYPE_UNKNOWN;
        }
        return entity;
    }

    if (resolveCustomEntities) {
        auto ptr = mEntititesMap.find(entity.toLower());
        if (ptr != mEntititesMap.end()) {
            if (entityType) {
                *entityType = ENTITY_TYPE_CUSTOM;
            }
            return *ptr;
        }
    }

    // Although Mudlet ignores case, MXP defines entity names as case-sensitive.
    // Also, the predefined entities &Auml; and &auml; are for example different.
    // So we first check for an exact match:
    auto stdPtr = scmStandardEntites.find(entity);
    if (stdPtr != scmStandardEntites.end()) {
        if (entityType) {
            *entityType = ENTITY_TYPE_SYSTEM;
        }
        return *stdPtr;
    }

    // then see if there is at least a case-insensitive match for backwards compatibility:
    stdPtr = scmStandardEntites.find(entity.toLower());
    if (stdPtr != scmStandardEntites.end()) {
        if (entityType) {
            *entityType = ENTITY_TYPE_SYSTEM;
        }
        return *stdPtr;
    }

    if (entity[1] == '#') {
        if (entityType) {
            *entityType = ENTITY_TYPE_SYSTEM;
        }
        return resolveCode(entity.mid(2, entity.size() - 3));
    }
    if (entityType) {
        *entityType = ENTITY_TYPE_UNKNOWN;
    }
    return entity;
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
    return mEntititesMap.remove(entity.toLower()) > 0;
}

QString TEntityResolver::resolveCode(const QString& entityValue)
{
    return entityValue.front() == 'x' ? resolveCode(entityValue.mid(1), 16) : resolveCode(entityValue, 10);
}

QString TEntityResolver::resolveCode(const QString& entityValue, int base)
{
    bool isNum = false;
    ushort const code = entityValue.toUShort(&isNum, base);
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
        {qsl("&shy;"), QChar(0x00AD)},
        {qsl("&reg;"), qsl("®")},
        {qsl("&macr;"), qsl("¯")},
        {qsl("&deg;"), qsl("°")},
        {qsl("&plusmn;"), qsl("±")},
        {qsl("&divide;"), qsl("÷")},
        {qsl("&times;"), qsl("×")},
        {qsl("&sup2;"), qsl("²")},
        {qsl("&sup3;"), qsl("³")},
        {qsl("&acute;"), qsl("´")},
        {qsl("&uml;"), qsl("¨")},
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
        {qsl("&iquest;"), qsl("¿")},
        {qsl("&Aacute;"), qsl("Á")},
        {qsl("&aacute;"), qsl("á")},
        {qsl("&Acirc;"), qsl("Â")},
        {qsl("&acirc;"), qsl("â")},
        {qsl("&AElig;"), qsl("Æ")},
        {qsl("&aelig;"), qsl("æ")},
        {qsl("&Agrave;"), qsl("À")},
        {qsl("&agrave;"), qsl("à")},
        {qsl("&Aring;"), qsl("Å")},
        {qsl("&aring;"), qsl("å")},
        {qsl("&Atilde;"), qsl("Ã")},
        {qsl("&atilde;"), qsl("ã")},
        {qsl("&Auml;"), qsl("Ä")},
        {qsl("&auml;"), qsl("ä")},
        {qsl("&Ccedil;"), qsl("Ç")},
        {qsl("&ccedil;"), qsl("ç")},
        {qsl("&Eacute;"), qsl("É")},
        {qsl("&eacute;"), qsl("é")},
        {qsl("&Ecirc;"), qsl("Ê")},
        {qsl("&ecirc;"), qsl("ê")},
        {qsl("&Egrave;"), qsl("È")},
        {qsl("&egrave;"), qsl("è")},
        {qsl("&Euml;"), qsl("Ë")},
        {qsl("&euml;"), qsl("ë")},
        {qsl("&Iacute;"), qsl("Í")},
        {qsl("&iacute;"), qsl("í")},
        {qsl("&Icirc;"), qsl("Î")},
        {qsl("&icirc;"), qsl("î")},
        {qsl("&Igrave;"), qsl("Ì")},
        {qsl("&igrave;"), qsl("ì")},
        {qsl("&Iuml;"), qsl("Ï")},
        {qsl("&iuml;"), qsl("ï")},
        {qsl("&ETH;"), qsl("Ð")},
        {qsl("&eth;"), qsl("ð")},
        {qsl("&Ntilde;"), qsl("Ñ")},
        {qsl("&ntilde;"), qsl("ñ")},
        {qsl("&Oacute;"), qsl("Ó")},
        {qsl("&oacute;"), qsl("ó")},
        {qsl("&Ocirc;"), qsl("Ô")},
        {qsl("&ocirc;"), qsl("ô")},
        {qsl("&Ograve;"), qsl("Ò")},
        {qsl("&ograve;"), qsl("ò")},
        {qsl("&Oslash;"), qsl("Ø")},
        {qsl("&oslash;"), qsl("ø")},
        {qsl("&Otilde;"), qsl("Õ")},
        {qsl("&otilde;"), qsl("õ")},
        {qsl("&Ouml;"), qsl("Ö")},
        {qsl("&ouml;"), qsl("ö")},
        {qsl("&Uacute;"), qsl("Ú")},
        {qsl("&uacute;"), qsl("ú")},
        {qsl("&Ucirc;"), qsl("Û")},
        {qsl("&ucirc;"), qsl("û")},
        {qsl("&Ugrave;"), qsl("Ù")},
        {qsl("&ugrave;"), qsl("ù")},
        {qsl("&Uuml;"), qsl("Ü")},
        {qsl("&uuml;"), qsl("ü")},
        {qsl("&Yacute;"), qsl("Ý")},
        {qsl("&yacute;"), qsl("ý")},
        {qsl("&THORN;"), qsl("Þ")},
        {qsl("&thorn;"), qsl("þ")},
        {qsl("&szlig;"), qsl("ß")}
};
// clang-format on
