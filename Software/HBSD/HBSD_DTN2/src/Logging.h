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


#ifndef LOGGING_H
#define LOGGING_H

#include <string>



class Logging 
{

public:

	/**
	 * Logging levels to be supported.
	 */
	const static  int ALL     = -1;
	const static  int TRACE   = 0;
	const static  int DEBUG   = 1;
	const static  int INFO    = 2;
	const static  int WARN    = 3;
	const static  int ERROR   = 4;
	const static  int FATAL   = 5;
	const static  int OFF     = 6;
	
	/**
	 * Called once to allow the logging implementation to configure itself.
	 */
	virtual void conf() = 0;
	
	/**
	 * An overload of the above conf() method that allows a configuration
	 * file to be specified.
	 * 
	 * @param confFile The name of the configuration file.
	 */
	virtual void conf(std::string confFile) = 0;
		
	/**
	 * Sets the logging level.
	 * 
	 * @param lev New level.
	 */
	virtual void setLevel(int lev) = 0;
	
	/**
	 * @return Returns the current logging level.
	 */
	virtual int getLevel() = 0;
	
	/**
	 * Indicates whether logging is enabled for the given level.
	 * 
	 * @param lev Level to check.
	 * @return True if enabled, otherwise false.
	 */
	virtual bool enabled(int lev) = 0;
		
	/**
	 * Logging write routines. It up to the implementation as to whether
	 * these routines strictly adhere to logging level.
	 * 
	 * @param message Message to be logged.
	 */
	virtual void trace(std::string message) = 0;
	virtual void debug(std::string message) = 0;
	virtual void info(std::string message) = 0;
	virtual void warn(std::string message) = 0;
	virtual void error(std::string message) = 0;
	virtual void fatal(std::string message) = 0;
};

#endif
