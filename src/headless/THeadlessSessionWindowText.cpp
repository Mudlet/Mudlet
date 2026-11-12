/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

// The text of the consoles a profile's scripts write to, for an app to draw as the GUI would

#include "THeadlessSession.h"

#include "Host.h"
#include "TConsoleModel.h"
#include "TNullConsoleFrontend.h"

#include <algorithm>

TConsoleModel* THeadlessSession::windowTextModel(const QString& window) const
{
    if (!mpHost) {
        return nullptr;
    }
    if (window.isEmpty() || window == qsl("main")) {
        return &mpHost->mainConsoleModel();
    }
    const auto* pView = dynamic_cast<TNullConsoleFrontend*>(mpHost->consoleFrontend());
    return pView ? pView->listedConsoleModel(window) : nullptr;
}

std::optional<THeadlessSession::WindowText> THeadlessSession::windowLines(const QString& window, const int first, const int count) const
{
    const TConsoleModel* pModel = windowTextModel(window);
    if (!pModel) {
        return std::nullopt;
    }
    const TBuffer& buffer = pModel->buffer;
    const bool main = pModel == &mpHost->mainConsoleModel();
    const QColor foreground = main ? mpHost->mFgColor : pModel->mFgColor;
    const QColor background = main ? mpHost->mBgColor : pModel->mBgColor;
    WindowText text;
    text.lineCount = static_cast<int>(buffer.lineBuffer.size());
    text.first = std::clamp(first < 0 ? text.lineCount + first : first, 0, text.lineCount);
    const int end = text.first + std::clamp(count, 0, text.lineCount - text.first);
    qint64 time = -1;
    for (int y = text.first; y < end; ++y) {
        const QString& line = buffer.lineBuffer.at(y);
        // Carried down a wrapped line's rows, as arrivalTime() walks back to its first row for each
        if (y == text.first || y >= buffer.timeBuffer.size() || buffer.timeBuffer.at(y) != TBuffer::csmWrappedLineTimeStamp) {
            time = arrivalTime(buffer, y);
        }
        text.lines.append({line, buffer.promptBuffer.at(y), textRuns(line, buffer.buffer.at(y), foreground, background), time, buffer.wrapsFromPreviousLine(y)});
    }
    return text;
}
