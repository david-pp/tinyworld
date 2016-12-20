#!/bin/python
# -*- coding: utf-8 -*-

import yaml
import os
import sys
import getopt

def Usage() :
	print "usage:", sys.argv[0], "[-i --header --cpp -o file] data1.yaml data2.yaml ..."
	print "options: "
	print "  -h --help             help"
	print "  -o file --output=file output to the file"
	print "  -a --header           generate c++ struct"
	print "  -b --cpp              generate c++ functions"     
	print "  -i --inline           member function is inlined."

config = {
	"header" : 0,
	"cpp" : 0,
	"inline" : 0,
	"outputfile" : "",
	"files" : []
}


def init() :
	if len(sys.argv) == 1:
		Usage()
		sys.exit()

	try :
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
			config["header"] = 1
		elif o in ("-b", "--cpp"):
			config["cpp"] = 1
		elif o in ("-i", "--inline"):
			config["inline"] = 1
		elif o in ("-o", "--output"):
			config["outputfile"] = a

	config["files"] = args;
	#print config


def printline(file, text, indent = 0):
	"""print line with indent"""
	for i in range(0, indent):
		print >> file, '\t',
	print >> file, text

def generate_struct_header(structname, struct, file):
	printline(file, "struct %s {" % structname, 0)
	printline(file, "%s();" % structname, 1)
	printline(file, "bool parseFromKVMap(mycache::KVMap& kvs);", 1)
	printline(file, "bool saveToKVMap(mycache::KVMap& kvs) const;", 1)
	printline(file, "")
	for mem in struct:
		printline(file, mem["type"] + " " + mem["field"] + ";", 1)
	printline(file, "};", 0)

def generate_struct_inline(structname, struct, file):
	printline(file, "#ifndef _MYCACH_" + structname.upper())
	printline(file, "#define _MYCACH_" + structname.upper())
	printline(file, "struct %s {" % structname, 0)

    # member functions
	printline(file, "%s() {" % structname, 1)
	for mem in struct:
		printline(file, mem["field"] + " = " + mem["type"] + "();", 2)
	printline(file, "}", 1)

	printline(file, "bool parseFromKVMap(KVMap& kvs) {", 1)
	for mem in struct:
		if mem.has_key("flag") and mem["flag"] == "protobuf":
			printline(file, '%s.ParseFromString(kvs["%s"]);'%(mem["field"], mem["field"]), 2)
		elif mem.has_key("flag") and mem["flag"] == "json":
			printline(file, 'Json::Reader().parse(kvs["%s"], %s);'%(mem["field"], mem["field"]), 2)
		else:
			printline(file, '%s = boost::lexical_cast<%s>(kvs["%s"]);'%(mem["field"], mem["type"], mem["field"]), 2)
	printline(file, "return true;", 2)
	printline(file, "}", 1)

	printline(file, "bool saveToKVMap(KVMap& kvs) const {", 1)
	for mem in struct:
		if mem.has_key("flag") and mem["flag"] == "protobuf":
			printline(file, '%s.SerializeToString(kvs["%s"]);'%(mem["field"], mem["field"]), 2)
		elif mem.has_key("flag") and mem["flag"] == "json":
			printline(file, 'kvs["%s"] = Json::FastWriter().write(%s)'%(mem["field"], mem["field"]), 2)
		else:
			printline(file, 'kvs["%s"] = boost::lexical_cast<std::string>(%s);'%(mem["field"], mem["field"]), 2)
	printline(file, "return true;", 2)
	printline(file, "}", 1)


	# member vars
	printline(file, "")
	for mem in struct:
		printline(file, mem["type"] + " " + mem["field"] + ";", 1)

	printline(file, "};", 0)
	printline(file, "#endif // _MYCACH_" + structname.upper())


def generate_header(datayaml, file=sys.stdout) :
	for struct in datayaml :
		for structname in struct:
			generate_struct_header(structname, struct[structname], file)
			printline(file, "")
			break

def generate_cpp(datayaml, file=sys.stdout) :
	printline(file, "hello")

def generate_inline(datayaml, file=sys.stdout) :
	for struct in datayaml :
		for structname in struct:
			generate_struct_inline(structname, struct[structname], file)
			printline(file, "")
			break


def run() :
	outputfile = sys.stdout
	if (config["outputfile"]):
		outputfile = open(config["outputfile"], 'w+')

	for i in range(0, len(config["files"])):
		try :
			datayaml = yaml.load(file(config["files"][i]))
		except IOError, arg:
			print arg
			sys.exit()

		if config["header"]:
			generate_header(datayaml, outputfile)
		if config["cpp"]:
			generate_cpp(datayaml, outputfile)
		if config["inline"]:
			generate_inline(datayaml, outputfile)

init()
run();

