#ifndef MUDLET_TAREAFIELDS_H
#define MUDLET_TAREAFIELDS_H

/***************************************************************************
 *   Copyright (C) 2026 by Jens Wegener - code@jenswegener.net             *
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

#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <optional>
#include <utility>

class TMap;

// The properties a map gives its areas: which there are is up to the map, kept in its user data under
// scmMapUserDataKey, and each area keeps its own values in its user data under the field's key, as plain text, so a
// script that already sets them with setAreaUserData() fills the fields in.
namespace TAreaFields {

inline const QString scmMapUserDataKey = QStringLiteral("system.areaFields");

enum class Type { Text, Number, Range, YesNo, Choice };

struct Field
{
    QString key;
    // As shown to the player; the key where none is given
    QString name;
    Type type = Type::Text;
    // An area without a value of its own takes its nearest parent area's
    bool inherit = false;
    // The values a choice may take, in the order given
    QStringList choices;

    bool operator==(const Field&) const = default;
};

QString typeName(Type);
std::optional<Type> typeFromName(const QString&);
// Why a field could not be defined so, or an empty string when it can
QString definitionError(const Field&);

// The map's fields by key; one stored that could not be read is left out
QMap<QString, Field> fields(const TMap&);
// Replaces the map's fields, all of them; an empty map removes the user data key
void setFields(TMap&, const QMap<QString, Field>&);

// A value as the field's type holds it: a QString for text and choice, a double for a number, a bool for yes/no and a
// QVariantList of two doubles, the lower first, for a range. None for text that does not fit the type.
std::optional<QVariant> parseValue(const Field&, const QString& text);
// As kept in user data, which parseValue() reads back
QString formatValue(const Field&, const QVariant& value);

struct Lookup
{
    // None where no area along the way has a value, or the one that has can not be read as the field's type
    std::optional<QVariant> value;
    // The area the value, or the unreadable text, was found in
    int areaId = 0;
    // Why the text found could not be read, or empty
    QString error;
};
// The area's value for the field or, if the field is inherited and the area has none, its nearest parent area's
Lookup valueFor(const TMap&, int areaId, const Field&);

} // namespace TAreaFields

#endif // MUDLET_TAREAFIELDS_H
