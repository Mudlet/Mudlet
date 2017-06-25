/*
  Copyright (C) 2008-2016 The Communi Project

  You may use this file under the terms of BSD license as follows:

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "ircpalette.h"
#include "irc.h"

IRC_BEGIN_NAMESPACE

/*!
    \file ircpalette.h
    \brief \#include &lt;IrcPalette&gt;
 */

/*!
    \class IrcPalette ircpalette.h <IrcPalette>
    \ingroup util
    \brief Specifies a palette of IRC colors.

    IrcPalette is used to specify the desired IRC color palette when
    converting IRC-style formatted messages to HTML using IrcTextFormat.

    \code
    IrcTextFormat format;
    IrcPalette* palette = format.palette();
    palette->setColorName(Irc::Red, "#ff3333");
    palette->setColorName(Irc::Green, "#33ff33");
    palette->setColorName(Irc::Blue, "#3333ff");
    // ...

    QString html = format.toHtml(message);
    \endcode

    \sa Irc::Color, <a href="http://www.mirc.com/colors.html">mIRC colors</a>, <a href="http://www.w3.org/TR/SVG/types.html#ColorKeywords">SVG color keyword names</a>
 */

class IrcPalettePrivate
{
public:
    QMap<int, QString> colors;
};

static QMap<int, QString>& irc_default_colors()
{
    static QMap<int, QString> x;
    if (x.isEmpty()) {
        x.insert(Irc::White, QLatin1String("white"));
        x.insert(Irc::Black, QLatin1String("black"));
        x.insert(Irc::Blue, QLatin1String("blue"));
        x.insert(Irc::Green, QLatin1String("green"));
        x.insert(Irc::Red, QLatin1String("red"));
        x.insert(Irc::Brown, QLatin1String("brown"));
        x.insert(Irc::Purple, QLatin1String("purple"));
        x.insert(Irc::Orange, QLatin1String("orange"));
        x.insert(Irc::Yellow, QLatin1String("yellow"));
        x.insert(Irc::LightGreen, QLatin1String("lightgreen"));
        x.insert(Irc::Cyan, QLatin1String("cyan"));
        x.insert(Irc::LightCyan, QLatin1String("lightcyan"));
        x.insert(Irc::LightBlue, QLatin1String("lightblue"));
        x.insert(Irc::Pink, QLatin1String("pink"));
        x.insert(Irc::Gray, QLatin1String("gray"));
        x.insert(Irc::LightGray, QLatin1String("lightgray"));
    }
    return x;
}

/*!
    \internal
    Constructs a new palette with \a parent.
 */
IrcPalette::IrcPalette(QObject* parent) : QObject(parent), d_ptr(new IrcPalettePrivate)
{
    Q_D(IrcPalette);
    d->colors = irc_default_colors();
}

/*!
    \internal
    Destructs the palette.
 */
IrcPalette::~IrcPalette()
{
}

/*!
    This property holds the white color name.

    The default value is \c "white".

    \par Access functions:
    \li QString <b>white</b>() const
    \li void <b>setWhite</b>(const QString& color)

    \sa Irc::White
 */
QString IrcPalette::white() const
{
    Q_D(const IrcPalette);
    return d->colors.value(Irc::White);
}

void IrcPalette::setWhite(const QString& color)
{
    Q_D(IrcPalette);
    d->colors.insert(Irc::White, color);
}

/*!
    This property holds the black color name.

    The default value is \c "black".

    \par Access functions:
    \li QString <b>black</b>() const
    \li void <b>setBlack</b>(const QString& color)

    \sa Irc::Black
 */
QString IrcPalette::black() const
{
    Q_D(const IrcPalette);
    return d->colors.value(Irc::Black);
}

void IrcPalette::setBlack(const QString& color)
{
    Q_D(IrcPalette);
    d->colors.insert(Irc::Black, color);
}

/*!
    This property holds the blue color name.

    The default value is \c "blue".

    \par Access functions:
    \li QString <b>blue</b>() const
    \li void <b>setBlue</b>(const QString& color)

    \sa Irc::Blue
 */
QString IrcPalette::blue() const
{
    Q_D(const IrcPalette);
    return d->colors.value(Irc::Blue);
}

