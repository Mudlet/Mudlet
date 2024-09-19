#include <TEntityHandler.h>
#include <QtTest/QtTest>

class TEntityHandlerTest : public QObject {
Q_OBJECT

private:

private slots:

    void initTestCase()
    {
    }

    void testHandleSimpleString()
    {
        TEntityHandler handler;

        std::string str = "Someone says &quot;Hello&quot;";
        QString result = processString(handler, str);

        QCOMPARE(result, "Someone says \"Hello\"");
    }

    void testHandleUnfinishedString()
    {
        TEntityHandler handler;

        std::string str = "Someone says &quot;Hello&qu";
        QString result = processString(handler, str);

        QCOMPARE(result, "Someone says \"Hello");
    }

    void testLongSequence()
    {
        TEntityHandler handler;

        std::string str = "Long sequence: &01234567; asdf";
        QString result = processString(handler, str);

        QCOMPARE(result, "Long sequence: &01234567; asdf");
    }

    void testHandleSplitInTwoParts()
    {
        TEntityHandler handler;

        std::string part1 = "Someone says &quot;Hello&qu";
        QString result1 = processString(handler, part1);
        QCOMPARE(result1, "Someone says \"Hello");

        std::string part2 = "ot; to you";
        QString result2 = processString(handler, part2);
        QCOMPARE(result2, "\" to you");

        QString result = result1 + result2;

        qDebug() << result1;
        qDebug() << result2;
        qDebug() << result;

        QCOMPARE(result, "Someone says \"Hello\" to you");
    }

    inline static QString processString(TEntityHandler& handler, std::string& str)
    {
        size_t len = str.length();
        QString result;
        for (size_t i = 0; i < len; i++) {
            if (!handler.handle(str[i], true)) {
                result += str[i];
            } else if (handler.isEntityResolved()) {
                result += handler.getResultAndReset();
            }
        }
        return result;
    }

    void cleanupTestCase()
    {
    }
};
#include "TEntityHandlerTest.moc"
QTEST_MAIN(TEntityHandlerTest)
