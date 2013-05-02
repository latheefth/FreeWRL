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

#ifndef _SAIEXCEPTION_H_
#define _SAIEXCEPTION_H_

#define SAI_GENERIC_EXCEPTION				0
#define SAI_BROWSER_UNAVAILABLE				1
#define SAI_CONNECTION_ERROR				2
#define SAI_DISPOSED						3
#define SAI_IMPORTED_NODE					4
#define SAI_INSUFFICIENT_CAPABILITIES		5
#define SAI_INVALID_ACCESS_TYPE				6
#define SAI_INVALID_BROWSER					7
#define SAI_INVALID_DOCUMENT				8
#define SAI_INVALID_FIELD					9
#define SAI_INVALID_NAME					10		//mapped to SAI_INVALID_FIELD or to SAI_INVALID_NODE as Java Language Binding does
#define SAI_INVALID_NODE					11
#define SAI_INVALID_OPERATION_TIMING		12
#define SAI_INVALID_URL						13
#define SAI_INVALID_X3D						14
#define SAI_NODE_NOT_AVAILABLE				15
#define SAI_NODE_IN_USE						16
#define SAI_NOT_SHARED						17
#define SAI_NOT_SUPPORTED					18
#define SAI_URL_UNAVAILABLE					19
#define SAI_INVALID_EXECUTION_CONTEXT		20
//derived exceptions as specified in Java Language Binding
#define SAI_NOT_READABLE_FIELD				21
#define SAI_NOT_WRITABLE_FIELD				22

//placeholder for custom exception left to the provider
#define SAI_CUSTOM_EXCEPTION				100

#include <exception>

namespace freeWRLSAI_cpp
{	
	//Generic SAI exception. Base class
	class saiException : public std::exception
	{
	public:
		saiException()
		{
			m_nErrorCode = 0;			
		}

		virtual const char* what(){return "Generic SAI exception.\n";}

		virtual int GetError(){return m_nErrorCode;}

	protected:
		int m_nErrorCode;
	};

	//SAI_BROWSER_UNAVAILABLE exception.
	class noSuchBrowserException : public saiException
	{
	public:
		noSuchBrowserException()
		{
			m_nErrorCode = SAI_BROWSER_UNAVAILABLE;			
		}

		virtual const char* what(){return	"The request to gain a reference to a SAIBrowserApp has failed.\n "
											"The connection may be down or the type of reference required is not supported.";}	
	};

	class connectionException : public saiException
	{
	public:
		connectionException()
		{
			m_nErrorCode = SAI_CONNECTION_ERROR;			
		}

		virtual const char* what(){return "Connection to browser failed or was never established.\n";}	
	};

	class disposedException : public saiException
	{
	public:
		disposedException()
		{
			m_nErrorCode = SAI_DISPOSED;			
		}

		virtual const char* what(){return "The reference required has already beed disposed in this context.\n";}	
	};

	class invalidImportException : public saiException
	{
	public:
		invalidImportException()
		{
			m_nErrorCode = SAI_IMPORTED_NODE;			
		}

		virtual const char* what(){return "An operation was attempted that used an imported node when it is not permitted.\n";}	
	};

	class insufficientCapabilitiesException : public saiException
	{
	public:
		insufficientCapabilitiesException()
		{
			m_nErrorCode = SAI_INSUFFICIENT_CAPABILITIES;			
		}

		virtual const char* what(){return "Cannot add a node to an execution context that is greater than the capabilities defined by the profile and components definition for the scene.\n";}	
	};

	class invalidAccessTypeException : public saiException
	{
	public:
		invalidAccessTypeException()
		{
			m_nErrorCode = SAI_INVALID_ACCESS_TYPE;			
		}

		virtual const char* what(){return "Cannot perform the requested operation because it is an invalid action for this field type.\n";}	
	};

	class invalidBrowserException : public disposedException
	{
	public:
		invalidBrowserException()
		{
			m_nErrorCode = SAI_INVALID_BROWSER;			
		}

		virtual const char* what(){return "The Browser service requested has been disposed of prior to this request.\n";}	
	};

	class invalidDocumentException : public saiException
	{
	public:
		invalidDocumentException()
		{
			m_nErrorCode = SAI_INVALID_DOCUMENT;			
		}

		virtual const char* what(){return "The document structure is not compliant with X3D DOM.\n";}	
	};

	class invalidOperationTimingException : public saiException
	{
	public:
		invalidOperationTimingException()
		{
			m_nErrorCode = SAI_INVALID_OPERATION_TIMING;			
		}

