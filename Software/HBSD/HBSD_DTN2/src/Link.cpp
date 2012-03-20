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


#include "Link.h"
#include "XMLTree.h"
#include "HBSD.h"
#include "Node.h"
#include <iostream>
#include "Policy.h"
#include "Links.h"
#include "Util.h"

using namespace std;

Link::Link(Links *links) 
{
	openRequested = false;
	type = TYPE_OTHER;
	state = STATE_NONEXTANT;
	// State should be taken from the state variables rather then this element.
	linkManager = links;
}

Link::~Link()
{
	linkManager = NULL;
}
	
void Link::available(XMLTree *element) 
{
	openRequested = false;
	stateChange(STATE_AVAILABLE);
}


void Link::opened(XMLTree *element) 
{
	assert(element != NULL);
	openRequested = false;
	stateChange(STATE_OPEN);

	// The following sets the remoteEID.
	findAndSaveAttributesContact(element);

	// Associate the destination node with this link. It's possible that
	// the node is already associated with another link, which can
	// happen due to opportunistic links. If that's the case we let the
	// link remain open with no associated node. We do have logic
	// elsewhere that allows a node to attach later on.
	Node* openedNode = linkManager->router->nodes->conditionalAdd(remoteEID);
	if (!openedNode->setLink(this)) 
	{
		openedNode = NULL;
	}
	currentNode = openedNode;
}


void Link::closed(XMLTree *element) 
{
	findAndSaveAttributesContact(element);
	if(HBSD::log->enabled(Logging::INFO))
	{
		HBSD::log->info(string("The link: ") + this->id + string(" is closed."));
	}
	openRequested = false;
	stateChange(STATE_CLOSED);
}


bool Link::associate(Node *node)
{
	attachNode = node;
	node->setLink(this);
	linkManager->router->policyMgr->linkAssociated(this, node);
	return true;
}

void Link::requestOpen() 
{
	// Do we already have an open request pending?
	if (openRequested) 
	{
		if(HBSD::log->enabled(Logging::ERROR))
			HBSD::log->error(string("An open request is pending"));
		return;
	}

	// Only states we assume we can request an open from.
	if ((state != STATE_AVAILABLE) && (state != STATE_CLOSED))
	{
		if(HBSD::log->enabled(Logging::ERROR))
			HBSD::log->error(string("Invalid link state to request an open for"));
		return;
	}

	// Only types we assume we request an open for.
	if ((type != TYPE_ALWAYSON) && (type != TYPE_ONDEMAND) && (type != TYPE_OPPORTUNISTIC))
	{
		if(HBSD::log->enabled(Logging::ERROR))
			HBSD::log->error(string("Invalid link type to request an open for, type: ")+Util::to_string(type));
		return;
	}

	// Send a request to open the link to DTN and mark the
	// link as having a request outstanding. We expect DTN to
	// eventually respond with something, e.g. this link is
	// unavailable or open, and we mark the request as not being
	// outstanding whenever we hear something.
	if(!HBSD::requester->requestOpenLink(id))
	{
		if(HBSD::log->enabled(Logging::ERROR))
			HBSD::log->error(string("Request to open the link failed"));
	}

	openRequested = true;
}

bool Link::init(XMLTree *evtLinkCreated) 
{
	assert(evtLinkCreated != NULL);
	// There should be an id, i.e. the link should be named.
	if (evtLinkCreated->haveAttr(string("link_id"))) 
	{
		id = evtLinkCreated->getAttr(string("link_id"));
	} else 
	{
		if(HBSD::log->enabled(Logging::FATAL))
			HBSD::log->fatal(string("Cannot extract the link ID"));
		return false;
	}

	// One child element should be the link attributes (link_attr).
	// Find it and get the attributes that we initially care about.
	// Save the entire element for possible future use.
	try 
	{
		XMLTree* el = evtLinkCreated->getChildElementRequired(string("link_attr"));
		assert(el != NULL);
		if (!parseLinkAttr(el) )
		{
			if(HBSD::log->enabled(Logging::FATAL))
				HBSD::log->fatal(string("Cannot parse the link_attr"));
			return false;
		}

		el = NULL;

		if (state >= STATE_AVAILABLE)
		{
			if(HBSD::log->enabled(Logging::INFO))
				HBSD::log->info(string("Link available, starting the thread"));
		}
		return true;
	} 
	catch (exception& e) 
	{
		if(HBSD::log->enabled(Logging::ERROR))
					HBSD::log->fatal(string("Exception occurred @ Link::init ") + string(e.what()));
		return false;
	}
}

