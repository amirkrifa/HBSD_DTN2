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


#ifndef LINK_H
#define LINK_H

#include <string>
#include <pthread.h>
#include <queue>
#include <semaphore.h>
#include <pthread.h>

class Node;
class XMLTree;
class Link;
class Links;



class Link
{
public:	
	// Link types
	const static int TYPE_ALWAYSON      = 0;
	const static int TYPE_ONDEMAND      = 1;
	const static int TYPE_SCHEDULED     = 2;
	const static int TYPE_OPPORTUNISTIC = 3;
	const static int TYPE_OTHER         = 4;
	
	// Link states. Do not assume the link goes from one state to the next.
	// But the ordinal values below are important.
	const static int STATE_NONEXTANT    = 0;
	const static int STATE_UNAVAILABLE  = 1;
	const static int STATE_AVAILABLE    = 2;
	const static int STATE_CLOSED       = 3;
	const static int STATE_OPENING      = 4;
	const static int STATE_OPEN         = 5;
	const static int STATE_BUSY         = 6;
	
	
	Link(Links *links);
	
	~Link();
	
	/**
	 * Called when the link becomes available. Start the thread
	 * if not running. If the link had been busy then this probably
	 * means that it is open as opposed to able to be opened. If our
	 * current state is open then we just ignore the request because
	 * it probably means the link had been busy and no longer is busy
	 * (and for some reason we didn't realize that it was busy). Got
	 * that?
	 *
	 * element Root XMLTree element for the event.
	 */
	void available(XMLTree *element);
	
	/**
	 * Called when the link becomes unavailable.
	 * 
	 * @param element Root XMLTree element for the event.
	 */
	void unavailable(XMLTree *element) 
	{
		openRequested = false;
		stateChange(STATE_UNAVAILABLE);
	}
	

	/**
	 * Called when the link is opened.
	 * @param element Root XMLTree element for the event.
	 */
	void opened(XMLTree *element);
	
	/**
	 * Called when the link is closed.
	 * 
	 * @param element Root XMLTree element for the event.
	 */
	void closed(XMLTree *element);

	
	/**
	 * Requests that the link be opened. This method is typically used
	 * by the policy manager to open a link.  
	 */
	void requestOpen();
	
	
	/**
	 * Returns the defined remote address for the link.
	 * 
	 * @return Remote address, as obtained from the <clinfo> element.
	 */

	std::string getRemoteAddr() 
	{
		return remoteAddr;
	}
	
	/**
	 * Initialize a Link given an event. The creation and initialization of
	 * a link is normally done via the create() method of the Links class.
	 * 
	 * @param evtLinkCreated link_created_event as the root of a tree.
	 * return True if initialized ok, else false.
	 */
	bool init(XMLTree *evtLinkCreated);

	/**
	 * Called when the link is deleted. This should not be called
	 * directly, but instead be called by Links.delete(). When called, the Links class should have no more
	 * knowledge of this instance of the link.
	 * 
	 * @param element Root XMLTree element for the event.
	 */
	void deleted(XMLTree *element) 
	{
		stateChange(STATE_NONEXTANT);
	}

	bool associate(Node *node);

	std::string id;
	std::string remoteEID;
	
	/*
	 * Returns the link current state
	 * @return return the link current state for debuggin
	 */
	std::string getCurrentState();

	int state;

protected:
	
	int type;
	
private:

	/**
	 * Gets pertinent information from the link_attr element.
	 * 
	 * @param element Element as represented by the XMLTree class.
	 * @return True if required attributes exist, else false.
	 */
	bool parseLinkAttr(XMLTree* element);

	void stateChange(int newState);
	/**
	 * Converts a string type value to an integer value.
	 * 
	 * @param str string value.
	 * @return Integer value.
	 */
	int whatType(std::string str);
	
	/**
	 * Converts a string state value to an integer value.
	 * 
	 * @param str string value.
	 * @return Integer value.
	 */
	int whatState(std::string str);
	

	Links *linkManager;
	
	// Information about the link that is written outside of the Link
	// thread but read/copied by the thread.
	Node* currentNode;
	Node* attachNode;
	
	


	std::string remoteAddr;

	bool openRequested;
	
	
	const static int ACTION_NONE = 0;
	const static int ACTION_RUN  = 1;
	const static int ACTION_STOP = 2;


	/**
	 * Search for a "contact_attr" element as a direct child element. If
	 * found, save it. Also, look and see if it has a "link_attr" element
	 * as a child.
	 * 
	 * @param event Root event where the contact_attr is expected to be a
	 *    direct child.
	 */
	void findAndSaveAttributesContact(XMLTree *event);
		
	/**
	 * Search for a "link_attr" element as a direct child element. If
	 * found, save it. Also, look and see if it has a "clinfo" element
	 * or "remote_eid" as a child.
	 * 
	 * @param event Root event where the link_attr is expected to be a
	 *    direct child.
	 */
	void findAndSaveAttributesLink(XMLTree *event);
	
	/**
	 * Search for a "remote_eid" element as a direct child element. If
	 * found, save the "uri" attribute.
	 * 
	 * @param event Root event where the remote_eid is expected to be a
	 *    direct child.
	 */
	void findAndSaveAttributesRemoteEID(XMLTree* event);
		

};
#endif
