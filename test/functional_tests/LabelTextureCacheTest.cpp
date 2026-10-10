/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

/*
 * Needs a real OpenGL context, so it skips under the offscreen platform CI uses;
 * run it with QT_QPA_PLATFORM=xcb on a display to exercise it.
 */

#include <QtTest/QtTest>

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>

#include <memory>

#include "LabelTextureCache.h"

#include "GroupedTest.h"

class LabelTextureCacheTest : public QObject
{
    Q_OBJECT

private:
    QOffscreenSurface mSurface;
    QOpenGLContext mContext;
    std::unique_ptr<LabelTextureCache> mpCache;
    QSize mTooWide;

    inline static int smUploadWarnings = 0;
    inline static QtMessageHandler smPreviousHandler = nullptr;

    static void countUploadWarnings(QtMsgType type, const QMessageLogContext& context, const QString& message)
    {
        if (type == QtWarningMsg && message.startsWith(qsl("LabelTextureCache"))) {
            ++smUploadWarnings;
        }
        smPreviousHandler(type, context, message);
    }

    QImage tooWideImage() const
    {
        QImage image(mTooWide, QImage::Format_ARGB32);
        image.fill(Qt::red);
        return image;
    }

    static QImage smallImage()
    {
        QImage image(8, 8, QImage::Format_ARGB32);
        image.fill(Qt::blue);
        return image;
    }

private slots:
    void initTestCase()
    {
        mSurface.create();
        if (!mContext.create() || !mContext.makeCurrent(&mSurface)) {
            QSKIP("no OpenGL context on this platform");
        }
        GLint maximumSize = 0;
        mContext.functions()->glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximumSize);
        QVERIFY(maximumSize > 0);
        // Over the driver's limit, so the upload fails with GL_INVALID_VALUE, but only one texel high
        mTooWide = QSize(maximumSize + 1, 1);
        mpCache = std::make_unique<LabelTextureCache>();
        mpCache->initialize();
        smPreviousHandler = qInstallMessageHandler(countUploadWarnings);
    }

    void cleanupTestCase()
    {
        if (smPreviousHandler) {
            qInstallMessageHandler(smPreviousHandler);
        }
        mpCache.reset();
        mContext.doneCurrent();
    }

    void init() { smUploadWarnings = 0; }

    void test_anImageTextureThatFailsIsNotRetried()
    {
        QCOMPARE(mpCache->addImageTexture(qsl("too wide"), tooWideImage()), GLuint(0));
        QCOMPARE(mpCache->addImageTexture(qsl("too wide"), tooWideImage()), GLuint(0));
        QCOMPARE(mpCache->imageTexture(qsl("too wide")), GLuint(0));
        QCOMPARE(smUploadWarnings, 1);
    }

    void test_aLabelThatFailsIsNotRetried()
    {
        const QPixmap tooWide = QPixmap::fromImage(tooWideImage());
        QCOMPARE(mpCache->getTexture(1, 1, tooWide), GLuint(0));
        QCOMPARE(mpCache->getTexture(1, 1, tooWide), GLuint(0));
        QCOMPARE(smUploadWarnings, 1);

        // A new pixmap is a new attempt
        QVERIFY(mpCache->getTexture(1, 1, QPixmap::fromImage(smallImage())) != 0);
    }

    // The texture a label had before its new pixmap failed is gone, and its name may since
    // have been handed out again
    void test_aLabelThatFailsAfterChangingDeletesItsOldTextureOnce()
    {
        const QPixmap tooWide = QPixmap::fromImage(tooWideImage());
        const GLuint before = mpCache->getTexture(1, 2, QPixmap::fromImage(smallImage()));
        QVERIFY(before != 0);
        QCOMPARE(mpCache->getTexture(1, 2, tooWide), GLuint(0));

        const GLuint other = mpCache->addImageTexture(qsl("made after the failure"), smallImage());
        QVERIFY(other != 0);
        mpCache->getTexture(1, 2, tooWide);
        QVERIFY2(mContext.functions()->glIsTexture(other), "a label deleted a texture that was not its own");
        QCOMPARE(mpCache->imageTexture(qsl("made after the failure")), other);
    }

    void test_makingRoomKeepsTheTexturesInUse()
    {
        mpCache->clearAll();
        for (const QString& key : {qsl("a"), qsl("b"), qsl("c")}) {
            QVERIFY(mpCache->addImageTexture(key, smallImage()) != 0);
        }
        const GLuint kept = mpCache->imageTexture(qsl("a"));

        QVERIFY(mpCache->makeRoomForImageTexture(3, {qsl("a")}));
        QCOMPARE(mpCache->imageTexture(qsl("a")), kept);
        QVERIFY(mContext.functions()->glIsTexture(kept));

        QVERIFY(mpCache->addImageTexture(qsl("d"), smallImage()) != 0);
        const QSet<QString> all{qsl("a"), qsl("b"), qsl("c"), qsl("d")};
        QSet<QString> cached;
        for (const QString& key : all) {
            if (mpCache->imageTexture(key)) {
                cached.insert(key);
            }
        }
        QCOMPARE(cached.size(), 3);
        QVERIFY2(!mpCache->makeRoomForImageTexture(3, cached), "a texture in use was dropped to make room");
        for (const QString& key : cached) {
            QVERIFY(mpCache->imageTexture(key) != 0);
        }
    }
};

#include "LabelTextureCacheTest.moc"
MUDLET_GROUPED_TEST_MAIN(LabelTextureCacheTest)
