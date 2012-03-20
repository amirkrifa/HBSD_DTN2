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


#include "HBSD.h"
#include <stdio.h>
#include <exception>
#include <stdlib.h>
#include <fstream>
#include "HBSD_Routing.h"
#include "HBSD_SAX.h"
#include <netinet/in.h>
#include <netdb.h>
#include "Util.h"
#include "Console_Logging.h"
#include "Requester.h"
#include <iostream>
#include "ConfigFile.h"
#include <xercesc/sax2/XMLReaderFactory.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>

using namespace std;
using namespace xercesc;

string HBSD::routerEndpoint;
string HBSD::loggingClass;
	
// "Global" values
Logging *HBSD::log;
ConfigFile * HBSD::routerConf;
string HBSD::localEID;
Requester* HBSD::requester;
string HBSD::hbsdRegistration;

int HBSD::dtndSocket;
struct sockaddr_in HBSD::dtndSocketAddr;

// Command line options
string HBSD::confFile;
bool HBSD::xmlSchemaOpt;
bool HBSD::logLevelOpt;
string HBSD::hbsdVersion;
string HBSD::defaultLoggingClass;

// Configurable values
string HBSD::multicastGroup;
int HBSD::multicastPort;
string HBSD::xmlSchema;
string HBSD::logConfiguration;
int HBSD::logLevel;
HBSD_Routing *HBSD::handlerHBSD;
HBSD_SAX* HBSD::saxHandler;
SAX2XMLReader* HBSD::saxReader;
string HBSD::LOOPBACKADR;
bool HBSD::staticInitialized;

HBSD::HBSD(int thrd) 
{
	if(!staticInitialized)
	{
		xmlSchemaOpt = false;
		logLevelOpt = true;
		hbsdVersion.assign("1.0");
		// we setup the default logging to the Console_Logging
		defaultLoggingClass.assign("Console_Logging");
		// Multicast port on which we'll received messages from DTN2
		multicastPort = 8001;
		// Configurable values
		multicastGroup.assign("224.0.0.2");
		// Handler to the HBSD_Router
		handlerHBSD = NULL;
		// Sax Handler
		saxHandler = NULL;
		// Handler to Sax Reader
		saxReader = NULL;
		// Settingup the loopbackAdr
		LOOPBACKADR.assign("127.0.0.1");
		requester = NULL;
		log = NULL;
		routerConf = NULL;
		// The default router End Point id
		routerEndpoint.assign("ext.rtr/HBSD");
		// Setting the logging class to the default one
		loggingClass = defaultLoggingClass;

	}
	switch (thrd) 
	{
		case THRD_ROUTERLOOP:
			// Starting the HBSD routing loop
			startRouterLoop();
			break;
		default:
			log->error(string("Attempt to start unknown thread: ") + Util::to_string(thrd));
			break;
	}
}

void HBSD::InitStaticMembers()
{
	xmlSchemaOpt = false;
	logLevelOpt = false;
	hbsdVersion.assign("0.0.1");
	defaultLoggingClass.assign("Console_Logging");
	multicastPort = 8001;
	// Configurable values
	multicastGroup.assign("224.0.0.2");
	handlerHBSD = NULL;
	saxHandler = NULL;
	saxReader = NULL;
	LOOPBACKADR.assign("127.0.0.1");
	requester = NULL;
	log = NULL;
	routerConf = NULL;
	routerEndpoint.append("ext.rtr/HBSD");
	loggingClass = defaultLoggingClass;
	staticInitialized = true;

}

HBSD::~HBSD()
{
	// Stopping the PeerListener Thread

	if(handlerHBSD != NULL)
		delete handlerHBSD;
	if(saxHandler != NULL)
		delete saxHandler;
	if(routerConf != NULL)
		delete routerConf;
	if(requester != NULL)
		delete requester;
}

