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

protected:
    // The view is a widget whose owner deletes it as one, so nothing deletes it through this interface.
    ~TConsoleFrontend() = default;
};

#endif // MUDLET_TCONSOLEFRONTEND_H
