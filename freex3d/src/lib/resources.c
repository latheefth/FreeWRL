/*

  FreeWRL support library.
  Resources handling: URL, files, ...

*/

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



#include <config.h>
#include <system.h>
#include <system_threads.h>
#include <display.h>
#include <internal.h>
#include <libFreeWRL.h>

#include "vrml_parser/Structs.h"
#include "input/InputFunctions.h"
#include "opengl/OpenGL_Utils.h"
#include "opengl/Textures.h"		/* for finding a texture url in a multi url */
#include "opengl/LoadTextures.h"	/* for finding a texture url in a multi url */

#include <list.h>
#include <io_files.h>
#include <io_http.h>
#include <threads.h>

#ifdef _ANDROID
#include <strings.h>
#endif

#include "zlib.h"
//#define DEBUG_RES printf
static void possiblyUnzip (openned_file_t *of);

void close_openned_file(openned_file_t *file);

typedef struct presources{
	struct Vector *resStack; //=NULL;
	resource_item_t *lastBaseResource; //=NULL;
}* presources;
void *resources_constructor()
{
	void *v = MALLOCV(sizeof(struct presources));
	memset(v,0,sizeof(struct presources));
	return v;
}
void resources_init(struct tresources* t)
{
	//public
	//private
	//presources p;
	t->prv = resources_constructor();
	//p = (presources)t->prv);
}
void resources_clear(struct tresources* t)
{
	//public
	//private
	presources p;
	p = (presources)t->prv;
	deleteVector(void *,p->resStack);
}


/* move Michel Briand's initialization code to one place to ensure consistency
   when fields are added/removed */

resource_item_t *newResourceItem() {
	resource_item_t *item = XALLOC(resource_item_t);
	
	/* item is NULL for every byte; some of the enums might not work out to 0 */
	item->media_type = resm_unknown;
	item->type = rest_invalid;
	item->status = ress_invalid;
	item->parent = NULL;
	item->actual_file = NULL;
	item->cached_files = NULL;
	item->tg = gglobal();
	return item;
}


/**
 *   When main world/file is initialized, setup this
 *   structure. It stores essential information to make
 *   further resource loading work. Resource loading take
 *   care of relative path/url to the main world/file.
 *   
 *   Main path/url will fill the  'base' field = base path
 *   or base url of the world.
 */

/**
 *   resource_create_single: create the resource object and add it to the root list.
 */
static void resource_tree_append(resource_item_t *item){
	/* Lock access to the resource tree */
	pthread_mutex_lock( &gglobal()->threads.mutex_resource_tree );

	if (!gglobal()->resources.root_res) {
		/* This is the first resource we try to load */
		gglobal()->resources.root_res = (void*)item;
		DEBUG_RES("setting root_res in resource_create_single for file %s\n",request);
	} else {
		/* Not the first, so keep it in the main list */
		((resource_item_t*)gglobal()->resources.root_res)->children = ml_append(((resource_item_t*)gglobal()->resources.root_res)->children, ml_new(item));
		item->parent = (resource_item_t*)gglobal()->resources.root_res;
	}

	/* Unlock the resource tree mutex */
	pthread_mutex_unlock( &gglobal()->threads.mutex_resource_tree );
}

resource_item_t* resource_create_single0(const char *request)
{
	resource_item_t *item;
	DEBUG_RES("creating resource: SINGLE: %s\n", request);

	item = newResourceItem();
	item->URLrequest = STRDUP(request);
    item->_loadThread = NULL;
	return item;
}

resource_item_t* resource_create_single(const char *request)
{
	resource_item_t *item = resource_create_single0(request);
	resource_tree_append(item);
	return item;
}

/**
 *   resource_create_single: create the resource object and add it to the root list.
 *
 *   TODO: finish the multi implementation.
 */
resource_item_t* resource_create_multi0(s_Multi_String_t *request)
{
	/* anchor to new scene might use the multi0 directly, so plugin_res isn't deleted in killOldWorld */
	int i;
	resource_item_t *item;
	DEBUG_RES("creating resource: MULTI: %d, %s ...\n", request->n, request->p[0]->strptr);
	item = newResourceItem();


	item->type = rest_multi;


	/* Convert Mutli_String to a list string */
	for (i = 0; i < request->n; i++) {
		char *url = STRDUP(request->p[i]->strptr);
			//ConsoleMessage ("putting %s on the list\n",url); 
			item->m_request = ml_append(item->m_request, ml_new(url));
	}
	return item;
}
resource_item_t* resource_create_multi(s_Multi_String_t *request)
{
	resource_item_t *item = resource_create_multi0(request);
	resource_tree_append(item);
	return item;
}

/**
 *   resource_create_single: create the resource object and add it to the root list.
 *                           A string resource may be an inline VRML/X3D code or an inline
 *                           shader code.
 */
resource_item_t* resource_create_from_string(const char *string)
{
	resource_item_t *item;
	DEBUG_RES("creating resource: STRING: %s\n", string);
	item = newResourceItem();


	item->URLrequest = STRDUP(string);
	item->type = rest_string;
	item->status = ress_loaded;

	resource_tree_append(item);
	return item;
}


