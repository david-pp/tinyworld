#ifndef TINYWORLD_TINYORM_MYSQL_IN_H
#define TINYWORLD_TINYORM_MYSQL_IN_H

bool TinyORM::showTables(std::unordered_set<std::string> &tables) {

    try {
        mysqlpp::Query query = mysql_->query();
        query << "SHOW TABLES";
        mysqlpp::StoreQueryResult res = query.store();
        if (res) {
            for (auto it = res.begin(); it != res.end(); ++it) {

                std::string name = it->at(0).data();
                std::for_each(name.begin(), name.end(), [](char &c) {
                    c = std::toupper(c);
                });
                tables.insert(name);
            }
        }
    }
    catch (std::exception &e) {
        LOG_ERROR("TinyORM", "showTables, error:%s", e.what());
        return false;
    }


    return true;
}

bool TinyORM::updateTables() {

    std::unordered_set<std::string> tables;
    if (!showTables(tables))
        return false;

    for (auto it = TableFactory::instance().tables().begin();
         it != TableFactory::instance().tables().end(); ++it) {

        if (tables.find(it->second->table) == tables.end()) { // not exist
            createTable(it->second->table);
        } else {
            updateExistTable(it->second);
        }
    }

    return true;
}

bool TinyORM::createTable(const std::string &table) {
    auto td = TableFactory::instance().tableByName(table);
    if (!td) {
        LOG_ERROR("TinyORM", "createTable:%s, error:table meta is nonexist", table.c_str());
        return false;
    }

    try {
        mysqlpp::Query query = mysql_->query();
        query << td->sql_create();
        mysqlpp::SimpleResult res = query.execute();
        if (res) {
            LOG_TRACE("TinyORM", "createTable:%s, success", table.c_str());
            return true;
        }
    }
    catch (std::exception &e) {
        LOG_ERROR("TinyORM", "createTable:%s, error:%s", table.c_str(), e.what());
        return false;
    }

    LOG_ERROR("TinyORM", "createTable:%s, failed", table.c_str());
    return false;
}

bool TinyORM::dropTable(const std::string &table) {
    auto td = TableFactory::instance().tableByName(table);
    if (!td) {
        LOG_ERROR("TinyORM", "dropTable:%s, error:table meta is nonexist", table.c_str());
        return false;
    }

    try {
        mysqlpp::Query query = mysql_->query();
        query << td->sql_drop();
        mysqlpp::SimpleResult res = query.execute();
        if (res) {
            LOG_TRACE("TinyORM", "dropTable:%s, success", table.c_str());
            return true;
        }
    }
    catch (std::exception &e) {
        LOG_ERROR("TinyORM", "dropTable:%s, error:%s", table.c_str(), e.what());
        return false;
    }

    LOG_ERROR("TinyORM", "dropTable:%s, failed", table.c_str());
    return false;
}

bool TinyORM::updateExistTable(TableDescriptorBase::Ptr td) {
    try {
        mysqlpp::Query query = mysql_->query();
        query << "DESC `" << td->table << "`";

        std::set<std::string> fields_db;
        mysqlpp::StoreQueryResult res = query.store();
        if (res) {
            mysqlpp::StoreQueryResult::const_iterator it;
            for (it = res.begin(); it != res.end(); ++it) {
                fields_db.insert(it->at(0).data());
            }
        }

        for (auto fd : td->fields()) {
            // need update
            if (fields_db.find(fd->name) == fields_db.end()) {
                query.reset();
                query << td->sql_addfield(fd->name);
                if (query.execute()) {
                    LOG_TRACE("TinyORM", "updateExistTable:%s, add %s OK", td->table.c_str(), fd->name.c_str());
                } else {
                    LOG_ERROR("TinyORM", "updateExistTable:%s, add %s FAILED", td->table.c_str(), fd->name.c_str());
                }
            }
        }
        return true;
    }
    catch (std::exception &e) {
        LOG_ERROR("TinyORM", "updateExistTable:%s, error:%s", td->table.c_str(), e.what());
        return false;
    }
}

