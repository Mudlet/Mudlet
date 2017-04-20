######################################################################
# Communi
######################################################################

INCLUDEPATH += $$PWD/src
DEPENDPATH += $$PWD/src

HEADERS += $$PWD/src/Big5Freq.tab
HEADERS += $$PWD/src/EUCKRFreq.tab
HEADERS += $$PWD/src/EUCTWFreq.tab
HEADERS += $$PWD/src/GB2312Freq.tab
HEADERS += $$PWD/src/JISFreq.tab

HEADERS += $$PWD/src/CharDistribution.h
HEADERS += $$PWD/src/JpCntx.h
HEADERS += $$PWD/src/nsBig5Prober.h
HEADERS += $$PWD/src/nsCharSetProber.h
HEADERS += $$PWD/src/nsCodingStateMachine.h
HEADERS += $$PWD/src/nscore.h
HEADERS += $$PWD/src/nsEscCharsetProber.h
HEADERS += $$PWD/src/nsEUCJPProber.h
HEADERS += $$PWD/src/nsEUCKRProber.h
HEADERS += $$PWD/src/nsEUCTWProber.h
HEADERS += $$PWD/src/nsGB2312Prober.h
HEADERS += $$PWD/src/nsHebrewProber.h
HEADERS += $$PWD/src/nsLatin1Prober.h
HEADERS += $$PWD/src/nsMBCSGroupProber.h
HEADERS += $$PWD/src/nsPkgInt.h
HEADERS += $$PWD/src/nsSBCharSetProber.h
HEADERS += $$PWD/src/nsSBCSGroupProber.h
HEADERS += $$PWD/src/nsSJISProber.h
HEADERS += $$PWD/src/nsUniversalDetector.h
HEADERS += $$PWD/src/nsUTF8Prober.h
HEADERS += $$PWD/src/prmem.h
HEADERS += $$PWD/src/uchardet.h

SOURCES += $$PWD/src/CharDistribution.cpp
SOURCES += $$PWD/src/JpCntx.cpp
SOURCES += $$PWD/src/LangBulgarianModel.cpp
SOURCES += $$PWD/src/LangCyrillicModel.cpp
SOURCES += $$PWD/src/LangGreekModel.cpp
SOURCES += $$PWD/src/LangHungarianModel.cpp
SOURCES += $$PWD/src/LangHebrewModel.cpp
SOURCES += $$PWD/src/LangThaiModel.cpp
SOURCES += $$PWD/src/nsHebrewProber.cpp
SOURCES += $$PWD/src/nsCharSetProber.cpp
SOURCES += $$PWD/src/nsBig5Prober.cpp
SOURCES += $$PWD/src/nsEUCJPProber.cpp
SOURCES += $$PWD/src/nsEUCKRProber.cpp
SOURCES += $$PWD/src/nsEUCTWProber.cpp
SOURCES += $$PWD/src/nsEscCharsetProber.cpp
SOURCES += $$PWD/src/nsEscSM.cpp
SOURCES += $$PWD/src/nsGB2312Prober.cpp
SOURCES += $$PWD/src/nsMBCSGroupProber.cpp
SOURCES += $$PWD/src/nsMBCSSM.cpp
SOURCES += $$PWD/src/nsSBCSGroupProber.cpp
SOURCES += $$PWD/src/nsSBCharSetProber.cpp
SOURCES += $$PWD/src/nsSJISProber.cpp
SOURCES += $$PWD/src/nsUTF8Prober.cpp
SOURCES += $$PWD/src/nsLatin1Prober.cpp
SOURCES += $$PWD/src/nsUniversalDetector.cpp
SOURCES += $$PWD/src/uchardet.cpp
