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


#ifndef _SAIFIELD_H_ABSTRACT_
#define _SAIFIELD_H_ABSTRACT_

#include "SAIGlobals.h"

namespace freeWRLSAI_cpp
{
	/*---------------------------------------------------------------------------------------
	IMPORTANT NOTICE: since freeWRL EAI uses a X3DNode union data type and other X3D tools could
	use a custom defined data struct, we use a typedefined void pointer as conventional pointer
	for those data struct. Should be care of the implementation to make the right conversions
	----------------------------------------------------------------------------------------*/
	typedef void* saiFieldValuePtr ;

	class saiField
	{
	public:
		enum saiFieldAccess
		{
			initializeOnly = 0,		//first access state
			inputOnly,				//write only
			outputOnly,				//read only
			inputOutput				//read/write
		};

		virtual saiFieldAccess getAccessType() = 0; //should return the protected saiFieldAccess flag

		virtual int getType() = 0; //maps to the FIELDTYPE defines as used by the EAI
		
		virtual const char* getName() = 0;

		virtual void dispose() = 0;
		
		//WAITING FOR DEFINITION		
		virtual const saiFieldValuePtr getValue() = 0; //read the IMPORTANT NOTICE above
		virtual void setValue(const saiFieldValuePtr pValue) = 0; 
		//virtual void registerFieldInterest(const saiListener* pListener) = 0; //saiListener model is not defined yet		
	};
};

#endif //_SAIFIELD_H_ABSTRACT_