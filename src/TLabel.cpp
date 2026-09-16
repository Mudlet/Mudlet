/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Ian Adkins - ieadkins@gmail.com                 *
 *   Copyright (C) 2017 by Chris Reid - WackyWormer@hotmail.com            *
 *   Copyright (C) 2020, 2023 by Stephen Lyons - slysven@virginmedia.com   *
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


#include "TLabel.h"
#include "TConsole.h"
#include "TDockWidget.h"
#include "mudlet.h"

#include <QDesktopServices>
#include <QRegularExpression>
#include <QTextCursor>
#include <QTimer>
#include <QUrl>
#include <QtEvents>
#include <chrono>

using namespace std::chrono_literals;

// Hand-rolled because a case-insensitive text.contains("<a ") cannot tell a tag
// from prose that happens to hold "<a ", and case-folds every character it walks.
// Any whitespace counts as the separator because HTML allows any; the styling pass
// in setText() recognises only the ASCII ones, so an anchor split by a non-breaking
// space comes out clickable but unstyled.
static bool containsAnchorTag(const QString& text)
{
    qsizetype close = -1;
    // Every later '<' is nearer the end still, so a '<' too close to it for a tag
    // name and a separator ends the walk rather than being skipped over.
    for (qsizetype at = text.indexOf(QLatin1Char('<')); at >= 0 && at + 2 < text.size(); at = text.indexOf(QLatin1Char('<'), at + 1)) {
        const char16_t tagName = text.at(at + 1).unicode();
        if ((tagName != u'a' && tagName != u'A') || !text.at(at + 2).isSpace()) {
            continue;
        }
        // Prose holds "<a" and a space too, so it is only a tag once the '>' that
        // closes it turns up; another '<' on the way there means this one never was
        // one. That also turns away an attribute value carrying a '<' of its own,
        // which HTML asks to be written &lt; anyway.
        if (close < at) {
            // Searched for again only once the walk has passed the last one found,
            // so the '>' scans stay linear over the whole text
            close = text.indexOf(QLatin1Char('>'), at + 3);
            if (close < 0) {
                // no tag anywhere past here can be closed either
                return false;
            }
        }
        const qsizetype nextOpen = text.indexOf(QLatin1Char('<'), at + 3);
        if (nextOpen < 0 || close < nextOpen) {
            return true;
        }
    }
    return false;
}

TLabel::TLabel(Host* pH, const QString& name, QWidget* pW)
: QLabel(pW)
, mpModel(std::make_unique<TLabelModel>(pH, name))
, mpHost(mpModel->mpHost)
, mName(mpModel->mName)
, mClickFunction(mpModel->mClickFunction)
, mDoubleClickFunction(mpModel->mDoubleClickFunction)
, mReleaseFunction(mpModel->mReleaseFunction)
, mMoveFunction(mpModel->mMoveFunction)
, mWheelFunction(mpModel->mWheelFunction)
, mEnterFunction(mpModel->mEnterFunction)
, mLeaveFunction(mpModel->mLeaveFunction)
, mLinkColor(mpModel->mLinkColor)
, mLinkVisitedColor(mpModel->mLinkVisitedColor)
, mLinkUnderline(mpModel->mLinkUnderline)
, mVisitedLinks(mpModel->mVisitedLinks)
, mBackgroundColor(mpModel->mBackgroundColor)
{
    setMouseTracking(true);
    setObjectName(qsl("label_%1_%2").arg(pH->getName(), mName));

    setTextFormat(Qt::RichText);
    setTextInteractionFlags(Qt::NoTextInteraction);
    setOpenExternalLinks(false);

    connect(this, &QLabel::linkActivated, this, &TLabel::slot_linkActivated);
}

TLabel::~TLabel()
{
    // The backstop against a stale entry: TMainConsole deregisters where it
    // destroys a label, but a label can also die as a child of a console that is
    // itself going, with nobody taking it out of the map first.
    if (mpHost) {
        mpHost->windowRegistry().deregisterLabel(mName, mpModel.get());
        // The tracker holds the movie raw and reads every entry it has to report
        if (mpMovie) {
            mpHost->getGifTracker()->unregisterGif(mpMovie);
        }
    }

    if (mpMovie) {
        mpMovie->deleteLater();
        mpMovie = nullptr;
    }
}

