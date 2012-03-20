/*
Copyright (C) 2010  INRIA, Plan��te Team

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


#ifndef BUNDLE_H
#define BUNDLE_H

#include <string>
#include <list>

class Bundles;
class Policy;
class XMLTree;
class GBOF;

class Bundle
{ 

public:	
	// Values set when created. These can be read by others.
	bool statisticsInfo;
	bool injected;
	std::string localId;
	int bytesReceived;
	int numMetaBlocks;
	long expiration;
	std::string creationTimestamp;
	long creationSeconds;
	long expiresAtMillis; // Not set for injected & delivery bundles!
	std::string sourceURI;
	std::string sourceEID;
	int fragLength;
	int fragOffset;
	bool isFragment;
	std::string destURI;
	std::string destEID;
	std::string custodianURI;
	std::string replyToURI;
	std::string prevHopURI;
	std::string gbofKey;

	/**
	* Constructor: sets the Bundles class that maintains knowledge of 
	* all bundles.
	* @param manager Bundles class that is creating this bundle.
	*/
	Bundle(Bundles *manager);
	~Bundle();
			
	/**
	* Called to initialize the Bundle object. Parses the
	* "bundle_received_event" XML data.
	* 
	* Note: This method should only be called by the Bundles class.
	* 
	* @param evtBundleRcvd Root of the bundle_received_event.
	* @return true if the bundle is correctly initialized and is not a metadata
	*/
		
	bool initReceived(XMLTree* evtBundleRcvd);
	
	/**
	* Called to initialize the Bundle object when a "generic"
	* event is received. Generic means that the event contains a
	* local_id attribute and has a gbof_id child element. This is
	* typically used for events such as "bundle_injected_event" and
	* "bundle_delivery_event".
	* 
	* Note: This method should only be called by the Bundles class.
	* 
	* @param event XML root.
	* @return The bundle's local_id, or NULL if an error was encountered.
	*/
	std::string initGeneric(XMLTree* event);
	
	/**
	* Returns the elapsed time in seconds since the bundle was created.
	* 
	* @param element Root of the gbof_id element.
	* @throws NoSuchElementException via XMLTree access routines.
	*/
	
	long getElapsedTimeSinceCreation();

	bool isMeDeHaBundle();
private:

	/**
	* Parses the "gbof_id" element. Stores the information in variables
	* global to this object.
	*
	* @param element Root of the gbof_id element.
	* @throws NoSuchElementException via XMLTree access routines.
	*/

	void parseGBOF(XMLTree *element);
	
	/**
	* Called to determine if the bundle is current, i.e. not expired and
	* not delivered. We query the bundle manager for this information.
	* 
	* @return True if current, else false.
	*/
	
	bool isCurrent();
	
	/**
	* Called to delete this bundle. Invoke the bundle manager to do the
	* actual work.
	* 
	* Note: This should normally be called by the policy manager.
	*/
	
	void deleteBundle();
				
	/**
	* Called to delete a bundle. This differs from delete in that here a
	* delete request is sent to DTN even if the bundle manager does not
	* know about the bundle, such as with an injected bundle.
	*/
	
	void finished();


	Bundles *bundleManager;
};

#endif

