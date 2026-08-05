/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include "TDebugFilterBar.h"

#include "TConsole.h"
#include "mudlet.h"

#include <QAction>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QStyle>
#include <QTimer>
#include <QToolButton>

using namespace std::chrono_literals;

// Kept in the order they should appear in the menu, which is roughly "most
// people want this" first:
static const QList<TDebug::Category> csmCategoryOrder = {TDebug::Category::Error,
                                                         TDebug::Category::TriggerMatch,
                                                         TDebug::Category::TriggerDetail,
                                                         TDebug::Category::Alias,
                                                         TDebug::Category::Item,
                                                         TDebug::Category::GameLine,
                                                         TDebug::Category::LuaSuccess,
                                                         TDebug::Category::LuaWarning,
                                                         TDebug::Category::Selection,
                                                         TDebug::Category::Protocol,
                                                         TDebug::Category::Network,
                                                         TDebug::Category::Map,
                                                         TDebug::Category::System,
                                                         TDebug::Category::Other};

static QString categoryName(const TDebug::Category category)
{
    switch (category) {
    case TDebug::Category::System:
        //: Central Debug Console filter: profile started/ended notices
        return TDebugFilterBar::tr("Profile start and end");
    case TDebug::Category::Error:
        //: Central Debug Console filter: compile and run-time errors
        return TDebugFilterBar::tr("Errors");
    case TDebug::Category::Network:
        //: Central Debug Console filter: connecting to the game, downloads
        return TDebugFilterBar::tr("Connection and downloads");
    case TDebug::Category::Protocol:
        //: Central Debug Console filter: GMCP, MSDP, MSSP and MXP events
        return TDebugFilterBar::tr("Protocol events (GMCP, MSDP, MSSP, MXP)");
    case TDebug::Category::GameLine:
        //: Central Debug Console filter: every line the game sends
        return TDebugFilterBar::tr("Every line from the game");
    case TDebug::Category::TriggerMatch:
        //: Central Debug Console filter: which triggers matched
        return TDebugFilterBar::tr("Triggers that matched");
    case TDebug::Category::TriggerDetail:
        //: Central Debug Console filter: capture groups and multiline trigger progress
        return TDebugFilterBar::tr("Trigger capture groups and match state");
    case TDebug::Category::Alias:
        //: Central Debug Console filter: alias matches
        return TDebugFilterBar::tr("Aliases");
    case TDebug::Category::Item:
        //: Central Debug Console filter: housekeeping notices about triggers, timers and the like
        return TDebugFilterBar::tr("Item housekeeping");
    case TDebug::Category::LuaSuccess:
        //: Central Debug Console filter: "ran without errors" notices
        return TDebugFilterBar::tr("Scripts that ran without errors");
    case TDebug::Category::LuaWarning:
        //: Central Debug Console filter: warnings from Lua functions
        return TDebugFilterBar::tr("Lua warnings");
    case TDebug::Category::Selection:
        //: Central Debug Console filter: selectString() and friends
        return TDebugFilterBar::tr("Text selection calls");
    case TDebug::Category::Map:
        //: Central Debug Console filter: mapper callbacks
        return TDebugFilterBar::tr("Mapper");
    case TDebug::Category::Other:
        //: Central Debug Console filter: messages not belonging to any of the other groups, such as a script changing a setting
        return TDebugFilterBar::tr("Other messages");
    }
    return QString();
}

