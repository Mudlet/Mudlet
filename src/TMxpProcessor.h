#ifndef MUDLET_SRC_TMXPPROCESSOR_H
#define MUDLET_SRC_TMXPPROCESSOR_H

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

#include "TMxpTagDetector.h"
#include "TMxpTagProcessor.h"
#include "TEntityHandler.h"
#include "TLinkStore.h"
#include <QString>

class Host;

enum TMXPMode {
    MXP_MODE_OPEN,
    MXP_MODE_SECURE,
    MXP_MODE_LOCKED,
    MXP_MODE_TEMP_SECURE
};

// handles the MXP protocol
class TMxpProcessor {

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

    Host* mpHost;
    TLinkStore* mLinkStore;

    // State of MXP systen:
    bool mMXP;
    TMXPMode mMXP_MODE;
    TMXPMode mMXP_DEFAULT;

    // delegated handlers
    TMxpTagDetector mMxpTagDetector;
    TMxpTagProcessor mMxpTagProcessor;
    TEntityHandler mEntityHandler;

public:
    TMxpProcessor(Host* pH, TLinkStore* store)
            : mpHost(pH), mLinkStore(store), mMXP(false), mMXP_MODE(MXP_MODE_OPEN), mMXP_DEFAULT(MXP_MODE_OPEN)
    {}

    bool negotiate(const QString& code);
    bool negotiate(int modeCode);

    bool isEnabled() const;
    TMXPMode getMode() const;

    TMxpProcessingResult processInput(char& ch,
                                      std::string& localBuffer,
                                      size_t& localBufferPosition,
                                      size_t localBufferLength);
    void resetToDefaultMode();
    void enable();
    bool isInLinkMode();
};

#endif //MUDLET_SRC_TMXPPROCESSOR_H
