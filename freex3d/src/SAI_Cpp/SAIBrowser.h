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

#ifndef _SAIBROWSER_H_ABSTRACT_
#define _SAIBROWSER_H_ABSTRACT_

#include "SAIGlobals.h"

namespace freeWRLSAI_cpp
{
	//forward declarations
	class saiScene;
	class saiNode;
	class saiExecutionContext;

	class saiBrowser
	{
	//GENERAL NOTICE: classes derived from saiBrowser should never be contructed with a standard ctor, so ctor/dtor group should be as follows
	//protected: 	
	//	//standard ctor is never to be used;
	//	saiBrowser(){};
	//public:
	//	saiBrowser(const SAIParameter* pParams) = 0;	//null pointer should be allowed but always managed
	//	virtual ~saiBrowser(){};
		
	public:
		//browser retrieval / creation
		virtual saiBrowser* getBrowser(const SAIParameter* pParams) = 0;		//null pointer should be allowed but always managed
		virtual saiBrowser* createBrowser(const SAIParameter* pParams, std::map<std::string, std::string>* pProperties) = 0;

		//browser services

		virtual const char* getName() = 0;				//return NULL if not supported
		virtual const char* getVersion() = 0;			//return NULL if not supported
		virtual float getCurrentSpeed() = 0;		//return 0.0f if not supported
		virtual float getCurrentFrameRate() = 0;	//return 0.0f if not supported
		
		
		
		virtual void replaceWorld(const char* sceneURI) = 0;	//no return required. Throws if error.
		//virtual void replaceWorld(const X3D_??* sourceScenePtr) = 0; //maybe not necessary, but W3C docs also declares the possibility of creating a scene from a scene object

		virtual void loadURL(const char* sceneURL) = 0; //no return required. Throws if error

		virtual void setDescription(const char* strDescription) = 0; //no return required.

		virtual saiScene* createX3DFromString(const char* strX3DSource) = 0;	

		virtual void updateControl(unsigned int nAction)= 0;				//it's better to throw a "not supported" exception if not really implemented
		
		virtual void registerBrowserInterest(unsigned int nAction, saiBrowser* pRequester) = 0; //it's better to throw a "not supported" exception if not really implemented
																								//it is perfectly acceptable that "pRequester" can be "this"
		virtual std::map<std::string, std::string>* getRenderingProperties() = 0;	

		virtual std::map<std::string, std::string>* getBrowserProperties() = 0;

		virtual void changeViewpoint(unsigned int nAction) = 0;				//changes only the active context's viewpoint.

		virtual void print() = 0;

		virtual void dispose() = 0;

		virtual bool setBrowserOption(const char* strOptionName, void* pOptionValue) = 0;	//returns true if the option is set, false if not.		
																							//it's better to throw a "not supported" exception if not really implemented
		
		virtual const std::vector<saiProfileDeclaration*>*  getSupportedProfiles() = 0; //should return a list of all the supported profiles and only that

		virtual const saiProfileDeclaration* getProfile(const char* strProfileName) = 0; //should return a pointer to the profile with matching name or throw NotSupportedException

		virtual const std::map<std::string,saiComponent*>*  getSupportedComponents() = 0; //should return a map of all the supported components. We use the saiComponentDeclaration pair to browse the map 																			

		virtual const saiComponent* getComponent(const char* strComponentName) = 0; //should return a pointer to an instance of saiComponent with matching name or throw NotSupportedException
		
		virtual const saiExecutionContext* getExecutionContext() = 0;	
		
		virtual saiExecutionContext* createScene() = 0;			
		
		virtual saiExecutionContext* importDocument(const char* DOMdocURI) = 0;

		virtual saiExecutionContext* createX3DFromStream(void* pStreambuf) = 0;	
		
		virtual saiExecutionContext* createX3DFromUrl(const char* srcURL) = 0;		
	};

};

#endif _SAIBROWSER_H_ABSTRACT
