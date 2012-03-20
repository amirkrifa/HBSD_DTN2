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


#ifndef REQUESTER_H
#define REQUESTER_H

#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <semaphore.h>
#include <arpa/inet.h>
#include <netinet/in.h>



class Link;
class Bundle;

class Requester 
{
public:	
	// Possible action when sending a bundle, either to forward it and delete the local
	// copy or to just generate and forward another copy.
	const static int FWD_ACTION_FORWARD = 0;
	const static int FWD_ACTION_COPY = 1;
	
	/**
	 * Constructor: Initialize values used to generate a unique id when
	 * injecting a bundle.
	 */
	Requester();

	~Requester();

	/**
	 * Creates a multicast/datagram socket that will be used to send requests
	 * to the dtnd multicast socket.
	 * 
	 * @param group Multicast group for sending requests.
	 * @param port Destination port.
	 * @throws Exception Throws exceptions caught when creating the socket.
	 */
	void init(std::string group, int port);
	

	/**
	 * Gets the commonly used start of a request:
	 *   <?xml ...><bpa eid=...>
	 *   
	 * @return string containing the start of the XML plus the start of the
	 *    bpa element.
	 */

	std::string getMsgStart();
	
	/**
	 * Encapsulate the XML element into a full XML message and send it to
	 * the supplied port.
	 * 
	 * @param data XML element.
	 * @param port Destination datagram port.
	 * @return True on success, false on failure.
	 */

	bool xmlEncapsulateAndSend(std::string data);
	
	/**
	 * Send XML element on the default port.
	 * 
	 * @param data XML element to be sent.
	 * @return True on success, false on failure.
	 */
	inline bool sendAsXML(std::string data) 
	{
		return xmlEncapsulateAndSend(data);
	}

	
	/**
	 * Send request to open a link.
	 * 
	 * @param linkId Id of the link to be opened.
	 * @return Status of the request.
	 */
	inline bool requestOpenLink(std::string linkId) 
	{
		std::string req = std::string("<open_link_request link_id=\"") + linkId + std::string("\"/>");
		return sendAsXML(req);
	}
	
	/**
	 * Send request to close a link.
	 * 
	 * @param linkId Id of the link to be closed.
	 * @return Status of the request.
	 */
	inline bool requestCloseLink(std::string linkId)
	{
		std::string req = std::string("<close_link_request link_id=\"") + linkId + std::string("\"/>");
		return sendAsXML(req);
	}
	
	/**
	 * Send request to delete a link.
	 * 
	 * @param linkId Id of the link to be delete.
	 * @return Status of the request.
	 */
	bool requestDeleteLink(std::string linkId) 
	{
		std::string req = std::string("<delete_link_request link_id=\"") + linkId + std::string("\"/>");
		return sendAsXML(req);
	}
	
	/**
	 * Generate a request to send a bundle.
	 * 
	 * @param bundle Bundle object representing the bundle to be sent.
	 * @param linkId Id of link bundle is to be sent on.
	 * @param action Forwarding action for the bundle.
	 * @return Indicates success or failure of the request.
	 */
	bool requestSendBundle(Bundle* bundle, std::string linkId, int action);
	
	/**
	 * Called to request that a bundle be injected. 
	 * 
	 * @param source Source EID.
	 * @param dest Destination EID.
	 * @param linkId Id of the link the bundle will be sent on.
	 * @param payloadFile File containing the payload data.
	 * @return Returns the ID associated with the injected bundle.
	 */
	std::string requestInjectBundle(std::string source, std::string dest,std::string linkId, std::string payloadFile);
	std::string requestInjectMeDeHaBundle(std::string source, std::string dest, std::string payloadFile);

	/**
	 * Generate a request to delete a bundle.
	 * 
	 * @param bundle Bundle object representing the bundle.
	 * @return Indicates success or failure of the request.
	 */
	bool requestDeleteBundle(Bundle* bundle);
	
	/**
	 * Generate a request to cancel a prior bundle transmission request.
	 * 
	 * @param bundle Bundle object representing the bundle.
	 * @return Indicates success or failure of the request.
	 */
	bool requestCancelBundle(Bundle* bundle, Link* link);	
	/**
	 * Sends a bundle_query.
	 * 
	 * @return Whether or not we were able to send the query.
	 */
	inline bool queryBundle()
	{
		return sendAsXML("<bundle_query/>");
	}
	
	/**
	 * Sends a link_query.
	 * 
	 * @return Whether or not we were able to send the query.
	 */
	inline bool queryLink() 
	{
		return sendAsXML("<link_query/>");
	}
	
	/**
	 * Sends a route_query.
	 * 
	 * @return Whether or not we were able to send the query.
	 */
	inline bool queryRoute() 
	{
		return sendAsXML("<route_query/>");
	}

	int getAndIncrement()
	{
		int tmp = injectIdSeq;
		injectIdSeq++;
		return tmp;
	}



private:
	/**
	 * Utility function to construct, save and re-use what is repeatedly
	 * used in XML requests.
	 */
	
	void constructStarts();

	static std::string xmlBanner;
	static std::string BPA_END;
	std::string bpaStart;
	std::string xmlWithBpaStart;

	int socketR;
	int defaultDestPort;
	struct sockaddr_in defaultDestAddr;

	int injectIdSeq;

	std::string idBase;

};



#endif
