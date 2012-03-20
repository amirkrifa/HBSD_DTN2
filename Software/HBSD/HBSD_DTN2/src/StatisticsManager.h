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


#ifndef STATISTICS_MANAGER_H
#define STATISTICS_MANAGER_H

#include <string>
#include <map>
#include <list>
#include "Util.h"

// Default values used if there is no values already specified in the config file
#define MAX_NUMBER_OF_STAT_MESSAGES 1000
#define DEFAULT_MUM_BUFFER_CAPACITY 20
#define AXE_LENGTH 360
#define AXE_SUBDIVISION 100
#define DEFAULT_NUMBER_OF_NODES_WITHIN_THE_NETWORK 20
#define USE_AXE_SUBDIVISION_AS_AVG_MEETING_TIME true
#define USE_ONLINE_APPROXIMATED_NUMBER_OF_NODES true

typedef struct NodeVersion{
	std::string nodeId;
	int version;
}NodeVersion;

typedef std::list<NodeVersion> NodeVersionList;

class StatisticsManager;

class StatisticsAxe{
public :
	StatisticsAxe()
	{
		maxLt = 0;
		minLt = 0;
		this->sm_ = NULL;
	};

	void initiateIntervall(double min,double max, StatisticsManager *s);


	double maxLt;
	double minLt;
	StatisticsManager* sm_;
};


// The DTN Statistics Node
class DtnStatNode
{
public :

	DtnStatNode(char * id, double lv, StatisticsManager *ns);
	~DtnStatNode();
	// Returns the current stat node ID
	char * getNodeId();
	// Returns the current stat node lats meeting time
	double getMeetingTime()
	{
		return lastMeetingTime;
	}
	
	// Updates the stat node last meeting time
	void updateMeetingTime(double t)
	{
		lastMeetingTime = t;
	}
	
	
	void addBundle()
	{
		numberOfBundles++;
	}
	
	// Updates the stat node associated version
	void updateStatVersion(int v)
	{
		statVersion = v;
	}
	
	// Shows the seen samples map
	void showMiMap();
	// Shows the copies samples map
	void showBitMap();
	// Updates the copies map samples
 	void setTimeBinValue(int bin, int value, bool, bool, double);
	// Updates the copies map samples starting from a received map
 	void updateBitMap(std::map<int, int> &);
	// Updates the seen samples map starting from the copies samples one
	void updateMiBitMap();	
	// The copies samples map
	std::map<int, int> bitMap;
	// The copies samples map status
	std::map<int, int> bitMapStatus;
	// The seen samples map
	std::map<int, int> miBitMap;
	// The seen map start index
	int miStartIndex;
	// Indicates whether the seen samples map is updated for the last time or not
	bool miMapDone;
	// The stat node related version
	// Only the source of the message could modify the version of the statistics
	int statVersion;
	// Indicates whether the node related stats was updated since the last time or not	
	double updated;

private :
	// The satst node id	
	std::string nodeId;
	// The stat node last meeting time
	double lastMeetingTime;
	// 
	int numberOfBundles;
	// The statistics axe ength
	int axeLength;
	// A pointer to the statistics manager
	StatisticsManager *sm_;

};


// The DTN statistics message
class DtnStatMessage
{
public:
	DtnStatMessage(char * bid,double ttl, StatisticsManager * nsb);	
	~DtnStatMessage();
	// Returns the stat message ID
	char * getBstatId();
	// Adds a node to the stat message
	void addNode(std::string nodeId, std::map<int, int> & m, double lm, int statVersion, int miStartIndex);
	void addNode(std::string nodeId, int bin, int binValue, double meetingTime, int statVersion);
	// Returns the number of nodes related to the stat message
	int getNumberOfNodes()
	{
		return (int)mapNodes.size();
	}
	
	// Shows the whole message seen samples map
	void showMiMapMessage();
	// Shows the whole message copies samples map
	void showNiMapMessage();

	// Shows the list of stat nodes related to the current message
	void showNodes();
	// Shows the whole message related statistics
	int showStatistics();

