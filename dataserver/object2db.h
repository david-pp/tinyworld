//
// Created by wangdawei on 2017/1/10.
//

#ifndef TINYWORLD_OBJECT2DB_H
#define TINYWORLD_OBJECT2DB_H

#include <string>
#include <vector>
#include <cstdarg>
#include <cstdio>

#include "mylogger.h"
#include "mydb.h"

struct MySQL
{
    static LoggerPtr logger;
};

class DBDescriptor
{
public:
    struct FieldInfo
    {
        std::string name;
        std::string dd_sql;
    };

    typedef std::vector<FieldInfo> FieldInfos;

    std::string table;

    FieldInfos fields;

public:
    FieldInfo *getFieldInfo(const std::string &field) {
        for (auto it = fields.begin(); it != fields.end(); ++it) {
            if (it->name == field)
                return &(*it);
        }
        return NULL;
    }

    //
    // Table Meta
    //
    void makeCreateQuery(mysqlpp::Query &query) {
        query << "CREATE TABLE `PLAYER` (";

        for (auto field : fields) {
            query << "`" << field.name << "` " << field.dd_sql << ",";
        }

        query << "PRIMARY KEY(`ID`)"
              << ") ENGINE=MyISAM DEFAULT CHARSET=latin1;";
    }

    bool makeAddFieldQuery(mysqlpp::Query &query, const std::string &field) {
        FieldInfo *info = getFieldInfo(field);
        if (info) {
            query << "ALTER TABLE `PLAYER` ADD "
                  << "`" << info->name << "` " << info->dd_sql;
            return true;
        }
        return false;
    }


    void fields2Query(mysqlpp::Query &query)
    {
        for (size_t i = 0; i < fields.size(); ++i)
        {
            query << "`" << fields[i].name << "`";
            if (i != (fields.size() - 1))
                query << ",";
        }
    }

    //
    // Batch
    //
    void makeSelectFromQuery(mysqlpp::Query& query, const char* clause)
    {
        query << "SELECT ";
        fields2Query(query);
        query << " FROM `" << table << "` ";
        query << clause << ";";
    }

    void makeDeleteFromQuery(mysqlpp::Query& query, const char* clause)
    {
        query << "DELETE FROM `" << table << "` " << clause << ";";
    }
};

template <typename T, typename T_DBDescriptor>
struct Object2DB : public T
{
public:
    typedef std::vector<T> Records;

    Object2DB() {}
    Object2DB(const T& object) : T(object) {}

    //
    // 创建、删除、自动更新表结构
    //
    static bool createTable();
    static bool dropTable();
    static bool updateTable();

    //
    // 数据库批量操作
    //
    static bool loadFromDB(Records& records, const char* clause, ...);
    static bool deleteFromDB(const char* where, ...);

protected:
    // 数据库描述
    static T_DBDescriptor descriptor_;

public:
    bool selectDB();
    bool insertDB();
    bool replaceDB();
    bool updateDB();
    bool deleteDB();
};


#define FormatArgs(buf, buflen, format) \
    do { \
        va_list ap; \
        va_start(ap, format); \
        vsnprintf(buf, buflen, format, ap); \
        va_end(ap); \
    } while(false)


#include "object2db.in.h"


#endif //TINYWORLD_OBJECT2DB_H
