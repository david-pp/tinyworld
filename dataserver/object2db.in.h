//
// Created by wangdawei on 2017/1/11.
//

#ifndef TINYWORLD_OBJECT2DB_IN_H
#define TINYWORLD_OBJECT2DB_IN_H

template <typename ObjectType>
inline void DBDescriptor::makeInsertQuery(mysqlpp::Query &query, const ObjectType& object)
{
    query << "INSERT INTO `"<< table << "` (";
    fields2Query(query);
    query << ") ";
    query << "VALUES (";
    object2Query(query, &object);
    query << ");";
}

template <typename ObjectType>
inline void DBDescriptor::makeReplaceQuery(mysqlpp::Query& query, const ObjectType& object)
{
    query << "REPLACE INTO `"<< table << "` (";
    fields2Query(query);
    query << ") ";
    query << "VALUES (";
    object2Query(query, &object);
    query << ");";
}

template <typename ObjectType>
inline void DBDescriptor::makeUpdateQuery(mysqlpp::Query& query, const ObjectType& object)
{
    query << "UPDATE `" << table << "` SET ";
    pair2Query(query, &object);
    query << " WHERE ";
    key2Query(query, &object);
    query << ";";
}

template <typename ObjectType>
inline void DBDescriptor::makeDeleteQuery(mysqlpp::Query& query, const ObjectType& object)
{
    query << "DELETE FROM `" << table << "` WHERE ";
    key2Query(query, &object);
    query << ";";
}

template <typename ObjectType>
inline void DBDescriptor::makeSelectQuery(mysqlpp::Query& query, const ObjectType& object)
{
    query << "SELECT ";
    fields2Query(query);
    query << " FROM `" << table << "` WHERE ";
    key2Query(query, &object);
    query << ";";
}


template <typename ObjectType>
inline bool DBDescriptor::loadFromRecord(ObjectType& object, mysqlpp::Row& record)
{
    return record2Object(record, &object);
}


/////////////////////////////////////////////////////////////////

template <typename T, typename T_DBDescriptor>
T_DBDescriptor Object2DB<T, T_DBDescriptor>::descriptor_;

template <typename T, typename T_DBDescriptor>
inline bool Object2DB<T, T_DBDescriptor>::createTable()
{
    ScopedMySqlConnection mysql;
    if (!mysql)
    {
        descriptor_.errlog("createTable, 无法获取mysql连接");
        return false;
    }

    try
    {
        mysqlpp::Query query = mysql->query();
        descriptor_.makeCreateQuery(query);
        mysqlpp::SimpleResult res = query.execute();
        if (res)
        {
            return true;
        }
    }
    catch (std::exception& err)
    {
        descriptor_.errlog("createTable, %s", err.what());
        return false;
    }

    return false;
}

template <typename T, typename T_DBDescriptor>
inline bool Object2DB<T, T_DBDescriptor>::dropTable()
{
    ScopedMySqlConnection mysql;
    if (!mysql)
    {
        descriptor_.errlog("dropTable, 无法获取mysql连接");
        return false;
    }

    try
    {
        mysqlpp::Query query = mysql->query();
        query << "DROP TABLE " << descriptor_.table;
        mysqlpp::SimpleResult res = query.execute();
        if (res)
        {
            return true;
        }
    }
    catch (std::exception& err)
    {
        descriptor_.errlog("dropTable, %s", err.what());
        return false;
    }

    return false;
}

template <typename T, typename T_DBDescriptor>
inline bool Object2DB<T, T_DBDescriptor>::updateTable()
{
    ScopedMySqlConnection mysql;
    if (!mysql)
    {
        descriptor_.errlog("updateTable, 无法获取mysql连接");
        return false;
    }

    try
    {
        mysqlpp::Query query = mysql->query();
        query << "SHOW TABLES LIKE " << mysqlpp::quote << descriptor_.table;
        mysqlpp::StoreQueryResult res = query.store();
        if (res)
        {
            // 存在
            if (res.num_rows() > 0)
            {
                query.reset();
                query << "DESC `" << descriptor_.table << "`";

                std::set<std::string> fields_db;
                mysqlpp::StoreQueryResult res = query.store();
                if (res)
                {
                    mysqlpp::StoreQueryResult::const_iterator it;
                    for (it = res.begin(); it != res.end(); ++it)
                    {
                        mysqlpp::Row row = *it;
                        fields_db.insert(row[0].data());
                    }
                }

                for (auto field : descriptor_.fields)
                {
                    // 需要更新
                    if (fields_db.find(field.name) == fields_db.end())
                    {
                        query.reset();
                        if (descriptor_.makeAddFieldQuery(query, field.name))
                        {
                            query.execute();
                        }
                    }
                }
            }
            // 不存在则创建
            else
            {
                query.reset();
                descriptor_.makeCreateQuery(query);
                query.execute();
            }

            return true;
        }
    }
    catch (std::exception& err)
    {
        descriptor_.errlog("updateTable, %s", err.what());
        return false;
    }

    return false;
}

////////////////////////////////////////////////////////////////////

