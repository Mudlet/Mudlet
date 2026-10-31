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
#include "TNullMapView.h"
#include "TPrintSink.h"
#include "TWindowRegistry.h"
#include "utils.h"

#include <QColor>
#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QSize>
#include <QString>

#include <array>
#include <map>
#include <memory>
#include <vector>

class Host;
class QDataStream;
class QMovie;
class TNullConsoleFrontend;
struct TLabelModel;

// Writes to a model what TConsole's own print sink writes to a widget's
class TConsoleModelSink final : public TPrintSink
{
public:
    explicit TConsoleModelSink(TConsoleModel& model)
    : mModel(model)
    {
    }

    void printFormatted(const QString& text, const std::vector<TChar>& formatting, const TLinkStore& sourceLinkStore) override;
    void discardAll() override;
    void discardLastLine() override;

private:
    TConsoleModel& mModel;
};

// Each frame gets a mini console model of the null view's under the frame's name, as a real frame gets a
// mini console, so <DEST> text lands where scripts can read it. A frame has no tab header here, so one
// docked into another is opened as a frame of its own.
class TNullMxpFrameFrontend final : public TMxpFrameFrontend
{
public:
    explicit TNullMxpFrameFrontend(TNullConsoleFrontend& view)
    : mView(view)
    {
    }

    void createInternalFrame(const QString& name, const QString&, const QString&, const QRect&, bool, bool) override;
    std::optional<QSize> createExternalFrame(const QString& name, const QString&, const QSize& size, bool) override;
    void createTabFrame(const QString& name, const QString&, const QString&, const QSize&, bool, bool) override;
    bool removeFromParentTabs(const QString&, const QString&) override { return false; }
    void destroyFrame(const QString& name) override;
    void showFrame(const QString&) override {}
    void focusFrame(const QString&) override {}
    void setGeometry(const QString&, const QRect&) override {}
    void reportSize() override {}
    TPrintSink* sink(const QString& name) const override;
    bool hasFrameWidget(const QString& name) const override;

private:
    TNullConsoleFrontend& mView;
};

// The view a Host has while no real one is attached: before its console is made, after that has
// gone, and in a run with no GUI. Labels, mini consoles, buffers, user windows and MXP frames made through
// it get a model of their own, registered as a real view registers its widgets', so scripts can find
// them, echo to them and delete them. What a real view would record of them as it moves, shows,
// styles, titles or scrolls them, this records too, so a later getter agrees. Every other operation fails,
// has no value or does nothing; a close has nothing to refuse it.
class TNullConsoleFrontend : public TConsoleFrontend
{
public:
    // Some callers hand a failure's message straight to Lua, so it has to say why.
    static std::pair<bool, QString> noView() { return {false, qsl("the profile has no main window")}; }

    struct Placement
    {
        QRect geometry;
        bool shown = false;

        friend QDataStream& operator<<(QDataStream&, const Placement&);
        friend QDataStream& operator>>(QDataStream&, Placement&);
    };

    explicit TNullConsoleFrontend(Host* pHost);
    // Virtual, as Host deletes whichever view it holds as this type
    virtual ~TNullConsoleFrontend();

