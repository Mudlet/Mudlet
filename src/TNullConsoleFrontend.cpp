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

#include "TNullConsoleFrontend.h"

#include "Host.h"
#include "TEvent.h"
#include "TLabelModel.h"

#include <QCoreApplication>
#include <QFontMetrics>
#include <QFontMetricsF>
#include <QImage>
#include <QObject>
#include <QStringList>

#include <algorithm>

TNullConsoleFrontend::TNullConsoleFrontend(Host* pHost)
: mpHost(pHost)
{
}

TNullConsoleFrontend::~TNullConsoleFrontend() = default;

void TNullConsoleFrontend::dropWindows()
{
    // Copies, as each call erases the node that holds the name
    while (!mSubConsoles.empty()) {
        const QString name = mSubConsoles.begin()->first;
        removeSubConsole(name);
    }
    while (!mLabels.empty()) {
        const QString name = mLabels.begin()->first;
        removeLabel(name);
    }
}

QString TNullConsoleFrontend::userWindowOrMain(const QString& windowname) const
{
    return mpHost->windowRegistry().hasDockWidget(windowname) ? windowname : QString();
}

void TNullConsoleFrontend::createLabel(const QString& windowname, const QString& name, int x, int y, int width, int height, bool, bool)
{
    auto pLabel = std::make_unique<TLabelModel>(mpHost, name);
    pLabel->mGeometry = QRect(x, y, width, height);
    const QString userWindow = userWindowOrMain(windowname);
    // A real label takes its console's font as it is made, and keeps it as that font changes
    pLabel->mFont = consoleFont(userWindow).value_or(displayFont());
    mpHost->windowRegistry().registerLabel(name, pLabel.get());
    mLabels[name] = {std::move(pLabel), userWindow};
    reportVisibility(name);
    // TMainConsole::createLabel()'s colour
    setLabelBackgroundColor(name, QColor(32, 32, 32, 255));
}

void TNullConsoleFrontend::deleteLabel(const QString& name)
{
    removeLabel(name);
}

void TNullConsoleFrontend::removeLabel(const QString& name)
{
    const auto it = mLabels.find(name);
    if (it == mLabels.end()) {
        return;
    }
    mpHost->windowRegistry().deregisterLabel(name, it->second.pModel.get());
    mRetiredLabels.push_back(std::move(it->second.pModel));
    mLabels.erase(it);
    queueRelease();
}

void TNullConsoleFrontend::queueRelease()
{
    if (mReleaseQueued) {
        return;
    }
    mReleaseQueued = true;
    // Host's reset drains deferred deletes before it swaps the Lua state, which a timer would miss
    auto pRelease = new QObject();
    QObject::connect(pRelease, &QObject::destroyed, mpHost, [this]() {
        releaseRetired();
    });
    pRelease->deleteLater();
}

void TNullConsoleFrontend::releaseRetired()
{
    mReleaseQueued = false;
    mRetiredConsoles.clear();
    mRetiredLabels.clear();
}

bool TNullConsoleFrontend::setLabelText(const QString& name, const QString& text)
{
    const auto it = mLabels.find(name);
    if (it == mLabels.end()) {
        return false;
    }
    it->second.pModel->mText = text;
    return true;
}

TConsoleModel& TNullConsoleFrontend::addSubConsole(const QString& name, const TWindowRegistry::SubConsoleKind kind, const QString& windowname)
{
    // As resolveConsoleModel() and the TConsole constructor set up a sub-console's model
    auto pModel = std::make_unique<TConsoleModel>(mpHost);
    pModel->mConsoleName = name;
    pModel->mScriptAddressable = true;
    pModel->mScrollBarEnabled = false;
    pModel->mUpperPaneFont = TFontAttributes(mpHost->fontsAntiAlias()).makeFont();
    TConsoleModel& model = *pModel;
    mpHost->windowRegistry().registerSubConsole(name, &model, kind);
    mSubConsoles[name] = {std::move(pModel), userWindowOrMain(windowname)};
    return model;
}

