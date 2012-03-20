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


#include "Nodes.h"
#include "Handlers.h"
#include "Node.h"
#include "HBSD.h"
#include "ConfigFile.h"
#include <iostream>
using namespace std;

Nodes::Nodes(Handlers *router) 
{
	assert(router != NULL);
	sem_init(&nodesLock, 0, 1);
	this->router = router;
	
}

Nodes::~Nodes()
{
	sem_destroy(&nodesLock);
	// Clearing the hash of Nodes
	while(!nodesList.empty())
	{
		map <string, Node *>::iterator iter = nodesList.begin();
		delete iter->second;
		nodesList.erase(iter);
	}
	// Destroying the semaphore
}


Node *Nodes::conditionalAdd(string eid) 
{
	assert(!eid.empty());

	// Don't create a node for ourself.
	if(eid.compare(HBSD::localEID) == 0)
	{
		if(HBSD::log->enabled(Logging::INFO))
			HBSD::log->info(string("It is me ! Just don't create yourself"));
		return NULL;
	}

	// First check if it exists using the read lock. We may want to revisit this if we typically call this without the EID already existing, but this is probably the correct approach.

	Node *node = NULL;

	sem_wait(&nodesLock);

	node = nodesList[eid];

	if (node != NULL) 
	{
		// Found it.
		if(HBSD::log->enabled(Logging::INFO))
			HBSD::log->info(string("we already know about this Node:")+eid);

		sem_post(&nodesLock);
		return node;
	}

	bool callPolicy = false;


	// Go ahead and create the Node object without the lock and then acquire the write lock just to add it to the table.
	if(HBSD::log->enabled(Logging::INFO))
		HBSD::log->info(string("Creating the node: ")+eid);
	node = new Node(this, eid);
	nodesList[eid] = node;
	callPolicy = true;


	if (callPolicy) 
	{
		// Notify the PolicyManager
		router->policyMgr->nodeCreated(node);
	}

	sem_post(&nodesLock);

	return node;
}

Node *Nodes::findNode(string eid) 
{
	Node *node = NULL;
	sem_wait(&nodesLock);
		node = nodesList[eid];
	sem_post(&nodesLock);
	return node;
}

void Nodes::allNodes(list<Node *> &listNodes) 
{
	// Getting the list of all the available nodes
	sem_wait(&nodesLock);
	for(map <string, Node *>::iterator iter = nodesList.begin();iter != nodesList.end(); iter++)
	{
		listNodes.push_back(iter->second);
	}
	sem_post(&nodesLock);
}

void Nodes::removeNode(string eid)
{
	sem_wait(&nodesLock);
	map <string, Node *>::iterator iter = nodesList.find(eid);
	if(iter != nodesList.end())
	{
		if(iter->second != NULL)
			delete iter->second;
		nodesList.erase(iter);
	}
	sem_post(&nodesLock);
}

string Nodes::getListOfAvailableNodes()
{
	string tmp;

	sem_wait(&nodesLock);

	if(nodesList.empty())
		return NULL;
	int i = 0;
	for(map <string, Node *>::iterator iter = nodesList.begin(); iter != nodesList.end(); iter++)
	{
		if(i > 0)
			tmp.append("#");
		i++;
		tmp.append(iter->first);
		cout<<"EIDDDDDDD: "<<iter->first<<endl;
	}

	sem_post(&nodesLock);

	return tmp;
}
