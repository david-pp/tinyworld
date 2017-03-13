//ODB: C++ Object-Relational Mapping (ORM)

#include <iostream>
#include <unordered_map>
#include <vector>
#include <memory>
#include <tinyreflection.h>

#include "tinylogger.h"

#include "tinyorm.h"
#include "tinyorm_mysql.h"

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

struct Player {
    uint32_t id = 1024;
    std::string name = "david";
    uint8_t age = 30;

    PoDMem obj1;
    NonPoDMem obj2;

    void dump() const {
        std::cout << "Player:" << id << "," << name << "," << (int) age << std::endl;
    }
};

////////////////////////////////////////////////////////////

// TODO: 生成DB操作
struct PlayerRegister {
    PlayerRegister() {
        StructFactory::instance().declare<NonPoDMem>()
                .property("id", &NonPoDMem::id)
                .property("name", &NonPoDMem::name);

        TableFactory::instance().table<Player>("PLAYER")
                .field(&Player::id, "ID", FieldType::UINT32)
                .field(&Player::name, "NAME", FieldType::VCHAR, "david", 32)
                .field(&Player::age, "AGE", FieldType::UINT8, "30")
                .field(&Player::obj1, "OBJ1", FieldType::OBJECT)
                .field(&Player::obj2, "OBJ2", FieldType::OBJECT)
                .key("ID")
                .index("NAME");
    }
};

static PlayerRegister register___;

////////////////////////////////////////////////////////////

void test_create() {
    TinyMySqlORM db;
    db.createTable("PLAYER");
}

void test_drop() {
    TinyMySqlORM db;
    db.dropTable("PLAYER");
}

void test_update() {
    TinyMySqlORM db;
    db.updateTables();
}

void test_sql() {
    TinyMySqlORM db;

    {
        mysqlpp::Query query(NULL);
        db.makeSelectQuery(query, Player());
        LOG_INFO(__FUNCTION__, "%s", query.str().c_str());
    }

    {
        mysqlpp::Query query(NULL);
        db.makeInsertQuery(query, Player());
        LOG_INFO(__FUNCTION__, "%s", query.str().c_str());
    }

    {
        mysqlpp::Query query(NULL);
        db.makeReplaceQuery(query, Player());
        LOG_INFO(__FUNCTION__, "%s", query.str().c_str());
    }

    {
        mysqlpp::Query query(NULL);
        db.makeUpdateQuery(query, Player());
        LOG_INFO(__FUNCTION__, "%s", query.str().c_str());
    }

    {
        mysqlpp::Query query(NULL);
        db.makeDeleteQuery(query, Player());
        LOG_INFO(__FUNCTION__, "%s", query.str().c_str());
    }
}

void test_insertDB() {

    TinyMySqlORM db;
    for (uint32_t i = 0; i < 10; ++i) {
        Player p;
        p.id = i;
        p.age = 30;
        p.name = "david-insert-" + std::to_string(i);
        db.insert(p);
    }
};

void test_replaceDB() {
    TinyMySqlORM db;
    for (uint32_t i = 0; i < 10; ++i) {
        Player p;
        p.id = i;
        p.age = 30;
        p.name = "david-replace-" + std::to_string(i);
        db.replace(p);
    }
}

void test_updateDB() {
    TinyMySqlORM db;
    for (uint32_t i = 0; i < 10; ++i) {
        Player p;
        p.id = i;
        p.age = 30;
        p.name = "david-update-" + std::to_string(i);
        db.update(p);
    }
}

void test_deleteDB() {
    TinyMySqlORM db;
    for (uint32_t i = 0; i < 10; ++i) {
        Player p;
        p.id = i;
        db.del(p);
    }
}

void test_selectDB() {
    TinyMySqlORM db;
    for (uint32_t i = 0; i < 10; ++i) {
        Player p;
        p.id = i;
        p.age = 0;
        p.name = "";
        if (db.select(p))
            p.dump();
    }
}

void test_load() {
    TinyMySqlORM db;
    TinyMySqlORM::Records<Player> players;
    db.loadFromDB(players, "WHERE ID %% %d=0 ORDER BY ID DESC", 2);

    for (auto p : players) {
        p->dump();
    }
}

void test_load2() {
    TinyMySqlORM db;
    db.loadFromDB<Player>([](std::shared_ptr<Player> p){
        p->dump();
    }, nullptr);
}

void test_load3() {
    TinyMySqlORM db;

    struct PlayerCompare {
        bool operator () (const std::shared_ptr<Player>& lhs, const std::shared_ptr<Player>& rhs) const {
            return lhs->id > rhs->id;
        }
    };

//    using PlayerSet = std::set<std::shared_ptr<Player>, PlayerCompare>;
    using PlayerSet = std::set<std::shared_ptr<Player>>;

    PlayerSet players;
    db.loadFromDB<Player, PlayerSet>(players, nullptr);

    for (auto it = players.begin(); it != players.end(); ++it) {
        (*it)->dump();
    }
}


#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>

namespace tiny {
    using boost::multi_index_container;
    using namespace boost::multi_index;

    struct by_id {};
    struct by_name {};

    using PlayerSet = boost::multi_index_container<
            Player,
            indexed_by<
                    // sort by employee::operator<
                    ordered_unique<tag<by_id>, member<Player, uint32_t , &Player::id> >,
                    // sort by less<string> on name
                    ordered_non_unique<tag<by_name>, member<Player, std::string, &Player::name> >
                    >
            >;
}

void test_load4() {
    TinyMySqlORM db;

    tiny::PlayerSet players;
    db.loadFromDB2MultiIndexSet<Player, tiny::PlayerSet>(players, nullptr);

    auto& players_by_id   = players.get<tiny::by_id>();
    auto& players_by_name = players.get<tiny::by_name>();

    auto it = players_by_id.find(5);
    if (it != players_by_id.end()) {
        it->dump();
    }

    for (auto it = players_by_name.begin(); it != players_by_name.end(); ++ it) {
        it->dump();
    }

    std::cout << players_by_name.count("david") << std::endl;
}


void test_delete() {
    TinyMySqlORM db;
    db.deleteFromDB<Player>("WHERE ID %% %d=0", 2);
}

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        std::cout << "Usage:" << argv[0]
                  << " create | drop | update" << std::endl;
        return 1;
    }

    MySqlConnectionPool::instance()->setServerAddress("mysql://david:123456@127.0.0.1/tinyworld?maxconn=5");
    MySqlConnectionPool::instance()->createAll();
    MySqlConnectionPool::instance()->setGrabWaitTime(0);

    std::string op = argv[1];
    if ("sql" == op)
        test_sql();
    else if ("create" == op)
        test_create();
    else if ("drop" == op)
        test_drop();
    else if ("update" == op)
        test_update();
    else if ("insertDB" == op)
        test_insertDB();
    else if ("replaceDB" == op)
        test_replaceDB();
    else if ("updateDB" == op)
        test_updateDB();
    else if ("deleteDB" == op)
        test_deleteDB();
    else if ("selectDB" == op)
        test_selectDB();
    else if ("load" == op)
        test_load();
    else if ("load2" == op)
        test_load2();
    else if ("load3" == op)
        test_load3();
    else if ("load4" == op)
        test_load4();
    else if ("delete" == op)
        test_delete();
//    else if ("testbin" == op)
//        test_bin();
//    else if ("testsuper" == op)
//        test_super();

    return 0;
}

