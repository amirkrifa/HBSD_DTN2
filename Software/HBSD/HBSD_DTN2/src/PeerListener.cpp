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


#include "PeerListener.h"
#include "XMLTree.h"
#include "Handlers.h"
#include <exception>
#include <fstream>
#include "HBSD.h"
#include <assert.h>
#include "MeDeHaInterface.h"
using namespace std;

PeerListener::PeerListener(Handlers *router) 
{
	assert(router != NULL);
	this->router = router;
	continueRunning = true;
	sem_init(&threadStatusLock, 0, 1);
	sem_init(&msgQueueLock, 0, 1);

}

PeerListener::~PeerListener() 
{
	sem_destroy(&threadStatusLock);
	sem_destroy(&msgQueueLock);
	while(!msgQueue.empty())
	{
		delete(msgQueue.front());
		msgQueue.pop();
	}

	router = NULL;
}

void PeerListener::init()
{
	pthread_t thread;
	pthread_attr_t threadAttr;

	if(pthread_attr_init(&threadAttr) != 0)
	{
		if(HBSD::log->enabled(Logging::FATAL))
			HBSD::log->fatal(string("Error occurred while initializing the PeerListener thread"));
		exit(1);
	}
	
	// Creates a detached Thread

	if(pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED) != 0)
	{
		if(HBSD::log->enabled(Logging::FATAL))
			HBSD::log->fatal(string("Error occurred while configuring the PeerListener thread to be detached"));
		exit(1);
	}

	ThreadParam * tp = (ThreadParam *)malloc(sizeof(ThreadParam));
	if(tp == NULL)
	{
		if(HBSD::log->enabled(Logging::FATAL))
			HBSD::log->fatal(string("Problem occurred while allocate PeerListener thread parameter"));
		exit(1);
	}

	tp->peerListener = this;	

	
	if(pthread_create (&thread, &threadAttr, PeerListener::run, (void *)tp) < 0 )
	{
		if(HBSD::log->enabled(Logging::FATAL))
			HBSD::log->fatal(string("Unable to create the PeerListener thread."));
		exit(1);
	}

	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("PeerListener thread loaded."));
	// Cleaning
	tp = NULL;
}



void PeerListener::eventDelivery(XMLTree *evtBundleDelivery, Bundle *bundle) 
{
	assert(evtBundleDelivery != NULL);
	assert(bundle != NULL);

	try 
	{
		// Make certain it is intended for the HBSD router. The endpoint name is configurable.
		XMLTree *bundleEvt = evtBundleDelivery->getChildElementRequired(string("bundle"));
		assert(bundleEvt != NULL);

		XMLTree *el = bundleEvt->getChildElementRequired(string("dest"));
		assert(el != NULL);

		string uri = el->getAttrRequired(string("uri"));
		assert(!uri.empty());

		// Verifying if the message is intended from the MeDeHa daemon
		if(bundle->isMeDeHaBundle())
		{
			// Forward to medeha daemon
			PeerBundle *peerBundle = new PeerBundle(bundle);


			el = bundleEvt->getChildElementRequired(string("payload_file"));
			assert(el != NULL);

			if(HBSD::log->enabled(Logging::INFO))
						HBSD::log->info(string("Receiving a MeDeHa payload file: ") + el->getValue());


			peerBundle->payloadFile = el->getValue();

			sem_wait(&msgQueueLock);
				msgQueue.push(peerBundle);
			sem_post(&msgQueueLock);
			peerBundle = NULL;

		}
		else
		{
			if (uri.compare(HBSD::hbsdRegistration) != 0)
			{
				if (HBSD::log->enabled(Logging::DEBUG))
				{
					HBSD::log->debug("Ignoring router bundle not intended for HBSD: " + uri);
				}

				if(bundle != NULL)
					delete bundle;
				el = NULL;
				bundleEvt = NULL;
				return;
			}

			// Note the name of the file containing the payload. Invoke
			// the thread to run asynchronously.

			PeerBundle *peerBundle = new PeerBundle(bundle);


			el = bundleEvt->getChildElementRequired(string("payload_file"));
			assert(el != NULL);

			if(HBSD::log->enabled(Logging::INFO))
						HBSD::log->info(string("Receiving a meta data bundle with attached payload file: ") + el->getValue());

			peerBundle->payloadFile = el->getValue();

			sem_wait(&msgQueueLock);
				msgQueue.push(peerBundle);
			sem_post(&msgQueueLock);
			peerBundle = NULL;
		}

		el = NULL;
		bundleEvt = NULL;

	} 

	catch (exception &e) 
	{
		if (HBSD::log->enabled(Logging::ERROR))
		{
			HBSD::log->debug(string("Problem with delivery XML message: ") + string(e.what()));
		}
		if(bundle != NULL)
			delete bundle;

		return;
	}
	
}


