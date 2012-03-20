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


#ifndef BUNDLES_H
#define BUNDLES_H

#include <stdlib.h>
#include <map>
#include <semaphore.h>
#include <string>

// Default buffer capacity in terms of bundles
#define DEFAULT_ACTIVE_CAPACITY 50

// By Default we set that the HBSD policy is used towards increasing the network average delivery rate
#define DEFAULT_HBSD_POLICY_PURPOSE 0
#define MAXINTDIGITS 8
#define MAXEIDLENGTH 64
#define MAXHASHLENGTH MAXEIDLENGTH+24
#define SVSENDINTERVAL 5

// An EPIDEMIC_SV_1 is first sent in order to start a new Epidemic routing session
// and EPIDEMIC_SV_2 is sent as an answer to an already received EPIDEMIC_SV_1
#define EPIDEMIC_SV_1 "EpidemicSV1"
#define EPIDEMIC_SV_2 "EpidemicSV2"
#define BUNDLES_LOCK_FILE "bundlesLockLog"
#define BUNDLES_LOCK_LOG false

class Handlers;
class Bundle;
class XMLTree;
class Policy;
class Link;

class Bundles
{

public:
	/**
	 * Constructor.
	 * 
	 * @param The router object, i.e. HBSD_Router.
	 */

	Bundles(Handlers *router);

	/**
	 * Destructor.
	 */
	~Bundles();

	
	/**
	 * Called to create a new bundle given a "bundle_received_event." The
	 * bundle is created, initialized, and added to the hash table.
	 * 
	 * @param evtBundleRcvd Root of bundle_received_event element.
	 * @return Created bundle; null if a bundle with this id already exists. 
	 */
	Bundle *newBundle(XMLTree *evtBundleRcvd);
	
	/**
	 * Called to initialize a bundle that has been injected. Though we use a
	 * Bundle object, we do not keep track of it like we do other received
	 * bundles.
	 * 
	 * @param evtBundleInjected Root of the bundle_injected_event element.
	 * @return The created bundle.
	 */
	Bundle *newInjectedBundle(XMLTree *evtBundleInjected);
	
	/**
	 * Called to initialize a new bundle that has been received and is
	 * intended for the router. We retain no knowledge of these bundles
	 * (though the policy manager may when it gets invoked).
	 */
	Bundle *newBundleDelivery(XMLTree* evtBundleDelivery);

	/**
	 * Gets the bundle given its GBOF hash key.
	 * 
	 * @param gbofKey GBOF hash key.
	 * @return Bundle object if found, else null.
	 */
	Bundle *getByKey(std::string gbofKey);
	
	
	/**
	 * Called to expire a bundle. Remove knowledge of the bundle from our
	 * tables.
	 * 
	 * Note: The bundles are automatically removed from the data store by
	 * DTN so no additional interaction is required.
	 * 
	 * @param localId local_id of expired bundle.
	 * @return bundle Object representing the expired bundle.
	 */
	Bundle *expire(std::string localId);
		
	/**
	 * Called to remove a specific bundle. Removing involves losing
	 * knowledge of the bundle, but not sending a delete request to DTN2 daemon.
	 * 
	 * @param bundle Bundle to be removed.
	 * @return True if the bundle had been active and therefore removed.
	 */
	bool remove(Bundle *bundle);
	
	/**
	 * Called to delete a specific bundle. This causes a delete request
	 * to also be sent.
	 * 
	 * @param bundle Bundle to be deleted.
	 * @return True if the bundle had been active and therefore deleted.
	 */
	bool deleteBundle(Bundle *bundle);

	/**
	 * Called to delete a bundle. This differs from delete in that here a
	 * delete request is sent to DTN even if the bundle manager does not
	 * know about the bundle, such as with an injected bundle.
	 * 
	 * @param bundle The bundle that is to be deleted.
	 */
	void finished(Bundle *bundle);
	
	/**
	 * Determines if a bundle is current, which is simply its presence
	 * in the active std::map table. A bundle will be removed when it expires
	 * or at the discretion of the policy manager.
	 * 
	 * @param localId local_id to look for.
	 * @return True if current, else false.
	 */
	bool isCurrent(std::string localId);
	
	/**
	 * Given a local id, return the bundle object.
	 * 
	 * @param localId Local id assigned by DTN to the bundle.
	 * @return Bundle object, or null if not found.
	 */
	Bundle *bundlefromLocalId(std::string localId);

