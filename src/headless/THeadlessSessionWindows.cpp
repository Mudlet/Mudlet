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

// The windows a profile's scripts make, for an app to draw as the GUI would

#include "THeadlessSession.h"

#include "Host.h"
#include "TConsoleModel.h"
#include "TLabelModel.h"
#include "TNullConsoleFrontend.h"
#include "TWindowNotifier.h"

#include <QDebug>
#include <QEnterEvent>
#include <QHash>
#include <QMouseEvent>
#include <QPointer>
#include <QWheelEvent>

#include <algorithm>
#include <functional>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

namespace {

using Record = TNullConsoleFrontend::WindowRecord;

THeadlessSession::ScriptWindow::Kind kindOf(const Record::Kind kind)
{
    using Kind = THeadlessSession::ScriptWindow::Kind;
    switch (kind) {
    case Record::Kind::miniConsole:
        return Kind::miniConsole;
    case Record::Kind::userWindow:
        return Kind::userWindow;
    case Record::Kind::label:
        return Kind::label;
    case Record::Kind::commandLine:
        return Kind::commandLine;
    case Record::Kind::textBox:
        return Kind::textBox;
    case Record::Kind::scrollBox:
        return Kind::scrollBox;
    case Record::Kind::mapper:
        return Kind::mapper;
    case Record::Kind::mxpFrame:
        return Kind::mxpFrame;
    }
    Q_UNREACHABLE();
}

THeadlessSession::ScriptWindow scriptWindowOf(const Record& record, const int stackOrder)
{
    THeadlessSession::ScriptWindow window{record.name, kindOf(record.kind), record.parent, record.geometry, record.visible, record.shown, stackOrder, record.background, record.font, record.inDock};
    window.scrolledBackTo = record.scrolledBackTo;
    window.grid = record.grid;
    window.scrollBar = record.scrollBar;
    window.timeStamps = record.timeStamps;
    window.parentKind = kindOf(record.parentKind);
    if (record.kind == Record::Kind::userWindow) {
        window.dock = THeadlessSession::ScriptWindow::Dock{record.autoDock, record.dockArea, record.floating, record.title};
    }
    return window;
}

// Only these hold other windows, so a label that shares a scroll box's name is not taken for it
bool holdsWindows(const Record::Kind kind)
{
    return kind == Record::Kind::userWindow || kind == Record::Kind::scrollBox || kind == Record::Kind::miniConsole || kind == Record::Kind::mxpFrame;
}

// An MXP frame's console is a mini console the game made
bool miniConsole(const THeadlessSession::ScriptWindow::Kind kind)
{
    return kind == THeadlessSession::ScriptWindow::Kind::miniConsole || kind == THeadlessSession::ScriptWindow::Kind::mxpFrame;
}

// A console's text, which a press or the wheel acts on
bool consoleText(const THeadlessSession::ScriptWindow::Kind kind)
{
    return miniConsole(kind) || kind == THeadlessSession::ScriptWindow::Kind::userWindow;
}

QHash<QString, QList<THeadlessSession::ScriptWindow>> byName(const QList<THeadlessSession::ScriptWindow>& windows)
{
    QHash<QString, QList<THeadlessSession::ScriptWindow>> named;
    for (const THeadlessSession::ScriptWindow& window : windows) {
        named[window.name].append(window);
    }
    return named;
}

QStringList changedNames(const QHash<QString, QList<THeadlessSession::ScriptWindow>>& before, const QHash<QString, QList<THeadlessSession::ScriptWindow>>& after)
{
    QStringList names;
    for (auto it = before.cbegin(); it != before.cend(); ++it) {
        if (after.value(it.key()) != it.value()) {
            names << it.key();
        }
    }
    for (auto it = after.cbegin(); it != after.cend(); ++it) {
        if (!before.contains(it.key())) {
            names << it.key();
        }
    }
    return names;
}

// Where a point in a window is in the main window's area, as QWidget::mapTo() the main window gives it
QPointF inMainArea(const QList<THeadlessSession::ScriptWindow>& windows, const THeadlessSession::ScriptWindow& window, const QPointF& position)
{
    using Kind = THeadlessSession::ScriptWindow::Kind;
    QPointF point = position + window.geometry.topLeft();
    QString parent = window.parent;
    Kind parentKind = window.parentKind;
    while (!parent.isEmpty()) {
        // A mini console and a scroll box may share a name, so the parent's kind picks between them
        const auto container = std::find_if(windows.cbegin(), windows.cend(), [&parent, parentKind](const THeadlessSession::ScriptWindow& candidate) {
            return candidate.name == parent && candidate.kind == parentKind;
        });
        if (container == windows.cend()) {
            break;
        }
        point += container->geometry.topLeft();
        parent = container->parent;
        parentKind = container->parentKind;
    }
    return point;
}

std::unique_ptr<QEvent> qtEvent(const THeadlessSession::WindowPointerEvent& event, const QPointF& global)
{
    using Type = THeadlessSession::WindowPointerEvent::Type;
    switch (event.type) {
    case Type::press:
        return std::make_unique<QMouseEvent>(QEvent::MouseButtonPress, event.position, global, global, event.button, event.buttons, event.modifiers);
    case Type::release:
        return std::make_unique<QMouseEvent>(QEvent::MouseButtonRelease, event.position, global, global, event.button, event.buttons, event.modifiers);
    case Type::doubleClick:
        return std::make_unique<QMouseEvent>(QEvent::MouseButtonDblClick, event.position, global, global, event.button, event.buttons, event.modifiers);
    case Type::move:
        return std::make_unique<QMouseEvent>(QEvent::MouseMove, event.position, global, global, Qt::NoButton, event.buttons, event.modifiers);
    case Type::enter:
        return std::make_unique<QEnterEvent>(event.position, global, global);
    case Type::leave:
        return std::make_unique<QEvent>(QEvent::Leave);
    case Type::wheel:
        return std::make_unique<QWheelEvent>(event.position, global, QPoint(), event.angleDelta, event.buttons, event.modifiers, Qt::NoScrollPhase, false);
    }
    Q_UNREACHABLE();
}

TLabelModel::Callback labelCallback(const THeadlessSession::WindowPointerEvent::Type type)
{
    using Type = THeadlessSession::WindowPointerEvent::Type;
    switch (type) {
    case Type::press:
        return TLabelModel::Callback::click;
    case Type::release:
        return TLabelModel::Callback::release;
    case Type::doubleClick:
        return TLabelModel::Callback::doubleClick;
    case Type::move:
        return TLabelModel::Callback::move;
    case Type::enter:
        return TLabelModel::Callback::enter;
    case Type::leave:
        return TLabelModel::Callback::leave;
    case Type::wheel:
        return TLabelModel::Callback::wheel;
    }
    Q_UNREACHABLE();
}

// A console and its own command line share a name; this is the console
const THeadlessSession::ScriptWindow* windowNamed(const QList<THeadlessSession::ScriptWindow>& windows, const QString& name)
{
    const auto found = std::find_if(windows.cbegin(), windows.cend(), [&name](const THeadlessSession::ScriptWindow& candidate) {
        return candidate.name == name && candidate.kind != THeadlessSession::ScriptWindow::Kind::commandLine;
    });
    return found == windows.cend() ? nullptr : &*found;
}

} // namespace

