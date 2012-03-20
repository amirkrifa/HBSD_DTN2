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


#include "Handlers.h"
#include "XMLTree.h"
#include "Bundles.h"
#include "Nodes.h"
#include "PeerListener.h"
#include "Policy.h"
#include "MeDeHaInterface.h"
#include "HBSD.h"
#include <string>
#include <iostream>
#include "ConfigFile.h"
#include <cassert>
using namespace std;


Handlers::Handlers()
{
	// Create instances of the classes that manage links, bundles, nodes
	// and routes.
	links = new Links(this);
	assert(links != NULL);
	bundles = new Bundles(this);
	assert(bundles != NULL);
	nodes = new Nodes(this);
	assert(nodes != NULL);
	peerListener = new PeerListener(this);
	assert(peerListener != NULL);
}


void Handlers::initialized()
{
}

void Handlers::handler_bpa_root(XMLTree *bpa)
{
}

void Handlers::handler_bundle_received_event(XMLTree *event, XMLTree *bpa)
{
}

void Handlers::handler_data_transmitted_event(XMLTree *event, XMLTree *bpa) 
{
}

void Handlers::handler_bundle_delivered_event(XMLTree *event, XMLTree *bpa)
{
}

void Handlers::handler_bundle_delivery_event(XMLTree *event, XMLTree *bpa)
{
}

void Handlers::handler_bundle_send_cancelled_event(XMLTree *event, XMLTree *bpa)
{
}

void Handlers::handler_bundle_expired_event(XMLTree *event, XMLTree *bpa)
{
}

void Handlers::handler_bundle_injected_event(XMLTree *event, XMLTree *bpa)
{
}

void Handlers::handler_link_opened_event(XMLTree *event, XMLTree *bpa)
{
}

void Handlers::handler_link_closed_event(XMLTree *event, XMLTree *bpa)
{}

void Handlers::handler_link_created_event(XMLTree *event, XMLTree *bpa)
{}

void Handlers::handler_link_deleted_event(XMLTree *event, XMLTree *bpa) 
{}

void Handlers::handler_link_available_event(XMLTree *event, XMLTree *bpa)
{}

void Handlers::handler_link_unavailable_event(XMLTree *event, XMLTree *bpa)
{}

