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


#ifndef HBSD_ROUTING
#define HBSD_ROUTING

#include <string>
#include "Handlers.h"
#include <list>

#define DEFAULT_RUN_HBSD_OPTIMIZATION false

class XMLTree;
class Bundle;
class Link;
class HBSD;

class HBSD_Routing : public Handlers 
{
public:	
	/**
	 * Constructor: Let the abstract class create the primary objects. Get
	 * the name of the class implementing the Policy Manager and initialize
	 * it.
	 */
	HBSD_Routing();
	~HBSD_Routing();
	/**
	 * Called once HBSD is fully initialized.
	 */
	void initialized();
	
	
	/**
	 * Called when there were no elements in the XML except the <bpa>
	 * element.
	 * 
	 * @param bpa XML bpa element.
	 */
	void handler_bpa_root(XMLTree* bpa);
	
	
	
	/**
	 * Called when a bundle has been received by DTN. This can either be a
	 * locally generated bundle or one received from another node.
	 * 
	 * @param Root XML element of the event.
	 * @param The bpa element.
	 */
	void handler_bundle_received_event(XMLTree* event, XMLTree* bpa);
	
	/**
	 * Called when all or part of a bundle has been transmitted. Notify the
	 * link.
	 * 
	 * @param Root XML element of the event.
	 * @param The bpa element.
	 */
	void handler_data_transmitted_event(XMLTree* event, XMLTree* bpa);
	
	/**
	 * Called when a bundle report is received. We request a report at start-up
	 * to learn what bundles exist if we are started after the DTN daemon.
	 * 
	 * @param Root XML element of the event.
	 * @param The bpa element.
	 */
	void handler_bundle_report_event(XMLTree* event, XMLTree* bpa);
	
	/**
	 * Called when a bundle was locally delivered. Let the bundle manager
	 * know.
	 * 
	 * @param Root XML element of the event.
	 * @param The bpa element.
	 */
	void handler_bundle_delivered_event(XMLTree* event, XMLTree* bpa);
	
	/**
	 * Called when a bundle is received whose destination endpoint indicates
	 * that the bundle is for an external router (such as this one). Invoke
	 * the peer listener thread.
	 * 
	 * @param Root XML element of the event.
	 * @param The bpa element.
	 */
	void handler_bundle_delivery_event(XMLTree* event, XMLTree* bpa);
	
	/**
	 * Called when a bundle has expired. Let the bundle manager know.
	 * 
	 * @param Root XML element of the event.
	 * @param The bpa element.
	 */
	void handler_bundle_expired_event(XMLTree* event, XMLTree* bpa);

	/**
	 * Called when a bundle has been injected. This is in response to our
	 * request to inject a meta data bundle.
	 * 
	 * @param Root XML element of the event.
	 * @param The bpa element.
	 */
	void handler_bundle_injected_event(XMLTree* event, XMLTree* bpa);
	
	/**
	 * Called when a link has opened. Let the link know.
	 * 
	 * @param Root XML element of the event.
	 * @param The bpa element.
	 */
	void handler_link_opened_event(XMLTree* event, XMLTree* bpa);

	/**
	 * Called when a link has closed. Inform the link.
	 * 
	 * @param Root XML element of the event.
	 * @param The bpa element.
	 */
	void handler_link_closed_event(XMLTree* event, XMLTree* bpa);
	
	/**
	 * Called when a link has been created. Inform the link manager.
	 * 
	 * @param Root XML element of the event.
	 * @param The bpa element.
	 */
	void handler_link_created_event(XMLTree* event, XMLTree* bpa);

	/**
	 * Called when a link has been deleted. Inform the link manager.
	 * 
	 * @param Root XML element of the event.
	 * @param The bpa element.
	 */
	void handler_link_deleted_event(XMLTree* event, XMLTree* bpa);

	/**
	 * Called when a link becomes available. Inform the link.
	 * 
	 * @param Root XML element of the event.
	 * @param The bpa element.
	 */
	void handler_link_available_event(XMLTree* event, XMLTree* bpa);

	/**
	 * Called when a link becomes unavailable. Let the link know.
	 * 
	 * @param Root XML element of the event.
	 * @param The bpa element.
	 */
	void handler_link_unavailable_event(XMLTree* event, XMLTree* bpa);

	/**
	 * Schedules the list of bundles based on their utilities and
	 * sends them to the remote peer according to the decided order.
	 * @param listBundlesIDs the list of bundles to be scheduled  and sent
	 */
	void scheduleDRAndSend(std::list<std::string>& listBundlesIDs, Link * link);
	void scheduleDDAndSend(std::list<std::string>& listBundlesIDs, Link * link);
	void sendWithoutscheduling(std::list<std::string>& listBundlesIDs, Link * link);

	/**
	 * Says whether the optimization based on HBSD framework is allowed or not
	 */
	bool enableOptimization()
	{
		return enableHbsdOptimization;
	}

	/**
		 * Determines if the source of the bundle is this node.
		 *
		 * @param bundle Bundle being examined.
		 * @return True if from this node, else false.
	 */
	bool localSource(Bundle* bundle);
	bool localDest(Bundle* bundle);

	std::string getStatMessageType(std::string & filePath);
	bool isStatisticsFileEmpty(std::string & filePath);

	/**
	 * Adds a bundles destination node to the list of known nodes if we
	 * were not previously aware of the node.
	 *
	 * @param bundle The newly created bundle.
	 */
	void addDestNode(Bundle* bundle);

private:
	/**
	 * Called when we first know that the DTN daemon is up. Sets the
	 * local EID found in the <bpa> element. 
	 * 
	 * @param bpa XML bpa element.
	 */
	void firstMessage(XMLTree* bpa);



	/**
	 * Returns the value of the "link_id" attribute from the element
	 * represented by the XMLTree data structure.
	 * 
	 * @param el XML element.
	 * @return Id, or NULL if the attribute does not exist.
	 */
	std::string linkId(XMLTree* el);

	/**
	 * Returns the Link object associated with the XML element.
	 * 
	 * @param el XML element.
	 * @return Link object, of NULL if there is none for
	 *    this link id.
	 */
	Link* linkStructure(XMLTree* el);

	static std::string BPA_ATTR_EID;
	static std::string defaultPolicy;
	bool enableHbsdOptimization;
};



#endif
