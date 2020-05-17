#include <QtTest/QtTest>
#include <TBufferStyle.h>

class TBufferStyleTest : public QObject {
    Q_OBJECT

private:
private slots:

    void initTestCase()
    {
    }

    void testDecodeSGR38Code5Regular()
    {
        TBufferStyle bufStyle;

        // code 5, values 0 to 7
        bufStyle.decodeSGR38(QStringList({"", "5", "0"}), false);
        QCOMPARE(bufStyle.mFgColor, bufStyle.mBlack);
        QCOMPARE(bufStyle.mFgColorLight, bufStyle.mLightBlack);
        QCOMPARE(bufStyle.mBold, false);

        bufStyle.decodeSGR38(QStringList({"", "5", "1"}), false);
        QCOMPARE(bufStyle.mFgColor, bufStyle.mRed);
        QCOMPARE(bufStyle.mFgColorLight, bufStyle.mLightRed);
        QCOMPARE(bufStyle.mBold, false);

        bufStyle.decodeSGR38(QStringList({"", "5", "7"}), false);
        QCOMPARE(bufStyle.mFgColor, bufStyle.mWhite);
        QCOMPARE(bufStyle.mFgColorLight, bufStyle.mLightWhite);
        QCOMPARE(bufStyle.mBold, false);
    }

    void testDecodeSGR38Code5Bold()
    {
        TBufferStyle bufStyle;

        // code 5, values 8 to 15 (bold)
        bufStyle.mBold = false;
        bufStyle.decodeSGR38(QStringList({"", "5", "8"}), false);
        QCOMPARE(bufStyle.mFgColor, bufStyle.mBlack);
        QCOMPARE(bufStyle.mFgColorLight, bufStyle.mLightBlack);
        QCOMPARE(bufStyle.mBold, true);

        bufStyle.mBold = false;
        bufStyle.decodeSGR38(QStringList({"", "5", "9"}), false);
        QCOMPARE(bufStyle.mFgColor, bufStyle.mRed);
        QCOMPARE(bufStyle.mFgColorLight, bufStyle.mLightRed);
        QCOMPARE(bufStyle.mBold, true);

        bufStyle.mBold = false;
        bufStyle.decodeSGR38(QStringList({"", "5", "15"}), false);
        QCOMPARE(bufStyle.mFgColor, bufStyle.mWhite);
        QCOMPARE(bufStyle.mFgColorLight, bufStyle.mLightWhite);
        QCOMPARE(bufStyle.mBold, true);
    }

    void testDecodeSGR38Code5Undefined()
    {
        TBufferStyle bufStyle;
        // code 5, undefined/invalid value (should default to black)
        bufStyle.decodeSGR38(QStringList({"", "5", ""}), false);
        QCOMPARE(bufStyle.mFgColor, bufStyle.mBlack);
        QCOMPARE(bufStyle.mFgColorLight, bufStyle.mLightBlack);
        QCOMPARE(bufStyle.mBold, false);
    }

    void testDecodeSGR38Code5_6x6x6ColorSpace()
    {
        TBufferStyle bufStyle;

        // code 5, values 16 to 231 (6x6x6 RGB color space)
        bufStyle.mBold = false;// shouldn't change variable bold

        bufStyle.decodeSGR38(QStringList({"", "5", "16"}), false);
        QCOMPARE(bufStyle.mFgColor, QColor("#000000"));
        QCOMPARE(bufStyle.mFgColorLight, QColor("#000000"));
        QCOMPARE(bufStyle.mBold, false);

        bufStyle.decodeSGR38(QStringList({"", "5", "16"}), false);
        bufStyle.mBold = true;
        QCOMPARE(bufStyle.mFgColor, QColor("#000000"));
        QCOMPARE(bufStyle.mFgColorLight, QColor("#000000"));
        QCOMPARE(bufStyle.mBold, true);

        bufStyle.decodeSGR38(QStringList({"", "5", "231"}), false);
        QCOMPARE(bufStyle.mFgColor, QColor("#ffffff"));
        QCOMPARE(bufStyle.mFgColorLight, QColor("#ffffff"));

        bufStyle.decodeSGR38(QStringList({"", "5", "124"}), false);
        QCOMPARE(bufStyle.mFgColor, QColor(3 * 51, 0, 0));
        QCOMPARE(bufStyle.mFgColorLight, QColor(3 * 51, 0, 0));
    }

