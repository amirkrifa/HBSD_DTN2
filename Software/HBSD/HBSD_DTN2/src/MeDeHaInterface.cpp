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


#include "MeDeHaInterface.h"
#include "Handlers.h"
#include <exception>
#include <iostream>
#include <fstream>
#include "HBSD.h"
#include <list>
#include "Bundles.h"
#include "HBSD_Policy.h"
using namespace std;

MeDeHaInterface::MeDeHaInterface(Handlers *router)
{
	this->router = router;
	this->stopInterfaceThread = false;
	sem_init(&protectStopFlag, 0, 1);
}


MeDeHaInterface::~MeDeHaInterface()
{
	router = NULL;
	sem_destroy(&protectStopFlag);
}

void MeDeHaInterface::init()
{
	pthread_t thread;
	pthread_attr_t threadAttr;

	if(pthread_attr_init(&threadAttr) != 0)
	{
		if(HBSD::log->enabled(Logging::FATAL))
			HBSD::log->fatal(string("Error occurred while initializing the MeDeHaInterface thread"));
		exit(1);
	}

	// Creates a detached Thread

	if(pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED) != 0)
	{
		if(HBSD::log->enabled(Logging::FATAL))
			HBSD::log->fatal(string("Error occurred while configuring the MeDeHaInterface thread to be detached"));
		exit(1);
	}

	ThreadParamMeDeHA * tp = (ThreadParamMeDeHA *)malloc(sizeof(ThreadParamMeDeHA));
	if(tp == NULL)
	{
		if(HBSD::log->enabled(Logging::FATAL))
			HBSD::log->fatal(string("Problem occurred while allocate MeDeHaInterface thread parameter"));
		exit(1);
	}

	tp->medehaInterface = this;

	if(tp->medehaInterface == NULL)
	{
		if(HBSD::log->enabled(Logging::FATAL))
			HBSD::log->fatal(string("MeDeHaInterface thread parameter was not correctly initialized"));
	}

	if(pthread_create (&thread, &threadAttr, MeDeHaInterface::run, (void *)tp) < 0 )
	{
		if(HBSD::log->enabled(Logging::FATAL))
			HBSD::log->fatal(string("Unable to create the MeDeHaInterface thread."));
	}

	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("MeDeHaInterface thread loaded."));

	tp = NULL;
}

void * MeDeHaInterface::run(void * arg)
{
	ThreadParamMeDeHA * recvArg =(ThreadParamMeDeHA*)arg;
	assert(recvArg != NULL);

	if(recvArg->medehaInterface == NULL)
	{
		if(HBSD::log->enabled(Logging::INFO))
			HBSD::log->info(string("Invalid arguments passed to MeDeHaInterface thread"));
	}

	// Opening the listening socket
	int lsock = recvArg->medehaInterface->initUDPSocketListener(DEFAULT_MEDEHA_INTERFACE_PORT);
	char buf[DEFAULT_MAX_DATA_LENGTH];

	while (!recvArg->medehaInterface->interfaceThreadStatus())
	{
		// Waiting for a new request
		int rlen = 0, fromlen = 0;

		struct  sockaddr_in from;
		fromlen = sizeof(from);

		memset(buf,'\0', DEFAULT_MAX_DATA_LENGTH);

		rlen = recvfrom(lsock, buf, DEFAULT_MAX_DATA_LENGTH, 0, (struct sockaddr *)&from, (socklen_t*)&fromlen);

	    if (rlen == -1)
		{
	    	if(HBSD::log->enabled(Logging::FATAL))
	    		HBSD::log->fatal(string("Error occurred while trying to receive a new request coming from MeDeHa"));
			continue;
		}
	    else
		{
	    	// A new request is received
			// Proceeding the request
			int commandNumber = -1;
			bool idAvailability;
			string listEIDS;
			bool injected;

			if(HBSD::log->enabled(Logging::INFO))
				HBSD::log->info(string("New MeDeHa request received: ")+string(buf) );

			recvArg->medehaInterface->processMeDeHaRequest(string(buf), commandNumber, idAvailability, listEIDS, injected);

			switch(commandNumber)
			{
			case 1:
					// Inject Bundle
					if(HBSD::log->enabled(Logging::INFO))
						HBSD::log->info("A new MedeHa request for injecting a Bundle is received");
					break;
			case 2:
					// Request list EIDs
					if(HBSD::log->enabled(Logging::INFO))
						HBSD::log->info(string("A new MeDeHa request is received for getting  the list of available EIDs"));
					// Send back the list of EIDs
					if(sendto(lsock, (char *)listEIDS.c_str(), listEIDS.length(), 0, (struct sockaddr *)&from, sizeof(from)) == -1)
					{
						if(HBSD::log->enabled(Logging::ERROR))
							HBSD::log->error(string("Error occurred when trying to send back the list of EIDs to MeDeHa daemon."));
					}else
					{
						if(HBSD::log->enabled(Logging::INFO))
							HBSD::log->info(string("The list of EIDs was successful sent to MeDeHa daemon: ") + listEIDS);
					}
					break;
			case 3:
					// Verify whether the EID is available or not.
				if(HBSD::log->enabled(Logging::INFO))
					HBSD::log->info(string("A new MeDeHa request is received to verify whether an EID exists or not"));
				// Sending back the answer
				if(sendto(lsock, (char*)Util::to_string(idAvailability).c_str(), Util::to_string(idAvailability).length(), 0, (struct sockaddr *)&from, sizeof(from)) == -1)
				{
					if(HBSD::log->enabled(Logging::ERROR))
						HBSD::log->error(string("Error occurred when trying to send back the availability status of the requested EID."));
				}else
				{
					if(HBSD::log->enabled(Logging::INFO))
						HBSD::log->info(string("The availability status of the requested EID was successful sent to MeDeHa daemon"));
				}
				break;
			default:
				if(HBSD::log->enabled(Logging::INFO))
					HBSD::log->error(string("Invalid request number, ignoring the request"));
			}
		}
	}

	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("Closing MeDeHa Interface."));

	free(recvArg);
	return NULL;
}

