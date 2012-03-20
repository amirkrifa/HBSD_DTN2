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


#ifndef NODE_H
#define NODE_H

#include <string>
#include "Nodes.h"
#include "Handlers.h"
#include "Link.h"
#include <time.h>

class Node
{

public:
	
	/**
	 * Constructor. Should be called only by the Nodes class.
	 * 
	 * @param nodeManager Node class that manages this node.
	 * @param eid EID this node represents.
	 */
	Node(Nodes *nodeManager, std::string eid);

	~Node();
	/**
	 * Sets the active link if it's not currently set.
	 * 
	 * @return True if set, false if not set because it's already set.
	 */
	bool setLink(Link *link);
	
	/**
	 * Clear the active link.
	 */
	void clearLink() 
	{
		activeLink = NULL;
	}
	
	/**
	 * Indicates whether the node is associated with a specific link.
	 * 
	 * @param link Link in question.
	 * @return True if associated with the link, else false.
	 */
	bool associated(Link *link) 
	{
		return (activeLink == link);
	}
	
	/**
	 * Indicates whether the node is associated with a link.
	 * 
	 * @return True if associated with some link, else false.
	 */
	bool associated() 
	{
		return (activeLink != NULL);
	}
	
	Link * getAssociatedLink()
	{
		return activeLink;
	}

	std::string eid;

	time_t getLastSessionTime()
	{
		return lastSessionAt;
	}

	void updateLastSessionTime();

private:
	Link * activeLink;
	Nodes * nodeManager;
	
	time_t lastSessionAt;

};



#endif
