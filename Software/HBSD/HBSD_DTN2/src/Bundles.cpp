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


#include "Bundles.h"
#include "Handlers.h"
#include "XMLTree.h"
#include "Bundle.h"
#include "HBSD.h"
#include "ConfigFile.h"
#include "Requester.h"
#include "GBOF.h"
#include "Logging.h"
#include "HBSD_Policy.h"
#include "HBSD_Routing.h"
#include "Policy.h"
#include <limits>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <fstream>
#include "Link.h"
#include <iostream>
#include <sstream>
#include "MeDeHaInterface.h"
using namespace std;


Bundles::Bundles(Handlers* router)
{
	assert(router!=NULL);
	this->router = router;
	// Getting the buffer capacity, by default it is equal to 100
	maxBufferCapacity = HBSD::routerConf->getInt(string("bundlesActiveCapacity"), DEFAULT_ACTIVE_CAPACITY);
	assert(maxBufferCapacity > 0);
	// Getting HBSD main optimization task, by default it is set to maximizing the network average delivery rate
	this->hbsdOptimizePerformance = HBSD::routerConf->getInt(string("hbsdOptimizePerformance"), DEFAULT_HBSD_POLICY_PURPOSE);
	// Initializing the bundlesLock which will be used to manage threads access to the bundles buffer
	sem_init(&bundlesLock, 0, 1);
	// Init bundlesLockFile
	if(BUNDLES_LOCK_LOG)
	{
		ofstream bundlesLockFile;
		bundlesLockFile.open(BUNDLES_LOCK_FILE, ios_base::trunc);
		bundlesLockFile.close();
	}
	sem_init(&storeStatus, 0, 1);

	lastTimeBundlesStoreChanged = Util::getCurrentTimeSeconds();
}

Bundles::~Bundles()
{
	router = NULL;


	localidGBOFMap.clear();

	while(!activeBundles.empty())
	{
		map<string, Bundle*>::iterator iter = activeBundles.begin();
		if(iter->second != NULL)
			delete iter->second;
		activeBundles.erase(iter);
	}

	sem_destroy(&bundlesLock);
	sem_destroy(&storeStatus);
} 


Bundle* Bundles::newBundle(XMLTree* evtBundleRcvd) 
{
	assert(evtBundleRcvd != NULL);
	Bundle* bundle = NULL;
	try
	{
		// DTN2 will normally detect duplicate bundles and not deliver
		// a notification to us. But, still, we check.

		if (alreadyExists(evtBundleRcvd))
		{
			if (HBSD::log->enabled(Logging::DEBUG))
			{
				HBSD::log->debug(string("Dropping duplicate bundle"));
			}
			return NULL;
		}

		// Create the instance of the Bundle object for the bundle.
		// Note that a local id of -1 is common: injected bundles
		// get created via a bundle_injected_event, then DTN2 sends
		// us the bundle_received_event for that same bundle.
		bundle = new Bundle(this);
		assert(bundle != NULL);
		if (bundle->initReceived(evtBundleRcvd) == NULL)
		{
			if(bundle != NULL)
				delete bundle;
			return NULL;
		}

		if(((HBSD_Routing*)router)->localDest(bundle))
		{
			// It is for a local dest, remove it
			HBSD::log->info(string("A new Bundle is delivered for a local application: ") + bundle->destURI);
			// Verify if we should forward the bundle towards the MeDeHa Gateway

			if(bundle != NULL)
				delete bundle;
			return NULL;
		}

		return addIfNew(bundle, bundle->localId);
	}

	catch(exception &e)
	{
		HBSD::log->error(string("Exception occurred at Bundles::newBundle: ") + string(e.what()));
		if(bundle != NULL)
			delete bundle;
		return NULL;
	}
}

// Called whenever there is a new injected bundle
Bundle* Bundles::newInjectedBundle(XMLTree* evtBundleInjected) 
{
	assert(evtBundleInjected != NULL);
	Bundle* bundle = new Bundle(this);
	bundle->initGeneric(evtBundleInjected);
	return bundle;
}


Bundle* Bundles::newBundleDelivery(XMLTree* evtBundleDelivery) 
{
	assert(evtBundleDelivery != NULL);
	Bundle* bundle = new Bundle(this);
	string localId = bundle->initGeneric(evtBundleDelivery);
	if (localId.empty())
	{
		return NULL;
	}
	// A bundle is delivered to HBSD, that should be either Epidemic SV or statistics
	bundle->statisticsInfo = true;
	return bundle;
}

