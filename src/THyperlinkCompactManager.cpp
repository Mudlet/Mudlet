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

#include "THyperlinkCompactManager.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>

THyperlinkCompactManager::THyperlinkCompactManager()
: QObject(nullptr)
{
    // Pure framework - no feature knowledge!
    // Features register themselves when they initialize
    // Note: No QObject parent - ownership managed by unique_ptr in TConsole
}

THyperlinkCompactManager::~THyperlinkCompactManager() = default;

void THyperlinkCompactManager::registerShorthand(const QString& shorthand, const QString& fullName)
{
    if (shorthand.isEmpty() || fullName.isEmpty()) {
        qWarning() << "THyperlinkCompactManager::registerShorthand: empty shorthand or fullName";
        return;
    }

    if (mShorthandRegistry.contains(shorthand)) {
        const ShorthandEntry& existing = mShorthandRegistry.value(shorthand);
        qWarning() << "THyperlinkCompactManager::registerShorthand: Replacing existing shorthand"
                   << shorthand << "→" << existing.fullName
                   << "with new mapping:" << shorthand << "→" << fullName;
    }

    mShorthandRegistry.insert(shorthand, ShorthandEntry(fullName));
    emit shorthandRegistered(shorthand, fullName);

#if defined(DEBUG_OSC_PROCESSING)
    qDebug() << "[CompactSyntax] Registered shorthand:" << shorthand << "→" << fullName;
#endif
}

void THyperlinkCompactManager::unregisterShorthand(const QString& shorthand)
{
    if (shorthand.isEmpty()) {
        qWarning() << "THyperlinkCompactManager::unregisterShorthand: empty shorthand";
        return;
    }

    if (!mShorthandRegistry.contains(shorthand)) {
        qWarning() << "THyperlinkCompactManager::unregisterShorthand: shorthand not found:" << shorthand;
        return;
    }

    mShorthandRegistry.remove(shorthand);
    emit shorthandUnregistered(shorthand);

#if defined(DEBUG_OSC_PROCESSING)
    qDebug() << "[CompactSyntax] Unregistered shorthand:" << shorthand;
#endif
}

void THyperlinkCompactManager::clearShorthands()
{
    mShorthandRegistry.clear();
    emit shorthandsCleared();

#if defined(DEBUG_OSC_PROCESSING)
    qDebug() << "[CompactSyntax] Cleared all shorthands";
#endif
}

QMap<QString, QString> THyperlinkCompactManager::expandShorthand(const QMap<QString, QString>& params) const
{
    QMap<QString, QString> expanded;

    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        const QString& key = it.key();

        if (mShorthandRegistry.contains(key)) {
            const QString& fullName = mShorthandRegistry.value(key).fullName;
            expanded.insert(fullName, it.value());
#if defined(DEBUG_OSC_PROCESSING)
            qDebug() << "[CompactSyntax] Expanded shorthand:" << key << "→" << fullName;
#endif
        } else {
            expanded.insert(key, it.value());
        }
    }

    return expanded;
}

void THyperlinkCompactManager::registerPresetProperty(const QString& propertyName)
{
    if (propertyName.isEmpty()) {
        qWarning() << "THyperlinkCompactManager::registerPresetProperty: empty propertyName";
        return;
    }

    if (mPresetPropertyRegistry.contains(propertyName)) {
        qWarning() << "THyperlinkCompactManager::registerPresetProperty: Replacing existing preset property"
                   << propertyName;
    }

    mPresetPropertyRegistry.insert(propertyName, PresetPropertyEntry());
    emit presetPropertyRegistered(propertyName);

#if defined(DEBUG_OSC_PROCESSING)
    qDebug() << "[CompactSyntax] Registered preset property:" << propertyName;
#endif
}

void THyperlinkCompactManager::unregisterPresetProperty(const QString& propertyName)
{
    if (propertyName.isEmpty()) {
        qWarning() << "THyperlinkCompactManager::unregisterPresetProperty: empty propertyName";
        return;
    }

    if (!mPresetPropertyRegistry.contains(propertyName)) {
        qWarning() << "THyperlinkCompactManager::unregisterPresetProperty: property not found:" << propertyName;
        return;
    }

    mPresetPropertyRegistry.remove(propertyName);
    emit presetPropertyUnregistered(propertyName);

#if defined(DEBUG_OSC_PROCESSING)
    qDebug() << "[CompactSyntax] Unregistered preset property:" << propertyName;
#endif
}

void THyperlinkCompactManager::clearPresetProperties()
{
    mPresetPropertyRegistry.clear();
    emit presetPropertiesCleared();

#if defined(DEBUG_OSC_PROCESSING)
    qDebug() << "[CompactSyntax] Cleared all preset properties";
#endif
}

bool THyperlinkCompactManager::isPresetProperty(const QString& propertyName) const
{
    return mPresetPropertyRegistry.contains(propertyName);
}

void THyperlinkCompactManager::registerPreset(const QString& name, const QJsonObject& config)
{
    if (name.isEmpty()) {
        qWarning() << "THyperlinkCompactManager::registerPreset: empty name";
        return;
    }

    mPresets.insert(name, config);
    emit presetRegistered(name);

#if defined(DEBUG_OSC_PROCESSING)
    qDebug() << "[CompactSyntax] Registered preset:" << name
             << "config:" << QJsonDocument(config).toJson(QJsonDocument::Compact);
#endif
}

QJsonObject THyperlinkCompactManager::getPreset(const QString& name) const
{
    return mPresets.value(name, QJsonObject());
}

bool THyperlinkCompactManager::hasPreset(const QString& name) const
{
    return mPresets.contains(name);
}

void THyperlinkCompactManager::clearPresets()
{
    mPresets.clear();
    emit presetsCleared();

#if defined(DEBUG_OSC_PROCESSING)
    qDebug() << "[CompactSyntax] Cleared all presets";
#endif
}

QJsonObject THyperlinkCompactManager::mergeConfigs(const QJsonObject& base, const QJsonObject& overlay) const
{
    return deepMerge(base, overlay, 0);
}

QJsonObject THyperlinkCompactManager::deepMerge(const QJsonObject& base, const QJsonObject& overlay, int depth) const
{
    // Check recursion depth limit to prevent stack overflow
    if (depth >= MAX_MERGE_DEPTH) {
        qWarning() << "THyperlinkCompactManager::deepMerge: Maximum recursion depth"
                   << MAX_MERGE_DEPTH << "reached. Stopping recursion and using overlay.";
        return overlay;
    }

    QJsonObject result = base;

    // Iterate through all keys in overlay
    for (auto it = overlay.constBegin(); it != overlay.constEnd(); ++it) {
        const QString& key = it.key();
        const QJsonValue& overlayValue = it.value();

        if (!base.contains(key)) {
            // Key doesn't exist in base - add it
            result.insert(key, overlayValue);
            continue;
        }

        const QJsonValue& baseValue = base[key];

        // Both values exist - determine merge strategy
        if (overlayValue.isObject() && baseValue.isObject()) {
            // Both are objects - recursive merge
            result.insert(key, deepMerge(baseValue.toObject(), overlayValue.toObject(), depth + 1));
        } else if (overlayValue.isArray() && baseValue.isArray()) {
            // Both are arrays - overlay completely replaces base (CSS behavior)
            result.insert(key, overlayValue);
        } else {
            // Primitives or type mismatch - overlay wins
            result.insert(key, overlayValue);
        }
    }

    return result;
}
