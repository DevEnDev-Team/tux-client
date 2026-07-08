#ifndef ISTORAGEPROVIDER_H
#define ISTORAGEPROVIDER_H

#include <vector>
#include "notemodel.h"

class IStorageProvider {
public:
    virtual ~IStorageProvider() = default;
    virtual std::vector<NoteModel> loadNotes() = 0;
    virtual bool saveNotes(const std::vector<NoteModel>& notes) = 0;
};

#endif // ISTORAGEPROVIDER_H
