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


#include "StatisticsManager.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sstream>
#include <map>
#include <list>
#include <complex>
#include <iostream>
#include <stdio.h>
#include <cstring>
#include "HBSD.h"
#include "ConfigFile.h"

using namespace std;

/** DTN Node Entry constructor 
*/

DtnStatNode::DtnStatNode(char * id, double lv, StatisticsManager *ns)
{
	statVersion = 0;
	nodeId.assign(id);
	lastMeetingTime = lv;
	updated = Util::getCurrentTimeSeconds();
	sm_ = ns;
	axeLength = ns->axeLength;
	// Init the bitmap
	for(int i = 0; i < axeLength; i++)
	{
		bitMap[i] = 0;
		miBitMap[i] = 0;
		bitMapStatus[i] = 0;
	}
	miStartIndex = -1;
	miMapDone = false;
}

DtnStatNode::~DtnStatNode()
{
	if(!bitMap.empty())
		bitMap.clear();
	if(!miBitMap.empty())
		miBitMap.clear();
}

/** Return the node ID
 * 
*/

char * DtnStatNode::getNodeId()
{
	return (char *)nodeId.c_str();
}

void DtnStatNode::showMiMap()
{
	fprintf(stdout, "NodeId : %s MiMap version: %i ",nodeId.c_str(), statVersion);
	for(int i= 0 ;i<axeLength;i++)
	{
		fprintf(stdout, "%i, ",  miBitMap[i]);
	}	
	fprintf(stdout,"\n");
}

void DtnStatNode::showBitMap()
{
	fprintf(stdout, "NodeId : %s BitMap version: %i ",nodeId.c_str(), statVersion);
	for(int i= 0 ;i<axeLength;i++)
	{
		fprintf(stdout, "%i, ",  bitMap[i]);
	}	
	fprintf(stdout,"\n");
}

void DtnStatNode::updateMiBitMap()
{
	if(!miMapDone && miStartIndex != -1)
	{
		for(int i = miStartIndex; i< axeLength; i++)
		{
			miBitMap[i] = 1;
		}
		miMapDone = true;
	}	
}

	
void DtnStatNode::setTimeBinValue(int bin, int value, bool source, bool newOld , double lt)
{
/*
	if(value = 1)
	{	
		bitMap[bin] = value;
		bitMapStatus[bin] = 1;
	}
	else if(value == 0)
	{
		if(bitMapStatus[bin] == value)
		{
			bitMap[bin] = value;
		}
	}
*/
	// Nothing to do if the bitMap[bin] = 1

	
	if(source)
	{
		if(newOld)
			miStartIndex = bin; 
		if(value == 1)
			bitMap[bin] = value;
		updated = Util::getCurrentTimeSeconds();
		// Only the source node sets the other bins to the same value	
		//if( value == 1 || (value == 0 && lt > bin*sm_->axeSubdivision ) )
		updateStatVersion(bin);
		miBitMap[bin] = 1;
		int j = bin + 1;
		for(; j < axeLength; j++)
		{
			if(value == 0)
				bitMap[j] = value;
			if(newOld)
				miBitMap[j] = 1;
			// just approximation
			//bitMapStatus[j] = 0;
		} 
	}

	if(bitMap[bin] != 0 && bitMap[bin] != 1)
	{
		fprintf(stdout, "bitMap[bin] == %i\n", bitMap[bin]);
		exit(1);
	}
}


void DtnStatNode::updateBitMap(map<int, int>  &map)
{
	// make an or operation with the received map

	for(int i = 0; i< axeLength;i++)
	{
		bitMap[i] = map[i];
		//fprintf(stdout,"%i ", map[i]); 
	}	
	//fprintf(stdout,"\n");
}


/** DtnStatMessage Constructor
*/

DtnStatMessage::DtnStatMessage(char * bid,double ttl, StatisticsManager * nsb)
{
	bundleId.assign(bid);
	this->ttl = ttl;
	updated = Util::getCurrentTimeSeconds();
	lastMiReport = 0;
	lastNiReport = 0;
	lastDdReport = 0;
	lastDrReport = 0;
	setLifeTime(0);
	toDelete = false;
	sm_ = nsb;
	messageStatus = 0;
	for(int i=0; i< nsb->axeLength; i++)
	{
		miMap[i] = 0;
		niMap[i] = 0;
		ddMap[i] = 0;
		drMap[i] = 0;
	}
	messageNumber = 0;
}


/** DtnStatMessage Destructor
*/
DtnStatMessage:: ~ DtnStatMessage()
{
	while(!mapNodes.empty())
	{
		map<string, DtnStatNode * >::iterator iter = mapNodes.begin();
		delete iter->second;
		mapNodes.erase(iter);
	}
	miMap.clear();
	niMap.clear();
	ddMap.clear();
	drMap.clear();
}


void DtnStatMessage::showNodes()
{
	fprintf(stdout, "Number of nodes: %i\n", mapNodes.size());
	for(map<string, DtnStatNode * >::iterator iter = mapNodes.begin();iter != mapNodes.end();iter++)
	{
		fprintf(stdout, "	Node id: %s\n", iter->first.c_str());
	}
}


int DtnStatMessage::getNodeVersion(string & nodeId)
{
	map<string, DtnStatNode * >::iterator iter = mapNodes.find(nodeId);
	if(iter != mapNodes.end())
	{
		return (iter->second)->statVersion;
	}else {
		// it should take this message into consideration, it does not have it
		return -1;
	}
}


int DtnStatMessage::getNumberOfNodesThatHaveSeeniT(int binIndex)
{
	if(lastMiReport < updated && mapNodes.size()>0)
	{	
		updateMiMap(binIndex);
		lastMiReport = Util::getCurrentTimeSeconds();
	}
	if(miMap[binIndex] == 0) return 1;
	return miMap[binIndex];
}

int DtnStatMessage::getNumberOfCopiesAt(int binIndex)
{
	if(lastNiReport < updated && mapNodes.size()>0)
	{	
		updateNiMap(binIndex);
		lastNiReport = Util::getCurrentTimeSeconds();
	}
	if(niMap[binIndex] == 0) return 1;
	return niMap[binIndex];
}

double DtnStatMessage::getAvgDdAt(int binIndex)
{
	if(lastDdReport < updated && mapNodes.size()>0)
	{
		updateDdMap();
		lastDdReport = Util::getCurrentTimeSeconds();
	}
	return ddMap[binIndex];
}

double DtnStatMessage::getAvgDrAt(int binIndex)
{
	if(lastDrReport < updated && mapNodes.size()>0)
	{
		updateDrMap();
		lastDrReport = Util::getCurrentTimeSeconds();
	}
	return drMap[binIndex];
}

/**Return A DtnStatMessage ID
 * 
 */

char * DtnStatMessage::getBstatId(){

	return (char *)bundleId.c_str();
}


