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
 * Needs a real OpenGL context, so it skips under the offscreen platform ctest sets,
 * and ctest lists it as not run. Run the group binary directly to exercise it:
 * QT_QPA_PLATFORM=xcb functional_map_tests LabelTextureCacheTest, on a display.
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
        // Only the upload warning: a failed image texture also logs its key, and a retry would repeat the upload
        if (type == QtWarningMsg && message.startsWith(qsl("LabelTextureCache: could not upload"))) {
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

        // Clearing the cache, as a new GL context does, is a new attempt
        mpCache->clearAll();
        QVERIFY(mpCache->addImageTexture(qsl("too wide"), smallImage()) != 0);
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
        if (other != before) {
            QSKIP("the driver did not hand the freed name out again, so deleting it twice cannot be seen");
        }
        mpCache->getTexture(1, 2, tooWide);
        QVERIFY2(mContext.functions()->glIsTexture(other), "a label deleted a texture that was not its own");
        QCOMPARE(mpCache->imageTexture(qsl("made after the failure")), other);
    }

    void test_makingRoomKeepsTheTexturesInUse()
    {
        mpCache->clearAll();
        QSet<QString> inUse;
        QHash<QString, GLuint> made;
        for (int i = 0; i < 10; ++i) {
            const QString key = qsl("key %1").arg(i);
            made.insert(key, mpCache->addImageTexture(key, smallImage()));
            QVERIFY(made.value(key) != 0);
            if (i < 5) {
                inUse.insert(key);
            }
        }

        // Exactly the five not in use must go; ignoring the ones in use would pick those five from
        // the cache's hash order only one time in 252
        QVERIFY(mpCache->makeRoomForImageTexture(6, inUse));
        for (auto it = made.cbegin(); it != made.cend(); ++it) {
            if (inUse.contains(it.key())) {
                QCOMPARE(mpCache->imageTexture(it.key()), it.value());
                QVERIFY(mContext.functions()->glIsTexture(it.value()));
            } else {
                QCOMPARE(mpCache->imageTexture(it.key()), GLuint(0));
                QVERIFY2(!mContext.functions()->glIsTexture(it.value()), "a texture dropped to make room was not deleted");
            }
        }

        QVERIFY(mpCache->addImageTexture(qsl("one more"), smallImage()) != 0);
        inUse.insert(qsl("one more"));
        QVERIFY2(!mpCache->makeRoomForImageTexture(6, inUse), "a texture in use was dropped to make room");
        for (const QString& key : std::as_const(inUse)) {
            QVERIFY(mpCache->imageTexture(key) != 0);
        }
    }
};

#include "LabelTextureCacheTest.moc"
MUDLET_GROUPED_TEST_MAIN(LabelTextureCacheTest)
