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

#include "ConfigFile.h"
#include "Util.h"
#include <exception>
#include <iostream>
#include <fstream>
#include "HBSD.h"
using namespace std;


ConfigFile::ConfigFile(string fn) 
{
	fileName = fn;
}
	
ConfigFile::~ConfigFile()
{
	mapConfig.clear();
}

bool ConfigFile::parse() 
{
	if (fileName.empty()) 
	{
		return false;
	}
	bool stat = true;
	ifstream cFile;
	try 
	{
	
		cFile.open(fileName.c_str(), ios::in);
		if(!cFile.is_open())
		{
			cerr<<"Unable to open the Config File."<<endl;
			return false;
		}

		char buf[1024];
		memset(buf,'0', 1024);	
		// For each line in the file ...
		while (!cFile.eof()) 
		{
			cFile.getline(buf, 1024, '\n');
			string line(buf);
			memset(buf,'0', 1024);	
			Util::stripSpace(line);
			
			if(line.length() <= 2)
			{
				// an empty line
				continue;
			}
			
			if (line.at(0) == '#' || (line.at(0) == ' ' && line.size() == 1)) 
			{
				// Comment line.
				continue;
			}
			
			
			unsigned int delim = line.find_first_of('=');
			
			if (delim <= 0) 
			{
				// No '=' found.
				cerr<<"delim: "<<delim<<" no '=' found in line: "<<line<<endl;
				stat = false;
				break;
			}
			
			string key = line.substr(0, delim);
			Util::stripSpace(key);
			
			// Value can be zero length.
			string val = "";

			if (++delim < line.length()) 
			{
				val = line.substr(delim);
				Util::stripSpace(val);
				if (val.length() >= 2) 
				{
					// Is value quoted?
					char cFirst = val.at(0);
					char cLast = val.at(val.length() - 1);
					if (((cFirst == '\'') || (cFirst == '"')) && (cFirst == cLast)) 
					{
						val = val.substr(1, val.length()-1);
					}
				}
			}
			// Everything is ok, store the key/value pair.
			mapConfig[key] = val;
			cout<<"Config Key: " << key << " value: " << val<<endl;
		}
		if(mapConfig.empty())
		{
			cerr<<"Error occured while loading the config file."<<endl;
			cFile.close();
			return false;
		}
	} 

	catch(exception &e) 
	{
		stat = false;
		cFile.close();
		cerr <<"Error occured: "<<e.what()<<endl;
	}
	
	cFile.close();
	return stat;
	
}


bool ConfigFile::haveFile() 
{
	bool test = false;
	ifstream cFile(fileName.c_str(), ios::in);
	test = cFile.is_open();
	cFile.close();
	return test;
}

string ConfigFile::getstring(string key, string dflt) 
{
	map<string, string>::iterator iter = mapConfig.find(key);

	string val = mapConfig[key];
	if (iter  == mapConfig.end()) 
	{
		return dflt;
	}
	return val;
}

bool ConfigFile::getBoolean(string key, bool dflt) 
{
	map<string, string>::iterator iter = mapConfig.find(key);

	if (iter == mapConfig.end()) 
	{
		return dflt;
	}

	string val = mapConfig[key];
	return Util::stringToBool(val);
}

int ConfigFile::getInt(string key, int dflt) 
{
	map<string, string>::iterator iter = mapConfig.find(key);
	string val = mapConfig[key];

	if (iter == mapConfig.end()) 
	{
		return dflt;
	}

	try 
	{
		return atoi(val.c_str());
	} 

	catch(exception &e) 
	{
		return dflt;
	}
}

