#ifndef TINYWORLD_TINYSERIALIZER_DEC_H
#define TINYWORLD_TINYSERIALIZER_DEC_H

#include <string>
#include <utility>

template<typename T, bool ispod>
struct TinySerializerImpl {
    static bool serialize(const T &object, std::string &bin);

    static bool deserialize(T &object, const std::string &bin);
};

template<typename T>
struct TinySerializer {
    static bool serialize(const T &object, std::string &bin) {
        return TinySerializerImpl<T, std::is_trivial<T>::value>::serialize(object, bin);
    }

    static bool deserialize(T &object, const std::string &bin) {
        return TinySerializerImpl<T, std::is_trivial<T>::value>::deserialize(object, bin);
    }
};

namespace tiny {
    template<typename T>
    inline std::string serialize(const T &object) {

        std::cout << __PRETTY_FUNCTION__ << std::endl;
        std::string bin;
        TinySerializer<typename std::remove_reference<T>::type>::serialize(object, bin);
        return bin;
    }

    template<typename T>
    inline bool deserialize(T &object, const std::string &bin) {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        return TinySerializer<typename std::remove_reference<T>::type>::deserialize(object, bin);
    }
}

#endif //TINYWORLD_TINYSERIALIZER_DEC_H
