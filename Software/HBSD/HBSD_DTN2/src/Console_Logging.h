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


#ifndef CONSOLE_LOGGING_H
#define CONSOLE_LOGGING_H

#include <string>
#include "Logging.h"


class Console_Logging: public Logging 
{
public:	
	const static int DEFAULT = WARN;

	/**
	 * Constructor: define how the time stamp is to be displayed.
	 */
	Console_Logging();
	
	/**
	 * Nothing to configure.
	 */
	void conf() 
	{	
	}
	
	/**
	 * Still nothing to configure, but the Logging interface insists that
	 * we accept a configuration file.
	 */
	void conf(std::string confFile) 
	{
		
	}
	
	/**
	 * Sets the logging level.
	 * 
	 * @param lev New level.
	 */
	void setLevel(int lev) 
	{
		currentLevel = lev;
	}
	
	/**
	 * @return Returns the current logging level.
	 */
	int getLevel() 
	{
		return currentLevel;
	}
	
	/**
	 * Indicates whether logging is enabled for the given level.
	 * 
	 * @param lev Level to check.
	 * @return True if enabled, otherwise false.
	 */
	bool enabled(int lev) 
	{
		
		return (currentLevel >= lev);
	}
		
	/**
	 * Logging write routines.
	 * 
	 * @param message Message to be logged.
	 */
	
	void trace(std::string message) 
	{
		if (currentLevel >= TRACE) 
		{
			output(message, "TRACE");
		}
	}
	
	void debug(std::string message) 
	{
		if (currentLevel >= DEBUG) 
		{
			output(message, "DEBUG");
		}
	}
	
	void info(std::string message) {
		if (currentLevel >= INFO) {
			output(message, "INFO");
		}
	}
	
	void warn(std::string message) 
	{
		if (currentLevel >= WARN) 
		{
			output(message, "WARN");
		}
	}
	
	void error(std::string message) 
	{
		if (currentLevel >= ERROR) 
		{
			output(message, "ERROR");
		}
	}
	
	void fatal(std::string message) 
	{
		if (currentLevel >= FATAL) 
		{
			output(message, "FATAL");
		}
	}

private:
	
	/**
	 * Outputs a string to stdout along with a time stamp and the name of
	 * the calling thread.
	 * 
	 * @param message std::string to be printed.
	 * @param level Logging level of the message.
	 */
	void output(std::string message, std::string level);

	static int currentLevel;
};

#endif
