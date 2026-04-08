#include "gaugewidget.h"
#include <QPainterPath>
#include <QtMath>
#include <QGraphicsDropShadowEffect>

GaugeWidget::GaugeWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(180, 180);
    
    // 添加阴影效果
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 40));
    shadow->setOffset(0, 4);
    setGraphicsEffect(shadow);
}

void GaugeWidget::setValue(double val)
{
    m_value = qBound(m_minValue, val, m_maxValue);
    update();
}

void GaugeWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    int side = qMin(width(), height());
    QRectF baseRect(0, 0, side, side);
    baseRect.moveCenter(QRectF(rect()).center());

    QRectF arcRect = baseRect.adjusted(20, 20, -20, -20);

    drawBackground(painter, baseRect);
    drawArc(painter, arcRect);
    drawTicks(painter, arcRect);
    drawValue(painter, arcRect);
    drawTitle(painter, arcRect);
    drawIndicator(painter, arcRect);
}

void GaugeWidget::drawBackground(QPainter &painter, const QRectF &rect)
{
    painter.save();
    
    // 外层卡片背景 - 玻璃拟态效果
    QRectF cardRect = rect.adjusted(6, 6, -6, -6);
    
    // 渐变背景
    QLinearGradient bgGrad(cardRect.topLeft(), cardRect.bottomRight());
    bgGrad.setColorAt(0, QColor(255, 255, 255, 250));
    bgGrad.setColorAt(1, QColor(248, 250, 252, 250));
    
    painter.setPen(QPen(QColor(230, 235, 240), 1));
    painter.setBrush(bgGrad);
    painter.drawRoundedRect(cardRect, 16, 16);
    
    // 内圈装饰
    QRectF innerCircle = cardRect.adjusted(15, 15, -15, -15);
    QRadialGradient innerGrad(innerCircle.center(), innerCircle.width() / 2);
    innerGrad.setColorAt(0, QColor(250, 252, 255));
    innerGrad.setColorAt(0.7, QColor(245, 247, 250));
    innerGrad.setColorAt(1, QColor(235, 240, 245));
    
    painter.setPen(Qt::NoPen);
    painter.setBrush(innerGrad);
    painter.drawEllipse(innerCircle);
    
    painter.restore();
}

