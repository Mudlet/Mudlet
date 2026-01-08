/***************************************************************************
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

#ifndef MUDLET_TEST_TMXPSTUBCLIENT_H
#define MUDLET_TEST_TMXPSTUBCLIENT_H

#include <qdebug.h>
#include "TMxpContext.h"
#include "TMxpClient.h"
#include "TMediaData.h"

class TMxpStubContext : public TMxpContext {
public:
    TMxpElementRegistry mElementRegistry;
    QMap<QString, QVector<QString>> mSupportedElements;
    TEntityResolver mEntityResolver;

    TMxpElementRegistry& getElementRegistry() override
    {
        return mElementRegistry;
    }
    QMap<QString, QVector<QString>>& getSupportedElements() override
    {
        return mSupportedElements;
    }

    TMxpTagHandlerResult handleTag(TMxpContext& ctx, TMxpClient& client, MxpTag* tag) override
    {
        return MXP_TAG_HANDLED;
    }

    void handleContent(char ch) override
    {

    }

    TMxpTagHandler& getMainHandler() override
    {
        return *this;
    }

    TEntityResolver& getEntityResolver() override
    {
        return mEntityResolver;
    }
};

class TMxpStubClient : public TMxpClient {
public:
    QString version = "Stub-1.0";
    bool linkMode;

    QString sentToServer;

    QString fgColor, bgColor;

    QStringList mHrefs, mHints;

    QString mExpireName;

    QString mPublishedEntityName, mPublishedEntityValue;

    QString style;

    // Count how many of this format have been stacked/applied on top of each other
    unsigned int boldCounter = 0;
    unsigned int italicCounter = 0;
    unsigned int underlineCounter = 0;
    unsigned int strikeOutCounter = 0;

    QString getVersion() override
    {
        return version;
    }

    void sendToServer(QString& str) override
    {
        sentToServer.append(str);
    }
    void setLinkMode(bool val) override
    {
        this->linkMode = val;
    }
    void setFlag(const QString& elementName, const QMap<QString, QString>& params, const QString& content) override
    {

    }
    void pushColor(const QString& fgColor, const QString& bgColor) override
    {
        this->fgColor = fgColor;
        this->bgColor = bgColor;
    }
    void popColor() override
    {
        fgColor.clear();
        bgColor.clear();
    }
    void pushFont(const QString& fontFace, const QString& fontSize) override
    {

    }
    void popFont() override
    {

    }

    void setBold(bool bold) override
    {
        if (bold) {
            boldCounter++;
        } else if (boldCounter > 0) {
            boldCounter--;
        }
    }

    void setItalic(bool italic) override
    {
        if (italic) {
            italicCounter++;
        } else if (italicCounter > 0) {
            italicCounter--;
        }
    }

    void setUnderline(bool underline) override
    {
        if (underline) {
            underlineCounter++;
        } else if (underlineCounter > 0) {
            underlineCounter--;
        }
    }

    void setStrikeOut(bool strikeOut) override
    {
        if (strikeOut) {
            strikeOutCounter++;
        } else if (strikeOutCounter > 0) {
            strikeOutCounter--;
        }
    }

    bool bold() override
    {
        return boldCounter > 0;
    }
    bool italic() override
    {
        return italicCounter > 0;
    }
    bool underline() override
    {
        return underlineCounter > 0;
    }
    bool strikeOut() override
    {
        return strikeOutCounter > 0;
    }

    void resetTextProperties() override
    {
    }

    void setStyle(const QString& val) override
    {
        style = val;
    }
    QString getStyle() override
    {
        return style;
    }

    int setLink(const QStringList& hrefs, const QStringList& hints) override
    {
        qDebug().noquote() << qsl("setLink([%1], [%2])").arg(hrefs.join(", "), hints.join(", "));
        mHrefs = hrefs;
        mHints = hints;

        return 1;
    }

    int setLink(const QStringList& hrefs, const QStringList& hints, const QString& expireName) override
    {
        qDebug().noquote() << qsl("setLink([%1], [%2], [%3])").arg(hrefs.join(", "), hints.join(", "), expireName);
        mHrefs = hrefs;
        mHints = hints;
        mExpireName = expireName;

        return 1;
    }

    void expireLinks(const QString& expireName) override
    {
        qDebug().noquote() << qsl("expireLinks([%1])").arg(expireName);
    }

    bool getLink(int id, QStringList** href, QStringList** hint) override
    {
        *href = &mHrefs;
        *hint = &mHints;

        return true;
    }

    void playMedia(TMediaData& mediaData) override
    {

    }

    void stopMedia(TMediaData& mediaData) override
    {

    }

    void publishEntity(const QString& name, const QString& value) override
    {
        qDebug().noquote() << qsl("publishEntity([%1], [%2])").arg(name, value);
        mPublishedEntityName = name;
        mPublishedEntityValue = value;
    }

    void setVariable(const QString& name, const QString& value) override {}

    // MXP Frame/Dest tracking
    bool createMxpFrameCalled = false;
    QString lastCreatedFrameName;
    QMap<QString, QString> lastFrameAttributes;
    
    bool setMxpDestinationCalled = false;
    QString lastDestinationName;
    bool lastDestinationEol = false;
    bool lastDestinationEof = false;
    
    bool clearMxpDestinationCalled = false;
    
    bool createMxpFrame(const QString& name, const QMap<QString, QString>& attributes) override
    {
        createMxpFrameCalled = true;
        lastCreatedFrameName = name;
        lastFrameAttributes = attributes;
        return false;
    }
    
    bool setMxpDestination(const QString& frameName, bool eol, bool eof) override
    {
        setMxpDestinationCalled = true;
        lastDestinationName = frameName;
        lastDestinationEol = eol;
        lastDestinationEof = eof;
        return false;
    }
    
    void clearMxpDestination() override
    {
        clearMxpDestinationCalled = true;
    }
    
    void resetMxpTracking()
    {
        createMxpFrameCalled = false;
        lastCreatedFrameName.clear();
        lastFrameAttributes.clear();
        setMxpDestinationCalled = false;
        lastDestinationName.clear();
        lastDestinationEol = false;
        lastDestinationEof = false;
        clearMxpDestinationCalled = false;
    }
};

#endif //MUDLET_TEST_TMXPSTUBCLIENT_H