bool TinyORM::updateTable(const std::string &table) {
    auto td = TableFactory::instance().tableByName(table);
    if (!td) {
        LOG_ERROR("TinyORM", "updateTable:%s, error:table meta is nonexist", table.c_str());
        return false;
    }

    try {
        mysqlpp::Query query = mysql_->query();
        query << "SHOW TABLES LIKE " << mysqlpp::quote << table;
        mysqlpp::StoreQueryResult res = query.store();
        if (res) {
            if (res.num_rows() > 0) { // table exist
                updateExistTable(td);
            } else { // table not exist then create
                createTable(table);
            }

            return true;
        }
    }
    catch (std::exception &e) {
        LOG_ERROR("TinyORM", "updateTable:%s, error:%s", table.c_str(), e.what());
        return false;
    }

    return false;
}


template<typename T>
bool TinyORM::select(T &obj) {

    auto td = TableFactory::instance().tableByType<T>();
    if (!td) {
        LOG_ERROR("TinyORM", "%s: Table descriptor is not exist", __PRETTY_FUNCTION__);
        return false;
    }

    try {
        mysqlpp::Query query = mysql_->query();
        makeSelectQuery(query, obj, td);
        query << " LIMIT 1";

        LOG_TRACE("TinyORM", "%s", query.str().c_str());

        mysqlpp::StoreQueryResult res = query.store();
        if (res) {
            if (res.num_rows() == 1) {
                return recordToObject(res[0], obj, td);
            }
        }
    }
    catch (std::exception &err) {
        LOG_ERROR("TinyORM", "%s: %s", __PRETTY_FUNCTION__, err.what());
        return false;
    }

    return false;
}

template<typename T>
bool TinyORM::insert(T &obj) {

    auto td = TableFactory::instance().tableByType<T>();
    if (!td) {
        LOG_ERROR("TinyORM", "%s: Table descriptor is not exist", __PRETTY_FUNCTION__);
        return false;
    }

    try {
        mysqlpp::Query query = mysql_->query();
        makeInsertQuery(query, obj, td);
        LOG_TRACE("TinyORM", "%s", query.str().c_str());
        mysqlpp::SimpleResult res = query.execute();
        if (res) {
            return true;
        }
    }
    catch (std::exception &err) {
        LOG_ERROR("TinyORM", "%s: %s", __PRETTY_FUNCTION__, err.what());
        return false;
    }

    return false;
}


template<typename T>
bool TinyORM::replace(T &obj) {

    auto td = TableFactory::instance().tableByType<T>();
    if (!td) {
        LOG_ERROR("TinyORM", "%s: Table descriptor is not exist", __PRETTY_FUNCTION__);
        return false;
    }

    try {
        mysqlpp::Query query = mysql_->query();
        makeReplaceQuery(query, obj, td);
        LOG_TRACE("TinyORM", "%s", query.str().c_str());
        mysqlpp::SimpleResult res = query.execute();
        if (res) {
            return true;
        }
    }
    catch (std::exception &err) {
        LOG_ERROR("TinyORM", "%s: %s", __PRETTY_FUNCTION__, err.what());
        return false;
    }

    return false;
}

template<typename T>
bool TinyORM::update(T &obj) {

    auto td = TableFactory::instance().tableByType<T>();
    if (!td) {
        LOG_ERROR("TinyORM", "%s: Table descriptor is not exist", __PRETTY_FUNCTION__);
        return false;
    }

    try {
        mysqlpp::Query query = mysql_->query();
        makeUpdateQuery(query, obj, td);
        LOG_TRACE("TinyORM", "%s", query.str().c_str());
        mysqlpp::SimpleResult res = query.execute();
        if (res) {
            return true;
        }
    }
    catch (std::exception &err) {
        LOG_ERROR("TinyORM", "%s: %s", __PRETTY_FUNCTION__, err.what());
        return false;
    }

    return false;
}

template<typename T>
bool TinyORM::del(T &obj) {
    auto td = TableFactory::instance().tableByType<T>();
    if (!td) {
        LOG_ERROR("TinyORM", "%s: Table descriptor is not exist", __PRETTY_FUNCTION__);
        return false;
    }

    try {
        mysqlpp::Query query = mysql_->query();
        makeDeleteQuery(query, obj, td);
        LOG_TRACE("TinyORM", "%s", query.str().c_str());
        mysqlpp::SimpleResult res = query.execute();
        if (res) {
            return true;
        }
    }
    catch (std::exception &err) {
        LOG_ERROR("TinyORM", "%s: %s", __PRETTY_FUNCTION__, err.what());
        return false;
    }

    return false;
}

