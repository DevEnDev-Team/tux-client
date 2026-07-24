#include "updatemanager.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QProcess>
#include <QDebug>
#include <QThread>

UpdateManager::UpdateManager(QObject *parent)
    : QObject(parent)
{
    m_fetchProcess = new QProcess(this);
    connect(m_fetchProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &UpdateManager::onFetchFinished);
}

UpdateManager::~UpdateManager() {}

bool UpdateManager::isGitRepository() const {
    return !getGitRepoPath().isEmpty();
}

QString UpdateManager::getGitRepoPath() const {
    // 1. Essayer le dossier courant de l'application ou ses parents
    QProcess proc;
    proc.setWorkingDirectory(QCoreApplication::applicationDirPath());
    proc.start("git", QStringList() << "rev-parse" << "--show-toplevel");
    if (proc.waitForFinished(2000) && proc.exitCode() == 0) {
        QString path = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
        if (!path.isEmpty()) return path;
    }

    // 2. Fallback sur le dossier d'exécution de travail
    proc.setWorkingDirectory(QDir::currentPath());
    proc.start("git", QStringList() << "rev-parse" << "--show-toplevel");
    if (proc.waitForFinished(2000) && proc.exitCode() == 0) {
        QString path = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
        if (!path.isEmpty()) return path;
    }

    return QString();
}

void UpdateManager::checkForUpdates(bool silent) {
    if (m_fetchProcess->state() != QProcess::NotRunning) {
        return; // Une vérification est déjà en cours
    }

    QString repoPath = getGitRepoPath();
    if (repoPath.isEmpty()) {
        emit updateCheckFailed("Aucun dépôt Git détecté pour cette installation.", silent);
        return;
    }

    m_silentCheck = silent;
    emit updateCheckStarted();

    m_fetchProcess->setWorkingDirectory(repoPath);
    m_fetchProcess->start("git", QStringList() << "fetch" << "origin");
}

void UpdateManager::onFetchFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitStatus);

    if (exitCode != 0) {
        emit updateCheckFailed("Impossible de contacter le serveur distant (git fetch a échoué).", m_silentCheck);
        return;
    }

    checkGitDiff();
}

void UpdateManager::checkGitDiff() {
    QString repoPath = getGitRepoPath();

    // 1. Récupérer le commit HEAD local
    QProcess pLocal;
    pLocal.setWorkingDirectory(repoPath);
    pLocal.start("git", QStringList() << "rev-parse" << "HEAD");
    if (!pLocal.waitForFinished(2000) || pLocal.exitCode() != 0) {
        emit updateCheckFailed("Erreur lors de la lecture du commit local.", m_silentCheck);
        return;
    }
    QString localHash = QString::fromUtf8(pLocal.readAllStandardOutput()).trimmed();

    // 2. Récupérer le commit de la branche amont (@{u} ou origin/main)
    QProcess pRemote;
    pRemote.setWorkingDirectory(repoPath);
    pRemote.start("git", QStringList() << "rev-parse" << "@{u}");
    if (!pRemote.waitForFinished(2000) || pRemote.exitCode() != 0) {
        // Fallback sur origin/main
        pRemote.start("git", QStringList() << "rev-parse" << "origin/main");
        if (!pRemote.waitForFinished(2000) || pRemote.exitCode() != 0) {
            emit updateCheckFailed("Erreur lors de la lecture du commit distant.", m_silentCheck);
            return;
        }
    }
    QString remoteHash = QString::fromUtf8(pRemote.readAllStandardOutput()).trimmed();

    if (localHash != remoteHash) {
        // Obtenir le sujet du dernier commit
        QProcess pLog;
        pLog.setWorkingDirectory(repoPath);
        pLog.start("git", QStringList() << "log" << "-1" << "--pretty=%s" << remoteHash);
        QString subject = "Nouvelle mise à jour disponible";
        if (pLog.waitForFinished(2000) && pLog.exitCode() == 0) {
            QString logMsg = QString::fromUtf8(pLog.readAllStandardOutput()).trimmed();
            if (!logMsg.isEmpty()) subject = logMsg;
        }

        emit updateAvailable(localHash.left(7), remoteHash.left(7), subject);
    } else {
        emit noUpdateAvailable(m_silentCheck);
    }
}

