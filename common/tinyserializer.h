// Copyright (c) 2017 david++
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef TINYWORLD_TINYSERIALIZER_H
#define TINYWORLD_TINYSERIALIZER_H

#include "tinyworld.h"

TINY_NAMESPACE_BEGIN

//template<typename T>
//struct Serializer {
//
//    std::string serialize(const T &object) const;
//
//    bool deserialize(T &object, const std::string &bin) const;
//};


template <typename T>
class ProtoSerializer;

template <typename T>
class ProtoDynSerializer;

//
// 序列化 vs 反序列化
//
template<template<typename T> class SerializerT = ProtoSerializer, typename T>
std::string serialize(const T &object, const SerializerT<T> &serializer = SerializerT<T>()) {
    return serializer.serialize(object);
};

template<template<typename T> class SerializerT = ProtoSerializer, typename T>
bool deserialize(T &object, const std::string &bin, const SerializerT<T> &serializer = SerializerT<T>()) {
    return serializer.deserialize(object, bin);
}

TINY_NAMESPACE_END

//#include "tinyreflection.h"
//
//
//template<typename T>
//struct TinySerializerImpl<T, true> {
//    static size_t serialize(const T &object, std::string &bin) {
////        std::cout << "Scalar:" << object << std::endl;
//        bin.append((char *) &object, sizeof(object));
//        return sizeof(object);
//    }
//
//    static size_t deserialize(T &object, const std::string &bin) {
//        if (bin.size() < sizeof(object))
//            return false;
//        memcpy(&object, bin.data(), sizeof(object));
////        std::cout << "Scalar:" << object << std::endl;
//        return sizeof(object);
//    }
//};
//
//template<typename T>
//struct TinySerializerImpl<T, false> {
//    static size_t serialize(const T &object, std::string &bin) {
////        std::cout << "Compound:" << typeid(object).name() << std::endl;
//
//        size_t binsize = 0;
//        auto reflection = StructFactory::instance().structByType<T>();
//        if (!reflection)
//            return false;
//
//        uint16_t version = reflection->version();
//        bin.append((char *) &version, sizeof(version));
//        binsize += sizeof(version);
//
//        uint16_t propnum = reflection->propertyIterator().size();
//        bin.append((char *) &propnum, sizeof(propnum));
//        binsize += sizeof(propnum);
//
//        for (auto prop : reflection->propertyIterator()) {
//            uint16_t key = prop->id();
//            bin.append((char *) &key, sizeof(key));
//            binsize += sizeof(key);
//
//            std::string propbin = prop->serialize(object);
//
//            uint32_t size = propbin.size();
//            bin.append((char *) &size, sizeof(size));
//            binsize += sizeof(size);
//
//            bin.append(propbin);
//            binsize += propbin.size();
//        }
//        return binsize;
//    }
//
//    static size_t deserialize(T &object, const std::string &bin) {
//
//        auto reflection = StructFactory::instance().structByType<T>();
//        if (!reflection)
//            return 0;
//
//        size_t binsize = 0;
//
//        char *data = (char *) bin.data();
//
//        uint16_t version = *(uint16_t *) data;
//        data += sizeof(version);
//        binsize += sizeof(version);
//
//        uint16_t propnum = *(uint16_t *) data;
//        data += sizeof(version);
//        binsize += sizeof(version);
//
////        std::cout << version << "----" << propnum << std::endl;
//
//        for (uint16_t i = 0; i < propnum; ++i) {
//            uint16_t key = *(uint16_t *) data;
//            data += sizeof(key);
//            binsize += sizeof(key);
//
//            uint32_t size = *(uint32_t *) data;
//            data += sizeof(size);
//            binsize += sizeof(size);
//
////            std::cout << key << ", size:" << size << std::endl;
//
//            auto prop = reflection->propertyByID(key);
//            if (prop) {
//                std::string propbin;
//                propbin.assign(data, size);
//                if (prop->deserialize(object, propbin))
//                    std::cout << prop->name() << std::endl;
//            }
//
//            data += size;
//            binsize += size;
//        }
//
//        return binsize;
//    }
//};
//
//template<>
//struct TinySerializer<std::string> {
//    static size_t serialize(const std::string &object, std::string &bin) {
//        bin.append(object);
//        return object.size();
//    }
//
//    static size_t deserialize(std::string &object, const std::string &bin) {
//        object = bin;
//        return object.size();
//    }
//};
//
//#include <vector>
//
//template<typename T, typename Alloc>
//struct TinySerializer<std::vector<T, Alloc> > {
//    static size_t serialize(const std::vector<T, Alloc> &object, std::string &bin) {
//        size_t binsize = 0;
//
//        uint32_t num = object.size();
//        bin.append((char *) &num, sizeof(num));
//        binsize += sizeof(num);
//
//        for (uint32_t i = 0; i < num; ++i) {
//
//            std::string membin;
//            TinySerializer<T>::serialize(object[i], membin);
//
//            uint32_t size = membin.size();
//            bin.append((char *) &size, sizeof(size));
//            binsize += sizeof(size);
//
//            bin.append(membin);
//            binsize += size;
//        }
//
//        return binsize;
//    }
//
//    static size_t deserialize(std::vector<T, Alloc> &object, const std::string &bin) {
//
//        size_t binsize = 0;
//
//        char *data = (char *) bin.data();
//
//        uint32_t num = *(uint32_t *) data;
//        data += sizeof(num);
//
//        for (uint32_t i = 0; i < num; ++i) {
//            uint32_t size = *(uint32_t *) data;
//            data += sizeof(size);
//
//            T obj;
//            TinySerializer<T>::deserialize(obj, std::string(data, size));
//            object.push_back(obj);
//
//            data += size;
//            binsize += size;
//        }
//
//        return binsize;
//    }
//};


#endif //TINYWORLD_TINYSERIALIZER_H
