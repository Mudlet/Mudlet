/***************************************************************************
 *   Copyright (C) 2026 by Mike Conley - mike.conley@stickmud.com          *
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

#include <OAuthClientFlow.h>
#include <QtTest/QtTest>
#include <QRegularExpression>
#include <QUrlQuery>

class OAuthClientFlowTest : public QObject
{
    Q_OBJECT

private slots:
    void testCodeVerifierFormat();
    void testCodeVerifierUnique();
    void testCodeChallengeRfc7636Vector();
    void testBuildAuthorizationUrl();
    void testBuildAuthorizationUrlOmitsEmptyNonce();
};

void OAuthClientFlowTest::testCodeVerifierFormat()
{
    const QString verifier = OAuthClientFlow::generateCodeVerifier();
    // RFC 7636 requires 43-128 characters from the unreserved set; 32 random bytes
    // base64url-encoded without padding is exactly 43.
    QCOMPARE(verifier.length(), 43);
    const QRegularExpression unreserved(QStringLiteral("^[A-Za-z0-9\\-._~]+$"));
    QVERIFY(unreserved.match(verifier).hasMatch());
}

void OAuthClientFlowTest::testCodeVerifierUnique()
{
    QVERIFY(OAuthClientFlow::generateCodeVerifier() != OAuthClientFlow::generateCodeVerifier());
}

void OAuthClientFlowTest::testCodeChallengeRfc7636Vector()
{
    // Test vector from RFC 7636 Appendix B.
    const QString verifier = QStringLiteral("dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk");
    QCOMPARE(OAuthClientFlow::codeChallengeS256(verifier), QStringLiteral("E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM"));
}

void OAuthClientFlowTest::testBuildAuthorizationUrl()
{
    const QUrl url = OAuthClientFlow::buildAuthorizationUrl(QUrl(QStringLiteral("https://example.com/authorize")),
                                                            QStringLiteral("mud-native-client"),
                                                            {QStringLiteral("openid"), QStringLiteral("profile")},
                                                            QStringLiteral("http://127.0.0.1:49152/"),
                                                            QStringLiteral("test-state"),
                                                            QStringLiteral("test-challenge"),
                                                            QStringLiteral("test-nonce"));
    QVERIFY(url.isValid());
    QCOMPARE(url.scheme(), QStringLiteral("https"));
    QCOMPARE(url.host(), QStringLiteral("example.com"));
    QCOMPARE(url.path(), QStringLiteral("/authorize"));

    const QUrlQuery query(url);
    QCOMPARE(query.queryItemValue(QStringLiteral("response_type")), QStringLiteral("code"));
    QCOMPARE(query.queryItemValue(QStringLiteral("client_id")), QStringLiteral("mud-native-client"));
    QCOMPARE(query.queryItemValue(QStringLiteral("redirect_uri"), QUrl::FullyDecoded), QStringLiteral("http://127.0.0.1:49152/"));
    QCOMPARE(query.queryItemValue(QStringLiteral("scope"), QUrl::FullyDecoded), QStringLiteral("openid profile"));
    QCOMPARE(query.queryItemValue(QStringLiteral("state")), QStringLiteral("test-state"));
    QCOMPARE(query.queryItemValue(QStringLiteral("code_challenge")), QStringLiteral("test-challenge"));
    QCOMPARE(query.queryItemValue(QStringLiteral("code_challenge_method")), QStringLiteral("S256"));
    QCOMPARE(query.queryItemValue(QStringLiteral("nonce")), QStringLiteral("test-nonce"));
}

void OAuthClientFlowTest::testBuildAuthorizationUrlOmitsEmptyNonce()
{
    const QUrl url = OAuthClientFlow::buildAuthorizationUrl(QUrl(QStringLiteral("https://example.com/authorize")),
                                                            QStringLiteral("mud-native-client"),
                                                            {QStringLiteral("openid")},
                                                            QStringLiteral("http://127.0.0.1:49152/"),
                                                            QStringLiteral("test-state"),
                                                            QStringLiteral("test-challenge"),
                                                            QString());
    const QUrlQuery query(url);
    QVERIFY(!query.hasQueryItem(QStringLiteral("nonce")));
}

#include "OAuthClientFlowTest.moc"
QTEST_MAIN(OAuthClientFlowTest)
