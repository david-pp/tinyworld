#ifndef TINYWORLD_TEST_ORM_H
#define TINYWORLD_TEST_ORM_H

#include "tinyorm.h"

struct PoDMem {
    uint32_t value;
};

struct NonPoDMem {
    uint32_t id;
    std::string name;
};

//template <>
//struct TinySerializer<NonPoDMem> {
//    static bool serialize(const NonPoDMem &object, std::string &bin) {
//        bin = "NonPoDMem";
//        return true;
//    }
//
//    static bool deserialize(NonPoDMem &object, const std::string &bin) {
//        return true;
//    }
//};

//
// Player -> PLAYER
//
struct Player {
    uint32_t id = 1024;
    std::string name = "david";
    uint8_t age = 30;

    // 整型
    int8_t   m_int8 = 1;
    uint8_t  m_uint8 = 2;
    int16_t  m_int16 = 3;
    uint16_t m_uint16 = 4;
    int32_t  m_int32 = 5;
    uint32_t m_uint32 = 6;
    int64_t  m_int64 = 7;
    uint64_t m_uint64 = 8;

    // 浮点
    float  m_float = 10.11;
    double m_double = 20.22;

    // 布尔
    bool   m_bool = false;

    // 字符串
    std::string m_string = "string";

    // 二进制
    std::string m_bytes = "bytes";
    std::string m_bytes_tiny = "tiny...bytes";
    std::string m_bytes_medium = "medium...bytes";
    std::string m_bytes_long = "long...bytes";

    // 复合对象
    PoDMem obj1;
    NonPoDMem obj2;
};

// TODO: 在cpp中定义一次即可
struct PlayerRegister {
    PlayerRegister() {
        StructFactory::instance().declare<NonPoDMem>()
                .property("id", &NonPoDMem::id)
                .property("name", &NonPoDMem::name);

        TableFactory::instance().table<Player>("PLAYER")
                .field(&Player::id, "ID", FieldType::UINT32)
                .field(&Player::name, "NAME", FieldType::VCHAR, "david", 32)
                .field(&Player::age, "AGE", FieldType::UINT8, "30")

                .field(&Player::m_int8,  "M_INT8", FieldType::INT8)
                .field(&Player::m_int16, "M_INT16", FieldType::INT16)
                .field(&Player::m_int32, "M_INT32", FieldType::INT32)
                .field(&Player::m_int64, "M_INT64", FieldType::INT64)
                .field(&Player::m_uint8,  "M_UINT8", FieldType::UINT8)
                .field(&Player::m_uint16, "M_UINT16", FieldType::UINT16)
                .field(&Player::m_uint32, "M_UINT32", FieldType::UINT32)
                .field(&Player::m_uint64, "M_UINT64", FieldType::UINT64)

                .field(&Player::m_bool,   "M_BOOL", FieldType::BOOL)
                .field(&Player::m_float,  "M_FLOAT", FieldType::FLOAT)
                .field(&Player::m_double, "M_DOUBLE", FieldType::DOUBLE)

                .field(&Player::m_bytes, "M_BYTES", FieldType::BYTES)
                .field(&Player::m_bytes_tiny, "M_BYTES_S", FieldType::BYTES_TINY)
                .field(&Player::m_bytes_medium, "M_BYTES_M", FieldType::BYTES_MEDIUM)
                .field(&Player::m_bytes_long, "M_BYTES_L", FieldType::BYTES_LONG)

                .field(&Player::obj1, "OBJ1", FieldType::OBJECT)
                .field(&Player::obj2, "OBJ2", FieldType::OBJECT)
                .key("ID")
                .index("NAME");
    }
};

static PlayerRegister player_register___;


std::ostream& operator << (std::ostream& os, const Player& p) {
    os << "-------- Player:(" << p.id << "," << p.name << ")------" << std::endl;
    os << "- m_int8    = " << (int)p.m_int8 << std::endl;
    os << "- m_int16   = " << p.m_int16 << std::endl;
    os << "- m_int32   = " << p.m_int32 << std::endl;
    os << "- m_int64   = " << p.m_int64 << std::endl;
    os << "- m_uint8   = " << (int)p.m_uint8 << std::endl;
    os << "- m_uint16  = " << p.m_uint16 << std::endl;
    os << "- m_uint32  = " << p.m_uint32 << std::endl;
    os << "- m_uint64  = " << p.m_uint64 << std::endl;
    os << "- m_float   = " << p.m_float << std::endl;
    os << "- m_double  = " << p.m_double << std::endl;
    os << "- m_bool    = " << p.m_bool << std::endl;
    os << "- m_string  = " << p.m_string << std::endl;

    os << "- m_bytes        = " << p.m_bytes.size() << std::endl;
    os << "- m_bytes_tiny   = " << p.m_bytes_tiny.size() << std::endl;
    os << "- m_bytes_medium = " << p.m_bytes_medium.size() << std::endl;
    os << "- m_bytes_long   = " << p.m_bytes_long.size() << std::endl;

    return os;
}


#endif //TINYWORLD_TEST_ORM_H
