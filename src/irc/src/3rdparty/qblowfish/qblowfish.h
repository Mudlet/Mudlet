#ifndef QBLOWFISH_H
#define QBLOWFISH_H

#include <QByteArray>

class QBlowfish
{
public:
    QBlowfish(const QByteArray &key);
    bool init();

    // Padding:
    //
    // Blowfish works on 8-byte blocks. Padding makes it usable even
    // in case where the input size is not in exact 8-byte blocks.
    //
    // If padding is disabled (the default), encrypted() will work only if the
    // input size (in bytes) is a multiple of 8. (If it's not a multiple of 8,
    // encrypted() will return a null bytearray.)
    //
    // If padding is enabled, we increase the input length to a multiple of 8
    // by padding bytes as per PKCS5
    //
    // If padding was enabled during encryption, it should be enabled during
    // decryption for correct decryption (and vice versa).

    void setPaddingEnabled(bool enabled);
    bool isPaddingEnabled() const;

    // Encrypt / decrypt
    QByteArray encrypted(const QByteArray &clearText);
    QByteArray decrypted(const QByteArray &cipherText);

private:
    // core encrypt/decrypt methods, encrypts/decrypts in-place
    void coreEncrypt(char *x);
    void coreDecrypt(char *x);

    QByteArray m_key;
    bool m_initialized;
    bool m_paddingEnabled;
    QByteArray m_parray;
    QByteArray m_sbox1, m_sbox2, m_sbox3, m_sbox4;
};

#endif // QBLOWFISH_H