int MeDeHaInterface::processMeDeHaRequest(std::string request, int & command, bool & idAvailability, string &listEIDS, bool & injected)
{
	// Parse the received request
	char resultingPart[DEFAULT_MAX_DATA_LENGTH];
	if(this->getRequestCommandPart((char*)request.c_str(), 0, resultingPart) < 0)
	{
		return -1;
	}

	if(strcmp(resultingPart, "INJECTBUNDLE") == 0)
	{
		// A Request for injecting a Bundle is received
		// Command number 1
		command = 1;
		// Get the dest EID
		char destEID[DEFAULT_MAX_DATA_LENGTH];
		if(getRequestCommandPart((char*)request.c_str(), 1, destEID) < 0)
					return -1;

		// Get the data to be sent
		char data[DEFAULT_MAX_DATA_LENGTH];
		if(getRequestCommandPart((char*)request.c_str(), 2, data) < 0)
					return -1;


		// Encapsulating the data received from MeDeHa and forwarding it
		// Create a file to hold the bundle data
		// Name of tmp file
		string tmpName;
		tmpName.append(string("/home/amir/hbsd_medeha_bundle_")+ Util::to_string(HBSD::requester->getAndIncrement()));

		ofstream tmpFile;
		tmpFile.open(tmpName.c_str());
		tmpFile << data<<endl;
		tmpFile.close();


		string bundleSourceURI = HBSD::hbsdRegistration +  string("/Medeha");
		string bundleDestURI = destEID + string("/") + HBSD::routerEndpoint + string("/Medeha");

		// Injection request ID
		string reqId;
		HBSD::log->info(string("Injecting a medeha bundle, src: ") + bundleSourceURI + string(" dest: ") + bundleDestURI);
		reqId = HBSD::requester->requestInjectMeDeHaBundle(bundleSourceURI, bundleDestURI, tmpName);

		if(reqId.empty())
		{
					if(HBSD::log->enabled(Logging::ERROR))
				HBSD::log->error(string("Unable to inject the MeDeHa bundle"));
		}else
		{
			((HBSD_Policy*)((HBSD_Routing*)this->router)->policyMgr)->addNewMeDeHaInjectedBundle(reqId, bundleSourceURI, bundleDestURI);
			if(HBSD::log->enabled(Logging::INFO))
				HBSD::log->info(string("The MeDeHa bundle is correctly injected."));
		}

	}
	else if(strcmp(resultingPart, "GETLISTEID") == 0)
	{
		// Command number 2
		// MeDeHa is requesting the list of available EIDs
		command = 2;
		if(((HBSD_Routing*)this->router)->enableOptimization())
		{
			// The statistics based optimization is enabled, get and return the list of nodes from th
			// statistics manager
			listEIDS = ((HBSD_Routing*)this->router)->nodes->getListOfAvailableNodes();
			listEIDS.append("#");
		}
		else
		{
			// Returns only the available nodes
			listEIDS = ((HBSD_Routing*)this->router)->nodes->getListOfAvailableNodes();
			listEIDS.append("#");

		}
	}
	else if(strcmp(resultingPart, "FIND") == 0)
	{
		// Command number 3
		// Verifying whether the provided EID is already available in the DTN2 powered network
		// Getting the EID
		command = 3;
		if(getRequestCommandPart((char*)request.c_str(), 1, resultingPart) < 0)
		{
			return -1;
		}
		if(((HBSD_Routing*)this->router)->enableOptimization())
		{
			// Statistics based optimization enabled
			// Asking the Statistics Manager to verify the existence of the Received EID
			idAvailability = ((HBSD_Routing*)this->router)->statisticsManager->isStatNodeAvailable(string(resultingPart));
		}
		else
		{
			// Statistics based optimization disabled
			idAvailability = (((HBSD_Routing*)this->router)->nodes->findNode(string(resultingPart)) != NULL);
		}
	}
	else
	{
		// Unknown request
		if(HBSD::log->enabled(Logging::ERROR))
			HBSD::log->error(string("Unknown request message received from MeDeHa"));
		return -1;
	}

	// Provide back an answer
	return -1;
}