    void testDecodeSGR38Code5_Grayscale()
    {
        TBufferStyle bufStyle;

        // code 5, values 232 to 255 (black+23 grayscale)
        bufStyle.mBold = false;// shouldn't change variable bold

        bufStyle.decodeSGR38(QStringList({"", "5", "232"}), false);
        QCOMPARE(bufStyle.mFgColor, QColor("#000000"));
        QCOMPARE(bufStyle.mFgColorLight, QColor("#000000"));
        QCOMPARE(bufStyle.mBold, false);

        bufStyle.mBold = true;
        bufStyle.decodeSGR38(QStringList({"", "5", "238"}), false);
        QCOMPARE(bufStyle.mFgColor, QColor(67, 67, 67));
        QCOMPARE(bufStyle.mFgColorLight, QColor(67, 67, 67));
        QCOMPARE(bufStyle.mBold, true);

        bufStyle.decodeSGR38(QStringList({"", "5", "255"}), false);
        QCOMPARE(bufStyle.mFgColor, QColor(255, 255, 255));
        QCOMPARE(bufStyle.mFgColorLight, QColor(255, 255, 255));
    }

    void testDecodeSGR38Code5_UndefinedGrayscaleValue()
    {
        TBufferStyle bufStyle;

        // code 5, values 232 to 255 (black+23 grayscale)
        bufStyle.decodeSGR38(QStringList({"", "5", "258"}), false);
        QCOMPARE(bufStyle.mFgColor, QColor(192, 192, 192));
        QCOMPARE(bufStyle.mFgColorLight, QColor(192, 192, 192));

        bufStyle.decodeSGR38(QStringList({"", "5", "270"}), false);
        QCOMPARE(bufStyle.mFgColor, QColor(192, 192, 192));
        QCOMPARE(bufStyle.mFgColorLight, QColor(192, 192, 192));
    }

    void testDecodeSGR38Code2_6Params()
    {
        TBufferStyle bufStyle;

        // code 2, 6 parameters
        bufStyle.decodeSGR38(QStringList({"", "2", "", "0", "0", "0"}), false);
        QCOMPARE(bufStyle.mFgColor, QColor(0, 0, 0));
        QCOMPARE(bufStyle.mFgColorLight, QColor(0, 0, 0));

        // code 2, 6 parameters
        bufStyle.decodeSGR38(QStringList({"", "2", "", "255", "0", "0"}), false);
        QCOMPARE(bufStyle.mFgColor, QColor(255, 0, 0));
        QCOMPARE(bufStyle.mFgColorLight, QColor(255, 0, 0));

        // code 2, 5 parameters
        bufStyle.decodeSGR38(QStringList({"", "2", "", "255", "100"}), false);
        QCOMPARE(bufStyle.mFgColor, QColor(255, 100, 0));
        QCOMPARE(bufStyle.mFgColorLight, QColor(255, 100, 0));

        // code 2, 4 parameters
        bufStyle.decodeSGR38(QStringList({"", "2", "", "100"}), false);
        QCOMPARE(bufStyle.mFgColor, QColor(100, 0, 0));
        QCOMPARE(bufStyle.mFgColorLight, QColor(100, 0, 0));

        // code 2, 3 parameters
        bufStyle.decodeSGR38(QStringList({"", "2", ""}), false);
        QCOMPARE(bufStyle.mFgColor, Qt::black);
        QCOMPARE(bufStyle.mFgColorLight, Qt::black);
    }

