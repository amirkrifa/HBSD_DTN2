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


#include "XMLTree.h"
#include <iostream>
#include "HBSD.h"

using namespace std;


XMLTree::XMLTree(string name, const Attributes &attrs) 
{
	elementName = name;
	elementValue.clear();

	for (unsigned int i=0; i< attrs.getLength(); i++)
	{
		elementAttributes[string(XMLString::transcode(attrs.getQName(i)))] = string(XMLString::transcode(attrs.getValue(i)));
	}
	
	
}

XMLTree::~XMLTree()
{
	elementAttributes.clear();
	while(!childElements.empty())
	{
		list<XMLTree*>::iterator iter = childElements.begin();
		delete (*iter);
		childElements.erase(iter);
	}
}

XMLTree::XMLTree(string name, map<string,string> &attrs) 
{
	elementName = name;
	elementValue.clear();

	for(map<string, string>::iterator iter = attrs.begin(); iter != attrs.end();iter++)
	{
		elementAttributes[iter->first] = iter->second;
	}
}


XMLTree * XMLTree::getChildElement(unsigned int x)
{
	if (childElements.empty()) 
	{
		if(HBSD::log->enabled(Logging::ERROR))
			HBSD::log->error(string("XMLTree::getChildElement error: childElements map is empty."));
		return NULL;
	}

	if (x >= childElements.size())
	{
		if(HBSD::log->enabled(Logging::ERROR))
			HBSD::log->error(string("XMLTree::getChildElement error: requested element > number of elements."));
		return NULL;
	}

	unsigned int i = 0;
	list<XMLTree*>::iterator iter = childElements.begin();
	while( i < x && iter != childElements.end())
	{
		iter++;
		i++;
	} 
	assert(*iter != NULL);

	return *iter;
}


void XMLTree::showElement()
{

	cout <<endl<< "Element Name: "<<elementName<< endl;
	if(elementValue.length() > 0)
		cout <<"Element Value: "<<elementValue<<endl;
	map<std::string,std::string>::iterator iter = elementAttributes.begin(); 
	while(iter != elementAttributes.end())
	{
		cout<<"Attr name: "<< iter->first<<" value: "<<iter->second<<endl;
		iter++;	
	}

	if(!childElements.empty())
	{
		cout<<"List of Sub Elements: "<<endl;
		list<XMLTree*>::iterator iter2 = childElements.begin();
		while( iter2 != childElements.end())
		{
			(*iter2)->showElement();
			iter2++;
		}
	}

	cout<<endl;
	
}


XMLTree *XMLTree::getChildElementRequired(std::string name)
{

	assert(!name.empty());
	for(std::list<XMLTree*>::iterator iter = childElements.begin(); iter != childElements.end();iter++)
	{
		if ((*iter)->elementName.compare(name) == 0)
		{
			return (*iter);
		}
	}

	if(HBSD::log->enabled(Logging::ERROR))
	{
		HBSD::log->error(string("Error occured within XMLTree::getChildElementRequired unable to find: ")+ name);
		showElement();
	}
	return NULL;
}
