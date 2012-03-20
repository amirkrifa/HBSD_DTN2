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
#include <list>
#include <string>
#include "Console_Logging.h"
#include <iostream>
#include <signal.h>
#include <unistd.h>

using namespace std;

HBSD* mainRouter = NULL;


// Catching the signal Ctrl + c
void exitHBSD(int sig)
{
	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("Signal Ctrl + C: Shutting down the HBSD external router."));
	if(mainRouter != NULL)
		delete mainRouter;
}

int main(int argc, const char** argv) 
{
	list<string> args;

	for(int i = 0; i<argc; i++)
	{	
		if(i == 0)
			continue;
		args.push_back(argv[i]);
	}

	HBSD::InitStaticMembers();

	// Get options.
	HBSD::parseOpts(args);
	
	// Get values from the configuration file.
	HBSD::loadConfig();
	
	// Initialize logging to use during execution. Only set
	// the logging level if specified in the command line or HBSD
	// configuration file, otherwise we are content with what the
	// logging system defaults to.
	try 
	{
		HBSD::log = (Logging*) new Console_Logging();
		if (HBSD::logConfiguration.empty()) 
		{
			HBSD::log->conf();
		} else 
		{
			HBSD::log->conf(HBSD::logConfiguration);
		}
		if (HBSD::logLevelOpt) 
		{
			HBSD::log->setLevel(HBSD::logLevel);
		} else 
		{
			HBSD::logLevel = HBSD::log->getLevel();
		}
	} 
	catch (exception &e) 
	{
		cerr<< "Unable to instantiate or initialize logging: "<< e.what()<<endl;
		exit(1);
	}
	
	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("Setting UP SAX XML Handler."));
	// Set up our SAX XML handler.
	HBSD::setupSAX();

	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("Setting UP the MUlticast Socket towards receiving messages form the DTN2 Daemon."));
	// Set up multicast socket that we'll be receiving messages
	// from the DTN daemon on.
	HBSD::setupMulticast();

	if(HBSD::log->enabled(Logging::INFO))
			HBSD::log->info(string("Starting HBSD Router main thread."));
	// We are now initialized and ready to begin routing. Start the main
	// thread.
	mainRouter = new HBSD(HBSD::THRD_ROUTERLOOP);
	signal(SIGINT, exitHBSD);
}