/*
 * Check to see if the file name is a local file, or a network file.
 * return TRUE if it looks like a file from the network, false if it
 * is local to this machine
 * October 2007 - Michel Briand suggested the https:// lines.
 */
/**
 *   checkNetworkFile:
 */
bool checkNetworkFile(const char *fn)
{
    //int i = 0; 
    //char *pt = fn; 
    
    if (fn == NULL) {
        ConsoleMessage ("checkNetworkFile, got a NULL here");
        return FALSE;
    }
    
  //  while (*pt != '\0') {
  //      ConsoleMessage ("cfn %d is %x %c",i,*pt,*pt);
  //      i++;
  //      pt++;
  //  }
    
    //ConsoleMessage ("checkNetworkFile, have %s, len %d\n",fn,strlen(fn));
    
	if ((strncmp(fn,"ftp://", strlen("ftp://"))) &&
	    (strncmp(fn,"FTP://", strlen("FTP://"))) &&
	    (strncmp(fn,"http://", strlen("http://"))) &&
	    (strncmp(fn,"HTTP://", strlen("HTTP://"))) &&
	    (strncmp(fn,"https://", strlen("https://"))) &&
	    (strncmp(fn,"HTTPS://", strlen("HTTPS://"))) &&
/* JAS - these really are local files | MB - indeed :^) !
	    (strncmp(fn,"file://", strlen("file://"))) &&
	    (strncmp(fn,"FILE://", strlen("FILE://"))) &&
*/
	    (strncmp(fn,"urn://", strlen("urn://"))) &&
	    (strncmp(fn,"URN://", strlen("URN://")))

	) {
        	//ConsoleMessage ("CNF returning FALSE");
		return FALSE;
	}
    	//ConsoleMessage ("CNF returning TRUE");
	return TRUE;
}


/**
 *   resource_identify: identify resource type and location relatively to base
 *		If base is NULL then this resource may become a root
 *			node of resource hierarchy.
 *	for sandbox apps, in general there are 3 locations of interest: a) internet b) intranet c) local
 *  and the 3 may be treated differently in the frontend, for example the app may have internet permission
 *  but not intranet permission. Local files would be in an AppData folder where fopen,fread have permission.
 *  we don't differentiate here between intranet and local, instead that's done with res->status == ress_downloaded
 *  when the file is local
 *  in frontend: 
 *    if(res->type == rest_url)
 *		internet file: download to local, and set res->status to ress_downloaded
 *    else if(res->type == rest_file && res->status != ress_downloaded)
 *		intranet file: get permission (ie filepicker or access permission cache) and copy from intranet to local and set res->status = ress_downloaded
 *    else if(res->type == rest_file && res->status == ress_downloaded)
 *		local file: load local file to blob in frontend, or enqueue a backend load task, depending on your platform configuration
 *
 *	
 *
 */
 static int res_id_error_once = 0;