	/**
	 * Called when a "bundle_expired_event" is received. Results in the
	 * bundle being removed from our hash of bundles. Note that links
	 * elsewhere may still reference the bundle.
	 *  
	 * @param evtBundleExpired XML element.
	 * @return bundle The expired bundle.
	 */
	Bundle *eventExpired(XMLTree* evtBundleExpired);
	
	/**
	 * Called when a "bundle_delivered_event" is received. Locates the
	 * corresponding bundle object.
	 *  
	 * @param evtBundleDelivered XML element.
	 * @return Bundle that was delivered. This can be null if the bundle is
	 *    a meta data bundle.
	 */
	Bundle *eventDelivered(XMLTree* evtBundleDelivered);

	/**
	 * Create the local bundles summary vector to send to the other peer
	 */

	std::string createSV(std::string type, bool lock);

	/**
	 * Compares the list of received bundles within the SV
	 * and decides on the ones to send to the remote peer
	 * if sendBack = true then, the node should send back its EPIDEMIC_SV_2 otherwise
	 * it has just to send the requested bundles
	 */
	void compareAndSend(std::string remoteSv, Link *link, bool sendBack);

	/*
	 * Shows the available list of bundles
	 */
	void showAvailableBundles();

	time_t getLastTimeBundlesStoreChanged();
	void changeLastTimeBundlesStoreUpdate();

	/**
	 * Add the bundle to the active hash table if it doesn't already exist.
	 *
	 * @param createdBundle The new bundle that is to be added.
	 * @param localId Bundle's local_id: the hash key.
	 * @return The created bundle, or null if already known or not added.
	 */
	Bundle *addIfNew(Bundle *createdBundle, std::string localId);

protected: 

	Handlers *router;

private:

	/**
	 * Extracts the local_id attribute from an XML element.
	 * 
	 * @param element XML element containing the attribute.
	 * @return The local_id as a long.
	 * @throws NoSuchElementException
	 */
	std::string xmlLocalId(XMLTree* element);

	// The map that represent the main bundles buffer
	std::map <std::string,Bundle *> activeBundles;
	std::map <std::string, std::string> localidGBOFMap;
	unsigned int maxBufferCapacity;

	// An integer that indicates whether the HBSD policy is used towards increasing the network average delivery rate (equal to 0) or
	// decreasing its average delivery delay (equal to 1)
	int hbsdOptimizePerformance;

	/**
	 * Determines if a bundle already exists. Obviously, this does not
	 * apply to injected bundles or bundles destined for the router.
	 * 
	 * @param evtBundleRcvd "bundle_received_event" XML message.
	 * @return True if it already exists, else false.
	 */
	bool alreadyExists(XMLTree* evtBundleRcvd);
	

	/**
	 * Try to add the bundle to the active hash table while trying to maximize the overall network average delivery rate.
	 *
	 * @param createdBundle The new bundle that is to be added.
	 * @param localId Bundle's local_id: the hash key.
	 * @return The created bundle, or null if not added.
	 */

	Bundle * hbsdAddBundleAndMaximizeAverageDeliveryRate(Bundle *createdBundle, std::string localId);

	/**
		 * Try to add the bundle to the active hash table while trying to minimize the overall network average delivery delay.
		 *
		 * @param createdBundle The new bundle that is to be added.
		 * @param localId Bundle's local_id: the hash key.
		 * @return The created bundle, or null if not added.
	*/

	Bundle * hbsdAddBundleAndMinimizeAverageDeliveryDelay(Bundle *createdBundle, std::string localId);


	/**
	 * Return the localId and a pointer to the bundle having the smallest utility value within the local buffer.
	 * @param sb a pointer to the bundle having the smallest utility value.
	 * @param metric a pointer to the smallest metric value found in the buffer
	 * @return the bundle localid
	 */
	std::string getBundleWithTheSmallestDRUtilityFromTheBuffer(Bundle * sb, double * smallestUtility);
	std::string getBundleWithTheSmallestDDUtilityFromTheBuffer(Bundle * sb, double * smallestUtility);

	sem_t bundlesLock;
	void getBundlesLock(std::string x);
	void leaveBundlesLock(std::string x);
	sem_t storeStatus;
	time_t lastTimeBundlesStoreChanged;
};

#endif

