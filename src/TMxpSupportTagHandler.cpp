/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2018 by Stephen Lyons - slysven@virginmedia.com    *
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


#include "TMxpSupportTagHandler.h"
#include "TMxpClient.h"
#include "TMxpContext.h"

TMxpTagHandlerResult TMxpSupportTagHandler::handleStartTag(TMxpContext& ctx, TMxpClient& client, MxpStartTag* tag)
{
    const QString& supportOptions = processSupportsRequest(ctx, tag);

    QString payload = QStringLiteral("\n\x1b[1z<SUPPORTS %1>\n").arg(supportOptions);
    client.sendToServer(payload);

    return MXP_TAG_HANDLED;
}
QString TMxpSupportTagHandler::processSupportsRequest(TMxpContext& ctx, MxpStartTag* tag)
{
    const auto& supportedMxpElements = ctx.getSupportedElements();
    // strip initial SUPPORT and tokenize all of the requested elements
    QStringList result;

    auto reportEntireElement = [](auto element, auto& result, auto& mSupportedMxpElements) {
        result.append("+" + element);

        for (const auto& attribute : mSupportedMxpElements.value(element)) {
            result.append("+" + element + QStringLiteral(".") + attribute);
        }

        return result;
    };

    auto reportAllElements = [reportEntireElement](auto& result, auto& mSupportedMxpElements) {
        auto elementsIterator = mSupportedMxpElements.constBegin();
        while (elementsIterator != mSupportedMxpElements.constEnd()) {
            result = reportEntireElement(elementsIterator.key(), result, mSupportedMxpElements);
            ++elementsIterator;
        }

        return result;
    };

    // empty <SUPPORT> - report all known elements
    if (tag->getAttributesCount() == 0) {
        result = reportAllElements(result, supportedMxpElements);
    } else {
        // otherwise it's <SUPPORT element1 element2 element3>
        for (auto& element : tag->getAttributesNames()) {
            if (!element.contains(QChar('.'))) {
                if (supportedMxpElements.contains(element)) {
                    result = reportEntireElement(element, result, supportedMxpElements);
                } else {
                    result.append("-" + element);
                }
            } else {
                auto elementName = element.section(QChar('.'), 0, 0);
                auto attributeName = element.section(QChar('.'), 1, 1);

                if (!supportedMxpElements.contains(elementName)) {
                    result.append("-" + element);
                } else if (attributeName == QLatin1String("*")) {
                    result = reportEntireElement(elementName, result, supportedMxpElements);
                } else {
                    if (supportedMxpElements.value(elementName).contains(attributeName)) {
                        result.append("+" + elementName + "." + attributeName);
                    } else {
                        result.append("-" + elementName + "." + attributeName);
                    }
                }
            }
        }
    }

    return result.join(QLatin1String(" "));
}
