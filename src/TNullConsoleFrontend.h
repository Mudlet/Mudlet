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
#include "utils.h"

#include <QColor>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QString>

#include <map>
#include <memory>
#include <vector>

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
// them, echo to them and delete them. What a real view would record of them as it moves, shows,
// styles, titles or scrolls them, this records too, so a later getter agrees. Every other operation fails,
// has no value or does nothing; a close has nothing to refuse it.
class TNullConsoleFrontend final : public TConsoleFrontend
{
public:
    // Some callers hand a failure's message straight to Lua, so it has to say why.
    static std::pair<bool, QString> noView() { return {false, qsl("the profile has no main window")}; }

    explicit TNullConsoleFrontend(Host* pHost);
    ~TNullConsoleFrontend();

    void dropWindows();
    // A deleted window's model can still be in use, as a sysBufferShrinkEvent handler runs inside its
    // buffer's append(), so models are kept until deferred deletes next run, as a real view's widgets are
    // by deleteLater(). Host calls this itself as it goes, while its Lua interpreter is still alive, since
    // a label's model frees its callbacks' Lua references when it goes.
    void releaseRetired();

    void createLabel(const QString& windowname, const QString& name, int x, int y, int width, int height, bool fillBackground, bool clickThrough) override;
    void deleteLabel(const QString& name) override;
    std::pair<bool, QString> setLabelStyleSheet(const QString& name, const QString& stylesheet) override;
    std::optional<QSize> getLabelSizeHint(const QString& name) const override;
    std::pair<bool, QString> setLabelToolTip(const QString& name, const QString& text, double duration) override;
    std::pair<bool, QString> setLabelCursor(const QString& name, int shape) override;
    std::pair<bool, QString> setLabelCustomCursor(const QString& name, const QString& pixMapLocation, int hotX, int hotY) override;
    bool setLabelClickThrough(const QString& name, bool) override { return labelModel(name) != nullptr; }
    bool setLabelLinkStyle(const QString& name, const QString& linkColor, const QString& linkVisitedColor, bool underline) override;
    bool resetLabelLinkStyle(const QString& name) override;
    bool clearLabelVisitedLinks(const QString& name) override;
    bool showLabel(const QString& name) override { return setLabelShown(name, true); }
    bool hideLabel(const QString& name) override { return setLabelShown(name, false); }
    bool resizeLabel(const QString& name, int width, int height) override;
    bool moveLabel(const QString& name, int x, int y) override;
    bool reparentLabel(const QString& windowname, const QString& name, int x, int y, bool show) override;
    bool setLabelText(const QString& name, const QString& text) override;
    std::pair<bool, QString> setLabelMovie(const QString&, const QString&) override { return noView(); }
    bool setLabelBackgroundColor(const QString& name, const QColor& color) override;
    std::optional<QColor> getLabelBackgroundColor(const QString& name) const override;
    bool setLabelBackgroundImage(const QString&, const QString&) override { return false; }
    bool resetLabelBackgroundImage(const QString&) override { return false; }
    bool setLabelSvgTint(const QString& name, const QColor& color) override;
    bool resetLabelSvgTint(const QString& name) override { return setLabelSvgTint(name, QColor()); }
    bool setLabelSvgRotation(const QString& name, double angle) override;
    bool resetLabelSvgRotation(const QString& name) override { return setLabelSvgRotation(name, 0.0); }
    bool setLabelSvgShear(const QString& name, double shearX, double shearY) override;
    bool resetLabelSvgShear(const QString& name) override { return setLabelSvgShear(name, 0.0, 0.0); }
    bool resetLabelSvgTransform(const QString& name) override { return setLabelSvgRotation(name, 0.0) && setLabelSvgShear(name, 0.0, 0.0); }
    bool setLabelFont(const QString& name, const QFont& font) override;
    std::optional<bool> labelShowsMovie(const QString&) const override { return std::nullopt; }
    bool startLabelMovie(const QString&) override { return false; }
    bool pauseLabelMovie(const QString&) override { return false; }
    bool setLabelMovieFrame(const QString&, int) override { return false; }
    bool setLabelMovieSpeed(const QString&, int) override { return false; }
    bool scaleLabelMovie(const QString&, bool) override { return false; }
    std::pair<bool, QString> createCommandLine(const QString&, const QString&, int, int, int, int) override { return noView(); }
    std::pair<bool, QString> deleteCommandLine(const QString&) override { return noView(); }
    bool setCommandLineAction(const QString&, int) override { return false; }
    bool resetCommandLineAction(const QString&) override { return false; }
    std::pair<bool, QString> setCmdLineStyleSheet(const QString&, const QString&) override { return noView(); }
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
    std::pair<bool, QString> createTextBox(const QString&, const QString&, int, int, int, int) override { return noView(); }
    std::pair<bool, QString> deleteTextBox(const QString&) override { return noView(); }
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
    bool showSubConsole(const QString& name) override { return setSubConsoleShown(name, true); }
    bool hideSubConsole(const QString& name) override { return setSubConsoleShown(name, false); }
    bool resizeSubConsole(const QString& name, int width, int height) override;
    bool moveSubConsole(const QString& name, int x, int y) override;
    void closeSubConsole(const QString& name) override;
    void openUserWindow(const QString& name, bool loadLayout, bool autoDock, const QString& area) override;
    std::pair<bool, QString> setUserWindowStyleSheet(const QString& name, const QString& userWindowStyleSheet) override;
    std::pair<bool, QString> setUserWindowTitle(const QString& name, const QString& text) override;
    bool createScrollBox(const QString&, const QString&, int, int, int, int) override { return false; }
    std::pair<bool, QString> deleteScrollBox(const QString&) override { return noView(); }
    bool showPlainWindow(const QString&) override { return false; }
    bool hidePlainWindow(const QString&) override { return false; }
    bool resizePlainWindow(const QString&, int, int) override { return false; }
    bool movePlainWindow(const QString&, int, int) override { return false; }
    bool raiseWindow(const QString&) override { return false; }
    bool lowerWindow(const QString&) override { return false; }
    std::pair<bool, QString> reparentWindow(const QString&, const QString&, int, int, bool) override { return noView(); }
    bool setWindowScrollBarVisible(const QString& name, bool visible) override;
    bool setWindowHorizontalScrollBarVisible(const QString& name, bool visible) override;
    bool setWindowScrolling(const QString& name, bool enabled) override;
    bool scrollWindowTo(const QString& name, int line, bool toEnd) override;
    std::optional<std::pair<bool, QString>> setWindowFontFamily(const QString& name, const QString& family, QFont::Weight weight) override;
    bool setWindowFontSize(const QString& name, int size) override;
    std::optional<QSize> consoleFontSize(const QString& name) const override;
    std::optional<int> consoleColumnWidth(const QString& name) const override;
    void setConsoleBgColor(int, int, int, int) override {}
    bool setConsoleBackgroundImage(const QString&, int) override { return false; }
    bool resetConsoleBackgroundImage() override { return false; }
    bool setWindowBackgroundImage(const QString&, int) override { return false; }
    bool resetWindowBackgroundImage() override { return false; }
    void setBorderColor(const QColor& color) override;
    void changeColors() override {}
    void setProfileStyleSheet(const QString&) override {}
    void applyBorders() override {}
    QFont displayFont() const override;
    void setFont(const QFont&) override {}
    void setFontSize(int) override {}
    bool setSubConsoleBackgroundColor(const QString& name, const QColor& color) override;
    bool setSubConsoleBackgroundImage(const QString&, const QString&, int) override { return false; }
    bool resetSubConsoleBackgroundImage(const QString&) override { return false; }
    bool setSubConsoleCommandBackgroundColor(const QString& name, const QColor& color) override;
    bool setSubConsoleCommandForegroundColor(const QString& name, const QColor& color) override;
    void changeSubConsoleColors(const QString&) override {}
    std::pair<bool, QString> createMapper(const QString&, int, int, int, int) override { return noView(); }
    void createMapperDock(const QString&, const QString&) override {}
    void showNewMapperDock() override {}
    std::pair<bool, QString> placeMapWidget(const QString&, int, int, int, int) override { return noView(); }
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
    // A window made inside a user window goes with it, as a real one is its dock's child widget, though
    // out of the registry at once rather than when deferred deletes run.
    // shown is the window's own flag, as QWidget::isHidden() is a widget's; what scripts read is also
    // false while the user window holding it is hidden.
    struct SubConsole
    {
        std::unique_ptr<TConsoleModel> pModel;
        QString userWindow;
        bool shown = false;
    };
    struct Label
    {
        std::unique_ptr<TLabelModel> pModel;
        QString userWindow;
        bool shown = true;
        // What a real label's QWidget and QStyleSheetStyle keep beside its font: which of its
        // attributes were set on it, and the font each restyle starts again from
        uint fontMask = 0;
        std::optional<QFont> savedFont;
        // The font properties the label's own sheet declares, if it declares any
        std::optional<QFont> sheetFont;
        // Qt::WA_StyleSheet
        bool styled = false;
        // TLabel::mPaletteSetSinceStyled
        bool restyleDue = false;
        // QLabel only measures its text once it has been given some, and caches the answer
        bool textLabel = false;
        mutable std::optional<QSize> sizeHint;
    };