		virtual const char* what(){return "The user is attempting to make a service request that is performed outside of the context that such operations are permitted in.\n";}	
	};

	class invalidUrlException : public saiException
	{
	public:
		invalidUrlException()
		{
			m_nErrorCode = SAI_INVALID_URL;			
		}

		virtual const char* what(){return "Syntax Error: the url specified is not correct.\n";}
	};

	class invalidX3DException : public saiException
	{
	public:
		invalidX3DException()
		{
			m_nErrorCode = SAI_INVALID_X3D;			
		}

		virtual const char* what(){return "Syntax Error: the X3D document requested or specified is not correct.\n";}
	};

	class nodeUnavailableException : public saiException
	{
	public:
		nodeUnavailableException()
		{
			m_nErrorCode = SAI_NODE_NOT_AVAILABLE;			
		}

		virtual const char* what(){return	"The imported node requested has not yet been verified for export.\n";}
	};

	class nodeInUseException : public saiException
	{
	public:
		nodeInUseException()
		{
			m_nErrorCode = SAI_NODE_IN_USE;			
		}

		virtual const char* what(){return	"A named node handling action has attempted to re-use a name that is already defined elsewhere in this current scene.\n "
											"Or the node, or one of its children, is currently in use in another scene.\n";}
	};

	class browserNotSharedException : public saiException
	{
	public:
		browserNotSharedException()
		{
			m_nErrorCode = SAI_NOT_SHARED;			
		}

		virtual const char* what(){return "A service request was made that assumed the browser was currently participating in a shared scene graph when it was not.\n";}
	};

	class notSupportedException : public saiException
	{
	public:
		notSupportedException()
		{
			m_nErrorCode = SAI_NOT_SUPPORTED;			
		}

		virtual const char* what(){return "The service request is not supported.\n";}
	};

	class urlUnavailableException : public saiException
	{
	public:
		urlUnavailableException()
		{
			m_nErrorCode = SAI_URL_UNAVAILABLE;			
		}

		virtual const char* what(){return "No valid URL has been specified for this request.\n";}
	};

	//DERIVED FROM disposedException

	class invalidExecutionContextException : public disposedException
	{
	public:
		invalidExecutionContextException()
		{
			m_nErrorCode = SAI_INVALID_EXECUTION_CONTEXT;			
		}

		virtual const char* what(){return "The execution context requested has been disposed of prior to this request.\n";}	
	};

	class invalidFieldException : public disposedException
	{
	public:
		invalidFieldException()
		{
			m_nErrorCode = SAI_INVALID_FIELD;			
		}

		virtual const char* what(){return "The requested field has been disposed of prior to this request.\n";}	
	};

	class invalidNodeException : public disposedException
	{
	public:
		invalidNodeException()
		{
			m_nErrorCode = SAI_INVALID_NODE;			
		}

		virtual const char* what(){return "The requested node has been disposed of prior to this request.\n";}	
	};	

	//OTHER DERIVED EXCEPTION
	class InvalidWritableFieldException : public invalidFieldException, public invalidAccessTypeException
	{
		public:
		InvalidWritableFieldException()
		{
			invalidFieldException::m_nErrorCode = SAI_NOT_WRITABLE_FIELD;			
		}

		virtual const char* what(){return "The requested field cannot be written.\n";}	
	};

	class InvalidReadableFieldException : public invalidFieldException, public invalidAccessTypeException
	{
		public:
		InvalidReadableFieldException()
		{
			invalidFieldException::m_nErrorCode = SAI_NOT_READABLE_FIELD;			
		}

		virtual const char* what(){return "The requested field cannot be read.\n";}	
	};

	//facility exception NOT REQUESTED by W3C specs
	class saiCustomException : public saiException
	{
	public:
		saiCustomException(const char* strWhat, const char* strFile, int strLine, const char* strFunc)
		{
			m_nErrorCode = SAI_CUSTOM_EXCEPTION;

			char buffer[256];

			int nChars = sprintf_s(buffer,256,"%s\n In file: %s, line: %d, function: %s",strWhat,strFile,strLine,strFunc);

			if(nChars > 0)
				m_strWhat = buffer;
			else
				m_strWhat = "Exception message was not well formed.\n";
		}

		virtual const char* what(){return m_strWhat.c_str();}	

		std::string m_strWhat;
	};

};

#endif //_SAIEXCEPTION_H_