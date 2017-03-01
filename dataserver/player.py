#!/usr/bin/python
# -*- coding: UTF-8 -*-

import yaml
import codecs
import sys
import os
import re
import getopt


# reload(sys)
# sys.setdefaultencoding('UTF-8')

#############################################
#
# 支持的基本类型
#
#############################################
class PrimitiveTypes(object):
    '支持的基本类型'
    # 支持的类型、C++类型、PROTO类型、MYSQL定义语句、默认值
    types = {

        # 整数-有符号
        "int8": ("int8", "sint32", "tinyint(3)  NOT NULL default '0'", '0'),
        "int16": ("int16", "sint32", "smallint(5) NOT NULL default '0'", '0'),
        "int32": ("int32", "sint32", "int(10)     NOT NULL default '0'", '0'),
        "int64": ("int64", "sint64", "bigint(20)  NOT NULL default '0'", '0'),

        # 整数-无符号
        "uint8": ("uint8", "uint32", "tinyint(3)  unsigned NOT NULL default '0'", '0'),
        "uint16": ("uint16", "uint32", "smallint(5) unsigned NOT NULL default '0'", '0'),
        "uint32": ("uint32", "uint32", "int(10)     unsigned NOT NULL default '0'", '0'),
        "uint64": ("uint64", "uint64", "bigint(20)  unsigned NOT NULL default '0'", '0'),

        # 浮点
        "float": ("float", "float", "float NOT NULL default '0'", '0'),
        "double": ("double", "double", "double NOT NULL default '0'", '0'),

        # 布尔
        "bool": ("bool", "bool", "bool NOT NULL default '0'", 'false'),

        # 字符串
        "string": ("std::string", "bytes", "text", ''),
        "char": ("char $$[#]", "bytes", "varchar(#) NOT NULL default ''", '\"\"'),

        # 二进制
        "bytes": ("std::string", "bytes", "blob", ''),
        "bytes8": ("std::string", "bytes", "tinyblob", ''),
        "bytes24": ("std::string", "bytes", "mediumblob", ''),
        "bytes32": ("std::string", "bytes", "longblob", ''),
    }

    def __init__(self):
        pass

    def isValid(self, type):
        charmatched = re.match(r'char(\d+)', type)
        if charmatched:
            return True
        return self.types.has_key(type)

    def getTypeDef(self, type):
        charmatched = re.match(r'char(\d+)', type)
        if charmatched:
            num = charmatched.group(1)
            return tuple(map(
                lambda x: x.replace('#', num),
                self.types['char']))

        if self.types.has_key(type):
            return self.types[type]
        else:
            return ()


primitive = PrimitiveTypes()


def test_primitive():
    print primitive.getTypeDef("int32")
    print primitive.getTypeDef("char[32]")


def printline(file, depth, text):
    """print line with indent"""
    for i in range(0, depth):
        print >> file, '\t',
    print >> file, text


#############################################
#
# 结构体字段
#
#############################################
class StructField:
    def __init__(self):
        # 字段名
        self.name = ''
        # 字段类型
        self.type = ''
        # 对应的C++类型
        self.type_cpp = ''
        # 对应的protobuf类型
        self.type_proto = ''
        # 对应的SQL类型
        self.type_sql = ''
        # 默认值
        self.default = ''
        # 注释
        self.comment = ''

    def __str__(self):
        return "%s,%s,%s" % (self.name, self.type, self.comment.encode('utf-8'))

    def parseFromYAML(self, yamlfield):
        # print yamlfield

        if 0 == len(yamlfield):
            return False

        self.name = yamlfield.keys()[0]
        attributes = yamlfield[self.name]

        if isinstance(attributes, str):
            self.type = attributes
        elif isinstance(attributes, dict):
            if attributes.has_key('$type'):
                self.type = attributes['$type']

            if attributes.has_key('$comment'):
                self.comment = attributes['$comment']

        self.type = self.type.strip()
        typedef = primitive.getTypeDef(self.type)
        if len(typedef) >= 4:
            self.type_cpp = typedef[0]
            self.type_proto = typedef[1]
            self.type_sql = typedef[2]
            self.default = typedef[3]
            return True
        else:
            print yamlfield, "has error!"

        return False

    def makeFieldInfo(self):
        info = {'name': '%s' % self.name.upper(), 'ddl': ''}
        typedef = primitive.getTypeDef(self.type)
        if len(typedef) >= 3:
            info['ddl'] = typedef[2]
        return info


