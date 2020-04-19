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
#include "TMxpNodeBuilder.h"
#include <QString>

class Host;

enum TMXPMode {
    MXP_MODE_OPEN,
    MXP_MODE_SECURE,
    MXP_MODE_LOCKED,
    MXP_MODE_TEMP_SECURE
};
enum TMxpProcessingResult {
    HANDLER_FALL_THROUGH, HANDLER_NEXT_CHAR, HANDLER_COMMIT_LINE
};

// handles the MXP protocol
class TMxpProcessor {
    Host* mpHost;

    // State of MXP systen:
    bool mMXP;
    TMXPMode mMXP_MODE;
    TMXPMode mMXP_DEFAULT;

    TEntityHandler mEntityHandler;

    // delegated handlers
    TMxpTagDetector mMxpTagDetector;
    TMxpNodeBuilder mMxpTagBuilder;
    TMxpTagProcessor mMxpTagProcessor;

    TMxpClient* mpMxpClient;

public:
    TMxpProcessor(Host* pHost, TMxpClient* pMxpClient) :
            mMxpTagBuilder(true),
            mpHost(pHost),
            mMXP(false),
            mMXP_MODE(MXP_MODE_OPEN), mMXP_DEFAULT(MXP_MODE_OPEN),
            mpMxpClient(pMxpClient)
    {
        mpMxpClient->initialize(&mMxpTagProcessor);
    }

    bool setMode(const QString& code);
    bool setMode(int modeCode);

    bool isEnabled() const;
    void enable();
    TMXPMode getMode() const;
    void resetToDefaultMode();

    TMxpProcessingResult processMxpInput(char& ch);
    void processRawInput(char ch);
};

#endif //MUDLET_SRC_TMXPPROCESSOR_H
