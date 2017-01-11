//
// Created by wangdawei on 2017/1/10.
//

#include "object2db.h"
#include "mylogger.h"

static LoggerPtr logger(Logger::getLogger("mysql"));

void DBDescriptor::errlog(const char* format, ...)
{
    char text[1024] = "";
    FormatArgs(text, sizeof(text), format);
    LOG4CXX_ERROR(logger, "[DB] [" << table << "] " << text);
}

void DBDescriptor::fields2Query(mysqlpp::Query &query)
{
    for (size_t i = 0; i < fields.size(); ++i)
    {
        query << "`" << fields[i].name << "`";
        if (i != (fields.size() - 1))
            query << ",";
    }
}

DBDescriptor::FieldInfo* DBDescriptor::getFieldInfo(const std::string &field)
{
    for (auto it = fields.begin(); it != fields.end(); ++it)
    {
        if (it->name == field)
            return &(*it);
    }
    return NULL;
}

void DBDescriptor::makeCreateQuery(mysqlpp::Query &query)
{
    query << "CREATE TABLE `"<< table <<"` (";

    if (keys.size() > 0)
    {
        for (auto field : fields)
        {
            query << "`" << field.name << "` " << field.dd_sql << ",";
        }

        query << "PRIMARY KEY(";
        for (size_t i = 0; i < keys.size(); ++i)
        {
            query << "`" << keys[i] << "`";
            if (i != (keys.size() - 1))
                query << ",";
        }
        query << ")";
    }
    else
    {
        for (size_t i = 0; i < fields.size(); ++i)
        {
            query << "`" << fields[i].name << "` " << fields[i].dd_sql;
            if (i != (fields.size() - 1))
                query << ",";
        }
    }

    query << ") ENGINE=MyISAM DEFAULT CHARSET=latin1;";
}

bool DBDescriptor::makeAddFieldQuery(mysqlpp::Query &query, const std::string &field) {
    FieldInfo *info = getFieldInfo(field);
    if (info) {
        query << "ALTER TABLE `" << table << "` ADD "
              << "`" << info->name << "` " << info->dd_sql;
        return true;
    }
    return false;
}

void DBDescriptor::makeSelectFromQuery(mysqlpp::Query& query, const char* clause)
{
    query << "SELECT ";
    fields2Query(query);
    query << " FROM `" << table << "` ";
    query << clause << ";";
}

void DBDescriptor::makeDeleteFromQuery(mysqlpp::Query& query, const char* clause)
{
    query << "DELETE FROM `" << table << "` " << clause << ";";
}

