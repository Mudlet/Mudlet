#ifndef MUDLET_TMXPMUDLET_H
#define MUDLET_TMXPMUDLET_H

/***************************************************************************
 *   Copyright (C) 2020 by Gustavo Sousa - gustavocms@gmail.com            *
 *   Copyright (C) 2020, 2022 by Stephen Lyons - slysven@virginmedia.com   *
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

#include "TEntityResolver.h"
#include "TLinkStore.h"
#include "TMxpClient.h"
#include "TMxpEvent.h"

#include <QList>
#include <QQueue>
#include <QStack>

class Host;
class TMediaData;

class TMxpMudlet : public TMxpClient
{
    // Count how many of this format have been stacked/applied on top of each other
    unsigned int boldCounter = 0;
    unsigned int italicCounter = 0;
    unsigned int underlineCounter = 0;
    unsigned int strikeOutCounter = 0;
    QString mxpStyle; // Name/Version of the MXP style sheet uploaded by the mud

public:
    explicit TMxpMudlet(Host* pHost)
    : mpHost(pHost)
    , mLinkMode(false)
    {}

    QString getVersion() override;

    void sendToServer(QString& str) override;

    void setLinkMode(bool val) override { mLinkMode = val; }
    bool isInLinkMode() const { return mLinkMode; }

    void pushColor(const QString& fgColor, const QString& bgColor) override;

    void popColor() override;

    void pushColor(QList<QColor>& stack, const QString& color);

    void popColor(QList<QColor>& stack);

    bool hasFgColor() const { return !fgColors.isEmpty(); }
    const QColor& getFgColor() { return fgColors.last(); }

    bool hasBgColor() const { return !bgColors.isEmpty(); }

    const QColor& getBgColor() { return bgColors.last(); }

    // TODO: implement support for fonts?
    void pushFont(const QString& fontFace, const QString& fontSize) override {
        Q_UNUSED(fontFace)
        Q_UNUSED(fontSize)
    }

    void popFont() override {}

    int setLink(const QStringList& links, const QStringList& hints) override;

    bool getLink(int id, QStringList** links, QStringList** hints) override;

    // EXPIRE tag support
    int setLink(const QStringList& links, const QStringList& hints, const QString& expireName) override;
    void expireLinks(const QString& expireName) override;

    void playMedia(TMediaData& mediaData) override;
    void stopMedia(TMediaData& mediaData) override;

    void setBold(bool bold) override;
    void setItalic(bool italic) override;
    void setUnderline(bool underline) override;
    void setStrikeOut(bool strikeOut) override;

    bool bold() override { return boldCounter > 0; }
    bool italic() override { return italicCounter > 0; }
    bool underline() override { return underlineCounter > 0; }
    bool strikeOut() override { return strikeOutCounter > 0; }

    void resetTextProperties() override;

    void setStyle(const QString& val) override { mxpStyle = val; }
    QString getStyle() override { return mxpStyle;}

    void setFlag(const QString& elementName, const QMap<QString, QString>& values, const QString& content) override {
        Q_UNUSED(elementName)
        Q_UNUSED(values)
        Q_UNUSED(content)
        // TODO: raise mxp event
    }

    void publishEntity(const QString& name, const QString& value) override {
        Q_UNUSED(name)
        Q_UNUSED(value)
    }

    void setVariable(const QString& name, const QString& value) override {
        Q_UNUSED(name)
        Q_UNUSED(value)
    }

    TMxpTagHandlerResult tagHandled(MxpTag* tag, TMxpTagHandlerResult result) override;

    void enqueueMxpEvent(MxpStartTag* tag);
    TLinkStore& getLinkStore();

    QList<QColor> fgColors;
    QList<QColor> bgColors;

    // Shouldn't be here, look for a better solution
    QQueue<TMxpEvent> mMxpEvents;

    void setCaptionForSendEvent(const QString& caption) override;

    QStack<TMxpEvent> mPendingSendEvents;

private:
    Host* mpHost;
    bool mLinkMode;
};

#endif //MUDLET_TMXPMUDLET_H
