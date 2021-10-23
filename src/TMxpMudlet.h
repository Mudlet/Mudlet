#ifndef MUDLET_TMXPMUDLET_H
#define MUDLET_TMXPMUDLET_H

/***************************************************************************
 *   Copyright (C) 2020 by Gustavo Sousa - gustavocms@gmail.com            *
 *   Copyright (C) 2020 by Stephen Lyons - slysven@virginmedia.com         *
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

#include "pre_guard.h"
#include <QList>
#include <QQueue>
#include "post_guard.h"

class Host;
class TMediaData;

class TMxpMudlet : public TMxpClient
{

    // These are also a kind of stack for parameters as fg/bgColours, but here a
    // simple counter suffices:
    int boldCtr, italicCtr, underlineCtr, strikeOutCtr;
    QString mxpStyle;

public:
    TMxpMudlet(Host* pHost)
    : boldCtr(0)
    , italicCtr(0)
    , underlineCtr(0)
    , strikeOutCtr(0)
    , mpHost(pHost)
    , mLinkMode(false)
    {}

    QString getVersion() override;

    void sendToServer(QString& str) override;

    void setLinkMode(bool val) override { mLinkMode = val; }
    bool isInLinkMode() const { return mLinkMode; }

    void pushColor(const QString& fgColor, const QString& bgColor) override;

    void popColor() override;

    static void pushColor(QList<QColor>& stack, const QString& color);

    static void popColor(QList<QColor>& stack);

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

    void playMedia(TMediaData& mediaData) override;
    void stopMedia(TMediaData& mediaData) override;

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

private:
    inline static const QString scmVersion = QStringLiteral(APP_VERSION APP_BUILD);

    Host* mpHost;
    bool mLinkMode;
};

#endif //MUDLET_TMXPMUDLET_H