    void testDecodeSGR48Code5Regular()
    {
        TBufferStyle bufStyle;

        // code 5, values 0 to 7
        bufStyle.decodeSGR48(QStringList({"", "5", "0"}), false);
        QCOMPARE(bufStyle.mBgColor, bufStyle.mBlack);

        bufStyle.decodeSGR48(QStringList({"", "5", "1"}), false);
        QCOMPARE(bufStyle.mBgColor, bufStyle.mRed);

        bufStyle.decodeSGR48(QStringList({"", "5", "7"}), false);
        QCOMPARE(bufStyle.mBgColor, bufStyle.mWhite);
    }

    void testDecodeSGR48Code5Light()
    {
        TBufferStyle bufStyle;

        // code 5, values 8 to 15 (bold)
        bufStyle.decodeSGR48(QStringList({"", "5", "8"}), false);
        QCOMPARE(bufStyle.mBgColor, bufStyle.mLightBlack);

        bufStyle.decodeSGR48(QStringList({"", "5", "9"}), false);
        QCOMPARE(bufStyle.mBgColor, bufStyle.mLightRed);

        bufStyle.decodeSGR48(QStringList({"", "5", "15"}), false);
        QCOMPARE(bufStyle.mBgColor, bufStyle.mLightWhite);
    }

    void testDecodeSGR48Code5Undefined()
    {
        TBufferStyle bufStyle;
        // code 5, undefined/invalid value (should default to black)
        bufStyle.decodeSGR48(QStringList({"", "5", ""}), false);
        QCOMPARE(bufStyle.mBgColor, bufStyle.mBlack);
        QCOMPARE(bufStyle.mBold, false);
    }

    void testDecodeSGR48Code5_6x6x6ColorSpace()
    {
        TBufferStyle bufStyle;

        // code 5, values 16 to 231 (6x6x6 RGB color space)

        bufStyle.decodeSGR48(QStringList({"", "5", "16"}), false);
        QCOMPARE(bufStyle.mBgColor, QColor("#000000"));

        bufStyle.decodeSGR48(QStringList({"", "5", "16"}), false);
        QCOMPARE(bufStyle.mBgColor, QColor("#000000"));

        bufStyle.decodeSGR48(QStringList({"", "5", "231"}), false);
        QCOMPARE(bufStyle.mBgColor, QColor("#ffffff"));

        bufStyle.decodeSGR48(QStringList({"", "5", "124"}), false);
        QCOMPARE(bufStyle.mBgColor, QColor(3 * 51, 0, 0));
    }

    void testDecodeSGR48Code5_Grayscale()
    {
        TBufferStyle bufStyle;

        // code 5, values 232 to 255 (black+23 grayscale)
        bufStyle.decodeSGR48(QStringList({"", "5", "232"}), false);
        QCOMPARE(bufStyle.mBgColor, QColor("#000000"));

        bufStyle.decodeSGR48(QStringList({"", "5", "248"}), false);
        QCOMPARE(bufStyle.mBgColor, QColor(177, 177, 177));

        bufStyle.decodeSGR48(QStringList({"", "5", "255"}), false);
        QCOMPARE(bufStyle.mBgColor, QColor(255, 255, 255));
    }

    void testDecodeSGR48Code5_UndefinedGrayscaleValue()
    {
        TBufferStyle bufStyle;

        // code 5, values 232 to 255 (black+23 grayscale)
        bufStyle.decodeSGR48(QStringList({"", "5", "258"}), false);
        QCOMPARE(bufStyle.mBgColor, QColor(64, 64, 64));

        bufStyle.decodeSGR48(QStringList({"", "5", "270"}), false);
        QCOMPARE(bufStyle.mBgColor, QColor(64, 64, 64));
    }

