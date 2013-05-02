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

/****************************************************************************
	IMPORTANT NOTICE:
	A class derived from saiScene should ALWAYS derive also from saiExecutionContext
	ALWAYS!
****************************************************************************/


#ifndef _SAISCENE_H_ABSTRACT_
#define _SAISCENE_H_ABSTRACT_

#include "SAIGlobals.h"

namespace freeWRLSAI_cpp
{
	class saiScene
	{
	public:
		virtual const char* getMetaData(const char* strKey) = 0;				//should return NULL if metadata not found
		virtual void setMetaData(const char* strKey, const char* strMetadata) = 0;	//if strMetadata = NULL, metadata pointed by strKey should be removed
		virtual void rootNodeHandling(const saiNode* pTargetNode, int nAction) = 0;  

		//not part of the standard but copied from Java Language Bindings
		virtual void AddRootNode(const saiNode* pNodeToAdd) = 0;		//calls rootNodeHandling(pNodeToAdd, AddRootNode)
		virtual void RemoveRootNode(const saiNode* pNodeToRemove) = 0;	//calls rootNodeHandling(pNodeToRemove, RemoveRootNode)
		
		//WAITING FOR DEFINITION		
		//virtual void namedNodeHandling... I believe it should be implemented with four different method signatures like Java Language Bindings does

	};
};


#endif //_SAISCENE_H_ABSTRACT_