void IrcPalette::setBlue(const QString& color)
{
    Q_D(IrcPalette);
    d->colors.insert(Irc::Blue, color);
}

/*!
    This property holds the green color name.

    The default value is \c "green".

    \par Access functions:
    \li QString <b>green</b>() const
    \li void <b>setGreen</b>(const QString& color)

    \sa Irc::Green
 */
QString IrcPalette::green() const
{
    Q_D(const IrcPalette);
    return d->colors.value(Irc::Green);
}

void IrcPalette::setGreen(const QString& color)
{
    Q_D(IrcPalette);
    d->colors.insert(Irc::Green, color);
}

/*!
    This property holds the red color name.

    The default value is \c "red".

    \par Access functions:
    \li QString <b>red</b>() const
    \li void <b>setRed</b>(const QString& color)

    \sa Irc::Red
 */
QString IrcPalette::red() const
{
    Q_D(const IrcPalette);
    return d->colors.value(Irc::Red);
}

void IrcPalette::setRed(const QString& color)
{
    Q_D(IrcPalette);
    d->colors.insert(Irc::Red, color);
}

/*!
    This property holds the brown color name.

    The default value is \c "brown".

    \par Access functions:
    \li QString <b>brown</b>() const
    \li void <b>setBrown</b>(const QString& color)

    \sa Irc::Brown
 */
QString IrcPalette::brown() const
{
    Q_D(const IrcPalette);
    return d->colors.value(Irc::Brown);
}

void IrcPalette::setBrown(const QString& color)
{
    Q_D(IrcPalette);
    d->colors.insert(Irc::Brown, color);
}

/*!
    This property holds the purple color name.

    The default value is \c "purple".

    \par Access functions:
    \li QString <b>purple</b>() const
    \li void <b>setPurple</b>(const QString& color)

    \sa Irc::Purple
 */
QString IrcPalette::purple() const
{
    Q_D(const IrcPalette);
    return d->colors.value(Irc::Purple);
}

void IrcPalette::setPurple(const QString& color)
{
    Q_D(IrcPalette);
    d->colors.insert(Irc::Purple, color);
}

/*!
    This property holds the orange color name.

    The default value is \c "orange".

    \par Access functions:
    \li QString <b>orange</b>() const
    \li void <b>setOrange</b>(const QString& color)

    \sa Irc::Orange
 */
QString IrcPalette::orange() const
{
    Q_D(const IrcPalette);
    return d->colors.value(Irc::Orange);
}

void IrcPalette::setOrange(const QString& color)
{
    Q_D(IrcPalette);
    d->colors.insert(Irc::Orange, color);
}

/*!
    This property holds the yellow color name.

    The default value is \c "yellow".

    \par Access functions:
    \li QString <b>yellow</b>() const
    \li void <b>setYellow</b>(const QString& color)

    \sa Irc::Yellow
 */
QString IrcPalette::yellow() const
{
    Q_D(const IrcPalette);
    return d->colors.value(Irc::Yellow);
}

void IrcPalette::setYellow(const QString& color)
{
    Q_D(IrcPalette);
    d->colors.insert(Irc::Yellow, color);
}

/*!
    This property holds the light green color name.

    The default value is \c "lightgreen".

    \par Access functions:
    \li QString <b>lightGreen</b>() const
    \li void <b>setLightGreen</b>(const QString& color)

    \sa Irc::LightGreen
 */
QString IrcPalette::lightGreen() const
{
    Q_D(const IrcPalette);
    return d->colors.value(Irc::LightGreen);
}

void IrcPalette::setLightGreen(const QString& color)
{
    Q_D(IrcPalette);
    d->colors.insert(Irc::LightGreen, color);
}

/*!
    This property holds the cyan color name.

    The default value is \c "cyan".

    \par Access functions:
    \li QString <b>cyan</b>() const
    \li void <b>setCyan</b>(const QString& color)

    \sa Irc::Cyan
 */
QString IrcPalette::cyan() const
{
    Q_D(const IrcPalette);
    return d->colors.value(Irc::Cyan);
}

void IrcPalette::setCyan(const QString& color)
{
    Q_D(IrcPalette);
    d->colors.insert(Irc::Cyan, color);
}