// returns messageId + TTL * NodeId = meetingTime@version[bitMap] $ NodeId = meetingTime@version[bitMap]
void DtnStatMessage::getDescription(string &d)
{
	d.clear();
	d.append(bundleId);

	// Life Time
	/*
	d.append(sizeof(char),'-');
	char length2[50];
	sprintf(length2,"%f", life_time);
	length2[strlen(length2)+1]='\0';
	d.append(length2);
	*/
	
	// appending the bundle ttl
	
	d.append(sizeof(char),'+');
	char length3[50];
	sprintf(length3,"%f", ttl);
	length3[strlen(length3)+1]='\0';
	d.append(length3);
		
	if(!mapNodes.empty())
	{
		d.append(sizeof(char),'*');
		
		for(map<string, DtnStatNode *>::iterator iterB = mapNodes.begin(); iterB != mapNodes.end(); iterB++)
		{
			if(iterB->first.length() != 0)
			{
				// Appending the nodes separator if more than one
				if(iterB != mapNodes.begin())
					d.append(sizeof(char),'$');

				// Appending the node id	
				d.append(iterB->first);
										
				// Appending the node meeting time
				
				char length[50];
				sprintf(length,"%f",sm_->getNodeMeetingTime(iterB->first));
				d.append(sizeof(char),'=');
				d.append(length);
							
				// Appending the stat version
				char version[50];
				if((iterB->second)->statVersion > sm_->axeLength)
				{
					fprintf(stdout, "Invalid version: %i Line: %i\n",(iterB->second)->statVersion, __LINE__);
					exit(1);
				}
				
				sprintf(version, "%i", (iterB->second)->statVersion);
				d.append(sizeof(char),'@');
				d.append(version);
				
				// Appending the miMap start index
				char miIndex[50];
				sprintf(miIndex, "%i", (iterB->second)->miStartIndex);
				d.append(sizeof(char),'?');
				d.append(miIndex);
//				fprintf(stdout, "MiStartIndex from description: %s\n", miIndex);					
									
				// Appending the bit status
				d.append(sizeof(char),'[');
				for(int k = 0; k< sm_->axeLength;k++)
				{
					if(k>0) 
						d.append(sizeof(char),',');
					char tmpi[2];
					sprintf(tmpi,"%i",(iterB->second)->bitMap[k]);
					d.append(tmpi);

				} 
				d.append(sizeof(char),(char)']');
			}else 
			{
				fprintf(stdout, "iterB->first.length() == 0, size: %i\n", mapNodes.size());exit(1);
			}
		}

	}
	
}
// Returns the description based on a subset of the nodes
void DtnStatMessage::getSubSetDescription(string &d, map<string, int> & nodesList)
{
	d.clear();
	d.append(bundleId);

	// Life Time
	/*
	d.append(sizeof(char),'-');
	char length2[50];
	sprintf(length2,"%f", life_time);
	length2[strlen(length2)+1]='\0';
	d.append(length2);
	*/
	
	// appending the bundle ttl
	
	d.append(sizeof(char),'+');
	char length3[50];
	sprintf(length3,"%f", ttl);
	length3[strlen(length3)+1]='\0';
	d.append(length3);
	
		
	if(!mapNodes.empty())
	{
		d.append(sizeof(char),'*');
		
		for(map<string, DtnStatNode *>::iterator iterB = mapNodes.begin(); iterB != mapNodes.end(); iterB++)
		{
			map<string, int>::iterator iterTest = nodesList.find(iterB->first);
			if(iterB->first.length() != 0 && iterTest != nodesList.end())
			{
				// Appending the nodes separator if more than one
				if(iterB != mapNodes.begin())
					d.append(sizeof(char),'$');

				// Appending the node id	
				d.append(iterB->first);
										
				// Appending the node meeting time
				
				char length[50];
				sprintf(length,"%f",sm_->getNodeMeetingTime(iterB->first));
				d.append(sizeof(char),'=');
				d.append(length);
							
				// Appending the stat version
				char version[50];
				if((iterB->second)->statVersion > sm_->axeLength)
				{
					fprintf(stdout, "Invalid version: %i Line: %i\n",(iterB->second)->statVersion, __LINE__);
					exit(1);
				}
	
				sprintf(version, "%i", (iterB->second)->statVersion);
				d.append(sizeof(char),'@');
				d.append(version);

				// Appending the miMap start index
				char miIndex[50];
				sprintf(miIndex, "%i", (iterB->second)->miStartIndex);
				d.append(sizeof(char),'?');
				d.append(miIndex);

				//fprintf(stdout, "MiStartIndex from description: %s\n", miIndex);				
				// Appending the bit status
				d.append(sizeof(char),'[');
				for(int k = 0; k< sm_->axeLength;k++)
				{
					if(k>0) 
						d.append(sizeof(char),',');
					char tmpi[2];
					sprintf(tmpi,"%i",(iterB->second)->bitMap[k]);
					d.append(tmpi);

				} 
				d.append(sizeof(char),(char)']');
			}else if(iterB->first.length() == 0 )
			{
				fprintf(stdout, "iterB->first: %s iterB->first.length() == 0, size: %i\n",iterB->first.c_str(), mapNodes.size());showNodes();exit(1);
			}
		}

	}else d.clear();
	
}

// returns messageid * nodeid @ version $ nodeid @ version ...
void DtnStatMessage::getVersionBasedDescription(string &d)
{
	d.clear();
	d.append(bundleId);

		
	if(!mapNodes.empty())
	{
		d.append(sizeof(char),'*');
		
		for(map<string, DtnStatNode *>::iterator iterB = mapNodes.begin(); iterB != mapNodes.end(); iterB++)
		{
			if(iterB->first.length() != 0)
			{
				// Appending the nodes separator if more than one
				if(iterB != mapNodes.begin())
					d.append(sizeof(char),'$');

				// Appending the node id	
				d.append(iterB->first);
										
				// Appending the stat version
				char version[50];
				if((iterB->second)->statVersion > sm_->axeLength)
				{
					fprintf(stdout, "Invalid version: %i Line: %i\n",(iterB->second)->statVersion, __LINE__);
					exit(1);
				}
	
				sprintf(version, "%i", (iterB->second)->statVersion);
				d.append(sizeof(char),'@');
				d.append(version);

			}else 
			{
				fprintf(stdout, "iterB->first.length() == 0, size: %i\n", mapNodes.size());exit(1);
			}
		}

	}

}


void DtnStatMessage::addNode(string nodeId, int bin, int binValue, double meetingTime, int statVersion)
{	

	
	if(statVersion > sm_->axeLength)
	{
		fprintf(stdout, "Invalid version: %i Line: %i\n",statVersion, __LINE__);
		exit(1);
	}
	
	map<string, DtnStatNode *>::iterator iter = mapNodes.find(nodeId);
	if(iter != mapNodes.end())
	{
		mapNodes[nodeId]->setTimeBinValue(bin, binValue, true, false, getLifeTime());
		mapNodes[nodeId]->updateMeetingTime(meetingTime);
	}else
	{ 
		mapNodes[nodeId] = new DtnStatNode((char*)nodeId.c_str(), meetingTime,sm_);
		mapNodes[nodeId]->setTimeBinValue(bin, binValue, true, true, getLifeTime());
	}
}

void DtnStatMessage::addNode(string nodeId, map<int, int> & m, double lm, int statVersion, int miStartIndex)
{	
//	fprintf(stdout, "Adding a node, miStartIndex: %i\n",miStartIndex);
	if(statVersion > sm_->axeLength)
	{
		fprintf(stdout, "Invalid version: %i Line: %i\n",statVersion, __LINE__);
		exit(1);
	}
	map<string, DtnStatNode *>::iterator iter = mapNodes.find(nodeId);
	if(iter != mapNodes.end())
	{
		// Update the node only if the new report has a newer version
		if(mapNodes[nodeId]->statVersion <= statVersion)
		{
			mapNodes[nodeId]->miStartIndex = miStartIndex;
			mapNodes[nodeId]->miMapDone = false;
			//fprintf(stdout,"miStartIndex: %i\n", miStartIndex);
			mapNodes[nodeId]->updateBitMap(m);
			mapNodes[nodeId]->updateMeetingTime(lm);
			mapNodes[nodeId]->updateMiBitMap();
			mapNodes[nodeId]->updateStatVersion(statVersion);
			
		}
	}else
	{ 
		mapNodes[nodeId] = new DtnStatNode((char*)nodeId.c_str(), lm,sm_);
		mapNodes[nodeId]->miStartIndex = miStartIndex;
		mapNodes[nodeId]->updateBitMap(m);
		mapNodes[nodeId]->updateMiBitMap();
		mapNodes[nodeId]->updateStatVersion(statVersion);
	}
	
}

int DtnStatMessage::showStatistics()
{
	fprintf(stdout, "*****************************************************\n");
	if(!mapNodes.empty())
	{		
		fprintf(stdout, "Number of Nodes: %i\n", mapNodes.size());
		map<string, DtnStatNode *>::iterator iter = mapNodes.begin();
		for(;iter != mapNodes.end(); iter ++)
		{
			(iter->second)->showMiMap();
			(iter->second)->showBitMap();
			fprintf(stdout, "-------------------------------\n");
		}
		updateMiMap(0);
		updateNiMap(0);
	}
		
	showMiMapMessage();
	showNiMapMessage();
	return 1;
}

