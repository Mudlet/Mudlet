#ifndef MUDLET_TCONSOLEFRONTEND_H
#define MUDLET_TCONSOLEFRONTEND_H

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

#include "TConsoleModel.h"

#include <QFont>

#include <list>
#include <optional>
#include <utility>

class QColor;
class QPoint;
class QRect;
class QSize;
class QString;
class TAction;
class TMxpFrameFrontend;

// A profile's main console as core code sees it. TMainConsole implements it; core code
// reaches it through Host::consoleFrontend() and names windows, never widgets.
class TConsoleFrontend
{
public:
    virtual bool createLabel(const QString& windowname, const QString& name, int x, int y, int width, int height, bool fillBackground, bool clickThrough) = 0;
    // These fail for a name that is not a label's.
    virtual std::pair<bool, QString> deleteLabel(const QString& name) = 0;
    virtual std::pair<bool, QString> setLabelStyleSheet(const QString& name, const QString& stylesheet) = 0;
    virtual std::optional<QSize> getLabelSizeHint(const QString& name) const = 0;
    virtual std::pair<bool, QString> setLabelToolTip(const QString& name, const QString& text, double duration) = 0;
    virtual std::pair<bool, QString> setLabelCursor(const QString& name, int shape) = 0;
    virtual std::pair<bool, QString> setLabelCustomCursor(const QString& name, const QString& pixMapLocation, int hotX, int hotY) = 0;
    virtual bool setLabelClickThrough(const QString& name, bool clickThrough) = 0;
    virtual bool setLabelLinkStyle(const QString& name, const QString& linkColor, const QString& linkVisitedColor, bool underline) = 0;
    virtual bool resetLabelLinkStyle(const QString& name) = 0;
    virtual bool clearLabelVisitedLinks(const QString& name) = 0;
    virtual bool showLabel(const QString& name) = 0;
    virtual bool hideLabel(const QString& name) = 0;
    virtual bool resizeLabel(const QString& name, int width, int height) = 0;
    virtual bool moveLabel(const QString& name, int x, int y) = 0;
    virtual bool reparentLabel(const QString& windowname, const QString& name, int x, int y, bool show) = 0;
    virtual bool setLabelText(const QString& name, const QString& text) = 0;
    virtual std::pair<bool, QString> setLabelMovie(const QString& name, const QString& moviePath) = 0;
    virtual bool setLabelBackgroundColor(const QString& name, const QColor& color) = 0;
    virtual std::optional<QColor> getLabelBackgroundColor(const QString& name) const = 0;
    virtual bool setLabelBackgroundImage(const QString& name, const QString& path) = 0;
    virtual bool resetLabelBackgroundImage(const QString& name) = 0;
    virtual bool setLabelSvgTint(const QString& name, const QColor& color) = 0;
    virtual bool resetLabelSvgTint(const QString& name) = 0;
    virtual bool setLabelSvgRotation(const QString& name, double angle) = 0;
    virtual bool resetLabelSvgRotation(const QString& name) = 0;
    virtual bool setLabelSvgShear(const QString& name, double shearX, double shearY) = 0;
    virtual bool resetLabelSvgShear(const QString& name) = 0;
    virtual bool resetLabelSvgTransform(const QString& name) = 0;
    virtual bool setLabelFont(const QString& name, const QFont& font) = 0;
    // No value for a name that is not a label's, false for a label that is not
    // showing a movie; the movie operations below report failure for either.
    virtual std::optional<bool> labelShowsMovie(const QString& name) const = 0;
    virtual bool startLabelMovie(const QString& name) = 0;
    virtual bool pauseLabelMovie(const QString& name) = 0;
    // Also false when the movie has no such frame.
    virtual bool setLabelMovieFrame(const QString& name, int frame) = 0;
    virtual bool setLabelMovieSpeed(const QString& name, int percent) = 0;
    virtual bool scaleLabelMovie(const QString& name, bool followLabelSize) = 0;

