#ifndef TINYWORLD_TINYSERIALIZER_DEC_H
#define TINYWORLD_TINYSERIALIZER_DEC_H

#include <string>
#include <utility>

template<typename T, bool ispod>
struct TinySerializerImpl {
    static size_t serialize(const T &object, std::string &bin);

    static size_t deserialize(T &object, const std::string &bin);
};

template<typename T>
struct TinySerializer {
    static size_t serialize(const T &object, std::string &bin) {
        return TinySerializerImpl<T, std::is_scalar<T>::value>::serialize(object, bin);
    }

    static size_t deserialize(T &object, const std::string &bin) {
        return TinySerializerImpl<T, std::is_scalar<T>::value>::deserialize(object, bin);
    }
};

template <typename T>
struct ProtoSerializer {
    static size_t serialize(const T &object, std::string &bin);

    static size_t deserialize(T &object, const std::string &bin);
};

namespace tiny {
    template<typename T>
    inline std::string serialize(const T &object) {

//        std::cout << __PRETTY_FUNCTION__ << std::endl;
        std::string bin;
        TinySerializer<typename std::remove_reference<T>::type>::serialize(object, bin);
        return bin;
    }

    template<typename T>
    inline bool deserialize(T &object, const std::string &bin) {
//        std::cout << __PRETTY_FUNCTION__ << std::endl;
        return TinySerializer<typename std::remove_reference<T>::type>::deserialize(object, bin) > 0;
    }
}

#endif //TINYWORLD_TINYSERIALIZER_DEC_H
