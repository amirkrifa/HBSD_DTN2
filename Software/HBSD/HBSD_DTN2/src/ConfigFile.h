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


#ifndef CONFIG_FILE_H
#define CONFIG_FILE_H

#include <string>
#include <map>


class ConfigFile 
{

public:
	/**
	 * Constructor: Called when there is no configuration file. A
	 * hash std::map is still created so that queries can be made on
	 * an empty hash table..
	 */
	
	ConfigFile() 
	{
	}
	
	/**
	 * Constructor: Defines the configuration file.
	 *  
	 * @param fn File name.
	 */
	ConfigFile(std::string fn);
	~ConfigFile();
	/**
	 * Read the file and parse it.
	 * 
	 * @return Returns true if parsed, false if an error was encountered.
	 */
	bool parse();

	/**
	 * Returns status of whether there is a readable configuration file.
	 * 
	 * @return True if file exists and is readable, else false.
	 */
	bool haveFile();
	/**
	 * Query if a key is defined. Existence is independent of validity.
	 *
	 * @param key Key to look for.
	 * @return True if it is defined, otherwise false.
	 */
	bool exists(std::string key) 
	{
		return mapConfig.find(key) != mapConfig.end();
	}
	
	/**
	 * Return value as a std::string, bool or integer. If key does not
	 * exist or cannot be formatted, return the specified default value.
	 * 
	 * @param key Key to search for
	 * @param dflt Default value to return if key not found.
	 * @return Value, or default.
	 */
	
	std::string getstring(std::string key, std::string dflt);
	bool getBoolean(std::string key, bool dflt);
	int getInt(std::string key, int dflt);

private:

	std::string fileName;
	std::map<std::string,std::string> mapConfig;

};


#endif
