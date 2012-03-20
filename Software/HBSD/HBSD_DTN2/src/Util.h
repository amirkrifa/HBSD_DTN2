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


#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <sstream>
#include <time.h>

class Util
{
public:
	Util(){}
	~Util(){}
	template <class T> static std::string to_string ( T t)
	{
		std::stringstream ss;
		ss << t;
		return ss.str();
	}

	static bool stringToBool(std::string s);
	static time_t getCurrentTimeSeconds();
	static void stripSpace(std::string &str);
};

#endif


