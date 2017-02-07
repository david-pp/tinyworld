#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdio>

#include "tinyworld.h"
#include "mylogger.h"
#include "mydb.h"

#include "object2db.h"
#include "object2bin.h"

#include "player.h"
#include "player.db.h"
#include "player.pb.h"
#include "player.bin.h"

LoggerPtr logger(Logger::getLogger("test"));

void dumpPlayer(const Player& p)
{
    std::cout << "Player:"
              << p.id << "\t"
              << p.name << "\t"
              << p.bin.size() << "\t"
              << p.score << std::endl;
}

typedef Object2DB<Player, PlayerDBDescriptor> PlayerDB;
typedef Object2Bin<Player, PlayerBinDescriptor> PlayerBin;

////////////////////////////////////////////////////

void test_bin()
{
//    bin::Player player;
//    player.set_id(10);
//    player.set_score(45.0);
//    player.set_name("david");
//    player.set_v_bool(true);
//    std::cout << player.DebugString() << std::endl;

    Player p;
    p.id = 1;
    strncpy(p.name, "name", sizeof(p.name)-1);
    p.bin ="binary.....";

    std::string bin;

    PlayerBin pb(p);
    pb.serialize(bin);
    std::cout << pb.debugString() << std::endl;

    PlayerBin pb2;
    pb2.deserialize(bin);
    std::cout << pb2.debugString() << std::endl;
}

////////////////////////////////////////////////////

void test_db()
{
    Player p;
    p.id = 1;
    strncpy(p.name, "name", sizeof(p.name)-1);
    p.bin ="binary.....";

    mysqlpp::Query query(NULL);
    PlayerDBDescriptor pd;

    pd.makeInsertQuery(query, p);
    std::cout << "SQL:" << query.str() << std::endl;
    query.reset();

    pd.makeReplaceQuery(query, p);
    std::cout << "SQL:" << query.str() << std::endl;
    query.reset();

    pd.makeUpdateQuery(query, p);
    std::cout << "SQL:" << query.str() << std::endl;
    query.reset();

    pd.makeDeleteQuery(query, p);
    std::cout << "SQL:" << query.str() << std::endl;
    query.reset();

    pd.makeSelectFromQuery(query, "WHERE `ID`=100");
    std::cout << "SQL:" << query.str() << std::endl;
    query.reset();

    pd.makeDeleteFromQuery(query, "WHERE `ID`=100");
    std::cout << "SQL:" << query.str() << std::endl;
    query.reset();

    pd.makeCreateQuery(query);
    std::cout << "SQL:" << query.str() << std::endl;
    query.reset();

    pd.makeAddFieldQuery(query, "`ID`");
    std::cout << "SQL:" << query.str() << std::endl;
    query.reset();
}

struct DummyData
{
    int int_ = 0x123456;
    float float_value = 3.1415;
    char char_ = 'A';

    void dump()
    {
        LOG4CXX_DEBUG(logger, "DUMMY DATA:" << int_ << "," << float_value << "," << char_);
    }
};

void test_create()
{
    PlayerDB::createTable();
}

void test_drop()
{
    PlayerDB::dropTable();
}

void test_update()
{
    PlayerDB::updateTable();
}

void test_insertDB()
{
    for (unsigned int i = 1; i <= 10; ++i )
    {
        Player p;
        p.id = i;
        p.score = 1.1f * i;
        snprintf(p.name, sizeof(p.name), "player:%u", i);

        DummyData data;
        data.float_value *= i;
        p.bin.assign((char*)&data, sizeof(data));


        PlayerDB pdb2;
        PlayerDB pdb(p);
//        dumpPlayer(pdb);
        pdb.insertDB();
    }
}

void test_replaceDB()
{
    for (unsigned int i = 1; i <= 10; ++i )
    {
        PlayerDB p;
        p.id = i;
        p.score = 1.1f * i;
        snprintf(p.name, sizeof(p.name), "player-replace:%u", i);
        p.replaceDB();
    }
}

void test_updateDB()
{
    for (unsigned int i = 1; i <= 10; ++i )
    {
        PlayerDB p;
        p.id = i;
        p.score = 1.1f * i;
        snprintf(p.name, sizeof(p.name), "player-update:%u", i);
        p.updateDB();
    }
}

void test_deleteDB()
{
    for (unsigned int i = 1; i <= 10; ++i )
    {
        PlayerDB p;
        p.id = i;
        p.score = 1.1f * i;
        snprintf(p.name, sizeof(p.name), "player-update:%u", i);
        p.deleteDB();
    }
}

void test_selectDB()
{
    for (unsigned int i = 1; i <= 10; ++i )
    {
        PlayerDB p;
        p.id = i;
        p.selectDB();
        dumpPlayer(p);
    }
}

void test_loadFromDB()
{
    PlayerDB::Records players;
    PlayerDB::loadFromDB(players, "");

    std::for_each(players.begin(), players.end(), dumpPlayer);
}

void test_deleteFromDB()
{
    PlayerDB::deleteFromDB("");
}


int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage:" << argv[0]
                  << " create | drop | update" << std::endl;
        return 1;
    }

    BasicConfigurator::configure();

    LOG4CXX_DEBUG(logger, "dataserver 启动")

    MySqlConnectionPool::instance()->setServerAddress("mysql://david:123456@127.0.0.1/tinyworld?maxconn=5");
    MySqlConnectionPool::instance()->createAll();

    std::string op = argv[1];
    if ("create" == op)
        test_create();
    else if ("drop" == op)
        test_drop();
    else if ("updatetable" == op)
        test_update();
    else if ("test" == op)
        test_db();
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
    else if ("loadFromDB" == op)
        test_loadFromDB();
    else if ("deleteFromDB" == op)
        test_deleteFromDB();
    else if ("testbin" == op)
        test_bin();

    return 0;
}
