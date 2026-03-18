/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2018 by Stephen Lyons - slysven@virginmedia.com    *
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

#include "TMxpProcessor.h"
#include "TEncodingHelper.h"
#include <QDebug>

// Static sets of MXP tags from the specification
// See: https://www.zuggsoft.com/zmud/mxp.htm

const QSet<QString>& TMxpProcessor::openModeTags()
{
    // MXP spec: "Only the tags described in this section [Text Formatting] are OPEN tags.
    // All other MXP tags are SECURE tags."
    // This includes text formatting, line spacing, and optional HTML tags
    static const QSet<QString> tags = {
            // Text formatting (OPEN)
            qsl("B"),
            qsl("BOLD"),
            qsl("STRONG"),
            qsl("I"),
            qsl("ITALIC"),
            qsl("EM"),
            qsl("U"),
            qsl("UNDERLINE"),
            qsl("S"),
            qsl("STRIKEOUT"),
            qsl("C"),
            qsl("COLOR"),
            qsl("H"),
            qsl("HIGH"),
            qsl("FONT"),
            // Line spacing (OPEN)
            qsl("NOBR"),
            qsl("P"),
            qsl("BR"),
            qsl("SBR"),
            // Optional HTML tags (formatting)
            qsl("H1"),
            qsl("H2"),
            qsl("H3"),
            qsl("H4"),
            qsl("H5"),
            qsl("H6"),
            qsl("HR"),
            qsl("SMALL"),
            qsl("TT"),
    };
    return tags;
}

const QSet<QString>& TMxpProcessor::allMxpTags()
{
    static const QSet<QString> tags = []() {
        QSet<QString> result = openModeTags();
        result.unite({
                // Links (SECURE)
                qsl("SEND"),
                qsl("A"),
                qsl("EXPIRE"),
                // Version control (SECURE)
                qsl("VERSION"),
                qsl("SUPPORT"),
                // MSP compatibility (SECURE)
                qsl("SOUND"),
                qsl("MUSIC"),
                // Entity display (SECURE)
                qsl("GAUGE"),
                qsl("STAT"),
                // Frames and cursor control (SECURE)
                qsl("FRAME"),
                qsl("DEST"),
                // Cross-linking (SECURE)
                qsl("RELOCATE"),
                qsl("USER"),
                qsl("PASSWORD"),
                // Images (SECURE)
                qsl("IMAGE"),
                // File filters (SECURE)
                qsl("FILTER"),
                // Definition commands (SECURE)
                qsl("!ELEMENT"),
                qsl("!EL"),
                qsl("!ATTLIST"),
                qsl("!AT"),
                qsl("!ENTITY"),
                qsl("!EN"),
                qsl("!TAG"),
                // Variables (SECURE)
                qsl("VAR"),
                qsl("V"),
                // Welcome text
                qsl("WELCOME"),
                // HTML comments
                qsl("!--"),
        });
        return result;
    }();
    return tags;
}

bool TMxpProcessor::isRecognizedMxpTag(const QString& tagName) const
{
    const QString upper = tagName.toUpper();
    if (allMxpTags().contains(upper)) {
        return true;
    }
    return mMxpTagProcessor.getElementRegistry().containsElement(upper);
}

bool TMxpProcessor::isTagAllowedInCurrentMode(const QString& tagName) const
{
    if (mMXP_MODE == MXP_MODE_LOCKED) {
        return false;
    }

    if (mMXP_MODE == MXP_MODE_SECURE || mMXP_MODE == MXP_MODE_TEMP_SECURE) {
        return isRecognizedMxpTag(tagName);
    }

    const QString upper = tagName.toUpper();
    if (openModeTags().contains(upper)) {
        return true;
    }
    return mMxpTagProcessor.getElementRegistry().isOpenElement(upper);
}

