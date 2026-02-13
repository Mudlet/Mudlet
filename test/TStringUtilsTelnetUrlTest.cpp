#include "TStringUtils.h"
#include <QtTest/QtTest>

class TStringUtilsTelnetUrlTest : public QObject {
  Q_OBJECT

private slots:
  void testValidDefaultPort() {
    const auto result =
        TStringUtils::parseTelnetUrl(qsl("telnet://example.com"));
    QVERIFY(result.has_value());
    QCOMPARE(result->host, qsl("example.com"));
    QCOMPARE(result->port, static_cast<quint16>(23));
  }

  void testValidPortAndScheme() {
    const auto result =
        TStringUtils::parseTelnetUrl(qsl("TELNET://mud.example:4444"));
    QVERIFY(result.has_value());
    QCOMPARE(result->host, qsl("mud.example"));
    QCOMPARE(result->port, static_cast<quint16>(4444));
  }

  void testValidIpv6() {
    const auto result =
        TStringUtils::parseTelnetUrl(qsl("telnet://[2001:db8::1]:4000"));
    QVERIFY(result.has_value());
    QCOMPARE(result->host, qsl("2001:db8::1"));
    QCOMPARE(result->port, static_cast<quint16>(4000));
  }

  void testRejectsInvalidInputs() {
    QVERIFY(!TStringUtils::parseTelnetUrl(qsl("telnet://")).has_value());
    QVERIFY(!TStringUtils::parseTelnetUrl(qsl("http://example.com:23"))
                 .has_value());
    QVERIFY(!TStringUtils::parseTelnetUrl(qsl("telnet://example.com/path"))
                 .has_value());
    QVERIFY(!TStringUtils::parseTelnetUrl(qsl("telnet://example.com:99999"))
                 .has_value());
    QVERIFY(!TStringUtils::parseTelnetUrl(qsl("example.com:23")).has_value());
  }
};

QTEST_APPLESS_MAIN(TStringUtilsTelnetUrlTest)

#include "TStringUtilsTelnetUrlTest.moc"
