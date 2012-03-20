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


#ifndef HBSD_SAX_H
#define HBSD_SAX_H

#include <string>
#include <map>
#include <stack>
#include <xercesc/sax2/DefaultHandler.hpp>
#include <xercesc/sax2/Attributes.hpp>
#include "XMLTree.h"

class Handlers;

// The HBSD SAX Handler used to handle the events once a new XML message is received from the DTN2 and parsed
// HBSD_SAX forwards the events to HBSD_Routing
class HBSD_SAX: public DefaultHandler
{

public:
	/**
	 * Constructor: Create stack that keeps track of current and
	 * parent elements. Define switch statement values for XML
	 * elements.
	 * 
	 * @param rtrHandlers Router interfaces for handling requests. 
	 */
	HBSD_SAX(Handlers *rtrHandlers);

	~HBSD_SAX();
	/**
	 * Called when a new element is encountered by the parser. We always
	 * build the tree with the <bpa> element as its root. Ignore any parent
	 * elements of <bpa>.
	 *
	 * @param uri Will be empty.
	 * @param name Will be empty.
	 * @param qName The element's name.
	 * @param atts An array of key/value pairs made up of the element's
	 *        attributes.
	 */
	void startElement (const   XMLCh* const uri, const   XMLCh* const    localname, const   XMLCh* const qname, const Attributes& attrs);
	/**
	 * Called for characters between elements, e.g. <element>cccc</element>
	 * 
	 * @param ch Array of characters containing the data.
	 * @param start Index into ch[] of first character.
	 */
	void characters(const   XMLCh* const ch, const unsigned int start);

	/**
	 * Called when the end of an element is encountered by the parser.
	 * This method is called for tag pairs (<element>...</element>) and
	 * single elements (<element/>).
	 *
	 * @param uri Will be empty.
	 * @param name Will be empty.
	 * @param qName The element's name.
	 */
	 void endElement( const XMLCh* const uri, const XMLCh* const localname, const XMLCh* const qname);
	/**
	 * Called when the parser begins parsing the XML document.
	 */
	void startDocument ();

	/**
	 * Called when we're done with the document. Invoke the appropriate
	 * router event handler method. Reset data structures to their initial
	 * states.
	 */
	void endDocument ();

	static std::string BPA_ELEMENT;
private:	

	/**
	 * Called when the parsed XML contains only a root element (<bpd>),
	 * which may contain attributes. 
	 */
	void rootOnly();
	
	/**
	 * Called when the root element (<bpa>) contains children elements.
	 * The router's methods are invoked to handle the children elements.
	 * 
	 * It *may* be more efficient to invoke the router when we get the
	 * close of each immediate BPA child element. But that's for if and
	 * when it is shown to be more efficient.
	 *  
	 * @param numElements Number of immediate children elements.
	 */
	void callRouter(int numElements);
	XMLTree *xmlRoot;
	std::stack<XMLTree*> elementStack;
	Handlers * intf;
	std::map<std::string, int> eventHash;
	
	// All known events that we may receive.
	const static int BUNDLE_RECEIVED_EVENT           = 0;
	const static int DATA_TRANSMITTED_EVENT          = 1;
	const static int BUNDLE_DELIVERED_EVENT          = 2;
	const static int BUNDLE_DELIVERY_EVENT           = 3;
	const static int BUNDLE_SEND_CANCELLED_EVENT     = 4;
	const static int BUNDLE_EXPIRED_EVENT            = 5;
	const static int BUNDLE_INJECTED_EVENT           = 6;
	const static int LINK_OPENED_EVENT               = 7;
	const static int LINK_CLOSED_EVENT               = 8;
	const static int LINK_CREATED_EVENT              = 9;
	const static int LINK_DELETED_EVENT              = 10;
	const static int LINK_AVAILABLE_EVENT            = 11;
	const static int LINK_UNAVAILABLE_EVENT          = 12;
	const static int LINK_ATTRIBUTE_CHANGED_EVENT    = 13;
	const static int CONTACT_ATTRIBUTE_CHANGED_EVENT = 14;
	const static int LINK_BUSY_EVENT                 = 15;
	const static int EID_REACHABLE_EVENT             = 16;
	const static int ROUTE_ADD_EVENT                 = 17;
	const static int ROUTE_DELETE_EVENT              = 18;
	const static int CUSTODY_SIGNAL_EVENT            = 19;
	const static int CUSTODY_TIMEOUT_EVENT           = 20;
	const static int INTENTIONAL_NAME_RESOLVED_EVENT = 21;
	const static int REGISTRATION_ADDED_EVENT        = 22;
	const static int REGISTRATION_REMOVED_EVENT      = 23;
	const static int REGISTRATION_EXPIRED_EVENT      = 24;
	const static int BUNDLE_REPORT                   = 25;
	const static int LINK_REPORT                     = 26;
	const static int LINK_ATTRIBUTES_REPORT          = 27;
	const static int ROUTE_REPORT                    = 28;
	const static int CONTACT_REPORT                  = 29;
	const static int BUNDLE_ATTRIBUTES_REPORT        = 30;
};
#endif