    void dropWindows();
    // For Host's constructor, as TConsole's constructor makes the main command line ahead of any other
    void makeMainCommandLine(bool savesHistory);
    // The main command line's history writer goes as a view attaches, with the main window's stand-in that is
    // its parent. Once the view goes, these make it again, so the view's last setting is still saved.
    void followView(QObject* view);
    void resumeMainCommandLine();
    bool ownsContainer(const QString& windowname) const;
    bool subConsoleShown(const QString& name) const;
    void reportUserWindowVisibility();
    bool holdsWindow(const QString& name) const { return mLabels.contains(name) || subConsoleModel(name) || mScrollBoxes.contains(name) || mCommandLines.contains(name) || mTextBoxes.contains(name); }
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
    std::pair<bool, QString> setLabelMovie(const QString& name, const QString& moviePath) override;
    bool setLabelBackgroundColor(const QString& name, const QColor& color) override;
    std::optional<QColor> getLabelBackgroundColor(const QString& name) const override;
    bool setLabelBackgroundImage(const QString& name, const QString& path) override;
    bool resetLabelBackgroundImage(const QString& name) override;
    bool setLabelSvgTint(const QString& name, const QColor& color) override;
    bool resetLabelSvgTint(const QString& name) override { return setLabelSvgTint(name, QColor()); }
    bool setLabelSvgRotation(const QString& name, double angle) override;
    bool resetLabelSvgRotation(const QString& name) override { return setLabelSvgRotation(name, 0.0); }
    bool setLabelSvgShear(const QString& name, double shearX, double shearY) override;
    bool resetLabelSvgShear(const QString& name) override { return setLabelSvgShear(name, 0.0, 0.0); }
    bool resetLabelSvgTransform(const QString& name) override { return setLabelSvgRotation(name, 0.0) && setLabelSvgShear(name, 0.0, 0.0); }
    bool setLabelFont(const QString& name, const QFont& font) override;
    std::optional<bool> labelShowsMovie(const QString& name) const override;
    bool startLabelMovie(const QString& name) override;
    bool pauseLabelMovie(const QString& name) override;
    bool setLabelMovieFrame(const QString& name, int frame) override;
    bool setLabelMovieSpeed(const QString& name, int percent) override;
    bool scaleLabelMovie(const QString& name, bool followLabelSize) override;
    std::pair<bool, QString> createCommandLine(const QString& windowname, const QString& name, int x, int y, int width, int height) override;
    std::pair<bool, QString> deleteCommandLine(const QString& name) override;
    bool setCommandLineAction(const QString& name, int func) override;
    bool resetCommandLineAction(const QString& name) override;
    std::pair<bool, QString> setCmdLineStyleSheet(const QString& name, const QString& styleSheet) override;
    bool replaceCommandLineText(const QString& name, const QString& text) override;
    bool appendCommandLineText(const QString& name, const QString& text) override;
    bool clearCommandLine(const QString& name) override { return replaceCommandLineText(name, QString()); }
    // Only typing and tab completion read what these set, and with no view nothing is typed
    bool selectCommandLineText(const QString& name) override { return ownsCommandLine(name); }
    bool addCommandLineSuggestion(const QString& name, const QString&) override { return ownsCommandLine(name); }
    bool removeCommandLineSuggestion(const QString& name, const QString&) override { return ownsCommandLine(name); }
    bool clearCommandLineSuggestions(const QString& name) override { return ownsCommandLine(name); }
    bool addCommandLineBlacklistWord(const QString& name, const QString&) override { return ownsCommandLine(name); }
    bool removeCommandLineBlacklistWord(const QString& name, const QString&) override { return ownsCommandLine(name); }
    bool clearCommandLineBlacklist(const QString& name) override { return ownsCommandLine(name); }
    bool addCommandLineMenuItem(const QString& name, const QString& label, const QString& eventName) override;
    std::optional<bool> removeCommandLineMenuItem(const QString& name, const QString& label) override;
    bool setCommandLineSavesHistory(const QString& name, bool savesHistory) override;
    bool setCommandLineVisible(const QString& name, bool visible) override;
    bool setWindowCommandLineVisible(const QString& name, bool visible) override;
    void setCommandLinePlaceholderText(const QString&) override {}
    void updateCommandLineSpellCheck(bool) override {}
    void setCommandLineText(const QString& text) override { replaceCommandLineText(QString(), text); }
    void focusActiveCommandLine() override {}
    std::pair<bool, QString> createTextBox(const QString& windowname, const QString& name, int x, int y, int width, int height) override;
    std::pair<bool, QString> deleteTextBox(const QString& name) override;
    bool setTextBoxText(const QString& name, const QString& text) override;
    bool clearTextBox(const QString& name) override { return setTextBoxText(name, QString()); }
    bool setTextBoxReadOnly(const QString& name, bool readOnly) override;
    // What these set changes nothing a Lua call reads back
    bool setTextBoxPlaceholder(const QString& name, const QString&) override { return mTextBoxes.contains(name); }
    bool setTextBoxStyleSheet(const QString& name, const QString& styleSheet) override;
    bool setTextBoxTabMovesFocus(const QString& name, bool) override { return mTextBoxes.contains(name); }
    bool setTextBoxFont(const QString& name, const QFont& font) override;
    void createBuffer(const QString& name) override;
    void addMiniConsole(const QString& windowname, const QString& name, int x, int y, int width, int height) override;
    void deleteMiniConsole(const QString& name) override;
    bool showSubConsole(const QString& name) override { return setSubConsoleShown(name, true); }
    bool hideSubConsole(const QString& name) override { return setSubConsoleShown(name, false); }
    bool resizeSubConsole(const QString& name, int width, int height) override;
    bool moveSubConsole(const QString& name, int x, int y) override;
    void closeSubConsole(const QString& name) override;
    bool openUserWindow(const QString& name, bool loadLayout, bool autoDock, const QString& area) override;
    std::pair<bool, QString> setUserWindowStyleSheet(const QString& name, const QString& userWindowStyleSheet) override;
    std::pair<bool, QString> setUserWindowTitle(const QString& name, const QString& text) override;
    bool createScrollBox(const QString& windowname, const QString& name, int x, int y, int width, int height) override;
    std::pair<bool, QString> deleteScrollBox(const QString& name) override;
    // A scroll box, a command line or a text box
    bool showPlainWindow(const QString& name) override { return setPlainWindowShown(name, true); }
    bool hidePlainWindow(const QString& name) override { return setPlainWindowShown(name, false); }
    bool resizePlainWindow(const QString& name, int width, int height) override;
    bool movePlainWindow(const QString& name, int x, int y) override;
    // No Lua call reads the stacking order back, so these only answer whether the window exists
    bool raiseWindow(const QString& name) override { return holdsWindow(name) || namesEmbeddedMapper(name); }
    bool lowerWindow(const QString& name) override { return raiseWindow(name); }
    std::pair<bool, QString> reparentWindow(const QString& windowname, const QString& name, int x, int y, bool show) override;
    bool setWindowScrollBarVisible(const QString& name, bool visible) override;
    bool setWindowHorizontalScrollBarVisible(const QString& name, bool visible) override;
    bool setWindowScrolling(const QString& name, bool enabled) override;
    bool scrollWindowTo(const QString& name, int line, bool toEnd) override;
    bool windowFollowsNewLines(const QString& name) override;
    std::optional<std::pair<bool, QString>> setWindowFontFamily(const QString& name, const QString& family, QFont::Weight weight) override;
    bool setWindowFontSize(const QString& name, int size) override;
    std::optional<QSize> consoleFontSize(const QString& name) const override;
    std::optional<int> consoleColumnWidth(const QString& name) const override;
    void setConsoleBgColor(int, int, int, int) override {}
    // TConsole::setConsoleBackgroundImage() builds a stylesheet for the image without loading it
    bool setConsoleBackgroundImage(const QString&, int mode) override { return mode >= 1 && mode <= 4; }
    bool resetConsoleBackgroundImage() override { return true; }
    bool setWindowBackgroundImage(const QString& imgPath, int mode) override;
    bool resetWindowBackgroundImage() override { return true; }
    void setBorderColor(const QColor& color) override;
    void changeColors() override {}
    void setProfileStyleSheet(const QString& styleSheet) override;
    void applyBorders() override {}
    QFont displayFont() const override;
    void setFont(const QFont&) override {}
    void setFontSize(int) override {}
    bool setSubConsoleBackgroundColor(const QString& name, const QColor& color) override;
    bool setSubConsoleBackgroundImage(const QString& name, const QString&, int) override { return subConsoleModel(name); }
    bool resetSubConsoleBackgroundImage(const QString& name) override { return subConsoleModel(name); }
    bool setSubConsoleCommandBackgroundColor(const QString& name, const QColor& color) override;
    bool setSubConsoleCommandForegroundColor(const QString& name, const QColor& color) override;
    void changeSubConsoleColors(const QString&) override {}
    // The embedded mapper's position, size and visibility are not kept, as no Lua call reads them back
    std::pair<bool, QString> createMapper(const QString& windowname, int, int, int, int) override;
    void createMapperDock(const QString& title, const QString&) override;
    void showNewMapperDock() override;
    std::pair<bool, QString> placeMapWidget(const QString& area, int x, int y, int width, int height) override;
    bool mapWidgetCreated() const override { return mMapWidget.has_value(); }
    bool setMapWidgetTitle(const QString& title) override;
    std::optional<QString> mapWidgetTitle() const override;
    std::optional<QRect> mapWidgetGeometry() const override;
    bool hideMapWidget() override;
    // Where the map widget is and whether it shows, even while hidden, for the window layout
    std::optional<Placement> mapWidgetPlacement() const;
    // As restoreState() places and shows or hides a dock, with no event of its own
    void restoreMapWidgetPlacement(const Placement&);
    // A map widget or an embedded mapper, either of which a real view would draw the map in
    bool hasMapper() const { return mapView() != nullptr; }
    TMapViewFrontend* mapView() const { return mpEmbeddedMapper ? mpEmbeddedMapper.data() : mMapWidget ? mMapWidget->pView.get() : nullptr; }
    void restoreOwnMapper() override {}
    // TMainConsole shows the dlgMapper drawing the map after a load; a closed map widget's dock keeps it hidden
    void showLoadedMap() override { showEmbeddedMapper(); }
    void showMapAfterFailedLoad() override { showEmbeddedMapper(); }
    void showMapAtPlayerArea() override {}
    bool mapperShown() const override { return false; }
    void setMapperShown(bool) override {}
    void setMapperPanelVisible(bool) override {}
    void setMapLargeAreaExitArrows(bool) override {}
    void requestMapRepaint() override {}
    void regenerateToolBars(const std::list<TAction*>&) override {}
    void regenerateEasyButtonBars(const std::list<TAction*>&) override {}
    void detachActionBars(TAction*) override {}
    void disposeActionBars(TAction*) override {}
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
    void resetMainConsole() override;
    void setProfileName(const QString&) override {}
    void setF3SearchEnabled(bool) override {}
    void setCompactInputLine(bool) override {}
    void setCaretMode(bool) override {}
    QPoint mousePosition() const override { return QPoint(); }
    bool hasKeyboardFocus() const override { return false; }
    TMxpFrameFrontend& mxpFrames() override { return mMxpFrames; }
    const TMxpFrameFrontend& mxpFrames() const override { return mMxpFrames; }

private:
    friend class TNullMxpFrameFrontend;