void DtnStatMessage::showMiMapMessage()
{
	fprintf(stdout, "MessageId : %s MiMapMessage size: %i \n",bundleId.c_str(),miMap.size());
	for(int i= 0 ;i < sm_->axeLength;i++)
	{
		fprintf(stdout, "%i, ",  miMap[i]);
	}	
	fprintf(stdout,"\n");
}

void DtnStatMessage::showNiMapMessage()
{
	fprintf(stdout, "MessageId : %s NiMapMessage size: %i \n",bundleId.c_str(),niMap.size());
	for(int i= 0 ;i < sm_->axeLength;i++)
	{
		fprintf(stdout, "%i, ",  niMap[i]);
	}	
	fprintf(stdout,"\n");
}


void DtnStatMessage::updateMiMap(int binIndex)
{
	if(messageStatus == 0)
	{
		for(int i= 0;i < sm_->axeLength; i++)
		{
			miMap[i] = 0;
			for(map<string, DtnStatNode *>::iterator iter = mapNodes.begin();iter != mapNodes.end(); iter ++)
			{	
				miMap[i] += (iter->second)->miBitMap[i];
			}
		}
	}
	
}

void DtnStatMessage::updateNiMap(int binIndex)
{
	if(messageStatus == 0)
	{
		
		for(int i= 0;i < sm_->axeLength; i++)
		{
			niMap[i] = 0;
			for(map<string, DtnStatNode *>::iterator iter = mapNodes.begin();iter != mapNodes.end(); iter ++)
			{
				niMap[i] += (iter->second)->bitMap[i];
			}
		}
	}
	
}

void DtnStatMessage::updateDdMap()
{
	if(messageStatus == 0)
	{
		
		for(int i= 0;i < sm_->axeLength; i++)
		{
			ddMap[i] = 0;	
			{
				int nc = getNumberOfCopiesAt(i);
				if(nc > 0)
					ddMap[i] = (double)((sm_->getApproximatedNumberOfNodes() - 1 - getNumberOfNodesThatHaveSeeniT(i))/nc);
				else {ddMap[i] = 0;}
			}
		}
	}
}

void DtnStatMessage::updateDrMap()
{

double alpha = 0;
try
{
	if(messageStatus == 0)
	{
		drMap.clear();

		double amt;
		if(sm_->numberOfMeeting > 0)
			amt = sm_->totalMeetingSamples/sm_->numberOfMeeting;
		else amt = sm_->totalMeetingSamples;
	
		alpha = (sm_->getApproximatedNumberOfNodes() - 1) * sm_->getAverageNetworkMeetingTime() ;
		//fprintf(stdout, "Number of nodes: %i alpha: %f number of meetings: %i total meeting samples: %f\n", sm_->get_numberOfStatNodes(), alpha, sm_->numberOfMeeting, sm_->totalMeetingSamples );

		for(int i= 0;i < sm_->axeLength; i++)
		{
			double lt = sm_->convertBinINdexToElapsedTime(i);
			{
				if(sm_->getApproximatedNumberOfNodes() > 1 && alpha > 0)
				{	
					drMap[i] = (1 - (getNumberOfNodesThatHaveSeeniT(i) / (sm_->getApproximatedNumberOfNodes()-1) ) ) * exp(-1*(ttl - lt)*getNumberOfCopiesAt(i)*(1/(alpha)));
				}
				else 
				{
					drMap[i] += (1-getNumberOfNodesThatHaveSeeniT(i))*exp(-1*(ttl - lt)*getNumberOfCopiesAt(i));
				}
			}
		}
	}
	

}

catch(exception e)
{
	fprintf(stderr, "Exception Line %i: %s\n", __LINE__, e.what());
	fprintf(stdout, "sm_->get_numberOfStatNodes(): %i alpha %f\n", sm_->getApproximatedNumberOfNodes(), alpha);
	exit(1);
}
}



bool DtnStatMessage::isValid(int binIndex)
{

	if(messageStatus == 1) 
	{
		return true;
	}
	// Verify and generate the TTL version
	if(getLifeTime() >= ttl)
	{
		DtnStatNode * n = mapNodes[HBSD::routerEndpoint];
		
		// Only the message owner can generates the last version
		if(n != NULL )
		{
			if(n->statVersion < sm_->axeLength)
			{
				n->setTimeBinValue(sm_->axeLength, 0, true, false , getLifeTime());
			}
			n = NULL;
		}
	}		
	
	int smallerVersion;
	int maxVersion = getMaxVersion(smallerVersion);

	toDelete = 0;
		
	if(smallerVersion >= sm_->axeLength && mapNodes.size() >= 2)
	{
		fprintf(stdout,"life time: %f smallest version: %i maxVersion %i axeLength: %i Number Nodes: %i\n",getLifeTime(),smallerVersion,maxVersion,sm_->axeLength,mapNodes.size() );
		//ShowStatistics();
		
		// Update the maps for the last time
		updateMiMap(binIndex);
		updateNiMap(binIndex);
		updateDrMap();
		updateDdMap();
		
		// clear the list of nodes to save some memory
		map<string, DtnStatNode * >::iterator iter = mapNodes.begin();
		while (!mapNodes.empty())
		{
			delete(iter->second);
			mapNodes.erase(iter);
			iter = mapNodes.begin();
		}
		
		//map<string, DtnStatMessage *>::iterator  iterOldest;
		//int nim = sm_->GetNumberOfValidMessages(iterOldest);
	
		//if(nim > 10000)
		//{
		//	toDelete = true;
		//	sm_->someThingToClean = true;
		//}
	
		messageStatus = 1;
		return true;	
	}
	
	if(smallerVersion >= binIndex)
		return true;
	return false;
	
}


int DtnStatMessage::getMaxVersion(int & smallerVersion)
{
	int infiniteVersion = sm_->axeLength * 10;
	smallerVersion = infiniteVersion;
	int max = 0;
	for(map<string, DtnStatNode * >::iterator iter = mapNodes.begin();iter != mapNodes.end(); iter++)
	{
		if(iter->second != NULL)
		{
			if((iter->second)->statVersion > max)
			{
				max = (iter->second)->statVersion;
			}
			if((iter->second)->statVersion < smallerVersion)
			{
				smallerVersion = (iter->second)->statVersion;	
			}
		}else fprintf(stdout, "Node NUll message_status %i\n", messageStatus);
	}
	return max;
}
	





void StatisticsAxe::initiateIntervall(double min,double max, StatisticsManager *s)
{
	maxLt=max;
	minLt=min;
	sm_ = s;
};


DtnStatMessage * StatisticsManager::isBundleHere(char *bundleId)
{
	map<string, DtnStatMessage *>::iterator iter = messagesMatrix.find(bundleId);
	if(iter !=  messagesMatrix.end())
	{
		return iter->second;
	}else
	{
		return NULL;
	}

}

/** Add A node to the stat matrix
 * 
 */

void StatisticsManager::addStatNode(char * nodeId, double tView)
{

	map<string, double>::iterator iter = nodesMatrix.find(string(nodeId));
	if(iter != nodesMatrix.end())
	{
		// Updating the Last Meeting time
		iter->second = tView;
	}else
	{
		nodesMatrix[string(nodeId)] = tView;
		numberOfStatNodes++;
	}

}



/** Network Stat constructor
 * 
 */

