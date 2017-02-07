#ifndef TINYWORLD_OBJECT2BIN_H
#define TINYWORLD_OBJECT2BIN_H

#include <string>

template <typename T, typename T_BinDescriptor>
struct Object2Bin : public T
{
public:
    typedef typename T_BinDescriptor::ProtoType ProtoType;

    Object2Bin() {}
    Object2Bin(const T& object) : T(object) {}

    bool serialize(std::string& bin) const
    {
        ProtoType proto;
        T_BinDescriptor::obj2proto(proto, *this);
        return proto.SerializeToString(&bin);
    }
    bool deserialize(const std::string& bin)
    {
        ProtoType proto;
        if (proto.ParseFromString(bin)) {
            T_BinDescriptor::proto2obj(*this, proto);
            return true;
        }
        return false;
    }

    std::string debugString()
    {
        ProtoType proto;
        T_BinDescriptor::obj2proto(proto, *this);
        return proto.DebugString();
    }
};

#endif //TINYWORLD_OBJECT2BIN_H
