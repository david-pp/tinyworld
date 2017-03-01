#ifndef TINYWORLD_TINYORM_H
#define TINYWORLD_TINYORM_H

#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
#include <memory>

enum class FieldType : uint8_t {
    INT8,
    INT16,
    INT32,
    INT64,

    UINT8,
    UINT16,
    UINT32,
    UINT64,

    BOOL,

    FLOAT,
    DOUBLE,

    STRING,
    VCHAR,

    BYTES,
    BYTES8,
    BYTES24,
    BYTES32,

    OBJECT,
};

class FieldDescriptor {
public:
    typedef std::shared_ptr<FieldDescriptor> Ptr;

    FieldDescriptor(const std::string &_name,
                    FieldType _type,
                    const std::string &_deflt,
                    size_t _size)
            : name(_name), type(_type), deflt(_deflt), size(_size) {}

    std::string sql_ddl();

    std::string sql_type();
    std::string sql_default();

    // 字段名
    std::string name;
    // 字段类型
    FieldType type;
    // 默认值
    std::string deflt;
    // 字段大小(部分类型有效)
    uint32_t size;
};

class TableDescriptor {
public:
    typedef std::shared_ptr<TableDescriptor> Ptr;

    std::string sql_create();
    std::string sql_drop();
    std::string sql_addfield(const std::string &field);

public:
    TableDescriptor &field(const std::string &name,
                           FieldType type,
                           const std::string &deflt = "",
                           size_t size = 0);

    TableDescriptor &key(const std::string &name);
    TableDescriptor &keys(const std::initializer_list<std::string> &names);

    TableDescriptor &index(const std::string &name);
    TableDescriptor &indexs(const std::initializer_list<std::string> &names);

    FieldDescriptor::Ptr getFieldDescriptor(const std::string &name);
    std::vector<FieldDescriptor::Ptr> &fieldIterators() { return fields_ordered_; }

public:
    TableDescriptor(const std::string name)
            : table(name) {}

    // 表名
    std::string table;
    // 主键字段
    std::vector<FieldDescriptor::Ptr> keys_;
    // 索引字段
    std::vector<FieldDescriptor::Ptr> indexs_;
    // 字段描述
    std::vector<FieldDescriptor::Ptr> fields_ordered_;
    std::unordered_map<std::string, FieldDescriptor::Ptr> fields_;
};

#include "tinyreflection.h"

template <typename T>
class TableDescriptor_T : public TableDescriptor{
public:
    Struct<T> reflection;
};

class TableFactory {
public:
    static TableFactory &instance() {
        static TableFactory factory_;
        return factory_;
    }

    TableDescriptor &table(const std::string &name) {
        TableDescriptor *td = new TableDescriptor(name);
        TableDescriptor::Ptr ptr(td);
        tables_[name] = ptr;
        return *td;
    }

    TableDescriptor::Ptr getTableDescriptor(const std::string &name) {
        auto it = tables_.find(name);
        if (it != tables_.end())
            return it->second;
        return nullptr;
    }

private:
    std::unordered_map<std::string, TableDescriptor::Ptr> tables_;
};


#endif //TINYWORLD_TINYORM_H