// Verifies if a bundle already exists in the activeBundles map or not and synchronized
bool Bundles::alreadyExists(XMLTree* evtBundleRcvd) 
{
	assert(evtBundleRcvd != NULL);
	string key;
	try 
	{
		XMLTree* el = evtBundleRcvd->getChildElementRequired(string("gbof_id"));
		assert(el != NULL);
		key = GBOF::keyFromXML(el);
		if (key.empty()) 
		{
			// Ignore
			HBSD::log->error(string("Empty key in Bundles::alreadyExists"));
			return true;
		}

		getBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));
			map <string,Bundle *>::iterator iter = activeBundles.find(key);
			bool exist = (iter != activeBundles.end());
		leaveBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));
		return exist;
	} 

	catch (exception& e) 
	{
		if (HBSD::log->enabled(Logging::ERROR)) 
		{
			HBSD::log->error(string("Ill-formed GBOF in received XML"));
		}
		// Ill-formed: treat as a duplicate.
		return true;
	}

	return false;
}

// Get a bundle from the activeBundles map given its ID, Synchronized
Bundle* Bundles::getByKey(string gbofKey) 
{
	Bundle* bundle = activeBundles[gbofKey];
	assert(bundle != NULL);
	return bundle;
}


// Add a bundle to the activeBundles map if it is a new one
// and triggers the StatisticsManager object so that it updates the maintained statistics
// Synchronized
Bundle* Bundles::addIfNew(Bundle* createdBundle, string localId)
{
	string gbof = GBOF::keyFromBundle(createdBundle);
	assert(!gbof.empty());

	getBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));

	map <string, string>::iterator iter = localidGBOFMap.find(localId);

	if (iter != localidGBOFMap.end()) 
	{
		// Already existing
		leaveBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));
		return NULL;
	}


	// Verify if there is still some free space within the node buffer so to add the new bundle
	// If the node buffer is full then, apply the selected HBSD drop policy in order to decide either to
	// add the received bundle in counter part of an already stored one or to delete it.

	if(activeBundles.size() < maxBufferCapacity)
	{

		//There is still some space
		// Updating the maintained statistics
		HBSD_Routing * hbsdRouter = (HBSD_Routing*)this->router;
		if(hbsdRouter->enableOptimization())
		{
			hbsdRouter->statisticsManager->addStatMessage((char *)gbof.c_str(), (char *)HBSD::localEID.c_str(), (double)createdBundle->getElapsedTimeSinceCreation(), (double)createdBundle->expiration);
		}

		// Add the bundle
		localidGBOFMap[localId] = gbof;
		activeBundles[gbof] = createdBundle;
		hbsdRouter = NULL;

		leaveBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));
		this->changeLastTimeBundlesStoreUpdate();
		return createdBundle;
	}
	else
	{
		// No space is left, we should select and delete a bundle

		//The buffer is full, we should unlock the bundlesLock once the optimized buffer management is done
		if(((HBSD_Routing*)this->router)->enableOptimization())
		{
			// Each of the drop policies shoud remember to leave the
			// semaphore once the job is finished
			switch(hbsdOptimizePerformance)
			{
			case 0:
				// Trying to add the bundle while maximizing the network average delivery rate
				return hbsdAddBundleAndMaximizeAverageDeliveryRate(createdBundle, localId);
				break;
			case 1:
				// Trying to add the bundle while minimizing the network average delivery delay
				return hbsdAddBundleAndMinimizeAverageDeliveryDelay(createdBundle, localId);
				break;
			default:
				// Wrong optimization problem selected
				if(HBSD::log->enabled(Logging::FATAL))
					HBSD::log->fatal(string("A wrong optimization problem selected, please be sure to correctly set the hbsdOptimizePerformance attribute in the HBSD config file"));
				leaveBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));
				return NULL;
			};
		}else
		{
			// Just apply the drop last policy, delete the last received bundle
			if(!deleteBundle(createdBundle))
			{
				if(HBSD::log->enabled(Logging::FATAL))
					HBSD::log->fatal(string("Unable to delete the new received bundle from the DTN2 store"));
			}

			createdBundle = NULL;
			leaveBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));
			return NULL;
		}
	}
}

