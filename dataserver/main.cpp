
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdio>

#include "mylogger.h"
#include "mydb.h"
#include "object2db.h"

LoggerPtr logger(Logger::getLogger("dataserver"));

struct Player
{
    unsigned int id = 0;
    char name[32] = "";
    float score = .0;
    std::string bin;

    typedef std::vector<Player> Vector;
};

void dumpPlayer(const Player& p)
{
    std::cout << "Player:"
              << p.id << "\t"
              << p.name << "\t"
              << p.bin.size() << "\t"
              << p.score << std::endl;
}

class PlayerDBDescriptor : public DBDescriptor
{
public:
    typedef Player ObjectType;

    PlayerDBDescriptor()
    {
        table = "PLAYER";

        fields = {
                {"ID",    "int(10) unsigned NOT NULL default '0'"},
                {"NAME",  "varchar(32) NOT NULL default ''"},
                {"BIN",   "blob"},
                {"SCORE", "float NOT NULL default '0'"},
        };
    }

    void object2Query(mysqlpp::Query &query, const ObjectType& object)
    {
        query << object.id << ","
              << mysqlpp::quote << object.name << ","
              << mysqlpp::quote << object.bin << ","
              << object.score;
    }

    void pair2Query(mysqlpp::Query &query, const ObjectType& object)
    {
        query << "ID=" << object.id << ","
              << "NAME=" << mysqlpp::quote_only << object.name << ","
              << "BIN=" << mysqlpp::quote << object.bin << ","
              << "SCORE=" << object.score;
    }

    void key2Query(mysqlpp::Query &query, const ObjectType& object)
    {
        query << "ID=" << object.id;
    }

    //
    // Single Object
    //
    void makeInsertQuery(mysqlpp::Query &query, const ObjectType& object)
    {
        query << "INSERT INTO ` "<< table << "` (";
        fields2Query(query);
        query << ") ";
        query << "VALUES (";
        object2Query(query, object);
        query << ");";
    }

    void makeReplaceQuery(mysqlpp::Query& query, const ObjectType& object)
    {
        query << "REPLACE INTO ` "<< table << "` (";
        fields2Query(query);
        query << ") ";
        query << "VALUES (";
        object2Query(query, object);
        query << ");";
    }

    void makeUpdateQuery(mysqlpp::Query& query, const ObjectType& object)
    {
        query << "UPDATE `" << table << "` SET ";
        pair2Query(query, object);
        query << " WHERE ";
        key2Query(query, object);
        query << ";";
    }

    void makeDeleteQuery(mysqlpp::Query& query, const ObjectType& object)
    {
        query << "DELETE FROM `" << table << "` WHERE ";
        key2Query(query, object);
        query << ";";
    }

    void makeSelectQuery(mysqlpp::Query& query, const ObjectType& object)
    {
        query << "SELECT ";
        fields2Query(query);
        query << " FROM `" << table << "` WHERE ";
        key2Query(query, object);
        query << ";";
    }




    //
    // Row -> Object
    //
    bool loadFromRecord(Player& object, mysqlpp::Row& record)
    {
        if (record.size() != fields.size())
            return false;
        object.id = record[0];
        strncpy(object.name, record[1].data(), 31);
        object.bin.assign(record[2].data(), record[2].size());
        object.score = record[3];
        return true;
    }
};

typedef Object2DB<Player, PlayerDBDescriptor> PlayerDB;

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
    LOG4CXX_DEBUG(logger, "SQL:" << query.str());
    query.reset();
    pd.makeReplaceQuery(query, p);
    LOG4CXX_DEBUG(logger, "SQL:" << query.str());
    query.reset();
    pd.makeUpdateQuery(query, p);
    LOG4CXX_DEBUG(logger, "SQL:" << query.str());
    query.reset();
    pd.makeDeleteQuery(query, p);
    LOG4CXX_DEBUG(logger, "SQL:" << query.str());
    query.reset();
    pd.makeSelectFromQuery(query, "WHERE ID=100");
    LOG4CXX_DEBUG(logger, "SQL:" << query.str());
    query.reset();
    pd.makeDeleteFromQuery(query, "WHERE ID=100");
    LOG4CXX_DEBUG(logger, "SQL:" << query.str());
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

    return 0;
}
