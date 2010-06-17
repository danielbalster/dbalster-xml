/*
 * dbalster's XML DOM parser
 *
 * Copyright (c) Daniel Balster
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Daniel Balster nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY DANIEL BALSTER ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL DANIEL BALSTER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DBALSTER_XML_H
#include "xml.h"
#endif

#include <string.h>		// strstr
#include <stdio.h>
#include <stdlib.h>

// internal data representation

struct _XmlDocument
{
	XmlElement*	  pRoot;
	unsigned int  nChars;		// byte-aligned
	unsigned int	nBytes;		// struct-aligned
	unsigned int	nUsedChars;
	unsigned int	nUsedBytes;
};

// private methods.

// alloc memory, if _string is true the string pool is used
CAPI void* xml_document_alloc_memory( XmlDocument* _doc, const unsigned int _bytes, bool _string /*= false*/ );
// duplicate string (automatically adds a null byte)
CAPI char* xml_document_clone_string( XmlDocument* _doc, const char* _str, const unsigned int _size, const bool _escape /*= true*/ );
// build the XML document tree in two passes (_scanonly = false,true)
CAPI const char* xml_document_scan( XmlDocument* _doc, XmlElement* _element, const char* _begin, const char* _end, bool _scanonly );


// scan for the next character not in [ \t\n\r]* in the range [_doc,_end]
static const char* scan_whitespace( const char* _doc, const char* _end )
{
  char ch;
	while( 0!=(ch = *_doc++) )
	{
		if ((_doc <= _end) && (' '==ch || '\t'==ch || '\n'==ch || '\r'==ch)) continue;
		break;
	}
	return _doc-1;
}

// scan for the first char not in [a-zA-Z0-9\.\:_\-\/]+ in the range [_doc,_end]
// any prefixing whitespace is ignored.
static const char* scan_identifier( const char* _doc, const char* _end )
{
  char ch;
	_doc = scan_whitespace(_doc,_end);
	while( 0 !=(ch = *_doc++) )
	{
		if (_doc > _end) break;
		if ((ch>='a' && ch<='z') || (ch>='A' && ch<='Z')
			|| (ch>='0' && ch<='9') || (ch=='.')	|| (ch==':')
			|| (ch=='_') || (ch=='-') || (ch=='/')) continue;
		else break;
	}
	return _doc-1;
}

// string xml_compare. I could have used strcmp or similar, but I want to extend this library to support
// different encodings and escapings (i.e. quoted html entities and plain entities)
CAPI bool xml_compare( const char* _str, const char* _text )
{
	int i=0;
	if (0==_str) return false;
	while (_text[i])
	{
		if (_str[i]!=_text[i]) return false;
		++i;
	}
	return true;
}

static CAPI bool xml_namespace_compare(const char* _name, const char* _value)
{
  const char* name = strchr(_name,':');
  const char* value = strchr(_value,':');
  if (name) name++; else name=_name;
  if (value) value++; else value=_value;
  return 0==strcmp(name,value);
}

CAPI bool xml_element_name( XmlElement* _elem, const char* _value )
{
  return (_elem && _elem->name && xml_namespace_compare(_elem->name,_value));
}

CAPI bool xml_attribute_name( XmlAttribute* _attr, const char* _value )
{
  return (_attr && _attr->name && xml_namespace_compare(_attr->name,_value));
}

// enqueue the attribute entry at the end of the attributes list
CAPI void xml_element_add_attribute( XmlElement* self, XmlAttribute* attribute )
{
	if (0==self->attributes)
	{
		self->attributes = attribute;
	}
	else
	{
		XmlAttribute* iter = self->attributes;
		while (iter->next) iter = iter->next;
		iter->next = attribute;
	}
}

// enqueue the element at the end of the elements list
CAPI void xml_element_add_element( XmlElement* self, XmlElement* child )
{
	child->parent = self;
	if (0==self->elements)
	{
		self->elements = child;
	}
	else
	{
		XmlElement* iter = self->elements;
    if (self->tail)
    {
      self->tail->next = child;
    }
    else
    {
      while (iter->next) iter = iter->next;
      iter->next = child;
    }
	}
  self->tail = child;
}

CAPI XmlElement* xml_element_find_element_by_attribute_value( XmlElement* self, const char* _elemName, const char* _attrName, const char* _attrValue )
{
	if (self->name && xml_element_name(self,_elemName))
	{
		XmlAttribute* iter = self->attributes;
		while (iter)
		{
			if (iter->name && xml_attribute_name(iter,_attrName))
			{
				if (iter->content && xml_compare(iter->content,_attrValue))
				{
					return self;
				}
			}
			iter = iter->next;
		}
	}

	XmlElement* iter = self->elements;
	while (iter)
	{
		XmlElement* e = xml_element_find_element_by_attribute_value(iter,_elemName,_attrName,_attrValue);
		if (e) return e;
		iter = iter->next;
	}

	return 0;
}

