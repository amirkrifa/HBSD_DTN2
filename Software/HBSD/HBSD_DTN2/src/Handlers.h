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


#ifndef HANDLERS_H
#define HANDLERS_H

#include "Links.h"
#include "Nodes.h"
#include "Policy.h"
#include "StatisticsManager.h"

#define ENABLE_MEDEHA_INTERFACE true

class XMLTree;
class Bundles;
class PeerListener;
class MeDeHaInterface;

class Handlers
{
public:
	Handlers();
	
	// Called after HBSD is fully initialized, before starting its main loop.
	virtual void initialized();
	// The class that extends this abstract class implements the handlers
	// that it supports. Unsupported events silently fall into these no-op
	// handlers.
	virtual void handler_bpa_root(XMLTree *bpa) ;
	virtual void handler_bundle_received_event(XMLTree *event, XMLTree *bpa) ;
	virtual void handler_data_transmitted_event(XMLTree *event, XMLTree *bpa) ;
	virtual void handler_bundle_delivered_event(XMLTree *event, XMLTree *bpa) ;
	virtual void handler_bundle_delivery_event(XMLTree *event, XMLTree *bpa) ;
	virtual void handler_bundle_send_cancelled_event(XMLTree *event, XMLTree *bpa) ;
	virtual void handler_bundle_expired_event(XMLTree *event, XMLTree *bpa) ;
	virtual void handler_bundle_injected_event(XMLTree *event, XMLTree *bpa) ;
	virtual void handler_link_opened_event(XMLTree *event, XMLTree *bpa) ;
	virtual void handler_link_closed_event(XMLTree *event, XMLTree *bpa) ;
	virtual void handler_link_created_event(XMLTree *event, XMLTree *bpa)  ;
	virtual void handler_link_deleted_event(XMLTree *event, XMLTree *bpa)  ;
	virtual void handler_link_available_event(XMLTree *event, XMLTree *bpa) ;
	virtual void handler_link_unavailable_event(XMLTree *event, XMLTree *bpa) ;
	// These are the principal objects that make up the HBSD router.
	Policy * policyMgr;
	Nodes *nodes;
	Links *links;
	StatisticsManager * statisticsManager;
	MeDeHaInterface * medehaInterface;
	Bundles *bundles;

protected:
	PeerListener *peerListener;
	bool enableMeDeHaInterface;
};

#endif
