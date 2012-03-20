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


#ifndef POLICY_H
#define POLICY_H


#include <string>

class Node;
class Link;
class Handlers;
class Bundle;

class Policy 
{
	
public:
	/**
	 * Expected to be called after the object is created and before any 
	 * other methods, this method allows the Policy Manager to perform
	 * initialization.
	 * 
	 * @param router The object that implements the Handlers abstract class.
	 *     Handlers is also the class that calls this init() method, and is
	 *     by default implemented by RAPID_Routing class.
	 * @return True if initialization was successful, else false. This allows
	 *     the policy manager to return a status, in contrast to the
	 *     constructor. 
	 */
	virtual bool init(Handlers* router) = 0;
	
	/**
	 * Called to indicate that a node has been created.
	 * 
	 * @param node Object representing the node.
	 */
	virtual void nodeCreated(Node* node) = 0;
	
	/**
	 * Called to determine whether a request to open a non-open link should
	 * be sent to DTN. If the node is not null, it is that Node object that
	 * is the catalyst for the inquiry.
	 * 
	 * @param link Link object representing the link in question.
	 * @param node Node object requesting that the link be opened.
	 * @return True if to open, otherwise false.
	 */

	virtual bool requestLinkOpen(Link* link, Node* node) = 0;
	
	/**
	 * Called when a link is created.
	 * 
	 * @param link Link object representing the created link.
	 */
	virtual void linkCreated(Link* link) = 0;
	
	/**
	 * Called when a link is deleted.
	 * 
	 * @param link Link object representing the deleted link.
	 */
	virtual void linkDeleted(Link* link) = 0;
	
	/**
	 * Called when a link is opened. This is called before notifying the
	 * link thread within the Link object.
	 * 
	 * @param link Link object representing the opened link.
	 * @param node Node object associated with the link. Note that this may
	 *    be null indicating no current association.
	 */
	virtual void linkOpened(Link* link, Node* node) = 0;
	
	/**
	 * Called when a link transitions from the open state. This is called
	 * after notifying the link thread.
	 * 
	 * @param link Link object representing the closed/unavailable link.
	 * @param node Node object associated with the link. Note that this may
	 *    be null.
	 */
	virtual void linkNotOpen(Link* link, Node* node) = 0;
	
	/**
	 * Called when the Link associated with a Node is being switched to
	 * another open Link. This differs from linkAssociated() in that
	 * this method is called when the Node had been previously associated
	 * and linkAssociated() is called when the Node is newly associated.
	 * 
	 * @param link Link object representing the new open link.
	 * @param node Node object associated with the link.
	 */
	virtual void linkSwitched(Link* link, Node* node) = 0;
	
	/**
	 * Called when a previously open Link becomes associated with a Node
	 * object. Note that typically a Link is associated with Node when the
	 * Link opens, unless the Node is already associated with another Link.
	 * If a Node switches links, the linkSwitched() method is used.
	 * 
	 * @param link Link object representing the already opened link.
	 * @param node Node object associated with the link.
	 */
	virtual void linkAssociated(Link* link, Node* node) = 0;
	
	/**
	 * Called when a bundle is received.
	 * 
	 * @param bundle The bundle object that was received.
	 */
	virtual void bundleReceived(Bundle* bundle) = 0;
	
	/**
	 * Called when  a bundle was locally delivered. Note that the bundle
	 * manager will have already moved the bundle to its deleted list and
	 * sent a request to DTN to have the data store delete the bundle.
	 * 
	 * @param bundle The bundle that was delivered.
	 */
	virtual void bundleDelivered(Bundle* bundle) = 0;
	
	/**
	 * Called when a bundle expires. At this point the bundle is no longer
	 * in the active or deleted bundles tables. Also, the DTN data store
	 * automatically removes any expired bundles.
	 * 
	 * @param bundle The bundle object that expired.
	 */
	virtual void bundleExpired(Bundle* bundle) = 0;
	
	/**
	 * Returns the policy the link should follow for sending bundles.
	 * We define three policies:
	 *    WAIT_METATDATA - No bundles should be sent until the meta-data
	 *                     bundle has been sent.
	 *    WAIT_DELIVERY  - Bundles on the replica queue cannot be sent
	 *                     until the meta-data bundle has been sent.
	 *                     However, bundles on the delivery queue can
	 *                     be sent before the meta-data bundle.
	 *    WAIT_ANY       - Any bundle can be sent before the meta-data
	 *                     bundle is ready.
	 *                     
	 * Note: This method can be invoked from any thread.
	 *
	 * @param link Link that the bundles will be sent on.
	 * @param node Node the link is associated with.
	 * @return Returns the policy value.
	 */
	const static int WAIT_METATDATA = 0;
	const static int WAIT_DELIVERY  = 1;
	const static int WAIT_ANY       = 2;
	virtual int waitPolicy(Link* link, Node* node) = 0;
	
