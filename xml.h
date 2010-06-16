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
#define DBALSTER_XML_H

//
// this is a very small XML parser. The document is scanned from a memory block
// and copied to a tree-like structure. the class uses its own allocator to simplify
// the memory management.
//
// caveats:
// - no validation or even DTD support
// - (currently) no encoding support
// - the scanned document is read-only
//

#ifndef DBALSTER_CONFIG_H
#include "config.h"
#endif

typedef struct _XmlAttribute XmlAttribute;
typedef struct _XmlElement   XmlElement;
typedef struct _XmlDocument  XmlDocument;

// linear linked list of Xml attributes
struct _XmlAttribute
{
	const char*   name;
	const char*   content;
	XmlAttribute*	next;
};

// Xml element holds a link to its parent element, a list of its children elements
//and a list of this element's atributes.
struct _XmlElement
{
	const char*   name;
	const char*   content;
	XmlElement*		tail;					// link to last node
	XmlElement*		next;					// link to next element or null if last element
	XmlElement*		parent;				// link to parent or null if root element
	XmlElement*		elements;			// children elements (or null if no children)
	XmlAttribute*	attributes;		// element attributes (or null...)
	void*         userdata;			// C-like userdata.
};

// simple string compare. the idea is to have a compare function that supports quoted and unquoted entities (i.e. compare("&gt;",">") == true)
CAPI bool xml_compare( const char* _str, const char* _text );
CAPI bool xml_element_name( XmlElement* _elem, const char* _value );
CAPI bool xml_attribute_name( XmlAttribute* _attr, const char* _value );

// link attribute to attributes list
CAPI void xml_element_add_attribute( XmlElement* _elem, XmlAttribute* _attr );

// link element to elements list
CAPI void xml_element_add_element( XmlElement* _elem, XmlElement* _child );

// linear search for an element. continue the search by providing
// an element as optional parameter
CAPI XmlElement* xml_element_find_element( XmlElement* _elem, const char* _name, XmlElement* _element /*= 0*/ );

// recursive search, stops at the first found element
CAPI XmlElement* xml_element_find_any( XmlElement* _elem, const char* _name );

// linear search for an attribute. continue the search by providing
// an attribute as optional parameter
CAPI XmlAttribute* xml_element_find_attribute( XmlElement* _elem, const char* _name, XmlAttribute* _attribute /*= 0*/ );

// find all elements matching "_name". if _element is null it returns
// how many elements have been found, then you can allocate an array of
// XmlElement pointers and do a second run. if _count is non zero, a check is
// performed while writing to the XmlElement pointers array.
CAPI unsigned int xml_element_find_elements( XmlElement* _elem, const char* _name, XmlElement* _begin[] /*= 0*/, XmlElement* _end[] /*= 0*/ );
CAPI unsigned int xml_element_find_attributes( XmlElement* _elem, const char* _name, XmlAttribute* _begin[] /*= 0*/, XmlAttribute* _end[] /*= 0*/ );

typedef void (*XmlForEachFunc)(XmlElement* _elem, void* _param);
CAPI void xml_element_foreach( XmlElement* _elem, XmlForEachFunc _func, void* _param );

CAPI XmlElement* xml_element_find_element_by_attribute_value( XmlElement* _elem, const char* _elemName, const char* _attrName, const char* _attrValue );

CAPI unsigned int xml_element_find_elements_by_attribute( XmlElement* _elem, const char* _name, XmlElement* _begin[] /*= 0*/, XmlElement* _end[] /*= 0*/ );

CAPI XmlElement* xml_element_get_root(XmlElement* _e);

// like above, but now we want to know the named attribute's value
CAPI XmlAttribute* xml_element_find_attribute_by_name( XmlElement* self, const char* _elemName, const char* _attrName );

// copy the element content to the buffer. if buffer is 0 it returns the
// size of the content.
CAPI unsigned int xml_element_get_content( XmlElement* _elem, char* _buffer, unsigned int _size );

// free memory used by the complete XML document tree
CAPI void xml_document_clear(XmlDocument* _doc);

CAPI XmlDocument* xml_document_create();
CAPI void xml_document_destroy(XmlDocument* _doc);
CAPI XmlElement* xml_document_get_root(XmlDocument* _doc);
CAPI bool xml_document_parse( XmlDocument* _doc, const char* _begin, const char* _end );

#endif
// vim:ts=2