// Deletes a bundle once it expires, synchronized
Bundle* Bundles::expire(string localId)
{
	getBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));

	map <string, string>::iterator iter = localidGBOFMap.find(localId);

	if(iter == localidGBOFMap.end())
	{
		// That could be an injected bundle
		leaveBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));

		return NULL;
	}

	string key = iter->second;

	localidGBOFMap.erase(iter);

	map <string,Bundle *>::iterator iter2 = activeBundles.find(key);
	if(iter2 == activeBundles.end())
	{
		return NULL;
	}
	HBSD_Routing * hbsdRouter = (HBSD_Routing*)this->router;

	if(hbsdRouter->enableOptimization())
	{
		// Updating the status of a bundle in the Statistics Matrix, Saying that the bundle has been deleted within the
		// bin corresponding to its elapsed time
		hbsdRouter->statisticsManager->updateBundleStatus((char*)HBSD::localEID.c_str(), (char*)localidGBOFMap[iter2->second->localId].c_str(), 0, (iter2->second)->getElapsedTimeSinceCreation(), (iter2->second)->expiration);
	}

	hbsdRouter = NULL;

	Bundle* bundle = iter2->second;
	iter2->second = NULL;
	activeBundles.erase(iter2);

	this->changeLastTimeBundlesStoreUpdate();

	leaveBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));

	return bundle;
}


// Remove a bundle without requesting the DTN2 daemon to definitely delete it, synchronized
bool Bundles::remove(Bundle* bundle) 
{
	assert(!bundle->injected);

	getBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));

	string gbof = localidGBOFMap[bundle->localId];
	assert(!gbof.empty());
	localidGBOFMap.erase(bundle->localId);

	map <string,Bundle *>::iterator iter = activeBundles.find(gbof);
	assert(iter != activeBundles.end());
	if(iter->second != NULL)
		delete iter->second;
	activeBundles.erase(iter);

	this->changeLastTimeBundlesStoreUpdate();

	leaveBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));

	return true;
}

//Removes a bundle and asks the DTN2 to delete it
bool Bundles::deleteBundle(Bundle* bundle) 
{
	if (remove(bundle))
	{
		return HBSD::requester->requestDeleteBundle(bundle);
	}
	return false;
}

//Removes a bundle and asks the DTN2 to delete it
void Bundles::finished(Bundle* bundle) 
{
	if (!bundle->injected)
	{
		remove(bundle);
	}
	if(!HBSD::requester->requestDeleteBundle(bundle))
	{
		if(HBSD::log->enabled(Logging::ERROR))
		{
			HBSD::log->error(string("In Bundles::finished, problem occured while requesting to delete the bundle."));
		}
	}
}
// Synchronized
bool Bundles::isCurrent(string localId)
{
	bool current = false;
	getBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));
	string gbof = localidGBOFMap[localId];
	if (!gbof.empty()) 
	{
		map <string,Bundle *>::iterator iter = activeBundles.find(gbof);

		if (iter != activeBundles.end()) 
		{
			current = true;
		}
	}

	leaveBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));

	return current;
}

// Synchronized
Bundle* Bundles::bundlefromLocalId(string localId)
{
	Bundle* bundle = NULL;

	getBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));

	string gbof = localidGBOFMap[localId];
	if (!gbof.empty()) 
	{
		bundle = activeBundles[gbof];
	}
	leaveBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));

	return bundle;
}


Bundle* Bundles::eventExpired(XMLTree* evtBundleExpired) 
{
	try 
	{
		return expire(xmlLocalId(evtBundleExpired));
	} 
	catch (exception& e) 
	{
		return NULL;
	}
}

// Synchronized
Bundle* Bundles::eventDelivered(XMLTree* evtBundleDelivered) 
{
	string localId;
	try 
	{
		localId = xmlLocalId(evtBundleDelivered);
	} 
	catch (exception& e) 
	{
		return NULL;
	}

	Bundle* bundle = NULL;

	// The following lookup should fail for an injected bundle. But
	// we still check again later.

	getBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));
	string gbof = localidGBOFMap[localId];
	if (!gbof.empty()) 
	{
		bundle = activeBundles[gbof];
	}


	if (bundle == NULL || bundle->injected)
	{
		leaveBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));
		return NULL;
	}

	leaveBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));
	return bundle;
}


