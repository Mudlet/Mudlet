/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#ifndef MUDLET_TEST_LOOPBACKTLSCERTIFICATE_H
#define MUDLET_TEST_LOOPBACKTLSCERTIFICATE_H

// Self-signed loopback certificate and its key, valid until 2126, for the test
// stubs that terminate TLS themselves. A client only accepts it because its test
// turns peer verification off or has the profile ignore certificate errors.
inline constexpr char csmLoopbackCertificatePem[] = R"PEM(-----BEGIN CERTIFICATE-----
MIIDJzCCAg+gAwIBAgIULa4vwGAVOB+r6qtcLMPqwzBlEJgwDQYJKoZIhvcNAQEL
BQAwFDESMBAGA1UEAwwJbG9jYWxob3N0MCAXDTI2MDgwNjE0MTM0OFoYDzIxMjYw
NzEzMTQxMzQ4WjAUMRIwEAYDVQQDDAlsb2NhbGhvc3QwggEiMA0GCSqGSIb3DQEB
AQUAA4IBDwAwggEKAoIBAQDEg1HE09f69FW/OLD0jrWEQRbKkSkIkexLfV5OtzbI
ZVDWcH3Y3NKrbZ60j8WEY8DqVzO2kMnOppc5LBEKGP1TTs6C9R+e5hlI6McoKown
ha4aU9nqM7dsjY71xGZNN9DCVxhRpqadlZon7M4wzVvUO5VIRhFeA2AO6LRVQhyi
9Whe/uJVlncb2tbiGgTavixWSQ5kH0ocE8Cp4SbuHuXPwgiZ9hYEIX2xAFSR48OB
bjWgqVISptu/s+UkK2XckI42qdxqzwglLIIqjFYJ1HvGqhqV69DeqB0XNw6qp8W2
qwTpv3gPGzI60vNL6aaHTivLxnsEClPbcrTfG1y8DnnLAgMBAAGjbzBtMB0GA1Ud
DgQWBBSyDWWzo202vFbYncaD2crvY5V2pDAfBgNVHSMEGDAWgBSyDWWzo202vFbY
ncaD2crvY5V2pDAPBgNVHRMBAf8EBTADAQH/MBoGA1UdEQQTMBGCCWxvY2FsaG9z
dIcEfwAAATANBgkqhkiG9w0BAQsFAAOCAQEAs5nw4GBPPHc9Nc08uLUYTDLkA2XM
WPugjSO7OxUe8NptVh/v4GbeKzQ4FRIF6rca8De15+OOZgIDppRUoy+fd+ncoDan
flw38rIj13XfV/3WF33Uag2xtZG0Hrpu4PFZQyIzr0MwGJJ/v2uRjMiV0CX+rc0L
BJg2JS4oCbNdQpwH81qOktoH8aHirAyLjtm732GQgAGLe0fIBBsb4Dg2ZdvN+TF5
xfKoFfri3H1rwju43zHXmUyCE/RPdIBR8flO6gzdgWAVY0jaixZi1fzQEQuReh2j
d2iZYOFSrVDea41ltrUvRC6q6gxe/REVjj1nCSYU1x44J9DQ6n6ljvJCVw==
-----END CERTIFICATE-----)PEM";

inline constexpr char csmLoopbackPrivateKeyPem[] = R"PEM(-----BEGIN PRIVATE KEY-----
MIIEwAIBADANBgkqhkiG9w0BAQEFAASCBKowggSmAgEAAoIBAQDEg1HE09f69FW/
OLD0jrWEQRbKkSkIkexLfV5OtzbIZVDWcH3Y3NKrbZ60j8WEY8DqVzO2kMnOppc5
LBEKGP1TTs6C9R+e5hlI6McoKownha4aU9nqM7dsjY71xGZNN9DCVxhRpqadlZon
7M4wzVvUO5VIRhFeA2AO6LRVQhyi9Whe/uJVlncb2tbiGgTavixWSQ5kH0ocE8Cp
4SbuHuXPwgiZ9hYEIX2xAFSR48OBbjWgqVISptu/s+UkK2XckI42qdxqzwglLIIq
jFYJ1HvGqhqV69DeqB0XNw6qp8W2qwTpv3gPGzI60vNL6aaHTivLxnsEClPbcrTf
G1y8DnnLAgMBAAECggEBALRPHebwzfrI2CilttAeZXTdWDEzsifX5K17cd3eBBkp
xVuNShuCupZq9bUNOhl4ghlDPALmpRTFDHp78YKHXWFkLN5CVeoxjL+2Po6fQ4w7
/3zOtWNMYp/q32Kn+4ocjaLT0U+SDs0G6LR7dtGWjAyXQylWiTbu9+OWJ2kXSTlH
QbdtamymoJrrjRTV1HUEq/a3qSHlqTA5/EKIcGeiETq2NR0fZ3NFbe+PLiOSpiNg
uIiVEdsItuZTdINSEzOtMFvRd2od0ITDpMtLG404aGsI4Zisiuhr5naf4DWqK2aL
n9Z/55LSuAdBqvrtJ9XVdtNsFdCRjbIj2R1qqDTFm/kCgYEA6W/+ufDXrhPi8XWS
8+7tlOoUd0jYZL9N+N1hfho21SN3eH5TtNO0b/os/PN/M+5dKeWPtnyzwg49EksF
Es9Z4+lLt/Z+71RDmYqSCwaLhXNKtUZluZmrGHcRogd4hJDYv9icgmpwMK5Hg634
PYCgVYb9C1Wug/mZhgLg7Aw3hn8CgYEA14GxAPViaSszVNRps+a9WVEJklPbPR8U
kAxWTP6n1SdT3Z9HRcHH9inIdTLyC/3ti4+4dc1pDkMrq+MUTjvF8BqN35uzJa7l
6dnsXBmWvB1cIcwQb4SLnDb7jzmiK2uIjMrO54x3+atB83GdvESLOQ/9NAJL/+NX
ILq5kAs2nrUCgYEAqq7/8pceLKNPybttMr0drEenpTx3NNsIORItydWDCD8BiPHd
ZJdzFHk5Uc780EzWg97dQNJXYWmlz+1YjVNdZ57ahW1PjNDxCKBgfn1PoMkW9ArA
MIAisSXGl9GcllmOkl/guB75Xy7fDXIz00xsb3zfIt2IV+k2Dt2l9hJMuyMCgYEA
lv45ZHCJeSJZntANF41NkazjxfCXJaYHJD5goSWztfcOHbOhnlB9qA3yc5s0WA6c
RzJ1jaRUPTf2+0HpUj8zGl2gldFjnb2DPWwA3S7YnAj+Knft9BSsNNGZQ+qfo0h+
rhbTDQ0wanABj25FlEl6OornX29UjH9e5oGtziztIhkCgYEAppTHqOgLiKmeV15d
i850uRyh7X6whywY8gm0VLO+xzCVsCR6CvgZY1MwwFuDwu2d/d5jdJXLpHueQwNU
3HipTI77OuIRv4ykXwPOIemT9VmL/N21CgrckJGA6dYywTnc/JNpOKxdTM9srOyr
Rcsgla9jttJevaHI71x2jLNBaKk=
-----END PRIVATE KEY-----)PEM";

#endif // MUDLET_TEST_LOOPBACKTLSCERTIFICATE_H
