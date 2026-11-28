/***************************************************************************
 *   Copyright (C) 2026 by Mike Conley - mike.conley@stickmud.com          *
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

#include "TLinkInteraction.h"

#include "TBuffer.h"
#include "TConsoleModel.h"
#include "mudlet.h"

#include <QAccessible>
#include <QCoreApplication>
#include <QLabel>
#include <QMenu>
#include <QWidgetAction>

TLinkInteraction::TLinkInteraction(TBuffer& buffer, TConsoleModel& model, QObject* parent)
: QObject(parent)
, mBuffer(buffer)
, mModel(model)
{
}

TLinkInteraction::Press TLinkInteraction::press(const int linkIndex)
{
    // A note has nothing for a click to do, so the press selects text as it would anywhere else
    if (mBuffer.mLinkStore.getLinks(linkIndex).empty() || mBuffer.linkIsNote(linkIndex)) {
        return Press::text;
    }
    mBuffer.setActiveLink(linkIndex);
    if (mModel.linkMayRun(linkIndex) != TConsoleModel::LinkFollowed::ran) {
        return Press::redraw;
    }

    // Clear hover to show selection immediately; mouse move will restore it
    mBuffer.setHoveredLink(0);

    // Remember this link was just clicked - suppress hover until mouse leaves
    mBuffer.setLastClickedLinkIndex(linkIndex);

    mModel.runLink(linkIndex, 0, false);
    if (mBuffer.linkOpensItsMenu(linkIndex)) {
        // The popup takes the release, which is what would have ended the active state
        mBuffer.setActiveLink(0);
        return Press::menu;
    }
    return Press::ran;
}

TLinkInteraction::RightClick TLinkInteraction::rightClick(const int linkIndex)
{
    // A note gets the menu any other text would
    if (mBuffer.linkIsNote(linkIndex)) {
        return RightClick::text;
    }

    const Mudlet::HyperlinkStyling hyperlinkStyling = mBuffer.getEffectiveHyperlinkStyling(linkIndex);

    // Handle spoiler revealing first (even for disabled links)
    if (hyperlinkStyling.isSpoiler && mBuffer.isSpoilerUnrevealed(linkIndex)) {
        mBuffer.revealSpoilerLink(linkIndex);
        return RightClick::revealed;
    }

    if (hyperlinkStyling.selection.disabled) {
#if defined(DEBUG_OSC_PROCESSING)
        qDebug() << "TLinkInteraction::rightClick - Right-click on disabled link" << linkIndex << ", blocking context menu";
#endif
        return RightClick::blocked;
    }

    return mBuffer.mLinkStore.opensMenu(linkIndex) ? RightClick::menu : RightClick::text;
}

TLinkInteraction::Activate TLinkInteraction::activateFocused()
{
    const int focusedLink = mBuffer.getFocusedLink();
    if (focusedLink <= 0 || mBuffer.linkIsNote(focusedLink)) {
        return Activate::none;
    }
    const TConsoleModel::LinkFollowed mayRun = mModel.linkMayRun(focusedLink);
    if (mayRun == TConsoleModel::LinkFollowed::revealedSpoiler || mayRun == TConsoleModel::LinkFollowed::disabled) {
        return Activate::redraw;
    }

    if (mBuffer.mLinkStore.getLinksConst(focusedLink).isEmpty()) {
        return Activate::ignored;
    }

    mModel.runLink(focusedLink, 0, false);
    return mBuffer.linkOpensItsMenu(focusedLink) ? Activate::menu : Activate::ran;
}

TLinkInteraction::Activate TLinkInteraction::menuKeyOnFocused()
{
    const int focusedLink = mBuffer.getFocusedLink();
    // A note takes no keys, so the Menu key on one opens what it would on any other text
    if (focusedLink <= 0 || mBuffer.linkIsNote(focusedLink)) {
        return Activate::none;
    }

    const Mudlet::HyperlinkStyling hyperlinkStyling = mBuffer.getEffectiveHyperlinkStyling(focusedLink);

    // Handle spoiler revealing first (even for disabled links)
    if (hyperlinkStyling.isSpoiler && mBuffer.isSpoilerUnrevealed(focusedLink)) {
        mBuffer.revealSpoilerLink(focusedLink);
        return Activate::redraw;
    }

    if (hyperlinkStyling.selection.disabled || mBuffer.mLinkStore.getLinksConst(focusedLink).isEmpty()) {
        return Activate::ignored;
    }

    if (!mBuffer.mLinkStore.opensMenu(focusedLink)) {
        // No menu needed: one command and no extra hints means direct activation
        mModel.runLink(focusedLink, 0, false);
        return Activate::ran;
    }
    return Activate::menu;
}

bool TLinkInteraction::hoverAt(const int linkIndex)
{
    if (linkIndex) {
        // Don't set hover state for disabled links - they should stay disabled
        // Also don't set hover if this link was just clicked - wait for mouse to leave first
        if (mBuffer.getHoveredLink() != linkIndex) {
            auto currentState = mBuffer.getLinkState(linkIndex);
            if (currentState != Mudlet::HyperlinkStyling::StateDisabled && linkIndex != mBuffer.getLastClickedLinkIndex()) {
                mBuffer.setHoveredLink(linkIndex);
                return true;
            }
        }
        return false;
    }

    bool changed = false;
    if (mBuffer.getHoveredLink() != 0) {
        mBuffer.setHoveredLink(0);
        changed = true;
    }

    // Clear last clicked link when mouse leaves - allows hover to work again
    if (mBuffer.getLastClickedLinkIndex() != 0) {
        mBuffer.clearLastClickedLinkIndex();
    }
    return changed;
}

bool TLinkInteraction::leave()
{
    const bool changed = mBuffer.getHoveredLink() != 0;
    if (changed) {
        mBuffer.setHoveredLink(0);
    }
    mBuffer.clearLastClickedLinkIndex();
    return changed;
}

Qt::CursorShape TLinkInteraction::cursorFor(const int linkIndex) const
{
    return mBuffer.linkIsNote(linkIndex) ? Qt::IBeamCursor : Qt::PointingHandCursor;
}

QString TLinkInteraction::tooltipMarkup(const int linkIndex) const
{
    // The server chooses this text and QToolTip renders anything
    // Qt::mightBeRichText() accepts as HTML, so escape it and wrap it
    // in an explicit document rather than letting that guess decide
    // whether the markup is live. white-space:pre keeps the tooltip's
    // line breaks, as plain text would.
    // An empty string is how QToolTip is told to hide, so it has to
    // stay empty rather than becoming an empty document.
    const QString tooltipText = mBuffer.mLinkStore.tooltip(linkIndex);
    return tooltipText.isEmpty() ? QString() : qsl("<html><body style='white-space:pre'>%1</body></html>").arg(tooltipText.toHtmlEscaped());
}

QMenu* TLinkInteraction::buildMenu(const int linkIndex, QWidget* parent, const QFont& font, const bool alreadyActivated)
{
    mPopupLinkAlreadyActivated = alreadyActivated;
    const Mudlet::HyperlinkStyling hyperlinkStyling = mBuffer.getEffectiveHyperlinkStyling(linkIndex);
    auto popup = new QMenu(parent);
    popup->setAttribute(Qt::WA_DeleteOnClose);
    popup->setFont(font);

    if (!hyperlinkStyling.menuTitle.isEmpty()) {
        auto titleLabel = new QLabel(hyperlinkStyling.menuTitle, popup);
        // The server picks this text and QLabel defaults to Qt::AutoText,
        // which would render markup in it.
        titleLabel->setTextFormat(Qt::PlainText);
        titleLabel->setFont(font);

        QStringList styleProps;
        styleProps << qsl("padding: 4px 20px");

        if (hyperlinkStyling.menuTitleStyle.hasForegroundColor) {
            styleProps << qsl("color: %1").arg(hyperlinkStyling.menuTitleStyle.foregroundColor.name());
        } else {
            styleProps << qsl("color: #5fbdaf");
        }

        if (hyperlinkStyling.menuTitleStyle.hasBackgroundColor) {
            styleProps << qsl("background-color: %1").arg(hyperlinkStyling.menuTitleStyle.backgroundColor.name());
        }

        if (hyperlinkStyling.menuTitleStyle.isBold) {
            styleProps << qsl("font-weight: bold");
        }

        if (hyperlinkStyling.menuTitleStyle.isItalic) {
            styleProps << qsl("font-style: italic");
        }

        titleLabel->setStyleSheet(styleProps.join(qsl("; ")));
        auto titleWidgetAction = new QWidgetAction(popup);
        titleWidgetAction->setDefaultWidget(titleLabel);
        popup->addAction(titleWidgetAction);
        popup->addSeparator();
    }

    mPopupLinkIndex = linkIndex;
    mPopupLinkIssue = mBuffer.mLinkStore.issueOf(linkIndex);
    const QList<TLinkStore::MenuItem> items = mBuffer.mLinkStore.menuItems(linkIndex);
    for (int i = 0, total = items.size(); i < total; ++i) {
        QAction* pA = items.at(i).separator ? popup->addSeparator() : popup->addAction(items.at(i).label);
        pA->setData(i + 1);
        if (items.at(i).enabled) {
            connect(pA, &QAction::triggered, this, [this, item = i + 1]() {
                runPick(item);
            });
        } else {
            pA->setEnabled(false);
        }
    }

    return popup;
}

void TLinkInteraction::runPick(const int item)
{
    if (item && mPopupLinkIndex > 0) {
        emit aboutToRunMenuPick();
        mModel.runMenuPick(mPopupLinkIndex, mPopupLinkIssue, item - 1, mPopupLinkAlreadyActivated);
        emit menuPickRan();
    }
    mPopupLinkIndex = 0;
}

void TLinkInteraction::announceFocus(const int linkIndex)
{
    if (!QAccessible::isActive() || linkIndex <= 0) {
        return;
    }

    if (auto* pMudlet = mudlet::self()) {
        pMudlet->announce(announcement(linkIndex), QString(), true);
    }
}

// Still in the "TTextEdit" translation context, where the existing translations of these strings live
QString TLinkInteraction::announcement(const int linkIndex) const
{
    const QString tooltip = mBuffer.getLinkTooltip(linkIndex);
    const QStringList commands = mBuffer.mLinkStore.getLinksConst(linkIndex);
    const Mudlet::HyperlinkStyling styling = mBuffer.getEffectiveHyperlinkStyling(linkIndex);
    const bool selected = mBuffer.isLinkSelected(linkIndex);

    // A link is announced by what it does, which its appearance need not show
    QString role;
    if (mBuffer.linkIsNote(linkIndex)) {
        //: Screen reader role for marked-up text from the game that does nothing when activated, such as text that only carries a tooltip
        role = QCoreApplication::translate("TTextEdit", "note");
    } else if (styling.isSpoiler && mBuffer.isSpoilerUnrevealed(linkIndex)) {
        //: Screen reader role for a game's hidden text that activating the link reveals
        role = QCoreApplication::translate("TTextEdit", "button");
    } else if (styling.selection.hasSelectionSettings && styling.selection.exclusive) {
        if (selected) {
            //: Screen reader role for the selected link of a group where only one can be selected
            role = QCoreApplication::translate("TTextEdit", "radio button, checked");
        } else {
            //: Screen reader role for an unselected link of a group where only one can be selected
            role = QCoreApplication::translate("TTextEdit", "radio button, not checked");
        }
    } else if (styling.selection.hasSelectionSettings) {
        if (selected) {
            //: Screen reader role for a selected link that can be selected independently of any others
            role = QCoreApplication::translate("TTextEdit", "checkbox, checked");
        } else {
            //: Screen reader role for an unselected link that can be selected independently of any others
            role = QCoreApplication::translate("TTextEdit", "checkbox, not checked");
        }
    } else {
        //: Screen reader role for an ordinary link, announced after its tooltip or command, or alone when it has neither
        role = QCoreApplication::translate("TTextEdit", "link");
    }

    QString announcement;
    if (!tooltip.isEmpty()) {
        announcement = qsl("%1, %2").arg(tooltip, role);
    } else if (!commands.isEmpty() && !commands.first().isEmpty()) {
        announcement = qsl("%1, %2").arg(commands.first(), role);
    } else {
        announcement = role;
    }

    if (mBuffer.isLinkVisited(linkIndex)) {
        //: Appended to link announcement when the link has been previously visited
        announcement += QCoreApplication::translate("TTextEdit", ", visited");
    }
    if (mBuffer.getLinkState(linkIndex) == Mudlet::HyperlinkStyling::StateDisabled) {
        //: Appended to link announcement when the link is disabled
        announcement += QCoreApplication::translate("TTextEdit", ", disabled");
    }

    if (commands.size() > 1 || mBuffer.mLinkStore.getHintsConst(linkIndex).size() > commands.size()) {
        //: Appended to link announcement when the link opens a menu
        announcement += QCoreApplication::translate("TTextEdit", ", has menu");
    }
    return announcement;
}
