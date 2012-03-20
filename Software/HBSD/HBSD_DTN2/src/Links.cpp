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


#include "Links.h"
#include "Handlers.h"
#include "Link.h"
#include "XMLTree.h"
#include "HBSD.h"
#include "ConfigFile.h"
#include "Bundles.h"
#include "HBSD_Policy.h"
#include "Node.h"
#include <iostream>
#include "Nodes.h"
#include "HBSD_Routing.h"
using namespace std;

Links::Links(Handlers* router) 
{
	this->router = router;
	sem_init(&linksLock, 0, 1);
}

Links::~Links()
{
	router = NULL;
	while(!linkMap.empty())
	{
		map<string,Link*>::iterator iter = linkMap.begin();
		delete iter->second;
		linkMap.erase(iter);
	}
	sem_destroy(&linksLock);
}


Link* Links::create(XMLTree* evt) 
{
	assert(evt != NULL);
	// Get the link id.
	if (!evt->haveAttr(string("link_id"))) 
	{
		if(HBSD::log->enabled(Logging::ERROR))
			HBSD::log->error(string("Unable to find the link ID"));
		return NULL;
	}

	string id = evt->getAttr(string("link_id"));
	// Do we already know about the id? The key may exist from
	// a previous incarnation, but the object should be NULL.
	Link* link = getById(id);

	if (link != NULL) 
	{
		if (HBSD::log->enabled(Logging::WARN)) 
		{
			HBSD::log->warn(string("Ignoringing request to create an existing link: ") + id);
		}
		return NULL;
	}
	
	// Create the new link.
	link = new Link(this);
	assert(link != NULL);

	if (!link->init(evt)) 
	{
		if(HBSD::log->enabled(Logging::FATAL))
			HBSD::log->fatal(string("Unable to initialize the new Link"));
		return NULL;
	}

	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("Adding the new created link"));

	add(link);

	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("Notifying the HBSD_Policy about the creation of the new link"));

	router->policyMgr->linkCreated(link);

	return link;
}

void Links::deleteEvt(XMLTree* evt) 
{
	assert(evt != NULL);

	// Remove the link in a synchronized method.
	Link* link = remove(evt);

	// Call into the link to change its state to non existent
	link->deleted(evt);

	// Removing the node currently associated with the link
	((HBSD_Routing*)router)->nodes->removeNode(link->remoteEID);
	if(link != NULL)
		delete link;
}

// Synchronized
int Links::numLinks() 
{
	int tmp = 0;
	getLinksLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));
	tmp = linkMap.size();
	leaveLinksLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));
	return tmp;
}

// Synchronized
Link* Links::getById(string id) 
{
	assert(!id.empty());
	Link * tmp = NULL;
	getLinksLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));
	tmp = linkMap[id];;
	leaveLinksLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));
	return tmp;
}

// Synchronized
void Links::add(Link* link) 
{
	assert(link != NULL);

	getLinksLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));

	map<string,Link*>::iterator iter = linkMap.find(link->id);

	if (iter != linkMap.end()) 
	{
		if(iter->second != NULL)
			delete iter->second;
		linkMap.erase(iter);
	}

	linkMap[link->id] = link;

	leaveLinksLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));
}

// Synchronized
Link* Links::remove(XMLTree* evt) 	
{
	assert(evt != NULL);
	// Get the link id.
	if (!evt->haveAttr(string("link_id"))) 
	{
		if(HBSD::log->enabled(Logging::ERROR))
			HBSD::log->error(string("Error occurred @ Links::remove, can't resolve the link ID."));
		return NULL;
	}

	string id = evt->getAttr(string("link_id"));

	getLinksLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));

	// Do we know about it?
	Link* link =  linkMap[id];

	if (link == NULL) 
	{
		if (HBSD::log->enabled(Logging::WARN)) 
		{
			HBSD::log->warn(string("Ignoring request to delete a non-existent link: ") + id);
		}
		leaveLinksLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));

		return NULL;
	}
	// Now remove it from the hash table.

	linkMap.erase(id);

	leaveLinksLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));

	router->policyMgr->linkDeleted(link);

	return link;
}

// Synchronized
void Links::showAvailableLinks()
{
	getLinksLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));

	if(HBSD::log->enabled(Logging::INFO))
	{
		HBSD::log->info(string("Number of available links: ")+ Util::to_string(linkMap.size()));
		for(map<std::string,Link*>::iterator iter = linkMap.begin(); iter != linkMap.end(); iter++)
		{
			HBSD::log->info(string("Link Id: ") + iter->first + string(" state: ") + iter->second->getCurrentState() + string(" remote Node: ")+Util::to_string(iter->second->remoteEID));
		}
	}
	leaveLinksLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));

}

// Synchronized
void Links::InitiateESWithAvailableLinks()
{
	if(HBSD::log->enabled(Logging::INFO))
	{
		HBSD::log->info(string("Looking for available links and starting ES, number of available links: ") + Util::to_string(linkMap.size()) + string(" looking for opened ones."));
	}

	getLinksLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));

	for(map<std::string,Link*>::iterator iter = linkMap.begin(); iter != linkMap.end(); iter++)
	{
		// Starting a new ES, if the link is available
		if((iter->second)->state == 5 && (iter->second)->remoteEID.compare(HBSD::localEID) > 0)
		{
			HBSD_Routing * hbsdRouter = (HBSD_Routing*)this->router;
			Node * destNode = hbsdRouter->nodes->findNode((iter->second)->remoteEID);
			//if(destNode->getLastSessionTime() < hbsdRouter->bundles->getLastTimeBundlesStoreChanged())
			{
				if(HBSD::log->enabled(Logging::INFO))
					HBSD::log->info(string("Link : ")+iter->second->id+string(" available, starting an ES."));
				// Sending the summary vector of bundles

				string remoteRouter = iter->second->remoteEID + string("/") + HBSD::routerEndpoint;
				string reqId;
				if((reqId = HBSD::requester->requestInjectBundle(HBSD::hbsdRegistration, remoteRouter, iter->second->id, ((HBSD_Routing*)this->router)->bundles->createSV(string(EPIDEMIC_SV_1), true))).empty())
				{
					if(HBSD::log->enabled(Logging::ERROR))
						HBSD::log->error(string("Unable to send the bundles summary vector to the remote peer"));
				}else
				{
					if(HBSD::log->enabled(Logging::INFO))
						HBSD::log->info(string("Summary vector is correctly sent."));
					HBSD_Policy * policy = (HBSD_Policy*)((HBSD_Routing*)this->router)->policyMgr;
					// Informing HBSD_Policy about that
					policy->addNewInjectedRequest(reqId, iter->second->id, iter->second->remoteEID);
					policy = NULL;
				}
			}
			/*else
			{
				if(HBSD::log->enabled(Logging::INFO))
				{
					HBSD::log->info(string("No changes since we last met: ") + (iter->second)->remoteEID);
				}
			}
			*/
			destNode = NULL;
		}
	}

	leaveLinksLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));

}

void Links::getLinksLock(string place)
{
	if(LINKS_LOCK_DEBUG)
	{
		cout<< "Asking for links lock @: "<<place<<endl;
	}
	sem_wait(&linksLock);
}

void Links::leaveLinksLock(string place )
{
	if(LINKS_LOCK_DEBUG)
	{
		cout<< "Leaving links lock @: "<<place<<endl;
	}
	sem_post(&linksLock);
}
