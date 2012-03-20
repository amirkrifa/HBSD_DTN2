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


#include "HBSD_Routing.h"
#include <exception>
#include <stdlib.h>
#include "Bundle.h"
#include "Bundles.h"
#include "XMLTree.h"
#include "HBSD.h"
#include <exception>
#include "Link.h"
#include "ConfigFile.h"
#include "HBSD_Policy.h"
#include "GBOF.h"
#include "Nodes.h"
#include "Policy.h"
#include "PeerListener.h"
#include "Requester.h"
#include <iostream>
#include "MeDeHaInterface.h"
#include <math.h>
#include <fstream>
using namespace std;

string HBSD_Routing::BPA_ATTR_EID;
string HBSD_Routing::defaultPolicy;


HBSD_Routing::HBSD_Routing()
{
	BPA_ATTR_EID.assign("eid");
	defaultPolicy.assign("HBSD_Policy");

	// Load and initialize both the router's policy manager and the statistics manager.
	string routerPolicyClassName = HBSD::routerConf->getstring("routerPolicyClass", defaultPolicy);
	enableHbsdOptimization = HBSD::routerConf->getBoolean("enableHbsdOptimization", DEFAULT_RUN_HBSD_OPTIMIZATION);

	try 
	{
		policyMgr = new HBSD_Policy();
		statisticsManager = new StatisticsManager();
		if (!policyMgr->init(this)) 	
		{
			if(HBSD::log->enabled(Logging::FATAL))
				HBSD::log->fatal(string("Unable to initialize Policy Manager: ") +	routerPolicyClassName);
			exit(1);
		}

		// Verifying whether to activate or not the MeDeHa interface
		enableMeDeHaInterface = HBSD::routerConf->getBoolean(string("enableMeDeHaInterface"), ENABLE_MEDEHA_INTERFACE);
		if(enableMeDeHaInterface)
		{
			HBSD::log->info(string("MeDeHa interface enabled."));
			medehaInterface = new MeDeHaInterface(this);
			assert(medehaInterface != NULL);
			// Initializing MeDeHAInterface object;
			medehaInterface->init();
		}else
		{
			HBSD::log->info(string("The MeDeHa interface is disabled."));
		}

	} 

	catch (exception& e) 
	{
		if (HBSD::log->enabled(Logging::ERROR)) 
		{
			HBSD::log->error("Unable to load the Policy Manager: " + string(e.what()));
		}
		exit(1);
	}
}

HBSD_Routing::~HBSD_Routing()
{
	// Shutting down the peerListener object
	if(peerListener != NULL);
	{
		peerListener->stopThread();
		delete peerListener;
	}

	// Shutting down the MeDeHa interface
	if(enableMeDeHaInterface)
		this->medehaInterface->stopsInterfaceThread();

	if(medehaInterface != NULL)
		delete medehaInterface;
	if(bundles != NULL)
		delete bundles;
	if(links != NULL)
		delete links;
	if(nodes != NULL)
		delete nodes;
	if(policyMgr != NULL)
		delete ((HBSD_Policy*)policyMgr);
	if(statisticsManager != NULL)
		delete statisticsManager;
}

void HBSD_Routing::initialized() 
{
	peerListener->init();

	// What we would like to do is get bundle, link and route reports so that we can populate
	// our tables if HBSD were to be started after dtnd. Unfortunately, link reports don't
	// provide the link_id so their usefulness is questionable. Bundle reports are only partially
	// useful, since the local_id cannot be ascertained unless the bundle was locally initiated.
	// Regardless, we make our requests.

	HBSD::requester->queryLink();
	HBSD::requester->queryBundle();
	HBSD::requester->queryRoute();
}

void HBSD_Routing::handler_bpa_root(XMLTree *bpa) 
{
	assert(bpa != NULL);

	if (HBSD::localEID.empty()) 
	{
		firstMessage(bpa);
	}
	
	if(bpa->haveAttr(string("hello_interval")))
	{
		// If there is some links up then start an Epidemic session
		if(HBSD::log->enabled(Logging::INFO))
			HBSD::log->info(string("Hello message received from dtnd."));
		links->InitiateESWithAvailableLinks();
	}

	if (bpa->haveAttr(string("alert"))) 
	{
		// An alert is received from DTN2 daemon

		if(string("shuttingDown").compare(bpa->getAttr(string("alert"))) == 0) 
		{
			bool verbose = HBSD::log->enabled(Logging::INFO);
			if (HBSD::routerConf->getBoolean(string("terminateWithDTN"),true)) 
			{
				if (verbose) 
				{
					if(HBSD::log->enabled(Logging::INFO))
						HBSD::log->info(string("DTN daemon is terminating; HBSD will now terminate"));
				}

				// Shutting down the MeDeHa interface
				if(enableMeDeHaInterface)
					this->medehaInterface->stopsInterfaceThread();

				exit(0);
			}
			if (verbose) 
			{
				HBSD::log->info(string("DTN daemon is terminating; HBSD will continue to execute"));
			}
		}
	}
}

