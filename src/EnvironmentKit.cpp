/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

#include "EnvironmentKit.h"

#include "RoomAppearance.h"
#include "TMap.h"
#include "utils.h"

#include <QColor>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

#include <array>

namespace EnvironmentKit {

namespace {
constexpr std::array<std::pair<Kit, const char*>, 5> scmKitNames{{
        {Kit::Plain, "plain"},
        {Kit::Water, "water"},
        {Kit::Road, "road"},
        {Kit::Forest, "forest"},
        {Kit::Building, "building"},
}};
} // namespace

QString name(const Kit kit)
{
    for (const auto& [each, kitName] : scmKitNames) {
        if (each == kit) {
            return QString::fromLatin1(kitName);
        }
    }
    return QString();
}

std::optional<Kit> fromName(const QString& name)
{
    for (const auto& [kit, kitName] : scmKitNames) {
        if (name == QLatin1String(kitName)) {
            return kit;
        }
    }
    return std::nullopt;
}

QStringList names()
{
    QStringList result;
    for (const auto& [kit, kitName] : scmKitNames) {
        result.append(QString::fromLatin1(kitName));
    }
    return result;
}

Kit guess(const QColor& environmentColor)
{
    if (!environmentColor.isValid()) {
        return Kit::Plain;
    }
    const QColor color = environmentColor.toHsl();
    const float lightness = color.lightnessF();
    if (lightness < 0.1f) {
        return Kit::Road;
    }
    if (lightness > 0.92f) {
        return Kit::Building;
    }
    const float saturation = color.hslSaturationF();
    // Saturation alone overstates it near black and white, where a little tint is still a grey
    const float chroma = static_cast<float>(qMax(qMax(color.redF(), color.greenF()), color.blueF()) - qMin(qMin(color.redF(), color.greenF()), color.blueF()));
    if (saturation < 0.15f || chroma < 0.08f) {
        return Kit::Road;
    }
    const float hue = color.hslHueF() * 360.0f;
    if (hue >= 170.0f && hue < 250.0f) {
        return Kit::Water;
    }
    if (hue >= 70.0f && hue < 170.0f) {
        return Kit::Forest;
    }
    // Brown and tan are a dull or dark orange; a bright one is a painted building
    if (hue >= 15.0f && hue < 50.0f && (saturation < 0.6f || lightness < 0.4f)) {
        return Kit::Road;
    }
    return Kit::Building;
}

QString userDataKey()
{
    return qsl("system.mapStyle.environmentKits");
}

namespace {
// Empty when nothing is saved, and nothing when what is saved is not a JSON object
std::optional<QJsonObject> storedObject(const QMap<QString, QString>& mapUserData)
{
    const auto found = mapUserData.constFind(userDataKey());
    if (found == mapUserData.cend()) {
        return QJsonObject();
    }
    const QJsonDocument document = QJsonDocument::fromJson(found->toUtf8());
    if (!document.isObject()) {
        return std::nullopt;
    }
    return document.object();
}

void warnOnce(const QString& saved)
{
    // Read for every environment on every repaint, so only a change in what is saved is worth repeating
    static QString lastWarned;
    if (saved == lastWarned) {
        return;
    }
    lastWarned = saved;
    qWarning().noquote() << "EnvironmentKit: ignoring what cannot be read in the map user data" << userDataKey() << ":" << saved;
}
} // namespace

QHash<int, Kit> stored(const QMap<QString, QString>& mapUserData)
{
    QHash<int, Kit> kits;
    const std::optional<QJsonObject> object = storedObject(mapUserData);
    if (!object) {
        warnOnce(mapUserData.value(userDataKey()));
        return kits;
    }
    bool skipped = false;
    for (const QString& key : object->keys()) {
        bool number = false;
        const int environmentId = key.toInt(&number);
        const std::optional<Kit> kit = fromName(object->value(key).toString());
        if (number && kit) {
            kits.insert(environmentId, *kit);
        } else {
            skipped = true;
        }
    }
    if (skipped) {
        warnOnce(mapUserData.value(userDataKey()));
    }
    return kits;
}

bool store(QMap<QString, QString>& mapUserData, const int environmentId, const std::optional<Kit> kit)
{
    std::optional<QJsonObject> object = storedObject(mapUserData);
    if (!object) {
        return false;
    }
    if (kit) {
        object->insert(QString::number(environmentId), name(*kit));
    } else {
        object->remove(QString::number(environmentId));
    }
    if (object->isEmpty()) {
        mapUserData.remove(userDataKey());
    } else {
        mapUserData.insert(userDataKey(), QString::fromUtf8(QJsonDocument(*object).toJson(QJsonDocument::Compact)));
    }
    return true;
}

std::pair<Kit, bool> forEnvironment(const TMap& map, const Host& host, const int environmentId)
{
    const QHash<int, Kit> named = stored(map.mUserData);
    if (const auto found = named.constFind(environmentId); found != named.cend()) {
        return {*found, true};
    }
    return {guess(RoomAppearance::environmentColor(map, host, environmentId)), false};
}

Kit forEnvironment(const QHash<int, Kit>& named, const int environmentId, const QColor& environmentColor)
{
    if (const auto found = named.constFind(environmentId); found != named.cend()) {
        return *found;
    }
    return guess(environmentColor);
}

} // namespace EnvironmentKit
