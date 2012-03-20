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


#include "HBSD_Policy.h"
#include "Policy.h"
#include "HBSD.h"
#include <iostream>
#include "HBSD_Routing.h"
#include "Handlers.h"
#include "Bundle.h"
#include "Bundles.h"
#include "Nodes.h"
#include "Node.h"
#include "Links.h"
#include "Link.h"
#include "GBOF.h"
using namespace std;

HBSD_Policy::HBSD_Policy()
{
	sem_init(&injectedRecordsLock, 0, 1);
	sem_init(&injectedMeDeHaBundlesLock, 0, 1);
}

HBSD_Policy::~HBSD_Policy()
{
	sem_destroy(&injectedRecordsLock);
	sem_destroy(&injectedMeDeHaBundlesLock);

	while(!mapInjectedRequests.empty())
	{
		map<string, InjectedRequest *>::iterator iter = mapInjectedRequests.begin();
		delete iter->second;
		mapInjectedRequests.erase(iter);
	}

	while(!mapInjectedMeDeHaBundles.empty())
	{
		map<string, InjectedMeDeHaBundle *>::iterator iter = mapInjectedMeDeHaBundles.begin();
		delete iter->second;
		mapInjectedMeDeHaBundles.erase(iter);
	}

}

bool HBSD_Policy::init(Handlers* router)
{
	assert(router != NULL);
	this->router = router;
	return true;
}

void HBSD_Policy::nodeCreated(Node* node)
{
	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("New Node Created."));
}

bool HBSD_Policy::requestLinkOpen(Link* link, Node* node)
{
	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("Request to open a new Link."));
	return true;
}


void HBSD_Policy::linkCreated(Link* link)
{

}

void HBSD_Policy::linkDeleted(Link* link)
{
	if(HBSD::log->enabled(Logging::INFO))
			HBSD::log->info(string("Link Deleted."));
}

void HBSD_Policy::linkOpened(Link* link, Node* node)
{
	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("Link Opened."));
}

void HBSD_Policy::linkNotOpen(Link* link, Node* node)
{
	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("Link Not Open."));
}

void HBSD_Policy::linkSwitched(Link* link, Node* node)
{
	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("Link Switched."));
}

void HBSD_Policy::linkAssociated(Link* link, Node* node)
{
	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("Link Associated."));
}

void HBSD_Policy::bundleReceived(Bundle* bundle)
{
	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("A new bundle is received, dest: ") + bundle->destURI + string(" source: ") + bundle->sourceURI);
}

void HBSD_Policy::bundleDelivered(Bundle* bundle)
{
	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("New Metadata bundle is delivered."));
}

void HBSD_Policy::bundleExpired(Bundle* bundle)
{
	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("Bundle Expired."));
	// Delete the bundle from the local buffer, update the buffer size as well as the statistics
}

int HBSD_Policy::waitPolicy(Link* link, Node* node)
{
	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("Wait Policy."));
	return 1;
}

int HBSD_Policy::maxOutstandingTransmits(Link* link, Node* node, bool & sentMetaData, int & linkValue)
{
	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("maxOutstandingTransmits."));
	return 1;
}
	
bool HBSD_Policy::shouldTransmit(Link * link, Bundle * bundle, Node * node, int & queue)
{
	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("Should Transmit."));
	return true;
}

void HBSD_Policy::bundleTransmitStart(Link * link, Node * node, Bundle * bundle, bool & finalHop, bool & metaData, int queue)
{
	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("Bundle Transmit Start."));
}
	
void HBSD_Policy::bundleTransmitStop(Link * link, Node * node, std::string localId, int bytesSent, int reliablySent)
{
	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("Bundle Transmit Stop."));
}


int HBSD_Policy::outstandingTransmits(Link * link, Node * node)
{
	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("outstandingTransmits."));
	return 1;
}

void HBSD_Policy::bundleInjected(std::string localId, std::string reqId, Bundle * bundle)
{
	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("bundle injected."));
	// It is a Meta Data bundle, asking DTN to forward the injected Bundle
	askDtndToForwardTheInjectedBundle(reqId, bundle);
}

