#ifndef __XML_PARSER_H__
#define __XML_PARSER_H__

#include <string.h>
#include "tinyxml2.h"

using namespace tinyxml2;
using std::string;

class XmlParser
{
    public:
        XmlParser(const char *xml_path);
		
        bool is_empty();
        XMLElement *get_key_node(const char *section, const char *key);
        int get_int(const char *section, const char *key, int &value);
        int get_ushort(const char *section, const char *key, unsigned short &value);
        int get_string(const char *section, const char *key, string &value);

    private:
        XMLDocument m_doc;
        XMLElement *m_root;
};









#endif