    // Lives as long as a TCommandLine would, as that writes its history setting on each end of session profile
    // save until it is destroyed. Each window that can hold one has a QObject standing for its widget, as a parent.
    class HistoryWriter : public QObject
    {
    public:
        using QObject::QObject;
        bool savesHistory = true;
    };

    // QWidget's minimumSize() and maximumSize(), which QStyleSheetStyle::setGeometry() sets from a sheet
    struct SizeLimits
    {
        QSize minimum{0, 0};
        // QWIDGETSIZE_MAX
        QSize maximum{(1 << 24) - 1, (1 << 24) - 1};
        // As QWidget::resize() bounds a size, the minimum winning
        QSize bounded(const QSize& size) const { return size.boundedTo(maximum).expandedTo(minimum); }
    };
    // A scroll box takes the font of the user window it is in, as does a label inside it with no style
    // sheet, but not a new label, whose background colour is a sheet, nor a mini console, which has its
    // own. Deleting it takes what it holds out of the registry once deferred deletes run, as
    // TScrollBox's children are destroyed with it then.
    struct ScrollBox
    {
        QString userWindow;
        std::shared_ptr<ScrollBox> box;
        bool shown = true;
        QPointer<QObject> holder;
    };
    // A window made inside a user window goes with it, as a real one is its dock's child widget, though
    // out of the registry at once rather than when deferred deletes run.
    // shown is the window's own flag, as QWidget::isHidden() is a widget's; what scripts read is also
    // false while the user window holding it is hidden.
    // A command line or a text box, whose text is in the registry, as a real one's is
    struct PlainWindow
    {
        QString userWindow;
        std::shared_ptr<ScrollBox> box;
        bool shown = true;
        // Its own, which the registry has only while TMainConsole::plainWindowWidget() would take this for the name
        QRect geometry;
        // A command line's TCommandLine::mActionFunction, a Lua registry index
        int action = 0;
        // A command line's TCommandLine::contextMenuItems, event names by label
        std::map<QString, QString> menuItems;
        QPointer<HistoryWriter> writer;
        // A console's own command line, still in TConsole::layerCommandLine, which TConsole::setCmdVisible() shows
        // and hides with it
        bool inConsole = false;
        bool layerShown = true;
        // Whether the console holding its own command line shows, once that console has left the registry
        bool consoleShown = true;
        SizeLimits limits;
        // QPlainTextEdit::isReadOnly() and the window's own sheet, which its limits depend on
        bool readOnly = false;
        QString styleSheet;
    };
    struct SubConsole
    {
        std::unique_ptr<TConsoleModel> pModel;
        QString userWindow;
        std::shared_ptr<ScrollBox> box;
        bool shown = false;
        // Only for an MXP frame's, which is how a frame tells its own from a script's window of the same name
        std::unique_ptr<TConsoleModelSink> pFrameSink;
        QPointer<QObject> holder;
        // TConsole::mpCommandLine, which still holds a deleted command line until deferred deletes run
        QPointer<HistoryWriter> ownLine;
        // A user window's console: its size, which a resize while the dock is hidden reaches only as it shows, and its
        // QWidget::WA_PendingResizeEvent, set on a new one and by a size it takes while not visible
        QSize consoleSize;
        bool resizePending = true;
        // From a show sending that resize until its handlers return, or a show nested in them after a hide ends
        bool sendingResize = false;
        // TDockWidget::hasLayoutAlready: whether opening this user window has asked for the saved layout,
        // which a load already running refuses
        bool layoutRequested = false;
    };
    // As TConsole::mpCommandLine tells a console's own command line from another of its name
    static bool ownsLine(const SubConsole& console, const PlainWindow& commandLine) { return commandLine.writer && commandLine.writer == console.ownLine; }
    struct Label
    {
        std::unique_ptr<TLabelModel> pModel;
        QString userWindow;
        std::shared_ptr<ScrollBox> box;
        bool shown = true;
        // What a real label's QWidget and QStyleSheetStyle keep beside its font: which of its
        // attributes were set on it, and the font each restyle starts again from
        uint fontMask = 0;
        std::optional<QFont> savedFont;
        // The font properties the label's own sheet declares, if it declares any
        std::optional<QFont> sheetFont;
        // The widths SE_ShapedFrameContents takes off each side under its sheet, as top, right, bottom and left
        std::array<int, 4> frame{};
        SizeLimits limits;
        // Qt::WA_StyleSheet
        bool styled = false;
        // TLabel::mPaletteSetSinceStyled
        bool restyleDue = false;
        // TLabel::mRepolished
        bool repolished = false;
        // QLabel only measures its text once it has been given some, and caches the answer
        bool textLabel = false;
        mutable std::optional<QSize> sizeHint;
        // TLabel::mpMovie, which outlives its place in QLabel so a later setMovie() reuses it
        std::unique_ptr<QMovie> movie;
        // QLabel::movie() is set
        bool showsMovie = false;
        // TMainConsole::scaleLabelMovie()'s connection to TLabel::resized
        bool movieFollowsSize = false;
        // Qt::WA_PendingResizeEvent
        bool resizePending = false;
        // QLabel::pixmap()'s size, which a raster background image puts in place of the text or movie
        std::optional<QSize> pixmapSize;
        // TLabel::mpSvgRenderer's default size, as an SVG background image paints under the content
        std::optional<QSize> svgSize;
    };
    struct MapWidget
    {
        QString title;
        QRect geometry;
        bool shown = false;
        std::unique_ptr<TNullMapView> pView;
    };
    // TMainConsole::raiseWindow(), lowerWindow() and reparentWindow() take "mapper" in any case for it
    bool namesEmbeddedMapper(const QString& name) const { return mpEmbeddedMapper && !name.compare(QLatin1String("mapper"), Qt::CaseInsensitive); }
    void copyPlayerRoomStyleToMap();
    void showEmbeddedMapper();
    void reportEmbeddedMapperVisibility();
    QMovie* shownMovie(const QString& name) const;
    void setLabelVisible(Label& label, bool visible);
    void setLabelSize(Label& label, const QSize& size);
    void restylePlainWindow(const QString& name, PlainWindow& window, bool commandLine);
    void clearLabelContents(Label& label);

