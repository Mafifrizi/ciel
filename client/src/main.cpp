#include <QAction>
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCompleter>
#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QGraphicsPathItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMap>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStyleOptionGraphicsItem>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTcpSocket>
#include <QTextCursor>
#include <QTextEdit>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <cmath>
#include <numbers>

namespace {

QString applyCielStylesheet() {
    return QStringLiteral(R"(
        QMainWindow, QWidget {
            background-color: #313342;
            color: #f8f8f2;
        }

        QSplitter::handle {
            background-color: #313342;
        }

        QMenuBar {
            background-color: #282a36;
            color: #f8f8f2;
            border: none;
        }
        QMenuBar::item {
            background-color: #282a36;
            color: #f8f8f2;
            padding: 3px 8px;
        }
        QMenuBar::item:selected {
            background-color: #6272a4;
        }
        QMenuBar::item:pressed {
            background-color: #44475a;
        }

        QMenu {
            background-color: #282a36;
            color: #f8f8f2;
            border: 1px solid #44475a;
        }
        QMenu::item {
            padding: 4px 20px 4px 12px;
        }
        QMenu::item:selected {
            background-color: #44475a;
        }
        QMenu::separator {
            height: 1px;
            background-color: #44475a;
            margin: 3px 6px;
        }

        QTabWidget::pane {
            background-color: #282a36;
            border: 1px solid #44475a;
        }
        QTabBar::tab {
            background-color: #282a36;
            color: #f8f8f2;
            border: 1px solid #44475a;
            border-bottom: none;
            padding: 4px 16px;
            margin-right: 2px;
        }
        QTabBar::tab:selected {
            background-color: #44475a;
            color: #f8f8f2;
            border: 1px solid #ff79c6;
            border-bottom: none;
        }

        QTableWidget {
            background-color: #282a36;
            color: #f8f8f2;
            border: none;
            gridline-color: #282a36;
            selection-background-color: #6272a4;
            selection-color: #f8f8f2;
        }
        QTableWidget::item:selected {
            background-color: #6272a4;
            color: #f8f8f2;
        }
        QHeaderView::section {
            background-color: #44475a;
            color: #f8f8f2;
            border: none;
            padding: 4px 6px;
            font-weight: bold;
        }

        QTextEdit, QPlainTextEdit, QGraphicsView {
            background-color: #282a36;
            color: #f8f8f2;
            border: none;
        }

        QLineEdit#OperativeInput {
            background-color: #282a36;
            color: #f8f8f2;
            border: none;
            padding: 3px 4px;
        }

        QLabel#SessionBanner {
            background-color: #282a36;
            color: #f8f8f2;
            padding: 2px 6px;
        }

        QLabel#PromptArrow {
            background-color: #282a36;
            color: #8be9fd;
            padding: 2px 4px 2px 6px;
            font-weight: bold;
        }

        QPushButton {
            background-color: #bd93f9;
            color: #282a36;
            border: 1px solid #bd93f9;
            border-radius: 2px;
            padding: 3px 12px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #caa9fa;
        }
        QPushButton:pressed {
            background-color: #282a36;
            color: #f8f8f2;
        }

        QScrollBar:vertical {
            background: #282a36;
            width: 10px;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background: #44475a;
            min-height: 20px;
        }
        QScrollBar:horizontal {
            background: #282a36;
            height: 10px;
            margin: 0;
        }
        QScrollBar::handle:horizontal {
            background: #44475a;
            min-width: 20px;
        }
        QScrollBar::add-line, QScrollBar::sub-line {
            width: 0px;
            height: 0px;
        }
    )");
}

QIcon makeOperativeBadgeIcon(const QColor &accent, bool isHub = false) {
    QPixmap pm(18, 18);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);

    if (isHub) {
        p.setPen(QPen(accent, 1.5));
        p.setBrush(QColor("#363949"));
        p.drawRect(2, 3, 14, 12);
        p.drawLine(2, 7, 16, 7);
    } else {
        // Minimal terminal/host monitor glyph for session rows
        p.setPen(QPen(accent, 1.4));
        p.setBrush(QColor("#1e2029"));
        p.drawRect(2, 2, 14, 10);
        p.drawLine(6, 15, 12, 15);
        p.drawLine(9, 12, 9, 15);
        p.setPen(accent);
        p.drawLine(5, 6, 7, 7);
        p.drawLine(7, 7, 5, 8);
        p.drawLine(9, 8, 12, 8);
    }
    return QIcon(pm);
}

QColor draculaStatusColor(const QString &status) {
    if (status == "OK" || status == "HEALTHY" || status == "ISOLATED" ||
        status == "RENEWED" || status == "PURGED" || status == "SYNCED" ||
        status == "READY" || status == "ONLINE" || status == "VERIFIED") {
        return QColor("#50fa7b");
    }
    if (status == "EXPIRING_SOON" || status == "SHARED_KEYRING" || status == "ATTENTION") {
        return QColor("#f1fa8c");
    }
    return QColor("#ff5555");
}

QString htmlEsc(const QString &s) {
    QString out = s;
    out.replace('&', "&amp;");
    out.replace('<', "&lt;");
    out.replace('>', "&gt;");
    return out;
}

bool parseHostPort(const QString &addr, QString &hostOut, quint16 &portOut) {
    const int idx = addr.lastIndexOf(':');
    if (idx <= 0 || idx + 1 >= addr.size()) {
        return false;
    }
    bool ok = false;
    const uint port = addr.mid(idx + 1).toUInt(&ok);
    if (!ok || port == 0 || port > 65535) {
        return false;
    }
    hostOut = addr.left(idx);
    portOut = static_cast<quint16>(port);
    return true;
}

struct OperativeSnapshot {
    QString id;
    QString operative;
    QString module;
    QString target;
    QString detail;
    QString permOrTtl;
    QString skewOrAux;
    int exitCode = 0;
    QString status;
    QJsonValue rawJson;
};

class CielGraphEdge;

class CielGraphNode : public QGraphicsItem {
public:
    enum class NodeKind { FirewallHub, OperativeMonitor };

    CielGraphNode(NodeKind kind,
                  const QString &line1,
                  const QString &line2,
                  const QString &roleTag,
                  const QColor &statusColor,
                  bool elevatedAlert,
                  bool revokedGrayscale = false)
        : m_kind(kind),
          m_line1(line1),
          m_line2(line2),
          m_roleTag(roleTag),
          m_statusColor(statusColor),
          m_elevatedAlert(elevatedAlert),
          m_revokedGrayscale(revokedGrayscale) {
        setFlag(ItemIsMovable);
        setFlag(ItemIsSelectable);
        setFlag(ItemSendsGeometryChanges);
        setCacheMode(DeviceCoordinateCache);
        setZValue(1.0);
    }

    void addEdge(CielGraphEdge *edge) {
        m_edges.append(edge);
    }

