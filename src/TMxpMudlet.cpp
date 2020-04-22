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
#include "TLinkStore.h"


TMxpMudlet::TMxpMudlet(Host* pHost, TLinkStore* linkStore) : mpHost(pHost), mpLinkStore(linkStore), mLinkMode(false) {}

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
        stack.push_back(mapColor(color));
    }
}
void TMxpMudlet::popColor(QList<QColor>& stack)
{
    if (!stack.isEmpty()) {
        stack.pop_back();
    }
}
QColor TMxpMudlet::mapColor(const QString& colorName)
{
    if (colorName.startsWith("#")) {
        return QColor::fromRgb(QRgb(colorName.mid(1).toInt(nullptr, 16)));
    } else {
        return QColor(colorName);
    }
}
int TMxpMudlet::setLink(const QStringList& links, const QStringList& hints)
{
    return mpLinkStore->addLinks(links, hints);
}
bool TMxpMudlet::getLink(int id, QStringList** links, QStringList** hints)
{
    *links = &mpLinkStore->getLinks(id);
    *hints = &mpLinkStore->getHints(id);

    return true;
}
TMxpTagHandlerResult TMxpMudlet::tagHandled(MxpTag* tag, TMxpTagHandlerResult result)
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
void TMxpMudlet::raiseMxpEvent(MxpStartTag* tag)
{
    TMxpEvent ev;
    ev.name = tag->getName();
    for (const auto& attrName : tag->getAttrsNames()) {
        ev.attrs[attrName] = tag->getAttrValue(attrName);
    }
    ev.actions = mpLinkStore->getCurrentLinks();
    mMxpEvents.enqueue(ev);
}