string Bundles::xmlLocalId(XMLTree* element)
{
	try 
	{
		assert(element != NULL);
		string localId = element->getAttrRequired(string("local_id"));
		return localId;
	}
	catch (exception& e) 
	{
		if(HBSD::log->enabled(Logging::ERROR))
			HBSD::log->error(string("Error occurred within Bundles::xmlLocalId."));
		return NULL;
	}
}

Bundle * Bundles::hbsdAddBundleAndMaximizeAverageDeliveryRate(Bundle *createdBundle, string localId)
{
	assert(createdBundle != NULL);
	assert(!localId.empty());

	HBSD_Routing * hbsdRouter = (HBSD_Routing*)this->router;

	string newBundleUid = this->localidGBOFMap[createdBundle->localId];

	// Getting the network average meeting time
	double averageMeetingTime = hbsdRouter->statisticsManager->getAverageNetworkMeetingTime();

	// Number of Nodes within the network
	int numberOfNodes = hbsdRouter->statisticsManager->getApproximatedNumberOfNodes();

	double parameterAlpha = averageMeetingTime * (numberOfNodes -1);

	double niAtT = 0;
	double miAtT = 0;
	double ddAtT = 0;
	double drAtT = 0;

	hbsdRouter->statisticsManager->getStatFromAxe(createdBundle->getElapsedTimeSinceCreation(), (char*)newBundleUid.c_str(),&niAtT,&miAtT,&ddAtT,&drAtT);

	// Calculating the utility of the new received bundle
	double newBundleUtilityValue=(1/(parameterAlpha))*(createdBundle->expiration - createdBundle->getElapsedTimeSinceCreation())*drAtT;

	// Getting from the buffer the bundle that has the smallest utility value
	Bundle * bundleHavingTheSmallestUtility = NULL;
	double smallestUtilityValue = 0;
	string idBundleHavingTheSmallestUtility = this->getBundleWithTheSmallestDDUtilityFromTheBuffer(bundleHavingTheSmallestUtility, &smallestUtilityValue);


	// Getting the local EID
	string nodeLocalEID = HBSD::localEID;

	// Comparing the utility of the new bundle and the already stored one
	// Deleting the bundle that has the smallest utility, return null if the new bundle was not added otherwise return a handler to the
	// later one

	// Don't delete bundles generated by the local applications
	if( smallestUtilityValue <= newBundleUtilityValue  || ((HBSD_Routing*)router)->localSource(createdBundle))
	{

		// Updating the status of a bundle in the Statistics Matrix, Saying that the bundle has been deleted within the
		// bin corresponding to its elapsed time
		hbsdRouter->statisticsManager->updateBundleStatus((char*)nodeLocalEID.c_str(), (char*)idBundleHavingTheSmallestUtility.c_str(), 0, bundleHavingTheSmallestUtility->getElapsedTimeSinceCreation(), bundleHavingTheSmallestUtility->expiration);

		// Adding the new bundle to the Statistics Matrix or updating its status if it is already there
		hbsdRouter->statisticsManager->addStatMessage((char *)newBundleUid.c_str(), (char *)nodeLocalEID.c_str(), (double)createdBundle->getElapsedTimeSinceCreation(), (double)createdBundle->expiration);

		// Adding the new received Bundle to HBSD buffer
		string gbof = GBOF::keyFromBundle(createdBundle);
		localidGBOFMap[createdBundle->localId] = gbof;
		activeBundles[gbof] = createdBundle;
		hbsdRouter = NULL;

		leaveBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));

		// Deleting the bundle from both HBSD buffer and DTN2 store
		if(!this->deleteBundle(bundleHavingTheSmallestUtility))
		{
			// Problem occurred while trying to delete the bundle
			if(HBSD::log->enabled(Logging::FATAL))
				HBSD::log->fatal(string("Unable to delete the bundle having the smallest utility"));
		}
		bundleHavingTheSmallestUtility = NULL;

		this->changeLastTimeBundlesStoreUpdate();

		return createdBundle;

	} else
	{
		leaveBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));
		// Just delete the new received bundle and keep the buffer as it is
		if(!deleteBundle(createdBundle))
		{
			if(HBSD::log->enabled(Logging::FATAL))
				HBSD::log->fatal(string("Unable to delete the new received bundle from the DTN2 store"));
		}
		createdBundle = NULL;
		bundleHavingTheSmallestUtility = NULL;
	    hbsdRouter = NULL;
	    return NULL;

	}
}


