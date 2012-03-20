/*
Copyright (C) 2010  INRIA, Planete Team

Authors:
--------------------------------------------------------------
Amir Krifa			:  Amir.Krifa@sophia.inria.fr
Chadi Barakat			: Chadi.Barakat@sophia.inria.fr
Thrasyvoulos Spyropoulos	: spyropoulos@tik.ee.ethz.ch
--------------------------------------------------------------
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 3
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/


#ifndef XMLTREE_H
#define XMLTREE_H

#include <map>
#include <list>
#include <string>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/sax2/Attributes.hpp>

using namespace xercesc;

class XMLTree
{

public:
	/**
	 * Constructor used by our SAX parsing routines.
	 * 
	 * @param name Name of the element.
	 * @param attrs Attribute key/value pairs.
	 */
	XMLTree(std::string name, const Attributes &attrs);
	~XMLTree();
	/**
	 * Constructor used elsewhere to build an XMLTree element.
	 * 
	 * @param name Name of the element.
	 * @param attrs Attribute key/value pairs.
	 */
	XMLTree(std::string name, std::map<std::string,std::string> &attrs);

	/**
	 * Returns the name of the element represented by the class.
	 */
	std::string getName() 
	{
		return elementName;
	}

	/**
	 * Assign the element's value (<element>value</element>).
	 *
	 * @param val Value to assign.
	 */
	void assignValue(std::string val) 
	{
		elementValue = val;
	}

	/**
	 * Returns whether or not the element has a character value.
	 *
	 * @return True if there is a value, false otherwise.
	 */
	bool haveValue() 
	{
		if (elementValue.length() == 0) 
		{
			return false;
		}
		return true;
	}

	/**
	 * Returns the value.
	 * 
	 * @return The element's character value.
	 */
	std::string getValue() 
	{
		return elementValue;
	}
	
	/**
	 * Queries whether the element has an attribute of the given name.
	 *
	 * @return True if the attribute exists, else false.
	 */
	bool haveAttr(std::string key) 
	{
		return (elementAttributes.find(key) != elementAttributes.end());
	}

	/**
	 * Gets the attribute value for the specified attribute.
	 *
	 * @return The attribute value, or null if no key.
	 */
	std::string getAttr(std::string key) 
	{
		return elementAttributes[key];
	}
	
	/**
	 * Gets the attribute value for the specified attribute, throws an
	 * exception if the key does not exist.
	 * 
	 * @return The attribute value.
	 * @throws NoSuchElementException If the attribute does not exist.
	 */
	std::string getAttrRequired(std::string key)
	{
		std::string val = elementAttributes[key];
		if (val.length() == 0) 
		{
			//throw new NoSuchElementException("Invalid key");
		}
		return val;
	}
	
	/**
	 * Adds a child element to this element.
	 *
	 * @param element Child element.
	 */
	void addChildElement(XMLTree * element) 
	{
		childElements.push_back(element);
	}
	
	/**
	 * @return The number of direct children elements.
	 */
	int numChildElements() 
	{
		if (childElements.empty()) 
		{
			return 0;
		}
		return childElements.size();
	}

	/**
	 * Get a specific direct child element.
	 *
	 * @param x Index into array of direct children.
	 * @return Child represented by the index.
	 */
	XMLTree *getChildElement(unsigned int x);
	
	/**
	 * Searches for the first direct child element of the supplied name and
	 * returns the XMLTree object for the child. An exception is thrown if
	 * no matching child is found. 
	 * 
	 * @param name Name of element to search for.
	 * @return Child XMLTree object.
	 * @throws NoSuchElementException If no matching child found.
	 */
	XMLTree *getChildElementRequired(std::string name);

	void showElement();

private:

	std::string elementName;
	std::string elementValue;
	std::map<std::string,std::string> elementAttributes;
	std::list<XMLTree*> childElements;

};

#endif