    TConsoleModel& addSubConsole(const QString& name, TWindowRegistry::SubConsoleKind kind, const QString& windowname);
    void removeSubConsole(const QString& name);
    void removeLabel(const QString& name);
    void queueRelease();
    QString userWindowOrMain(const QString& windowname) const;
    TLabelModel* labelModel(const QString& name) const;
    TConsoleModel* subConsoleModel(const QString& name) const;
    bool setLabelShown(const QString& name, bool shown);
    QFont naturalLabelFont(const Label& label) const;
    void setLabelWidgetFont(Label& label, const QFont& font, bool applySheet);
    void polishLabel(Label& label);
    void restyleLabel(Label& label, const QString& sheet);
    bool setSubConsoleShown(const QString& name, bool shown);
    void reportVisibility(const QString& name);
    std::optional<QFont> consoleFont(const QString& name) const;
    void setSubConsoleFont(const QString& name, const QFont& font);
    void reportDisplayFontChange(const QFont& before);
    void raiseFontEvent(const QString& eventName, const QString& subject, const QFont& font);
    void reportGridSize(const QString& name);

    Host* mpHost = nullptr;
    TNullMxpFrameFrontend mMxpFrames;
    std::map<QString, SubConsole> mSubConsoles;
    std::map<QString, Label> mLabels;
    std::vector<std::unique_ptr<TConsoleModel>> mRetiredConsoles;
    std::vector<std::unique_ptr<TLabelModel>> mRetiredLabels;
    bool mReleaseQueued = false;
};

#endif // MUDLET_TNULLCONSOLEFRONTEND_H