Bundle * Bundles::hbsdAddBundleAndMinimizeAverageDeliveryDelay(Bundle *createdBundle, string localId)
{
	assert(createdBundle != NULL);
	assert(!localId.empty());

	HBSD_Routing * hbsdRouter = (HBSD_Routing*)this->router;

	string newBundleUid = this->localidGBOFMap[createdBundle->localId];

	// Getting the network average meeting time
	double averageMeetingTime = hbsdRouter->statisticsManager->getAverageNetworkMeetingTime();

	// Number of Nodes within the network
	int numberOfNodes = hbsdRouter->statisticsManager->getApproximatedNumberOfNodes();

	double parameterAlpha = averageMeetingTime * (numberOfNodes -1);

	double niAtT = 0;
	double miAtT = 0;
	double ddAtT = 0;
	double drAtT = 0;

	hbsdRouter->statisticsManager->getStatFromAxe(createdBundle->getElapsedTimeSinceCreation(), (char*)newBundleUid.c_str(),&niAtT,&miAtT,&ddAtT,&drAtT);

	// Calculating the utility of the new received bundle
	double 	newBundleUtilityValue=((parameterAlpha/(numberOfNodes -1))*pow((double)ddAtT, 2.0))/(numberOfNodes - 1 - miAtT);

	// Getting from the buffer the bundle that has the smallest utility value
	Bundle * bundleHavingTheSmallestUtility = NULL;
	double smallestUtilityValue = 0;
	string idBundleHavingTheSmallestUtility = this->getBundleWithTheSmallestDDUtilityFromTheBuffer(bundleHavingTheSmallestUtility, &smallestUtilityValue);


	// Getting the local EID
	string nodeLocalEID = HBSD::localEID;

	// Comparing the utility of the new bundle and the already stored one
	// Deleting the bundle that has the smallest utility, return null if the new bundle was not added otherwise return a handler to the
	// later one

	// Don't drop the bundles created by the local applications
	if( smallestUtilityValue <= newBundleUtilityValue || ((HBSD_Routing*)router)->localSource(createdBundle))
	{

		// Updating the status of a bundle in the Statistics Matrix, Saying that the bundle has been deleted within the
		// bin corresponding to its elapsed time
		hbsdRouter->statisticsManager->updateBundleStatus((char*)nodeLocalEID.c_str(), (char*)idBundleHavingTheSmallestUtility.c_str(), 0, bundleHavingTheSmallestUtility->getElapsedTimeSinceCreation(), bundleHavingTheSmallestUtility->expiration);


		// Adding the new bundle to the Statistics Matrix or updating its status if it is already there
		hbsdRouter->statisticsManager->addStatMessage((char *)newBundleUid.c_str(), (char *)nodeLocalEID.c_str(), (double)createdBundle->getElapsedTimeSinceCreation(), (double)createdBundle->expiration);

		// Adding the new received Bundle to HBSD buffer
		string gbof = GBOF::keyFromBundle(createdBundle);
		localidGBOFMap[createdBundle->localId] = gbof;
		activeBundles[gbof] = createdBundle;
		hbsdRouter = NULL;

		leaveBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));

		// Deleting the bundle from both HBSD buffer and DTN2 store
		if(!this->deleteBundle(bundleHavingTheSmallestUtility))
		{
			// Problem occurred while trying to delete the bundle
			if(HBSD::log->enabled(Logging::FATAL))
				HBSD::log->fatal(string("Unable to delete the bundle having the smallest utility"));
		}
		bundleHavingTheSmallestUtility = NULL;

		this->changeLastTimeBundlesStoreUpdate();

		return createdBundle;

	} else
	{
		leaveBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));

		// Just delete the new received bundle and keep the buffer as it is
		if(!deleteBundle(createdBundle))
		{
			if(HBSD::log->enabled(Logging::FATAL))
				HBSD::log->fatal(string("Unable to delete the new received bundle from the DTN2 store"));
		}
		createdBundle = NULL;
		bundleHavingTheSmallestUtility = NULL;
	    hbsdRouter = NULL;
	    return NULL;

	}
}