CAPI XmlAttribute* xml_element_find_attribute_by_name( XmlElement* self, const char* _elemName, const char* _attrName )
{
	if (self->name && xml_element_name(self,_elemName))
	{
		XmlAttribute* iter = self->attributes;
		while (iter)
		{
			if (iter->name && xml_attribute_name(iter,_attrName))
			{
        return iter;
			}
			iter = iter->next;
		}
	}
  
	XmlElement* iter = self->elements;
	while (iter)
	{
		XmlAttribute* a = xml_element_find_attribute_by_name(iter,_elemName,_attrName);
		if (a) return a;
		iter = iter->next;
	}
  
	return 0;
}

CAPI XmlElement* xml_element_find_element( XmlElement* self, const char* _name, XmlElement* _element )
{
	XmlElement* iter = _element ? _element->next : self->elements;
	while (iter)
	{
		if (iter->name && xml_element_name(iter,_name)) return iter;
		iter = iter->next;
	}
	return 0;
}

// iterator helper
CAPI void xml_element_foreach( XmlElement* _elem, XmlForEachFunc _func, void* _param )
{
  if (_func) _func(_elem,_param);

	XmlElement* iter = _elem->elements;
  
	while (iter)
	{
		xml_element_foreach(iter, _func, _param);
		iter = iter->next;
	}
}

CAPI XmlElement* xml_element_find_any( XmlElement* _elem, const char* _name )
{
  if (xml_element_name(_elem,_name)) return _elem;
  
	XmlElement* iter = _elem->elements;
	while (iter)
	{
		XmlElement* e = xml_element_find_any(iter, _name);
    if (e) return e;
		iter = iter->next;
	}
  return 0;
}

CAPI unsigned int xml_element_find_elements( XmlElement* self, const char* _name, XmlElement* _begin[], XmlElement* _end[] )
{
	unsigned int count = 0;

	if (_name==0 || xml_element_name(self,_name))
	{
		if (_begin < _end)
		{
			*_begin = self;
		}
		count++;
	}

	XmlElement* iter = self->elements;

	while (iter)
	{
		count += xml_element_find_elements(iter,_name,_begin ? _begin+count : 0,_end);
		iter = iter->next;
	}

	return count;
}

CAPI unsigned int xml_element_find_elements_by_attribute( XmlElement* self, const char* _name, XmlElement* _begin[] /*= 0*/, XmlElement* _end[] /*= 0*/ )
{
	unsigned int count = 0;

  XmlAttribute* attr = self->attributes;
  while (attr)
	{
    if (xml_attribute_name(attr,_name))
    {
      if (_begin < _end)
      {
        *_begin = self;
      }
      count++;
    }
    attr = attr->next;
	}
  
	XmlElement* iter = self->elements;
  
	while (iter)
	{
		count += xml_element_find_elements_by_attribute(iter,_name,_begin ? _begin+count : 0,_end);
		iter = iter->next;
	}
  
	return count;
}

CAPI unsigned int xml_element_find_attributes( XmlElement* self, const char* _name, XmlAttribute* _begin[], XmlAttribute* _end[] )
{
	unsigned int count = 0;
	XmlAttribute* iter = self->attributes;

	while (iter)
	{
		if (xml_attribute_name(iter,_name))
		{
			if (_begin < _end)
			{
				*_begin = iter;
				_begin++;
			}
			count++;
		}

		iter = iter->next;
	}

	return count;
}

CAPI XmlAttribute* xml_element_find_attribute( XmlElement* self, const char* _name, XmlAttribute* _attribute )
{
  if (self==0) return 0;
	XmlAttribute* iter = _attribute ? _attribute : self->attributes;
	while (iter)
	{
		if (xml_attribute_name(iter,_name)) return iter;
		iter = iter->next;
	}
	return 0;
}

CAPI XmlElement* xml_element_get_root(XmlElement* _e)
{
  while (_e && _e->parent) _e = _e->parent;
  return _e;
}