void UpdateManager::applyUpdate() {
    if (m_isUpdating) return;
    m_isUpdating = true;

    QString repoPath = getGitRepoPath();
    if (repoPath.isEmpty()) {
        emit updateFinished(false, "Dépôt Git introuvable.");
        m_isUpdating = false;
        return;
    }

    emit updateProgress("Téléchargement de la dernière version (git pull)...");

    QProcess pullProc;
    pullProc.setWorkingDirectory(repoPath);
    pullProc.start("git", QStringList() << "pull");
    if (!pullProc.waitForFinished(30000) || pullProc.exitCode() != 0) {
        QString err = QString::fromUtf8(pullProc.readAllStandardError());
        emit updateFinished(false, "Échec de 'git pull': " + err);
        m_isUpdating = false;
        return;
    }

    emit updateProgress("Compilation et installation de l'application...");

    // Si install.sh existe dans repoPath, l'exécuter
    QString installScript = repoPath + "/install.sh";
    QProcess buildProc;
    buildProc.setWorkingDirectory(repoPath);

    if (QFile::exists(installScript)) {
        buildProc.start("bash", QStringList() << installScript);
    } else {
        // Fallback compilation manuelle avec cmake & make
        buildProc.start("bash", QStringList() << "-c" << "mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j$(nproc) && cp tux-it ~/.local/bin/tux-it");
    }

    if (!buildProc.waitForFinished(120000) || buildProc.exitCode() != 0) {
        QString err = QString::fromUtf8(buildProc.readAllStandardError());
        emit updateFinished(false, "Échec de la compilation: " + err);
        m_isUpdating = false;
        return;
    }

    emit updateProgress("Mise à jour réussie ! Redémarrage de l'application...");
    emit updateFinished(true, "Application mise à jour avec succès.");

    // Relancer la nouvelle version de l'application et quitter l'ancienne
    QString installedBin = QDir::homePath() + "/.local/bin/tux-it";
    if (QFile::exists(installedBin)) {
        QProcess::startDetached(installedBin, QStringList());
    } else {
        QProcess::startDetached(QCoreApplication::applicationFilePath(), QCoreApplication::arguments());
    }

    QCoreApplication::quit();
}

QIcon UpdateManager::createRefreshIcon(bool isDark) {
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QColor color = isDark ? QColor("#E0E0E0") : QColor("#424242");
    painter.setPen(QPen(color, 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);

    // Flèche circulaire de rafraîchissement
    QRectF rect(7, 7, 18, 18);
    painter.drawArc(rect, 40 * 16, 280 * 16);

    // Pointe de la flèche
    QPainterPath arrow;
    arrow.moveTo(22, 5);
    arrow.lineTo(26, 9);
    arrow.lineTo(21, 11);
    painter.setBrush(color);
    painter.drawPath(arrow);

    return QIcon(pixmap);
}

QIcon UpdateManager::createUpdateIcon(bool hasUpdate, bool isDark) {
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QColor mainColor = isDark ? QColor("#E0E0E0") : QColor("#424242");
    if (hasUpdate) {
        mainColor = QColor("#29B6F6"); // Bleu vif quand mise à jour dispo
    }

    painter.setPen(QPen(mainColor, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);

    // Flèche vers le bas (Téléchargement/Update)
    painter.drawLine(16, 6, 16, 20);
    
    QPainterPath arrowHead;
    arrowHead.moveTo(10, 14);
    arrowHead.lineTo(16, 21);
    arrowHead.lineTo(22, 14);
    painter.drawPath(arrowHead);

    // Barre sous la flèche
    painter.drawLine(8, 25, 24, 25);

    // Badge indicateur si une mise à jour est disponible (pastille verte)
    if (hasUpdate) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#66BB6A"));
        painter.drawEllipse(21, 3, 8, 8);
    }

    return QIcon(pixmap);
}
