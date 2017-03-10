#ifndef TINYWORLD_TINYORM_MYSQL_H
#define TINYWORLD_TINYORM_MYSQL_H

#include <vector>
#include <unordered_set>
#include "tinyorm.h"
#include "tinymysql.h"

class TinyORM {
public:
    TinyORM(MySqlConnectionPool *pool = MySqlConnectionPool::instance()) {
        if (pool) {
            pool_ = pool;
            mysql_ = pool->grab();
        }
    }

    TinyORM(mysqlpp::Connection *connection) {
        if (connection) {
            mysql_ = connection;
            pool_ = nullptr;
        }
    }

    ~TinyORM() {
        if (pool_)
            pool_->putback(mysql_);
    }

    //
    // 更新所有表格的结构
    //
    bool showTables(std::unordered_set<std::string> &tables);

    bool updateTables();

    //
    // 创建、删除、自动更新表结构
    //
    bool createTable(const std::string &table);

    bool dropTable(const std::string &table);

    bool updateTable(const std::string &table);


    //
    // 针对某个对象的数据库操作
    //
    template<typename T>
    bool select(T &obj);

    template<typename T>
    bool insert(T &obj);

    template<typename T>
    bool replace(T &obj);

    template<typename T>
    bool update(T &obj);

    template<typename T>
    bool del(T &obj);

    //
    // 数据库批量操作
    //
    template<typename T>
    bool loadFromDB(std::vector<T> &records, const char *clause, ...);

    template<typename T>
    bool deleteFromDB(const char *where, ...);

    template<typename T, typename TSet>
    static bool loadFromDB(TSet &records, const char *clause, ...);


    void test() {
        try {
            mysqlpp::Query query = mysql_->query();
            query << "show tables";
            mysqlpp::StoreQueryResult res = query.store();
            if (res) {
                LOG_TRACE(__FUNCTION__, "rows:%u", res.num_rows());
            }
        }
        catch (std::exception &err) {
            LOG_ERROR(__FUNCTION__, "error:%s", err.what());
        }
    }

public:
    //
    // Generate SQL
    //
    template<typename T>
    bool makeSelectQuery(mysqlpp::Query &query, const T &obj, TableDescriptor<T> *td = nullptr);

    template<typename T>
    bool makeInsertQuery(mysqlpp::Query &query, const T &obj, TableDescriptor<T> *td = nullptr);

    template<typename T>
    bool makeReplaceQuery(mysqlpp::Query &query, const T &obj, TableDescriptor<T> *td = nullptr);

    template<typename T>
    bool makeUpdateQuery(mysqlpp::Query &query, const T &obj, TableDescriptor<T> *td = nullptr);

    template<typename T>
    bool makeDeleteQuery(mysqlpp::Query &query, const T &obj, TableDescriptor<T> *td = nullptr);


protected:
    bool updateExistTable(TableDescriptorBase::Ptr td);


    //
    // value1,value2,...,valueN
    //
    template<typename T>
    void makeValueList(mysqlpp::Query &query, T &obj, TableDescriptor<T> *td, const FieldDescriptorList &fdlist);

    //
    // key1=value1,key2=valule2,...,keyN=valueN
    //
    template<typename T>
    void makeKeyValueList(mysqlpp::Query &query, T &obj, TableDescriptor<T> *td, const FieldDescriptorList &fdlist,
                          const std::string &seperator = ",");


    template<typename T>
    bool fieldToQuery(mysqlpp::Query &query, T &obj, TableDescriptor<T> *td, FieldDescriptor::Ptr fd);

    template<typename T>
    bool recordToObject(mysqlpp::Row &record, T &obj, TableDescriptor<T> *td);


private:
    mysqlpp::Connection *mysql_ = nullptr;
    MySqlConnectionPool *pool_ = nullptr;
};

#include "tinyorm_mysql.in.h"

#endif //TINYWORLD_TINYORM_MYSQL_H