// concatenate all child elements that are content elements (name==0)
// *NOT* useful for SVG and HTML
CAPI unsigned int xml_element_get_content( XmlElement* self, char* _buffer, unsigned int _size )
{
	unsigned int size = 0;
	XmlElement* iter = self->elements;
	while (iter)
	{
		if (0==iter->name)
		{
			int i=0;
			while (iter->content[i])
			{
				if (_buffer && size<_size) _buffer[size] = iter->content[i];
				size++;
				i++;
			}
		}
//    size += xml_element_get_content(iter,_buffer+size,_size);
		iter = iter->next;
	}
	return size;
}

CAPI XmlDocument* xml_document_create()
{
  XmlDocument* doc = (XmlDocument*) calloc(1,sizeof(XmlDocument));
  return doc;
}

CAPI void xml_document_destroy(XmlDocument* self)
{
  if (self)
  {
    xml_document_clear(self);
    free(self);
  }
}

CAPI XmlElement* xml_document_get_root(XmlDocument* self)
{
	return self->pRoot;
}

CAPI void xml_document_clear(XmlDocument* self)
{
  if (self->pRoot) free (self->pRoot);
	self->pRoot = 0;
	self->nBytes = self->nChars = self->nUsedChars = self->nUsedBytes = 0;
}

// allocMem memory. two pools are used - one for strings and one for 4-byte aligned structs
CAPI void* xml_document_alloc_memory( XmlDocument* self, const unsigned int _bytes, bool _string )
{
	char* pointer = (char*) self->pRoot;
	if (_string)
	{
		if ( (self->nUsedChars + _bytes) > self->nChars ) return 0;
		pointer += ( self->nBytes + self->nUsedChars );
		self->nUsedChars += _bytes;
	}
	else
	{
		if ( (self->nUsedBytes + _bytes) > self->nBytes ) return 0;
		pointer += self->nUsedBytes;
		self->nUsedBytes += _bytes;
	}
	return pointer;
}

// create a zero-terminated string clone
CAPI char* xml_document_clone_string( XmlDocument* self, const char* _str, const unsigned int _size, const bool _escape )
{
	char* str = (char*) xml_document_alloc_memory(self,_size+1,true);
	if (str)
	{
		unsigned int i=0, j=0;
		for (; i<_size;i++,j++)
		{
			if (_escape && _str[i]=='&')
			{
				if (xml_compare(_str+i,"&lt;")) { str[j]='<'; i+=3; }
				else if (xml_compare(_str+i,"&gt;")) { str[j]='>'; i+=3; }
				else if (xml_compare(_str+i,"&amp;")) { str[j]='&'; i+=4; }
				else if (xml_compare(_str+i,"&quot;")) { str[j]='\''; i+=5; }
				else
				{
					// skip unknown entity
					while (_str[i]!=';' && i<_size) i++;
				}
			}
			else str[j] = _str[i];
		}
		str[j]=0;
	}
	return str;
}

