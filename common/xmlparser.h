#ifndef _COMMON_PARSER_H
#define _COMMON_PARSER_H

#include <vector>
#include <libxml/parser.h>
#include "tw_cast.h"

class XMLDoc
{
public:
	XMLDoc(xmlDocPtr doc = NULL) : doc_(doc) {}

	~XMLDoc()
	{
		if (doc_)
		{
			xmlFreeDoc(doc_);
			doc_ = NULL;
		}
	}

	bool parseFromFile(const std::string& filename);
	bool saveToFile(const std::string& filename, const std::string& encoding = "utf-8", bool indent = 1);
	bool dump(std::string& text, const std::string& encoding = "utf-8", bool indent = 1);

	bool createDoc();

	operator xmlDocPtr () const { return doc_; }

private:
	xmlDocPtr doc_;
};

class XMLParser
{
public:
	typedef  xmlNodePtr NodePtr;
	typedef  std::vector<xmlNodePtr> NodePtrs; 

	XMLParser() {}
	~XMLParser() {}

	bool parseFile(const std::string& filename) { return doc_.parseFromFile(filename); }

	NodePtr getRootNode(const char* name = NULL);
	NodePtr getChildNode(NodePtr node, const char* name = NULL);
	NodePtr getNextNode(NodePtr node, const char* name = NULL);
	NodePtrs getChildNodes(NodePtr node, const char* name = NULL);

	const std::string getNodePropStr(NodePtr node, const char* prop);
	const std::string getNodeContentStr(NodePtr node);

	template <typename T>
	const T getNodeProp(NodePtr node, const char* prop)
	{
		return tw_cast<T>(getNodePropStr(node, prop).c_str());
	}

	template <typename T>
	const T getNodeContent(NodePtr node)
	{
		return tw_cast<T>(getNodeContentStr(node).c_str());
	}

	bool save(const std::string& filename, const std::string& encoding = "utf-8", bool indent = 1)
	{
		return doc_.saveToFile(filename, encoding, indent);
	}

	bool dump(std::string& text, const std::string& encoding = "utf-8", bool indent = 1)
	{
		return doc_.dump(text, encoding, indent);
	}

private:
	NodePtr checkNextNode(NodePtr node, const char* name);

	XMLDoc doc_;
};

struct XMLWriter
{
public:
	typedef  xmlNodePtr NodePtr;
	typedef  std::vector<xmlNodePtr> NodePtrs; 

	XMLWriter()
	{
		doc_.createDoc();
	}

	~XMLWriter() {}

	NodePtr createRootNode(const char* name);
	NodePtr createChildNode(const NodePtr parent, const char* name, const char* content);
	bool createNodePropStr(const NodePtr node, const char* prop, const char* value);
	NodePtr createComment(const NodePtr parent, const char* comment);
	
	template <typename T>
	bool createNodeProp(const NodePtr node, const char* prop, const T& value)
	{
		return createNodePropStr(node, prop, tw_cast<std::string>(value).c_str());
	}

	bool save(const std::string& filename, const std::string& encoding = "utf-8", bool indent = 1)
	{
		return doc_.saveToFile(filename, encoding, indent);
	}

	bool dump(std::string& text, const std::string& encoding = "utf-8", bool indent = 1)
	{
		return doc_.dump(text, encoding, indent);
	}

private:
	XMLDoc doc_;
};

#define INLINE inline
#include "xmlparser.in.h"
#undef INLINE

#endif // _COMMON_PARSER_H