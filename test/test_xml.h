struct Config {
	bool parse(XMLParser& xml, XMLParser::NodePtr node);
	bool write(XMLWriter& xml, XMLWriter::NodePtr node);
	void reset();
	
	struct Master {
		bool parse(XMLParser& xml, XMLParser::NodePtr node);
		bool write(XMLWriter& xml, XMLWriter::NodePtr node);
		void reset();
		
		std::string& operator () () { return content__; }
		const std::string& operator () () const { return content__; }
		std::string content__;
		
	};
	Master master;
	
	struct Bind {
		bool parse(XMLParser& xml, XMLParser::NodePtr node);
		bool write(XMLWriter& xml, XMLWriter::NodePtr node);
		void reset();
		
		int& operator () () { return content__; }
		const int& operator () () const { return content__; }
		int content__;
		
	};
	Bind bind;
	
	struct App {
		bool parse(XMLParser& xml, XMLParser::NodePtr node);
		bool write(XMLWriter& xml, XMLWriter::NodePtr node);
		void reset();
		
		std::string& operator () () { return content__; }
		const std::string& operator () () const { return content__; }
		std::string content__;
		
		const std::string& name() { return name_; } 
		
		std::string name_;
	};
	typedef std::map<std::string, App> AppMap;
	typedef AppMap::iterator AppMapIter;
	typedef AppMap::const_iterator AppMapConstIter;
	AppMap app;
	
	struct List {
		bool parse(XMLParser& xml, XMLParser::NodePtr node);
		bool write(XMLWriter& xml, XMLWriter::NodePtr node);
		void reset();
		
		std::string& operator () () { return content__; }
		const std::string& operator () () const { return content__; }
		std::string content__;
		
		const int& prop1() { return prop1_; } 
		const std::string& prop2() { return prop2_; } 
		
		int prop1_;
		std::string prop2_;
	};
	typedef std::vector<List> ListCont;
	typedef ListCont::iterator ListContIter;
	typedef ListCont::const_iterator ListContConstIter;
	ListCont list;
	
	struct Onlyprop {
		bool parse(XMLParser& xml, XMLParser::NodePtr node);
		bool write(XMLWriter& xml, XMLWriter::NodePtr node);
		void reset();
		
		const int& prop1() { return prop1_; } 
		const float& prop2() { return prop2_; } 
		const std::string& prop3() { return prop3_; } 
		
		int prop1_;
		float prop2_;
		std::string prop3_;
		struct Subnode {
			bool parse(XMLParser& xml, XMLParser::NodePtr node);
			bool write(XMLWriter& xml, XMLWriter::NodePtr node);
			void reset();
			
			const int& p2() { return p2_; } 
			const int& p1() { return p1_; } 
			
			int p2_;
			int p1_;
		};
		Subnode subnode;
		
	};
	Onlyprop onlyprop;
	
	struct Parent {
		bool parse(XMLParser& xml, XMLParser::NodePtr node);
		bool write(XMLWriter& xml, XMLWriter::NodePtr node);
		void reset();
		
		const int& id() { return id_; } 
		
		int id_;
		struct MyVec {
			bool parse(XMLParser& xml, XMLParser::NodePtr node);
			bool write(XMLWriter& xml, XMLWriter::NodePtr node);
			void reset();
			
			const std::string& name() { return name_; } 
			
			std::string name_;
			struct Onlyone {
				bool parse(XMLParser& xml, XMLParser::NodePtr node);
				bool write(XMLWriter& xml, XMLWriter::NodePtr node);
				void reset();
				
				const int& p2() { return p2_; } 
				const std::string& p1() { return p1_; } 
				
				int p2_;
				std::string p1_;
			};
			typedef std::vector<Onlyone> OnlyoneCont;
			typedef OnlyoneCont::iterator OnlyoneContIter;
			typedef OnlyoneCont::const_iterator OnlyoneContConstIter;
			OnlyoneCont onlyone;
			
		};
		typedef std::list<MyVec> MyVecCont;
		typedef MyVecCont::iterator MyVecContIter;
		typedef MyVecCont::const_iterator MyVecContConstIter;
		MyVecCont myVec;
		
	};
	Parent parent;
	
};
inline bool Config::parse(XMLParser& xml, XMLParser::NodePtr node) {
	if (!node) return false;
	master.parse(xml, xml.getChildNode(node, "master"));
	bind.parse(xml, xml.getChildNode(node, "bind"));
	
	XMLParser::NodePtr App_node = xml.getChildNode(node, "App");
	while (App_node) {
		App item;
		if (item.parse(xml, App_node))
			app.insert(std::make_pair(item.name_, item));
		App_node = xml.getNextNode(App_node, "App");
	}
	
	XMLParser::NodePtr list_node = xml.getChildNode(node, "list");
	while (list_node) {
		List item;
		if (item.parse(xml, list_node))
			list.push_back(item);
		list_node = xml.getNextNode(list_node, "list");
	}
	onlyprop.parse(xml, xml.getChildNode(node, "onlyprop"));
	parent.parse(xml, xml.getChildNode(node, "parent"));
	return true;
}