bool TMxpProcessor::setMode(const QString& code)
{
    bool isOk = false;
    const int modeCode = code.toInt(&isOk);
    if (isOk) {
        return setMode(modeCode);
    } else {
        // isOk is false here as toInt(...) failed
        qDebug().noquote().nospace() << "TMxpProcessor::setMode(...) INFO - Non-numeric MXP control sequence CSI " << code << " z received, Mudlet will ignore it.";
        return false;
    }
}

/*
 * The documentation at https://www.zuggsoft.com/zmud/mxp.htm says: "
 * * 0 - OPEN LINE - initial default mode: only MXP commands in the 'open'
 *     category are allowed.  When a newline is received from the MUD, the
 *     mode reverts back to the Default mode.  OPEN mode starts as the
 *     default mode until changes with one of the 'lock mode' tags listed
 *     below.
 * * 1 - SECURE LINE (until next newline) all tags and commands in MXP are
 *     allowed within the line.  When a newline is received from the MUD,
 *     the mode reverts back to the Default mode.
 * * 2 - LOCKED LINE (until next newline) no MXP or HTML commands are
 *     allowed in the line.  The line is not parsed for any tags at all.
 *     This is useful for "verbatim" text output from the MUD.  When a
 *     newline is received from the MUD, the mode reverts back to the
 *     Default mode.
 * The following additional modes were added to the v0.4 MXP spec:
 * * 3 - RESET close all open tags.  Set mode to Open.  Set text color and
 *     properties to default.
 * * 4 - TEMP SECURE MODE set secure mode for the next tag only.  Must be
 *     immediately followed by a < character to start a tag.  Remember to
 *     set secure mode when closing the tag also.
 * * 5 - LOCK OPEN MODE set open mode.  Mode remains in effect until
 *     changed.  OPEN mode becomes the new default mode.
 * * 6 - LOCK SECURE MODE set secure mode.  Mode remains in effect until
 *     changed.  Secure mode becomes the new default mode.
 * * 7 - LOCK LOCKED MODE set locked mode.  Mode remains in effect until
 *     changed.  Locked mode becomes the new default mode."
 */