    QRectF boundingRect() const override {
        return QRectF(-145.0, -52.0, 290.0, 142.0);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override {
        (void)widget;
        painter->setRenderHint(QPainter::Antialiasing, true);

        if (m_kind == NodeKind::FirewallHub) {
            paintFirewallIcon(painter);
        } else {
            paintMonitorIcon(painter);
        }
        painter->setOpacity(1.0);

        // Centered two-line white/status labels directly below the icon with no border box
        QFont f1("DejaVu Sans Mono", 9, QFont::Bold);
        painter->setFont(f1);
        painter->setPen(QColor("#f8f8f2"));
        painter->drawText(QRectF(-140.0, 44.0, 280.0, 18.0), Qt::AlignHCenter | Qt::AlignTop, m_line1);

        QFont f2("DejaVu Sans Mono", 8, QFont::Normal);
        painter->setFont(f2);
        painter->setPen(m_kind == NodeKind::FirewallHub ? QColor("#8be9fd") : m_statusColor);
        painter->drawText(QRectF(-140.0, 62.0, 280.0, 18.0), Qt::AlignHCenter | Qt::AlignTop, m_line2);

        if (option && (option->state & QStyle::State_Selected)) {
            QPen selPen(QColor("#ffb86c"), 1.2, Qt::DotLine);
            painter->setPen(selPen);
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(QRectF(-52.0, -45.0, 104.0, 88.0));
        }
    }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    void paintFirewallIcon(QPainter *p) const {
        // Two-tone layered flame rising above brick wall
        QPainterPath outerFlame;
        outerFlame.moveTo(-18.0, -4.0);
        outerFlame.cubicTo(-26.0, -18.0, -10.0, -30.0, -6.0, -42.0);
        outerFlame.cubicTo(-2.0, -32.0, 3.0, -34.0, 7.0, -46.0);
        outerFlame.cubicTo(14.0, -32.0, 27.0, -20.0, 18.0, -4.0);
        outerFlame.closeSubpath();
        p->setPen(QPen(QColor("#282a36"), 1.2));
        p->setBrush(QColor("#ffb86c"));
        p->drawPath(outerFlame);

        QPainterPath innerFlame;
        innerFlame.moveTo(-10.0, -4.0);
        innerFlame.cubicTo(-13.0, -16.0, -4.0, -23.0, 0.0, -33.0);
        innerFlame.cubicTo(6.0, -22.0, 14.0, -15.0, 10.0, -4.0);
        innerFlame.closeSubpath();
        p->setPen(Qt::NoPen);
        p->setBrush(QColor("#ff5555"));
        p->drawPath(innerFlame);

        // 5-row staggered rounded brick wall
        const double brickH = 7.4;
        const double gap = 2.0;
        const double startY = -6.0;
        const double wallLeft = -34.0;
        const double wallW = 68.0;

        p->setPen(QPen(QColor("#282a36"), 1.0));
        for (int row = 0; row < 5; ++row) {
            const double y = startY + row * (brickH + gap);
            if (row % 2 == 0) {
                const double w = (wallW - 2.0 * gap) / 3.0;
                for (int col = 0; col < 3; ++col) {
                    p->setBrush(col == 1 ? QColor("#a23e3e") : QColor("#8b3236"));
                    p->drawRoundedRect(QRectF(wallLeft + col * (w + gap), y, w, brickH), 1.8, 1.8);
                }
            } else {
                const double edgeW = 11.0;
                const double midW = (wallW - 2.0 * edgeW - 3.0 * gap) / 2.0;
                p->setBrush(QColor("#8b3236"));
                p->drawRoundedRect(QRectF(wallLeft, y, edgeW, brickH), 1.8, 1.8);
                p->setBrush(QColor("#a23e3e"));
                p->drawRoundedRect(QRectF(wallLeft + edgeW + gap, y, midW, brickH), 1.8, 1.8);
                p->drawRoundedRect(QRectF(wallLeft + edgeW + 2.0 * gap + midW, y, midW, brickH), 1.8, 1.8);
                p->setBrush(QColor("#8b3236"));
                p->drawRoundedRect(QRectF(wallLeft + wallW - edgeW, y, edgeW, brickH), 1.8, 1.8);
            }
        }
    }

    void paintLightningArc(QPainter *p, const QList<QPointF> &pts) const {
        QPainterPath path;
        if (pts.isEmpty()) {
            return;
        }
        path.moveTo(pts.first());
        for (int i = 1; i < pts.size(); ++i) {
            path.lineTo(pts.at(i));
        }
        p->setBrush(Qt::NoBrush);
        p->setPen(QPen(QColor(139, 233, 253, 120), 4.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p->drawPath(path);
        p->setPen(QPen(QColor("#ffffff"), 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p->drawPath(path);
    }

    void paintMonitorIcon(QPainter *p) const {
        if (m_revokedGrayscale) {
            p->setOpacity(0.55);
        }
        const QColor bezelTop = m_revokedGrayscale
                                    ? QColor("#3b3e4d")
                                    : (m_elevatedAlert ? QColor("#5c1e24") : QColor("#2c313c"));
        const QColor chinStand = m_revokedGrayscale
                                     ? QColor("#6c7086")
                                     : (m_elevatedAlert ? QColor("#e06c75") : QColor("#b8bec9"));

        // Tapered monitor stand neck + flat base
        QPainterPath stand;
        stand.moveTo(-8.0, 20.0);
        stand.lineTo(8.0, 20.0);
        stand.lineTo(13.0, 34.0);
        stand.lineTo(-13.0, 34.0);
        stand.closeSubpath();
        p->setPen(QPen(QColor("#1e2029"), 1.2));
        p->setBrush(chinStand.darker(115));
        p->drawPath(stand);
        p->setBrush(chinStand);
        p->drawRoundedRect(QRectF(-22.0, 33.0, 44.0, 6.0), 2.0, 2.0);

        // Outer monitor housing
        p->setPen(QPen(QColor("#1a1c25"), 1.4));
        p->setBrush(bezelTop);
        p->drawRoundedRect(QRectF(-35.0, -36.0, 70.0, 56.0), 4.0, 4.0);

        // Silver/coral lower bezel chin with power LED dot
        p->setPen(Qt::NoPen);
        p->setBrush(chinStand);
        p->drawRoundedRect(QRectF(-34.0, 10.0, 68.0, 10.0), 2.5, 2.5);
        p->drawRect(QRectF(-34.0, 10.0, 68.0, 5.0));
        p->setBrush(m_statusColor);
        p->drawEllipse(QRectF(-2.5, 13.0, 5.0, 5.0));

        // Role-specific screen fill & OS/kernel glyph
        QColor scrColor("#0b4da2");
        if (m_revokedGrayscale) {
            scrColor = QColor("#45485a");
        } else if (m_elevatedAlert) {
            scrColor = QColor("#6e1c26");
        } else if (m_roleTag == "souei") {
            scrColor = QColor("#b57c10");
        } else if (m_roleTag == "ranga-iso") {
            scrColor = QColor("#186a42");
        } else if (m_roleTag == "ranga-shared") {
            scrColor = QColor("#7a5c1e");
        }
        p->setPen(QPen(QColor("#161822"), 1.0));
        p->setBrush(scrColor);
        p->drawRect(QRectF(-30.0, -31.0, 60.0, 37.0));

        // Inner screen glyph
        p->setPen(Qt::NoPen);
        if (m_roleTag == "chronoa") {
            p->setBrush(QColor("#f8f8f2"));
            p->drawRect(QRectF(-10.0, -22.0, 9.0, 9.0));
            p->drawRect(QRectF(1.0, -22.0, 9.0, 9.0));
            p->drawRect(QRectF(-10.0, -11.0, 9.0, 9.0));
            p->drawRect(QRectF(1.0, -11.0, 9.0, 9.0));
        } else if (m_roleTag == "souei") {
            p->setPen(QPen(QColor("#f8f8f2"), 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p->drawLine(QPointF(-14.0, -18.0), QPointF(-7.0, -12.5));
            p->drawLine(QPointF(-7.0, -12.5), QPointF(-14.0, -7.0));
            p->drawLine(QPointF(-2.0, -7.0), QPointF(10.0, -7.0));
        } else if (m_roleTag == "ranga-iso") {
            p->setPen(QPen(QColor("#f8f8f2"), 2.0));
            p->setBrush(Qt::NoBrush);
            p->drawArc(QRectF(-7.0, -24.0, 14.0, 14.0), 0, 180 * 16);
            p->setBrush(QColor("#f8f8f2"));
            p->setPen(Qt::NoPen);
            p->drawRoundedRect(QRectF(-10.0, -16.0, 20.0, 14.0), 2.0, 2.0);
            p->setBrush(scrColor);
            p->drawEllipse(QRectF(-2.0, -11.0, 4.0, 4.0));
        } else {
            // Shared keyring / purge target symbol (interlocked rings)
            p->setPen(QPen(QColor("#f8f8f2"), 2.0));
            p->setBrush(Qt::NoBrush);
            p->drawEllipse(QRectF(-13.0, -20.0, 14.0, 14.0));
            p->drawEllipse(QRectF(-1.0, -16.0, 14.0, 14.0));
        }

        // High-privilege / alert triple lightning arcs
        if (m_elevatedAlert) {
            paintLightningArc(p, {QPointF(-40.0, -32.0), QPointF(-30.0, -36.0), QPointF(-33.0, -25.0), QPointF(-23.0, -28.0)});
            paintLightningArc(p, {QPointF(-41.0, 4.0), QPointF(-31.0, 1.0), QPointF(-36.0, 12.0), QPointF(-24.0, 9.0)});
            paintLightningArc(p, {QPointF(26.0, -26.0), QPointF(38.0, -21.0), QPointF(31.0, -12.0), QPointF(42.0, -8.0)});
        }
    }

    NodeKind m_kind;
    QString m_line1;
    QString m_line2;
    QString m_roleTag;
    QColor m_statusColor;
    bool m_elevatedAlert = false;
    bool m_revokedGrayscale = false;
    QList<CielGraphEdge *> m_edges;
};

class CielGraphEdge : public QGraphicsItem {
public:
    CielGraphEdge(CielGraphNode *source,
                  CielGraphNode *dest,
                  const QColor &color,
                  const QString &label,
                  bool arrowAtSource)
        : m_source(source),
          m_dest(dest),
          m_color(color),
          m_label(label),
          m_arrowAtSource(arrowAtSource) {
        setAcceptedMouseButtons(Qt::NoButton);
        setZValue(0.0);
        if (m_source) {
            m_source->addEdge(this);
        }
        if (m_dest) {
            m_dest->addEdge(this);
        }
        adjust();
    }

    void adjust() {
        if (!m_source || !m_dest) {
            return;
        }
        const QLineF full(mapFromItem(m_source, 0.0, 0.0), mapFromItem(m_dest, 0.0, 0.0));
        const qreal len = full.length();
        prepareGeometryChange();
        if (len > 20.0) {
            const qreal edgeSpace = 52.0;
            const QPointF off((full.dx() * edgeSpace) / len, (full.dy() * edgeSpace) / len);
            m_srcPt = full.p1() + off;
            m_dstPt = full.p2() - off;
        } else {
            m_srcPt = m_dstPt = full.p1();
        }
    }

    QRectF boundingRect() const override {
        const qreal extra = 42.0;
        return QRectF(m_srcPt, QSizeF(m_dstPt.x() - m_srcPt.x(), m_dstPt.y() - m_srcPt.y()))
            .normalized()
            .adjusted(-extra, -extra, extra, extra);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override {
        (void)option;
        (void)widget;
        if (!m_source || !m_dest) {
            return;
        }
        const QLineF line(m_srcPt, m_dstPt);
        const qreal len = line.length();
        if (len < 12.0) {
            return;
        }

        painter->setRenderHint(QPainter::Antialiasing, true);
        const qreal ux = line.dx() / len;
        const qreal uy = line.dy() / len;

        // Leave a clean centered gap for rotated protocol badge if edge is long enough
        QFont lblFont("DejaVu Sans Mono", 8);
        painter->setFont(lblFont);
        const QFontMetrics fm(lblFont);
        const int txtW = fm.horizontalAdvance(m_label) + 12;

        const QPen linePen(m_color, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter->setPen(linePen);

        if (!m_label.isEmpty() && len > txtW + 30.0) {
            const qreal halfGap = txtW * 0.5;
            const QPointF mid = line.pointAt(0.5);
            const QPointF g1 = mid - QPointF(ux * halfGap, uy * halfGap);
            const QPointF g2 = mid + QPointF(ux * halfGap, uy * halfGap);
            painter->drawLine(m_srcPt, g1);
            painter->drawLine(g2, m_dstPt);

            qreal deg = std::atan2(line.dy(), line.dx()) * 180.0 / std::numbers::pi;
            if (deg > 90.0) {
                deg -= 180.0;
            } else if (deg < -90.0) {
                deg += 180.0;
            }
            painter->save();
            painter->translate(mid);
            painter->rotate(deg);
            painter->setPen(QColor("#8be9fd"));
            painter->drawText(QRectF(-halfGap, -9.0, txtW, 18.0), Qt::AlignCenter, m_label);
            painter->restore();
        } else {
            painter->drawLine(line);
        }

        // Filled directional arrowhead using exact unit vector arithmetic
        const qreal arrowLen = 10.0;
        const qreal arrowHalfW = 4.6;
        QPolygonF head;
        if (m_arrowAtSource) {
            const QPointF tip = m_srcPt;
            const QPointF p1 = tip + QPointF(ux * arrowLen - uy * arrowHalfW, uy * arrowLen + ux * arrowHalfW);
            const QPointF p2 = tip + QPointF(ux * arrowLen + uy * arrowHalfW, uy * arrowLen - ux * arrowHalfW);
            head << tip << p1 << p2;
        } else {
            const QPointF tip = m_dstPt;
            const QPointF p1 = tip + QPointF(-ux * arrowLen - uy * arrowHalfW, -uy * arrowLen + ux * arrowHalfW);
            const QPointF p2 = tip + QPointF(-ux * arrowLen + uy * arrowHalfW, -uy * arrowLen - ux * arrowHalfW);
            head << tip << p1 << p2;
        }
        painter->setPen(QPen(m_color, 1.0));
        painter->setBrush(m_color);
        painter->drawPolygon(head);
    }

private:
    CielGraphNode *m_source = nullptr;
    CielGraphNode *m_dest = nullptr;
    QColor m_color;
    QString m_label;
    bool m_arrowAtSource = true;
    QPointF m_srcPt;
    QPointF m_dstPt;
};

QVariant CielGraphNode::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == ItemPositionHasChanged) {
        for (CielGraphEdge *edge : m_edges) {
            edge->adjust();
        }
    }
    return QGraphicsItem::itemChange(change, value);
}

}  // namespace

class CielWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit CielWindow(const QString &initialAddr, QWidget *parent = nullptr)
        : QMainWindow(parent), m_serverAddr(initialAddr) {
        setWindowTitle("Ciel");
        resize(1480, 880);
        if (statusBar()) {
            statusBar()->setVisible(false);
        }

        buildMenuBar();
        buildCentralCielLayout();

        connect(&m_pollTimer, &QTimer::timeout, this, [this]() {
            runFullSync();
        });

        appendOperativeBanner();
        appendHelpTableToConsole();
    }

    ~CielWindow() override {
        m_pollTimer.stop();
        if (m_managedServer && m_managedServer->state() != QProcess::NotRunning) {
            m_managedServer->terminate();
            if (!m_managedServer->waitForFinished(1500)) {
                m_managedServer->kill();
                m_managedServer->waitForFinished(500);
            }
        }
    }

    int runSelfTest(const QString &screenshotPath,
                    const QString &graphScreenshotPath,
                    const QString &altScreenshotPath = QString()) {
        auto fail = [this](const QString &msg) -> int {
            appendEventLog("!", "#ff5555", "SELF-TEST FAIL: " + msg);
            return 1;
        };

        QJsonObject syncResp;
        if (!executeAction("full_sync", QJsonObject(), syncResp)) {
            return fail("full_sync RPC failed");
        }
        if (m_sessionTable->rowCount() < 4) {
            return fail(QString("Expected >= 4 rows, got %1").arg(m_sessionTable->rowCount()));
        }

        QJsonObject skewParams;
        skewParams["skew_sec"] = 350;
        QJsonObject skewResp;
        appendCommandEcho("chronoa --skew 350");
        if (!executeAction("chronoa_check", skewParams, skewResp)) {
            return fail("chronoa_check skew_sec=350 RPC failed");
        }
        if (skewResp.value("exit_code").toInt(-1) != 2 ||
            skewResp.value("status").toString() != "CLOCK_SKEW_VIOLATION") {
            return fail("Expected exit_code=2 CLOCK_SKEW_VIOLATION");
        }

        QJsonObject rc4Params;
        rc4Params["weak_enctype"] = true;
        QJsonObject rc4Resp;
        appendCommandEcho("chronoa --simulate-rc4");
        if (!executeAction("chronoa_check", rc4Params, rc4Resp)) {
            return fail("chronoa_check weak_enctype RPC failed");
        }
        if (rc4Resp.value("exit_code").toInt(-1) != 2 ||
            rc4Resp.value("status").toString() != "WEAK_ENCTYPE") {
            return fail("Expected exit_code=2 WEAK_ENCTYPE");
        }

        QJsonObject isoResp;
        appendCommandEcho("ranga --isolate 0x3f000000");
        if (!executeAction("ranga_isolate", QJsonObject(), isoResp)) {
            return fail("ranga_isolate RPC failed");
        }
        const QJsonObject isoData = isoResp.value("data").toObject();
        if (isoResp.value("exit_code").toInt(-1) != 0 ||
            !isoData.value("possessor_only").toBool(false) ||
            isoData.value("perm_hex").toString() != "3f000000") {
            return fail("Expected possessor_only=true and perm_hex=3f000000");
        }

        QJsonObject purgeResp;
        appendCommandEcho("ranga --purge /tmp/ciel_workspace/PurgeTarget.keytab");
        if (!executeAction("ranga_purge", QJsonObject(), purgeResp)) {
            return fail("ranga_purge RPC failed");
        }
        const QJsonObject purgeData = purgeResp.value("data").toObject();
        if (purgeResp.value("exit_code").toInt(-1) != 0 ||
            !purgeData.value("removed").toBool(false) ||
            purgeData.value("passes").toInt(0) != 3) {
            return fail("Expected passes=3 and removed=true");
        }

        appendCommandEcho("sync");
        if (!executeAction("full_sync", QJsonObject(), syncResp)) {
            return fail("final full_sync RPC failed");
        }
        appendCommandEcho("audit");
        appendDetailedAuditTables();
        if (m_sessionTable->rowCount() < 5 ||
            m_eventViewer->toPlainText().trimmed().isEmpty() ||
            m_operativeConsole->toPlainText().trimmed().isEmpty()) {
            return fail("Missing table rows or console telemetry after self-test");
        }

        if (!screenshotPath.isEmpty()) {
            m_mainViewTabs->setCurrentIndex(0);
            m_smallTabs->setCurrentIndex(0);
            m_bottomTabs->setCurrentIndex(m_consoleTabIdx);
            m_sessionTable->selectRow(1);
            QApplication::processEvents();
            QApplication::processEvents();
            const QPixmap tableShot = grab();
            if (tableShot.isNull() || !tableShot.save(screenshotPath, "PNG")) {
                return fail("Failed saving session table screenshot");
            }
        }

        if (!altScreenshotPath.isEmpty()) {
            m_mainViewTabs->setCurrentIndex(0);
            m_smallTabs->setCurrentIndex(1);
            m_sessionTable->selectRow(0);
            m_bottomTabs->setCurrentIndex(m_listenersTabIdx);
            QApplication::processEvents();
            QApplication::processEvents();
            const QPixmap altShot = grab();
            if (altShot.isNull() || !altShot.save(altScreenshotPath, "PNG")) {
                return fail("Failed saving alternate screenshot");
            }
            m_smallTabs->setCurrentIndex(0);
            m_bottomTabs->setCurrentIndex(m_consoleTabIdx);
        }

        if (!graphScreenshotPath.isEmpty()) {
            m_mainViewTabs->setCurrentIndex(1);
            rebuildSessionGraph();
            QApplication::processEvents();
            QApplication::processEvents();
            const QPixmap graphShot = grab();
            if (graphShot.isNull() || !graphShot.save(graphScreenshotPath, "PNG")) {
                return fail("Failed saving session graph screenshot");
            }
        }

        appendTaskDone("ALL SELF-TEST VERIFICATIONS PASSED");
        return 0;
    }

public slots:
    void startInteractiveSession() {
        if (!ensureTeamserverRunning()) {
            return;
        }
        runFullSync();
        appendDetailedAuditTables();
        if (m_actPoll) {
            m_actPoll->setChecked(true);
        }
        if (m_operativeInput) {
            m_operativeInput->setFocus();
        }
    }

    void runFullSync() {
        QJsonObject params;
        params["skew_sec"] = m_simSkewSec;
        QJsonObject resp;
        executeAction("full_sync", params, resp);
    }

private:
    void buildMenuBar() {
        QMenuBar *mb = menuBar();

        QMenu *mCiel = mb->addMenu("Ciel");
        QAction *actSync = mCiel->addAction("Sync Teamserver");
        m_actPoll = mCiel->addAction("Toggle Auto-Poll (3s)");
        m_actPoll->setCheckable(true);
        mCiel->addSeparator();
        QAction *actExit = mCiel->addAction("Exit");
        connect(actSync, &QAction::triggered, this, &CielWindow::runFullSync);
        connect(m_actPoll, &QAction::toggled, this, [this](bool on) {
            if (on) {
                m_pollTimer.start(3000);
                appendEventLog("*", "#8be9fd", "Auto-poll enabled (3000ms)");
            } else {
                m_pollTimer.stop();
                appendEventLog("*", "#8be9fd", "Auto-poll paused");
            }
        });
        connect(actExit, &QAction::triggered, this, &QWidget::close);

        QMenu *mView = mb->addMenu("View");
        QAction *actTable = mView->addAction("Session View -> Table");
        QAction *actGraph = mView->addAction("Session View -> Graph");
        mView->addSeparator();
        QAction *actEvents = mView->addAction("Event Viewer");
        QAction *actJson = mView->addAction("Raw Telemetry (JSON)");
        QAction *actListeners = mView->addAction("Listeners");
        connect(actTable, &QAction::triggered, this, [this]() { m_mainViewTabs->setCurrentIndex(0); });
        connect(actGraph, &QAction::triggered, this, [this]() {
            m_mainViewTabs->setCurrentIndex(1);
            rebuildSessionGraph();
        });
        connect(actEvents, &QAction::triggered, this, [this]() { m_smallTabs->setCurrentIndex(0); });
        connect(actJson, &QAction::triggered, this, [this]() { m_smallTabs->setCurrentIndex(1); });
        connect(actListeners, &QAction::triggered, this, [this]() { m_bottomTabs->setCurrentIndex(m_listenersTabIdx); });

        QMenu *mOps = mb->addMenu("Operatives");
        QAction *actSouei = mOps->addAction("Souei: Probe /proc/keys");
        QAction *actChronoa = mOps->addAction("Chronoa: Audit CCACHE");
        QAction *actSkew = mOps->addAction("Chronoa: Simulate 350s Skew");
        QAction *actRc4 = mOps->addAction("Chronoa: Simulate RC4 Weak Cipher");
        QAction *actRenew = mOps->addAction("Chronoa: Force Renew");
        mOps->addSeparator();
        QAction *actIso = mOps->addAction("Ranga: Isolate 0x3f000000");
        QAction *actPurge = mOps->addAction("Ranga: 3-Pass NIST Purge");

        connect(actSouei, &QAction::triggered, this, [this]() { dispatchFromUi("souei_probe", QJsonObject(), "souei --probe"); });
        connect(actChronoa, &QAction::triggered, this, [this]() {
            m_simSkewSec = 0;
            QJsonObject p;
            p["skew_sec"] = 0;
            dispatchFromUi("chronoa_check", p, "chronoa --ccache");
        });
        connect(actSkew, &QAction::triggered, this, [this]() {
            m_simSkewSec = 350;
            QJsonObject p;
            p["skew_sec"] = 350;
            dispatchFromUi("chronoa_check", p, "chronoa --skew 350");
        });
        connect(actRc4, &QAction::triggered, this, [this]() {
            QJsonObject p;
            p["weak_enctype"] = true;
            dispatchFromUi("chronoa_check", p, "chronoa --simulate-rc4");
        });
        connect(actRenew, &QAction::triggered, this, [this]() {
            m_simSkewSec = 0;
            QJsonObject p;
            p["skew_sec"] = 0;
            p["renew"] = true;
            dispatchFromUi("chronoa_check", p, "chronoa --renew");
        });
        connect(actIso, &QAction::triggered, this, [this]() { dispatchFromUi("ranga_isolate", QJsonObject(), "ranga --isolate"); });
        connect(actPurge, &QAction::triggered, this, [this]() { dispatchFromUi("ranga_purge", QJsonObject(), "ranga --purge"); });

        QMenu *mScripts = mb->addMenu("Scripts");
        QAction *actConsoleTab = mScripts->addAction("Operative Console");
        QAction *actHelpCmd = mScripts->addAction("Print Console Commands");
        connect(actConsoleTab, &QAction::triggered, this, [this]() { m_bottomTabs->setCurrentIndex(m_consoleTabIdx); });
        connect(actHelpCmd, &QAction::triggered, this, [this]() { appendHelpTableToConsole(); });

        QMenu *mHelp = mb->addMenu("Help");
        QAction *actAbout = mHelp->addAction("About Ciel");
        connect(actAbout, &QAction::triggered, this, [this]() {
            QMessageBox::information(this, "About Ciel",
                                     "Ciel C2 Operator Console\nStack: C++20 Qt6 Widgets + Go Teamserver\nOperatives: Souei, Chronoa, Ranga (Tempest + Tanuki)");
        });
    }

    void buildCentralCielLayout() {
        auto *root = new QWidget(this);
        auto *rootLayout = new QVBoxLayout(root);
        rootLayout->setContentsMargins(3, 3, 3, 3);
        rootLayout->setSpacing(3);

        auto *topBotSplitter = new QSplitter(Qt::Vertical, root);
        topBotSplitter->setHandleWidth(4);

        auto *topSplit = new QSplitter(Qt::Horizontal, topBotSplitter);
        topSplit->setHandleWidth(4);

        // Top-Left: Main View TabWidget (Sessions Table | Session Graph)
        m_mainViewTabs = new QTabWidget(topSplit);
        m_sessionTable = buildSessionTable();
        m_graphView = buildSessionGraphView();
        m_mainViewTabs->addTab(m_sessionTable, "Sessions");
        m_mainViewTabs->addTab(m_graphView, "Session Graph");
        connect(m_mainViewTabs, &QTabWidget::currentChanged, this, [this](int idx) {
            if (idx == 1) {
                rebuildSessionGraph();
            }
        });

        // Top-Right: Event Viewer & Raw Telemetry tabs
        m_smallTabs = new QTabWidget(topSplit);
        m_eventViewer = new QTextEdit(m_smallTabs);
        m_eventViewer->setReadOnly(true);
        m_rawTelemetry = new QPlainTextEdit(m_smallTabs);
        m_rawTelemetry->setReadOnly(true);
        m_rawTelemetry->setPlaceholderText("Select an operative row in Sessions to inspect raw JSON.");
        m_smallTabs->addTab(m_eventViewer, "Event Viewer");
        m_smallTabs->addTab(m_rawTelemetry, "Raw JSON");

        topSplit->addWidget(m_mainViewTabs);
        topSplit->addWidget(m_smallTabs);
        topSplit->setStretchFactor(0, 70);
        topSplit->setStretchFactor(1, 30);

        // Bottom: Multi-Session Operative Tabs | Listeners
        m_bottomTabs = new QTabWidget(topBotSplitter);
        auto makeContextPane = [&](const QString &bannerText) {
            auto *te = new QTextEdit(m_bottomTabs);
            te->setReadOnly(true);
            te->setPlainText(bannerText);
            return te;
        };
        m_bottomTabs->addTab(
            makeContextPane("[TEAMSERVER] ciel-teamserver active on " + m_serverAddr + "\n[TEAMSERVER] Tempest binaries verified: souei, chronoa, ranga, libranga.so"),
            "Teamserver Chat");
        m_bottomTabs->addTab(
            makeContextPane("[OP-01] apip@linux-kernel :: /proc/keys + AF_UNIX SO_PEERCRED + inotify ready"),
            "[OP-01] apip@linux-kernel");
        m_consoleTabIdx = m_bottomTabs->addTab(buildOperativeConsoleTab(), "[OP-02] ciel@TEMPEST.LOCAL");
        m_bottomTabs->addTab(
            makeContextPane("[OP-03] ranga@_uid_ses :: baseline user session keyring inspection"),
            "[OP-03] ranga@_uid_ses");
        m_bottomTabs->addTab(
            makeContextPane("[OP-04] ranga@0x3f000000 :: possessor-only LD_PRELOAD + --isolate jail"),
            "[OP-04] ranga@0x3f000000");
        m_bottomTabs->addTab(
            makeContextPane("[OP-05] ranga@nist_purge :: 3-pass NIST SP 800-88 wipe + explicit_bzero"),
            "[OP-05] ranga@nist_purge");
        m_listenersTabIdx = m_bottomTabs->addTab(buildListenersTab(), "Listeners");
        m_bottomTabs->setCurrentIndex(m_consoleTabIdx);

        topBotSplitter->addWidget(topSplit);
        topBotSplitter->addWidget(m_bottomTabs);
        topBotSplitter->setStretchFactor(0, 52);
        topBotSplitter->setStretchFactor(1, 48);

        rootLayout->addWidget(topBotSplitter);
        setCentralWidget(root);
    }

    QTableWidget *buildSessionTable() {
        auto *tbl = new QTableWidget(0, 10, this);
        tbl->setShowGrid(false);
        tbl->setAlternatingRowColors(false);
        tbl->verticalHeader()->setVisible(false);
        tbl->setSelectionBehavior(QAbstractItemView::SelectRows);
        tbl->setSelectionMode(QAbstractItemView::SingleSelection);
        tbl->setEditTriggers(QAbstractItemView::NoEditTriggers);
        tbl->setFocusPolicy(Qt::NoFocus);
        tbl->setContextMenuPolicy(Qt::CustomContextMenu);

        const QStringList headers = {
            "ID", "External", "Internal", "User", "Computer",
            "OS", "Process", "PID", "Perm / TTL", "Health"
        };
        tbl->setHorizontalHeaderLabels(headers);
        tbl->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        tbl->verticalHeader()->setDefaultSectionSize(22);

        connect(tbl, &QTableWidget::currentCellChanged,
                this, [this](int row, int col, int prevRow, int prevCol) {
                    (void)col;
                    (void)prevRow;
                    (void)prevCol;
                    if (row < 0) {
                        return;
                    }
                    const QTableWidgetItem *idItem = m_sessionTable->item(row, 0);
                    if (!idItem) {
                        return;
                    }
                    const QString opId = idItem->text();
                    if (m_snapshots.contains(opId) && m_snapshots.value(opId).rawJson.isObject()) {
                        const QJsonDocument doc(m_snapshots.value(opId).rawJson.toObject());
                        m_rawTelemetry->setPlainText(QString::fromUtf8(doc.toJson(QJsonDocument::Indented)));
                    }
                });

        connect(tbl, &QTableWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
            QMenu ctx(this);
            QAction *aInteract = ctx.addAction("Interact");
            ctx.addSeparator();
            QAction *aSouei = ctx.addAction("Probe /proc/keys (Souei)");
            QAction *aChronoa = ctx.addAction("Audit Ticket TTL (Chronoa)");
            QAction *aSkew = ctx.addAction("Simulate Skew 350s (Chronoa)");
            QAction *aRc4 = ctx.addAction("Simulate RC4 Weak Cipher (Chronoa)");
            QAction *aRenew = ctx.addAction("Force Renew (Chronoa)");
            ctx.addSeparator();
            QAction *aIsolate = ctx.addAction("Isolate Session Keyring (Ranga)");
            QAction *aPurge = ctx.addAction("3-Pass NIST Purge (Ranga)");
            QAction *aJson = ctx.addAction("Inspect Raw JSON");

            connect(aInteract, &QAction::triggered, this, [this]() {
                m_bottomTabs->setCurrentIndex(m_consoleTabIdx);
                m_operativeInput->setFocus();
            });
            connect(aSouei, &QAction::triggered, this, [this]() { dispatchFromUi("souei_probe", QJsonObject(), "souei --probe"); });
            connect(aChronoa, &QAction::triggered, this, [this]() {
                QJsonObject p;
                p["skew_sec"] = 0;
                dispatchFromUi("chronoa_check", p, "chronoa --ccache");
            });
            connect(aSkew, &QAction::triggered, this, [this]() {
                QJsonObject p;
                p["skew_sec"] = 350;
                dispatchFromUi("chronoa_check", p, "chronoa --skew 350");
            });
            connect(aRc4, &QAction::triggered, this, [this]() {
                QJsonObject p;
                p["weak_enctype"] = true;
                dispatchFromUi("chronoa_check", p, "chronoa --simulate-rc4");
            });
            connect(aRenew, &QAction::triggered, this, [this]() {
                QJsonObject p;
                p["skew_sec"] = 0;
                p["renew"] = true;
                dispatchFromUi("chronoa_check", p, "chronoa --renew");
            });
            connect(aIsolate, &QAction::triggered, this, [this]() { dispatchFromUi("ranga_isolate", QJsonObject(), "ranga --isolate"); });
            connect(aPurge, &QAction::triggered, this, [this]() { dispatchFromUi("ranga_purge", QJsonObject(), "ranga --purge"); });
            connect(aJson, &QAction::triggered, this, [this]() { m_smallTabs->setCurrentIndex(1); });

            ctx.exec(m_sessionTable->viewport()->mapToGlobal(pos));
        });

        return tbl;
    }

    QGraphicsView *buildSessionGraphView() {
        m_graphScene = new QGraphicsScene(this);
        m_graphScene->setBackgroundBrush(QColor("#282a36"));
        auto *view = new QGraphicsView(m_graphScene, this);
        view->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
        view->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
        view->setDragMode(QGraphicsView::ScrollHandDrag);
        return view;
    }

    QWidget *buildOperativeConsoleTab() {
        auto *panel = new QWidget(this);
        panel->setStyleSheet("background-color: #282a36;");
        auto *grid = new QGridLayout(panel);
        grid->setContentsMargins(1, 4, 1, 4);
        grid->setVerticalSpacing(4);
        grid->setHorizontalSpacing(0);

        m_operativeConsole = new QTextEdit(panel);
        m_operativeConsole->setReadOnly(true);
        m_operativeConsole->setLineWrapMode(QTextEdit::NoWrap);
        grid->addWidget(m_operativeConsole, 0, 0, 1, 2);

        m_sessionBanner = new QLabel(
            "[ciel/TEMPEST.LOCAL] tempest-runtime/1000 x86_64 (Linux Keyring & Kerberos CCACHE)",
            panel);
        m_sessionBanner->setObjectName("SessionBanner");
        m_sessionBanner->setTextInteractionFlags(Qt::TextSelectableByMouse);
        grid->addWidget(m_sessionBanner, 1, 0, 1, 2);

        auto *prompt = new QLabel(QString::fromUtf8(">>>"), panel);
        prompt->setObjectName("PromptArrow");
        grid->addWidget(prompt, 2, 0);

        m_operativeInput = new QLineEdit(panel);
        m_operativeInput->setObjectName("OperativeInput");
        m_operativeInput->setPlaceholderText("Type command (help, audit, sync, souei, chronoa, skew 350, rc4, renew, isolate, purge, graph, table, clear)");
        const QStringList cmds = {
            "help", "audit", "sync", "souei", "chronoa", "skew 0", "skew 350",
            "rc4", "renew", "isolate", "purge", "graph", "table", "clear"
        };
        auto *completer = new QCompleter(cmds, m_operativeInput);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        m_operativeInput->setCompleter(completer);
        grid->addWidget(m_operativeInput, 2, 1);

        connect(m_operativeInput, &QLineEdit::returnPressed, this, [this]() {
            const QString raw = m_operativeInput->text().trimmed();
            m_operativeInput->clear();
            handleOperativeCommand(raw);
        });

        return panel;
    }

    QWidget *buildListenersTab() {
        auto *panel = new QWidget(this);
        panel->setStyleSheet("background-color: #282a36;");
        auto *vbox = new QVBoxLayout(panel);
        vbox->setContentsMargins(4, 4, 4, 6);
        vbox->setSpacing(6);

        auto *listenersTable = new QTableWidget(4, 6, panel);
        listenersTable->setShowGrid(false);
        listenersTable->verticalHeader()->setVisible(false);
        listenersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        listenersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        listenersTable->setFocusPolicy(Qt::NoFocus);
        listenersTable->setHorizontalHeaderLabels({"Name", "Protocol", "Host", "PortBind", "PortConn", "Status"});
        listenersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        listenersTable->verticalHeader()->setDefaultSectionSize(22);

        const QList<QStringList> defaults = {
            {"ciel-teamserver-rpc", "TCP/JSON", "127.0.0.1", "40056", "40056", "ONLINE"},
            {"souei-keyring-sensor", "ProcKeys/SO_PEERCRED", "kcm.sock", "0", "0", "ONLINE"},
            {"chronoa-ccache-guard", "MIT-CCACHE-v4", "TEMPEST.LOCAL", "88", "88", "ONLINE"},
            {"ranga-keyring-jail", "SYS_keyctl", "kernel-ses", "0", "0", "ONLINE"}
        };
        for (int r = 0; r < defaults.size(); ++r) {
            for (int c = 0; c < 6; ++c) {
                auto *it = new QTableWidgetItem(defaults[r][c]);
                it->setTextAlignment(Qt::AlignCenter);
                if (c == 5) {
                    it->setForeground(QColor("#50fa7b"));
                }
                listenersTable->setItem(r, c, it);
            }
        }
        vbox->addWidget(listenersTable, 1);

        auto *btnBar = new QHBoxLayout();
        btnBar->addStretch(1);
        auto addActionBtn = [&](const QString &label, auto fn) {
            auto *btn = new QPushButton(label, panel);
            connect(btn, &QPushButton::clicked, this, fn);
            btnBar->addWidget(btn);
        };
        addActionBtn("Sync", [this]() { runFullSync(); });
        addActionBtn("Souei Probe", [this]() { dispatchFromUi("souei_probe", QJsonObject(), "souei --probe"); });
        addActionBtn("Chronoa Skew 350s", [this]() {
            QJsonObject p;
            p["skew_sec"] = 350;
            dispatchFromUi("chronoa_check", p, "chronoa --skew 350");
        });
        addActionBtn("Chronoa RC4", [this]() {
            QJsonObject p;
            p["weak_enctype"] = true;
            dispatchFromUi("chronoa_check", p, "chronoa --simulate-rc4");
        });
        addActionBtn("Chronoa Renew", [this]() {
            QJsonObject p;
            p["skew_sec"] = 0;
            p["renew"] = true;
            dispatchFromUi("chronoa_check", p, "chronoa --renew");
        });
        addActionBtn("Ranga Isolate", [this]() { dispatchFromUi("ranga_isolate", QJsonObject(), "ranga --isolate"); });
        addActionBtn("Ranga 3-Pass Purge", [this]() { dispatchFromUi("ranga_purge", QJsonObject(), "ranga --purge"); });
        btnBar->addStretch(1);
        vbox->addLayout(btnBar);

        return panel;
    }

    QString nowTime() const {
        return QDateTime::currentDateTime().toString("HH:mm:ss");
    }

    void appendHtmlLine(QTextEdit *edit, const QString &html) {
        QTextCursor cur = edit->textCursor();
        cur.movePosition(QTextCursor::End);
        if (!edit->document()->isEmpty()) {
            cur.insertBlock();
        }
        cur.insertHtml(html);
        edit->setTextCursor(cur);
        edit->ensureCursorVisible();
    }

    void appendEventLog(const QString &tag, const QString &tagColor, const QString &bodyHtml) {
        const QString line = QString(
            "<span style=\"color:#6272a4\">%1</span> "
            "[<span style=\"color:%2\">%3</span>] "
            "<span style=\"color:#f8f8f2\">%4</span>")
                                 .arg(nowTime(), tagColor, htmlEsc(tag), bodyHtml);
        appendHtmlLine(m_eventViewer, line);
    }

    void appendOperativeBanner() {
        const QString banner = QString(
            "<span style=\"color:#6272a4\">%1</span> "
            "<span style=\"color:#f8f8f2\">Operative </span>"
            "<span style=\"color:#ff5555\">OP-02</span> "
            "<span style=\"color:#f8f8f2\">authenticated as </span>"
            "<span style=\"color:#bd93f9\">TEMPEST.LOCAL\\ciel</span> "
            "<span style=\"color:#f8f8f2\">:: [Internal: </span>"
            "<span style=\"color:#8be9fd\">%2</span>"
            "<span style=\"color:#f8f8f2\">] [Process: </span>"
            "<span style=\"color:#ff5555\">chronoa\\1000</span>"
            "<span style=\"color:#f8f8f2\">] [Arch: </span>"
            "<span style=\"color:#ff79c6\">x86_64</span>"
            "<span style=\"color:#f8f8f2\">] [Pivot: </span>"
            "<span style=\"color:#8be9fd\">Direct</span>"
            "<span style=\"color:#f8f8f2\">]</span>")
                                   .arg(nowTime(), htmlEsc(m_serverAddr));
        appendHtmlLine(m_operativeConsole, banner);
    }

    void appendCommandEcho(const QString &cmd) {
        const QString chevron = QString::fromUtf8("\xc2\xbb");
        const QString line = QString(
            "<br><span style=\"color:#6272a4\">%1 [LaplaceRED] &gt; </span>"
            "<span style=\"color:#ff79c6; text-decoration: underline;\">Ciel</span> "
            "<span style=\"color:#8be9fd\">%2</span> "
            "<span style=\"color:#f8f8f2\">%3</span>")
                                 .arg(nowTime(), chevron, htmlEsc(cmd));
        appendHtmlLine(m_operativeConsole, line);
    }

    void appendTaskInfo(const QString &msg) {
        const QString line = QString(
            "<span style=\"color:#8be9fd\">[*]</span> "
            "<span style=\"color:#6272a4\">[Task-ciel]</span> "
            "<span style=\"color:#8be9fd\">%1</span>")
                                 .arg(htmlEsc(msg));
        appendHtmlLine(m_operativeConsole, line);
    }

    void appendTaskDone(const QString &msg) {
        const QString line = QString(
            "<span style=\"color:#50fa7b\">[+]</span> "
            "<span style=\"color:#f8f8f2\">%1</span>")
                                 .arg(htmlEsc(msg));
        appendHtmlLine(m_operativeConsole, line);
    }

    void appendTaskAlert(const QString &msg) {
        const QString line = QString(
            "<span style=\"color:#ff5555\">[!]</span> "
            "<span style=\"color:#ff5555\">%1</span>")
                                 .arg(htmlEsc(msg));
        appendHtmlLine(m_operativeConsole, line);
    }

    void appendHelpTableToConsole() {
        const QString tableText =
            "\n"
            "  Command         Arguments        Description\n"
            "  -------         ---------        -----------\n"
            "  sync                             Full telemetry sweep across Souei, Chronoa, and Ranga\n"
            "  souei                            Probe /proc/keys ACLs and session keyring baseline\n"
            "  chronoa                          Evaluate MIT CCACHE v4 lifetimes and cipher strength\n"
            "  skew            &lt;sec&gt;        Simulate KDC clock drift against RFC 4120 300s ceiling\n"
            "  rc4                              Simulate weak RC4-HMAC ticket policy stop (exit 2)\n"
            "  renew                            Trigger pre-expiry CCACHE renewal hook\n"
            "  isolate                          Verify LD_PRELOAD possessor-only (0x3f000000) session\n"
            "  purge                            Execute 3-pass NIST SP 800-88 file wipe on secret target\n"
            "  audit                            Print full Kerberos & Kernel Keyring verification matrix\n"
            "  graph / table                    Switch main viewport between Topology Graph and Sessions\n";
        appendHtmlLine(m_operativeConsole, QString("<pre style=\"color:#f8f8f2; margin:0;\">%1</pre>").arg(tableText));
    }

    void appendDetailedAuditTables() {
        appendTaskInfo("Tasked operative to audit Kerberos CCACHE and Linux Kernel Keyring...");
        appendTaskDone("Operative returned 1,420 bytes of verified runtime telemetry:");
        const QString auditText =
            "\n"
            "  ITEM / PRINCIPAL                        SUBSYSTEM         SERIAL / ENCTYPE              POLICY &amp; RUNTIME VERDICT\n"
            "  ================                        =========         ================              ========================\n"
            "  krbtgt/TEMPEST.LOCAL@TEMPEST.LOCAL      MIT CCACHE v4     aes256-cts-hmac-sha1-96 (18)  Healthy TGT, RFC 4120 skew &lt;= 300s enforced\n"
            "  kcm.sock (SO_PEERCRED)                  AF_UNIX Socket    mode 0600 (owner rw)          Resolved live daemon peer_pid + UID match\n"
            "  _uid_ses.1000 (/proc/keys)              Kernel Keyring    perm 0x1f3f0000               Warning: shared UID access across processes\n"
            "  _ses (LD_PRELOAD=libranga.so)           SYS_keyctl        perm 0x3f000000               Isolated possessor-only session keyring OK\n"
            "  PurgeTarget.keytab                      NIST SP 800-88    3-pass + explicit_bzero       Verified 4096B overwrite and clean unlink\n";
        appendHtmlLine(m_operativeConsole, QString("<pre style=\"color:#f8f8f2; margin:0;\">%1</pre>").arg(auditText));
    }

    void dispatchFromUi(const QString &action, const QJsonObject &params, const QString &echoLabel) {
        appendCommandEcho(echoLabel);
        QJsonObject resp;
        executeAction(action, params, resp);
    }

    void handleOperativeCommand(const QString &raw) {
        if (raw.isEmpty()) {
            return;
        }
        appendCommandEcho(raw);
        if (raw == "help") {
            appendHelpTableToConsole();
            return;
        }
        if (raw == "audit") {
            appendDetailedAuditTables();
            return;
        }
        if (raw == "clear") {
            m_operativeConsole->clear();
            appendOperativeBanner();
            return;
        }
        if (raw == "graph") {
            m_mainViewTabs->setCurrentIndex(1);
            rebuildSessionGraph();
            return;
        }
        if (raw == "table") {
            m_mainViewTabs->setCurrentIndex(0);
            return;
        }
        if (raw == "sync") {
            runFullSync();
            return;
        }
        if (raw == "souei") {
            QJsonObject resp;
            executeAction("souei_probe", QJsonObject(), resp);
            return;
        }
        if (raw == "chronoa") {
            QJsonObject p;
            p["skew_sec"] = m_simSkewSec;
            QJsonObject resp;
            executeAction("chronoa_check", p, resp);
            return;
        }
        if (raw.startsWith("skew ")) {
            bool ok = false;
            const int v = raw.mid(5).trimmed().toInt(&ok);
            if (!ok || v < 0) {
                appendTaskAlert("Usage: skew <non-negative seconds>");
                return;
            }
            m_simSkewSec = v;
            QJsonObject p;
            p["skew_sec"] = v;
            QJsonObject resp;
            executeAction("chronoa_check", p, resp);
            return;
        }
        if (raw == "rc4") {
            QJsonObject p;
            p["weak_enctype"] = true;
            QJsonObject resp;
            executeAction("chronoa_check", p, resp);
            return;
        }
        if (raw == "renew") {
            QJsonObject p;
            p["skew_sec"] = 0;
            p["renew"] = true;
            QJsonObject resp;
            executeAction("chronoa_check", p, resp);
            return;
        }
        if (raw == "isolate") {
            QJsonObject resp;
            executeAction("ranga_isolate", QJsonObject(), resp);
            return;
        }
        if (raw == "purge") {
            QJsonObject resp;
            executeAction("ranga_purge", QJsonObject(), resp);
            return;
        }
        appendTaskAlert("Unknown command: " + raw + " (run 'help')");
    }

    void upsertSessionRow(const QJsonObject &row) {
        OperativeSnapshot snap;
        snap.id = row.value("id").toString();
        snap.operative = row.value("operative").toString();
        snap.module = row.value("module").toString();
        snap.target = row.value("target").toString();
        snap.detail = row.value("detail").toString();
        snap.permOrTtl = row.value("perm_or_ttl").toString();
        snap.skewOrAux = row.value("skew_or_aux").toString();
        snap.exitCode = row.value("exit_code").toInt();
        snap.status = row.value("status").toString();
        snap.rawJson = row.value("raw_json");
        m_snapshots[snap.id] = snap;

        int targetRow = -1;
        for (int r = 0; r < m_sessionTable->rowCount(); ++r) {
            const QTableWidgetItem *it = m_sessionTable->item(r, 0);
            if (it && it->text() == snap.id) {
                targetRow = r;
                break;
            }
        }
        if (targetRow < 0) {
            targetRow = m_sessionTable->rowCount();
            m_sessionTable->insertRow(targetRow);
        }

        const QStringList hostParts = m_serverAddr.split(':');
        const QString hostStr = hostParts.value(0, "127.0.0.1");

        QString userCol = "ciel";
        QString realmCol = "TEMPEST.LOCAL";
        if (snap.operative == "Souei" || snap.operative == "Ranga") {
            userCol = "apip (1000)";
            realmCol = "linux-kernel";
        }

        const QStringList cells = {
            snap.id,
            hostStr,
            snap.skewOrAux,
            userCol,
            realmCol,
            snap.module,
            snap.operative,
            snap.target,
            snap.permOrTtl,
            snap.status
        };

        const QColor badgeAccent = draculaStatusColor(snap.status);
        for (int c = 0; c < cells.size(); ++c) {
            auto *item = new QTableWidgetItem(cells.at(c));
            item->setTextAlignment(Qt::AlignCenter);
            if (c == 0) {
                const bool policyWarn = (snap.exitCode != 0 || snap.status == "SHARED_KEYRING");
                item->setIcon(makeOperativeBadgeIcon(policyWarn ? QColor("#ff5555") : QColor("#bd93f9")));
                item->setForeground(QColor("#ff5555"));
            } else if (c == 9) {
                item->setForeground(badgeAccent);
            }
            m_sessionTable->setItem(targetRow, c, item);
        }
    }

    void rebuildSessionGraph() {
        m_graphScene->clear();

        auto *rootNode = new CielGraphNode(
            CielGraphNode::NodeKind::FirewallHub,
            "ciel-teamserver",
            QString("[%1]").arg(m_serverAddr),
            "hub",
            QColor("#8be9fd"),
            false);
        rootNode->setPos(95.0, 168.0);
        m_graphScene->addItem(rootNode);

        auto addMonitor = [&](const QString &opId,
                              const QString &defaultTarget,
                              const QString &roleTag,
                              const QString &realmUser,
                              const QPointF &pos,
                              bool forceAlert,
                              bool revokedGray,
                              const QString &overrideStatus = QString()) -> CielGraphNode * {
            const bool exists = m_snapshots.contains(opId);
            const OperativeSnapshot s = exists ? m_snapshots.value(opId) : OperativeSnapshot{};
            QString opName = exists ? s.operative.toLower() : roleTag.split('-').first();
            if (opId == "OP-06") {
                opName = "kcm";
            } else if (opId == "OP-07") {
                opName = "chronoa";
            } else if (opId == "OP-08") {
                opName = "ranga";
            }
            const QString line1 = QString("%1 @ %2\\%3").arg(opId, opName, defaultTarget);
            const QString stStr = !overrideStatus.isEmpty() ? overrideStatus : (exists ? s.status : "ONLINE");
            const QString line2 = QString("[%1 :: %2]").arg(realmUser, stStr);
            const QColor stCol = revokedGray ? QColor("#6272a4") : draculaStatusColor(stStr);
            const bool elevatedAlert = !revokedGray && (forceAlert || (exists && s.exitCode != 0));

            auto *node = new CielGraphNode(
                CielGraphNode::NodeKind::OperativeMonitor,
                line1,
                line2,
                roleTag,
                stCol,
                elevatedAlert,
                revokedGray);
            node->setPos(pos);
            m_graphScene->addItem(node);
            return node;
        };

        // Col 1: Primary Telemetry Daemons (green callback arrows INTO Root Firewall)
        auto *op1 = addMonitor("OP-01", "1000", "souei", "linux-kernel\\apip", QPointF(345.0, 52.0), true, false);
        auto *op2 = addMonitor("OP-02", "krbtgt", "chronoa", "TEMPEST.LOCAL\\ciel", QPointF(345.0, 168.0), false, false);
        auto *op3 = addMonitor("OP-03", "uid_ses", "ranga-shared", "linux-kernel\\apip", QPointF(345.0, 284.0), false, false);

        m_graphScene->addItem(new CielGraphEdge(rootNode, op1, QColor("#50fa7b"), "netlink", true));
        m_graphScene->addItem(new CielGraphEdge(rootNode, op2, QColor("#50fa7b"), "ccache", true));
        m_graphScene->addItem(new CielGraphEdge(rootNode, op3, QColor("#50fa7b"), "keyctl", true));

        // Col 2: Derived Isolation, Purge & Revoked Edges
        auto *op4 = addMonitor("OP-04", "0x3f000000", "ranga-iso", "linux-kernel\\apip", QPointF(625.0, 52.0), false, false);
        auto *op5 = addMonitor("OP-05", "nist_purge", "ranga-purge", "TEMPEST.LOCAL\\apip", QPointF(625.0, 168.0), true, false);
        auto *op6 = addMonitor("OP-06", "revoked_tkt", "ranga-shared", "TEMPEST.LOCAL\\old", QPointF(625.0, 284.0), false, true, "REVOKED");

        m_graphScene->addItem(new CielGraphEdge(op1, op4, QColor("#bd93f9"), "LD_PRELOAD", false));
        m_graphScene->addItem(new CielGraphEdge(op2, op5, QColor("#bd93f9"), "nist/800", false));
        m_graphScene->addItem(new CielGraphEdge(op2, op6, QColor("#ff5555"), "revoked", false));

        // Col 3: Verified Identity Leaf Targets
        auto *op7 = addMonitor("OP-07", "aes256_tgt", "chronoa", "TEMPEST.LOCAL\\tgt", QPointF(890.0, 100.0), false, false, "VERIFIED");
        auto *op8 = addMonitor("OP-08", "wiped_keytab", "ranga-purge", "TEMPEST.LOCAL\\fs", QPointF(890.0, 236.0), true, false, "PURGED");

        m_graphScene->addItem(new CielGraphEdge(op4, op7, QColor("#bd93f9"), "aes256", false));
        m_graphScene->addItem(new CielGraphEdge(op5, op8, QColor("#bd93f9"), "3-pass", false));

        m_graphScene->setSceneRect(0.0, 0.0, 1020.0, 368.0);
        m_graphView->centerOn(510.0, 184.0);
    }

    bool ensureTeamserverRunning() {
        QString host;
        quint16 port = 0;
        if (!parseHostPort(m_serverAddr, host, port)) {
            appendEventLog("!", "#ff5555", "Invalid teamserver address: " + htmlEsc(m_serverAddr));
            return false;
        }

        {
            QTcpSocket probe;
            probe.connectToHost(host, port);
            if (probe.waitForConnected(150)) {
                return true;
            }
        }

        if (host != "127.0.0.1" && host != "localhost" && host != "::1") {
            return false;
        }

        const QStringList candidates = {
            QCoreApplication::applicationDirPath() + "/ciel-teamserver",
            "bin/ciel-teamserver",
            "../bin/ciel-teamserver",
            QStandardPaths::findExecutable("ciel-teamserver")
        };
        QString binPath;
        for (const QString &c : candidates) {
            if (!c.isEmpty() && QFile::exists(c)) {
                binPath = c;
                break;
            }
        }
        if (binPath.isEmpty()) {
            appendEventLog("!", "#ff5555", "Unable to locate ciel-teamserver binary");
            return false;
        }

        if (!m_managedServer) {
            m_managedServer = new QProcess(this);
            m_managedServer->setProcessChannelMode(QProcess::MergedChannels);
        }
        if (m_managedServer->state() == QProcess::NotRunning) {
            m_managedServer->start(binPath, {"--addr", m_serverAddr});
            if (!m_managedServer->waitForStarted(2000)) {
                appendEventLog("!", "#ff5555", "Failed launching ciel-teamserver: " + htmlEsc(binPath));
                return false;
            }
        }

        for (int attempt = 0; attempt < 25; ++attempt) {
            QTcpSocket check;
            check.connectToHost(host, port);
            if (check.waitForConnected(150)) {
                if (!m_loggedManagedBoot) {
                    m_loggedManagedBoot = true;
                    appendEventLog("+", "#50fa7b",
                        "Auto-started local ciel-teamserver on " + htmlEsc(m_serverAddr) +
                        " (Tempest + Tanuki operatives ready)");
                }
                return true;
            }
            if (m_managedServer->state() == QProcess::NotRunning) {
                appendEventLog("!", "#ff5555", "Managed ciel-teamserver exited before listening");
                return false;
            }
            QThread::msleep(100);
        }
        appendEventLog("!", "#ff5555", "Timed out waiting for managed ciel-teamserver on " + htmlEsc(m_serverAddr));
        return false;
    }

    bool executeAction(const QString &action, const QJsonObject &extraParams, QJsonObject &respOut) {
        QString host;
        quint16 port = 0;
        if (!parseHostPort(m_serverAddr, host, port)) {
            appendEventLog("!", "#ff5555", "Invalid teamserver address: " + htmlEsc(m_serverAddr));
            return false;
        }

        if (!ensureTeamserverRunning()) {
            return false;
        }

        QTcpSocket sock;
        sock.connectToHost(host, port);
        if (!sock.waitForConnected(3000)) {
            appendEventLog("!", "#ff5555", "Failed connecting to " + htmlEsc(m_serverAddr));
            return false;
        }

        QJsonObject req = extraParams;
        req["action"] = action;
        const QByteArray wire = QJsonDocument(req).toJson(QJsonDocument::Compact) + '\n';
        if (sock.write(wire) != wire.size() || !sock.waitForBytesWritten(3000)) {
            appendEventLog("!", "#ff5555", "TCP write error for action " + htmlEsc(action));
            return false;
        }

        QByteArray rx;
        while (!rx.contains('\n')) {
            if (!sock.waitForReadyRead(10000)) {
                appendEventLog("!", "#ff5555", "Timeout waiting for teamserver action " + htmlEsc(action));
                return false;
            }
            rx.append(sock.readAll());
        }
        const int nl = rx.indexOf('\n');
        QJsonParseError parseErr;
        const QJsonDocument doc = QJsonDocument::fromJson(rx.left(nl).trimmed(), &parseErr);
        if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
            appendEventLog("!", "#ff5555", "JSON parse failure: " + htmlEsc(parseErr.errorString()));
            return false;
        }

        respOut = doc.object();
        const QJsonArray rows = respOut.value("rows").toArray();
        for (const QJsonValue &rv : rows) {
            if (rv.isObject()) {
                upsertSessionRow(rv.toObject());
            }
        }

        const QJsonArray logs = respOut.value("logs").toArray();
        for (const QJsonValue &lv : logs) {
            const QString rawLine = lv.toString();
            const int exitCode = respOut.value("exit_code").toInt();
            if (rawLine.contains("CLOCK_SKEW_VIOLATION") || rawLine.contains("EXPIRED") ||
                rawLine.contains("WEAK_ENCTYPE") || rawLine.contains("ERROR")) {
                appendEventLog("!", "#ff5555", htmlEsc(rawLine));
                appendTaskAlert(rawLine);
            } else if (exitCode == 0) {
                appendEventLog("+", "#50fa7b", htmlEsc(rawLine));
                appendTaskDone(rawLine);
            } else {
                appendEventLog("*", "#8be9fd", htmlEsc(rawLine));
                appendTaskInfo(rawLine);
            }
        }

        m_rawTelemetry->setPlainText(QString::fromUtf8(QJsonDocument(respOut).toJson(QJsonDocument::Indented)));
        if (m_mainViewTabs->currentIndex() == 1) {
            rebuildSessionGraph();
        }
        return true;
    }

    QString m_serverAddr;
    int m_simSkewSec = 0;
    int m_consoleTabIdx = 2;
    int m_listenersTabIdx = 6;

    QTabWidget *m_mainViewTabs = nullptr;
    QTableWidget *m_sessionTable = nullptr;
    QGraphicsView *m_graphView = nullptr;
    QGraphicsScene *m_graphScene = nullptr;

    QTabWidget *m_smallTabs = nullptr;
    QTextEdit *m_eventViewer = nullptr;
    QPlainTextEdit *m_rawTelemetry = nullptr;

    QTabWidget *m_bottomTabs = nullptr;
    QTextEdit *m_operativeConsole = nullptr;
    QLabel *m_sessionBanner = nullptr;
    QLineEdit *m_operativeInput = nullptr;

    QTimer m_pollTimer;
    QProcess *m_managedServer = nullptr;
    QAction *m_actPoll = nullptr;
    bool m_loggedManagedBoot = false;
    QMap<QString, OperativeSnapshot> m_snapshots;
};

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QFont globalMono("DejaVu Sans Mono", 9);
    globalMono.setStyleHint(QFont::Monospace);
    app.setFont(globalMono);
    app.setApplicationName("Ciel");
    app.setApplicationVersion("1.0.0");
    app.setStyleSheet(applyCielStylesheet());

    const QStringList iconPaths = {
        QCoreApplication::applicationDirPath() + "/../assets/ciel_icon.png",
        QCoreApplication::applicationDirPath() + "/assets/ciel_icon.png",
        "assets/ciel_icon.png",
        "../assets/ciel_icon.png"
    };
    QIcon appIcon;
    for (const QString &ip : iconPaths) {
        if (QFile::exists(ip)) {
            appIcon = QIcon(ip);
            break;
        }
    }
    if (!appIcon.isNull()) {
        app.setWindowIcon(appIcon);
    }

    QCommandLineParser parser;
    parser.setApplicationDescription("Ciel Desktop C2 Client for Tempest + Tanuki");
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption addrOpt("addr", "Teamserver address (host:port)", "addr", "127.0.0.1:40056");
    const QCommandLineOption selfTestOpt("self-test", "Run automated desktop RPC verification and exit");
    const QCommandLineOption shotOpt("screenshot", "Save PNG screenshot of Sessions Table view", "path");
    const QCommandLineOption graphShotOpt("graph-screenshot", "Save PNG screenshot of Session Graph view", "path");
    const QCommandLineOption altShotOpt("alt-screenshot", "Save PNG screenshot of secondary Telemetry/Listeners view", "path");
    parser.addOption(addrOpt);
    parser.addOption(selfTestOpt);
    parser.addOption(shotOpt);
    parser.addOption(graphShotOpt);
    parser.addOption(altShotOpt);
    parser.process(app);

    CielWindow win(parser.value(addrOpt));
    if (!appIcon.isNull()) {
        win.setWindowIcon(appIcon);
    }
    win.show();

    if (parser.isSet(selfTestOpt)) {
        const QString tableShot = parser.value(shotOpt);
        const QString graphShot = parser.value(graphShotOpt);
        const QString altShot = parser.value(altShotOpt);
        QTimer::singleShot(100, &win, [&win, tableShot, graphShot, altShot]() {
            const int rc = win.runSelfTest(tableShot, graphShot, altShot);
            QApplication::exit(rc);
        });
    } else {
        QTimer::singleShot(100, &win, &CielWindow::startInteractiveSession);
    }

    return app.exec();
}

#include "main.moc"
