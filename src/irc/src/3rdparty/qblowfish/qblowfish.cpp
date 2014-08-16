/*
  This file is part of QBlowfish and is licensed under the MIT License

  Copyright (C) 2012 Roopesh Chander <roop@forwardbias.in>

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject
  to the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include "qblowfish.h"
#include "qblowfish_p.h"
#include <QtEndian>
#include <QDebug>

QBlowfish::QBlowfish(const QByteArray &key)
    : m_key(key)
    , m_initialized(false)
    , m_paddingEnabled(false)
{
}

void QBlowfish::setPaddingEnabled(bool enabled)
{
    m_paddingEnabled = enabled;
}

bool QBlowfish::isPaddingEnabled() const
{
    return m_paddingEnabled;
}

QByteArray QBlowfish::encrypted(const QByteArray &_clearText)
{
    QByteArray clearText(_clearText);
    if (clearText.isEmpty()) {
        return QByteArray();
    }

    if (isPaddingEnabled()) {
        // Add padding as per PKCS5
        // Ref: RFC 5652 http://tools.ietf.org/html/rfc5652#section-6.3
        quint8 paddingLength = 8 - (clearText.size() % 8);
        QByteArray paddingBa(paddingLength, static_cast<char>(paddingLength));
        clearText.append(paddingBa);
    } else {
        if (clearText.size() % 8 != 0) {
            qWarning("Cannot encrypt. Clear-text length is not a multiple of 8 and padding is not enabled.");
            return QByteArray();
        }
    }

    Q_ASSERT(clearText.size() % 8 == 0);
    if ((clearText.size() % 8 == 0) && init()) {

        QByteArray copyBa(clearText.constData(), clearText.size());
        for (int i = 0; i < clearText.size(); i += 8) {
            coreEncrypt(copyBa.data() + i);
        }
        return copyBa;

    }
    return QByteArray();
}

QByteArray QBlowfish::decrypted(const QByteArray &cipherText)
{
    if (cipherText.isEmpty()) {
        return QByteArray();
    }

    Q_ASSERT(cipherText.size() % 8 == 0);
    if ((cipherText.size() % 8 == 0) && init()) {

        QByteArray copyBa(cipherText.constData(), cipherText.size());
        for (int i = 0; i < cipherText.size(); i += 8) {
            coreDecrypt(copyBa.data() + i);
        }

        if (isPaddingEnabled()) {
            // Remove padding as per PKCS5
            quint8 paddingLength = static_cast<quint8>(copyBa.right(1).at(0));
            QByteArray paddingBa(paddingLength, static_cast<char>(paddingLength));
            if (copyBa.right(paddingLength) == paddingBa) {
                return copyBa.left(copyBa.length() - paddingLength);
            }
            return QByteArray();
        }
        return copyBa;
    }
    return QByteArray();
}

/*
  Core encryption code follows. This is an implementation of the Blowfish algorithm as described at:
  http://www.schneier.com/paper-blowfish-fse.html
*/

