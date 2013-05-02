/****************************************************************************
    This file is part of the FreeWRL/FreeX3D Distribution.

    Copyright 2009 CRC Canada. (http://www.crc.gc.ca)

    FreeWRL/FreeX3D is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FreeWRL/FreeX3D is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FreeWRL/FreeX3D.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/


#ifndef _SAINODE_H_ABSTRACT_
#define _SAINODE_H_ABSTRACT_

#include "SAIGlobals.h"

namespace freeWRLSAI_cpp
{
	//forward declarations
	class saiField;

	class saiNode
	{
	public:
		virtual const char* getTypeName() = 0;

		virtual const char* getType() = 0;
		
		virtual saiField* getField(const char* strFieldName) = 0;
		
		virtual std::vector<saiField*>* getFieldDefinitions(const char* strNodeType) = 0;

		virtual void dispose() = 0;
	};

};

#endif //_SAINODE_H_ABSTRACT_