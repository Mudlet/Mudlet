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

#include "ActionUnit.h"
#include "AliasUnit.h"
#include "Host.h"
#include "KeyUnit.h"
#include "ScriptUnit.h"
#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TConsole.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"
#include "TimerUnit.h"
#include "TriggerUnit.h"
#include "mudlet.h"

#include <QAction>
#include <QComboBox>
#include <QCompleter>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QStyle>
#include <QTimer>
#include <QToolButton>

using namespace std::chrono_literals;

// A combo box that rebuilds its list as it is opened, so it always offers the
// triggers, aliases and the rest that the profile has right now rather than
// whatever it had when the console was first shown. Lives here rather than in
// the header so that including TDebugFilterBar.h does not drag QComboBox in.
class TRefreshingComboBox : public QComboBox
{
public:
    explicit TRefreshingComboBox(std::function<void()> refresh, QWidget* parent = nullptr)
    : QComboBox(parent)
    , mRefresh(std::move(refresh))
    {
    }

    void showPopup() override
    {
        if (mRefresh) {
            mRefresh();
        }
        QComboBox::showPopup();
    }

private:
    std::function<void()> mRefresh;
};

// Kept in the order they should appear in the menu. "Every line from the game"
// leads because it is off by default and the one most people come looking for:
static const QList<TDebug::Category> csmCategoryOrder = {TDebug::Category::GameLine,
                                                         TDebug::Category::Error,
                                                         TDebug::Category::TriggerMatch,
                                                         TDebug::Category::TriggerDetail,
                                                         TDebug::Category::Alias,
                                                         TDebug::Category::Item,
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
    addItemFilter();
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
    mpCategoryButton = new QToolButton(this);
    auto* pButton = mpCategoryButton;
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
    //: Central Debug Console filter preset that turns every kind of message on
    connect(mpCategoryMenu->addAction(tr("Show all")), &QAction::triggered, this, [this]() {
        TDebug::setEnabledCategories(TDebug::csmAllCategories);
        applyCategoryFromMenu();
    });
    //: Central Debug Console filter preset that turns every kind of message off
    connect(mpCategoryMenu->addAction(tr("Hide all")), &QAction::triggered, this, [this]() {
        TDebug::setEnabledCategories({});
        applyCategoryFromMenu();
        // Nothing at all will arrive from here on, and there is no longer a
        // running count on the button to explain why - so say it once, now:
        if (mudlet::smpDebugConsole) {
            //: Shown in the Central Debug Console the moment the user hides every kind of message
            mudlet::smpDebugConsole->print(tr("[*] Every kind of message is hidden now - nothing further will appear until you show some again.\n"), Qt::white, Qt::darkBlue);
        }
    });
    //: Central Debug Console filter preset that turns off only the kinds of message which flood it
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

// Picking the trigger you care about by name, from a list of the ones the
// profile actually has - nobody knows their items by ID.
void TDebugFilterBar::addItemFilter()
{
    mpItemFilter = new TRefreshingComboBox(
            [this]() {
                refreshItemList();
            },
            this);
    mpItemFilter->setEditable(true);
    mpItemFilter->setInsertPolicy(QComboBox::NoInsert);
    mpItemFilter->setMinimumWidth(180);
    mpItemFilter->setMaxVisibleItems(25);
    mpItemFilter->completer()->setCompletionMode(QCompleter::PopupCompletion);
    mpItemFilter->completer()->setFilterMode(Qt::MatchContains);
    mpItemFilter->completer()->setCaseSensitivity(Qt::CaseInsensitive);
    //: Tooltip for the Central Debug Console's item filter, which narrows it to one trigger, alias, timer and so on
    mpItemFilter->setToolTip(utils::richText(tr("Show only messages about one trigger, alias, timer, key, button or script. Type to search by name.")));
    refreshItemList();

    // Deliberately NOT currentTextChanged: that fires per keystroke, so typing
    // "Combat" would filter on "C", then "Co", then "Com"... each matching
    // nothing and blanking the console while the user is still typing.
    connect(mpItemFilter, &QComboBox::activated, this, [this](const int index) {
        // Index 0 is the "all items" entry rather than a real name - compared by
        // position so that an item genuinely called "All items" still works:
        TDebug::setItemFilter(index == 0 ? QString() : mpItemFilter->itemText(index));
    });
    connect(mpItemFilter->lineEdit(), &QLineEdit::editingFinished, this, [this]() {
        applyTypedItemFilter();
    });
    addWidget(mpItemFilter);
}

// Takes what was typed into the item box once the user has finished typing it.
// A name that matches nothing would silence the console with no explanation, so
// say so rather than leaving them staring at an empty window.
void TDebugFilterBar::applyTypedItemFilter()
{
    const QString typed = mpItemFilter->currentText().trimmed();
    if (typed.isEmpty() || typed == allItemsLabel()) {
        TDebug::setItemFilter(QString());
        return;
    }

    // findText matches the same way the box's own completer does, so what the
    // completer offered is what gets applied:
    const int index = mpItemFilter->findText(typed, Qt::MatchFixedString);
    if (index > 0) {
        TDebug::setItemFilter(mpItemFilter->itemText(index));
        return;
    }

    TDebug::setItemFilter(typed);
    if (mudlet::smpDebugConsole) {
        //: Shown in the Central Debug Console when the name typed into its item filter matches nothing the profile has. %1 is what was typed.
        mudlet::smpDebugConsole->print(tr("[*] Nothing called \"%1\" was found in this profile, so only its system messages will show.\n").arg(typed), Qt::white, Qt::darkRed);
    }
}

// The first entry of the item filter, meaning "do not filter by item".
/* static */ QString TDebugFilterBar::allItemsLabel()
{
    //: First entry of the Central Debug Console's item filter, meaning no item filter is applied
    return tr("All items");
}

void TDebugFilterBar::refreshItemList()
{
    if (!mpItemFilter) {
        return;
    }

    QStringList names;
    // Only the profile in the foreground: the console is shared by all of them,
    // so an item belonging to another profile has to be typed rather than picked.
    if (auto* pHost = mudlet::self()->getActiveHost(); pHost) {
        // The lookup tables are keyed by name and flat, so there is no tree to
        // walk. They also hold temporary items, which are named after their id -
        // a profile using tempTrigger() would bury the real names under a list
        // of numbers - and groups, which never emit anything of their own.
        for (auto it = pHost->getTriggerUnit()->mLookupTable.cbegin(); it != pHost->getTriggerUnit()->mLookupTable.cend(); ++it) {
            if (it.value() && !it.value()->isTemporary() && !it.value()->isFolder()) {
                names << it.key();
            }
        }
        for (auto it = pHost->getAliasUnit()->mLookupTable.cbegin(); it != pHost->getAliasUnit()->mLookupTable.cend(); ++it) {
            if (it.value() && !it.value()->isTemporary() && !it.value()->isFolder()) {
                names << it.key();
            }
        }
        for (auto it = pHost->getTimerUnit()->mLookupTable.cbegin(); it != pHost->getTimerUnit()->mLookupTable.cend(); ++it) {
            if (it.value() && !it.value()->isTemporary() && !it.value()->isFolder()) {
                names << it.key();
            }
        }
        for (auto it = pHost->getKeyUnit()->mLookupTable.cbegin(); it != pHost->getKeyUnit()->mLookupTable.cend(); ++it) {
            if (it.value() && !it.value()->isTemporary() && !it.value()->isFolder()) {
                names << it.key();
            }
        }
        for (auto* pScript : pHost->getScriptUnit()->getScriptList()) {
            if (pScript && !pScript->isFolder()) {
                names << pScript->getName();
            }
        }
        for (auto* pAction : pHost->getActionUnit()->getActionList()) {
            if (pAction && !pAction->isFolder()) {
                names << pAction->getName();
            }
        }
    }
    // An item with no name at all cannot be told apart from "no item" by the
    // filter, so there is nothing useful to offer for it:
    names.removeAll(QString());
    names.removeDuplicates();
    names.sort(Qt::CaseInsensitive);
    names.prepend(allItemsLabel());

    // Show what the filter actually is, rather than assuming it is unset - the
    // filter is application-wide and outlives any one toolbar:
    const QString wanted = TDebug::itemFilter().isEmpty() ? allItemsLabel() : TDebug::itemFilter();
    const QSignalBlocker blocker(mpItemFilter);
    mpItemFilter->clear();
    mpItemFilter->addItems(names);
    mpItemFilter->setCurrentText(wanted);
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

// Puts the box back in step with the filter after something else changed it,
// such as the console's own right-click menu.
void TDebugFilterBar::refreshTextFilter()
{
    if (!mpTextFilter) {
        return;
    }
    const QSignalBlocker blocker(mpTextFilter);
    mpTextFilter->setText(TDebug::textFilter());
}

void TDebugFilterBar::slot_textFilterChanged(const QString& text)
{
    TDebug::setTextFilter(text, TDebug::textFilterCaseSensitivity());
}

void TDebugFilterBar::slot_caseSensitivityChanged(const bool caseSensitive)
{
    TDebug::setTextFilter(TDebug::textFilter(), caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
}
