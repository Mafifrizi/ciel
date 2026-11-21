#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCompleter>
#include <QCoreApplication>
#include <QDateTime>
#include <QDialog>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QFormLayout>
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
#include <QKeyEvent>
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
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QSslError>
#include <QSslKey>
#include <QSslSocket>
#include <QStackedWidget>
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
        QMainWindow#CielMainWindow, QDialog#ModuleModal, QDialog, QMessageBox, QInputDialog, QFileDialog {
            background-color: #111318;
            color: #F3F4F6;
        }

        QWidget#RootCanvas, QStackedWidget#WorkspaceStack {
            background-color: #111318;
            color: #F3F4F6;
        }

        QWidget#LeftRail {
            background-color: #181B22;
            border-right: 1px solid #2B303C;
        }

        QWidget#TopKpiBar {
            background-color: #181B22;
            border-bottom: 1px solid #2B303C;
        }

        QWidget#VaultBanner {
            background-color: #2A161B;
            border: 1px solid #EF4444;
            border-radius: 3px;
        }

        QWidget#InspectorDrawer {
            background-color: #181B22;
            border-left: 1px solid #2B303C;
        }

        QWidget#InspectorHeader {
            background-color: #20242E;
            border-bottom: 1px solid #2B303C;
        }

        QWidget#ConsolePanel, QWidget#ModulesPanel, QWidget#FleetPanel, QWidget#VaultPanel, QWidget#TargetsPanel {
            background-color: #181B22;
            color: #F3F4F6;
        }

        QWidget#ModuleCard, QWidget#VaultCard, QWidget#PkiCard {
            background-color: #20242E;
            border: 1px solid #2B303C;
            border-radius: 4px;
        }

        QWidget#StatusBarStrip {
            background-color: #181B22;
            border-top: 1px solid #2B303C;
        }

        QLabel {
            background-color: transparent;
            color: #F3F4F6;
        }

        QLabel#MutedLabel, QLabel#StatusStripItem {
            color: #8B949E;
        }

        QLabel#KpiBadge {
            background-color: #20242E;
            color: #F3F4F6;
            border: 1px solid #2B303C;
            border-radius: 3px;
            padding: 2px 8px;
            font-weight: bold;
        }

        QLabel#VaultBannerText {
            color: #FCA5A5;
            font-weight: bold;
        }

        QSplitter::handle {
            background-color: #2B303C;
        }

        QMenuBar {
            background-color: #181B22;
            color: #F3F4F6;
            border-bottom: 1px solid #2B303C;
        }
        QMenuBar::item {
            background-color: transparent;
            color: #F3F4F6;
            padding: 4px 10px;
        }
        QMenuBar::item:selected {
            background-color: #20242E;
            color: #60A5FA;
        }
        QMenuBar::item:pressed {
            background-color: #2B303C;
        }

        QMenu {
            background-color: #181B22;
            color: #F3F4F6;
            border: 1px solid #2B303C;
        }
        QMenu::item {
            padding: 5px 22px 5px 12px;
        }
        QMenu::item:selected {
            background-color: #20242E;
            color: #60A5FA;
        }
        QMenu::separator {
            height: 1px;
            background-color: #2B303C;
            margin: 3px 6px;
        }

        QTabWidget::pane {
            background-color: #181B22;
            border: 1px solid #2B303C;
        }
        QTabBar::tab {
            background-color: #181B22;
            color: #9CA3AF;
            border: 1px solid #2B303C;
            border-bottom: none;
            padding: 5px 14px;
            margin-right: 2px;
        }
        QTabBar::tab:selected {
            background-color: #20242E;
            color: #F3F4F6;
            border-top: 2px solid #3B82F6;
        }

        QTableWidget {
            background-color: #181B22;
            color: #F3F4F6;
            border: 1px solid #2B303C;
            gridline-color: #20242E;
            selection-background-color: #1E3A8A;
            selection-color: #F3F4F6;
        }
        QTableWidget:focus {
            border: 1px solid #3B82F6;
        }
        QTableWidget::item:selected {
            background-color: #1E3A8A;
            color: #F3F4F6;
        }
        QHeaderView::section {
            background-color: #20242E;
            color: #F3F4F6;
            border: none;
            border-bottom: 1px solid #2B303C;
            border-right: 1px solid #2B303C;
            padding: 4px 8px;
            font-weight: bold;
        }

        QTextEdit, QPlainTextEdit, QGraphicsView {
            background-color: #181B22;
            color: #F3F4F6;
            border: none;
        }
        QTextEdit:focus, QPlainTextEdit:focus {
            border: 1px solid #3B82F6;
        }

        QLineEdit, QComboBox {
            background-color: #111318;
            color: #F3F4F6;
            border: 1px solid #2B303C;
            border-radius: 3px;
            padding: 3px 6px;
        }
        QLineEdit:focus, QComboBox:focus {
            border: 1px solid #3B82F6;
        }

        QCheckBox {
            color: #F3F4F6;
            spacing: 6px;
        }

        QLineEdit#OperativeInput {
            background-color: #181B22;
            color: #F3F4F6;
            border: none;
            border-radius: 0px;
            padding: 3px 4px;
        }

        QLabel#SessionBanner {
            background-color: #20242E;
            color: #9CA3AF;
            border-top: 1px solid #2B303C;
            border-bottom: 1px solid #2B303C;
            padding: 2px 6px;
        }

        QLabel#PromptArrow {
            background-color: #181B22;
            color: #60A5FA;
            padding: 2px 4px 2px 6px;
            font-weight: bold;
        }

        QLabel#SectionLabel {
            color: #60A5FA;
            font-weight: bold;
            padding-top: 2px;
        }

        QPushButton, QPushButton#PrimaryBtn {
            background-color: #3B82F6;
            color: #F3F4F6;
            border: 1px solid #3B82F6;
            border-radius: 3px;
            padding: 4px 10px;
            font-weight: bold;
            min-height: 22px;
        }
        QPushButton:hover, QPushButton#PrimaryBtn:hover {
            background-color: #2563EB;
            border-color: #60A5FA;
            color: #FFFFFF;
        }
        QPushButton:pressed, QPushButton#PrimaryBtn:pressed {
            background-color: #1D4ED8;
            color: #FFFFFF;
            border-color: #93C5FD;
        }
        QPushButton:focus {
            border: 1px solid #93C5FD;
        }
        QPushButton:disabled {
            background-color: #1E222D;
            color: #8B949E;
            border-color: #2B303C;
        }

        QPushButton#SecondaryBtn {
            background-color: #20242E;
            color: #F3F4F6;
            border: 1px solid #2B303C;
            padding: 3px 8px;
            font-weight: normal;
        }
        QPushButton#SecondaryBtn:hover {
            background-color: #2B303C;
            border-color: #3B82F6;
            color: #FFFFFF;
        }
        QPushButton#SecondaryBtn:pressed {
            background-color: #181B22;
            color: #F3F4F6;
        }

        QPushButton#DangerBtn {
            background-color: #DC2626;
            color: #FFFFFF;
            border: 1px solid #EF4444;
        }
        QPushButton#DangerBtn:hover {
            background-color: #B91C1C;
            border-color: #FCA5A5;
            color: #FFFFFF;
        }
        QPushButton#DangerBtn:pressed {
            background-color: #991B1B;
            color: #FFFFFF;
        }

        QPushButton#RailBtn {
            background-color: transparent;
            color: #8B949E;
            border: none;
            border-left: 3px solid transparent;
            border-radius: 0px;
            padding: 8px 2px;
            font-size: 10px;
            font-weight: bold;
            min-height: 32px;
        }
        QPushButton#RailBtn:hover {
            background-color: #20242E;
            color: #F3F4F6;
        }
        QPushButton#RailBtn:checked {
            background-color: #20242E;
            color: #60A5FA;
            border-left: 3px solid #3B82F6;
        }

        QScrollBar:vertical {
            background: #181B22;
            width: 10px;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background: #2B303C;
            min-height: 20px;
        }
        QScrollBar:horizontal {
            background: #181B22;
            height: 10px;
            margin: 0;
        }
        QScrollBar::handle:horizontal {
            background: #2B303C;
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
        p.setBrush(QColor("#20242E"));
        p.drawRect(2, 3, 14, 12);
        p.drawLine(2, 7, 16, 7);
    } else {
        p.setPen(QPen(accent, 1.4));
        p.setBrush(QColor("#111318"));
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

QColor cielStatusColor(const QString &status) {
    if (status == "OK" || status == "HEALTHY" || status == "ISOLATED" ||
        status == "RENEWED" || status == "PURGED" || status == "SYNCED" ||
        status == "READY" || status == "ONLINE" || status == "VERIFIED" ||
        status == "SUCCESS" || status == "PASS" || status == "PARSED" ||
        status == "MATCHED" || status == "5_RUNGS" || status == "NONE_DISCOVERED" ||
        status == "CLEAN" || status == "DRY_RUN" || status == "LOW" ||
        status == "EXTRACTED" || status == "UNLOCKED" || status == "ROTATED" ||
        status == "ISSUED" || status == "EXPORTED" || status == "SAVED") {
        return QColor("#10B981");
    }
    if (status == "REVOKED") {
        return QColor("#9CA3AF");
    }
    if (status == "EXPIRING_SOON" || status == "SHARED_KEYRING" ||
        status == "ATTENTION" || status == "WARN" || status == "MEDIUM" ||
        status == "MODERATE" || status == "FINDINGS" || status == "HIGH" ||
        status == "MISSING" || status == "NO_CREDENTIALS" || status == "RESOURCE_MISSING" ||
        status == "N_A" || status == "PENDING" || status == "ROTATING") {
        return QColor("#F59E0B");
    }
    return QColor("#EF4444");
}

inline QColor draculaStatusColor(const QString &status) {
    return cielStatusColor(status);
}

QString formatDotLeader(const QString &key, const QString &val, int width = 22) {
    const int dots = qMax(2, width - static_cast<int>(key.size()));
    return key + QString(dots, QChar('.')) + ": " + (val.isEmpty() ? QStringLiteral("-") : val);
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

QString redactConsoleCommand(const QString &raw) {
    const QStringList tokens = splitCommandLineArgs(raw);
    if (tokens.isEmpty()) {
        return raw;
    }
    const QString cmd = tokens.first().toLower();
    if ((cmd == "unlock" || cmd == "rotate") && tokens.size() >= 2) {
        return tokens.first() + " ********";
    }
    QStringList out;
    out.reserve(tokens.size());
    bool redactNext = false;
    for (int i = 0; i < tokens.size(); ++i) {
        const QString &t = tokens.at(i);
        if (redactNext) {
            out.append("********");
            redactNext = false;
            continue;
        }
        const QString low = t.toLower();
        if (low == "--password" || low == "-p" || low == "--passphrase" || low == "--new-passphrase") {
            out.append(t);
            redactNext = true;
            continue;
        }
        if (low.startsWith("--password=") || low.startsWith("--passphrase=") || low.startsWith("--new-passphrase=")) {
            const int eq = t.indexOf('=');
            out.append(t.left(eq + 1) + "********");
            continue;
        }
        out.append(t);
    }
    return out.join(' ');
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
            paintHubIcon(painter);
        } else {
            paintMonitorIcon(painter);
        }
        painter->setOpacity(1.0);

        QFont f1("DejaVu Sans Mono", 10, QFont::Bold);
        QFont f2("DejaVu Sans Mono", 9, QFont::Bold);
        const QFontMetrics fm2(f2);
        const qreal cardW = m_cardW;

        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(9, 10, 14, 140));
        painter->drawRoundedRect(QRectF(-cardW * 0.5 + 2.0, 43.0, cardW, 36.0), 5.0, 5.0);

        painter->setPen(QPen(QColor("#2B303C"), 1.0));
        painter->setBrush(QColor(24, 27, 34, 235));
        painter->drawRoundedRect(QRectF(-cardW * 0.5, 41.0, cardW, 36.0), 5.0, 5.0);

        painter->setFont(f1);
        painter->setPen(QColor("#F3F4F6"));
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

            painter->setPen(QColor("#9CA3AF"));
            painter->drawText(QPointF(startX, yBase), prefix);
            painter->setPen(m_statusColor);
            painter->drawText(QPointF(startX + w1, yBase), statusPart);
            painter->setPen(QColor("#9CA3AF"));
            painter->drawText(QPointF(startX + w1 + w2, yBase), suffix);
        } else {
            painter->setPen(m_kind == NodeKind::FirewallHub ? QColor("#60A5FA") : m_statusColor);
            painter->drawText(QRectF(-cardW * 0.5 + 4.0, 60.0, cardW - 8.0, 16.0), Qt::AlignCenter, m_line2);
        }

        if (option && (option->state & QStyle::State_Selected)) {
            QPen selPen(QColor("#3B82F6"), 1.6, Qt::DashLine);
            painter->setPen(selPen);
            painter->setBrush(Qt::NoBrush);
            painter->drawRoundedRect(QRectF(-48.0, -44.0, 96.0, 86.0), 6.0, 6.0);
        }
    }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    void paintHubIcon(QPainter *p) const {
        p->setPen(Qt::NoPen);
        p->setBrush(QColor(9, 10, 15, 130));
        p->drawRoundedRect(QRectF(-34.0, -34.0, 72.0, 72.0), 10.0, 10.0);

        QLinearGradient bgGrad(0.0, -36.0, 0.0, 34.0);
        bgGrad.setColorAt(0.0, QColor("#20242E"));
        bgGrad.setColorAt(1.0, QColor("#181B22"));
        p->setPen(QPen(m_statusColor, 1.8));
        p->setBrush(bgGrad);
        p->drawRoundedRect(QRectF(-35.0, -35.0, 70.0, 70.0), 9.0, 9.0);

        // Post-quantum mTLS shield emblem inside server core hub
        QPainterPath shield;
        shield.moveTo(0.0, -24.0);
        shield.lineTo(18.0, -17.0);
        shield.lineTo(16.0, 3.0);
        shield.quadTo(0.0, 21.0, 0.0, 21.0);
        shield.quadTo(0.0, 21.0, -16.0, 3.0);
        shield.lineTo(-18.0, -17.0);
        shield.closeSubpath();

        QLinearGradient shGrad(0.0, -24.0, 0.0, 21.0);
        shGrad.setColorAt(0.0, QColor("#1E3A8A"));
        shGrad.setColorAt(1.0, QColor("#172554"));
        p->setPen(QPen(QColor("#60A5FA"), 1.6));
        p->setBrush(shGrad);
        p->drawPath(shield);

        // Inner key/lock core
        p->setPen(QPen(QColor("#F3F4F6"), 1.8));
        p->setBrush(Qt::NoBrush);
        p->drawArc(QRectF(-5.0, -12.0, 10.0, 10.0), 0, 180 * 16);
        p->setBrush(QColor("#F3F4F6"));
        p->setPen(Qt::NoPen);
        p->drawRoundedRect(QRectF(-7.5, -6.0, 15.0, 11.0), 2.0, 2.0);
        p->setBrush(QColor("#1E3A8A"));
        p->drawEllipse(QPointF(0.0, -1.0), 2.0, 2.0);
    }

    void paintAlertBadge(QPainter *p) const {
        p->setPen(QPen(QColor("#EF4444"), 2.0));
        p->setBrush(QColor("#2A161B"));
        p->drawEllipse(QPointF(30.0, -30.0), 9.0, 9.0);
        QFont bf("DejaVu Sans Mono", 8, QFont::Bold);
        p->setFont(bf);
        p->setPen(QColor("#FCA5A5"));
        p->drawText(QRectF(21.0, -39.0, 18.0, 18.0), Qt::AlignCenter, "!");
    }

    void paintMonitorIcon(QPainter *p) const {
        if (m_mutedState) {
            p->setOpacity(0.76);
        }
        p->setPen(Qt::NoPen);
        p->setBrush(QColor(9, 10, 14, 125));
        p->drawRoundedRect(QRectF(-33.0, -33.0, 72.0, 58.0), 5.0, 5.0);
        p->drawRoundedRect(QRectF(-20.0, 35.0, 45.0, 6.0), 2.5, 2.5);

        const QColor bezelTop = m_mutedState
                                    ? QColor("#2B303C")
                                    : (m_elevatedAlert ? QColor("#451A20") : QColor("#20242E"));
        const QColor chinStand = m_mutedState
                                     ? QColor("#6B7280")
                                     : (m_elevatedAlert ? QColor("#EF4444") : QColor("#9CA3AF"));

        if (m_elevatedAlert) {
            p->setPen(QPen(QColor("#EF4444"), 2.0));
            p->setBrush(Qt::NoBrush);
            p->drawRoundedRect(QRectF(-38.0, -39.0, 76.0, 62.0), 6.0, 6.0);
        }

        QPainterPath stand;
        stand.moveTo(-8.0, 20.0);
        stand.lineTo(8.0, 20.0);
        stand.lineTo(13.0, 34.0);
        stand.lineTo(-13.0, 34.0);
        stand.closeSubpath();
        p->setPen(QPen(QColor("#111318"), 1.1));
        p->setBrush(chinStand.darker(115));
        p->drawPath(stand);
        p->setBrush(chinStand);
        p->drawRoundedRect(QRectF(-22.0, 33.0, 44.0, 6.0), 2.2, 2.2);

        p->setPen(QPen(QColor("#111318"), 1.4));
        p->setBrush(bezelTop);
        p->drawRoundedRect(QRectF(-35.0, -36.0, 70.0, 56.0), 4.5, 4.5);

        p->setPen(QPen(QColor(255, 255, 255, 24), 1.0));
        p->drawLine(QPointF(-31.0, -34.5), QPointF(31.0, -34.5));

        p->setPen(Qt::NoPen);
        p->setBrush(chinStand);
        p->drawRoundedRect(QRectF(-34.0, 10.0, 68.0, 10.0), 2.5, 2.5);
        p->drawRect(QRectF(-34.0, 10.0, 68.0, 5.0));
        p->setBrush(m_statusColor);
        p->drawEllipse(QRectF(-2.5, 13.0, 5.0, 5.0));

        QColor scrTop("#1D4ED8");
        QColor scrBot("#1E3A8A");
        if (m_mutedState) {
            scrTop = QColor("#374151");
            scrBot = QColor("#1F2937");
        } else if (m_elevatedAlert) {
            scrTop = QColor("#991B1B");
            scrBot = QColor("#7F1D1D");
        } else if (m_roleTag == "souei") {
            scrTop = QColor("#B45309");
            scrBot = QColor("#78350F");
        } else if (m_roleTag == "ranga-iso") {
            scrTop = QColor("#047857");
            scrBot = QColor("#064E3B");
        } else if (m_roleTag == "ranga-shared") {
            scrTop = QColor("#92400E");
            scrBot = QColor("#78350F");
        } else if (m_roleTag == "tanuki") {
            scrTop = QColor("#1D4ED8");
            scrBot = QColor("#1E3A8A");
        }
        QLinearGradient scrGrad(0.0, -31.0, 0.0, 6.0);
        scrGrad.setColorAt(0.0, scrTop);
        scrGrad.setColorAt(1.0, scrBot);
        p->setPen(QPen(QColor("#111318"), 1.0));
        p->setBrush(scrGrad);
        p->drawRoundedRect(QRectF(-30.0, -31.0, 60.0, 37.0), 1.5, 1.5);

        p->setPen(Qt::NoPen);
        if (m_roleTag == "chronoa") {
            p->setBrush(QColor("#F3F4F6"));
            p->drawRect(QRectF(-10.0, -22.0, 9.0, 9.0));
            p->drawRect(QRectF(1.0, -22.0, 9.0, 9.0));
            p->drawRect(QRectF(-10.0, -11.0, 9.0, 9.0));
            p->drawRect(QRectF(1.0, -11.0, 9.0, 9.0));
        } else if (m_roleTag == "souei") {
            p->setPen(QPen(QColor("#F3F4F6"), 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p->drawLine(QPointF(-14.0, -18.0), QPointF(-7.0, -12.5));
            p->drawLine(QPointF(-7.0, -12.5), QPointF(-14.0, -7.0));
            p->drawLine(QPointF(-2.0, -7.0), QPointF(10.0, -7.0));
        } else if (m_roleTag == "ranga-iso") {
            p->setPen(QPen(QColor("#F3F4F6"), 2.0));
            p->setBrush(Qt::NoBrush);
            p->drawArc(QRectF(-7.0, -24.0, 14.0, 14.0), 0, 180 * 16);
            p->setBrush(QColor("#F3F4F6"));
            p->setPen(Qt::NoPen);
            p->drawRoundedRect(QRectF(-10.0, -16.0, 20.0, 14.0), 2.0, 2.0);
            p->setBrush(scrBot);
            p->drawEllipse(QRectF(-2.0, -11.0, 4.0, 4.0));
        } else if (m_roleTag == "tanuki") {
            p->setPen(QPen(QColor("#F3F4F6"), 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
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
            p->setPen(QPen(QColor("#F3F4F6"), 2.0));
            p->setBrush(Qt::NoBrush);
            p->drawEllipse(QRectF(-13.0, -20.0, 14.0, 14.0));
            p->drawEllipse(QRectF(-1.0, -16.0, 14.0, 14.0));
        }

        if (m_elevatedAlert) {
            paintAlertBadge(p);
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

        painter->setPen(QPen(QColor(11, 13, 18, 175), 4.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
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
            painter->setPen(QPen(QColor("#2B303C"), 1.0));
            painter->setBrush(QColor(24, 27, 34, 240));
            painter->drawRoundedRect(QRectF(-halfGap, -8.5, txtW, 17.0), 4.0, 4.0);
            painter->setPen(QColor("#F3F4F6"));
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
        grad.setColorAt(0.0, QColor("#181B22"));
        grad.setColorAt(1.0, QColor("#111318"));
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
                        const QString &pkiDirPath = QString(),
                        bool allowClassicalTlsFallback = false,
                        QWidget *parent = nullptr)
        : QMainWindow(parent),
          m_serverAddr(initialAddr),
          m_authToken(explicitToken.trimmed()),
          m_tokenFilePath(tokenFilePath.trimmed()),
          m_pkiDir(pkiDirPath.trimmed()),
          m_allowClassicalTlsFallback(allowClassicalTlsFallback) {
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

        if (m_pkiDir.isEmpty() && !m_tokenFilePath.isEmpty()) {
            const QString siblingPki = QFileInfo(m_tokenFilePath).absolutePath() + "/pki";
            if (QFile::exists(siblingPki + "/ca.crt")) {
                m_pkiDir = siblingPki;
            }
        }
        if (m_pkiDir.isEmpty()) {
            m_pkiDir = m_workDir + "/pki";
        }
        if (m_authToken.isEmpty()) {
            m_authToken = qEnvironmentVariable("CIEL_AUTH_TOKEN").trimmed();
        }

        setObjectName("CielMainWindow");
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

    void keyPressEvent(QKeyEvent *event) override {
        if (event && event->key() == Qt::Key_Escape) {
            if (m_inspectorDrawer && m_inspectorDrawer->isVisible()) {
                m_inspectorDrawer->setVisible(false);
                if (m_sessionTable) {
                    m_sessionTable->setFocus();
                }
                event->accept();
                return;
            }
        }
        QMainWindow::keyPressEvent(event);
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
            const int w = qMax(m_topSplit->width(), qMax(width() - 50, 1430));
            const int leftW = qRound(w * 0.62);
            m_topSplit->setSizes({leftW, w - leftW});
        }
        if (m_topBotSplitter) {
            const int h = qMax(m_topBotSplitter->height(), qMax(height() - 90, 790));
            const int topH = qRound(h * 0.49);
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
            appendEventLog("!", "#EF4444", "SELF-TEST FAIL: " + htmlEsc(msg));
            return 1;
        };

        QJsonObject pingResp;
        if (!executeAction("ping", QJsonObject(), pingResp, false) ||
            !pingResp.value("ok").toBool(false)) {
            return fail("ping RPC failed");
        }
        const QJsonObject pingData = pingResp.value("data").toObject();
        if (!pingData.value("mtls_enforced").toBool(false) ||
            !pingData.value("pqc_group").toString().contains("X25519MLKEM768")) {
            return fail("Expected mTLS 1.3 X25519MLKEM768 enforcement in ping response");
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
        appendCommandEcho("[GUI -> CLI] souei --watch-file " + watchTarget);
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
        appendCommandEcho("[GUI -> CLI] ranga --isolate 0x3f000000");
        if (!executeAction("ranga_isolate", QJsonObject(), isoResp, false)) {
            return fail("ranga_isolate RPC failed");
        }
        const QJsonObject isoData = isoResp.value("data").toObject();
        if (isoResp.value("exit_code").toInt(-1) != 0 ||
            !isoData.value("possessor_only").toBool(false) ||
            isoData.value("perm_hex").toString() != "3f000000") {
            return fail("Expected possessor_only=true and perm_hex=3f000000");
        }

        // Verify Ranga purge rejects empty target (OP-05 alert row -> #EF4444) and succeeds on temp artifact
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
                    sawAlertIdColor = (m_sessionTable->item(r, 0)->foreground().color() == QColor("#EF4444"));
                    break;
                }
            }
            if (!sawAlertIdColor) {
                return fail("Expected alert OP-05 row ID cell to use #EF4444 foreground");
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
        appendCommandEcho("[GUI -> CLI] ranga --purge " + purgeTarget);
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
            appendCommandEcho("[GUI -> CLI] tanuki " + tCmd);
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

        // Verify InfinityCurve identity runtime banner, secret redaction, and absence of stale Havoc holdovers
        {
            const QString bannerPlain = m_operativeConsole->toPlainText();
            if (!bannerPlain.contains("Ciel Identity Runtime Console") ||
                !bannerPlain.contains("mTLS 1.3") ||
                bannerPlain.contains("[Pivot: Direct]") ||
                bannerPlain.contains("Operative OP-01..06 authenticated as")) {
                return fail("Operative Console banner missing InfinityCurve header or still contains stale Havoc text");
            }
            handleOperativeCommand("set");
            if (!m_operativeConsole->toPlainText().contains("[^] Targets ->")) {
                return fail("Operative Console 'set' command did not emit [^] neutral line");
            }
            // Verify passphrase redaction on unlock/rotate command echo
            const QString redactedUnlock = redactConsoleCommand("unlock MyTopSecretPassphrase99!");
            const QString redactedRotate = redactConsoleCommand("rotate MyRotatedSecretPass99!");
            const QString redactedLdap = redactConsoleCommand("tanuki ldap --host 10.0.0.1 -p SecretLdapPw!");
            if (redactedUnlock != "unlock ********" ||
                redactedRotate != "rotate ********" ||
                redactedLdap.contains("SecretLdapPw!")) {
                return fail("Console secret redaction failed");
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
        switchWorkspaceMode(0);
        if (m_inspectorDrawer) {
            m_inspectorDrawer->setVisible(true);
        }
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

        // Verify Inspector Summary pane (dot-leader view) preserves pixel scroll across background polls
        m_smallTabs->setCurrentIndex(0);
        QApplication::processEvents();
        if (m_inspectorSummary) {
            if (!m_inspectorSummary->toPlainText().contains("...: ")) {
                return fail("Inspector Summary missing dot-leader key-value formatting");
            }
            if (QScrollBar *ctxVb = m_inspectorSummary->verticalScrollBar()) {
                if (ctxVb->maximum() > 0) {
                    const int targetCtxScroll = qMin(80, ctxVb->maximum());
                    ctxVb->setValue(targetCtxScroll);
                    QApplication::processEvents();
                    const int ctxTopBefore = m_inspectorSummary->cursorForPosition(QPoint(8, 8)).blockNumber();
                    QJsonObject bgCtxResp;
                    if (!executeAction("full_sync", targetParams(), bgCtxResp, true)) {
                        return fail("background inspector scroll-check full_sync failed");
                    }
                    QApplication::processEvents();
                    const int ctxTopAfter = m_inspectorSummary->cursorForPosition(QPoint(8, 8)).blockNumber();
                    if (ctxVb->value() != targetCtxScroll || ctxTopAfter != ctxTopBefore) {
                        return fail(QString("Background poll jumped Inspector scroll (%1->%2, block %3->%4)")
                                        .arg(targetCtxScroll).arg(ctxVb->value())
                                        .arg(ctxTopBefore).arg(ctxTopAfter));
                    }
                }
            }
        }

        // Verify Escape key closes Inspector Drawer and clicking a row re-opens it
        {
            QKeyEvent escEv(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
            QApplication::sendEvent(this, &escEv);
            if (m_inspectorDrawer && m_inspectorDrawer->isVisible()) {
                return fail("Escape key did not close Inspector Drawer");
            }
            m_inspectorDrawer->setVisible(true);
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
                "chronoa", QColor("#EF4444"), true, false);
            auto *revokedNode = new CielGraphNode(
                CielGraphNode::NodeKind::OperativeMonitor, "OP-98",
                "OP-98 @ ranga\\ses", "[linux-kernel\\svc :: REVOKED]",
                "ranga-iso", QColor("#9CA3AF"), false, true);
            alertNode->setPos(160.0, 100.0);
            revokedNode->setPos(460.0, 160.0);
            checkScene.addItem(alertNode);
            checkScene.addItem(revokedNode);
            checkScene.addItem(new CielGraphEdge(alertNode, revokedNode, QColor("#EF4444"), "skew-alert", false));
            QImage offImg(640, 280, QImage::Format_ARGB32);
            offImg.fill(QColor("#181B22"));
            QPainter offP(&offImg);
            checkScene.render(&offP);
            offP.end();
            if (offImg.pixelColor(160, 100) == QColor("#181B22")) {
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
            switchWorkspaceMode(0);
            m_mainViewTabs->setCurrentIndex(0);
            m_smallTabs->setCurrentIndex(0);
            m_bottomTabs->setCurrentIndex(m_eventTabIdx);
            m_sessionTable->selectRow(0);
            showRowJson(0);
            QApplication::processEvents();
            QApplication::processEvents();
            const QPixmap tableShot = grab();
            if (tableShot.isNull() || !tableShot.save(screenshotPath, "PNG")) {
                return fail("Failed saving session table screenshot");
            }
        }

        if (!altScreenshotPath.isEmpty()) {
            switchWorkspaceMode(1);
            m_bottomTabs->setCurrentIndex(m_consoleTabIdx);
            QApplication::processEvents();
            QApplication::processEvents();
            const QPixmap altShot = grab();
            if (altShot.isNull() || !altShot.save(altScreenshotPath, "PNG")) {
                return fail("Failed saving alternate screenshot");
            }
            switchWorkspaceMode(0);
        }

        if (!consoleScreenshotPath.isEmpty()) {
            switchWorkspaceMode(0);
            m_mainViewTabs->setCurrentIndex(0);
            m_smallTabs->setCurrentIndex(1);
            m_sessionTable->selectRow(0);
            showRowJson(0);
            m_bottomTabs->setCurrentIndex(m_consoleTabIdx);
            QApplication::processEvents();
            QApplication::processEvents();
            const QPixmap conShot = grab();
            if (conShot.isNull() || !conShot.save(consoleScreenshotPath, "PNG")) {
                return fail("Failed saving console screenshot");
            }
        }

        if (!graphScreenshotPath.isEmpty()) {
            switchWorkspaceMode(0);
            m_mainViewTabs->setCurrentIndex(1);
            m_smallTabs->setCurrentIndex(0);
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
        if (executeAction("full_sync", targetParams(), resp, background)) {
            maybeLoadProfileFromDb();
        }
    }

private:
    void switchWorkspaceMode(int modeIdx) {
        if (!m_workspaceStack) {
            return;
        }
        const int clamped = qBound(0, modeIdx, m_workspaceStack->count() - 1);
        m_workspaceStack->setCurrentIndex(clamped);
        for (int i = 0; i < m_railBtns.size(); ++i) {
            if (m_railBtns.at(i)) {
                m_railBtns.at(i)->setChecked(i == clamped);
            }
        }
        if (clamped == 0 && m_mainViewTabs && m_mainViewTabs->currentIndex() == 1) {
            rebuildSessionGraph();
        }
    }

    void maybeLoadProfileFromDb() {
        if (!m_dbConnected || m_vaultLocked || m_profileLoadedFromDb || m_actionInFlight) {
            return;
        }
        m_profileLoadedFromDb = true;
        const bool allEmpty =
            (!m_ccacheEdit || m_ccacheEdit->text().trimmed().isEmpty()) &&
            (!m_keytabEdit || m_keytabEdit->text().trimmed().isEmpty()) &&
            (!m_krb5ConfEdit || m_krb5ConfEdit->text().trimmed().isEmpty()) &&
            (!m_kcmSockEdit || m_kcmSockEdit->text().trimmed().isEmpty());
        if (!allEmpty) {
            return;
        }
        QJsonObject profResp;
        if (executeAction("profile_get", QJsonObject(), profResp, true) &&
            profResp.value("ok").toBool(false)) {
            const QJsonObject d = profResp.value("data").toObject();
            if (d.value("found").toBool(false)) {
                if (m_ccacheEdit) m_ccacheEdit->setText(d.value("ccache_path").toString());
                if (m_keytabEdit) m_keytabEdit->setText(d.value("keytab_path").toString());
                if (m_krb5ConfEdit) m_krb5ConfEdit->setText(d.value("krb5_conf_path").toString());
                if (m_kcmSockEdit) m_kcmSockEdit->setText(d.value("kcm_socket").toString());
                commitTargetFieldsFromEdits();
            }
        }
    }

    void saveProfileIfDbConnected() {
        if (!m_dbConnected || m_vaultLocked) {
            return;
        }
        QJsonObject profResp;
        (void)executeAction("profile_save", targetParams(), profResp, true);
    }

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
        QAction *actAuditMatrix = mCiel->addAction("Run Host Audit Matrix (audit)");
        QAction *actHelpCmd = mCiel->addAction("Print Console Commands");
        QAction *actClearConsole = mCiel->addAction("Clear Console");
        mCiel->addSeparator();
        QAction *actExit = mCiel->addAction("Exit");
        connect(actSync, &QAction::triggered, this, [this]() { runFullSync(false); });
        connect(m_actPoll, &QAction::toggled, this, [this](bool on) {
            if (on) {
                m_pollTimer.start(3000);
                appendEventLog("*", "#60A5FA", "Background telemetry monitor enabled (3000ms, diff-only)");
            } else {
                m_pollTimer.stop();
                appendEventLog("*", "#60A5FA", "Background telemetry monitor paused");
            }
            updateStatusStrip();
        });
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
        connect(actExit, &QAction::triggered, this, &QWidget::close);

        QMenu *mView = mb->addMenu("View");
        QAction *actOpsMode = mView->addAction("Workspace -> Operations (Table / Graph)");
        QAction *actModMode = mView->addAction("Workspace -> Modules Launcher");
        QAction *actFltMode = mView->addAction("Workspace -> Fleet & History");
        QAction *actVltMode = mView->addAction("Workspace -> Shuna Vault & PKI");
        QAction *actCfgMode = mView->addAction("Workspace -> Targets & Sensors");
        mView->addSeparator();
        QAction *actTable = mView->addAction("Operations View -> Table");
        QAction *actGraph = mView->addAction("Operations View -> Session Graph");
        mView->addSeparator();
        QAction *actInspector = mView->addAction("Toggle Inspector Drawer");
        QAction *actTeamserver = mView->addAction("Inspect Teamserver Status");
        QAction *actViewConsole = mView->addAction("Operative Console");
        QAction *actEvents = mView->addAction("Event Viewer");

        connect(actOpsMode, &QAction::triggered, this, [this]() { switchWorkspaceMode(0); });
        connect(actModMode, &QAction::triggered, this, [this]() { switchWorkspaceMode(1); });
        connect(actFltMode, &QAction::triggered, this, [this]() { switchWorkspaceMode(2); });
        connect(actVltMode, &QAction::triggered, this, [this]() { switchWorkspaceMode(3); });
        connect(actCfgMode, &QAction::triggered, this, [this]() { switchWorkspaceMode(4); });
        connect(actTable, &QAction::triggered, this, [this]() {
            switchWorkspaceMode(0);
            m_mainViewTabs->setCurrentIndex(0);
        });
        connect(actGraph, &QAction::triggered, this, [this]() {
            switchWorkspaceMode(0);
            m_mainViewTabs->setCurrentIndex(1);
            rebuildSessionGraph();
        });
        connect(actInspector, &QAction::triggered, this, [this]() {
            if (m_inspectorDrawer) {
                m_inspectorDrawer->setVisible(!m_inspectorDrawer->isVisible());
            }
        });
        connect(actTeamserver, &QAction::triggered, this, [this]() {
            switchWorkspaceMode(0);
            showTeamserverInInspector();
        });
        connect(actViewConsole, &QAction::triggered, this, [this]() {
            m_bottomTabs->setCurrentIndex(m_consoleTabIdx);
            if (m_operativeInput) {
                m_operativeInput->setFocus();
            }
        });
        connect(actEvents, &QAction::triggered, this, [this]() {
            m_bottomTabs->setCurrentIndex(m_eventTabIdx);
        });

        QMenu *mOps = mb->addMenu("Operatives");
        QAction *actSouei = mOps->addAction("Souei: Probe /proc/keys & KCM");
        QAction *actWatch = mOps->addAction("Souei: Watch File (Inotify)...");
        QAction *actChronoa = mOps->addAction("Chronoa: Audit Active CCACHE...");
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
        connect(actWatch, &QAction::triggered, this, [this]() { openTempestModuleModal("souei_watch"); });
        connect(actChronoa, &QAction::triggered, this, [this]() { openTempestModuleModal("chronoa_check"); });
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
        connect(actPurge, &QAction::triggered, this, [this]() { openTempestModuleModal("ranga_purge"); });

        QMenu *mTanuki = mb->addMenu("Tanuki");
        auto addTanukiModalItem = [&](const QString &label, const QString &subcmd) {
            QAction *act = mTanuki->addAction(label);
            connect(act, &QAction::triggered, this, [this, subcmd]() {
                commitTargetFieldsFromEdits();
                openTanukiModuleModal(subcmd);
            });
        };
        addTanukiModalItem("Doctor: Pre-Flight & OPSEC Audit...", "doctor");
        addTanukiModalItem("Fix: Self-Healing Engine (Dry-Run / Apply)...", "fix");
        addTanukiModalItem("Keytab: Inspect RFC 4120 Binary...", "keytab");
        addTanukiModalItem("Config: Generate Zero-DNS krb5.conf...", "config");
        mTanuki->addSeparator();
        addTanukiModalItem("Triage: Error Code Lookup...", "triage");
        addTanukiModalItem("Ladder: 5-Rung Decision Ladder", "ladder");
        addTanukiModalItem("NHI: Scan / Mesh / Inspect / Live Exchange...", "nhi");
        addTanukiModalItem("Token: Validate Workload JWT...", "token");
        mTanuki->addSeparator();
        addTanukiModalItem("Blast: AD Privilege & Tier Mapper...", "blast");
        addTanukiModalItem("ADCS: Scan Templates (ESC1-11)...", "adcs");
        addTanukiModalItem("PAC: Decode MS-PAC Privileges...", "pac");
        addTanukiModalItem("Shadow: Inspect msDS-KeyCredentialLink...", "shadow");
        addTanukiModalItem("KCM: Triage Local SSSD Caches...", "kcm");
        mTanuki->addSeparator();
        addTanukiModalItem("Auth: Live AS-REQ Pre-Auth...", "auth");
        addTanukiModalItem("TGS: Live Service Ticket Request...", "tgs");
        addTanukiModalItem("LDAP: AD SPN / Delegation Query (LDAPS)...", "ldap");
        QAction *actTanukiPurge = mTanuki->addAction("Purge: Wipe Kerberos Caches / Artifact...");
        connect(actTanukiPurge, &QAction::triggered, this, [this]() { promptTanukiPurge(); });
        addTanukiModalItem("Skill: Inspect Tanuki Manifest", "skill");

        QMenu *mHelp = mb->addMenu("Help");
        QAction *actAbout = mHelp->addAction("About Ciel");
        connect(actAbout, &QAction::triggered, this, [this]() {
            QMessageBox::information(this, "About Ciel",
                                     "Ciel Identity Runtime Console v1.0.0\n"
                                     "Stack: C++20 Qt6 Widgets + Go Teamserver (mTLS 1.3 X25519MLKEM768)\n"
                                     "Ecosystem: Tanuki + Tempest + Shuna + Ciel");
        });
    }

    void buildCentralCielLayout() {
        auto *root = new QWidget(this);
        root->setObjectName("RootCanvas");
        auto *outerVBox = new QVBoxLayout(root);
        outerVBox->setContentsMargins(0, 0, 0, 0);
        outerVBox->setSpacing(0);

        auto *bodyHBox = new QHBoxLayout();
        bodyHBox->setContentsMargins(0, 0, 0, 0);
        bodyHBox->setSpacing(0);

        // 1. Slim Left Activity Rail (InfinityCurve shell)
        auto *rail = new QWidget(root);
        rail->setObjectName("LeftRail");
        rail->setFixedWidth(54);
        auto *railLay = new QVBoxLayout(rail);
        railLay->setContentsMargins(0, 6, 0, 6);
        railLay->setSpacing(4);

        auto addRailButton = [&](int idx, const QString &shortText, const QString &tooltip) {
            auto *btn = new QPushButton(shortText, rail);
            btn->setObjectName("RailBtn");
            btn->setCheckable(true);
            btn->setToolTip(tooltip);
            connect(btn, &QPushButton::clicked, this, [this, idx]() { switchWorkspaceMode(idx); });
            m_railBtns.append(btn);
            return btn;
        };

        railLay->addWidget(addRailButton(0, "OPS", "Operations: Live Operatives Matrix, Session Graph & Inspector"));
        railLay->addWidget(addRailButton(1, "MOD", "Modules: 4-Cycle Tanuki & Tempest Module Launcher"));
        railLay->addWidget(addRailButton(2, "FLT", "Fleet & History: Decrypted PostgreSQL Hosts & Audit Timeline"));
        railLay->addWidget(addRailButton(3, "VLT", "Vault & PKI: Shuna Vault Lockout/Rotation/Backup & PQC mTLS PKI"));
        railLay->addStretch(1);
        railLay->addWidget(addRailButton(4, "CFG", "Targets & Sensors: Field Target Paths & Transport Sensors"));
        if (!m_railBtns.isEmpty()) {
            m_railBtns.first()->setChecked(true);
        }
        bodyHBox->addWidget(rail);

        // 2. Main Right Column (Top KPI Bar + Vault Alert Banner + Vertical Splitter [Workspace | 2-Tab Console Dock])
        auto *mainCol = new QVBoxLayout();
        mainCol->setContentsMargins(4, 4, 4, 4);
        mainCol->setSpacing(4);

        // Top KPI & Filter Bar
        auto *kpiBar = new QWidget(root);
        kpiBar->setObjectName("TopKpiBar");
        auto *kpiLay = new QHBoxLayout(kpiBar);
        kpiLay->setContentsMargins(8, 4, 8, 4);
        kpiLay->setSpacing(8);

        m_kpiTransportBadge = new QLabel("mTLS 1.3 :: X25519MLKEM768", kpiBar);
        m_kpiTransportBadge->setObjectName("KpiBadge");
        m_kpiVaultBadge = new QLabel("Vault: STANDALONE", kpiBar);
        m_kpiVaultBadge->setObjectName("KpiBadge");
        m_kpiOpsBadge = new QLabel("Operatives: 0", kpiBar);
        m_kpiOpsBadge->setObjectName("KpiBadge");
        m_kpiFreshnessLabel = new QLabel("Sync: waiting", kpiBar);
        m_kpiFreshnessLabel->setObjectName("MutedLabel");

        m_filterEdit = new QLineEdit(kpiBar);
        m_filterEdit->setObjectName("FilterInput");
        m_filterEdit->setPlaceholderText("Filter operatives by ID, module, target, or status...");
        m_filterEdit->setMinimumWidth(220);
        connect(m_filterEdit, &QLineEdit::textChanged, this, [this](const QString &txt) {
            applySessionTableFilter(txt);
        });

        auto *btnTableMode = new QPushButton("Table", kpiBar);
        btnTableMode->setObjectName("SecondaryBtn");
        connect(btnTableMode, &QPushButton::clicked, this, [this]() {
            switchWorkspaceMode(0);
            m_mainViewTabs->setCurrentIndex(0);
        });
        auto *btnGraphMode = new QPushButton("Graph", kpiBar);
        btnGraphMode->setObjectName("SecondaryBtn");
        connect(btnGraphMode, &QPushButton::clicked, this, [this]() {
            switchWorkspaceMode(0);
            m_mainViewTabs->setCurrentIndex(1);
            rebuildSessionGraph();
        });
        auto *btnInspectToggle = new QPushButton("Inspector", kpiBar);
        btnInspectToggle->setObjectName("SecondaryBtn");
        connect(btnInspectToggle, &QPushButton::clicked, this, [this]() {
            switchWorkspaceMode(0);
            if (m_inspectorDrawer) {
                m_inspectorDrawer->setVisible(!m_inspectorDrawer->isVisible());
            }
        });
        auto *btnSyncNow = new QPushButton("Sync Now", kpiBar);
        btnSyncNow->setObjectName("PrimaryBtn");
        connect(btnSyncNow, &QPushButton::clicked, this, [this]() { runFullSync(false); });

        kpiLay->addWidget(m_kpiTransportBadge);
        kpiLay->addWidget(m_kpiVaultBadge);
        kpiLay->addWidget(m_kpiOpsBadge);
        kpiLay->addWidget(m_kpiFreshnessLabel);
        kpiLay->addStretch(1);
        kpiLay->addWidget(m_filterEdit);
        kpiLay->addWidget(btnTableMode);
        kpiLay->addWidget(btnGraphMode);
        kpiLay->addWidget(btnInspectToggle);
        kpiLay->addWidget(btnSyncNow);
        mainCol->addWidget(kpiBar);

        // Top Vault Lockout / Wiped Alert Banner (hidden unless m_vaultLocked or m_vaultWiped)
        m_vaultBanner = new QWidget(root);
        m_vaultBanner->setObjectName("VaultBanner");
        auto *vbLay = new QHBoxLayout(m_vaultBanner);
        vbLay->setContentsMargins(10, 4, 10, 4);
        vbLay->setSpacing(10);
        m_vaultBannerLabel = new QLabel("SHUNA VAULT LOCKED :: Encrypted PostgreSQL fleet operations paused.", m_vaultBanner);
        m_vaultBannerLabel->setObjectName("VaultBannerText");
        auto *vbUnlockBtn = new QPushButton("Unlock Vault...", m_vaultBanner);
        vbUnlockBtn->setObjectName("PrimaryBtn");
        connect(vbUnlockBtn, &QPushButton::clicked, this, [this]() { promptVaultUnlock(); });
        auto *vbViewBtn = new QPushButton("Vault & PKI", m_vaultBanner);
        vbViewBtn->setObjectName("SecondaryBtn");
        connect(vbViewBtn, &QPushButton::clicked, this, [this]() { switchWorkspaceMode(3); });
        vbLay->addWidget(m_vaultBannerLabel, 1);
        vbLay->addWidget(vbUnlockBtn);
        vbLay->addWidget(vbViewBtn);
        m_vaultBanner->setVisible(false);
        mainCol->addWidget(m_vaultBanner);

        // Vertical Splitter: Top = WorkspaceStack, Bottom = 2-Tab Dock (Operative Console | Event Viewer)
        m_topBotSplitter = new QSplitter(Qt::Vertical, root);
        m_topBotSplitter->setHandleWidth(4);

        m_workspaceStack = new QStackedWidget(m_topBotSplitter);
        m_workspaceStack->setObjectName("WorkspaceStack");

        // Mode 0: Operations (Left: Operatives Table | Session Graph, Right: Closable Inspector Drawer)
        m_topSplit = new QSplitter(Qt::Horizontal, m_workspaceStack);
        m_topSplit->setHandleWidth(4);

        m_mainViewTabs = new QTabWidget(m_topSplit);
        m_mainViewTabs->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        m_mainViewTabs->setMinimumWidth(460);
        m_sessionTable = buildSessionTable();
        QWidget *graphContainer = buildSessionGraphContainer();
        m_mainViewTabs->addTab(m_sessionTable, "Operatives");
        m_mainViewTabs->addTab(graphContainer, "Session Graph");
        connect(m_mainViewTabs, &QTabWidget::currentChanged, this, [this](int idx) {
            if (idx == 1) {
                rebuildSessionGraph();
            }
        });

        m_inspectorDrawer = new QWidget(m_topSplit);
        m_inspectorDrawer->setObjectName("InspectorDrawer");
        m_inspectorDrawer->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        m_inspectorDrawer->setMinimumWidth(340);
        auto *inspLay = new QVBoxLayout(m_inspectorDrawer);
        inspLay->setContentsMargins(0, 0, 0, 0);
        inspLay->setSpacing(0);

        auto *inspHdr = new QWidget(m_inspectorDrawer);
        inspHdr->setObjectName("InspectorHeader");
        auto *inspHdrLay = new QHBoxLayout(inspHdr);
        inspHdrLay->setContentsMargins(8, 4, 6, 4);
        inspHdrLay->setSpacing(6);
        m_inspectorTitleLabel = new QLabel("Inspector :: Select an operative row", inspHdr);
        m_inspectorTitleLabel->setObjectName("SectionLabel");
        auto *btnTsInspect = new QPushButton("Teamserver", inspHdr);
        btnTsInspect->setObjectName("SecondaryBtn");
        btnTsInspect->setToolTip("Inspect Teamserver mTLS & Shuna Vault telemetry");
        connect(btnTsInspect, &QPushButton::clicked, this, [this]() { showTeamserverInInspector(); });
        auto *btnCloseInsp = new QPushButton("Hide [Esc]", inspHdr);
        btnCloseInsp->setObjectName("SecondaryBtn");
        btnCloseInsp->setToolTip("Close Inspector Drawer (Escape)");
        connect(btnCloseInsp, &QPushButton::clicked, this, [this]() {
            m_inspectorDrawer->setVisible(false);
            if (m_sessionTable) {
                m_sessionTable->setFocus();
            }
        });
        inspHdrLay->addWidget(m_inspectorTitleLabel, 1);
        inspHdrLay->addWidget(btnTsInspect);
        inspHdrLay->addWidget(btnCloseInsp);
        inspLay->addWidget(inspHdr);

        m_smallTabs = new QTabWidget(m_inspectorDrawer);
        m_inspectorSummary = new QTextEdit(m_smallTabs);
        m_inspectorSummary->setReadOnly(true);
        m_inspectorSummary->setFont(QFont("DejaVu Sans Mono", 9));
        m_inspectorSummary->setPlainText(
            formatDotLeader("Endpoint", m_serverAddr) + "\n" +
            formatDotLeader("Transport", "mTLS 1.3 (X25519MLKEM768)") + "\n" +
            formatDotLeader("Auth Binding", "256-bit cert-bound session token") + "\n" +
            formatDotLeader("State", "Awaiting initial sync..."));

        m_rawTelemetry = new QPlainTextEdit(m_smallTabs);
        m_rawTelemetry->setReadOnly(true);
        m_rawTelemetry->setPlaceholderText("Select an operative row in Operatives to inspect raw JSON.");
        m_smallTabs->addTab(m_inspectorSummary, "Summary");
        m_smallTabs->addTab(m_rawTelemetry, "Raw JSON");
        inspLay->addWidget(m_smallTabs, 1);

        m_topSplit->addWidget(m_mainViewTabs);
        m_topSplit->addWidget(m_inspectorDrawer);
        m_topSplit->setStretchFactor(0, 62);
        m_topSplit->setStretchFactor(1, 38);
        m_topSplit->setSizes({900, 520});

        m_workspaceStack->addWidget(m_topSplit);                 // 0: Operations
        m_workspaceStack->addWidget(buildModulesPage());         // 1: Modules
        m_workspaceStack->addWidget(buildFleetAndHistoryPage()); // 2: Fleet & History
        m_workspaceStack->addWidget(buildVaultAndPkiPage());     // 3: Vault & PKI
        m_workspaceStack->addWidget(buildTargetsPage());         // 4: Targets & Sensors

        // Bottom Dock: strictly 2 tabs (Operative Console | Event Viewer)
        m_bottomTabs = new QTabWidget(m_topBotSplitter);
        m_bottomTabs->setObjectName("BottomDock");
        m_bottomTabs->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);
        m_bottomTabs->setMinimumHeight(210);

        m_eventViewer = new QTextEdit(m_bottomTabs);
        m_eventViewer->setReadOnly(true);
        m_eventViewer->setFont(QFont("DejaVu Sans Mono", 8));
        m_eventViewer->setLineWrapMode(QTextEdit::WidgetWidth);
        m_eventViewer->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

        m_consoleTabIdx = m_bottomTabs->addTab(buildOperativeConsoleTab(), "Operative Console");
        m_eventTabIdx = m_bottomTabs->addTab(m_eventViewer, "Event Viewer");
        m_bottomTabs->setCurrentIndex(m_consoleTabIdx);

        m_topBotSplitter->addWidget(m_workspaceStack);
        m_topBotSplitter->addWidget(m_bottomTabs);
        m_topBotSplitter->setStretchFactor(0, 49);
        m_topBotSplitter->setStretchFactor(1, 51);
        m_topBotSplitter->setSizes({405, 420});

        mainCol->addWidget(m_topBotSplitter, 1);
        bodyHBox->addLayout(mainCol, 1);
        outerVBox->addLayout(bodyHBox, 1);

        // 3. 1-Line Bottom Status Strip
        auto *statusStrip = new QWidget(root);
        statusStrip->setObjectName("StatusBarStrip");
        statusStrip->setFixedHeight(24);
        auto *stLay = new QHBoxLayout(statusStrip);
        stLay->setContentsMargins(10, 2, 10, 2);
        stLay->setSpacing(16);
        m_statusTransportLabel = new QLabel("mTLS: OFFLINE", statusStrip);
        m_statusTransportLabel->setObjectName("StatusStripItem");
        m_statusCertLabel = new QLabel("Serial: -", statusStrip);
        m_statusCertLabel->setObjectName("StatusStripItem");
        m_statusDbLabel = new QLabel("DB: standalone", statusStrip);
        m_statusDbLabel->setObjectName("StatusStripItem");
        m_statusVaultLabel = new QLabel("Vault: ephemeral", statusStrip);
        m_statusVaultLabel->setObjectName("StatusStripItem");
        m_statusPollLabel = new QLabel("Monitor: PAUSED", statusStrip);
        m_statusPollLabel->setObjectName("StatusStripItem");
        m_statusRpcLabel = new QLabel("RPC: IDLE", statusStrip);
        m_statusRpcLabel->setObjectName("StatusStripItem");

        stLay->addWidget(m_statusTransportLabel);
        stLay->addWidget(m_statusCertLabel);
        stLay->addWidget(m_statusDbLabel);
        stLay->addWidget(m_statusVaultLabel);
        stLay->addWidget(m_statusPollLabel);
        stLay->addStretch(1);
        stLay->addWidget(m_statusRpcLabel);
        outerVBox->addWidget(statusStrip);

        setCentralWidget(root);
        updateStatusStrip();
    }

    void applySessionTableFilter(const QString &query) {
        if (!m_sessionTable) {
            return;
        }
        const QString q = query.trimmed().toLower();
        for (int r = 0; r < m_sessionTable->rowCount(); ++r) {
            if (q.isEmpty()) {
                m_sessionTable->setRowHidden(r, false);
                continue;
            }
            bool match = false;
            for (int c = 0; c < m_sessionTable->columnCount(); ++c) {
                if (const QTableWidgetItem *it = m_sessionTable->item(r, c)) {
                    if (it->text().toLower().contains(q)) {
                        match = true;
                        break;
                    }
                }
            }
            m_sessionTable->setRowHidden(r, !match);
        }
    }

    void updateStatusStrip() {
        if (m_statusTransportLabel) {
            m_statusTransportLabel->setText(
                QString("mTLS: %1 (%2)").arg(m_teamserverOnline ? "ONLINE" : "OFFLINE", m_pqcGroup));
        }
        if (m_statusCertLabel) {
            m_statusCertLabel->setText(
                QString("Serial: %1").arg(m_clientCertSerial.isEmpty() ? "-" : m_clientCertSerial.left(16)));
        }
        if (m_statusDbLabel) {
            m_statusDbLabel->setText(
                QString("DB: %1").arg(m_dbConnected ? ("PG16/" + m_sslMode) : "standalone"));
        }
        if (m_statusVaultLabel) {
            QString vtxt = "ephemeral";
            if (m_dbConnected || m_shunaKeyVersion > 0) {
                if (m_vaultWiped) {
                    vtxt = "WIPED_LOCKED";
                } else if (m_vaultLocked) {
                    vtxt = QString("LOCKED (fails=%1)").arg(m_vaultFailedAttempts);
                } else {
                    vtxt = QString("UNLOCKED (kv=%1%2)").arg(m_shunaKeyVersion).arg(m_rotationInProgress ? ", rotating" : "");
                }
            }
            m_statusVaultLabel->setText("Vault: " + vtxt);
        }
        if (m_statusPollLabel) {
            const bool active = m_actPoll && m_actPoll->isChecked();
            m_statusPollLabel->setText(active ? "Monitor: 3s ACTIVE" : "Monitor: PAUSED");
        }
        if (m_kpiTransportBadge) {
            m_kpiTransportBadge->setText(QString("mTLS 1.3 :: %1").arg(m_pqcGroup));
        }
        if (m_kpiVaultBadge) {
            if (m_vaultWiped) {
                m_kpiVaultBadge->setText("Vault: WIPED");
            } else if (m_vaultLocked) {
                m_kpiVaultBadge->setText("Vault: LOCKED");
            } else if (m_dbConnected || m_shunaKeyVersion > 0) {
                m_kpiVaultBadge->setText(QString("Vault: kv=%1").arg(m_shunaKeyVersion));
            } else {
                m_kpiVaultBadge->setText("Vault: STANDALONE");
            }
        }
        if (m_kpiOpsBadge) {
            m_kpiOpsBadge->setText(QString("Operatives: %1").arg(m_snapshots.size()));
        }
        if (m_vaultBanner && m_vaultBannerLabel) {
            const bool showBanner = m_vaultLocked || m_vaultWiped;
            m_vaultBanner->setVisible(showBanner);
            if (showBanner) {
                m_vaultBannerLabel->setText(
                    QString("SHUNA VAULT %1 :: failed_attempts=%2/5 | retry_after=%3s | wiped=%4")
                        .arg(m_vaultWiped ? "WIPED & LOCKED" : "LOCKED")
                        .arg(m_vaultFailedAttempts)
                        .arg(m_vaultRetryAfterSec)
                        .arg(m_vaultWiped ? "true (offline recovery required)" : "false"));
            }
        }
        if (m_vaultDetailLabel) {
            m_vaultDetailLabel->setText(
                formatDotLeader("Database Mode", m_dbConnected ? ("PostgreSQL 16 (" + m_sslMode + ")") : "Ephemeral Standalone") + "\n" +
                formatDotLeader("Vault State", m_vaultWiped ? "WIPED_LOCKED" : (m_vaultLocked ? "VAULT_LOCKED" : "UNLOCKED")) + "\n" +
                formatDotLeader("Active DEK Version", QString::number(m_shunaKeyVersion)) + "\n" +
                formatDotLeader("Online Rotation", m_rotationInProgress ? "IN_PROGRESS" : "IDLE") + "\n" +
                formatDotLeader("Failed Unlocks", QString("%1 / 5").arg(m_vaultFailedAttempts)) + "\n" +
                formatDotLeader("Cooldown / Retry", QString("%1s").arg(m_vaultRetryAfterSec)) + "\n" +
                formatDotLeader("Strict Wipe Triggered", m_vaultWiped ? "true" : "false") + "\n" +
                formatDotLeader("Local Blind Host ID", m_hostId.isEmpty() ? "-" : m_hostId.left(24) + "..."));
        }
        if (m_pkiDetailLabel) {
            m_pkiDetailLabel->setText(
                formatDotLeader("Teamserver Endpoint", m_serverAddr) + "\n" +
                formatDotLeader("TLS Protocol", "TLS 1.3 Mutual Auth") + "\n" +
                formatDotLeader("PQC Hybrid Group", m_pqcGroup) + "\n" +
                formatDotLeader("Client Cert Serial", m_clientCertSerial.isEmpty() ? "-" : m_clientCertSerial) + "\n" +
                formatDotLeader("Session Token TTL", m_tokenExpiresAt.isEmpty() ? "-" : m_tokenExpiresAt) + "\n" +
                formatDotLeader("PKI Directory", m_pkiDir));
        }
        updateListenersTable();
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

        auto setCell = [&](int row, int col, const QString &text, const QColor &colr = QColor("#F3F4F6")) {
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

        setCell(0, 1, QString("mTLS 1.3 / %1").arg(m_pqcGroup));
        setCell(0, 2, m_serverAddr);
        setCell(0, 3, rpcScope);
        const QString capText = m_dbConnected
                                    ? QString("PG16:%1 | kv=%2").arg(m_sslMode).arg(m_shunaKeyVersion)
                                    : QString("standalone | mTLS");
        setCell(0, 4, capText);
        if (m_authFailed) {
            setCell(0, 5, "UNAUTHORIZED", QColor("#EF4444"));
        } else if (m_vaultLocked) {
            setCell(0, 5, "VAULT_LOCKED", QColor("#F59E0B"));
        } else {
            setCell(0, 5, m_teamserverOnline ? "ONLINE" : "OFFLINE",
                    m_teamserverOnline ? QColor("#10B981") : QColor("#EF4444"));
        }

        setCell(1, 2, m_resolvedKcmSocket.isEmpty() ? "none (passive)" : m_resolvedKcmSocket);
        setCell(1, 3, "uid=" + m_localUid);
        setCell(2, 2, m_resolvedCCache.isEmpty() ? "auto ($KRB5CCNAME)" : m_resolvedCCache);
        setCell(2, 4, m_kinitAvailable ? "kinit -R" : "no-kinit");
        setCell(4, 2, m_resolvedKrb5Conf.isEmpty() ? "/etc/krb5.conf" : m_resolvedKrb5Conf);

        auto syncOpRow = [&](int tableRow, const QString &opId) {
            if (m_authFailed) {
                setCell(tableRow, 5, "UNAUTHORIZED", QColor("#EF4444"));
                return;
            }
            if (!m_teamserverOnline) {
                setCell(tableRow, 5, "OFFLINE", QColor("#EF4444"));
                return;
            }
            if (!m_snapshots.contains(opId)) {
                setCell(tableRow, 5, "PENDING", QColor("#F59E0B"));
                return;
            }
            const OperativeSnapshot &s = m_snapshots.value(opId);
            setCell(tableRow, 5, s.status, cielStatusColor(s.status));
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

    QString buildDotLeaderForSnapshot(const OperativeSnapshot &snap) const {
        QStringList lines;
        lines << formatDotLeader("Operative ID", snap.id);
        lines << formatDotLeader("Operative Engine", snap.operative);
        lines << formatDotLeader("Active Module", snap.module);
        lines << formatDotLeader("Status", snap.status);
        lines << formatDotLeader("Exit Code", QString::number(snap.exitCode));
        lines << formatDotLeader("Target / Principal", snap.target);
        lines << formatDotLeader("Permissions / TTL", snap.permOrTtl);
        lines << formatDotLeader("Skew / Telemetry", snap.skewOrAux);
        lines << formatDotLeader("Detail Summary", snap.detail);
        lines << formatDotLeader("Host Context", QString("%1@%2 (uid=%3, %4)").arg(m_localUser, m_localHost, m_localUid, m_localArch));
        if (snap.rawJson.isObject()) {
            lines << QString();
            lines << "--- Structured Telemetry Fields ---";
            const QJsonObject obj = snap.rawJson.toObject();
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                QString vStr;
                if (it.value().isString()) {
                    vStr = it.value().toString();
                } else if (it.value().isBool()) {
                    vStr = it.value().toBool() ? "true" : "false";
                } else if (it.value().isDouble()) {
                    vStr = QString::number(it.value().toVariant().toLongLong());
                } else if (it.value().isArray()) {
                    vStr = QString("[%1 item(s)]").arg(it.value().toArray().size());
                } else if (it.value().isObject()) {
                    vStr = QString::fromUtf8(QJsonDocument(it.value().toObject()).toJson(QJsonDocument::Compact));
                }
                lines << formatDotLeader(it.key(), vStr);
            }
        }
        return lines.join('\n');
    }

    void showTeamserverInInspector() {
        if (m_inspectorDrawer) {
            m_inspectorDrawer->setVisible(true);
        }
        if (m_inspectorTitleLabel) {
            m_inspectorTitleLabel->setText("Inspector :: TEAMSERVER (" + m_serverAddr + ")");
        }
        m_inspectedOpId = "TEAMSERVER";
        if (m_inspectorSummary) {
            updateEditorPreservingScroll(m_inspectorSummary, m_teamserverSummaryText);
        }
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
        m_inspectedOpId = opId;
        if (m_inspectorTitleLabel && m_snapshots.contains(opId)) {
            const OperativeSnapshot &s = m_snapshots.value(opId);
            m_inspectorTitleLabel->setText(QString("Inspector :: %1 %2 (%3)").arg(s.id, s.operative, s.module));
        }
        if (m_inspectorSummary && m_snapshots.contains(opId)) {
            const QString sumText = buildDotLeaderForSnapshot(m_snapshots.value(opId));
            if (m_summarySourceOpId == opId) {
                updateEditorPreservingScroll(m_inspectorSummary, sumText);
            } else {
                m_summarySourceOpId = opId;
                m_inspectorSummary->setPlainText(sumText);
                if (QScrollBar *vb = m_inspectorSummary->verticalScrollBar()) {
                    vb->setValue(0);
                }
            }
        }
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
        if (m_inspectorDrawer) {
            m_inspectorDrawer->setVisible(true);
        }
        for (int r = 0; r < m_sessionTable->rowCount(); ++r) {
            if (m_sessionTable->item(r, 0) && m_sessionTable->item(r, 0)->text() == opId) {
                showRowJson(r);
                break;
            }
        }
    }

    QTableWidget *buildSessionTable() {
        auto *tbl = new QTableWidget(0, 6, this);
        tbl->setShowGrid(false);
        tbl->setAlternatingRowColors(false);
        tbl->verticalHeader()->setVisible(false);
        tbl->setSelectionBehavior(QAbstractItemView::SelectRows);
        tbl->setSelectionMode(QAbstractItemView::SingleSelection);
        tbl->setEditTriggers(QAbstractItemView::NoEditTriggers);
        tbl->setFocusPolicy(Qt::StrongFocus);
        tbl->setContextMenuPolicy(Qt::CustomContextMenu);

        const QStringList headers = {
            "ID", "Operative", "Module", "Target / Principal", "Perm / TTL / Aux", "Status"
        };
        tbl->setHorizontalHeaderLabels(headers);
        QHeaderView *hdr = tbl->horizontalHeader();
        hdr->setMinimumSectionSize(58);
        hdr->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        hdr->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        hdr->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        hdr->setSectionResizeMode(3, QHeaderView::Stretch);
        hdr->setSectionResizeMode(4, QHeaderView::Stretch);
        hdr->setSectionResizeMode(5, QHeaderView::ResizeToContents);
        tbl->verticalHeader()->setDefaultSectionSize(24);

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
                    if (m_inspectorDrawer && !m_inspectorDrawer->isVisible()) {
                        m_inspectorDrawer->setVisible(true);
                    }
                    showRowJson(row);
                });
        connect(tbl, &QTableWidget::cellDoubleClicked,
                this, [this](int row, int col) {
                    (void)col;
                    if (m_inspectorDrawer) {
                        m_inspectorDrawer->setVisible(true);
                    }
                    showRowJson(row);
                });

        connect(tbl, &QTableWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
            QMenu ctx(this);
            QAction *aInteract = ctx.addAction("Interact (Operative Console)");
            QAction *aConfigure = ctx.addAction("Open Module Configuration Modal...");
            ctx.addSeparator();
            QAction *aSouei = ctx.addAction("Probe /proc/keys & KCM (Souei)");
            QAction *aWatch = ctx.addAction("Watch File Inotify (Souei)...");
            QAction *aChronoa = ctx.addAction("Audit Active CCACHE (Chronoa)...");
            QAction *aRenew = ctx.addAction("Renew TGT via kinit -R (Chronoa)");
            ctx.addSeparator();
            QAction *aIsolate = ctx.addAction("Isolate Session Keyring (Ranga)");
            QAction *aPurge = ctx.addAction("3-Pass NIST Purge File (Ranga)...");
            ctx.addSeparator();
            QMenu *tanukiSub = ctx.addMenu("Tanuki Identity Engine");
            auto addCtxTanuki = [&](const QString &label, const QString &subcmd) {
                QAction *act = tanukiSub->addAction(label);
                connect(act, &QAction::triggered, this, [this, subcmd]() {
                    commitTargetFieldsFromEdits();
                    openTanukiModuleModal(subcmd);
                });
            };
            addCtxTanuki("Doctor Pre-Flight & Enctype Audit...", "doctor");
            addCtxTanuki("Fix Self-Healing (Dry-Run / Apply)...", "fix");
            addCtxTanuki("Config Zero-DNS krb5.conf...", "config");
            addCtxTanuki("5-Rung Decision Ladder...", "ladder");
            addCtxTanuki("Triage Error Code...", "triage");
            addCtxTanuki("NHI Scan / Mesh / Live Exchange...", "nhi");
            addCtxTanuki("AD Blast Radius & Tiering...", "blast");
            addCtxTanuki("LDAP / LDAPS Directory Query...", "ldap");
            addCtxTanuki("Live Kerberos Auth (AS-REQ)...", "auth");
            addCtxTanuki("Live Kerberos Service Ticket (TGS)...", "tgs");
            ctx.addSeparator();
            QAction *aJson = ctx.addAction("Inspect Raw JSON");

            connect(aInteract, &QAction::triggered, this, [this]() {
                m_bottomTabs->setCurrentIndex(m_consoleTabIdx);
                m_operativeInput->setFocus();
            });
            connect(aConfigure, &QAction::triggered, this, [this]() {
                const QString opId = selectedOperativeId();
                if (opId == "OP-01") openTempestModuleModal("souei_watch");
                else if (opId == "OP-02") openTempestModuleModal("chronoa_check");
                else if (opId == "OP-05") openTempestModuleModal("ranga_purge");
                else openTanukiModuleModal(m_lastExplicitTanukiSubcmd.isEmpty() ? "doctor" : m_lastExplicitTanukiSubcmd);
            });
            connect(aSouei, &QAction::triggered, this, [this]() {
                commitTargetFieldsFromEdits();
                dispatchFromUi("souei_probe", targetParams(), "souei --probe");
            });
            connect(aWatch, &QAction::triggered, this, [this]() { openTempestModuleModal("souei_watch"); });
            connect(aChronoa, &QAction::triggered, this, [this]() { openTempestModuleModal("chronoa_check"); });
            connect(aRenew, &QAction::triggered, this, [this]() {
                commitTargetFieldsFromEdits();
                QJsonObject p = targetParams();
                p["renew"] = true;
                dispatchFromUi("chronoa_check", p, "chronoa --renew (kinit -R)");
            });
            connect(aIsolate, &QAction::triggered, this, [this]() {
                dispatchFromUi("ranga_isolate", QJsonObject(), "ranga --isolate");
            });
            connect(aPurge, &QAction::triggered, this, [this]() { openTempestModuleModal("ranga_purge"); });
            connect(aJson, &QAction::triggered, this, [this]() {
                if (m_inspectorDrawer) m_inspectorDrawer->setVisible(true);
                m_smallTabs->setCurrentIndex(1);
            });

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
        m_graphScene->setBackgroundBrush(QColor("#181B22"));
        m_graphView = new CielGraphView(m_graphScene, container);
        m_graphView->setToolTip("Drag nodes, scroll wheel to zoom, click node to inspect telemetry");
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
            "  background-color: rgba(24, 27, 34, 230);"
            "  border: 1px solid #2B303C;"
            "  border-radius: 6px;"
            "}"
            "QWidget#GraphZoomPill QPushButton {"
            "  background-color: transparent;"
            "  color: #F3F4F6;"
            "  border: none;"
            "  border-radius: 4px;"
            "  padding: 2px 6px;"
            "  min-height: 18px;"
            "  font-size: 11px;"
            "  font-weight: bold;"
            "}"
            "QWidget#GraphZoomPill QPushButton:hover {"
            "  background-color: #2B303C;"
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
                    if (opId.isEmpty()) {
                        showTeamserverInInspector();
                    } else if (m_sessionTable) {
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
        m_operativeInput->setPlaceholderText("Command (help, audit, sync, set, hosts, history, unlock, rotate, backup, session_issue, token_rotate, cert_revoke, souei, chronoa, ranga, tanuki <cmd> ...)");
        const QStringList cmds = {
            "help", "audit", "sync", "set", "hosts", "history", "unlock", "rotate", "backup",
            "session_issue", "token_rotate", "cert_revoke",
            "souei", "watch", "chronoa", "renew",
            "ranga", "isolate", "purge",
            "tanuki doctor", "tanuki fix", "tanuki fix --apply", "tanuki keytab", "tanuki config",
            "tanuki triage SKEW", "tanuki ladder", "tanuki nhi scan", "tanuki nhi mesh", "tanuki nhi exchange",
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

    QWidget *buildModulesPage() {
        auto *panel = new QWidget(this);
        panel->setObjectName("ModulesPanel");
        auto *vbox = new QVBoxLayout(panel);
        vbox->setContentsMargins(10, 8, 10, 8);
        vbox->setSpacing(8);

        auto *hdr = new QLabel("4-Cycle Module Catalog :: Progressive-Disclosure Operator Launcher (Options | Preview & CLI)", panel);
        hdr->setObjectName("SectionLabel");
        vbox->addWidget(hdr);

        auto *grid = new QGridLayout();
        grid->setHorizontalSpacing(10);
        grid->setVerticalSpacing(8);

        auto makeGroupCard = [&](const QString &title, const QList<QPair<QString, std::function<void()>>> &actions) {
            auto *card = new QWidget(panel);
            card->setObjectName("ModuleCard");
            auto *cl = new QVBoxLayout(card);
            cl->setContentsMargins(10, 8, 10, 8);
            cl->setSpacing(6);
            auto *lbl = new QLabel(title, card);
            lbl->setObjectName("SectionLabel");
            cl->addWidget(lbl);
            auto *bg = new QGridLayout();
            bg->setHorizontalSpacing(6);
            bg->setVerticalSpacing(6);
            for (int i = 0; i < actions.size(); ++i) {
                auto *b = new QPushButton(actions[i].first, card);
                b->setObjectName(actions[i].first.contains("Purge") ? "DangerBtn" : "SecondaryBtn");
                connect(b, &QPushButton::clicked, this, actions[i].second);
                bg->addWidget(b, i / 3, i % 3);
            }
            cl->addLayout(bg);
            cl->addStretch(1);
            return card;
        };

        grid->addWidget(makeGroupCard("Cycle 1A :: Tanuki Pre-Flight, Self-Healing & Zero-DNS Config", {
            {"Doctor (+OPSEC / Enctype)...", [this]() { openTanukiModuleModal("doctor"); }},
            {"Fix (Dry-Run / Apply)...", [this]() { openTanukiModuleModal("fix"); }},
            {"Config (FAST / AES)...", [this]() { openTanukiModuleModal("config"); }},
            {"Keytab (SHA-256 Redacted)...", [this]() { openTanukiModuleModal("keytab"); }},
            {"KCM Extractor...", [this]() { openTanukiModuleModal("kcm"); }},
            {"Error Triage...", [this]() { openTanukiModuleModal("triage"); }},
            {"5-Rung Decision Ladder", [this]() { dispatchTanuki("ladder"); }},
            {"Agent Skill Manifest", [this]() { dispatchTanuki("skill"); }},
            {"Purge Caches (NIST)...", [this]() { promptTanukiPurge(); }},
        }), 0, 0);

        grid->addWidget(makeGroupCard("Cycle 1B :: Tanuki Kerberos Wire, Verified LDAPS & AD Tiering", {
            {"Auth TGT (FAST / Armor)...", [this]() { openTanukiModuleModal("auth"); }},
            {"TGS Service Ticket...", [this]() { openTanukiModuleModal("tgs"); }},
            {"LDAPS / Directory Recon...", [this]() { openTanukiModuleModal("ldap"); }},
            {"Blast Radius && Tiering...", [this]() { openTanukiModuleModal("blast"); }},
            {"AD CS Scanner (ESC1-11)...", [this]() { openTanukiModuleModal("adcs"); }},
            {"MS-PAC NDR Decoder...", [this]() { openTanukiModuleModal("pac"); }},
            {"Shadow Credentials...", [this]() { openTanukiModuleModal("shadow"); }},
        }), 0, 1);

        grid->addWidget(makeGroupCard("Cycle 1C :: Tanuki Non-Human Identity (NHI) & RFC 8693 Exchange", {
            {"NHI Local Token Scan", [this]() { dispatchTanuki("nhi", "scan"); }},
            {"NHI Cloud/SPIFFE Mesh...", [this]() { openTanukiModuleModal("nhi"); }},
            {"NHI Live STS Exchange...", [this]() { openTanukiModuleModal("nhi"); }},
            {"Validate Workload JWT...", [this]() { openTanukiModuleModal("token"); }},
        }), 1, 0);

        grid->addWidget(makeGroupCard("Cycle 2 :: Tempest Linux Kernel Keyring, CCACHE & Inotify", {
            {"Souei Keyring && KCM Probe", [this]() { commitTargetFieldsFromEdits(); dispatchFromUi("souei_probe", targetParams(), "souei --probe"); }},
            {"Souei Inotify File Watch...", [this]() { openTempestModuleModal("souei_watch"); }},
            {"Chronoa CCACHE && Skew...", [this]() { openTempestModuleModal("chronoa_check"); }},
            {"Chronoa Renew (kinit -R)", [this]() { commitTargetFieldsFromEdits(); QJsonObject p = targetParams(); p["renew"] = true; dispatchFromUi("chronoa_check", p, "chronoa --renew (kinit -R)"); }},
            {"Ranga Baseline Keyring", [this]() { dispatchFromUi("ranga_status", QJsonObject(), "ranga --status"); }},
            {"Ranga Isolate (0x3f000000)", [this]() { dispatchFromUi("ranga_isolate", QJsonObject(), "ranga --isolate"); }},
            {"Ranga Wrapper Isolate", [this]() { QJsonObject p; p["isolate_mode"] = "wrapper"; dispatchFromUi("ranga_isolate", p, "ranga --isolate (wrapper)"); }},
            {"Ranga File Purge + Revoke...", [this]() { openTempestModuleModal("ranga_purge"); }},
        }), 1, 1);

        vbox->addLayout(grid, 1);
        return panel;
    }

    QWidget *buildFleetAndHistoryPage() {
        auto *panel = new QWidget(this);
        panel->setObjectName("FleetPanel");
        auto *vbox = new QVBoxLayout(panel);
        vbox->setContentsMargins(10, 8, 10, 8);
        vbox->setSpacing(6);

        auto *topRow = new QHBoxLayout();
        auto *hLbl = new QLabel("Encrypted PostgreSQL 16 Fleet Hosts & Row-Bound AEAD Audit Timeline", panel);
        hLbl->setObjectName("SectionLabel");
        auto *btnRefHosts = new QPushButton("Refresh Hosts", panel);
        btnRefHosts->setObjectName("SecondaryBtn");
        connect(btnRefHosts, &QPushButton::clicked, this, [this]() { refreshFleetHostsView(); });
        m_historyFilterCombo = new QComboBox(panel);
        m_historyFilterCombo->addItems({"All Operatives", "OP-01", "OP-02", "OP-03", "OP-04", "OP-05", "OP-06"});
        auto *btnRefHist = new QPushButton("Refresh History", panel);
        btnRefHist->setObjectName("PrimaryBtn");
        connect(btnRefHist, &QPushButton::clicked, this, [this]() { refreshHistoryTimelineView(); });
        topRow->addWidget(hLbl, 1);
        topRow->addWidget(btnRefHosts);
        topRow->addWidget(m_historyFilterCombo);
        topRow->addWidget(btnRefHist);
        vbox->addLayout(topRow);

        m_hostsTable = new QTableWidget(0, 6, panel);
        m_hostsTable->setShowGrid(false);
        m_hostsTable->verticalHeader()->setVisible(false);
        m_hostsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_hostsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_hostsTable->setFocusPolicy(Qt::StrongFocus);
        m_hostsTable->setHorizontalHeaderLabels({"Host ID (HMAC)", "Hostname", "Operator / Arch", "DEK Version", "Status", "Last Seen"});
        m_hostsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        m_hostsTable->verticalHeader()->setDefaultSectionSize(22);
        vbox->addWidget(m_hostsTable, 1);

        m_historyTable = new QTableWidget(0, 5, panel);
        m_historyTable->setShowGrid(false);
        m_historyTable->verticalHeader()->setVisible(false);
        m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_historyTable->setFocusPolicy(Qt::StrongFocus);
        m_historyTable->setHorizontalHeaderLabels({"Timestamp", "Operative", "Severity / Exit", "Status", "Decrypted Summary / Target"});
        m_historyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        m_historyTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
        m_historyTable->verticalHeader()->setDefaultSectionSize(22);
        vbox->addWidget(m_historyTable, 2);

        return panel;
    }

    QWidget *buildVaultAndPkiPage() {
        auto *panel = new QWidget(this);
        panel->setObjectName("VaultPanel");
        auto *hbox = new QHBoxLayout(panel);
        hbox->setContentsMargins(10, 8, 10, 8);
        hbox->setSpacing(12);

        // Card 1: Shuna Vault & PostgreSQL Persistence
        auto *vCard = new QWidget(panel);
        vCard->setObjectName("VaultCard");
        auto *vLay = new QVBoxLayout(vCard);
        vLay->setContentsMargins(12, 10, 12, 10);
        vLay->setSpacing(8);
        auto *vTitle = new QLabel("Cycle 3 :: Shuna Envelope Vault & Encrypted PostgreSQL 16", vCard);
        vTitle->setObjectName("SectionLabel");
        vLay->addWidget(vTitle);

        m_vaultDetailLabel = new QLabel(vCard);
        m_vaultDetailLabel->setFont(QFont("DejaVu Sans Mono", 9));
        m_vaultDetailLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        vLay->addWidget(m_vaultDetailLabel, 1);

        auto *vBtnRow = new QHBoxLayout();
        auto *btnUnlock = new QPushButton("Unlock Vault...", vCard);
        btnUnlock->setObjectName("PrimaryBtn");
        connect(btnUnlock, &QPushButton::clicked, this, [this]() { promptVaultUnlock(); });
        auto *btnRotate = new QPushButton("Rotate DEK (Online)...", vCard);
        btnRotate->setObjectName("SecondaryBtn");
        connect(btnRotate, &QPushButton::clicked, this, [this]() { promptVaultRotate(); });
        auto *btnBackup = new QPushButton("Export SHUNAS1 Backup...", vCard);
        btnBackup->setObjectName("SecondaryBtn");
        connect(btnBackup, &QPushButton::clicked, this, [this]() { promptBackupExport(); });
        vBtnRow->addWidget(btnUnlock);
        vBtnRow->addWidget(btnRotate);
        vBtnRow->addWidget(btnBackup);
        vLay->addLayout(vBtnRow);

        // Card 2: Post-Quantum Hybrid mTLS 1.3 & Session PKI
        auto *pCard = new QWidget(panel);
        pCard->setObjectName("PkiCard");
        auto *pLay = new QVBoxLayout(pCard);
        pLay->setContentsMargins(12, 10, 12, 10);
        pLay->setSpacing(8);
        auto *pTitle = new QLabel("Cycle 3 & 4 :: Post-Quantum mTLS 1.3 (X25519MLKEM768) & CRL Revocation", pCard);
        pTitle->setObjectName("SectionLabel");
        pLay->addWidget(pTitle);

        m_pkiDetailLabel = new QLabel(pCard);
        m_pkiDetailLabel->setFont(QFont("DejaVu Sans Mono", 9));
        m_pkiDetailLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        pLay->addWidget(m_pkiDetailLabel, 1);

        auto *pBtnRow = new QHBoxLayout();
        auto *btnTokRot = new QPushButton("Rotate Session Token (30s Grace)", pCard);
        btnTokRot->setObjectName("PrimaryBtn");
        connect(btnTokRot, &QPushButton::clicked, this, [this]() { handleOperativeCommand("token_rotate"); });
        auto *btnSessIssue = new QPushButton("Issue Cert-Bound Token...", pCard);
        btnSessIssue->setObjectName("SecondaryBtn");
        connect(btnSessIssue, &QPushButton::clicked, this, [this]() { promptSessionIssue(); });
        auto *btnRevoke = new QPushButton("Revoke Certificate...", pCard);
        btnRevoke->setObjectName("DangerBtn");
        connect(btnRevoke, &QPushButton::clicked, this, [this]() { promptCertRevoke(); });
        pBtnRow->addWidget(btnTokRot);
        pBtnRow->addWidget(btnSessIssue);
        pBtnRow->addWidget(btnRevoke);
        pLay->addLayout(pBtnRow);

        hbox->addWidget(vCard, 1);
        hbox->addWidget(pCard, 1);
        return panel;
    }

    QWidget *buildTargetsPage() {
        auto *panel = new QWidget(this);
        panel->setObjectName("TargetsPanel");
        auto *vbox = new QVBoxLayout(panel);
        vbox->setContentsMargins(10, 8, 10, 8);
        vbox->setSpacing(8);

        auto *sLbl = new QLabel("Active Runtime Sensors & Transport Endpoints:", panel);
        sLbl->setObjectName("SectionLabel");
        vbox->addWidget(sLbl);

        m_listenersTable = new QTableWidget(5, 6, panel);
        m_listenersTable->setShowGrid(false);
        m_listenersTable->verticalHeader()->setVisible(false);
        m_listenersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_listenersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_listenersTable->setFocusPolicy(Qt::StrongFocus);
        m_listenersTable->setHorizontalHeaderLabels({"Sensor / Subsystem", "Protocol", "Endpoint / Path", "Scope", "Capability", "Status"});
        m_listenersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        m_listenersTable->verticalHeader()->setDefaultSectionSize(24);

        QString rpcHost = "127.0.0.1";
        quint16 rpcPort = 40056;
        parseHostPort(m_serverAddr, rpcHost, rpcPort);
        const QString rpcScope = (rpcHost == "127.0.0.1" || rpcHost == "localhost" || rpcHost == "::1")
                                     ? "loopback"
                                     : "remote-rpc";

        const QList<QStringList> defaults = {
            {"ciel-teamserver-rpc", "mTLS 1.3 / X25519MLKEM768", m_serverAddr, rpcScope, "standalone | mTLS", "OFFLINE"},
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
                    it->setForeground(QColor("#EF4444"));
                }
                m_listenersTable->setItem(r, c, it);
            }
        }
        vbox->addWidget(m_listenersTable, 1);

        auto *cfgLabel = new QLabel("Field Target Paths (leave blank for host auto-discovery; auto-synced to encrypted PostgreSQL profile when unlocked):", panel);
        cfgLabel->setObjectName("SectionLabel");
        vbox->addWidget(cfgLabel);

        auto *cfgGrid = new QGridLayout();
        cfgGrid->setHorizontalSpacing(6);
        cfgGrid->setVerticalSpacing(6);

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
        applyBtn->setObjectName("PrimaryBtn");
        applyBtn->setToolTip("Apply Field Target Paths and run a full host sync across Tempest + Tanuki");
        auto *clearBtn = new QPushButton("Reset Auto", panel);
        clearBtn->setObjectName("SecondaryBtn");
        clearBtn->setToolTip("Clear custom target paths and revert to host auto-discovery");
        connect(applyBtn, &QPushButton::clicked, this, [this]() {
            runFullSync(false);
            saveProfileIfDbConnected();
        });
        connect(clearBtn, &QPushButton::clicked, this, [this]() {
            m_ccacheEdit->clear();
            m_keytabEdit->clear();
            m_krb5ConfEdit->clear();
            m_kcmSockEdit->clear();
            commitTargetFieldsFromEdits();
            runFullSync(false);
            saveProfileIfDbConnected();
        });
        cfgGrid->addWidget(applyBtn, 0, 6);
        cfgGrid->addWidget(clearBtn, 1, 6);
        cfgGrid->setColumnStretch(1, 1);
        cfgGrid->setColumnStretch(4, 1);
        vbox->addLayout(cfgGrid);
        return panel;
    }

    void refreshFleetHostsView() {
        appendCommandEcho("[GUI -> CLI] hosts");
        QJsonObject resp;
        if (!executeAction("host_list", QJsonObject(), resp, false) || !m_hostsTable) {
            return;
        }
        const QJsonArray hosts = resp.value("data").toObject().value("hosts").toArray();
        m_hostsTable->setRowCount(0);
        for (int i = 0; i < hosts.size(); ++i) {
            const QJsonObject o = hosts.at(i).toObject();
            const QJsonObject m = o.value("meta").toObject();
            m_hostsTable->insertRow(i);
            const QStringList cols = {
                o.value("host_id").toString().left(16),
                m.value("hostname").toString(),
                QString("%1 (%2) / %3").arg(m.value("username").toString()).arg(m.value("uid").toInt()).arg(m.value("arch").toString()),
                QString("v%1").arg(o.value("key_version").toInt()),
                o.value("status").toString(),
                o.value("last_seen_at").toString().left(19)
            };
            for (int c = 0; c < cols.size(); ++c) {
                auto *it = new QTableWidgetItem(cols.at(c));
                it->setTextAlignment(Qt::AlignCenter);
                if (c == 4) {
                    it->setForeground(cielStatusColor(cols.at(c)));
                }
                m_hostsTable->setItem(i, c, it);
            }
        }
    }

    void refreshHistoryTimelineView() {
        QJsonObject p;
        p["limit"] = 50;
        QString cliCmd = "history";
        if (m_historyFilterCombo && m_historyFilterCombo->currentIndex() > 0) {
            const QString op = m_historyFilterCombo->currentText();
            p["operative_id"] = op;
            cliCmd += " " + op;
        }
        appendCommandEcho("[GUI -> CLI] " + cliCmd);
        QJsonObject resp;
        if (!executeAction("history_list", p, resp, false) || !m_historyTable) {
            return;
        }
        const QJsonArray evs = resp.value("data").toObject().value("events").toArray();
        m_historyTable->setRowCount(0);
        for (int i = 0; i < evs.size(); ++i) {
            const QJsonObject ev = evs.at(i).toObject();
            const QJsonObject det = ev.value("detail").toObject();
            m_historyTable->insertRow(i);
            const QString statusStr = ev.value("status").toString();
            const QStringList cols = {
                ev.value("created_at").toString().left(19),
                ev.value("operative_id").toString(),
                QString("%1 (rc=%2)").arg(ev.value("severity").toString()).arg(ev.value("exit_code").toInt()),
                statusStr,
                QString("[%1] %2").arg(det.value("action").toString(), det.value("summary").toString())
            };
            for (int c = 0; c < cols.size(); ++c) {
                auto *it = new QTableWidgetItem(cols.at(c));
                if (c < 4) it->setTextAlignment(Qt::AlignCenter);
                if (c == 3) it->setForeground(cielStatusColor(statusStr));
                m_historyTable->setItem(i, c, it);
            }
        }
    }

    void promptVaultUnlock() {
        bool ok = false;
        const QString pass = QInputDialog::getText(
            this, "Unlock Shuna Vault", "Enter Shuna vault passphrase:", QLineEdit::Password, QString(), &ok);
        if (!ok || pass.isEmpty()) {
            return;
        }
        appendCommandEcho("[GUI -> CLI] unlock ********");
        QJsonObject p;
        p["passphrase"] = pass;
        QJsonObject resp;
        if (executeAction("vault_unlock", p, resp, false) && resp.value("ok").toBool(false)) {
            m_vaultLocked = false;
            m_vaultWiped = false;
            m_vaultFailedAttempts = 0;
            m_vaultRetryAfterSec = 0;
            updateListenersTable();
            updateStatusStrip();
            maybeLoadProfileFromDb();
        }
    }

    void promptVaultRotate() {
        bool ok = false;
        const QString pass = QInputDialog::getText(
            this, "Online Shuna DEK Rotation",
            "Optional new vault passphrase (leave empty to re-key with active passphrase):",
            QLineEdit::Password, QString(), &ok);
        if (!ok) {
            return;
        }
        appendCommandEcho(pass.isEmpty() ? "[GUI -> CLI] rotate" : "[GUI -> CLI] rotate ********");
        QJsonObject p;
        if (!pass.isEmpty()) {
            p["new_passphrase"] = pass;
        }
        QJsonObject resp;
        (void)executeAction("vault_rotate", p, resp, false);
        updateStatusStrip();
    }

    void promptBackupExport() {
        bool ok = false;
        const QString name = QInputDialog::getText(
            this, "Export Encrypted SHUNAS1 Backup",
            "Backup filename (confined under <workDir>/backups):",
            QLineEdit::Normal, "ciel-fleet.shunas1", &ok).trimmed();
        if (!ok) {
            return;
        }
        appendCommandEcho("[GUI -> CLI] backup " + name);
        QJsonObject p;
        if (!name.isEmpty()) {
            p["out_path"] = name;
        }
        QJsonObject resp;
        (void)executeAction("backup_export", p, resp, false);
    }

    void promptSessionIssue() {
        bool ok = false;
        const QString fp = QInputDialog::getText(
            this, "Issue Cert-Bound Session Token",
            "Target mTLS Client Certificate SHA-256 Fingerprint (64 lowercase hex chars):",
            QLineEdit::Normal, QString(), &ok).trimmed();
        if (!ok || fp.isEmpty()) {
            return;
        }
        handleOperativeCommand("session_issue " + fp);
    }

    void promptCertRevoke() {
        bool ok = false;
        const QString serial = QInputDialog::getText(
            this, "Revoke mTLS Client Certificate",
            "Certificate serial (hex) and optional reason (e.g. <serial_hex> key_compromise):",
            QLineEdit::Normal, QString(), &ok).trimmed();
        if (!ok || serial.isEmpty()) {
            return;
        }
        handleOperativeCommand("cert_revoke " + serial);
    }

    void openTanukiModuleModal(const QString &subcmd) {
        commitTargetFieldsFromEdits();
        QDialog dlg(this);
        dlg.setObjectName("ModuleModal");
        dlg.setWindowTitle(QString("Tanuki Module :: tanuki %1").arg(subcmd));
        dlg.setMinimumWidth(560);
        auto *vbox = new QVBoxLayout(&dlg);

        auto *tabs = new QTabWidget(&dlg);
        auto *optPage = new QWidget(tabs);
        auto *form = new QFormLayout(optPage);

        auto *argEdit = new QLineEdit(optPage);
        auto *chkOpsec = new QCheckBox("Enable OPSEC endpoint sensor probe (--opsec)", optPage);
        auto *adEnctypeEdit = new QLineEdit(optPage);
        auto *adKvnoEdit = new QLineEdit(optPage);
        auto *chkApply = new QCheckBox("Execute live repair instead of dry-run (--apply)", optPage);
        auto *chkEnforceAes = new QCheckBox("Strictly enforce AES-128/256 (--enforce-aes)", optPage);
        auto *chkFast = new QCheckBox("Enable RFC 6113 FAST armoring (--fast)", optPage);
        auto *armorEdit = new QLineEdit(optPage);
        auto *chkCtypes = new QCheckBox("Force userspace ctypes libkrb5 (--use-ctypes)", optPage);
        auto *chkSsl = new QCheckBox("Use LDAPS TLS (--ssl)", optPage);
        auto *tlsModeCombo = new QComboBox(optPage);
        tlsModeCombo->addItems({"--secure (Verify System Trust Store)", "--verify-ca (Pin CA Bundle)", "--insecure (Lab Self-Signed Opt-Out)"});
        auto *verifyCaEdit = new QLineEdit(optPage);
        auto *nhiModeCombo = new QComboBox(optPage);
        nhiModeCombo->addItems({"scan (Local Workload Tokens)", "mesh (SPIFFE & Cloud IMDS)", "inspect (Validate Workload JWT)", "exchange (RFC 8693 Token Exchange)"});
        auto *chkLive = new QCheckBox("Execute live HTTP/HTTPS STS exchange (--live)", optPage);
        auto *endpointEdit = new QLineEdit(optPage);

        chkOpsec->setVisible(subcmd == "doctor");
        chkOpsec->setChecked(subcmd == "doctor");
        adEnctypeEdit->setVisible(subcmd == "doctor");
        adEnctypeEdit->setPlaceholderText("Optional hex/int e.g. 0x18");
        adKvnoEdit->setVisible(subcmd == "doctor");
        adKvnoEdit->setPlaceholderText("Optional AD msDS-KeyVersionNumber e.g. 3");
        chkApply->setVisible(subcmd == "fix");
        chkEnforceAes->setVisible(subcmd == "config");
        chkFast->setVisible(subcmd == "config" || subcmd == "auth");
        armorEdit->setVisible(subcmd == "config" || subcmd == "auth");
        armorEdit->setPlaceholderText("Optional armor ccache path (--armor-cache)");
        chkCtypes->setVisible(subcmd == "auth" || subcmd == "tgs");
        chkSsl->setVisible(subcmd == "ldap");
        tlsModeCombo->setVisible(subcmd == "ldap");
        verifyCaEdit->setVisible(subcmd == "ldap");
        verifyCaEdit->setPlaceholderText("CA PEM path when --verify-ca is selected");
        nhiModeCombo->setVisible(subcmd == "nhi");
        chkLive->setVisible(subcmd == "nhi");
        endpointEdit->setVisible(subcmd == "nhi");
        endpointEdit->setPlaceholderText("STS endpoint URL (--endpoint https://sts.corp.local/oauth/token)");

        if (subcmd == "keytab") {
            argEdit->setText(!m_committedKeytab.isEmpty() ? m_committedKeytab : m_resolvedKeytab);
        } else if (subcmd == "config") {
            argEdit->setText("--realm CORP.LOCAL --kdc 192.168.56.106");
        } else if (subcmd == "triage") {
            argEdit->setText("SKEW");
        } else if (subcmd == "auth") {
            const QString kt = !m_committedKeytab.isEmpty() ? m_committedKeytab : m_resolvedKeytab;
            argEdit->setText(QString("--keytab %1 --principal host/app@CORP.LOCAL --kdc 192.168.56.106").arg(kt));
        } else if (subcmd == "tgs") {
            argEdit->setText("--spn HTTP/web.corp.local --kdc 192.168.56.106");
        } else if (subcmd == "ldap") {
            argEdit->setText("--host dc01.corp.local --realm CORP.LOCAL --query all");
        } else if (subcmd == "blast") {
            if (!m_committedKeytab.isEmpty()) argEdit->setText("--keytab " + m_committedKeytab);
            else if (!m_committedCcache.isEmpty()) argEdit->setText("--ccache " + m_committedCcache);
        }

        form->addRow("Module Arguments:", argEdit);
        if (subcmd == "doctor") {
            form->addRow("", chkOpsec);
            form->addRow("AD Enctypes (--ad-enctypes):", adEnctypeEdit);
            form->addRow("AD KVNO (--ad-kvno):", adKvnoEdit);
        } else if (subcmd == "fix") {
            form->addRow("", chkApply);
        } else if (subcmd == "config" || subcmd == "auth") {
            if (subcmd == "config") form->addRow("", chkEnforceAes);
            form->addRow("", chkFast);
            form->addRow("Armor CCACHE:", armorEdit);
            if (subcmd == "auth") form->addRow("", chkCtypes);
        } else if (subcmd == "tgs") {
            form->addRow("", chkCtypes);
        } else if (subcmd == "ldap") {
            form->addRow("", chkSsl);
            form->addRow("LDAPS Verification:", tlsModeCombo);
            form->addRow("CA Bundle (--verify-ca):", verifyCaEdit);
        } else if (subcmd == "nhi") {
            form->addRow("NHI Operation:", nhiModeCombo);
            form->addRow("", chkLive);
            form->addRow("STS Endpoint:", endpointEdit);
        }

        auto *prevPage = new QWidget(tabs);
        auto *prevLay = new QVBoxLayout(prevPage);
        auto *previewEdit = new QPlainTextEdit(prevPage);
        previewEdit->setReadOnly(true);
        prevLay->addWidget(previewEdit);

        auto buildArgsList = [&]() -> QStringList {
            QStringList args;
            if (subcmd == "nhi") {
                const QString modeWord = nhiModeCombo->currentText().section(' ', 0, 0);
                args << modeWord;
                if (chkLive->isChecked()) args << "--live";
                if (!endpointEdit->text().trimmed().isEmpty()) args << "--endpoint" << endpointEdit->text().trimmed();
            }
            if (!argEdit->text().trimmed().isEmpty()) {
                args.append(splitCommandLineArgs(argEdit->text().trimmed()));
            }
            if (subcmd == "doctor") {
                if (chkOpsec->isChecked() && !args.contains("--opsec")) args << "--opsec";
                if (!adEnctypeEdit->text().trimmed().isEmpty()) args << "--ad-enctypes" << adEnctypeEdit->text().trimmed();
                if (!adKvnoEdit->text().trimmed().isEmpty()) args << "--ad-kvno" << adKvnoEdit->text().trimmed();
            } else if (subcmd == "fix") {
                if (chkApply->isChecked()) args << "--apply";
            } else if (subcmd == "config" || subcmd == "auth") {
                if (subcmd == "config" && chkEnforceAes->isChecked()) args << "--enforce-aes";
                if (chkFast->isChecked()) args << "--fast";
                if (!armorEdit->text().trimmed().isEmpty()) args << "--armor-cache" << armorEdit->text().trimmed();
                if (subcmd == "auth" && chkCtypes->isChecked()) args << "--use-ctypes";
            } else if (subcmd == "tgs" && chkCtypes->isChecked()) {
                args << "--use-ctypes";
            } else if (subcmd == "ldap" && chkSsl->isChecked()) {
                args << "--ssl";
                if (tlsModeCombo->currentIndex() == 0) args << "--secure";
                else if (tlsModeCombo->currentIndex() == 1 && !verifyCaEdit->text().trimmed().isEmpty()) {
                    args << "--verify-ca" << verifyCaEdit->text().trimmed();
                } else if (tlsModeCombo->currentIndex() == 2) args << "--insecure";
            }
            return args;
        };

        auto updatePreview = [&]() {
            const QStringList args = buildArgsList();
            const QString cli = QString("tanuki %1 %2 --json").arg(subcmd, args.join(' ')).simplified();
            QJsonObject rpc = targetParams();
            rpc["action"] = "tanuki_exec";
            rpc["subcommand"] = subcmd;
            if (!args.isEmpty()) {
                rpc["args"] = QJsonArray::fromStringList(args);
            }
            previewEdit->setPlainText(
                "[GUI -> CLI Command Preview]\n" + redactConsoleCommand(cli) + "\n\n" +
                "[mTLS 1.3 RPC Payload Preview]\n" +
                QString::fromUtf8(QJsonDocument(rpc).toJson(QJsonDocument::Indented)));
        };

        connect(tabs, &QTabWidget::currentChanged, &dlg, [&](int) { updatePreview(); });
        connect(argEdit, &QLineEdit::textChanged, &dlg, [&](const QString &) { updatePreview(); });
        updatePreview();

        tabs->addTab(optPage, "Options");
        tabs->addTab(prevPage, "Preview & CLI");
        vbox->addWidget(tabs);

        auto *btnRow = new QHBoxLayout();
        btnRow->addStretch(1);
        auto *btnCancel = new QPushButton("Cancel", &dlg);
        btnCancel->setObjectName("SecondaryBtn");
        auto *btnRun = new QPushButton("Execute Module", &dlg);
        btnRun->setObjectName("PrimaryBtn");
        connect(btnCancel, &QPushButton::clicked, &dlg, &QDialog::reject);
        connect(btnRun, &QPushButton::clicked, &dlg, &QDialog::accept);
        btnRow->addWidget(btnCancel);
        btnRow->addWidget(btnRun);
        vbox->addLayout(btnRow);

        if (dlg.exec() == QDialog::Accepted) {
            const QStringList args = buildArgsList();
            if (args.size() == 1 && !args.first().startsWith('-')) {
                dispatchTanuki(subcmd, args.first());
            } else {
                dispatchTanuki(subcmd, QString(), args);
            }
        }
    }

    void openTempestModuleModal(const QString &action) {
        commitTargetFieldsFromEdits();
        QDialog dlg(this);
        dlg.setObjectName("ModuleModal");
        dlg.setWindowTitle("Tempest Runtime Operative :: " + action);
        dlg.setMinimumWidth(520);
        auto *vbox = new QVBoxLayout(&dlg);

        auto *tabs = new QTabWidget(&dlg);
        auto *optPage = new QWidget(tabs);
        auto *form = new QFormLayout(optPage);

        auto *pathEdit = new QLineEdit(optPage);
        auto *auxEdit = new QLineEdit(optPage);
        auto *warnEdit = new QLineEdit(optPage);
        auto *chkFlag = new QCheckBox(optPage);

        if (action == "souei_watch") {
            pathEdit->setText(!m_committedCcache.isEmpty() ? m_committedCcache : (m_resolvedCCache.isEmpty() ? "/etc/krb5.conf" : m_resolvedCCache));
            auxEdit->setText("1000");
            form->addRow("Watch Target Path (--watch-file):", pathEdit);
            form->addRow("Timeout ms (100..10000):", auxEdit);
        } else if (action == "chronoa_check") {
            pathEdit->setText(!m_committedCcache.isEmpty() ? m_committedCcache : m_resolvedCCache);
            auxEdit->setPlaceholderText("Optional KDC Unix epoch (--kdc-time)");
            warnEdit->setText("300");
            chkFlag->setText("Execute live TGT renewal via kinit -R (--renew)");
            form->addRow("CCACHE Path (--ccache):", pathEdit);
            form->addRow("KDC Time Epoch (--kdc-time):", auxEdit);
            form->addRow("Warn Window Sec (--warn-window-sec):", warnEdit);
            form->addRow("", chkFlag);
        } else if (action == "ranga_purge") {
            pathEdit->setPlaceholderText("Explicit regular file path to 3-pass NIST wipe & unlink");
            chkFlag->setText("Also revoke active process session keyring (--revoke-session)");
            form->addRow("Purge Target File (--purge):", pathEdit);
            form->addRow("", chkFlag);
        }

        auto *prevPage = new QWidget(tabs);
        auto *prevLay = new QVBoxLayout(prevPage);
        auto *previewEdit = new QPlainTextEdit(prevPage);
        previewEdit->setReadOnly(true);
        prevLay->addWidget(previewEdit);

        auto buildPayload = [&](QString &cliOut) -> QJsonObject {
            QJsonObject p = targetParams();
            if (action == "souei_watch") {
                const QString wp = pathEdit->text().trimmed();
                const int ms = qBound(100, auxEdit->text().trimmed().toInt(), 10000);
                p["watch_path"] = wp;
                p["watch_timeout_ms"] = ms;
                cliOut = QString("souei --watch-file %1 --timeout-ms %2 --json").arg(wp).arg(ms);
            } else if (action == "chronoa_check") {
                const QString cp = pathEdit->text().trimmed();
                if (!cp.isEmpty()) p["ccache_path"] = cp;
                const qint64 kdcEpoch = auxEdit->text().trimmed().toLongLong();
                if (kdcEpoch > 0) p["kdc_time_epoch"] = kdcEpoch;
                const qint64 warnSec = warnEdit->text().trimmed().toLongLong();
                if (warnSec > 0) p["warn_window_sec"] = warnSec;
                if (chkFlag->isChecked()) p["renew"] = true;
                cliOut = QString("chronoa --ccache %1%2 --warn-window-sec %3%4 --once --json")
                             .arg(cp.isEmpty() ? "<auto>" : cp,
                                  kdcEpoch > 0 ? QString(" --kdc-time %1").arg(kdcEpoch) : "",
                                  QString::number(warnSec > 0 ? warnSec : 300),
                                  chkFlag->isChecked() ? " --renew" : "");
            } else if (action == "ranga_purge") {
                const QString pp = pathEdit->text().trimmed();
                p["purge_path"] = pp;
                if (chkFlag->isChecked()) p["revoke_session"] = true;
                cliOut = QString("ranga --purge %1%2 --json").arg(pp, chkFlag->isChecked() ? " --revoke-session" : "");
            }
            return p;
        };

        auto updatePreview = [&]() {
            QString cli;
            QJsonObject rpc = buildPayload(cli);
            rpc["action"] = action;
            previewEdit->setPlainText(
                "[GUI -> CLI Command Preview]\n" + cli + "\n\n" +
                "[mTLS 1.3 RPC Payload Preview]\n" +
                QString::fromUtf8(QJsonDocument(rpc).toJson(QJsonDocument::Indented)));
        };
        connect(tabs, &QTabWidget::currentChanged, &dlg, [&](int) { updatePreview(); });
        connect(pathEdit, &QLineEdit::textChanged, &dlg, [&](const QString &) { updatePreview(); });
        updatePreview();

        tabs->addTab(optPage, "Options");
        tabs->addTab(prevPage, "Preview & CLI");
        vbox->addWidget(tabs);

        auto *btnRow = new QHBoxLayout();
        btnRow->addStretch(1);
        auto *btnCancel = new QPushButton("Cancel", &dlg);
        btnCancel->setObjectName("SecondaryBtn");
        auto *btnRun = new QPushButton(action == "ranga_purge" ? "Confirm & Purge" : "Execute Operative", &dlg);
        btnRun->setObjectName(action == "ranga_purge" ? "DangerBtn" : "PrimaryBtn");
        connect(btnCancel, &QPushButton::clicked, &dlg, &QDialog::reject);
        connect(btnRun, &QPushButton::clicked, &dlg, &QDialog::accept);
        btnRow->addWidget(btnCancel);
        btnRow->addWidget(btnRun);
        vbox->addLayout(btnRow);

        if (dlg.exec() == QDialog::Accepted) {
            QString cli;
            const QJsonObject p = buildPayload(cli);
            if (action == "ranga_purge") {
                const QString pp = p.value("purge_path").toString();
                if (pp.isEmpty()) return;
                const auto confirm = QMessageBox::warning(
                    this, "Confirm NIST SP 800-88 File Purge",
                    QString("Permanently 3-pass overwrite and unlink:\n\n%1").arg(pp),
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
                if (confirm != QMessageBox::Yes) return;
            }
            dispatchFromUi(action, p, cli);
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
                                       const QColor &baseColor = QColor("#F3F4F6"),
                                       const QColor &badgeColor = QColor("#60A5FA"),
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
                    vColor = cielStatusColor(v).name();
                } else if (k == "exit" || k == "rc") {
                    vColor = (v == "0") ? "#10B981" : "#EF4444";
                } else if (k == "principal" || k == "target" || k == "path" ||
                           k == "session_keyring" || k == "keyring_id" || k == "enctype") {
                    if (isAlert) {
                        vColor = "#EF4444";
                    } else {
                        vColor = (v == "-" || v == "none") ? "#F59E0B" : "#60A5FA";
                    }
                } else if (k == "possessor_only") {
                    vColor = (v == "true") ? "#10B981" : "#F59E0B";
                } else if (k == "removed" || k == "renewed") {
                    vColor = (v == "true") ? "#10B981" : "#9CA3AF";
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
            "<span style=\"color:#9CA3AF\">%1</span> "
            "<span style=\"color:%2\">[%3]</span> "
            "<span style=\"color:#F3F4F6\">%4</span>")
                                 .arg(htmlEsc(ts), tagColor, htmlEsc(tag), bodyHtml);
        appendHtmlLine(m_eventViewer, line);
    }

    void appendOperativeBanner() {
        const QString procName = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
        const QString pidStr = QString::number(QCoreApplication::applicationPid());
        const QString banner = QString(
            "<span style=\"color:#9CA3AF\">%1</span> "
            "<span style=\"color:#60A5FA; font-weight:bold;\">Ciel Identity Runtime Console</span> "
            "<span style=\"color:#F3F4F6\">:: connected as </span>"
            "<span style=\"color:#93C5FD\">%2\\%3</span> "
            "<span style=\"color:#F3F4F6\">[Endpoint: </span>"
            "<span style=\"color:#60A5FA\">%4</span>"
            "<span style=\"color:#F3F4F6\">] [Process: </span>"
            "<span style=\"color:#F59E0B\">%5\\%6</span>"
            "<span style=\"color:#F3F4F6\">] [Arch: </span>"
            "<span style=\"color:#93C5FD\">%7</span>"
            "<span style=\"color:#F3F4F6\">] [Transport: </span>"
            "<span style=\"color:#10B981\">mTLS 1.3 %8</span>"
            "<span style=\"color:#F3F4F6\">]</span>")
                                   .arg(nowTime(), htmlEsc(m_localHost), htmlEsc(m_localUser),
                                        htmlEsc(m_serverAddr),
                                        htmlEsc(procName.isEmpty() ? "ciel-client" : procName),
                                        htmlEsc(pidStr), htmlEsc(m_localArch), htmlEsc(m_pqcGroup));
        appendHtmlLine(m_operativeConsole, banner);
    }

    void appendCommandEcho(const QString &cmd) {
        const QString safeCmd = redactConsoleCommand(cmd);
        const QString chevron = QString::fromUtf8("\xc2\xbb");
        const QString line = QString(
            "<br><span style=\"color:#9CA3AF\">%1 [%2@%3]</span> "
            "<u style=\"color:#60A5FA\">Ciel</u> "
            "<span style=\"color:#93C5FD\">%4</span> "
            "<span style=\"color:#F3F4F6\">%5</span>")
                                 .arg(nowTime(), htmlEsc(m_localUser), htmlEsc(m_localHost), chevron, htmlEsc(safeCmd));
        appendHtmlLine(m_operativeConsole, line);
    }

    void appendTaskInfo(const QString &msg) {
        QString ts;
        const QString cleanMsg = stripLeadingTimestamp(msg, ts);
        const QString line = QString(
            "<span style=\"color:#60A5FA\">[*] %1</span>")
                                 .arg(htmlEsc(cleanMsg));
        appendHtmlLine(m_operativeConsole, line);
    }

    void appendTaskDone(const QString &msg) {
        QString ts;
        const QString cleanMsg = stripLeadingTimestamp(msg, ts);
        const QString line = QString(
            "<span style=\"color:#10B981\">[+]</span> %1")
                                 .arg(formatTelemetryHtml(cleanMsg, QColor("#F3F4F6"), QColor("#10B981"), false));
        appendHtmlLine(m_operativeConsole, line);
    }

    void appendTaskWarn(const QString &msg) {
        QString ts;
        const QString cleanMsg = stripLeadingTimestamp(msg, ts);
        const QString line = QString(
            "<span style=\"color:#F59E0B\">[!]</span> %1")
                                 .arg(formatTelemetryHtml(cleanMsg, QColor("#F3F4F6"), QColor("#F59E0B"), false));
        appendHtmlLine(m_operativeConsole, line);
    }

    void appendTaskNeutral(const QString &msg) {
        QString ts;
        const QString cleanMsg = stripLeadingTimestamp(msg, ts);
        const QString line = QString(
            "<span style=\"color:#93C5FD\">[^]</span> "
            "<span style=\"color:#F3F4F6\">%1</span>")
                                 .arg(htmlEsc(cleanMsg));
        appendHtmlLine(m_operativeConsole, line);
    }

    void appendTaskAlert(const QString &msg) {
        QString ts;
        const QString cleanMsg = stripLeadingTimestamp(msg, ts);
        const QString line = QString(
            "<span style=\"color:#EF4444\">[!]</span> %1")
                                 .arg(formatTelemetryHtml(cleanMsg, QColor("#EF4444"), QColor("#EF4444"), true));
        appendHtmlLine(m_operativeConsole, line);
    }

    void appendHelpTableToConsole() {
        const QString tableText =
            "\n"
            "  Command         Arguments        Description\n"
            "  -------         ---------        -----------\n"
            "  sync                             Full host sweep across Souei, Chronoa, Ranga, and Tanuki\n"
            "  set             &lt;key&gt; &lt;val&gt;  Set target (ccache|keytab|krb5conf|kcm|clear) or show active\n"
            "  hosts                            List fleet hosts decrypted from PostgreSQL via Shuna\n"
            "  history         [op_id] [limit]  Inspect decrypted audit events &amp; telemetry snapshots\n"
            "  unlock          [passphrase]     Unlock Shuna vault (rate-limited Argon2id)\n"
            "  rotate          [new_pass]       Dual-key online Shuna DEK rotation (batched re-encrypt)\n"
            "  backup          [out_path]       Export SHUNAS1 encrypted pg_dump archive under backups/\n"
            "  token_rotate                     Rotate mTLS-bound 256-bit session auth_token (30s grace)\n"
            "  session_issue   &lt;fp&gt; [serial]    Issue mTLS-bound session token for secondary operator cert\n"
            "  cert_revoke     &lt;serial&gt; [why]   Revoke mTLS client cert in CRL + DB &amp; drop sockets\n"
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
            "  audit / graph / table / clear    Print telemetry matrix or switch Operatives / Session Graph\n";
        appendHtmlLine(m_operativeConsole, QString("<pre style=\"color:#F3F4F6; margin:0;\">%1</pre>").arg(tableText));
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
                                              .arg(cielStatusColor(s.status).name(), htmlEsc(statusPadded));
            matrix += QString("  %1  %2  %3  %4  %5  %6\n")
                          .arg(htmlEsc(idCol.leftJustified(6, ' ')),
                               htmlEsc(opCol.leftJustified(9, ' ')),
                               htmlEsc(modCol.leftJustified(18, ' ')),
                               htmlEsc(targetCol.leftJustified(33, ' ')),
                               statusColored,
                               htmlEsc(auxCombined));
        }
        appendHtmlLine(m_operativeConsole, QString("<pre style=\"color:#F3F4F6; margin:0;\">%1</pre>").arg(matrix));
    }

    void dispatchFromUi(const QString &action, const QJsonObject &params, const QString &echoLabel) {
        const QString prefixed = echoLabel.startsWith("[GUI -> CLI]") ? echoLabel : ("[GUI -> CLI] " + echoLabel);
        appendCommandEcho(prefixed);
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
            switchWorkspaceMode(0);
            m_mainViewTabs->setCurrentIndex(1);
            rebuildSessionGraph();
            return;
        }
        if (cmd == "table") {
            switchWorkspaceMode(0);
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
                if (m_ccacheEdit) m_ccacheEdit->clear();
                if (m_keytabEdit) m_keytabEdit->clear();
                if (m_krb5ConfEdit) m_krb5ConfEdit->clear();
                if (m_kcmSockEdit) m_kcmSockEdit->clear();
                commitTargetFieldsFromEdits();
                saveProfileIfDbConnected();
                appendTaskDone("Cleared custom target overrides (reverted to host auto-discovery; run 'sync' to refresh)");
                return;
            }
            if (key == "ccache") {
                if (m_ccacheEdit) m_ccacheEdit->setText(val);
            } else if (key == "keytab") {
                if (m_keytabEdit) m_keytabEdit->setText(val);
            } else if (key == "krb5conf" || key == "krb5.conf") {
                if (m_krb5ConfEdit) m_krb5ConfEdit->setText(val);
            } else if (key == "kcm") {
                if (m_kcmSockEdit) m_kcmSockEdit->setText(val);
            } else {
                appendTaskAlert("Usage: set <ccache|keytab|krb5conf|kcm|clear> [path]");
                return;
            }
            commitTargetFieldsFromEdits();
            saveProfileIfDbConnected();
            appendTaskDone(QString("Updated %1 = %2 (run 'sync' to probe new target)").arg(key, val.isEmpty() ? "<auto>" : val));
            return;
        }
        if (cmd == "hosts") {
            QJsonObject resp;
            if (!executeAction("host_list", QJsonObject(), resp, false)) {
                return;
            }
            const QJsonArray hosts = resp.value("data").toObject().value("hosts").toArray();
            if (hosts.isEmpty()) {
                appendTaskInfo("No encrypted fleet hosts recorded in PostgreSQL yet.");
                return;
            }
            QString table = "\n  HOST_ID (HMAC)    HOSTNAME          USER@ARCH            KV   STATUS      LAST_SEEN\n"
                            "  --------------    --------          ---------            --   ------      ---------\n";
            for (const QJsonValue &hv : hosts) {
                const QJsonObject o = hv.toObject();
                const QJsonObject m = o.value("meta").toObject();
                const QString hid = o.value("host_id").toString().left(14);
                const QString hn = m.value("hostname").toString().left(16);
                const QString ua = QString("%1/%2").arg(m.value("username").toString(), m.value("arch").toString()).left(19);
                const QString kv = QString::number(o.value("key_version").toInt());
                const QString st = o.value("status").toString().left(10);
                const QString ls = o.value("last_seen_at").toString().left(19);
                table += QString("  %1  %2  %3  %4  %5  %6\n")
                             .arg(htmlEsc(hid.leftJustified(16, ' ')),
                                  htmlEsc(hn.leftJustified(16, ' ')),
                                  htmlEsc(ua.leftJustified(19, ' ')),
                                  htmlEsc(kv.leftJustified(3, ' ')),
                                  htmlEsc(st.leftJustified(10, ' ')),
                                  htmlEsc(ls));
            }
            appendHtmlLine(m_operativeConsole, QString("<pre style=\"color:#F3F4F6; margin:0;\">%1</pre>").arg(table));
            return;
        }
        if (cmd == "history") {
            QJsonObject p;
            if (tokens.size() >= 2) {
                const QString arg1 = tokens[1];
                if (arg1.startsWith("OP-", Qt::CaseInsensitive)) {
                    p["operative_id"] = arg1.toUpper();
                } else {
                    bool okLim = false;
                    const int lim = arg1.toInt(&okLim);
                    if (okLim && lim > 0) {
                        p["limit"] = lim;
                    } else {
                        p["host_id"] = arg1;
                    }
                }
            }
            if (tokens.size() >= 3) {
                bool okLim = false;
                const int lim = tokens[2].toInt(&okLim);
                if (okLim && lim > 0) {
                    p["limit"] = lim;
                }
            }
            QJsonObject resp;
            if (!executeAction("history_list", p, resp, false)) {
                return;
            }
            const QJsonObject d = resp.value("data").toObject();
            const QJsonArray evs = d.value("events").toArray();
            const QJsonArray snaps = d.value("snapshots").toArray();
            appendTaskInfo(QString("Decrypted %1 audit event(s) and %2 snapshot(s) from PostgreSQL:")
                               .arg(evs.size()).arg(snaps.size()));
            for (const QJsonValue &evVal : evs) {
                const QJsonObject ev = evVal.toObject();
                const QJsonObject det = ev.value("detail").toObject();
                appendTaskNeutral(QString("[%1] %2 %3 (%4) :: %5")
                                      .arg(ev.value("created_at").toString().left(19),
                                           ev.value("operative_id").toString(),
                                           ev.value("status").toString(),
                                           ev.value("severity").toString(),
                                           det.value("summary").toString()));
            }
            return;
        }
        if (cmd == "unlock") {
            QString pass;
            if (tokens.size() >= 2) {
                pass = tokens.mid(1).join(' ');
            } else {
                bool okPass = false;
                pass = QInputDialog::getText(this, "Unlock Shuna Vault", "Vault passphrase:", QLineEdit::Password, QString(), &okPass);
                if (!okPass || pass.isEmpty()) {
                    return;
                }
            }
            QJsonObject p;
            p["passphrase"] = pass;
            QJsonObject resp;
            if (executeAction("vault_unlock", p, resp, false) && resp.value("ok").toBool(false)) {
                m_vaultLocked = false;
                m_vaultWiped = false;
                m_vaultFailedAttempts = 0;
                m_vaultRetryAfterSec = 0;
                updateStatusStrip();
                maybeLoadProfileFromDb();
            }
            return;
        }
        if (cmd == "rotate") {
            QJsonObject p;
            if (tokens.size() >= 2) {
                p["new_passphrase"] = tokens.mid(1).join(' ');
            }
            QJsonObject resp;
            (void)executeAction("vault_rotate", p, resp, false);
            updateStatusStrip();
            return;
        }
        if (cmd == "backup") {
            QJsonObject p;
            if (tokens.size() >= 2) {
                p["out_path"] = tokens[1];
            }
            QJsonObject resp;
            (void)executeAction("backup_export", p, resp, false);
            return;
        }
        if (cmd == "token_rotate") {
            QJsonObject resp;
            if (executeAction("token_rotate", QJsonObject(), resp, false) && resp.value("ok").toBool(false)) {
                const QString newTok = resp.value("data").toObject().value("auth_token").toString();
                if (!newTok.isEmpty()) {
                    m_authToken = newTok;
                    appendTaskDone("Updated active session auth_token in client memory (30s grace overlap)");
                }
            }
            return;
        }
        if (cmd == "session_issue") {
            if (tokens.size() < 2) {
                appendTaskAlert("Usage: session_issue <client_cert_sha256_fp> [serial_hex]");
                return;
            }
            QJsonObject p;
            p["cert_fp"] = tokens[1];
            if (tokens.size() >= 3) {
                p["serial_hex"] = tokens[2];
            }
            QJsonObject resp;
            if (executeAction("session_issue", p, resp, false) && resp.value("ok").toBool(false)) {
                const QJsonObject d = resp.value("data").toObject();
                appendTaskDone(QString("Issued mTLS-bound session token for cert_fp=%1 (expires=%2)")
                                   .arg(d.value("cert_fp").toString().left(16),
                                        d.value("token_expires_at").toString()));
            }
            return;
        }
        if (cmd == "cert_revoke") {
            if (tokens.size() < 2) {
                appendTaskAlert("Usage: cert_revoke <serial_hex> [reason]");
                return;
            }
            QJsonObject p;
            p["serial_hex"] = tokens[1];
            if (tokens.size() >= 3) {
                p["reason"] = tokens[2];
            }
            QJsonObject resp;
            (void)executeAction("cert_revoke", p, resp, false);
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

        const QString permAuxCombined = (snap.skewOrAux.isEmpty() || snap.skewOrAux == "-")
                                            ? snap.permOrTtl
                                            : QString("%1 | %2").arg(snap.permOrTtl, snap.skewOrAux);

        // 6-column InfinityCurve Operatives matrix:
        // 0: ID | 1: Operative | 2: Module | 3: Target / Principal | 4: Perm / TTL / Aux | 5: Status
        const QStringList cells = {
            snap.id,
            snap.operative,
            snap.module,
            snap.target,
            permAuxCombined,
            snap.status
        };

        const QColor badgeAccent = cielStatusColor(snap.status);
        const bool isAlert = isAlertSnapshot(snap);
        const bool isWarn = !isAlert && isWarningLogEntry(QString(), snap.status);
        const QColor col0IconColor = isAlert ? QColor("#EF4444") : (isWarn ? QColor("#F59E0B") : QColor("#60A5FA"));
        const QColor col0TextColor = isAlert ? QColor("#EF4444") : QColor("#F3F4F6");
        for (int c = 0; c < cells.size(); ++c) {
            QTableWidgetItem *item = m_sessionTable->item(targetRow, c);
            if (!item) {
                item = new QTableWidgetItem();
                item->setTextAlignment((c == 3 || c == 4) ? (Qt::AlignLeft | Qt::AlignVCenter) : Qt::AlignCenter);
                m_sessionTable->setItem(targetRow, c, item);
            }
            if (item->text() != cells.at(c)) {
                item->setText(cells.at(c));
                item->setToolTip(QString("%1 (%2) :: %3\nDetail: %4").arg(snap.id, snap.operative, cells.at(c), snap.detail));
            }
            if (c == 0) {
                item->setIcon(makeOperativeBadgeIcon(col0IconColor));
                item->setForeground(col0TextColor);
            } else if (c == 5) {
                item->setForeground(badgeAccent);
            } else {
                item->setForeground(QColor("#F3F4F6"));
            }
        }
        if (m_filterEdit && !m_filterEdit->text().trimmed().isEmpty()) {
            applySessionTableFilter(m_filterEdit->text());
        }
        updateStatusStrip();
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
        if (!m_inspectorSummary) {
            return;
        }
        if (m_inspectedOpId.isEmpty()) {
            m_inspectedOpId = snap.id;
        }
        if (m_inspectedOpId != snap.id) {
            return;
        }
        const QString stableBody = buildDotLeaderForSnapshot(snap);
        if (m_lastContextSig.value(snap.id) == stableBody && m_summarySourceOpId == snap.id) {
            return;
        }
        m_lastContextSig[snap.id] = stableBody;
        m_summarySourceOpId = snap.id;
        if (m_inspectorTitleLabel) {
            m_inspectorTitleLabel->setText(QString("INSPECTOR :: %1 (%2 :: %3)")
                                               .arg(snap.id, snap.operative, snap.module));
        }
        updateEditorPreservingScroll(m_inspectorSummary, stableBody);
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
            m_teamserverOnline ? QColor("#60A5FA") : QColor("#EF4444"),
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
            const QColor stCol = cielStatusColor(s.status);
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

        // Reingold-Tilford left-to-right tree:
        // Root mTLS shield hub (far left x~90) <- emerald callback arrows from Column 1 (x~330) -> blue/red forward arrows to Column 2 (x~625)
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
            const QColor edgeCol = isAlertSnapshot(s) ? QColor("#EF4444") : QColor("#10B981");
            m_graphScene->addItem(new CielGraphEdge(rootNode, n1, edgeCol, edgeLbl, true, 0.30));
        }
        if (m_snapshots.contains("OP-02")) {
            const OperativeSnapshot &s = m_snapshots.value("OP-02");
            n2 = makeRealMonitor(s, "chronoa", QPointF(345.0, 172.0));
            const QString edgeLbl = (s.detail.isEmpty() || s.detail == "-") ? "ccache" : s.detail.section('-', 0, 0);
            const QColor edgeCol = isAlertSnapshot(s) ? QColor("#EF4444") : QColor("#10B981");
            m_graphScene->addItem(new CielGraphEdge(rootNode, n2, edgeCol, edgeLbl, true, 0.50));
        }
        if (m_snapshots.contains("OP-06")) {
            const OperativeSnapshot &s = m_snapshots.value("OP-06");
            n6 = makeRealMonitor(s, "tanuki", QPointF(315.0, 294.0));
            const QColor edgeCol = isAlertSnapshot(s) ? QColor("#EF4444") : QColor("#10B981");
            m_graphScene->addItem(new CielGraphEdge(rootNode, n6, edgeCol, s.module.isEmpty() ? "nhi-rpc" : s.module, true, 0.52));
        }

        // Column 2 (x~625-650): Derived kernel/filesystem/AD operatives (forward arrows)
        if (m_snapshots.contains("OP-04")) {
            const OperativeSnapshot &s = m_snapshots.value("OP-04");
            n4 = makeRealMonitor(s, "ranga-iso", QPointF(625.0, 48.0));
            CielGraphNode *src = n1 ? n1 : rootNode;
            const QColor edgeCol = isAlertSnapshot(s) ? QColor("#EF4444") : QColor("#3B82F6");
            m_graphScene->addItem(new CielGraphEdge(src, n4, edgeCol, s.module, false, 0.50));
        }
        if (m_snapshots.contains("OP-03")) {
            const OperativeSnapshot &s = m_snapshots.value("OP-03");
            n3 = makeRealMonitor(s, "ranga-shared", QPointF(650.0, 162.0));
            CielGraphNode *src = n1 ? n1 : rootNode;
            const QColor edgeCol = isAlertSnapshot(s) ? QColor("#EF4444") : QColor("#3B82F6");
            m_graphScene->addItem(new CielGraphEdge(src, n3, edgeCol, "keyctl", false, 0.34));
        }
        if (m_snapshots.contains("OP-05")) {
            const OperativeSnapshot &s = m_snapshots.value("OP-05");
            n5 = makeRealMonitor(s, "ranga-purge", QPointF(630.0, 294.0));
            CielGraphNode *src = n2 ? n2 : rootNode;
            const QColor edgeCol = (s.status == "PURGED" || isAlertSnapshot(s)) ? QColor("#EF4444") : QColor("#3B82F6");
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
        if (data.contains("db_connected")) {
            m_dbConnected = data.value("db_connected").toBool(false);
        }
        if (data.contains("ssl_mode")) {
            m_sslMode = data.value("ssl_mode").toString("standalone");
        }
        if (data.contains("pqc_group")) {
            const QString g = data.value("pqc_group").toString();
            if (!g.isEmpty()) {
                m_pqcGroup = g;
            }
        }
        if (data.contains("shuna_key_version")) {
            m_shunaKeyVersion = data.value("shuna_key_version").toInt(0);
        }
        if (data.contains("vault_locked")) {
            m_vaultLocked = data.value("vault_locked").toBool(false);
        }
        if (data.contains("vault_wiped")) {
            m_vaultWiped = data.value("vault_wiped").toBool(false);
        } else if (data.contains("wiped_locked")) {
            m_vaultWiped = data.value("wiped_locked").toBool(false);
        }
        if (data.contains("failed_attempts")) {
            m_vaultFailedAttempts = data.value("failed_attempts").toInt(0);
        }
        if (data.contains("retry_after_sec")) {
            m_vaultRetryAfterSec = data.value("retry_after_sec").toInt(0);
        }
        if (data.contains("rotation_in_progress")) {
            m_rotationInProgress = data.value("rotation_in_progress").toBool(false);
        }
        if (data.contains("token_expires_at")) {
            m_tokenExpiresAt = data.value("token_expires_at").toString();
        }
        if (data.contains("client_cert_serial")) {
            m_clientCertSerial = data.value("client_cert_serial").toString();
        }
        if (data.contains("host_id")) {
            m_hostId = data.value("host_id").toString();
        }

        if (changed && m_sessionBanner) {
            m_sessionBanner->setText(
                QString("[%1@%2] Ciel Identity Runtime (%3 / uid=%4)")
                    .arg(m_localUser, m_localHost, m_localArch, m_localUid));
        }
        if (m_kpiFreshnessLabel) {
            m_kpiFreshnessLabel->setText("Sync: " + nowTime());
        }

        const QString vaultState = (m_shunaKeyVersion == 0 && !m_dbConnected)
                                       ? QString("ephemeral (no vault)")
                                       : (m_vaultLocked
                                              ? QString("VAULT_LOCKED (key_version=%1)").arg(m_shunaKeyVersion)
                                              : QString("UNLOCKED (key_version=%1%2)").arg(m_shunaKeyVersion).arg(m_rotationInProgress ? ", rotating" : ""));
        const QString dbState = m_dbConnected
                                    ? QString("connected (sslmode=%1)").arg(m_sslMode)
                                    : QString("standalone (ephemeral)");

        QStringList tsLines = {
            formatDotLeader("Teamserver Endpoint", m_serverAddr),
            formatDotLeader("Transport Security", QString("mTLS 1.3 (%1)").arg(m_pqcGroup)),
            formatDotLeader("Client Cert Serial", m_clientCertSerial.isEmpty() ? "-" : m_clientCertSerial),
            formatDotLeader("Token Expires At", m_tokenExpiresAt.isEmpty() ? "-" : m_tokenExpiresAt),
            formatDotLeader("PostgreSQL FleetDB", dbState),
            formatDotLeader("Shuna Vault State", vaultState),
            formatDotLeader("Operator Identity", QString("%1@%2 (uid=%3, %4)").arg(m_localUser, m_localHost, m_localUid, m_localArch)),
            formatDotLeader("Workspace Dir", m_workDir),
            formatDotLeader("PKI Directory", m_pkiDir),
            formatDotLeader("Resolved CCACHE", m_resolvedCCache.isEmpty() ? "none" : m_resolvedCCache),
            formatDotLeader("Resolved Keytab", m_resolvedKeytab.isEmpty() ? "none" : m_resolvedKeytab),
            formatDotLeader("Resolved krb5.conf", m_resolvedKrb5Conf.isEmpty() ? "none" : m_resolvedKrb5Conf),
            formatDotLeader("Resolved KCM Socket", m_resolvedKcmSocket.isEmpty() ? "none (passive)" : m_resolvedKcmSocket),
            formatDotLeader("kinit -R Renewal", m_kinitAvailable ? "available" : "unavailable")
        };
        m_teamserverSummaryText = tsLines.join('\n');
        if (m_inspectedOpId == "TEAMSERVER" && m_inspectorSummary) {
            updateEditorPreservingScroll(m_inspectorSummary, m_teamserverSummaryText);
        }
        updateStatusStrip();
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
            appendEventLog("!", "#EF4444", "Invalid teamserver address: " + htmlEsc(m_serverAddr));
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
            appendEventLog("!", "#EF4444", "Unable to locate ciel-teamserver binary");
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
            QStringList srvArgs = {"--addr", m_serverAddr, "--work-dir", m_workDir, "--pki-dir", m_pkiDir};
            if (!m_tokenFilePath.isEmpty()) {
                srvArgs << "--token-file" << m_tokenFilePath;
            }
            if (m_allowClassicalTlsFallback) {
                srvArgs << "--allow-classical-tls-fallback";
            }
            m_managedServer->start(binPath, srvArgs);
            if (!m_managedServer->waitForStarted(2000)) {
                appendEventLog("!", "#EF4444", "Failed launching ciel-teamserver: " + htmlEsc(binPath));
                return false;
            }
        }

        for (int attempt = 0; attempt < 25; ++attempt) {
            if (probeTcpNonBlocking(host, port, 150)) {
                if (!m_loggedManagedBoot) {
                    m_loggedManagedBoot = true;
                    appendEventLog("+", "#10B981",
                        QString("Initialized <span style=\"color:#10B981\">ciel-teamserver</span> :: <span style=\"color:#60A5FA\">%1</span> (Tempest + Tanuki)")
                            .arg(htmlEsc(m_serverAddr)));
                }
                return true;
            }
            if (m_managedServer->state() == QProcess::NotRunning) {
                appendEventLog("!", "#EF4444", "Managed ciel-teamserver exited before listening");
                return false;
            }
            QEventLoop pauseLoop;
            QTimer::singleShot(100, &pauseLoop, &QEventLoop::quit);
            pauseLoop.exec();
        }
        appendEventLog("!", "#EF4444", "Timed out waiting for managed ciel-teamserver on " + htmlEsc(m_serverAddr));
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
            rowStatus == "USAGE_ERROR" || rowStatus == "UNAUTHORIZED" ||
            rowStatus == "VAULT_LOCKED") {
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

    bool buildSslConfiguration(QSslConfiguration &cfgOut, QString &errOut) const {
        if (!QSslSocket::supportsSsl()) {
            errOut = "Qt6 TLS/OpenSSL backend unavailable";
            return false;
        }
        const QString caPath = m_pkiDir + "/ca.crt";
        const QString certPath = m_pkiDir + "/client.crt";
        const QString keyPath = m_pkiDir + "/client.key";
        const QList<QSslCertificate> caCerts = QSslCertificate::fromPath(caPath, QSsl::Pem);
        if (caCerts.isEmpty()) {
            errOut = "Missing or invalid CA certificate: " + caPath;
            return false;
        }
        const QList<QSslCertificate> cliCerts = QSslCertificate::fromPath(certPath, QSsl::Pem);
        if (cliCerts.isEmpty()) {
            errOut = "Missing or invalid mTLS client certificate: " + certPath;
            return false;
        }
        QFile keyFile(keyPath);
        if (!keyFile.open(QIODevice::ReadOnly)) {
            errOut = "Unable to open mTLS client private key: " + keyPath;
            return false;
        }
        const QByteArray keyBytes = keyFile.readAll();
        keyFile.close();
        QSslKey cliKey(keyBytes, QSsl::Ec, QSsl::Pem, QSsl::PrivateKey);
        if (cliKey.isNull()) {
            cliKey = QSslKey(keyBytes, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
        }
        if (cliKey.isNull()) {
            errOut = "Invalid mTLS client private key PEM: " + keyPath;
            return false;
        }

        cfgOut = QSslConfiguration::defaultConfiguration();
        cfgOut.setProtocol(QSsl::TlsV1_3);
        cfgOut.setPeerVerifyMode(QSslSocket::VerifyPeer);
        cfgOut.setCaCertificates(caCerts);
        cfgOut.setLocalCertificate(cliCerts.first());
        cfgOut.setPrivateKey(cliKey);
        cfgOut.setBackendConfigurationOption(
            "Groups",
            m_allowClassicalTlsFallback ? QByteArray("X25519MLKEM768:X25519")
                                        : QByteArray("X25519MLKEM768"));
        return true;
    }

    bool executeAction(const QString &action, const QJsonObject &extraParams, QJsonObject &respOut, bool background = false) {
        if (m_actionInFlight) {
            if (!background && action != "full_sync") {
                appendTaskInfo("Action already in flight; skipping overlapping request: " + action);
            }
            return false;
        }
        BoolScopeGuard inFlightGuard(m_actionInFlight);
        QPointer<CielWindow> safeThis(this);
        if (m_statusRpcLabel) {
            m_statusRpcLabel->setText("RPC: " + action);
            m_statusRpcLabel->setStyleSheet("color: #60A5FA; font-weight: bold;");
        }
        struct RpcIdleReset {
            QPointer<CielWindow> win;
            ~RpcIdleReset() {
                if (win && win->m_statusRpcLabel) {
                    win->m_statusRpcLabel->setText("RPC: IDLE");
                    win->m_statusRpcLabel->setStyleSheet("color: #9CA3AF;");
                }
            }
        } rpcReset{safeThis};

        QString host;
        quint16 port = 0;
        if (!parseHostPort(m_serverAddr, host, port)) {
            m_teamserverOnline = false;
            updateStatusStrip();
            if (!background) {
                appendEventLog("!", "#EF4444", "Invalid teamserver address: " + htmlEsc(m_serverAddr));
            }
            return false;
        }

        if (!ensureTeamserverRunning()) {
            if (!safeThis) {
                return false;
            }
            if (m_teamserverOnline || !background) {
                appendEventLog("!", "#EF4444", "Teamserver unreachable at " + htmlEsc(m_serverAddr));
            }
            m_teamserverOnline = false;
            updateStatusStrip();
            return false;
        }
        if (!safeThis) {
            return false;
        }

        if (m_authToken.isEmpty()) {
            m_authToken = loadAuthTokenFromDisk();
        }

        QSslConfiguration sslCfg;
        QString sslCfgErr;
        if (!buildSslConfiguration(sslCfg, sslCfgErr)) {
            m_teamserverOnline = false;
            updateStatusStrip();
            if (!background) {
                appendEventLog("!", "#EF4444", "mTLS configuration failed (fail-closed): " + htmlEsc(sslCfgErr));
                appendTaskAlert("mTLS config error: " + sslCfgErr);
            }
            return false;
        }

        QJsonObject req = extraParams;
        req["action"] = action;
        if (!m_authToken.isEmpty()) {
            req["auth_token"] = m_authToken;
        }
        const QByteArray wire = QJsonDocument(req).toJson(QJsonDocument::Compact) + '\n';

        QSslSocket sock;
        sock.setSslConfiguration(sslCfg);
        QEventLoop loop;
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);

        QByteArray rx;
        bool gotLine = false;
        bool timedOut = false;
        bool writeErr = false;
        QString sockErr;
        QStringList tlsErrList;

        connect(&sock, &QSslSocket::encrypted, &loop, [&]() {
            if (sock.write(wire) != wire.size()) {
                writeErr = true;
                sockErr = "mTLS write error for action " + action;
                loop.quit();
                return;
            }
            sock.flush();
        });
        connect(&sock, &QSslSocket::sslErrors, &loop, [&](const QList<QSslError> &errs) {
            for (const QSslError &e : errs) {
                tlsErrList.append(e.errorString());
            }
            // Fail closed: never ignoreSslErrors().
        });
        connect(&sock, &QSslSocket::readyRead, &loop, [&]() {
            if (sock.isOpen() && sock.bytesAvailable() > 0) {
                rx.append(sock.readAll());
                if (rx.contains('\n')) {
                    gotLine = true;
                    loop.quit();
                }
            }
        });
        connect(&sock, &QSslSocket::errorOccurred, &loop, [&](QAbstractSocket::SocketError) {
            if (!gotLine) {
                sockErr = tlsErrList.isEmpty() ? sock.errorString() : tlsErrList.join("; ");
                loop.quit();
            }
        });
        connect(&sock, &QSslSocket::disconnected, &loop, [&]() {
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
        sock.connectToHostEncrypted(host, port);
        loop.exec();
        timeoutTimer.stop();
        disconnect(&sock, nullptr, nullptr, nullptr);
        if (sock.state() != QAbstractSocket::UnconnectedState) {
            sock.abort();
        }
        if (!safeThis) {
            return false;
        }

        if (timedOut) {
            if (m_teamserverOnline || !background) {
                appendEventLog("!", "#EF4444", "Timeout waiting for teamserver mTLS action " + htmlEsc(action));
            }
            m_teamserverOnline = false;
            updateStatusStrip();
            return false;
        }
        if (writeErr || !gotLine) {
            const QString msg = !tlsErrList.isEmpty()
                                    ? ("mTLS verification failed: " + tlsErrList.join("; "))
                                    : (sockErr.isEmpty() ? ("Failed mTLS RPC for action " + action) : sockErr);
            if (m_teamserverOnline || !background) {
                appendEventLog("!", "#EF4444", htmlEsc(msg));
            }
            m_teamserverOnline = false;
            updateStatusStrip();
            return false;
        }

        const int nl = rx.indexOf('\n');
        QJsonParseError parseErr;
        const QJsonDocument doc = QJsonDocument::fromJson(rx.left(nl).trimmed(), &parseErr);
        if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
            m_teamserverOnline = false;
            updateStatusStrip();
            appendEventLog("!", "#EF4444", "JSON parse failure: " + htmlEsc(parseErr.errorString()));
            return false;
        }

        respOut = doc.object();
        const QString respStatus = respOut.value("status").toString();
        if (respStatus == "UNAUTHORIZED") {
            const bool firstAuthFail = !m_authFailed;
            m_authFailed = true;
            m_teamserverOnline = false;
            updateStatusStrip();
            if (m_actPoll && m_actPoll->isChecked()) {
                m_actPoll->setChecked(false);
            }
            const QString errMsg = respOut.value("error").toString("Unauthorized RPC token");
            if (firstAuthFail || !background) {
                appendEventLog("!", "#EF4444", "Teamserver auth failed: " + htmlEsc(errMsg));
                if (!background) {
                    appendTaskAlert("Teamserver rejected auth_token: " + errMsg);
                }
            }
            return false;
        }
        m_authFailed = false;

        if (respStatus == "VAULT_LOCKED") {
            m_vaultLocked = true;
            m_teamserverOnline = true;
            updateTelemetryEnvironment(respOut.value("data").toObject());
            updateStatusStrip();
            const QString errMsg = respOut.value("error").toString("Shuna vault is locked");
            if (!background) {
                appendEventLog("WARN", "#F59E0B", "Shuna vault locked: " + htmlEsc(errMsg));
                appendTaskWarn("Shuna vault is locked (" + errMsg + "). Run 'unlock <passphrase>' in Operative Console.");
            }
            return false;
        }

        const bool wasOffline = !m_teamserverOnline;
        m_teamserverOnline = true;
        if (background && wasOffline && m_hasSyncedOnce) {
            appendEventLog("+", "#10B981", "Teamserver connection restored on " + htmlEsc(m_serverAddr));
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
                const QString body = formatTelemetryHtml(cleanLine, QColor("#F3F4F6"), opCol, alertFlag);
                if (rowId.isEmpty()) {
                    return body;
                }
                return QString("<span style=\"color:%1\">%2</span> <span style=\"color:#F3F4F6\">::</span> %3")
                    .arg(opCol.name(), htmlEsc(rowId), body);
            };
            if (isAlertLogEntry(rawLine, rowStatus, rowExitCode)) {
                if (emitToEventViewer) {
                    appendEventLog("!", "#EF4444", buildEventBody(QColor("#EF4444"), true), logTs);
                }
                if (!background) {
                    appendTaskAlert(cleanLine);
                }
            } else if (isWarningLogEntry(rawLine, rowStatus)) {
                if (emitToEventViewer) {
                    appendEventLog("WARN", "#F59E0B", buildEventBody(QColor("#F59E0B"), false), logTs);
                }
                if (!background) {
                    appendTaskWarn(cleanLine);
                }
            } else if (rowExitCode == 0) {
                if (emitToEventViewer) {
                    appendEventLog("+", "#10B981", buildEventBody(QColor("#10B981"), false), logTs);
                }
                if (!background) {
                    appendTaskDone(cleanLine);
                }
            } else {
                if (emitToEventViewer) {
                    appendEventLog("*", "#60A5FA", buildEventBody(QColor("#60A5FA"), false), logTs);
                }
                if (!background) {
                    appendTaskInfo(cleanLine);
                }
            }
        }

        updateStatusStrip();

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

        if (m_mainViewTabs && m_mainViewTabs->currentIndex() == 1) {
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
    QString m_pkiDir;
    bool m_allowClassicalTlsFallback = false;
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
    QString m_pqcGroup = "X25519MLKEM768";
    QString m_sslMode = "standalone";
    QString m_tokenExpiresAt;
    QString m_clientCertSerial;
    QString m_hostId;
    int m_shunaKeyVersion = 0;
    bool m_dbConnected = false;
    bool m_vaultLocked = false;
    bool m_vaultWiped = false;
    int m_vaultFailedAttempts = 0;
    int m_vaultRetryAfterSec = 0;
    bool m_rotationInProgress = false;
    bool m_profileLoadedFromDb = false;
    bool m_kinitAvailable = false;
    bool m_actionInFlight = false;
    bool m_teamserverOnline = false;
    bool m_authFailed = false;
    bool m_hasSyncedOnce = false;
    bool m_rawShowingExplicitAction = false;
    bool m_graphUserZoomed = false;
    bool m_initialSplitApplied = false;
    int m_consoleTabIdx = 0;
    int m_eventTabIdx = 1;
    QString m_lastGraphSig;
    QString m_rawSourceOpId;
    QString m_inspectedOpId;
    QString m_summarySourceOpId;
    QString m_teamserverSummaryText;

    QStackedWidget *m_workspaceStack = nullptr;
    QList<QPushButton *> m_railBtns;
    QLabel *m_kpiTransportBadge = nullptr;
    QLabel *m_kpiVaultBadge = nullptr;
    QLabel *m_kpiOpsBadge = nullptr;
    QLabel *m_kpiFreshnessLabel = nullptr;
    QWidget *m_vaultBanner = nullptr;
    QLabel *m_vaultBannerLabel = nullptr;

    QSplitter *m_topBotSplitter = nullptr;
    QSplitter *m_topSplit = nullptr;
    QTabWidget *m_mainViewTabs = nullptr;
    QTableWidget *m_sessionTable = nullptr;
    QLineEdit *m_filterEdit = nullptr;
    CielGraphView *m_graphView = nullptr;
    QGraphicsScene *m_graphScene = nullptr;
    QPushButton *m_graphZoomOutBtn = nullptr;
    QPushButton *m_graphZoomBtn = nullptr;
    QPushButton *m_graphZoomInBtn = nullptr;
    QPushButton *m_graphFitBtn = nullptr;

    QWidget *m_inspectorDrawer = nullptr;
    QLabel *m_inspectorTitleLabel = nullptr;
    QTabWidget *m_smallTabs = nullptr;
    QTextEdit *m_inspectorSummary = nullptr;
    QPlainTextEdit *m_rawTelemetry = nullptr;

    QTableWidget *m_hostsTable = nullptr;
    QTableWidget *m_historyTable = nullptr;
    QTableWidget *m_listenersTable = nullptr;
    QComboBox *m_historyFilterCombo = nullptr;
    QLabel *m_vaultDetailLabel = nullptr;
    QLabel *m_pkiDetailLabel = nullptr;

    QTabWidget *m_bottomTabs = nullptr;
    QTextEdit *m_eventViewer = nullptr;
    QTextEdit *m_operativeConsole = nullptr;
    QLabel *m_sessionBanner = nullptr;
    QLineEdit *m_operativeInput = nullptr;

    QLabel *m_statusTransportLabel = nullptr;
    QLabel *m_statusCertLabel = nullptr;
    QLabel *m_statusDbLabel = nullptr;
    QLabel *m_statusVaultLabel = nullptr;
    QLabel *m_statusPollLabel = nullptr;
    QLabel *m_statusRpcLabel = nullptr;

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
    const QCommandLineOption pkiDirOpt("pki-dir", "PKI directory containing ca.crt, client.crt, client.key", "path");
    const QCommandLineOption allowClassicalOpt("allow-classical-tls-fallback", "Allow X25519 fallback alongside X25519MLKEM768");
    const QCommandLineOption selfTestOpt("self-test", "Run automated desktop RPC verification and exit");
    const QCommandLineOption shotOpt("screenshot", "Save PNG screenshot of Sessions Table view", "path");
    const QCommandLineOption graphShotOpt("graph-screenshot", "Save PNG screenshot of Session Graph view", "path");
    const QCommandLineOption altShotOpt("alt-screenshot", "Save PNG screenshot of secondary Telemetry/Listeners view", "path");
    const QCommandLineOption consoleShotOpt("console-screenshot", "Save PNG screenshot of Operative Console view", "path");
    parser.addOption(addrOpt);
    parser.addOption(tokenOpt);
    parser.addOption(tokenFileOpt);
    parser.addOption(pkiDirOpt);
    parser.addOption(allowClassicalOpt);
    parser.addOption(selfTestOpt);
    parser.addOption(shotOpt);
    parser.addOption(graphShotOpt);
    parser.addOption(altShotOpt);
    parser.addOption(consoleShotOpt);
    parser.process(app);

    CielWindow win(
        parser.value(addrOpt),
        parser.value(tokenOpt),
        parser.value(tokenFileOpt),
        parser.value(pkiDirOpt),
        parser.isSet(allowClassicalOpt));
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
