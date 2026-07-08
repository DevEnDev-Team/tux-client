#ifndef LOCALSTORAGEPROVIDER_H
#define LOCALSTORAGEPROVIDER_H

#include "istorageprovider.h"
#include <QString>

class LocalStorageProvider : public IStorageProvider {
public:
    LocalStorageProvider();
    std::vector<NoteModel> loadNotes() override;
    bool saveNotes(const std::vector<NoteModel>& notes) override;

private:
    QString getFilePath() const;
};

#endif // LOCALSTORAGEPROVIDER_H