int MeDeHaInterface::getRequestCommandPart(char *complete_request, int part_number, char * resulting_part)
{

	assert(complete_request != NULL);

	memset(resulting_part, '\0',DEFAULT_MAX_DATA_LENGTH);

	if(part_number <= MAX_REQUEST_MESSAGE_PARTS)
	{
		/* Looking for a field from the network layer */

		int i=0;
		int error=0;
		char * sep_position = NULL;
		char * prev_sep_position = NULL;

		// Asking for the first part of the request message
		if(part_number == 0)
		{
			sep_position=strchr(complete_request,'#');
			if(strncpy(resulting_part, complete_request, sep_position - complete_request) == NULL)
			{
				if(HBSD::log->enabled(Logging::FATAL))
					HBSD::log->fatal(string("Error occurred while parsing the received request from MeDeHa e1"));
				return -1;
			}
			else return 0;
		}

		// Asking for another part of the request
		prev_sep_position = complete_request;
		sep_position = complete_request;

		for(;i <= part_number; i++)
		{
			prev_sep_position = sep_position;
			if(i == MAX_REQUEST_MESSAGE_PARTS )
			{
				/*The last part of the message*/

				if(strncpy(resulting_part, prev_sep_position + 1, (complete_request+ strlen(complete_request))-prev_sep_position-1) == NULL)
				{
					if(HBSD::log->enabled(Logging::FATAL))
						HBSD::log->fatal(string("Error occurred while parsing the received request from MeDeHa e2"));
					return -1;
				}
				else return 0;
			}else if((sep_position = strchr(prev_sep_position + 1, '#')) == NULL)
			{
				error = 1;
				break;
			}
		}

		if(!error)
		{
			if(strncpy(resulting_part,prev_sep_position+1,sep_position-prev_sep_position-1)==NULL)
			{
				if(HBSD::log->enabled(Logging::FATAL))
					HBSD::log->fatal(string("Error occurred while parsing the received request from MeDeHa e3"));
				return -1;
			}
			else return 0;
		}
		else
		{
			if(HBSD::log->enabled(Logging::FATAL))
				HBSD::log->fatal(string("Error occurred while parsing the received request from MeDeHa e4"));
			return -1;
		}
	} else
	{
		if(HBSD::log->enabled(Logging::FATAL))
			HBSD::log->fatal(string("Error occurred while parsing the received request from MeDeHa e5"));
		return -1;
	}
}


/*Listener base on an UDP AF_INET socket */
int MeDeHaInterface::initUDPSocketListener(int hostPort)
{
	int     sock;
    struct  sockaddr_in servaddr;

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		if(HBSD::log->enabled(Logging::FATAL))
			HBSD::log->fatal(string("Error occurred while trying to open the MeDeHa interface socket"));
		exit(1);
    }

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(hostPort);

	if (bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
                close(sock);
                if(HBSD::log->enabled(Logging::FATAL))
                		HBSD::log->fatal(string("Error occurred while trying to bind the MeDeHa interface socket"));
                exit(1);
    }
	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("MeDeHaInterface thread is listennig at port: ")+Util::to_string(hostPort));
	return sock;
}


void MeDeHaInterface::stopsInterfaceThread()
{
	sem_wait(&protectStopFlag);
	stopInterfaceThread = true;
	sem_post(&protectStopFlag);
}

bool MeDeHaInterface::interfaceThreadStatus()
{
	bool tmp;
	sem_wait(&protectStopFlag);
	tmp = this->stopInterfaceThread;
	sem_post(&protectStopFlag);
	return tmp;
}

void MeDeHaInterface::sendBackDataToMeDeHaDaemon(std::string data, std::string src)
{
	int     sock;
    struct  sockaddr_in receiver_adr;

	if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		if(HBSD::log->enabled(Logging::FATAL))
			HBSD::log->fatal(string("Error occurred while trying to open a socket towards MeDeHa daemon"));
		exit(1);
    }

	bzero(&receiver_adr, sizeof(receiver_adr));
	receiver_adr.sin_family = AF_INET;
	receiver_adr.sin_addr.s_addr = inet_addr("10.0.1.26");
	receiver_adr.sin_port = htons(DEFAULT_MEDEHA_DAEMON_PORT);

	string msg;

	try
	{
		msg.append(data);
		msg.append("#");
		msg.append(src);
	}
	catch(exception &e)
	{
		if(HBSD::log->enabled(Logging::ERROR))
			HBSD::log->error(string("Exception occured while trying to read the MeDeHa data file: ") + data);
		close(sock);
		return;
	}
	if (sendto(sock, (char*)msg.c_str(), msg.length(), 0, (struct sockaddr*)&receiver_adr, sizeof(receiver_adr)) < 0)
	{
                close(sock);
                if(HBSD::log->enabled(Logging::FATAL))
                		HBSD::log->fatal(string("Error occurred while trying to bind the MeDeHa interface socket"));
                exit(1);
    }

	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("Data sent back to MeDeHa daemon: ") + msg);

}