QList<THeadlessSession::ScriptWindow> THeadlessSession::scriptWindows() const
{
    const auto* pView = mpHost ? dynamic_cast<TNullConsoleFrontend*>(mpHost->consoleFrontend()) : nullptr;
    if (!pView) {
        return {};
    }
    std::vector<Record> records = pView->windowRecords();
    std::stable_sort(records.begin(), records.end(), [](const Record& a, const Record& b) {
        return a.stackOrder < b.stackOrder;
    });
    std::vector<bool> placed(records.size(), false);
    QList<ScriptWindow> windows;
    // The docks share the main window's area's empty parent, but are not stacked with what is in it
    const std::function<void(const QString&, Record::Kind, bool)> placeChildren = [&](const QString& parent, const Record::Kind parentKind, const bool docks) {
        int stackOrder = 0;
        for (size_t i = 0; i < records.size(); ++i) {
            if (placed[i] || records[i].parent != parent || (!parent.isEmpty() && records[i].parentKind != parentKind) || records[i].inDock != docks) {
                continue;
            }
            placed[i] = true;
            const Record& record = records[i];
            windows.append(scriptWindowOf(record, stackOrder++));
            if (holdsWindows(record.kind)) {
                placeChildren(record.name, record.kind, false);
            }
        }
    };
    placeChildren(QString(), Record::Kind::userWindow, false);
    placeChildren(QString(), Record::Kind::userWindow, true);
    // Only a window made in a user window or scroll box the null view does not hold, which a detached GUI view can
    // leave behind, has no parent here: listed last rather than lost, as Lua still finds it
    for (size_t i = 0; i < records.size(); ++i) {
        if (!placed[i]) {
            qWarning().nospace().noquote() << "THeadlessSession::scriptWindows() WARNING - the window \"" << records[i].name << "\" is in \"" << records[i].parent
                                           << "\", which is not among the windows, so it is listed last.";
            windows.append(scriptWindowOf(records[i], 0));
        }
    }
    return windows;
}

std::optional<THeadlessSession::ScriptWindow> THeadlessSession::scriptWindow(const QString& name, const std::optional<ScriptWindow::Kind> kind) const
{
    for (const ScriptWindow& window : scriptWindows()) {
        if (window.name == name && (!kind || window.kind == *kind)) {
            return window;
        }
    }
    return std::nullopt;
}