StatisticsManager::StatisticsManager()
{
	clearFlag = 1;
	someThingToClean = false;
	numberOfStatNodes = 0;
	statLastUpdate = Util::getCurrentTimeSeconds();
	numberOfMeeting = 0;
	numberOfStatMessages = 0;
	totalMeetingSamples = 0;
	// Setting values desccribed within HBSD config File versus the Default ones
	axeLength = HBSD::routerConf->getInt(string("numberOfBins"), AXE_LENGTH);
	statAxe = new StatisticsAxe[axeLength];
	axeSubdivision = HBSD::routerConf->getInt(string("binSize"), AXE_SUBDIVISION);
	this->statMessagesNumber = HBSD::routerConf->getInt(string("mchBufferCapacity"), MAX_NUMBER_OF_STAT_MESSAGES);
	this->mumBuffercapacity = HBSD::routerConf->getInt(string("mumBufferCapacity"), DEFAULT_MUM_BUFFER_CAPACITY);
	this->useBinSizeForAvgMeeting = HBSD::routerConf->getBoolean(string("useBinSizeAsAvgMeetingTime"), USE_AXE_SUBDIVISION_AS_AVG_MEETING_TIME);
	this->useOnlineAproximatedNumberOfNodes = HBSD::routerConf->getBoolean(string("useOnlineAproximatedNumberOfNodes"), USE_ONLINE_APPROXIMATED_NUMBER_OF_NODES);
	this->numberOfNodesWithinTheNetwork = HBSD::routerConf->getInt(string("numberOfNodesWithinTheNetwork"),DEFAULT_NUMBER_OF_NODES_WITHIN_THE_NETWORK);
	for(int i = 0; i < axeLength; i++)
	{
		statAxe[i].initiateIntervall(axeSubdivision*i, axeSubdivision*(i+1),this);
	}
}



StatisticsManager::~StatisticsManager()
{
	delete [] statAxe;
	
}


/**Add a bundle to a specific node in the stat matrix
 * 
 */
int StatisticsManager::addStatMessage(char * bundleId, char * nodeId, double lt, double ttl)
{

	// Verify if we should clear the messages matrix:
	
	if(getNumberOfInvalidMessages() < this->statMessagesNumber)
	{

		if(strlen(bundleId) == 0)
		{
			fprintf(stderr, "Invalid Bundle Id\n");
			exit(1);
		}

		// double lt is used to identify the bin to update
		int binIndex = convertElapsedTimeToBinIndex(lt);


		DtnStatMessage *bs = this->isBundleHere(bundleId);

		if(bs != NULL)
		{
			bs->ttl = ttl;
			bs->updated = Util::getCurrentTimeSeconds();
			bs->setLifeTime(lt);
			statLastUpdate = Util::getCurrentTimeSeconds();
			if(strlen(nodeId) > 0)
				bs->addNode(string(nodeId), binIndex, 1, Util::getCurrentTimeSeconds(), binIndex);
			return 1;
		}
		else
		{
			// we add the bundle to the existing node
			numberOfStatMessages++;
			statLastUpdate = Util::getCurrentTimeSeconds();
			messagesMatrix[string(bundleId)] = new DtnStatMessage(bundleId,ttl, this);
			messagesMatrix[string(bundleId)]->messageNumber = numberOfStatMessages;
			messagesMatrix[string(bundleId)]->updated = Util::getCurrentTimeSeconds();
			messagesMatrix[string(bundleId)]->setLifeTime(lt);
			if(strlen(nodeId) > 0)
				messagesMatrix[string(bundleId)]->addNode(string(nodeId), binIndex, 1, Util::getCurrentTimeSeconds(), binIndex);
			return 1;
		}
	}
	else
	{
		// Maximum capacity reached, the bundle will not be added
		return 0;
	}
}

/** Get the estimated number of copies for a specific bundle from the stat matrix
 * 
 */

int  StatisticsManager::getNumberOfCopies(char * bundleId, double lt)
{
	// The current message bin Index
	int binIndex = convertElapsedTimeToBinIndex(lt);	

	int nc=0;
	map<string, DtnStatMessage* >::iterator iter =  messagesMatrix.find(string(bundleId));
	DtnStatMessage * dm = NULL; 
	if(iter != messagesMatrix.end())
	{
		dm = iter->second;
		nc = dm->getNumberOfCopiesAt(binIndex);
		if( nc == 0 )
			fprintf(stderr, "number of copies == 0 \n");
		
	}
	else
	{
		nc = 1;
	}
	
	return nc;

}

/** Get the number of nodes that have seen a specific Bundle from the stat matrix
 * 
 */
int  StatisticsManager::getNumberOfStatNodesThatHaveSeenIt(char * bundleId, double lt)
{
	// The current message bin Index
	int binIndex = convertElapsedTimeToBinIndex(lt);
	int ns = 0;
	map<string, DtnStatMessage* >::iterator iter =  messagesMatrix.find(string(bundleId));
	DtnStatMessage * dm = NULL; 
	if(iter != messagesMatrix.end())
	{
		dm = iter->second;
		ns = dm->getNumberOfNodesThatHaveSeeniT(binIndex);
	}
	else
	{
		ns = 1;
	}

	return ns;
}

/** Return A Node Infos from A stat
 */

void StatisticsManager::getMessageIFromStat(char *recived_stat, int p, string &id)
{	
try{

	if(strlen(recived_stat) != 0)
	{
		string rs(recived_stat);
		if(p==1)
		{	
			size_t pos;
			if(this->getNumberOfMessagesFromStat(recived_stat) == 1)
				pos = rs.find("#");
			else pos=rs.find("\\");
			if(pos!=std::string::npos)
			{
				string r = rs.substr(0,pos);
				char rr[r.size()+1];
				strcpy(rr,(char*)r.c_str());
				id.assign(rr);
			}
											
		}
		else 
		{
			string rs(recived_stat);
		  	int j=1;
		  	size_t pos;
		  	pos=rs.find("\\");
		  	while(j<p)
		  	{
				rs.assign(rs.substr(pos+1));
		  	  	pos=rs.find("\\");
			  	j++;
		  	}
		  	size_t pos2=rs.find("R");
		  	size_t pos3=rs.find("\\");
		  	string r_s;
		  		  
		  	if(pos3!=std::string::npos)
				  r_s=rs.substr(pos2,pos3);
		 	else	r_s=rs.substr(pos2);
		  	char r[r_s.size()+1];
		  	strcpy(r,(char*)r_s.c_str());
		  	id.assign(r);
		}
	}
}
catch(exception e)
{
	fprintf(stderr, "Exception Line %i: %s\n", __LINE__, e.what());
	fprintf(stderr, "received_stat: %s  message number: %i \n", recived_stat, p);
	exit(1);
}
}

/** Return A Node Id from Stat
 */

void StatisticsManager::getNodeIdFromStat(char *node, string &id)
{	
	if(node!=NULL)
	{
		char *n = strchr(node,'=');
		int nod_l = strlen(node);
		int n_l = strlen(n);
		if(n != NULL)
		{
			char n_id[nod_l-n_l];
			strcpy(n_id,"");
			strncat(n_id,node,nod_l-n_l);
			strcat(n_id,"\0");
			n=NULL;
			id.assign(n_id);
		}
	}
}

/** Return A node last meeting time from a stat
 * 
 */

double StatisticsManager::getNodeLm(char *node)
{
try{
	string nod(node);
	size_t pos1=nod.find("=");
	size_t pos2=nod.find("@");
	string lm=nod.substr(pos1+1,pos2-1);
	return atof(lm.c_str());
}
catch(exception e)
{
	fprintf(stderr, "Exception Line %i: %s\n", __LINE__, e.what());
	exit(1);
}
}

int StatisticsManager::getNodeStatVersion(char *node)
{
try{
	string nod(node);
	size_t pos1=nod.find("@");
	size_t pos2=nod.find("?");
	string lm=nod.substr(pos1+1,pos2-1);
	if(atoi(lm.c_str()) > axeLength)
	{
		fprintf(stdout, "Invalid version: %i\n", atoi(lm.c_str()));
		fprintf(stdout, "Node: %s\n", node);
		exit(1);
	}
	return atoi(lm.c_str());
}
catch(exception e)
{
	fprintf(stderr, "Exception Line %i: %s\n", __LINE__, e.what());
	exit(1);
}
}

int StatisticsManager::getMiIndexFromStat(char *node)
{
try{
	string nod(node);
	size_t pos1=nod.find("?");
	size_t pos2=nod.find("[");
	string lm=nod.substr(pos1+1,pos2-1);
	if(atoi(lm.c_str()) > axeLength)
	{
		fprintf(stdout, "Invalid miIndex: %i\n", atoi(lm.c_str()));
		fprintf(stdout, "Node: %s\n", node);
		exit(1);
	}
	return atoi(lm.c_str());
}
catch(exception e)
{
	fprintf(stderr, "Exception Line %i: %s\n", __LINE__, e.what());
	exit(1);
}
}