/*!
    This property holds the light cyan color name.

    The default value is \c "lightcyan".

    \par Access functions:
    \li QString <b>lightCyan</b>() const
    \li void <b>setLightCyan</b>(const QString& color)

    \sa Irc::LightCyan
 */
QString IrcPalette::lightCyan() const
{
    Q_D(const IrcPalette);
    return d->colors.value(Irc::LightCyan);
}

void IrcPalette::setLightCyan(const QString& color)
{
    Q_D(IrcPalette);
    d->colors.insert(Irc::LightCyan, color);
}

/*!
    This property holds the light blue color name.

    The default value is \c "lightblue".

    \par Access functions:
    \li QString <b>lightBlue</b>() const
    \li void <b>setLightBlue</b>(const QString& color)

    \sa Irc::LightBlue
 */
QString IrcPalette::lightBlue() const
{
    Q_D(const IrcPalette);
    return d->colors.value(Irc::LightBlue);
}

void IrcPalette::setLightBlue(const QString& color)
{
    Q_D(IrcPalette);
    d->colors.insert(Irc::LightBlue, color);
}

/*!
    This property holds the pink color name.

    The default value is \c "pink".

    \par Access functions:
    \li QString <b>pink</b>() const
    \li void <b>setPink</b>(const QString& color)

    \sa Irc::Pink
 */
QString IrcPalette::pink() const
{
    Q_D(const IrcPalette);
    return d->colors.value(Irc::Pink);
}

void IrcPalette::setPink(const QString& color)
{
    Q_D(IrcPalette);
    d->colors.insert(Irc::Pink, color);
}

/*!
    This property holds the gray color name.

    The default value is \c "gray".

    \par Access functions:
    \li QString <b>gray</b>() const
    \li void <b>setGray</b>(const QString& color)

    \sa Irc::Gray
 */
QString IrcPalette::gray() const
{
    Q_D(const IrcPalette);
    return d->colors.value(Irc::Gray);
}

void IrcPalette::setGray(const QString& color)
{
    Q_D(IrcPalette);
    d->colors.insert(Irc::Gray, color);
}

/*!
    This property holds the light gray color name.

    The default value is \c "lightgray".

    \par Access functions:
    \li QString <b>lightGray</b>() const
    \li void <b>setLightGray</b>(const QString& color)

    \sa Irc::LightGray
 */
QString IrcPalette::lightGray() const
{
    Q_D(const IrcPalette);
    return d->colors.value(Irc::LightGray);
}

void IrcPalette::setLightGray(const QString& color)
{
    Q_D(IrcPalette);
    d->colors.insert(Irc::LightGray, color);
}

/*!
    Returns the map of color names.
 */
QMap<int, QString> IrcPalette::colorNames() const
{
    Q_D(const IrcPalette);
    return d->colors;
}

/*!
    Sets the map of color \a names.
 */
void IrcPalette::setColorNames(const QMap<int, QString>& names)
{
    Q_D(IrcPalette);
    d->colors = names;
}

/*!
    Converts a \a color code to a color name. If the \a color code
    is unknown, the function returns the \a fallback color name.
*/
QString IrcPalette::colorName(int color, const QString& fallback) const
{
    Q_D(const IrcPalette);
    return d->colors.value(color, fallback);
}

/*!
    Assigns a \a name for \a color code.

    The color \a name may be in one of these formats:

    \li \#RGB (each of R, G, and B is a single hex digit)
    \li \#RRGGBB
    \li \#RRRGGGBBB
    \li \#RRRRGGGGBBBB
    \li A name from the list of colors defined in the list of <a href="http://www.w3.org/TR/SVG/types.html#ColorKeywords">SVG color keyword names</a>
        provided by the World Wide Web Consortium; for example, "steelblue" or "gainsboro". These color names work on all platforms. Note that these
        color names are not the same as defined by the Qt::GlobalColor enums, e.g. "green" and Qt::green does not refer to the same color.
    \li transparent - representing the absence of a color.
*/
void IrcPalette::setColorName(int color, const QString& name)
{
    Q_D(IrcPalette);
    d->colors.insert(color, name);
}

#include "moc_ircpalette.cpp"

IRC_END_NAMESPACE