#############################################
#
# 结构体描述信息
#
#############################################
class StructDescription:
    def __init__(self):
        # 结构名称
        self.name = ''
        # 结构字段
        self.fields = []
        # 主键
        self.primary_keys = []
        # 索引
        self.indexs = []
        # 注释
        self.comment = ''

    def dump(self):
        print 'Struct:%s' % self.name
        print '---'
        for f in self.fields:
            print " -", f
        for k in self.primary_keys:
            print " - $key:", k
        for k in self.indexs:
            print " - $index:", k

    def makeFieldList(self, fields):
        flist = ""
        for i in range(len(fields)):
            flist += '"%s"' % fields[i].name.upper()
            if i != len(fields) - 1:
                flist += ','
        return flist

    def parseByYAML(self, name, doc):
        self.name = name
        for yamlfield in doc:
            fieldname = yamlfield.keys()[0]
            # 保留关键字
            if fieldname[0] == '$':
                attributes = yamlfield[fieldname]
                if fieldname == '$key':
                    self.primary_keys.append(attributes)
                elif fieldname == '$index':
                    self.indexs.append(attributes)
                elif fieldname == '$comment':
                    self.comment = attributes
            # 正常字段
            else:
                field = StructField()
                if field.parseFromYAML(yamlfield):
                    self.fields.append(field)
                    # if field.iskey:
                    #     self.primary_keys.append(field)

    def generateStruct(self, depth, file=sys.stdout):
        printline(file, depth, '')
        if len(self.comment) > 0:
            printline(file, depth, '// %s' % self.comment)
        else:
            printline(file, depth, '')

        printline(file, depth, 'struct %s {' % self.name)
        for f in self.fields:
            if len(f.comment):
                printline(file, depth + 1, '/// %s' % f.comment.encode('utf-8'))

            if f.type_cpp.find('char') != -1:
                printline(file, depth + 1, '%s  = "";' % (f.type_cpp.replace('$$', f.name)))
                continue

            if len(f.default):
                printline(file, depth + 1, '%s %s = %s;' % (f.type_cpp, f.name, f.default))
            else:
                printline(file, depth + 1, '%s %s;' % (f.type_cpp, f.name))

        printline(file, depth, '};')

    def generateStructSet(self, depth, file=sys.stdout):
        printline(file, depth, '')
        # printline(file, depth, 'typedef boost::multi_index_container<')
        # printline(file, depth+2, '%s,' % self.name)
        # printline(file, depth+1, '> %sSet;' % self.name)

    def generateDBDescriptor(self, depth, file=sys.stdout):
        classname = self.name + "DBDescriptor"
        tablename = self.name.upper()

        printline(file, depth, 'class %s : public DBDescriptor {' % classname)
        printline(file, depth, 'public:')
        printline(file, depth + 1, 'typedef %s ObjectType;' % self.name)
        printline(file, depth + 1, '')

        # 构造函数
        printline(file, depth + 1, '%s() {' % classname)
        printline(file, depth + 2, 'table = "%s";' % tablename)
        printline(file, depth + 2, 'keys = {%s};' % self.makeFieldList(self.primary_keys))
        printline(file, depth + 2, 'fields = {')
        for f in self.fields:
            info = f.makeFieldInfo()
            printline(file, depth + 3, '{"%s", "%s"},' % (info['name'], info['ddl']))
        printline(file, depth + 2, '};')
        printline(file, depth + 1, '};')
        printline(file, depth + 1, '')

        # object2Query
        printline(file, depth + 1, 'void object2Query(mysqlpp::Query &query, const void* objptr) {')
        printline(file, depth + 2, 'const ObjectType& object = *(ObjectType*)objptr;')
        for i in range(len(self.fields)):
            if self.fields[i].type_proto == 'bytes':
                printline(file, depth + 2, 'query << mysqlpp::quote << object.%s;' % self.fields[i].name)
            else:
                if self.fields[i].type == 'int8' or self.fields[i].type == 'uint8':
                    printline(file, depth + 2, 'query << (int)object.%s;' % self.fields[i].name)
                else:
                    printline(file, depth + 2, 'query << object.%s;' % self.fields[i].name)
            if i != len(self.fields) - 1:
                printline(file, depth + 2, 'query << ",";')
        printline(file, depth + 1, '};')
        printline(file, depth + 1, '')

        # pair2Query
        printline(file, depth + 1, 'void pair2Query(mysqlpp::Query &query, const void* objptr) {')
        printline(file, depth + 2, 'const ObjectType& object = *(ObjectType*)objptr;')
        for i in range(len(self.fields)):
            if self.fields[i].type_proto == 'bytes':
                printline(file, depth + 2, 'query << "`%s`=" << mysqlpp::quote << object.%s;' % (
                self.fields[i].name.upper(), self.fields[i].name))
            else:
                if self.fields[i].type == 'int8' or self.fields[i].type == 'uint8':
                    printline(file, depth + 2, 'query << "`%s`=" << (int)object.%s;' % (
                    self.fields[i].name.upper(), self.fields[i].name))
                else:
                    printline(file, depth + 2,
                              'query << "`%s`=" << object.%s;' % (self.fields[i].name.upper(), self.fields[i].name))
            if i != len(self.fields) - 1:
                printline(file, depth + 2, 'query << ",";')
        printline(file, depth + 1, '};')
        printline(file, depth + 1, '')

        # key2Query
        printline(file, depth + 1, 'void key2Query(mysqlpp::Query &query, const void* objptr){')
        printline(file, depth + 2, 'const ObjectType& object = *(ObjectType*)objptr;')
        for i in range(len(self.primary_keys)):
            if self.primary_keys[i].type_proto == 'bytes':
                printline(file, depth + 2, 'query << "`%s`=" << mysqlpp::quote << object.%s;' % (
                self.primary_keys[i].name.upper(), self.primary_keys[i].name))
            else:
                if self.fields[i].type == 'int8' or self.fields[i].type == 'uint8':
                    printline(file, depth + 2, 'query << "`%s`=" << (int)object.%s;' % (
                    self.primary_keys[i].name.upper(), self.primary_keys[i].name))
                else:
                    printline(file, depth + 2, 'query << "`%s`=" << object.%s;' % (
                    self.primary_keys[i].name.upper(), self.primary_keys[i].name))
            if i != len(self.primary_keys) - 1:
                printline(file, depth + 2, 'query << ",";')
        printline(file, depth + 1, '};')
        printline(file, depth + 1, '')

        # record2Object
        printline(file, depth + 1, 'bool record2Object(mysqlpp::Row& record, void* objptr){')
        printline(file, depth + 2, 'if (record.size() != fields.size()) return false;');
        printline(file, depth + 2, 'ObjectType& object = *(ObjectType*)objptr;')
        for i in range(len(self.fields)):
            f = self.fields[i]
            if f.type.find('char') == 0:
                printline(file, depth + 2,
                          'strncpy(object.%s, record[%u].data(), sizeof(object.%s)-1);' % (f.name, i, f.name))
            elif f.type_cpp == 'std::string':
                printline(file, depth + 2, 'object.%s.assign(record[%u].data(), record[%u].size());' % (f.name, i, i))
            else:
                printline(file, depth + 2, 'object.%s = record[%u];' % (self.fields[i].name, i))
        printline(file, depth + 1, '};')
        printline(file, depth + 1, '')

        printline(file, depth, '};')

    def generateProtobuf(self, depth, file=sys.stdout):
        printline(file, depth, 'package bin;')
        printline(file, depth, '')
        printline(file, depth, 'message %s {' % self.name)
        for i in range(len(self.fields)):
            f = self.fields[i]
            printline(file, depth + 1, 'optional %s %s = %d;' % (f.type_proto, f.name, i + 1))
        printline(file, depth, '}')
        printline(file, depth, '')

    def generateBinDescriptor(self, depth, file=sys.stdout):
        classname = self.name + "BinDescriptor"

        printline(file, depth, 'struct %s {' % classname)
        printline(file, depth + 1, "typedef %s ObjectType;" % self.name)
        printline(file, depth + 1, "typedef bin::%s ProtoType;" % self.name)
        printline(file, depth + 1, "")

        printline(file, depth + 1, "static void obj2proto(ProtoType& proto, const ObjectType& object) {")
        for f in self.fields:
            printline(file, depth + 2, "proto.set_%s(object.%s);" % (f.name, f.name))
        printline(file, depth + 1, "}")
        printline(file, depth + 1, "")

        printline(file, depth + 1, "static void proto2obj(ObjectType& object, const ProtoType& proto) {")
        for f in self.fields:
            if f.type.find('char') == 0:
                printline(file, depth + 2,
                          "strncpy(object.%s, proto.%s().c_str(), sizeof(object.%s)-1);" % (f.name, f.name, f.name))
            else:
                printline(file, depth + 2, "object.%s = proto.%s();" % (f.name, f.name))
        printline(file, depth + 1, "}")
        printline(file, depth, '};')


