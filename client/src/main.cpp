#include <QAction>
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCompleter>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
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
#include <QInputDialog>
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
#include <QPointer>
#include <QProcess>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QScrollBar>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStyleOptionGraphicsItem>
#include <QSysInfo>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QWidget>

#include <cmath>
#include <functional>
#include <numbers>
#ifndef Q_OS_WIN
#include <unistd.h>
#endif

namespace {

QString applyCielStylesheet() {
    return QStringLiteral(R"(
        QMainWindow, QWidget, QDialog, QMessageBox, QInputDialog {
            background-color: #313342;
            color: #f8f8f2;
        }

        QWidget#ListenersPanel, QWidget#ConsolePanel {
            background-color: #282a36;
            color: #f8f8f2;
        }

        QLabel {
            background-color: transparent;
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
            padding: 4px 14px;
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

        QLineEdit {
            background-color: #1e1f29;
            color: #f8f8f2;
            border: 1px solid #44475a;
            border-radius: 3px;
            padding: 3px 6px;
        }
        QLineEdit:focus {
            border: 1px solid #bd93f9;
        }

        QLineEdit#OperativeInput {
            background-color: #282a36;
            color: #f8f8f2;
            border: none;
            border-radius: 0px;
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

        QLabel#SectionLabel {
            color: #8be9fd;
            font-weight: bold;
            padding-top: 2px;
        }

        QPushButton {
            background-color: #bd93f9;
            color: #1e1f29;
            border: 1px solid #bd93f9;
            border-radius: 3px;
            padding: 4px 10px;
            font-weight: bold;
            min-height: 22px;
        }
        QPushButton:hover {
            background-color: #caa9fa;
            border-color: #caa9fa;
            color: #1e1f29;
        }
        QPushButton:pressed {
            background-color: #6272a4;
            color: #f8f8f2;
            border-color: #f8f8f2;
        }
        QPushButton:disabled {
            background-color: #383a4a;
            color: #6272a4;
            border-color: #44475a;
        }

        QPushButton#SecondaryBtn {
            background-color: #44475a;
            color: #f8f8f2;
            border: 1px solid #6272a4;
            padding: 3px 8px;
            font-weight: normal;
        }
        QPushButton#SecondaryBtn:hover {
            background-color: #6272a4;
            color: #f8f8f2;
        }
        QPushButton#SecondaryBtn:pressed {
            background-color: #282a36;
            color: #f8f8f2;
        }

        QPushButton#DangerBtn {
            background-color: #ff5555;
            color: #1e1f29;
            border: 1px solid #ff5555;
        }
        QPushButton#DangerBtn:hover {
            background-color: #ff6e6e;
            border-color: #ff6e6e;
            color: #1e1f29;
        }
        QPushButton#DangerBtn:pressed {
            background-color: #a23e3e;
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
        status == "READY" || status == "ONLINE" || status == "VERIFIED" ||
        status == "SUCCESS" || status == "PASS" || status == "PARSED" ||
        status == "MATCHED" || status == "5_RUNGS" || status == "NONE_DISCOVERED" ||
        status == "CLEAN" || status == "DRY_RUN" || status == "LOW" ||
        status == "EXTRACTED") {
        return QColor("#50fa7b");
    }
    if (status == "REVOKED") {
        return QColor("#9aa5ce");
    }
    if (status == "EXPIRING_SOON" || status == "SHARED_KEYRING" ||
        status == "ATTENTION" || status == "WARN" || status == "MEDIUM" ||
        status == "MODERATE" || status == "FINDINGS" || status == "HIGH" ||
        status == "MISSING" || status == "NO_CREDENTIALS" || status == "RESOURCE_MISSING" ||
        status == "N_A" || status == "PENDING") {
        return QColor("#f1fa8c");
    }
    return QColor("#ff5555");
}

struct BoolScopeGuard {
    bool &flag;
    explicit BoolScopeGuard(bool &f) : flag(f) { flag = true; }
    ~BoolScopeGuard() { flag = false; }
};

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

QStringList splitCommandLineArgs(const QString &line) {
    QStringList out;
    QString cur;
    bool inSingle = false;
    bool inDouble = false;
    bool escaped = false;
    bool hasToken = false;

    for (int i = 0; i < line.size(); ++i) {
        const QChar ch = line.at(i);
        if (escaped) {
            cur.append(ch);
            escaped = false;
            hasToken = true;
            continue;
        }
        if (ch == '\\' && !inSingle) {
            escaped = true;
            hasToken = true;
            continue;
        }
        if (ch == '\'' && !inDouble) {
            inSingle = !inSingle;
            hasToken = true;
            continue;
        }
        if (ch == '"' && !inSingle) {
            inDouble = !inDouble;
            hasToken = true;
            continue;
        }
        if (ch.isSpace() && !inSingle && !inDouble) {
            if (hasToken) {
                out.append(cur);
                cur.clear();
                hasToken = false;
            }
            continue;
        }
        cur.append(ch);
        hasToken = true;
    }
    if (hasToken) {
        out.append(cur);
    }
    return out;
}

