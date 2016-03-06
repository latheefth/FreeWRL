#ifdef DISABLER		
#ifdef FRONTEND_GETS_FILES
		void (*_frontEndOnResourceRequiredListener)(char *);
#endif
		void (*_frontEndOnX3DFileLoadedListener)(char *);
#endif		
#ifdef FRONTEND_GETS_FILES
void fwg_setFrontEndOnResourceRequiredListener(void (*frontEndOnResourceRequiredListener)(char *));
#endif //FRONTEND_GETS_FILES

#ifdef FRONTEND_GETS_FILES
/* these variables are used on return of data from front end, and are passed on to create_openned_file */
static char *fileText = NULL;
static char *fileToGet = NULL;
static int frontend_return_status = 0;
static int fileSize = 0;
static int imageWidth;
static int imageHeight;
static bool imageAlpha;

static pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
#define LOCK_LOAD_FILE_FUNCTION pthread_mutex_lock( &mutex1 );
#define UNLOCK_LOAD_FILE_FUNCTION pthread_mutex_unlock(&mutex1);

static pthread_mutex_t  getAFileLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t waitingForFile = PTHREAD_COND_INITIALIZER;
#define MUTEX_LOCK_FILE_RETRIEVAL               pthread_mutex_lock(&getAFileLock);
#define MUTEX_FREE_LOCK_FILE_RETRIEVAL       pthread_mutex_unlock(&getAFileLock);
#define WAIT_FOR_FILE_SIGNAL		pthread_cond_wait(&waitingForFile,&getAFileLock);
#define SEND_FILE_SIGNAL		pthread_cond_signal(&waitingForFile);

/* accessor functions */
/* return the filename of the file we want. */

char *fwg_frontEndWantsFileName() {
	//if (fileToGet != NULL) printf ("fwg_frontEndWantsFileName called - fileName currently %s\n",fileToGet);
	return fileToGet;
}

void fwg_frontEndReturningData(char* fileData,int len,int width,int height,bool hasAlpha) {

	MUTEX_LOCK_FILE_RETRIEVAL
    
    //ConsoleMessage ("fwg_frontEndReturningData, len %d",len);
	/* did we get data? is "len" not zero?? */
	if (len == 0) {
		// printf ("fwg_frontEndReturningData, returning error\n");
		//frontend_return_status = -1;
		fileText = NULL;
		fileSize = 0;

	} else {
		// printf ("fwg_frontEndReturningData, returning ok\n");
    		/* note the "+1" ....*/
        FREE_IF_NZ(fileText);
        
		fileText = MALLOC (char *, len+1);
        if (NULL != fileText)
        {
            memcpy (fileText, fileData, len);
        }
		fileSize = len;
		imageWidth = width;
		imageHeight = height;
		imageAlpha = hasAlpha;
    
	    /* ok - we do not know if this is a binary or a text file,
	       but because we added 1 to it, we can put a null terminator
	       on the end - that will terminate a text string, but will
	       not affect a binary file, because we have the binary data
	       and binary length recorded. */

	    fileText[len] = '\0';  /* the string terminator */
         //printf ("fwg_frontEndReturningData: returning data, but fileToGet setting to NULL, was %s\n",fileToGet);

		frontend_return_status = 0;
		/* got the file, send along a message */
	}

    
	SEND_FILE_SIGNAL

	MUTEX_FREE_LOCK_FILE_RETRIEVAL
}

