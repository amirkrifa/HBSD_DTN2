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


#ifndef PEER_LISTENER_H
#define PEER_LISTENER_H

#include <string>
#include <queue>
#include <pthread.h>
#include <semaphore.h>
#include "Bundle.h"
#include <iostream>

class PeerListener;

typedef struct ThreadParam
{
	PeerListener * peerListener;	
}ThreadParam;


class Handlers;
class XMLTree;
class Bundle;

class  PeerBundle
{
public:
	PeerBundle(Bundle * b)
	{
		bundle = b;
	}
	~PeerBundle()
	{
		if(bundle != NULL)
			delete bundle;
	}

	Bundle *bundle;
	std::string payloadFile;
};


class PeerListener
{

public:
	
	/**
	 * Constructor. Create the queue used to wake up the thread.
	 */
	PeerListener(Handlers *router);

	/**
	 * Destructor.
	 */
	
	~PeerListener();
	/**
	 * Starts the listener thread.
	 */
	void init();
		
	/**
	 * Called when a "bundle_delivery_event" is received. This event marks
	 * receipt of a bundle from a peer router. Create a PeerBundle object
	 * from data in the XML, then notify this class's thread of the bundle.
	 * 
	 * @param evtBundleDelivery XML root of the event.
	 * @param bundle Bundle object for the delivered bundle.
	 */
	void eventDelivery(XMLTree *evtBundleDelivery, Bundle *bundle);
	
	//////////////////////////// Listener Thread ////////////////////////////
	
	/**
	 * Main loop of the PeerListener thread. Wait for any peer bundles
	 * that have been received.
	 */
	static void  * run(void * arg);
	
	/**
	 * Called when a bundle has been received from a peer router.
	 * Read in the data, then call the policy manager to deal with it.
	 * The data store is requested to delete the bundle after we have
	 * read the data.
	 * 
	 * @param peerBundle Object that contains the Bundle object and
	 *    location of the payload file.
	 */
	void processPeerMessage(PeerBundle *peerBundle);

	bool getStatus()
	{
		bool tmp;
		sem_wait(&threadStatusLock);
		tmp = continueRunning;
		sem_post(&threadStatusLock);
		return tmp;
	}

	void stopThread()
	{
		sem_wait(&threadStatusLock);
		continueRunning = false;
		sem_post(&threadStatusLock);
	}

	void lockMsgQueue()
	{
		sem_wait(&msgQueueLock);
	}
	void unlockMsgQueue()
	{
		sem_post(&msgQueueLock);
	}

protected:

	Handlers *router;

private:
	bool continueRunning;
	std::queue<PeerBundle *> msgQueue;

	sem_t threadStatusLock;
	sem_t msgQueueLock;
};



#endif

