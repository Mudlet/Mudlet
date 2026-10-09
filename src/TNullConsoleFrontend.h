#ifndef MUDLET_TNULLCONSOLEFRONTEND_H
#define MUDLET_TNULLCONSOLEFRONTEND_H

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

#include "TConsoleFrontend.h"
#include "TMxpFrameFrontend.h"
#include "TWindowRegistry.h"

#include <QColor>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QString>

#include <map>
#include <memory>

class Host;
struct TLabelModel;

class TNullMxpFrameFrontend final : public TMxpFrameFrontend
{
public:
    void createInternalFrame(const QString&, const QString&, const QString&, const QRect&, bool, bool) override {}
    std::optional<QSize> createExternalFrame(const QString&, const QString&, const QSize&, bool) override { return std::nullopt; }
    void createTabFrame(const QString&, const QString&, const QString&, const QSize&, bool, bool) override {}
    bool removeFromParentTabs(const QString&, const QString&) override { return false; }
    void destroyFrame(const QString&) override {}
    void showFrame(const QString&) override {}
    void focusFrame(const QString&) override {}
    void setGeometry(const QString&, const QRect&) override {}
    void reportSize() override {}
    TPrintSink* sink(const QString&) const override { return nullptr; }
    bool hasFrameWidget(const QString&) const override { return false; }
};

// The view a Host has while no real one is attached: before its console is made, after that has
// gone, and in a run with no GUI. Labels, mini consoles, buffers and user windows made through it
// get a model of their own, registered as a real view registers its widgets', so scripts can find
// them, echo to them and delete them. Every other window is missing, so each call fails, has no
// value or does nothing; a close has nothing to refuse it.
class TNullConsoleFrontend final : public TConsoleFrontend
{
public:
    explicit TNullConsoleFrontend(Host* pHost);
    ~TNullConsoleFrontend();

    // Host calls this while its Lua interpreter is still alive, as a label's model frees its callbacks'
    // Lua references when it goes and Host destroys the interpreter before this.
    void dropWindows();