// Parsing commandline options
void HBSD::parseOpts(list<string>& args) 
{
	list<string>::iterator iter = args.begin();
	while (iter != args.end()) 
	{
		string arg = *iter;
		try 
		{
			if (arg.compare("-config_file") == 0)
			{
				iter++;
				confFile = *iter;
			} else if (arg.compare("-xml") == 0) 
			{
				iter++;
				xmlSchema = *iter;
				xmlSchemaOpt = true;
			} else if (arg.compare("-log_file") == 0) 
			{
				iter++;
				logConfiguration = *iter;
			} else if (arg.compare("-log_level") == 0) 
			{
				iter++;
				logLevel = atoi((*(iter)).c_str());
				logLevelOpt = true;
			} else if (arg.compare("-version") == 0) 
			{
				versionExit();
			} else if (arg.compare("-help") == 0) 
			{
				helpExit();
			} else 
			{
				cerr<<"Unknown option: "<< arg.c_str()<<endl;
				exit(1);
			}
		
		} 
		catch (exception &e) 
		{
			cerr<< "Missing parameter after option: "<< arg.c_str();
			exit(1);
		}
		iter++;
	}
}

// Printing the help menu
void HBSD::helpExit() 
{
	fprintf(stdout,"HBSD [-config_file] [-xml] [-log_file] [-level]\n");
	fprintf(stdout,"   -config_file   Specifies the HBSD configuration file to be used.\n");
	fprintf(stdout,"   -xml           Router XML schema file. Default is /etc/router.xsd.\n");
	fprintf(stdout,"   -log_file      Logging configuration file.\n");
	fprintf(stdout,"   -log_level     Logging level (0-6).\n");
	fprintf(stdout,"   -version       Print the HBSD version number and exit.\n");
	fprintf(stdout,"   -help          Print this help text and exit.\n"); 

	exit(0);
}


// Printing the HBSD version and exit
void HBSD::versionExit() 
{
	cout<<"HBSD Version: "<<hbsdVersion<<endl;
	exit(0);
}


//Loading the config file
void HBSD::loadConfig() 
{
	if (!confFile.empty()) 
	{
		routerConf = new ConfigFile(confFile);
		if (routerConf->parse())
		{
			loggingClass = routerConf->getstring(string("loggingClass"), defaultLoggingClass);
			//routerEndpoint = routerConf->getstring(string("routerEndpoint"), routerEndpoint);
			multicastGroup = routerConf->getstring(string("multicastGroup"), multicastGroup);
			multicastPort = routerConf->getInt(string("multicastPort"), multicastPort);
			if (!xmlSchemaOpt)
			{
				xmlSchema = routerConf->getstring(string("xmlSchema"), xmlSchema);
			}
			if (!logConfiguration.empty()) 
			{
				logConfiguration = routerConf->getstring(string("logConfiguration"), string(""));
			}
			if (!logLevelOpt) 
			{
				if (routerConf->exists(string("logLevel"))) 
				{
					logLevel = routerConf->getInt(string("logLevel"), Logging::WARN);
					logLevelOpt = true;
				}
			}
		} else 
		{
			cerr<<"Error in routerConf->parse(), "<<__FILE__<<" : "<<__LINE__<<endl;
			exit(1);
		}
	} else 
	{
		// "Empty" configuration file.
		routerConf = new ConfigFile();
	}
}

// Setting up Sax
void HBSD::setupSAX()
{
	try
	{
		// Set up the SAX parser/reader and associate it with our handler.
		handlerHBSD = new HBSD_Routing();
		saxHandler = new HBSD_SAX(handlerHBSD);

		XMLPlatformUtils::Initialize();

		saxReader = XMLReaderFactory::createXMLReader();
		// Pointing to the sax error ahndler
		saxReader->setErrorHandler(saxHandler);
		// Pointing to the sax content handler
		saxReader->setContentHandler(saxHandler);
	} 

	catch(const XMLException & toCatch)
	{
		char * message = XMLString::transcode(toCatch.getMessage());
		if(HBSD::log->enabled(Logging::ERROR))
			HBSD::log->error(string("Error During Initialization :") + string(message));
		XMLString::release(&message);
		exit(1);
	}

	catch (exception &e) 
	{
		if(HBSD::log->enabled(Logging::ERROR))
			HBSD::log->error(string("Error During Initialization HBSD::setupSAX :") + string(e.what()));
		exit(1);
	}

}
	
