#ifndef TINYWORLD_TINYREFLECTION_H
#define TINYWORLD_TINYREFLECTION_H

#include <iostream>
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>
#include <typeinfo>
#include <type_traits>
#include <boost/any.hpp>
#include "tinyserializer_dec.h"


template<typename T>
class Property {
public:
    typedef std::shared_ptr<Property> Ptr;

    Property(const std::string &name, uint16_t id)
            : name_(name), id_(id) {}

    const std::string &name() { return name_; }

    uint16_t id() { return id_; }

public:
    virtual const std::type_info &type() = 0;

    virtual boost::any get(T &) = 0;

    virtual void set(T &, boost::any &) = 0;

    virtual std::string serialize(const T &) = 0;

    virtual bool deserialize(T &object, const std::string &bin) = 0;

private:
    std::string name_;
    uint16_t id_;
};


template<typename T, typename MemFn>
class Property_T : public Property<T> {
public:
    Property_T(const std::string &name, uint16_t id, MemFn fn)
            : Property<T>(name, id), fn_(fn) {}

    using PropRefType = typename std::result_of<MemFn(T &)>::type;
    using PropType = typename std::remove_reference<PropRefType>::type;

    virtual const std::type_info &type() final {
        return typeid(PropType);
    };

    boost::any get(T &obj) final {
        return fn_(obj);
    }

    void set(T &obj, boost::any &v) final {
        fn_(obj) = boost::any_cast<PropType>(v);
    }

    std::string serialize(const T &obj) final {
        return tiny::serialize(fn_(obj));
    }

    bool deserialize(T &obj, const std::string &bin) final {
        return tiny::deserialize(fn_(obj), bin);
    }

private:
    MemFn fn_;
};

template<typename T, typename MemFn>
Property_T<T, MemFn> *makePropery(const std::string &name, uint16_t id, MemFn fn) {
    return new Property_T<T, MemFn>(name, id, fn);
}


struct StructBase {
};

template<typename T>
struct Struct : public StructBase {
public:
    typedef std::vector<typename Property<T>::Ptr> PropertyContainer;
    typedef std::unordered_map<std::string, typename Property<T>::Ptr> PropertyMap;

    Struct(const std::string &name, uint16_t version = 0)
            : name_(name), version_(version) {}

    virtual ~Struct() {}

    T *clone() { return new T; }

    template<typename PropType>
    Struct<T> &property(const std::string &name, PropType T::* prop, uint16_t id = 0) {
        if (!hasPropery(name)) {
            typename Property<T>::Ptr ptr(makePropery<T>(name, id, std::mem_fn(prop)));
            properties_[name] = ptr;
            properties_ordered_.push_back(ptr);
        }

        return *this;
    }

    Struct<T> &version(uint16_t ver) {
        version_ = ver;
        return *this;
    }

    bool hasPropery(const std::string &name) {
        return properties_.find(name) != properties_.end();
    }

    size_t propertyCount() { return properties_.size(); }

    PropertyContainer propertyIterator() { return properties_ordered_; }

    typename Property<T>::Ptr getPropertyByName(const std::string &name) {
        auto it = properties_.find(name);
        if (it != properties_.end())
            return it->second;
        return typename Property<T>::Ptr();
    }

    template<typename PropType>
    PropType get(T &obj, const std::string &propname) {
        auto prop = getPropertyByName(propname);
        if (prop)
            return boost::any_cast<PropType>(prop->get(obj));
        return PropType();
    }

    template<typename PropType>
    void set(T &obj, const std::string &propname, const PropType &value) {
        auto prop = getPropertyByName(propname);
        if (prop) {
            boost::any v = value;
            prop->set(obj, v);
        }
    }

    const std::string &name() { return name_; }

    uint16_t version() { return version_; }

private:
    std::string name_;
    PropertyContainer properties_ordered_;
    PropertyMap properties_;

    uint16_t version_ = 0;
};

struct StructFactory {
    static StructFactory &instance() {
        static StructFactory instance_;
        return instance_;
    }

    template<typename T>
    Struct<T> &declare(const std::string name = "") {
        std::string type_name = typeid(T).name();
        std::string struct_name = name;
        if (name.empty())
            struct_name = type_name;

        auto desc = std::make_shared<Struct<T>>(struct_name);
        structs_by_typeid_[type_name] = desc;
        structs_by_name_[struct_name] = desc;
        return *desc;
    }

    template<typename T>
    Struct<T> *structByType() {
        std::string type_name = typeid(T).name();
        auto it = structs_by_typeid_.find(type_name);
        if (it != structs_by_typeid_.end())
            return static_cast<Struct<T> *>(it->second.get());
        return NULL;
    }

    template<typename T>
    Struct<T> *structByName(const std::string &name) {
        auto it = structs_by_name_.find(name);
        if (it != structs_by_name_.end())
            return static_cast<Struct<T> *>(it->second.get());
        return NULL;
    }

private:
    typedef std::unordered_map<std::string, std::shared_ptr<StructBase>> Structs;

    Structs structs_by_typeid_;
    Structs structs_by_name_;
};

#endif //TINYWORLD_TINYREFLECTION_H
