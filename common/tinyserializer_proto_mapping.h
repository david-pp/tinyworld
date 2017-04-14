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

#ifndef TINYWORLD_TINYSERIALIZER_PROTO_MAPPING_H
#define TINYWORLD_TINYSERIALIZER_PROTO_MAPPING_H

#include "tinyworld.h"

#include <ostream>
#include <unordered_map>
#include <vector>
#include <iostream>

#include <google/protobuf/reflection.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>

#include "tinyreflection.h"

TINY_NAMESPACE_BEGIN

template<typename T>
class ProtoMapping;

class ProtoMappingFactory;


class ProtoMappingBase {
public:
    using DescriptorPool = google::protobuf::DescriptorPool;
    using MessageFactory = google::protobuf::MessageFactory;
    using FileDescriptorProto = google::protobuf::FileDescriptorProto;

    //
    // 构造Message的描述符
    //
    virtual bool createProto() = 0;

    //
    // Message的proto定义
    //
    std::string protoDefine() {
        if (!descriptor_pool_) return "";
        auto *descriptor = descriptor_pool_->FindMessageTypeByName(proto_name_);
        if (descriptor)
            return descriptor->DebugString();
        return "";
    }

    //
    // 关联的描述符工厂和消息工厂
    //
    DescriptorPool *descriptorPool() { return descriptor_pool_; }

    MessageFactory *messageFactory() { return message_factory_; }

    void setDescriptorPool(DescriptorPool *pool) { descriptor_pool_ = pool; }

    void setMessageFactory(MessageFactory *factory) { message_factory_ = factory; }


    const std::string &cppName() { return cpp_name_; }

    const std::string &protoName() { return proto_name_; }

protected:
    DescriptorPool *descriptor_pool_ = nullptr;
    MessageFactory *message_factory_ = nullptr;

    std::string cpp_name_;
    std::string proto_name_;
};

template<typename T>
class ProtoMapping : public ProtoMappingBase {
public:
    ProtoMapping(Struct <T> *reflection,
                 const std::string &cpp_name,
                 const std::string &proto_name = "") {
        cpp_name_ = cpp_name;

        if (proto_name.empty())
            proto_name_ = cpp_name_ + "DynProto";
        else
            proto_name_ = proto_name;

        if (reflection) {
            struct_ = reflection;
            from_struct_factory_ = true;
        } else {
            struct_ = new Struct<T>(cpp_name_);
            from_struct_factory_ = false;
        }
    }

    virtual ~ProtoMapping() {
        if (!from_struct_factory_ && struct_)
            delete struct_;
    }

    template<template<typename> class SerializerT/*=DynSerializer*/, typename PropType>
    ProtoMapping<T> &property(const std::string &name, PropType T::* prop, uint16_t number) {
        struct_->template property<SerializerT, PropType>(name, prop, number);
        return *this;
    }

    bool createProto() final {
        using namespace google::protobuf;

        if (!descriptor_pool_) return false;

        FileDescriptorProto file_proto;
        file_proto.set_name(cpp_name_ + ".proto");

        DescriptorProto *descriptor = file_proto.add_message_type();
        descriptor->set_name(proto_name_);

        for (auto prop : struct_->propertyIterator()) {
            if (typeid(uint32_t) == prop->type()) {
                FieldDescriptorProto *fd = descriptor->add_field();
                fd->set_name(prop->name());
                fd->set_type(FieldDescriptorProto::TYPE_UINT32);
                fd->set_number(prop->number());
                fd->set_label(FieldDescriptorProto::LABEL_OPTIONAL);
            } else {
                FieldDescriptorProto *fd = descriptor->add_field();
                fd->set_name(prop->name());
                fd->set_type(FieldDescriptorProto::TYPE_BYTES);
                fd->set_number(prop->number());
                fd->set_label(FieldDescriptorProto::LABEL_OPTIONAL);
            }
        }

        descriptor_pool_->BuildFile(file_proto);
        return true;
    }

public:
    Struct <T> *struct_;
    bool from_struct_factory_;
};

