// Temporary reproducer for QTBUG-147602 (QImage::toCGImage() hands CGImageCreate a released colour space)
#include <QApplication>
#include <QColorSpace>
#include <QCursor>
#include <QImage>
#include <QTimer>
#include <QWidget>

#include <CoreGraphics/CoreGraphics.h>
#include <cstdio>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    const QString mode = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("cursor");
    std::printf("Qt runtime %s, mode %s\n", qVersion(), qPrintable(mode));
    std::fflush(stdout);

    if (mode == QLatin1String("image")) {
        QImage image(QStringLiteral(":/qt-project.org/mac/cursors/images/spincursor.png"));
        const bool fromResource = !image.isNull();
        if (!fromResource && argc > 2) {
            image = QImage(QString::fromLocal8Bit(argv[2]));
        }
        std::printf("image from %s: null=%d format=%d colorSpace valid=%d description='%s' icc bytes=%lld\n",
                    fromResource ? "Qt resource" : "file", image.isNull(), int(image.format()), image.colorSpace().isValid(),
                    qPrintable(image.colorSpace().description()), qlonglong(image.colorSpace().iccProfile().size()));
        std::fflush(stdout);
        for (int i = 0; i < 2000; ++i) {
            CGImageRef cgImage = image.toCGImage();
            CGColorSpaceRef colorSpace = CGImageGetColorSpace(cgImage);
            if (i == 0) {
                std::printf("first CGImage: model=%d colour space retain count=%ld\n", int(CGColorSpaceGetModel(colorSpace)), long(CFGetRetainCount(colorSpace)));
                std::fflush(stdout);
            }
            CGImageRelease(cgImage);
        }
        std::puts("image: survived 2000 conversions");
        return 0;
    }

    // Mirrors Mudlet's startup: a wait override cursor is active when the main window is first shown
    app.setOverrideCursor(QCursor(Qt::WaitCursor));
    QWidget window;
    window.resize(300, 200);
    window.show();
    QTimer::singleShot(400, &window, [&]() {
        app.restoreOverrideCursor();
        window.setCursor(Qt::SizeAllCursor);
    });
    QTimer::singleShot(1200, &app, &QApplication::quit);
    const int result = app.exec();
    std::puts("cursor: survived show() with a wait override cursor and a size-all cursor");
    return result;
}