string Bundles::getBundleWithTheSmallestDRUtilityFromTheBuffer(Bundle * sb, double * smallestUtility)
{
	// The statistics manager
	StatisticsManager * statisticsManager = ((HBSD_Routing*)this->router)->statisticsManager;

	// Getting the network average meeting time
	double averageMeetingTime = statisticsManager->getAverageNetworkMeetingTime();

	// Number of Nodes within the network
	int numberOfNodes = statisticsManager->getApproximatedNumberOfNodes();

	double parameterAlpha = averageMeetingTime * (numberOfNodes -1);
	double minUtility = numeric_limits<double>::max();
	double currentUtilityValue = 0;
	map <std::string,Bundle *>::iterator selectedBundle = activeBundles.begin();
	map <std::string,Bundle *>::iterator iter = activeBundles.begin();

	double niAtT,miAtT;
	double ddAtT,drAtT;

	while(iter != activeBundles.end())
	{
		Bundle * cb = iter->second;
		statisticsManager->getStatFromAxe(cb->getElapsedTimeSinceCreation(), (char*)iter->first.c_str(),&niAtT,&miAtT,&ddAtT,&drAtT);


		currentUtilityValue = (1/(parameterAlpha))*(cb->expiration - cb->getElapsedTimeSinceCreation())*drAtT;
		// Don't consider bundles generated by the local applictions
		if((currentUtilityValue < minUtility) && !((HBSD_Routing*)router)->localSource(iter->second))
		{
			minUtility = currentUtilityValue;
			selectedBundle = iter;
		}
		iter++;
	}

	*smallestUtility = minUtility;
	*sb = *(selectedBundle->second);
	return sb->localId;
}


string Bundles::getBundleWithTheSmallestDDUtilityFromTheBuffer(Bundle * sb, double * smallestUtility)
{
	// The statistics manager
	StatisticsManager * statisticsManager = ((HBSD_Routing*)this->router)->statisticsManager;

	// Getting the network average meeting time
	double averageMeetingTime = statisticsManager->getAverageNetworkMeetingTime();

	// Number of Nodes within the network
	int numberOfNodes = statisticsManager->getApproximatedNumberOfNodes();

	double parameterAlpha = averageMeetingTime * (numberOfNodes -1);
	double minUtility = numeric_limits<double>::max();
	double currentUtilityValue = 0;

	map <std::string,Bundle *>::iterator selectedBundle = activeBundles.begin();
	map <std::string,Bundle *>::iterator iter = activeBundles.begin();

	double niAtT,miAtT;
	double ddAtT,drAtT;

	while(iter != activeBundles.end())
	{
		Bundle * cb = iter->second;
		statisticsManager->getStatFromAxe(cb->getElapsedTimeSinceCreation(), (char*)iter->first.c_str(),&niAtT,&miAtT,&ddAtT,&drAtT);

		currentUtilityValue = ((parameterAlpha/(numberOfNodes - 1))*pow((double)ddAtT, 2.0))/(numberOfNodes - 1 - miAtT);
		// Don't consider bundles generated by local applications
		if((currentUtilityValue < minUtility) && !((HBSD_Routing*)router)->localSource(iter->second))
		{
			minUtility = currentUtilityValue;
			selectedBundle = iter;
		}
		iter++;
	}

	*smallestUtility = minUtility;
	*sb = *(selectedBundle->second);
	return sb->localId;
}