void HBSD_Routing::handler_bundle_received_event(XMLTree* event, XMLTree* bpa) 
{
	assert(event != NULL);
	assert(bpa != NULL);

	try
	{
		if (HBSD::localEID.empty())
		{
			firstMessage(bpa);
		}

		// Create the new bundle.
		Bundle* bundle = bundles->newBundle(event);

		if (bundle == NULL)
		{
			// We can get here if it's a special bundle for this router.
			return;
		}

		addDestNode(bundle);

		policyMgr->bundleReceived(bundle);
	}
	catch (exception &e)
	{
		cout<<"Exception occurred at HBSD_Routing::handler_bundle_received_event: "<<e.what()<<endl;
		exit(1);
	}
}


void HBSD_Routing::handler_data_transmitted_event(XMLTree* event, XMLTree* bpa) 
{
	assert(event != NULL);
	assert(bpa != NULL);

	if (HBSD::localEID.empty())
	{
		firstMessage(bpa);
	}

	try 
	{
		string linkId = event->getAttrRequired(string("link_id"));
		Link* link = links->getById(linkId);
		if (link == NULL) 
		{
			if (HBSD::log->enabled(Logging::ERROR)) 
			{
				HBSD::log->error("Unable to locate link id to complete transmission: " + linkId);
			}
			return;
		}

	} 
	catch (exception & e) 
	{
			if(HBSD::log->enabled(Logging::ERROR))
			{
				HBSD::log->error(string("Error occurred within HBSD_Routing::handler_data_transmitted_event: ") + string(e.what()));
			}
	}
}



void HBSD_Routing::handler_bundle_delivered_event(XMLTree* event, XMLTree* bpa) 
{
	assert(event != NULL);
	assert(bpa != NULL);

	if (HBSD::localEID.empty()) 
	{
		firstMessage(bpa);
	}

	Bundle* bundle = bundles->eventDelivered(event);

	if (bundle != NULL) 
	{
		policyMgr->bundleDelivered(bundle);
	}
	bundle = NULL;
}


void HBSD_Routing::handler_bundle_delivery_event(XMLTree* event, XMLTree* bpa) 
{
	assert(event != NULL);
	assert(bpa != NULL);

	if (HBSD::localEID.empty()) 
	{
		firstMessage(bpa);
	}
	
	Bundle* bundle = bundles->newBundleDelivery(event);
	if (bundle == NULL) 
	{
		return;
	}

	// If we inject a meta data bundle we'll see it just as if had been
	// sent to us from another EID. If we are the source, ignore it.
	if (localSource(bundle)) 
	{
		if (HBSD::log->enabled(Logging::DEBUG)) 
		{
			HBSD::log->debug(string("Ignoring HBSD routing originated by this node"));
		}
		return;
	}

	// The meta data bundle is then forwarded to the peerListener object which will
	// take care of extracting the meta data information and trigger the HBSD_Policy object
	// in order to process the received meta data
	peerListener->eventDelivery(event, bundle);
	bundle = NULL;
}


void HBSD_Routing::handler_bundle_expired_event(XMLTree* event, XMLTree* bpa) 
{
	assert(event != NULL);
	assert(bpa != NULL);

	if (HBSD::localEID.empty()) 
	{
		firstMessage(bpa);
	}

	Bundle* bundle = bundles->eventExpired(event);

	if (bundle != NULL) 
	{
		policyMgr->bundleExpired(bundle);
	}
	if(bundle != NULL)
		delete bundle;
}


void HBSD_Routing::handler_bundle_injected_event(XMLTree* event, XMLTree* bpa) 
{
	assert(event != NULL);
	assert(bpa != NULL);

	if (HBSD::localEID.empty()) 
	{
		firstMessage(bpa);
	}
	
	try 
	{
		Bundle* bundle = bundles->newInjectedBundle(event);

		if(bundle->isMeDeHaBundle())
		{
			if(HBSD::log->enabled(Logging::INFO))
			{
				HBSD::log->info(string("The injected bundle is a MeDeHa one, considering it as a new received one."));
			}
			// Redirect and consider it as a received one
			bundle->injected = false;
			XMLTree* el = event->getChildElementRequired(string("request_id"));
			((HBSD_Policy*)policyMgr)->addTheMeDeHaInjectedBundle(el->getValue(), bundle);
			bundles->showAvailableBundles();

		}else
		{
			if (bundle != NULL)
			{
				XMLTree* el = event->getChildElementRequired(string("request_id"));
				bundle->injected = true;
				policyMgr->bundleInjected(bundle->localId, el->getValue(), bundle);

			}
		}
	} 
	catch (exception& e) 
	{
		if(HBSD::log->enabled(Logging::ERROR))
			HBSD::log->error(string("Error parsing injected bundle element: ") + string(e.what()));
		exit(1);
	}
}

