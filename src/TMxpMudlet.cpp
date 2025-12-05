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
#include "TMedia.h"
#include "TConsole.h"
#include "TLinkStore.h"

#include <QSet>
#include <QStack>

static const QString PLACEHOLDER_TEXT = QLatin1String("&text;");


QString TMxpMudlet::getVersion()
{
    return mudlet::self()->scmVersion;
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
    return getLinkStore().addLinks(links, hints, mpHost);
}

int TMxpMudlet::setLink(const QStringList& links, const QStringList& hints, const QString& expireName)
{
    return getLinkStore().addLinks(links, hints, mpHost, QVector<int>(), expireName);
}

void TMxpMudlet::expireLinks(const QString& expireName)
{
    getLinkStore().expireLinks(expireName, mpHost);
}

bool TMxpMudlet::getLink(int id, QStringList** links, QStringList** hints)
{
    *links = &getLinkStore().getLinks(id);
    *hints = &getLinkStore().getHints(id);

    return true;
}

void TMxpMudlet::playMedia(TMediaData& mediaData)
{
    mpHost->mpMedia->playMedia(mediaData);
}

void TMxpMudlet::stopMedia(TMediaData& mediaData)
{
    mpHost->mpMedia->stopMedia(mediaData);
}

TMxpTagHandlerResult TMxpMudlet::tagHandled(MxpTag* tag, TMxpTagHandlerResult result)
{
    if (tag->isStartTag()) {
        if (mpContext->getElementRegistry().containsElement(tag->getName())) {
            enqueueMxpEvent(tag->asStartTag());
        } else if (tag->isNamed("SEND")) {
            // send events are queued on closing tag so the caption is available
            TMxpEvent event;
            event.name = tag->getName();
            auto* startTag = tag->asStartTag();
            for (const auto& attrName : startTag->getAttributesNames()) {
                event.attrs[attrName] = startTag->getAttributeValue(attrName);
            }
            event.actions = getLinkStore().getCurrentLinks();
            event.caption.clear();
            mPendingSendEvents.push(event);
        }
    }

    return result;
}

void TMxpMudlet::enqueueMxpEvent(MxpStartTag* tag)
{
    TMxpEvent mxpEvent;
    mxpEvent.name = tag->getName();
    for (const auto& attrName : tag->getAttributesNames()) {
        mxpEvent.attrs[attrName] = tag->getAttributeValue(attrName);
    }
    mxpEvent.actions = getLinkStore().getCurrentLinks();
    mxpEvent.caption.clear();
    mMxpEvents.enqueue(mxpEvent);
}

void TMxpMudlet::setCaptionForSendEvent(const QString& caption)
{
    if (!mPendingSendEvents.isEmpty()) {
        TMxpEvent event = mPendingSendEvents.pop();
        event.caption = caption;
        for (QString& act : event.actions) {
            act.replace(PLACEHOLDER_TEXT, caption, Qt::CaseInsensitive);
        }
        for (auto it = event.attrs.begin(); it != event.attrs.end(); ++it) {
            it.value().replace(PLACEHOLDER_TEXT, caption, Qt::CaseInsensitive);
        }
        mMxpEvents.enqueue(event);
    }
}

TLinkStore& TMxpMudlet::getLinkStore()
{
    return mpHost->mpConsole->getLinkStore();
}

// Handle 'stacks' of attribute settings:
void TMxpMudlet::setBold(bool bold)
{
    if (bold) {
        boldCounter++;
    } else if (boldCounter > 0) {
        boldCounter--;
    }
}

void TMxpMudlet::setItalic(bool italic)
{
    if (italic) {
        italicCounter++;
    } else if (italicCounter > 0) {
        italicCounter--;
    }
}

void TMxpMudlet::setUnderline(bool underline)
{
    if (underline) {
        underlineCounter++;
    } else if (underlineCounter > 0) {
        underlineCounter--;
    }
}

void TMxpMudlet::setStrikeOut(bool strikeOut)
{
    if (strikeOut) {
        strikeOutCounter++;
    } else if (strikeOutCounter > 0) {
        strikeOutCounter--;
    }
}

// reset text Properties (from open tags) at end of line
void TMxpMudlet::resetTextProperties()
{
    boldCounter = 0;
    italicCounter = 0;
    underlineCounter = 0;
    strikeOutCounter = 0;

    // for the next two, we can assume both lists are usually empty (in case of properly
    // balanced MXP tags), and if not, they'll only contain very few entries. Thus it seems
    // sensible to check first, and then just remove all of the few nodes rather than
    // removing the whole list first and then create a new one from scratch.
    while (!fgColors.isEmpty()) {
        fgColors.pop_back();
    }

    while (!bgColors.isEmpty()) {
        bgColors.pop_back();
    }
}

bool TMxpMudlet::startTagReceived(MxpStartTag* startTag)
{
    // Get the current MXP mode from the processor
    TMXPMode currentMode = mpHost->mMxpProcessor.mode();
    
    // In LOCKED mode, no tags are allowed
    if (currentMode == MXP_MODE_LOCKED) {
        return false;
    }
    
    // Check if this tag is allowed in the current mode
    const QString tagName = startTag->getName().toUpper();

    if (!isTagAllowedInMode(tagName, currentMode)) {
        return false;
    }
    
    return true;
}

bool TMxpMudlet::isTagAllowedInMode(const QString& tagName, TMXPMode mode) const
{
    // In SECURE or TEMP_SECURE mode, all tags are allowed
    if (mode == MXP_MODE_SECURE || mode == MXP_MODE_TEMP_SECURE) {
        return true;
    }
    
    // In LOCKED mode, no tags are allowed (handled in startTagReceived)
    if (mode == MXP_MODE_LOCKED) {
        return false;
    }
    
    // In OPEN mode, only specific formatting tags are allowed
    // According to MXP spec: https://www.zuggsoft.com/zmud/mxp.htm
    // OPEN tags are: B, I, U, S, C (color), H (high), FONT, NOBR, P, BR, SBR
    // Plus their variations: BOLD, ITALIC, UNDERLINE, STRIKEOUT, EM, STRONG, HIGH
    static const QSet<QString> openModeTags = {
        qsl("B"), qsl("BOLD"), qsl("STRONG"),
        qsl("I"), qsl("ITALIC"), qsl("EM"),
        qsl("U"), qsl("UNDERLINE"),
        qsl("S"), qsl("STRIKEOUT"),
        qsl("C"), qsl("COLOR"),
        qsl("H"), qsl("HIGH"),
        qsl("FONT"),
        qsl("NOBR"),
        qsl("P"),
        qsl("BR"), qsl("SBR")
    };
    
    return openModeTags.contains(tagName);
}

QByteArray TMxpMudlet::getEncoding() const
{
    return mpHost->mTelnet.getEncoding();
}

int TMxpMudlet::getWrapWidth() const
{
    // Return the host's configured wrap width, with a sensible minimum
    return qMax(mpHost->mWrapAt, 40);
}

void TMxpMudlet::insertText(const QString& text)
{
    // Insert text by feeding it back through the MXP processing pipeline
    // This ensures it respects the current line buffer state
    if (mpHost && mpHost->mpConsole) {
        std::string textToInsert = text.toStdString();
        mpHost->mpConsole->buffer.translateToPlainText(textToInsert, false);
    }
}

bool TMxpMudlet::shouldLockModeToSecure() const
{
    return mpHost && mpHost->getForceMXPProcessorOn();
}
