#pragma once
#include <vector>

struct Sensor {
    int    id;
    double last;    // most recent reading
};

class Registry {
public:
    void add(int id);                     // register a sensor
    Sensor* find(int id);                 // borrow: valid only until the next add()
    void record(int id, double value);    // store a reading

private:
    std::vector<Sensor> sensors_;
};