TDebugFilterBar::TDebugFilterBar(QWidget* parent)
: QToolBar(parent)
{
    //: Title of the toolbar holding the Central Debug Console's filter controls
    setWindowTitle(tr("Debug filters"));
    setObjectName(qsl("debugFilterBar"));
    setMovable(false);
    setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    //: Button in the Central Debug Console that stops new messages appearing
    mpActionPause = addAction(style()->standardIcon(QStyle::SP_MediaPause), tr("Pause"));
    mpActionPause->setCheckable(true);
    //: Tooltip for the Central Debug Console's Pause button
    mpActionPause->setToolTip(utils::richText(tr("Hold back new messages so the console stays still. They are shown when you resume.")));
    connect(mpActionPause, &QAction::toggled, this, &TDebugFilterBar::slot_togglePause);

    //: Button in the Central Debug Console that empties it
    auto* pActionClear = addAction(style()->standardIcon(QStyle::SP_DialogResetButton), tr("Clear"));
    //: Tooltip for the Central Debug Console's Clear button
    pActionClear->setToolTip(utils::richText(tr("Empty the console.")));
    connect(pActionClear, &QAction::triggered, this, &TDebugFilterBar::slot_clear);

    addSeparator();
    addCategoryMenu();
    addProfileMenu();
    addTextFilter();

    mpPausedLabel = new QLabel(this);
    mpPausedLabel->setContentsMargins(6, 0, 6, 0);
    mpActionPausedLabel = addWidget(mpPausedLabel);
    mpActionPausedLabel->setVisible(false);

    mpPausedLabelTimer = new QTimer(this);
    mpPausedLabelTimer->setInterval(500ms);
    connect(mpPausedLabelTimer, &QTimer::timeout, this, &TDebugFilterBar::slot_updatePausedCount);
}

void TDebugFilterBar::addCategoryMenu()
{
    auto* pButton = new QToolButton(this);
    pButton->setIcon(QIcon(qsl(":/icons/view-filter.png")));
    //: Menu button in the Central Debug Console for picking which kinds of message it shows
    pButton->setText(tr("Show"));
    pButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    pButton->setPopupMode(QToolButton::InstantPopup);
    //: Tooltip for the Central Debug Console's category menu
    pButton->setToolTip(utils::richText(tr("Choose which kinds of message the console shows from now on.")));

    mpCategoryMenu = new QMenu(pButton);
    for (const auto category : csmCategoryOrder) {
        auto* pAction = mpCategoryMenu->addAction(categoryName(category));
        pAction->setCheckable(true);
        pAction->setChecked(TDebug::categoryEnabled(category));
        connect(pAction, &QAction::toggled, this, [this, category](const bool checked) {
            TDebug::setCategoryEnabled(category, checked);
            mudlet::self()->writeSettings();
        });
        mCategoryActions.insert(category, pAction);
    }

    mpCategoryMenu->addSeparator();
    //: Central Debug Console filter preset that turns every category on
    connect(mpCategoryMenu->addAction(tr("Show everything")), &QAction::triggered, this, [this]() {
        TDebug::setEnabledCategories(TDebug::csmAllCategories);
        applyCategoryFromMenu();
    });
    //: Central Debug Console filter preset that turns off the categories which flood the console
    connect(mpCategoryMenu->addAction(tr("Quiet (hide the noisy ones)")), &QAction::triggered, this, [this]() {
        TDebug::setEnabledCategories(TDebug::csmAllCategories & ~TDebug::csmNoisyCategories);
        applyCategoryFromMenu();
    });

    pButton->setMenu(mpCategoryMenu);
    addWidget(pButton);
}

// Pushes the filter state back into the menu's tick boxes after a preset has
// changed it wholesale.
void TDebugFilterBar::applyCategoryFromMenu()
{
    for (auto it = mCategoryActions.cbegin(); it != mCategoryActions.cend(); ++it) {
        const QSignalBlocker blocker(it.value());
        it.value()->setChecked(TDebug::categoryEnabled(it.key()));
    }
    mudlet::self()->writeSettings();
}

void TDebugFilterBar::addProfileMenu()
{
    mpProfileButton = new QToolButton(this);
    //: Menu button in the Central Debug Console for picking which profiles it shows messages from
    mpProfileButton->setText(tr("Profiles"));
    mpProfileButton->setPopupMode(QToolButton::InstantPopup);
    //: Tooltip for the Central Debug Console's profile menu
    mpProfileButton->setToolTip(utils::richText(tr("Choose which profiles the console shows messages from.")));

    mpProfileMenu = new QMenu(mpProfileButton);
    mpProfileButton->setMenu(mpProfileMenu);
    mpActionProfiles = addWidget(mpProfileButton);
    refreshProfiles();
}

