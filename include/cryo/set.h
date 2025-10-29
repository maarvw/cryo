#pragma once

namespace cryo {

template<class K>
class Set {
public:
    Set(const K& key)
        : key_(key) {}

    K key() const { return key_; }

private:
    K key_;
};

}
