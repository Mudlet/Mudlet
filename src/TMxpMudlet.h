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
#ifndef MUDLET_SRC_TMXPMUDLET_H
#define MUDLET_SRC_TMXPMUDLET_H

#include "Host.h"
#include "TEntityResolver.h"
#include "TLinkStore.h"
#include "TMxpClient.h"
#include "TMxpEvent.h"

#include <QList>
#include <QQueue>

class TMxpMudlet : public TMxpClient
{
    inline static const QString scmVersion = QStringLiteral("%1%2").arg(APP_VERSION, APP_BUILD);

    Host* mpHost;
    TLinkStore* mpLinkStore;

    bool mLinkMode;

public:
    // Shouldn't be here, look for a better solution
    QQueue<TMxpEvent> mMxpEvents;

    TMxpMudlet(Host* pHost, TLinkStore* linkStore) : mpHost(pHost), mpLinkStore(linkStore), mLinkMode(false) {}

    virtual QString getVersion() { return scmVersion; }

    virtual void sendToServer(QString& str) { mpHost->mTelnet.sendData(str); }

    void setLinkMode(bool val) override { mLinkMode = val; }

    bool isInLinkMode() { return mLinkMode; }

    QList<QColor> fgColors, bgColors;
    virtual void pushColor(const QString& fgColor, const QString& bgColor)
    {
        pushColor(fgColors, fgColor);
        pushColor(bgColors, bgColor);
    }

    virtual void popColor()
    {
        popColor(fgColors);
        popColor(bgColors);
    }

    static void pushColor(QList<QColor>& stack, const QString& color)
    {
        if (color.isEmpty()) {
            if (!stack.isEmpty()) {
                stack.push_back(stack.last());
            }
        } else {
            stack.push_back(mapColor(color));
        }
    }

    static void popColor(QList<QColor>& stack)
    {
        if (!stack.isEmpty()) {
            stack.pop_back();
        }
    }

    bool hasFgColor() { return !fgColors.isEmpty(); }
    const QColor& getFgColor() { return fgColors.last(); }

    bool hasBgColor() { return !bgColors.isEmpty(); }

    const QColor& getBgColor() { return bgColors.last(); }

    static QColor mapColor(const QString& colorName)
    {
        if (colorName.startsWith("#")) {
            return QColor::fromRgb(QRgb(colorName.mid(1).toInt(nullptr, 16)));
        } else {
            return QColor(colorName);
        }
    }

    // TODO: implement support for fonts?
    void pushFont(const QString& fontFace, const QString& fontSize) override {}
    void popFont() override {}

    int setLink(const QStringList& links, const QStringList& hints) override { return mpLinkStore->addLinks(links, hints); }

    bool getLink(int id, QStringList** links, QStringList** hints) override
    {
        *links = &mpLinkStore->getLinks(id);
        *hints = &mpLinkStore->getHints(id);

        return true;
    }

    bool isBold, isItalic, isUnderline;

    void setBold(bool bold) override { isBold = bold; }
    void setItalic(bool italic) override { isItalic = italic; }
    void setUnderline(bool underline) override { isUnderline = underline; }

    virtual void setFlag(const QString& elementName, const QMap<QString, QString>& values, const QString& content)
    {
        // TODO: raise mxp event
    }

    void publishEntity(const QString& name, const QString& value) override {}

    void setVariable(const QString& name, const QString& value) override {}

    TMxpTagHandlerResult tagHandled(MxpTag* tag, TMxpTagHandlerResult result) override
    {
        if (tag->isStartTag()) {
            if (mpContext->getElementRegistry().containsElement(tag->getName())) {
                raiseMxpEvent(tag->asStartTag());
            } else if (tag->isNamed("SEND")) {
                raiseMxpEvent(tag->asStartTag());
            }
        }

        return result;
    }

    void raiseMxpEvent(MxpStartTag* tag)
    {
        TMxpEvent ev;
        ev.name = tag->getName();
        for (const auto& attrName : tag->getAttrsNames()) {
            ev.attrs[attrName] = tag->getAttrValue(attrName);
        }
        ev.actions = mpLinkStore->getCurrentLinks();
        mMxpEvents.enqueue(ev);
    }
};

#endif //MUDLET_SRC_TMXPMUDLET_H
