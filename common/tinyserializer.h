#ifndef TINYWORLD_TINYSERIALIZER_H
#define TINYWORLD_TINYSERIALIZER_H

#include "tinyreflection.h"

#include <iomanip>
inline void hexdump(const std::string& hex, std::ostream& os = std::cout) {
    size_t line = 0;
    size_t left = hex.size();

    while (left)
    {
        size_t linesize = (left >= 16 ? 16 : left);
        std::string linestr = hex.substr(line*16, linesize);

        os << std::hex << std::setw(8) << std::setfill('0') << line*16 << " ";
        for (size_t i = 0; i < linestr.size(); ++i) {
            if (i % 8 == 0)
                os << " ";

            int v = (uint8_t)linestr[i];
            os << std::hex << std::setw(2) << std::setfill('0') << v << " ";
        }

        os << " " << "|";
        for (size_t i = 0; i < linestr.size(); ++i) {
            if (std::isprint(linestr[i]))
                os << linestr[i];
            else
                os << '.';
        }
        os << "|" << std::endl;

        left -= linesize;
        line ++;
    }
}

template<typename T>
struct TinySerializerImpl<T, true> {
    static size_t serialize(const T &object, std::string &bin) {
//        std::cout << "Scalar:" << object << std::endl;
        bin.append((char *) &object, sizeof(object));
        return sizeof(object);
    }

    static size_t deserialize(T &object, const std::string &bin) {
        if (bin.size() < sizeof(object))
            return false;
        memcpy(&object, bin.data(), sizeof(object));
//        std::cout << "Scalar:" << object << std::endl;
        return sizeof(object);
    }
};

template<typename T>
struct TinySerializerImpl<T, false> {
    static size_t serialize(const T &object, std::string &bin) {
//        std::cout << "Compound:" << typeid(object).name() << std::endl;

        size_t binsize = 0;
        auto reflection = StructFactory::instance().structByType<T>();
        if (!reflection)
            return false;

        uint16_t version = reflection->version();
        bin.append((char *) &version, sizeof(version));
        binsize += sizeof(version);

        uint16_t propnum = reflection->propertyIterator().size();
        bin.append((char *) &propnum, sizeof(propnum));
        binsize += sizeof(propnum);

        for (auto prop : reflection->propertyIterator()) {
            uint16_t key = prop->id();
            bin.append((char *) &key, sizeof(key));
            binsize += sizeof(key);

            std::string propbin = prop->serialize(object);

            uint32_t size = propbin.size();
            bin.append((char *) &size, sizeof(size));
            binsize += sizeof(size);

            bin.append(propbin);
            binsize += propbin.size();
        }
        return binsize;
    }

    static size_t deserialize(T &object, const std::string &bin) {

        auto reflection = StructFactory::instance().structByType<T>();
        if (!reflection)
            return 0;

        size_t binsize = 0;

        char *data = (char *) bin.data();

        uint16_t version = *(uint16_t *) data;
        data += sizeof(version);
        binsize += sizeof(version);

        uint16_t propnum = *(uint16_t *) data;
        data += sizeof(version);
        binsize += sizeof(version);

//        std::cout << version << "----" << propnum << std::endl;

        for (uint16_t i = 0; i < propnum; ++i) {
            uint16_t key = *(uint16_t *) data;
            data += sizeof(key);
            binsize += sizeof(key);

            uint32_t size = *(uint32_t *) data;
            data += sizeof(size);
            binsize += sizeof(size);

//            std::cout << key << ", size:" << size << std::endl;

            auto prop = reflection->propertyByID(key);
            if (prop) {
                std::string propbin;
                propbin.assign(data, size);
                if (prop->deserialize(object, propbin))
                    std::cout << prop->name() << std::endl;
            }

            data += size;
            binsize += size;
        }

        return binsize;
    }
};

template<>
struct TinySerializer<std::string> {
    static size_t serialize(const std::string &object, std::string &bin) {
        bin.append(object);
        return object.size();
    }

    static size_t deserialize(std::string &object, const std::string &bin) {
        object = bin;
        return object.size();
    }
};

#include <vector>

template <typename T, typename Alloc>
struct TinySerializer<std::vector<T, Alloc> > {
    static size_t serialize(const std::vector<T, Alloc> &object, std::string &bin) {
        size_t binsize = 0;

        uint32_t num = object.size();
        bin.append((char*)&num, sizeof(num));
        binsize += sizeof(num);

        for (uint32_t i = 0; i < num; ++i) {

            std::string membin;
            TinySerializer<T>::serialize(object[i], membin);

            uint32_t size = membin.size();
            bin.append((char*)&size, sizeof(size));
            binsize += sizeof(size);

            bin.append(membin);
            binsize += size;
        }

        return binsize;
    }

    static size_t deserialize(std::vector<T, Alloc> &object, const std::string &bin) {

        size_t binsize = 0;

        char *data = (char *) bin.data();

        uint32_t num = *(uint32_t *) data;
        data += sizeof(num);

        for (uint32_t i = 0; i < num; ++i) {
            uint32_t size = *(uint32_t*)data;
            data += sizeof(size);

            T obj;
            TinySerializer<T>::deserialize(obj, std::string(data, size));
            object.push_back(obj);

            data += size;
            binsize += size;
        }

        return  binsize;
    }
};


#endif //TINYWORLD_TINYSERIALIZER_H
