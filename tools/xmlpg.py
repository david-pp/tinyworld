#!/bin/python
# -*- coding: utf-8 -*-
#######################################################
#
# xmlparser generator - by David++ (2014/08/15)
# 
# usage:
# xmlpg.py --help
#
#######################################################

import sys
import os
import getopt
import xml.etree.ElementTree as ET


#######################################################
#
# Helper Functions 
#
#######################################################

def printline(file, depth, text):
    """print line with indent"""
    for i in range(0, depth):
        print >> file, '\t',
    print >> file, text


def NodeAsContainer(node):
    container = ''
    key = ''
    for attr in node.attrib:
        if attr in ('container_', 'cont_'):
            container = node.attrib[attr]
        elif attr in ('key_'):
            key = node.attrib[attr]
    return container, key


def Node2Struct(node):
    return node.tag[0:1].upper() + node.tag[1:]


def Node2Member(node):
    return node.tag[0:1].lower() + node.tag[1:]


def NodeHasContent(node):
    return node.text and node.text.strip()


def CppType(type):
    if (type == 'string'):
        return 'std::' + type
    return type


def IsKeyword(word):
    return word in ('container_', 'cont_', 'key_')


def AttrVarName(name):
    return name + '_';


def IsSeqCont(cont):
    return cont in ('vector', 'list', 'dequeue')


def IsMapCont(cont):
    return cont in ('map', 'multimap')


#######################################################
#
# Generate the c++ header file with config struct.
#
#######################################################
class HeaderGenerator(object):
    """docstring for HeaderGenerator """

    def __init__(self, arg):
        super(HeaderGenerator, self).__init__()
        self.xmlpg = arg

    def gen(self, tree, file=sys.stdout):
        self.genHeader(tree.getroot(), 0, file)

    def genHeader(self, node, depth, file=sys.stdout):
        # print node.tag, node.attrib, node.text
        container, key = NodeAsContainer(node)

        printline(file, depth, 'struct %s {' % Node2Struct(node))

        # memfuctions ...
        printline(file, depth + 1, 'bool parse(XMLParser& xml, XMLParser::NodePtr node);')
        printline(file, depth + 1, 'bool write(XMLWriter& xml, XMLWriter::NodePtr node);')
        printline(file, depth + 1, 'void reset();')
        printline(file, depth + 1, '')

        # content
        if NodeHasContent(node):
            printline(file, depth + 1, '%s& operator () () { return content__; }' % CppType(node.text))
            printline(file, depth + 1, 'const %s& operator () () const { return content__; }' % CppType(node.text))
            printline(file, depth + 1, '%s content__;' % CppType(node.text))
            printline(file, depth + 1, '')

        # accessor
        for attr in node.attrib:
            if not IsKeyword(attr):
                printline(file, depth + 1, 'const %s& %s() { return %s; } ' %
                          (CppType(node.attrib[attr]), attr, AttrVarName(attr)))

        if len(node.attrib) > 0:
            printline(file, depth + 1, '')

        # members
        for attr in node.attrib:
            if not IsKeyword(attr):
                printline(file, depth + 1, '%s %s;' % (CppType(node.attrib[attr]), AttrVarName(attr)))

        for child in node:
            self.genHeader(child, depth + 1, file)

        printline(file, depth, '};')

        if not depth == 0:
            if len(container) > 0:
                if IsMapCont(container):
                    if not node.attrib.has_key(key):
                        print >> sys.stderr, 'node:%s has no attrib:%s' % (node.tag, key)
                        return 0
                    printline(file, depth, 'typedef std::%s<%s, %s> %sMap;' % (
                    container, CppType(node.attrib[key]), Node2Struct(node), Node2Struct(node)))
                    printline(file, depth,
                              'typedef %sMap::iterator %sMapIter;' % (Node2Struct(node), Node2Struct(node)))
                    printline(file, depth,
                              'typedef %sMap::const_iterator %sMapConstIter;' % (Node2Struct(node), Node2Struct(node)))
                    printline(file, depth, '%sMap %s;' % (Node2Struct(node), Node2Member(node)))

                elif IsSeqCont(container):
                    printline(file, depth,
                              'typedef std::%s<%s> %sCont;' % (container, Node2Struct(node), Node2Struct(node)))
                    printline(file, depth,
                              'typedef %sCont::iterator %sContIter;' % (Node2Struct(node), Node2Struct(node)))
                    printline(file, depth, 'typedef %sCont::const_iterator %sContConstIter;' % (
                    Node2Struct(node), Node2Struct(node)))
                    printline(file, depth, '%sCont %s;' % (Node2Struct(node), Node2Member(node)))
                else:
                    print >> sys.stderr, 'node:%s container is invalid:%s' % (node.tag, container)
                    return 0
            else:
                printline(file, depth, '%s %s;' % (Node2Struct(node), Node2Member(node)))

            printline(file, depth, '')


