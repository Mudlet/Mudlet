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

#ifndef MUDLET_THYPERLINKCOMPACTMANAGER_H
#define MUDLET_THYPERLINKCOMPACTMANAGER_H

#include <QHash>
#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>

class TConsole;

// Registry entry for preset properties
struct PresetPropertyEntry {
    PresetPropertyEntry() {}
};

// Registry entry for shorthand mappings
struct ShorthandEntry {
    QString fullName;
    
    ShorthandEntry() {}
    ShorthandEntry(const QString& full) : fullName(full) {}
};

// Pure plugin framework for OSC 8 hyperlink compact syntax
// Features (style, menu, tooltip, visibility, selection, etc.) register themselves
// Provides shorthand expansion and preset management with deep merge support
class THyperlinkCompactManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(THyperlinkCompactManager)

public:
    // Constructor: No QObject parent - ownership managed by unique_ptr in TConsole
    THyperlinkCompactManager();
    ~THyperlinkCompactManager();

    // Register a shorthand expansion (e.g., "s" → "style")
    void registerShorthand(const QString& shorthand, const QString& fullName);
    
    // Unregister a shorthand expansion
    void unregisterShorthand(const QString& shorthand);
    
    // Clear all shorthands
    void clearShorthands();

    // Expand shorthand properties to full names
    QMap<QString, QString> expandShorthand(const QMap<QString, QString>& params) const;

    // Query registered shortcuts (for debugging/introspection)
    int getShorthandCount() const { return mShorthandRegistry.size(); }
    QStringList getRegisteredShorthands() const { return mShorthandRegistry.keys(); }

    // Register a property as preset-aware (allows it to be included in presets)
    void registerPresetProperty(const QString& propertyName);
    
    // Unregister a preset property
    void unregisterPresetProperty(const QString& propertyName);
    
    // Clear all preset properties
    void clearPresetProperties();

    // Check if a property can be used in presets
    bool isPresetProperty(const QString& propertyName) const;

    // Query registered preset properties (for debugging/introspection)
    int getPresetPropertyCount() const { return mPresetPropertyRegistry.size(); }
    QStringList getRegisteredPresetProperties() const { return mPresetPropertyRegistry.keys(); }

    // Preset storage and retrieval (session-scoped, cleared on disconnect)
    void registerPreset(const QString& name, const QJsonObject& config);
    QJsonObject getPreset(const QString& name) const;
    bool hasPreset(const QString& name) const;
    void clearPresets();

    // Combine two configurations with override taking precedence
    QJsonObject mergeConfigs(const QJsonObject& base, const QJsonObject& overlay) const;

signals:
    void shorthandRegistered(const QString& shorthand, const QString& fullName);
    void shorthandUnregistered(const QString& shorthand);
    void shorthandsCleared();
    
    void presetPropertyRegistered(const QString& propertyName);
    void presetPropertyUnregistered(const QString& propertyName);
    void presetPropertiesCleared();
    
    void presetRegistered(const QString& name);
    void presetsCleared();

private:
    QHash<QString, ShorthandEntry> mShorthandRegistry;

    QHash<QString, PresetPropertyEntry> mPresetPropertyRegistry;

    QHash<QString, QJsonObject> mPresets;

    static constexpr int MAX_MERGE_DEPTH = 32;
    QJsonObject deepMerge(const QJsonObject& base, const QJsonObject& overlay, int depth = 0) const;
};

#endif // MUDLET_THYPERLINKCOMPACTMANAGER_H