void THeadlessSession::connectWindowRelays(Host* pHost, const unsigned int run)
{
    mDeliveredWindows.reset();
    mHoveredLabel.reset();
    mGrab.reset();
    mWheelRemainders.clear();
    auto* pView = dynamic_cast<TNullConsoleFrontend*>(pHost->consoleFrontend());
    if (!pView) {
        return;
    }
    connect(
            &pView->windowNotifier(),
            &TWindowNotifier::windowsChanged,
            this,
            [this, run]() {
                if (run != mRun || !mpHost || !mDeliveredWindows) {
                    return;
                }
                WindowsTold now = windowsNow();
                QStringList names = changedNames(byName(mDeliveredWindows->windows), byName(now.windows));
                for (auto it = now.labels.cbegin(); it != now.labels.cend(); ++it) {
                    if (const auto told = mDeliveredWindows->labels.constFind(it.key()); told != mDeliveredWindows->labels.cend() && *told != it.value()) {
                        names << it.key();
                    }
                }
                if (names.isEmpty()) {
                    return;
                }
                names.removeDuplicates();
                names.sort();
                mDeliveredWindows = std::move(now);
                const QString profile = mProfileName;
                emit scriptWindowsChanged(profile, names);
            },
            Qt::QueuedConnection);
    connect(
            &pView->windowNotifier(),
            &TWindowNotifier::textChanged,
            this,
            [this, run](const QStringList& names) {
                if (run != mRun || !mpHost || mShuttingDown || !mDeliveredWindows) {
                    return;
                }
                QStringList windows = names;
                // Deleted since it was noted, or a buffer, which windowLines() does not list
                windows.removeIf([this](const QString& name) {
                    return !windowTextModel(name);
                });
                if (windows.isEmpty()) {
                    return;
                }
                windows.sort();
                const QString profile = mProfileName;
                emit windowTextChanged(profile, windows);
            },
            Qt::QueuedConnection);
    connect(
            &pView->windowNotifier(),
            &TWindowNotifier::commandLinesSet,
            this,
            [this, run](const QStringList& names) {
                if (run != mRun || !mpHost || mShuttingDown) {
                    return;
                }
                QStringList commandLines;
                for (const QString& name : names) {
                    if (windowCommandLineText(name)) {
                        commandLines << name;
                    }
                }
                if (commandLines.isEmpty()) {
                    return;
                }
                commandLines.sort();
                const QString profile = mProfileName;
                emit windowCommandLineTextChanged(profile, commandLines);
            },
            Qt::QueuedConnection);
}

std::optional<QString> THeadlessSession::windowCommandLineText(const QString& commandLine) const
{
    const auto* pView = mpHost ? dynamic_cast<TNullConsoleFrontend*>(mpHost->consoleFrontend()) : nullptr;
    if (!pView || !pView->commandLineAction(commandLine)) {
        return std::nullopt;
    }
    return mpHost->commandLineText(commandLine);
}

bool THeadlessSession::setWindowCommandLineText(const QString& commandLine, const QString& text)
{
    if (!usable()) {
        return false;
    }
    auto* pView = dynamic_cast<TNullConsoleFrontend*>(mpHost->consoleFrontend());
    if (!pView || !pView->commandLineAction(commandLine)) {
        //: Error from a frontend with no window that was given text for a command line, other than the main one, that the profile's scripts have not made; %1 is the name it gave
        mErrorString = tr("The profile has no command line called \"%1\".").arg(commandLine);
        return false;
    }
    if (!pView->commandLineShown(commandLine)) {
        //: Error from a frontend with no window that was given text for, or Enter on, a command line that a script has hidden or disabled, which a player could not type on; %1 is its name
        mErrorString = tr("The command line \"%1\" is not shown, so nothing can be typed on it.").arg(commandLine);
        return false;
    }
    pView->typeCommandLine(commandLine, text);
    return true;
}

bool THeadlessSession::submitWindowCommandLine(const QString& commandLine, const QString& line)
{
    // A script may run the event loop, where the frontend may delete the session, and not the profile with it
    const QPointer<THeadlessSession> self(this);
    if (!setWindowCommandLineText(commandLine, line)) {
        return false;
    }
    const QPointer<Host> pHost = mpHost;
    const auto view = [&pHost]() {
        return pHost ? dynamic_cast<TNullConsoleFrontend*>(pHost->consoleFrontend()) : nullptr;
    };
    const QStringList commands = pHost->commandLineText(commandLine).value_or(QString()).split(QChar::LineFeed);
    // TCommandLine::enterCommand() goes on with its own line, so one an action made again under the name gets none
    const QPointer<const QObject> pLine(view()->commandLineInstance(commandLine));
    const auto sameLine = [&view, &commandLine, &pLine]() {
        return pLine && view() && view()->commandLineInstance(commandLine) == pLine.data();
    };
    for (const QString& command : commands) {
        if (!usable()) {
            return false;
        }
        if (!sameLine()) {
            return true;
        }
        const std::optional<int> action = view()->commandLineAction(commandLine);
        if (!action) {
            return true;
        }
        if (*action) {
            pHost->getLuaInterpreter()->callCmdLineAction(*action, command);
        } else {
            pHost->send(command, true, false);
            if (pHost) {
                pHost->playerSentLineFromCommandLine();
            }
        }
        if (!self || !sameLine()) {
            return true;
        }
        view()->echoSentCommandLine(commandLine, command);
        // The echo can raise sysBufferShrinkEvent, whose handlers may delete the session
        if (!self) {
            return true;
        }
    }
    if (sameLine()) {
        view()->commandLineSent(commandLine);
    }
    return true;
}

THeadlessSession::WindowsTold THeadlessSession::windowsNow() const
{
    WindowsTold now{scriptWindows(), {}};
    for (const ScriptWindow& window : std::as_const(now.windows)) {
        if (window.kind != ScriptWindow::Kind::label) {
            continue;
        }
        // The list is the null view's and the content the registry's model, which could disagree
        if (std::optional<LabelContent> content = labelContent(window.name)) {
            now.labels.insert(window.name, std::move(*content));
        }
    }
    return now;
}

void THeadlessSession::noteWindowsAtLoad()
{
    mDeliveredWindows = windowsNow();
}