void TNullConsoleFrontend::removeSubConsole(const QString& name)
{
    const auto it = mSubConsoles.find(name);
    if (it == mSubConsoles.end()) {
        return;
    }
    TWindowRegistry& registry = mpHost->windowRegistry();
    if (registry.subConsoleKind(name) == TWindowRegistry::SubConsoleKind::UserWindow) {
        QStringList contents;
        for (const auto& [childName, child] : mSubConsoles) {
            if (child.userWindow == name) {
                contents << childName;
            }
        }
        for (const QString& childName : contents) {
            removeSubConsole(childName);
        }
        contents.clear();
        for (const auto& [childName, child] : mLabels) {
            if (child.userWindow == name) {
                contents << childName;
            }
        }
        for (const QString& childName : contents) {
            removeLabel(childName);
        }
        registry.deregisterDockWidget(name);
    }
    registry.deregisterSubConsole(name, it->second.pModel.get());
    mRetiredConsoles.push_back(std::move(it->second.pModel));
    mSubConsoles.erase(it);
    queueRelease();
}

void TNullConsoleFrontend::createBuffer(const QString& name)
{
    TConsoleModel& model = addSubConsole(name, TWindowRegistry::SubConsoleKind::Buffer, QString());
    model.setWrapAt(mpHost->mWrapAt);
    model.setIndentCount(mpHost->mWrapIndentCount);
    model.setHangingIndentCount(mpHost->mWrapHangingIndentCount);
    reportGridSize(name);
}

void TNullConsoleFrontend::addMiniConsole(const QString& windowname, const QString& name, int x, int y, int width, int height)
{
    addSubConsole(name, TWindowRegistry::SubConsoleKind::MiniConsole, windowname);
    mpHost->windowRegistry().setSubConsoleGeometry(name, QRect(x, y, width, height));
    // As TMainConsole::createMiniConsole() sizes its font, which raises sysFontChangeEvent
    setWindowFontSize(name, 12);
    setSubConsoleShown(name, true);
}

void TNullConsoleFrontend::deleteMiniConsole(const QString& name)
{
    if (mSubConsoles.count(name)) {
        mpHost->windowRegistry().forgetUserWindowSize(name);
        removeSubConsole(name);
    }
}

void TNullConsoleFrontend::closeSubConsole(const QString& name)
{
    if (!mSubConsoles.count(name)) {
        return;
    }
    if (mpHost->isClosingDown()) {
        removeSubConsole(name);
    } else {
        setSubConsoleShown(name, false);
    }
}

void TNullConsoleFrontend::openUserWindow(const QString& name, bool, bool, const QString&)
{
    TWindowRegistry& registry = mpHost->windowRegistry();
    if (!mSubConsoles.count(name)) {
        if (registry.hasSubConsole(name)) {
            // A detached view's, which this has no dock for
            return;
        }
        addSubConsole(name, TWindowRegistry::SubConsoleKind::UserWindow, QString());
        registry.registerDockWidget(name);
        registry.setUserWindowTitle(name, name);
        registry.setUserWindowStyleSheet(name, mpHost->mProfileStyleSheet);
        // There is no layout to dock it into, so it takes the main window's size
        const QSize size = mpHost->mainWindowSize().value_or(QSize());
        registry.setSubConsoleGeometry(name, QRect(QPoint(0, 0), size));
        registry.setUserWindowSize(name, size);
        // As TMainConsole::createUserWindow() sizes its font, which raises sysFontChangeEvent
        setWindowFontSize(name, 10);
    }
    setSubConsoleShown(name, true);
}

TLabelModel* TNullConsoleFrontend::labelModel(const QString& name) const
{
    const auto it = mLabels.find(name);
    return it == mLabels.end() ? nullptr : it->second.pModel.get();
}

TConsoleModel* TNullConsoleFrontend::subConsoleModel(const QString& name) const
{
    const auto it = mSubConsoles.find(name);
    return it == mSubConsoles.end() ? nullptr : it->second.pModel.get();
}

bool TNullConsoleFrontend::setLabelShown(const QString& name, const bool shown)
{
    const auto it = mLabels.find(name);
    if (it == mLabels.end()) {
        return false;
    }
    it->second.shown = shown;
    reportVisibility(name);
    return true;
}

bool TNullConsoleFrontend::setSubConsoleShown(const QString& name, const bool shown)
{
    const auto it = mSubConsoles.find(name);
    if (it == mSubConsoles.end()) {
        return false;
    }
    it->second.shown = shown;
    reportVisibility(name);
    return true;
}