/** Return a bundle Id from a stat
 * 
 */
void StatisticsManager::getBundleId(char * bundle, string &id)
{
try{	
	
	string bun(bundle);
	size_t pos2=bun.find("+");
	string r=bun.substr(0,pos2);
	char rr[r.size()+1];
	strcpy(rr,r.c_str());
	id.assign(rr);
	//fprintf(stdout, "Bundle to Id: %s ID %s\n",bundle, id.c_str()); 
}
catch(exception e)
{
	fprintf(stderr, "Exception Line %i: %s\n", __LINE__, e.what());
	exit(1);
}
}

void StatisticsManager::getNodeFromStat(char * bund, unsigned int p, string & node)
{
try{	
	if(getNumberOfStatNodesForABundle(bund) >= p)
 	{	string bundle(bund);
 		if(p == 1)
 		{
 			size_t pos_e = bundle.find("*");
 			size_t pos_d_1 = bundle.find("$");
 			string br = bundle.substr(pos_e+1,pos_d_1-1);
 			node.assign(br);
			//fprintf(stdout, "P == %i Node == %s\n",p,br.c_str());
 		}else
		{
 			unsigned int j = 1;
 			size_t pos;
 			pos=bundle.find("$");
 			while(pos != std::string::npos && j<p)
 			{
 				bundle.assign(bundle.substr(pos+1));
 				pos=bundle.find("$");
 				j++;
 			}
			// TODO: Verify
 			size_t pos2 = bundle.find("R");
 			size_t pos3 = bundle.find("\\");
 			string r_s;

 			if(pos3==std::string::npos)
 			{
 				r_s = bundle.substr(pos2);
 			}else 
			{
 				r_s = bundle.substr(pos2,pos3-1);
 			}

 			node.assign(r_s);
 		}
 	}
}

catch(exception e)
{
	fprintf(stderr, "Exception Line %i: %s\n", __LINE__, e.what());
	exit(1);
}

}

/** Return if the bundle was deleted or Not From A stat
 * 
 */

int StatisticsManager::getBundleDel(char * node)
{
try{
	// TODO: Verify
	//fprintf(stdout, "getBundleDel %s\n", node);
	string bun(node);
	size_t pos1 = bun.find("@");
	string r = bun.substr(pos1+1);
	//fprintf(stdout, "Node %s r %s\n", node, r.c_str());
	int i = atoi(r.c_str());
	return i;
	
}
catch(exception e)
{
	fprintf(stderr, "Exception Line %i: %s\n", __LINE__, e.what());
	exit(1);
}
}

/** Return the bundle TTL From from the stat
 * 
 */

double StatisticsManager::getBundleTtl(char * bundle)
{
try{
	string bun(bundle);
	size_t pos1=bun.find("+");
	size_t pos2=bun.find("*");
	string r=bun.substr(pos1+1,pos2-1);
	double lt=atof(r.c_str());
	return lt;
	
}
catch(exception e)
{
	fprintf(stderr, "Exception Line %i: %s\n", __LINE__, e.what());
	exit(1);
}
}

/** Return the number of bundles of a Node from Stat
 * 
 */

unsigned int StatisticsManager::getNumberOfStatNodesForABundle(char *bundle)
{	
	if(bundle == NULL)
		return 0;
	else
	{
		unsigned int i=0;
		unsigned int j=0;
		unsigned int e=0;
		while(bundle[i]!='\0')
		{
			if(bundle[i]=='*')
				e++;
			if(bundle[i]=='$')
				j++;
			i++;
		}
		if(e==1 && j==0)
			return 1;
		if(e==0)
			return 0;
		return j+1;
	}
}

/** Update the stat matrix using data from the recived matrix resume 
 * 
 */

void StatisticsManager::updateNetworkStat(char * recv)
{

	if(strlen(recv) != 0)
	{
		//fprintf(stdout, "Updating network stat: %s\n", recv);
		int nm = getNumberOfMessagesFromStat(recv);
		int i = 1;
		
		// Extracting messages IDS
		while(i <= nm)
		{	
			
			string recived_message;
			this->getMessageIFromStat(recv,i, recived_message);

			string id;
			this->getBundleId((char *)recived_message.c_str(), id);
			//fprintf(stdout, "	message : %s message id: %s\n", recived_message.c_str(), id.c_str()); 
			if(id.length())
			{
				// view if the node already exist 
				DtnStatMessage * dm;
				if((dm = this->isBundleHere((char*)id.c_str())) == NULL)
				{	
					this->addStatMessage((char*)recived_message.c_str(), (char*)id.c_str());
				}
				else 
				{
					this->updateMessage(recived_message, id, dm);
		 		}
			}
			i++;	
		}
	}
}
/** Return the number of nodes recived from Stat
 * 
 */
int StatisticsManager::getNumberOfMessagesFromStat(char * recived_stat)
{
	if(recived_stat == NULL)
		return 0;
	else
	{
	    	int nn = 0;
		int i = 0;
		while(recived_stat[i]!='#')
		{
			if(recived_stat[i] == '\\')
				nn++;
			i++;
		}
		return nn+1;
	}
}

/** Get a resume of the current stat matrix
 * 
 */

void StatisticsManager::getStatToSend(string &stat)
{
	statLastUpdate = Util::getCurrentTimeSeconds();
	if(!messagesMatrix.empty())
	{
		
		
		stat.clear();
		for(map<string, DtnStatMessage *>::iterator iter = messagesMatrix.begin(); iter != messagesMatrix.end(); iter++)
		{
			// New Message
			if(iter != messagesMatrix.begin())
				stat.append(sizeof(char),'\\');
			string tmp;
			tmp.clear();
			(iter->second)->getDescription(tmp);
			//fprintf(stdout, "Current message description: %s\n\n", tmp.c_str());
			stat.append(tmp);
			
		}
		stat.append(sizeof(char),'#');
	}
}	

void StatisticsManager::getStatToSendBasedOnReceivedIds(string & ids, string & stat)
{
	map<string, int> idsMap;
	//fprintf(stdout, "Ids: %s\n", ids.c_str());
	//TODO:
	//bm_->er_->get_map_ids_for_stat_request(ids, idsMap);
	
	if(!messagesMatrix.empty())
	{
		int i = 0;
		
		stat.clear();
		for(map<string, DtnStatMessage *>::iterator iter = messagesMatrix.begin(); iter != messagesMatrix.end(); iter++, i++)
		{
			// add only messages it asked for
			if(idsMap.find(iter->first) != idsMap.end())
			{
				// New Message
				if(i > 0)
					stat.append(sizeof(char),'\\');
				string tmp;
				(iter->second)->getDescription(tmp);
				//fprintf(stdout, "Current message description: %s\n\n", tmp.c_str());
				stat.append(tmp);
			}
		}
		if(stat.length() > 0)
			stat.append(sizeof(char),'#');
	//	fprintf(stdout, "Stat to send: %s\n", stat.c_str());
	}
}