template <typename T, typename T_DBDescriptor>
inline bool Object2DB<T, T_DBDescriptor>::loadFromDB(Records &records, const char *clause, ...)
{
    char sql[1024] = "";
    FormatArgs(sql, sizeof(sql), clause);

    ScopedMySqlConnection mysql;
    if (!mysql)
    {
        descriptor_.errlog("loadFromDB, 无法获取mysql连接");
        return false;
    }

    try
    {
        mysqlpp::Query query = mysql->query();
        descriptor_.makeSelectFromQuery(query, sql);
        mysqlpp::StoreQueryResult res = query.store();
        if (res)
        {
            for (size_t i = 0; i < res.num_rows(); i++)
            {
                T object;
                if (descriptor_.loadFromRecord(object, res[i]))
                {
                    records.push_back(object);
                }
            }
            return true;
        }
    }
    catch (std::exception& err)
    {
        descriptor_.errlog("updateTable, %s", err.what());
        return false;
    }

    return false;
}

template <typename T, typename T_DBDescriptor>
inline bool Object2DB<T, T_DBDescriptor>::deleteFromDB(const char* where, ...)
{
    char sql[1024] = "";
    FormatArgs(sql, sizeof(sql), where);

    ScopedMySqlConnection mysql;
    if (!mysql)
    {
        descriptor_.errlog("deleteFromDB, 无法获取mysql连接");
        return false;
    }

    try
    {
        mysqlpp::Query query = mysql->query();
        descriptor_.makeDeleteFromQuery(query, sql);
        mysqlpp::SimpleResult res = query.execute();
        if (res)
        {
            return true;
        }
    }
    catch (std::exception& err)
    {
        descriptor_.errlog("deleteFromDB, %s", err.what());
        return false;
    }

    return false;
};


template <typename T, typename T_DBDescriptor>
inline bool Object2DB<T, T_DBDescriptor>::insertDB()
{
    ScopedMySqlConnection mysql;
    if (!mysql)
    {
        descriptor_.errlog("insertFromDB, 无法获取mysql连接");
        return false;
    }

    try
    {
        mysqlpp::Query query = mysql->query();
        descriptor_.makeInsertQuery(query, *this);
        mysqlpp::SimpleResult res = query.execute();
        if (res)
        {
            return true;
        }
    }
    catch (std::exception& err)
    {
        descriptor_.errlog("insertFromDB, %s", err.what());
        return false;
    }

    return false;
}


template <typename T, typename T_DBDescriptor>
inline bool Object2DB<T, T_DBDescriptor>::selectDB()
{
    ScopedMySqlConnection mysql;
    if (!mysql)
    {
        descriptor_.errlog("selectDB, 无法获取mysql连接");
        return false;
    }

    try
    {
        mysqlpp::Query query = mysql->query();
        descriptor_.makeSelectQuery(query, *this);
        mysqlpp::StoreQueryResult res = query.store();
        if (res)
        {
            if (res.num_rows() == 1)
            {
                descriptor_.loadFromRecord(*this, res[0]);
                return true;
            }
        }
    }
    catch (std::exception& err)
    {
        descriptor_.errlog("selectDB, %s", err.what());
        return false;
    }

    return false;
}


template <typename T, typename T_DBDescriptor>
inline bool Object2DB<T, T_DBDescriptor>::replaceDB()
{
    ScopedMySqlConnection mysql;
    if (!mysql)
    {
        descriptor_.errlog("replaceDB, 无法获取mysql连接");
        return false;
    }

    try
    {
        mysqlpp::Query query = mysql->query();
        descriptor_.makeReplaceQuery(query, *this);
        mysqlpp::SimpleResult res = query.execute();
        if (res)
        {
            return true;
        }
    }
    catch (std::exception& err)
    {
        descriptor_.errlog("replaceDB, %s", err.what());
        return false;
    }

    return false;
}

template <typename T, typename T_DBDescriptor>
inline bool Object2DB<T, T_DBDescriptor>::updateDB()
{
    ScopedMySqlConnection mysql;
    if (!mysql)
    {
        descriptor_.errlog("updateDB, 无法获取mysql连接");
        return false;
    }

    try
    {
        mysqlpp::Query query = mysql->query();
        descriptor_.makeUpdateQuery(query, *this);
        mysqlpp::SimpleResult res = query.execute();
        if (res)
        {
            return true;
        }
    }
    catch (std::exception& err)
    {
        descriptor_.errlog("updateTable, %s", err.what());
        return false;
    }

    return false;
}

template <typename T, typename T_DBDescriptor>
inline bool Object2DB<T, T_DBDescriptor>::deleteDB()
{
    ScopedMySqlConnection mysql;
    if (!mysql)
    {
        descriptor_.errlog("deleteDB, 无法获取mysql连接");
        return false;
    }

    try
    {
        mysqlpp::Query query = mysql->query();
        descriptor_.makeDeleteQuery(query, *this);
        mysqlpp::SimpleResult res = query.execute();
        if (res)
        {
            return true;
        }
    }
    catch (std::exception& err)
    {
        descriptor_.errlog("deleteDB, %s", err.what());
        return false;
    }

    return false;
}

#endif //TINYWORLD_OBJECT2DB_IN_H
