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

#ifndef GBOF_H
#define GBOF_H

#include <string>
#include <exception>
#include "Util.h"

class XMLTree;
class Bundle;

class GBOF 
{
	
public:
	/**
	 * Generates a "gbofIdType" XML element.
	 * 
	 * @param bundle Bundle object.
	 * @return XML element.
	 */
	static std::string xmlFromBundle(Bundle* bundle);
	
	
	/**
	 * Generates a "gbofIdType" XML element.
	 * 
	 * @param timestamp Creation time stamp.
	 * @param len Length of bundle.
	 * @param offset Bundle offset.
	 * @param frag Whether it is a fragment.
	 * @param uri The source URI.
	 * @return The XML element.
	 */
	static std::string xmlFromParms(long timestamp, int len, int offset, bool frag, std::string uri);

	/**
	 * This function returns a string derived from the values that 
	 * compose a GBOF-ID. This string is then used as a key to locate
	 * the bundle.
	 * 
	 * @param bundle Bundle object.
	 * @return The generated key.
	 */
	static std::string keyFromBundleObject(Bundle* bundle);
	
	/**
	 * Returns the GBOF key associated with a bundle. This value is stored
	 * in the bundle when the bundle is created so we do not need to 
	 * derive it. The caller could simply read the value in the Bundle
	 * object if it wanted to.
	 * 
	 * @param bundle The Bundle object.
	 * @return The hash key.
	 */
	static std::string keyFromBundle(Bundle* bundle);
	
	/**
	 * Given a "gbofIdType," generate the key used to locate the bundle in the
	 * bundles hash.
	 * 
	 * @param element The "gbof_id" XML element.
	 * @return The generated key.
	 */
	static std::string keyFromXML(XMLTree* element);

	/**
	 * Returns a hash key given the parsed values from a "gbofIdType"
	 * element.
	 * .
	 * @param timestamp Creation time stamp.
	 * @param len Length of bundle.
	 * @param offset Bundle offset.
	 * @param frag Whether it is a fragment.
	 * @param uri The source URI.
	 * @return The key.
	 */
	static std::string keyFromParms(std::string timestamp, std::string len, std::string offset, std::string frag, std::string uri) 
	{
		bool tmp;
		if(frag.compare("true") == 0)
			tmp = true;
		else tmp = false;

		return makeKey(timestamp, len, offset, tmp, uri);
	}
	
	/**
	 * Returns a hash key given the parsed values from a "gbofIdType"
	 * element.
	 * .
	 * @param timestamp Creation time stamp.
	 * @param len Length of bundle.
	 * @param offset Bundle offset.
	 * @param frag Whether it is a fragment.
	 * @param uri The source URI.
	 * @return The key.
	 */
	static std::string keyFromParms(std::string timestamp, int len, int offset, bool frag, std::string uri)
	{
		return makeKey(timestamp, len, offset, frag, uri);
	}
	
	
	
	/**
	 * Simplistic parser to return the EID from a URI.
	 * 
	 * @param The URI to be parsed.
	 * @return The EID.
	 */
	static std::string eidFromURI(std::string uri);

	/**
	 * Called to calculate the expiration time of a bundle.
	 * 
	 * @param timestamp GBOF creation time.
	 * @param deltaSeconds Seconds after creation the bundle expires.
	 * @return GMT epoch milliseconds the bundle is to expire (i.e. something
	 *    that can be compared against System.currentTimeMillis()).
	 */
	static long calculateExpiration(long long int timestamp, long deltaSeconds)
	{
		// The seconds since January 1, 2000 are in the high order 32 bits
		// of the creation time stamp found in the GBOF. Since epoch time
		// is since January 1, 1970 we add in the delta.
		long epochSeconds = (timestamp >> 32) + DELTA1970_SECONDS;
		// Now add the expiration seconds and convert it to milliseconds.
		return ((epochSeconds + deltaSeconds) * 1000) + 999;
	}
	
	/**
	 * Calculates the creation time of a bundle given its GBOF time stamp.
	 * 
	 * @param timestamp GBOF creation time.
	 * @return GMT epoch seconds of when the bundle was created.
	 */
	static long creationSeconds(long long int timestamp)
	{
		return (timestamp >> 32) + DELTA1970_SECONDS;
	}

private:

/**
	 * Generates the unique key given the required values. This is essentially
	 * a more compact version of the XML.
	 * 
	 * @param ts Creation time stamp.
	 * @param len Length of bundle.
	 * @param offset Bundle offset.
	 * @param frag Whether it is a fragment.
	 * @param uri The source URI.
	 * @return The key.
	 */
	static std::string makeKey( std::string ts,  std::string len,  std::string offset,  bool frag,  std::string uri) 
	{
		std::string binaryFrag;
		if (frag) 
		{
			binaryFrag.assign(":1");
		} else 
		{
			binaryFrag.assign(":0");
		}
		return ts + std::string("[") + uri + std::string("]") + offset + std::string("+") + len + binaryFrag;
		
	}

	/**
	 * Overloaded method for creating a bundle key.
	 * 
	 * @param ts Creation time stamp.
	 * @param len Length of bundle.
	 * @param offset Bundle offset.
	 * @param frag Whether it is a fragment.
	 * @param uri The source URI.
	 * @return The key.
	 */

	static std::string makeKey(std::string ts, int len, int offset, bool frag, std::string uri)
	{
		return makeKey(ts, Util::to_string(len), Util::to_string(offset), frag, uri);
	}
	
	// Seconds between January 1, 1970 (standard epoch) and January 1, 2000
	// (DTN epoch).
	const static int DELTA1970_SECONDS = 946684800;

};

#endif
