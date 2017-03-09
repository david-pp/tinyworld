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
                std::for_each(name.begin(), name.end(), [](char& c){
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
    auto td = TableFactory::instance().getTableDescriptor(table);
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
    auto td = TableFactory::instance().getTableDescriptor(table);
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

        for (auto fd : td->fieldIterators()) {
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
    auto td = TableFactory::instance().getTableDescriptor(table);
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
bool TinyORM::insert(T &obj) {
    TableDescriptor<T> *td = (TableDescriptor<T> *) TableFactory::instance().getTableDescriptor("PLAYER").get();
    try {
        mysqlpp::Query query = mysql_->query();

        query << "INSERT INTO `" << td->table
              << "`(" << td->sql_fieldlist() << ")"
              << " VALUES (";

        size_t i = 0;
        for (auto fd : td->fieldIterators()) {

            if (fd->type == FieldType::UINT32)
                query << td->reflection.template get<uint32_t>(obj, fd->name);
            else if (fd->type == FieldType::VCHAR)
                query << mysqlpp::quote << td->reflection.template get<std::string>(obj, fd->name);
            else if (fd->type == FieldType::UINT8)
                query << (int) td->reflection.template get<uint8_t>(obj, fd->name);
            else if (fd->type == FieldType::OBJECT) {
                std::string bin;
                auto prop = td->reflection.getPropertyByName(fd->name);
                if (prop)
                    bin = prop->serialize(obj);
                query << mysqlpp::quote << bin;
            }


            i++;

            if (i != td->fieldIterators().size()) {
                query << ",";
            }
        }

        query << ")";

        mysqlpp::SimpleResult res = query.execute();
        if (res) {
            std::cout << "--- end ---" << std::endl;
        }
    }
    catch (std::exception &er) {
        std::cout << "Error:" << er.what() << std::endl;
    }

    return true;
}


#endif //TINYWORLD_TINYORM_MYSQL_IN_H