void *PeerListener::run(void *arg) 
{
	ThreadParam * recvArg =(ThreadParam*)arg;
	assert(recvArg != NULL);

	if(recvArg->peerListener == NULL)
	{
		if(HBSD::log->enabled(Logging::ERROR))
			HBSD::log->error(string("Invalid arguments passed to PeerListener thread"));
		exit(1);
	}
	PeerBundle *peerBundle = NULL;
	
	while (recvArg->peerListener->getStatus())
	{
		try 
		{
			recvArg->peerListener->lockMsgQueue();

			if(recvArg->peerListener->msgQueue.empty())
			{
				recvArg->peerListener->unlockMsgQueue();
				continue;
			}

			peerBundle = recvArg->peerListener->msgQueue.front();

			recvArg->peerListener->msgQueue.pop();
			recvArg->peerListener->unlockMsgQueue();

			recvArg->peerListener->processPeerMessage(peerBundle);
			if(peerBundle != NULL)
				delete peerBundle;

		} 

		catch (exception &e) 
		{
			if (HBSD::log->enabled(Logging::ERROR)) 
			{
				HBSD::log->error(string("Unanticipated exception in PeerListener thread: ") + string(e.what()));
			}
		}
	}

	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("Shutting down the PeerListener thread."));

	if(recvArg != NULL)
		free(recvArg);
	return NULL;
}
	
void PeerListener::processPeerMessage(PeerBundle *peerBundle) 
{
	assert(peerBundle != NULL);
	try 
	{

		// Notify the Policy Manager.
		string msgSrc = peerBundle->bundle->sourceEID;

		if(peerBundle->bundle->isMeDeHaBundle())
		{
			// Get the data from the file.
			HBSD::log->info(string("Getting MeDeHa data from the payload file: ")+peerBundle->payloadFile);
			ifstream dataFile;
			dataFile.open (peerBundle->payloadFile.c_str(), ios::out);
			string data;
			string line;
			while(dataFile >> line)
			{
					data.append(line);
			}

			dataFile.close();
			// Forward the message to the MeDeHa daemon
			((HBSD_Routing*)router)->medehaInterface->sendBackDataToMeDeHaDaemon(data, msgSrc);
		}
		else
		{
			HBSD::log->info(string("Getting HBSD data from the payload file: ")+peerBundle->payloadFile);

			ifstream dataFile;
			dataFile.open (peerBundle->payloadFile.c_str(), ios::out);

			string data;
			string line;
			string type;
			dataFile >> type;

			while(dataFile >> line)
			{
					data.append(line);
			}

			dataFile.close();
			assert(!type.empty());

			// Request that the bundle be deleted. This removes the file.
			HBSD::requester->requestDeleteBundle(peerBundle->bundle);

			router->policyMgr->metaDataReceived(msgSrc, data, type);
		}

	}

	catch (exception &e) 
	{
		// Failed to access file. Try to be helpful in isolating the
		// problem since it may be due to file or directory access
		// restrictions that are otherwise difficult to troubleshoot.
		if (HBSD::log->enabled(Logging::ERROR)) 
		{
			HBSD::log->error(string("Unable to access bundle payload: ") + string(e.what()));
		}
	}
}