    // These act only on command lines made by createCommandLine() or a mini console's, never the main console's own;
    // createCommandLine() fails for a name already taken.
    virtual std::pair<bool, QString> createCommandLine(const QString& windowname, const QString& name, int x, int y, int width, int height) = 0;
    virtual std::pair<bool, QString> deleteCommandLine(const QString& name) = 0;
    virtual bool setCommandLineAction(const QString& name, int func) = 0;
    virtual bool resetCommandLineAction(const QString& name) = 0;
    // A command line is named: an empty name or "main" is this console's own, any other one made by
    // createCommandLine() or a mini console's. Each operation fails for a name that is none of those.
    virtual std::pair<bool, QString> setCmdLineStyleSheet(const QString& name, const QString& styleSheet) = 0;
    virtual bool replaceCommandLineText(const QString& name, const QString& text) = 0;
    virtual bool appendCommandLineText(const QString& name, const QString& text) = 0;
    virtual bool clearCommandLine(const QString& name) = 0;
    virtual bool selectCommandLineText(const QString& name) = 0;
    virtual bool addCommandLineSuggestion(const QString& name, const QString& word) = 0;
    virtual bool removeCommandLineSuggestion(const QString& name, const QString& word) = 0;
    virtual bool clearCommandLineSuggestions(const QString& name) = 0;
    virtual bool addCommandLineBlacklistWord(const QString& name, const QString& word) = 0;
    virtual bool removeCommandLineBlacklistWord(const QString& name, const QString& word) = 0;
    virtual bool clearCommandLineBlacklist(const QString& name) = 0;
    virtual bool addCommandLineMenuItem(const QString& name, const QString& label, const QString& eventName) = 0;
    // No value for a name that is not a command line's, false for a command line
    // with no such item.
    virtual std::optional<bool> removeCommandLineMenuItem(const QString& name, const QString& label) = 0;
    virtual bool setCommandLineSavesHistory(const QString& name, bool savesHistory) = 0;
    virtual bool setCommandLineVisible(const QString& name, bool visible) = 0;
    // A console's own command line, by the console's name; created the first time it is shown.
    virtual bool setWindowCommandLineVisible(const QString& name, bool visible) = 0;
    // These act on this console's own command line.
    virtual void setCommandLinePlaceholderText(const QString& text) = 0;
    virtual void updateCommandLineSpellCheck(bool enabled) = 0;
    virtual void setCommandLineText(const QString& text) = 0;
    // Raises and focuses the command line the player used last, or this console's own when none is on record.
    virtual void focusActiveCommandLine() = 0;

    virtual std::pair<bool, QString> createTextBox(const QString& windowname, const QString& name, int x, int y, int width, int height) = 0;
    // These fail for a name that is not a text box's.
    virtual std::pair<bool, QString> deleteTextBox(const QString& name) = 0;
    virtual bool setTextBoxText(const QString& name, const QString& text) = 0;
    virtual bool clearTextBox(const QString& name) = 0;
    virtual bool setTextBoxReadOnly(const QString& name, bool readOnly) = 0;
    virtual bool setTextBoxPlaceholder(const QString& name, const QString& text) = 0;
    virtual bool setTextBoxStyleSheet(const QString& name, const QString& styleSheet) = 0;
    virtual bool setTextBoxFont(const QString& name, const QFont& font) = 0;
    virtual bool setTextBoxTabMovesFocus(const QString& name, bool tabMovesFocus) = 0;

    // A sub-console is a mini console, user window or buffer. createBuffer() fails for a name already a
    // sub-console's, the others for a name that is not one; they act on a user window's dock with it.
    virtual bool createBuffer(const QString& name) = 0;
    // Fails for a name already a sub-console's. Puts the mini console in the scroll box or user window
    // named windowname, else on the main window, at font size 12, shown.
    virtual bool addMiniConsole(const QString& windowname, const QString& name, int x, int y, int width, int height) = 0;
    virtual std::pair<bool, QString> deleteMiniConsole(const QString& name) = 0;
    virtual bool showSubConsole(const QString& name) = 0;
    virtual bool hideSubConsole(const QString& name) = 0;
    // Resizing or moving a docked user window floats it first.
    virtual bool resizeSubConsole(const QString& name, int width, int height) = 0;
    virtual bool moveSubConsole(const QString& name, int x, int y) = 0;
    // For shutdown: closes the sub-console, first taking a user window's dock out of the main window; does
    // nothing for a name that is not a sub-console's. Only while Mudlet or the profile is closing down
    // does that take the name out of the window registry; at any other time the sub-console is just hidden.
    virtual void closeSubConsole(const QString& name) = 0;

