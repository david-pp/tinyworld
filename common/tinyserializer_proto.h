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

#ifndef TINYWORLD_TINYSERIALIZER_PROTO_H
#define TINYWORLD_TINYSERIALIZER_PROTO_H

#include <string>
#include "archive.pb.h"

template<typename T>
struct ProtoCase;

template<typename T, uint32_t proto_case>
struct ProtoSerializerImpl;

template<typename T>
struct ProtoSerializer : public ProtoSerializerImpl<T, ProtoCase<T>::value> {
};

//
// 顺序必须要固定的归档器
//
class ProtoArchiver : public ArchiveProto {
public:
    template<typename T>
    ProtoArchiver &operator<<(const T &object) {
        ArchiveMemberProto *mem = this->add_members();
        if (mem) {
            mem->set_data(serialize(object));
        }
        return *this;
    }

    template<typename T>
    ProtoArchiver &operator>>(T &object) {
        if (read_pos_ < this->members_size()) {
            const ArchiveMemberProto &mem = this->members(read_pos_);
            deserialize(object, mem.data());
            read_pos_++;
        }
        return *this;
    }

private:
    int read_pos_ = 0;
};


// Proto Case

enum ProtoTypeEnum {
    kProtoType_Integer,
    kProtoType_Float,
    kProtoType_String,
    kProtoType_Proto,
    kProtoType_List,
    kProtoType_Map,
    kProtoType_UserDefined,
};

template<typename T>
struct ProtoCase : public std::integral_constant<ProtoTypeEnum,
        std::is_base_of<google::protobuf::Message, T>::value ? kProtoType_Proto : kProtoType_UserDefined> {
};

template<>
struct ProtoCase<uint32_t> : public std::integral_constant<ProtoTypeEnum,
        kProtoType_Integer> {
};

template<>
struct ProtoCase<std::string> : public std::integral_constant<ProtoTypeEnum,
        kProtoType_String> {
};

template<typename T>
struct ProtoCase<std::vector<T>> : public std::integral_constant<ProtoTypeEnum, kProtoType_List> {
};

template<typename KeyT, typename ValueT>
struct ProtoCase<std::map<KeyT, ValueT>> : public std::integral_constant<ProtoTypeEnum, kProtoType_Map> {
};

// Impl

//
// 整数
//
template<typename T>
struct ProtoSerializerImpl<T, kProtoType_Integer> {

    std::string serialize(const T &value) const {
        IntegerProto proto;
        proto.set_value(value);
        return proto.SerializeAsString();
    }

    bool deserialize(T &value, const std::string &data) const {
        IntegerProto proto;
        if (proto.ParseFromString(data)) {
            value = proto.value();
            return true;
        }
        return false;
    }
};


//
// 浮点
//
template<typename T>
struct ProtoSerializerImpl<T, kProtoType_Float> {

    std::string serialize(const T &value) const {
        FloatProto proto;
        proto.set_value(value);
        return proto.SerializeAsString();
    }

    bool deserialize(T &value, const std::string &data) const {
        FloatProto proto;
        if (proto.ParseFromString(data)) {
            value = proto.value();
            return true;
        }
        return false;
    }
};

//
// 字符串
//
template<typename T>
struct ProtoSerializerImpl<T, kProtoType_String> {

    std::string serialize(const T &value) const {
        StringProto proto;
        proto.set_value(value);
        return proto.SerializeAsString();
    }

    bool deserialize(T &value, const std::string &data) const {
        StringProto proto;
        if (proto.ParseFromString(data)) {
            value = proto.value();
            return true;
        }
        return false;
    }
};


//
// Protobuf生成的类型
//
template<typename T>
struct ProtoSerializerImpl<T, kProtoType_Proto> {
public:
    std::string serialize(const T &proto) const {
        return proto.SerializeAsString();
    }

    bool deserialize(T &proto, const std::string &data) const {
        return proto.ParseFromString(data);
    }
};

//
// vector<T>
//

template<typename T>
struct ProtoSerializerImpl<std::vector<T>, kProtoType_List> {

    std::string serialize(const std::vector<T> &objects) const {
        SequenceProto proto;
        ProtoSerializer<T> member_serializer;
        for (auto &v : objects) {
            ArchiveMemberProto *mem = proto.add_values();
            if (mem) {
                mem->set_data(member_serializer.serialize(v));
            }
        }

        return proto.SerializeAsString();
    }

    bool deserialize(std::vector<T> &objects, const std::string &data) const {
        SequenceProto proto;
        ProtoSerializer<T> member_serializer;
        if (proto.ParseFromString(data)) {
            for (int i = 0; i < proto.values_size(); ++i) {
                T obj;
                if (member_serializer.deserialize(obj, proto.values(i).data()))
                    objects.push_back(obj);
            }
            return true;
        }
        return false;
    }
};

//
// map<KeyT, ValueT>
//
template<typename KeyT, typename ValueT>
struct ProtoSerializerImpl<std::map<KeyT, ValueT>, kProtoType_Map> {
    std::string serialize(const std::map<KeyT, ValueT> &objects) const {
        AssociateProto proto;
        ProtoSerializer<KeyT> key_serializer;
        ProtoSerializer<ValueT> value_serializer;
        for (auto &v : objects) {
            AssociateProto::ValueType *mem = proto.add_values();
            if (mem) {
                ArchiveMemberProto *key = mem->mutable_key();
                ArchiveMemberProto *value = mem->mutable_value();
                if (key && value) {
                    key->set_data(key_serializer.serialize(v.first));
                    value->set_data(value_serializer.serialize(v.second));
                }
            }
        }

        return proto.SerializeAsString();
    }

    bool deserialize(std::map<KeyT, ValueT> &objects, const std::string &bin) const {
        AssociateProto proto;
        ProtoSerializer<KeyT> key_serializer;
        ProtoSerializer<ValueT> value_serializer;
        if (proto.ParseFromString(bin)) {
            for (int i = 0; i < proto.values_size(); ++i) {
                KeyT key;
                ValueT value;
                if (key_serializer.deserialize(key, proto.values(i).key().data())
                    && value_serializer.deserialize(value, proto.values(i).value().data())) {
                    objects.insert(std::make_pair(key, value));
                }
            }
            return true;
        }
        return false;
    }
};


//
// 用户自定义类型
//
template<typename T>
struct ProtoSerializerImpl<T, kProtoType_UserDefined> {
public:
    std::string serialize(const T &object) const {
        return object.serialize();
    }

    bool deserialize(T &object, const std::string &data) const {
        return object.deserialize(data);
    }
};


#endif //TINYWORLD_TINYSERIALIZER_PROTO_H
