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

#ifndef _SAIEXECUTIONCONTEXT_H_ABSTRACT_
#define _SAIEXECUTIONCONTEXT_H_ABSTRACT_

#include "SAIGlobals.h"

#define		NO_SCENE 0				//There is no scene defined. This value should never be seen directly.
#define		SCRIPTED_ENCODING 1		//The scene was created dynamically through scripting calls.
#define		ASCII_ENCODING 2		//The scene described an original VRML 1.0 encoding.
#define		CLASSIC_VRML_ENCODING 3	//The scene is encoded using the Classic VRML encoding. The scene may be VRML 97 or this specification.
#define		XML_ENCODING 4			//The scene is encoded using the XML file format.
#define		BINARY_ENCODING 5		//The scene was encoded using the binary format specified in ISO/IEC 19776-3.  
									//It shall be an error to use this value to describe a browser-specific proprietary binary format.
#define		BIFS_ENCODING 6			//The scene was encoded using the MPEG4 BIFS encoding.
#define		LAST_STD_ENCODING = 100	//A definition of the last constant used by the standard encodings. 
									//A browser is permitted to allow other, proprietary encoding mechanisms, 
									//and therefore any constant used to describe that shall use a value greater than this number. 
									//Code using these values shall not expect to be transportable across multiple browser implementations.			

namespace freeWRLSAI_cpp
{
	//forward declarations
	class saiNode;
	class saiProtoDeclaration;
	class saiRoute;

	class saiExecutionContext
	{
	public:
		enum saiContextType
		{
			saiGenericContext = 0,	
			saiSceneContext,			
			//others
			saiUndefinedContext		//for error checking
		};
	
	//GENERAL NOTICE: classes derived from saiExecutionContext should never be contructed with a standard ctor, so ctor/dtor group should be as follows
	//protected:
	//	saiExecutionContext();	//standard ctor is never to be used

	//public:
	//saiExecutionContext should be constructed with at least the pointer to the protected fwl X3D_"something" struct
	//so every derived class should have something along the lines of
	//	saiExecutionContext(void* pProtectedStruct){ m_pProtectedStruct = pProtectedStruct;} 
		
		//not part of W3C specifications, but it seems necessary
		virtual saiContextType getContextType() = 0;	

		virtual const char* getSpecificationVersion() = 0;
		
		virtual int getEncoding() = 0;

		virtual const char* getWorldURL() = 0;	//returns NULL if no url was provided for scene creation
		
		virtual saiNode* getNode(const char* strNodeName, int nAction) = 0;

		virtual saiNode* createNode(const char* strNodeType) = 0;

		virtual saiNode* createProto(const char* strProtoName) = 0;

		virtual saiProtoDeclaration* getProtoDeclaration(const char* strProtoName) = 0;
		
		virtual void protoDeclarationHandling(const char* strProtoName, saiNode* pNode, int nAction) = 0;

		virtual saiProtoDeclaration* getExternProtoDeclaration(const char* strProtoName) = 0;

		virtual void externProtoDeclarationHandling(const char* strProtoName, saiNode* pNode, int nAction) = 0;

		virtual std::vector<saiNode*>* getRootNodes() = 0;

		virtual std::vector<saiRoute*>* getRoutes() = 0;

		virtual void dispose() = 0;
		
		virtual saiProfileDeclaration*	getProfile() = 0;	
		virtual std::map<std::string, saiComponent*>* getComponents() = 0; 

		//WAITING FOR DEFINITION
		//virtual void namedNodeHandling... I believe it should be implemented with four different method signatures like Java Language Bindings does
		//virtual void dynamicRouteHandling... same as above	
	};
};

#endif //_SAIEXECUTIONCONTEXT_H_ABSTRACT_