bool Link::parseLinkAttr(XMLTree *element) 
{
	assert(element != NULL);
	try 
	{
		string str = element->getAttrRequired(string("type"));
		type = whatType(str);
		str = element->getAttrRequired(string("state"));
		state = whatState(str);

		return true;
	} 
	catch (exception & e) 
	{
		if(HBSD::log->enabled(Logging::ERROR))
					HBSD::log->fatal(string("Exception occurred @ Link::parseLinkAttr ") + string(e.what()));
		return false;
	}
}


void Link::stateChange(int newState) 
{
	if (newState == state) 
	{
		return;
	}

	bool toOpenFromClosed = false;

	if ((newState >= STATE_OPEN) && (state < STATE_OPEN)) 
	{
		// Non-open to open.
		toOpenFromClosed = true;
		// Create a temporal route.
		// Tell the policy manager about the link opening before
		// telling the thread.
		linkManager->router->policyMgr->linkOpened(this, currentNode);

	} else if ((state >= STATE_OPEN) && (newState <STATE_OPEN)) 
	{
		// Open to non-open.
		toOpenFromClosed = false;

		// Un-associate link and node.
		if (currentNode != NULL) 
		{
			currentNode->clearLink();
			currentNode = NULL;
		}
	}

	// Setting the new state
	state = newState;

}
	


//HBSD::requester->requestSendBundle(bundle, id, Requester::FWD_ACTION_COPY);
void Link::findAndSaveAttributesContact(XMLTree *event) 
{
	assert(event != NULL);
	try 
	{
		XMLTree* el = event->getChildElementRequired(string("contact_attr"));
		assert(el != NULL);
		findAndSaveAttributesLink(el);
	} 
	catch (exception& e) 
	{
		cerr<<"Exception occured in Link::findAndSaveAttributesContact: "<<e.what()<<endl;
	}
}


void Link::findAndSaveAttributesLink(XMLTree *event) 
{
	assert(event != NULL);
	try 
	{
		XMLTree* el = event->getChildElementRequired(string("link_attr"));
		assert(el != NULL);
		findAndSaveAttributesRemoteEID(el);
	} 
	catch (exception& e) 
	{
		if(HBSD::log->enabled(Logging::ERROR))
					HBSD::log->fatal(string("Exception occurred @ Link::findAndSaveAttributesLink ") + string(e.what()));
	}
}



void Link::findAndSaveAttributesRemoteEID(XMLTree *event) 
{
	assert(event != NULL);
	try 
	{
		//XMLTree* el = event->getChildElementRequired(string("clinfo"))->getChildElementRequired(string("remote_eid"));

		XMLTree* el = event->getChildElementRequired(string("remote_eid"));
		assert(el != NULL);
		string uri = el->getAttrRequired(string("uri"));
		if (remoteEID.empty()) 
		{
			remoteEID = uri;
		}
		else if (uri.compare(remoteEID) != 0)
		{
			remoteEID = uri;
		}
	}

	catch (exception & e) 
	{
		if(HBSD::log->enabled(Logging::ERROR))
					HBSD::log->fatal(string("Exception occurred @ Link::findAndSaveAttributesRemoteEID ") + string(e.what()));
	}
}



int Link::whatState(string str) 
{
	if (str.compare("available") == 0) 
	{
		return STATE_AVAILABLE;
	}
	if (str.compare("open") == 0) 
	{
		return STATE_OPEN;
	}
	if (str.compare("opening") == 0) 
	{
		return STATE_OPENING;
	}
	if (str.compare("busy") == 0) 
	{
		return STATE_BUSY;
	}
	return STATE_UNAVAILABLE;
}

int Link::whatType(string str) 
{
	if (str.compare("alwayson") == 0) 
	{
		return TYPE_ALWAYSON;
	}
	if (str.compare("ondemand") == 0) 
	{
		return TYPE_ONDEMAND;
	}
	if (str.compare("scheduled") == 0) 
	{
		return TYPE_SCHEDULED;
	}
	if (str.compare("opportunistic") == 0) 
	{
		return TYPE_OPPORTUNISTIC;
	}
	return TYPE_OTHER;
}

string Link::getCurrentState()
{
	switch(state)
	{
	case 0:
		return string("State Non Existant");
		break;
	case 1:
		return string("UNAVAILABLE");
		break;
	case 2:
		return string("AVAILABLE");
		break;
	case 3:
		return string("CLOSED");
		break;
	case 4:
		return string("OPENING");
		break;
	case 5:
		return string("OPEN");
		break;
	case 6:
		return string("BUSY");
		break;
	default:
		return string("Invalid State");
	};
}