// Called by PeerListener
void HBSD_Policy::metaDataReceived(string src, string data, string type)
{
try
{
	// For the moment compare the summary vectors and send
	HBSD_Routing * hbsdRouter = (HBSD_Routing*)this->router;
	assert(hbsdRouter != NULL);
	assert(!type.empty());
	// Get the source of the metaData file
	Node * destNode = hbsdRouter->nodes->findNode(src);

	if(destNode != NULL)
	{
		// Get the link actually associated with the source node
		Link * linkToDest = destNode->getAssociatedLink();
		// Answer the received metadata file
		if(type.compare(string(EPIDEMIC_SV_1)) == 0)
		{
			if(HBSD::log->enabled(Logging::INFO))
				HBSD::log->info(string("Meta data received EPIDEMIC_SV_1: ") + string(data));

			// Sends back the requested bundles as well as our Epidemic sV if needed
			hbsdRouter->bundles->compareAndSend(data, linkToDest, true);
		}else if(type.compare(string(EPIDEMIC_SV_2)) == 0)
		{
			if(HBSD::log->enabled(Logging::INFO))
				HBSD::log->info(string("Meta data received EPIDEMIC_SV_2: ") + string(data));

			// Just sends back the requested bundles if still existing
			hbsdRouter->bundles->compareAndSend(data, linkToDest, false);

		}else
		{
			if(HBSD::log->enabled(Logging::ERROR))
			{
				HBSD::log->error(string("Unknown metaData message type in HBSD_Policy::metaDataReceived type: ")+type);
			}
		}

		destNode->updateLastSessionTime();
		linkToDest = NULL;
		destNode = NULL;
	}
	hbsdRouter = NULL;
}
catch(exception &e)
{
	cout <<" Exception occured :"<<e.what()<<endl;
	exit(1);
}
}

bool HBSD_Policy::deliveryIsEmpty(Node * node)
{
	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("Delivery is empty."));
	return true;
}
bool HBSD_Policy::replicaIsEmpty(Node * node)
{
	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("Replica is empty."));
	return true;
}

bool HBSD_Policy::metaDataIsEmpty(Node * node)
{
	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("Meta data is empty."));
	return true;
}

Bundle * HBSD_Policy::deliveryPoll(Node * node)
{
	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("Delivery Poll."));
	return NULL;
}

Bundle * HBSD_Policy::replicaPoll(Node * node)
{
	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("Replica Poll."));
	return NULL;
}

Bundle * HBSD_Policy::metaDataPoll(Node *  node)
{
	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("Meta Data poll."));
	return NULL;
}


	
bool HBSD_Policy::localDest(Bundle *bundle)
{
	assert(bundle != NULL);
	string me = HBSD::localEID + "/";
	// Ignore endpoint.
	string destEID = bundle->destURI.substr(0, me.length());
	return (destEID.compare(me) == 0);
}


void HBSD_Policy::addNewInjectedRequest(string reqId, string linkId, string destNode)
{
	// Try to find the Injected Request
	sem_wait(&injectedRecordsLock);

	if(mapInjectedRequests.empty())
	{
		InjectedRequest * ir = (InjectedRequest*)malloc(sizeof(InjectedRequest));
		strcpy(ir->destNode, (char*)destNode.c_str());
		strcpy(ir->linkId, (char*)linkId.c_str());
		mapInjectedRequests[reqId] = ir;
		ir = NULL;
	}else
	{
		map<string, InjectedRequest*>::iterator iter = mapInjectedRequests.find(reqId);
		if(iter == mapInjectedRequests.end())
		{
			InjectedRequest * ir = (InjectedRequest*)malloc(sizeof(InjectedRequest));
			strcpy(ir->destNode, (char*)destNode.c_str());
			strcpy(ir->linkId, (char*)linkId.c_str());
			mapInjectedRequests[reqId] = ir;
			ir = NULL;
		}else
		{
			if(HBSD::log->enabled(Logging::ERROR))
			{
				HBSD::log->error(string("HBSD_Policy: error occurred when trying to add MeDeHa request: ")+reqId);
				sem_post(&injectedRecordsLock);
				exit(1);
			}
		}
	}
	sem_post(&injectedRecordsLock);
}

