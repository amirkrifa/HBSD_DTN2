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


#ifndef MEDEHA_INTERFACE_H
#define MEDEHA_INTERFACE_H

#include <string>
#include <pthread.h>
#include <semaphore.h>
#include <string>

#define DEFAULT_MEDEHA_INTERFACE_PORT 2617
#define MAX_REQUEST_MESSAGE_PARTS 2
#define DEFAULT_MAX_DATA_LENGTH 1024
#define DEFAULT_MEDEHA_DAEMON_PORT 2618

class MeDeHaInterface;

typedef struct ThreadParamMeDeHA
{
	MeDeHaInterface * medehaInterface;
}ThreadParamMeDeHA;

class Handlers;

class MeDeHaInterface
{

public:

	/**
	 * Constructor.
	 */

	MeDeHaInterface(Handlers *router);

	/**
	 * Destructor.
	 */

	~MeDeHaInterface();

	/**
	 * Starts the MeDeHaInterface thread.
	 */
	void init();

	//////////////////////////// MeDeHaInterface Thread ////////////////////////////

	/**
	 * Main loop of the MeDeHaInterface thread. Wait for any coming request from the
	 * MeDeHa overlay.
	 */

	static void  * run(void * arg);

	/**
	 * Called when a request has been received from a peer router.
	 * Read in the data, then call the policy manager to deal with it.
	 *
	 * @param string Object that contains the request.
	 */

	int processMeDeHaRequest(std::string request, int & command, bool & idAvailability, std::string &listEIDS, bool & injected);

	/**
		* Opening and binding the MeDeHaInterface Socket
		* @param int that represent the port number on which MeDeHaInterface will run on.
	*/

	int initUDPSocketListener(int hostPort);


	/**
	 * Used to stop the interface thread
	 */
	void stopsInterfaceThread();

	/**
	 *  Verifies whether the interface thread should be stopped or not
	 *  @return bool true if the interface should be stopped and false otherwise
	 */

	bool interfaceThreadStatus();

	int getRequestCommandPart(char *completeRequest, int partNumber, char * resultingPart);

	void sendBackDataToMeDeHaDaemon(std::string data, std::string src);

protected:

	Handlers *router;
	bool stopInterfaceThread;
	sem_t protectStopFlag;
};
#endif
