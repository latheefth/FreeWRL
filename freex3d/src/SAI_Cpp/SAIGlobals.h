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

#ifndef _SAIGLOBALS_H_
#define _SAIGLOBALS_H_

#include <map>
#include <vector>
#include <string>
#include "SAIexception.h"

namespace freeWRLSAI_cpp
{
	//Parameter container base struct. Implement the saiParameterList specification
	typedef class _SAIParameter
	{
	public:
		_SAIParameter()
		{
			interactor = NULL;
		}

		void* interactor;	//void pointer to the class interacting with the browser useful for mapping who is interacting with who
	} SAIParameter;
		
	//saiProperty base pair
	typedef std::pair<std::string, std::string> saiProperty;

	
	//what follows is actually a draft. 
	class saiComponent
	{
	public:
		virtual const char* getComponentName() = 0; //should return the name of the compontent declaration

		//add here other "get" methods common to all the components.
	};

	class saiProfileDeclaration
	{
	public:
		virtual const char* getProfileName() = 0; //should return the name of the profile declaration

		virtual std::map<std::string, saiComponent*> getComponentDeclaration() = 0; //should return the map of the components declared in this profile
	};


	//saiComponentDeclaration pair
	typedef std::pair<std::string, saiComponent*> saiComponentDeclaration;

	typedef enum _SAIActions
	{
		//saiBrowser::updateControl
		BeginUpdate,
		EndUpdate,
		//saiBrowser::registerBrowserInterest
		AddBrowserInterest,
		RemoveBrowserInterest,
		//saiBrowser::changeViewPoint
		Next,
		Previous,
		First,
		Last,
		//saiExecutionContext::getNode
		DEFNode,
		IMPORTNode,
		EXPORTNode,
		//saiExecutionContext::protoDeclarationHandling
		AddProto,
		UpdateProto,
		RemoveProto,
		//saiExecutionContext::externProtoDeclarationHandling
		AddExternProto,
		UpdateExternProto,
		RemoveExternProto,
		//saiScene::rootNodeHandling
		AddRootNode,
		RemoveRootNode
	} SAIActions;

};

#endif //_SAIGLOBALS_H_