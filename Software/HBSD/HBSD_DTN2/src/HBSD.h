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


#ifndef HBSD_H
#define HBSD_H

#include <string>
#include <list>
#include <sys/socket.h>
#include <sys/types.h>
#include "Logging.h"
#include "HBSD_Routing.h"
#include <xercesc/sax2/SAX2XMLReader.hpp>
#include "Requester.h"

using namespace xercesc;

class ConfigFile;

class HBSD_SAX;
	
class HBSD
{
public:
	/**
	 * Constructor initiates the appropriate thread. 
	 */
	HBSD(int thrd);
	static void InitStaticMembers();
	~HBSD();
	
	/**
	 * Sets the local EID, used by all, as well as the router endpoint name.
	 * 
	 * @param eid The local EID.
	 */
	static void setLocalEID(std::string eid) 
	{
		// Node local  EID
		localEID = eid;
		// HBSD registration as an application
		hbsdRegistration = eid + std::string("/") + routerEndpoint;
	}

	static std::string routerEndpoint;
	// The logging class which will be used
	static std::string loggingClass;
	
	// "Global" values
	static Logging *log;
	static ConfigFile * routerConf;
	// The DTN node local EID
	static std::string localEID;
	static Requester* requester;
	static std::string hbsdRegistration;

	static int dtndSocket;
	static struct sockaddr_in dtndSocketAddr;
	// Max XML message size received from DTN2
	const static int MAX_DTNDXML_SZ = 10240;
	
	/**
	 * Main thread. Loop waiting for messages from dtnd.
	 */
	void startRouterLoop();

	/**
	 * We receive XML messages from the DTN daemon on a local multicast
	 * socket. Here we create that socket. We also create the socket to
	 * send messages to the DTN daemon via the Requester class.
	 */
	static void setupMulticast();
	
	/**
	 * Initializes the SAX XML handler.
	 */
	static void setupSAX();

	/**
	 * If a configuration file is specified, read the values.
	 * Command line arguments take precedence over configuration
	 * file values, i.e. if passed on the command line don't look
	 * for it in the configuration file.
	 */
	static void loadConfig();
	
	/**
	 * Parses the command line options. These should be kept to a minimum;
	 * use the configuration file.
	 * 
	 * @param args Arguments passed in at runtime.
	 */
	static void parseOpts(std::list<std::string>& args);
	
	/**
	 * Print the version and exit the program.
	 */
	static void versionExit();
	
	/**
	 * Prints help text and then terminates the program.
	 */
	static void helpExit();
	

	// Command line options
	static std::string confFile;
	static bool xmlSchemaOpt;
	static bool logLevelOpt;
	// The current version of the HBSd external router
	static std::string hbsdVersion;
	// The default HBSD logging class
	static std::string defaultLoggingClass;
	
	// Configurable values
	static std::string multicastGroup;
	static int multicastPort;
	static std::string xmlSchema;
	static std::string logConfiguration;
	static int logLevel;
	static HBSD_Routing *handlerHBSD;
	static HBSD_SAX* saxHandler;
	static SAX2XMLReader* saxReader;
	static std::string LOOPBACKADR;
	const static int THRD_ROUTERLOOP = 0;
	static bool staticInitialized;
};


#endif
