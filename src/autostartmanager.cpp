#include "autostartmanager.h"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QPainter>
#include <QPainterPath>

QString AutostartManager::getAutostartFilePath() {
    return QDir::homePath() + "/.config/autostart/tux-it.desktop";
}

bool AutostartManager::isEnabled() {
    return QFile::exists(getAutostartFilePath());
}

bool AutostartManager::setEnabled(bool enable) {
    QString autostartPath = getAutostartFilePath();
    if (!enable) {
        if (QFile::exists(autostartPath)) {
            return QFile::remove(autostartPath);
        }
        return true;
    } else {
        QString homePath = QDir::homePath();
        QString autostartDir = homePath + "/.config/autostart";
        QDir().mkpath(autostartDir);
        QFile file(autostartPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "[Desktop Entry]\n"
                << "Name=Tux-It\n"
                << "Comment=Vos post-its mignons sur le bureau\n"
                << "Exec=" << homePath << "/.local/bin/tux-it\n"
                << "Icon=" << homePath << "/.local/share/icons/tux-it.png\n"
                << "Terminal=false\n"
                << "Type=Application\n"
                << "Categories=Utility;Office;\n"
                << "StartupNotify=true\n";
            file.close();
            QFile::setPermissions(autostartPath, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner | 
                                                 QFile::ReadGroup | QFile::ReadOther);
            return true;
        }
        return false;
    }
}

bool AutostartManager::toggle() {
    return setEnabled(!isEnabled());
}

QIcon AutostartManager::createAutostartIcon(bool enabled, bool darkTheme) {
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QColor strokeColor;
    if (enabled) {
        // Emerald active color in both themes for clear "ON" state
        strokeColor = darkTheme ? QColor("#34d399") : QColor("#059669");
    } else {
        strokeColor = darkTheme ? QColor("#64748b") : QColor("#94a3af");
    }

    QPen pen(strokeColor, 2.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);

    // Power Arc: Circle with gap at the top
    // Qt angle 0 = 3 o'clock, 90 = 12 o'clock (top).
    // Start at 125° (top-left) and sweep -270° (clockwise) to 55° (top-right).
    QRectF arcRect(7.0, 7.0, 18.0, 18.0);
    painter.drawArc(arcRect, 125 * 16, -270 * 16);

    // Vertical top line from (16, 4.5) to (16, 14.5)
    painter.drawLine(QPointF(16.0, 4.5), QPointF(16.0, 14.5));

    // If enabled, add a small subtle power core dot
    if (enabled) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(strokeColor);
        painter.drawEllipse(QPointF(16.0, 19.5), 1.8, 1.8);
    }

    return QIcon(pixmap);
}