std::optional<THeadlessSession::LabelContent> THeadlessSession::labelContent(const QString& name) const
{
    const auto* pView = mpHost ? dynamic_cast<TNullConsoleFrontend*>(mpHost->consoleFrontend()) : nullptr;
    const std::optional<TNullConsoleFrontend::LabelRecord> record = pView ? pView->labelRecord(name) : std::nullopt;
    const TLabelModel* pModel = record ? mpHost->windowRegistry().labelModel(name) : nullptr;
    if (!pModel) {
        return std::nullopt;
    }
    LabelContent content;
    content.text = pModel->mText;
    content.image = record->image;
    content.svg = record->svg;
    content.svgTint = pModel->mSvgTintColor;
    content.svgRotation = pModel->mSvgRotation;
    content.svgShearX = pModel->mSvgShearX;
    content.svgShearY = pModel->mSvgShearY;
    content.movie = record->movie;
    content.moviePaused = record->moviePaused;
    content.movieFrame = record->movieFrame;
    content.movieSpeed = record->movieSpeed;
    content.movieSize = record->movieSize;
    content.toolTip = pModel->mToolTip;
    content.toolTipDuration = record->toolTipDuration;
    content.cursor = record->cursor;
    content.customCursor = record->customCursor;
    content.customCursorHotSpot = record->customCursorHotSpot;
    content.clickThrough = pModel->mClickThrough;
    content.accessibleName = pModel->effectiveAccessibleName();
    content.accessibleButton = pModel->actsAsButton();
    content.styleSheet = pModel->mStyleSheet;
    return content;
}

bool THeadlessSession::setUserWindowGeometry(const QString& name, const QRect& geometry, const bool floating)
{
    if (!usable()) {
        return false;
    }
    auto* pView = dynamic_cast<TNullConsoleFrontend*>(mpHost->consoleFrontend());
    if (!pView || !pView->userWindow(name)) {
        //: Error from a frontend with no window asked to place or close a user window the profile's scripts have not opened; %1 is its name
        mErrorString = tr("There is no user window \"%1\".").arg(name);
        return false;
    }
    // Its resize handlers run in this, and may take the window away
    const QPointer<THeadlessSession> self(this);
    if (pView->placeUserWindow(name, geometry, floating)) {
        return true;
    }
    if (self) {
        //: Error from a frontend with no window placing a user window that a script's handler of its resize took away, or made again; %1 is its name
        mErrorString = tr("The user window \"%1\" was taken away, or made again, by a script as it was placed.").arg(name);
    }
    return false;
}

bool THeadlessSession::closeUserWindow(const QString& name)
{
    if (!usable()) {
        return false;
    }
    const auto* pView = dynamic_cast<TNullConsoleFrontend*>(mpHost->consoleFrontend());
    if (!pView || !pView->userWindow(name)) {
        //: Error from a frontend with no window asked to place or close a user window the profile's scripts have not opened; %1 is its name
        mErrorString = tr("There is no user window \"%1\".").arg(name);
        return false;
    }
    // As TDockWidget::closeEvent()
    if (mpHost->hideWindow(name)) {
        return true;
    }
    //: Error from a frontend with no window closing a user window the profile could not hide; %1 is its name
    mErrorString = tr("The user window \"%1\" could not be closed.").arg(name);
    return false;
}