#######################################################
#
# Generate the c++ cpp file with implementation.
#
#######################################################
class CppGenerator(object):
    """docstring for CppGenerator """

    def __init__(self, arg):
        super(CppGenerator, self).__init__()
        self.xmlpg = arg

    def inline(self):
        if self.xmlpg.inline:
            return "inline "
        else:
            return ""

    def gen(self, tree, file=sys.stdout):
        self.gen_parse(Node2Struct(tree.getroot()), tree.getroot(), file)
        self.gen_writer(Node2Struct(tree.getroot()), tree.getroot(), file)
        self.gen_reset(Node2Struct(tree.getroot()), tree.getroot(), file)

    def gen_parse(self, prefix, node, file=sys.stdout):
        printline(file, 0, '%sbool %s::parse(XMLParser& xml, XMLParser::NodePtr node) {' % (self.inline(), prefix))
        printline(file, 1, 'if (!node) return false;')

        if NodeHasContent(node):
            printline(file, 1, 'content__ = xml.getNodeContent<%s>(node);' % (CppType(node.text)))

        for attr in node.attrib:
            if not IsKeyword(attr):
                printline(file, 1, '%s = xml.getNodeProp<%s>(node, "%s");' % (
                AttrVarName(attr), CppType(node.attrib[attr]), attr))

        for child in node:
            container, key = NodeAsContainer(child)
            if len(container) > 0:
                printline(file, 1, '')
                if IsMapCont(container):
                    printline(file, 1,
                              'XMLParser::NodePtr %s_node = xml.getChildNode(node, "%s");' % (child.tag, child.tag))
                    printline(file, 1, 'while (%s_node) {' % child.tag)
                    printline(file, 2, '%s item;' % Node2Struct(child))
                    printline(file, 2, 'if (item.parse(xml, %s_node))' % child.tag)
                    printline(file, 3,
                              '%s.insert(std::make_pair(item.%s, item));' % (Node2Member(child), AttrVarName(key)))
                    printline(file, 2, '%s_node = xml.getNextNode(%s_node, "%s");' % (child.tag, child.tag, child.tag))
                    printline(file, 1, '}')
                elif IsSeqCont(container):
                    printline(file, 1,
                              'XMLParser::NodePtr %s_node = xml.getChildNode(node, "%s");' % (child.tag, child.tag))
                    printline(file, 1, 'while (%s_node) {' % child.tag)
                    printline(file, 2, '%s item;' % Node2Struct(child))
                    printline(file, 2, 'if (item.parse(xml, %s_node))' % child.tag)
                    printline(file, 3, '%s.push_back(item);' % Node2Member(child))
                    printline(file, 2, '%s_node = xml.getNextNode(%s_node, "%s");' % (child.tag, child.tag, child.tag))
                    printline(file, 1, '}')
                else:
                    return 0
            else:
                printline(file, 1, '%s.parse(xml, xml.getChildNode(node, "%s"));' % (Node2Member(child), child.tag))

        printline(file, 1, 'return true;')
        printline(file, 0, '}')
        printline(file, 0, '')

        for child in node:
            self.gen_parse('%s::%s' % (prefix, Node2Struct(child)), child, file)

    def gen_writer(self, prefix, node, file=sys.stdout):
        printline(file, 0, '%sbool %s::write(XMLWriter& xml, XMLWriter::NodePtr node) {' % (self.inline(), prefix))
        printline(file, 1, 'if (!node) return false;')

        for attr in node.attrib:
            if not IsKeyword(attr):
                printline(file, 1, 'xml.createNodeProp(node, "%s", %s);' % (attr, AttrVarName(attr)))

        for child in node:
            container, key = NodeAsContainer(child)
            printline(file, 1, '')
            if len(container) > 0:
                if IsMapCont(container):
                    printline(file, 1, 'for (%sMapIter it = %s.begin(); it != %s.end(); ++ it) {' %
                              (Node2Struct(child), Node2Member(child), Node2Member(child)))
                    if NodeHasContent(child):
                        printline(file, 2,
                                  'XMLWriter::NodePtr subnode = xml.createChildNode(node, "%s", tw_cast<std::string>(it->second.content__).c_str());' % child.tag)
                    else:
                        printline(file, 2,
                                  'XMLWriter::NodePtr subnode = xml.createChildNode(node, "%s","");' % child.tag)

                    printline(file, 2, 'if (subnode)')
                    printline(file, 3, 'it->second.write(xml, subnode);')
                    printline(file, 1, '}')

                elif IsSeqCont(container):
                    printline(file, 1, 'for (%sContIter it = %s.begin(); it != %s.end(); ++ it) {' %
                              (Node2Struct(child), Node2Member(child), Node2Member(child)))
                    if NodeHasContent(child):
                        printline(file, 2,
                                  'XMLWriter::NodePtr subnode = xml.createChildNode(node, "%s", tw_cast<std::string>(it->content__).c_str());' % child.tag)
                    else:
                        printline(file, 2,
                                  'XMLWriter::NodePtr subnode = xml.createChildNode(node, "%s","");' % child.tag)

                    printline(file, 2, 'if (subnode)')
                    printline(file, 3, 'it->write(xml, subnode);')
                    printline(file, 1, '}')

                else:
                    return 0
            else:
                if NodeHasContent(child):
                    printline(file, 1,
                              'XMLWriter::NodePtr %s_node = xml.createChildNode(node, "%s", tw_cast<std::string>(%s.content__).c_str());' % (
                              child.tag, child.tag, Node2Member(child)))
                else:
                    printline(file, 1, 'XMLWriter::NodePtr %s_node = xml.createChildNode(node, "%s", "");' % (
                    child.tag, child.tag))
                printline(file, 1, 'if (%s_node)' % child.tag)
                printline(file, 2, '%s.write(xml, %s_node);' % (Node2Member(child), child.tag))

        printline(file, 1, 'return true;')
        printline(file, 0, '}')
        printline(file, 0, '')

        for child in node:
            self.gen_writer('%s::%s' % (prefix, Node2Struct(child)), child, file)

    def gen_reset(self, prefix, node, file=sys.stdout):
        printline(file, 0, '%svoid %s::reset() {' % (self.inline(), prefix))

        if NodeHasContent(node):
            printline(file, 1, 'content__ = %s();' % (CppType(node.text)))

        for attr in node.attrib:
            if not IsKeyword(attr):
                printline(file, 1, '%s = %s();' % (AttrVarName(attr), CppType(node.attrib[attr])))

        for child in node:
            container, key = NodeAsContainer(child)
            if len(container) > 0:
                if IsMapCont(container):
                    printline(file, 1, '%s.clear();' % (Node2Member(child)))
                elif IsSeqCont(container):
                    printline(file, 1, '%s.clear();' % (Node2Member(child)))
                else:
                    return 0
            else:
                printline(file, 1, '%s.reset();' % (Node2Member(child)))

        printline(file, 0, '}')
        printline(file, 0, '')

        for child in node:
            self.gen_reset('%s::%s' % (prefix, Node2Struct(child)), child, file)


