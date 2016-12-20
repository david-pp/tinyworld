
///////////////////////////////////////////////////////////////////////////////

INLINE XMLParser::NodePtr XMLParser::getRootNode(const char* name /*= NULL*/)
{
	NodePtr cur = xmlDocGetRootElement(doc_);
	if (cur == NULL) {
		fprintf(stderr,"empty document\n");
		return NULL;
	}

	if (name && xmlStrcmp(cur->name, (const xmlChar *)name))
	{
		fprintf(stderr,"no root node:%s\n", name);
		return NULL;
	}

	return cur;
}

INLINE XMLParser::NodePtr XMLParser::getChildNode(NodePtr node, const char* name /*= NULL*/)
{
	return node ? checkNextNode(node->xmlChildrenNode, name) : NULL;
}

INLINE XMLParser::NodePtr XMLParser::getNextNode(NodePtr node, const char* name /*= NULL*/)
{
	return node ? checkNextNode(node->next, name) : NULL;
}

INLINE XMLParser::NodePtr XMLParser::checkNextNode(NodePtr cur, const char* name)
{
	if (name)
	{
		while(cur)
		{
			if (!xmlStrcmp(cur->name, (const xmlChar*)name)) break;
			cur = cur->next;
		}
	}
	else
	{
		while (cur)
		{
			if (!xmlNodeIsText(cur)) break;
			cur = cur->next;
		}
	}

	return cur;
}

INLINE XMLParser::NodePtrs XMLParser::getChildNodes(NodePtr node, const char* name /*= NULL*/)
{
	NodePtrs childs;
	if (node && node->xmlChildrenNode)
	{
		NodePtr cur = node->xmlChildrenNode;
		while (cur)
		{
			if (name)
			{
				if (!xmlStrcmp(cur->name, (const xmlChar*)name))
					childs.push_back(cur);
			}
			else
			{
				if (!xmlNodeIsText(cur))
					childs.push_back(cur);
			}
			cur = cur->next;
		}
	}

	return childs;
}

INLINE const std::string XMLParser::getNodePropStr(NodePtr node, const char* prop)
{
	std::string value;
	xmlChar* name = xmlGetProp(node, (const xmlChar*)prop);
	if (name)
	{
		value = (char*)name;
		xmlFree(name);
	}

	return value;
}

INLINE const std::string XMLParser::getNodeContentStr(NodePtr node)
{
	std::string value;
	xmlChar* content = xmlNodeGetContent(node);
	if (content)
	{
		value = (char*)content;
		xmlFree(content);
	}

	return value;
}

///////////////////////////////////////////////////////////////////////////////

INLINE XMLWriter::NodePtr XMLWriter::createRootNode(const char* name)
{
	if (doc_)
	{
		NodePtr root = xmlNewNode(NULL, (const xmlChar*)name);
		if (root)
		{
			xmlDocSetRootElement(doc_, root);
			return root;
		}
	}
	return NULL;
}

INLINE XMLWriter::NodePtr XMLWriter::createChildNode(const NodePtr parent, const char* name, const char* content)
{
	if (doc_ && parent)
	{
		return xmlNewChild(parent, NULL, (const xmlChar*)name, (const xmlChar*)content);
	}
	return NULL;
}

INLINE bool XMLWriter::createNodePropStr(const NodePtr node, const char* prop, const char* value)
{
	return node && xmlNewProp(node, (const xmlChar*)prop, (const xmlChar*)value);
}

INLINE XMLWriter::NodePtr XMLWriter::createComment(const NodePtr parent, const char* comment)
{
	if (parent)
	{
		NodePtr node = xmlNewComment((const xmlChar*)comment);
		if (node)
		{
			return xmlAddChild(parent, node);
		}
	}
	return NULL;
}
