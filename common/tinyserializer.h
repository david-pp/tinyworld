#ifndef TINYWORLD_TINYSERIALIZER_H
#define TINYWORLD_TINYSERIALIZER_H

#include <string>
#include <utility>

template<typename T>
struct TinySerializer {
    static bool serialize(const T &object, std::string &bin) {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        bin = typeid(T).name();
        return true;
    }

    static bool deserialize(T &object, const std::string &bin) {
        return true;
    }
};

namespace tiny {
    template<typename T>
    inline std::string serialize(T &&object) {

        std::cout << __PRETTY_FUNCTION__ << std::endl;
        std::string bin;
        TinySerializer<T>::serialize(object, bin);
        return bin;
    }

    template <typename T>
    inline bool deserialize(T &&object, const std::string &bin) {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        return TinySerializer<T>::deserialize(object, bin);
    }
}

#endif //TINYWORLD_TINYSERIALIZER_H