    TConsoleModel& addSubConsole(const QString& name, TWindowRegistry::SubConsoleKind kind, const QString& windowname);
    // keepWidget for a console whose widget outlives its place in the registry, with its own command line
    void removeSubConsole(const QString& name, bool keepWidget = false);
    void addFrameConsole(const QString& name, bool external = false, const QString& hostFrame = QString());
    void removeFrameConsole(const QString& name);
    TConsoleModelSink* frameSink(const QString& name) const;
    void removeLabel(const QString& name);
    void queueRelease();
    QString userWindowOrMain(const QString& windowname) const;
    std::pair<QString, std::shared_ptr<ScrollBox>> containerFor(const QString& windowname, bool scrollBoxFirst) const;
    bool shownWithin(const QString& userWindow, const ScrollBox* box) const;
    bool setPlainWindowShown(const QString& name, bool shown);
    bool ownsCommandLine(const QString& name) const;
    PlainWindow* commandLineNamed(const QString& name);
    void releaseCommandLine(const QString& name, const PlainWindow& commandLine);
    QObject* holderFor(const QString& userWindow, const ScrollBox* box) const;
    HistoryWriter* makeHistoryWriter(QObject* parent, const QString& name, bool savesHistory);
    void dropModels();
    PlainWindow* plainWindow(const QString& name);
    void republishPlainWindow(const QString& name);
    static bool within(const ScrollBox* container, const ScrollBox* box);
    void removeScrollBoxContents(const ScrollBox* box);
    void moveScrollBoxContents(const ScrollBox* box);
    void inheritLabelFont(Label& label, bool restyle);
    void showLabelText(Label& label, const QString& shown);
    void reapplyLabelLinkStyle(Label& label);
    TLabelModel* labelModel(const QString& name) const;
    TConsoleModel* subConsoleModel(const QString& name) const;
    bool setLabelShown(const QString& name, bool shown);
    QFont naturalLabelFont(const Label& label) const;
    void setLabelWidgetFont(Label& label, const QFont& font, bool applySheet);
    void polishLabel(Label& label);
    void restyleLabel(Label& label, const QString& sheet);
    bool setSubConsoleShown(const QString& name, bool shown);
    void raiseUserWindowResize(const QString& name);
    void reportVisibility(const QString& changedWindow, const ScrollBox* changedBox);
    void reportWindowVisibility(const QString& name);
    std::optional<QFont> consoleFont(const QString& name) const;
    void setSubConsoleFont(const QString& name, const QFont& font);
    void reportDisplayFontChange(const QFont& before);
    void raiseFontEvent(const QString& eventName, const QString& subject, const QFont& font);
    void reportGridSize(const QString& name);