bool TMxpProcessor::setMode(int modeCode)
{
    // we really do not handle these well...
    // MXP line modes - comments are from http://www.zuggsoft.com/zmud/mxp.htm#MXP%20Line%20TagsmMXP = true; // some servers don't negotiate, they assume!

    mMXP = true;

    switch (modeCode) {
    case MXP_MODE_CODE_OPEN: // open line - only MXP commands in the "open" category are allowed.  When a newline is received from the MUD, the mode reverts back to the Default mode.  OPEN MODE starts as the Default mode until changes with one of the "lock mode" tags listed below.
        mMXP_MODE = MXP_MODE_OPEN;
        break;
    case MXP_MODE_CODE_SECURE: // secure line (until next newline) all tags and commands in MXP are allowed within the line.  When a newline is received from the MUD, the mode reverts back to the Default mode.
        // When the mode is changed from OPEN mode to any other mode, any unclosed OPEN tags are automatically closed.
        if (mMXP_MODE == MXP_MODE_OPEN) {
            mpMxpClient->resetTextProperties();
        }
        mMXP_MODE = MXP_MODE_SECURE;
        break;
    case MXP_MODE_CODE_LOCKED: // locked line (until next newline) no MXP or HTML commands are allowed in the line.  The line is not parsed for any tags at all.  This is useful for "verbatim" text output from the MUD.  When a newline is received from the MUD, the mode reverts back to the Default mode.
        // When the mode is changed from OPEN mode to any other mode, any unclosed OPEN tags are automatically closed.
        if (mMXP_MODE == MXP_MODE_OPEN) {
            mpMxpClient->resetTextProperties();
        }
        mMXP_MODE = MXP_MODE_LOCKED;
        break;
    case MXP_MODE_CODE_RESET: //  reset (MXP 0.4 or later) - close all open tags.  Set mode to Open.  Set text color and properties to default.
        mMxpTagBuilder.reset();
        mpMxpClient->resetTextProperties();
        mMXP_MODE = mMXP_DEFAULT;
        break;
    case MXP_MODE_CODE_TEMP_SECURE: // temp secure mode (MXP 0.4 or later) - set secure mode for the next tag only.  Must be immediately followed by a < character to start a tag.  Remember to set secure mode when closing the tag also.
        mMXP_MODE = MXP_MODE_TEMP_SECURE;
        break;
    case MXP_MODE_CODE_LOCK_OPEN: // lock open mode (MXP 0.4 or later) - set open mode.  Mode remains in effect until changed.  OPEN mode becomes the new default mode.
        // When force MXP is enabled with secure mode locked, prevent server from changing default back to OPEN
        if (mMXP_DEFAULT == MXP_MODE_SECURE && mpMxpClient && mpMxpClient->shouldLockModeToSecure()) {
            return true; // Acknowledge but don't change mode
        }
        mMXP_DEFAULT = mMXP_MODE = MXP_MODE_OPEN;
        break;
    case MXP_MODE_CODE_LOCK_SECURE: // lock secure mode (MXP 0.4 or later) - set secure mode.  Mode remains in effect until changed.  Secure mode becomes the new default mode.
        // When the mode is changed from OPEN mode to any other mode, any unclosed OPEN tags are automatically closed.
        if (mMXP_MODE == MXP_MODE_OPEN) {
            mpMxpClient->resetTextProperties();
        }
        mMXP_DEFAULT = mMXP_MODE = MXP_MODE_SECURE;
        break;
    case MXP_MODE_CODE_LOCK_LOCKED: // lock locked mode (MXP 0.4 or later) - set locked mode.  Mode remains in effect until changed.  Locked mode becomes the new default mode.
        // When force MXP is enabled with secure mode locked, prevent server from changing default to LOCKED
        if (mMXP_DEFAULT == MXP_MODE_SECURE && mpMxpClient && mpMxpClient->shouldLockModeToSecure()) {
            return true; // Acknowledge but don't change mode
        }
        // When the mode is changed from OPEN mode to any other mode, any unclosed OPEN tags are automatically closed.
        if (mMXP_MODE == MXP_MODE_OPEN) {
            mpMxpClient->resetTextProperties();
        }
        mMXP_DEFAULT = mMXP_MODE = MXP_MODE_LOCKED;
        break;
    default:
        qDebug().noquote().nospace() << "TMxpProcessor::setMode(...) INFO - Unhandled MXP control sequence CSI " << QString::number(modeCode) << " z received, Mudlet will ignore it.";
        return false;
    }

    return true;
}
TMXPMode TMxpProcessor::mode() const
{
    return mMXP_MODE;
}

TMXPMode TMxpProcessor::defaultMode() const
{
    return mMXP_DEFAULT;
}

bool TMxpProcessor::isEnabled() const
{
    return mMXP;
}

void TMxpProcessor::resetToDefaultMode()
{
    // Also, when in OPEN mode, any unclosed OPEN tags are automatically closed when a newline is received from the MUD.
    if (mMXP_MODE == MXP_MODE_OPEN) {
        mpMxpClient->resetTextProperties();
    }
    mMXP_MODE = mMXP_DEFAULT;
}

void TMxpProcessor::enable()
{
    mMXP = true;
}

void TMxpProcessor::disable()
{
    mMXP = false;
}

