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

#include "TLabelModel.h"

#include "Host.h"

#include <QFile>
#include <QImage>
#include <QSvgRenderer>

TLabelModel::TLabelModel(Host* pHost, const QString& name)
: mpHost(pHost)
, mName(name)
{
}

TLabelModel::~TLabelModel()
{
    if (!mpHost) {
        return;
    }

    auto* interpreter = mpHost->getLuaInterpreter();
    for (const int funcRef : {mClickFunction, mDoubleClickFunction, mReleaseFunction, mMoveFunction, mWheelFunction, mEnterFunction, mLeaveFunction}) {
        if (funcRef) {
            interpreter->freeLuaRegistryIndex(funcRef);
        }
    }
}

void TLabelModel::setClick(const int func)
{
    releaseFunc(mClickFunction, func);
    mClickFunction = func;
}

void TLabelModel::setDoubleClick(const int func)
{
    releaseFunc(mDoubleClickFunction, func);
    mDoubleClickFunction = func;
}

void TLabelModel::setRelease(const int func)
{
    releaseFunc(mReleaseFunction, func);
    mReleaseFunction = func;
}

void TLabelModel::setMove(const int func)
{
    releaseFunc(mMoveFunction, func);
    mMoveFunction = func;
}

void TLabelModel::setWheel(const int func)
{
    releaseFunc(mWheelFunction, func);
    mWheelFunction = func;
}

void TLabelModel::setEnter(const int func)
{
    releaseFunc(mEnterFunction, func);
    mEnterFunction = func;
}

void TLabelModel::setLeave(const int func)
{
    releaseFunc(mLeaveFunction, func);
    mLeaveFunction = func;
}

void TLabelModel::releaseFunc(const int existingFunction, const int newFunction)
{
    if (!mpHost) {
        return;
    }

    if (newFunction != existingFunction) {
        mpHost->getLuaInterpreter()->freeLuaRegistryIndex(existingFunction);
    }
}

// QPixmap and QImage read a file by its content, so a raster saved under a .svg
// name has always displayed. This only asks whether the renderer is worth trying:
// the renderer itself is the authority on what is an SVG.
bool TLabelModel::svgCandidate(const QString& path)
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

bool TLabelModel::loadSvg(QSvgRenderer& renderer, const QString& path)
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

std::optional<QSize> TLabelModel::imageSize(const QString& path)
{
    // QImage reads an SVG only where the qsvg image plugin is deployed, so the
    // document's own reader answers first; anything it cannot read - a raster
    // under a .svg name included - falls through to QImage
    if (svgCandidate(path)) {
        QSvgRenderer renderer;
        if (loadSvg(renderer, path) && !renderer.defaultSize().isEmpty()) {
            return renderer.defaultSize();
        }
    }

    const QImage image(path);

    if (image.isNull()) {
        return {};
    }

    return image.size();
}
