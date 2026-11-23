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

#include "TAreaFields.h"

#include "TArea.h"
#include "TMap.h"
#include "TRoomDB.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>

#include <cmath>

namespace TAreaFields {

namespace {

const QList<std::pair<Type, QString>>& typeNames()
{
    static const QList<std::pair<Type, QString>> names{
            {Type::Text, QStringLiteral("text")},
            {Type::Number, QStringLiteral("number")},
            {Type::Range, QStringLiteral("range")},
            {Type::YesNo, QStringLiteral("yesno")},
            {Type::Choice, QStringLiteral("choice")},
    };
    return names;
}

// As QString::number(), without the exponent it would give a whole number past six digits
QString numberText(const double value)
{
    return QString::number(value, 'g', 15);
}

std::optional<double> finiteNumber(const QString& text)
{
    bool ok = false;
    const double value = text.trimmed().toDouble(&ok);
    if (!ok || !std::isfinite(value)) {
        return std::nullopt;
    }
    return value;
}

} // namespace

QString typeName(const Type type)
{
    for (const auto& [known, name] : typeNames()) {
        if (known == type) {
            return name;
        }
    }
    Q_UNREACHABLE();
}

std::optional<Type> typeFromName(const QString& name)
{
    for (const auto& [type, known] : typeNames()) {
        if (known == name) {
            return type;
        }
    }
    return std::nullopt;
}

QString definitionError(const Field& field)
{
    if (field.key.trimmed().isEmpty()) {
        return QStringLiteral("a field needs a key");
    }
    // Those are Mudlet's own, which it keeps beside an area's values
    if (field.key.startsWith(QLatin1String("system."))) {
        return QStringLiteral("field \"%1\": keys starting with \"system.\" are Mudlet's own").arg(field.key);
    }
    if (field.type == Type::Choice) {
        if (field.choices.isEmpty()) {
            return QStringLiteral("field \"%1\": a choice needs at least one value to choose from").arg(field.key);
        }
        for (const QString& choice : field.choices) {
            if (choice.isEmpty()) {
                return QStringLiteral("field \"%1\": a choice cannot be empty").arg(field.key);
            }
        }
        if (QSet<QString>(field.choices.cbegin(), field.choices.cend()).size() != field.choices.size()) {
            return QStringLiteral("field \"%1\": a choice is listed twice").arg(field.key);
        }
    } else if (!field.choices.isEmpty()) {
        return QStringLiteral("field \"%1\": only a choice has values to choose from").arg(field.key);
    }
    return QString();
}

QMap<QString, Field> fields(const TMap& map)
{
    QMap<QString, Field> result;
    const QJsonObject stored = QJsonDocument::fromJson(map.mUserData.value(scmMapUserDataKey).toUtf8()).object();
    for (auto it = stored.constBegin(); it != stored.constEnd(); ++it) {
        const QJsonObject definition = it.value().toObject();
        const std::optional<Type> type = typeFromName(definition.value(QLatin1String("type")).toString());
        if (!type) {
            continue;
        }
        Field field;
        field.key = it.key();
        field.name = definition.value(QLatin1String("name")).toString(field.key);
        field.type = *type;
        field.inherit = definition.value(QLatin1String("inherit")).toBool();
        for (const QJsonValue& choice : definition.value(QLatin1String("choices")).toArray()) {
            field.choices << choice.toString();
        }
        if (definitionError(field).isEmpty()) {
            result.insert(field.key, field);
        }
    }
    return result;
}

void setFields(TMap& map, const QMap<QString, Field>& fields)
{
    if (fields.isEmpty()) {
        map.mUserData.remove(scmMapUserDataKey);
        return;
    }
    QJsonObject stored;
    for (const Field& field : fields) {
        QJsonObject definition;
        definition.insert(QLatin1String("type"), typeName(field.type));
        if (field.name != field.key) {
            definition.insert(QLatin1String("name"), field.name);
        }
        if (field.inherit) {
            definition.insert(QLatin1String("inherit"), true);
        }
        if (field.type == Type::Choice) {
            definition.insert(QLatin1String("choices"), QJsonArray::fromStringList(field.choices));
        }
        stored.insert(field.key, definition);
    }
    map.mUserData.insert(scmMapUserDataKey, QString::fromUtf8(QJsonDocument(stored).toJson(QJsonDocument::Compact)));
}

std::optional<QVariant> parseValue(const Field& field, const QString& text)
{
    switch (field.type) {
    case Type::Text:
        return QVariant(text);
    case Type::Number:
        if (const std::optional<double> number = finiteNumber(text)) {
            return QVariant(*number);
        }
        return std::nullopt;
    case Type::Range: {
        // "10-20", and "-5-5" for one starting below zero: the dash between the two is the one after a digit
        static const QRegularExpression rangeText(QStringLiteral(R"(^\s*(-?[\d.]+)\s*-\s*(-?[\d.]+)\s*$)"));
        const QRegularExpressionMatch match = rangeText.match(text);
        if (!match.hasMatch()) {
            return std::nullopt;
        }
        const std::optional<double> lower = finiteNumber(match.captured(1));
        const std::optional<double> upper = finiteNumber(match.captured(2));
        if (!lower || !upper || *lower > *upper) {
            return std::nullopt;
        }
        return QVariant(QVariantList{*lower, *upper});
    }
    case Type::YesNo: {
        const QString word = text.trimmed().toLower();
        if (word == QLatin1String("true") || word == QLatin1String("yes") || word == QLatin1String("1")) {
            return QVariant(true);
        }
        if (word == QLatin1String("false") || word == QLatin1String("no") || word == QLatin1String("0")) {
            return QVariant(false);
        }
        return std::nullopt;
    }
    case Type::Choice:
        if (field.choices.contains(text)) {
            return QVariant(text);
        }
        return std::nullopt;
    }
    Q_UNREACHABLE();
}

QString formatValue(const Field& field, const QVariant& value)
{
    switch (field.type) {
    case Type::Text:
    case Type::Choice:
        return value.toString();
    case Type::Number:
        return numberText(value.toDouble());
    case Type::Range: {
        const QVariantList bounds = value.toList();
        return QStringLiteral("%1-%2").arg(numberText(bounds.value(0).toDouble()), numberText(bounds.value(1).toDouble()));
    }
    case Type::YesNo:
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    }
    Q_UNREACHABLE();
}

Lookup valueFor(const TMap& map, const int areaId, const Field& field)
{
    // setAreaParent() refuses a loop, but a map file could still hold one
    QSet<int> visited;
    std::optional<int> id = areaId;
    while (id && !visited.contains(*id)) {
        visited.insert(*id);
        // An area that so far only has a name has no user data, so no value of its own, but may be nested
        if (const TArea* pArea = map.mpRoomDB->getArea(*id); pArea && pArea->mUserData.contains(field.key)) {
            const QString text = pArea->mUserData.value(field.key);
            if (const std::optional<QVariant> value = parseValue(field, text)) {
                return {value, *id, QString()};
            }
            return {std::nullopt, *id, QStringLiteral("area %1 holds \"%2\" for field \"%3\", which is not a valid %4").arg(QString::number(*id), text, field.key, typeName(field.type))};
        }
        if (!field.inherit) {
            break;
        }
        id = map.mpRoomDB->getAreaParent(*id);
    }
    return {};
}

} // namespace TAreaFields
