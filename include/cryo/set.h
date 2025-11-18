#pragma once

namespace cryo {

template<typename T>
class Set {
public:

    Set() = default;

    Set(const T& key)
        : key_(key) {}

    T key() const { return key_; }

private:
    T key_;
};

}