    // Makes the user window unless one has that name and shows it, then floats it ("f") or docks it
    // ("r", "l", "t", "b"), each also accepted as the word it stands for; an empty area leaves it where
    // it is. An unknown area fails with the window already showing. No name is refused: a mini console's
    // gets a new user window that takes the name over.
    virtual std::pair<bool, QString> openUserWindow(const QString& name, bool loadLayout, bool autoDock, const QString& area) = 0;
    // These fail for a name that is not a user window's; an empty text restores the default title.
    virtual std::pair<bool, QString> setUserWindowStyleSheet(const QString& name, const QString& userWindowStyleSheet) = 0;
    virtual std::pair<bool, QString> setUserWindowTitle(const QString& name, const QString& text) = 0;

    // createScrollBox() fails for a name already a scroll box's, deleteScrollBox() for one that is not. A
    // windowname that is neither a user window nor a scroll box puts the new one on this console.
    virtual bool createScrollBox(const QString& windowname, const QString& name, int x, int y, int width, int height) = 0;
    virtual std::pair<bool, QString> deleteScrollBox(const QString& name) = 0;
    // A plain window is a scroll box, command line or text box, tried in that order; these fail for a
    // name that is none of those.
    virtual bool showPlainWindow(const QString& name) = 0;
    virtual bool hidePlainWindow(const QString& name) = 0;
    virtual bool resizePlainWindow(const QString& name, int width, int height) = 0;
    virtual bool movePlainWindow(const QString& name, int x, int y) = 0;

    // These take a sub-console, label, scroll box, command line or text box by name, or "mapper" in
    // any case while there is a mapper, and fail for a name that is none of those.
    virtual bool raiseWindow(const QString& name) = 0;
    virtual bool lowerWindow(const QString& name) = 0;
    // As raiseWindow() but never a label, which reparentLabel() moves. The new parent is the scroll box,
    // else the user window, named windowname, or this console when there is neither.
    virtual bool reparentWindow(const QString& windowname, const QString& name, int x, int y, bool show) = 0;

    // An empty name or "main" is this console, any other a mini console, user window or buffer; each
    // fails for a name that is none of those.
    virtual bool setWindowScrollBarVisible(const QString& name, bool visible) = 0;
    virtual bool setWindowHorizontalScrollBarVisible(const QString& name, bool visible) = 0;
    virtual bool setWindowScrolling(const QString& name, bool enabled) = 0;
    // A negative line counts back from the end. One at or past the end, or toEnd, puts the console back
    // to following new lines.
    virtual bool scrollWindowTo(const QString& name, int line, bool toEnd) = 0;

    // These name a console as the scroll bar ones do; for a name that is none of those, setWindowFontSize()
    // fails and the others have no value. This console's font is the profile's display font, which Host
    // keeps and tells scripts about when it changes.
    // A new family keeps the point size; the answer says whether the font was taken and, when not, why.
    virtual std::optional<std::pair<bool, QString>> setWindowFontFamily(const QString& name, const QString& family, QFont::Weight weight) = 0;
    virtual bool setWindowFontSize(const QString& name, int size) = 0;
    // The width of a 'W' and the height of a line in that console's font.
    virtual std::optional<QSize> consoleFontSize(const QString& name) const = 0;

    // This console's own appearance. The image modes are 1 border, 2 center, 3 tile and 4 style, whose
    // path is a style sheet fragment instead; the window background also takes 5, cover, which fails for
    // a file that is not an image. Any other mode fails.
    virtual void setConsoleBgColor(int r, int g, int b, int a) = 0;
    virtual bool setConsoleBackgroundImage(const QString& imgPath, int mode) = 0;
    virtual bool resetConsoleBackgroundImage() = 0;
    // The window background spans the borders as well as the text, and hides the border colour.
    virtual bool setWindowBackgroundImage(const QString& imgPath, int mode) = 0;
    virtual bool resetWindowBackgroundImage() = 0;
    virtual void setBorderColor(const QColor& color) = 0;
    // Re-applies the profile's colours.
    virtual void changeColors() = 0;
    // Also styles every user window's dock and the map's.
    virtual void setProfileStyleSheet(const QString& styleSheet) = 0;
    // Lays the console out again for Host's current borders and raises sysWindowResizeEvent with the room they leave.
    virtual void applyBorders() = 0;
    virtual QFont displayFont() const = 0;
    virtual void setFont(const QFont& font) = 0;
    virtual void setFontSize(int size) = 0;