void HBSD_Routing::handler_link_opened_event(XMLTree* event, XMLTree* bpa) 
{
	assert(event != NULL);
	assert(bpa != NULL);

	if (HBSD::localEID.empty()) 
	{
		firstMessage(bpa);
	}

	Link* link = linkStructure(event);

	if (link != NULL) 
	{
		link->opened(event);

		assert(!link->remoteEID.empty());
		assert(!link->id.empty());

		//Identify the node who has the bigger ID and send the Epidemic summary vector of messages
		if(link->remoteEID.compare(HBSD::localEID) > 0)
		{
			// Sending a request to DTN2 to inject the SV bundle
			string reqId;

			string remoteRouter = link->remoteEID + string("/") + HBSD::routerEndpoint;
			reqId = HBSD::requester->requestInjectBundle(HBSD::hbsdRegistration, remoteRouter, link->id, bundles->createSV(string(EPIDEMIC_SV_1), true));
			if(reqId.empty())
			{
						if(HBSD::log->enabled(Logging::ERROR))
					HBSD::log->error(string("Unable to send the bundles summary vector to the remote peer"));
			}else
			{
				if(HBSD::log->enabled(Logging::INFO))
					HBSD::log->info(string("Summary vector is correctly sent."));
				// Informing HBSD_Policy about that
				((HBSD_Policy*)policyMgr)->addNewInjectedRequest(reqId, link->id, link->remoteEID);
			}

		}

	} else 
	{
		if(HBSD::log->enabled(Logging::ERROR))
			HBSD::log->error(string("Opened unknown link"));
	}
	link = NULL;
}
	
void HBSD_Routing::handler_link_closed_event(XMLTree* event, XMLTree* bpa) 
{
	assert(event != NULL);
	assert(bpa != NULL);

	if (HBSD::localEID.empty()) 
	{
		firstMessage(bpa);
	}
	
	Link* link = linkStructure(event);

	if (link != NULL) 
	{
		link->closed(event);
		link = NULL;
	} else 
	{
		if(HBSD::log->enabled(Logging::ERROR))
			HBSD::log->error(string("Closed unknown link"));
	}
}


void HBSD_Routing::handler_link_created_event(XMLTree* event, XMLTree* bpa)
{
	assert(event != NULL);
	assert(bpa != NULL);

	if (HBSD::localEID.empty()) 
	{
		firstMessage(bpa);
	}

	links->create(event);
}

void HBSD_Routing::handler_link_deleted_event(XMLTree* event, XMLTree* bpa) 
{
	assert(event != NULL);
	assert(bpa != NULL);

	if (HBSD::localEID.empty()) 
	{
		firstMessage(bpa);
	}
	links->deleteEvt(event);
}



void HBSD_Routing::firstMessage(XMLTree* bpa) 
{
	assert(bpa != NULL);

	if (!HBSD::localEID.empty()) 
	{
		return;
	}
	// Initializing the local node EID starting from the received message
	if (bpa->haveAttr(BPA_ATTR_EID)) 
	{
		HBSD::setLocalEID(bpa->getAttr(BPA_ATTR_EID));
	} else 
	{
		return;
	}
}


void HBSD_Routing::addDestNode(Bundle* bundle) 
{
	assert(bundle != NULL);

	if (!bundle->destEID.empty()) 
	{
		nodes->conditionalAdd(bundle->destEID);
	}
}

// Says whether the bundle comes from a local source application or not
bool HBSD_Routing::localSource(Bundle* bundle) 
{
	assert(bundle != NULL);
	try 
	{
		string me = HBSD::localEID + string("/");
		// Ignore endpoint.
		string sourceEID = bundle->sourceURI.substr(0, me.length());
		if (sourceEID.compare(me) == 0) 
		{
			return true;
		}
	
		return false;
	} 
	catch (exception& e) 
	{
		if(HBSD::log->enabled(Logging::ERROR))
		{
			HBSD::log->error(string("An exception occurred in HBSD_Routing::localSource: ") + string(e.what()));
		}
		return false;
	}
}