void TLabel::setText(const QString& text)
{
    const bool hasAnchor = containsAnchorTag(text);

    setTextInteractionFlags(hasAnchor ? scmLinkInteraction : Qt::TextInteractionFlags(Qt::NoTextInteraction));

    // If we have link styling configured and the text contains HTML links,
    // we need to inject inline styles because QTextDocument doesn't use
    // widget stylesheets or QPalette for link colors when a stylesheet exists
    if ((!mLinkColor.isEmpty() || !mLinkVisitedColor.isEmpty()) && hasAnchor) {
        QString styledText = text;

        // Replace all <a href="..."> tags with <a href="..." style="...">
        // Note: This regex is intentionally strict (lowercase, href first, no spacing around =)
        // because Mudlet's HTML generation (via echo(), setLabelText(), etc.) consistently
        // uses this format. User-provided HTML outside this pattern will still render as
        // clickable links (Qt handles that), but won't receive custom styling.
        static const QRegularExpression anchorRegex(qsl("<a\\s+href=([\"'][^\"']*[\"'])([^>]*)>"));
        QRegularExpressionMatchIterator it = anchorRegex.globalMatch(styledText);

        // Process matches in reverse order to avoid offset issues
        QList<QRegularExpressionMatch> matches;
        while (it.hasNext()) {
            matches.prepend(it.next());
        }

        for (const auto& match : matches) {
            QString fullMatch = match.captured(0);
            QString hrefPart = match.captured(1);   // The href="..." part
            QString otherAttrs = match.captured(2); // Other attributes

            // Extract the actual URL from hrefPart (remove quotes)
            QString url = hrefPart;
            url.remove(0, 1); // Remove opening quote
            url.chop(1);      // Remove closing quote

            bool isVisited = mVisitedLinks.contains(url);

            QString linkStyle;
            if (isVisited && !mLinkVisitedColor.isEmpty()) {
                linkStyle += qsl("color: %1; ").arg(mLinkVisitedColor);
            } else if (!mLinkColor.isEmpty()) {
                linkStyle += qsl("color: %1; ").arg(mLinkColor);
            }

            if (!mLinkUnderline) {
                linkStyle += qsl("text-decoration: none; ");
            }

            if (!linkStyle.isEmpty()) {
                linkStyle = linkStyle.trimmed();

                QString replacement;
                if (otherAttrs.contains(qsl("style="))) {
                    // Already has a style attribute - merge our styles
                    // This is complex, so for now just prepend our styles
                    replacement = qsl("<a href=%1 style=\"%2\"").arg(hrefPart, linkStyle);
                    // Intentionally overwrites any existing style attribute rather than merging
                    // to keep implementation simple for the common case (labels without pre-existing inline styles)
                    otherAttrs.remove(QRegularExpression(qsl("style=([\"'][^\"']*[\"'])")));
                    replacement += otherAttrs + qsl(">");
                } else {
                    replacement = qsl("<a href=%1 style=\"%2\"%3>").arg(hrefPart, linkStyle, otherAttrs);
                }

                styledText.replace(match.capturedStart(), match.capturedLength(), replacement);
            }
        }

        QLabel::setText(styledText);
    } else {
        QLabel::setText(text);
    }
}

bool TLabel::carriesLink() const
{
    return textFormat() == Qt::RichText && containsAnchorTag(text());
}

void TLabel::mousePressEvent(QMouseEvent* event)
{
    // QLabel needs the press to note which link it landed on, so the matching
    // release can activate it; with links-only flags it records the anchor and
    // leaves the press ignored, so the label's own click callback still runs.
    bool takenByQt = false;
    if (carriesLink()) {
        QLabel::mousePressEvent(event);
        takenByQt = event->isAccepted();
    }

    if (mpHost && mClickFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mClickFunction, event);
        // The use of accept() here prevents the propagation of the event to
        // any parent, e.g. the containing TConsole
        event->accept();
        mudlet::self()->activateProfile(mpHost);
    } else if (!takenByQt) {
        QWidget::mousePressEvent(event);
    }
}

