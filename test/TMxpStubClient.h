//
// Created by gustavo on 19/04/2020.
//

#ifndef MUDLET_TEST_TMXPSTUBCLIENT_H
#define MUDLET_TEST_TMXPSTUBCLIENT_H

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
    QString version;
    bool linkMode;

    QString sentToServer;

    QString fgColor, bgColor;

    QStringList mHrefs, mHints;

    QString mPublishedEntityName, mPublishedEntityValue;

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

    bool isBold, isItalic, isUnderline;

    void setBold(bool bold) override
    {
        isBold = bold;
    }
    void setItalic(bool italic) override
    {
        isItalic = italic;
    }
    void setUnderline(bool underline) override
    {
        isUnderline = underline;
    }

    int setLink(const QStringList& hrefs, const QStringList& hints) override
    {
        qDebug() << qsl("setLink([%1], [%2])").arg(hrefs.join(", ")).arg(hints.join(", "));
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

    void publishEntity(const QString& name, const QString& value) override
    {
        qDebug() << qsl("publishEntity([%1], [%2])").arg(name, value);
        mPublishedEntityName = name;
        mPublishedEntityValue = value;
    }

    void setVariable(const QString& name, const QString& value) override {}
};

#endif //MUDLET_TEST_TMXPSTUBCLIENT_H
