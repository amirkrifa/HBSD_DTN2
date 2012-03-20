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


#ifndef NODES_H
#define NODES_H

#include <string>
#include <list>
#include <semaphore.h>
#include <map>



class Node;
class Handlers;


class Nodes 
{

public:
	
	/**
	 * Constructor. Initialize the table of known nodes.
	 */
	Nodes(Handlers *router);
	~Nodes();


	/**
	 * Add a node (EID), but only if it doesn't already exist in our tables.
	 * 
	 * @param eid EID to add.
	 * @return The instance of the Node class for the EID, regardless of
	 *    whether it already existed or we created it.
	 */

	Node * conditionalAdd(std::string eid);
	
	/**
	 * Locates the Node object for the specified EID.
	 * 
	 * @param eid EID to locate.
	 * @return The Node for the EID, else null if it does not exist.
	 */

	Node *findNode(std::string eid);
	
	/**
	 * Determines if we have knowledge of a specific EID.
	 * 
	 * @param eid EID to verify.
	 * @return True if known, else false.
	 */
	bool knowledgeOf(std::string eid) 
	{
		return (findNode(eid) != NULL);
	}
	
	/**
	 * Queries for a list of all currently known nodes. Note that the list
	 * may become out of date as soon as we release the lock.
	 * 
	 * @return An array of all known nodes.
	 */
	void allNodes(std::list<Node *> &listNodes);

	void removeNode(std::string eid);

	Handlers *router;
	std::string getListOfAvailableNodes();
private:

	sem_t nodesLock;
	std::map <std::string, Node *> nodesList;
	
};

#endif