// Says whether the bundle comes from a local source application or not
bool HBSD_Routing::localDest(Bundle* bundle)
{
	assert(bundle != NULL);
	try
	{
		string me = HBSD::localEID + string("/");
		// Ignore endpoint.
		string destEID = bundle->destURI.substr(0, me.length());
		if (destEID.compare(me) == 0)
		{
			return true;
		}

		return false;
	}
	catch (exception& e)
	{
		if(HBSD::log->enabled(Logging::ERROR))
		{
			HBSD::log->error(string("An exception occurred in HBSD_Routing::localDest: ") + string(e.what()));
		}
		return false;
	}
}

string HBSD_Routing::linkId(XMLTree* el) 
{
	assert(el != NULL);
	if (el->haveAttr(string("link_id"))) 
	{
		return el->getAttr(string("link_id"));
	}
	return NULL;
}


Link* HBSD_Routing::linkStructure(XMLTree* el) 
{
	assert(el != NULL);
	string id = linkId(el);
	assert(!id.empty());

	if (id.empty()) 
	{
		return NULL;
	}

	return links->getById(id);
}




void HBSD_Routing::handler_link_available_event(XMLTree* event, XMLTree* bpa) 
{
	assert(event != NULL);
	assert(bpa != NULL);

	if (HBSD::localEID.empty()) 
	{
		firstMessage(bpa);
	}

	Link* link = linkStructure(event);

	if (link != NULL) 
	{
		link->available(event);
		// Asking DTND to open the link
		link->requestOpen();
		link = NULL;
	} else 
	{
		if (HBSD::log->enabled(Logging::ERROR)) 
		{
			HBSD::log->info(string("Unknown link became available"));
		}
	}
}

void HBSD_Routing::handler_link_unavailable_event(XMLTree* event, XMLTree* bpa)
{
	assert(event != NULL);
	assert(bpa != NULL);

	if (HBSD::localEID.empty()) 
	{
		firstMessage(bpa);
	}
	Link* link = linkStructure(event);
	if (link != NULL) 
	{
		link->unavailable(event);
		link = NULL;
	} else 
	{
		if (HBSD::log->enabled(Logging::ERROR)) 
		{
			HBSD::log->error(string("Unknown link became unavailable"));
		}
	}
}

void HBSD_Routing::sendWithoutscheduling(list<std::string>& listBundlesIDs, Link * link)
{
	assert(link != NULL);
	// Cycle through the list and send the bundles
	int i = 0;
	for(list<string>::iterator iter = listBundlesIDs.begin(); iter != listBundlesIDs.end(); iter++)
	{
		// request to send this bundle to the remote node
		if(HBSD::requester->requestSendBundle(bundles->getByKey(*iter), link->id, HBSD::requester->FWD_ACTION_COPY))
		{
			if(HBSD::log->enabled(Logging::INFO))
				HBSD::log->info(string("The requested bundle: ") + *iter + string(" is successfully sent."));
			i++;
		}else
		{
			if(HBSD::log->enabled(Logging::ERROR))
							HBSD::log->error(string("Error occurred when trying to send the scheduled bundle: ") + *iter);
		}
	}
	HBSD::log->info(Util::to_string(i) + string(" bundles was sent without scheduling."));
}

void HBSD_Routing::scheduleDRAndSend(list<std::string>& listBundlesIDs, Link * link)
{
	assert(link != NULL);

	// Scheduling the bundles
	map<double, string> sortedListWithUtilities;
	for(list<string>::iterator iter = listBundlesIDs.begin(); iter != listBundlesIDs.end();iter++)
	{
		// Get the bundle from its ID
		Bundle * currentBundle = bundles->getByKey(*iter);
		// Get the bundle utility
		double niAtT = 0;
		double miAtT = 0;
		double ddAtT = 0;
		double drAtT = 0;
		double currentUtility = 0;
		statisticsManager->getStatFromAxe(currentBundle->getElapsedTimeSinceCreation(), (char*)(*iter).c_str(), &niAtT,  &miAtT, &ddAtT, &drAtT);


		// Calculating the DR utility of the bundle
		// Getting the network average meeting time
		double averageMeetingTime = statisticsManager->getAverageNetworkMeetingTime();

		// Number of Nodes within the network
		int numberOfNodes = statisticsManager->getApproximatedNumberOfNodes();

		double parameterAlpha = averageMeetingTime * (numberOfNodes -1);

		currentUtility = (1/(parameterAlpha))*(currentBundle->expiration - currentBundle->getElapsedTimeSinceCreation())*drAtT;

		// Insert it to the map
		sortedListWithUtilities.insert(make_pair<double, string>( currentUtility, *iter));
		currentBundle = NULL;
	}

	// The map is already sorted
	// Cycle through the map in reverse order and send the bundles
	for(map<double, string>::reverse_iterator riter = sortedListWithUtilities.rbegin(); riter != sortedListWithUtilities.rend(); riter++)
	{
		// request to send this bundle to the remote node
		if(HBSD::requester->requestSendBundle(bundles->getByKey(riter->second), link->id, HBSD::requester->FWD_ACTION_COPY))
		{
			if(HBSD::log->enabled(Logging::INFO))
				HBSD::log->info(string("Requested bundle: ") + riter->second + string(" utility: ")+ Util::to_string(riter->first)+ string(" Successfully sent."));
		}else
		{
			if(HBSD::log->enabled(Logging::ERROR))
							HBSD::log->error(string("Error occurred when trying to send the scheduled bundle: ") + riter->second);
		}
	}

	sortedListWithUtilities.clear();

}

