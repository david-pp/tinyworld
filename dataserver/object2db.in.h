//
// Created by wangdawei on 2017/1/11.
//

#ifndef TINYWORLD_OBJECT2DB_IN_H
#define TINYWORLD_OBJECT2DB_IN_H

template <typename T, typename T_DBDescriptor>
T_DBDescriptor Object2DB<T, T_DBDescriptor>::descriptor_;

template <typename T, typename T_DBDescriptor>
bool Object2DB<T, T_DBDescriptor>::createTable()
{
    ScopedMySqlConnection mysql;
    if (!mysql)
    {
        LOG4CXX_ERROR(MySQL::logger, "[Object2DB] 无法获取mysql连接");
        return false;
    }

    try
    {
        mysqlpp::Query query = mysql->query();
        descriptor_.makeCreateQuery(query);
        mysqlpp::SimpleResult res = query.execute();
        if (res)
        {
            LOG4CXX_INFO(MySQL::logger, "create table: ok");
            return true;
        }
    }
    catch (std::exception& err)
    {
        LOG4CXX_WARN(MySQL::logger, "mysql error:" << err.what());
        return false;
    }

    return false;
}

template <typename T, typename T_DBDescriptor>
bool Object2DB<T, T_DBDescriptor>::dropTable()
{
    ScopedMySqlConnection mysql;
    if (!mysql)
    {
        LOG4CXX_ERROR(MySQL::logger, "[TC] Player2DB::dropTable, 无法获取mysql连接")
        return false;
    }

    try
    {
        mysqlpp::Query query = mysql->query();
        query << "DROP TABLE " << descriptor_.table;
        mysqlpp::SimpleResult res = query.execute();
        if (res)
        {
            LOG4CXX_INFO(MySQL::logger, "drop table: ok " << res.info());
            return true;
        }
    }
    catch (std::exception& err)
    {
        LOG4CXX_WARN(MySQL::logger, "mysql error:" << err.what());
        return false;
    }

    return false;
}

template <typename T, typename T_DBDescriptor>
bool Object2DB<T, T_DBDescriptor>::updateTable()
{
    ScopedMySqlConnection mysql;
    if (!mysql)
    {
        LOG4CXX_ERROR(MySQL::logger, "[TC] Player2DB::dropTable, 无法获取mysql连接")
        return false;
    }

    try
    {
        mysqlpp::Query query = mysql->query();
        query << "SHOW TABLES LIKE " << mysqlpp::quote << descriptor_.table;
        mysqlpp::StoreQueryResult res = query.store();
        if (res)
        {
            if (res.num_rows() > 0)
            {
                LOG4CXX_DEBUG(MySQL::logger, "存在");
                query.reset();
                query << "DESC `PLAYER`";

                std::set<std::string> fields_db;

                mysqlpp::StoreQueryResult res = query.store();
                if (res)
                {
                    mysqlpp::StoreQueryResult::const_iterator it;
                    for (it = res.begin(); it != res.end(); ++it)
                    {
                        mysqlpp::Row row = *it;
                        LOG4CXX_INFO(MySQL::logger, row[0] << "\t" << row[1]);
                        fields_db.insert(row[0].data());
                    }
                }

                for (auto field : descriptor_.fields)
                {
                    if (fields_db.find(field.name) == fields_db.end())
                    {
                        LOG4CXX_DEBUG(MySQL::logger, "需要更新");
                        query.reset();
                        if (descriptor_.makeAddFieldQuery(query, field.name))
                        {
                            query.execute();
                        }
                    }
                }
            }
            else
            {
                LOG4CXX_DEBUG(MySQL::logger, "不存在");
                query.reset();
                descriptor_.makeCreateQuery(query);
                query.execute();
            }
            return true;
        }
    }
    catch (std::exception& err)
    {
        LOG4CXX_WARN(MySQL::logger, "mysql error:" << err.what());
        return false;
    }

    return false;
}

////////////////////////////////////////////////////////////////////

template <typename T, typename T_DBDescriptor>
bool Object2DB<T, T_DBDescriptor>::loadFromDB(Records &records, const char *clause, ...)
{
    char sql[1024] = "";
    FormatArgs(sql, sizeof(sql), clause);

    ScopedMySqlConnection mysql;
    if (!mysql)
    {
        LOG4CXX_ERROR(MySQL::logger, "[TC] Player2DB::dropTable, 无法获取mysql连接")
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
        LOG4CXX_WARN(MySQL::logger, "mysql error:" << err.what());
        return false;
    }

    return false;
}