template<typename T>
bool TinyORM::vloadFromDB(const std::function<void(std::shared_ptr<T>)> &callback, const char *clause, va_list ap) {
    auto td = TableFactory::instance().tableByType<T>();
    if (!td) {
        LOG_ERROR("TinyORM", "%s: Table descriptor is not exist", __PRETTY_FUNCTION__);
        return false;
    }

    char statement[1024] = "";
    if (clause) {
        vsnprintf(statement, sizeof(statement), clause, ap);
    }

    try {
        mysqlpp::Query query = mysql_->query();
        query << "SELECT " << td->sql_fieldlist();
        query << " FROM `" << td->table << "` ";
        query << statement;

        LOG_TRACE("TinyORM", "%s", query.str().c_str());
        mysqlpp::StoreQueryResult res = query.store();
        if (res) {
            for (size_t i = 0; i < res.num_rows(); ++i) {
                std::shared_ptr<T> obj = std::make_shared<T>();
                if (recordToObject(res[i], *obj.get(), td)) {
                    callback(obj);
                } else {
                    LOG_ERROR("TinyORM", "%s: recordToObject FAILED", __PRETTY_FUNCTION__);
                }
            }
        }
    }
    catch (std::exception &err) {
        LOG_ERROR("TinyORM", "%s: %s", __PRETTY_FUNCTION__, err.what());
        return false;
    }

    return false;
}

template<typename T>
bool TinyORM::loadFromDB(const std::function<void(std::shared_ptr<T>)> &callback, const char *clause, ...) {
    va_list ap;
    va_start(ap, clause);
    bool ret = vloadFromDB<T>(callback, clause, ap);
    va_end(ap);

    return ret;
}

template<typename T>
bool TinyORM::loadFromDB(Records<T> &records, const char *clause, ...) {

    va_list ap;
    va_start(ap, clause);

    bool ret = vloadFromDB<T>([&records](std::shared_ptr<T> record) {
        records.push_back(record);
    }, clause, ap);

    va_end(ap);
    return ret;
}

template<typename T, typename TSet>
bool TinyORM::loadFromDB(TSet &records, const char *clause, ...) {
    va_list ap;
    va_start(ap, clause);

    bool ret = vloadFromDB<T>([&records](std::shared_ptr<T> record) {
        records.insert(record);
    }, clause, ap);

    va_end(ap);
    return ret;
};

template<typename T, typename TMultiIndexSet>
bool TinyORM::loadFromDB2MultiIndexSet(TMultiIndexSet &records, const char *clause, ...) {
    va_list ap;
    va_start(ap, clause);

    bool ret = vloadFromDB<T>([&records](std::shared_ptr<T> record) {
        records.insert(*record);
    }, clause, ap);

    va_end(ap);
    return ret;
};


template<typename T>
bool TinyORM::deleteFromDB(const char *where, ...) {
    auto td = TableFactory::instance().tableByType<T>();
    if (!td) {
        LOG_ERROR("TinyORM", "%s: Table descriptor is not exist", __PRETTY_FUNCTION__);
        return false;
    }

    char statement[1024] = "";
    if (where) {
        va_list ap;
        va_start(ap, where);
        vsnprintf(statement, sizeof(statement), where, ap);
        va_end(ap);
    }

    try {
        mysqlpp::Query query = mysql_->query();
        query << "DELETE FROM `" << td->table << "` ";
        query << statement;

        LOG_TRACE("TinyORM", "%s", query.str().c_str());
        mysqlpp::SimpleResult res = query.execute();
        if (res) {
            return true;
        }
    }
    catch (std::exception &err) {
        LOG_ERROR("TinyORM", "%s: %s", __PRETTY_FUNCTION__, err.what());
        return false;
    }


    return false;
}


template<typename T>
void TinyORM::makeValueList(mysqlpp::Query &query, T &obj, TableDescriptor<T> *td, const FieldDescriptorList &fdlist) {
    if (!td) return;

    for (size_t i = 0; i < fdlist.size(); ++i) {

        fieldToQuery(query, obj, td, fdlist[i]);

        if (i != fdlist.size() - 1) {
            query << ",";
        }
    }
}

template<typename T>
void TinyORM::makeKeyValueList(mysqlpp::Query &query, T &obj, TableDescriptor<T> *td, const FieldDescriptorList &fdlist,
                               const std::string &seperator) {

    for (size_t i = 0; i < fdlist.size(); ++i) {

        query << "`" << fdlist[i]->name << "`=";
        fieldToQuery(query, obj, td, fdlist[i]);

        if (i != fdlist.size() - 1) {
            query << seperator;
        }
    }
}