class ProtoMappingFactory {
public:
    //
    // Default Instance (As Singleton)
    //
    static ProtoMappingFactory &instance() {
        static ProtoMappingFactory instance_;
        return instance_;
    }

    ProtoMappingFactory() {
    }


    //
    // 自定义映射
    //
    template<typename T>
    ProtoMapping<T> &declare(const std::string &cpp_name, const std::string &proto_name = "") {
        return createProtoMapping<T>(nullptr, cpp_name, proto_name);
    }

    //
    // 使用StructFactory定义映射
    //
    template<typename T>
    ProtoMappingFactory &mapping(const std::string &proto_name = "",
                                 StructFactory &struct_factory = StructFactory::instance()) {
        auto reflection = struct_factory.structByType<T>();
        if (reflection) {
            createProtoMapping<T>(reflection, reflection->name(), proto_name);
        } else {
            std::cerr << "[ProtoMapping] mapping failed: reflection is not exist : " << __PRETTY_FUNCTION__
                      << std::endl;
        }
        return *this;
    }

    template<typename T>
    ProtoMapping<T> *mappingByType() {
        auto it = mappings_by_typeid_.find(typeid(T).name());
        if (it != mappings_by_typeid_.end())
            return static_cast<ProtoMapping<T> *>(it->second.get());
        return NULL;
    }

    ProtoMappingBase *mappingByName(const std::string &name) {
        auto it = mappings_by_name_.find(name);
        if (it != mappings_by_name_.end())
            return it->second.get();
        return NULL;
    }

    template<typename T>
    bool createProtoByType() {
        ProtoMapping<T> *mapping = mappingByType<T>();
        if (mapping) {
            return mapping->createProto();
        }
    }

    void createAllProto() {
        for (auto v : this->mappings_by_order_) {
            v->createProto();
        }
    }


    template<typename T>
    std::string protoDefineByType() {
        ProtoMapping<T> *mapping = mappingByType<T>();
        if (mapping) {
            return mapping->protoDefine();
        }
        return "";
    }

    void generateAllProtoDefine(std::ostream &os = std::cout) {
        for (auto v : this->mappings_by_order_) {
            os << "// " << v->cppName() << " -> " << v->protoName() << std::endl;
            os << v->protoDefine();
            os << std::endl;
        }
    }

protected:
    template<typename T>
    ProtoMapping<T> &createProtoMapping(Struct <T> *reflection,
                                        const std::string &cpp_name,
                                        const std::string &proto_name) {

        auto mapping = std::make_shared<ProtoMapping<T>>(reflection, cpp_name, proto_name);

        mapping->setDescriptorPool(&descriptor_pool_);
        mapping->setMessageFactory(&message_factory_);

        if (mappingByType<T>())
            std::cerr << "[ProtoMapping] mappingByType exist: " << __PRETTY_FUNCTION__ << std::endl;

        if (mappingByName(cpp_name))
            std::cerr << "[ProtoMapping] mappingByName exist: " << cpp_name << " : " << __PRETTY_FUNCTION__
                      << std::endl;

        mappings_by_typeid_[typeid(T).name()] = mapping;
        mappings_by_typeid_[cpp_name] = mapping;
        mappings_by_order_.push_back(mapping);

        return *mapping;
    }


protected:
    typedef std::unordered_map<std::string, std::shared_ptr<ProtoMappingBase>> ProtoMappings;

    ProtoMappings mappings_by_typeid_;
    ProtoMappings mappings_by_name_;
    std::vector<std::shared_ptr<ProtoMappingBase>> mappings_by_order_;

    google::protobuf::DescriptorPool descriptor_pool_;
    google::protobuf::DynamicMessageFactory message_factory_;
};

TINY_NAMESPACE_END

#endif //TINYWORLD_TINYSERIALIZER_PROTO_MAPPING_H
