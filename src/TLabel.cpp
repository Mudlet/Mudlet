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
#include <QFile>
#include <QPainter>
#include <QRegularExpression>
#include <QSvgRenderer>
#include <QTextCursor>
#include <QTextDocumentFragment>
#include <QTimer>
#include <QUrl>
#include <QtEvents>
#include <chrono>

using namespace std::chrono_literals;

// A case-insensitive text.contains("<a ") answers the same, but it case-folds every
// character it walks, and Lua UIs echo into labels on every prompt. Any whitespace
// counts as the separator because HTML allows any; the styling pass in setText()
// recognises only the ASCII ones, so an anchor split by a non-breaking space comes
// out clickable but unstyled.
static bool containsAnchorTag(const QString& text)
{
    qsizetype from = 0;
    while (true) {
        const qsizetype at = text.indexOf(QLatin1Char('<'), from);
        // Too near the end for a tag name and a separator, and every later '<' is
        // nearer still, so there is nothing left to find
        if (at < 0 || at + 2 >= text.size()) {
            return false;
        }
        const char16_t tagName = text.at(at + 1).unicode();
        if ((tagName == u'a' || tagName == u'A') && text.at(at + 2).isSpace()) {
            return true;
        }
        from = at + 1;
    }
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
, mSvgTintColor(mpModel->mSvgTintColor)
, mSvgRotation(mpModel->mSvgRotation)
, mSvgShearX(mpModel->mSvgShearX)
, mSvgShearY(mpModel->mSvgShearY)
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

    // Enable TextBrowserInteraction only when the label contains hyperlinks
    // This prevents Qt's default context menu from appearing on labels without links
    if (hasAnchor) {
        setTextInteractionFlags(Qt::TextBrowserInteraction);
    } else {
        setTextInteractionFlags(Qt::NoTextInteraction);
    }

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

        stopMovie();
        QLabel::setText(styledText);
    } else {
        stopMovie();
        QLabel::setText(text);
    }
}

