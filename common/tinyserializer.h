#ifndef TINYWORLD_TINYSERIALIZER_H
#define TINYWORLD_TINYSERIALIZER_H

#include "tinyreflection.h"

template<typename T>
struct TinySerializerImpl<T, true> {
    static bool serialize(const T &object, std::string &bin) {

//        std::cout << __PRETTY_FUNCTION__ << std::endl;
        bin = "pod";
        return true;
    }

    static bool deserialize(T &object, const std::string &bin) {
        return true;
    }
};

template<typename T>
struct TinySerializerImpl<T, false> {
    static bool serialize(const T &object, std::string &bin) {
//        std::cout << __PRETTY_FUNCTION__ << std::endl;
        auto reflection = StructFactory::instance().structByType<T>();
        if (reflection) {
            bin = "(" + reflection->name() + ":";
            for (auto prop : reflection->propertyIterator()) {
                bin += prop->serialize(object) + ",";
            }

            bin += ")";
            return true;
        } else {
            bin = "unkown";
            return false;
        }
    }

    static bool deserialize(T &object, const std::string &bin) {
        return true;
    }
};

template<>
struct TinySerializer<std::string> {
    static bool serialize(const std::string &object, std::string &bin) {
        bin = "string";
        return true;
    }

    static bool deserialize(std::string &object, const std::string &bin) {
        return true;
    }
};


#endif //TINYWORLD_TINYSERIALIZER_H
