#include "localstorageprovider.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

LocalStorageProvider::LocalStorageProvider() {}

QString LocalStorageProvider::getFilePath() const {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    return QDir(configDir).filePath("notes.json");
}

std::vector<NoteModel> LocalStorageProvider::loadNotes() {
    std::vector<NoteModel> notes;
    QString path = getFilePath();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        // Return empty list if file doesn't exist
        return notes;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isArray()) {
        return notes;
    }

    QJsonArray array = doc.array();
    for (const QJsonValue& val : array) {
        if (!val.isObject()) continue;
        QJsonObject obj = val.toObject();

        NoteModel note;
        note.id = obj["id"].toString();
        note.content = obj["content"].toString();
        note.position = QPoint(obj["x"].toInt(100), obj["y"].toInt(100));
        note.size = QSize(obj["width"].toInt(250), obj["height"].toInt(250));
        note.color = obj["color"].toString("#FFF59D");
        note.alwaysOnTop = obj["alwaysOnTop"].toBool(false);
        note.opacity = obj["opacity"].toDouble(1.0);
        note.locked = obj["locked"].toBool(false);
        note.favorite = obj["favorite"].toBool(false);
        note.archived = obj["archived"].toBool(false);
        note.trashed = obj["trashed"].toBool(false);
        note.lastModified = QDateTime::fromString(obj["lastModified"].toString(), Qt::ISODate);
        if (!note.lastModified.isValid()) {
            note.lastModified = QDateTime::currentDateTime();
        }
        note.created = QDateTime::fromString(obj["created"].toString(), Qt::ISODate);
        if (!note.created.isValid()) {
            note.created = note.lastModified;
        }

        note.synced = obj["synced"].toBool(true);

        QJsonArray tagsArr = obj["tags"].toArray();
        for (const QJsonValue& tagVal : tagsArr) {
            note.tags.append(tagVal.toString());
        }

        notes.push_back(note);
    }

    return notes;
}

bool LocalStorageProvider::saveNotes(const std::vector<NoteModel>& notes) {
    QString path = getFilePath();
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open notes file for writing:" << path;
        return false;
    }

    QJsonArray array;
    for (const auto& note : notes) {
        QJsonObject obj;
        obj["id"] = note.id;
        obj["content"] = note.content;
        obj["x"] = note.position.x();
        obj["y"] = note.position.y();
        obj["width"] = note.size.width();
        obj["height"] = note.size.height();
        obj["color"] = note.color;
        obj["alwaysOnTop"] = note.alwaysOnTop;
        obj["opacity"] = note.opacity;
        obj["locked"] = note.locked;
        obj["favorite"] = note.favorite;
        obj["archived"] = note.archived;
        obj["trashed"] = note.trashed;
        obj["lastModified"] = note.lastModified.toString(Qt::ISODate);
        obj["created"] = note.created.toString(Qt::ISODate);
        obj["synced"] = note.synced;

        QJsonArray tagsArr;
        for (const QString& tag : note.tags) {
            tagsArr.append(tag);
        }
        obj["tags"] = tagsArr;

        array.append(obj);
    }

    QJsonDocument doc(array);
    file.write(doc.toJson());
    return true;
}