// As TMainConsole::reportVisibility() writes QWidget::isVisibleTo() the main console, for which a
// window inside a hidden user window is hidden too.
void TNullConsoleFrontend::reportVisibility(const QString& name)
{
    const auto shownWithin = [this](const QString& userWindow) {
        if (userWindow.isEmpty()) {
            return true;
        }
        const auto it = mSubConsoles.find(userWindow);
        return it != mSubConsoles.end() && it->second.shown;
    };
    if (const auto it = mLabels.find(name); it != mLabels.end()) {
        it->second.pModel->mVisible = it->second.shown && shownWithin(it->second.userWindow);
    }
    const auto it = mSubConsoles.find(name);
    if (it == mSubConsoles.end()) {
        return;
    }
    TWindowRegistry& registry = mpHost->windowRegistry();
    registry.setSubConsoleVisible(name, it->second.shown && shownWithin(it->second.userWindow));
    if (registry.subConsoleKind(name) != TWindowRegistry::SubConsoleKind::UserWindow) {
        return;
    }
    for (const auto& [childName, child] : mSubConsoles) {
        if (child.userWindow == name) {
            registry.setSubConsoleVisible(childName, child.shown && it->second.shown);
        }
    }
    for (const auto& [childName, child] : mLabels) {
        if (child.userWindow == name) {
            child.pModel->mVisible = child.shown && it->second.shown;
        }
    }
}

std::pair<bool, QString> TNullConsoleFrontend::setLabelStyleSheet(const QString& name, const QString& stylesheet)
{
    if (name.isEmpty()) {
        return {false, qsl("a label cannot have an empty string as its name")};
    }
    TLabelModel* pLabel = labelModel(name);
    if (!pLabel) {
        return {false, qsl("label name '%1' not found").arg(name)};
    }
    pLabel->mStyleSheet = stylesheet;
    return {true, QString()};
}

std::pair<bool, QString> TNullConsoleFrontend::setLabelToolTip(const QString& name, const QString& text, double)
{
    if (name.isEmpty()) {
        return {false, qsl("a label cannot have an empty string as its name")};
    }
    TLabelModel* pLabel = labelModel(name);
    if (!pLabel) {
        return {false, qsl("label name '%1' not found").arg(name)};
    }
    pLabel->mToolTip = text;
    return {true, QString()};
}

std::pair<bool, QString> TNullConsoleFrontend::setLabelCursor(const QString& name, const int shape)
{
    if (name.isEmpty()) {
        return {false, qsl("a label cannot have an empty string as its name")};
    }
    if (!labelModel(name)) {
        return {false, qsl("label name '%1' not found").arg(name)};
    }
    // The shapes Qt::CursorShape has, bar the bitmap ones, and -1 to unset it
    if (shape < -1 || shape > 21) {
        return {false, qsl("cursor shape '%1' not found. see https://doc.qt.io/qt-5/qt.html#CursorShape-enum").arg(shape)};
    }
    return {true, QString()};
}

std::pair<bool, QString> TNullConsoleFrontend::setLabelCustomCursor(const QString& name, const QString& pixMapLocation, int, int)
{
    if (name.isEmpty()) {
        return {false, qsl("a label cannot have an empty string as its name")};
    }
    if (pixMapLocation.isEmpty()) {
        return {false, qsl("custom cursor location cannot be an empty string")};
    }
    if (!labelModel(name)) {
        return {false, qsl("label name '%1' not found").arg(name)};
    }
    // A QPixmap needs a GUI application; it loads files through QImage regardless
    if (QImage(pixMapLocation).isNull()) {
        return {false, qsl("couldn't find custom cursor, is the location \"%1\" correct?").arg(pixMapLocation)};
    }
    return {true, QString()};
}

bool TNullConsoleFrontend::setLabelLinkStyle(const QString& name, const QString& linkColor, const QString& linkVisitedColor, const bool underline)
{
    TLabelModel* pLabel = labelModel(name);
    if (!pLabel) {
        return false;
    }
    pLabel->mLinkColor = linkColor;
    pLabel->mLinkVisitedColor = linkVisitedColor;
    pLabel->mLinkUnderline = underline;
    return true;
}

bool TNullConsoleFrontend::resetLabelLinkStyle(const QString& name)
{
    return setLabelLinkStyle(name, QString(), QString(), true);
}

bool TNullConsoleFrontend::clearLabelVisitedLinks(const QString& name)
{
    TLabelModel* pLabel = labelModel(name);
    if (!pLabel) {
        return false;
    }
    pLabel->mVisitedLinks.clear();
    return true;
}

// QWidget::resize() takes a negative size as nothing
bool TNullConsoleFrontend::resizeLabel(const QString& name, const int width, const int height)
{
    TLabelModel* pLabel = labelModel(name);
    if (!pLabel) {
        return false;
    }
    pLabel->mGeometry.setSize(QSize(width, height).expandedTo(QSize(0, 0)));
    return true;
}