inline bool Config::Master::parse(XMLParser& xml, XMLParser::NodePtr node) {
	if (!node) return false;
	content__ = xml.getNodeContent<std::string>(node);
	return true;
}

inline bool Config::Bind::parse(XMLParser& xml, XMLParser::NodePtr node) {
	if (!node) return false;
	content__ = xml.getNodeContent<int>(node);
	return true;
}

inline bool Config::App::parse(XMLParser& xml, XMLParser::NodePtr node) {
	if (!node) return false;
	content__ = xml.getNodeContent<std::string>(node);
	name_ = xml.getNodeProp<std::string>(node, "name");
	return true;
}

inline bool Config::List::parse(XMLParser& xml, XMLParser::NodePtr node) {
	if (!node) return false;
	content__ = xml.getNodeContent<std::string>(node);
	prop1_ = xml.getNodeProp<int>(node, "prop1");
	prop2_ = xml.getNodeProp<std::string>(node, "prop2");
	return true;
}

inline bool Config::Onlyprop::parse(XMLParser& xml, XMLParser::NodePtr node) {
	if (!node) return false;
	prop1_ = xml.getNodeProp<int>(node, "prop1");
	prop2_ = xml.getNodeProp<float>(node, "prop2");
	prop3_ = xml.getNodeProp<std::string>(node, "prop3");
	subnode.parse(xml, xml.getChildNode(node, "subnode"));
	return true;
}

inline bool Config::Onlyprop::Subnode::parse(XMLParser& xml, XMLParser::NodePtr node) {
	if (!node) return false;
	p2_ = xml.getNodeProp<int>(node, "p2");
	p1_ = xml.getNodeProp<int>(node, "p1");
	return true;
}

inline bool Config::Parent::parse(XMLParser& xml, XMLParser::NodePtr node) {
	if (!node) return false;
	id_ = xml.getNodeProp<int>(node, "id");
	
	XMLParser::NodePtr MyVec_node = xml.getChildNode(node, "MyVec");
	while (MyVec_node) {
		MyVec item;
		if (item.parse(xml, MyVec_node))
			myVec.push_back(item);
		MyVec_node = xml.getNextNode(MyVec_node, "MyVec");
	}
	return true;
}

inline bool Config::Parent::MyVec::parse(XMLParser& xml, XMLParser::NodePtr node) {
	if (!node) return false;
	name_ = xml.getNodeProp<std::string>(node, "name");
	
	XMLParser::NodePtr onlyone_node = xml.getChildNode(node, "onlyone");
	while (onlyone_node) {
		Onlyone item;
		if (item.parse(xml, onlyone_node))
			onlyone.push_back(item);
		onlyone_node = xml.getNextNode(onlyone_node, "onlyone");
	}
	return true;
}

inline bool Config::Parent::MyVec::Onlyone::parse(XMLParser& xml, XMLParser::NodePtr node) {
	if (!node) return false;
	p2_ = xml.getNodeProp<int>(node, "p2");
	p1_ = xml.getNodeProp<std::string>(node, "p1");
	return true;
}

inline bool Config::write(XMLWriter& xml, XMLWriter::NodePtr node) {
	if (!node) return false;
	
	XMLWriter::NodePtr master_node = xml.createChildNode(node, "master", tw_cast<std::string>(master.content__).c_str());
	if (master_node)
		master.write(xml, master_node);
	
	XMLWriter::NodePtr bind_node = xml.createChildNode(node, "bind", tw_cast<std::string>(bind.content__).c_str());
	if (bind_node)
		bind.write(xml, bind_node);
	
	for (AppMapIter it = app.begin(); it != app.end(); ++ it) {
		XMLWriter::NodePtr subnode = xml.createChildNode(node, "App", tw_cast<std::string>(it->second.content__).c_str());
		if (subnode)
			it->second.write(xml, subnode);
	}
	
	for (ListContIter it = list.begin(); it != list.end(); ++ it) {
		XMLWriter::NodePtr subnode = xml.createChildNode(node, "list", tw_cast<std::string>(it->content__).c_str());
		if (subnode)
			it->write(xml, subnode);
	}
	
	XMLWriter::NodePtr onlyprop_node = xml.createChildNode(node, "onlyprop", "");
	if (onlyprop_node)
		onlyprop.write(xml, onlyprop_node);
	
	XMLWriter::NodePtr parent_node = xml.createChildNode(node, "parent", "");
	if (parent_node)
		parent.write(xml, parent_node);
	return true;
}

