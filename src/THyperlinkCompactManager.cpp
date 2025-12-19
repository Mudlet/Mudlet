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
#include "TConsole.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>

THyperlinkCompactManager::THyperlinkCompactManager(TConsole* pConsole, QObject* parent)
: QObject(parent)
, mpConsole(pConsole)
{
    // Pure framework - no feature knowledge!
    // Features register themselves when they initialize
    if (!pConsole) {
        qWarning() << "THyperlinkCompactManager: pConsole parameter is null";
    }
}

THyperlinkCompactManager::~THyperlinkCompactManager() = default;

// ═══════════════════════════════════════════════════════════
// Shorthand Registration (Phase 1)
// ═══════════════════════════════════════════════════════════

void THyperlinkCompactManager::registerShorthand(const QString& shorthand, const QString& fullName, QObject* owner)
{
    if (shorthand.isEmpty() || fullName.isEmpty()) {
        qWarning() << "THyperlinkCompactManager::registerShorthand: empty shorthand or fullName";
        return;
    }

    mShorthandRegistry.insert(shorthand, qMakePair(fullName, QPointer<QObject>(owner)));
    emit shorthandRegistered(shorthand, fullName);

#if defined(DEBUG_OSC_PROCESSING)
    qDebug() << "[CompactSyntax] Registered shorthand:" << shorthand << "→" << fullName
             << "(owner:" << (owner ? owner->objectName() : "core") << ")";
#endif
}

void THyperlinkCompactManager::unregisterOwner(QObject* owner)
{
    if (!owner) {
        return;
    }

    // Remove all shortcuts owned by this object
    auto it = mShorthandRegistry.begin();
    while (it != mShorthandRegistry.end()) {
        if (it.value().second == owner) {
            it = mShorthandRegistry.erase(it);
        } else {
            ++it;
        }
    }

    // Remove all preset properties owned by this object
    auto it2 = mPresetPropertyRegistry.begin();
    while (it2 != mPresetPropertyRegistry.end()) {
        if (it2.value() == owner) {
            it2 = mPresetPropertyRegistry.erase(it2);
        } else {
            ++it2;
        }
    }

#if defined(DEBUG_OSC_PROCESSING)
    qDebug() << "[CompactSyntax] Unregistered all entries for owner:" << owner->objectName();
#endif
}

QMap<QString, QString> THyperlinkCompactManager::expandShorthand(const QMap<QString, QString>& params)
{
    QMap<QString, QString> expanded;

    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        const QString& key = it.key();

        // Check if this key is a registered shorthand
        if (mShorthandRegistry.contains(key)) {
            const auto& registration = mShorthandRegistry[key];
            const QPointer<QObject>& owner = registration.second;

            // Only expand if owner is still valid (or nullptr = core shortcut)
            if (owner.isNull() || !owner.isNull()) {
                expanded.insert(registration.first, it.value());
#if defined(DEBUG_OSC_PROCESSING)
                qDebug() << "[CompactSyntax] Expanded shorthand:" << key << "→" << registration.first;
#endif
                continue;
            }
        }

        // Not a shorthand or owner destroyed - keep original key
        expanded.insert(key, it.value());
    }

    return expanded;
}

// ═══════════════════════════════════════════════════════════
// Preset System (Phase 2)
// ═══════════════════════════════════════════════════════════

void THyperlinkCompactManager::registerPresetProperty(const QString& propertyName, QObject* owner)
{
    if (propertyName.isEmpty()) {
        qWarning() << "THyperlinkCompactManager::registerPresetProperty: empty propertyName";
        return;
    }

    mPresetPropertyRegistry.insert(propertyName, QPointer<QObject>(owner));
    emit presetPropertyRegistered(propertyName);

#if defined(DEBUG_OSC_PROCESSING)
    qDebug() << "[CompactSyntax] Registered preset property:" << propertyName
             << "(owner:" << (owner ? owner->objectName() : "core") << ")";
#endif
}

bool THyperlinkCompactManager::isPresetProperty(const QString& propertyName) const
{
    if (!mPresetPropertyRegistry.contains(propertyName)) {
        return false;
    }

    // Check if owner is still valid
    const QPointer<QObject>& owner = mPresetPropertyRegistry[propertyName];
    return owner.isNull() || !owner.isNull(); // null = core property (always valid)
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
    return deepMerge(base, overlay);
}

QJsonObject THyperlinkCompactManager::deepMerge(const QJsonObject& base, const QJsonObject& overlay) const
{
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
            result.insert(key, deepMerge(baseValue.toObject(), overlayValue.toObject()));
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
