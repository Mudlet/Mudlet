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

void THyperlinkCompactManager::registerShorthand(const QString& shorthand, const QString& fullName, QObject* owner)
{
    if (shorthand.isEmpty() || fullName.isEmpty()) {
        qWarning() << "THyperlinkCompactManager::registerShorthand: empty shorthand or fullName";
        return;
    }

    // Check for existing entry and warn about collision
    if (mShorthandRegistry.contains(shorthand)) {
        const ShorthandEntry& existing = mShorthandRegistry.value(shorthand);
        qWarning() << "THyperlinkCompactManager::registerShorthand: Replacing existing shorthand"
                   << shorthand << "→" << existing.fullName
                   << "(owner:" << (existing.owner ? existing.owner->objectName() : "core") << ")"
                   << "with new mapping:" << shorthand << "→" << fullName
                   << "(owner:" << (owner ? owner->objectName() : "core") << ")";
    }

    bool isCore = (owner == nullptr);
    mShorthandRegistry.insert(shorthand, ShorthandEntry(fullName, owner, isCore));
    emit shorthandRegistered(shorthand, fullName);

    // Automatic cleanup: when a non-null owner is destroyed, unregister all its entries
    // Only connect once per owner to avoid duplicate unregisterOwner() calls
    if (owner && !mConnectedOwners.contains(owner)) {
        // Save owner name before connecting to avoid UAF when destroyed
        QString ownerName = owner->objectName();
        connect(owner, &QObject::destroyed, this, [this, owner, ownerName]() {
            mConnectedOwners.remove(owner);
            unregisterOwner(owner, ownerName);
        });
        mConnectedOwners.insert(owner);
    }

#if defined(DEBUG_OSC_PROCESSING)
    qDebug() << "[CompactSyntax] Registered shorthand:" << shorthand << "→" << fullName
             << "(owner:" << (owner ? owner->objectName() : "core") << ")";
#endif
}

void THyperlinkCompactManager::unregisterOwner(QObject* owner, const QString& ownerName)
{
    if (!owner) {
        return;
    }

    auto it = mShorthandRegistry.begin();
    while (it != mShorthandRegistry.end()) {
        if (it.value().owner == owner) {
            it = mShorthandRegistry.erase(it);
        } else {
            ++it;
        }
    }

    // Remove all preset properties owned by this object
    auto it2 = mPresetPropertyRegistry.begin();
    while (it2 != mPresetPropertyRegistry.end()) {
        if (it2.value().owner == owner) {
            it2 = mPresetPropertyRegistry.erase(it2);
        } else {
            ++it2;
        }
    }

#if defined(DEBUG_OSC_PROCESSING)
    // Use ownerName parameter if provided, otherwise try to safely access objectName
    QString debugName = !ownerName.isEmpty() ? ownerName : (owner ? owner->objectName() : QStringLiteral("unknown"));
    qDebug() << "[CompactSyntax] Unregistered all entries for owner:" << debugName;
#endif
}

QMap<QString, QString> THyperlinkCompactManager::expandShorthand(const QMap<QString, QString>& params) const
{
    QMap<QString, QString> expanded;

    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        const QString& key = it.key();

        if (mShorthandRegistry.contains(key)) {
            const auto& registration = mShorthandRegistry.value(key);

            if (registration.isCore || !registration.owner.isNull()) {
                expanded.insert(registration.fullName, it.value());
#if defined(DEBUG_OSC_PROCESSING)
                qDebug() << "[CompactSyntax] Expanded shorthand:" << key << "→" << registration.fullName;
#endif
                continue;
            }
        }
        expanded.insert(key, it.value());
    }

    return expanded;
}

void THyperlinkCompactManager::registerPresetProperty(const QString& propertyName, QObject* owner, bool isCore)
{
    if (propertyName.isEmpty()) {
        qWarning() << "THyperlinkCompactManager::registerPresetProperty: empty propertyName";
        return;
    }

    // Non-core properties require a valid owner
    if (!isCore && !owner) {
        qWarning() << "THyperlinkCompactManager::registerPresetProperty: non-core property"
                   << propertyName << "requires a valid owner (owner is null)";
        return;
    }

    // Check for existing entry and warn about collision
    if (mPresetPropertyRegistry.contains(propertyName)) {
        const PresetPropertyEntry& existing = mPresetPropertyRegistry.value(propertyName);
        qWarning() << "THyperlinkCompactManager::registerPresetProperty: Replacing existing preset property"
                   << propertyName
                   << "(owner:" << (existing.owner ? existing.owner->objectName() : "core") << ")"
                   << "with new registration"
                   << "(owner:" << (owner ? owner->objectName() : "core") << ")";
    }

    mPresetPropertyRegistry.insert(propertyName, PresetPropertyEntry(owner, isCore));
    emit presetPropertyRegistered(propertyName);

    // Automatic cleanup: when a non-null owner is destroyed, unregister all its entries
    // Only connect once per owner to avoid duplicate unregisterOwner() calls
    if (owner && !mConnectedOwners.contains(owner)) {
        // Save owner name before connecting to avoid UAF when destroyed
        QString ownerName = owner->objectName();
        connect(owner, &QObject::destroyed, this, [this, owner, ownerName]() {
            mConnectedOwners.remove(owner);
            unregisterOwner(owner, ownerName);
        });
        mConnectedOwners.insert(owner);
    }

#if defined(DEBUG_OSC_PROCESSING)
    qDebug() << "[CompactSyntax] Registered preset property:" << propertyName
             << "(owner:" << (owner ? owner->objectName() : "none")
             << ", isCore:" << isCore << ")";
#endif
}

bool THyperlinkCompactManager::isPresetProperty(const QString& propertyName) const
{
    if (!mPresetPropertyRegistry.contains(propertyName)) {
        return false;
    }

    const PresetPropertyEntry& entry = mPresetPropertyRegistry.value(propertyName);
    // Valid if core property OR owner still exists
    return entry.isCore || !entry.owner.isNull();
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
