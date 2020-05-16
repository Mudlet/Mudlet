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
        std::string result = processString(handler, str);

        QCOMPARE(result.c_str(), "Someone says \"Hello\"");
    }

    void testHandleUnfinishedString()
    {
        TEntityHandler handler;

        std::string str = "Someone says &quot;Hello&qu";
        std::string result = processString(handler, str);

        QCOMPARE(result.c_str(), "Someone says \"Hello");
    }

    void testLongSequence()
    {
        TEntityHandler handler;

        std::string str = "Long sequence: &01234567; asdf";
        std::string result = processString(handler, str);

        QCOMPARE(result.c_str(), "Long sequence: ; asdf");
    }

    void testHandleSplitInTwoParts()
    {
        TEntityHandler handler;

        std::string part1 = "Someone says &quot;Hello&qu";
        std::string result1 = processString(handler, part1);
        QCOMPARE(result1.c_str(), "Someone says \"Hello");

        std::string part2 = "ot; to you";
        std::string result2 = processString(handler, part2);
        QCOMPARE(result2.c_str(), "\" to you");

        std::string result = result1 + result2;

        qDebug() << result1.c_str();
        qDebug() << result2.c_str();
        qDebug() << result.c_str();

        QCOMPARE(result.c_str(), "Someone says \"Hello\" to you");
    }

    inline static std::string processString(TEntityHandler& handler, std::string& str)
    {
        size_t len = str.length();
        std::string result;
        for (size_t i = 0; i < len; i++) {
            if (!handler.handle(str[i])) {
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