// Periodically and each time a new link is opened the HBSD_Routing object calls this method
// in order to create an sends a summary vector to the other node
// Synchronized
string Bundles::createSV(string type, bool lock)
{
	assert(!type.empty());
	// Name of payload file
	// this should be unique per injected bundle because dtnd will delete it
	if(lock)
	{
		getBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));
	}


	try
	{
		string payloadFile;
		payloadFile.assign(string("/home/amir/hbsd_dtn2_epidemic_sv_") +  Util::to_string(HBSD::requester->getAndIncrement()));

		// open and truncate the summary vector file
		ofstream svFile;
		svFile.open(payloadFile.c_str(), ios_base::trunc);

		if(HBSD::log->enabled(Logging::INFO))
			HBSD::log->info(string("creating the SV for transmission"));

		// Starting a new Epidemic routing session
		if(type.compare(string(EPIDEMIC_SV_1)) == 0)
		{
			svFile<<string(EPIDEMIC_SV_1)<<endl;
		}
		else if( type.compare(string(EPIDEMIC_SV_2)) == 0)
		{
			svFile<<string(EPIDEMIC_SV_2)<<endl;
		}
		else
		{
			if(HBSD::log->enabled(Logging::ERROR))
				HBSD::log->error(string("Unknown Epidemic metadata file type: ") + type);
		}

		if(activeBundles.size() > 0)
		{
			// for every real bundle in our list..
			for(map <std::string,Bundle *>::iterator iter = activeBundles.begin(); iter != activeBundles.end(); iter++)
			{
				// add the bundle's hash to the file
				svFile << localidGBOFMap[iter->second->localId]<<endl;
			}
		}

		// Closing the file
		svFile.close();

		ifstream tmpFile;
		tmpFile.open(payloadFile.c_str());
		string tmpLine;

		if(HBSD::log->enabled(Logging::INFO))
		{
			HBSD::log->info(string("Epidemic summary vector created of type: ") + type);
			cout<<"---------------------------"<<endl;
			while(tmpFile >> tmpLine)
			{
				cout << tmpLine<<endl;
			}
			cout<<"---------------------------"<<endl;
		}

		tmpFile.close();


		string tmp(payloadFile);

		if(lock)
		{
			leaveBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));
		}
		// Returning the path to the created bundles summary vector
		return tmp;
	}

	catch(exception& e)
	{
		if(HBSD::log->enabled(Logging::ERROR))
			HBSD::log->error(string("Error occurred @ Bundles::createSV: ") + string(e.what()));

		leaveBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));
		return NULL;
	}
}

// COMPARE LOCAL SUMMARY VECTOR TO THAT PAYLOAD FILE OF REMOTE NODE
// AND SEND BUNDLES THE REMOTE NODE LACKS
// Synchronized
void Bundles::compareAndSend(string  remoteSv, Link *link, bool sendBack)
{
	assert(link !=NULL);
	try
	{


		getBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));


		showAvailableBundles();

		bool send;
		int nbrToSend = 0;
		stringstream remoteSVStream(stringstream::in | stringstream::out);
		remoteSVStream << remoteSv;
		list<string> listToSend;

		// for each bundle in our list of bundles..
		for(map <std::string, Bundle *>::iterator iter = activeBundles.begin(); iter != activeBundles.end(); iter++)
		{
			// get the bundle object
			Bundle * bundle = iter->second;
			assert(bundle != NULL);
			// Get the pointer to the bundle's hash, for convenience
			string localHash = localidGBOFMap[bundle->localId];

			// Start with the intent of sending this bundle
			send = true;
			// this is the buffer where we read in each line
			if(!remoteSv.empty())
			{
				remoteSVStream.clear();
				remoteSVStream.seekg(0, ios::beg);
				string remoteHash;
				// for every line in the remote node's summary vector..
				while(remoteSVStream >> remoteHash)
				{
					// if the bundle hashes match, the remote node already has it
					if(localHash.compare(remoteHash) == 0)
					{
						send = false;
						break; // we can stop looking through the remote node's summary vector
					}
				}
			}

			if(!send)
			{
				continue; // check next bundle in our list
			}
			else
			{
				nbrToSend++;
				listToSend.push_back(localHash);
			}
		}

		if(nbrToSend > 0)
		{
			if(HBSD::log->enabled(Logging::INFO))
				HBSD::log->info(string("Sending back ") + Util::to_string(listToSend.size()) + string(" bundles ") + link->remoteEID );


			if(((HBSD_Routing*)this->router)->enableOptimization())
			{
				switch(hbsdOptimizePerformance)
				{
				case 0:
					((HBSD_Routing*)router)->scheduleDRAndSend(listToSend, link);
					break;
				case 1:
					((HBSD_Routing*)router)->scheduleDDAndSend(listToSend, link);
					break;
				default:
					if(HBSD::log->enabled(Logging::FATAL))
					{
						HBSD::log->fatal(string("A bad optimization problem is selected"));
					}
				}
			}else
			{
				// Optimization is not enabled so, just send bundles randomly
				((HBSD_Routing*)router)->sendWithoutscheduling(listToSend, link);
			}
		}
		else
		{
			if(HBSD::log->enabled(Logging::INFO))
				HBSD::log->info(string("0 bundles will be sent back."));
		}


		// See if we should answer or not the received EPIDEMIC_SV_1
		if(sendBack)
		{
			if(!remoteSv.empty() || !activeBundles.empty())
			{
				if(HBSD::log->enabled(Logging::INFO))
					HBSD::log->info(string("Trying to send back local SV."));

				// Now we need to send our SV if they have a bundle that we don't have
				// (re)set the file cursor to the beginning of the file
				bool identicalSVs = true;

				if(!remoteSv.empty() && !activeBundles.empty())
				{
					remoteSVStream.clear();
					remoteSVStream.seekg(0, ios::beg);

					string remoteHash;

					// for every line in the remote node's summary vector..
					while(remoteSVStream >> remoteHash)
					{
						if(activeBundles.find(remoteHash) == activeBundles.end())
						{
							// We can stop looking, we have found a bundle they have that we don't have
							identicalSVs = false;
							break;
						}
					}
				}
				else if(!remoteSv.empty() && activeBundles.empty())
				{
					identicalSVs = false;
				}

				if (!identicalSVs)
				{
					// Sending our SV
					// Sending the summary vector of bundles
					string reqId;
					string remoteRouter = link->remoteEID + string("/") + HBSD::routerEndpoint;
					if((reqId = HBSD::requester->requestInjectBundle(HBSD::hbsdRegistration, remoteRouter, link->id, createSV(string(EPIDEMIC_SV_2), false))).empty())
					{
						if(HBSD::log->enabled(Logging::ERROR))
							HBSD::log->error(string("Unable to send the bundles summary vector to the remote peer"));
					}else
					{
						// Informing HBSD_Policy about that
						HBSD_Policy * policy = (HBSD_Policy*)((HBSD_Routing*)this->router)->policyMgr;
						policy->addNewInjectedRequest(reqId, link->id, link->remoteEID);
						policy = NULL;
					}
				}
				else
				{
					if(remoteSv.empty() && !activeBundles.empty())
					{
						if(HBSD::log->enabled(Logging::INFO))
							HBSD::log->info(string("Remote Epidemic SV is empty, we will ask for nothing."));

					}else
					{
						if(HBSD::log->enabled(Logging::INFO))
							HBSD::log->info(string("Both Epidemic SV are equal, nothing to send back."));
					}
				}
			}
			else
			{
				if(HBSD::log->enabled(Logging::INFO))
					HBSD::log->info(string("Both the received SV and the local bundles store are empty."));
			}
		}

		leaveBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));

	}

	catch(exception & e)
	{
		if(HBSD::log->enabled(Logging::ERROR))
			HBSD::log->error(string("Error occurred @ Bundles::compareAndSend: ") + string(e.what()));
		leaveBundlesLock(Util::to_string(__FILE__)+string(":")+Util::to_string(__LINE__));
	}
}


