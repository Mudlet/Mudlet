#ifndef MUDLET_TLABELMODEL_H
#define MUDLET_TLABELMODEL_H

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

#include <QColor>
#include <QFont>
#include <QList>
#include <QPointer>
#include <QRect>
#include <QSet>
#include <QSize>
#include <QString>

#include <functional>
#include <optional>

class Host;
class QSvgRenderer;

// How a change of a label's text is announced to a screen reader, as ARIA's aria-live
enum class TLabelLiveRegion : int { Off, Polite, Assertive };

// A label's widget-free state, so the Widgets-free core can hold a named label without a QLabel.
// Owned by its TLabel, whose same-named reference members alias it; TWindowRegistry indexes
// these by name but does NOT own them.
struct TLabelModel
{
    // Defined out of line: mpHost is a QPointer, which needs Host complete.
    TLabelModel(Host* pHost, const QString& name);
    // Frees the callbacks' Lua registry indexes.
    ~TLabelModel();

    // A copy would free the same Lua registry indexes twice. This also suppresses the implicit moves.
    TLabelModel(const TLabelModel&) = delete;
    TLabelModel& operator=(const TLabelModel&) = delete;

    // The name a screen reader hears: the one a script gave it, else its text, else its tooltip
    QString effectiveAccessibleName() const;
    // The label's text as a screen reader would say it
    QString spokenText() const;
    bool namedByToolTip() const;
    // Whether a screen reader is told it is a button: as a script said, else when it has a click or release
    // callback - never when the mouse passes through it
    bool actsAsButton() const;
    // What it shows, which a screen reader is told unless it is a button. Kept by the view, which alone knows.
    enum class ShownContent { Text, Graphic, Animation };
    ShownContent mShownContent = ShownContent::Text;
    void setAccessibleName(const QString& name);
    void setAccessibleButton(std::optional<bool> button);
    void setClickThrough(bool clickThrough);
    // Nothing to tell the view: a live region is heard only when the text next changes
    void setLiveRegion(TLabelLiveRegion liveRegion) { mLiveRegion = liveRegion; }
    // The view's copies of what the label shows, which name it unless a script named it
    void setText(const QString& text);
    void setToolTip(const QString& toolTip);

    // KeyboardAccess: the profile's accessibleLabels setting changed
    enum class AccessibilityChange { Name, Role, KeyboardAccess };
    // Set by the widget, if there is one, to update its focus policy and tell assistive technology of a change
    std::function<void(AccessibilityChange)> mAccessibilityChanged;
    void accessibilityChanged(AccessibilityChange change) const;

    // Whether the SVG renderer is worth trying on a file, judged by its first bytes.
    static bool svgCandidate(const QString& path);
    static bool loadSvg(QSvgRenderer& renderer, const QString& path);
    // A file's natural size as an SVG document or a raster image, without a widget.
    static std::optional<QSize> imageSize(const QString& path);

    // Each releases the Lua registry index it replaces.
    void setClick(const int func);
    void setDoubleClick(const int func);
    void setRelease(const int func);
    void setMove(const int func);
    void setWheel(const int func);
    void setEnter(const int func);
    void setLeave(const int func);

    // What TLabel::setBackgroundColor() restyles a label with: the sheet with its background-color
    // declarations given the colour, or the colour's appended.
    static QString styleSheetWithBackgroundColor(const QString& styleSheet, const QColor& color);
    // What TLabel::resetBackgroundImage() restyles a label with: the sheet without its background-image
    // declarations, but for those inside a {...} rule, such as a :hover image.
    static QString styleSheetWithoutBackgroundImage(const QString& styleSheet);
    // Whether the text has an anchor tag, which makes the label's links clickable
    static bool containsAnchorTag(const QString& text);
    // What TLabel::setText() shows for the text: each anchor given the link style as an inline style
    QString linkStyledText(const QString& text);
    // What the label shows for its text once the link style or the visited links change
    QString restyledText();

    // A QPointer because Host and view are torn down in either order: quitting
    // destroys every Host before the labels' deferred deletes run.
    QPointer<Host> mpHost;
    QString mName;
    int mClickFunction = 0;
    int mDoubleClickFunction = 0;
    int mReleaseFunction = 0;
    int mMoveFunction = 0;
    int mWheelFunction = 0;
    int mEnterFunction = 0;
    int mLeaveFunction = 0;
    QString mLinkColor;
    QString mLinkVisitedColor;
    bool mLinkUnderline = true;
    QSet<QString> mVisitedLinks;
    // What setBackgroundColor() was last given; a label restyled with its own
    // background-color stylesheet paints something else.
    QColor mBackgroundColor;
    // The tint and the transforms of an SVG background are properties of the
    // label that outlive any particular image, so they belong with the rest of
    // its widget-free state; the renderer that draws them stays with the widget.
    QColor mSvgTintColor;
    double mSvgRotation = 0.0;
    double mSvgShearX = 0.0;
    double mSvgShearY = 0.0;
    // The widget's pos() and size(), kept current by the view as Qt moves or resizes it.
    QRect mGeometry;
    // QWidget::isVisibleTo() the main console, kept current by the view.
    bool mVisible = false;
    // The widget's own styleSheet(), toolTip() and font(), kept current by the view.
    QString mStyleSheet;
    QString mToolTip;
    // What a script named the label for screen readers; empty means its text is used, else its tooltip
    QString mAccessibleName;
    // Set by a script whose label's callbacks are not a button's, such as a drag handle; empty means automatic
    std::optional<bool> mAccessibleButton;
    TLabelLiveRegion mLiveRegion = TLabelLiveRegion::Off;
    bool mClickThrough = false;
    QFont mFont;
    // QLabel::text(), which a pixmap or movie empties, written by the view after each change.
    QString mText;

private:
    void releaseFunc(const int existingFunction, const int newFunction);
    QString withoutInjectedLinkStyles(const QString& text) const;
    QString styleLinks(const QString& text);

    struct StyledAnchor
    {
        QString styled;
        QString original;
    };

    // the text the last styling pass started from, with the anchor tags it rewrote put back
    QString mUnstyledText;
    // each anchor tag the last styling pass wrote and the one it replaced, in the order of the text
    QList<StyledAnchor> mStyledAnchors;
    // what the last styling pass gave the view to show
    QString mStyledText;
};

#endif // MUDLET_TLABELMODEL_H