	/**
	 * Returns the number of outstanding transmits permitted on the link by
	 * the policy manager. Note that this value is separate from the depth
	 * defined by the DTN link. A typical use of this method is flow control
	 * transmits on data queues until the meta-data bundle can be sent.
	 * 
	 * Note: This method can be invoked from any thread.
	 * 
	 * @param link Link that the bundles will be sent on.
	 * @param node Node the link is associated with.
	 * @param sentMetaData True if the meta-data bundle has been transmitted
	 *    (or at least queued for transmission). 
	 * @param linkValue Transmits the DTN link permits.
	 * @return Transmits permitted by the policy. This shouldn't exceed what
	 *    the link permits.
	 */
	virtual int maxOutstandingTransmits(Link* link, Node* node, bool & sentMetaData, int & linkValue) = 0;
	
	/**
	 * These constants are used for a caller to refer to each of the three
	 * Node queues for transmitting data.
	 */
	const static int QUEUE_METADATA = 0;
	const static int QUEUE_DELIVERY = 1;
	const static int QUEUE_REPLICA  = 2;
	
	/**
	 * This method is called by the Link thread before attempting to
	 * send a bundle. It allows the policy manager to have a final
	 * opportunity to prevent a bundle from being sent to another node.
	 * For example, the policy manager may choose a lazy approach in
	 * updating the Node's send queues, so the queue could contain
	 * acknowledged bundles.
	 * 
	 * Note: This method is invoked from a Link thread.
	 * 
	 * @param link Link the bundle would be sent on.
	 * @param bundle Bundle to be sent.
	 * @param node Node the bundle would be sent to.
	 * @param queue Queue that the bundle was taken from.
	 * @return True if the bundle should be sent, else false.
	 */
	virtual bool shouldTransmit(Link * link, Bundle * bundle, Node * node, int & queue) = 0;
	
	/**
	 * Called when the link initiates a request to transmit a bundle.
	 * 
	 * Note: This method is invoked from a Link thread.
	 * 
	 * @param link Link the bundle is being sent on.
	 * @param node Node the bundle is being sent to.
	 * @param bundle Bundle being sent.
	 * @param finalHop True if the node is the bundle's final destination.
	 * @param metaData True if the bundle is meta data intended for a peer.
	 * @param queue Indicates which queue bundle was removed from.
	 */
	virtual void bundleTransmitStart(Link * link, Node * node, Bundle * bundle, bool & finalHop, bool & metaData, int queue) = 0;
	
	/**
	 * Called when the transmission of a bundle has been acknowledged by
	 * DTN. Note that DTN does not provide negative acknowledgments (i.e.
	 * acknowledgments of transmissions failing). Also note that that
	 * bundles are always sent with the copy option (versus forward).
	 * The expectation is that the DTN configuration has "param set
	 * early_deletion false" defined. If set to true then be aware that
	 * DTN will likely delete bundles after they have been successfully
	 * transmitted. Finally, note that transmission does not imply
	 * delivery to a specific endpoint.
	 * 
	 * @param link Link the bundle was sent on.
	 * @param node Node the bundle had been sent to.
	 * @param localId Local id of the bundle that was sent.
	 * @param bytesSent Count of bytes sent.
	 * @param reliablySent Count of bytes reliably sent. The distinction
	 *    is defined by DTN.
	 */
	 virtual void bundleTransmitStop(Link * link, Node * node, std::string localId, int bytesSent, int reliablySent) = 0;
	
	/**
	 * Called to get the number of sends outstanding for a link/node.
	 * Note that the policy manager is expected to maintain this
	 * information, not the link.
	 * 
	 * Note: This method can be invoked from any thread.
	 * 
	 * @param link Link making the request.
	 * @param node Node associated with the link.
	 * @return Number of currently outstanding sends.
	 */
	virtual int outstandingTransmits(Link * link, Node * node) = 0;
	
	/**
	 * Called when a bundle has been successfully injected.
	 * 
	 * @param localId Local Id DTN assigned to the bundle.
	 * @param reqId The local request id we assigned to the bundle.
	 * @param bundle Bundle object representing the injected bundle.
	 */
	virtual void bundleInjected(std::string localId, std::string reqId, Bundle * bundle) = 0;
	
	/**
	 * Called when a meta data bundle is received from a peer router. These
	 * bundles are not represented in the active or deleted bundles tables,
	 * and the data store has been told to delete the bundle prior to
	 * calling this method.
	 * 
	 * Note: This method is invoked from the PeerListener thread.
	 * 
	 * @param bundle Object representing the bundle.
	 * @param data Byte array containing the bundle's data.
	 */
	 virtual void metaDataReceived(std::string, std::string data, std::string type) = 0;
	
	/**
	 * The following mirror the LinkedQueue methods for each of the three
	 * defined bundle queues. However, the policy manager implementation
	 * can choose to back these calls with any type of data structure.
	 * 
	 * Note: These methods should be invokable from any thread.
	 * 
	 * @param node Node object to be acted upon.
	 * @return Typically the bundle, unless simply checking for existence.
	 */
	virtual bool deliveryIsEmpty(Node * node) = 0;
	virtual bool replicaIsEmpty(Node * node) = 0;
	virtual bool metaDataIsEmpty(Node * node) = 0;
	virtual Bundle * deliveryPoll(Node * node) = 0;
	virtual Bundle * replicaPoll(Node * node) = 0;
	virtual Bundle * metaDataPoll(Node *  node) = 0;
	

};


#endif
