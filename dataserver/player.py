#!/usr/bin/python
# -*- coding: UTF-8 -*-

import yaml
import codecs
import sys

# reload(sys)
# sys.setdefaultencoding('utf-8')


ymlfile = file('player.yml', 'r')

# player = yaml.load(ymlfile)
# print player

def printline(file, depth, text):
    """print line with indent"""
    for i in range(0, depth):
        print >> file, '\t',
    print >> file, text

class StructField:

    def __init__(self):
        # 字段名
        self.name = ''
        # 字段类型
        self.type = ''
        # 字段大小
        self.size = 0
        # 注释
        self.comment = ''
        # 是键值吗?
        self.iskey = False

    def __str__(self):
        return "%s,%s,%u" % (self.name, self.type, self.size)

    def parseFromYAML(self, yamlfield):
        # print yamlfield

        if 0 == len(yamlfield):
            return False

        self.name = yamlfield.keys()[0]
        attributes = yamlfield[self.name]

        if isinstance(attributes, str):
            self.type = attributes
        elif isinstance(attributes, dict):
            self.type = attributes['type']

            if attributes.has_key('size'):
                self.size = attributes['size']
            if attributes.has_key('comment'):
                self.comment = attributes['comment']
            if attributes.has_key('primary_key'):
                self.iskey = True
        else:
            return False

        return True

    def makeFieldInfo(self):
        info = { 'name': '`%s`' % self.name.upper(), 'ddl': ''}
        if self.type == 'unsigned int':
            info['ddl'] = 'int(10) unsigned NOT NULL default \'0\''
        elif self.type == 'cstring':
            info['ddl'] = 'varchar(%u) NOT NULL default \'\'' % self.size
        elif self.type == 'string':
            info['ddl'] = 'blob'
        elif self.type == 'float':
            info['ddl'] = 'float NOT NULL default \'0\''
        return info


class StructDescription:
    def __init__(self):
        # 结构名称
        self.name = ''
        # 结构字段
        self.fields = []
        # 主键
        self.primary_keys = []

    def dump(self):
        print 'Struct:%s' % self.name
        print '---'
        for f in self.fields:
            print " -", f
        for k in self.primary_keys:
            print " - KEYS:", k

    def makeFieldList(self, fields):
        flist = ""
        for i in range(len(fields)):
            flist += '"`%s`"' % fields[i].name.upper()
            if i != len(fields)-1:
                flist += ','
        return flist

    def parseByYAML(self, name, doc):
        self.name = name
        for yamlfield in doc:
            field = StructField()
            if field.parseFromYAML(yamlfield):
                self.fields.append(field)
                if field.iskey:
                    self.primary_keys.append(field)

    def generateStruct(self, depth):
        file = sys.stdout
        printline(file, depth, 'struct %s {' % self.name)
        for f in self.fields:
            printline(file, depth+1, '%s %s;' % (f.type, f.name))
        printline(file, depth, '};')

    def generateDBDescriptor(self, depth):
        file = sys.stdout
        classname = self.name + "DBDescriptor"
        tablename = self.name.upper()

        printline(file, depth, 'class %s : public DBDescriptor {' % classname)
        printline(file, depth, 'public:')
        printline(file, depth+1, 'typedef %s ObjectType;' % self.name)
        printline(file, depth+1, '')

        # 构造函数
        printline(file, depth+1, '%s() {' % classname)
        printline(file, depth+2,    'table = "%s"' % tablename)
        printline(file, depth+2,    'keys = {%s}' % self.makeFieldList(self.primary_keys))
        printline(file, depth+2,    'fields = {')
        for f in self.fields:
            info = f.makeFieldInfo()
            printline(file, depth+3,    '{"%s", "%s"}' % (info['name'], info['ddl']))
        printline(file, depth+2,    '};')
        printline(file, depth+1, '};')
        printline(file, depth+1, '')

        # object2Query
        printline(file, depth+1, 'void object2Query(mysqlpp::Query &query, const void* objptr) {')
        printline(file, depth+2,    'const ObjectType& object = *(ObjectType*)objptr;')
        for i in range(len(self.fields)):
            if self.fields[i].type == 'cstring' or self.fields[i].type == 'string':
                printline(file, depth+2, 'query << mysqlpp::quote << object.%s;' % self.fields[i].name)
            else:
                printline(file, depth+2, 'query << object.%s;' % self.fields[i].name)
            if i != len(self.fields)-1:
                printline(file, depth+2, 'query << ",";')
        printline(file, depth+1, '};')
        printline(file, depth+1, '')

        # pair2Query
        printline(file, depth+1, 'void pair2Query(mysqlpp::Query &query, const void* objptr) {')
        printline(file, depth+2,    'const ObjectType& object = *(ObjectType*)objptr;')
        for i in range(len(self.fields)):
            if self.fields[i].type == 'cstring' or self.fields[i].type == 'string':
                printline(file, depth+2, 'query << "`%s`=" << mysqlpp::quote << object.%s;' % (self.fields[i].name.upper(), self.fields[i].name))
            else:
                printline(file, depth+2, 'query << "`%s`=" << object.%s;' % (self.fields[i].name.upper(), self.fields[i].name))
            if i != len(self.fields)-1:
                printline(file, depth+2, 'query << ",";')
        printline(file, depth+1, '};')
        printline(file, depth+1, '')

        # key2Query
        printline(file, depth+1, 'void key2Query(mysqlpp::Query &query, const void* objptr){')
        printline(file, depth+2,    'const ObjectType& object = *(ObjectType*)objptr;')
        for i in range(len(self.primary_keys)):
            if self.primary_keys[i].type == 'cstring' or self.primary_keys[i].type == 'string':
                printline(file, depth+2, 'query << "`%s`=" << mysqlpp::quote << object.%s;' % (self.primary_keys[i].name.upper(), self.primary_keys[i].name))
            else:
                printline(file, depth+2, 'query << "`%s`=" << object.%s;' % (self.primary_keys[i].name.upper(), self.primary_keys[i].name))
            if i != len(self.primary_keys)-1:
                printline(file, depth+2, 'query << ",";')
        printline(file, depth+1, '};')
        printline(file, depth+1, '')

        # record2Object
        printline(file, depth+1, 'bool record2Object(mysqlpp::Row& record, void* objptr){')
        printline(file, depth+2,    'if (record.size() != fields.size()) return false;');
        printline(file, depth+2,    'ObjectType& object = *(ObjectType*)objptr;')
        for i in range(len(self.fields)):
            f = self.fields[i]
            if f.type =='cstring':
                printline(file, depth+2, 'strncpy(object.%s, record[%u].data(), %u);' % (f.name, i, f.size-1))
            elif f.type == 'string':
                printline(file, depth+2, 'object.%s.assign(record[%u].data(), record[%u].size());' % (f.name, i, i))
            else:
                printline(file, depth+2, 'object.%s = record[%u];' % (self.fields[i].name, i))
        printline(file, depth+1, '};')
        printline(file, depth+1, '')

        printline(file, 0, '};')

def parse_ddl(doc):
    for name in doc:
        struct = StructDescription()
        struct.parseByYAML(name, doc[name])
        struct.dump()
        struct.generateDBDescriptor(0)
        struct.generateStruct(0)

for doc in yaml.load_all(ymlfile):
    print '-------------'
    # print data
    parse_ddl(doc)
