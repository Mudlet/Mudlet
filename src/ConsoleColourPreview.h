#ifndef MUDLET_CONSOLECOLOURPREVIEW_H
#define MUDLET_CONSOLECOLOURPREVIEW_H

/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers - mudlet@mudlet.org           *
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

#include <QPixmap>
#include <QPointer>
#include <QWidget>

class Host;
class QPainter;

namespace uiDesign {

// A few lines of a game drawn the way the main display draws them, in the profile's colours and display font and over
// any picture a script set behind the main console, so a choice is seen in use: black words on a black background
// show here, where two swatches side by side say nothing.
// Shows only; the colours are read from the Host at every paint, so whoever changes one calls update().
class ConsoleColourPreview : public QWidget
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(ConsoleColourPreview)
    explicit ConsoleColourPreview(QWidget* pParent = nullptr);

    void setHost(Host* pHost);
    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

protected:
    void changeEvent(QEvent* pEvent) override;
    void paintEvent(QPaintEvent* pEvent) override;
    void showEvent(QShowEvent* pEvent) override;

private:
    // A picture a script put behind the main console, loaded from its file once rather than at every paint
    struct Picture
    {
        QString path;
        QPixmap loaded;
        // The part of it last stretched over the preview, scaled to the device's pixels so it is drawn sharp
        QRect fittedPart;
        QPixmap fitted;
    };

    void retranslate();
    static void paintPicture(QPainter& painter, Picture& picture, const QString& path, int mode, const QRect& area);

    QPointer<Host> mpHost;
    Picture mWindowPicture;
    Picture mDisplayPicture;
};

} // namespace uiDesign

#endif // MUDLET_CONSOLECOLOURPREVIEW_H
