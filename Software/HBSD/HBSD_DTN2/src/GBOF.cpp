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


#include "GBOF.h"
#include "XMLTree.h"
#include "Util.h"
#include "HBSD.h"
#include "Bundle.h"

using namespace std;

string GBOF::xmlFromBundle(Bundle* bundle) 
{
	assert(bundle != NULL);
	string xml = "<gbof_id creation_ts=\"" + Util::to_string(bundle->creationTimestamp);
	xml += string("\" frag_length=\"") + Util::to_string(bundle->fragLength) + string("\" frag_offset=\"") + Util::to_string(bundle->fragOffset);
	if (bundle->isFragment) 
	{
		xml += "\" is_fragment=\"true\">";
	} else 
	{
		xml += "\" is_fragment=\"false\">";
	}
	xml += "<source uri=\"" + bundle->sourceURI + "\"/></gbof_id>";
	return xml;
}


string GBOF::xmlFromParms(long timestamp, int len, int offset, bool frag, string uri)
{
	string xml = "<gbof_id creation_ts=\"" + Util::to_string(timestamp);
	xml += "\" frag_length=\"" + Util::to_string(len) +
		"\" frag_offset=\"" + Util::to_string(offset);
	if (frag) 
	{
		xml += "\" is_fragment=\"true\">";
	} else 
	{
		xml += "\" is_fragment=\"false\">";
	}
	xml += "<source uri=\"" + uri + "\"/></gbof_id>";
	return xml;
}

string GBOF::keyFromBundleObject(Bundle* bundle)
{
	assert(bundle != NULL);
	return makeKey(bundle->creationTimestamp, bundle->fragLength, bundle->fragOffset, bundle->isFragment, bundle->sourceURI);
}



string GBOF::keyFromXML(XMLTree* element) 
{
	assert(element != NULL);
	try 
	{
		XMLTree * el = element->getChildElementRequired(string("source"));
		assert(el != NULL);
		bool tmp; 

		if(element->getAttrRequired(string("is_fragment")).compare("true") == 0)
			tmp = true;
		else tmp = false;

		return makeKey(element->getAttrRequired(string("creation_ts")), element->getAttrRequired(string("frag_length")), element->getAttrRequired(string("frag_offset")), tmp, el->getAttrRequired(string("uri")));
	} 
	catch (exception &e) 
	{
		if(HBSD::log->enabled(Logging::ERROR))
		{
			HBSD::log->error(string("Exception occurred within GBOF::keyFromXML: ") + string(e.what()));
		}
		return NULL;
	}
}



string GBOF::eidFromURI(string uri) 
{
	if (uri.find_first_of("dtn://") == string::npos) 
	{
		if(HBSD::log->enabled(Logging::ERROR))
		{
			HBSD::log->error(string("Invalid URI within GBOF::eidFromURI"));
		}
		return NULL;
	}
	int cnt = 0;
	int sz = uri.length();
	int indx = 0;
	for (indx=0; indx<sz; indx++) 
	{
		if (uri.at(indx) == '/') 
		{
			if (++cnt == 3) 
			{
				break;
			}
		}
	}
	return uri.substr(0, indx);
}



std::string GBOF::keyFromBundle(Bundle* bundle)
{
	if(bundle->gbofKey.empty())
		return keyFromBundleObject(bundle);
	else return bundle->gbofKey;
}