#######################################################
#
# Generator
#
#######################################################
class XMLParserGenerator(object):
    """docstring for XMLParserGenerator """

    def __init__(self):
        super(XMLParserGenerator, self).__init__()
        self.header = 0
        self.cpp = 0
        self.inline = 0
        self.outputfile = ""
        self.schemas = []

    def schemaFileName(self, schema):
        return os.path.splitext(os.path.basename(schema))[0]

    def outputFileName(self, schema, ext):
        return os.path.join("path...", self.schemaFileName(schema) + '.' + ext)

    def run(self):
        # print self.headerfile, self.cppfile, self.inline, self.schemas
        for i in range(0, len(self.schemas)):
            self.generate(self.schemas[i])

    def generate(self, filename):
        try:
            tree = ET.ElementTree(file=filename)

            file = sys.stdout
            if (self.outputfile):
                file = open(self.outputfile, 'w+')

            hg = HeaderGenerator(self)
            cg = CppGenerator(self)

            if self.header:
                hg.gen(tree, file)

            if self.cpp:
                cg.gen(tree, file)

        except IOError, arg:
            print arg


#######################################################
#
# Main
#
#######################################################
xmlpg = XMLParserGenerator()


def Usage():
    print "usage:", sys.argv[0], "[-i --header --cpp -o file] schema1.xml schema2.xml ..."
    print "options: "
    print "  -h --help             help"
    print "  -o file --output=file output to the file"
    print "  -a --header           generate c++ struct"
    print "  -b --cpp              generate c++ functions"
    print "  -i --inline           member function is inlined."


if len(sys.argv) == 1:
    Usage()
    sys.exit()

try:
    opts, args = getopt.getopt(sys.argv[1:], 'hiabo:',
                               ['help', 'inline', 'header', 'cpp', 'output='])
except getopt.GetoptError:
    Usage()
    sys.exit()

for o, a in opts:
    if o in ("-h", "--help"):
        Usage()
        sys.exit()
    elif o in ("-a", "--header"):
        xmlpg.header = 1
    elif o in ("-b", "--cpp"):
        xmlpg.cpp = 1
    elif o in ("-i", "--inline"):
        xmlpg.inline = 1
    elif o in ("-o", "--output"):
        xmlpg.outputfile = a

xmlpg.schemas = args
if len(xmlpg.schemas) > 0:
    xmlpg.run()
else:
    Usage()