void TLabel::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (mpHost && mDoubleClickFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mDoubleClickFunction, event);
        event->accept();
    } else {
        QWidget::mouseDoubleClickEvent(event);
    }
}

void TLabel::mouseReleaseEvent(QMouseEvent* event)
{
    // The release is where QLabel activates a link
    bool takenByQt = false;
    if (carriesLink()) {
        QLabel::mouseReleaseEvent(event);
        takenByQt = event->isAccepted();
    }

    auto labelParent = qobject_cast<TConsole*>(parent());
    if (labelParent && labelParent->mpDockWidget && labelParent->mpDockWidget->isFloating()) {
        // move focus back to the active console / command line:
        mudlet::self()->activateProfile(mpHost);
    }

    if (mpHost && mReleaseFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mReleaseFunction, event);
        event->accept();
    } else if (!takenByQt) {
        QWidget::mouseReleaseEvent(event);
    }
}

void TLabel::mouseMoveEvent(QMouseEvent* event)
{
    if (mpHost && mMoveFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mMoveFunction, event);
        event->accept();
    } else {
        QWidget::mouseMoveEvent(event);
    }
}

void TLabel::wheelEvent(QWheelEvent* event)
{
    if (mpHost && mWheelFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mWheelFunction, event);
        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }
}

void TLabel::leaveEvent(QEvent* event)
{
    if (mpHost && mLeaveFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mLeaveFunction, event);
        event->accept();
    } else {
        QWidget::leaveEvent(event);
    }
}

void TLabel::enterEvent(TEnterEvent* event)
{
    if (mpHost && mEnterFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mEnterFunction, event);
        event->accept();
    } else {
        QWidget::enterEvent(event);
    }
}

void TLabel::resizeEvent(QResizeEvent* event)
{
    emit resized();
    QWidget::resizeEvent(event);
}

// A label is game UI with its own right-click handling, so Qt's "Copy Link
// Location" menu over a link is left out. QWidget's rather than QLabel's: passing
// the event on untouched is what a plain widget does.
void TLabel::contextMenuEvent(QContextMenuEvent* event)
{
    QWidget::contextMenuEvent(event);
}


void TLabel::setClickThrough(bool clickthrough)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, clickthrough);

    // If clickthrough is enabled, text interaction (including hyperlinks) won't work
    // So we need to disable text interaction when clickthrough is on
    if (clickthrough) {
        setTextInteractionFlags(Qt::NoTextInteraction);
    } else {
        // Re-enable text interaction only if the current text has hyperlinks
        setTextInteractionFlags(containsAnchorTag(text()) ? scmLinkInteraction : Qt::TextInteractionFlags(Qt::NoTextInteraction));
    }
}

// the lookbehind keeps selection-background-color and friends out of it
static const QRegularExpression& backgroundColorDeclaration()
{
    static const QRegularExpression declaration(qsl("(?<![-\\w])background-color\\s*:[^;]*;"));
    return declaration;
}

void TLabel::setBackgroundColor(const QColor& color)
{
    mBackgroundColor = color;

    const QString newColor = qsl("background-color: rgba(%1, %2, %3, %4);").arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alpha());
    QString sheet = styleSheet();
    if (sheet.contains(backgroundColorDeclaration())) {
        sheet.replace(backgroundColorDeclaration(), newColor);
    } else {
        if (!sheet.isEmpty() && !sheet.endsWith(QChar::LineFeed)) {
            sheet.append(QChar::LineFeed);
        }
        sheet.append(newColor);
    }
    setStyleSheet(sheet);
}

// Qt hands a widget back the palette it saved when it first styled it, so every
// restyle - this label's own, an ancestor's, or the application's - drops the colour
// and leaves a label that fills its background on Qt's near-white default
void TLabel::changeEvent(QEvent* event)
{
    QLabel::changeEvent(event);

    if (event->type() == QEvent::StyleChange || event->type() == QEvent::PaletteChange) {
        applyBackgroundColor();
    }
}

