#include "registry.h"

void Registry::add(int id) {
    sensors_.push_back(Sensor{id, 0.0});
}

Sensor* Registry::find(int id) {
    for (Sensor& s : sensors_) {
        if (s.id == id) {
            return &s;
        }
    }
    return nullptr;
}

void Registry::record(int id, double value) {
    if (Sensor* s = find(id)) {
        s->last = value;
    }
}
