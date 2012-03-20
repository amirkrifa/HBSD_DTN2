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


#include "Bundle.h"
#include "stdlib.h"
#include "Util.h"
#include <exception>
#include "Bundles.h"
#include "Policy.h"
#include "XMLTree.h"
#include <time.h>
#include "HBSD.h"
#include "GBOF.h"
#include "Logging.h"
#include <iostream>
#include <xercesc/sax2/DefaultHandler.hpp>
#include <xercesc/sax2/Attributes.hpp>
using namespace std;


Bundle::Bundle(Bundles *manager)
{
	injected = false;
	bytesReceived = 0;
	numMetaBlocks = 0;
	expiration = 0;
	creationSeconds = 0;
	expiresAtMillis = 0; // Not set for injected & delivery bundles!
	fragLength = 0;
	fragOffset = 0;
	isFragment = false;
	bundleManager = manager;
}

Bundle::~Bundle()
{
	bundleManager = NULL;
}

bool Bundle::initReceived(XMLTree* evtBundleRcvd)
{
	try
	{
		// This bundle was received, not injected.
		// Get mandatory attributes. Convert them to their integer/long
		// value.
		localId = evtBundleRcvd->getAttrRequired(string("local_id"));

		string strExpiration = evtBundleRcvd->getAttrRequired(string("expiration"));
		expiration = strtol(strExpiration.c_str(), NULL, 10);

		string strBytesReceived = evtBundleRcvd->getAttrRequired(string("bytes_received"));
		bytesReceived = atoi(strBytesReceived.c_str());

		// Optional attributes.
		if (evtBundleRcvd->haveAttr(string("num_meta_blocks")))
		{
			string strNumMetaBlocks = evtBundleRcvd->getAttr(string("num_meta_blocks"));
			numMetaBlocks = atoi(strNumMetaBlocks.c_str());
		}

		// Now parse the child elements.
		int nElements = evtBundleRcvd->numChildElements();

		for (int x=0; x<nElements; x++)
		{
			XMLTree *el = evtBundleRcvd->getChildElement(x);
			assert(el != NULL);
			if (el->getName().compare("gbof_id") == 0)
			{
				parseGBOF(el);
			} else if (el->getName().compare("dest") == 0)
			{
				destURI = el->getAttrRequired(string("uri"));
				destEID = GBOF::eidFromURI(destURI);
				assert(!destURI.empty());
				assert(!destEID.empty());

			} else if (el->getName().compare("custodian") == 0)
			{
				custodianURI = el->getAttrRequired(string("uri"));
			} else if (el->getName().compare("replyto") == 0)
			{
				replyToURI = el->getAttrRequired(string("uri"));

			} else if (el->getName().compare("prevhop") == 0)
			{
				prevHopURI = el->getAttrRequired(string("uri"));
			}
		}

		// Special case: the bundle is destined for this router. If that is the
		// case then we should have already received a bundle_delivery_event for
		// this bundle. So, after all that work, forget about it.

		// Time in Mills at which this bundle will expire
		expiresAtMillis = GBOF::calculateExpiration(strtol((char*)creationTimestamp.c_str(), NULL, 10), expiration);

		if (destURI.compare(HBSD::hbsdRegistration) == 0)
		{
			return false;
		}

		return true;
	}
	catch (exception &e)
	{
		if (HBSD::log->enabled(Logging::ERROR))
		{
			HBSD::log->error(string("Unanticipated exception initializing Bundle object: ") + string(e.what()));
		}
		return false;
	}

}


string Bundle::initGeneric(XMLTree* event)
{
try 
{
	string strExpiration = event->getAttrRequired(string("expiration"));
	expiration = strtol(strExpiration.c_str(), NULL, 10);

	// Get the local id of the bundle.
	localId = event->getAttrRequired(string("local_id"));
	assert(!localId.empty());
	// Now parse the GBOF.
	XMLTree *el = event->getChildElementRequired(string("gbof_id"));

	assert(el != NULL);
	parseGBOF(el);

	// Time in Mills at which this bundle will expire
	expiresAtMillis = GBOF::calculateExpiration(strtol((char*)creationTimestamp.c_str(), NULL, 10), expiration);

	return localId;
} 

catch (exception &e) 
{
	if (HBSD::log->enabled(Logging::ERROR)) 
	{
		HBSD::log->error(string("Unanticipated exception initializing Bundle object: ") + string(e.what()));
	}
	return NULL;
}
}

void Bundle::parseGBOF(XMLTree *element)  
{
	assert(element != NULL);
	creationTimestamp = element->getAttrRequired(string("creation_ts"));
	creationSeconds = GBOF::creationSeconds(strtol((char*)creationTimestamp.c_str(), NULL, 10));
	fragLength = atoi((element->getAttrRequired(string("frag_length"))).c_str());
	fragOffset = atoi((element->getAttrRequired(string("frag_offset"))).c_str());
	isFragment = (bool)atoi((element->getAttrRequired(string("is_fragment"))).c_str());
	XMLTree *el = element->getChildElementRequired(string("source"));
	sourceURI = el->getAttrRequired(string("uri"));
	sourceEID = GBOF::eidFromURI(sourceURI);
	el = NULL;
}

bool Bundle::isCurrent() 
{
	return bundleManager->isCurrent(localId);
}
	
void Bundle::deleteBundle() 
{
	bundleManager->deleteBundle(this);
}

void Bundle::finished() 
{
	bundleManager->finished(this);
}


long Bundle::getElapsedTimeSinceCreation()
{
	return Util::getCurrentTimeSeconds() - this->creationSeconds;
}

bool Bundle::isMeDeHaBundle()
{
	try
	{

		if(!sourceURI.empty())
		{
			size_t pos = sourceURI.find_last_of("/");
			if(pos != string::npos)
			{
				string app = sourceURI.substr(pos + 1, sourceURI.length());
				if(HBSD::log->enabled(Logging::INFO))
				{
					HBSD::log->info(string("Verifying if the bundle has as a destination a Medeha Gateway: ")+app);
				}
				if(!app.empty())
					return (app.compare(string("Medeha")) == 0);
				else return false;
			}else
			{
				HBSD::log->error(string("Invalid source URI: ") + sourceURI);
			}

			return false;

		}
	}

	catch(exception &e)
	{
		if(HBSD::log->enabled(Logging::ERROR))
			HBSD::log->error(string("Error parsing injected bundle element: ") + string(e.what()) + string("sourceURI: ") + sourceURI );
		exit(1);
	}
}
