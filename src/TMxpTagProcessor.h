#ifndef MUDLET_SRC_TMXPTAGPROCESSOR_H
#define MUDLET_SRC_TMXPTAGPROCESSOR_H

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

#include <QString>
#include <QMap>
#include <QVector>

#include "TLinkStore.h"
#include "ctelnet.h"

struct TMxpElement {
    QString name;
    QString href;
    QString hint;
};

enum TMxpProcessingResult {
    HANDLER_SKIP_INPUT, HANDLER_NEXT_CHAR, HANDLER_COMMIT_LINE
};

class TMxpTagProcessor {
    static const QMap<QString, QVector<QString>> mSupportedMxpElements;

    QMap<QString, TMxpElement> mMXP_Elements;
    std::string mAssembleRef;
    bool mMXP_LINK_MODE;
    bool mMXP_SEND_NO_REF_MODE;

public:
    TMxpTagProcessor() : mMXP_LINK_MODE(false), mMXP_SEND_NO_REF_MODE(false)
    {
        TMxpElement _element;
        _element.name = "SEND";
        _element.href = "";
        _element.hint = "";
        mMXP_Elements["SEND"] = _element;

        TMxpElement _aURL;
        _aURL.name = "A";
        _aURL.href = "";
        _aURL.hint = "";
        mMXP_Elements["A"] = _aURL;
    }
    TMxpProcessingResult process(cTelnet& mTelnet, TLinkStore& mLinkStore, const std::string& currentToken, char& ch);
    TMxpProcessingResult processElementDefinition(const std::string& currentToken, QString _tn);
    QString processSupportsRequest(const QString& elements);

    bool isInLinkMode() const;
    TMxpProcessingResult processCustomElement(TLinkStore& mLinkStore, const std::string& currentToken, QString _tn);
    void processTextContent(char& ch);
};

#endif //MUDLET_SRC_TMXPTAGPROCESSOR_H
