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


#include "Node.h"
#include "HBSD.h"


using namespace std;

Node::Node(Nodes *nodeManager, string eid) 
{
	activeLink = NULL;
	nodeManager = nodeManager;
	eid = eid;
	lastSessionAt = Util::getCurrentTimeSeconds();
}

Node::~Node()
{
	activeLink = NULL;
	nodeManager = NULL;
}

bool Node::setLink(Link *link) 
{
	assert(link != NULL);
	bool set = true;
	if (activeLink == NULL) 
	{
		activeLink = link;
		if (HBSD::log->enabled(Logging::DEBUG)) 
		{
			HBSD::log->debug(string("Associating: ") + link->id + string(" and ") + this->eid);
		}
	}
	else
	{
		if (HBSD::log->enabled(Logging::DEBUG)) 
		{
			HBSD::log->debug(string("Not associating to ") + eid + string(": ") + link->id + string(" already associated to ") + link->remoteEID);
		}

		if (activeLink != link) 
		{
			set = false;
		}
	}

	return set;
}


void Node::updateLastSessionTime()
{
	lastSessionAt = Util::getCurrentTimeSeconds();
}
