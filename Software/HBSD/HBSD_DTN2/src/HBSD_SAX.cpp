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


#include "HBSD_SAX.h"
#include "Handlers.h"
#include <exception>
#include "HBSD.h"
#include <iostream>
using namespace std;
using namespace xercesc;

string HBSD_SAX::BPA_ELEMENT;

HBSD_SAX::HBSD_SAX(Handlers *rtrHandlers) 
{
	assert(rtrHandlers != NULL);
	BPA_ELEMENT = "bpa";
	intf = rtrHandlers;

	// Map element names to a value that we can switch off of
	// to call the appropriate handler.
	eventHash["bundle_received_event"] = BUNDLE_RECEIVED_EVENT;
	eventHash["data_transmitted_event"] = DATA_TRANSMITTED_EVENT;
	eventHash["bundle_delivered_event"] = BUNDLE_DELIVERED_EVENT;
	eventHash["bundle_delivery_event"] = BUNDLE_DELIVERY_EVENT;
	eventHash["bundle_send_cancelled_event"] = BUNDLE_SEND_CANCELLED_EVENT;
	eventHash["bundle_expired_event"] = BUNDLE_EXPIRED_EVENT;
	eventHash["bundle_injected_event"] = BUNDLE_INJECTED_EVENT;
	eventHash["link_opened_event"] = LINK_OPENED_EVENT;
	eventHash["link_closed_event"] = LINK_CLOSED_EVENT;
	eventHash["link_created_event"] = LINK_CREATED_EVENT;
	eventHash["link_deleted_event"] = LINK_DELETED_EVENT;
	eventHash["link_available_event"] = LINK_AVAILABLE_EVENT;
	eventHash["link_unavailable_event"] = LINK_UNAVAILABLE_EVENT;
}

HBSD_SAX::~HBSD_SAX()
{
	eventHash.clear();
	while(!elementStack.empty())
	{
		delete (elementStack.top());
		elementStack.pop();	
	}
	intf = NULL;
	xmlRoot = NULL;	
}

// Start parsing a new element
void HBSD_SAX::startElement (const   XMLCh* const uri, const   XMLCh* const localname, const XMLCh* const qname, const Attributes& attrs)
{
	if ((xmlRoot == NULL) && (string(XMLString::transcode(qname)).compare(BPA_ELEMENT) != 0)) 
	{
		return;
	}

	XMLTree *el = new XMLTree(string(XMLString::transcode(qname)), attrs);
	assert(el != NULL);
	if (xmlRoot == NULL) 
	{
		xmlRoot = el;
	} else 
	{
		XMLTree *parent = elementStack.top();
		parent->addChildElement(el);
		parent = NULL;
	}
	elementStack.push(el);
	el = NULL;
}

void HBSD_SAX::characters(const   XMLCh* const ch, const unsigned int start)
{
	string val;
	val.assign(XMLString::transcode(ch));
	if (val.length() == 0) 
	{
		return;
	}
	elementStack.top()->assignValue(val);
}

void HBSD_SAX::endElement ( const XMLCh* const uri, const XMLCh* const localname, const XMLCh* const qname)
{
	if (xmlRoot == NULL) 
	{
		// No <bpa> element yet.
		return;
	}

	elementStack.pop();
}

void HBSD_SAX::startDocument () 
{
	xmlRoot = NULL;
	while(!elementStack.empty())
	{
		if(elementStack.top() != NULL)
			delete (elementStack.top());
		elementStack.pop();	
	}
}

void HBSD_SAX::endDocument () 
{
	if (xmlRoot != NULL) 
	{
		int numElements = xmlRoot->numChildElements();
		if (numElements == 0) 
		{
			rootOnly();
		} else 
		{
			callRouter(numElements);
		}
	}
	xmlRoot = NULL;
	while(!elementStack.empty())
	{
		if(elementStack.top() != NULL)
			delete (elementStack.top());
		elementStack.pop();	
	}
}


void HBSD_SAX::rootOnly() 
{
	intf->handler_bpa_root(xmlRoot);
}


void HBSD_SAX::callRouter(int numElements) 
{
	// We invoke the router method with the event element as the root (as
	// opposed to the <bpa> element). Assume 1 or more router events.

	for (int indx=0; indx<numElements; indx++) 
	{
		XMLTree * elementRoot = xmlRoot->getChildElement(indx);
		assert(elementRoot != NULL);
		string tmp = elementRoot->getName();
		assert(!tmp.empty());
		if (eventHash.find(tmp) == eventHash.end())
		{
			continue;
		}

		try 
		{
			assert(intf != NULL);
			switch (eventHash[tmp])
			{
				case BUNDLE_RECEIVED_EVENT:
					intf->handler_bundle_received_event(elementRoot, xmlRoot);
					break;
				case DATA_TRANSMITTED_EVENT:
					intf->handler_data_transmitted_event(elementRoot, xmlRoot);
					break;
				case BUNDLE_DELIVERED_EVENT:
					intf->handler_bundle_delivered_event(elementRoot, xmlRoot);
					break;
				case BUNDLE_DELIVERY_EVENT:
					intf->handler_bundle_delivery_event(elementRoot, xmlRoot);
					break;
				case BUNDLE_SEND_CANCELLED_EVENT:
					intf->handler_bundle_send_cancelled_event(elementRoot, xmlRoot);
					break;
				case BUNDLE_EXPIRED_EVENT:
					intf->handler_bundle_expired_event(elementRoot, xmlRoot);
					break;
				case BUNDLE_INJECTED_EVENT:
					intf->handler_bundle_injected_event(elementRoot, xmlRoot);
					break;
				case LINK_OPENED_EVENT:
					intf->handler_link_opened_event(elementRoot, xmlRoot);
					break;
				case LINK_CLOSED_EVENT:
					intf->handler_link_closed_event(elementRoot, xmlRoot);
					break;
				case LINK_CREATED_EVENT:
					intf->handler_link_created_event(elementRoot, xmlRoot);
					break;
				case LINK_DELETED_EVENT:
					intf->handler_link_deleted_event(elementRoot, xmlRoot);
					break;
				case LINK_AVAILABLE_EVENT:
					intf->handler_link_available_event(elementRoot, xmlRoot);
					break;
				case LINK_UNAVAILABLE_EVENT:
					intf->handler_link_unavailable_event(elementRoot, xmlRoot);
					break;
					default:
					if (HBSD::log->enabled(Logging::WARN)) 
					{
						HBSD::log->warn(string("Received unknown XML event from DTN server: ") + elementRoot->getName());
					}
					break;
				
			}
		} 
		catch (exception &e) 
		{
			if (HBSD::log->enabled(Logging::ERROR)) 
			{
				HBSD::log->error(string("Unhandled exception in message handler: ") + string(e.what()) + string("tmp: ") + tmp);
			}
		}
		
	}
}
