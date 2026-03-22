#include "mudlet.h"
#include <QtTest/QtTest>

class BlinkAnimationTest : public QObject {
  Q_OBJECT

private slots:
  void testOpacityFloor_data() {
    QTest::addColumn<qreal>("normalizedX");
    QTest::addColumn<qreal>("blinkTimeMs");
    QTest::addColumn<bool>("isFastBlink");

    // Sweep the full phase range at edges and midpoints
    for (int t = 0; t <= 2000; t += 100) {
      for (int x10 = -2; x10 <= 12; x10++) {
        qreal nx = x10 / 10.0;
        QTest::addRow("nx=%.1f t=%d slow", nx, t) << nx << qreal(t) << false;
        QTest::addRow("nx=%.1f t=%d fast", nx, t) << nx << qreal(t) << true;
      }
    }
  }

  void testOpacityFloor() {
    QFETCH(qreal, normalizedX);
    QFETCH(qreal, blinkTimeMs);
    QFETCH(bool, isFastBlink);

    const qreal opacity =
        mudlet::computeBlinkOpacity(normalizedX, blinkTimeMs, isFastBlink);
    QVERIFY2(opacity >= 0.4,
             qPrintable(QString("Opacity %1 below 0.4 floor at nx=%2 t=%3")
                            .arg(opacity)
                            .arg(normalizedX)
                            .arg(blinkTimeMs)));
    QVERIFY2(opacity <= 1.0,
             qPrintable(QString("Opacity %1 above 1.0 at nx=%2 t=%3")
                            .arg(opacity)
                            .arg(normalizedX)
                            .arg(blinkTimeMs)));
  }

  void testPeakAtPhaseCenter() {
    // At the moment the Gaussian peak is centered on normalizedX=0.5,
    // the opacity should be near 1.0 (the maximum).
    // Slow blink peak at center: phase = 0.5 => t/period = (0.5 + 0.4) / 1.8 =>
    // t = 2000 * 0.9/1.8 = 1000ms
    const qreal peakOpacity = mudlet::computeBlinkOpacity(0.5, 1000.0, false);
    QVERIFY2(
        peakOpacity > 0.99,
        qPrintable(QString("Expected peak near 1.0, got %1").arg(peakOpacity)));
  }

  void testWrapContinuity() {
    // Opacity at t=0 must equal opacity at t=2000 (seamless wrap for slow
    // blink)
    const qreal atZero = mudlet::computeBlinkOpacity(0.5, 0.0, false);
    const qreal atWrap = mudlet::computeBlinkOpacity(0.5, 2000.0, false);
    QVERIFY2(
        qFuzzyCompare(atZero, atWrap),
        qPrintable(QString("Wrap discontinuity: t=0 gives %1, t=2000 gives %2")
                       .arg(atZero)
                       .arg(atWrap)));
  }

  void testFastBlinkTwoSweepsPerCycle() {
    // Fast blink (1000ms period) must phase-reset at 1000ms, matching t=0.
    // This verifies it completes two full sweeps in the 2000ms accumulator
    // cycle.
    const qreal atZero = mudlet::computeBlinkOpacity(0.5, 0.0, true);
    const qreal atHalfCycle = mudlet::computeBlinkOpacity(0.5, 1000.0, true);
    QVERIFY2(
        qFuzzyCompare(atZero, atHalfCycle),
        qPrintable(
            QString("Fast blink phase mismatch: t=0 gives %1, t=1000 gives %2")
                .arg(atZero)
                .arg(atHalfCycle)));
  }

  void testFastBlinkTwiceAsFrequentAsSlow() {
    // At the same normalizedX, fast blink at t=500ms should match slow blink at
    // t=1000ms (fast covers the same phase in half the time).
    const qreal fast500 = mudlet::computeBlinkOpacity(0.5, 500.0, true);
    const qreal slow1000 = mudlet::computeBlinkOpacity(0.5, 1000.0, false);
    QVERIFY2(
        qFuzzyCompare(fast500, slow1000),
        qPrintable(
            QString("Fast/slow period ratio broken: fast@500=%1 slow@1000=%2")
                .arg(fast500)
                .arg(slow1000)));
  }
};

QTEST_MAIN(BlinkAnimationTest)
#include "BlinkAnimationTest.moc"