void Bundles::showAvailableBundles()
{
	if(HBSD::log->enabled(Logging::INFO))
	{
		HBSD::log->info(string("Number of available bundles: ")+ Util::to_string(activeBundles.size()));
		for(map<string, Bundle *>::iterator iter = activeBundles.begin(); iter != activeBundles.end(); iter++)
		{
			HBSD::log->info(string("Bundle gbof: ") + iter->first + string(" ttl: ") + Util::to_string(iter->second->expiration) + string(" creation ts: ")+Util::to_string(iter->second->creationTimestamp));
		}
	}
}

void Bundles::getBundlesLock(string x)
{
	if(BUNDLES_LOCK_LOG)
	{
		ofstream bundlesLockFile;
		bundlesLockFile.open(BUNDLES_LOCK_FILE, ios_base::app);
		bundlesLockFile <<"sem_wait called @: "<<x<<endl;
		bundlesLockFile.close();

	}

	sem_wait(&bundlesLock);
}

void Bundles::leaveBundlesLock(string x)
{
	if(BUNDLES_LOCK_LOG)
	{
		ofstream bundlesLockFile;
		bundlesLockFile.open(BUNDLES_LOCK_FILE, ios_base::app);
		bundlesLockFile <<"sem_post called @: "<<x<<endl;
		bundlesLockFile.close();
	}

	sem_post(&bundlesLock);
}

time_t Bundles::getLastTimeBundlesStoreChanged()
{
	time_t tmp;
	sem_wait(&storeStatus);
	tmp = lastTimeBundlesStoreChanged;
	sem_post(&storeStatus);
	return tmp;
}

void Bundles::changeLastTimeBundlesStoreUpdate()
{
	sem_wait(&storeStatus);
	lastTimeBundlesStoreChanged = Util::getCurrentTimeSeconds();
	sem_post(&storeStatus);
}
