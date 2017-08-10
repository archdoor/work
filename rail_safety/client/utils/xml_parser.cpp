#include <iostream>
#include "xml_parser.h"


XmlParser::XmlParser(const char *xml_path) : m_root(NULL)
{
    if ( m_doc.LoadFile( xml_path ) == 0 )
    {
        m_root = m_doc.RootElement();
    }
}

bool XmlParser::is_empty()
{
    return ( m_root == NULL ? true : false );
}

//获取配置节点
XMLElement * XmlParser::get_key_node(const char *section, const char *key)
{
    XMLElement *section_node = m_root->FirstChildElement(section);
    if ( section_node == NULL )
    {
        return NULL;
    }

    XMLElement *key_node = section_node->FirstChildElement(key);
    if ( key_node == NULL )
    {
        return NULL;
    }

    return key_node;
}

//获取int节点值
int XmlParser::get_int(const char *section, const char *key, int &value)
{
    XMLElement *key_node = get_key_node(section, key);
    if ( key_node == NULL )
    {
        return -1;
    }
    value = key_node->IntText(0);
    return 0;
}

//获取unsigned short节点值
int XmlParser::get_ushort(const char *section, const char *key, unsigned short &value)
{
    XMLElement *key_node = get_key_node(section, key);
    if ( key_node == NULL )
    {
        return -1;
    }
    value = (unsigned short)atoi( key_node->GetText() );
    return 0;
}

//获取string节点值
int XmlParser::get_string(const char *section, const char *key, string &value)
{
    XMLElement *key_node = get_key_node(section, key);
    if ( key_node == NULL )
    {
        return -1;
    }
    value = string( key_node->GetText() );
    return 0; 
}

