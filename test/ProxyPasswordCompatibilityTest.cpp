/***************************************************************************
 *   Copyright (C) 2025 by Mike Conley - mike.conley@stickmud.com          *
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

#include <SecureStringUtils.h>
#include <QtTest/QtTest>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QBuffer>

class ProxyPasswordCompatibilityTest : public QObject {
Q_OBJECT

private:

private slots:

    void initTestCase()
    {
    }

    void testLegacyPlaintextPasswordImport()
    {
        // Simulate importing a legacy XML file with plaintext proxy password
        QString xmlContent = R"(
            <Host mProxyAddress="proxy.example.com" 
                  mProxyPort="8080" 
                  mProxyUsername="user" 
                  mProxyPassword="plaintext_password" 
                  mUseProxy="yes">
            </Host>
        )";

        QByteArray xmlData = xmlContent.toUtf8();
        QBuffer buffer(&xmlData);
        buffer.open(QIODevice::ReadOnly);
        
        QXmlStreamReader reader(&buffer);
        
        // Find the Host element
        while (!reader.atEnd() && !reader.isStartElement()) {
            reader.readNext();
        }
        
        QVERIFY(reader.isStartElement());
        QCOMPARE(reader.name().toString(), "Host");
        
        // Test backward compatibility detection
        QString storedPassword = reader.attributes().value("mProxyPassword").toString();
        QCOMPARE(storedPassword, "plaintext_password");
        
        // Should NOT be detected as encrypted format
        QVERIFY(!SecureStringUtils::isEncryptedFormat(storedPassword));
        
        // Import logic should use plaintext as-is
        QString importedPassword;
        if (SecureStringUtils::isEncryptedFormat(storedPassword)) {
            importedPassword = SecureStringUtils::decryptString(storedPassword);
        } else {
            importedPassword = storedPassword; // Use plaintext as-is
        }
        
        QCOMPARE(importedPassword, "plaintext_password");
    }

    void testEncryptedPasswordImport()
    {
        // Test importing an XML file with encrypted proxy password
        QString originalPassword = "my_secure_password";
        QString encryptedPassword = SecureStringUtils::encryptString(originalPassword);
        
        QString xmlTemplate = R"(
            <Host mProxyAddress="proxy.example.com" 
                  mProxyPort="8080" 
                  mProxyUsername="user" 
                  mProxyPassword="%1" 
                  mUseProxy="yes">
            </Host>
        )";
        
        QString xmlContent = xmlTemplate.arg(encryptedPassword);
        QByteArray xmlData = xmlContent.toUtf8();
        QBuffer buffer(&xmlData);
        buffer.open(QIODevice::ReadOnly);
        
        QXmlStreamReader reader(&buffer);
        
        // Find the Host element
        while (!reader.atEnd() && !reader.isStartElement()) {
            reader.readNext();
        }
        
        QVERIFY(reader.isStartElement());
        QCOMPARE(reader.name().toString(), "Host");
        
        // Test encrypted password detection and decryption
        QString storedPassword = reader.attributes().value("mProxyPassword").toString();
        QCOMPARE(storedPassword, encryptedPassword);
        
        // Should be detected as encrypted format
        QVERIFY(SecureStringUtils::isEncryptedFormat(storedPassword));
        
        // Import logic should decrypt it
        QString importedPassword;
        if (SecureStringUtils::isEncryptedFormat(storedPassword)) {
            importedPassword = SecureStringUtils::decryptString(storedPassword);
        } else {
            importedPassword = storedPassword;
        }
        
        QCOMPARE(importedPassword, originalPassword);
    }

    void testPasswordExportEncryption()
    {
        // Test that export always encrypts passwords
        QString originalPassword = "export_test_password";
        
        // Test safe encryption (should encrypt plaintext)
        QString exportedPassword = SecureStringUtils::safeEncryptString(originalPassword);
        QVERIFY(SecureStringUtils::isEncryptedFormat(exportedPassword));
        
        // Verify it can be decrypted back to original
        QString decryptedPassword = SecureStringUtils::decryptString(exportedPassword);
        QCOMPARE(decryptedPassword, originalPassword);
    }

    void testExportAlreadyEncryptedPassword()
    {
        // Test that export handles already-encrypted passwords correctly
        QString originalPassword = "already_encrypted_test";
        QString alreadyEncrypted = SecureStringUtils::encryptString(originalPassword);
        
        // Safe encryption should return the same encrypted value
        QString exportedPassword = SecureStringUtils::safeEncryptString(alreadyEncrypted);
        QCOMPARE(exportedPassword, alreadyEncrypted);
        
        // Should still decrypt to original
        QString decryptedPassword = SecureStringUtils::decryptString(exportedPassword);
        QCOMPARE(decryptedPassword, originalPassword);
    }

    void testRoundTripCompatibility()
    {
        // Test full round trip: plaintext -> export (encrypt) -> import (detect & decrypt)
        QString originalPassword = "roundtrip_password";
        
        // Step 1: Export (encrypt)
        QString exportedPassword = SecureStringUtils::safeEncryptString(originalPassword);
        QVERIFY(SecureStringUtils::isEncryptedFormat(exportedPassword));
        
        // Step 2: Import (detect and decrypt)
        QString importedPassword;
        if (SecureStringUtils::isEncryptedFormat(exportedPassword)) {
            importedPassword = SecureStringUtils::decryptString(exportedPassword);
        } else {
            importedPassword = exportedPassword;
        }
        
        // Should match original
        QCOMPARE(importedPassword, originalPassword);
    }

    void testEmptyPasswordHandling()
    {
        // Test handling of empty passwords
        QVERIFY(!SecureStringUtils::isEncryptedFormat(""));
        QCOMPARE(SecureStringUtils::safeEncryptString(""), QString());
        QCOMPARE(SecureStringUtils::decryptString(""), QString());
    }

    void cleanupTestCase()
    {
    }
};

#include "ProxyPasswordCompatibilityTest.moc"
QTEST_MAIN(ProxyPasswordCompatibilityTest)
