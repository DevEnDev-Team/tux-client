#include "updatemanager.h"
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>
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

static QString checkGitRepo(const QString& path) {
    if (path.isEmpty()) return QString();
    QDir dir(path);
    if (!dir.exists()) return QString();

    QProcess proc;
    proc.setWorkingDirectory(path);
    proc.start("git", QStringList() << "rev-parse" << "--show-toplevel");
    if (proc.waitForFinished(2000) && proc.exitCode() == 0) {
        QString repoPath = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
        if (!repoPath.isEmpty() && QDir(repoPath).exists()) {
            return repoPath;
        }
    }
    return QString();
}

static void saveRepoPath(const QString& repoPath) {
    if (repoPath.isEmpty()) return;
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    QFile file(QDir(configDir).filePath("repo_path.txt"));
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << repoPath << "\n";
    }
}

QString UpdateManager::getGitRepoPath() const {
    // 1. Lire le chemin enregistré dans le fichier de configuration (AppConfigLocation)
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configFilePath = QDir(configDir).filePath("repo_path.txt");
    QFile file(configFilePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString savedPath = in.readLine().trimmed();
        file.close();
        QString valid = checkGitRepo(savedPath);
        if (!valid.isEmpty()) return valid;
    }

    // 1b. Fallback dossier config alternatif ~/.config/tux-it/repo_path.txt
    QString altConfigFilePath = QDir::homePath() + "/.config/tux-it/repo_path.txt";
    QFile altFile(altConfigFilePath);
    if (altFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&altFile);
        QString savedPath = in.readLine().trimmed();
        altFile.close();
        QString valid = checkGitRepo(savedPath);
        if (!valid.isEmpty()) {
            saveRepoPath(valid);
            return valid;
        }
    }

    // 2. Variables d'environnement TUX_IT_REPO ou TUX_REPO_PATH
    QString envPath = QString::fromUtf8(qgetenv("TUX_IT_REPO"));
    if (envPath.isEmpty()) envPath = QString::fromUtf8(qgetenv("TUX_REPO_PATH"));
    if (!envPath.isEmpty()) {
        QString valid = checkGitRepo(envPath);
        if (!valid.isEmpty()) {
            saveRepoPath(valid);
            return valid;
        }
    }

    // 3. Essayer le dossier courant de l'application ou ses parents
    QString appValid = checkGitRepo(QCoreApplication::applicationDirPath());
    if (!appValid.isEmpty()) {
        saveRepoPath(appValid);
        return appValid;
    }

    // 4. Essayer le dossier d'exécution de travail courant
    QString currValid = checkGitRepo(QDir::currentPath());
    if (!currValid.isEmpty()) {
        saveRepoPath(currValid);
        return currValid;
    }

    // 5. Recherche dans les chemins usuels connus
    QString home = QDir::homePath();
    QStringList candidates = {
        home + "/Documents/Projets/logiciel/tux-it/tux-client",
        home + "/Documents/Projets/logiciel/tux-it",
        home + "/Documents/Projets/tux-client",
        home + "/Documents/Projets/tux-it",
        home + "/Documents/Projects/tux-client",
        home + "/Documents/Projects/tux-it",
        home + "/Documents/tux-client",
        home + "/Documents/tux-it",
        home + "/Projets/logiciel/tux-it/tux-client",
        home + "/Projets/logiciel/tux-it",
        home + "/Projets/tux-client",
        home + "/Projets/tux-it",
        home + "/Projects/tux-client",
        home + "/Projects/tux-it",
        home + "/tux-client",
        home + "/tux-it",
        home + "/src/tux-client",
        home + "/src/tux-it",
        home + "/dev/tux-client",
        home + "/dev/tux-it",
        home + "/Downloads/tux-client",
        home + "/Downloads/tux-it"
    };

    for (const QString& candidate : candidates) {
        QString valid = checkGitRepo(candidate);
        if (!valid.isEmpty()) {
            saveRepoPath(valid);
            return valid;
        }
    }

    // 6. Recherche dynamique dans les dossiers principaux
    QStringList searchRoots = {
        home + "/Documents",
        home + "/Projects",
        home + "/Projets",
        home + "/Development",
        home + "/src",
        home + "/dev",
        home + "/code"
    };

    for (const QString& root : searchRoots) {
        QDir rootDir(root);
        if (!rootDir.exists()) continue;

        QDirIterator it(root, QStringList() << "tux-client" << "tux-it", QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        int maxChecks = 20;
        while (it.hasNext() && maxChecks-- > 0) {
            QString dirPath = it.next();
            QString valid = checkGitRepo(dirPath);
            if (!valid.isEmpty()) {
                saveRepoPath(valid);
                return valid;
            }
        }
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

    // Si install.sh existe dans repoPath (ou repoPath/tux-client), l'exécuter
    QString installScript = repoPath + "/install.sh";
    QProcess buildProc;

    if (QFile::exists(installScript)) {
        buildProc.setWorkingDirectory(repoPath);
        buildProc.start("bash", QStringList() << installScript);
    } else if (QFile::exists(repoPath + "/tux-client/install.sh")) {
        installScript = repoPath + "/tux-client/install.sh";
        buildProc.setWorkingDirectory(repoPath + "/tux-client");
        buildProc.start("bash", QStringList() << installScript);
    } else {
        buildProc.setWorkingDirectory(repoPath);
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

QIcon UpdateManager::createHourglassIcon(bool isDark) {
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QColor frameColor = isDark ? QColor("#38BDF8") : QColor("#0284C7");
    QColor sandColor  = isDark ? QColor("#F59E0B") : QColor("#D97706");

    painter.setPen(QPen(frameColor, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);

    // Plaques supérieure et inférieure du sablier
    painter.drawLine(9, 6, 23, 6);
    painter.drawLine(9, 26, 23, 26);

    // Verre (corps du sablier)
    QPainterPath glass;
    glass.moveTo(11, 7);
    glass.lineTo(21, 7);
    glass.lineTo(16, 16);
    glass.lineTo(21, 25);
    glass.lineTo(11, 25);
    glass.lineTo(16, 16);
    glass.closeSubpath();
    painter.drawPath(glass);

    // Sable dans la partie inférieure (tas de sable)
    QPainterPath bottomSand;
    bottomSand.moveTo(12, 25);
    bottomSand.lineTo(20, 25);
    bottomSand.lineTo(17, 20);
    bottomSand.lineTo(15, 20);
    bottomSand.closeSubpath();
    painter.setPen(Qt::NoPen);
    painter.setBrush(sandColor);
    painter.drawPath(bottomSand);

    // Fil de sable qui coule au centre
    painter.setPen(QPen(sandColor, 1.5, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(16, 14, 16, 21);

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
