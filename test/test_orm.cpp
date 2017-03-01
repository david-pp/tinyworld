//ODB: C++ Object-Relational Mapping (ORM)

#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <initializer_list>
#include <memory>
#include <tinyreflection.h>

#include "tinyorm.h"

////////////////////////////////////////////////////////////////
//
// 数据库表描述基类
//
////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////
//
// C++结构体存储到MYSQL
//
////////////////////////////////////////////////////////////////
template<typename T, typename T_DBDescriptor>
struct Object2DB : public T {
public:
    using T::T;

    typedef std::vector<T> Records;

    Object2DB() {}

    Object2DB(const T &object) : T(object) {}

    //
    // 创建、删除、自动更新表结构
    //
    static bool createTable();

    static bool dropTable();

    static bool updateTable();

    //
    // 数据库批量操作
    //
    static bool loadFromDB(Records &records, const char *clause, ...);

    static bool deleteFromDB(const char *where, ...);

    template<typename T_Set>
    static bool loadFromDB(T_Set &records, const char *clause, ...);

protected:
    // 数据库描述
    static T_DBDescriptor descriptor_;

public:
    //
    // 该对象的数据库操作
    //
    bool selectDB();

    bool insertDB();

    bool replaceDB();

    bool updateDB();

    bool deleteDB();
};


struct Player {
    uint32_t id = 1024;
    std::string name = "david";
    uint8_t age = 30;
};

typedef std::unordered_map<uint32_t, Player> PlayerSet;

template <typename T>
void insert(Struct<T>& reflection, TableDescriptor::Ptr td) {

}

// TODO: 生成DB操作
struct PlayerDDDDDDDDD
{
    PlayerDDDDDDDDD()
    {
        TableFactory::instance().table("PLAYER")
                .field("ID",   FieldType::UINT32)
                .field("NAME", FieldType::VCHAR, "david", 32)
                .field("AGE",  FieldType::UINT8, "30")
                .key("ID")
                .index("NAME");
    }
};
static PlayerDDDDDDDDD dddd;



template<typename T, typename T_DBDescriptor>
struct ORM {

};




// TODO: 自动生成
#include "soci.h"
#include "soci-mysql.h"

namespace soci {

    template<>
    struct type_conversion<Player> {
        typedef values base_type;

        static void from_base(const values &v, indicator ind, Player &p) {
            p.id = v.get<uint32_t>("id");
            p.name = v.get<std::string>("name");
            p.age = v.get<uint8_t>("age");
        }

        static void to_base(const Player &p, values &v, indicator &ind) {
            v.set("id", p.id);
            v.set("name", p.name);
            v.set("age", p.age);
            ind = i_ok;
        }
    };
}

// TODO: 自动生成反射代码
#include "tinyreflection.h"

struct PlayerReflectionRegister {
    PlayerReflectionRegister() {
        StructFactory::instance().declare<Player>()
                .property("id", &Player::id)
                .property("name", &Player::name)
                .property("age", &Player::age);

    }
};

PlayerReflectionRegister _reg_;

int main() {
    // TODO: 初始化DB连接池
//    db << xx << xxset;
//    db >> xx >> xxset;

    auto td = TableFactory::instance().getTableDescriptor("PLAYER");
    std::cout << td->sql_create() << std::endl;
    std::cout << td->sql_drop() << std::endl;
    std::cout << td->sql_addfield("NAME") << std::endl;
}