# ymlfile = file('player.yml', 'r')
#
# # player = yaml.load(ymlfile)
# # print player
#
# for doc in yaml.load_all(ymlfile):
#     print '-------------'
#     # print data
#     parse_ddl(doc)


#######################################################
#
# Main
#
#######################################################

class TinyCacheCompiler(object):
    def __init__(self):
        self.dir_input = ''
        self.dir_struct = ''
        self.dir_db = ''
        self.dir_proto = ''
        self.schemas = []

    def schemaFileName(self, schema):
        return os.path.splitext(os.path.basename(schema))[0]

    def outputFileName(self, schema, path, ext):
        return os.path.join(path, self.schemaFileName(schema) + '.' + ext)

    def run(self):
        print self.dir_input, self.dir_struct, self.dir_db, self.dir_proto, self.schemas

        for schema in self.schemas:
            self.generate(os.path.join(self.dir_input, schema))

    def generate(self, filename):
        ymlfile = file(filename, 'r')
        for doc in yaml.load_all(ymlfile):
            self.parse_ddl(filename, doc)

        print self.schemaFileName(filename)
        print self.outputFileName(filename, self.dir_struct, 'h')
        print self.outputFileName(filename, self.dir_db, 'db.h')
        print self.outputFileName(filename, self.dir_proto, 'proto')

    def parse_ddl(self, filename, doc):
        if (self.dir_struct):
            outfile = open(self.outputFileName(filename, self.dir_struct, 'h'), 'w+')
            print >> outfile, '//'
            print >> outfile, '// !!ATTENTION: generated by tools, do not edit it manually!'
            print >> outfile, '//'
            outfile.close()

        for name in doc:
            struct = StructDescription()
            struct.parseByYAML(name, doc[name])
            struct.dump()

            if (self.dir_struct):
                outfile = open(self.outputFileName(filename, self.dir_struct, 'h'), 'a+')
                struct.generateStruct(0, outfile)
                struct.generateStructSet(0, outfile)

            continue

            if (self.dir_db):
                outfile = open(self.outputFileName(filename, self.dir_db, 'db.h'), 'w+')
                struct.generateDBDescriptor(0, outfile)

            if (self.dir_proto):
                # 生成.proto文件
                protofilename = self.outputFileName(filename, self.dir_proto, 'proto')
                outfile = open(protofilename, 'w+')
                struct.generateProtobuf(0, outfile)
                outfile.close()

                # 生成proto对应的c++文件
                print os.popen('/usr/local/bin/protoc -I%s --cpp_out=%s %s' % (
                self.dir_proto, self.dir_proto, protofilename)).read()

                # 生成BinDescriptor文件
                outfile = open(self.outputFileName(filename, self.dir_db, 'bin.h'), 'w+')
                struct.generateBinDescriptor(0, outfile)
                outfile.close()