void StatisticsManager::getStatToSendBasedOnVersions(string & ids, string & stat)
{
	map<string, NodeVersionList> mapIds;
	//fprintf(stdout, "Ids: %s\n", ids.c_str());
	convertVersionsBasedStatToMap(ids, mapIds);
	//fprintf(stdout, "mapIds size: %i\n",mapIds.size());

	if(!messagesMatrix.empty())
	{
		int i = 0;
		stat.clear();
		for(map<string, NodeVersionList>::iterator iter = mapIds.begin(); iter != mapIds.end(); iter++)
		{
			
			map<string, DtnStatMessage *>::iterator iter2 = messagesMatrix.find(iter->first);
			
			if(iter2 != messagesMatrix.end())
			{
				//fprintf(stdout, "Messageid: %s\n",iter->first.c_str());
				map<string, int> selectedNodes;
				// message found
				for(map<string, DtnStatNode * >::iterator iter4 = (iter2->second)->mapNodes.begin(); iter4 != (iter2->second)->mapNodes.end();iter4++)
				{
	
					bool haveIt = false;
					for(NodeVersionList::iterator iter3 = (iter->second).begin(); iter3 != (iter->second).end(); iter3++)
					{
						if(iter4->first.compare((*iter3).nodeId) == 0)
						{
							haveIt = true;
							int myVersion = (iter4->second)->statVersion;
							if(myVersion > (*iter3).version)
							{
								selectedNodes[(*iter3).nodeId] = 1;
								//fprintf(stdout, "Nodeid: %s version:%i\n",(*iter3).nodeId.c_str(),(*iter3).version);
							}
							break;
						}
					}
					if(!haveIt)
					{
						selectedNodes[iter4->first] = 1;
					}
				}
				
				// Get the message description based on the selected nodes
					
				// New Message
				

				if(selectedNodes.size() > 0)
				{
					
					string tmp;
					(iter2->second)->getSubSetDescription(tmp, selectedNodes);
				
					if(tmp.length() > 0)
					{
						
						if(i > 0 )
							stat.append(sizeof(char),'\\');
						//fprintf(stdout, "Current message sub description: %s\n\n", tmp.c_str());
						stat.append(tmp);
						i++;
					}
				}
			}
		}

		if(stat.length() > 0)
		{
			stat.append(sizeof(char),'#');
		}
		//	fprintf(stdout, "Stat to send: %s\n", stat.c_str());
	}
}

void StatisticsManager::convertVersionsBasedStatToMap(string &stat, map<string, NodeVersionList> & map)
{
	int nbrMsg = getNumberOfMessagesFromStat((char*)stat.c_str());
	//fprintf(stdout, "stat: %s\n", stat.c_str());
	for(int i = 0; i< nbrMsg;i++)
	{
		string msg;
		getMessageIFromStat((char*)stat.c_str(), i+1, msg);
		// msg = msgId*Nodeid@Version $ Nodeid@Version $ Nodeid@Version ...		
		size_t pos1 = msg.find('*');
		string msgId = msg.substr(0, pos1);
		//fprintf(stdout, "msg: %s id %s\n", msg.c_str(), msgId.c_str());
		
		int nbrNodes = getNumberOfStatNodesForABundle((char*)msg.c_str());
		for(int j = 0; j< nbrNodes; j++)
		{
			string node;
			getNodeFromStat((char*)msg.c_str(), j+1, node);
			// Node = nodeid@version
			string nodeId;
			int version;
			size_t pos = node.find('@');
			nodeId = node.substr(0, pos);
			version = atoi((char*)node.substr(pos + 1).c_str());
			NodeVersion nv;
			nv.version = version;
			nv.nodeId = nodeId;
			//fprintf(stdout, "Node id: %s\n", nv.nodeId.c_str());
			map[msgId].push_back(nv);
		}										
												
	}
}

void StatisticsManager::getMessageNodesCouples(string &message_id, string &description)
{
	statLastUpdate = Util::getCurrentTimeSeconds();
	map<string, DtnStatMessage *>::iterator iter = messagesMatrix.find(message_id);
	if(iter != messagesMatrix.end())
	{
		// message found 
		string desc;
		(iter->second)->getVersionBasedDescription(desc);
		description.assign(desc);	
	}
}

/** Update the bundle statu
 */
void StatisticsManager::updateBundleStatus(char * node_id, char * bundleId, int del,double lt,double ttl)
{

	DtnStatMessage *dm;
	string bid;
	bid.assign(bundleId);
	int binIndex = convertElapsedTimeToBinIndex(lt);
	
	if((dm = isBundleHere(bundleId)) != NULL)
	{	
		dm->updated = Util::getCurrentTimeSeconds();
		dm->addNode(string(node_id), binIndex, del, Util::getCurrentTimeSeconds(), binIndex);
		dm->setLifeTime(lt);
		dm->ttl=ttl;
		statLastUpdate = Util::getCurrentTimeSeconds();

	}
}

/** Add a node to the stat matrix
 */
 
void StatisticsManager::addStatMessage(char * message,char * messageId)
{
	string message_id(message);

	// first adding the message

	double bundleTTL = getBundleTtl(message);

	// Adding the message
	

	DtnStatMessage *dm = this->isBundleHere(messageId);

	if(dm != NULL)
	{	
		dm->ttl = bundleTTL;
		dm->updated = Util::getCurrentTimeSeconds();
		statLastUpdate = Util::getCurrentTimeSeconds();
	}
	else
	{	
		// we add the bundle to the existing node 
		statLastUpdate = Util::getCurrentTimeSeconds();
		numberOfStatMessages++;
		messagesMatrix[string(messageId)] = new DtnStatMessage(messageId,bundleTTL, this);
		messagesMatrix[string(messageId)]->messageNumber = numberOfStatMessages;
		dm = messagesMatrix[string(messageId)];
		dm->updated = Util::getCurrentTimeSeconds();
	}
	

		
	int nn = this->getNumberOfStatNodesForABundle(message);
	//fprintf(stdout, "Number of nodes: %i Message id %s message %s\n\n", nn, messageId, message);
	for(int i=0; i < nn ;i++)
	{
		string tbid;
		this->getNodeFromStat(message,i+1, tbid);
		string bId;
		this->getNodeIdFromStat((char*)tbid.c_str(), bId);
		//fprintf(stdout, "	Number %i Node: %s Node Id %s Node Del %i\n",i+1, tbid.c_str(), bId.c_str(), this->get_bundle_del(((char*)tbid.c_str())));
		this->addStatNode((char*)bId.c_str(), this->getNodeLm((char*)tbid.c_str()));

		// Get the stat version
		int statVersion = this->getNodeStatVersion((char*)tbid.c_str());
		// Get miStartIndex
		int miIndex = this->getMiIndexFromStat((char*)tbid.c_str());

		map<int, int> tmpBitMap;
		this->getNodeBitMap(tbid, tmpBitMap);
		dm->addNode(bId, tmpBitMap, this->getNodeLm((char*)tbid.c_str()), statVersion, miIndex);
	}
}

void StatisticsManager::getNodeBitMap(string & node, map<int, int> & m)
{	
try
{
	//fprintf(stdout, "Node: %s\n", node.c_str());
	size_t pos1 = node.find('[');
	size_t pos2 = node.find(']');
	string table = node.substr(pos1 + 1, pos2 - 1);
	pos1 = table.find(',');
	int i = 0;
	while(pos1 != std::string::npos)
	{
		string rs = table.substr(0,pos1);
		
		m[i] = atoi(rs.c_str());
		i++;
		table = table.substr(pos1 +1);
		pos1 = table.find(',');
	}
	m[i] = atoi(table.c_str());
	
}
catch(exception e)
{
	fprintf(stderr, "Exception Line %i: %s\n", __LINE__, e.what());
	exit(1);
}
}

void StatisticsManager::updateMessage(string& message,string& message_id, DtnStatMessage* dm)
{
	int nn = this->getNumberOfStatNodesForABundle((char*)message.c_str());
	//	fprintf(stdout, "Updating message: %s\n Number of Bundles %i\n",message.c_str(), nn); 

	//Bunlde TTL
	double ttl=this->getBundleTtl((char*)message.c_str());
	dm->ttl = ttl;	

	for(int i = 0; i < nn; i++)
	{
		string node, nodeId;

		//All the Bundle
		this->getNodeFromStat((char*)message.c_str(), i+1, node);

		// His ID
		this->getNodeIdFromStat((char *)node.c_str(), nodeId);

		int version = getNodeStatVersion((char *)node.c_str());
		int miIndex = getMiIndexFromStat((char*)node.c_str());
 
		map<int, int> tmpBitMap;	
		this->getNodeBitMap(node, tmpBitMap);

		dm->addNode(nodeId, tmpBitMap, this->getNodeLm((char *)node.c_str()), version, miIndex);
	}
}



