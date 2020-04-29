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

#ifndef MUDLET_TMXPCLIENT_H
#define MUDLET_TMXPCLIENT_H

#include "MxpTag.h"
#include "TMxpContext.h"
#include "TMxpTagHandlerResult.h"

class TMxpClient
{
protected:
    TMxpContext* mpContext;

public:
    TMxpClient() : mpContext(nullptr) {}

    virtual void initialize(TMxpContext* context) { mpContext = context; }

    virtual QString getVersion() = 0;

    virtual void sendToServer(QString& str) = 0;

    virtual void setLinkMode(bool val) = 0;

    virtual void setFlag(const QString& elementName, const QMap<QString, QString>& params, const QString& content) = 0;

    virtual void publishEntity(const QString& name, const QString& value) = 0;

    virtual void setVariable(const QString& name, const QString& value) = 0;

    virtual void pushColor(const QString& fgColor, const QString& bgColor) = 0;
    virtual void popColor() = 0;

    virtual void pushFont(const QString& fontFace, const QString& fontSize) = 0;
    virtual void popFont() = 0;

    virtual void setBold(bool val) = 0;
    virtual void setItalic(bool val) = 0;
    virtual void setUnderline(bool val) = 0;

    virtual int setLink(const QStringList& hrefs, const QStringList& hints) = 0;
    virtual bool getLink(int id, QStringList** hrefs, QStringList** hints) = 0;

    virtual bool tagReceived(MxpTag* tag) { return tag->isStartTag() ? startTagReceived(tag->asStartTag()) : endTagReceived(tag->asEndTag()); }

    virtual bool startTagReceived(MxpStartTag* startTag) { return true; }

    virtual bool endTagReceived(MxpEndTag* startTag) { return true; }

    virtual TMxpTagHandlerResult tagHandled(MxpTag* tag, TMxpTagHandlerResult result) { return result; }
};

#endif //MUDLET_TMXPCLIENT_H