#endif
openned_file_t* load_file(const char *filename)
{
	openned_file_t *of;
	if (NULL == filename) {
		return NULL;
	}

    of = NULL;


    
    
    
	DEBUG_RES("loading file: %s pthread %p\n", filename,pthread_self());
    //printf ("load_file, fileToGet %s, load_file %s thread %ld\n",fileToGet,filename,pthread_self());
    
#ifdef FRONTEND_GETS_FILES

 
    
    LOCK_LOAD_FILE_FUNCTION


    MUTEX_LOCK_FILE_RETRIEVAL
    
	//JAS - we keep this around until done with resource FREE_IF_NZ(fileText);


    FREE_IF_NZ(fileToGet);
    fileToGet = STRDUP(filename);

    ttglobal tg = gglobal();
    if (tg->ProdCon._frontEndOnResourceRequiredListener) {
    		tg->ProdCon._frontEndOnResourceRequiredListener(fileToGet);
    }

    WAIT_FOR_FILE_SIGNAL

	MUTEX_FREE_LOCK_FILE_RETRIEVAL

	FREE_IF_NZ(fileToGet);
	fileToGet = NULL; /* not freed as only passed by pointer */

	if(frontend_return_status == -1) of = NULL;
    else of = create_openned_file(STRDUP(filename), -1, fileSize, fileText, imageHeight, imageWidth, imageAlpha);
    FREE_IF_NZ(fileText);
    fileText = NULL;
    UNLOCK_LOAD_FILE_FUNCTION  
    return of;
    
#else //FRONTEND_GETS_FILES 



#if defined(FW_USE_MMAP)
#if !defined(_MSC_VER)
	/* UNIX mmap */
	of = load_file_mmap(filename);
#else
	/* Windows CreateFileMapping / MapViewOfFile */
	of = load_file_win32_mmap(filename);
#endif
#else
	/* Standard read */
	of = load_file_read(filename);
#endif
	DEBUG_RES("%s loading status: %s\n", filename, BOOL_STR((of!=NULL)));
	return of;
#endif //FRONTEND_GETS_FILES
}

#if defined(FRONTEND_GETS_FILES)
	res->actual_file = STRDUP(res->parsed_request);
#ifdef FRONTEND_GETS_FILES
void fwg_setFrontEndOnX3DFileLoadedListener(void (*frontEndOnX3DFileLoadedListener)(char *))
{
	ttglobal tg = gglobal();
	struct tProdCon *t = &tg->ProdCon;
	t->_frontEndOnX3DFileLoadedListener = frontEndOnX3DFileLoadedListener;
}

void fwg_setFrontEndOnResourceRequiredListener(void (*frontEndOnResourceRequiredListener)(char *))
{
	ttglobal tg = gglobal();
	struct tProdCon *t = &tg->ProdCon;
	t->_frontEndOnResourceRequiredListener = frontEndOnResourceRequiredListener;
}
#endif //FRONTEND_GETS_FILES

#ifdef FRONTEND_GETS_FILES
/**
 *   resource_fetch: download remote url or check for local file access.
 */
bool resource_fetch(resource_item_t *res)
{
	DEBUG_RES("fetching resource: %s, %s resource %s\n", resourceTypeToString(res->type), resourceStatusToString(res->status) ,res->URLrequest);

	ASSERT(res);

	switch (res->type) {

	case rest_invalid:
		res->status = ress_invalid;
		ERROR_MSG("resource_fetch: can't fetch an invalid resource: %s\n", res->URLrequest);
		break;

	case rest_url:
		switch (res->status) {
		case ress_none:
		case ress_starts_good:
			DEBUG_RES ("resource_fetch, calling download_url\n");
			download_url(res);
			break;
		default:
			/* error */
			break;
		}
		break;

	case rest_file:
		switch (res->status) {
		case ress_none:
		case ress_starts_good:
			/* SJD If this is a PROTO expansion, need to take of trailing part after # */
#ifdef FRONTEND_GETS_FILES
			res->status = ress_downloaded;
			res->actual_file = STRDUP(res->parsed_request);
			if (res->media_type == resm_image) {
				res->_loadFunc = (int(*)(void*))imagery_load;
			} else {
				res->_loadFunc = (int(*)(void*))resource_load;
			}

			/* copy the name out, so that Anchors can go to correct Viewpoint */
			res->afterPoundCharacters = '\0';
#endif //FRONTEND_GETS_FILES

			break;
		default:
			/* error */
			break;
		}
		break;

	case rest_multi:
	case rest_string:
		/* Nothing to do */
		break;
	}
	DEBUG_RES ("resource_fetch (end): network=%s type=%s status=%s"
		  " request=<%s> base=<%s> url=<%s> [parent %p, %s]\n",
		  BOOL_STR(res->network), resourceTypeToString(res->type),
		  resourceStatusToString(res->status), res->URLrequest,
		  res->URLbase, res->parsed_request,
		  res->parent, (res->parent ? res->parent->URLbase : "N/A"));
	return (res->status == ress_downloaded);
}
#endif