bool TNullConsoleFrontend::moveLabel(const QString& name, const int x, const int y)
{
    TLabelModel* pLabel = labelModel(name);
    if (!pLabel) {
        return false;
    }
    pLabel->mGeometry.moveTo(x, y);
    return true;
}

// QWidget::setParent() hides the widget, so it only shows again when asked
bool TNullConsoleFrontend::reparentLabel(const QString& windowname, const QString& name, const int x, const int y, const bool show)
{
    const auto it = mLabels.find(name);
    if (it == mLabels.end()) {
        return false;
    }
    const QString userWindow = userWindowOrMain(windowname);
    // A real label with no font of its own takes the application's, not its new parent's, as it moves
    if (userWindow != it->second.userWindow && !it->second.ownFont) {
        it->second.pModel->mFont = QFont();
    }
    it->second.userWindow = userWindow;
    it->second.pModel->mGeometry.moveTo(x, y);
    it->second.shown = show;
    reportVisibility(name);
    return true;
}

bool TNullConsoleFrontend::setLabelBackgroundColor(const QString& name, const QColor& color)
{
    TLabelModel* pLabel = labelModel(name);
    if (!pLabel) {
        return false;
    }
    pLabel->mBackgroundColor = color;
    pLabel->mStyleSheet = TLabelModel::styleSheetWithBackgroundColor(pLabel->mStyleSheet, color);
    return true;
}

// TLabel stamps its palette with the colour it was last given, which is what the real view reads
std::optional<QColor> TNullConsoleFrontend::getLabelBackgroundColor(const QString& name) const
{
    const TLabelModel* pLabel = labelModel(name);
    if (!pLabel) {
        return std::nullopt;
    }
    return pLabel->mBackgroundColor;
}

bool TNullConsoleFrontend::setLabelSvgTint(const QString& name, const QColor& color)
{
    TLabelModel* pLabel = labelModel(name);
    if (!pLabel) {
        return false;
    }
    pLabel->mSvgTintColor = color;
    return true;
}

bool TNullConsoleFrontend::setLabelSvgRotation(const QString& name, const double angle)
{
    TLabelModel* pLabel = labelModel(name);
    if (!pLabel) {
        return false;
    }
    pLabel->mSvgRotation = angle;
    return true;
}

bool TNullConsoleFrontend::setLabelSvgShear(const QString& name, const double shearX, const double shearY)
{
    TLabelModel* pLabel = labelModel(name);
    if (!pLabel) {
        return false;
    }
    pLabel->mSvgShearX = shearX;
    pLabel->mSvgShearY = shearY;
    return true;
}

bool TNullConsoleFrontend::setLabelFont(const QString& name, const QFont& font)
{
    const auto it = mLabels.find(name);
    if (it == mLabels.end()) {
        return false;
    }
    it->second.pModel->mFont = font;
    it->second.ownFont = true;
    return true;
}

// A user window has no frame here, so its console is its dock's size
bool TNullConsoleFrontend::resizeSubConsole(const QString& name, const int width, const int height)
{
    if (!mSubConsoles.count(name)) {
        return false;
    }
    TWindowRegistry& registry = mpHost->windowRegistry();
    QRect geometry = registry.subConsoleGeometry(name).value_or(QRect());
    geometry.setSize(QSize(width, height).expandedTo(QSize(0, 0)));
    registry.setSubConsoleGeometry(name, geometry);
    registry.setUserWindowSize(name, geometry.size());
    reportGridSize(name);
    return true;
}

bool TNullConsoleFrontend::moveSubConsole(const QString& name, const int x, const int y)
{
    if (!mSubConsoles.count(name)) {
        return false;
    }
    TWindowRegistry& registry = mpHost->windowRegistry();
    QRect geometry = registry.subConsoleGeometry(name).value_or(QRect());
    geometry.moveTo(x, y);
    registry.setSubConsoleGeometry(name, geometry);
    return true;
}

std::pair<bool, QString> TNullConsoleFrontend::setUserWindowStyleSheet(const QString& name, const QString& userWindowStyleSheet)
{
    if (name.isEmpty()) {
        return {false, qsl("a userwindow cannot have an empty string as its name")};
    }
    TWindowRegistry& registry = mpHost->windowRegistry();
    if (!mSubConsoles.count(name) || !registry.hasDockWidget(name)) {
        return {false, qsl("userwindow name '%1' not found").arg(name)};
    }
    registry.setUserWindowStyleSheet(name, userWindowStyleSheet);
    return {true, QString()};
}

