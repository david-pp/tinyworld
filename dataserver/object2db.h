//
// Created by wangdawei on 2017/1/10.
//

#ifndef TINYWORLD_OBJECT2DB_H
#define TINYWORLD_OBJECT2DB_H

#include <string>
#include <vector>
#include <cstdarg>
#include <cstdio>


#include "mydb.h"

#define FormatArgs(buf, buflen, format) \
    do { \
        va_list ap; \
        va_start(ap, format); \
        vsnprintf(buf, buflen, format, ap); \
        va_end(ap); \
    } while(false)



////////////////////////////////////////////////////////////////
//
// 数据库表描述基类
//
////////////////////////////////////////////////////////////////
class DBDescriptor
{
public:
    //
    // 生成SQL片段
    //  fields2Query - 字段列表:    field1,field2,...,fieldN
    //  object2Query - 对象输出:    value1,value2,...,valueN
    //  pair2Query   - 用于UPDATE: field1=value1,field2=value2,...
    //  keys2Query   - 用于WHERE:  key1=value1,key2=value2,....
    //
    virtual void fields2Query(mysqlpp::Query &query);
    virtual void object2Query(mysqlpp::Query &query, const void* objptr) = 0;
    virtual void pair2Query(mysqlpp::Query &query, const void* objptr) = 0;
    virtual void key2Query(mysqlpp::Query &query, const void* objptr) = 0;

    //
    // 表的一列转换为对象
    //
    virtual bool record2Object(mysqlpp::Row& record, void* objptr) = 0;


public:
    //
    // 生成创建和修改表结构的SQL语句
    //
    void makeCreateQuery(mysqlpp::Query &query);
    bool makeAddFieldQuery(mysqlpp::Query &query, const std::string &field);


    //
    // 生成SELECT和DELETE语句
    //
    void makeSelectFromQuery(mysqlpp::Query& query, const char* clause);
    void makeDeleteFromQuery(mysqlpp::Query& query, const char* clause);

    //
    // 生成指定对象的INSERT,REPLACE,UPDATE,DELETE,SELECT语句
    //
    template <typename ObjectType>
    void makeInsertQuery(mysqlpp::Query &query, const ObjectType& object);

    template <typename ObjectType>
    void makeReplaceQuery(mysqlpp::Query& query, const ObjectType& object);

    template <typename ObjectType>
    void makeUpdateQuery(mysqlpp::Query& query, const ObjectType& object);

    template <typename ObjectType>
    void makeDeleteQuery(mysqlpp::Query& query, const ObjectType& object);

    template <typename ObjectType>
    void makeSelectQuery(mysqlpp::Query& query, const ObjectType& object);

    //
    // 表的一列转换为对应的C++结构
    //
    template <typename ObjectType>
    bool loadFromRecord(ObjectType& object, mysqlpp::Row& record);

public:
    struct FieldInfo
    {
        std::string name;
        std::string dd_sql;
    };

    typedef std::vector<FieldInfo> FieldInfos;
    FieldInfo *getFieldInfo(const std::string &field);

    void errlog(const char* format, ...);

    // 表名
    std::string table;
    // 主键字段名
    std::vector<std::string> keys;
    // 字段描述
    FieldInfos fields;
};

////////////////////////////////////////////////////////////////
//
// C++结构体存储到MYSQL
//
////////////////////////////////////////////////////////////////
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
    //
    // 该对象的数据库操作
    //
    bool selectDB();
    bool insertDB();
    bool replaceDB();
    bool updateDB();
    bool deleteDB();
};

#include "object2db.in.h"
#endif //TINYWORLD_OBJECT2DB_H
