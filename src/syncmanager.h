#ifndef SYNCMANAGER_H
#define SYNCMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <vector>
#include <functional>
#include "notemodel.h"

class SyncManager : public QObject {
    Q_OBJECT
public:
    explicit SyncManager(QObject* parent = nullptr);

    bool isEnabled() const { return !m_serverUrl.isEmpty() && !m_apiKey.isEmpty(); }
    QString serverUrl() const { return m_serverUrl; }
    QString apiKey() const { return m_apiKey; }
    bool isConfigured() const { return m_isConfigured; }

    void saveConfig(const QString& serverUrl, const QString& apiKey, bool configured);

    // Envoi des notes au serveur (asynchrone)
    void pushNotes(const std::vector<NoteModel>& notes);

signals:
    void syncFinished(bool success);

public:
    // Récupération des notes depuis le serveur (asynchrone avec callback)
    void pullNotes(std::function<void(const std::vector<NoteModel>&, bool success)> callback);

    // Fusion intelligente entre notes locales et distantes
    static std::vector<NoteModel> mergeNotes(const std::vector<NoteModel>& local, const std::vector<NoteModel>& remote);

private:
    void loadConfig();
    QByteArray serializeNotes(const std::vector<NoteModel>& notes) const;
    std::vector<NoteModel> deserializeNotes(const QByteArray& data) const;

    QNetworkAccessManager* m_networkManager;
    QString m_serverUrl;
    QString m_apiKey;
    bool m_isConfigured;
};

#endif // SYNCMANAGER_H