    void testDecodeSGR48Code2_6Params()
    {
        TBufferStyle bufStyle;

        // code 2, 6 parameters
        bufStyle.decodeSGR48(QStringList({"", "2", "", "0", "0", "0"}), false);
        QCOMPARE(bufStyle.mBgColor, QColor(0, 0, 0));

        // code 2, 6 parameters
        bufStyle.decodeSGR48(QStringList({"", "2", "", "255", "0", "0"}), false);
        QCOMPARE(bufStyle.mBgColor, QColor(255, 0, 0));

        // code 2, 5 parameters
        bufStyle.decodeSGR48(QStringList({"", "2", "", "255", "100"}), false);
        QCOMPARE(bufStyle.mBgColor, QColor(255, 100, 0));

        // code 2, 4 parameters
        bufStyle.decodeSGR48(QStringList({"", "2", "", "100"}), false);
        QCOMPARE(bufStyle.mBgColor, QColor(100, 0, 0));

        // code 2, 3 parameters
        bufStyle.decodeSGR48(QStringList({"", "2", ""}), false);
        QCOMPARE(bufStyle.mBgColor, Qt::black);
    }

    void testDecodeSGRColonSeparated()
    {
        TColorSettings hostSettings;
        TBufferStyle bufStyle;

        bufStyle.decodeSGR("38:5:1", false, hostSettings);
        QCOMPARE(bufStyle.mFgColor, bufStyle.mRed);
        QCOMPARE(bufStyle.mFgColorLight, bufStyle.mLightRed);
        QCOMPARE(bufStyle.mBold, false);
    }

    void testDecodeSGRSemiColonSeparated()
    {
        TColorSettings hostSettings;
        TBufferStyle bufStyle;

        bufStyle.decodeSGR("38;5;1", false, hostSettings);
        QCOMPARE(bufStyle.mFgColor, bufStyle.mRed);
        QCOMPARE(bufStyle.mFgColorLight, bufStyle.mLightRed);
        QCOMPARE(bufStyle.mBold, false);

        bufStyle.decodeSGR("48;5;1", false, hostSettings);
        QCOMPARE(bufStyle.mBgColor, bufStyle.mRed);
    }

    void testDecodeSGRUnderline()
    {
        TColorSettings hostSettings;
        TBufferStyle bufStyle;

        bufStyle.mUnderline = false;
        bufStyle.decodeSGR("4:1", false, hostSettings);
        QCOMPARE(bufStyle.mUnderline, true);

        bufStyle.mUnderline = true;
        bufStyle.decodeSGR("4:0", false, hostSettings);
        QCOMPARE(bufStyle.mUnderline, false);

        bufStyle.mUnderline = false;
        bufStyle.decodeSGR("4:3", false, hostSettings);
        QCOMPARE(bufStyle.mUnderline, true);

        bufStyle.mUnderline = true;
        bufStyle.decodeSGR("4:4", false, hostSettings);
        QCOMPARE(bufStyle.mUnderline, false);
    }

    void testDecodeSGRItalics()
    {
        TColorSettings hostSettings;
        TBufferStyle bufStyle;

        bufStyle.mItalics = false;
        bufStyle.decodeSGR("3:1", false, hostSettings);
        QCOMPARE(bufStyle.mItalics, true);

        bufStyle.mItalics = true;
        bufStyle.decodeSGR("3:0", false, hostSettings);
        QCOMPARE(bufStyle.mItalics, false);

//        bufStyle.mItalics = false;
//        bufStyle.decodeSGR("3:2", false, hostSettings);
//        QCOMPARE(bufStyle.mItalics, true);
//
//        bufStyle.mItalics = true;
//        bufStyle.decodeSGR("3:3", false, hostSettings);
//        QCOMPARE(bufStyle.mItalics, false);
    }