bool THeadlessSession::pointer(const QString& window, const WindowPointerEvent& event)
{
    using Type = WindowPointerEvent::Type;
    // buttons is what is held after the event, which for a press is its own button whether or not the app gave it
    if ((event.type == Type::press || event.type == Type::doubleClick) && event.button != Qt::NoButton && !(event.buttons & event.button)) {
        WindowPointerEvent holding = event;
        holding.buttons |= event.button;
        return pointer(window, holding);
    }
    if (!usable()) {
        return false;
    }
    const QPointer<THeadlessSession> self(this);
    QList<ScriptWindow> windows = scriptWindows();
    // Before any refusal, as the app's next call after a label hid under its pointer may well name that label
    if (dropHiddenHover(windows)) {
        if (!self || !mpHost) {
            return true;
        }
        windows = scriptWindows();
    }
    // As the Lua window functions take the main console: no name, or "main" in that case
    const bool main = window.isEmpty() || window == qsl("main");
    const auto target = main ? windows.cend() : std::find_if(windows.cbegin(), windows.cend(), [&window](const ScriptWindow& candidate) {
        return candidate.name == window;
    });
    // getMousePosition() follows the pointer, whether or not a window takes what it did there, from where a window
    // the grab names was at the press if it has gone since
    const auto notePointer = [&]() {
        auto* pView = dynamic_cast<TNullConsoleFrontend*>(mpHost->consoleFrontend());
        if (!pView || event.type == Type::leave) {
            return;
        }
        std::optional<QPointF> origin;
        if (main) {
            origin = QPointF();
        } else if (target != windows.cend()) {
            origin = inMainArea(windows, *target, QPointF());
        } else if (mGrab && window == mGrab->name) {
            origin = mGrab->origin;
        } else if (mGrab && !mGrab->through.isEmpty() && window == mGrab->through) {
            origin = mGrab->throughOrigin;
        }
        if (origin) {
            pView->setMousePosition((*origin + event.position).toPoint());
        }
    };
    const bool pressing = event.type == Type::press || event.type == Type::doubleClick;
    // As Qt's implicit grab, which a press with no other button held starts again, and a move with none held ends
    if (mGrab && ((event.type == Type::move && event.buttons == Qt::NoButton) || (pressing && (event.buttons & ~event.button) == Qt::NoButton))) {
        mGrab.reset();
    }
    const bool held = mGrab && (pressing || event.type == Type::move || event.type == Type::release);
    const bool ends = held && event.type == Type::release && event.buttons == Qt::NoButton;
    // QWidgetPrivate::deactivateWidgetCleanup() drops the grab of a widget that hides or goes, and what it held goes nowhere
    if (held && !grabHolds(windows)) {
        notePointer();
        if (ends) {
            mGrab.reset();
        }
        return true;
    }
    const bool hovering = event.type == Type::enter || event.type == Type::leave;
    // The click-through label the press went through, which the app may name for what took it, even once it has gone
    const bool namesThrough = mGrab && !main && !mGrab->through.isEmpty() && window == mGrab->through;
    const bool through = held && namesThrough;
    if (held && !through && (main != mGrab->main || (!main && window != mGrab->name))) {
        //: Error from a frontend with no window given what its mouse did over another window than the one that took the press of a button still held; %1 is that one's name
        mErrorString = tr("A button is held on the window \"%1\", which has the mouse until it is released.").arg(mGrab->main ? qsl("main") : mGrab->name);
        return false;
    }
    // An enter or a leave at it is taken as the grab holds, but a wheel there has no window to turn over once it has gone
    if (!main && !through && !(namesThrough && hovering) && target == windows.cend()) {
        //: Error from a frontend with no window given what its mouse did over a window the profile's scripts have not made; %1 is its name
        mErrorString = tr("There is no window \"%1\".").arg(window);
        return false;
    }
    // While a grab holds, Qt sends no widget an enter or a leave
    if (mGrab && hovering) {
        notePointer();
        return true;
    }
    const std::optional<QSize> size = main ? mpHost->mainWindowSize() : target != windows.cend() ? std::optional(target->geometry.size()) : std::nullopt;
    const bool outside = size && (event.position.x() < 0 || event.position.y() < 0 || event.position.x() >= size->width() || event.position.y() >= size->height());
    // A leave says the pointer went, as Qt sends one to a widget that hides under it
    if (event.type != Type::leave) {
        if (!held && !main && !target->shown) {
            //: Error from a frontend with no window given what its mouse did over a window of the profile's that is hidden; %1 is its name
            mErrorString = tr("The window \"%1\" is not shown.").arg(window);
            return false;
        }
        if (!held && outside) {
            //: Error from a frontend with no window given what its mouse did at a point outside the window it named; %1 and %2 are the point, %3 the window's name
            mErrorString = tr("The point (%1, %2) is not in the window \"%3\".").arg(event.position.x()).arg(event.position.y()).arg(window);
            return false;
        }
        notePointer();
    }
    // QWidget::childAt() passes over a click-through label, so what is under it takes the press, and the grab
    const ScriptWindow* pNamed = main || target == windows.cend() ? nullptr : &*target;
    const ScriptWindow* pTarget = pNamed;
    WindowPointerEvent passed = event;
    if (through) {
        pTarget = grabbedWindow(windows);
        // From where the click-through label was at the press, if the press's callback deleted it
        const QPointF at = pNamed ? inMainArea(windows, *pNamed, event.position) : mGrab->throughOrigin + event.position;
        passed.position = at - (pTarget ? inMainArea(windows, *pTarget, QPointF()) : QPointF());
    } else if (pressing && !held && pNamed && pNamed->kind == ScriptWindow::Kind::label) {
        std::tie(pTarget, passed.position) = passedThrough(windows, *pNamed, event.position);
    }
    const bool label = pTarget && pTarget->kind == ScriptWindow::Kind::label;
    const bool away = label && (passed.position.x() < 0 || passed.position.y() < 0 || passed.position.x() >= pTarget->geometry.width() || passed.position.y() >= pTarget->geometry.height());
    const std::optional<bool> pressedOnText = held ? std::optional(mGrab->text) : std::nullopt;
    if (ends) {
        mGrab.reset();
    } else if (pressing && !held && event.buttons == event.button) {
        mGrab = Grab{!pTarget,
                     onConsoleText(pTarget, passed.position),
                     pTarget ? pTarget->name : QString(),
                     pTarget ? pTarget->kind : ScriptWindow::Kind::miniConsole,
                     pTarget != pNamed ? window : QString(),
                     pTarget ? deactivated(*pTarget) : std::nullopt,
                     pTarget ? inMainArea(windows, *pTarget, QPointF()) : QPointF(),
                     pNamed && pTarget != pNamed ? inMainArea(windows, *pNamed, QPointF()) : QPointF()};
    }
    const bool taken = pointWindow(windows, pTarget, passed, pressedOnText);
    // QApplicationPrivate::sendMouseEvent() leaves the widget that had the grab as a release away from it ends that,
    // which is not a label its release's callback made again under the name
    if (ends && away && self && mpHost && mHoveredLabel && mHoveredLabel->name == pTarget->name && labelMade(pTarget->name) == mHoveredLabel->made) {
        leaveHoveredLabel();
    }
    return taken;
}

