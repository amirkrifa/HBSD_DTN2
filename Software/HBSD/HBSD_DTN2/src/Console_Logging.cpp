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


#include "Console_Logging.h"
#include <time.h>
#include <iostream>

using namespace std;

int Console_Logging::currentLevel;

Console_Logging::Console_Logging()
{
	currentLevel = DEFAULT;
}

void Console_Logging::output(string message, string level) 
{
	time_t currentRawTime;

	time(&currentRawTime);

	struct tm* timeInfo;

	timeInfo = localtime(&currentRawTime);
	string localTime (asctime(timeInfo));
	cout <<  level + ": "<< message << endl;
}