std::pair<bool, QString> TNullConsoleFrontend::setUserWindowTitle(const QString& name, const QString& text)
{
    if (name.isEmpty()) {
        return {false, qsl("a user window cannot have an empty string as its name")};
    }
    if (!mSubConsoles.count(name)) {
        return {false, qsl("user window name '%1' not found").arg(name)};
    }
    TWindowRegistry& registry = mpHost->windowRegistry();
    if (registry.subConsoleKind(name) != TWindowRegistry::SubConsoleKind::UserWindow) {
        return {false, qsl("\"%1\" is not a user window").arg(name)};
    }
    // The real view's default title, so translated in its context
    //: Default title of a user window; %1 is the profile's name and %2 the window's
    registry.setUserWindowTitle(name, text.isEmpty() ? QCoreApplication::translate("TMainConsole", "User window - %1 - %2").arg(mpHost->getName(), name) : text);
    return {true, QString()};
}

void TNullConsoleFrontend::setBorderColor(const QColor& color)
{
    mpHost->mainConsoleModel().mBorderColor = color;
}

// As TConsole::setConsoleBgColor() leaves a sub-console's model
bool TNullConsoleFrontend::setSubConsoleBackgroundColor(const QString& name, const QColor& color)
{
    TConsoleModel* pModel = subConsoleModel(name);
    if (!pModel) {
        return false;
    }
    pModel->mBgColor = color;
    pModel->buffer.updateColors();
    return true;
}

bool TNullConsoleFrontend::setSubConsoleCommandBackgroundColor(const QString& name, const QColor& color)
{
    TConsoleModel* pModel = subConsoleModel(name);
    if (!pModel) {
        return false;
    }
    pModel->mCommandBgColor = color;
    return true;
}

bool TNullConsoleFrontend::setSubConsoleCommandForegroundColor(const QString& name, const QColor& color)
{
    TConsoleModel* pModel = subConsoleModel(name);
    if (!pModel) {
        return false;
    }
    pModel->mCommandFgColor = color;
    return true;
}

bool TNullConsoleFrontend::setWindowScrollBarVisible(const QString& name, const bool visible)
{
    TConsoleModel* pModel = mpHost->consoleModelNamed(name);
    if (!pModel) {
        return false;
    }
    pModel->mScrollBarEnabled = visible;
    return true;
}

// Nothing reads a horizontal scroll bar back, so there is nothing to record
bool TNullConsoleFrontend::setWindowHorizontalScrollBarVisible(const QString& name, bool)
{
    return mpHost->consoleModelNamed(name) != nullptr;
}

// As TConsole::setScrolling(), which leaves the main console and buffers scrolling
bool TNullConsoleFrontend::setWindowScrolling(const QString& name, const bool enabled)
{
    TConsoleModel* pModel = mpHost->consoleModelNamed(name);
    if (!pModel) {
        return false;
    }
    const auto kind = mpHost->windowRegistry().subConsoleKind(name);
    if (kind == TWindowRegistry::SubConsoleKind::MiniConsole || kind == TWindowRegistry::SubConsoleKind::UserWindow) {
        pModel->mScrollingEnabled = enabled;
        if (!enabled) {
            pModel->mScrolledBackTo.reset();
        }
    }
    return true;
}

// Lands on the line itself, where TMainConsole::scrollWindowTo() lands its first scroll short by the rows of
// the split screen pane it opens
bool TNullConsoleFrontend::scrollWindowTo(const QString& name, const int line, const bool toEnd)
{
    TConsoleModel* pModel = mpHost->consoleModelNamed(name);
    if (!pModel) {
        return false;
    }
    const int lastLine = pModel->buffer.getLastLineNumber();
    if (toEnd || line >= lastLine) {
        pModel->mScrolledBackTo.reset();
        return true;
    }
    // A buffer is never painted, so a real one never shows itself scrolled back
    if (!pModel->mScrollingEnabled || mpHost->windowRegistry().subConsoleKind(name) == TWindowRegistry::SubConsoleKind::Buffer) {
        return true;
    }
    pModel->mScrolledBackTo = line < 0 ? std::max(lastLine + line, 0) : line;
    return true;
}

