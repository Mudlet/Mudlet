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
#include "pre_guard.h"
#include <QDebug>
#include "post_guard.h"

bool TMxpProcessor::setMode(const QString& code)
{
    bool isOk = false;
    int modeCode = code.toInt(&isOk);
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
    case 0: // open line - only MXP commands in the "open" category are allowed.  When a newline is received from the MUD, the mode reverts back to the Default mode.  OPEN MODE starts as the Default mode until changes with one of the "lock mode" tags listed below.
        mMXP_MODE = MXP_MODE_OPEN;
        break;
    case 1: // secure line (until next newline) all tags and commands in MXP are allowed within the line.  When a newline is received from the MUD, the mode reverts back to the Default mode.
        mMXP_MODE = MXP_MODE_SECURE;
        break;
    case 2: // locked line (until next newline) no MXP or HTML commands are allowed in the line.  The line is not parsed for any tags at all.  This is useful for "verbatim" text output from the MUD.  When a newline is received from the MUD, the mode reverts back to the Default mode.
        mMXP_MODE = MXP_MODE_LOCKED;
        break;
    case 3: //  reset (MXP 0.4 or later) - close all open tags.  Set mode to Open.  Set text color and properties to default.
        mMxpTagBuilder.reset();
        mMXP_MODE = mMXP_DEFAULT;
        break;
    case 4: // temp secure mode (MXP 0.4 or later) - set secure mode for the next tag only.  Must be immediately followed by a < character to start a tag.  Remember to set secure mode when closing the tag also.
        mMXP_MODE = MXP_MODE_TEMP_SECURE;
        break;
    case 5: // lock open mode (MXP 0.4 or later) - set open mode.  Mode remains in effect until changed.  OPEN mode becomes the new default mode.
        mMXP_DEFAULT = mMXP_MODE = MXP_MODE_OPEN;
        break;
    case 6: // lock secure mode (MXP 0.4 or later) - set secure mode.  Mode remains in effect until changed.  Secure mode becomes the new default mode.
        mMXP_DEFAULT = mMXP_MODE = MXP_MODE_SECURE;
        break;
    case 7: // lock locked mode (MXP 0.4 or later) - set locked mode.  Mode remains in effect until changed.  Locked mode becomes the new default mode.
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
bool TMxpProcessor::isEnabled() const
{
    return mMXP;
}

void TMxpProcessor::resetToDefaultMode()
{
    mMXP_MODE = mMXP_DEFAULT;
}

void TMxpProcessor::enable()
{
    mMXP = true;
}

TMxpProcessingResult TMxpProcessor::processMxpInput(char& ch)
{
    if (!mMxpTagBuilder.accept(ch) && mMxpTagBuilder.isInsideTag() && !mMxpTagBuilder.hasTag()) {
        return HANDLER_NEXT_CHAR;
    }

    if (mMxpTagBuilder.hasTag()) {
        QScopedPointer<MxpTag> tag(mMxpTagBuilder.buildTag());

        //        qDebug() << "TAG RECEIVED: " << tag->asString();
        if (mMXP_MODE == MXP_MODE_TEMP_SECURE) {
            mMXP_MODE = mMXP_DEFAULT;
        }

        TMxpTagHandlerResult result = mMxpTagProcessor.handleTag(mMxpTagProcessor, *mpMxpClient, tag.get());
        return result == MXP_TAG_COMMIT_LINE ? HANDLER_COMMIT_LINE : HANDLER_NEXT_CHAR;
    }

    if (mEntityHandler.handle(ch)) {             // ch is part of an entity
        if (mEntityHandler.isEntityResolved()) { // entity has been mapped (i.e. ch == ';')
            ch = mEntityHandler.getResultAndReset();
        } else { // ask for the next char
            return HANDLER_NEXT_CHAR;
        }
    }

    mMxpTagProcessor.handleContent(ch);

    return HANDLER_FALL_THROUGH;
}

void TMxpProcessor::processRawInput(char ch)
{
    mMxpTagProcessor.handleContent(ch);
}
