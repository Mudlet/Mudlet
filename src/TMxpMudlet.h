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

#include "TEntityResolver.h"
#include "TMxpClient.h"
#include "TMxpEvent.h"

#include <QList>
#include <QQueue>

class Host;
class TLinkStore;

class TMxpMudlet : public TMxpClient
{
    inline static const QString scmVersion = QStringLiteral("%1%2");//.arg(APP_VERSION, APP_BUILD);

    Host* mpHost;
    TLinkStore* mpLinkStore;

    bool mLinkMode;

public:
    // Shouldn't be here, look for a better solution
    QQueue<TMxpEvent> mMxpEvents;

    TMxpMudlet(Host* pHost, TLinkStore* linkStore);

    QString getVersion() override;

    virtual void sendToServer(QString& str);

    void setLinkMode(bool val) override { mLinkMode = val; }

    bool isInLinkMode() { return mLinkMode; }

    QList<QColor> fgColors, bgColors;
    void pushColor(const QString& fgColor, const QString& bgColor) override;

    void popColor() override;

    void pushColor(QList<QColor>& stack, const QString& color);

    void popColor(QList<QColor>& stack);

    bool hasFgColor() const { return !fgColors.isEmpty(); }
    const QColor& getFgColor() { return fgColors.last(); }

    bool hasBgColor() const { return !bgColors.isEmpty(); }

    const QColor& getBgColor() { return bgColors.last(); }

    static QColor mapColor(const QString& colorName);

    // TODO: implement support for fonts?
    void pushFont(const QString& fontFace, const QString& fontSize) override {}
    void popFont() override {}

    int setLink(const QStringList& links, const QStringList& hints) override;

    bool getLink(int id, QStringList** links, QStringList** hints) override;

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

    TMxpTagHandlerResult tagHandled(MxpTag* tag, TMxpTagHandlerResult result) override;

    void raiseMxpEvent(MxpStartTag* tag);
};

#endif //MUDLET_SRC_TMXPMUDLET_H