bool QBlowfish::init()
{
    if (m_initialized) {
        return true;
    }

    if (m_key.isEmpty()) {
        qWarning("Cannot init. Key is empty.");
        return false;
    }

    m_sbox1 = QByteArray::fromHex(QByteArray::fromRawData(sbox0, SBOX_SIZE_BYTES * 2));
    m_sbox2 = QByteArray::fromHex(QByteArray::fromRawData(sbox1, SBOX_SIZE_BYTES * 2));
    m_sbox3 = QByteArray::fromHex(QByteArray::fromRawData(sbox2, SBOX_SIZE_BYTES * 2));
    m_sbox4 = QByteArray::fromHex(QByteArray::fromRawData(sbox3, SBOX_SIZE_BYTES * 2));
    m_parray = QByteArray::fromHex(QByteArray::fromRawData(parray, PARRAY_SIZE_BYTES * 2));

    const QByteArray &key = m_key;
    int keyLength = key.length();
    for (int i = 0; i < PARRAY_SIZE_BYTES; i++) {
        m_parray[i] = static_cast<char>(static_cast<quint8>(m_parray[i]) ^ static_cast<quint8>(key[i % keyLength]));
    }

    char seed[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

    // Update p-array
    for (int i = 0; i < (PARRAY_SIZE_BYTES / 4); i += 2) {
        coreEncrypt(seed);
        for (int j = 0; j < 8; j++) {
            // P1 = xL; P2 = xR
            m_parray[i * 4 + j] = seed[j];
        }
    }

    // Update s-boxes
    for (int sboxIndex = 1; sboxIndex <= 4; sboxIndex++) {
        QByteArray *sbox = 0;
        switch (sboxIndex) {
        case 1: sbox = &m_sbox1; break;
        case 2: sbox = &m_sbox2; break;
        case 3: sbox = &m_sbox3; break;
        case 4: sbox = &m_sbox4; break;
        default: Q_ASSERT(false);
        }
        Q_ASSERT(sbox != 0);

        for (int i = 0; i < (SBOX_SIZE_BYTES / 4); i += 2) {
            coreEncrypt(seed);
            for (int j = 0; j < 8; j++) {
                // S1,1 = xL; S1,2 = xR
                sbox->operator[](i * 4 + j) = seed[j];
            }
        }
    }

    m_initialized = true;
    return true;
}

void QBlowfish::coreEncrypt(char *x) // encrypts 8 bytes pointed to by x, result is written to the same location
{
    // Divide x into two 32-bit halves: xL, xR
    char *xL = x;
    char *xR = x + 4;
    uchar f_xL_bytes[4] = { 0, 0, 0, 0 };

    for (int i = 0; i < 16; i++) {

        // xL = xL XOR Pi
        for (int j = 0; j < 4; j++) {
            // quint8 old_xL = xL[j];
            xL[j] = static_cast<char>(static_cast<quint8>(xL[j]) ^ static_cast<quint8>(m_parray[i * 4 + j]));
        }

        // Divide xL into four eight-bit quarters: a, b, c, and d
        quint8 a = static_cast<quint8>(xL[0]);
        quint8 b = static_cast<quint8>(xL[1]);
        quint8 c = static_cast<quint8>(xL[2]);
        quint8 d = static_cast<quint8>(xL[3]);

        // F(xL) = ((S1,a + S2,b mod 2**32) XOR S3,c) + S4,d mod 2**32
        quint32 s1a = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(m_sbox1.constData() + a * 4));
        quint32 s2b = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(m_sbox2.constData() + b * 4));
        quint32 s3c = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(m_sbox3.constData() + c * 4));
        quint32 s4d = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(m_sbox4.constData() + d * 4));
        quint32 f_xL = ((((s1a + s2b) & 0xffffffff) ^ s3c) + s4d) & 0xffffffff;
        qToBigEndian<quint32>(f_xL, f_xL_bytes);

        // xR = F(xL) XOR xR
        for (int j = 0; j < 4; j++) {
            xR[j] = static_cast<char>(static_cast<quint8>(f_xL_bytes[j]) ^ static_cast<quint8>(xR[j]));
        }

        // Swap xL and xR, but not in the last iteration
        if (i != 15) {
            for (int j = 0; j < 4; j++) {
                char temp = xL[j];
                xL[j] = xR[j];
                xR[j] = temp;
            }
        }

    }

    // xR = xR XOR P17
    // xL = xL XOR P18
    for (int j = 0; j < 4; j++) {
        xR[j] = static_cast<char>(static_cast<quint8>(xR[j]) ^ static_cast<quint8>(m_parray[16 * 4 + j]));
        xL[j] = static_cast<char>(static_cast<quint8>(xL[j]) ^ static_cast<quint8>(m_parray[17 * 4 + j]));
    }
}

void QBlowfish::coreDecrypt(char *x) // decrypts 8 bytes pointed to by x, result is written to the same location
{
    // Divide x into two 32-bit halves: xL, xR
    char *xL = x;
    char *xR = x + 4;
    uchar f_xL_bytes[4] = { 0, 0, 0, 0 };

    // xL = xL XOR P18
    // xR = xR XOR P17
    for (int j = 0; j < 4; j++) {
        xL[j] = static_cast<char>(static_cast<quint8>(xL[j]) ^ static_cast<quint8>(m_parray[17 * 4 + j]));
        xR[j] = static_cast<char>(static_cast<quint8>(xR[j]) ^ static_cast<quint8>(m_parray[16 * 4 + j]));
    }

    for (int i = 15; i >= 0; i--) {

        // Swap xL and xR, but not in the first iteration
        if (i != 15) {
            for (int j = 0; j < 4; j++) {
                char temp = xL[j];
                xL[j] = xR[j];
                xR[j] = temp;
            }
        }

        // Divide xL into four eight-bit quarters: a, b, c, and d
        quint8 a = static_cast<quint8>(xL[0]);
        quint8 b = static_cast<quint8>(xL[1]);
        quint8 c = static_cast<quint8>(xL[2]);
        quint8 d = static_cast<quint8>(xL[3]);

        // F(xL) = ((S1,a + S2,b mod 2**32) XOR S3,c) + S4,d mod 2**32
        quint32 s1a = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(m_sbox1.constData() + a * 4));
        quint32 s2b = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(m_sbox2.constData() + b * 4));
        quint32 s3c = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(m_sbox3.constData() + c * 4));
        quint32 s4d = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(m_sbox4.constData() + d * 4));
        quint32 f_xL = ((((s1a + s2b) & 0xffffffff) ^ s3c) + s4d) & 0xffffffff;
        qToBigEndian<quint32>(f_xL, f_xL_bytes);

        // xR = F(xL) XOR xR
        for (int j = 0; j < 4; j++) {
            xR[j] = static_cast<char>(static_cast<quint8>(f_xL_bytes[j]) ^ static_cast<quint8>(xR[j]));
        }

        // xL = xL XOR Pi
        for (int j = 0; j < 4; j++) {
            xL[j] = static_cast<char>(static_cast<quint8>(xL[j]) ^ static_cast<quint8>(m_parray[i * 4 + j]));
        }

    }
}