const THeadlessSession::ScriptWindow* THeadlessSession::grabbedWindow(const QList<ScriptWindow>& windows) const
{
    if (mGrab->main) {
        return nullptr;
    }
    const auto grabbed = std::find_if(windows.cbegin(), windows.cend(), [this](const ScriptWindow& candidate) {
        return candidate.name == mGrab->name && candidate.kind == mGrab->kind;
    });
    return grabbed == windows.cend() ? nullptr : &*grabbed;
}

bool THeadlessSession::grabHolds(const QList<ScriptWindow>& windows) const
{
    if (mGrab->main) {
        return true;
    }
    // One that lost it between two calls, as a label that hid and showed again, has lost it all the same; one hidden
    // with the window it is in keeps it, as Qt's grab is on the widget itself
    const ScriptWindow* pGrabbed = grabbedWindow(windows);
    return pGrabbed && deactivated(*pGrabbed) == mGrab->deactivated;
}

std::optional<qint64> THeadlessSession::deactivated(const ScriptWindow& window) const
{
    const auto* pView = mpHost ? dynamic_cast<TNullConsoleFrontend*>(mpHost->consoleFrontend()) : nullptr;
    return pView ? pView->deactivated(window.name, window.kind == ScriptWindow::Kind::label) : std::nullopt;
}

bool THeadlessSession::dropHiddenHover(const QList<ScriptWindow>& windows)
{
    if (!mHoveredLabel) {
        return false;
    }
    // One deleted, or deleted and made again, since the pointer entered it is not the label the pointer is over
    if (labelMade(mHoveredLabel->name) != mHoveredLabel->made) {
        mHoveredLabel.reset();
        return false;
    }
    // One that keeps the grab as the window it is in hides has its leave with the release or the next call
    if (mGrab && mGrab->kind == ScriptWindow::Kind::label && mGrab->name == mHoveredLabel->name && grabHolds(windows)) {
        return false;
    }
    const auto hovered = std::find_if(windows.cbegin(), windows.cend(), [this](const ScriptWindow& candidate) {
        return candidate.name == mHoveredLabel->name && candidate.kind == ScriptWindow::Kind::label;
    });
    if (hovered == windows.cend() || hovered->shown) {
        return false;
    }
    leaveHoveredLabel();
    return true;
}

void THeadlessSession::leaveHoveredLabel()
{
    if (!mHoveredLabel) {
        return;
    }
    if (const TLabelModel* pModel = mpHost->windowRegistry().labelModel(std::exchange(mHoveredLabel, std::nullopt)->name)) {
        pModel->runCallback(TLabelModel::Callback::leave, QEvent(QEvent::Leave));
    }
}

std::optional<qint64> THeadlessSession::labelMade(const QString& name) const
{
    const auto* pView = mpHost ? dynamic_cast<TNullConsoleFrontend*>(mpHost->consoleFrontend()) : nullptr;
    const std::optional<TNullConsoleFrontend::LabelRecord> record = pView ? pView->labelRecord(name) : std::nullopt;
    return record ? std::optional(record->made) : std::nullopt;
}

void THeadlessSession::noteMapPointer(const QPointF& position)
{
    auto* pView = dynamic_cast<TNullConsoleFrontend*>(mpHost->consoleFrontend());
    if (!pView) {
        return;
    }
    const QList<ScriptWindow> windows = scriptWindows();
    const auto mapper = std::ranges::find(windows, ScriptWindow::Kind::mapper, &ScriptWindow::kind);
    // T2DMap fills its window from the top left, so a point of the map is that point of the window
    if (mapper != windows.cend() && mapper->shown) {
        pView->setMousePosition(inMainArea(windows, *mapper, position).toPoint());
    }
}

bool THeadlessSession::onConsoleText(const ScriptWindow* pWindow, const QPointF& position) const
{
    if (!pWindow) {
        return mainConsoleTextRect().contains(position.toPoint());
    }
    return consoleText(pWindow->kind);
}

bool THeadlessSession::pointWindow(const QList<ScriptWindow>& windows, const ScriptWindow* pWindow, const WindowPointerEvent& event, const std::optional<bool> pressedOnText)
{
    using Kind = ScriptWindow::Kind;
    if (pWindow && pWindow->kind == Kind::label) {
        return pointLabel(windows, *pWindow, event);
    }
    // QWidgetWindow::handleWheelEvent() sends a wheel to the widget under the pointer, which a held button does not
    // enter
    if (mHoveredLabel && !(mGrab && event.type == WindowPointerEvent::Type::wheel)) {
        const QPointer<THeadlessSession> self(this);
        leaveHoveredLabel();
        if (!self || !mpHost) {
            return true;
        }
    }
    if (pWindow && pWindow->kind == Kind::mapper) {
        return pointMapper(*pWindow, event);
    }
    // A command line or text box keeps the mouse for itself
    if (pWindow && !miniConsole(pWindow->kind) && pWindow->kind != Kind::userWindow && pWindow->kind != Kind::scrollBox) {
        return true;
    }
    if (event.type == WindowPointerEvent::Type::wheel) {
        if (!pWindow || pWindow->kind != Kind::scrollBox) {
            scrollConsole(pWindow ? pWindow->name : qsl("main"), event);
        }
        return true;
    }
    // TTextEdit::mouseReleaseEvent() opens its menu for the right button and raises nothing, wherever the grab its
    // press gave it has the release
    if (event.type == WindowPointerEvent::Type::release && event.button == Qt::RightButton && pressedOnText.value_or(onConsoleText(pWindow, event.position))) {
        return true;
    }
    // TTextEdit gives a mini console's press to the widget it is in
    if (pWindow && pWindow->kind != Kind::userWindow) {
        pointConsole(windows, pWindow->parent, event.position + pWindow->geometry.topLeft(), event);
    } else {
        pointConsole(windows, pWindow ? pWindow->name : QString(), event.position, event);
    }
    return true;
}