TMxpProcessingResult TMxpProcessor::processMxpInput(char& ch, bool resolveCustomEntities)
{
    // LOCKED mode: per MXP spec, line is not parsed for any tags at all
    if (mMXP_MODE == MXP_MODE_LOCKED) {
        mMxpTagProcessor.handleContent(ch);
        return HANDLER_FALL_THROUGH;
    }

    // Newline while inside a tag: MXP tags cannot span lines
    // Reject the partial tag as literal text, then let the newline trigger line commit
    if ((ch == '\n' || ch == '\r') && mMxpTagBuilder.isInsideTag() && !mMxpTagBuilder.hasTag() && !mMxpTagBuilder.isInsideComment()) {
        const std::string rawBytes = mMxpTagBuilder.getRawTagContent();
        const QString decoded = decodeRawBytes(rawBytes, mpMxpClient->getEncoding());

        lastEntityValue = qsl("<") + decoded;
        mMxpTagBuilder.reset();
        // Return HANDLER_INSERT_AND_REPROCESS to output the rejected tag
        // and reprocess the newline character (which will cause line commit)
        return HANDLER_INSERT_AND_REPROCESS;
    }

    if (ch == '<' && mMxpTagBuilder.isInsideTag() && !mMxpTagBuilder.isQuotedSequence() && !mMxpTagBuilder.isInsideComment()) {
        // Error recovery: nested '<' inside a tag
        // Output the incomplete tag as text and prepare to process the new '<' as a tag start
        const std::string rawBytes = mMxpTagBuilder.getRawTagContent();
        const QString decoded = decodeRawBytes(rawBytes, mpMxpClient->getEncoding());

        lastEntityValue = qsl("<") + decoded;
        // resetForNewTag() puts the builder in "inside tag" state, as if we just processed '<'
        // This allows the next character to be processed as part of the new tag
        mMxpTagBuilder.resetForNewTag();
        return HANDLER_INSERT_ENTITY_SYS;
    }

    if (!mMxpTagBuilder.accept(ch) && mMxpTagBuilder.isInsideTag() && !mMxpTagBuilder.hasTag()) {
        // Character consumed, tag still building - validate the tag name
        // against known MXP tags for early rejection of non-MXP content.
        const std::string partialName = mMxpTagBuilder.getPartialTagName();
        if (!partialName.empty()) {
            // First check: reject immediately if tag name contains invalid characters
            // like %, ^, @, etc. This catches non-MXP content like <%^BOLD immediately
            if (!isValidTagName(partialName)) {
                return rejectCurrentTag();
            }
            // Second check: if tag name is complete (hit a boundary), validate
            // against known MXP tags. We intentionally do NOT reject based on
            // partial prefix matching because "SE" might not match any OPEN-mode
            // tag yet still be a legitimate MXP tag (SEND) that we need to
            // collect fully before deciding.
            if (mMxpTagBuilder.isTagNameComplete()) {
                const QString qPartialName = QString::fromStdString(partialName);
                if (!isTagAllowedInCurrentMode(qPartialName)) {
                    return rejectCurrentTag();
                }
            }
        }
        return HANDLER_NEXT_CHAR;
    }
    if (mMxpTagBuilder.hasTag()) {
        // Save raw tag content before it gets cleared by buildTag()
        // Note: getRawTagContent() returns content INCLUDING the closing '>'
        const std::string rawTagBytes = mMxpTagBuilder.getRawTagContent();
        const QByteArray encoding = mpMxpClient->getEncoding();

        const QString rawTagContent = qsl("<") + decodeRawBytes(rawTagBytes, encoding);

        QScopedPointer<MxpTag> const tag(mMxpTagBuilder.buildTag());

        if (!isTagAllowedInCurrentMode(tag->getName())) {
            lastEntityValue = rawTagContent;
            return HANDLER_INSERT_ENTITY_SYS;
        }

        TMxpTagHandlerResult const result = mMxpTagProcessor.handleTag(mMxpTagProcessor, *mpMxpClient, tag.get());

        if (mMXP_MODE == MXP_MODE_TEMP_SECURE) {
            mMXP_MODE = mMXP_DEFAULT;
        }

        // If tag was not handled (not valid MXP and not a custom element), display it as-is
        // Use HANDLER_INSERT_ENTITY_SYS so the Unicode content is inserted directly
        // without being reprocessed through toLatin1() which would destroy non-ASCII chars
        if (result == MXP_TAG_NOT_HANDLED) {
            lastEntityValue = rawTagContent;
            return HANDLER_INSERT_ENTITY_SYS;
        }

        return result == MXP_TAG_COMMIT_LINE ? HANDLER_COMMIT_LINE : HANDLER_NEXT_CHAR;
    }

    if (mEntityHandler.handle(ch, resolveCustomEntities)) { // ch is part of an entity
        if (mEntityHandler.isEntityResolved()) {            // entity has been mapped (i.e. ch == ';')
            lastEntityValue = mEntityHandler.getResultAndReset();
            switch (mEntityHandler.getEntityType()) {
            case ENTITY_TYPE_CUSTOM:
                return HANDLER_INSERT_ENTITY_CUST;
            case ENTITY_TYPE_SYSTEM:
                // Note special handling for '\n' as a result of &newline;
                return lastEntityValue == qsl("\n") ? HANDLER_COMMIT_LINE : HANDLER_INSERT_ENTITY_SYS;
            default:
                return HANDLER_INSERT_ENTITY_LIT;
            }
        } else { // ask for the next char
            return HANDLER_NEXT_CHAR;
        }
    }

    mMxpTagProcessor.handleContent(ch);

    return HANDLER_FALL_THROUGH;
}