// Setting up the Multicast socket
void HBSD::setupMulticast() 
{
	try 
	{
		dtndSocket = socket(AF_INET, SOCK_DGRAM, 0);
		if(dtndSocket <0)
		{		
			perror ("The following error occurred");
			exit(1);
		}
			
		// Init the socket
		memset(&dtndSocketAddr, 0, sizeof(dtndSocketAddr));
		dtndSocketAddr.sin_family = AF_INET;
		dtndSocketAddr.sin_port = htons(multicastPort);
		dtndSocketAddr.sin_addr.s_addr = inet_addr(multicastGroup.c_str());
	
		
		struct ip_mreq imr;
		imr.imr_multiaddr.s_addr = inet_addr(multicastGroup.c_str());
		imr.imr_interface.s_addr = inet_addr(LOOPBACKADR.c_str());

		// Join the Multicast Group
		if (setsockopt(dtndSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &imr, sizeof(struct ip_mreq)) < 0) 
		{
        		perror ("The following error occurred");
        		exit(1);
    	}

		// Reusing the same port as the DTN2 Daemon
		int reuse = 1;
		if(setsockopt(dtndSocket, SOL_SOCKET, SO_REUSEADDR, (int *) &reuse, sizeof(reuse)) < 0)
		{
        		perror ("The following error occurred");
        		exit(1);
		}

		// Disaable the Multicast LOOP
		unsigned char loop = 0;
		if(setsockopt(dtndSocket, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0)
		{
			perror ("The following error occurred");
			exit(1);
		}
		
		//Bind the Multicast socket for reception
		if (bind(dtndSocket, (sockaddr*)&dtndSocketAddr, sizeof(dtndSocketAddr)) < 0) 
		{
         		perror ("The following error occurred");
         		exit(1);
     	}

		requester = new Requester();
		requester->init(multicastGroup, multicastPort);
	} 
	catch (exception &e) 
	{
		if(HBSD::log->enabled(Logging::ERROR))
			HBSD::log->error(string("Error during HBSD::setupMulticast :") + string(e.what()));
		exit(1);
	}
}

// Starting the HBSD routing loop
void HBSD::startRouterLoop()
{
	log->info(string("HBSD external DTN router version: ") + hbsdVersion);

	log->info(string("HBSD router started and listening on port ") + Util::to_string(multicastPort));

	// Will hold the message which will be received from the DTN2 daemon
	char msg[MAX_DTNDXML_SZ];

	//Initialize the HBSD Router
	handlerHBSD->initialized();

	while (true) 
	{
		try 
		{
			
			// Wait for a packet on the loopback multicast socket.
			int sizeAddr = sizeof(dtndSocketAddr);	
			memset(msg,'\0',MAX_DTNDXML_SZ);
			int cnt = recvfrom(dtndSocket, msg, sizeof(msg), 0, (sockaddr*)&dtndSocketAddr, (socklen_t*)&sizeAddr);
			assert(cnt > 0);
			try 
			{
				string s;
				s.assign(msg);
				/*if(HBSD::log->enabled(Logging::INFO))
				{
					HBSD::log->info(string("Received xml message from dtnd: ")+s);
				}*/

				//Converting the string message to andMemBufInputSource to be parsed
				MemBufInputSource inputsource((const XMLByte*)s.c_str(), s.length(), "msg", false);
				//Parse the received xml message and fire the corresponding events
				saxReader->parse(inputsource);
			}
			catch (SAXException &e)
			{
				if (log->enabled(Logging::ERROR)) 
				{
					log->error(string("Error parsing XML packet: ") + string(XMLString::transcode(e.getMessage())));
				}
				continue;
			} 

			catch (exception &e) 
			{
				if (log->enabled(Logging::ERROR)) 
				{
					log->error(string("Unanticipated XML parsing error: ") + string(e.what()));
				}
				continue;
			}
			
		} 

		catch (exception &e) 
		{
			log->fatal(string("Unanticipated exception in multicast receive loop"));
			break;
		}
	}
}
