#ifndef GAUGEWIDGET_H
#define GAUGEWIDGET_H

#include <QWidget>
#include <QPainter>

/**
 * @brief 现代化仪表盘控件
 * 玻璃拟态设计，支持渐变、发光效果、刻度显示
 */
class GaugeWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(double value READ value WRITE setValue)
    Q_PROPERTY(double minValue READ minValue WRITE setMinValue)
    Q_PROPERTY(double maxValue READ maxValue WRITE setMaxValue)

public:
    explicit GaugeWidget(QWidget *parent = nullptr);

    double value() const { return m_value; }
    double minValue() const { return m_minValue; }
    double maxValue() const { return m_maxValue; }
    QString title() const { return m_title; }
    QString unit() const { return m_unit; }

    void setValue(double val);
    void setMinValue(double val) { m_minValue = val; update(); }
    void setMaxValue(double val) { m_maxValue = val; update(); }
    void setTitle(const QString &title) { m_title = title; update(); }
    void setUnit(const QString &unit) { m_unit = unit; update(); }
    void setArcColor(const QColor &color) { m_arcColor = color; update(); }
    void setBackgroundArcColor(const QColor &color) { m_bgArcColor = color; update(); }

    QSize minimumSizeHint() const override { return QSize(180, 180); }
    QSize sizeHint() const override { return QSize(220, 220); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void drawBackground(QPainter &painter, const QRectF &rect);
    void drawArc(QPainter &painter, const QRectF &rect);
    void drawTicks(QPainter &painter, const QRectF &rect);
    void drawIndicator(QPainter &painter, const QRectF &rect);
    void drawValue(QPainter &painter, const QRectF &rect);
    void drawTitle(QPainter &painter, const QRectF &rect);

    double m_value = 0.0;
    double m_minValue = 0.0;
    double m_maxValue = 100.0;
    QString m_title;
    QString m_unit;
    QColor m_arcColor = QColor("#3B82F6");
    QColor m_bgArcColor = QColor("#E2E8F0");
};

#endif // GAUGEWIDGET_H