QString TMxpProcessor::decodeRawBytes(const std::string& raw, const QByteArray& encoding) const
{
    if (encoding == QByteArrayLiteral("UTF-8")) {
        return QString::fromStdString(raw);
    } else if (encoding == QByteArrayLiteral("ISO 8859-1")) {
        return QString::fromLatin1(raw.c_str(), static_cast<int>(raw.length()));
    } else {
        return TEncodingHelper::decode(QByteArray::fromRawData(raw.c_str(), raw.length()), encoding);
    }
}

bool TMxpProcessor::isValidTagName(const std::string& tagName) const
{
    if (tagName.empty()) {
        return true;
    }
    for (char ch : tagName) {
        // Valid tag name characters: A-Z, a-z, 0-9, underscore, hyphen, slash (for closing tags), exclamation mark (for !ELEMENT)
        if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '-' || ch == '/' || ch == '!')) {
            return false;
        }
    }
    return true;
}

QString TMxpProcessor::abortCurrentTag()
{
    const std::string rawBytes = mMxpTagBuilder.getRawTagContent();
    const QString decoded = decodeRawBytes(rawBytes, mpMxpClient->getEncoding());
    const QString result = qsl("<") + decoded;
    mMxpTagBuilder.reset();
    return result;
}

TMxpProcessingResult TMxpProcessor::rejectCurrentTag()
{
    const std::string rawBytes = mMxpTagBuilder.getRawTagContent();

    // Remove the last byte (the character that triggered or followed rejection).
    // That character will be re-processed through HANDLER_INSERT_AND_REPROCESS,
    // allowing ANSI escape sequences starting with ESC to be properly handled
    // instead of being output as literal text.
    std::string validPrefix = rawBytes.empty() ? "" : rawBytes.substr(0, rawBytes.length() - 1);

    const QByteArray encoding = mpMxpClient->getEncoding();

    // Trim any incomplete trailing UTF-8 sequence to avoid passing it to
    // decodeRawBytes. First strip continuation bytes (10xxxxxx, 0x80-0xBF),
    // then remove any orphaned leading byte (0xC0+) left behind.
    // Only apply this for UTF-8; single-byte encodings like ISO 8859-1 use
    // 0x80-0xBF for valid characters.
    if (encoding == QByteArrayLiteral("UTF-8")) {
        while (!validPrefix.empty()) {
            unsigned char lastByte = static_cast<unsigned char>(validPrefix.back());
            if ((lastByte & 0xC0) == 0x80) {
                validPrefix.pop_back();
            } else {
                break;
            }
        }

        if (!validPrefix.empty()) {
            unsigned char lastByte = static_cast<unsigned char>(validPrefix.back());

            if (lastByte >= 0xC0) {
                validPrefix.pop_back();
            }
        }
    }

    const QString decoded = decodeRawBytes(validPrefix, encoding);

    lastEntityValue = qsl("<") + decoded;
    mMxpTagBuilder.reset();
    return HANDLER_INSERT_AND_REPROCESS;
}

void TMxpProcessor::processRawInput(char ch)
{
    mMxpTagProcessor.handleContent(ch);
}