void HBSD_Routing::scheduleDDAndSend(list<std::string>& listBundlesIDs, Link * link)
{
	assert(link != NULL);

	// Scheduling the bundles
	map<double, string> sortedListWithUtilities;
	for(list<string>::iterator iter = listBundlesIDs.begin(); iter != listBundlesIDs.end();iter++)
	{
		// Get the bundle from its ID
		Bundle * currentBundle = bundles->getByKey(*iter);

		// Get the bundle utility
		double niAtT = 0;
		double miAtT = 0;
		double ddAtT = 0;
		double drAtT = 0;

		statisticsManager->getStatFromAxe(currentBundle->getElapsedTimeSinceCreation(), (char*)(*iter).c_str(), &niAtT,  &miAtT, &ddAtT, &drAtT);

		// Calculating the DD utility of the new bundle
		// Getting the network average meeting time
		double averageMeetingTime = statisticsManager->getAverageNetworkMeetingTime();

		// Number of Nodes within the network
		int numberOfNodes = statisticsManager->getApproximatedNumberOfNodes();

		double parameterAlpha = averageMeetingTime * (numberOfNodes -1);

		double currentUtility = ((parameterAlpha/(numberOfNodes -1))*pow((double)ddAtT, 2.0))/(numberOfNodes - 1 - miAtT);

		// Insert it to the map
		sortedListWithUtilities.insert(make_pair<double, string>(currentUtility, *iter));
		currentBundle = NULL;
	}

	// The map is already sorted
	// Cycle through the map in reverse order and send the bundles
	for(map<double, string>::reverse_iterator riter = sortedListWithUtilities.rbegin(); riter != sortedListWithUtilities.rend(); riter++)
	{
		// request to send this bundle to the remote node
		if(HBSD::requester->requestSendBundle(bundles->getByKey(riter->second), link->id, HBSD::requester->FWD_ACTION_COPY))
		{
			if(HBSD::log->enabled(Logging::INFO))
				HBSD::log->info(string("Requested bundle: ") + riter->second + string(" utility: ")+ Util::to_string(riter->first)+ string(" Successfully sent."));
		}else
		{
			if(HBSD::log->enabled(Logging::ERROR))
							HBSD::log->error(string("Error occurred when trying to send the scheduled bundle: ") + riter->second);
		}
	}

	sortedListWithUtilities.clear();
}

std::string HBSD_Routing::getStatMessageType(std::string & filePath)
{
	assert(!filePath.empty());
	// open payload file from remote node
	fstream remoteFile(filePath.c_str(), ios::in);

	// this is the buffer where we read in each line
	char remoteHash[MAXHASHLENGTH];
	memset(remoteHash, '\0', MAXHASHLENGTH);

	// Pick the first line
	remoteFile >> remoteHash;

	string fileType;
	if(strcmp(remoteHash, "EpidemicSC") == 0)
	{
		fileType.assign(remoteHash);
	}

	remoteFile.close();
	return fileType;
}


bool HBSD_Routing::isStatisticsFileEmpty(std::string & filePath)
{
	assert(!filePath.empty());
	// open payload file from remote node
	fstream remoteFile(filePath.c_str(), ios::in);

	// Skip the first line
	remoteFile.seekg(1, ios::beg);
	// this is the buffer where we read in each line
	char remoteHash[MAXHASHLENGTH];
	memset(remoteHash, '\0', MAXHASHLENGTH);
	// Pick the next line
	remoteFile >> remoteHash;
	remoteFile.close();
	if(strlen(remoteHash) < 1)
		return true;
	else return false;
}