	// Returns the whole message description to be sent
	void getDescription(std::string &d);
	void getSubSetDescription(std::string &d, std::map<std::string, int> & nodesList);
	// Returns a (MessageId, NodeId, Version) description
	void getVersionBasedDescription(std::string &d);
	// Returns a given stat node related version
	int getNodeVersion(std::string & nodeId);
	
	// Updates the message related seen samples map
	void updateMiMap(int binIndex);
	// Updates the message related copies samples map
	void updateNiMap(int binIndex);
	// Updates the message DD samples map
	void updateDdMap();
	// Updates the message DR samples map
	void updateDrMap();

	void immediateUpdateMiMap();
	void immediateUpdateNiMap();
	void immediateUpdateDdMap();
	void immediateUpdateDrMap();

	// Returns the number of nodes that have seen the current message at a given bin
	int getNumberOfNodesThatHaveSeeniT(int binIndex);
	// Returns the number of copies of the current message at a given bin
	int getNumberOfCopiesAt(int binIndex);
	// Returns the expected avg DD at a given bin
	double getAvgDdAt(int binIndex);
	// Returns the expected avg DR at a given bin
	double getAvgDrAt(int binIndex);
	// Returns if the current stat message is valid with respect to the given binIndex
	bool isValid(int binIndex);
	// Returns the current stat message sampllest version
	int getMaxVersion(int & smallerVersion);
	// Sets the current message life time
	void setLifeTime(double lt)
	{
		lifeTime = lt;
		lastLtUpdate = Util::getCurrentTimeSeconds();
	}
	// Returns the current message life time
	double getLifeTime()
	{
		lifeTime = (Util::getCurrentTimeSeconds() - lastLtUpdate) + lifeTime;
		lastLtUpdate = Util::getCurrentTimeSeconds();
		return lifeTime;
	}
	
	// The message life time
	double lifeTime;
	// The message last update time
	double lastLtUpdate;
	// The message TTL
	double ttl;
	// Indicates whether the message has been updated or not
	double updated;
	// Indicates when we've received the last Mi report
	double lastMiReport;
	// Indicates when we've received the last Ni report
	double lastNiReport;
	// Indicates when we've received the last DD report
	double lastDdReport;
	// Indicates when we've received the last DR report
	double lastDrReport;
	// indicates if the stat message is old enought to be removed from
	// the active stats and put within the old ones
	// message_status = 0 the message is still considered during the stat exchanges between two nodes
	// message_status = 1 the message is old enought ( version >= axe_lenght) the node will not ask for stat updates related to this message
	int messageStatus;
	std::map<std::string, DtnStatNode* > mapNodes;
	// The current message number (in the matrix)
	int messageNumber;
	// Indicates whether the current statistics node should be deleted or not
	bool toDelete;
private :
	// A pointer to the statistics manager
	StatisticsManager * sm_;
	// The current message ID
	std::string bundleId;
	// Map for maintaining the number of nodes that have seen these message for each time bin
	std::map<int, int> miMap;
	// Map for maintaining the number of copies of the message at each time bin
	std::map<int, int> niMap;
	// map for maintaining the a dd value for each time bin
	std::map<int, double> ddMap;
	// map for maintaining the dr value for each time bin
	std::map<int, double > drMap;
};


// The statistics manager
class StatisticsManager{

public :

	StatisticsManager();
	~StatisticsManager();
	
