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


#include "Util.h"
#include <cassert>

using namespace std;



bool Util::stringToBool(string s)
{
	assert(!s.empty());

	if(s.compare("true") == 0)
		return true;
	else if(s.compare("false") == 0)
		return false;

	//Default
	return false;
}

time_t Util::getCurrentTimeSeconds()
{
	return time(NULL);
}

void Util::stripSpace(string &str)
{
	for (unsigned int i = 0;i < str.length();i++)
	{
		if (str[i]==' ')
		{
			str.erase(i,1);
			i--;
		}
	}
}