void GaugeWidget::drawArc(QPainter &painter, const QRectF &rect)
{
    painter.save();

    QRectF arcRect = rect.adjusted(8, 8, -8, -8);
    int startAngle = 225 * 16;
    int spanAngle = -270 * 16;

    // 背景弧 - 更细腻的渐变
    QPen bgPen(QColor(230, 235, 240), 10, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(bgPen);
    painter.drawArc(arcRect, startAngle, spanAngle);

    // 数值弧 - 渐变色
    double ratio = 0.0;
    if (m_maxValue != m_minValue) {
        ratio = (m_value - m_minValue) / (m_maxValue - m_minValue);
    }
    int valueSpan = static_cast<int>(-270.0 * ratio * 16);

    // 创建圆锥渐变效果
    QConicalGradient arcGrad(arcRect.center(), 225);
    QColor startColor = m_arcColor;
    QColor endColor = m_arcColor.lighter(130);
    arcGrad.setColorAt(0, startColor);
    arcGrad.setColorAt(0.5, endColor);
    arcGrad.setColorAt(1, startColor);

    QPen valuePen(QBrush(arcGrad), 10, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(valuePen);
    painter.drawArc(arcRect, startAngle, valueSpan);

    // 发光效果
    if (ratio > 0.01) {
        QColor glowColor = m_arcColor;
        glowColor.setAlpha(60);
        QPen glowPen(glowColor, 16, Qt::SolidLine, Qt::RoundCap);
        painter.setPen(glowPen);
        painter.drawArc(arcRect, startAngle, valueSpan);
    }

    painter.restore();
}

void GaugeWidget::drawTicks(QPainter &painter, const QRectF &rect)
{
    painter.save();
    
    QRectF tickRect = rect.adjusted(4, 4, -4, -4);
    QPointF center = tickRect.center();
    double radius = tickRect.width() / 2;
    
    // 绘制刻度
    for (int i = 0; i <= 10; ++i) {
        double angle = (225 - i * 27) * M_PI / 180.0;
        double innerR = radius - 6;
        double outerR = radius - (i % 5 == 0 ? 2 : 4);
        
        QPointF inner(center.x() + innerR * qCos(angle), center.y() - innerR * qSin(angle));
        QPointF outer(center.x() + outerR * qCos(angle), center.y() - outerR * qSin(angle));
        
        QPen tickPen(i % 5 == 0 ? QColor(180, 185, 190) : QColor(210, 215, 220), 
                     i % 5 == 0 ? 2 : 1);
        painter.setPen(tickPen);
        painter.drawLine(inner, outer);
    }
    
    painter.restore();
}

void GaugeWidget::drawIndicator(QPainter &painter, const QRectF &rect)
{
    painter.save();
    
    QRectF arcRect = rect.adjusted(8, 8, -8, -8);
    QPointF center = arcRect.center();
    double radius = arcRect.width() / 2;
    
    double ratio = 0.0;
    if (m_maxValue != m_minValue) {
        ratio = (m_value - m_minValue) / (m_maxValue - m_minValue);
    }
    double angle = (225 - ratio * 270) * M_PI / 180.0;
    
    // 指示点
    QPointF indicatorPos(center.x() + radius * qCos(angle), 
                         center.y() - radius * qSin(angle));
    
    // 外发光
    QRadialGradient glow(indicatorPos, 10);
    glow.setColorAt(0, m_arcColor);
    glow.setColorAt(0.5, QColor(m_arcColor.red(), m_arcColor.green(), m_arcColor.blue(), 100));
    glow.setColorAt(1, Qt::transparent);
    
    painter.setPen(Qt::NoPen);
    painter.setBrush(glow);
    painter.drawEllipse(indicatorPos, 10, 10);
    
    // 中心点
    painter.setBrush(m_arcColor);
    painter.drawEllipse(indicatorPos, 5, 5);
    
    // 白色高光
    painter.setBrush(QColor(255, 255, 255, 180));
    painter.drawEllipse(indicatorPos + QPointF(-1.5, -1.5), 2, 2);
    
    painter.restore();
}

void GaugeWidget::drawValue(QPainter &painter, const QRectF &rect)
{
    painter.save();

    // 数值
    QFont valueFont("SF Pro Display", static_cast<int>(rect.height() * 0.16));
    valueFont.setWeight(QFont::Bold);
    valueFont.setLetterSpacing(QFont::PercentageSpacing, 98);
    painter.setFont(valueFont);
    
    // 数值阴影
    painter.setPen(QColor(0, 0, 0, 30));
    QString text = QString::number(m_value, 'f', 1);
    QRectF valueRect = rect;
    valueRect.moveTop(rect.top() + rect.height() * 0.32 + 1);
    painter.drawText(valueRect, Qt::AlignHCenter | Qt::AlignTop, text);
    
    // 数值主体
    painter.setPen(QColor(30, 41, 59));
    valueRect.moveTop(rect.top() + rect.height() * 0.32);
    painter.drawText(valueRect, Qt::AlignHCenter | Qt::AlignTop, text);

    // 单位
    if (!m_unit.isEmpty()) {
        QFont unitFont("SF Pro Text", static_cast<int>(rect.height() * 0.09));
        unitFont.setWeight(QFont::Medium);
        painter.setFont(unitFont);
        painter.setPen(QColor(100, 116, 139));
        
        QRectF unitRect = rect;
        unitRect.moveTop(rect.top() + rect.height() * 0.50);
        painter.drawText(unitRect, Qt::AlignHCenter | Qt::AlignTop, m_unit);
    }

    painter.restore();
}

void GaugeWidget::drawTitle(QPainter &painter, const QRectF &rect)
{
    if (m_title.isEmpty()) return;

    painter.save();

    QFont font("SF Pro Text", static_cast<int>(rect.height() * 0.085));
    font.setWeight(QFont::Medium);
    font.setLetterSpacing(QFont::PercentageSpacing, 102);
    painter.setFont(font);
    painter.setPen(QColor(71, 85, 105));

    QRectF titleRect = rect;
    titleRect.moveTop(rect.top() + rect.height() * 0.62);
    painter.drawText(titleRect, Qt::AlignHCenter | Qt::AlignTop, m_title);

    painter.restore();
}
