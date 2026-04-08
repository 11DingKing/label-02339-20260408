#include <QtTest/QtTest>
#include "widgets/gaugewidget.h"

/**
 * @brief GaugeWidget 单元测试
 * 覆盖：属性设置、值边界、渲染不崩溃
 */
class TestGaugeWidget : public QObject
{
    Q_OBJECT

private slots:
    // ---- 默认值 ----
    void testDefaultValues()
    {
        GaugeWidget gauge;
        QCOMPARE(gauge.value(), 0.0);
        QCOMPARE(gauge.minValue(), 0.0);
        QCOMPARE(gauge.maxValue(), 100.0);
        QVERIFY(gauge.title().isEmpty());
        QVERIFY(gauge.unit().isEmpty());
    }

    // ---- 属性设置 ----
    void testSetProperties()
    {
        GaugeWidget gauge;
        gauge.setTitle("温度");
        gauge.setUnit("℃");
        gauge.setMinValue(-20);
        gauge.setMaxValue(60);
        gauge.setValue(25.5);

        QCOMPARE(gauge.title(), QString("温度"));
        QCOMPARE(gauge.unit(), QString("℃"));
        QCOMPARE(gauge.minValue(), -20.0);
        QCOMPARE(gauge.maxValue(), 60.0);
        QCOMPARE(gauge.value(), 25.5);
    }

    // ---- 值边界裁剪 ----
    void testSetValue_clampToMin()
    {
        GaugeWidget gauge;
        gauge.setMinValue(0);
        gauge.setMaxValue(100);
        gauge.setValue(-50);
        QCOMPARE(gauge.value(), 0.0);
    }

    void testSetValue_clampToMax()
    {
        GaugeWidget gauge;
        gauge.setMinValue(0);
        gauge.setMaxValue(100);
        gauge.setValue(200);
        QCOMPARE(gauge.value(), 100.0);
    }

    void testSetValue_withinRange()
    {
        GaugeWidget gauge;
        gauge.setMinValue(0);
        gauge.setMaxValue(100);
        gauge.setValue(50);
        QCOMPARE(gauge.value(), 50.0);
    }

    void testSetValue_atBoundary()
    {
        GaugeWidget gauge;
        gauge.setMinValue(0);
        gauge.setMaxValue(100);

        gauge.setValue(0);
        QCOMPARE(gauge.value(), 0.0);

        gauge.setValue(100);
        QCOMPARE(gauge.value(), 100.0);
    }

    // ---- 负数范围 ----
    void testNegativeRange()
    {
        GaugeWidget gauge;
        gauge.setMinValue(-50);
        gauge.setMaxValue(50);
        gauge.setValue(-25);
        QCOMPARE(gauge.value(), -25.0);
    }

    // ---- sizeHint ----
    void testSizeHint()
    {
        GaugeWidget gauge;
        QCOMPARE(gauge.sizeHint(), QSize(220, 220));
        QCOMPARE(gauge.minimumSizeHint(), QSize(180, 180));
    }

    // ---- 颜色设置不崩溃 ----
    void testSetColors()
    {
        GaugeWidget gauge;
        gauge.setArcColor(QColor("#FF0000"));
        gauge.setBackgroundArcColor(QColor("#CCCCCC"));
    }

    // ---- 渲染测试（不崩溃） ----
    void testPaint_doesNotCrash()
    {
        GaugeWidget gauge;
        gauge.setTitle("Test");
        gauge.setUnit("U");
        gauge.setMinValue(0);
        gauge.setMaxValue(100);
        gauge.setValue(50);
        gauge.resize(200, 200);

        QPixmap pixmap(200, 200);
        gauge.render(&pixmap);
        QVERIFY(!pixmap.isNull());
    }

    void testPaint_zeroRange()
    {
        GaugeWidget gauge;
        gauge.setMinValue(50);
        gauge.setMaxValue(50);
        gauge.setValue(50);
        gauge.resize(200, 200);

        QPixmap pixmap(200, 200);
        gauge.render(&pixmap);
        QVERIFY(!pixmap.isNull());
    }

    void testPaint_emptyTitle()
    {
        GaugeWidget gauge;
        gauge.setTitle("");
        gauge.setValue(50);
        gauge.resize(200, 200);

        QPixmap pixmap(200, 200);
        gauge.render(&pixmap);
        QVERIFY(!pixmap.isNull());
    }

    void testPaint_verySmallSize()
    {
        GaugeWidget gauge;
        gauge.setValue(50);
        gauge.resize(10, 10);

        QPixmap pixmap(10, 10);
        gauge.render(&pixmap);
        QVERIFY(!pixmap.isNull());
    }
};

QTEST_MAIN(TestGaugeWidget)
#include "test_gaugewidget.moc"