template<typename T>
bool TinyORM::fieldToQuery(mysqlpp::Query &query, T &obj, TableDescriptor<T> *td, FieldDescriptor::Ptr fd) {
    if (!td || !fd) return false;

    switch (fd->type) {

        case FieldType::INT8   : {
            query << (int) td->reflection.template get<int8_t>(obj, fd->name);
            return true;
        }
        case FieldType::INT16  : {
            query << td->reflection.template get<int16_t>(obj, fd->name);
            return true;
        }
        case FieldType::INT32  : {
            query << td->reflection.template get<int32_t>(obj, fd->name);
            return true;
        }
        case FieldType::INT64  : {
            query << td->reflection.template get<int64_t>(obj, fd->name);
            return true;
        }
        case FieldType::UINT8   : {
            query << (int) td->reflection.template get<uint8_t>(obj, fd->name);
            return true;
        }
        case FieldType::UINT16  : {
            query << td->reflection.template get<uint16_t>(obj, fd->name);
            return true;
        }
        case FieldType::UINT32  : {
            query << td->reflection.template get<uint32_t>(obj, fd->name);
            return true;
        }
        case FieldType::UINT64  : {
            query << td->reflection.template get<uint64_t>(obj, fd->name);
            return true;
        }

        case FieldType::BOOL   : {
            query << td->reflection.template get<bool>(obj, fd->name);
            return true;
        }

        case FieldType::FLOAT  : {
            query << td->reflection.template get<float>(obj, fd->name);
            return true;
        }
        case FieldType::DOUBLE : {
            query << td->reflection.template get<double>(obj, fd->name);
            return true;
        }

        case FieldType::STRING : {
            query << mysqlpp::quote << td->reflection.template get<std::string>(obj, fd->name);
            return true;
        }
        case FieldType::VCHAR  : {
            query << mysqlpp::quote << td->reflection.template get<std::string>(obj, fd->name);
            return true;
        }

        case FieldType::BYTES: {
            query << mysqlpp::quote << td->reflection.template get<std::string>(obj, fd->name);
            return true;
        }
        case FieldType::BYTES8: {
            query << mysqlpp::quote << td->reflection.template get<std::string>(obj, fd->name);
            return true;
        }
        case FieldType::BYTES24: {
            query << mysqlpp::quote << td->reflection.template get<std::string>(obj, fd->name);
            return true;
        }
        case FieldType::BYTES32: {
            query << mysqlpp::quote << td->reflection.template get<std::string>(obj, fd->name);
            return true;
        }
        case FieldType::OBJECT: {
            auto pd = td->reflection.propertyByName(fd->name);
            if (pd) {
                std::string bin = pd->serialize(obj);
                query << mysqlpp::quote << bin;
                return bin.size() > 0;
            } else {
                query << mysqlpp::quote << "";
                return false;
            }
        }
    }

    return true;
}

