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
#include "MudletApp.h"
#include "TConsoleModel.h"

#include <QFile>
#include <QPointer>

#include <utility>

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

void THeadlessSession::connectLinkRelays(Host* pHost, const unsigned int run)
{
    mChangedLinks.clear();
    mChangedLinkIds.clear();
    mLinksChangedPending = false;
    mDeliveredTimeStampsShown.reset();
    connect(
            &pHost->mainConsoleModel().mNotifier,
            &TConsoleModelNotifier::timeStampsToggled,
            this,
            [this, run]() {
                // A shutdown() runs the event loop as the profiles close
                if (mShuttingDown || run != mRun || !mpHost || !mDeliveredTimeStampsShown) {
                    return;
                }
                const bool shown = timeStampsShown();
                if (shown == *mDeliveredTimeStampsShown) {
                    return;
                }
                mDeliveredTimeStampsShown = shown;
                const QString profile = mProfileName;
                emit timeStampsShownChanged(profile, shown);
            },
            Qt::QueuedConnection);
    connect(&pHost->mainConsoleModel().mNotifier, &TConsoleModelNotifier::linkChanged, this, [this, run](const int linkIndex) {
        if (run != mRun) {
            return;
        }
        if (!mChangedLinkIds.contains(linkIndex)) {
            mChangedLinkIds.insert(linkIndex);
            mChangedLinks.append(linkIndex);
        }
        if (!mLinksChangedPending) {
            mLinksChangedPending = true;
            QMetaObject::invokeMethod(
                    this,
                    [this, run]() {
                        sendLinksChanged(run);
                    },
                    Qt::QueuedConnection);
        }
    });
}

void THeadlessSession::sendLinksChanged(const unsigned int run)
{
    // A later run gathers its own
    if (run != mRun) {
        return;
    }
    mLinksChangedPending = false;
    mChangedLinkIds.clear();
    const QList<int> links = std::exchange(mChangedLinks, {});
    // A shutdown() runs the event loop as the profiles close
    if (mShuttingDown || !mpHost || links.isEmpty()) {
        return;
    }
    const QString profile = mProfileName;
    emit linksChanged(profile, links);
}

bool THeadlessSession::timeStampsShown() const
{
    return mpHost && mpHost->mainConsoleModel().mShowTimeStamps;
}

bool THeadlessSession::setTimeStampsShown(const bool shown)
{
    if (!usable()) {
        return false;
    }
    mpHost->setWindowTimeStamps(qsl("main"), shown);
    // Host only warns where the choice could not be saved; the next load reads whether this exists
    const QString saved = MudletApp::getMudletPath(enums::profileDataItemPath, mpHost->getName(), qsl("autotimestamp"));
    if (QFile::exists(saved) != shown) {
        //: Error from a frontend with no window that showed or hid the main console's timestamps, as the GUI's timestamp button does, but could not save that for the profile's next load
        mErrorString = tr("The timestamps were shown or hidden, but that could not be saved for the profile's next load.");
        return false;
    }
    return true;
}