    void createLabel(const QString& windowname, const QString& name, int x, int y, int width, int height, bool fillBackground, bool clickThrough) override;
    void deleteLabel(const QString& name) override;
    std::pair<bool, QString> setLabelStyleSheet(const QString&, const QString&) override { return {false, QString()}; }
    std::optional<QSize> getLabelSizeHint(const QString&) const override { return std::nullopt; }
    std::pair<bool, QString> setLabelToolTip(const QString&, const QString&, double) override { return {false, QString()}; }
    std::pair<bool, QString> setLabelCursor(const QString&, int) override { return {false, QString()}; }
    std::pair<bool, QString> setLabelCustomCursor(const QString&, const QString&, int, int) override { return {false, QString()}; }
    bool setLabelClickThrough(const QString&, bool) override { return false; }
    bool setLabelLinkStyle(const QString&, const QString&, const QString&, bool) override { return false; }
    bool resetLabelLinkStyle(const QString&) override { return false; }
    bool clearLabelVisitedLinks(const QString&) override { return false; }
    bool showLabel(const QString&) override { return false; }
    bool hideLabel(const QString&) override { return false; }
    bool resizeLabel(const QString&, int, int) override { return false; }
    bool moveLabel(const QString&, int, int) override { return false; }
    bool reparentLabel(const QString&, const QString&, int, int, bool) override { return false; }
    bool setLabelText(const QString& name, const QString& text) override;
    std::pair<bool, QString> setLabelMovie(const QString&, const QString&) override { return {false, QString()}; }
    bool setLabelBackgroundColor(const QString&, const QColor&) override { return false; }
    std::optional<QColor> getLabelBackgroundColor(const QString&) const override { return std::nullopt; }
    bool setLabelBackgroundImage(const QString&, const QString&) override { return false; }
    bool resetLabelBackgroundImage(const QString&) override { return false; }
    bool setLabelSvgTint(const QString&, const QColor&) override { return false; }
    bool resetLabelSvgTint(const QString&) override { return false; }
    bool setLabelSvgRotation(const QString&, double) override { return false; }
    bool resetLabelSvgRotation(const QString&) override { return false; }
    bool setLabelSvgShear(const QString&, double, double) override { return false; }
    bool resetLabelSvgShear(const QString&) override { return false; }
    bool resetLabelSvgTransform(const QString&) override { return false; }
    bool setLabelFont(const QString&, const QFont&) override { return false; }
    std::optional<bool> labelShowsMovie(const QString&) const override { return std::nullopt; }
    bool startLabelMovie(const QString&) override { return false; }
    bool pauseLabelMovie(const QString&) override { return false; }
    bool setLabelMovieFrame(const QString&, int) override { return false; }
    bool setLabelMovieSpeed(const QString&, int) override { return false; }
    bool scaleLabelMovie(const QString&, bool) override { return false; }
    std::pair<bool, QString> createCommandLine(const QString&, const QString&, int, int, int, int) override { return {false, QString()}; }
    std::pair<bool, QString> deleteCommandLine(const QString&) override { return {false, QString()}; }
    bool setCommandLineAction(const QString&, int) override { return false; }
    bool resetCommandLineAction(const QString&) override { return false; }
    std::pair<bool, QString> setCmdLineStyleSheet(const QString&, const QString&) override { return {false, QString()}; }
    bool replaceCommandLineText(const QString&, const QString&) override { return false; }
    bool appendCommandLineText(const QString&, const QString&) override { return false; }
    bool clearCommandLine(const QString&) override { return false; }
    bool selectCommandLineText(const QString&) override { return false; }
    bool addCommandLineSuggestion(const QString&, const QString&) override { return false; }
    bool removeCommandLineSuggestion(const QString&, const QString&) override { return false; }
    bool clearCommandLineSuggestions(const QString&) override { return false; }
    bool addCommandLineBlacklistWord(const QString&, const QString&) override { return false; }
    bool removeCommandLineBlacklistWord(const QString&, const QString&) override { return false; }
    bool clearCommandLineBlacklist(const QString&) override { return false; }
    bool addCommandLineMenuItem(const QString&, const QString&, const QString&) override { return false; }
    std::optional<bool> removeCommandLineMenuItem(const QString&, const QString&) override { return std::nullopt; }
    bool setCommandLineSavesHistory(const QString&, bool) override { return false; }
    bool setCommandLineVisible(const QString&, bool) override { return false; }
    bool setWindowCommandLineVisible(const QString&, bool) override { return false; }
    void setCommandLinePlaceholderText(const QString&) override {}
    void updateCommandLineSpellCheck(bool) override {}
    void setCommandLineText(const QString&) override {}
    void focusActiveCommandLine() override {}
    std::pair<bool, QString> createTextBox(const QString&, const QString&, int, int, int, int) override { return {false, QString()}; }
    std::pair<bool, QString> deleteTextBox(const QString&) override { return {false, QString()}; }
    bool setTextBoxText(const QString&, const QString&) override { return false; }
    bool clearTextBox(const QString&) override { return false; }
    bool setTextBoxReadOnly(const QString&, bool) override { return false; }
    bool setTextBoxPlaceholder(const QString&, const QString&) override { return false; }
    bool setTextBoxStyleSheet(const QString&, const QString&) override { return false; }
    bool setTextBoxFont(const QString&, const QFont&) override { return false; }
    bool setTextBoxTabMovesFocus(const QString&, bool) override { return false; }
    void createBuffer(const QString& name) override;
    void addMiniConsole(const QString& windowname, const QString& name, int x, int y, int width, int height) override;
    void deleteMiniConsole(const QString& name) override;
    bool showSubConsole(const QString&) override { return false; }
    bool hideSubConsole(const QString&) override { return false; }
    bool resizeSubConsole(const QString&, int, int) override { return false; }
    bool moveSubConsole(const QString&, int, int) override { return false; }
    void closeSubConsole(const QString& name) override;
    void openUserWindow(const QString& name, bool loadLayout, bool autoDock, const QString& area) override;
    std::pair<bool, QString> setUserWindowStyleSheet(const QString&, const QString&) override { return {false, QString()}; }
    std::pair<bool, QString> setUserWindowTitle(const QString&, const QString&) override { return {false, QString()}; }
    bool createScrollBox(const QString&, const QString&, int, int, int, int) override { return false; }
    std::pair<bool, QString> deleteScrollBox(const QString&) override { return {false, QString()}; }
    bool showPlainWindow(const QString&) override { return false; }
    bool hidePlainWindow(const QString&) override { return false; }
    bool resizePlainWindow(const QString&, int, int) override { return false; }
    bool movePlainWindow(const QString&, int, int) override { return false; }
    bool raiseWindow(const QString&) override { return false; }
    bool lowerWindow(const QString&) override { return false; }
    std::pair<bool, QString> reparentWindow(const QString&, const QString&, int, int, bool) override { return {false, QString()}; }
    bool setWindowScrollBarVisible(const QString&, bool) override { return false; }
    bool setWindowHorizontalScrollBarVisible(const QString&, bool) override { return false; }
    bool setWindowScrolling(const QString&, bool) override { return false; }
    bool scrollWindowTo(const QString&, int, bool) override { return false; }
    std::optional<std::pair<bool, QString>> setWindowFontFamily(const QString&, const QString&, QFont::Weight) override { return std::nullopt; }
    bool setWindowFontSize(const QString&, int) override { return false; }
    std::optional<QSize> consoleFontSize(const QString&) const override { return std::nullopt; }
    void setConsoleBgColor(int, int, int, int) override {}
    bool setConsoleBackgroundImage(const QString&, int) override { return false; }
    bool resetConsoleBackgroundImage() override { return false; }
    bool setWindowBackgroundImage(const QString&, int) override { return false; }
    bool resetWindowBackgroundImage() override { return false; }
    void setBorderColor(const QColor&) override {}
    void changeColors() override {}
    void setProfileStyleSheet(const QString&) override {}
    void applyBorders() override {}
    QFont displayFont() const override { return QFont(); }
    void setFont(const QFont&) override {}
    void setFontSize(int) override {}
    bool setSubConsoleBackgroundColor(const QString&, const QColor&) override { return false; }
    bool setSubConsoleBackgroundImage(const QString&, const QString&, int) override { return false; }
    bool resetSubConsoleBackgroundImage(const QString&) override { return false; }
    bool setSubConsoleCommandBackgroundColor(const QString&, const QColor&) override { return false; }
    bool setSubConsoleCommandForegroundColor(const QString&, const QColor&) override { return false; }
    void changeSubConsoleColors(const QString&) override {}
    std::pair<bool, QString> createMapper(const QString&, int, int, int, int) override { return {false, QString()}; }
    void createMapperDock(const QString&, const QString&) override {}
    void showNewMapperDock() override {}
    std::pair<bool, QString> placeMapWidget(const QString&, int, int, int, int) override { return {false, QString()}; }
    bool mapWidgetCreated() const override { return false; }
    bool setMapWidgetTitle(const QString&) override { return false; }
    std::optional<QString> mapWidgetTitle() const override { return std::nullopt; }
    std::optional<QRect> mapWidgetGeometry() const override { return std::nullopt; }
    bool hideMapWidget() override { return false; }
    void restoreOwnMapper() override {}
    void showLoadedMap() override {}
    void showMapAfterFailedLoad() override {}
    void showMapAtPlayerArea() override {}
    bool mapperShown() const override { return false; }
    void setMapperShown(bool) override {}
    void setMapperPanelVisible(bool) override {}
    void setMapLargeAreaExitArrows(bool) override {}
    void requestMapRepaint() override {}
    void regenerateToolBars(const std::list<TAction*>&) override {}
    void regenerateEasyButtonBars(const std::list<TAction*>&) override {}
    void detachActionBars(TAction*) override {}
    bool hasEasyButtonBar(TAction*) const override { return false; }
    void setActionButtonChecked(TAction*, bool) override {}
    bool restyleActionButton(TAction*) override { return false; }
    void releaseParentActionBars(TAction*, TAction*) override {}
    void renameActionToolBar(TAction*, const QString&) override {}
    void deleteActionToolBars() override {}
    void deleteActionToolBarsLater() override {}
    bool commitToolBarLayoutChanges() override { return false; }
    void discardToolBarLayoutChanges() override {}
    void setDockLayoutChanged(const QString&) override {}
    bool clearDockLayoutChanged(const QString&) override { return false; }
    bool startIncomingText() override { return false; }
    void alertNewData() override {}
    void finishIncomingText(bool) override {}
    void finalize() override {}
    void showNewLines() override {}
    void showCommandEcho(const TConsoleModel::CommandEcho&) override {}
    void markSelectionDirty() override {}
    void refreshView() const override {}
    void requestRepaintAfterCommand() override {}
    bool requestClose() override { return true; }
    void resetMainConsole() override { dropWindows(); }
    void setProfileName(const QString&) override {}
    void setF3SearchEnabled(bool) override {}
    void setCompactInputLine(bool) override {}
    void setCaretMode(bool) override {}
    QPoint mousePosition() const override { return QPoint(); }
    bool hasKeyboardFocus() const override { return false; }
    TMxpFrameFrontend& mxpFrames() override { return mMxpFrames; }
    const TMxpFrameFrontend& mxpFrames() const override { return mMxpFrames; }

private:
    // A window made into a user window goes with it, as a real one is its dock's child widget.
    struct SubConsole
    {
        std::unique_ptr<TConsoleModel> pModel;
        QString userWindow;
    };
    struct Label
    {
        std::unique_ptr<TLabelModel> pModel;
        QString userWindow;
    };

    TConsoleModel& addSubConsole(const QString& name, TWindowRegistry::SubConsoleKind kind, const QString& windowname);
    void removeSubConsole(const QString& name);
    void removeLabel(const QString& name);
    QString userWindowOrMain(const QString& windowname) const;

    Host* mpHost = nullptr;
    TNullMxpFrameFrontend mMxpFrames;
    std::map<QString, SubConsole> mSubConsoles;
    std::map<QString, Label> mLabels;
};

#endif // MUDLET_TNULLCONSOLEFRONTEND_H