template<typename T>
bool TinyORM::recordToObject(mysqlpp::Row &record, T &obj, TableDescriptor<T> *td) {
    if (!td || record.size() < td->fields().size())
        return false;

    bool ret = true;

    for (size_t i = 0; i < td->fields().size(); ++i) {
        auto fd = td->fields().at(i);

        switch (fd->type) {

            case FieldType::INT8   : {
                td->reflection.template set<int8_t>(obj, fd->name, record[i]);
                break;
            }
            case FieldType::INT16  : {
                td->reflection.template set<int16_t>(obj, fd->name, record[i]);
                break;
            }
            case FieldType::INT32  : {
                td->reflection.template set<int32_t>(obj, fd->name, record[i]);
                break;
            }
            case FieldType::INT64  : {
                td->reflection.template set<int64_t>(obj, fd->name, record[i]);
                break;
            }
            case FieldType::UINT8   : {
                td->reflection.template set<uint8_t>(obj, fd->name, record[i]);
                break;
            }
            case FieldType::UINT16  : {
                td->reflection.template set<uint16_t>(obj, fd->name, record[i]);
                break;
            }
            case FieldType::UINT32  : {
                td->reflection.template set<uint32_t>(obj, fd->name, record[i]);
                break;
            }
            case FieldType::UINT64  : {
                td->reflection.template set<uint64_t>(obj, fd->name, record[i]);
                break;
            }

            case FieldType::BOOL   : {
                td->reflection.template set<bool>(obj, fd->name, record[i]);
                break;
            }

            case FieldType::FLOAT  : {
                td->reflection.template set<float>(obj, fd->name, record[i]);
                break;
            }
            case FieldType::DOUBLE : {
                td->reflection.template set<double>(obj, fd->name, record[i]);
                break;
            }

            case FieldType::STRING : {
                td->reflection.template set<std::string>(
                        obj, fd->name, std::string(record[i].data(), record[i].size()));
                break;
            }
            case FieldType::VCHAR  : {
                td->reflection.template set<std::string>(
                        obj, fd->name, std::string(record[i].data(), record[i].size()));
                break;
            }

            case FieldType::BYTES: {
                td->reflection.template set<std::string>(
                        obj, fd->name, std::string(record[i].data(), record[i].size()));
                break;
            }
            case FieldType::BYTES8: {
                td->reflection.template set<std::string>(
                        obj, fd->name, std::string(record[i].data(), record[i].size()));
                break;
            }
            case FieldType::BYTES24: {
                td->reflection.template set<std::string>(
                        obj, fd->name, std::string(record[i].data(), record[i].size()));
                break;
            }
            case FieldType::BYTES32: {
                td->reflection.template set<std::string>(
                        obj, fd->name, std::string(record[i].data(), record[i].size()));
                break;
            }
            case FieldType::OBJECT: {
                auto prop = td->reflection.propertyByName(fd->name);
                if (!prop) {
                    ret = false;
                    LOG_ERROR("TinyORM", "%s.%s Property Reflection is not exist", td->table.c_str(), fd->name.c_str());
                    break;
                }
                if (!prop->deserialize(obj, std::string(record[i].data(), record[i].size()))) {
                    ret = false;
                    LOG_ERROR("TinyORM", "%s.%s Property deserialize failed", td->table.c_str(), fd->name.c_str());
                    break;
                }
            }
        }
    }

    return ret;
}


template<typename T>
bool TinyORM::makeSelectQuery(mysqlpp::Query &query, const T &obj, TableDescriptor<T> *td) {
    if (!td) td = TableFactory::instance().tableByType<T>();
    if (!td) return false;

    query << "SELECT " << td->sql_fieldlist();
    query << " FROM `" << td->table << "` WHERE ";
    makeKeyValueList(query, const_cast<T &>(obj), td, td->keys(), " AND ");

    return true;
}

template<typename T>
bool TinyORM::makeInsertQuery(mysqlpp::Query &query, const T &obj, TableDescriptor<T> *td) {
    if (!td) td = TableFactory::instance().tableByType<T>();
    if (!td) return false;

    query << "INSERT INTO `" << td->table
          << "`(" << td->sql_fieldlist() << ")"
          << " VALUES (";
    makeValueList(query, const_cast<T &>(obj), td, td->fields());
    query << ")";

    return true;
}


template<typename T>
bool TinyORM::makeReplaceQuery(mysqlpp::Query &query, const T &obj, TableDescriptor<T> *td) {
    if (!td) td = TableFactory::instance().tableByType<T>();
    if (!td) return false;

    query << "REPLACE INTO `" << td->table
          << "`(" << td->sql_fieldlist() << ")"
          << " VALUES (";
    makeValueList(query, const_cast<T &>(obj), td, td->fields());
    query << ")";

    return true;
}

template<typename T>
bool TinyORM::makeUpdateQuery(mysqlpp::Query &query, const T &obj, TableDescriptor<T> *td) {
    if (!td) td = TableFactory::instance().tableByType<T>();
    if (!td) return false;

    query << "UPDATE `" << td->table << "` SET ";
    makeKeyValueList(query, const_cast<T &>(obj), td, td->fields());
    query << " WHERE ";
    makeKeyValueList(query, const_cast<T &>(obj), td, td->keys(), " AND ");

    return true;
}


template<typename T>
bool TinyORM::makeDeleteQuery(mysqlpp::Query &query, const T &obj, TableDescriptor<T> *td) {
    if (!td) td = TableFactory::instance().tableByType<T>();
    if (!td) return false;

    query << "DELETE FROM `" << td->table << "` WHERE ";
    makeKeyValueList(query, const_cast<T &>(obj), td, td->keys(), " AND ");

    return true;
}

#endif //TINYWORLD_TINYORM_MYSQL_IN_H