    Host* mpHost = nullptr;
    TNullMxpFrameFrontend mMxpFrames{*this};
    std::map<QString, SubConsole> mSubConsoles;
    std::map<QString, Label> mLabels;
    std::map<QString, std::shared_ptr<ScrollBox>> mScrollBoxes;
    std::map<QString, PlainWindow> mCommandLines;
    // Only its menu items and history writer, as its text, sheet and setting are in the registry
    PlainWindow mMainCommandLine;
    std::map<QString, PlainWindow> mTextBoxes;
    std::vector<std::unique_ptr<TConsoleModel>> mRetiredConsoles;
    std::vector<std::unique_ptr<TLabelModel>> mRetiredLabels;
    bool mReleaseQueued = false;
    // While a hidden user window is removed, the names its deferred deletes are to report
    QStringList* mpDeferredReports = nullptr;
    bool mDroppingWindows = false;
    // The main window, which holds what a script makes in it
    std::unique_ptr<QObject> mpMainHolder = std::make_unique<QObject>();
    // TMxpFrameWidgets' container widget of each frame, which holds the frame's console until a script moves it
    std::map<QString, QPointer<QObject>> mFrameHolders;
    std::optional<MapWidget> mMapWidget;
    // In place of the dlgMapper createMapper() makes, a child of what holds it, so it goes with the window it is in
    QPointer<TNullMapView> mpEmbeddedMapper;
    // Where it is and whether it shows there, as it paints only while it and the windows holding it show
    QString mEmbeddedMapperWindow;
    std::shared_ptr<ScrollBox> mpEmbeddedMapperBox;
    bool mEmbeddedMapperShown = false;
};

#endif // MUDLET_TNULLCONSOLEFRONTEND_H
