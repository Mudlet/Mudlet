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
#include "Host.h"

bool TMxpProcessor::negotiate(const QString& code)
{
    bool isOk = false;
    int modeCode = code.toInt(&isOk);
    if (isOk) {
        return negotiate(modeCode);
    } else {
        // isOk is false here as toInt(...) failed
        qDebug().noquote().nospace() << "TBuffer::translateToPlainText(...) INFO - Non-numeric MXP control sequence CSI " << code
                                     << " z received, Mudlet will ignore it.";
        return false;
    }
}
bool TMxpProcessor::negotiate(int modeCode)
{
    // we really do not handle these well...
    // MXP line modes - comments are from http://www.zuggsoft.com/zmud/mxp.htm#MXP%20Line%20Tags
    mMXP = true; // some servers don't negotiate, they assume!

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
        mMxpTagDetector.reset();
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
        qDebug().noquote().nospace() << "TBuffer::translateToPlainText(...) INFO - Unhandled MXP control sequence CSI "
                                     << QString::number(modeCode) << " z received, Mudlet will ignore it.";
        return false;
    }

    return true;
}
TMXPMode TMxpProcessor::getMode() const
{
    return mMXP_MODE;
}
bool TMxpProcessor::isEnabled() const
{
    return mMXP;
}
TMxpProcessingResult TMxpProcessor::processInput(char& ch,
                                                 std::string& localBuffer,
                                                 size_t& localBufferPosition,
                                                 size_t localBufferLength)
{
    if (mMxpTagDetector.handle(ch, localBufferPosition)) {
        return HANDLER_NEXT_CHAR;
    }

    if (mMxpTagDetector.hasToken()) {
        std::string currentToken = mMxpTagDetector.getToken();

        if (mMXP_MODE == MXP_MODE_TEMP_SECURE) {
            mMXP_MODE = mMXP_DEFAULT;
        }

        TMxpProcessingResult result = mMxpTagProcessor.process(mpHost->mTelnet, *mLinkStore, currentToken, ch);
        if (result != HANDLER_FALL_THROUGH) {
            return result;
        }
    }

    if (mEntityHandler.handle(ch, localBuffer, localBufferPosition, localBufferLength)) {
        return HANDLER_NEXT_CHAR;
    }

    mMxpTagProcessor.processTextContent(ch);

    return HANDLER_FALL_THROUGH;
}
void TMxpProcessor::resetToDefaultMode()
{
    mMXP_MODE = mMXP_DEFAULT;
}
void TMxpProcessor::enable()
{
    mMXP = true;
}
bool TMxpProcessor::isInLinkMode()
{
    return mMxpTagProcessor.isInLinkMode();
}
