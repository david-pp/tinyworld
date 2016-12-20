
#include <iostream>
#include <map>
#include <vector>
#include <list>
#include <stdio.h>

#include "xmlparser.h"

struct IConfig
{
public:
	virtual bool load(const char* filename) = 0;
	virtual bool save(const char* filename) = 0;
	virtual std::string dump() = 0;
};

template <typename Config>
struct xml_config : public Config, public IConfig
{
public:
	virtual bool load(const char* filename)
	{
		XMLParser xml;
		if (!xml.parseFile(filename)) return false;
		
		XMLParser::NodePtr root = xml.getRootNode();
		if (!root) return false;

		this->reset();
		return this->parse(xml, root);
	}

	virtual bool save(const char* filename)
	{
		XMLWriter xml;
		XMLWriter::NodePtr root = xml.createRootNode("config");
		if (root && this->write(xml, root))
		{
			xml.save(filename);
		}
		return false;
	}

	virtual std::string dump()
	{
		std::string result;

		XMLWriter xml;
		XMLWriter::NodePtr root = xml.createRootNode("config");
		if (root && this->write(xml, root))
		{
			xml.dump(result);
		}

		return result;
	}

public:
	template <typename T>
	std::string dumpnode(T& data, const char* nodename) 
	{
		std::string result;

		XMLWriter xml;
		XMLWriter::NodePtr root = xml.createRootNode(nodename);
		if (root && data.write(xml, root))
		{
			xml.dump(result);
		}

		return result;
	}
};

#if 0
struct Config : public IConfig
{
public:
	std::string master;
	int bind;

	struct AppItem
	{
		std::string name;
		std::string& operator () () { return content_; }
		const std::string& operator () () const { return content_; }
	private:
		std::string content_;
	};

	typedef std::map<std::string, AppItem> AppMap;
	typedef AppMap::iterator AppMapIter;
	typedef AppMap::const_iterator AppMapConstIter;
	AppMap apps;

	struct ListItem
	{
		int prop1;
		std::string prop2;

		std::string& operator () () { return content_; }
		const std::string& operator () () const { return content_; }
	private:
		std::string content_;
	};
	std::vector<ListItem> lists;
public:
	bool load(const char* filename);
	
	XMLWriter write();

	bool save(const char* filename)
	{
		return write().save(filename);
	}

	std::string dump()
	{
		std::string text;
		write().dump(text);
		return text;
	}
};


bool Config::load(const char* filename)
{
	XMLParser xml;
	if (!xml.parseFile(filename)) return false;
	
	XMLParser::NodePtr root = xml.getRootNode();
	if (!root) return false;
	
	XMLParser::NodePtr master_node = xml.getChildNode(root, "master");
	if (master_node)
	{
		master = xml.getNodeContent<std::string>(master_node);
	}

	XMLParser::NodePtr app_node = xml.getChildNode(root, "app");
	while (app_node)
	{
		AppItem item;
		item.name = xml.getNodeProp<std::string>(app_node, "name");
		item() = xml.getNodeContent<std::string>(app_node);
		apps.insert(std::make_pair(item.name, item));
	
		app_node = xml.getNextNode(app_node, "app");
	}

	XMLParser::NodePtrs listnodes = xml.getChildNodes(root, "list");
	for (size_t i = 0; i < listnodes.size(); i++)
	{
		ListItem item;
		item.prop1 = xml.getNodeProp<int>(listnodes[i], "prop1");
		item.prop2 = xml.getNodeProp<std::string>(listnodes[i], "prop2");
		item() = xml.getNodeContent<std::string>(listnodes[i]);
		lists.push_back(item);
	}
	
	std::string text;
	xml.dump(text);
	std::cout << text << std::endl;

	return true;
}

XMLWriter Config::write()
{
	XMLWriter xml;
	XMLWriter::NodePtr root = xml.createRootNode("config");

	xml.createComment(root, "master");
	xml.createChildNode(root, "master", master.c_str());

	xml.createComment(root, "apps");
	for (AppMapIter it = apps.begin(); it != apps.end(); ++ it)
	{
		XMLWriter::NodePtr app_node = xml.createChildNode(root, "app", it->second().c_str());
		if (app_node)
		{
			xml.createNodeProp(app_node, "name", it->second.name);
		}
	} 

	xml.createComment(root, "list");
	for (size_t i = 0; i < lists.size(); i++)
	{
		XMLWriter::NodePtr list_node = xml.createChildNode(root, "list",  lists[i]().c_str());
		if (list_node)
		{
			xml.createNodeProp(list_node, "prop1", lists[i].prop1);
			xml.createNodeProp(list_node, "prop2", lists[i].prop2);
		}
	}

	return xml;
}

#else
#include "test_xml.h"
#endif


xml_config<Config> cfg;

int main(int argc, const char* argv[])
{
	cfg.load("test_xml.xml");
	cfg.load("test_xml.xml");
	std::cout << cfg.dump() << std::endl;
	cfg.save("test_xml_out.xml");

	std::cout << cfg.master() << std::endl;
	std::cout << cfg.onlyprop.prop1() << std::endl;
	std::cout << cfg.onlyprop.subnode.p2() << std::endl;

	for (Config::Parent::MyVecContIter it = cfg.parent.myVec.begin(); it != cfg.parent.myVec.end(); ++ it)
	{
		std::cout << it->name() << std::endl;

		for (size_t i = 0; i < it->onlyone.size(); i++)
		{
			std::cout << it->onlyone[i].p1() << " - " << it->onlyone[i].p2() << std::endl;
		}
	}

	std::cout << cfg.dumpnode(cfg.parent, "parent") << std::endl;

	cfg.reset();
	std::cout << cfg.dump() << std::endl;
	
	return 0;
}