void resource_identify(resource_item_t *baseResource, resource_item_t *res)
{
	bool network;
	char *url = NULL;
	size_t len;
	resource_item_t *defaults = NULL;

	ASSERT(res);
	DEBUG_RES("resource_identify, we have resource %s ptrs %p and %p\n",res->URLrequest,baseResource,baseResource);

	if (baseResource) {
		DEBUG_RES(" base specified, taking the base values.\n");
		defaults = baseResource;
		res->parent = baseResource;
	} else {
		if (res->parent) {
			DEBUG_RES(" no base specified, taking parent's values.\n");
			defaults = res->parent;
		} else {
			DEBUG_RES(" no base neither parent, no default values.\n");
		}
	}

	if (defaults) {
		DEBUG_RES(" default values: network=%s type=%s status=%s"
			  " URLrequest=<%s> URLbase=<%s>parsed_request=<%s> [parent %p, %s]\n",
			  BOOL_STR(defaults->network), resourceTypeToString(defaults->type), 
			  resourceStatusToString(defaults->status), defaults->URLrequest, 
			  defaults->URLbase, defaults->parsed_request,
			  defaults->parent, (defaults->parent ? defaults->parent->URLbase : "N/A")
			);
	}

	if (res->type == rest_multi) {
		/* We want to consume the list of requests */
		if (res->m_request) {
			s_list_t *l;
			l = res->m_request;
			/* Pick up next request in our list */
			FREE_IF_NZ(res->URLrequest);
			res->URLrequest = (char *) l->elem;
			/* Point to the next... */
			res->m_request = res->m_request->next;
			ml_free(l);
		} else {
			/* list empty - this error can be caused by a wrong USE='name' on URL node */
			if(!res_id_error_once)  //don't flood, there's probably a better error message before this
				ERROR_MSG("resource_identify: ERROR: empty multi string as input\n");
			res_id_error_once++;
			return;
		}
	}

	network = FALSE;
	if (defaults) {
		network = defaults->network;
	}

	{	
		char* pound;
		pound = NULL;
		pound = strchr(res->URLrequest, '#'); //moved here Aug2014 Q. should it be later on strdup of URLrequest?
		if (pound != NULL) {
			*pound = '\0';
			/* copy the name out, so that Anchors can go to correct Viewpoint */
			pound++;
			res->afterPoundCharacters = STRDUP(pound);
		}
	}
	/* URI specifier at the beginning ? */
	res->network = checkNetworkFile(res->URLrequest);

	DEBUG_RES("resource_identify: base network / resource network: %s/%s\n", 
		  BOOL_STR(network),
		  BOOL_STR(res->network));

	/* Parse request as url or local file ? */
	if (res->network || network) {
		/* We will always have a network url */

		if (res->network) {
			/* We have an absolute url for this resource */
			res->type = rest_url;
			res->status = ress_starts_good;
			url = STRDUP(res->URLrequest);

		} else {
			/* We have an absolute url for main world,
			   and a relative url for this resource:
			   Create an url with base+request */
			if (defaults) {

				/* note that, if FRONTEND_GETS_FILES is defined, we have to clean
				   this, here. */

				char *cleanedURL;
				cleanedURL = stripLocalFileName(res->URLrequest);

				/* Relative to base */
				IF_cleanedURL_IS_ABSOLUTE {
					/* this is an absolute url, which we can do, even if we have a base to
					   base this from. eg, url='/Users/john/tests/2.wrl'  */
					url = STRDUP(cleanedURL);
				} else {
					char *cwd;
					cwd = STRDUP(defaults->URLbase);
					url = concat_path(cwd, cleanedURL);
					FREE_IF_NZ(cwd);
				}
				res->network = TRUE; //dug9 sep1,2013 added this line, so geoLod 2nd level texture sees its parent 2nd level .x3d as a network file
				res->type = rest_url;
				res->status = ress_starts_good;
			} else {
				res->type = rest_invalid;
				ERROR_MSG("resource_identify: can't handle relative url without base: %s\n", res->URLrequest);
			}
		}		
			
	} else {
		/* We may have a local file */
		DEBUG_RES("resource_identify, we may have a local file for resource %s\n", res->URLrequest);

		/* We do not want to have system error */
		len = strlen(res->URLrequest);
		if (len > PATH_MAX) {

			res->type = rest_invalid;
			url="invalid URL";
			ERROR_MSG("resource_identify: path too long: %s\n", res->URLrequest);

		} else {
			char *cleanedURL = NULL;
			/* remove any possible file:// off of the front of the name */
			/* NOTE: this is NOT a new string, possibly just incremented res->request */

			cleanedURL = stripLocalFileName(res->URLrequest);

			/* We are relative to current dir or base */
			if (defaults) {
				/* Relative to base */
				IF_cleanedURL_IS_ABSOLUTE {
					/* this is an absolute url, which we can do, even if we have a base to
					   base this from. eg, url='/Users/john/tests/2.wrl'  */
					res->type = rest_file;
					res->status = ress_starts_good;
					url = STRDUP(cleanedURL);
				} else {
					char *cwd;
					cwd = STRDUP(defaults->URLbase);
					res->type = rest_file;
					res->status = ress_starts_good;
					url = concat_path(cwd, cleanedURL);
					FREE_IF_NZ(cwd);
				}

			} else {
				/* No default values: we are hanging alone */
				/* Is this a full path ? */
				IF_cleanedURL_IS_ABSOLUTE {
					/* This is an absolute filename */

					res->type = rest_file;
					res->status = ress_starts_good;
					url = STRDUP(cleanedURL);

				} else {
					/* Relative to current dir (we are loading main file/world) */
					char *cwd;

					cwd = get_current_dir();
					removeFilenameFromPath(cwd);

					/* Make full path from current dir and relative filename */

					/* printf("about to join :%s: and :%s: resource.c L299\n",cwd,res->request);*/
					url = concat_path(cwd, res->URLrequest);
					res->type = rest_file;
					res->status = ress_starts_good;
				}
			}
		}
	}

	/* record the url, and the path to the url */
	FREE_IF_NZ(res->parsed_request);
	res->parsed_request = url;
	FREE_IF_NZ(res->URLbase);
	res->URLbase = STRDUP(url);
	removeFilenameFromPath(res->URLbase);

    // ok we should be good to go now        res->network = TRUE;

	DEBUG_RES("resource_identify (end): network=%s type=%s status=%s"
		  " request=<%s> base=<%s> url=<%s> [parent %p, %s]\n", 
		  BOOL_STR(res->network), resourceTypeToString(res->type), 
		  resourceStatusToString(res->status), res->URLrequest, 
		  res->URLbase, res->parsed_request,
		  res->parent, (res->parent ? res->parent->URLbase : "N/A"));
	return;
}
textureTableIndexStruct_s *getTableIndex(int i);
bool imagery_load(resource_item_t *res){
	bool retval;
	int textureNumber;
	struct textureTableIndexStruct *entry; // = res->whereToPlaceData;
	textureNumber = res->textureNumber;
	if(res->status == ress_downloaded){
		entry = getTableIndex(textureNumber);
		if(entry)
		if (texture_load_from_file(entry, res->actual_file)) {
			entry->status = TEX_READ; /* tell the texture thread to convert data to OpenGL-format */
			res->status = ress_loaded;
			retval = TRUE;
			return retval;
		}
	}
	res->status = ress_not_loaded;
	retval = FALSE;
	return retval;
}


/**
 *   resource_load: load the actual file into memory, add it to openned files list.
 */
