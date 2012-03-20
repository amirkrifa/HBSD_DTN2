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


#include "Requester.h"
#include <exception>
#include <stdio.h>
#include <stdlib.h>
#include "Link.h"
#include "Bundle.h"
#include "Util.h"
#include "HBSD.h"
#include "GBOF.h"
#include "ConfigFile.h"
#include <iostream>
#include <fstream>
using namespace std;

// Good tutorial for dealing with Multicast sockets
// http://www.tenouk.com/Module41c.html

string Requester::xmlBanner;
string Requester::BPA_END;
	
Requester::Requester()
{
	defaultDestPort = 0;
	idBase = Util::to_string(this) + string("-") + Util::to_string(time(NULL)/1000) + string("-");
	injectIdSeq = 0;
	xmlBanner.assign("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	BPA_END.assign("\n</bpa>");
}

Requester::~Requester()
{

}

void Requester::init(string group, int port)  
{
	assert(!group.empty());
	assert(port > 0);

	try 
	{
		defaultDestPort = port;
		
		// Init multicast socket

		socketR = socket(AF_INET, SOCK_DGRAM, 0);
		
		if(socketR < 0)
		{
		      perror ("socket: The following error occurred");
		      exit(1);
    	}

		// Initialize the group sockaddr with a group adress and a
		// port number
		memset((char *) &defaultDestAddr, 0, sizeof(defaultDestAddr));

		defaultDestAddr.sin_family = AF_INET;
		defaultDestAddr.sin_addr.s_addr = inet_addr(group.c_str());
		defaultDestAddr.sin_port = htons(defaultDestPort);
		
		// Disable loopback so we do not receive our own datagrams.

		char loopch = 0;
		if(setsockopt(socketR, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loopch, sizeof(loopch)) < 0)
		{
			perror("setsockopt: Setting IP_MULTICAST_LOOP error");
			close(socketR);
			exit(1);
		}

		// Set local interface for outbound multicast datagrams.
		// The IP address specified must be associated with a local,
		// multicast capable interface.

		struct in_addr localInterface;
		localInterface.s_addr = inet_addr("127.0.0.1");
		if(setsockopt(socketR, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInterface, sizeof(localInterface)) < 0)
		{
			perror("setsockopt: Setting local interface error");
			exit(1);
		}
	}

	catch (exception &e) 
	{
		if(HBSD::log->enabled(Logging::ERROR))
			HBSD::log->error(string("Requester::init unanticipated exception ocuurred: ") + string(e.what()));
		exit(1);
	}
}
	

string Requester::getMsgStart() 
{
	if (HBSD::localEID.empty()) 	
	{
		// Local EID not yet known: don't cache value yet.
		return xmlBanner + string("\n<bpa eid=\"null\">\n  ");
	}

	if (xmlWithBpaStart.length() == 0) 
	{
		constructStarts();
	}

	return xmlWithBpaStart;
}


bool Requester::xmlEncapsulateAndSend(string data)
{
	assert(!data.empty());
	try 
	{
		string strMsg = getMsgStart() + data + BPA_END;
		if(sendto(socketR ,strMsg.c_str(),strMsg.length(), 0,(sockaddr *)&defaultDestAddr, sizeof(defaultDestAddr))<0)
		{
		    if(HBSD::log->enabled(Logging::ERROR))
		    	perror("Requester::xmlEncapsulateAndSend sendto: ");
		    exit(1);
    	}
		return true;
	}
	
	catch (exception &e) 
	{
			if (HBSD::log->enabled(Logging::ERROR)) 
			{
				HBSD::log->error(string("Failed to send request to dtnd: ") + string(e.what()));
			}
			return false;
	}
}

bool Requester::requestSendBundle(Bundle* bundle, string linkId, int action) 
{
	assert(bundle != NULL);
	assert(!linkId.empty());
	string req = string("<send_bundle_request local_id=\"") + bundle->localId + string("\" link_id=\"") + linkId + string("\" fwd_action=\"");
	if (action == FWD_ACTION_FORWARD) 
	{
		req += string("forward\">");
	} else 
	{
		req += string("copy\">");
	}
	req += GBOF::xmlFromBundle(bundle) + string("</send_bundle_request>");
	return sendAsXML(req);
}

string Requester::requestInjectBundle(string source, string dest,string linkId, string payloadFile) 
{
	assert(!source.empty());
	assert(!dest.empty());
	assert(!linkId.empty());
	assert(!payloadFile.empty());

	// Generate an id that will uniquely identify the injected bundle.
	string requestId =  idBase + Util::to_string(getAndIncrement());
	
	// Define the bundle to be forwarded with a hardcoded expiration.
	string req;
	req = "<inject_bundle_request request_id=\"" + requestId + "\" link_id=\"" + linkId +
			"\" fwd_action=\"forward\" expiration=\"30\" payload_file=\"" +
			payloadFile + "\">"
			+ "\n"+ "<source uri=\"" + source + "\"/>"
			+ "\n" + "<dest uri=\"" + dest + "\"/>" +
			"\n" + "</inject_bundle_request>";

	if (!sendAsXML(req))
	{
		if(HBSD::log->enabled(Logging::ERROR))
			HBSD::log->error(string("Error occurred when asking dtnd to inject a bundle"));
		return NULL;
	}

	return requestId;
}

string Requester::requestInjectMeDeHaBundle(string source, string dest, string payloadFile)
{
	assert(!source.empty());
	assert(!dest.empty());
	assert(!payloadFile.empty());

	// Generate an id that will uniquely identify the injected bundle.
	string requestId =  idBase + Util::to_string(getAndIncrement());
	
	// Define the bundle to be forwarded with a hardcoded expiration.
	string req;
	req = "<inject_bundle_request request_id=\"" + requestId + "\" link_id=\""  +
			"\" fwd_action=\"copy\" expiration=\"300\" payload_file=\"" +
			payloadFile + "\">"
			+ "\n"+ "<source uri=\"" + source + "\"/>"
			+ "\n" + "<dest uri=\"" + dest + "\"/>" +
			"\n" + "</inject_bundle_request>";

	if (!sendAsXML(req))
	{
		if(HBSD::log->enabled(Logging::ERROR))
			HBSD::log->error(string("Error occurred when asking dtnd to inject a bundle"));
		return NULL;
	}

	return requestId;
}

bool Requester::requestDeleteBundle(Bundle* bundle) 
{
	assert(bundle != NULL);
	string req = string("<delete_bundle_request local_id=\"") + bundle->localId + string("\">") + GBOF::xmlFromBundle(bundle) + string("</delete_bundle_request>");
	return sendAsXML(req);
}
	
bool Requester::requestCancelBundle(Bundle* bundle, Link* link) 
{
	assert(bundle != NULL);
	assert(link != NULL);
	string req = string("<cancel_bundle_request local_id=\"") + bundle->localId + string("\" link_id=\"") + link->id + string("\">") +  GBOF::xmlFromBundle(bundle) + string("</cancel_bundle_request>");
	return sendAsXML(req);
}

void Requester::constructStarts()
{
	if (bpaStart.empty()) 
	{
		bpaStart = string("<bpa eid=\"") + HBSD::localEID + string("\">");
	}

	if (xmlWithBpaStart.empty()) 
	{
		if (HBSD::log->enabled(Logging::TRACE)) 
		{
			// Add some whitespace if logging simply to make it easier
			// to read.
			xmlWithBpaStart = xmlBanner + string("\n") + bpaStart + string("\n  ");
		} else 
		{
			xmlWithBpaStart = xmlBanner + bpaStart;
		}
	}
}