#ifdef FRONTEND_GETS_FILES
	case ress_downloaded:
		of = load_file(res->actual_file);

		// of should never be null....

		// printf ("XXXXX load_file, of filename %s, fd %d, dataSize %d, data %p\n",of->fileFileName, of->fileDescriptor, of->fileDataSize, of->fileData);

		if (of) {
			if (of->fileData) {

			res->status = ress_loaded;
			res->openned_files = ml_append( (s_list_t *) res->openned_files,
							ml_new(of) );

			/* If type is not specified by the caller try to identify it automatically */
			if (res->media_type == resm_unknown) {
				resource_identify_type(res);
			}
			} else {
			res->status = ress_not_loaded;
			ERROR_MSG("resource_load: can't load file: %s\n", res->actual_file);

			// force this to return false
			of = NULL;

			}

		} else {

			// printf ("resource load, of failed, but fwg_frontEndWantsFilename is %s\n",fwg_frontEndWantsFileName());

			if (fwg_frontEndWantsFileName() != NULL) {
				/* printf ("resource still loading, lets yield here\n"); */
			} else {


			res->status = ress_not_loaded;
			ERROR_MSG("resource_load: can't load file: %s\n", res->actual_file);
		}
		}

		break;


#else //FRONTEND_GETS_FILES

#ifdef DISABLER
void resource_on_did_not_parse(resource_item_t *res) {
	s_list_t *of, *cf;
	of = (s_list_t *) res->openned_files;
	if (!of) {
		/* error */
		return;
	}

	ml_foreach(of, close_openned_file(__l->elem));

	/* Remove cached file ? */
	cf = (s_list_t *) res->cached_files;
	if (cf) {
		/* remove any cached file:
		   TODO: reference counter on cached files...
		 */
		ml_foreach(cf, resource_remove_cached_file(__l->elem));
	}

	/* free the actual file  */
	FREE(res->actual_file);
	res->actual_file = NULL;
}
#endif
#ifdef DISABLER            
            resource_on_did_not_parse(res);
#else            
#ifdef DISABLER		
			resource_on_did_not_parse(res);
#else			
#ifdef DISABLER
	case rest_multi:
		/* Free the list */
		ml_delete_all2(res->m_request, &_resourceFreeCallback);
		res->m_request = NULL;
		break;
#endif

#ifdef DISABLER
void _resourceFreeCallback(void *resource)
{
    FREE_IF_NZ(resource);
}

void close_openned_file(openned_file_t *file) {
	if (file->fileDescriptor != 0) {
		close(file->fileDescriptor );
	}
    FREE_IF_NZ(file->fileData);
    file->fileData = NULL;
    FREE_IF_NZ(file->fileFileName);
    file->fileFileName = NULL;
}
#endif
#ifdef DISABLER		 
		ml_foreach(cf, resource_remove_cached_file(__l->elem));
#else
#ifdef DISABLER	
	{
		s_list_t *of;

		of = (s_list_t *) res->openned_files;
		if (NULL != of)
			ml_foreach(of, close_openned_file(__l->elem));
		FREE_IF_NZ(of);
		res->openned_files = NULL;
	}
#endif
#ifdef DISABLER
		s_list_t *of = (s_list_t*)ofv;	
		ml_foreach(of, PRINTF("%s ", (char *) ((openned_file_t *)ml_elem(__l))->fileFileName));
#else		
