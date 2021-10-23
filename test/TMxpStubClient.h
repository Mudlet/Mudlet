/***************************************************************************
 *   Copyright (C) 2020 by Gustavo Sousa - gustavocms@gmail.com            *
 *   Copyright (C) 2021 by Florian Scheel - keneanung@gmail.com            *
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

// Text Formatting 'Counters', version, style by Michael Weller, michael.weller@t-online.de



#ifndef MUDLET_TEST_TMXPSTUBCLIENT_H
#define MUDLET_TEST_TMXPSTUBCLIENT_H

#include <QDebug>
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

    // These are also a kind of stack for parameters as fg/bgColours, but here a
    // simple counter suffices:
    int boldCtr = 0, italicCtr = 0, underlineCtr = 0, strikeOutCtr = 0;
    QString mxpStyle;

    QStringList mHrefs, mHints;

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

    void setBold(bool bold) override;
    void setItalic(bool italic) override;
    void setUnderline(bool underline) override;
    void setStrikeOut(bool strikeOut) override;

    bool isBold() override { return boldCtr > 0; }
    bool isItalic() override { return italicCtr > 0; }
    bool isUnderline() override { return underlineCtr > 0; }
    bool isStrikeOut() override { return strikeOutCtr > 0; }

    void setStyle(const QString& val) override {mxpStyle = val; }
    const QString &getStyle() override {return mxpStyle;}

    int setLink(const QStringList& hrefs, const QStringList& hints) override
    {
        qDebug() << QString("setLink([%1], [%2])").arg(hrefs.join(", ")).arg(hints.join(", "));
        mHrefs = hrefs;
        mHints = hints;

        return 1;
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

    void publishEntity(const QString& name, const QString& value) override {}

    void setVariable(const QString& name, const QString& value) override {}
};

#endif //MUDLET_TEST_TMXPSTUBCLIENT_H