bool resource_load(resource_item_t *res)
{
	openned_file_t *of = NULL;

	DEBUG_RES("loading resource: %s, %s\n", resourceTypeToString(res->type), resourceStatusToString(res->status));
	
	ASSERT(res);

	switch (res->status) {
	case ress_none:
	case ress_starts_good:
	case ress_invalid:
	case ress_failed:
		ERROR_MSG("resource_load: can't load not available resource: %s\n", res->URLrequest);
		break;



	case ress_downloaded:
		of = load_file(res->actual_file);

		if (of) {
			res->status = ress_loaded;
			//res->openned_files = ml_append( (s_list_t *) res->openned_files, ml_new(of) );
			res->openned_files = of; 

			/* If type is not specified by the caller try to identify it automatically */
			if (res->media_type == resm_unknown) {
				resource_identify_type(res);
			}

		} else {

			res->status = ress_not_loaded;
			ERROR_MSG("resource_load: can't load file: %s\n", res->actual_file);
		}

		break;
	
	case ress_loaded:
		ERROR_MSG("resource_load: MISTAKE: can't load already loaded resource: %s\n", res->URLrequest);
		break;

	case ress_not_loaded:
		ERROR_MSG("resource_load: loader already failed for this resource: %s\n", res->URLrequest);
		break;

	case ress_parsed:
		ERROR_MSG("resource_load: MISTAKE: can't load resource already parsed: %s\n", res->URLrequest);
		break;

	case ress_not_parsed:
		ERROR_MSG("resource_load: MISTAKE: can't load resource already parsed (and failed): %s\n", res->URLrequest);
		break;
	}

	return (of != NULL);
}

/**
 *   resource_identify_type: determine media (file) type.
 */
void resource_identify_type(resource_item_t *res)
{
	char *test_it = NULL;
    int test_it_len = 0;
    
	//s_list_t *l;
	openned_file_t *of;
	int t;

	if (res->media_type != resm_unknown)
		/* caller specified type, or we already identified it */
		return;

	switch (res->status) {
	case ress_loaded:
		switch (res->type) {
		case rest_invalid:
			ERROR_MSG("can't identify type for invalid resource: %s\n", res->URLrequest);
			return;
			break;
		case rest_string:
			test_it = (char*)res->URLrequest;
                ConsoleMessage ("test_it is :%s:",test_it);
            test_it_len = (int)strlen(res->URLrequest);
			break;
		case rest_url:
		case rest_file:
		case rest_multi:
			//l = (s_list_t *) res->openned_files;
			//if (!l) {
			//	/* error */
			//	return;
			//}
			//
			//of = ml_elem(l);
			of = res->openned_files;
			if (!of) {
				/* error */
				return;
			}
			/* maybe .x3z (.zip) archive? */
			{
				char *sourcename = (char *)of->fileFileName;
				if(res->type == rest_url) sourcename = res->URLrequest;
				if(!strcmp(&sourcename[strlen(sourcename)-4],".x3z")){
					res->media_type = resm_x3z;
					return;
				}
			}
			/* might this be a gzipped input file? */
			possiblyUnzip(of);
			test_it = of->fileData;
                test_it_len = of->fileDataSize;
			break;
		}


		/* Test it */
		t = determineFileType(test_it,test_it_len);
		switch (t) {
		case IS_TYPE_VRML:
		case IS_TYPE_VRML1:
                
#if defined (INCLUDE_STL_FILES)
            case IS_TYPE_BINARY_STL: case IS_TYPE_ASCII_STL:
#endif //INCLUDE_STL_FILES

                                
			res->media_type = resm_vrml;
			break;
                
#if defined (INCLUDE_NON_WEB3D_FORMATS)
		case IS_TYPE_COLLADA:
		case IS_TYPE_KML:
		case IS_TYPE_SKETCHUP:
#endif  //INCLUDE_NON_WEB3D_FORMATS
                
		case IS_TYPE_XML_X3D:
			res->media_type = resm_x3d;
			break;
		}
		break;
	default:
		break;
	}
	return;
}


/**
 *   resource_remove_cached_file: TODO.
 */
void remove_file_or_folder(const char *path);

void resource_remove_cached_file(s_list_t *cfe)
{
	const char *cached_file;
	cached_file = (const char *) cfe->elem;
	ASSERT(cached_file);
	/* TODO: reference counter on cached files... */
	remove_file_or_folder(cached_file);
	//UNLINK(cached_file);
}

/**
 *   resource_destroy: destroy this object (and all contained allocated data).
 *                     It may not be used anymore.
 */

void _resourceFreeCallback(void *resource);

