#ifndef MUDLET_TMXPPROCESSOR_H
#define MUDLET_TMXPPROCESSOR_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2018, 2022 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
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

#include "TEntityHandler.h"
#include "TMxpNodeBuilder.h"
#include "TMxpTagProcessor.h"

#include <QSet>

class Host;

enum TMXPMode { MXP_MODE_OPEN, MXP_MODE_SECURE, MXP_MODE_LOCKED, MXP_MODE_TEMP_SECURE };

// MXP mode codes sent by the server via ESC[#z sequences.
// Codes 0-4 only affect the current line; codes 5-7 also set the default mode.
constexpr int MXP_MODE_CODE_OPEN = 0;
constexpr int MXP_MODE_CODE_SECURE = 1;
constexpr int MXP_MODE_CODE_LOCKED = 2;
constexpr int MXP_MODE_CODE_RESET = 3;
constexpr int MXP_MODE_CODE_TEMP_SECURE = 4;
constexpr int MXP_MODE_CODE_LOCK_OPEN = 5;
constexpr int MXP_MODE_CODE_LOCK_SECURE = 6;
constexpr int MXP_MODE_CODE_LOCK_LOCKED = 7;

enum TMxpProcessingResult {
    HANDLER_FALL_THROUGH,
    HANDLER_NEXT_CHAR,
    HANDLER_COMMIT_LINE,
    HANDLER_INSERT_ENTITY_CUST,
    HANDLER_INSERT_ENTITY_SYS,
    HANDLER_INSERT_ENTITY_LIT,
    HANDLER_INSERT_AND_REPROCESS
};

// handles the MXP protocol
class TMxpProcessor
{
public:
    explicit TMxpProcessor(TMxpClient* pMxpClient)
    : mMxpTagBuilder(true)
    , mEntityHandler(mMxpTagProcessor.getEntityResolver())
    , mpMxpClient(pMxpClient)
    {
    }

    bool setMode(const QString& code);
    bool setMode(int modeCode);
    TMXPMode mode() const;
    TMXPMode defaultMode() const;

    void enable();
    void disable();
    bool isEnabled() const;
    void resetToDefaultMode();

    TMxpProcessingResult processMxpInput(char& ch, bool resolveCustomEntities);
    void processRawInput(char ch);
    QString getEntityValue() { return lastEntityValue; }
    void setLastEntityValue(const QString& value) { lastEntityValue = value; }
    TMxpTagProcessor& getMxpTagProcessor() { return mMxpTagProcessor; }
    TMxpNodeBuilder& getMxpTagBuilder() { return mMxpTagBuilder; }

    // Tag recognition: checks if the tag name is a known MXP tag from the spec
    // or a user-defined element registered in the element registry
    bool isRecognizedMxpTag(const QString& tagName) const;

    // Mode-aware check: is this tag allowed given the current MXP mode?
    // In OPEN mode, only OPEN tags + OPEN user-defined elements are allowed
    // In SECURE/TEMP_SECURE mode, all recognized tags are allowed
    bool isTagAllowedInCurrentMode(const QString& tagName) const;

    // Abort the current tag being built (e.g., when ANSI escape interrupts it)
    // Returns the literal text that should be output ("<" + accumulated content)
    QString abortCurrentTag();

    // All MXP tags defined in the specification (case-insensitive, stored uppercase)
    static const QSet<QString>& allMxpTags();
    // MXP tags allowed in OPEN mode per the specification
    static const QSet<QString>& openModeTags();

private:
    TMxpProcessingResult rejectCurrentTag();
    QString decodeRawBytes(const std::string& raw, const QByteArray& encoding) const;
    bool isValidTagName(const std::string& tagName) const;

    // State of MXP system:
    bool mMXP = false;
    TMXPMode mMXP_MODE = MXP_MODE_OPEN;
    TMXPMode mMXP_DEFAULT = MXP_MODE_OPEN;

    // MXP delegated handlers
    TMxpNodeBuilder mMxpTagBuilder;
    TMxpTagProcessor mMxpTagProcessor;
    // The creation of this element requires the preceding one:
    TEntityHandler mEntityHandler;

    TMxpClient* mpMxpClient = nullptr;

    // value of the last resolved entity:
    QString lastEntityValue;
};

#endif //MUDLET_TMXPPROCESSOR_H
