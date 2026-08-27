#ifndef LON_MODEL_IO_H
#define LON_MODEL_IO_H

#include "raylib.h"

struct LONModel;

namespace LONModelIO
{
    bool Load(LONModel& model, const char* lonmodelPath, const char* texturePath);
    void Unload(LONModel& model);
}

#endif // LON_MODEL_IO_H