double StatisticsManager::getAvgNumberOfCopies(double lt)
{
	int binIndex = convertElapsedTimeToBinIndex(lt);
	if(messagesMatrix.empty()) return 1;
	int totalNumberOfCopies = 0;
	int numberOfMessages = 0;
	for(map<string, DtnStatMessage *>::iterator iter = messagesMatrix.begin(); iter!=messagesMatrix.end();iter++)
	{
		if((iter->second)->isValid(binIndex) )
		{	
			if((iter->second)->getNumberOfCopiesAt(binIndex) > 1)	
			{
				totalNumberOfCopies += (iter->second)->getNumberOfCopiesAt(binIndex);
				numberOfMessages++;
			}
			//fprintf(stdout, "%i, ",(iter->second)->getNumberOfCopiesAt(binIndex));
		}
	}
	//fprintf(stdout, "Avg totalNumberOfCopies: %i numberOfMessages: %i\n",totalNumberOfCopies, numberOfMessages);
	if(numberOfMessages == 0) return 1;
	//fprintf(stdout," avg = %f\n",(double)totalNumberOfCopies / (double)numberOfMessages);
	return ((double)totalNumberOfCopies / (double)numberOfMessages);

}

double StatisticsManager::getAvgNumberOfCopies(int binIndex)
{
	int totalNumberOfCopies = 0;
	int numberOfMessages = 0;
	for(map<string, DtnStatMessage *>::iterator iter = messagesMatrix.begin(); iter!=messagesMatrix.end();iter++)
	{
		if((iter->second)->isValid(binIndex))
		{	
			if((iter->second)->getNumberOfCopiesAt(binIndex) >1)
			{
				totalNumberOfCopies += (iter->second)->getNumberOfCopiesAt(binIndex);
				numberOfMessages ++;
			}
		}
	}
	if(numberOfMessages == 0) return 1;
	//fprintf(stdout, "Avg totalNumberOfCopies: %i numberOfMessages: %i",totalNumberOfCopies, numberOfMessages);
	return ((double)totalNumberOfCopies / (double)numberOfMessages);

}


double StatisticsManager::getAvgNumberOfStatNodesThatHaveSeenIt(int binIndex)
{
	int totalNumberOfNodes = 0;
	int numberOfMessages = 0;
	for(map<string, DtnStatMessage *>::iterator iter = messagesMatrix.begin(); iter!=messagesMatrix.end();iter++)
	{
		if((iter->second)->isValid(binIndex))
		{	
			if((iter->second)->getNumberOfNodesThatHaveSeeniT(binIndex)>1)
			{
				totalNumberOfNodes += (iter->second)->getNumberOfNodesThatHaveSeeniT(binIndex);
				numberOfMessages++;
			}
		}
	}
	//fprintf(stdout, "avg number of mi, messages: %i total: %i\n", numberOfMessages, totalNumberOfNodes);
	if(numberOfMessages == 0) return 1;
	return ((double)totalNumberOfNodes / (double)numberOfMessages);

}

double StatisticsManager::getAvgNumberOfStatNodesThatHaveSeenIt(double lt)
{
	int binIndex = convertElapsedTimeToBinIndex(lt);
	int totalNumberOfNodes = 0;
	int numberOfMessages = 0;
	for(map<string, DtnStatMessage *>::iterator iter = messagesMatrix.begin(); iter!=messagesMatrix.end();iter++)
	{
		if((iter->second)->isValid(binIndex))
		{	
			if((iter->second)->getNumberOfNodesThatHaveSeeniT(binIndex)>1)
			{
				totalNumberOfNodes += (iter->second)->getNumberOfNodesThatHaveSeeniT(binIndex);
				numberOfMessages++;
			}
		}
	}
	//fprintf(stdout, "avg number of mi 2 , messages: %i total: %i\n", numberOfMessages, totalNumberOfNodes);
	if(numberOfMessages == 0) return 1;
	return ((double)totalNumberOfNodes / (double)numberOfMessages);

}

double StatisticsManager::getAvgDrAt(double lt)
{
	int binIndex = convertElapsedTimeToBinIndex(lt);
	int numberOfMessages =  0;
	double totalDr = 0;
	for(map<string, DtnStatMessage *>::iterator iter = messagesMatrix.begin(); iter!=messagesMatrix.end();iter++)
	{
		if((iter->second)->isValid(binIndex))
		{
		
			totalDr += (iter->second)->getAvgDrAt(binIndex);
			numberOfMessages++;
		}
	}
	if(numberOfMessages == 0) {return 0;}
	return ((double)totalDr / (double)numberOfMessages);

}

double StatisticsManager::getAvgDrAt(int binIndex)
{

	int numberOfMessages =  0;
	double totalDr = 0;
	for(map<string, DtnStatMessage *>::iterator iter = messagesMatrix.begin(); iter!=messagesMatrix.end();iter++)
	{
		if((iter->second)->isValid(binIndex))
		{
			totalDr += (iter->second)->getAvgDrAt(binIndex);
			numberOfMessages++;
		}
	}
	if(numberOfMessages == 0) {return 0;}
	return ((double)totalDr / (double)numberOfMessages);

}

double StatisticsManager::getAvgDdAt(double lt)	
{
	int binIndex = convertElapsedTimeToBinIndex(lt);
	int numberOfMessages = 0;
	double totalDd = 0;
	for(map<string, DtnStatMessage *>::iterator iter = messagesMatrix.begin(); iter!=messagesMatrix.end();iter++)
	{
		if((iter->second)->isValid(binIndex))
		{
			totalDd += (iter->second)->getAvgDdAt(binIndex);
			numberOfMessages++;
		}	
	}
	if(numberOfMessages == 0) {return 0;}
	return ((double)totalDd / (double)numberOfMessages);

}

double StatisticsManager::getAvgDdAt(int binIndex)	
{

	int numberOfMessages = 0;
	double totalDd = 0;
	for(map<string, DtnStatMessage *>::iterator iter = messagesMatrix.begin(); iter!=messagesMatrix.end();iter++)
	{
		if((iter->second)->isValid(binIndex))
		{
			totalDd += (iter->second)->getAvgDdAt(binIndex);
			numberOfMessages++;
		}
	}
	if(numberOfMessages == 0) {return 0;}
	return ((double)totalDd / (double)numberOfMessages);

}

/** Return the number f copies from the stat axe
 */