void TLabel::mousePressEvent(QMouseEvent* event)
{
    // If the label has rich text with potential hyperlinks, let QLabel handle the event first
    // QLabel will emit linkActivated if a link was clicked
    if (!text().isEmpty() && textFormat() == Qt::RichText && containsAnchorTag(text())) {
        QLabel::mousePressEvent(event);
        // If QLabel didn't accept the event, then it wasn't a link click
        if (event->isAccepted()) {
            return;
        }
    }

    if (mpHost && mClickFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mClickFunction, event);
        // The use of accept() here prevents the propagation of the event to
        // any parent, e.g. the containing TConsole
        event->accept();
        mudlet::self()->activateProfile(mpHost);
    } else {
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
    // If the label has rich text with potential hyperlinks, let QLabel handle the event first
    if (!text().isEmpty() && textFormat() == Qt::RichText && containsAnchorTag(text())) {
        QLabel::mouseReleaseEvent(event);
        // If QLabel accepted the event, it was handling a link click
        if (event->isAccepted()) {
            return;
        }
    }

    auto labelParent = qobject_cast<TConsole*>(parent());
    if (labelParent && labelParent->mpDockWidget && labelParent->mpDockWidget->isFloating()) {
        // move focus back to the active console / command line:
        mudlet::self()->activateProfile(mpHost);
    }

    if (mpHost && mReleaseFunction) {
        mpHost->getLuaInterpreter()->callLabelCallbackEvent(mReleaseFunction, event);
        event->accept();
    } else {
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

// The SVG is a layer of its own rather than the label's content, because QLabel
// keeps text, a pixmap and a movie in a single slot where each replaces the last.
// Qt paints a QFrame's palette and stylesheet backgrounds before it delivers the
// paint event, so drawing here puts the SVG over the label's background colour
// and under whatever the label is showing.
void TLabel::paintEvent(QPaintEvent* event)
{
    // QLabel draws its content inside contentsRect(), so the layer belongs there
    // too rather than over a stylesheet border or in its padding
    if (const QRect area = contentsRect(); mpSvgRenderer && !area.isEmpty()) {
        // the cache has to be compared in device pixels with the rounding the
        // render uses, or a fractional ratio leaves the test never matching and
        // the document re-rendered on every paint
        const qreal dpr = devicePixelRatioF();
        if (mSvgPixmapCache.size() != area.size() * dpr || !qFuzzyCompare(mSvgPixmapCache.devicePixelRatio(), dpr)) {
            mSvgPixmapCache = renderSvgPixmap(area.size());
        }
        QPainter painter(this);
        painter.drawPixmap(area.topLeft(), mSvgPixmapCache);
    }

    QLabel::paintEvent(event);
}

// QLabel takes its hint from the text, pixmap or movie in its content slot, and
// the SVG is none of those - it is a layer of this class's own, which QLabel's
// hint cannot see. Geyser's autoAdjustSize() sizes a label from the hint, so a
// label showing nothing but an SVG has to answer with the document plus the same
// extras QLabel would add around a raster of that size. A label that does carry
// content keeps QLabel's answer, because the SVG scales to fit whatever size the
// content asks for and its own size says nothing. Geyser.Label:new always echoes
// an empty rich-text div, so what counts as visible text is what the document
// renders rather than whether the string is empty.
QSize TLabel::sizeHint() const
{
    if (!mpSvgRenderer) {
        return QLabel::sizeHint();
    }

    const bool showsContent = !pixmap().isNull() || movie() || !QTextDocumentFragment::fromHtml(text()).toPlainText().trimmed().isEmpty();
    const QSize documentSize = mpSvgRenderer->defaultSize();
    // a document without width, height or viewBox has no size to offer
    if (showsContent || documentSize.isEmpty()) {
        return QLabel::sizeHint();
    }
    return documentSize.grownBy(contentsMargins()) + QSize(2 * margin(), 2 * margin());
}

// QPixmap and QImage read a file by its content, so a raster saved under a .svg
// name has always displayed. This only asks whether the renderer is worth trying:
// the renderer itself is the authority on what is an SVG.
bool TLabel::svgCandidate(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QByteArray head = file.read(64);
    if (head.startsWith(QByteArrayLiteral("\x1f\x8b"))) {
        return true;
    }

    qsizetype at = 0;
    bool utf16 = false;
    if (head.startsWith(QByteArrayLiteral("\xef\xbb\xbf"))) {
        at = 3;
    } else if (head.startsWith(QByteArrayLiteral("\xff\xfe")) || head.startsWith(QByteArrayLiteral("\xfe\xff"))) {
        at = 2;
        utf16 = true;
    }
    for (; at < head.size(); ++at) {
        const char byte = head.at(at);
        // in UTF-16 every ASCII character is half of a code unit whose other half
        // is a NUL, whichever way round the byte order mark put them
        if (utf16 && byte == '\0') {
            continue;
        }
        if (byte == ' ' || byte == '\t' || byte == '\n' || byte == '\r' || byte == '\f' || byte == '\v') {
            continue;
        }
        return byte == '<';
    }
    return false;
}

bool TLabel::loadSvg(QSvgRenderer& renderer, const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    // QSvgRenderer inflates a gzipped document by its content on the QByteArray
    // overload, but only by a .svgz or .svg.gz name on the path one
    if (file.peek(2) == QByteArrayLiteral("\x1f\x8b")) {
        renderer.load(file.readAll());
    } else {
        file.close();
        // the path overload resolves a relative href inside the document against
        // the directory the document sits in
        renderer.load(path);
    }
    return renderer.isValid();
}

bool TLabel::setBackgroundImage(const QString& path)
{
    if (svgCandidate(path) && setSvgImage(path)) {
        return true;
    }

    // the label keeps what it is showing when the file is not an image at all,
    // and the caller's error message is then the truth
    const QPixmap raster(path);
    if (raster.isNull()) {
        return false;
    }

    clearSvgImage();
    stopMovie();
    setPixmap(raster);
    return true;
}

bool TLabel::setSvgImage(const QString& path)
{
    // the new document has to prove readable before the current one goes, or a
    // mistyped path takes the SVG down with it
    auto* renderer = new QSvgRenderer(this);
    if (!loadSvg(*renderer, path)) {
        delete renderer;
        return false;
    }

    // the tint and the transforms belong to the label rather than to the document,
    // so they carry over to the new image - and one set before any SVG arrived
    // applies as soon as it does
    delete mpSvgRenderer;
    mpSvgRenderer = renderer;
    // a raster background is the label's content, so it would otherwise stay on
    // top of the new SVG layer - text and movies are left alone
    if (!pixmap().isNull()) {
        QLabel::clear();
    }
    refreshSvg();
    updateGeometry();
    return true;
}

void TLabel::resetBackgroundImage()
{
    // the SVG layer and whatever image sits in QLabel's content slot go together;
    // text is not an image and stays
    clearSvgImage();
    if (!pixmap().isNull() || movie()) {
        stopMovie();
        clear();
    }
}

void TLabel::stopMovie()
{
    if (auto* pMovie = movie()) {
        // the Host hangs on to this movie to reuse it, so an unstopped one would
        // go on decoding frames for a label that no longer shows it
        pMovie->stop();
    }
}

QPixmap TLabel::renderSvgPixmap(const QSize& size) const
{
    const qreal dpr = devicePixelRatioF();
    QPixmap svgPixmap(size * dpr);
    svgPixmap.fill(Qt::transparent);
    QPainter painter(&svgPixmap);

    if (!qFuzzyIsNull(mSvgRotation) || !qFuzzyIsNull(mSvgShearX) || !qFuzzyIsNull(mSvgShearY)) {
        const qreal cx = svgPixmap.width() / 2.0;
        const qreal cy = svgPixmap.height() / 2.0;
        painter.translate(cx, cy);
        painter.rotate(mSvgRotation);
        painter.shear(mSvgShearX, mSvgShearY);
        painter.translate(-cx, -cy);
    }

    // QSvgRenderer honours its aspect ratio mode only for a document with an
    // explicit viewBox, so the fit is done here
    QRectF targetRect(svgPixmap.rect());
    if (const QSizeF documentSize(mpSvgRenderer->defaultSize()); !documentSize.isEmpty()) {
        const QSizeF fitted = documentSize.scaled(QSizeF(svgPixmap.size()), Qt::KeepAspectRatio);
        targetRect = QRectF(QPointF((svgPixmap.width() - fitted.width()) / 2.0, (svgPixmap.height() - fitted.height()) / 2.0), fitted);
    }
    mpSvgRenderer->render(&painter, targetRect);

    if (mSvgTintColor.isValid()) {
        painter.resetTransform();
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(svgPixmap.rect(), mSvgTintColor);
    }

    painter.end();
    svgPixmap.setDevicePixelRatio(dpr);
    return svgPixmap;
}

// the tint and the transforms are the label's, not the document's, so only their
// own reset functions clear them
void TLabel::clearSvgImage()
{
    if (!mpSvgRenderer) {
        return;
    }
    delete mpSvgRenderer;
    mpSvgRenderer = nullptr;
    refreshSvg();
    // the size hint answered from the document while there was one
    updateGeometry();
}

void TLabel::refreshSvg()
{
    mSvgPixmapCache = QPixmap();
    update();
}

void TLabel::setSvgTint(const QColor& color)
{
    mSvgTintColor = color;
    refreshSvg();
}

void TLabel::clearSvgTint()
{
    mSvgTintColor = QColor();
    refreshSvg();
}

void TLabel::setSvgRotation(double angle)
{
    mSvgRotation = angle;
    refreshSvg();
}

void TLabel::setSvgShear(double shearX, double shearY)
{
    mSvgShearX = shearX;
    mSvgShearY = shearY;
    refreshSvg();
}

void TLabel::resetSvgTransform()
{
    mSvgRotation = 0.0;
    mSvgShearX = 0.0;
    mSvgShearY = 0.0;
    refreshSvg();
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
        if (containsAnchorTag(text())) {
            setTextInteractionFlags(Qt::TextBrowserInteraction);
        } else {
            setTextInteractionFlags(Qt::NoTextInteraction);
        }
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