void resource_destroy(resource_item_t *res)
{
	s_list_t *cf; // *of,

	if(!res) return;
	DEBUG_RES("destroying resource: %d, %d\n", res->type, res->status);

	ASSERT(res);

	switch (res->type) {
	case rest_invalid:
		/* nothing to do */
		break;
	case rest_url:
		switch (res->status) {
		case ress_none:
		case ress_starts_good:
		case ress_invalid:
			/* nothing to do */
			break;

		case ress_downloaded:
		case ress_failed:
		case ress_loaded:
		case ress_not_loaded:
		case ress_parsed:
		case ress_not_parsed:
		if(0){
			/* Remove openned file ? */
			//of = (s_list_t *) res->openned_files;
			//of = res->openned_files;
			//if (of) {
			//	/* close any openned file */
			//	close( ((openned_file_t*)of->elem)->fileDescriptor );
			//}

			/* Remove cached file ? */
			cf = (s_list_t *) res->cached_files;
			if (cf) {
				/* remove any cached file:
				   TODO: reference counter on cached files...
				 */
				ml_foreach(cf, resource_remove_cached_file(__l));
			}
		}
			/* free the actual file  */
			FREE_IF_NZ(res->actual_file);
			break;
		}

		/* free the parsed_request url */
		FREE_IF_NZ(res->parsed_request);
		break;

	case rest_file:
		switch (res->status) {
		case ress_none:
		case ress_starts_good:
		case ress_invalid:
			/* nothing to do */
			break;

		case ress_downloaded:
		case ress_failed:
		case ress_loaded:
		case ress_not_loaded:
		case ress_parsed:
		case ress_not_parsed:
			/* Remove openned file ? */
			//of = (s_list_t *) res->openned_files;
			//if (of) {
			//	/* close any openned file */
			//}

			/* free the actual file  */
			FREE(res->actual_file);
			break;
		}

		/* free the parsed_request url */
		FREE_IF_NZ(res->parsed_request);
		break;

	case rest_string:
		/* Nothing to do */
		break;
	}

	/* Free the list */
	ml_delete_all2(res->m_request, ml_free);
	res->m_request = NULL;

	FREE_IF_NZ(res->URLbase);
	FREE_IF_NZ(res->afterPoundCharacters);
	FREE_IF_NZ(res->openned_files);
	//if (!res->parent) {
	//	/* Remove base */
	//	FREE_IF_NZ(res->URLbase);
	//} else {
	//	/* We used parent's base, so remove us from parent's childs */
	//	//resource_remove_child(res->parent, res);
	//}

	FREE_IF_NZ(res->URLrequest);
	FREE_IF_NZ(res);
}

void resource_unlink_cachedfiles(resource_item_t *res)
{
	s_list_t *cf;

	if(!res) return;
	DEBUG_RES("destroying resource: %d, %d\n", res->type, res->status);

	ASSERT(res);

	/* Remove cached file ? */
	cf = (s_list_t *) res->cached_files;
	if (cf) {
		/* remove any cached file:
		   TODO: reference counter on cached files...
		 */
		ml_foreach(cf, resource_remove_cached_file(__l));
	}

}

void resource_close_files(resource_item_t *res)
{

	if(!res) return;
	DEBUG_RES("closing resource file: %d, %d\n", res->type, res->status);

	ASSERT(res);

	/* Remove openned file ? */

}


/**
 *   resource_remove_child: remove given child from the parent's list of _children_ // cached files.
 */
void resource_remove_child(resource_item_t *parent, resource_item_t *child)
{
	s_list_t *cf;

	ASSERT(parent);
	ASSERT(child);

	//cf = ml_find_elem(parent->cached_files, child);
	cf = ml_find_elem(parent->children, child);
	if (cf) {
		//ml_delete(parent->cached_files, cf);
		ml_delete(parent->children, cf);
	}
}

/**
 *   destroy_root_res: clean up all resources loaded under this root_res.
 */
void destroy_root_res()
{
	resource_destroy((resource_item_t*)gglobal()->resources.root_res);
	gglobal()->resources.root_res = NULL;
}

void resource_tree_destroy()
{
	resource_item_t* root;
	root = (resource_item_t*)gglobal()->resources.root_res;
	if(root){
		ml_foreach(root->children,resource_close_files((resource_item_t*)ml_elem(__l)));
		ml_foreach(root->children,resource_unlink_cachedfiles((resource_item_t*)ml_elem(__l)));
		ml_foreach(root->children,resource_destroy((resource_item_t*)ml_elem(__l)));
		ml_foreach(root->children,resource_remove_child(root,(resource_item_t*)ml_elem(__l)));
		ml_foreach(root->children,ml_free(__l));
		resource_close_files(root);
		resource_unlink_cachedfiles(root);
		destroy_root_res();
	}

}
/**
 *   resource_dump: debug function.
 */
void resource_dump(resource_item_t *res)
{
	s_list_t *cf;
	//openned_file_t *of;
	//s_list_t *of;
	void *ofv;

	PRINTF ("resource_dump: %p\n"
		  "request: %s\n"
		  "parsed request: %s\n"
		  "actual file: %s\n"
		  "cached files: ",
		  res, res->URLrequest, res->parsed_request, res->actual_file);

	cf = (s_list_t *) res->cached_files;
	if (cf) {
		ml_foreach(cf, PRINTF("%s ", (char *) ml_elem(__l)));
	} else {
		PRINTF("none");
	}
	PRINTF("\nopenned files: ");

	//of = (s_list_t *) res->openned_files;
	ofv = res->openned_files;
	if (ofv) {
		openned_file_t *of = (openned_file_t*)ofv;
		PRINTF("%s ", of->fileFileName);
	} else {
		PRINTF("none");
	}
	PRINTF("\n");
}
void splitpath_local_suffix(const char *url, char **local_name, char **suff);
/**
 *   fwl_resource_push_single_request: easy function to launch a load process (asynchronous).
 */
