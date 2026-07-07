#include "syncmanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDateTime>

SyncManager::SyncManager(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    loadConfig();
}

void SyncManager::loadConfig() {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configPath = configDir + "/sync_config.json";

    // Si le dossier n'existe pas, le créer
    QDir().mkpath(configDir);

    QFile file(configPath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject obj = doc.object();
        m_serverUrl = obj.value("server_url").toString().trimmed();
        m_apiKey = obj.value("api_key").toString().trimmed();
        m_isConfigured = obj.value("installed").toBool(false);
        file.close();
    } else {
        m_isConfigured = false;
        // Créer un fichier de configuration exemple vide
        if (file.open(QIODevice::WriteOnly)) {
            QJsonObject obj;
            obj["server_url"] = "";
            obj["api_key"] = "";
            obj["installed"] = false;
            file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
            file.close();
        }
    }
}

void SyncManager::saveConfig(const QString& serverUrl, const QString& apiKey, bool configured) {
    m_serverUrl = serverUrl.trimmed();
    m_apiKey = apiKey.trimmed();
    m_isConfigured = configured;

    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configPath = configDir + "/sync_config.json";

    QFile file(configPath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject obj;
        obj["server_url"] = m_serverUrl;
        obj["api_key"] = m_apiKey;
        obj["installed"] = m_isConfigured;
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        file.close();
    }
}

void SyncManager::pushNotes(const std::vector<NoteModel>& notes) {
    if (!isEnabled()) {
        emit syncFinished(false);
        return;
    }

    QUrl url(m_serverUrl + "/notes");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_apiKey.toUtf8());

    QByteArray data = serializeNotes(notes);
    QNetworkReply* reply = m_networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        bool success = (reply->error() == QNetworkReply::NoError);
        emit syncFinished(success);
    });
}

void SyncManager::pullNotes(std::function<void(const std::vector<NoteModel>&, bool success)> callback) {
    if (!isEnabled()) {
        callback({}, false);
        return;
    }

    QUrl url(m_serverUrl + "/notes");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + m_apiKey.toUtf8());

    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            std::vector<NoteModel> remoteNotes = deserializeNotes(data);
            callback(remoteNotes, true);
        } else {
            callback({}, false);
        }
    });
}

std::vector<NoteModel> SyncManager::mergeNotes(const std::vector<NoteModel>& local, const std::vector<NoteModel>& remote) {
    std::vector<NoteModel> merged;

    // 1. Parcourir les notes locales
    for (const auto& lNote : local) {
        // Chercher si la note existe sur le serveur distant
        auto rIt = std::find_if(remote.begin(), remote.end(), [&lNote](const NoteModel& rNote) {
            return rNote.id == lNote.id;
        });

        if (rIt != remote.end()) {
            // Note présente des deux côtés : conserver la plus récente
            if (rIt->lastModified > lNote.lastModified) {
                merged.push_back(*rIt);
            } else {
                merged.push_back(lNote);
            }
        } else {
            // Note présente uniquement en local
            // Si la note a déjà été synchronisée avec le serveur par le passé (synced == true),
            // cela signifie qu'elle a été supprimée sur un autre poste. On ne la conserve pas.
            // Sinon, c'est une nouvelle note locale pas encore poussée, on la garde pour la synchroniser.
            if (!lNote.synced) {
                merged.push_back(lNote);
            }
        }
    }

    // 2. Ajouter les notes distantes qui ne sont pas présentes en local
    for (const auto& rNote : remote) {
        auto lIt = std::find_if(local.begin(), local.end(), [&rNote](const NoteModel& lNote) {
            return lNote.id == rNote.id;
        });

        if (lIt == local.end()) {
            // La note n'existe pas du tout localement, on l'ajoute
            merged.push_back(rNote);
        }
    }

    return merged;
}

QByteArray SyncManager::serializeNotes(const std::vector<NoteModel>& notes) const {
    QJsonArray array;
    for (const auto& note : notes) {
        QJsonObject obj;
        obj["id"] = note.id;
        obj["content"] = note.content;
        obj["color"] = note.color;
        obj["x"] = note.position.x();
        obj["y"] = note.position.y();
        obj["width"] = note.size.width();
        obj["height"] = note.size.height();
        obj["alwaysOnTop"] = note.alwaysOnTop;
        obj["opacity"] = note.opacity;
        obj["locked"] = note.locked;
        obj["archived"] = note.archived;
        obj["trashed"] = note.trashed;
        obj["favorite"] = note.favorite;
        obj["lastModified"] = note.lastModified.toString(Qt::ISODate);
        obj["created"] = note.created.toString(Qt::ISODate);

        QJsonArray tagsArray;
        for (const auto& tag : note.tags) {
            tagsArray.append(tag);
        }
        obj["tags"] = tagsArray;

        array.append(obj);
    }
    return QJsonDocument(array).toJson();
}

std::vector<NoteModel> SyncManager::deserializeNotes(const QByteArray& data) const {
    std::vector<NoteModel> notes;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) return notes;

    QJsonArray array = doc.array();
    for (int i = 0; i < array.size(); ++i) {
        QJsonObject obj = array.at(i).toObject();
        NoteModel note;
        note.id = obj.value("id").toString();
        note.content = obj.value("content").toString();
        note.color = obj.value("color").toString();
        note.position = QPoint(obj.value("x").toInt(150), obj.value("y").toInt(150));
        note.size = QSize(obj.value("width").toInt(250), obj.value("height").toInt(250));
        note.alwaysOnTop = obj.value("alwaysOnTop").toBool(false);
        note.opacity = obj.value("opacity").toDouble(1.0);
        note.locked = obj.value("locked").toBool(false);
        note.archived = obj.value("archived").toBool(false);
        note.trashed = obj.value("trashed").toBool(false);
        note.favorite = obj.value("favorite").toBool(false);
        note.lastModified = QDateTime::fromString(obj.value("lastModified").toString(), Qt::ISODate);
        note.created = QDateTime::fromString(obj.value("created").toString(), Qt::ISODate);

        QJsonArray tagsArray = obj.value("tags").toArray();
        for (int j = 0; j < tagsArray.size(); ++j) {
            note.tags.append(tagsArray.at(j).toString());
        }

        notes.push_back(note);
    }
    return notes;
}
