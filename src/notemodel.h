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
    QString color; // Hex color (ex: #FEF08A)
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

// Normalisation des anciennes teintes 2014 vers la nouvelle palette 2026
inline QString normalizeNoteColor(const QString& hex) {
    QString upper = hex.toUpper().trimmed();
    if (upper == "#FFF59D") return "#FEF08A"; // Jaune -> Ambre
    if (upper == "#90CAF9") return "#BAE6FD"; // Bleu -> Azur
    if (upper == "#A5D6A7") return "#BBF7D0"; // Vert -> Sauge
    if (upper == "#FFCC80") return "#FED7AA"; // Orange -> Pêche
    if (upper == "#CE93D8") return "#E9D5FF"; // Violet -> Lavande
    if (upper == "#F8BBD0") return "#FBCFE8"; // Rose -> Rose Poudré
    if (upper == "#E0E0E0") return "#E2E8F0"; // Gris -> Ardoise
    if (upper == "#FAFAFA" || upper == "#FFFFFF") return "#F8FAFC"; // Blanc -> Minéral
    return hex.isEmpty() ? "#FEF08A" : hex;
}

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

