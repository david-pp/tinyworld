#include "xmlparser.h"

bool XMLDoc::createDoc()
{
	doc_ = xmlNewDoc((const xmlChar*)"1.0");
	return doc_ ? true : false;
}

bool XMLDoc::parseFromFile(const std::string& filename)
{
	doc_ = xmlParseFile(filename.c_str());
	return doc_;
}

bool XMLDoc::saveToFile(const std::string& filename, const std::string& encoding /*= "utf-8"*/, bool indent /*= 1*/)
{
	return doc_ && xmlSaveFormatFileEnc(filename.c_str(), doc_, encoding.c_str(), indent) > 0;
}

bool XMLDoc::dump(std::string& text, const std::string& encoding /*= "utf-8"*/, bool indent /*= 1*/)
{
	if (doc_)
	{
		xmlChar* out = NULL;
		int size = 0;
		xmlDocDumpFormatMemoryEnc(doc_, &out, &size, encoding.c_str(), indent);
		if (out)
		{
			text = (char*)out;
			xmlFree(out);
			return true;
		}
	}
	return false;
}
