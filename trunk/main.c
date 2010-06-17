#include <sys/types.h>
#include <sys/stat.h> // fstat
#include <sys/mman.h> // mmap,munmap
#include <fcntl.h>    // open 
#include <unistd.h>   // close
#include <stdio.h>    // printf
#include <stdlib.h>   // calloc

#include "xml.h"

void my_xml_foreach_func( XmlElement* _elem, void* _param )
{
  if (_elem->name!=0) printf("%s\n",_elem->name);
}

static void do_xml_tests( XmlDocument* doc )
{
  XmlElement* root = xml_document_get_root(doc);
  
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


int main (int argc, const char * argv[])
{
  const char* filename = "test.xml";
  if (argc>1) filename = argv[1];
  
  XmlDocument* doc = xml_document_create();
  if (doc)
  {
    // load a file to memory, do whatever you want here
    struct stat st;
    int fd = open(filename,0);
    if (fd && 0==fstat(fd,&st))
    {
      char* mem = (char*) mmap(0,st.st_size,PROT_READ,MAP_FILE+MAP_PRIVATE,fd,0);
      if (mem)
      {
        // parse the block of memory
        xml_document_parse(doc, mem, mem+st.st_size);
        
        // after parsing, the memory is no longer needed
        
        munmap(mem, st.st_size);
      }
      close(fd);
    }
    
    do_xml_tests(doc);

    xml_document_destroy(doc);
  }
  
  return 0;
}
// vim:ts=2