	// Called from the outside to get the statistics results
	void getStatFromAxe(double et,char *,double *ni,double *mi,double *dd_m,double *dr_m);
	void getStatFromAxe(int binIndex, double *ni, double *mi, double *dd_m, double *dr_m, double *et);
	int  getNumberOfCopies(char * bundleId, double lt);
	int  getNumberOfStatNodesThatHaveSeenIt(char * bundleId, double lt);
	double getAvgNumberOfCopies(double lt);
	double getAvgNumberOfStatNodesThatHaveSeenIt(double lt);
	double getAvgDrAt(double lt);
	double getAvgDdAt(double lt);	
	double getAvgNumberOfCopies(int binIndex);
	double getAvgNumberOfStatNodesThatHaveSeenIt(int binIndex);
	double getAvgDrAt(int binIndex);
	double getAvgDdAt(int binIndex);
	void updateBundleStatus(char * nodeId, char * bundleId, int del,double lt,double ttl);
	// Used for the pushing policy
	void getStatToSend(std::string &);
	// Used for the reverse schema policy
	void getStatToSendBasedOnReceivedIds(std::string & ids, std::string & stat);
	void getMessageNodesCouples(std::string &message_id, std::string &description);
	void getStatToSendBasedOnVersions(std::string & ids, std::string & stat);
	void convertVersionsBasedStatToMap(std::string &stat, std::map<std::string, NodeVersionList> &);
	

	// Returns the number of stat nodes
	int  getNumberOfStatNodes();
	// Shows all the messages statistics
	void showAllMessagesStatistics();
	// Shows the avg statistics
	void showAvgStatistics();
	// Returns whether a gien message id is in the matrix or not
	DtnStatMessage *isBundleHere(char *bundleId);
	// Adds a stat node
	void addStatNode(char * nodeId, double tView);
	// Adds a stat message to the matrix
	int addStatMessage(char * bundleId,char * nodeId, double lt,double ttl);
	void addStatMessage(char * message,char * messageId);
	double getNodeMeetingTime(std::string nodeId);
	bool isStatNodeAvailable(std::string nodeId);
	std::string getListOfAvailableStatNodes();

	//Managing the received stat std:string
	int  getNumberOfMessagesFromStat(char * recived_stat);
	void getMessageIFromStat(char *recived_stat, int p, std::string &id);
	void getBundleId(char * bundle, std::string & id);
	unsigned int getNumberOfStatNodesForABundle(char *bundle);
	void getNodeFromStat(char *bundle, unsigned int p, std::string & node);
	void getNodeBitMap(std::string & node, std::map<int, int> &m);
	double getBundleTtl(char * bundle);
	int  getBundleDel(char * node);
	void getNodeIdFromStat(char *node, std::string &id);
	double getNodeLm(char *node);
	int getNodeStatVersion(char *node);
	int getMiIndexFromStat(char *node);
	
	// Called from the outside to update the collected statistics	
	void updateNetworkStat(char * recived_stat);
	void updateMessage( std::string&,std::string&, DtnStatMessage*);

	// Convert an elapsed time to a bin index
	int convertElapsedTimeToBinIndex(double et);
	double convertBinINdexToElapsedTime(int binIndex);
	// Returns a bloom filter of all messages	
	void getMessagesBloomFilter(std::string & bf);
	int getNumberOfMessages(){return messagesMatrix.size();}
	
	// Returns the number of invalid messages i'm interested in
	int getNumberOfInvalidMessages();
	int getNumberOfValidMessages(std::map<std::string, DtnStatMessage *>::iterator & iterOldest);
	void deleteMessage(std::map<std::string, DtnStatMessage *>::iterator & iterOldest);
	bool isMessageValid(std::string msgId);
	void clearMessages();
	void logAvgStatistics();

	double getAverageNetworkMeetingTime();
    int getApproximatedNumberOfNodes();

    StatisticsAxe *statAxe;


	double 	statLastUpdate;
	int numberOfMeeting;
   	double totalMeetingSamples;
  	int axeLength;
	int axeSubdivision;
	int statMessagesNumber;
	int mumBuffercapacity;
	int numberOfNodesWithinTheNetwork;
	bool someThingToClean;
	bool useBinSizeForAvgMeeting;
	bool useOnlineAproximatedNumberOfNodes;
	unsigned int clearFlag;

private :
	
	std::map<std::string, DtnStatMessage *> messagesMatrix;
	std::map<std::string, double > nodesMatrix;
	int numberOfStatNodes;
	int numberOfStatMessages;
};

#endif