void TLabel::applyBackgroundColor()
{
    // the equality test is also what stops setPalette() below recursing back in
    // through changeEvent()
    if (!mBackgroundColor.isValid() || palette().color(QPalette::Window) == mBackgroundColor) {
        return;
    }

    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, mBackgroundColor);
    setPalette(palette);
}

void TLabel::setLinkStyle(const QString& linkColor, const QString& linkVisitedColor, bool underline)
{
    mLinkColor = linkColor;
    mLinkVisitedColor = linkVisitedColor;
    mLinkUnderline = underline;

    // Set QPalette as a fallback (works if no stylesheet is set on the widget)
    QPalette palette = this->palette();

    if (!linkColor.isEmpty()) {
        QColor color(linkColor);
        palette.setColor(QPalette::Active, QPalette::Link, color);
        palette.setColor(QPalette::Inactive, QPalette::Link, color);
    }

    if (!linkVisitedColor.isEmpty()) {
        QColor color(linkVisitedColor);
        palette.setColor(QPalette::Active, QPalette::LinkVisited, color);
        palette.setColor(QPalette::Inactive, QPalette::LinkVisited, color);
    }

    setPalette(palette);

    // Note: Widget stylesheets don't affect QTextDocument rendering
    // Link colors are applied via inline styles in setText()

    // Force update to re-render with new styles
    update();
}

void TLabel::resetLinkStyle()
{
    // starting from a fresh palette would take the background colour with the link colours
    QPalette palette;
    if (mBackgroundColor.isValid()) {
        palette.setColor(QPalette::Window, mBackgroundColor);
    }
    setPalette(palette);

    mLinkColor.clear();
    mLinkVisitedColor.clear();
    mLinkUnderline = true;

    // Force update to re-render with new styles
    update();
}

void TLabel::clearVisitedLinks()
{
    mVisitedLinks.clear();

    QString currentText = text();
    if (!currentText.isEmpty() && containsAnchorTag(currentText)) {
        setText(currentText);
    }
}

void TLabel::slot_linkActivated(const QString& link)
{
    if (!mpHost) {
        return;
    }

    if (!mLinkVisitedColor.isEmpty()) {
        mVisitedLinks.insert(link);

        // Refresh the label to update link colors
        // We need to re-apply the current text to trigger the styling update
        QString currentText = text();
        if (!currentText.isEmpty() && containsAnchorTag(currentText)) {
            setText(currentText);
        }
    }

    // Check for custom schemes by looking for the colon separator
    const int colonPos = link.indexOf(':');

    if (colonPos > 0) {
        const QString scheme = link.left(colonPos).toLower(); // RFC 3986: schemes are case-insensitive
        const QString payload = link.mid(colonPos + 1);       // Everything after the colon

        // Handle custom Mudlet URL schemes for Lua commands
        if (scheme == qsl("send")) {
            // send: scheme - send the command to the MUD immediately
            mpHost->send(payload);
            return;
        }

        if (scheme == qsl("prompt")) {
            // prompt: scheme - put text in command line and wait for user to press enter
            if (mpHost->mpConsole && mpHost->mpConsole->mpCommandLine) {
                QPointer<TCommandLine> commandLine = mpHost->mpConsole->mpCommandLine;
                commandLine->setPlainText(payload);
                QTextCursor cursor = commandLine->textCursor();
                cursor.movePosition(QTextCursor::End);
                commandLine->setTextCursor(cursor);
                // Defer the focus operation to avoid issues with QPointer manipulation
                // during the signal handler execution
                QTimer::singleShot(0ms, commandLine.data(), [commandLine]() {
                    if (commandLine) {
                        commandLine->setFocus();
                    }
                });
            }
            return;
        }

        if (scheme == qsl("http") || scheme == qsl("https")) {
            QDesktopServices::openUrl(QUrl(link));
            return;
        }

        // Unknown scheme - ignore safely to prevent unintended Lua execution
        // Only links without a scheme should be treated as Lua code
        return;
    }

    // No scheme - treat as Lua code to execute
    mpHost->mLuaInterpreter.compileAndExecuteScript(link);
}