inline bool Config::Master::write(XMLWriter& xml, XMLWriter::NodePtr node) {
	if (!node) return false;
	return true;
}

inline bool Config::Bind::write(XMLWriter& xml, XMLWriter::NodePtr node) {
	if (!node) return false;
	return true;
}

inline bool Config::App::write(XMLWriter& xml, XMLWriter::NodePtr node) {
	if (!node) return false;
	xml.createNodeProp(node, "name", name_);
	return true;
}

inline bool Config::List::write(XMLWriter& xml, XMLWriter::NodePtr node) {
	if (!node) return false;
	xml.createNodeProp(node, "prop1", prop1_);
	xml.createNodeProp(node, "prop2", prop2_);
	return true;
}

inline bool Config::Onlyprop::write(XMLWriter& xml, XMLWriter::NodePtr node) {
	if (!node) return false;
	xml.createNodeProp(node, "prop1", prop1_);
	xml.createNodeProp(node, "prop2", prop2_);
	xml.createNodeProp(node, "prop3", prop3_);
	
	XMLWriter::NodePtr subnode_node = xml.createChildNode(node, "subnode", "");
	if (subnode_node)
		subnode.write(xml, subnode_node);
	return true;
}

inline bool Config::Onlyprop::Subnode::write(XMLWriter& xml, XMLWriter::NodePtr node) {
	if (!node) return false;
	xml.createNodeProp(node, "p2", p2_);
	xml.createNodeProp(node, "p1", p1_);
	return true;
}

inline bool Config::Parent::write(XMLWriter& xml, XMLWriter::NodePtr node) {
	if (!node) return false;
	xml.createNodeProp(node, "id", id_);
	
	for (MyVecContIter it = myVec.begin(); it != myVec.end(); ++ it) {
		XMLWriter::NodePtr subnode = xml.createChildNode(node, "MyVec","");
		if (subnode)
			it->write(xml, subnode);
	}
	return true;
}

inline bool Config::Parent::MyVec::write(XMLWriter& xml, XMLWriter::NodePtr node) {
	if (!node) return false;
	xml.createNodeProp(node, "name", name_);
	
	for (OnlyoneContIter it = onlyone.begin(); it != onlyone.end(); ++ it) {
		XMLWriter::NodePtr subnode = xml.createChildNode(node, "onlyone","");
		if (subnode)
			it->write(xml, subnode);
	}
	return true;
}

inline bool Config::Parent::MyVec::Onlyone::write(XMLWriter& xml, XMLWriter::NodePtr node) {
	if (!node) return false;
	xml.createNodeProp(node, "p2", p2_);
	xml.createNodeProp(node, "p1", p1_);
	return true;
}

inline void Config::reset() {
	master.reset();
	bind.reset();
	app.clear();
	list.clear();
	onlyprop.reset();
	parent.reset();
}

inline void Config::Master::reset() {
	content__ = std::string();
}

inline void Config::Bind::reset() {
	content__ = int();
}

inline void Config::App::reset() {
	content__ = std::string();
	name_ = std::string();
}

inline void Config::List::reset() {
	content__ = std::string();
	prop1_ = int();
	prop2_ = std::string();
}

inline void Config::Onlyprop::reset() {
	prop1_ = int();
	prop2_ = float();
	prop3_ = std::string();
	subnode.reset();
}

inline void Config::Onlyprop::Subnode::reset() {
	p2_ = int();
	p1_ = int();
}

inline void Config::Parent::reset() {
	id_ = int();
	myVec.clear();
}

inline void Config::Parent::MyVec::reset() {
	name_ = std::string();
	onlyone.clear();
}

inline void Config::Parent::MyVec::Onlyone::reset() {
	p2_ = int();
	p1_ = std::string();
}