QString generateSecureHexToken() {
    QByteArray raw(32, '\0');
    auto *rng = QRandomGenerator::system();
    for (int i = 0; i < 32; i += 4) {
        const quint32 v = rng->generate();
        raw[i] = static_cast<char>((v >> 24) & 0xff);
        raw[i + 1] = static_cast<char>((v >> 16) & 0xff);
        raw[i + 2] = static_cast<char>((v >> 8) & 0xff);
        raw[i + 3] = static_cast<char>(v & 0xff);
    }
    return QString::fromLatin1(raw.toHex());
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
                  const QString &opId,
                  const QString &line1,
                  const QString &line2,
                  const QString &roleTag,
                  const QColor &statusColor,
                  bool elevatedAlert,
                  bool mutedState = false)
        : m_kind(kind),
          m_opId(opId),
          m_line1(line1),
          m_line2(line2),
          m_roleTag(roleTag),
          m_statusColor(statusColor),
          m_elevatedAlert(elevatedAlert),
          m_mutedState(mutedState) {
        setFlag(ItemIsMovable);
        setFlag(ItemIsSelectable);
        setFlag(ItemSendsGeometryChanges);
        setCacheMode(DeviceCoordinateCache);
        setZValue(1.0);

        const QFont f1("DejaVu Sans Mono", 10, QFont::Bold);
        const QFont f2("DejaVu Sans Mono", 9, QFont::Bold);
        const QFontMetrics fm1(f1);
        const QFontMetrics fm2(f2);
        const qreal textW = qMax(fm1.horizontalAdvance(m_line1), fm2.horizontalAdvance(m_line2));
        m_cardW = qBound(156.0, textW + 24.0, 440.0);
    }

    QString opId() const {
        return m_opId;
    }

    qreal cardWidth() const {
        return m_cardW;
    }

    QPointF edgeAnchorOffset(qreal ux, qreal uy) const {
        const qreal halfIconW = 46.0;
        const qreal topIcon = -48.0;
        const qreal botIcon = 42.0;
        qreal tIcon = 1e9;
        if (std::abs(ux) > 1e-5) {
            tIcon = qMin(tIcon, halfIconW / std::abs(ux));
        }
        if (uy > 1e-5) {
            tIcon = qMin(tIcon, botIcon / uy);
        } else if (uy < -1e-5) {
            tIcon = qMin(tIcon, topIcon / uy);
        }
        if (uy > 0.65) {
            const qreal pillHalfW = m_cardW * 0.5 + 6.0;
            const qreal pillBot = 82.0;
            const qreal ax = std::abs(ux);
            qreal tPill = pillBot / uy;
            if (ax > 1e-5) {
                tPill = qMin(tPill, pillHalfW / ax);
            }
            tIcon = qMax(tIcon, tPill);
        }
        return QPointF(ux * tIcon, uy * tIcon);
    }

    void addEdge(CielGraphEdge *edge) {
        m_edges.append(edge);
    }

    QRectF boundingRect() const override {
        const qreal halfW = qMax(50.0, m_cardW * 0.5 + 8.0);
        return QRectF(-halfW, -52.0, halfW * 2.0, 138.0);
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

        QFont f1("DejaVu Sans Mono", 10, QFont::Bold);
        QFont f2("DejaVu Sans Mono", 9, QFont::Bold);
        const QFontMetrics fm2(f2);
        const qreal cardW = m_cardW;

        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(14, 15, 22, 110));
        painter->drawRoundedRect(QRectF(-cardW * 0.5 + 2.0, 43.0, cardW, 36.0), 5.0, 5.0);

        painter->setPen(QPen(QColor(68, 71, 90, 125), 1.0));
        painter->setBrush(QColor(26, 28, 38, 212));
        painter->drawRoundedRect(QRectF(-cardW * 0.5, 41.0, cardW, 36.0), 5.0, 5.0);

        painter->setFont(f1);
        painter->setPen(QColor("#f8f8f2"));
        painter->drawText(QRectF(-cardW * 0.5 + 4.0, 43.0, cardW - 8.0, 17.0), Qt::AlignCenter, m_line1);

        painter->setFont(f2);
        const int sepIdx = m_line2.indexOf(" :: ");
        if (m_kind != NodeKind::FirewallHub && sepIdx > 0 && m_line2.endsWith(']')) {
            const QString prefix = m_line2.left(sepIdx + 4);
            const QString statusPart = m_line2.mid(sepIdx + 4, m_line2.size() - (sepIdx + 4) - 1);
            const QString suffix = "]";
            const qreal w1 = fm2.horizontalAdvance(prefix);
            const qreal w2 = fm2.horizontalAdvance(statusPart);
            const qreal w3 = fm2.horizontalAdvance(suffix);
            const qreal totalW = w1 + w2 + w3;
            const qreal startX = -totalW * 0.5;
            const qreal yBase = 60.0 + fm2.ascent() + 1.0;

            painter->setPen(QColor("#f8f8f2"));
            painter->drawText(QPointF(startX, yBase), prefix);
            painter->setPen(m_statusColor);
            painter->drawText(QPointF(startX + w1, yBase), statusPart);
            painter->setPen(QColor("#f8f8f2"));
            painter->drawText(QPointF(startX + w1 + w2, yBase), suffix);
        } else {
            painter->setPen(m_kind == NodeKind::FirewallHub ? QColor("#8be9fd") : m_statusColor);
            painter->drawText(QRectF(-cardW * 0.5 + 4.0, 60.0, cardW - 8.0, 16.0), Qt::AlignCenter, m_line2);
        }

        if (option && (option->state & QStyle::State_Selected)) {
            QPen selPen(QColor("#8be9fd"), 1.5, Qt::DotLine);
            painter->setPen(selPen);
            painter->setBrush(Qt::NoBrush);
            painter->drawRoundedRect(QRectF(-48.0, -44.0, 96.0, 86.0), 6.0, 6.0);
        }
    }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    void paintFirewallIcon(QPainter *p) const {
        p->setPen(Qt::NoPen);
        p->setBrush(QColor(12, 13, 20, 120));
        p->drawRoundedRect(QRectF(-32.0, -4.0, 70.0, 46.0), 4.0, 4.0);

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
                    p->setBrush(col == 1 ? QColor("#b84343") : QColor("#963539"));
                    p->drawRoundedRect(QRectF(wallLeft + col * (w + gap), y, w, brickH), 1.8, 1.8);
                }
            } else {
                const double edgeW = 11.0;
                const double midW = (wallW - 2.0 * edgeW - 3.0 * gap) / 2.0;
                p->setBrush(QColor("#963539"));
                p->drawRoundedRect(QRectF(wallLeft, y, edgeW, brickH), 1.8, 1.8);
                p->setBrush(QColor("#b84343"));
                p->drawRoundedRect(QRectF(wallLeft + edgeW + gap, y, midW, brickH), 1.8, 1.8);
                p->drawRoundedRect(QRectF(wallLeft + edgeW + 2.0 * gap + midW, y, midW, brickH), 1.8, 1.8);
                p->setBrush(QColor("#963539"));
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
        p->setPen(QPen(QColor(139, 233, 253, 125), 4.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p->drawPath(path);
        p->setPen(QPen(QColor("#ffffff"), 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p->drawPath(path);
    }

    void paintMonitorIcon(QPainter *p) const {
        if (m_mutedState) {
            p->setOpacity(0.76);
        }
        p->setPen(Qt::NoPen);
        p->setBrush(QColor(12, 13, 20, 115));
        p->drawRoundedRect(QRectF(-33.0, -33.0, 72.0, 58.0), 5.0, 5.0);
        p->drawRoundedRect(QRectF(-20.0, 35.0, 45.0, 6.0), 2.5, 2.5);

        const QColor bezelTop = m_mutedState
                                    ? QColor("#3d4050")
                                    : (m_elevatedAlert ? QColor("#5e1f26") : QColor("#2c313e"));
        const QColor chinStand = m_mutedState
                                     ? QColor("#73788f")
                                     : (m_elevatedAlert ? QColor("#e06c75") : QColor("#c0c6d2"));

        if (m_elevatedAlert) {
            p->setPen(QPen(QColor(255, 85, 85, 110), 2.4));
            p->setBrush(Qt::NoBrush);
            p->drawRoundedRect(QRectF(-37.0, -38.0, 74.0, 60.0), 5.5, 5.5);
        }

        QPainterPath stand;
        stand.moveTo(-8.0, 20.0);
        stand.lineTo(8.0, 20.0);
        stand.lineTo(13.0, 34.0);
        stand.lineTo(-13.0, 34.0);
        stand.closeSubpath();
        p->setPen(QPen(QColor("#1e2029"), 1.1));
        p->setBrush(chinStand.darker(115));
        p->drawPath(stand);
        p->setBrush(chinStand);
        p->drawRoundedRect(QRectF(-22.0, 33.0, 44.0, 6.0), 2.2, 2.2);

        p->setPen(QPen(QColor("#181a24"), 1.4));
        p->setBrush(bezelTop);
        p->drawRoundedRect(QRectF(-35.0, -36.0, 70.0, 56.0), 4.5, 4.5);

        p->setPen(QPen(QColor(255, 255, 255, 28), 1.0));
        p->drawLine(QPointF(-31.0, -34.5), QPointF(31.0, -34.5));

        p->setPen(Qt::NoPen);
        p->setBrush(chinStand);
        p->drawRoundedRect(QRectF(-34.0, 10.0, 68.0, 10.0), 2.5, 2.5);
        p->drawRect(QRectF(-34.0, 10.0, 68.0, 5.0));
        p->setBrush(m_statusColor);
        p->drawEllipse(QRectF(-2.5, 13.0, 5.0, 5.0));

        QColor scrTop("#1462cc");
        QColor scrBot("#0a3f8c");
        if (m_mutedState) {
            scrTop = QColor("#4d5164");
            scrBot = QColor("#383b4a");
        } else if (m_elevatedAlert) {
            scrTop = QColor("#8f2430");
            scrBot = QColor("#5c161e");
        } else if (m_roleTag == "souei") {
            scrTop = QColor("#c68a14");
            scrBot = QColor("#8c5e09");
        } else if (m_roleTag == "ranga-iso") {
            scrTop = QColor("#1f8554");
            scrBot = QColor("#135937");
        } else if (m_roleTag == "ranga-shared") {
            scrTop = QColor("#8e6c24");
            scrBot = QColor("#614915");
        } else if (m_roleTag == "tanuki") {
            scrTop = QColor("#1b62bd");
            scrBot = QColor("#0f3f7d");
        }
        QLinearGradient scrGrad(0.0, -31.0, 0.0, 6.0);
        scrGrad.setColorAt(0.0, scrTop);
        scrGrad.setColorAt(1.0, scrBot);
        p->setPen(QPen(QColor("#161822"), 1.0));
        p->setBrush(scrGrad);
        p->drawRoundedRect(QRectF(-30.0, -31.0, 60.0, 37.0), 1.5, 1.5);

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
            p->setBrush(scrBot);
            p->drawEllipse(QRectF(-2.0, -11.0, 4.0, 4.0));
        } else if (m_roleTag == "tanuki") {
            p->setPen(QPen(QColor("#f8f8f2"), 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p->setBrush(Qt::NoBrush);
            QPainterPath shield;
            shield.moveTo(0.0, -24.0);
            shield.lineTo(11.0, -19.0);
            shield.lineTo(10.0, -9.0);
            shield.quadTo(0.0, -1.0, 0.0, -1.0);
            shield.quadTo(0.0, -1.0, -10.0, -9.0);
            shield.lineTo(-11.0, -19.0);
            shield.closeSubpath();
            p->drawPath(shield);
            p->drawEllipse(QPointF(0.0, -13.0), 3.2, 3.2);
        } else {
            p->setPen(QPen(QColor("#f8f8f2"), 2.0));
            p->setBrush(Qt::NoBrush);
            p->drawEllipse(QRectF(-13.0, -20.0, 14.0, 14.0));
            p->drawEllipse(QRectF(-1.0, -16.0, 14.0, 14.0));
        }

        if (m_elevatedAlert) {
            paintLightningArc(p, {QPointF(-40.0, -32.0), QPointF(-30.0, -36.0), QPointF(-33.0, -25.0), QPointF(-23.0, -28.0)});
            paintLightningArc(p, {QPointF(-41.0, 4.0), QPointF(-31.0, 1.0), QPointF(-36.0, 12.0), QPointF(-24.0, 9.0)});
            paintLightningArc(p, {QPointF(26.0, -26.0), QPointF(38.0, -21.0), QPointF(31.0, -12.0), QPointF(42.0, -8.0)});
        }
    }

    NodeKind m_kind;
    QString m_opId;
    QString m_line1;
    QString m_line2;
    QString m_roleTag;
    QColor m_statusColor;
    bool m_elevatedAlert = false;
    bool m_mutedState = false;
    qreal m_cardW = 156.0;
    QList<CielGraphEdge *> m_edges;
};

class CielGraphEdge : public QGraphicsItem {
public:
    CielGraphEdge(CielGraphNode *source,
                  CielGraphNode *dest,
                  const QColor &color,
                  const QString &label,
                  bool arrowAtSource,
                  qreal labelT = -1.0)
        : m_source(source),
          m_dest(dest),
          m_color(color),
          m_label(label),
          m_arrowAtSource(arrowAtSource),
          m_customLabelT(labelT) {
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
            const qreal ux = full.dx() / len;
            const qreal uy = full.dy() / len;
            const QPointF srcOff = m_source->edgeAnchorOffset(ux, uy);
            const QPointF dstOff = m_dest->edgeAnchorOffset(-ux, -uy);
            const qreal srcSpace = std::hypot(srcOff.x(), srcOff.y());
            const qreal dstSpace = std::hypot(dstOff.x(), dstOff.y());
            if (len > srcSpace + dstSpace + 12.0) {
                m_srcPt = full.p1() + srcOff;
                m_dstPt = full.p2() + dstOff;
            } else {
                const qreal mid = len * 0.5;
                m_srcPt = full.p1() + QPointF(ux * (mid - 4.0), uy * (mid - 4.0));
                m_dstPt = full.p1() + QPointF(ux * (mid + 4.0), uy * (mid + 4.0));
            }
        } else {
            m_srcPt = m_dstPt = full.p1();
        }
    }

    QRectF boundingRect() const override {
        const qreal extra = 58.0;
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

        painter->setPen(QPen(QColor(18, 20, 28, 165), 4.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->drawLine(line);

        const QPen linePen(m_color, 2.1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter->setPen(linePen);
        painter->drawLine(line);

        const qreal arrowLen = 10.5;
        const qreal arrowHalfW = 4.8;
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

        QFont lblFont("DejaVu Sans Mono", 8, QFont::Bold);
        painter->setFont(lblFont);
        const QFontMetrics fm(lblFont);
        const int txtW = fm.horizontalAdvance(m_label) + 12;
        if (!m_label.isEmpty() && len >= 28.0) {
            const qreal halfGap = txtW * 0.5;
            qreal labelT = (m_customLabelT >= 0.0) ? qBound(0.15, m_customLabelT, 0.85) : 0.5;
            if (m_customLabelT < 0.0) {
                if (uy < -0.35) {
                    labelT = 0.30;
                } else if (uy > 0.35) {
                    labelT = 0.62;
                } else if (uy > 0.15) {
                    labelT = 0.56;
                }
            }
            const QPointF mid = line.pointAt(labelT);
            qreal deg = std::atan2(line.dy(), line.dx()) * 180.0 / std::numbers::pi;
            if (deg > 90.0) {
                deg -= 180.0;
            } else if (deg < -90.0) {
                deg += 180.0;
            }
            if (std::abs(uy) > 0.85) {
                deg = 0.0;
            }
            painter->save();
            painter->translate(mid);
            painter->rotate(deg);
            painter->setPen(QPen(QColor(68, 71, 90, 175), 1.0));
            painter->setBrush(QColor(24, 26, 36, 236));
            painter->drawRoundedRect(QRectF(-halfGap, -8.5, txtW, 17.0), 4.0, 4.0);
            painter->setPen(QColor("#e6e8ef"));
            painter->drawText(QRectF(-halfGap, -8.5, txtW, 17.0), Qt::AlignCenter, m_label);
            painter->restore();
        }
    }

private:
    CielGraphNode *m_source = nullptr;
    CielGraphNode *m_dest = nullptr;
    QColor m_color;
    QString m_label;
    bool m_arrowAtSource = true;
    qreal m_customLabelT = -1.0;
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

class CielGraphView : public QGraphicsView {
public:
    explicit CielGraphView(QGraphicsScene *scene, QWidget *parent = nullptr)
        : QGraphicsView(scene, parent) {
        setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
        setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
        setDragMode(QGraphicsView::ScrollHandDrag);
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        setResizeAnchor(QGraphicsView::AnchorViewCenter);
    }

    void setZoomCallback(std::function<void(qreal, bool)> cb) {
        m_onZoom = std::move(cb);
    }

    qreal currentScale() const {
        return transform().m11();
    }

    void setZoomScale(qreal targetScale, bool userInitiated = true) {
        const qreal clamped = qBound(0.45, targetScale, 2.50);
        setTransform(QTransform::fromScale(clamped, clamped));
        if (m_onZoom) {
            m_onZoom(currentScale(), userInitiated);
        }
    }

    void zoomBy(qreal factor, bool userInitiated = true) {
        setZoomScale(currentScale() * factor, userInitiated);
    }

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override {
        (void)rect;
        painter->save();
        painter->resetTransform();
        const QRect vp = viewport()->rect();
        QLinearGradient grad(0.0, 0.0, 0.0, static_cast<qreal>(vp.height()));
        grad.setColorAt(0.0, QColor("#282a36"));
        grad.setColorAt(1.0, QColor("#242632"));
        painter->fillRect(vp, grad);
        painter->restore();
    }

    void wheelEvent(QWheelEvent *event) override {
        const int delta = event->angleDelta().y();
        if (delta != 0) {
            zoomBy(delta > 0 ? 1.14 : (1.0 / 1.14), true);
            event->accept();
            return;
        }
        QGraphicsView::wheelEvent(event);
    }

private:
    std::function<void(qreal, bool)> m_onZoom;
};

template <typename EditT>
void updateEditorPreservingScroll(EditT *edit, const QString &plainText) {
    if (!edit || edit->toPlainText() == plainText) {
        return;
    }
    if (edit->textCursor().hasSelection()) {
        return;
    }
    QScrollBar *vbar = edit->verticalScrollBar();
    QScrollBar *hbar = edit->horizontalScrollBar();
    if ((vbar && vbar->isSliderDown()) || (hbar && hbar->isSliderDown())) {
        return;
    }
    const int vVal = vbar ? vbar->value() : 0;
    const int hVal = hbar ? hbar->value() : 0;

    // Do NOT block scrollbar signals: QAbstractScrollArea scrolls its viewport via valueChanged(int).
    // Do NOT call setTextCursor(findBlockByNumber): QTextEdit scrollbars are pixel-based, not line-based.
    edit->setPlainText(plainText);
    if (vbar) {
        vbar->setValue(qBound(0, vVal, vbar->maximum()));
    }
    if (hbar) {
        hbar->setValue(qBound(0, hVal, hbar->maximum()));
    }

    QPointer<EditT> safeEdit(edit);
    QTimer::singleShot(0, edit, [safeEdit, vVal, hVal]() {
        if (!safeEdit) {
            return;
        }
        QScrollBar *vb = safeEdit->verticalScrollBar();
        QScrollBar *hb = safeEdit->horizontalScrollBar();
        if (vb) {
            const int targetV = qBound(0, vVal, vb->maximum());
            if (vb->value() != targetV) {
                vb->setValue(targetV);
            }
        }
        if (hb) {
            const int targetH = qBound(0, hVal, hb->maximum());
            if (hb->value() != targetH) {
                hb->setValue(targetH);
            }
        }
    });
}

}  // namespace

class CielWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit CielWindow(const QString &initialAddr,
                        const QString &explicitToken = QString(),
                        const QString &tokenFilePath = QString(),
                        QWidget *parent = nullptr)
        : QMainWindow(parent),
          m_serverAddr(initialAddr),
          m_authToken(explicitToken.trimmed()),
          m_tokenFilePath(tokenFilePath.trimmed()) {
        m_localUser = qEnvironmentVariable("USER");
        if (m_localUser.isEmpty()) {
            m_localUser = qEnvironmentVariable("USERNAME");
        }
        if (m_localUser.isEmpty()) {
            m_localUser = "operator";
        }
        m_localUid = qEnvironmentVariable("UID");
#ifndef Q_OS_WIN
        if (m_localUid.isEmpty()) {
            m_localUid = QString::number(static_cast<unsigned long>(::getuid()));
        }
#endif
        if (m_localUid.isEmpty()) {
            m_localUid = "-";
        }
        m_localHost = QSysInfo::machineHostName();
        if (m_localHost.isEmpty()) {
            m_localHost = "localhost";
        }
        m_localArch = QSysInfo::currentCpuArchitecture();
        if (m_localArch.isEmpty()) {
            m_localArch = "x86_64";
        }
        const QString xdgRuntime = qEnvironmentVariable("XDG_RUNTIME_DIR");
        if (!xdgRuntime.isEmpty()) {
            m_workDir = xdgRuntime + "/ciel_workspace";
        } else {
            const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
            const QString uidSuffix = (m_localUid != "-" && !m_localUid.isEmpty()) ? ("_" + m_localUid) : QString();
            m_workDir = !cacheDir.isEmpty() ? (cacheDir + "/ciel_workspace") : ("/tmp/ciel_workspace" + uidSuffix);
        }

        if (m_authToken.isEmpty()) {
            m_authToken = qEnvironmentVariable("CIEL_AUTH_TOKEN").trimmed();
        }

        setWindowTitle("Ciel");
        resize(1480, 880);
        if (statusBar()) {
            statusBar()->setVisible(false);
        }

        buildMenuBar();
        buildCentralCielLayout();

        connect(&m_pollTimer, &QTimer::timeout, this, [this]() {
            if (!m_actionInFlight) {
                runFullSync(true);
            }
        });

        appendOperativeBanner();
        appendHelpTableToConsole();
    }

    void showEvent(QShowEvent *event) override {
        QMainWindow::showEvent(event);
        if (!m_initialSplitApplied) {
            m_initialSplitApplied = true;
            applyDefaultSplitterProportions();
            QTimer::singleShot(0, this, &CielWindow::applyDefaultSplitterProportions);
        }
    }

    void applyDefaultSplitterProportions() {
        if (m_topSplit) {
            const int w = qMax(m_topSplit->width(), qMax(width() - 6, 1474));
            const int leftW = qRound(w * 0.62);
            m_topSplit->setSizes({leftW, w - leftW});
        }
        if (m_topBotSplitter) {
            const int h = qMax(m_topBotSplitter->height(), qMax(height() - 32, 848));
            const int topH = qRound(h * 0.48);
            m_topBotSplitter->setSizes({topH, h - topH});
        }
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
                    const QString &altScreenshotPath = QString(),
                    const QString &consoleScreenshotPath = QString()) {
        auto fail = [this](const QString &msg) -> int {
            fprintf(stderr, "SELF-TEST FAIL: %s\n", msg.toUtf8().constData());
            appendEventLog("!", "#ff5555", "SELF-TEST FAIL: " + htmlEsc(msg));
            return 1;
        };

        QJsonObject pingResp;
        if (!executeAction("ping", QJsonObject(), pingResp, false) ||
            !pingResp.value("ok").toBool(false)) {
            return fail("ping RPC failed");
        }

        QJsonObject syncResp;
        if (!executeAction("full_sync", targetParams(), syncResp, false)) {
            return fail("full_sync RPC failed");
        }
        if (m_sessionTable->rowCount() < 5) {
            return fail(QString("Expected >= 5 rows, got %1").arg(m_sessionTable->rowCount()));
        }

        // Verify repeated background polls do NOT spam Operative Console or Event Viewer when unchanged
        const QString consoleBeforePoll = m_operativeConsole->toPlainText();
        const QString eventsBeforePoll = m_eventViewer->toPlainText();
        for (int tick = 0; tick < 2; ++tick) {
            QJsonObject bgResp;
            if (!executeAction("full_sync", targetParams(), bgResp, true)) {
                return fail("background full_sync RPC failed");
            }
            if (m_operativeConsole->toPlainText() != consoleBeforePoll) {
                return fail("Background poll unexpectedly wrote to Operative Console");
            }
            if (m_eventViewer->toPlainText() != eventsBeforePoll) {
                return fail("Unchanged background poll unexpectedly spammed Event Viewer");
            }
        }

        // Verify real temporary file operations inside QTemporaryDir (never in teamserver)
        QTemporaryDir tempDir;
        if (!tempDir.isValid()) {
            return fail("Failed creating temporary self-test directory");
        }
        const QString watchTarget = tempDir.filePath("watch_probe.ccache");
        {
            QFile wf(watchTarget);
            if (!wf.open(QIODevice::WriteOnly)) {
                return fail("Failed creating temp watch file");
            }
            wf.write("real-watch-probe");
            wf.close();
        }
        QTimer touchTimer;
        connect(&touchTimer, &QTimer::timeout, this, [watchTarget]() {
            QFile f(watchTarget);
            if (f.open(QIODevice::ReadOnly)) {
                (void)f.readAll();
                f.close();
            }
        });
        touchTimer.start(80);
        QJsonObject watchParams;
        watchParams["watch_path"] = watchTarget;
        watchParams["watch_timeout_ms"] = 800;
        QJsonObject watchResp;
        appendCommandEcho("souei --watch-file " + watchTarget);
        const bool watchRpcOk = executeAction("souei_watch", watchParams, watchResp, false);
        touchTimer.stop();
        if (!watchRpcOk) {
            return fail("souei_watch RPC failed");
        }
        const QJsonObject inoObj = watchResp.value("data").toObject().value("inotify").toObject();
        if (!inoObj.value("watched").toBool(false) || inoObj.value("events").toArray().isEmpty()) {
            return fail("Expected live inotify events from souei_watch");
        }

        // Verify missing CCACHE reports MISSING honestly (exit 3)
        QJsonObject missParams;
        missParams["ccache_path"] = tempDir.filePath("nonexistent.ccache");
        QJsonObject missResp;
        if (!executeAction("chronoa_check", missParams, missResp, false)) {
            return fail("chronoa_check missing path RPC failed");
        }
        if (missResp.value("exit_code").toInt(-1) != 3 ||
            missResp.value("status").toString() != "MISSING") {
            return fail("Expected exit_code=3 MISSING for nonexistent ccache");
        }

        // Verify Ranga isolate (0x3f000000)
        QJsonObject isoResp;
        appendCommandEcho("ranga --isolate 0x3f000000");
        if (!executeAction("ranga_isolate", QJsonObject(), isoResp, false)) {
            return fail("ranga_isolate RPC failed");
        }
        const QJsonObject isoData = isoResp.value("data").toObject();
        if (isoResp.value("exit_code").toInt(-1) != 0 ||
            !isoData.value("possessor_only").toBool(false) ||
            isoData.value("perm_hex").toString() != "3f000000") {
            return fail("Expected possessor_only=true and perm_hex=3f000000");
        }

        // Verify Ranga purge rejects empty target (OP-05 alert row -> #ff5555) and succeeds on temp artifact
        QJsonObject badPurgeResp;
        if (!executeAction("ranga_purge", QJsonObject(), badPurgeResp, false) ||
            badPurgeResp.value("exit_code").toInt(-1) != 1 ||
            badPurgeResp.value("status").toString() != "USAGE_ERROR") {
            return fail("Expected ranga_purge without purge_path to return USAGE_ERROR");
        }
        {
            bool sawAlertIdColor = false;
            for (int r = 0; r < m_sessionTable->rowCount(); ++r) {
                if (m_sessionTable->item(r, 0) && m_sessionTable->item(r, 0)->text() == "OP-05") {
                    sawAlertIdColor = (m_sessionTable->item(r, 0)->foreground().color() == QColor("#ff5555"));
                    break;
                }
            }
            if (!sawAlertIdColor) {
                return fail("Expected alert OP-05 row ID cell to use #ff5555 foreground");
            }
        }

        const QString purgeTarget = tempDir.filePath("ephemeral_secret.bin");
        {
            QFile pf(purgeTarget);
            if (!pf.open(QIODevice::WriteOnly)) {
                return fail("Failed creating temp purge file");
            }
            pf.write(QByteArray(2048, 'K'));
            pf.close();
        }
        QJsonObject purgeParams;
        purgeParams["purge_path"] = purgeTarget;
        QJsonObject purgeResp;
        appendCommandEcho("ranga --purge " + purgeTarget);
        if (!executeAction("ranga_purge", purgeParams, purgeResp, false)) {
            return fail("ranga_purge RPC failed");
        }
        const QJsonObject purgeData = purgeResp.value("data").toObject();
        if (purgeResp.value("exit_code").toInt(-1) != 0 ||
            !purgeData.value("removed").toBool(false) ||
            purgeData.value("passes").toInt(0) != 3) {
            return fail("Expected passes=3 and removed=true");
        }

        // Verify real host-safe Tanuki modules + subcommand allowlist rejection
        for (const QString &tCmd : QStringList{"doctor", "ladder", "triage", "nhi", "skill"}) {
            QJsonObject tp;
            tp["subcommand"] = tCmd;
            QJsonObject tResp;
            appendCommandEcho("tanuki " + tCmd);
            if (!executeAction("tanuki_exec", tp, tResp, false)) {
                return fail("tanuki_exec " + tCmd + " RPC failed");
            }
            if (tResp.value("exit_code").toInt(-1) != 0) {
                return fail(QString("tanuki_exec %1 returned exit %2").arg(tCmd).arg(tResp.value("exit_code").toInt()));
            }
        }
        {
            QJsonObject badTanuki;
            badTanuki["subcommand"] = "invalid_cmd";
            QJsonObject badTResp;
            if (!executeAction("tanuki_exec", badTanuki, badTResp, false) ||
                badTResp.value("exit_code").toInt(-1) != 1 ||
                badTResp.value("status").toString() != "USAGE_ERROR") {
                return fail("Expected disallowed tanuki subcommand to be rejected with USAGE_ERROR");
            }
        }

        // Verify Havoc auth banner and neutral [^] output in Operative Console
        {
            const QString bannerPlain = m_operativeConsole->toPlainText();
            const QString bannerHtml = m_operativeConsole->toHtml();
            if (!bannerPlain.contains("Operative OP-01..06 authenticated as") ||
                !bannerPlain.contains("[Pivot: Direct]") ||
                !bannerHtml.contains("#6272a4") ||
                !bannerHtml.contains("#ff5555") ||
                !bannerHtml.contains("#bd93f9") ||
                !bannerHtml.contains("#ff79c6")) {
                return fail("Operative Console missing Havoc auth banner or semantic colors");
            }
            handleOperativeCommand("set");
            if (!m_operativeConsole->toPlainText().contains("[^] Targets ->")) {
                return fail("Operative Console 'set' command did not emit [^] neutral line");
            }
        }

        appendCommandEcho("sync");
        if (!executeAction("full_sync", targetParams(), syncResp, false)) {
            return fail("final full_sync RPC failed");
        }
        appendCommandEcho("audit");
        appendDetailedAuditTables();
        if (m_sessionTable->rowCount() < 6 ||
            m_eventViewer->toPlainText().trimmed().isEmpty() ||
            m_operativeConsole->toPlainText().trimmed().isEmpty()) {
            return fail("Missing table rows or console telemetry after self-test");
        }

        // Verify background poll preserves Raw JSON scroll AND actual visible top block (zero scroll-jump)
        m_smallTabs->setCurrentIndex(1);
        for (int r = 0; r < m_sessionTable->rowCount(); ++r) {
            if (m_sessionTable->item(r, 0) && m_sessionTable->item(r, 0)->text() == "OP-06") {
                m_sessionTable->selectRow(r);
                showRowJson(r);
                break;
            }
        }
        QApplication::processEvents();
        QScrollBar *rawVb = m_rawTelemetry->verticalScrollBar();
        if (!rawVb || rawVb->maximum() <= 0) {
            return fail("Expected OP-06 Raw JSON to be scrollable during self-test");
        }
        const int targetRawScroll = qMin(8, rawVb->maximum());
        rawVb->setValue(targetRawScroll);
        QApplication::processEvents();
        const int topBlockBefore = m_rawTelemetry->cursorForPosition(QPoint(8, 8)).blockNumber();
        {
            QJsonObject bgScrollResp;
            if (!executeAction("full_sync", targetParams(), bgScrollResp, true)) {
                return fail("background scroll-check full_sync failed");
            }
            QApplication::processEvents();
        }
        if (rawVb->value() != targetRawScroll) {
            return fail(QString("Background poll reset Raw JSON scroll from %1 to %2")
                            .arg(targetRawScroll)
                            .arg(rawVb->value()));
        }
        const int topBlockAfter = m_rawTelemetry->cursorForPosition(QPoint(8, 8)).blockNumber();
        if (topBlockAfter != topBlockBefore) {
            return fail(QString("Background poll jumped Raw JSON visible top block from %1 to %2")
                            .arg(topBlockBefore)
                            .arg(topBlockAfter));
        }

        // Verify QTextEdit context pane (OP-06 Tanuki) preserves pixel scroll across background polls
        m_bottomTabs->setCurrentIndex(7);
        QApplication::processEvents();
        if (QTextEdit *op06Pane = m_contextPanes.value("OP-06", nullptr)) {
            if (QScrollBar *ctxVb = op06Pane->verticalScrollBar()) {
                if (ctxVb->maximum() > 0) {
                    const int targetCtxScroll = qMin(120, ctxVb->maximum());
                    ctxVb->setValue(targetCtxScroll);
                    QApplication::processEvents();
                    const int ctxTopBefore = op06Pane->cursorForPosition(QPoint(8, 8)).blockNumber();
                    QJsonObject bgCtxResp;
                    if (!executeAction("full_sync", targetParams(), bgCtxResp, true)) {
                        return fail("background context scroll-check full_sync failed");
                    }
                    QApplication::processEvents();
                    const int ctxTopAfter = op06Pane->cursorForPosition(QPoint(8, 8)).blockNumber();
                    if (ctxVb->value() != targetCtxScroll || ctxTopAfter != ctxTopBefore) {
                        return fail(QString("Background poll jumped OP-06 context scroll (%1->%2, block %3->%4)")
                                        .arg(targetCtxScroll).arg(ctxVb->value())
                                        .arg(ctxTopBefore).arg(ctxTopAfter));
                    }
                }
            }
        }

        // Verify default splitter proportions (60-64% left, 48-50% top) and Session Graph readability
        applyDefaultSplitterProportions();
        m_mainViewTabs->setCurrentIndex(1);
        m_graphUserZoomed = false;
        QApplication::processEvents();
        if (m_topSplit && m_topSplit->sizes().size() == 2) {
            const int totW = m_topSplit->sizes().at(0) + m_topSplit->sizes().at(1);
            const qreal leftFrac = (totW > 0) ? (static_cast<qreal>(m_topSplit->sizes().at(0)) / totW) : 0.0;
            if (leftFrac < 0.58 || leftFrac > 0.66) {
                return fail(QString("Top splitter proportion out of range: %1").arg(leftFrac));
            }
        }
        if (m_topBotSplitter && m_topBotSplitter->sizes().size() == 2) {
            const int totH = m_topBotSplitter->sizes().at(0) + m_topBotSplitter->sizes().at(1);
            const qreal topFrac = (totH > 0) ? (static_cast<qreal>(m_topBotSplitter->sizes().at(0)) / totH) : 0.0;
            if (topFrac < 0.45 || topFrac > 0.53) {
                return fail(QString("Vertical splitter proportion out of range: %1").arg(topFrac));
            }
        }
        rebuildSessionGraph();
        QApplication::processEvents();
        if (!m_graphView || m_graphView->currentScale() < 0.90 || m_graphView->currentScale() > 1.15) {
            return fail(QString("Session Graph zoom scale out of [0.90, 1.15] range (%1)")
                            .arg(m_graphView ? m_graphView->currentScale() : 0.0));
        }

        // Exercise interactive zoom, small-window resize, and alert/REVOKED graph node paint paths
        {
            QGraphicsScene checkScene;
            auto *alertNode = new CielGraphNode(
                CielGraphNode::NodeKind::OperativeMonitor, "OP-99",
                "OP-99 @ chronoa\\tgt", "[CORP.LOCAL\\svc :: CLOCK_SKEW_VIOLATION]",
                "chronoa", QColor("#ff5555"), true, false);
            auto *revokedNode = new CielGraphNode(
                CielGraphNode::NodeKind::OperativeMonitor, "OP-98",
                "OP-98 @ ranga\\ses", "[linux-kernel\\svc :: REVOKED]",
                "ranga-iso", QColor("#9aa5ce"), false, true);
            alertNode->setPos(160.0, 100.0);
            revokedNode->setPos(460.0, 160.0);
            checkScene.addItem(alertNode);
            checkScene.addItem(revokedNode);
            checkScene.addItem(new CielGraphEdge(alertNode, revokedNode, QColor("#ff5555"), "skew-alert", false));
            QImage offImg(640, 280, QImage::Format_ARGB32);
            offImg.fill(QColor("#282a36"));
            QPainter offP(&offImg);
            checkScene.render(&offP);
            offP.end();
            if (offImg.pixelColor(160, 100) == QColor("#282a36")) {
                return fail("Offscreen alert/revoked graph node render produced blank pixels");
            }

            // Verify vertical edgeAnchorOffset clears bottom text card when downward (uy>0.65) and stays at icon top when upward (uy<-0.65)
            const QPointF downOff = alertNode->edgeAnchorOffset(0.0, 1.0);
            const QPointF upOff = alertNode->edgeAnchorOffset(0.0, -1.0);
            if (downOff.y() < 80.0 || upOff.y() < -50.0 || upOff.y() > -46.0) {
                return fail(QString("edgeAnchorOffset vertical geometry invalid: downY=%1 upY=%2")
                                .arg(downOff.y()).arg(upOff.y()));
            }
            revokedNode->setPos(160.0, 260.0);
            QApplication::processEvents();

            // Exercise all 4 floating zoom pill buttons directly
            if (!m_graphZoomInBtn || !m_graphZoomOutBtn || !m_graphFitBtn || !m_graphZoomBtn) {
                return fail("Floating zoom pill buttons missing");
            }
            const qreal baseScale = m_graphView->currentScale();
            m_graphZoomInBtn->click();
            if (m_graphView->currentScale() <= baseScale) {
                return fail("Floating '+' zoom button did not zoom in");
            }
            m_graphZoomOutBtn->click();
            m_graphZoomBtn->click();
            if (qAbs(m_graphView->currentScale() - 1.0) > 0.02 || m_graphZoomBtn->text() != "100%") {
                return fail("Floating '100%' zoom button did not reset scale to 100%");
            }
            m_graphFitBtn->click();
            if (m_graphView->currentScale() < 0.90 || m_graphView->currentScale() > 1.15) {
                return fail("Floating 'Fit' button produced out-of-range scale");
            }

            m_graphView->zoomBy(1.18, true);
            const qreal zoomedIn = m_graphView->currentScale();
            if (zoomedIn <= 0.85) {
                return fail(QString("Expected zoomBy(1.18) to increase scale, got %1").arg(zoomedIn));
            }
            resize(1024, 700);
            QApplication::processEvents();
            m_graphUserZoomed = false;
            rebuildSessionGraph();
            QApplication::processEvents();
            if (m_graphView->currentScale() < 0.90) {
                return fail(QString("Small-window Session Graph shrank below readable minimum (%1 < 0.90)")
                                .arg(m_graphView->currentScale()));
            }
            m_graphView->setZoomScale(1.0, true);
            if (qAbs(m_graphView->currentScale() - 1.0) > 0.02) {
                return fail("Zoom reset to 1.0 failed");
            }
            resize(1480, 880);
            QApplication::processEvents();
            applyDefaultSplitterProportions();
            m_graphUserZoomed = false;
            rebuildSessionGraph();
            QApplication::processEvents();
        }

        if (!screenshotPath.isEmpty()) {
            m_mainViewTabs->setCurrentIndex(0);
            m_smallTabs->setCurrentIndex(0);
            m_bottomTabs->setCurrentIndex(m_op01TabIdx);
            m_sessionTable->selectRow(0);
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
            showRowJson(0);
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

        if (!consoleScreenshotPath.isEmpty()) {
            m_mainViewTabs->setCurrentIndex(0);
            m_smallTabs->setCurrentIndex(0);
            m_bottomTabs->setCurrentIndex(m_consoleTabIdx);
            QApplication::processEvents();
            QApplication::processEvents();
            const QPixmap conShot = grab();
            if (conShot.isNull() || !conShot.save(consoleScreenshotPath, "PNG")) {
                return fail("Failed saving console screenshot");
            }
        }

        if (!graphScreenshotPath.isEmpty()) {
            m_mainViewTabs->setCurrentIndex(1);
            m_graphUserZoomed = false;
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
        runFullSync(false);
        appendDetailedAuditTables();
        if (m_actPoll) {
            m_actPoll->setChecked(true);
        }
        if (m_operativeInput) {
            m_operativeInput->setFocus();
        }
    }

    void runFullSync(bool background = false) {
        if (m_actionInFlight) {
            return;
        }
        if (!background) {
            commitTargetFieldsFromEdits();
            m_lastExplicitTanukiSubcmd = "doctor";
        }
        QJsonObject resp;
        executeAction("full_sync", targetParams(), resp, background);
    }

private:
    void commitTargetFieldsFromEdits() {
        if (m_ccacheEdit) {
            m_committedCcache = m_ccacheEdit->text().trimmed();
        }
        if (m_keytabEdit) {
            m_committedKeytab = m_keytabEdit->text().trimmed();
        }
        if (m_krb5ConfEdit) {
            m_committedKrb5Conf = m_krb5ConfEdit->text().trimmed();
        }
        if (m_kcmSockEdit) {
            m_committedKcmSocket = m_kcmSockEdit->text().trimmed();
        }
    }

    QJsonObject targetParams() const {
        QJsonObject p;
        if (!m_committedCcache.isEmpty()) {
            p["ccache_path"] = m_committedCcache;
        }
        if (!m_committedKeytab.isEmpty()) {
            p["keytab_path"] = m_committedKeytab;
        }
        if (!m_committedKrb5Conf.isEmpty()) {
            p["krb5_conf_path"] = m_committedKrb5Conf;
        }
        if (!m_committedKcmSocket.isEmpty()) {
            p["kcm_socket"] = m_committedKcmSocket;
        }
        return p;
    }

    QString loadAuthTokenFromDisk() const {
        QStringList candidates;
        if (!m_tokenFilePath.isEmpty()) {
            candidates.append(m_tokenFilePath);
        }
        candidates.append(m_workDir + "/teamserver.token");
        for (const QString &path : candidates) {
            QFile f(path);
            if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                const QString tok = QString::fromUtf8(f.readAll()).trimmed();
                if (!tok.isEmpty()) {
                    return tok;
                }
            }
        }
        return QString();
    }

    void buildMenuBar() {
        QMenuBar *mb = menuBar();

        QMenu *mCiel = mb->addMenu("Ciel");
        QAction *actSync = mCiel->addAction("Sync Teamserver");
        m_actPoll = mCiel->addAction("Background Monitor (3s)");
        m_actPoll->setCheckable(true);
        mCiel->addSeparator();
        QAction *actExit = mCiel->addAction("Exit");
        connect(actSync, &QAction::triggered, this, [this]() { runFullSync(false); });
        connect(m_actPoll, &QAction::toggled, this, [this](bool on) {
            if (on) {
                m_pollTimer.start(3000);
                appendEventLog("*", "#8be9fd", "Background telemetry monitor enabled (3000ms, diff-only)");
            } else {
                m_pollTimer.stop();
                appendEventLog("*", "#8be9fd", "Background telemetry monitor paused");
            }
        });
        connect(actExit, &QAction::triggered, this, &QWidget::close);

        QMenu *mView = mb->addMenu("View");
        QAction *actTable = mView->addAction("Session View -> Table");
        QAction *actGraph = mView->addAction("Session View -> Graph");
        mView->addSeparator();
        QAction *actEvents = mView->addAction("Event Viewer");
        QAction *actJson = mView->addAction("Raw Telemetry (JSON)");
        mView->addSeparator();
        QAction *actTeamserver = mView->addAction("Teamserver Status");
        QAction *actViewConsole = mView->addAction("Operative Console");
        QAction *actListeners = mView->addAction("Listeners & Target Config");
        connect(actTable, &QAction::triggered, this, [this]() { m_mainViewTabs->setCurrentIndex(0); });
        connect(actGraph, &QAction::triggered, this, [this]() {
            m_mainViewTabs->setCurrentIndex(1);
            rebuildSessionGraph();
        });
        connect(actEvents, &QAction::triggered, this, [this]() { m_smallTabs->setCurrentIndex(0); });
        connect(actJson, &QAction::triggered, this, [this]() { m_smallTabs->setCurrentIndex(1); });
        connect(actTeamserver, &QAction::triggered, this, [this]() { m_bottomTabs->setCurrentIndex(0); });
        connect(actViewConsole, &QAction::triggered, this, [this]() {
            m_bottomTabs->setCurrentIndex(m_consoleTabIdx);
            if (m_operativeInput) {
                m_operativeInput->setFocus();
            }
        });
        connect(actListeners, &QAction::triggered, this, [this]() { m_bottomTabs->setCurrentIndex(m_listenersTabIdx); });

        QMenu *mOps = mb->addMenu("Operatives");
        QAction *actSouei = mOps->addAction("Souei: Probe /proc/keys & KCM");
        QAction *actWatch = mOps->addAction("Souei: Watch File (Inotify)...");
        QAction *actChronoa = mOps->addAction("Chronoa: Audit Active CCACHE");
        QAction *actRenew = mOps->addAction("Chronoa: Renew TGT (kinit -R)");
        mOps->addSeparator();
        QAction *actRangaStatus = mOps->addAction("Ranga: Inspect Session Keyring");
        QAction *actIso = mOps->addAction("Ranga: Isolate 0x3f000000 (LD_PRELOAD)");
        QAction *actIsoWrap = mOps->addAction("Ranga: Isolate 0x3f000000 (Wrapper)");
        QAction *actPurge = mOps->addAction("Ranga: 3-Pass NIST Purge File...");

        connect(actSouei, &QAction::triggered, this, [this]() {
            commitTargetFieldsFromEdits();
            dispatchFromUi("souei_probe", targetParams(), "souei --probe");
        });
        connect(actWatch, &QAction::triggered, this, [this]() { promptAndWatchFile(); });
        connect(actChronoa, &QAction::triggered, this, [this]() {
            commitTargetFieldsFromEdits();
            dispatchFromUi("chronoa_check", targetParams(), "chronoa --ccache");
        });
        connect(actRenew, &QAction::triggered, this, [this]() {
            commitTargetFieldsFromEdits();
            QJsonObject p = targetParams();
            p["renew"] = true;
            dispatchFromUi("chronoa_check", p, "chronoa --renew (kinit -R)");
        });
        connect(actRangaStatus, &QAction::triggered, this, [this]() {
            dispatchFromUi("ranga_status", QJsonObject(), "ranga --status");
        });
        connect(actIso, &QAction::triggered, this, [this]() {
            dispatchFromUi("ranga_isolate", QJsonObject(), "ranga --isolate");
        });
        connect(actIsoWrap, &QAction::triggered, this, [this]() {
            QJsonObject p;
            p["isolate_mode"] = "wrapper";
            dispatchFromUi("ranga_isolate", p, "ranga --isolate (wrapper)");
        });
        connect(actPurge, &QAction::triggered, this, [this]() { promptAndPurgeFile(); });

        QMenu *mTanuki = mb->addMenu("Tanuki");
        auto addTanukiMenuItem = [&](const QString &label, const QString &subcmd, const QString &defaultArg = QString()) {
            QAction *act = mTanuki->addAction(label);
            connect(act, &QAction::triggered, this, [this, subcmd, defaultArg]() {
                commitTargetFieldsFromEdits();
                dispatchTanuki(subcmd, defaultArg);
            });
        };
        auto addTanukiPromptItem = [&](const QString &label, const QString &subcmd, const QString &promptText, auto presetFn) {
            QAction *act = mTanuki->addAction(label);
            connect(act, &QAction::triggered, this, [this, label, subcmd, promptText, presetFn]() {
                commitTargetFieldsFromEdits();
                promptTanuki(label, subcmd, promptText, presetFn());
            });
        };
        addTanukiMenuItem("Doctor: Pre-Flight & OPSEC Audit", "doctor");
        addTanukiMenuItem("Fix: Self-Healing Engine (Dry-Run)", "fix");
        addTanukiPromptItem("Keytab: Inspect RFC 4120 Binary...", "keytab", "Keytab file path:", [this]() {
            return m_keytabEdit && !m_keytabEdit->text().trimmed().isEmpty()
                       ? m_keytabEdit->text().trimmed()
                       : m_resolvedKeytab;
        });
        addTanukiPromptItem("Config: Generate Zero-DNS krb5.conf...", "config",
                            "Arguments (--realm CORP.LOCAL --kdc <IP> [-o /path/krb5.conf]):",
                            []() { return QString("--realm CORP.LOCAL --kdc "); });
        mTanuki->addSeparator();
        addTanukiPromptItem("Triage: Error Code Lookup...", "triage", "Kerberos / SSSD error query (or SKEW):", []() { return QString("SKEW"); });
        addTanukiMenuItem("Ladder: 5-Rung Decision Ladder", "ladder");
        addTanukiMenuItem("NHI: Local Workload Token Scan", "nhi", "scan");
        addTanukiMenuItem("NHI: Cloud Federation Mesh Probe", "nhi", "mesh");
        addTanukiPromptItem("Token: Validate Workload JWT...", "token", "JWT file path or raw JWT token:", []() { return QString(); });
        mTanuki->addSeparator();
        addTanukiPromptItem("Blast: AD Privilege & Tier Mapper...", "blast", "Principal, keytab/ccache path, or --host <DC>:", [this]() {
            if (m_keytabEdit && !m_keytabEdit->text().trimmed().isEmpty()) {
                return QString("--keytab %1").arg(m_keytabEdit->text().trimmed());
            }
            if (m_ccacheEdit && !m_ccacheEdit->text().trimmed().isEmpty()) {
                return QString("--ccache %1").arg(m_ccacheEdit->text().trimmed());
            }
            return QString();
        });
        addTanukiPromptItem("ADCS: Scan Templates (ESC1-11)...", "adcs", "Templates JSON/LDIF or X.509 certificate path:", []() { return QString(); });
        addTanukiPromptItem("PAC: Decode MS-PAC Privileges...", "pac", "PAC file path or hex/base64 payload:", []() { return QString(); });
        addTanukiPromptItem("Shadow: Inspect msDS-KeyCredentialLink...", "shadow", "KeyCredentialLink file path, hex, or DN-Binary:", []() { return QString(); });
        addTanukiMenuItem("KCM: Triage Local SSSD Caches", "kcm");
        mTanuki->addSeparator();
        addTanukiPromptItem("Auth: Live AS-REQ Pre-Auth...", "auth", "Arguments (--keytab <path> --principal <user@REALM> --kdc <IP>):",
                            [this]() {
                                const QString kt = (m_keytabEdit && !m_keytabEdit->text().trimmed().isEmpty())
                                                       ? m_keytabEdit->text().trimmed()
                                                       : m_resolvedKeytab;
                                return kt.isEmpty() ? QString("--keytab ") : QString("--keytab %1 --principal ").arg(kt);
                            });
        addTanukiPromptItem("TGS: Live Service Ticket Request...", "tgs", "Arguments (--spn <svc/host@REALM> --ccache <path> --kdc <IP>):",
                            []() { return QString("--spn "); });
        addTanukiPromptItem("LDAP: AD SPN / Delegation Query...", "ldap", "Arguments (--host <DC_IP> --realm <REALM> --query all):",
                            []() { return QString("--host  --realm  --query all"); });
        QAction *actTanukiPurge = mTanuki->addAction("Purge: Wipe Kerberos Caches / Artifact...");
        connect(actTanukiPurge, &QAction::triggered, this, [this]() { promptTanukiPurge(); });
        addTanukiMenuItem("Skill: Inspect Tanuki Manifest", "skill");

        QMenu *mScripts = mb->addMenu("Scripts");
        QAction *actAuditMatrix = mScripts->addAction("Run Host Audit Matrix (audit)");
        QAction *actHelpCmd = mScripts->addAction("Print Console Commands");
        QAction *actClearConsole = mScripts->addAction("Clear Console");
        connect(actAuditMatrix, &QAction::triggered, this, [this]() {
            m_bottomTabs->setCurrentIndex(m_consoleTabIdx);
            appendCommandEcho("audit");
            appendDetailedAuditTables();
            if (m_operativeInput) {
                m_operativeInput->setFocus();
            }
        });
        connect(actHelpCmd, &QAction::triggered, this, [this]() {
            m_bottomTabs->setCurrentIndex(m_consoleTabIdx);
            appendHelpTableToConsole();
        });
        connect(actClearConsole, &QAction::triggered, this, [this]() {
            m_bottomTabs->setCurrentIndex(m_consoleTabIdx);
            m_operativeConsole->clear();
            appendOperativeBanner();
        });

        QMenu *mHelp = mb->addMenu("Help");
        QAction *actAbout = mHelp->addAction("About Ciel");
        connect(actAbout, &QAction::triggered, this, [this]() {
            QMessageBox::information(this, "About Ciel",
                                     "Ciel Operator Console\nStack: C++20 Qt6 Widgets + Go Teamserver\nOperatives: Souei, Chronoa, Ranga (Tempest + Tanuki)");
        });
    }

    void buildCentralCielLayout() {
        auto *root = new QWidget(this);
        auto *rootLayout = new QVBoxLayout(root);
        rootLayout->setContentsMargins(3, 3, 3, 3);
        rootLayout->setSpacing(3);

        m_topBotSplitter = new QSplitter(Qt::Vertical, root);
        m_topBotSplitter->setHandleWidth(4);

        m_topSplit = new QSplitter(Qt::Horizontal, m_topBotSplitter);
        m_topSplit->setHandleWidth(4);

        m_mainViewTabs = new QTabWidget(m_topSplit);
        m_mainViewTabs->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        m_mainViewTabs->setMinimumWidth(460);
        m_sessionTable = buildSessionTable();
        QWidget *graphContainer = buildSessionGraphContainer();
        m_mainViewTabs->addTab(m_sessionTable, "Sessions");
        m_mainViewTabs->addTab(graphContainer, "Session Graph");
        connect(m_mainViewTabs, &QTabWidget::currentChanged, this, [this](int idx) {
            if (idx == 1) {
                rebuildSessionGraph();
            }
        });

        m_smallTabs = new QTabWidget(m_topSplit);
        m_smallTabs->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        m_smallTabs->setMinimumWidth(360);
        m_eventViewer = new QTextEdit(m_smallTabs);
        m_eventViewer->setReadOnly(true);
        m_eventViewer->setFont(QFont("DejaVu Sans Mono", 8));
        m_eventViewer->setLineWrapMode(QTextEdit::WidgetWidth);
        m_eventViewer->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        m_rawTelemetry = new QPlainTextEdit(m_smallTabs);
        m_rawTelemetry->setReadOnly(true);
        m_rawTelemetry->setPlaceholderText("Select an operative row in Sessions to inspect raw JSON.");
        m_smallTabs->addTab(m_eventViewer, "Event Viewer");
        m_smallTabs->addTab(m_rawTelemetry, "Raw JSON");

        m_topSplit->addWidget(m_mainViewTabs);
        m_topSplit->addWidget(m_smallTabs);
        m_topSplit->setStretchFactor(0, 62);
        m_topSplit->setStretchFactor(1, 38);
        m_topSplit->setSizes({900, 560});

        m_bottomTabs = new QTabWidget(m_topBotSplitter);
        m_bottomTabs->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);
        m_bottomTabs->setMinimumHeight(240);
        auto makeContextPane = [&](const QString &key, const QString &bannerText) {
            auto *te = new QTextEdit(m_bottomTabs);
            te->setReadOnly(true);
            te->setPlainText(bannerText);
            m_contextPanes[key] = te;
            return te;
        };
        m_bottomTabs->addTab(
            makeContextPane("TEAMSERVER", "[TEAMSERVER] ciel-teamserver endpoint: " + m_serverAddr + "\n[TEAMSERVER] Auth: Per-session 256-bit token enforced\n[TEAMSERVER] Waiting for initial sync..."),
            "Teamserver");
        m_op01TabIdx = m_bottomTabs->addTab(
            makeContextPane("OP-01", QString("[OP-01] %1@%2 :: /proc/keys + AF_UNIX SO_PEERCRED + inotify observer\nAwaiting live telemetry...").arg(m_localUser, m_localHost)),
            "OP-01 Souei");
        m_bottomTabs->addTab(
            makeContextPane("OP-02", "[OP-02] chronoa :: MIT CCACHE v3/v4 + RFC 4120 clock skew + kinit -R\nAwaiting live telemetry..."),
            "OP-02 Chronoa");
        m_consoleTabIdx = m_bottomTabs->addTab(buildOperativeConsoleTab(), "Operative Console");
        m_bottomTabs->addTab(
            makeContextPane("OP-03", "[OP-03] ranga:ses :: baseline user session keyring inspection\nAwaiting live telemetry..."),
            "OP-03 Ranga");
        m_bottomTabs->addTab(
            makeContextPane("OP-04", "[OP-04] ranga:iso :: possessor-only 0x3f000000 LD_PRELOAD + --isolate\nAwaiting live telemetry..."),
            "OP-04 Isolate");
        m_bottomTabs->addTab(
            makeContextPane("OP-05", "[OP-05] ranga:purge :: 3-pass NIST SP 800-88 wipe + explicit_bzero\nNo purge executed in this session yet."),
            "OP-05 Purge");
        m_bottomTabs->addTab(
            makeContextPane("OP-06", "[OP-06] tanuki :: NHI / Kerberos / AD 18-module engine\nAwaiting live telemetry..."),
            "OP-06 Tanuki");
        m_listenersTabIdx = m_bottomTabs->addTab(buildListenersTab(), "Listeners");
        m_bottomTabs->setCurrentIndex(m_consoleTabIdx);

        m_topBotSplitter->addWidget(m_topSplit);
        m_topBotSplitter->addWidget(m_bottomTabs);
        m_topBotSplitter->setStretchFactor(0, 48);
        m_topBotSplitter->setStretchFactor(1, 52);
        m_topBotSplitter->setSizes({408, 440});

        rootLayout->addWidget(m_topBotSplitter);
        setCentralWidget(root);
    }

    void showRowJson(int row) {
        if (row < 0 || !m_sessionTable) {
            return;
        }
        const QTableWidgetItem *idItem = m_sessionTable->item(row, 0);
        if (!idItem) {
            return;
        }
        const QString opId = idItem->text();
        if (m_snapshots.contains(opId) && m_snapshots.value(opId).rawJson.isObject()) {
            m_rawShowingExplicitAction = false;
            const QJsonDocument doc(m_snapshots.value(opId).rawJson.toObject());
            const QString jsonText = QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
            if (m_rawSourceOpId == opId) {
                updateEditorPreservingScroll(m_rawTelemetry, jsonText);
            } else {
                m_rawSourceOpId = opId;
                m_rawTelemetry->setPlainText(jsonText);
                if (QScrollBar *vb = m_rawTelemetry->verticalScrollBar()) {
                    vb->setValue(0);
                }
                if (QScrollBar *hb = m_rawTelemetry->horizontalScrollBar()) {
                    hb->setValue(0);
                }
            }
        }
    }

    void focusContextTabForOpId(const QString &opId) {
        static const QMap<QString, int> tabMap = {
            {"OP-01", 1},
            {"OP-02", 2},
            {"OP-03", 4},
            {"OP-04", 5},
            {"OP-05", 6},
            {"OP-06", 7},
        };
        if (tabMap.contains(opId)) {
            m_bottomTabs->setCurrentIndex(tabMap.value(opId));
        }
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
            "ID", "Host", "Telemetry", "Principal", "Scope",
            "Module", "Operative", "Target", "Perm / TTL", "Health"
        };
        tbl->setHorizontalHeaderLabels(headers);
        QHeaderView *hdr = tbl->horizontalHeader();
        hdr->setMinimumSectionSize(54);
        hdr->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        hdr->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        hdr->setSectionResizeMode(2, QHeaderView::Stretch);
        hdr->setSectionResizeMode(3, QHeaderView::ResizeToContents);
        hdr->setSectionResizeMode(4, QHeaderView::ResizeToContents);
        hdr->setSectionResizeMode(5, QHeaderView::Stretch);
        hdr->setSectionResizeMode(6, QHeaderView::ResizeToContents);
        hdr->setSectionResizeMode(7, QHeaderView::Stretch);
        hdr->setSectionResizeMode(8, QHeaderView::Stretch);
        hdr->setSectionResizeMode(9, QHeaderView::ResizeToContents);
        tbl->verticalHeader()->setDefaultSectionSize(22);

        connect(tbl, &QTableWidget::currentCellChanged,
                this, [this](int row, int col, int prevRow, int prevCol) {
                    (void)col;
                    (void)prevRow;
                    (void)prevCol;
                    showRowJson(row);
                });
        connect(tbl, &QTableWidget::cellClicked,
                this, [this](int row, int col) {
                    (void)col;
                    showRowJson(row);
                });
        connect(tbl, &QTableWidget::cellDoubleClicked,
                this, [this](int row, int col) {
                    (void)col;
                    showRowJson(row);
                    if (const QTableWidgetItem *idItem = m_sessionTable->item(row, 0)) {
                        focusContextTabForOpId(idItem->text());
                    }
                });

        connect(tbl, &QTableWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
            QMenu ctx(this);
            QAction *aInteract = ctx.addAction("Interact (Console)");
            ctx.addSeparator();
            QAction *aSouei = ctx.addAction("Probe /proc/keys & KCM (Souei)");
            QAction *aWatch = ctx.addAction("Watch File Inotify (Souei)...");
            QAction *aChronoa = ctx.addAction("Audit Active CCACHE (Chronoa)");
            QAction *aRenew = ctx.addAction("Renew TGT via kinit -R (Chronoa)");
            ctx.addSeparator();
            QAction *aIsolate = ctx.addAction("Isolate Session Keyring (Ranga)");
            QAction *aPurge = ctx.addAction("3-Pass NIST Purge File (Ranga)...");
            ctx.addSeparator();
            QMenu *tanukiSub = ctx.addMenu("Tanuki Identity Engine");
            auto addCtxTanuki = [&](const QString &label, const QString &subcmd, const QString &arg = QString()) {
                QAction *act = tanukiSub->addAction(label);
                connect(act, &QAction::triggered, this, [this, subcmd, arg]() {
                    commitTargetFieldsFromEdits();
                    dispatchTanuki(subcmd, arg);
                });
            };
            addCtxTanuki("Doctor Pre-Flight Audit (+OPSEC)", "doctor");
            addCtxTanuki("Fix Self-Healing (Dry-Run)", "fix");
            addCtxTanuki("5-Rung Decision Ladder", "ladder");
            addCtxTanuki("Triage Error Code (All)", "triage");
            addCtxTanuki("NHI Local Token Scan", "nhi", "scan");
            addCtxTanuki("NHI Cloud Mesh Probe", "nhi", "mesh");
            addCtxTanuki("Triage Local SSSD KCM", "kcm");
            addCtxTanuki("Inspect Tanuki Manifest", "skill");
            ctx.addSeparator();
            QAction *aJson = ctx.addAction("Inspect Raw JSON");

            connect(aInteract, &QAction::triggered, this, [this]() {
                m_bottomTabs->setCurrentIndex(m_consoleTabIdx);
                m_operativeInput->setFocus();
            });
            connect(aSouei, &QAction::triggered, this, [this]() {
                commitTargetFieldsFromEdits();
                dispatchFromUi("souei_probe", targetParams(), "souei --probe");
            });
            connect(aWatch, &QAction::triggered, this, [this]() { promptAndWatchFile(); });
            connect(aChronoa, &QAction::triggered, this, [this]() {
                commitTargetFieldsFromEdits();
                dispatchFromUi("chronoa_check", targetParams(), "chronoa --ccache");
            });
            connect(aRenew, &QAction::triggered, this, [this]() {
                commitTargetFieldsFromEdits();
                QJsonObject p = targetParams();
                p["renew"] = true;
                dispatchFromUi("chronoa_check", p, "chronoa --renew (kinit -R)");
            });
            connect(aIsolate, &QAction::triggered, this, [this]() {
                dispatchFromUi("ranga_isolate", QJsonObject(), "ranga --isolate");
            });
            connect(aPurge, &QAction::triggered, this, [this]() { promptAndPurgeFile(); });
            connect(aJson, &QAction::triggered, this, [this]() { m_smallTabs->setCurrentIndex(1); });

            ctx.exec(m_sessionTable->viewport()->mapToGlobal(pos));
        });

        return tbl;
    }

    QWidget *buildSessionGraphContainer() {
        auto *container = new QWidget(this);
        auto *layout = new QGridLayout(container);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        m_graphScene = new QGraphicsScene(this);
        m_graphScene->setBackgroundBrush(QColor("#282a36"));
        m_graphView = new CielGraphView(m_graphScene, container);
        m_graphView->setToolTip("Drag nodes, scroll wheel to zoom, click node to inspect JSON");
        layout->addWidget(m_graphView, 0, 0);

        auto *pillWrap = new QWidget(container);
        pillWrap->setAttribute(Qt::WA_TransparentForMouseEvents, false);
        auto *pillWrapLay = new QHBoxLayout(pillWrap);
        pillWrapLay->setContentsMargins(0, 8, 12, 0);
        pillWrapLay->setSpacing(0);

        auto *pill = new QWidget(pillWrap);
        pill->setObjectName("GraphZoomPill");
        pill->setStyleSheet(
            "QWidget#GraphZoomPill {"
            "  background-color: rgba(30, 31, 41, 215);"
            "  border: 1px solid #44475a;"
            "  border-radius: 6px;"
            "}"
            "QWidget#GraphZoomPill QPushButton {"
            "  background-color: transparent;"
            "  color: #f8f8f2;"
            "  border: none;"
            "  border-radius: 4px;"
            "  padding: 2px 6px;"
            "  min-height: 18px;"
            "  font-size: 11px;"
            "  font-weight: bold;"
            "}"
            "QWidget#GraphZoomPill QPushButton:hover {"
            "  background-color: #44475a;"
            "}");
        auto *pillLay = new QHBoxLayout(pill);
        pillLay->setContentsMargins(4, 2, 4, 2);
        pillLay->setSpacing(2);

        m_graphZoomOutBtn = new QPushButton("-", pill);
        m_graphZoomOutBtn->setFixedWidth(22);
        m_graphZoomOutBtn->setToolTip("Zoom out");

        m_graphZoomBtn = new QPushButton("100%", pill);
        m_graphZoomBtn->setFixedWidth(44);
        m_graphZoomBtn->setToolTip("Reset to 1:1 native scale (100%)");

        m_graphZoomInBtn = new QPushButton("+", pill);
        m_graphZoomInBtn->setFixedWidth(22);
        m_graphZoomInBtn->setToolTip("Zoom in");

        m_graphFitBtn = new QPushButton("Fit", pill);
        m_graphFitBtn->setFixedWidth(32);
        m_graphFitBtn->setToolTip("Fit topology into viewport");

        pillLay->addWidget(m_graphZoomOutBtn);
        pillLay->addWidget(m_graphZoomBtn);
        pillLay->addWidget(m_graphZoomInBtn);
        pillLay->addWidget(m_graphFitBtn);
        pillWrapLay->addWidget(pill);
        layout->addWidget(pillWrap, 0, 0, Qt::AlignTop | Qt::AlignRight);
        pillWrap->raise();

        m_graphView->setZoomCallback([this](qreal s, bool userInitiated) {
            if (userInitiated) {
                m_graphUserZoomed = true;
            }
            if (m_graphZoomBtn) {
                m_graphZoomBtn->setText(QString("%1%").arg(qRound(s * 100.0)));
            }
        });
        connect(m_graphScene, &QGraphicsScene::selectionChanged, this, [this]() {
            if (!m_graphScene) {
                return;
            }
            const auto items = m_graphScene->selectedItems();
            for (QGraphicsItem *it : items) {
                if (auto *node = dynamic_cast<CielGraphNode *>(it)) {
                    const QString opId = node->opId();
                    if (!opId.isEmpty() && m_sessionTable) {
                        for (int r = 0; r < m_sessionTable->rowCount(); ++r) {
                            if (m_sessionTable->item(r, 0) && m_sessionTable->item(r, 0)->text() == opId) {
                                m_sessionTable->selectRow(r);
                                showRowJson(r);
                                break;
                            }
                        }
                    }
                    break;
                }
            }
        });

        connect(m_graphZoomOutBtn, &QPushButton::clicked, this, [this]() {
            if (m_graphView) {
                m_graphView->zoomBy(1.0 / 1.18, true);
            }
        });
        connect(m_graphZoomInBtn, &QPushButton::clicked, this, [this]() {
            if (m_graphView) {
                m_graphView->zoomBy(1.18, true);
            }
        });
        connect(m_graphZoomBtn, &QPushButton::clicked, this, [this]() {
            if (m_graphView && m_graphScene) {
                m_graphUserZoomed = true;
                m_graphView->setZoomScale(1.0, true);
                m_graphView->centerOn(m_graphScene->itemsBoundingRect().center());
            }
        });
        connect(m_graphFitBtn, &QPushButton::clicked, this, [this]() {
            m_graphUserZoomed = false;
            fitGraphToViewport();
        });

        return container;
    }

    QWidget *buildOperativeConsoleTab() {
        auto *panel = new QWidget(this);
        panel->setObjectName("ConsolePanel");
        auto *grid = new QGridLayout(panel);
        grid->setContentsMargins(1, 4, 1, 4);
        grid->setVerticalSpacing(4);
        grid->setHorizontalSpacing(0);

        m_operativeConsole = new QTextEdit(panel);
        m_operativeConsole->setReadOnly(true);
        m_operativeConsole->setLineWrapMode(QTextEdit::NoWrap);
        grid->addWidget(m_operativeConsole, 0, 0, 1, 2);

        m_sessionBanner = new QLabel(
            QString("[%1/%2] tempest-runtime/%3 %4 (Linux Keyring & Kerberos CCACHE)")
                .arg(m_localUser, m_localHost, m_localUid, m_localArch),
            panel);
        m_sessionBanner->setObjectName("SessionBanner");
        m_sessionBanner->setTextInteractionFlags(Qt::TextSelectableByMouse);
        grid->addWidget(m_sessionBanner, 1, 0, 1, 2);

        auto *prompt = new QLabel(QString::fromUtf8(">>>"), panel);
        prompt->setObjectName("PromptArrow");
        grid->addWidget(prompt, 2, 0);

        m_operativeInput = new QLineEdit(panel);
        m_operativeInput->setObjectName("OperativeInput");
        m_operativeInput->setPlaceholderText("Command (help, audit, sync, set, souei, watch <path>, chronoa [ccache], renew, ranga, isolate, purge <path>, tanuki <cmd> ...)");
        const QStringList cmds = {
            "help", "audit", "sync", "set", "souei", "watch", "chronoa", "renew",
            "ranga", "isolate", "purge",
            "tanuki doctor", "tanuki fix", "tanuki keytab", "tanuki config",
            "tanuki triage SKEW", "tanuki ladder", "tanuki nhi scan", "tanuki nhi mesh",
            "tanuki token", "tanuki blast", "tanuki adcs", "tanuki pac", "tanuki shadow",
            "tanuki kcm", "tanuki purge", "tanuki skill", "tanuki help",
            "doctor", "fix", "keytab", "config",
            "triage SKEW", "ladder", "nhi", "token", "blast", "adcs",
            "pac", "shadow", "kcm", "skill", "auth", "tgs", "ldap",
            "graph", "table", "clear"
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
        panel->setObjectName("ListenersPanel");
        auto *vbox = new QVBoxLayout(panel);
        vbox->setContentsMargins(8, 6, 8, 8);
        vbox->setSpacing(6);

        m_listenersTable = new QTableWidget(5, 6, panel);
        m_listenersTable->setShowGrid(false);
        m_listenersTable->verticalHeader()->setVisible(false);
        m_listenersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_listenersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_listenersTable->setFocusPolicy(Qt::NoFocus);
        m_listenersTable->setHorizontalHeaderLabels({"Sensor / Listener", "Protocol", "Endpoint / Path", "Scope", "Capability", "Status"});
        m_listenersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        m_listenersTable->verticalHeader()->setDefaultSectionSize(22);

        QString rpcHost = "127.0.0.1";
        quint16 rpcPort = 40056;
        parseHostPort(m_serverAddr, rpcHost, rpcPort);
        const QString rpcScope = (rpcHost == "127.0.0.1" || rpcHost == "localhost" || rpcHost == "::1")
                                     ? "loopback"
                                     : "remote-rpc";

        const QList<QStringList> defaults = {
            {"ciel-teamserver-rpc", "TCP/JSON+Token", m_serverAddr, rpcScope, "auth-enforced", "OFFLINE"},
            {"souei-keyring-sensor", "ProcKeys/SO_PEERCRED", "auto-detect", "uid=" + m_localUid, "inotify", "OFFLINE"},
            {"chronoa-ccache-guard", "MIT-CCACHE-v3/v4", "auto ($KRB5CCNAME)", "RFC4120", "kinit -R", "OFFLINE"},
            {"ranga-keyring-jail", "SYS_keyctl", "session-keyring", "0x3f000000", "NIST-800-88", "OFFLINE"},
            {"tanuki-nhi-engine", "NHI/Kerberos/AD", "/etc/krb5.conf", "18-modules", "local", "OFFLINE"}
        };
        for (int r = 0; r < defaults.size(); ++r) {
            for (int c = 0; c < 6; ++c) {
                auto *it = new QTableWidgetItem(defaults[r][c]);
                it->setTextAlignment(Qt::AlignCenter);
                if (c == 5) {
                    it->setForeground(QColor("#ff5555"));
                }
                m_listenersTable->setItem(r, c, it);
            }
        }
        vbox->addWidget(m_listenersTable, 1);

        // Field Target Configuration Strip (real paths; blank = host auto-discovery)
        auto *cfgLabel = new QLabel("Field Target Paths (leave blank for host auto-discovery):", panel);
        cfgLabel->setObjectName("SectionLabel");
        vbox->addWidget(cfgLabel);

        auto *cfgGrid = new QGridLayout();
        cfgGrid->setHorizontalSpacing(6);
        cfgGrid->setVerticalSpacing(4);

        auto makeBrowseField = [&](int row, int colOffset, const QString &labelText,
                                   const QString &placeholder, QLineEdit *&editOut, const QString &dialogTitle) {
            auto *lbl = new QLabel(labelText, panel);
            editOut = new QLineEdit(panel);
            editOut->setPlaceholderText(placeholder);
            connect(editOut, &QLineEdit::returnPressed, this, [this]() {
                commitTargetFieldsFromEdits();
                runFullSync(false);
            });
            auto *btnBrowse = new QPushButton("Browse", panel);
            btnBrowse->setObjectName("SecondaryBtn");
            btnBrowse->setFixedWidth(68);
            connect(btnBrowse, &QPushButton::clicked, this, [this, editOut, dialogTitle]() {
                QString chosen;
                if (dialogTitle.contains("Socket")) {
                    QFileDialog dlg(this, dialogTitle, editOut->text().trimmed());
                    dlg.setFileMode(QFileDialog::ExistingFile);
                    dlg.setFilter(QDir::AllEntries | QDir::System | QDir::Hidden);
                    dlg.setOption(QFileDialog::DontUseNativeDialog, true);
                    dlg.setOption(QFileDialog::DontResolveSymlinks, true);
                    if (dlg.exec() == QDialog::Accepted && !dlg.selectedFiles().isEmpty()) {
                        chosen = dlg.selectedFiles().first();
                    }
                } else {
                    chosen = QFileDialog::getOpenFileName(this, dialogTitle, editOut->text().trimmed());
                }
                if (!chosen.isEmpty()) {
                    editOut->setText(chosen);
                    commitTargetFieldsFromEdits();
                }
            });
            cfgGrid->addWidget(lbl, row, colOffset);
            cfgGrid->addWidget(editOut, row, colOffset + 1);
            cfgGrid->addWidget(btnBrowse, row, colOffset + 2);
        };

        makeBrowseField(0, 0, "CCACHE:", "Auto ($KRB5CCNAME / /tmp/krb5cc_<uid>)", m_ccacheEdit, "Select Kerberos CCACHE File");
        makeBrowseField(0, 3, "Keytab:", "Auto ($KRB5_KTNAME / /etc/krb5.keytab)", m_keytabEdit, "Select Kerberos Keytab File");
        makeBrowseField(1, 0, "krb5.conf:", "Auto ($KRB5_CONFIG / /etc/krb5.conf)", m_krb5ConfEdit, "Select krb5.conf File");
        makeBrowseField(1, 3, "KCM Socket:", "Auto-discover system KCM socket", m_kcmSockEdit, "Select KCM UNIX Socket");

        auto *applyBtn = new QPushButton("Apply && Sync", panel);
        applyBtn->setToolTip("Apply Field Target Paths and run a full host sync across Tempest + Tanuki");
        auto *clearBtn = new QPushButton("Reset Auto", panel);
        clearBtn->setObjectName("SecondaryBtn");
        clearBtn->setToolTip("Clear custom target paths and revert to host auto-discovery");
        connect(applyBtn, &QPushButton::clicked, this, [this]() { runFullSync(false); });
        connect(clearBtn, &QPushButton::clicked, this, [this]() {
            m_ccacheEdit->clear();
            m_keytabEdit->clear();
            m_krb5ConfEdit->clear();
            m_kcmSockEdit->clear();
            commitTargetFieldsFromEdits();
            runFullSync(false);
        });
        cfgGrid->addWidget(applyBtn, 0, 6);
        cfgGrid->addWidget(clearBtn, 1, 6);
        cfgGrid->setColumnStretch(1, 1);
        cfgGrid->setColumnStretch(4, 1);
        vbox->addLayout(cfgGrid);

        // Tempest Runtime Dispatch Row (7 distinct kernel/CCACHE actions)
        auto *tempestLabel = new QLabel("Tempest Runtime Operatives (Kernel Keyring, CCACHE & Inotify):", panel);
        tempestLabel->setObjectName("SectionLabel");
        vbox->addWidget(tempestLabel);

        auto *tempestRow = new QHBoxLayout();
        tempestRow->setSpacing(6);
        auto addTempestBtn = [&](const QString &label, const QString &tooltip, auto fn, const QString &objName = QString()) {
            auto *btn = new QPushButton(label, panel);
            if (!objName.isEmpty()) {
                btn->setObjectName(objName);
            }
            btn->setToolTip(tooltip);
            connect(btn, &QPushButton::clicked, this, fn);
            tempestRow->addWidget(btn);
        };

        addTempestBtn("Souei Probe", "Audit /proc/keys, session keyring, and host KCM socket (OP-01)", [this]() {
            commitTargetFieldsFromEdits();
            dispatchFromUi("souei_probe", targetParams(), "souei --probe");
        });
        addTempestBtn("Souei Watch...", "Attach Linux inotify file access watch to a real path (OP-01)", [this]() { promptAndWatchFile(); });
        addTempestBtn("Chronoa Audit", "Inspect active CCACHE v3/v4 ticket lifetime & cipher (OP-02)", [this]() {
            commitTargetFieldsFromEdits();
            dispatchFromUi("chronoa_check", targetParams(), "chronoa --ccache");
        });
        addTempestBtn("Renew (kinit -R)", "Execute real kinit -R renewal against active CCACHE (OP-02)", [this]() {
            commitTargetFieldsFromEdits();
            QJsonObject p = targetParams();
            p["renew"] = true;
            dispatchFromUi("chronoa_check", p, "chronoa --renew (kinit -R)");
        });
        addTempestBtn("Ranga Status", "Inspect current process session keyring permissions (OP-03)", [this]() {
            dispatchFromUi("ranga_status", QJsonObject(), "ranga --status");
        });
        addTempestBtn("Ranga Isolate", "Verify 0x3f000000 possessor-only keyring isolation (OP-04)", [this]() {
            dispatchFromUi("ranga_isolate", QJsonObject(), "ranga --isolate");
        });
        addTempestBtn("Ranga Purge File...", "3-pass NIST SP 800-88 wipe & unlink on a single explicit file (OP-05)", [this]() { promptAndPurgeFile(); }, "DangerBtn");
        tempestRow->addStretch();
        vbox->addLayout(tempestRow);

        // Tanuki Identity Engine Dispatch Grid (all 18 Tanuki modules)
        auto *tanukiLabel = new QLabel("Tanuki Identity Engine (NHI, Kerberos & Active Directory):", panel);
        tanukiLabel->setObjectName("SectionLabel");
        vbox->addWidget(tanukiLabel);

        auto *btnGrid = new QGridLayout();
        btnGrid->setHorizontalSpacing(6);
        btnGrid->setVerticalSpacing(5);

        auto addGridBtn = [&](int row, int col, const QString &label, const QString &tooltip, auto fn, const QString &objName = QString()) {
            auto *btn = new QPushButton(label, panel);
            if (!objName.isEmpty()) {
                btn->setObjectName(objName);
            }
            if (!tooltip.isEmpty()) {
                btn->setToolTip(tooltip);
            }
            connect(btn, &QPushButton::clicked, this, fn);
            btnGrid->addWidget(btn, row, col);
        };

        // Row 0 (10): Diagnostics, Keytab, Config, Triage, NHI, KCM, Skill
        addGridBtn(0, 0, "Doctor", "Pre-flight Kerberos, SSSD, and host OPSEC audit", [this]() {
            commitTargetFieldsFromEdits();
            dispatchTanuki("doctor");
        });
        addGridBtn(0, 1, "Fix (Dry-Run)", "Dry-run self-healing remediation check", [this]() {
            commitTargetFieldsFromEdits();
            dispatchTanuki("fix");
        });
        addGridBtn(0, 2, "Keytab...", "Inspect RFC 4120 keytab binary", [this]() {
            commitTargetFieldsFromEdits();
            promptTanuki("Inspect Keytab", "keytab", "Keytab file path:",
                         !m_committedKeytab.isEmpty() ? m_committedKeytab : m_resolvedKeytab);
        });
        addGridBtn(0, 3, "Config...", "Generate hardened zero-DNS krb5.conf", [this]() {
            promptTanuki("Generate krb5.conf", "config", "Arguments (--realm CORP.LOCAL --kdc <IP> [-o path]):", "--realm CORP.LOCAL --kdc ");
        });
        addGridBtn(0, 4, "Triage...", "Lookup Kerberos/SSSD error code", [this]() {
            promptTanuki("Error Triage", "triage", "Error code or query:", "SKEW");
        });
        addGridBtn(0, 5, "Ladder", "Display 5-rung Tactical Decision Ladder", [this]() { dispatchTanuki("ladder"); });
        addGridBtn(0, 6, "NHI Scan", "Scan local filesystem for workload tokens", [this]() { dispatchTanuki("nhi", "scan"); });
        addGridBtn(0, 7, "NHI Mesh", "Probe cloud / SPIFFE workload federation mesh", [this]() { dispatchTanuki("nhi", "mesh"); });
        addGridBtn(0, 8, "KCM Triage", "Inspect/extract local SSSD KCM caches", [this]() { dispatchTanuki("kcm"); });
        addGridBtn(0, 9, "Skill", "Inspect Tanuki capability manifest", [this]() { dispatchTanuki("skill"); });

        // Row 1 (9): AD Analysis, Protocol Decoding, Live Wire & Cache Purge
        addGridBtn(1, 0, "Blast...", "Compute AD privilege tier & blast radius", [this]() {
            commitTargetFieldsFromEdits();
            QString preset;
            if (!m_committedKeytab.isEmpty()) {
                preset = "--keytab " + m_committedKeytab;
            } else if (!m_committedCcache.isEmpty()) {
                preset = "--ccache " + m_committedCcache;
            }
            promptTanuki("AD Blast Radius", "blast", "Principal, --keytab/--ccache, or --host <DC>:", preset);
        });
        addGridBtn(1, 1, "ADCS...", "Scan AD CS templates/certs for ESC1-ESC11", [this]() {
            promptTanuki("ADCS Scanner", "adcs", "Templates JSON/LDIF or X.509 cert path:", QString());
        });
        addGridBtn(1, 2, "PAC...", "Decode MS-PAC privilege buffers", [this]() {
            promptTanuki("PAC Decoder", "pac", "PAC file path or hex/base64:", QString());
        });
        addGridBtn(1, 3, "Shadow...", "Decode msDS-KeyCredentialLink blob", [this]() {
            promptTanuki("Shadow Credentials", "shadow", "KeyCredentialLink file path, hex, or DN-Binary:", QString());
        });
        addGridBtn(1, 4, "Token...", "Validate RFC 8693 workload JWT", [this]() {
            promptTanuki("Workload JWT", "token", "JWT file path or raw JWT:", QString());
        });
        addGridBtn(1, 5, "LDAP...", "Query Active Directory over LDAP/LDAPS", [this]() {
            promptTanuki("AD LDAP Query", "ldap", "Arguments (--host <DC_IP> --realm <REALM> --query all):", "--host  --realm  --query all");
        });
        addGridBtn(1, 6, "Auth (TGT)...", "Acquire Kerberos TGT via AS-REQ pre-auth", [this]() {
            commitTargetFieldsFromEdits();
            promptTanuki("Live Kerberos Auth", "auth", "Arguments (--keytab <path> --principal <p> --kdc <IP>):",
                         !m_committedKeytab.isEmpty()
                             ? ("--keytab " + m_committedKeytab + " --principal ")
                             : "--keytab ");
        });
        addGridBtn(1, 7, "TGS...", "Request Kerberos TGS service ticket", [this]() {
            commitTargetFieldsFromEdits();
            const QString cc = !m_committedCcache.isEmpty() ? m_committedCcache : m_resolvedCCache;
            promptTanuki("Live Kerberos TGS", "tgs", "Arguments (--spn <svc/host@REALM> [--ccache <path>] --kdc <IP>):",
                         cc.isEmpty() ? "--spn " : ("--spn  --ccache " + cc + " --kdc "));
        });
        addGridBtn(1, 8, "Purge Caches...", "Sweep Kerberos caches/temp configs (--all or explicit path) via Tanuki", [this]() {
            promptTanukiPurge();
        }, "DangerBtn");

        vbox->addLayout(btnGrid);
        return panel;
    }

    void promptTanuki(const QString &title, const QString &subcmd, const QString &promptText, const QString &preset) {
        bool ok = false;
        const QString val = QInputDialog::getText(this, title, promptText, QLineEdit::Normal, preset, &ok).trimmed();
        if (!ok || val.isEmpty()) {
            return;
        }
        const QStringList parts = splitCommandLineArgs(val);
        if (parts.size() > 1) {
            dispatchTanuki(subcmd, QString(), parts);
        } else {
            dispatchTanuki(subcmd, val);
        }
    }

    void promptTanukiPurge() {
        bool ok = false;
        const QString val = QInputDialog::getText(
                                this, "Tanuki Credential Cache Purge",
                                "Target credential artifact path or --all (shreds discovered ticket caches & temp configs):",
                                QLineEdit::Normal, "--all", &ok)
                                .trimmed();
        if (!ok || val.isEmpty()) {
            return;
        }
        const auto confirm = QMessageBox::warning(
            this,
            "Confirm Tanuki NIST SP 800-88 Purge",
            QString("Cryptographically shred and unlink target via Tanuki:\n\n%1\n\nThis cannot be undone.").arg(val),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (confirm != QMessageBox::Yes) {
            return;
        }
        const QStringList parts = splitCommandLineArgs(val);
        if (parts.size() > 1) {
            dispatchTanuki("purge", QString(), parts);
        } else {
            dispatchTanuki("purge", val);
        }
    }

    void promptAndWatchFile() {
        commitTargetFieldsFromEdits();
        QString preset = !m_committedCcache.isEmpty() ? m_committedCcache : m_resolvedCCache;
        if (preset.isEmpty()) {
            preset = "/etc/krb5.conf";
        }
        bool ok = false;
        const QString path = QInputDialog::getText(
                                 this, "Souei Inotify Watch",
                                 "Target file path to monitor for open/access/modify:",
                                 QLineEdit::Normal, preset, &ok)
                                 .trimmed();
        if (!ok || path.isEmpty()) {
            return;
        }
        QJsonObject p = targetParams();
        p["watch_path"] = path;
        p["watch_timeout_ms"] = 1000;
        dispatchFromUi("souei_watch", p, "souei --watch-file " + path);
    }

    void promptAndPurgeFile() {
        bool ok = false;
        const QString path = QInputDialog::getText(
                                 this, "Ranga NIST SP 800-88 File Purge",
                                 "Explicit regular file path to 3-pass overwrite and unlink:",
                                 QLineEdit::Normal, QString(), &ok)
                                 .trimmed();
        if (!ok || path.isEmpty()) {
            return;
        }
        const auto confirm = QMessageBox::warning(
            this,
            "Confirm NIST SP 800-88 File Purge",
            QString("Permanently 3-pass overwrite (0x00, 0xFF, random) and unlink:\n\n%1\n\nThis cannot be undone.").arg(path),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (confirm != QMessageBox::Yes) {
            return;
        }
        QJsonObject p;
        p["purge_path"] = path;
        dispatchFromUi("ranga_purge", p, "ranga --purge " + path);
    }

    QString nowTime() const {
        return QDateTime::currentDateTime().toString("HH:mm:ss");
    }

    void appendHtmlLine(QTextEdit *edit, const QString &html) {
        if (!edit) {
            return;
        }
        QScrollBar *vbar = edit->verticalScrollBar();
        QScrollBar *hbar = edit->horizontalScrollBar();
        const bool wasAtBottom = !vbar || vbar->maximum() == 0 || vbar->value() >= vbar->maximum() - 16;
        const int savedV = vbar ? vbar->value() : 0;
        const int savedH = hbar ? hbar->value() : 0;

        QTextCursor cur(edit->document());
        cur.movePosition(QTextCursor::End);
        if (!edit->document()->isEmpty()) {
            cur.insertBlock();
        }
        cur.insertHtml(html);

        if (vbar) {
            if (wasAtBottom) {
                vbar->setValue(vbar->maximum());
            } else {
                vbar->setValue(qBound(0, savedV, vbar->maximum()));
            }
        }
        if (hbar) {
            hbar->setValue(qBound(0, savedH, hbar->maximum()));
        }
    }

    static QString stripLeadingTimestamp(const QString &raw, QString &tsOut) {
        static const QRegularExpression tsRx(QStringLiteral("^(\\d{2}:\\d{2}:\\d{2})\\s+(.*)$"));
        const QRegularExpressionMatch m = tsRx.match(raw);
        if (m.hasMatch()) {
            tsOut = m.captured(1);
            return m.captured(2);
        }
        return raw;
    }

    static QString formatTelemetryHtml(const QString &raw,
                                       const QColor &baseColor = QColor("#f8f8f2"),
                                       const QColor &badgeColor = QColor("#8be9fd"),
                                       bool isAlert = false) {
        const QString baseHex = baseColor.name();
        const QString badgeHex = badgeColor.name();
        const QStringList parts = raw.split(' ', Qt::KeepEmptyParts);
        QStringList out;
        out.reserve(parts.size());

        for (const QString &tok : parts) {
            if (tok.isEmpty()) {
                out.append(QString());
                continue;
            }
            if (tok.startsWith('[') && tok.endsWith(']') && tok.size() > 2) {
                out.append(QString("<span style=\"color:%1\">%2</span>").arg(badgeHex, htmlEsc(tok)));
                continue;
            }
            const int eq = tok.indexOf('=');
            if (eq > 0 && eq + 1 < tok.size()) {
                const QString k = tok.left(eq);
                const QString v = tok.mid(eq + 1);
                QString vColor = baseHex;
                if (k == "status") {
                    vColor = draculaStatusColor(v).name();
                } else if (k == "exit" || k == "rc") {
                    vColor = (v == "0") ? "#50fa7b" : "#ff5555";
                } else if (k == "principal" || k == "target" || k == "path" ||
                           k == "session_keyring" || k == "keyring_id" || k == "enctype") {
                    if (isAlert) {
                        vColor = "#ff5555";
                    } else {
                        vColor = (v == "-" || v == "none") ? "#f1fa8c" : "#8be9fd";
                    }
                } else if (k == "possessor_only") {
                    vColor = (v == "true") ? "#50fa7b" : "#f1fa8c";
                } else if (k == "removed" || k == "renewed") {
                    vColor = (v == "true") ? "#50fa7b" : "#9aa5ce";
                }
                out.append(QString("<span style=\"color:%1\">%2=</span><span style=\"color:%3\">%4</span>")
                               .arg(baseHex, htmlEsc(k), vColor, htmlEsc(v)));
                continue;
            }
            out.append(QString("<span style=\"color:%1\">%2</span>").arg(baseHex, htmlEsc(tok)));
        }
        return out.join(' ');
    }

    void appendEventLog(const QString &tag, const QString &tagColor, const QString &bodyHtml, const QString &tsOverride = QString()) {
        const QString ts = tsOverride.isEmpty() ? nowTime() : tsOverride;
        const QString line = QString(
            "<span style=\"color:#6272a4\">%1</span> "
            "<span style=\"color:%2\">[%3]</span> "
            "<span style=\"color:#f8f8f2\">%4</span>")
                                 .arg(htmlEsc(ts), tagColor, htmlEsc(tag), bodyHtml);
        appendHtmlLine(m_eventViewer, line);
    }

    void appendOperativeBanner() {
        const QString procName = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
        const QString pidStr = QString::number(QCoreApplication::applicationPid());
        const QString banner = QString(
            "<span style=\"color:#6272a4\">%1</span> "
            "<span style=\"color:#f8f8f2\">Operative </span>"
            "<span style=\"color:#ff5555\">OP-01..06</span> "
            "<span style=\"color:#f8f8f2\">authenticated as </span>"
            "<span style=\"color:#bd93f9\">%2\\%3</span> "
            "<span style=\"color:#f8f8f2\">:: [Internal: </span>"
            "<span style=\"color:#8be9fd\">%4</span>"
            "<span style=\"color:#f8f8f2\">] [Process: </span>"
            "<span style=\"color:#ff5555\">%5\\%6</span>"
            "<span style=\"color:#f8f8f2\">] [Arch: </span>"
            "<span style=\"color:#ff79c6\">%7</span>"
            "<span style=\"color:#f8f8f2\">] [Pivot: </span>"
            "<span style=\"color:#8be9fd\">Direct</span>"
            "<span style=\"color:#f8f8f2\">]</span>")
                                   .arg(nowTime(), htmlEsc(m_localHost), htmlEsc(m_localUser),
                                        htmlEsc(m_serverAddr),
                                        htmlEsc(procName.isEmpty() ? "ciel-client" : procName),
                                        htmlEsc(pidStr), htmlEsc(m_localArch));
        appendHtmlLine(m_operativeConsole, banner);
    }

    void appendCommandEcho(const QString &cmd) {
        const QString chevron = QString::fromUtf8("\xc2\xbb");
        const QString line = QString(
            "<br><span style=\"color:#6272a4\">%1 [%2@%3]</span> "
            "<u style=\"color:#ff79c6\">Ciel</u> "
            "<span style=\"color:#8be9fd\">%4</span> "
            "<span style=\"color:#f8f8f2\">%5</span>")
                                 .arg(nowTime(), htmlEsc(m_localUser), htmlEsc(m_localHost), chevron, htmlEsc(cmd));
        appendHtmlLine(m_operativeConsole, line);
    }

    void appendTaskInfo(const QString &msg) {
        QString ts;
        const QString cleanMsg = stripLeadingTimestamp(msg, ts);
        const QString line = QString(
            "<span style=\"color:#8be9fd\">[*] %1</span>")
                                 .arg(htmlEsc(cleanMsg));
        appendHtmlLine(m_operativeConsole, line);
    }

    void appendTaskDone(const QString &msg) {
        QString ts;
        const QString cleanMsg = stripLeadingTimestamp(msg, ts);
        const QString line = QString(
            "<span style=\"color:#50fa7b\">[+]</span> %1")
                                 .arg(formatTelemetryHtml(cleanMsg, QColor("#f8f8f2"), QColor("#50fa7b"), false));
        appendHtmlLine(m_operativeConsole, line);
    }

    void appendTaskWarn(const QString &msg) {
        QString ts;
        const QString cleanMsg = stripLeadingTimestamp(msg, ts);
        const QString line = QString(
            "<span style=\"color:#f1fa8c\">[!]</span> %1")
                                 .arg(formatTelemetryHtml(cleanMsg, QColor("#f8f8f2"), QColor("#f1fa8c"), false));
        appendHtmlLine(m_operativeConsole, line);
    }

    void appendTaskNeutral(const QString &msg) {
        QString ts;
        const QString cleanMsg = stripLeadingTimestamp(msg, ts);
        const QString line = QString(
            "<span style=\"color:#bd93f9\">[^]</span> "
            "<span style=\"color:#f8f8f2\">%1</span>")
                                 .arg(htmlEsc(cleanMsg));
        appendHtmlLine(m_operativeConsole, line);
    }

    void appendTaskAlert(const QString &msg) {
        QString ts;
        const QString cleanMsg = stripLeadingTimestamp(msg, ts);
        const QString line = QString(
            "<span style=\"color:#ff5555\">[!]</span> %1")
                                 .arg(formatTelemetryHtml(cleanMsg, QColor("#ff5555"), QColor("#ff5555"), true));
        appendHtmlLine(m_operativeConsole, line);
    }

    void appendHelpTableToConsole() {
        const QString tableText =
            "\n"
            "  Command         Arguments        Description\n"
            "  -------         ---------        -----------\n"
            "  sync                             Full host sweep across Souei, Chronoa, Ranga, and Tanuki\n"
            "  set             &lt;key&gt; &lt;val&gt;  Set target (ccache|keytab|krb5conf|kcm|clear) or show active\n"
            "  souei                            Probe /proc/keys ACLs, session keyring, and real KCM socket\n"
            "  watch           [path] [ms]      Attach Linux inotify access monitor to real file\n"
            "  chronoa         [ccache_path]    Evaluate MIT CCACHE v3/v4 lifetimes and cipher strength\n"
            "  renew                            Execute real kinit -R renewal against active CCACHE\n"
            "  ranga / isolate [wrapper]        Inspect session keyring or verify 0x3f000000 isolation\n"
            "  purge           &lt;path&gt;           Ranga 3-pass NIST SP 800-88 wipe &amp; unlink on single file\n"
            "  tanuki          &lt;subcmd&gt; ...     Execute any Tanuki NHI/Kerberos/AD module (tanuki help)\n"
            "  doctor / fix / keytab / config   Pre-flight audit, dry-run fix, keytab parse, zero-DNS krb5.conf\n"
            "  triage / ladder / nhi / token    KRB5 errors, 5-rung ladder, NHI scan/mesh, JWT check\n"
            "  blast / adcs / pac / shadow      AD blast radius, ESC1-11 scanner, PAC, shadow credentials\n"
            "  kcm / auth / tgs / ldap / skill  SSSD KCM triage, AS-REQ, TGS, LDAP, skill manifest\n"
            "  audit / graph / table / clear    Print telemetry matrix or switch Sessions / Session Graph\n";
        appendHtmlLine(m_operativeConsole, QString("<pre style=\"color:#f8f8f2; margin:0;\">%1</pre>").arg(tableText));
    }

    void appendDetailedAuditTables() {
        if (m_snapshots.isEmpty()) {
            appendTaskInfo("No live telemetry snapshots recorded yet; run 'sync' to poll teamserver.");
            return;
        }
        appendTaskInfo(QString("Live host telemetry across %1 operative(s):").arg(m_snapshots.size()));
        QString matrix =
            "\n"
            "  ID      OPERATIVE   MODULE              TARGET                             STATUS                DETAIL / AUX\n"
            "  --      ---------   ------              ------                             ------                ------------\n";
        for (auto it = m_snapshots.cbegin(); it != m_snapshots.cend(); ++it) {
            const OperativeSnapshot &s = it.value();
            const QString idCol = s.id.left(6);
            const QString opCol = s.operative.left(9);
            const QString modCol = s.module.left(18);
            const QString targetCol = s.target.left(33);
            const QString auxCombined = QString("%1 | %2 | %3").arg(s.permOrTtl, s.skewOrAux, s.detail);
            const QString statusPadded = s.status.left(20).leftJustified(20, ' ');
            const QString statusColored = QString("<span style=\"color:%1\">%2</span>")
                                              .arg(draculaStatusColor(s.status).name(), htmlEsc(statusPadded));
            matrix += QString("  %1  %2  %3  %4  %5  %6\n")
                          .arg(htmlEsc(idCol.leftJustified(6, ' ')),
                               htmlEsc(opCol.leftJustified(9, ' ')),
                               htmlEsc(modCol.leftJustified(18, ' ')),
                               htmlEsc(targetCol.leftJustified(33, ' ')),
                               statusColored,
                               htmlEsc(auxCombined));
        }
        appendHtmlLine(m_operativeConsole, QString("<pre style=\"color:#f8f8f2; margin:0;\">%1</pre>").arg(matrix));
    }

    void dispatchFromUi(const QString &action, const QJsonObject &params, const QString &echoLabel) {
        appendCommandEcho(echoLabel);
        QJsonObject resp;
        executeAction(action, params, resp, false);
    }

    void dispatchTanuki(const QString &subcmd, const QString &arg = QString(), const QStringList &extraArgs = QStringList()) {
        m_lastExplicitTanukiSubcmd = subcmd.toLower();
        QJsonObject p = targetParams();
        p["subcommand"] = subcmd;
        QString label = "tanuki " + subcmd;
        if (!extraArgs.isEmpty()) {
            p["args"] = QJsonArray::fromStringList(extraArgs);
            label += " " + extraArgs.join(' ');
        } else if (!arg.isEmpty()) {
            p["target_arg"] = arg;
            label += " " + arg;
        }
        dispatchFromUi("tanuki_exec", p, label);
    }

    void handleOperativeCommand(const QString &raw) {
        if (raw.isEmpty()) {
            return;
        }
        appendCommandEcho(raw);
        const QStringList tokens = splitCommandLineArgs(raw);
        if (tokens.isEmpty()) {
            return;
        }
        const QString cmd = tokens[0].toLower();

        if (cmd == "help") {
            appendHelpTableToConsole();
            return;
        }
        if (cmd == "audit") {
            appendDetailedAuditTables();
            return;
        }
        if (cmd == "clear") {
            m_operativeConsole->clear();
            appendOperativeBanner();
            return;
        }
        if (cmd == "graph") {
            m_mainViewTabs->setCurrentIndex(1);
            rebuildSessionGraph();
            return;
        }
        if (cmd == "table") {
            m_mainViewTabs->setCurrentIndex(0);
            return;
        }
        if (cmd == "sync") {
            runFullSync(false);
            return;
        }
        if (cmd == "set") {
            if (tokens.size() == 1) {
                appendTaskNeutral(QString("Targets -> ccache=%1 | keytab=%2 | krb5conf=%3 | kcm=%4")
                                      .arg(m_committedCcache.isEmpty() ? ("auto:" + m_resolvedCCache) : m_committedCcache,
                                           m_committedKeytab.isEmpty() ? ("auto:" + m_resolvedKeytab) : m_committedKeytab,
                                           m_committedKrb5Conf.isEmpty() ? ("auto:" + m_resolvedKrb5Conf) : m_committedKrb5Conf,
                                           m_committedKcmSocket.isEmpty() ? (m_resolvedKcmSocket.isEmpty() ? "auto:none" : "auto:" + m_resolvedKcmSocket) : m_committedKcmSocket));
                return;
            }
            const QString key = tokens[1].toLower();
            const QString val = (tokens.size() >= 3) ? tokens.mid(2).join(' ') : QString();
            if (key == "clear") {
                m_ccacheEdit->clear();
                m_keytabEdit->clear();
                m_krb5ConfEdit->clear();
                m_kcmSockEdit->clear();
                commitTargetFieldsFromEdits();
                appendTaskDone("Cleared custom target overrides (reverted to host auto-discovery; run 'sync' to refresh)");
                return;
            }
            if (key == "ccache") {
                m_ccacheEdit->setText(val);
            } else if (key == "keytab") {
                m_keytabEdit->setText(val);
            } else if (key == "krb5conf" || key == "krb5.conf") {
                m_krb5ConfEdit->setText(val);
            } else if (key == "kcm") {
                m_kcmSockEdit->setText(val);
            } else {
                appendTaskAlert("Usage: set <ccache|keytab|krb5conf|kcm|clear> [path]");
                return;
            }
            commitTargetFieldsFromEdits();
            appendTaskDone(QString("Updated %1 = %2 (run 'sync' to probe new target)").arg(key, val.isEmpty() ? "<auto>" : val));
            return;
        }
        if (cmd == "souei") {
            commitTargetFieldsFromEdits();
            QJsonObject resp;
            executeAction("souei_probe", targetParams(), resp, false);
            return;
        }
        if (cmd == "watch") {
            commitTargetFieldsFromEdits();
            QJsonObject p = targetParams();
            if (tokens.size() >= 2) {
                if (tokens[1].startsWith('-')) {
                    appendTaskAlert("Usage: watch [file_path] [timeout_ms]");
                    return;
                }
                p["watch_path"] = tokens[1];
            }
            if (tokens.size() >= 3) {
                bool okMs = false;
                const int ms = tokens[2].toInt(&okMs);
                if (okMs && ms > 0) {
                    p["watch_timeout_ms"] = qBound(100, ms, 10000);
                }
            }
            QJsonObject resp;
            executeAction("souei_watch", p, resp, false);
            return;
        }
        if (cmd == "chronoa") {
            commitTargetFieldsFromEdits();
            QJsonObject p = targetParams();
            if (tokens.size() >= 2) {
                if (tokens[1].startsWith('-')) {
                    appendTaskAlert("Usage: chronoa [ccache_path]");
                    return;
                }
                p["ccache_path"] = tokens[1];
            }
            QJsonObject resp;
            executeAction("chronoa_check", p, resp, false);
            return;
        }
        if (cmd == "renew") {
            commitTargetFieldsFromEdits();
            QJsonObject p = targetParams();
            p["renew"] = true;
            QJsonObject resp;
            executeAction("chronoa_check", p, resp, false);
            return;
        }
        if (cmd == "ranga") {
            QJsonObject resp;
            executeAction("ranga_status", QJsonObject(), resp, false);
            return;
        }
        if (cmd == "isolate") {
            QJsonObject p;
            if (tokens.size() >= 2 && tokens[1].toLower() == "wrapper") {
                p["isolate_mode"] = "wrapper";
            }
            QJsonObject resp;
            executeAction("ranga_isolate", p, resp, false);
            return;
        }
        if (cmd == "purge") {
            if (tokens.size() < 2 || tokens[1].startsWith('-')) {
                appendTaskAlert("Usage: purge <file_path> [--revoke-session]");
                return;
            }
            QJsonObject p;
            p["purge_path"] = tokens[1];
            if (tokens.contains("--revoke-session")) {
                p["revoke_session"] = true;
            }
            QJsonObject resp;
            executeAction("ranga_purge", p, resp, false);
            return;
        }

        static const QStringList tanukiDirect = {
            "doctor", "fix", "keytab", "config", "triage", "ladder", "nhi",
            "token", "blast", "adcs", "pac", "shadow", "kcm", "skill",
            "auth", "tgs", "ldap"
        };
        if (cmd == "tanuki" || tanukiDirect.contains(cmd)) {
            QString subcmd = "doctor";
            QStringList rest;
            if (cmd == "tanuki") {
                if (tokens.size() >= 2) {
                    subcmd = tokens[1].toLower();
                    rest = tokens.mid(2);
                }
            } else {
                subcmd = cmd;
                rest = tokens.mid(1);
            }
            if (subcmd == "help" || subcmd == "-h" || subcmd == "--help") {
                appendTaskInfo("Tanuki subcommands: doctor, fix, keytab, config, triage, ladder, nhi, token, blast, adcs, pac, shadow, kcm, auth, tgs, ldap, purge, skill");
                return;
            }
            commitTargetFieldsFromEdits();
            m_lastExplicitTanukiSubcmd = subcmd;
            QJsonObject p = targetParams();
            p["subcommand"] = subcmd;
            if (rest.size() == 1 && (!rest[0].startsWith('-') || (subcmd == "purge" && rest[0] == "--all"))) {
                p["target_arg"] = rest[0];
            } else if (!rest.isEmpty()) {
                p["args"] = QJsonArray::fromStringList(rest);
            }
            QJsonObject resp;
            executeAction("tanuki_exec", p, resp, false);
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
        updateContextPaneForSnapshot(snap);

        int targetRow = -1;
        int insertAt = m_sessionTable->rowCount();
        for (int r = 0; r < m_sessionTable->rowCount(); ++r) {
            const QTableWidgetItem *it = m_sessionTable->item(r, 0);
            if (!it) {
                continue;
            }
            if (it->text() == snap.id) {
                targetRow = r;
                break;
            }
            if (insertAt == m_sessionTable->rowCount() && snap.id < it->text()) {
                insertAt = r;
            }
        }
        if (targetRow < 0) {
            targetRow = insertAt;
            m_sessionTable->insertRow(targetRow);
        }

        const QStringList hostParts = m_serverAddr.split(':');
        const QString hostStr = hostParts.value(0, "127.0.0.1");

        QString userCol = m_localUser;
        QString realmCol = m_localHost;
        if (snap.operative == "Chronoa") {
            const int atIdx = snap.target.indexOf('@');
            if (atIdx > 0) {
                userCol = snap.target.left(atIdx);
                realmCol = snap.target.mid(atIdx + 1);
            } else {
                userCol = m_localUser;
                realmCol = "KERBEROS";
            }
        } else if (snap.operative == "Souei" || snap.operative == "Ranga") {
            QString uidText = m_localUid;
            if (snap.rawJson.isObject() && snap.rawJson.toObject().contains("uid")) {
                uidText = QString::number(snap.rawJson.toObject().value("uid").toInt());
            }
            userCol = (uidText != "-" && !uidText.isEmpty())
                          ? QString("%1 (%2)").arg(m_localUser, uidText)
                          : m_localUser;
            realmCol = "linux-kernel";
        } else if (snap.operative == "Tanuki") {
            userCol = "nhi-engine";
            realmCol = m_localHost;
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
        const bool isAlert = isAlertSnapshot(snap);
        const bool isWarn = !isAlert && isWarningLogEntry(QString(), snap.status);
        const QColor col0IconColor = isAlert ? QColor("#ff5555") : (isWarn ? QColor("#f1fa8c") : QColor("#bd93f9"));
        const QColor col0TextColor = isAlert ? QColor("#ff5555") : QColor("#f8f8f2");
        for (int c = 0; c < cells.size(); ++c) {
            QTableWidgetItem *item = m_sessionTable->item(targetRow, c);
            if (!item) {
                item = new QTableWidgetItem();
                item->setTextAlignment(Qt::AlignCenter);
                m_sessionTable->setItem(targetRow, c, item);
            }
            if (item->text() != cells.at(c)) {
                item->setText(cells.at(c));
                item->setToolTip(cells.at(c));
            }
            if (c == 0) {
                item->setIcon(makeOperativeBadgeIcon(col0IconColor));
                item->setForeground(col0TextColor);
            } else if (c == 9) {
                item->setForeground(badgeAccent);
            } else {
                item->setForeground(QColor("#f8f8f2"));
            }
        }
    }

    static bool isAlertSnapshot(const OperativeSnapshot &s) {
        if (s.status == "MISSING" || s.status == "NO_CREDENTIALS") {
            return false;
        }
        return s.status == "ERROR" || s.status == "CRITICAL" ||
               s.status == "POLICY_STOP" || s.status == "REFUSED" ||
               s.status == "CLOCK_SKEW_VIOLATION" || s.status == "EXPIRED" ||
               s.status == "WEAK_ENCTYPE" || s.status == "RENEW_FAILED" ||
               s.status == "CONNECTION_FAILED" || s.status == "BIND_FAILED" ||
               s.status == "PARSE_ERROR" || s.status == "PURGE_FAILED" ||
               s.status == "USAGE_ERROR" || s.status == "UNAUTHORIZED";
    }

    void updateContextPaneForSnapshot(const OperativeSnapshot &snap) {
        QTextEdit *pane = m_contextPanes.value(snap.id, nullptr);
        if (!pane) {
            return;
        }
        QString jsonStr;
        if (snap.rawJson.isObject()) {
            jsonStr = QString::fromUtf8(QJsonDocument(snap.rawJson.toObject()).toJson(QJsonDocument::Indented));
        }
        const QString stableHeader = QString("[%1] %2 :: module=%3 | status=%4 (exit=%5)\n"
                                             "Target   : %6\n"
                                             "Detail   : %7\n"
                                             "Perm/TTL : %8 | Aux: %9\n")
                                         .arg(snap.id, snap.operative, snap.module, snap.status)
                                         .arg(snap.exitCode)
                                         .arg(snap.target, snap.detail, snap.permOrTtl, snap.skewOrAux);
        const QString bodySig = stableHeader + "\n" + jsonStr;
        if (m_lastContextSig.value(snap.id) == bodySig) {
            return;
        }
        m_lastContextSig[snap.id] = bodySig;

        QString summary = stableHeader + QString("Updated  : %1\n").arg(nowTime());
        if (!jsonStr.isEmpty()) {
            summary += "\n--- Live Telemetry JSON ---\n" + jsonStr;
        }
        updateEditorPreservingScroll(pane, summary);
    }

    QString currentGraphSignature() const {
        QString sig = QString("%1|%2|%3|").arg(m_teamserverOnline ? 1 : 0).arg(m_authFailed ? 1 : 0).arg(m_resolvedKcmSocket);
        for (auto it = m_snapshots.cbegin(); it != m_snapshots.cend(); ++it) {
            const OperativeSnapshot &s = it.value();
            const QString stableTarget = (s.id == "OP-04") ? s.permOrTtl : s.target;
            sig += QString("%1:%2:%3:%4:%5;").arg(s.id, s.module, stableTarget, s.detail, s.status);
        }
        return sig;
    }

    void rebuildSessionGraph() {
        m_lastGraphSig = currentGraphSignature();
        if (!m_graphScene || !m_graphView) {
            return;
        }
        QTransform savedTransform;
        QPointF savedCenter;
        const bool keepView = m_graphUserZoomed && !m_graphScene->items().isEmpty();
        if (keepView) {
            savedTransform = m_graphView->transform();
            savedCenter = m_graphView->mapToScene(m_graphView->viewport()->rect().center());
        }

        m_graphScene->clear();

        auto *rootNode = new CielGraphNode(
            CielGraphNode::NodeKind::FirewallHub,
            QString(),
            "ciel-teamserver",
            QString("[%1]").arg(m_serverAddr),
            "hub",
            m_teamserverOnline ? QColor("#8be9fd") : QColor("#ff5555"),
            !m_teamserverOnline && !m_snapshots.isEmpty());
        rootNode->setPos(82.0, 174.0);
        m_graphScene->addItem(rootNode);

        if (m_snapshots.isEmpty()) {
            m_graphScene->setSceneRect(0.0, 0.0, 780.0, 350.0);
            m_graphView->setZoomScale(1.0, false);
            m_graphView->centerOn(390.0, 175.0);
            return;
        }

        auto makeRealMonitor = [&](const OperativeSnapshot &s, const QString &roleTag, const QPointF &pos) -> CielGraphNode * {
            QString targetLabel = s.target;
            if (s.id == "OP-06" && !s.module.isEmpty()) {
                targetLabel = s.module;
            } else {
                const int parenIdx = targetLabel.indexOf(" (");
                if (parenIdx > 0) {
                    targetLabel = targetLabel.left(parenIdx);
                }
                if (targetLabel.contains('/')) {
                    targetLabel = targetLabel.section('/', -1);
                } else if (targetLabel.contains('@')) {
                    targetLabel = targetLabel.section('@', 0, 0);
                }
            }
            if (targetLabel.size() > 16) {
                targetLabel = targetLabel.left(13) + "...";
            }
            const QString line1 = QString("%1 @ %2\\%3").arg(s.id, s.operative.toLower(), targetLabel);
            QString scope = m_localHost;
            QString userLabel = m_localUser;
            if (s.operative == "Chronoa") {
                scope = s.target.contains('@') ? s.target.section('@', 1) : "kerberos";
                if (s.target.contains('@')) {
                    userLabel = s.target.section('@', 0, 0);
                }
            } else if (s.operative == "Souei" || s.operative == "Ranga") {
                scope = "linux-kernel";
            } else if (s.operative == "Tanuki") {
                scope = "nhi-engine";
            }
            if (userLabel.size() > 12) {
                userLabel = userLabel.left(12);
            }
            const QString line2 = QString("[%1\\%2 :: %3]").arg(scope, userLabel, s.status);
            const QColor stCol = draculaStatusColor(s.status);
            const bool elevated = isAlertSnapshot(s);
            const bool muted = (s.status == "MISSING" || s.status == "NO_CREDENTIALS" || s.status == "REVOKED");

            auto *node = new CielGraphNode(
                CielGraphNode::NodeKind::OperativeMonitor,
                s.id,
                line1,
                line2,
                roleTag,
                stCol,
                elevated,
                muted);
            node->setPos(pos);
            m_graphScene->addItem(node);
            return node;
        };

        // Havoc-style Reingold-Tilford left-to-right tree:
        // Root firewall hub (far left x~90) <- green callback arrows from Column 1 (x~330) -> purple/red forward arrows to Column 2 (x~590)
        CielGraphNode *n1 = nullptr;
        CielGraphNode *n2 = nullptr;
        CielGraphNode *n3 = nullptr;
        CielGraphNode *n4 = nullptr;
        CielGraphNode *n5 = nullptr;
        CielGraphNode *n6 = nullptr;

        if (m_snapshots.contains("OP-01")) {
            const OperativeSnapshot &s = m_snapshots.value("OP-01");
            n1 = makeRealMonitor(s, "souei", QPointF(315.0, 52.0));
            const QString edgeLbl = m_resolvedKcmSocket.isEmpty() ? "proc/keys" : "kcm+keys";
            const QColor edgeCol = isAlertSnapshot(s) ? QColor("#ff5555") : QColor("#50fa7b");
            m_graphScene->addItem(new CielGraphEdge(rootNode, n1, edgeCol, edgeLbl, true, 0.30));
        }
        if (m_snapshots.contains("OP-02")) {
            const OperativeSnapshot &s = m_snapshots.value("OP-02");
            n2 = makeRealMonitor(s, "chronoa", QPointF(345.0, 172.0));
            const QString edgeLbl = (s.detail.isEmpty() || s.detail == "-") ? "ccache" : s.detail.section('-', 0, 0);
            const QColor edgeCol = isAlertSnapshot(s) ? QColor("#ff5555") : QColor("#50fa7b");
            m_graphScene->addItem(new CielGraphEdge(rootNode, n2, edgeCol, edgeLbl, true, 0.50));
        }
        if (m_snapshots.contains("OP-06")) {
            const OperativeSnapshot &s = m_snapshots.value("OP-06");
            n6 = makeRealMonitor(s, "tanuki", QPointF(315.0, 294.0));
            const QColor edgeCol = isAlertSnapshot(s) ? QColor("#ff5555") : QColor("#50fa7b");
            m_graphScene->addItem(new CielGraphEdge(rootNode, n6, edgeCol, s.module.isEmpty() ? "nhi-rpc" : s.module, true, 0.52));
        }

        // Column 2 (x~590-650, Y_SEP~115): Derived kernel/filesystem/AD operatives (forward arrows)
        if (m_snapshots.contains("OP-04")) {
            const OperativeSnapshot &s = m_snapshots.value("OP-04");
            n4 = makeRealMonitor(s, "ranga-iso", QPointF(625.0, 48.0));
            CielGraphNode *src = n1 ? n1 : rootNode;
            const QColor edgeCol = isAlertSnapshot(s) ? QColor("#ff5555") : QColor("#bd93f9");
            m_graphScene->addItem(new CielGraphEdge(src, n4, edgeCol, s.module, false, 0.50));
        }
        if (m_snapshots.contains("OP-03")) {
            const OperativeSnapshot &s = m_snapshots.value("OP-03");
            n3 = makeRealMonitor(s, "ranga-shared", QPointF(650.0, 162.0));
            CielGraphNode *src = n1 ? n1 : rootNode;
            const QColor edgeCol = isAlertSnapshot(s) ? QColor("#ff5555") : QColor("#bd93f9");
            m_graphScene->addItem(new CielGraphEdge(src, n3, edgeCol, "keyctl", false, 0.34));
        }
        if (m_snapshots.contains("OP-05")) {
            const OperativeSnapshot &s = m_snapshots.value("OP-05");
            n5 = makeRealMonitor(s, "ranga-purge", QPointF(630.0, 294.0));
            CielGraphNode *src = n2 ? n2 : rootNode;
            const QColor edgeCol = (s.status == "PURGED" || isAlertSnapshot(s)) ? QColor("#ff5555") : QColor("#bd93f9");
            m_graphScene->addItem(new CielGraphEdge(src, n5, edgeCol, "nist-800-88", false, 0.78));
        }

        if (keepView) {
            const QRectF bounds = m_graphScene->itemsBoundingRect().adjusted(-14.0, -10.0, 14.0, -4.0);
            m_graphScene->setSceneRect(bounds);
            m_graphView->setTransform(savedTransform);
            m_graphView->centerOn(savedCenter);
            if (m_graphZoomBtn) {
                m_graphZoomBtn->setText(QString("%1%").arg(qRound(m_graphView->currentScale() * 100.0)));
            }
        } else {
            fitGraphToViewport();
        }
    }

    void fitGraphToViewport() {
        if (!m_graphView || !m_graphScene) {
            return;
        }
        const QRectF bounds = m_graphScene->itemsBoundingRect().adjusted(-14.0, -10.0, 14.0, -4.0);
        m_graphScene->setSceneRect(bounds);
        m_graphView->fitInView(bounds, Qt::KeepAspectRatio);
        const qreal curScale = m_graphView->currentScale();
        if (curScale < 0.90) {
            m_graphView->setZoomScale(0.90, false);
            m_graphView->centerOn(bounds.center());
        } else if (curScale > 1.15) {
            m_graphView->setZoomScale(1.15, false);
            m_graphView->centerOn(bounds.center());
        } else if (curScale >= 0.94 && curScale <= 1.05) {
            m_graphView->setZoomScale(1.0, false);
            m_graphView->centerOn(bounds.center());
        }
        if (m_graphZoomBtn) {
            m_graphZoomBtn->setText(QString("%1%").arg(qRound(m_graphView->currentScale() * 100.0)));
        }
    }

    void updateListenersTable() {
        if (!m_listenersTable || m_listenersTable->rowCount() < 5) {
            return;
        }
        QString rpcHost = "127.0.0.1";
        quint16 rpcPort = 40056;
        parseHostPort(m_serverAddr, rpcHost, rpcPort);
        const QString rpcScope = (rpcHost == "127.0.0.1" || rpcHost == "localhost" || rpcHost == "::1")
                                     ? "loopback"
                                     : "remote-rpc";

        auto setCell = [&](int row, int col, const QString &text, const QColor &colr = QColor("#f8f8f2")) {
            QTableWidgetItem *it = m_listenersTable->item(row, col);
            if (!it) {
                it = new QTableWidgetItem();
                it->setTextAlignment(Qt::AlignCenter);
                m_listenersTable->setItem(row, col, it);
            }
            if (it->text() != text) {
                it->setText(text);
                it->setToolTip(text);
            }
            it->setForeground(colr);
        };

        setCell(0, 2, m_serverAddr);
        setCell(0, 3, rpcScope);
        setCell(0, 4, "auth-enforced");
        if (m_authFailed) {
            setCell(0, 5, "UNAUTHORIZED", QColor("#ff5555"));
        } else {
            setCell(0, 5, m_teamserverOnline ? "ONLINE" : "OFFLINE",
                    m_teamserverOnline ? QColor("#50fa7b") : QColor("#ff5555"));
        }

        setCell(1, 2, m_resolvedKcmSocket.isEmpty() ? "none (passive)" : m_resolvedKcmSocket);
        setCell(1, 3, "uid=" + m_localUid);
        setCell(2, 2, m_resolvedCCache.isEmpty() ? "auto ($KRB5CCNAME)" : m_resolvedCCache);
        setCell(2, 4, m_kinitAvailable ? "kinit -R" : "no-kinit");
        setCell(4, 2, m_resolvedKrb5Conf.isEmpty() ? "/etc/krb5.conf" : m_resolvedKrb5Conf);

        auto syncOpRow = [&](int tableRow, const QString &opId) {
            if (m_authFailed) {
                setCell(tableRow, 5, "UNAUTHORIZED", QColor("#ff5555"));
                return;
            }
            if (!m_teamserverOnline) {
                setCell(tableRow, 5, "OFFLINE", QColor("#ff5555"));
                return;
            }
            if (!m_snapshots.contains(opId)) {
                setCell(tableRow, 5, "PENDING", QColor("#ffb86c"));
                return;
            }
            const OperativeSnapshot &s = m_snapshots.value(opId);
            setCell(tableRow, 5, s.status, draculaStatusColor(s.status));
        };

        syncOpRow(1, "OP-01");
        syncOpRow(2, "OP-02");
        if (m_snapshots.contains("OP-04")) {
            syncOpRow(3, "OP-04");
        } else {
            syncOpRow(3, "OP-03");
        }
        syncOpRow(4, "OP-06");
    }

    void updateTelemetryEnvironment(const QJsonObject &data) {
        if (data.isEmpty()) {
            return;
        }
        bool changed = false;
        const QString u = data.value("user").toString();
        if (!u.isEmpty() && u != m_localUser) {
            m_localUser = u;
            changed = true;
        }
        if (data.contains("uid")) {
            const QString uidStr = QString::number(data.value("uid").toInt());
            if (uidStr != m_localUid) {
                m_localUid = uidStr;
                changed = true;
            }
        }
        const QString h = data.value("hostname").toString();
        if (!h.isEmpty() && h != m_localHost) {
            m_localHost = h;
            changed = true;
        }
        const QString arch = data.value("arch").toString();
        if (!arch.isEmpty() && arch != m_localArch) {
            m_localArch = arch;
            changed = true;
        }
        const QString wd = data.value("work_dir").toString();
        if (!wd.isEmpty()) {
            m_workDir = wd;
        }
        if (data.contains("resolved_ccache")) {
            m_resolvedCCache = data.value("resolved_ccache").toString();
        }
        if (data.contains("resolved_kcm_socket")) {
            m_resolvedKcmSocket = data.value("resolved_kcm_socket").toString();
        }
        if (data.contains("resolved_keytab")) {
            m_resolvedKeytab = data.value("resolved_keytab").toString();
        }
        if (data.contains("resolved_krb5_conf")) {
            m_resolvedKrb5Conf = data.value("resolved_krb5_conf").toString();
        }
        if (data.contains("kinit_available")) {
            m_kinitAvailable = data.value("kinit_available").toBool(false);
        }

        if (changed) {
            if (m_sessionBanner) {
                m_sessionBanner->setText(
                    QString("[%1/%2] tempest-runtime/%3 %4 (Linux Keyring & Kerberos CCACHE)")
                        .arg(m_localUser, m_localHost, m_localUid, m_localArch));
            }
        }
        if (QTextEdit *tsPane = m_contextPanes.value("TEAMSERVER", nullptr)) {
            const QString tsStable = QString(
                "[TEAMSERVER] ciel-teamserver endpoint : %1\n"
                "Operator / Host   : %2@%3 (uid=%4, arch=%5)\n"
                "Workspace         : %6\n"
                "Resolved CCACHE   : %7\n"
                "Resolved Keytab   : %8\n"
                "Resolved krb5.conf: %9\n"
                "Resolved KCM Sock : %10\n"
                "kinit -R Available: %11\n"
                "Auth Enforced     : true (per-session 256-bit token)\n")
                                         .arg(m_serverAddr, m_localUser, m_localHost, m_localUid, m_localArch,
                                              m_workDir,
                                              m_resolvedCCache.isEmpty() ? "none" : m_resolvedCCache,
                                              m_resolvedKeytab.isEmpty() ? "none" : m_resolvedKeytab,
                                              m_resolvedKrb5Conf.isEmpty() ? "none" : m_resolvedKrb5Conf,
                                              m_resolvedKcmSocket.isEmpty() ? "none (passive)" : m_resolvedKcmSocket,
                                              m_kinitAvailable ? "yes" : "no");
            if (m_lastContextSig.value("TEAMSERVER") != tsStable) {
                m_lastContextSig["TEAMSERVER"] = tsStable;
                updateEditorPreservingScroll(tsPane, tsStable + QString("Last Updated      : %1").arg(nowTime()));
            }
        }
    }

    bool probeTcpNonBlocking(const QString &host, quint16 port, int timeoutMs) {
        QTcpSocket probe;
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        bool ok = false;
        connect(&probe, &QTcpSocket::connected, &loop, [&]() {
            ok = true;
            loop.quit();
        });
        connect(&probe, &QTcpSocket::errorOccurred, &loop, [&](QAbstractSocket::SocketError) {
            loop.quit();
        });
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(timeoutMs);
        probe.connectToHost(host, port);
        loop.exec();
        disconnect(&probe, nullptr, nullptr, nullptr);
        probe.abort();
        return ok;
    }

    bool ensureTeamserverRunning() {
        QString host;
        quint16 port = 0;
        if (!parseHostPort(m_serverAddr, host, port)) {
            appendEventLog("!", "#ff5555", "Invalid teamserver address: " + htmlEsc(m_serverAddr));
            return false;
        }

        if (probeTcpNonBlocking(host, port, 180)) {
            if (m_authToken.isEmpty()) {
                m_authToken = loadAuthTokenFromDisk();
            }
            return true;
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

        if (m_authToken.isEmpty()) {
            m_authToken = generateSecureHexToken();
        }

        if (!m_managedServer) {
            m_managedServer = new QProcess(this);
            m_managedServer->setProcessChannelMode(QProcess::MergedChannels);
        }
        if (m_managedServer->state() == QProcess::NotRunning) {
            QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
            env.insert("CIEL_AUTH_TOKEN", m_authToken);
            m_managedServer->setProcessEnvironment(env);
            m_managedServer->start(binPath, {"--addr", m_serverAddr});
            if (!m_managedServer->waitForStarted(2000)) {
                appendEventLog("!", "#ff5555", "Failed launching ciel-teamserver: " + htmlEsc(binPath));
                return false;
            }
        }

        for (int attempt = 0; attempt < 25; ++attempt) {
            if (probeTcpNonBlocking(host, port, 150)) {
                if (!m_loggedManagedBoot) {
                    m_loggedManagedBoot = true;
                    appendEventLog("+", "#50fa7b",
                        QString("Initialized <span style=\"color:#50fa7b\">ciel-teamserver</span> :: <span style=\"color:#8be9fd\">%1</span> (Tempest + Tanuki)")
                            .arg(htmlEsc(m_serverAddr)));
                }
                return true;
            }
            if (m_managedServer->state() == QProcess::NotRunning) {
                appendEventLog("!", "#ff5555", "Managed ciel-teamserver exited before listening");
                return false;
            }
            QEventLoop pauseLoop;
            QTimer::singleShot(100, &pauseLoop, &QEventLoop::quit);
            pauseLoop.exec();
        }
        appendEventLog("!", "#ff5555", "Timed out waiting for managed ciel-teamserver on " + htmlEsc(m_serverAddr));
        return false;
    }

    static bool isAlertLogEntry(const QString &rawLine, const QString &rowStatus, int rowExitCode) {
        if (rowStatus == "MISSING" || rowStatus == "NO_CREDENTIALS" || rowStatus == "N_A") {
            return false;
        }
        if (rowStatus == "CLOCK_SKEW_VIOLATION" || rowStatus == "EXPIRED" ||
            rowStatus == "WEAK_ENCTYPE" || rowStatus == "RENEW_FAILED" ||
            rowStatus == "POLICY_STOP" || rowStatus == "REFUSED" ||
            rowStatus == "CONNECTION_FAILED" || rowStatus == "BIND_FAILED" ||
            rowStatus == "PARSE_ERROR" || rowStatus == "PURGE_FAILED" ||
            rowStatus == "ERROR" || rowStatus == "CRITICAL" ||
            rowStatus == "USAGE_ERROR" || rowStatus == "UNAUTHORIZED") {
            return true;
        }
        if (rawLine.contains("status=CLOCK_SKEW_VIOLATION") || rawLine.contains("status=EXPIRED") ||
            rawLine.contains("status=WEAK_ENCTYPE") || rawLine.contains("status=RENEW_FAILED") ||
            rawLine.contains("status=POLICY_STOP") || rawLine.contains("status=REFUSED") ||
            rawLine.contains("status=CONNECTION_FAILED") || rawLine.contains("status=BIND_FAILED") ||
            rawLine.contains("status=PARSE_ERROR") || rawLine.contains("status=PURGE_FAILED") ||
            rawLine.contains("status=ERROR") || rawLine.contains("status=CRITICAL") ||
            rawLine.contains("status=USAGE_ERROR") || rawLine.contains("status=UNAUTHORIZED")) {
            return true;
        }
        return rowExitCode != 0 && rowStatus != "SHARED_KEYRING" && rowStatus != "FINDINGS";
    }

    static bool isWarningLogEntry(const QString &rawLine, const QString &rowStatus) {
        return rowStatus == "FINDINGS" || rowStatus == "EXPIRING_SOON" ||
               rowStatus == "SHARED_KEYRING" || rowStatus == "WARN" ||
               rowStatus == "ATTENTION" || rowStatus == "HIGH" ||
               rowStatus == "MEDIUM" || rowStatus == "MODERATE" ||
               rowStatus == "MISSING" || rowStatus == "NO_CREDENTIALS" ||
               rowStatus == "RESOURCE_MISSING" || rowStatus == "N_A" ||
               rawLine.contains("status=FINDINGS") ||
               rawLine.contains("status=EXPIRING_SOON") ||
               rawLine.contains("status=SHARED_KEYRING") ||
               rawLine.contains("status=WARN") ||
               rawLine.contains("status=ATTENTION") ||
               rawLine.contains("status=MISSING") ||
               rawLine.contains("status=NO_CREDENTIALS") ||
               rawLine.contains("status=HIGH");
    }

    QString selectedOperativeId() const {
        if (!m_sessionTable) {
            return QString();
        }
        const int row = m_sessionTable->currentRow();
        if (row < 0) {
            return QString();
        }
        const QTableWidgetItem *it = m_sessionTable->item(row, 0);
        return it ? it->text() : QString();
    }

    bool executeAction(const QString &action, const QJsonObject &extraParams, QJsonObject &respOut, bool background = false) {
        if (m_actionInFlight) {
            if (!background && action != "full_sync") {
                appendTaskInfo("Action already in flight; skipping overlapping request: " + action);
            }
            return false;
        }
        BoolScopeGuard inFlightGuard(m_actionInFlight);

        QString host;
        quint16 port = 0;
        if (!parseHostPort(m_serverAddr, host, port)) {
            m_teamserverOnline = false;
            updateListenersTable();
            if (!background) {
                appendEventLog("!", "#ff5555", "Invalid teamserver address: " + htmlEsc(m_serverAddr));
            }
            return false;
        }

        if (!ensureTeamserverRunning()) {
            if (m_teamserverOnline || !background) {
                appendEventLog("!", "#ff5555", "Teamserver unreachable at " + htmlEsc(m_serverAddr));
            }
            m_teamserverOnline = false;
            updateListenersTable();
            return false;
        }

        if (m_authToken.isEmpty()) {
            m_authToken = loadAuthTokenFromDisk();
        }

        QJsonObject req = extraParams;
        req["action"] = action;
        if (!m_authToken.isEmpty()) {
            req["auth_token"] = m_authToken;
        }
        const QByteArray wire = QJsonDocument(req).toJson(QJsonDocument::Compact) + '\n';

        QTcpSocket sock;
        QEventLoop loop;
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);

        QByteArray rx;
        bool gotLine = false;
        bool timedOut = false;
        bool writeErr = false;
        QString sockErr;

        connect(&sock, &QTcpSocket::connected, &loop, [&]() {
            if (sock.write(wire) != wire.size()) {
                writeErr = true;
                sockErr = "TCP write error for action " + action;
                loop.quit();
                return;
            }
            sock.flush();
        });
        connect(&sock, &QTcpSocket::readyRead, &loop, [&]() {
            if (sock.isOpen() && sock.bytesAvailable() > 0) {
                rx.append(sock.readAll());
                if (rx.contains('\n')) {
                    gotLine = true;
                    loop.quit();
                }
            }
        });
        connect(&sock, &QTcpSocket::errorOccurred, &loop, [&](QAbstractSocket::SocketError) {
            if (!gotLine) {
                sockErr = sock.errorString();
                loop.quit();
            }
        });
        connect(&sock, &QTcpSocket::disconnected, &loop, [&]() {
            if (sock.isOpen() && sock.bytesAvailable() > 0) {
                rx.append(sock.readAll());
                if (rx.contains('\n')) {
                    gotLine = true;
                }
            }
            loop.quit();
        });
        connect(&timeoutTimer, &QTimer::timeout, &loop, [&]() {
            timedOut = true;
            loop.quit();
        });

        timeoutTimer.start(20000);
        sock.connectToHost(host, port);
        loop.exec();
        timeoutTimer.stop();
        disconnect(&sock, nullptr, nullptr, nullptr);
        if (sock.state() != QAbstractSocket::UnconnectedState) {
            sock.abort();
        }

        if (timedOut) {
            if (m_teamserverOnline || !background) {
                appendEventLog("!", "#ff5555", "Timeout waiting for teamserver action " + htmlEsc(action));
            }
            m_teamserverOnline = false;
            updateListenersTable();
            return false;
        }
        if (writeErr || !gotLine) {
            const QString msg = sockErr.isEmpty() ? ("Failed RPC for action " + action) : sockErr;
            if (m_teamserverOnline || !background) {
                appendEventLog("!", "#ff5555", htmlEsc(msg));
            }
            m_teamserverOnline = false;
            updateListenersTable();
            return false;
        }

        const int nl = rx.indexOf('\n');
        QJsonParseError parseErr;
        const QJsonDocument doc = QJsonDocument::fromJson(rx.left(nl).trimmed(), &parseErr);
        if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
            m_teamserverOnline = false;
            updateListenersTable();
            appendEventLog("!", "#ff5555", "JSON parse failure: " + htmlEsc(parseErr.errorString()));
            return false;
        }

        respOut = doc.object();
        if (respOut.value("status").toString() == "UNAUTHORIZED") {
            const bool firstAuthFail = !m_authFailed;
            m_authFailed = true;
            m_teamserverOnline = false;
            updateListenersTable();
            if (m_actPoll && m_actPoll->isChecked()) {
                m_actPoll->setChecked(false);
            }
            const QString errMsg = respOut.value("error").toString("Unauthorized RPC token");
            if (firstAuthFail || !background) {
                appendEventLog("!", "#ff5555", "Teamserver auth failed: " + htmlEsc(errMsg));
                if (!background) {
                    appendTaskAlert("Teamserver rejected auth_token: " + errMsg);
                }
            }
            return false;
        }
        m_authFailed = false;

        const bool wasOffline = !m_teamserverOnline;
        m_teamserverOnline = true;
        if (background && wasOffline && m_hasSyncedOnce) {
            appendEventLog("+", "#50fa7b", "Teamserver connection restored on " + htmlEsc(m_serverAddr));
        }
        m_hasSyncedOnce = true;

        updateTelemetryEnvironment(respOut.value("data").toObject());

        const QString prevSelectedId = selectedOperativeId();
        const int prevTableScroll = (m_sessionTable && m_sessionTable->verticalScrollBar())
                                        ? m_sessionTable->verticalScrollBar()->value()
                                        : 0;
        const int prevTableHScroll = (m_sessionTable && m_sessionTable->horizontalScrollBar())
                                         ? m_sessionTable->horizontalScrollBar()->value()
                                         : 0;
        const bool preserveCustomTanuki = background && action == "full_sync" &&
                                          !m_lastExplicitTanukiSubcmd.isEmpty() &&
                                          m_lastExplicitTanukiSubcmd != "doctor";

        const QJsonArray rows = respOut.value("rows").toArray();
        for (const QJsonValue &rv : rows) {
            if (rv.isObject()) {
                const QJsonObject obj = rv.toObject();
                if (preserveCustomTanuki && obj.value("id").toString() == "OP-06") {
                    continue;
                }
                upsertSessionRow(obj);
            }
        }
        if (!prevSelectedId.isEmpty()) {
            for (int r = 0; r < m_sessionTable->rowCount(); ++r) {
                if (m_sessionTable->item(r, 0) && m_sessionTable->item(r, 0)->text() == prevSelectedId) {
                    if (m_sessionTable->currentRow() != r) {
                        QSignalBlocker sb(m_sessionTable);
                        m_sessionTable->selectRow(r);
                    }
                    break;
                }
            }
        }
        if (m_sessionTable && m_sessionTable->verticalScrollBar() &&
            m_sessionTable->verticalScrollBar()->value() != prevTableScroll) {
            m_sessionTable->verticalScrollBar()->setValue(prevTableScroll);
        }
        if (m_sessionTable && m_sessionTable->horizontalScrollBar() &&
            m_sessionTable->horizontalScrollBar()->value() != prevTableHScroll) {
            m_sessionTable->horizontalScrollBar()->setValue(prevTableHScroll);
        }

        const QJsonArray logs = respOut.value("logs").toArray();
        for (int i = 0; i < logs.size(); ++i) {
            const QString rawLine = logs.at(i).toString();
            const QJsonObject rowObj = (i < rows.size() && rows.at(i).isObject())
                                           ? rows.at(i).toObject()
                                           : QJsonObject();
            const QString rowId = rowObj.value("id").toString();
            if (preserveCustomTanuki && rowId == "OP-06") {
                continue;
            }
            const int rowExitCode = rowObj.isEmpty()
                                        ? respOut.value("exit_code").toInt()
                                        : rowObj.value("exit_code").toInt(respOut.value("exit_code").toInt());
            const QString rowStatus = rowObj.isEmpty()
                                          ? respOut.value("status").toString()
                                          : rowObj.value("status").toString();
            const QString rowTarget = rowObj.value("target").toString();
            const QString rowPermOrTtl = rowObj.value("perm_or_ttl").toString();
            const QString stableKey = (rowId == "OP-04") ? rowPermOrTtl : rowTarget;
            const QString stateSig = QString("%1|%2|%3").arg(rowStatus).arg(rowExitCode).arg(stableKey);

            bool emitToEventViewer = !background;
            if (background && !rowId.isEmpty()) {
                if (m_lastEventState.value(rowId) != stateSig) {
                    m_lastEventState[rowId] = stateSig;
                    emitToEventViewer = true;
                }
            } else if (!rowId.isEmpty()) {
                m_lastEventState[rowId] = stateSig;
            }

            QString logTs;
            const QString cleanLine = stripLeadingTimestamp(rawLine, logTs);
            auto buildEventBody = [&](const QColor &opCol, bool alertFlag) -> QString {
                const QString body = formatTelemetryHtml(cleanLine, QColor("#f8f8f2"), opCol, alertFlag);
                if (rowId.isEmpty()) {
                    return body;
                }
                return QString("<span style=\"color:%1\">%2</span> <span style=\"color:#f8f8f2\">::</span> %3")
                    .arg(opCol.name(), htmlEsc(rowId), body);
            };
            if (isAlertLogEntry(rawLine, rowStatus, rowExitCode)) {
                if (emitToEventViewer) {
                    appendEventLog("!", "#ff5555", buildEventBody(QColor("#ff5555"), true), logTs);
                }
                if (!background) {
                    appendTaskAlert(cleanLine);
                }
            } else if (isWarningLogEntry(rawLine, rowStatus)) {
                if (emitToEventViewer) {
                    appendEventLog("WARN", "#f1fa8c", buildEventBody(QColor("#f1fa8c"), false), logTs);
                }
                if (!background) {
                    appendTaskWarn(cleanLine);
                }
            } else if (rowExitCode == 0) {
                if (emitToEventViewer) {
                    appendEventLog("+", "#50fa7b", buildEventBody(QColor("#50fa7b"), false), logTs);
                }
                if (!background) {
                    appendTaskDone(cleanLine);
                }
            } else {
                if (emitToEventViewer) {
                    appendEventLog("*", "#8be9fd", buildEventBody(QColor("#8be9fd"), false), logTs);
                }
                if (!background) {
                    appendTaskInfo(cleanLine);
                }
            }
        }

        updateListenersTable();

        // Preserve user's scroll position and active action payload in Raw JSON during background polls
        if (!background) {
            m_rawShowingExplicitAction = (action != "full_sync");
            const QString srcKey = "ACTION:" + action;
            const QString jsonText = QString::fromUtf8(QJsonDocument(respOut).toJson(QJsonDocument::Indented));
            if (m_rawSourceOpId == srcKey) {
                updateEditorPreservingScroll(m_rawTelemetry, jsonText);
            } else {
                m_rawSourceOpId = srcKey;
                m_rawTelemetry->setPlainText(jsonText);
                if (QScrollBar *vb = m_rawTelemetry->verticalScrollBar()) {
                    vb->setValue(0);
                }
                if (QScrollBar *hb = m_rawTelemetry->horizontalScrollBar()) {
                    hb->setValue(0);
                }
            }
        } else if (!m_rawShowingExplicitAction && !prevSelectedId.isEmpty() &&
                   !(preserveCustomTanuki && prevSelectedId == "OP-06") &&
                   m_snapshots.contains(prevSelectedId) &&
                   m_snapshots.value(prevSelectedId).rawJson.isObject()) {
            const QString jsonText = QString::fromUtf8(
                QJsonDocument(m_snapshots.value(prevSelectedId).rawJson.toObject()).toJson(QJsonDocument::Indented));
            m_rawSourceOpId = prevSelectedId;
            updateEditorPreservingScroll(m_rawTelemetry, jsonText);
        }

        if (m_mainViewTabs->currentIndex() == 1) {
            const QString sig = currentGraphSignature();
            if (sig != m_lastGraphSig) {
                rebuildSessionGraph();
            }
        }
        return true;
    }

    QString m_serverAddr;
    QString m_authToken;
    QString m_tokenFilePath;
    QString m_localUser;
    QString m_localUid;
    QString m_localHost;
    QString m_localArch;
    QString m_workDir;
    QString m_resolvedCCache;
    QString m_resolvedKcmSocket;
    QString m_resolvedKeytab = "/etc/krb5.keytab";
    QString m_resolvedKrb5Conf = "/etc/krb5.conf";
    QString m_committedCcache;
    QString m_committedKeytab;
    QString m_committedKrb5Conf;
    QString m_committedKcmSocket;
    QString m_lastExplicitTanukiSubcmd = "doctor";
    bool m_kinitAvailable = false;
    bool m_actionInFlight = false;
    bool m_teamserverOnline = false;
    bool m_authFailed = false;
    bool m_hasSyncedOnce = false;
    bool m_rawShowingExplicitAction = false;
    bool m_graphUserZoomed = false;
    bool m_initialSplitApplied = false;
    int m_op01TabIdx = 1;
    int m_consoleTabIdx = 3;
    int m_listenersTabIdx = 8;
    QString m_lastGraphSig;
    QString m_rawSourceOpId;

    QSplitter *m_topBotSplitter = nullptr;
    QSplitter *m_topSplit = nullptr;
    QTabWidget *m_mainViewTabs = nullptr;
    QTableWidget *m_sessionTable = nullptr;
    QTableWidget *m_listenersTable = nullptr;
    CielGraphView *m_graphView = nullptr;
    QGraphicsScene *m_graphScene = nullptr;
    QPushButton *m_graphZoomOutBtn = nullptr;
    QPushButton *m_graphZoomBtn = nullptr;
    QPushButton *m_graphZoomInBtn = nullptr;
    QPushButton *m_graphFitBtn = nullptr;

    QTabWidget *m_smallTabs = nullptr;
    QTextEdit *m_eventViewer = nullptr;
    QPlainTextEdit *m_rawTelemetry = nullptr;

    QTabWidget *m_bottomTabs = nullptr;
    QTextEdit *m_operativeConsole = nullptr;
    QLabel *m_sessionBanner = nullptr;
    QLineEdit *m_operativeInput = nullptr;

    QLineEdit *m_ccacheEdit = nullptr;
    QLineEdit *m_keytabEdit = nullptr;
    QLineEdit *m_krb5ConfEdit = nullptr;
    QLineEdit *m_kcmSockEdit = nullptr;

    QTimer m_pollTimer;
    QProcess *m_managedServer = nullptr;
    QAction *m_actPoll = nullptr;
    bool m_loggedManagedBoot = false;
    QMap<QString, OperativeSnapshot> m_snapshots;
    QMap<QString, QString> m_lastEventState;
    QMap<QString, QString> m_lastContextSig;
    QMap<QString, QTextEdit *> m_contextPanes;
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
    const QCommandLineOption tokenOpt("token", "Teamserver RPC auth token", "token");
    const QCommandLineOption tokenFileOpt("token-file", "Path to teamserver 0600 auth token file", "path");
    const QCommandLineOption selfTestOpt("self-test", "Run automated desktop RPC verification and exit");
    const QCommandLineOption shotOpt("screenshot", "Save PNG screenshot of Sessions Table view", "path");
    const QCommandLineOption graphShotOpt("graph-screenshot", "Save PNG screenshot of Session Graph view", "path");
    const QCommandLineOption altShotOpt("alt-screenshot", "Save PNG screenshot of secondary Telemetry/Listeners view", "path");
    const QCommandLineOption consoleShotOpt("console-screenshot", "Save PNG screenshot of Operative Console view", "path");
    parser.addOption(addrOpt);
    parser.addOption(tokenOpt);
    parser.addOption(tokenFileOpt);
    parser.addOption(selfTestOpt);
    parser.addOption(shotOpt);
    parser.addOption(graphShotOpt);
    parser.addOption(altShotOpt);
    parser.addOption(consoleShotOpt);
    parser.process(app);

    CielWindow win(parser.value(addrOpt), parser.value(tokenOpt), parser.value(tokenFileOpt));
    if (!appIcon.isNull()) {
        win.setWindowIcon(appIcon);
    }
    win.show();

    if (parser.isSet(selfTestOpt)) {
        const QString tableShot = parser.value(shotOpt);
        const QString graphShot = parser.value(graphShotOpt);
        const QString altShot = parser.value(altShotOpt);
        const QString consoleShot = parser.value(consoleShotOpt);
        QTimer::singleShot(100, &win, [&win, tableShot, graphShot, altShot, consoleShot]() {
            const int rc = win.runSelfTest(tableShot, graphShot, altShot, consoleShot);
            QApplication::exit(rc);
        });
    } else {
        QTimer::singleShot(100, &win, &CielWindow::startInteractiveSession);
    }

    return app.exec();
}

#include "main.moc"
