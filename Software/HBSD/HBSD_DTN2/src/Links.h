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


#ifndef LINKS_H
#define LINKS_H

#include <string>
#include <map>
#include <semaphore.h>


#define LINKS_LOCK_DEBUG false

class Handlers;
class Link;
class XMLTree;

class Links 
{

public:

	/**
	 * Constructor: Get configuration values and create an empty table.
	 */
	Links(Handlers* router);
	~Links();
	
	/**
	 * Creates and initializes a link, adding it to the list. We only
	 * create links from the main thread so we don't need to hold the
	 * lock from start to finish, just around the actual updates to the
	 * hash table.
	 * 
	 * @param evt Root of link created event.
	 * @return Created Link (NULL if failed).
	 */
	Link* create(XMLTree* evt);
	
	
	/**
	 * Deletes a link. The link id key is not removed, but the object
	 * becomes NULL.
	 * 
	 * @param evt Root of link deleted event.
	 */
	void deleteEvt(XMLTree* evt);
	
	/**
	 * Returns the number of links in the linkArray. This includes
	 * NULL links.
	 * 
	 * @return The number of links.
	 */
	int numLinks();
	
	/**
	 * Returns a Link given its name. Note that NULL can be returned
	 * even if the key exists.
	 * 
	 * @param id Name (id) of the link.
	 * @return Link object or NULL.
	 */
	Link* getById(std::string id);

	Handlers* router;
	
	void showAvailableLinks();

	void InitiateESWithAvailableLinks();

	void getLinksLock(std::string p);
	void leaveLinksLock(std::string p);
private: 

	/**
	 * Add a link to the list. We'll replace the current entry
	 * if it already exists. An add of an existing key should
	 * only be done if the Link value is NULL.
	 * 
	 * @param link Link to be added.
	 */
	void add(Link* link);

	
	/**
	 * Finds and removes a Link object. This method is called by
	 * delete() and does all of the work that should be synchronized
	 * within this class.
	 * 
	 * @param evt The XMLTree object representing the delete event.
	 * @return The link object
	 */
	Link* remove(XMLTree* evt);

	std::map<std::string,Link*> linkMap;
	sem_t linksLock;
};


#endif
