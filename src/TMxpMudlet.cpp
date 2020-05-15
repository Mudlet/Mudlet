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

#include "TMxpMudlet.h"
#include "Host.h"
#include "TConsole.h"
#include "TLinkStore.h"


TMxpMudlet::TMxpMudlet(Host* pHost) : mpHost(pHost), mLinkMode(false), isBold(false), isUnderline(false), isItalic(false) {}

QString TMxpMudlet::getVersion()
{
    return scmVersion;
}

void TMxpMudlet::sendToServer(QString& str)
{
    mpHost->mTelnet.sendData(str);
}

void TMxpMudlet::pushColor(const QString& fgColor, const QString& bgColor)
{
    pushColor(fgColors, fgColor);
    pushColor(bgColors, bgColor);
}

void TMxpMudlet::popColor()
{
    popColor(fgColors);
    popColor(bgColors);
}

void TMxpMudlet::pushColor(QList<QColor>& stack, const QString& color)
{
    if (color.isEmpty()) {
        if (!stack.isEmpty()) {
            stack.push_back(stack.last());
        }
    } else {
        stack.push_back(QColor(color));
    }
}
void TMxpMudlet::popColor(QList<QColor>& stack)
{
    if (!stack.isEmpty()) {
        stack.pop_back();
    }
}

int TMxpMudlet::setLink(const QStringList& links, const QStringList& hints)
{
    return getLinkStore().addLinks(links, hints);
}

bool TMxpMudlet::getLink(int id, QStringList** links, QStringList** hints)
{
    *links = &getLinkStore().getLinks(id);
    *hints = &getLinkStore().getHints(id);

    return true;
}

TMxpTagHandlerResult TMxpMudlet::tagHandled(MxpTag* tag, TMxpTagHandlerResult result)
{
    if (tag->isStartTag()) {
        if (mpContext->getElementRegistry().containsElement(tag->getName())) {
            enqueueMxpEvent(tag->asStartTag());
        } else if (tag->isNamed("SEND")) {
            enqueueMxpEvent(tag->asStartTag());
        }
    }

    return result;
}

void TMxpMudlet::enqueueMxpEvent(MxpStartTag* tag)
{
    TMxpEvent ev;
    ev.name = tag->getName();
    for (const auto& attrName : tag->getAttributesNames()) {
        ev.attrs[attrName] = tag->getAttributeValue(attrName);
    }
    ev.actions = getLinkStore().getCurrentLinks();
    mMxpEvents.enqueue(ev);
}

TLinkStore& TMxpMudlet::getLinkStore()
{
    return mpHost->mpConsole->getLinkStore();
}
