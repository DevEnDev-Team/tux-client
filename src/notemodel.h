#ifndef NOTEMODEL_H
#define NOTEMODEL_H

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QPoint>
#include <QSize>

struct NoteModel {
    QString id;
    QString content;
    QPoint position;
    QSize size;
    QString color; // Hex color like #FFF59D
    bool alwaysOnTop = false;
    double opacity = 1.0;
    bool locked = false;
    bool favorite = false;
    bool archived = false;
    bool trashed = false;
    QDateTime lastModified;
    QDateTime created;
    QStringList tags;
    bool synced = true;
};

inline bool operator==(const NoteModel& lhs, const NoteModel& rhs) {
    return lhs.id == rhs.id &&
           lhs.content == rhs.content &&
           lhs.position == rhs.position &&
           lhs.size == rhs.size &&
           lhs.color == rhs.color &&
           lhs.alwaysOnTop == rhs.alwaysOnTop &&
           lhs.opacity == rhs.opacity &&
           lhs.locked == rhs.locked &&
           lhs.favorite == rhs.favorite &&
           lhs.archived == rhs.archived &&
           lhs.trashed == rhs.trashed &&
           lhs.lastModified == rhs.lastModified &&
           lhs.tags == rhs.tags;
}

inline bool operator!=(const NoteModel& lhs, const NoteModel& rhs) {
    return !(lhs == rhs);
}

#endif // NOTEMODEL_H