template <typename T, typename T_DBDescriptor>
bool Object2DB<T, T_DBDescriptor>::deleteFromDB(const char* where, ...)
{
    char sql[1024] = "";
    FormatArgs(sql, sizeof(sql), where);

    ScopedMySqlConnection mysql;
    if (!mysql)
    {
        LOG4CXX_ERROR(MySQL::logger, "[TC] Player2DB::dropTable, 无法获取mysql连接")
        return false;
    }

    try
    {
        mysqlpp::Query query = mysql->query();
        descriptor_.makeDeleteFromQuery(query, sql);
        mysqlpp::SimpleResult res = query.execute();
        if (res)
        {
            LOG4CXX_INFO(MySQL::logger, "delete ok");
            return true;
        }
    }
    catch (std::exception& err)
    {
        LOG4CXX_WARN(MySQL::logger, "mysql error:" << err.what());
        return false;
    }

    return false;
};


template <typename T, typename T_DBDescriptor>
bool Object2DB<T, T_DBDescriptor>::insertDB()
{
    ScopedMySqlConnection mysql;
    if (!mysql)
    {
        LOG4CXX_ERROR(MySQL::logger, "[TC] Player2DB::dropTable, 无法获取mysql连接")
        return false;
    }

    try
    {
        mysqlpp::Query query = mysql->query();
        descriptor_.makeInsertQuery(query, *this);
        mysqlpp::SimpleResult res = query.execute();
        if (res)
        {
            LOG4CXX_INFO(MySQL::logger, "insert ok");
            return true;
        }
    }
    catch (std::exception& err)
    {
        LOG4CXX_WARN(MySQL::logger, "mysql error:" << err.what());
        return false;
    }

    return false;
}


template <typename T, typename T_DBDescriptor>
bool Object2DB<T, T_DBDescriptor>::selectDB()
{
    ScopedMySqlConnection mysql;
    if (!mysql)
    {
        LOG4CXX_ERROR(MySQL::logger, "[TC] Player2DB::dropTable, 无法获取mysql连接")
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
        LOG4CXX_WARN(MySQL::logger, "mysql error:" << err.what());
        return false;
    }

    return false;
}


template <typename T, typename T_DBDescriptor>
bool Object2DB<T, T_DBDescriptor>::replaceDB()
{
    ScopedMySqlConnection mysql;
    if (!mysql)
    {
        LOG4CXX_ERROR(MySQL::logger, "[TC] Player2DB::dropTable, 无法获取mysql连接")
        return false;
    }

    try
    {
        mysqlpp::Query query = mysql->query();
        descriptor_.makeReplaceQuery(query, *this);
        mysqlpp::SimpleResult res = query.execute();
        if (res)
        {
            LOG4CXX_INFO(MySQL::logger, "replace ok");
            return true;
        }
    }
    catch (std::exception& err)
    {
        LOG4CXX_WARN(MySQL::logger, "mysql error:" << err.what());
        return false;
    }

    return false;
}

template <typename T, typename T_DBDescriptor>
bool Object2DB<T, T_DBDescriptor>::updateDB()
{
    ScopedMySqlConnection mysql;
    if (!mysql)
    {
        LOG4CXX_ERROR(MySQL::logger, "[TC] Player2DB::dropTable, 无法获取mysql连接")
        return false;
    }

    try
    {
        mysqlpp::Query query = mysql->query();
        descriptor_.makeUpdateQuery(query, *this);
        mysqlpp::SimpleResult res = query.execute();
        if (res)
        {
            LOG4CXX_INFO(MySQL::logger, "update ok");
            return true;
        }
    }
    catch (std::exception& err)
    {
        LOG4CXX_WARN(MySQL::logger, "mysql error:" << err.what());
        return false;
    }

    return false;
}

template <typename T, typename T_DBDescriptor>
bool Object2DB<T, T_DBDescriptor>::deleteDB()
{
    ScopedMySqlConnection mysql;
    if (!mysql)
    {
        LOG4CXX_ERROR(MySQL::logger, "[TC] Player2DB::dropTable, 无法获取mysql连接")
        return false;
    }

    try
    {
        mysqlpp::Query query = mysql->query();
        descriptor_.makeDeleteQuery(query, *this);
        mysqlpp::SimpleResult res = query.execute();
        if (res)
        {
            LOG4CXX_INFO(MySQL::logger, "delete ok");
            return true;
        }
    }
    catch (std::exception& err)
    {
        LOG4CXX_WARN(MySQL::logger, "mysql error:" << err.what());
        return false;
    }

    return false;
}

#endif //TINYWORLD_OBJECT2DB_IN_H