    void testDecodeSGRSingleCode()
    {
        TColorSettings hostSettings;
        TBufferStyle bufStyle;

        bufStyle.mBold = false;
        bufStyle.decodeSGR("1", false, hostSettings);
        QCOMPARE(bufStyle.mBold, true);
        bufStyle.decodeSGR("2", false, hostSettings);
        QCOMPARE(bufStyle.mBold, false);

        bufStyle.mItalics = false;
        bufStyle.decodeSGR("3", false, hostSettings);
        QCOMPARE(bufStyle.mItalics, true);
        bufStyle.decodeSGR("23", false, hostSettings);
        QCOMPARE(bufStyle.mItalics, false);

        bufStyle.mUnderline = false;
        bufStyle.decodeSGR("4", false, hostSettings);
        QCOMPARE(bufStyle.mUnderline, true);
        bufStyle.decodeSGR("24", false, hostSettings);
        QCOMPARE(bufStyle.mUnderline, false);
    }

    void testDecodeSGRColors()
    {
        TColorSettings hostSettings;
        TBufferStyle bufStyle;

        bufStyle.decodeSGR("30", false, hostSettings);
        QCOMPARE(bufStyle.mFgColor, bufStyle.mBlack);
        QCOMPARE(bufStyle.mFgColorLight, bufStyle.mLightBlack);

        bufStyle.decodeSGR("31", false, hostSettings);
        QCOMPARE(bufStyle.mFgColor, bufStyle.mRed);
        QCOMPARE(bufStyle.mFgColorLight, bufStyle.mLightRed);

        bufStyle.decodeSGR("37", false, hostSettings);
        QCOMPARE(bufStyle.mFgColor, bufStyle.mWhite);
        QCOMPARE(bufStyle.mFgColorLight, bufStyle.mLightWhite);

        hostSettings.mFgColor = Qt::darkCyan;
        bufStyle.decodeSGR("39", false, hostSettings);
        QCOMPARE(bufStyle.mFgColor, Qt::darkCyan);

        bufStyle.mBgColor = Qt::green;
        bufStyle.decodeSGR("40", false, hostSettings);
        QCOMPARE(bufStyle.mBgColor, bufStyle.mBlack);

        bufStyle.decodeSGR("41", false, hostSettings);
        QCOMPARE(bufStyle.mBgColor, bufStyle.mRed);

        bufStyle.decodeSGR("47", false, hostSettings);
        QCOMPARE(bufStyle.mBgColor, bufStyle.mWhite);

        bufStyle.decodeSGR("90", false, hostSettings);
        QCOMPARE(bufStyle.mFgColor, bufStyle.mLightBlack);
        QCOMPARE(bufStyle.mFgColorLight, bufStyle.mLightBlack);

        bufStyle.decodeSGR("93", false, hostSettings);
        QCOMPARE(bufStyle.mFgColor, bufStyle.mLightYellow);
        QCOMPARE(bufStyle.mFgColorLight, bufStyle.mLightYellow);

        bufStyle.decodeSGR("97", false, hostSettings);
        QCOMPARE(bufStyle.mFgColor, bufStyle.mLightWhite);
        QCOMPARE(bufStyle.mFgColorLight, bufStyle.mLightWhite);

        bufStyle.decodeSGR("100", false, hostSettings);
        QCOMPARE(bufStyle.mBgColor, bufStyle.mLightBlack);

        bufStyle.decodeSGR("107", false, hostSettings);
        QCOMPARE(bufStyle.mBgColor, bufStyle.mLightWhite);

        // UNDEFINED Value (shouldn't change)
        bufStyle.mFgColor = Qt::darkBlue;
        bufStyle.mBgColor = Qt::darkBlue;
        bufStyle.decodeSGR("99", false, hostSettings);
        QCOMPARE(bufStyle.mBgColor, Qt::darkBlue);
        QCOMPARE(bufStyle.mFgColor, Qt::darkBlue);

    }

    void cleanupTestCase()
    {
    }
};

#include "TBufferStyleTest.moc"
QTEST_MAIN(TBufferStyleTest)