void StatisticsManager::getStatFromAxe(double et,char *bid,double *ni,double *mi,double *dd_m,double *dr_m)
{
	//ShowAllMessagesStatistics();
	//ShowAvgStatistics();
	*mi = 0;
	*ni = 0;
	*dd_m = 0;
	*dr_m = 0;
	if(numberOfStatMessages == 0) return;
	/*
	ResultsThreadParam *rtp1 = (ResultsThreadParam*)malloc(sizeof(ResultsThreadParam));
	rtp1->workToDo = 1;
	rtp1->sm_ = this;
	rtp1->et = et;
	pthread_t niThread;

	ResultsThreadParam *rtp2 = (ResultsThreadParam*)malloc(sizeof(ResultsThreadParam));
	rtp2->workToDo = 2;
	rtp2->sm_ = this;
	rtp2->et = et;
	pthread_t miThread;

	ResultsThreadParam *rtp3 = (ResultsThreadParam*)malloc(sizeof(ResultsThreadParam));
	rtp3->workToDo = 3;
	rtp3->sm_ = this;
	rtp3->et = et;
	pthread_t drThread;

	ResultsThreadParam *rtp4 = (ResultsThreadParam*)malloc(sizeof(ResultsThreadParam));
	rtp4->workToDo = 4;
	rtp4->sm_ = this;
	rtp4->et = et;
	pthread_t ddThread;

	if (pthread_create (&niThread,NULL, GetResultsThreadProcess, (void *)rtp1) < 0) 
	{
		fprintf(stderr,"pthread_create 1 error.");
	}

	if (pthread_create (&miThread,NULL, GetResultsThreadProcess, (void *)rtp2) < 0) 
	{
		fprintf(stderr,"pthread_create 2 error.");
	}

	if (pthread_create (&drThread,NULL, GetResultsThreadProcess, (void *)rtp3) < 0) 
	{
		fprintf(stderr,"pthread_create 3 error.");
	}

	if (pthread_create (&ddThread,NULL, GetResultsThreadProcess, (void *)rtp4) < 0) 
	{
		fprintf(stderr,"pthread_create 4 error.");
	}

	void *status;
	int r = 0;
	if(r = pthread_join(ddThread, &status))
	{
		fprintf(stderr, "Error joining dd thread: %i\n", r);
		free(rtp4);
		exit(1);
	}else
	{
		*dd_m = rtp4->floatRes;
		free(rtp4);	
	}

	r = 0;
	if(r = pthread_join(drThread, &status))
	{
		fprintf(stderr, "Error joining dr thread: %i\n", r);
		free(rtp3);
		exit(1);
	}else
	{
		*dr_m = rtp3->floatRes;	
		free(rtp3);
	}

 	r = 0;
	if(r = pthread_join(miThread, &status))
	{
		fprintf(stderr, "Error joining mi thread: %i\n", r);
		free(rtp2);
		exit(1);
	}else
	{
		*mi = rtp2->intRes;
		free(rtp2);	
	}
 	r = 0;
	if(r = pthread_join(niThread, &status))
	{
		fprintf(stderr, "Error joining ni thread: %i\n", r);
		free(rtp1);
		exit(1);
	}else
	{
		*ni = rtp1->intRes;
		free(rtp1);	
	}

	
	*/	
	*ni = getAvgNumberOfCopies(et);
	*mi = getAvgNumberOfStatNodesThatHaveSeenIt(et);
	*dr_m = getAvgDrAt(et);
	*dd_m = getAvgDdAt(et);
	

	//fprintf(stdout, "ni: %f mi: %f dr: %f dd: %f\n", *ni, *mi, *dr_m, *dd_m);	
	
	// clean stat messages to delete 
	/*
	map<string, DtnStatMessage *>::iterator iter = messagesMatrix.begin();
	if(someThingToClean)
	{
		while(iter != messagesMatrix.end())
		{
			if((iter->second)->toDelete)
			{
				fprintf(stdout, "#################################     deleting a message \n");
				delete iter->second;
				messagesMatrix.erase(iter);
				statLastUpdate = Util::getCurrentTimeSeconds();
			
			}
			iter++;
		}
		someThingToClean = false;
	}
	*/
}	


void StatisticsManager::getStatFromAxe(int binIndex, double *ni, double *mi, double *dd_m, double *dr_m, double *et )
{
	//ShowAllMessagesStatistics();
	//ShowAvgStatistics();
	*ni = getAvgNumberOfCopies(binIndex);
	*mi = getAvgNumberOfStatNodesThatHaveSeenIt(binIndex);
	*dr_m = getAvgDrAt(binIndex);
	*dd_m = getAvgDdAt(binIndex);
	*et = convertBinINdexToElapsedTime(binIndex);
}

void StatisticsManager::showAllMessagesStatistics()
{	
	for(map<string, DtnStatMessage *>::iterator iter = messagesMatrix.begin(); iter != messagesMatrix.end(); iter++)
	{
		(iter->second)->showStatistics();
	}
}

void StatisticsManager::showAvgStatistics()
{
	for(int i =0;i<axeLength;i++)
	{
		fprintf(stdout, "ni %f mi %f avg_dd %f avg_dr %f\n",getAvgNumberOfCopies(i),getAvgNumberOfStatNodesThatHaveSeenIt(i), getAvgDdAt(i), getAvgDrAt(i));
	}

	fprintf(stdout, "New Stat -------------------------------------------------\n");
}

void StatisticsManager::logAvgStatistics()
{
	for(int i =0;i<axeLength;i++)
	{
		fprintf(stdout, "ni %f mi %f avg_dd %f avg_dr %f\n",getAvgNumberOfCopies(i),getAvgNumberOfStatNodesThatHaveSeenIt(i), getAvgDdAt(i), getAvgDrAt(i));
	}

}


int StatisticsManager::convertElapsedTimeToBinIndex(double et)
{
 	if(et > (axeSubdivision * axeLength))
		return axeLength;
	for(int i = 0; i < axeLength ; i++)
	{	
		if(et == 0.0) 
		{
			return 0;
		}
	
		if(statAxe[i].minLt < et && et <= statAxe[i].maxLt)
		{
			return i;			
		}
	}
	fprintf(stdout, "invalid et ! ET: %f Line: %i file %s\n",et, __LINE__, __FILE__);
	exit(1);
}	

double StatisticsManager::convertBinINdexToElapsedTime(int binIndex)
{
	if(binIndex == 0) return 0;
	
	for(int i = 0; i < axeLength; i++)
	{	
		if(i == binIndex) 
		{
			return statAxe[i].minLt + 1;
		}
	}

	fprintf(stdout, "invalid binIndex: %i Line: %i file %s\n",binIndex, __LINE__, __FILE__);
	exit(1);
	
}



int StatisticsManager::getNumberOfInvalidMessages()
{
	int n = 0;
	if(messagesMatrix.empty()) return 0;
	for(map<string, DtnStatMessage *>::iterator iter = messagesMatrix.begin(); iter!= messagesMatrix.end();iter++)
	{
		if(!isMessageValid(string((iter->second)->getBstatId())))
			n++;
	}
	return n;
}

int StatisticsManager::getNumberOfValidMessages(map<string, DtnStatMessage *>::iterator & iterOldest)
{
	int n = 0;
	int oldest = 9999;
	if(messagesMatrix.empty()) return 0;
	for(map<string, DtnStatMessage *>::iterator iter = messagesMatrix.begin(); iter!= messagesMatrix.end();iter++)
	{
		if((iter->second)->messageNumber < oldest)
		{
			iterOldest = iter;
			oldest =  (iter->second)->messageNumber;
		}
		if((iter->second)->isValid(axeLength))
			n++;
	}
	return n;
}

bool StatisticsManager::isMessageValid(string msgId)
{
	DtnStatMessage *bs = this->isBundleHere((char*)msgId.c_str());
	if(bs == NULL)
		return false;
	else return bs->isValid(axeLength);
}

void StatisticsManager::clearMessages()
{
	while(!messagesMatrix.empty())
	{
		map<string, DtnStatMessage *>::iterator iter = messagesMatrix.begin();
		delete iter->second;
		messagesMatrix.erase(iter);
	}
	messagesMatrix.clear();
	numberOfStatMessages = 0;
	statLastUpdate = Util::getCurrentTimeSeconds();
}


double StatisticsManager::getNodeMeetingTime(string nodeId)
{
	map<string, double>::iterator iter = nodesMatrix.find(nodeId);
	if(iter != nodesMatrix.end())
	{
		return iter->second;
	}
	else return 0;	
}	


void StatisticsManager::deleteMessage(map<string, DtnStatMessage *>::iterator & iterOldest)
{
	delete iterOldest->second; 
	messagesMatrix.erase(iterOldest);
}


double StatisticsManager::getAverageNetworkMeetingTime()
{
	if(useBinSizeForAvgMeeting)
		return (double)axeSubdivision;
	else
	{
		if(this->numberOfMeeting == 0) return 1;
		else return this->totalMeetingSamples/this->numberOfMeeting;
	}
}


int StatisticsManager::getApproximatedNumberOfNodes()
{
	if(useOnlineAproximatedNumberOfNodes)
	{
		return this->numberOfStatNodes;
	}else
	{
		return this->numberOfNodesWithinTheNetwork;
	}
}

bool StatisticsManager::isStatNodeAvailable(std::string nodeId)
{
	std::map<std::string, double >::iterator iter = nodesMatrix.find(nodeId);
	if(iter != nodesMatrix.end())
		return true;
	else return false;
}

string StatisticsManager::getListOfAvailableStatNodes()
{
	string tmp;
	if(nodesMatrix.empty())
		return string("");
	int i = 0;
	for(map<string, double>::iterator iter = nodesMatrix.begin(); iter != nodesMatrix.end(); iter++)
	{
		if(i > 0)
			tmp.append("#");
		i++;
		tmp.append(iter->first);
	}

	return tmp;
}
