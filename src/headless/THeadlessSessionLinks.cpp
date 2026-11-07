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

#include "THeadlessSession.h"

#include "Host.h"
#include "TConsoleModel.h"

#include <QPointer>

std::optional<THeadlessSession::Link> THeadlessSession::link(const int id) const
{
    if (!mpHost || id <= 0) {
        return std::nullopt;
    }
    const TConsoleModel& model = mpHost->mainConsoleModel();
    const TBuffer& buffer = model.buffer;
    const TLinkStore& store = buffer.mLinkStore;
    if (!buffer.linkHeld(id)) {
        return std::nullopt;
    }
    const Mudlet::HyperlinkStyling styling = buffer.getEffectiveHyperlinkStyling(id);
    Link link;
    link.tooltip = store.tooltip(id);
    link.menu = store.opensMenu(id);
    link.menuTitle = styling.menuTitle;
    for (const TLinkStore::MenuItem& item : store.menuItems(id)) {
        link.items.append({item.label, item.enabled, item.separator});
    }
    link.spoiler = styling.isSpoiler;
    link.revealed = styling.isSpoiler && !buffer.isSpoilerUnrevealed(id);
    link.disabled = styling.selection.disabled;
    link.concealed = model.mHyperlinkVisibilityManager.isLinkConcealed(id);
    link.visited = buffer.isLinkVisited(id);
    link.selected = buffer.isLinkSelected(id);
    // As TBuffer::updateLinkCharacters() paints the link's characters
    if (styling.hasCustomStyling) {
        if (styling.hasForegroundColor) {
            link.foreground = styling.foregroundColor;
        }
        if (styling.hasBackgroundColor) {
            link.background = styling.backgroundColor;
        }
        TextStyles styles;
        styles.setFlag(TextStyle::Bold, styling.isBold);
        styles.setFlag(TextStyle::Italic, styling.isItalic);
        styles.setFlag(TextStyle::Underline, styling.isUnderlined);
        styles.setFlag(TextStyle::Overline, styling.isOverlined);
        styles.setFlag(TextStyle::StrikeOut, styling.isStrikeOut);
        link.styles = styles;
    }
    return link;
}

THeadlessSession::LinkResult THeadlessSession::followLink(const int link, const std::optional<int> item)
{
    if (!usable()) {
        return LinkResult::refused;
    }
    TConsoleModel& model = mpHost->mainConsoleModel();
    const TLinkStore& store = model.buffer.mLinkStore;
    if (!model.buffer.linkHeld(link)) {
        return LinkResult::unknown;
    }
    const QList<TLinkStore::MenuItem> items = store.menuItems(link);
    if (item && (*item < 0 || *item >= items.size())) {
        return LinkResult::noSuchItem;
    }
    // Its characters are blanked and carry no link, so no click reaches it
    if (model.mHyperlinkVisibilityManager.isLinkConcealed(link)) {
        return LinkResult::concealed;
    }
    switch (model.linkMayRun(link)) {
    case TConsoleModel::LinkFollowed::ran:
        break;
    case TConsoleModel::LinkFollowed::revealedSpoiler:
        return LinkResult::revealedSpoiler;
    case TConsoleModel::LinkFollowed::disabled:
        return LinkResult::disabled;
    case TConsoleModel::LinkFollowed::unknown:
        return LinkResult::unknown;
    }
    // A left click runs the first item whatever it holds; the GUI's menu greys out an item with nothing to run
    const bool fromMenu = item && store.opensMenu(link);
    if (fromMenu && !items.at(*item).enabled) {
        return LinkResult::disabled;
    }
    QString errorText;
    const auto errorCapture = connect(mpHost, &Host::signal_errorConsolePrint, this, [&errorText](const QString& text) {
        errorText = text;
    });
    const QPointer<THeadlessSession> self(this);
    const bool ran = model.runLink(link, item.value_or(0), fromMenu);
    disconnect(errorCapture);
    if (ran) {
        return LinkResult::ran;
    }
    if (self) {
        mErrorString = luaErrorOf(errorText);
    }
    return LinkResult::failed;
}