void HBSD_Policy::askDtndToForwardTheInjectedBundle(std::string reqId, Bundle * bundle)
{
	assert(bundle!=NULL);

	sem_wait(&injectedRecordsLock);

	map<string, InjectedRequest*>::iterator iter = mapInjectedRequests.find(reqId);

	if(iter != mapInjectedRequests.end())
	{
		if(HBSD::requester->requestSendBundle(bundle, (iter->second)->linkId, Requester::FWD_ACTION_FORWARD))
		{
			// delete the pending request
			if(iter->second != NULL)
				free(iter->second);
			// All is ok
			mapInjectedRequests.erase(iter);
			// Delete the track we have about the injected Bundle
			if(bundle != NULL)
				delete bundle;
		}else
		{
			if(HBSD::log->enabled(Logging::ERROR))
			{
				HBSD::log->error(string("HBSD_Policy: error occurred when trying to request sending an already injected Bundle."));
			}
		}
	}
	else
	{
		if(HBSD::log->enabled(Logging::ERROR))
		{
			HBSD::log->error(string("HBSD_Policy: Invalid pending request ID"));
			cout<<"Available ids:"<<endl;
			for(map<string, InjectedRequest*>::iterator iter = mapInjectedRequests.begin();iter!=mapInjectedRequests.end();iter++)
			{
				cout<<"ReqId: "<<iter->first<<endl;
			}
		}
	}

	sem_post(&injectedRecordsLock);
}

void HBSD_Policy::addNewMeDeHaInjectedBundle(string reqId, string sourceNode, string destNode)
{
	// Try to find the Injected Request
	sem_wait(&injectedMeDeHaBundlesLock);

	if(mapInjectedMeDeHaBundles.empty())
	{
		InjectedMeDeHaBundle * ir = (InjectedMeDeHaBundle *)malloc(sizeof(InjectedMeDeHaBundle));
		strcpy(ir->destNode, (char*)destNode.c_str());
		strcpy(ir->sourceNode, (char*)sourceNode.c_str());
		mapInjectedMeDeHaBundles[reqId] = ir;
		ir = NULL;
	}else
	{
		map<string, InjectedMeDeHaBundle*>::iterator iter = mapInjectedMeDeHaBundles.find(reqId);
		if(iter == mapInjectedMeDeHaBundles.end())
		{
			InjectedMeDeHaBundle * ir = (InjectedMeDeHaBundle*)malloc(sizeof(InjectedMeDeHaBundle));
			strcpy(ir->destNode, (char*)destNode.c_str());
			strcpy(ir->sourceNode, (char*)sourceNode.c_str());
			mapInjectedMeDeHaBundles[reqId] = ir;
			ir = NULL;
		}else
		{
			if(HBSD::log->enabled(Logging::ERROR))
			{
				HBSD::log->error(string("HBSD_Policy: error occurred when trying to add MeDeHa request: ")+reqId);
				sem_post(&injectedMeDeHaBundlesLock);
				exit(1);
			}
		}
	}

	sem_post(&injectedMeDeHaBundlesLock);
}

void HBSD_Policy::addTheMeDeHaInjectedBundle(std::string reqId, Bundle * bundle)
{

	assert(bundle!=NULL);

	sem_wait(&injectedMeDeHaBundlesLock);

	map<string, InjectedMeDeHaBundle*>::iterator iter = mapInjectedMeDeHaBundles.find(reqId);

	if(iter != mapInjectedMeDeHaBundles.end())
	{
		// All is ok, adding the bundle
		bundle->destURI.assign(iter->second->destNode);
		bundle->destEID = GBOF::eidFromURI(bundle->destURI);
		bundle->injected = false;

		((HBSD_Routing*)router)->bundles->addIfNew(bundle, bundle->localId);

		// adding the dest node
		((HBSD_Routing*)router)->addDestNode(bundle);

		if(iter->second)
			free(iter->second);

		// Remove the request
		mapInjectedMeDeHaBundles.erase(iter);
	}
	else
	{
		if(HBSD::log->enabled(Logging::ERROR))
		{
			HBSD::log->error(string("HBSD_Policy: Invalid pending MeDeHa request ID"));
			cout<<"Available ids:"<<endl;
			for(map<string, InjectedMeDeHaBundle*>::iterator iter = mapInjectedMeDeHaBundles.begin();iter!=mapInjectedMeDeHaBundles.end();iter++)
			{
				cout<<"ReqId: "<<iter->first<<endl;
			}
		}
	}

	sem_post(&injectedMeDeHaBundlesLock);
}
