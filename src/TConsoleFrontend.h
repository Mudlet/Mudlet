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

#include <optional>
#include <utility>

class QColor;
class QFont;
class QSize;
class QString;

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

    // These act only on command lines made by createCommandLine() or a mini console's, never a console's own;
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

protected:
    // The view is a widget whose owner deletes it as one, so nothing deletes it through this interface.
    ~TConsoleFrontend() = default;
};

#endif // MUDLET_TCONSOLEFRONTEND_H