void THeadlessSession::pointConsole(const QList<ScriptWindow>& windows, const QString& container, const QPointF& position, const WindowPointerEvent& event)
{
    if (event.type != WindowPointerEvent::Type::press && event.type != WindowPointerEvent::Type::release && event.type != WindowPointerEvent::Type::doubleClick) {
        return;
    }
    // As Qt passes an event a widget leaves up to its parent, until a console takes it
    QString console = container;
    QPointF at = position;
    while (!console.isEmpty()) {
        const ScriptWindow* pWindow = windowNamed(windows, console);
        if (!pWindow) {
            return;
        }
        // A frame the game nested others in is a plain widget around them, unless it is an EXTERNAL one's window
        if (consoleText(pWindow->kind) && (pWindow->kind != ScriptWindow::Kind::mxpFrame || pWindow->inDock)) {
            break;
        }
        at += pWindow->geometry.topLeft();
        console = pWindow->parent;
    }
    // QWidget::mouseDoubleClickEvent() presses again
    const bool press = event.type != WindowPointerEvent::Type::release;
    const QMouseEvent qtEvent(press ? QEvent::MouseButtonPress : QEvent::MouseButtonRelease, at, at, event.button, event.buttons, event.modifiers);
    mpHost->raiseWindowMouseEvent(console.isEmpty() ? qsl("main") : console, qtEvent, press);
}

bool THeadlessSession::opensConsoleMenu(const QList<ScriptWindow>& windows, const ScriptWindow& label, const QPointF& position) const
{
    using Kind = ScriptWindow::Kind;
    const auto bare = [this](const QString& name) {
        const TLabelModel* pModel = mpHost->windowRegistry().labelModel(name);
        return pModel && !pModel->mClickFunction && !pModel->mReleaseFunction;
    };
    if (!bare(label.name)) {
        return false;
    }
    // QWidget::childAt() in a parent, with the label and each bare one it finds left out
    const auto hitIn = [&](const QString& parent, const QPoint& point) {
        const ScriptWindow* pHit = nullptr;
        for (const ScriptWindow& candidate : windows) {
            if (candidate.parent != parent || candidate.inDock || !candidate.shown || !candidate.geometry.contains(point) || (pHit && candidate.stackOrder < pHit->stackOrder)) {
                continue;
            }
            if (candidate.kind == Kind::label) {
                const std::optional<LabelContent> content = labelContent(candidate.name);
                if ((content && content->clickThrough) || bare(candidate.name)) {
                    continue;
                }
            }
            pHit = &candidate;
        }
        return pHit;
    };
    const QPoint inParent = (position + label.geometry.topLeft()).toPoint();
    const ScriptWindow* pParent = label.parent.isEmpty() ? nullptr : windowNamed(windows, label.parent);
    // QWidget::childAt() finds nothing outside the widget it is asked of
    const std::optional<QSize> area = pParent ? std::optional(pParent->geometry.size()) : mpHost->mainWindowSize();
    if ((!label.parent.isEmpty() && !pParent) || (area && !QRect(QPoint(), *area).contains(inParent))) {
        return false;
    }
    QPoint point = inParent;
    const ScriptWindow* pHit = hitIn(label.parent, point);
    // childAt() goes on into what a scroll box or mini console holds there; a scroll box's own widget is not a console's text
    while (pHit && (pHit->kind == Kind::scrollBox || miniConsole(pHit->kind))) {
        point -= pHit->geometry.topLeft();
        const ScriptWindow* pInner = hitIn(pHit->name, point);
        if (!pInner) {
            return miniConsole(pHit->kind);
        }
        pHit = pInner;
    }
    if (pHit) {
        return false;
    }
    if (!pParent) {
        return mainConsoleTextRect().contains(inParent);
    }
    return consoleText(pParent->kind);
}

void THeadlessSession::scrollConsole(const QString& name, const WindowPointerEvent& event)
{
    TConsoleModel* pModel = mpHost->consoleModelNamed(name);
    const std::optional<int> scroll = mpHost->windowScroll(name);
    if (!pModel || !scroll) {
        return;
    }
    // As TTextEdit::wheelEvent(): three lines a notch, one with Shift, half the console's rows with Control
    const double speedUp = event.modifiers & Qt::ShiftModifier ? 1.0 : (event.modifiers & Qt::ControlModifier ? qRound(pModel->mGridSize.height() / 2.0) : 3.0);
    const QObject* pConsole = &pModel->mNotifier;
    const std::pair<QPointer<const QObject>, int> carried = mWheelRemainders.value(name);
    const double degrees = event.angleDelta.y() / 8.0 * speedUp + (carried.first == pConsole ? carried.second : 0);
    const int lines = qRound(degrees / 15.0);
    mWheelRemainders.insert(name, {pConsole, static_cast<int>(degrees - 15 * lines)});
    if (!lines) {
        return;
    }
    const int lastLine = pModel->buffer.getLastLineNumber();
    const int from = mpHost->consoleFrontend()->windowFollowsNewLines(name) ? lastLine : *scroll;
    const int to = std::clamp(from - lines, 0, lastLine);
    mpHost->consoleFrontend()->scrollWindowTo(name, to, to >= lastLine);
}