// As TTextEdit::getColumnCount() and getRowCount() count it
void TNullConsoleFrontend::reportGridSize(const QString& name)
{
    TConsoleModel* pModel = subConsoleModel(name);
    if (!pModel) {
        return;
    }
    const QSize size = mpHost->windowRegistry().subConsoleGeometry(name).value_or(QRect()).size();
    const QFontMetricsF metrics(pModel->mUpperPaneFont);
    pModel->mGridSize = QSize(qRound(size.width() / metrics.averageCharWidth()), qRound(size.height() / metrics.lineSpacing()));
}

QFont TNullConsoleFrontend::displayFont() const
{
    return mpHost->getDisplayFont();
}

std::optional<QFont> TNullConsoleFrontend::consoleFont(const QString& name) const
{
    if (name.isEmpty() || name == qsl("main")) {
        return {displayFont()};
    }
    if (const TConsoleModel* pModel = subConsoleModel(name)) {
        return {pModel->mUpperPaneFont};
    }
    return {};
}

std::optional<QSize> TNullConsoleFrontend::consoleFontSize(const QString& name) const
{
    const auto font = consoleFont(name);
    if (!font) {
        return {};
    }
    const QFontMetrics metrics(*font);
    return {QSize(metrics.horizontalAdvance(QChar('W')), metrics.height())};
}

std::optional<int> TNullConsoleFrontend::consoleColumnWidth(const QString& name) const
{
    const auto font = consoleFont(name);
    if (!font) {
        return {};
    }
    return {QFontMetrics(*font).averageCharWidth()};
}

// As TConsole::setFont() changes a sub-console's, and tells scripts of it
void TNullConsoleFrontend::setSubConsoleFont(const QString& name, const QFont& font)
{
    TConsoleModel* pModel = subConsoleModel(name);
    if (!pModel) {
        return;
    }
    pModel->mUpperPaneFont = font;
    reportGridSize(name);
    if (mpHost->windowRegistry().subConsoleKind(name) == TWindowRegistry::SubConsoleKind::Buffer) {
        return;
    }
    raiseFontEvent(qsl("sysFontChangeEvent"), name, font);
}

// As TConsole::setFont() and Host::updateConsolesFont() tell scripts of a new main console font
void TNullConsoleFrontend::reportDisplayFontChange(const QFont& before)
{
    const QFont font = displayFont();
    if (TFontAttributes(font) == TFontAttributes(before)) {
        return;
    }
    raiseFontEvent(qsl("sysFontChangeEvent"), qsl("main"), font);
    // Read afresh, as Host::updateConsolesFont() does: a sysFontChangeEvent handler may have changed it again
    raiseFontEvent(qsl("sysSettingChanged"), qsl("main window font"), displayFont());
}

void TNullConsoleFrontend::raiseFontEvent(const QString& eventName, const QString& subject, const QFont& font)
{
    TEvent event{};
    event.mArgumentList.append(eventName);
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(subject);
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(font.family());
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(QString::number(font.pointSize()));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    mpHost->raiseEvent(event);
}

std::optional<std::pair<bool, QString>> TNullConsoleFrontend::setWindowFontFamily(const QString& name, const QString& family, const QFont::Weight weight)
{
    if (name.isEmpty() || name == qsl("main")) {
        QFont font = mpHost->createFontWithSettings(family, mpHost->getDisplayFont().pointSize());
        if (weight != QFont::Normal) {
            font.setWeight(weight);
        }
        const QFont before = displayFont();
        const auto result = mpHost->setDisplayFont(font, Host::DisplayFontChange::UserChoice);
        reportDisplayFontChange(before);
        return {result};
    }
    TConsoleModel* pModel = subConsoleModel(name);
    if (!pModel) {
        return {};
    }
    QFont font = mpHost->createFontWithSettings(family, pModel->mUpperPaneFont.pointSize());
    if (weight != QFont::Normal) {
        font.setWeight(weight);
    }
    if (TFontAttributes(font) != TFontAttributes(pModel->mUpperPaneFont)) {
        setSubConsoleFont(name, font);
    }
    return {{true, QString()}};
}

bool TNullConsoleFrontend::setWindowFontSize(const QString& name, const int size)
{
    if (name.isEmpty() || name == qsl("main")) {
        const QFont before = displayFont();
        mpHost->setDisplayFontSize(size);
        reportDisplayFontChange(before);
        return true;
    }
    TConsoleModel* pModel = subConsoleModel(name);
    if (!pModel) {
        return false;
    }
    if (pModel->mUpperPaneFont.pointSize() != size) {
        TFontAttributes font(pModel->mUpperPaneFont);
        font.mPointSize = size;
        setSubConsoleFont(name, font.makeFont());
    }
    return true;
}