#######################################################
#
# Main
#
#######################################################

tcc = TinyCacheCompiler()


def Usage():
    print "usage:", sys.argv[0], "[options] schema1.yml schema2.yml ..."
    print "options: "
    print "  -h --help             help"
    print ""
    print "  -I PATH --input=PATH  .yml dir"
    print "  -O PATH --output=PATH generated code dir(include all below)"
    print ""
    print "  --struct_out=DIR      generate c++ struct"
    print "  --db_out=DIR          generate DB descriptor"
    print "  --proto_out=DIR       generate protobuf proto & c++ files"


if len(sys.argv) == 1:
    Usage()
    sys.exit()

try:
    opts, args = getopt.getopt(sys.argv[1:], 'hI:O:s:d:p:',
                               ['help', 'input=', 'output=', 'struct_out=', 'db_out=', 'proto_out='])
except getopt.GetoptError:
    Usage()
    sys.exit()

for o, a in opts:
    if o in ("-h", "--help"):
        Usage()
        sys.exit()
    elif o in ("-I", "--input"):
        tcc.dir_input = a
    elif o in ("-O", "--output"):
        tcc.dir_struct = a
        tcc.dir_db = a
        tcc.dir_proto = a
    elif o in ("-s", "--struct_out"):
        tcc.dir_struct = a
    elif o in ("-d", "--db_out"):
        tcc.dir_db = a
    elif o in ("-p", "--proto_out"):
        tcc.dir_proto = a

tcc.schemas = args
if len(tcc.schemas) > 0:
    tcc.run()
else:
    Usage()
