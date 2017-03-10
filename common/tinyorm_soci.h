#ifndef TINYWORLD_TINYORM_SOCI_H
#define TINYWORLD_TINYORM_SOCI_H

#include <vector>
#include <unordered_set>
#include <functional>
#include "tinyorm.h"
#include "tinydb.h"

class TinySociORM {
public:
    //
    // 构造: 支持两种方式:
    //   1. 连接池
    //   2. 指定连接
    //
    TinySociORM() {
    }

    //
    // 析构: 如果是用连接池初始化的则把连接放回
    //
    ~TinySociORM() {
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
    // 数据库批量加载
    //
    template<typename T>
    using Records = std::vector<std::shared_ptr<T>>;

    template<typename T>
    bool loadFromDB(Records<T> &records, const char *clause, ...);

    template<typename T, typename TSet>
    bool loadFromDB(TSet &records, const char *clause, ...);

    template<typename T, typename TMultiIndexSet>
    bool loadFromDB2MultiIndexSet(TMultiIndexSet &records, const char *clause, ...);

    template<typename T>
    bool loadFromDB(const std::function<void(std::shared_ptr<T>)> &callback, const char *clause, ...);

    template<typename T>
    bool vloadFromDB(const std::function<void(std::shared_ptr<T>)> &callback, const char *clause, va_list ap);

    //
    // 数据库批量删除
    //
    template<typename T>
    bool deleteFromDB(const char *where, ...);
};


#include "tinyorm_soci.in.h"

#endif //TINYWORLD_TINYORM_SOCI_H