void fwl_resource_push_single_request(const char *request)
{
	resource_item_t *res;

	if (!request)
		return;

	res = resource_create_single(request);
	//send_resource_to_parser(res);
	resitem_enqueue(ml_new(res));
	if(request){
		//update information about the scene for scripting (Q. what about window title?)
		//not sure this is a good place, in part because the calling thread may not be in a gglobal thread
		ttglobal tg = gglobal();
		char* suff = NULL;
		char* local_name = NULL;
		splitpath_local_suffix(request, &local_name, &suff);
		tg->Mainloop.scene_name = local_name;
		tg->Mainloop.scene_suff = suff;
	}

}

/**
 *   resource_push_multi_request: easy function to launch a load process (asynchronous).
 */
void resource_push_multi_request(struct Multi_String *request)
{
	resource_item_t *res;

	if (!request)
		return;

	res = resource_create_multi(request);
	resitem_enqueue(ml_new(res));
	//send_resource_to_parser(res);
}


/**
 *   resource_tree_dump: print the resource tree for debugging.
 *   NB: call this recursive function with level==0
 */
void resource_tree_dump(int level, resource_item_t *root)
{
#define spacer	for (lc=0; lc<level; lc++) printf ("\t");

	s_list_t *children;
	int lc;

	if (root == NULL) return; 
	if (level == 0) printf("\nResource tree:\n\n");
	else printf("\n");

	spacer printf("==> request:\t %s\n\n", root->URLrequest);
	spacer printf("this:\t %p\n", root);
	spacer printf("parent:\t %p\n", root->parent);
	spacer printf("network:\t %s\n", BOOL_STR(root->network));
	spacer printf("new_root:\t %s\n", BOOL_STR(root->new_root));
	spacer printf("type:\t %u\n", root->type);
	spacer printf("status:\t %u\n", root->status);
	spacer printf("complete:\t %s\n", BOOL_STR(root->complete));
	spacer printf("where:\t %p\n", root->whereToPlaceData);
	spacer printf("offsetFromWhere:\t %d\n", root->offsetFromWhereToPlaceData);
	spacer printf("m_request:\t %p\n", root->m_request);
	spacer printf("base:\t %s\n", root->URLbase);
	spacer printf("temp_dir:\t %s\n", root->temp_dir);
	spacer printf("parsed_request:\t %s\n", root->parsed_request);
	spacer printf("actual_file:\t %s\n", root->actual_file);
	spacer printf("cached_files:\t %p\n", root->cached_files);
	//if (root->openned_files) {
	//	spacer printf("openned_files:\t "); ml_foreach(root->openned_files, of_dump((openned_file_t *)ml_elem(__l)));
	//} else {
	//	spacer printf("openned_files:\t <empty>\n");
	//}
	spacer printf("four_first_bytes:\t %c %c %c %c\n", root->four_first_bytes[0], root->four_first_bytes[1], root->four_first_bytes[2], root->four_first_bytes[3]);
	spacer printf("media_type:\t %u\n", root->media_type);

	children = root->children;

	ml_foreach(children, resource_tree_dump(level + 1, ml_elem(__l)));

	printf("\n");
}

void resource_tree_count_files(int *count, resource_item_t *root)
{
	if (root == NULL) return;
	(*count)++;
	ml_foreach(root->children, resource_tree_count_files(count, ml_elem(__l)));
}
void printStatsResources()
{
	int count = 0;
	resource_tree_count_files(&count, gglobal()->resources.root_res);
	ConsoleMessage("%25s %d\n","resource file count", count);
}

/**
 * resource_tree_list_files: print all the files loaded via resources
 * (local files or URL resources cached as temporary files).
 */
void resource_tree_list_files(int level, resource_item_t *root)
{
#define spacer	for (lc=0; lc<level; lc++) printf ("\t");
	int lc;

	if (root == NULL) return; 
	if (level == 0) printf("\nResource file list:\n");

	spacer printf("%s\n", root->actual_file);
	ml_foreach(root->children, resource_tree_list_files(-1, ml_elem(__l)));
}

char *resourceTypeToString(int type) {
	switch (type) {
		case rest_invalid: return "rest_invalid";
		case rest_url: return "rest_url";
		case rest_file: return "rest_file";
		case rest_multi: return "rest_multi";
		case rest_string : return "rest_string ";
		default: return "resource OUT OF RANGE";
	}
}


char *resourceStatusToString(int status) {
	switch (status) {
		case ress_none: return "ress_none";
		case ress_starts_good: return "ress_starts_good";
		case ress_invalid: return "ress_invalid";
		case ress_downloaded: return "ress_downloaded";
		case ress_failed: return "ress_failed";
		case ress_loaded: return "ress_loaded";
		case ress_not_loaded: return "ress_not_loaded";
		case ress_parsed: return "ress_parsed";
		case ress_not_parsed: return "ress_not_parsed";
		default: return "resource OUT OF RANGE";
	}
}

char *resourceMediaTypeToString (int mt) {
	switch (mt) {
		case  resm_unknown: return " resm_unknown";
		case  resm_vrml: return " resm_vrml";
		case  resm_x3d: return " resm_x3d";
		case  resm_image: return " resm_image";
		case  resm_movie: return " resm_movie";
		case  resm_pshader: return " resm_pshader";
		case  resm_fshader: return " resm_fshader";
		case  resm_x3z: return " resm_x3z";
		default: return "resource OUT OF RANGE";
	}
}



