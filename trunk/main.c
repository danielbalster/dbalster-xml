#include <sys/types.h>

#ifdef WIN32
#pragma warning(disable:4996) // unsafe...
#endif
#include <stdio.h>    // printf
#include <stdlib.h>   // calloc

#include "xml.h"

void my_xml_foreach_func( XmlElement* _elem, void* _param )
{
  if (_elem->name!=0) printf("%s\n",_elem->name);
}

static void do_xml_tests( XmlElement* root )
{
  
  XmlElement* e;      // XML element, ie <element>
  XmlAttribute* a;    // XML attribute, ie <element attribute="value">
  

  // XML namespaces: if you provide element or attribute names without namespace,
  // it will ignore namespaces:
  // 
  // "href" will match "href", "xlink:href" and "foo:href"
  // "xlink:href" will only match "xlink:href"
  
  // simple recursive tree walker
  xml_element_foreach(root, my_xml_foreach_func, /*param*/ 0);
  
  // find the very first element with the given name
  // xpath.SelectSingleNode
  e = xml_element_find_any(root, "foo");
  
  // find element with given name that is a direct child of the specified node
  e = xml_element_find_element(root, "foo", 0);
  
  // find named attribute in specified element
  a = xml_element_find_attribute(e, "name", 0);
  
  // find (recursive) the first element with the given name that has the named attribute
  a = xml_element_find_attribute_by_name(root, "foo", "version");
  
  // find (recursive) the first element with element name, attribute name and attribute value
  e = xml_element_find_element_by_attribute_value(root, "xml", "version", "1.1");
  
  
  // two pass finding of named elements
  // similar to xpath.select("//foo")
  {
    // count number of elements (recursive) with a specific name
    size_t n_elements = xml_element_find_elements(root, "bar", 0, 0);
    // allocate array of points for n_elements
    XmlElement** elements = (XmlElement**) calloc(n_elements,sizeof(XmlElement*));
    // get element pointers
    xml_element_find_elements(root,"bar",elements,elements+n_elements);
    printf("found %ld elements\n",n_elements);
    for (size_t i=0; i<n_elements; ++i)
    {
      printf("%ld: %s=\"%s\"\n",i,elements[i]->name,(elements[i]->content!=0) ? elements[i]->content : "(null)");
    }
  }
  
  if (true == xml_element_name(e, "foo"))
  {
    // check if element matches this name (ignoring namespaces)
  }
  
  if (true == xml_element_name(e,"ns:foo"))
  {
    // check if element name matches (including namespace)
  }
  
  // two pass collecting recursive content
  // ie "this is <b>bold</b> text" will collect "this is bold text"
  if (e)
  {
    size_t n_chars = xml_element_get_content(e, 0, 0);
    char* content = (char*) calloc(1,n_chars+1);  // + \0 byte
    xml_element_get_content(e,content,n_chars);
  }
  
  // get element content
  e = xml_element_find_any(root, "path");
  // if e is zero, element wasn't found
  // content can be zero, so you should always check!
  if (e && e->elements)
  {
    printf("Content of path: %s\n",e->content);
  }
  
  // get attribute content
  a = xml_element_find_attribute(e, "xlink:href", 0);
  if (a && a->content)
  {
    printf("Attribute Value: %s\n",a->content);
  }
  
  // iterating through elements
  
  for (XmlElement* iter = root->elements; iter != 0; iter = iter->next )
  {
    if (iter->name==0)
    {
      printf("Text Content: \"%s\"\n",iter->content);
    }
    else
    {
      printf("Element: <%s/>\n",iter->name);
    }
  }
}

void xml_error_handler( const char* _errorMessage, const char* _begin, const char* _current )
{
  fprintf(stderr,"ERROR: %s\n",_errorMessage);
}

void process_file( const char* filename )
{
  // load a file to memory, do whatever you want here
  char* mem = 0;
  size_t size = 0;
  
  FILE* file = fopen(filename,"rb");
  if (file)
  {
    fseek(file,0,SEEK_END);
    size = ftell(file);
    fseek(file,0,SEEK_SET);
    mem = (char*) malloc(size);
    fread(mem,size,1,file);
    fclose(file);
  }
  /*
   XmlSizeofHint hints[] = {
   { "foo:bar", 0, 101 },
   { "fee:bar", 0, 102 },
   { "bar", 0, 100 },
   { "start", 0, 123 },
   0
   };
   */
  XmlElement* root = xml_create(mem,mem+size,xml_error_handler,malloc,0);
  if (root)
  {
    printf("passed : %s\n",filename);
    do_xml_tests(root);
    free(root);
  }
  else
  {
    printf("failed : %s\n",filename);
  }
  
  if (mem) free(mem);
}

int main (int argc, const char * argv[])
{
  const char* filename = "test.xml";
  if (argc>1) filename = argv[1];
  process_file(filename);
  
  return 0;
}
// vim:ts=2
