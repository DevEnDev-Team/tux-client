#ifndef UPDATEMANAGER_H
#define UPDATEMANAGER_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QIcon>

class UpdateManager : public QObject {
    Q_OBJECT
public:
    explicit UpdateManager(QObject *parent = nullptr);
    ~UpdateManager() override;

    bool isGitRepository() const;
    QString getGitRepoPath() const;
    void checkForUpdates(bool silent = false);
    void applyUpdate();

    static QIcon createUpdateIcon(bool hasUpdate, bool isDark = false);
    static QIcon createRefreshIcon(bool isDark = false);

signals:
    void updateCheckStarted();
    void updateAvailable(const QString& currentHash, const QString& remoteHash, const QString& commitSubject);
    void noUpdateAvailable(bool silent);
    void updateCheckFailed(const QString& errorMsg, bool silent);

    void updateProgress(const QString& stepDescription);
    void updateFinished(bool success, const QString& message);

private slots:
    void onFetchFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void checkGitDiff();

    QProcess* m_fetchProcess = nullptr;
    bool m_silentCheck = false;
    bool m_isUpdating = false;
};

#endif // UPDATEMANAGER_H