    // These fail only for a name that is not a sub-console's, for which changeSubConsoleColors() does nothing.
    virtual bool setSubConsoleBackgroundColor(const QString& name, const QColor& color) = 0;
    virtual bool setSubConsoleBackgroundImage(const QString& name, const QString& path, int mode) = 0;
    virtual bool resetSubConsoleBackgroundImage(const QString& name) = 0;
    virtual bool setSubConsoleCommandBackgroundColor(const QString& name, const QColor& color) = 0;
    virtual bool setSubConsoleCommandForegroundColor(const QString& name, const QColor& color) = 0;
    virtual void changeSubConsoleColors(const QString& name) = 0;

    // An embedded mapper, in the user window windowname names, else on this console for an empty name or
    // "main"; any other name fails, as does a map dock on screen, while a hidden one is removed. The first
    // raises mapOpenEvent, loading the profile's map if none is loaded; later calls move and resize it.
    virtual std::pair<bool, QString> createMapper(const QString& windowname, int x, int y, int width, int height) = 0;
    // The map dock, for when nothing draws the map: made hidden around a new mapper that then does.
    virtual void createMapperDock(const QString& title, const QString& objectName) = 0;
    // Docks the dock createMapperDock() made on the right, restores the saved window layout and then shows
    // the dock and its mapper regardless of it.
    virtual void showNewMapperDock() = 0;
    // Shows the map dock and places it by area as openUserWindow() does; only floating uses the position
    // and size, each left alone when either of its values is -1. Fails when there is no map dock.
    virtual std::pair<bool, QString> placeMapWidget(const QString& area, int x, int y, int width, int height) = 0;
    // mapWidgetCreated() does not mean on screen, which is what the four after it go by: while the map
    // dock is hidden they fail or have no value.
    virtual bool mapWidgetCreated() const = 0;
    virtual bool setMapWidgetTitle(const QString& title) = 0;
    virtual std::optional<QString> mapWidgetTitle() const = 0;
    virtual std::optional<QRect> mapWidgetGeometry() const = 0;
    virtual bool hideMapWidget() = 0;
    // Hands TMap::mpMapper back to this profile's own mapper, if it has one.
    virtual void restoreOwnMapper() = 0;

    // The mapper drawing the map is TMap::mpMapper, which a main window or detached window dock may have
    // borrowed from this console, so these act on that one and do nothing when there is none.
    // After a map load: redraw from scratch and show the player's area.
    virtual void showLoadedMap() = 0;
    // After a failed load: redraw from scratch, staying on the area shown.
    virtual void showMapAfterFailedLoad() = 0;
    // The map was already loaded when the mapper was made: show the player's area.
    virtual void showMapAtPlayerArea() = 0;
    // A mapper in a dock counts as shown when its dock does, and is shown and hidden with it; no mapper
    // counts as not shown.
    virtual bool mapperShown() const = 0;
    virtual void setMapperShown(bool shown) = 0;
    virtual void setMapperPanelVisible(bool visible) = 0;
    virtual void setMapLargeAreaExitArrows(bool enabled) = 0;
    // Also repaints the map's 3D view, if one is open.
    virtual void requestMapRepaint() = 0;

    // A root action set to float has a floating toolbar, any other a button bar in this console. An
    // action shown as a menu on another's bar (on a floating toolbar, any entry of such a menu) is
    // recorded against that bar too, so what is asked of its bar is done to the whole bar.
    // These bring the bars in line with the root actions (for a package, with the toolbars in it): make,
    // fill and place each, and destroy any left from an action that has switched between docked and floating.
    virtual void regenerateToolBars(const std::list<TAction*>& rootActions) = 0;
    virtual void regenerateEasyButtonBars(const std::list<TAction*>& rootActions) = 0;
    // Takes an action's bars out of the window without destroying them, for an action that is being
    // removed or has stopped being a root one.
    virtual void detachActionBars(TAction* pAction) = 0;
    virtual bool hasEasyButtonBar(TAction* pAction) const = 0;
    // Checks or unchecks the action's button and its entry in a bar's menu, where it has them.
    virtual void setActionButtonChecked(TAction* pAction, bool checked) = 0;
    // For a child moved out from under pOldParent: the child no longer belongs to whichever of its bars
    // it shared with its old parent.
    virtual void releaseParentActionBars(TAction* pOldParent, TAction* pChild) = 0;
    // Renames the action's floating toolbar, if it has one.
    virtual void renameActionToolBar(TAction* pAction, const QString& name) = 0;
    // Floating toolbars are the main window's children rather than this console's, so the profile has
    // to delete them itself: at once, or once control returns to the event loop.
    virtual void deleteActionToolBars() = 0;
    virtual void deleteActionToolBarsLater() = 0;

