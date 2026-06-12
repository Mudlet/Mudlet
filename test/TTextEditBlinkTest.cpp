#include "TTextEdit.h"

#include <QTest>

class TTextEditBlinkTest : public QObject
{
    Q_OBJECT

private slots:
    void keepsRegistrationWhenCachedBlinkingContentMayStillBeVisible() { QVERIFY(TTextEdit::shouldRegisterBlinkClient(true, false, true, true)); }

    void unregistersWhenBlinkingIsDisabled() { QVERIFY(!TTextEdit::shouldRegisterBlinkClient(false, true, true, true)); }

    void unregistersWhenBlinkingIsDisabledRegardlessOfOtherFlags() { QVERIFY(!TTextEdit::shouldRegisterBlinkClient(false, false, false, false)); }

    void registersWhenCurrentPaintFindsBlinkingContent() { QVERIFY(TTextEdit::shouldRegisterBlinkClient(true, true, false, false)); }

    void unregistersWhenPaintDidNotReuseCache() { QVERIFY(!TTextEdit::shouldRegisterBlinkClient(true, false, true, false)); }

    void unregistersWhenClientWasNotRegisteredEvenWithCachedContent() { QVERIFY(!TTextEdit::shouldRegisterBlinkClient(true, false, false, true)); }
};

QTEST_GUILESS_MAIN(TTextEditBlinkTest)
#include "TTextEditBlinkTest.moc"