// scanning is performed in two passes, the first one (_scanonly=true) is used to estimate the memory usage,
// whereas the second pass (_scanonly=false) will construct the XML document tree.
// the complete document will be placed into one single memory block, this is cache friendly and does not
// fragment the memory manager.
CAPI const char* xml_document_scan( XmlDocument* self, XmlElement* _element, const char* _doc, const char* _end, bool _scanonly )
{
	const char* marker = 0;
	// TODO check if _doc<_end is correct (valgrind demanded this!)
	while( _doc && _doc<_end && *_doc )
	{
		if (_doc >= _end) return 0;
		char c = *_doc++;
		if ('<'==c)
		{
			if (marker)
			{
				int n = _doc-marker-1;
				if (_scanonly)
				{
					self->nChars += n+1;
					self->nBytes += sizeof(XmlElement);
				}
				else
				{
					XmlElement* text = (XmlElement*) xml_document_alloc_memory(self,sizeof(XmlElement),false);
					text->content = xml_document_clone_string(self,marker,n,true);
					text->name = 0;
          
          // convenience
          _element->content = text->content;
          
					xml_element_add_element( _element, text );
				}
				marker = 0;
			}
			bool recurse = true;
			bool allocate = false;
			XmlElement* element = 0;
			if ('!' == *_doc) // skip comments, cdata, dtds, doctypes. not supported
			{
				int nesting=1;
				if (xml_compare(_doc,"![CDATA["))
				{
					_doc+=8;	// skip '![CDATA['
					const char* end = strstr(_doc,"]]>");
					if (end)
					{
						int n = end - _doc;
						if (_scanonly)
						{
							self->nChars += n+1;
							self->nBytes += sizeof(XmlElement);
						}
						else
						{
							XmlElement* text = (XmlElement*) xml_document_alloc_memory(self,sizeof(XmlElement),false);
							text->content = xml_document_clone_string(self,_doc,n,false);
							text->name = 0;

              // convenience
              _element->content = text->content;

							xml_element_add_element( _element,text );
						}

						_doc = end+3; // "]]>"
					}
          else fprintf(stderr,"XML: unterminated CDATA");
				}
				else if (xml_compare(_doc,"!--"))	// TODO: create comment element?
				{
					_doc = strstr(_doc,"-->");
					if (_doc) _doc+=3;
          else fprintf(stderr,"XML: unterminated comment");
				}
				else do
				{
					if ('<' == *_doc) nesting++;
					if ('>' == *_doc) nesting--;
					_doc++;
				}
				while ( nesting>0 && _doc < _end );
				continue;
			}

			// processing instructions: not well supported
			if ('?' == *_doc)
			{
				_doc++;
				recurse = false;
			}

			const char* end = scan_identifier(_doc,_end);
			if ('/' != *_doc)	// this is not a terminating element
			{
				if (end[-1]=='/') --end;
				allocate = true;
				if (_scanonly)
				{
					self->nChars += (end-_doc) + 1;
					self->nBytes += sizeof(XmlElement);
				}
				else
				{
					element = (XmlElement*) xml_document_alloc_memory(self,sizeof(XmlElement),false);
					element->name = xml_document_clone_string(self,_doc,end-_doc,true);
					element->content = 0;
					xml_element_add_element( _element,element );
				}
			}
			else // this is a terminating element (</name>)
			{
				_doc = scan_whitespace(end,_end);
				if ('>' != _doc[0] && '>' != _doc[1])
          fprintf(stderr,"XML: '>' expected");
				return _doc;
			}
			// scan the element (and all attributes)
			_doc = scan_whitespace(end,_end);
			while ( '>' != *_doc )
			{
				_doc = scan_whitespace(_doc,_end);
				if ('?' == *_doc)	// ending of <?tag ... ?>
				{
					_doc++;
					if ('>' != *_doc)
            fprintf(stderr,"XML: '>' expected");
					break;
				}
				if ('/' == *_doc)		// element has no content and is complete
				{
					_doc = scan_whitespace(_doc+1,_end);
					if ('>' != *_doc)
            fprintf(stderr,"XML: '>' expected");
					recurse = false;
					break;
				}
				if (allocate) // scan all attributes
				{
					XmlAttribute* attribute = 0;
					end = scan_identifier(_doc,_end);
					if (_scanonly)
					{
						self->nChars += (end-_doc) + 1;
						self->nBytes += sizeof(XmlAttribute);
					}
					else
					{
						attribute = (XmlAttribute*) xml_document_alloc_memory(self,sizeof(XmlAttribute),false);
						attribute->name = xml_document_clone_string(self,_doc,end-_doc,true);
						attribute->content = "";
						xml_element_add_attribute( element, attribute );
					}
					_doc = scan_whitespace(end,_end);
					if ('=' == *_doc)	// attribute with assignment
					{
						_doc = scan_whitespace(_doc+1,_end);
						char quote = *_doc;
						if (quote!='"' && quote!='\'')
              fprintf(stderr,"XML: quoted string (\" or ') expected");
						for (_doc++,end=_doc;*end!=quote && end<_end;++end);		// scan end of quoted string
						if (end-_doc>0)
						{
							if (_scanonly)
							{
								self->nChars += (end-_doc) + 1;
							}
							else
							{
								attribute->content = xml_document_clone_string(self,_doc,end-_doc,true);
							}
						}
					}
					_doc = scan_whitespace(end+1,_end);
				}
				else
				{
					_doc++;
				}
			}
			if (allocate && recurse)
			{
				// so, tag ist offen und gescanned, dann rekursion
				_doc = xml_document_scan(self,element,_doc+1,_end,_scanonly);
			}
			if (_doc) _doc++;	// skip '>'
		}
		else
		{
			if (0==marker) marker = _doc-1;
		}
	}
	return _doc;
}

CAPI bool xml_document_parse( XmlDocument* self, const char* _doc, const char* _end )
{
	// phase #1: estimate exact memory usage
	xml_document_clear(self);
	self->nChars = 0;
	self->nBytes = sizeof(XmlElement);		// pRoot element
	self->nUsedBytes = self->nBytes;				// initial allocation
	xml_document_scan(self,0,_doc,_end,true);

	// phase #2: scan and construct document tree
	self->pRoot = (XmlElement*) calloc( 1, self->nChars + self->nBytes );
	self->pRoot->name = "";
	self->pRoot->content = "";
	xml_document_scan(self,self->pRoot,_doc,_end,false);
  
  return true;
}
// vim:ts=2