    // Floating toolbars raise a flag when moved or resized. Committing lowers the flags and answers
    // whether any was raised; discarding, for a layout just restored, stops counting them but leaves
    // them raised.
    virtual bool commitToolBarLayoutChanges() = 0;
    virtual void discardToolBarLayoutChanges() = 0;
    // The same flag on a user window's dock: clearDockLayoutChanged() is true only when there was a
    // raised flag to lower. Both do nothing for a name with no dock.
    virtual void setDockLayoutChanged(const QString& name) = 0;
    virtual bool clearDockLayoutChanged(const QString& name) = 0;

    // The view's part of Host::printOnDisplay(). startIncomingText() starts timing the pass for the latency
    // box and answers whether new text should alert the player: true before this console is first shown, and
    // from its being hidden, with Mudlet minimized and the profile asking for alerts, until it is shown again.
    virtual bool startIncomingText() = 0;
    // Asks the desktop to draw the player's attention to Mudlet's window until it is next activated.
    virtual void alertNewData() = 0;
    // Ends the timing, schedules the paced latency box refresh and has the profile's tab marked for new text,
    // which shows only while another profile's tab is the active one and Mudlet is not in multi-view.
    virtual void finishIncomingText() = 0;
    // Does what showNewLines() does, for the end of a batch of incoming text.
    virtual void finalize() = 0;

    // Repaint cues for text written to this console's model: showNewLines() for lines appended to it (a view
    // scrolled back stays where it is), showCommandEcho() for what TConsoleModel::printCommand() did and
    // markSelectionDirty() for the lines the selection covers, after their text changed in place.
    virtual void showNewLines() = 0;
    virtual void showCommandEcho(const TConsoleModel::CommandEcho& echo) = 0;
    virtual void markSelectionDirty() = 0;
    // Re-applies this console's font to its text and redraws all of it.
    virtual void refreshView() const = 0;
    // Schedules a repaint after a command echo, skipped once the mapper drawing the map has made a 3D view
    // (even if it has since gone back to 2D) or, in builds without one, while there is any mapper.
    virtual void requestRepaintAfterCommand() = 0;

    // Raises sysExitEvent, then saves the profile and map if it always saves on exit or its close is forced,
    // else asks the player whether to save. False when the player cancels; once a close has been accepted,
    // a later one is accepted at once, with no event, save or prompt.
    virtual bool requestClose() = 0;
    // For a profile reset: takes every sub-console (a user window with its dock), command line, label, scroll
    // box and text box out of the window registry and deletes each once control returns to the event loop.
    virtual void resetMainConsole() = 0;
    // Gives this console and each of its sub-consoles the profile's new name.
    virtual void setProfileName(const QString&) = 0;
    // Adds or removes the F3 / Shift+F3 buffer search keys; adding them warns the profile of any add-on
    // command that already holds either key.
    virtual void setF3SearchEnabled(bool enabled) = 0;
    // Hides the button row beside the command line when true, shows it when false.
    virtual void setCompactInputLine(bool state) = 0;
    // Caret navigation of the text: on moves keyboard focus to it, grabbing the keyboard on Windows and Linux;
    // off releases that and makes the command line its focus proxy again.
    virtual void setCaretMode(bool enabled) = 0;
    // The mouse pointer's position relative to this console's top-left corner, so it can lie outside it.
    virtual QPoint mousePosition() const = 0;
    // True while keyboard focus is on this console's focus proxy, normally its own command line, so false
    // while it is on the text, as in caret navigation, or in another window.
    virtual bool hasKeyboardFocus() const = 0;

    // The widgets of the profile's MXP frames, which live as long as this console.
    virtual TMxpFrameFrontend& mxpFrames() = 0;
    virtual const TMxpFrameFrontend& mxpFrames() const = 0;

protected:
    // The view is a widget whose owner deletes it as one, so nothing deletes it through this interface.
    ~TConsoleFrontend() = default;
};

#endif // MUDLET_TCONSOLEFRONTEND_H