#define SLASHDOTDOTSLASH "/../"
#if defined(_MSC_VER) || defined(_ANDROID) || defined(ANDROIDNDK)
#define rindex strrchr
#endif
void removeFilenameFromPath (char *path) {
	char *slashindex;
	char *slashDotDotSlash;

	/* and strip off the file name from the current path, leaving any path */
	slashindex = (char *) rindex(path, ((int) '/'));
	if (slashindex != NULL) {
		/* slashindex ++; */ /* <msvc DO NOT> leave the slash there */
		*slashindex = 0;
	} else {path[0] = 0;}
	/* printf ("removeFielnameFromPath, parenturl is %s\n",path); */

	/* are there any "/../" bits in the path? if so, lets clean them up */
	slashDotDotSlash = strstr(path, SLASHDOTDOTSLASH);
	while (slashDotDotSlash != NULL) {
		char tmpline[2000];
		/* might have something like: _levels_plus/tiles/0/../1/../1/../2/../ */
		/* find the preceeding slash: */
		*slashDotDotSlash = '\0';
		/* printf ("have slashdotdot, path now :%s:\n",path); */

		slashindex = (char *)rindex(path, ((int) '/'));
		if (slashindex != NULL) {
			
			slashindex ++;
			*slashindex = '\0';
			slashDotDotSlash += strlen(SLASHDOTDOTSLASH);
			strcpy(tmpline,path);
			/* printf ("tmpline step 1 is :%s:\n",tmpline); */
			strcat (tmpline, slashDotDotSlash);
			/* printf ("tmpline step 2 is :%s:\n",tmpline); */
			strcpy (path, tmpline);
			slashDotDotSlash = strstr(path, SLASHDOTDOTSLASH);
			/* printf ("end of loop, path :%s: slashdot %u\n",path,slashDotDotSlash); */


		}
	}
}


/* is this a gzipped file? if so, unzip the text and replace the original with this. */
static void possiblyUnzip (openned_file_t *of) {
#if !(defined(IPHONE) || defined(_ANDROID))
	if (of->fileData == NULL) return;
	if (of->fileData[0] == '\0') return;
	if (of->fileData[1] == '\0') return;
        if (((unsigned char) of->fileData[0] == 0x1f) && ((unsigned char) of->fileData[1] == 0x8b)) {
		#define GZIP_BUFF_SIZE 2048

		gzFile *source;
		FILE *dest;
		char buffer[GZIP_BUFF_SIZE];
		int num_read = 0;
		openned_file_t *newFile;

		char *tempname; // [1000];

		/* make a temporary name for the gunzipped file */
        // sprintf (tempname, "%s",tempnam(gglobal()->Mainloop.tmpFileLocation,"freewrl_tmp")); 
		tempname = tempnam(gglobal()->Mainloop.tmpFileLocation, "freewrl_tmp");

		/* read in the text, unzip it, write it out again */
		source = gzopen(of->fileFileName,"rb");
		dest = fopen(tempname,"wb");

		if (!source || !source) {
			ConsoleMessage ("unable to unzip this file: %s\n",of->fileFileName);
			printf ("wow - problem\n");
		}

		while ((num_read = gzread(source, buffer, GZIP_BUFF_SIZE)) > 0) {
			fwrite(buffer, 1, num_read, dest);
		}

		gzclose(source);
		fclose(dest);

		/* read in the unzipped text... */
		newFile = load_file((const char *) tempname);
		UNLINK(tempname);

		if (newFile->fileData == NULL) {
			ConsoleMessage ("problem re-reading gunzipped text file");
			return;
		}

		/* replace the old text with the unzipped; and clean up */
		FREE_IF_NZ(of->fileData);
		of->fileData = newFile->fileData;
/* seems odd that we wouldn't need to also update the fileDataSize, like so:
		of->fileDataSize = newFile->fileDataSize; */
		FREE_IF_NZ(newFile);
		unlink (tempname);
	}
#endif
}

bool resource_is_root_loaded()
{
	return ((gglobal()->resources.root_res != NULL) && (((resource_item_t*)gglobal()->resources.root_res)->status == ress_parsed));
}

/**
 *   For keeping track of current url (for parsing / textures).
 *
 * this is a Vector; we keep track of n depths.
 */

/* keep the last base resource around, for times when we are making nodes during runtime, eg
   textures in Background nodes */

void pushInputResource(resource_item_t *url) 
{
	presources p = gglobal()->resources.prv;
	DEBUG_MSG("pushInputResource current Resource is %s", url->parsed_request);

            
        
	/* push this one */
	if (p->resStack==NULL) {
		p->resStack = newStack (resource_item_t *);
	}

    /* is this an EAI/SAI request? If not, we don't push this one on the stack */
    /*
    if (url->parsed_request != NULL)
        if (strncmp(url->parsed_request,EAI_Flag,strlen(EAI_Flag)) == 0) {
            DEBUG_MSG("pushInputResource, from EAI, ignoring");
            return;
        }
*/
	stack_push (resource_item_t*, p->resStack, url);
    DEBUG_MSG("pushInputResource, after push, stack size %d",vectorSize(p->resStack));
}