// The profile list changes as profiles are opened and closed, and only carries
// any meaning once there is more than one of them.
void TDebugFilterBar::refreshProfiles()
{
    if (!mpProfileMenu || !mpActionProfiles) {
        return;
    }

    const auto profiles = TDebug::activeProfiles();
    if (profiles.count() <= 1) {
        // The menu is about to be hidden, so anything muted through it would be
        // stuck that way - and with a single profile there is nothing to tell
        // apart in the first place:
        TDebug::enableAllHosts();
    }

    mpProfileMenu->clear();
    for (const auto& profile : profiles) {
        auto* pAction = mpProfileMenu->addAction(profile.second);
        pAction->setCheckable(true);
        pAction->setChecked(TDebug::hostEnabled(profile.first));
        const Host* pHost = profile.first;
        connect(pAction, &QAction::toggled, this, [pHost](const bool checked) {
            TDebug::setHostEnabled(pHost, checked);
        });
    }

    // With a single profile there is nothing to tell apart, which is also why
    // TDebug leaves the "[A] " marking off its messages in that case:
    mpActionProfiles->setVisible(profiles.count() > 1);
}

void TDebugFilterBar::addTextFilter()
{
    mpTextFilter = new QLineEdit(this);
    mpTextFilter->setClearButtonEnabled(true);
    //: Placeholder in the Central Debug Console's text filter box
    mpTextFilter->setPlaceholderText(tr("Show only lines containing..."));
    mpTextFilter->setMaximumWidth(300);
    mpTextFilter->setText(TDebug::textFilter());
    connect(mpTextFilter, &QLineEdit::textChanged, this, &TDebugFilterBar::slot_textFilterChanged);
    addWidget(mpTextFilter);

    //: Very short label on the Central Debug Console's case-sensitivity toggle, next to its text filter box. Keep it to a couple of characters.
    auto* pActionCaseSensitive = addAction(tr("Aa"));
    pActionCaseSensitive->setCheckable(true);
    pActionCaseSensitive->setChecked(TDebug::textFilterCaseSensitivity() == Qt::CaseSensitive);
    //: Tooltip for the Central Debug Console's case-sensitivity toggle
    pActionCaseSensitive->setToolTip(utils::richText(tr("Match the text filter's upper and lower case exactly.")));
    connect(pActionCaseSensitive, &QAction::toggled, this, &TDebugFilterBar::slot_caseSensitivityChanged);
}

void TDebugFilterBar::slot_togglePause(const bool paused)
{
    TDebug::setPaused(paused);
    mpActionPause->setIcon(style()->standardIcon(paused ? QStyle::SP_MediaPlay : QStyle::SP_MediaPause));
    //: Button in the Central Debug Console that lets held-back messages through again
    mpActionPause->setText(paused ? tr("Resume") : tr("Pause"));
    mpActionPausedLabel->setVisible(paused);
    if (paused) {
        slot_updatePausedCount();
        mpPausedLabelTimer->start();
    } else {
        mpPausedLabelTimer->stop();
    }
}

void TDebugFilterBar::slot_updatePausedCount()
{
    //: Shown in the Central Debug Console's toolbar while it is paused
    QString text = tr("%n message(s) held", "", TDebug::pausedMessageCount());
    if (const int dropped = TDebug::pausedDroppedCount(); dropped) {
        // Once the cap is reached the count stops climbing, so say what is
        // happening rather than letting it look stuck:
        //: Appended to the "N messages held" label once the Central Debug Console has been paused long enough to start discarding the oldest ones
        text.append(tr(", %n dropped", "", dropped));
    }
    mpPausedLabel->setText(text);
}

void TDebugFilterBar::slot_clear()
{
    if (mudlet::smpDebugConsole) {
        mudlet::smpDebugConsole->clear();
    }
    TDebug::discardPausedMessages();
    if (TDebug::paused()) {
        slot_updatePausedCount();
    }
}

void TDebugFilterBar::slot_textFilterChanged(const QString& text)
{
    TDebug::setTextFilter(text, TDebug::textFilterCaseSensitivity());
}

void TDebugFilterBar::slot_caseSensitivityChanged(const bool caseSensitive)
{
    TDebug::setTextFilter(TDebug::textFilter(), caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
}