std::pair<const THeadlessSession::ScriptWindow*, QPointF> THeadlessSession::passedThrough(const QList<ScriptWindow>& windows, const ScriptWindow& label, const QPointF& position) const
{
    const ScriptWindow* pWindow = &label;
    QPointF at = position;
    // Qt::WA_TransparentForMouseEvents: what is under it at that point has it instead, through each such label
    while (pWindow && pWindow->kind == ScriptWindow::Kind::label) {
        const std::optional<LabelContent> content = labelContent(pWindow->name);
        if (!content || !content->clickThrough) {
            break;
        }
        const ScriptWindow& over = *pWindow;
        const QPointF inParent = at + over.geometry.topLeft();
        const ScriptWindow* pUnder = nullptr;
        for (const ScriptWindow& candidate : windows) {
            if (candidate.parent == over.parent && !candidate.inDock && candidate.shown && candidate.stackOrder < over.stackOrder && candidate.geometry.contains(inParent.toPoint())
                && (!pUnder || candidate.stackOrder > pUnder->stackOrder)) {
                pUnder = &candidate;
            }
        }
        at = pUnder ? inParent - pUnder->geometry.topLeft() : inParent;
        pWindow = pUnder ? pUnder : windowNamed(windows, over.parent);
    }
    return {pWindow, at};
}

bool THeadlessSession::pointLabel(const QList<ScriptWindow>& windows, const ScriptWindow& label, const WindowPointerEvent& event)
{
    if (const std::optional<LabelContent> content = labelContent(label.name); content && content->clickThrough) {
        WindowPointerEvent passed = event;
        const ScriptWindow* pUnder = nullptr;
        // A leave says the pointer went, not from where, so it leaves whatever the pointer was over through it
        if (event.type == WindowPointerEvent::Type::leave) {
            passed.position = event.position + label.geometry.topLeft();
            pUnder = windowNamed(windows, label.parent);
        } else {
            std::tie(pUnder, passed.position) = passedThrough(windows, label, event.position);
        }
        return pointWindow(windows, pUnder, passed);
    }
    const QPointF global = inMainArea(windows, label, event.position);
    // Each callback may close the profile, or delete the session from a function the app bound into Lua
    const QPointer<THeadlessSession> self(this);
    const auto run = [this, &self](const QString& name, const TLabelModel::Callback callback, const QEvent& qtEvent) {
        if (!self || !mpHost) {
            return false;
        }
        const TLabelModel* pModel = mpHost->windowRegistry().labelModel(name);
        return pModel && pModel->runCallback(callback, qtEvent);
    };
    // Qt delivers the event to the widget it found under the pointer, after the leave and enter it sends first; one
    // of those deleting the label leaves that widget, with its callbacks, till deferred deletes run, and a label made
    // again under the name is another widget
    const TLabelModel* pLabel = mpHost->windowRegistry().labelModel(label.name);
    const auto runOwn = [this, &self, pLabel](const TLabelModel::Callback callback, const QEvent& qtEvent) {
        if (!self || !mpHost) {
            return false;
        }
        const auto* pView = dynamic_cast<TNullConsoleFrontend*>(mpHost->consoleFrontend());
        return pView && pView->holdsLabelModel(pLabel) && pLabel->runCallback(callback, qtEvent);
    };
    if (event.type == WindowPointerEvent::Type::leave) {
        if (mHoveredLabel && mHoveredLabel->name == label.name) {
            mHoveredLabel.reset();
            run(label.name, TLabelModel::Callback::leave, QEvent(QEvent::Leave));
        }
        return true;
    }
    // As Qt sends the label it was over its leave, and this one its enter, before the event itself; a wheel while a
    // button is held enters nothing
    if ((!mHoveredLabel || mHoveredLabel->name != label.name) && !(mGrab && event.type == WindowPointerEvent::Type::wheel)) {
        const std::optional<HoveredLabel> left = std::exchange(mHoveredLabel, HoveredLabel{label.name, labelMade(label.name).value_or(0)});
        if (left) {
            run(left->name, TLabelModel::Callback::leave, QEvent(QEvent::Leave));
        }
        runOwn(TLabelModel::Callback::enter, QEnterEvent(event.position, global, global));
        if (event.type == WindowPointerEvent::Type::enter) {
            return true;
        }
    } else if (event.type == WindowPointerEvent::Type::enter) {
        return true;
    }
    const std::unique_ptr<QEvent> pQtEvent = qtEvent(event, global);
    if (runOwn(labelCallback(event.type), *pQtEvent)) {
        return true;
    }
    // QWidget::mouseDoubleClickEvent() presses again
    if (event.type == WindowPointerEvent::Type::doubleClick && runOwn(TLabelModel::Callback::click, *pQtEvent)) {
        return true;
    }
    if (!self || !mpHost) {
        return true;
    }
    // TLabel::mouseReleaseEvent() gives it to the menu
    if (event.type == WindowPointerEvent::Type::release && event.button == Qt::RightButton && opensConsoleMenu(windows, label, event.position)) {
        return true;
    }
    pointConsole(windows, label.parent, event.position + label.geometry.topLeft(), event);
    return true;
}