void popInputResource() {
	resource_item_t *cwu;
	presources p = gglobal()->resources.prv;

	/* lets just keep this one around, to see if it is really the bottom of the stack */
    DEBUG_MSG("popInputResource, stack size %d",vectorSize(p->resStack));
    
	cwu = stack_top(resource_item_t *, p->resStack);

	/* pop the stack, and if we are at "nothing" keep the pointer to the last resource */
	stack_pop((resource_item_t *), p->resStack);

	if (stack_empty(p->resStack)) {
		DEBUG_MSG ("popInputResource, stack now empty and we have saved the last resource\n");
		p->lastBaseResource = cwu;
	} else {
		cwu = stack_top(resource_item_t *, p->resStack);
        DEBUG_MSG("popInputResource, cwu = %p",cwu);
		DEBUG_MSG("popInputResource before pop, current Resource is %s\n", cwu->parsed_request);
	}
}

resource_item_t *getInputResource()
{
	resource_item_t *cwu;
	presources p = gglobal()->resources.prv;

    
	DEBUG_MSG("getInputResource \n");
	if (p->resStack==NULL) {
		DEBUG_MSG("getInputResource, stack NULL\n");
		return NULL;
	}

	/* maybe we are running, and are, say, making up background textures at runtime? */
	if (stack_empty(p->resStack)) {
		if (p->lastBaseResource == NULL) {
			ConsoleMessage ("stacking error - looking for input resource, but it is null");
		} else {
			DEBUG_MSG("so, returning %s\n",p->lastBaseResource->parsed_request);
		}
		return p->lastBaseResource;
	}


	cwu = stack_top(resource_item_t *, p->resStack);
	DEBUG_MSG("getInputResource current Resource is %lu %lx %s\n", (unsigned long int) cwu, (unsigned long int) cwu, cwu->parsed_request);
	return cwu;
}

//used by FEGF configs in frontend
char* fwl_resitem_getURL(void *resp){
	resource_item_t *res = (resource_item_t *)resp;
	return res->parsed_request;
}
void fwl_resitem_setActualFile(void *resp, char *fname){
	resource_item_t *res = (resource_item_t *)resp;
	res->actual_file = STRDUP(fname);
	if(strcmp(res->actual_file,res->parsed_request)){
		//it's a temp file 
		s_list_t *item;
		item = ml_new(res->actual_file);
		if (!res->cached_files)
			res->cached_files = (void *)item;
		else
			res->cached_files = ml_append(res->cached_files,item);
	}
}
char* fwl_resitem_getTempDir(void *resp){
	resource_item_t *res = (resource_item_t *)resp;
	return res->temp_dir;
}
void fwl_resitem_enqueuNextMulti(void *resp){
	resource_item_t *res = (resource_item_t *)resp;
	int more_multi = (res->status == ress_failed) && (res->m_request != NULL);
	if(more_multi){
		//still some hope via multi_string url, perhaps next one
		res->status = ress_invalid; //downgrade ress_fail to ress_invalid
		res->type = rest_multi; //should already be flagged
		//must consult BE to convert relativeURL to absoluteURL via baseURL 
		//(or could we absolutize in a batch in resource_create_multi0()?)
		resource_identify(res->parent, res); //should increment multi pointer/iterator
		frontenditem_enqueue(ml_new(res));
	}
}
char *strBackslash2fore(char *);
//int file2blob(resource_item_t *res);
void fwl_resitem_setLocalPath(void *resp, char* path){
	int delete_after_load;
	resource_item_t *res = (resource_item_t *)resp;
	res->status = ress_downloaded;
	res->actual_file = strBackslash2fore(STRDUP(path));
	delete_after_load = 1;
	if (delete_after_load){
		//warning this will delete the actual_file setLocalPath is for downloaded/copied/cached files only, 
		//not direct intranet files as with desktop.c
		s_list_t *item;
		item = ml_new(res->actual_file);
		if (!res->cached_files)
			res->cached_files = (void *)item;
		else
			res->cached_files = ml_append(res->cached_files, item);
	}
	res->_loadFunc = (void *)file2blob; //msvc can also do &file2blob
}
int	fwl_resitem_getStatus(void *resp){
	resource_item_t *res = (resource_item_t *)resp;
	return res->status;
}
void fwl_resitem_setStatus(void *resp, int status) {
	resource_item_t *res = (resource_item_t *)resp;
	res->status = status;
}

int	fwl_resitem_getType(void *resp){
	resource_item_t *res = (resource_item_t *)resp;
	return res->type;
}
int	fwl_resitem_getMediaType(void *resp){
	resource_item_t *res = (resource_item_t *)resp;
	return res->media_type;
}
void fwl_resitem_setDownloadThread(void *resp, void *thread){
	resource_item_t *res = (resource_item_t *)resp;
	res->_loadThread = (pthread_t*)thread;
}
void * fwl_resitem_getDownloadThread(void *resp){
	resource_item_t *res = (resource_item_t *)resp;
	return res->_loadThread;
}
void * fwl_resitem_getGlobal(void *resp){
	resource_item_t *res = (resource_item_t *)resp;
	return res->tg;
}