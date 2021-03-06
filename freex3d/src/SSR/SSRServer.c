
/*	win32: include the headers of your operating system that define the size_t, fd_set, socklen_t and struct sockaddr data types and define MHD_PLATFORM_H
	linux: don't define MHD_PLATFORM_H and it will figure it out
	license MIT or similar - hack away, have fun

	SSR - server-side rendering - the user drags on a webgl canvas featuring a simple grid, and when
		they lift their mousebutton/touchfinger the client code ajaxes (asynchronous javascript and xml, via xmlhttprequest)
		the viewpoint pose to the server
		the server reponds by correcting the pose for gravity/collision and sends back
		then the client sends the corrected pose and asks via  for a snapshot
		the server takes a screen snapshot and sends back to clien
		the client shows the snapshot over the grid and waits for the next mousedown/touch to show the grid again
	SSRServer.exe this commandline program that takes Port (ie 8080 or your choice) on the command line,
		- runs linked-in libmicrohttpd C web server to listen and call functions below
		- you need to run this program in src/SSR/public to server SSRClient.html
		- serves SSRClient.html when in a browser you go: http://localhost:8080/SSRClient.html or -via your noip/dyndns hostname http://myhostname:8080/SSRClient.html
	Current state Feb 25, 2015:
		* works a bit on win32 and in up-to-date desktop web browsers IE, Chrome, FF
		x doesn't work in iPhone, BBPlaybook webbrowsers 
				- needs shims/polyfills in SSRClient.html for a few modern things window.requestNextAnimationFrame and xhr2 xmlhttprequest JSON/blob payloads
		* tested with free noip.com account, which re-directed to home server computer behind DSL modem
			- DSL modem had settings Advanced > DDNS (dynamic dns) > no-ip auto-update feature,
				and PortForwarding (set to our chosen PORT  8080 on WAN and LAN)
	Apr 10, 2015:
		* ZoneBalancer:
			- will be adding a geographic zoneserver/loadbalancer/reverseproxy/gateway
			- ZoneBalancer acts as a gateway between LAN and internet
			- gets request from an internet client
			- sniffs the request content for geographic coords
			- looks up in a table of {polygon,IP:PORT} using point-in-polygon algorithm 
				to determine which zone polygon the client's viewer is in, then uses 
				the associated IP and port number to call regular SSRserver,
				wait for response, then relay response back to internet client
			- benefit: more CPUs, GPUs and memory slots can be used for very large 3D datasets
				allowing the solution to scale up using ordinary desktop computers which these days 
				have 32GB RAM limit (4x8GB). So if your data has a 96GB footprint in memory, you could use
				3 SSRservers and 1 ZoneBalancer to distribute the load
			- for programmer convenience the ZoneBalancer will be just a runtime configuration of SSRserver
		In SSRserver.c response handler pseudocode:
		if( static html) {
			serve directly (ie SSRClient.html)
		} else if working as SSRserver {
			render snapshot and return response - the original SSRserver configuration
		} else if working as zone balancer {
			1. sniff request for geographic coords of client's ViewPose/avatar
			2. do point-in-polygon on a list of polygons to determine which zone the avatar is in
				and which IP:Port / SSRServer instance to relay to
			3. open a TCP connection using sockets, and forward the request
			4. wait for response from SSR on the ZoneBalancer's request handler thread
			5. when response arrives, copy it to the zoneBalancer's response and return response to client
		}
	May 26, 2015:
		revamping SSR_API using json (via cson lib/functions)
		making SSRservers into a tree-like cascade, with nodes and leafs of overviews and LOD (level of detail)
		SSRClient.html -- zoneserver -- SSRserver - leaf scene
									 -- SSRserver - leaf
									 -- SSRserver - leaf
											-- SSRserver - leaf
											-- SSRserver - leaf
												 -- SSRserver - leaf
		each SSRserver instance will be responsible for knowing its:
			 service volume -extent and/or polygon extrusion (polgon2D, -height, +height) and/or shape
			 placemark tree (placemark is "name",x,y or more generally "name",level, pose {.position, .orientation))
				including aggregating child SSR placemarks
				so placemarks show up as clickable links or pulldown list in html client
			 transform, if any 
				- usually just optional x,y offset of scene origin, 
					so geonodes aren't needed in the scene to get double SFVec3d, 
					floats / SFVec3f will be sufficient in the scene, and doubles added/subtracted by the SSR code
					so html client uses absolute coordinates (ie GIS/geographic/mapping-plane coords) in doubles
		each SSR html client being sessionless, will transmit its desired LOD level and its position so ZoneServer 
			will know which child SSR to relay it to, and avatarSize.height so libfreewrl will know how to 
			apply gravity to the avatar before rendering the snapshot

Proposed SSR API JSON
Get: (POST)
- initial_pose() 
	//generally, its the 'most absolute' pose of the (currently bound) viewpoint as positioned in the scene
	// Client -- ZoneServer -- SSR +shifts -- libfreewrl_scene geoViewpoint ? geoCoords : (ViewMatrix x viewpoint.position)
	//										  where ViewMatrix is the sceene-root to Viewpoint part of modelView matrix
	//another client may have moved the view, so this would make sure its the .pos, .quat = 0 or inital
	//or if no viewpoint in the scene freewrl and vivaty make viewpoint.pos.z = 10, then at least the client will know z=10 and can sync
	request: {"command":"initial_pose", "level":0} 
	response: {"command":"initial_pose", "status":"OK", "pose": { "position":[x,y,z], "orientation": [q0,q1,q2,q3]}
- levels available 
	//a fuzzy concept, of whether an overview or more detailed scene is desired at a given position
	request: {"command":"levels_available"}
	response: {"command":"levels_available", "status":"OK", "value":1}
- placemark tree
	//html client can request placemarks and get a tree list, showing overviews and more detailed LOD sub-scenes 
	// and can click on a placemark like a link, or choose from a pulldown list, to navigate to the placemark's pose
	request: {"command":"placemark_tree"} 
	response: {"command":" placemark_tree", "status":"OK", 
	"placemarks": [
		{..tree..},
		{"placemark": {
			"name": "overzicht", 
			"pose":{ "position":[x,y,z], "orientation": [q0,q1,q2,q3]},

			"children":[
				{"placemark": {
					"name": "Afrikaanderwuk", 
					"pose":{ "position":[x,y,z], "orientation": [q0,q1,q2,q3]}
				}
			]
			}
		},
		{..tree..}.
	]}
- adjusted pose 
	- needs to deliver previous pose, so wall and ground penetration can be done, and an adjusted pose returned to client
	July 3, 2015 update: forget wall penetration for now, and forget previous pose. Problem if previous and current pose 
		are in different leaf scenes, freewrl wall penetration calculation can't happen
	{"command":"posepose", "level":0, "height":1234.567, 
		"pose":{ "position":[x,y,z], "orientation": [q0,q1,q2,q3]}}
	{"command":"posepose", "status":"OK", 
		"pose":{ "position":[x,y,z], "orientation": [q0,q1,q2,q3]}}
- snapshot
	- assumes the pose has been adjusted already, because the return is not in json. Its a blob representing the image.
	{"command":"posesnapshot", "level":0, "height":1234.567, 
		"pose":{ "position":[x,y,z], "orientation": [q0,q1,q2,q3]}}
	blob = .response; //not json

- service volume
	//the client talks to a single ZoneServer, and the Zoneserver sniffs the coordinates
	//of the client's request to decide which SSRserver instance (host:port) to forward the request to
	//zoneserver decides by testing the client's position against each SSR's service volume
	// - a kind of advanced point-in-polygon test
	// EITHER during startup, ZoneServer will ask each SSR once for its service volume
	// OR asks just for the UNION of extents of its leaf and sub-SSRs on startup,
	//   and thereafter when in doubt asks each SSR to give detailed check if
	//   a given position is in its (more detailed) service volume
	//html client won't ask or perform tests. It assumes its inside the service volume of 
	// the zoneserver (or directly SSR) it's talking to
	//so the default action by html client and ssr server ie when no intermediaries, is to 
	//assume the client is within the extent of the SSRserver leaf scene
	{"command":"service_volume", "level":0, "position":[x,y,z] }	
	{"command":"service_volume", "status":"OK", "volumes": [
		"extent":[x,y,z,x,y,z],
		"extrusion":{ "polygon":[x,y,z,x,y,z...x,y,z], "bottom":-z, "top":+z},
		"shape": {"indexes":[0 1 2 -1 3 4 5 -1], "vertices":[x,y,z,x,y,z...]}
		]
	}
- inside
	//if the service volume responses above are limited to Extents and extent testing, then
	// we would need another command to ask each extent-success SSR to check
	// again against finer granularity service volume definition, 
	// such as polygon extrusion or general surface shape
	{"command":"inside", "level":0, "position":[x,y,z] }
	{"command":"inside", "isInside":true }

- maybe fov (and aspect)?
	//if the scene has a navigationInfo it can adjust the fov
	// so to synchronize the client will need to know fov to set it in webgl
	// in theory if there's a bunch of SSRs running with different scenes, the fov could be different in each
	{"command":"fov", "level":0, "position":[x,y,z] }	
	{"command":"fov", "status":"OK", "fov":123.456}	
- quit	
	//disabled for internet-facing outer SSR/Zoneserver, just for use by internal 
	//option: put on another port, contrlled by a per-server-node launcher utility
	{"command":"quit"}
	{"command":"quit", "status":"OK"}

x Set: (POST) - there's no such thing because we are (currently) sessionless.
	- resend any client-specific data with any needy request

July 3, 2015 update
	SSRClient and SSRServer can do yaw, pitch, height/world-z, vp and world xy.
	Pitch = 0 is looking Nadir/down.
	Scene can have a Transform with rotations, translations, and Viewpoint can have .postion, .orientation.
	Scale in the transform is untested, and client can't/doesn't do scale, just translation and orientation of viewpoint.
*/


#ifdef _MSC_VER
//WIN32
#define MHD_PLATFORM_H WIN32
#include <sys/types.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <ws2tcpip.h>
#define ssize_t size_t
#define off_t ptrdiff_t
#include <sys/stat.h>
#include <limits.h> 
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#define R_OK 4
#define X_OK 4
#define O_NONBLOCK 0
#define O_BLOCK 0
#define SSIZE_MAX 100000000L
#define close _close
#define open _open
#define read _read
#define write _write
//END WIN32
#endif
// in theory scene_path could/should be a command line arg, or something the server helps the user upload or select on another page
#define scene_path "http://dug9.users.sourceforge.net/web3d/townsite_2014/townsite.x3d" //"C:/Users/Public/dev/source2/placeholders/townsite_2014/townsite.x3d"
#include <cdllFreeWRL.h>
#define SSR_SERVER 1
#include "../lib/SSRhelper.h"
#include "cson/fw_cson.h"
int run_fw = 1;
void *fwctx = NULL;
int runFW(char *url){
	if(run_fw){
		fwctx = dllFreeWRL_dllFreeWRL();
		dllFreeWRL_commandline(fwctx,"pin,FF");
		dllFreeWRL_commandline(fwctx,"set_keyval,SSR,true");
		dllFreeWRL_onInit(fwctx,400,300,NULL,FALSE,FALSE);
		dllFreeWRL_onLoad(fwctx,url); 
	}
	return 0;
}
int stopFW(){
	if(run_fw)
		dllFreeWRL_onClose(fwctx);
	return 0;
}


//===========CSON JSON>>>>>>>>>>>>>>>>>>>>
//typedef int (*cbkey_cson)(const char *key, int index, cson_value *val, void *cbdata);
//typedef int (*cbval_cson)(cson_value *val, int index, void *cbdata);
struct keyval {
	char *key;
	cson_value *cv;
};
typedef struct walk_cbdata {
	int (*fkey)(const char *key, int index, cson_value *val, void *cbdata);
	int (*fval)(cson_value *val, int index, void *cbdata);
	void *data;
	//int level; //could increment before descending, decrement after ascending, in case a cb wants it
	//cson_object *parent; //could set before descending in case a cb wants it
	void *arr; //points to array
	int arrtype; //0 double, 1 int, 2 char*
} walk_cbdata;
int walk_array_cson(cson_array *arr, void *cbdata);


int walk_obj_cson(cson_object *obj, void *cbdata){
	int i;
	cson_object_iterator iter;
	walk_cbdata *wcbd = (walk_cbdata*)cbdata;
	int rc = cson_object_iter_init( obj, &iter );
	if( 0 != rc ) { 
		printf("error, but can only fail if obj is NULL\n");
	}
	cson_kvp * kvp; // key/value pair
	i = 0;
	while( (kvp = cson_object_iter_next(&iter)) )
	{
		cson_string const * ckey = cson_kvp_key(kvp);
		cson_value * v = cson_kvp_value(kvp);
		rc = wcbd->fkey(cson_string_cstr(ckey),i,v,cbdata);
		if(!rc)
			rc = wcbd->fval(v,0,cbdata);
		if(rc) break;
		i++;
	}
	return rc;
}
int walk_array_cson(cson_array *arr, void *cbdata){
	int len, i, rc;
	walk_cbdata *wcbd = (walk_cbdata*)cbdata;
	len = cson_array_length_get(arr);
	rc = 0;
	for( i = 0; i < len; ++i ) {
		cson_value *vi;
		vi = cson_array_get( arr, i );
		rc = wcbd->fval(vi,i,cbdata);
		if(rc) break;
	}
	return rc;
}
int cb_print_key(const char *key, int index, cson_value *val, void *cbdata){
	int indent;
	walk_cbdata *wcbd = (walk_cbdata*)cbdata;
	indent = *((int*)(wcbd->data));
	if(index) printf(",");
	printf("\"%s\":", key );
	return 0;
}
int cb_print_val(cson_value *val, int index, void *cbdata){
	int rc;
	int indent;
	walk_cbdata *wcbd = (walk_cbdata*)cbdata;

	indent = *((int*)wcbd->data);
	rc = 0;
	if(index) printf(",");
	switch(cson_value_type_id(val))
	{
	case CSON_TYPE_UNDEF:
	case CSON_TYPE_NULL:
		printf("null");
		break;
	case CSON_TYPE_BOOL:
		{
			if(cson_value_get_bool(val))
				printf("true");
			else
				printf("false");
		}
		break;
	case CSON_TYPE_INTEGER:
		{
			cson_int_t ii;
			rc = cson_value_fetch_integer(val, &ii );
			printf("%lld",ii);
		}
		break;
	case CSON_TYPE_DOUBLE:
		{
			cson_double_t dd;
			rc = cson_value_fetch_double(val, &dd );
			printf("%lf",dd);
		}
		break;
	case CSON_TYPE_STRING:
		{
			cson_string *str;
			rc = cson_value_fetch_string(val, &str );
			printf("\"%s\"",cson_string_cstr(str));
		}
		break;
	case CSON_TYPE_OBJECT:
		{
			cson_object * obji = cson_value_get_object(val);
			printf("{");
			rc = walk_obj_cson(obji,cbdata);
			printf("}");
		}
		break;
	case CSON_TYPE_ARRAY:
		{
			cson_array *ar;
			rc = cson_value_fetch_array(val,&ar);
			if(!rc){
				printf("[");
				rc = walk_array_cson(ar,cbdata);
				printf("]");
			}
		}
		break;
	default:
		break;
	}
	return rc;
}
int print_json_tree(cson_object *obj){
	walk_cbdata cbdata;
	int indent = 0;
	cbdata.data = &indent;
	cbdata.fkey = cb_print_key;
	cbdata.fval = cb_print_val;
	printf("{");
	walk_obj_cson(obj,&cbdata);
	printf("}");
	return 0;
}
int cb_sniff_key(const char *key, int index, cson_value *val, void *cbdata){
	struct keyval *kv;
	walk_cbdata *wcbd = (walk_cbdata*)cbdata;
	kv = (struct keyval *)(wcbd->data);
	if(!strcmp(kv->key,key))
		kv->cv = val;
	return 0;
}
int cb_sniff_val(cson_value *val, int index, void *cbdata){
	int rc;
	walk_cbdata *wcbd = (walk_cbdata*)cbdata;

	rc = 0;
	switch(cson_value_type_id(val))
	{
	case CSON_TYPE_OBJECT:
		{
			cson_object * obji = cson_value_get_object(val);
			rc = walk_obj_cson(obji,cbdata);
		}
		break;
	case CSON_TYPE_ARRAY:
		{
			cson_array *ar;
			rc = cson_value_fetch_array(val,&ar);
			if(!rc){
				rc = walk_array_cson(ar,cbdata);
			}
		}
		break;
	default:
		break;
	}
	return rc;
}
int sniff_json_tree(char *key, cson_object *obj){
	walk_cbdata cbdata;
	struct keyval kv;
	kv.key = key;
	kv.cv = NULL;
	cbdata.data = &kv;
	cbdata.fkey = cb_sniff_key;
	cbdata.fval = cb_sniff_val;
	walk_obj_cson(obj,&cbdata);
	if(kv.cv) printf("sniffed key: %s\n",key);
	return 0;
}

int cb_gather_key(const char *key, int index, cson_value *val, void *cbdata){
	int rc;
	SSR_request *req;
	walk_cbdata *wcbd = (walk_cbdata*)cbdata;
	req = (SSR_request *)(wcbd->data);
	if(!strcmp(key,"command")){
		
		if(!strcmp(key,"init_pose")){
			req->type = SSR_INITPOSE;
		}else if(!strcmp(key,"posepose")){
			req->type = SSR_POSEPOSE;
		}else if(!strcmp(key,"posesnapshot")){
			req->type = SSR_POSESNAPSHOT;
		}else if(!strcmp(key,"")){
		}else if(!strcmp(key,"")){
		}else if(!strcmp(key,"")){
		}else if(!strcmp(key,"")){
		}else if(!strcmp(key,"")){
		}else if(!strcmp(key,"")){
		}

	}else if(!strcmp(key,"level")){
		cson_int_t ii;
		rc = cson_value_fetch_integer(val, &ii );
		req->LOD = (int)ii;
	}else if(!strcmp(key,"position")){
		wcbd->arr = req->vec3;
		wcbd->arrtype = 0; //double
	}else if(!strcmp(key,"orientation")){
		wcbd->arr = req->quat4;
		wcbd->arrtype = 0; //double
	}else if(!strcmp(key,"")){
	}else if(!strcmp(key,"")){
	}
	return 0;
}
int cb_gather_val(cson_value *val, int index, void *cbdata){
	int rc;
	walk_cbdata *wcbd = (walk_cbdata*)cbdata;

	rc = 0;
	switch(cson_value_type_id(val))
	{
	case CSON_TYPE_OBJECT:
		{
			cson_object * obji = cson_value_get_object(val);
			rc = walk_obj_cson(obji,cbdata);
		}
		break;
	case CSON_TYPE_ARRAY:
		{
			cson_array *ar;
			rc = cson_value_fetch_array(val,&ar);
			if(!rc){
				rc = walk_array_cson(ar,cbdata);
			}
		}
		break;
	case CSON_TYPE_DOUBLE:
		{
			cson_double_t dd;
			rc = cson_value_fetch_double(val, &dd );
			((double *)wcbd->arr)[index] = dd;
		}
		break;
	default:
		break;
	}
	return rc;
}
int gather_ssr_req(cson_object *obj, SSR_request *ssr_req ){
	walk_cbdata cbdata;
	cbdata.data = ssr_req;
	cbdata.fkey = cb_gather_key;
	cbdata.fval = cb_gather_val;
	walk_obj_cson(obj,&cbdata);

	return 0;
}
int	parse_json_and_gather_ssr_req(char *strdata, SSR_request *ssr_req){
	int rc;
	cson_value *root;
	cson_parse_info info;
	memset(&info,0,sizeof(cson_parse_info));
	rc = cson_parse_string(&root,strdata,strlen(strdata),NULL,&info); //opt,info);
	if(!rc){
		if( cson_value_is_object(root) ) {
			cson_object * obj = cson_value_get_object(root);
			//print_json_obj_cson(obj);
			//print_json_tree(obj);
			//find_my_key("coords2",obj);
			//sniff_json_tree("coords2",obj);
			gather_ssr_req(root,ssr_req);
		}
		cson_value_free(root);
	}
	return rc;
}
//<<<<<<<<<<<CSON JSON======================




//=========ZONE BALANCER==>>>>>>>>>>>>
static int running_as_zonebalancer = 0;
#ifdef _MSC_VER
#include <direct.h>
#define chdir _chdir
#define strcasecmp _stricmp
WSADATA wsaData;
void initialize_sockets(){
	int iResult;
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
	}
}
#else
void initialize_sockets(){}
#endif

//POINT IN POLY
typedef struct Point {
	double x;
	double y;
} Point;

typedef struct zone {
	char *name;
	Point center;
	Point *poly;
	int n;
	char * ssrname;
	void * ssr;
	void * next;
} zone;
typedef struct ssr {
	char *name;
	char *ip;
	char *port;
	void *next;
	double extent[6]; //added for ssr2
	int levels_available;
} ssr;
static zone *zones = NULL;
static ssr *ssrs = NULL;
void ssr_list_add(ssr *s){
	if(!ssrs){
		ssrs = s;
		s->next = NULL;
	}else{
		ssr *cur = ssrs;
		while(cur->next)
			cur = cur->next;
		cur->next = s;
		s->next = NULL;
	}
}
void zone_list_add(zone *z){
	if(!zones){
		zones = z;
		z->next = NULL;
	}else{
		zone *cur = zones;
		while(cur->next)
			cur = cur->next;
		cur->next = z;
		z->next = NULL;
	}
}


#include <libxml/parser.h>
#include <libxml/tree.h>
static void print_element_names(xmlNode * a_node)
{
    xmlNode *cur_node = NULL;

    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
			struct _xmlAttr *cp;
            printf("node type: Element, name: %s\n", cur_node->name);
			for(cp = cur_node->properties; cp; cp = cp->next) {
				// http://www.xmlsoft.org/html/libxml-tree.html
				printf("\tname=%s value=%s\n",cp->name,xmlGetNoNsProp(cur_node,cp->name));
			}
        }

        print_element_names(cur_node->children);
    }
}

static void load_zone_elements(xmlNode * a_node)
{
    xmlNode *cur_node = NULL;

    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
			struct _xmlAttr *cp;
			char *value;
            printf("node type: Element, name: %s\n", cur_node->name);
			if(!strcasecmp(cur_node->name,"zone")){
				//add to zone list
				zone *z = malloc(sizeof(zone));
				for(cp = cur_node->properties; cp; cp = cp->next) {
					// http://www.xmlsoft.org/html/libxml-tree.html
					value = xmlGetNoNsProp(cur_node,cp->name);
					printf("\tname=%s value=%s\n",cp->name,value);
					if(!strcasecmp(cp->name,"ssr")){
						z->ssrname = strdup(value);
					}else if(!strcmp(cp->name,"name")){
						z->name = strdup(value);
					}else if(!strcasecmp(cp->name,"center")){
						double x,y,zz;
						sscanf(value,"%lf %lf %lf",&x,&y,&zz);
						z->center.x = x;
						z->center.y = y; 
					}else if(!strcasecmp(cp->name,"polygon")){
						double x,y;
						Point points[1000];
						int n, m;
						char *p, *q;
						q = value;
						n = 0;
						while(q){
							m = sscanf(q,"%lf %lf",&x,&y);
							if(m==2){
								points[n].x = x;
								points[n].y = y;
								n++;
							}else{
								break;
							}
							q = strchr(q,',');
							if(!q) break;
							q++;
						}
						z->n = n;
						z->poly = malloc(n*sizeof(Point));
						memcpy(z->poly,points,n*sizeof(Point));
					}
				}

				zone_list_add(z);
			}else if(!strcasecmp(cur_node->name,"ssrserver")){
				//add to ssr list
				ssr *s = malloc(sizeof(ssr));
				for(cp = cur_node->properties; cp; cp = cp->next) {
					value = xmlGetNoNsProp(cur_node,cp->name);
					printf("\tname=%s value=%s\n",cp->name,value);
					if(!strcasecmp(cp->name,"name")){
						s->name = strdup(value);
					}else if(!strcasecmp(cp->name,"ip")){
						s->ip = strdup(value);
					}else if(!strcasecmp(cp->name,"port")){
						s->port = strdup(value);
					}
				}
				ssr_list_add(s);
			}
        }
		//recurse
        load_zone_elements(cur_node->children);
    }
}
zone *find_zone_by_name(char *name){
	zone *z = zones;
	while(z){
		if(!strcmp(z->name,name))
			break;
		z=z->next;
	}
	return z;
}
int cn_PnPoly( Point P, Point *V, int n );
zone *find_zone_by_point(Point P){
	int inside;
	zone *z;
	z = zones;
	while(z){
		inside = cn_PnPoly( P, z->poly, z->n -1 );
		if(inside)
			break;
		z = z->next;
	}
	return z;
}
void test_pointinpoly(){
	zone *z, *zn;
	int inside;
	Point p;
	zn = find_zone_by_name("StrandEnDuin"); //("Afrikaanderwuk");
	z = NULL;
	if(zn){
		p.x = zn->center.x;
		p.y = zn->center.y;
		z = find_zone_by_point(p);
		if(z){
			printf("inside polygon %s\n",z->name);
		}else{
			printf("polygon not found\n");
		}
	}

}
void assign_ssr_by_name(){
	zone *z;
	ssr *s;
	z = zones;
	while(z){
		z->ssr = NULL;
		s = ssrs;
		while(s){
			if(!strcasecmp(z->ssrname,s->name)){
				z->ssr = s;
				break;
			}
			s = s->next;
		}
		z = z->next;
	}
}
static int testing_pointinpoly = 1;
void load_polys(char *filename){
    xmlDocPtr doc; /* the resulting document tree */
    xmlNodePtr root_node = NULL, node = NULL, node1 = NULL;/* node pointers */
    doc = xmlReadFile(filename, NULL, 0);
    if (doc == NULL) {
        fprintf(stderr, "Failed to parse %s\n", filename);
	return;
    }
    root_node = xmlDocGetRootElement(doc);
	//print_element_names(root_node);
	load_zone_elements(root_node);
	assign_ssr_by_name();
    xmlFreeDoc(doc);	
	if(testing_pointinpoly)
		test_pointinpoly();
	printf("done test_pointinpoly\n");
}
//<<<<<<<<===ZONE BALANCER===========



//===========LEAF SSR >>>>>>>>>>>>>>>>>>>>>>>
/*	currently each SSR instance can have 1 leaf scene (when acting as SSR) 
	and many children scenes (when acting as zoneserver)
	ssr_leaf 1:1 ssr process 1:1 static
	we could leave the leaf scene items as scattered statics, but
	we gather them here in ssr_leaf{} for conceptual clarity, and maintenance convenience
*/
typedef struct polygon {
	int n;
	double *pts; //xyz, so polygon can have varying z
} polygon;
typedef struct extrusion {
	polygon poly;
	double below;
	double above;
} extrusion;
static struct ssr_leaf {
	//double transform[16]; //each SSR can have an xy offset, or more generally a transform, 
			//so a scene with no geo nodes can stay float/SFVec3f, and the offset here will expand to double absolute coords
	double xoff, yoff,zoff;
	double inverse[16];   //and its inverse when going the other way, prepared on init
	double extent[6]; //of leaf scene
	extrusion volume; //more detailed than extent, used for 3D version of point-in-polygon test
	int volume_type;
	double extents[6]; //union of extents of leaf and children scenes
} ssrleaf;

//<<<<<<<<<<<<<<<LEAF SSR ==================




#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PAGE "<html><head><title>libmicrohttpd demo</title>"\
			"</head><body>libmicrohttpd demo</body></html>"

//the iterate_post, request_completed functions and connection_info_struct 
// were copied from the POST tutorial at:
// http://www.gnu.org/software/libmicrohttpd/tutorial.html
#define PORT            8888
#define POSTBUFFERSIZE  512
#define MAXNAMESIZE     20
#define MAXANSWERSIZE   512

#define GET             0
#define POST            1

struct connection_info_struct
{
	int connectiontype;
	char *answerstring;
	int len;
	struct MHD_PostProcessor *postprocessor;
};

const char *askpage = "<html><body>\
                       What's your name, Sir?<br>\
                       <form action=\"/namepost\" method=\"post\">\
                       <input name=\"name\" type=\"text\"\
                       <input type=\"submit\" value=\" Send \"></form>\
                       </body></html>";

const char *greetingpage =
  "<html><body><h1>Welcome, %s!</center></h1></body></html>";

const char *errorpage =
  "<html><body>This doesn't seem to be right.</body></html>";


static int
send_page (struct MHD_Connection *connection, const char *page, int len)
{
	int ret;
	struct MHD_Response *response;


	response = MHD_create_response_from_buffer (len, (void *) page, MHD_RESPMEM_PERSISTENT);
	if (!response)
		return MHD_NO;

	ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
	MHD_destroy_response (response);

	return ret;
}
#define MAXPOSESIZE 255
static void jsonPose2double(double *quat4, double *vec3, char *data){
	sscanf(data,"[%lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf]",
		&quat4[0],&quat4[1],&quat4[2],&quat4[3],
		&vec3[0],&vec3[1],&vec3[2]);
}
static void doublePose2json(double *quat4, double *vec3, char *data, int MAX){
	sprintf(data,"[%lf, %lf, %lf, %lf, %lf, %lf, %lf]",
		quat4[0],quat4[1],quat4[2],quat4[3],
		vec3[0],vec3[1],vec3[2]);
}
static void doublePose2jsonB(double *quat4, double *vec3, char *data, int MAX){
	//a gruelling workout with cson to generate and stringify some json
	//-compare with simpler sprintf above. Should get something like:
	// {"position":[2500333.123456,510444.123456,1022.9876],"orientation":[0.0,0.001,-0.005,0.99998]}
	//-goal is to generalize, and provide sample code for more complex API functions, more complex projects
	int i, rc;
	// Create a root object:
	cson_value * objroot = cson_value_new_object();
	// Objects, Arrays, and Strings are represented by higher-level
	// objects owned by their containing cson_value, so we fetch
	// that Object:
	cson_object * obj = cson_value_get_object(objroot);
	// Achuntg: destroying objV will also invalidate/destroy obj.

	//// Add some values to it:
	//cson_object_set( obj, "myInt", cson_value_new_integer(42) );
	//cson_object_set( obj, "myDouble", cson_value_new_double(42.24) );

	{
		//1. create object tree
		// Add an array:
		cson_array *ar;
		cson_value *arori, *arpos;
		arpos = cson_value_new_array();
		cson_object_set( obj, "position", arpos ); // transfers ownership of arpos to obj
		ar = cson_value_get_array(arpos);
		for(i=0;i<3;i++)
			cson_array_set( ar, i, cson_value_new_double(vec3[i]) );
		arori = cson_value_new_array();
		cson_object_set( obj, "orientation", arori ); // transfers ownership of arori to obj
		ar = cson_value_get_array(arori);
		for(i=0;i<4;i++)
			cson_array_set( ar, i, cson_value_new_double(quat4[i]) );
	}
	{
		//2. stringify
		cson_buffer buf = cson_buffer_empty;
		rc = cson_output_buffer( objroot, &buf, NULL );
		if( 0 != rc ) { 
			//... error ... 
		} else {
		   //JSON data is the first (buf.used) bytes of (buf.mem).
		}
		// Regardless of success or failure, make sure to either
		// clean up the buffer:
		//3. copy string to our buf
		if(buf.used < MAX){
			memcpy(data,buf.mem,buf.used);
			data[buf.used] = '\0';
		}
		//4. free cson stringify buf
		cson_buffer_reserve( &buf, 0 );
	}
	// or take over ownership of its bytes:
	//{
	//   char * mem = (char *)buf.mem;
	//   // mem is (buf.capacity) bytes long, of which (buf.used)
	//   // are "used" (they contain the JSON data in this case).
	//   buf = cson_buffer_empty;
	//   //... you now own the buffer's memory and must eventually free() it ...
	//}

	//5. Free cson obj tree: root and all child values it owns:
	cson_value_free( objroot );
}


static char* getSnapshot(int *len){
	struct stat ss;
	int fd;
	ssize_t blocksz; //, left2read;
	char *filename = "2.jpg";
	if (stat(filename, &ss) < 0) {
		printf("load_file_read: could not stat: %s\n", filename);
		return MHD_NO;
	}
	if (!ss.st_size) {
		printf("load_file_read: file is empty %s\n", filename);
		return MHD_NO;
	}
	if (ss.st_size > SSIZE_MAX) {
		/* file is greater that read's max block size: we must make a loop */
		blocksz = SSIZE_MAX;
	} else {
		blocksz = ss.st_size;
	}
	fd = open(filename, O_RDONLY |  O_BINARY | O_BLOCK);
	if (fd < 0) {
		printf("load_file_read: could not open: %s\n", filename);
		return MHD_NO;
	}
	//response = MHD_create_response_from_fd(ss.st_size,fd); //didn't work in win32, complained ERR_CONTENT_LENGTH_MISMATCH in client
	// will be closed when response is destroyed
	//close(fd);

	char *blob = malloc(blocksz);
	read(fd,blob,blocksz);
	//on win32, it works most reliably -without crashing- if I say for it to deep copy the blob, and free its own copy
	//then free my copy here
	//response = MHD_create_response_from_data(blocksz,(void*) blob,	MHD_NO, MHD_YES);
	//free(blob);
	close(fd);
	*len = blocksz;
	return blob;

}
#ifdef WIN32
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#define strdup _strdup
	#include <winsock2.h>	
	#include <ws2tcpip.h> /* for TCPIP - are we using tcp? */
	#define SHUT_RDWR SD_BOTH
	#include <windows.h>
	#define snprintf _snprintf
	//#define sscanf sscanf_s
	#define STRTOK_S strtok_s
int sockwrite(SOCKET s, const char *buf, int len){
	return send(s,buf,len,0);
}
int sockread(SOCKET s, const char *buf, int len){
	return recv(s,buf,len,0);
}

#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#define STRTOK_S strtok_r
int sockwrite(SOCKET s, const char *buf, int len){
	return write(s,buf,len);
}
int sockread(SOCKET s, const char *buf, int len){
	return recv(s,buf,len,0);
}
#endif

int socket_connect(char *host, int port){
	struct hostent *hp;
	struct sockaddr_in addr;
	int on = 1, sock;     
	unsigned short port16;
	struct addrinfo *result = NULL,
                *ptr = NULL,
                hints;
	int iResult;

	ZeroMemory( &hints, sizeof(hints) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	iResult = getaddrinfo("localhost","8081", &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		//WSACleanup();
		return 0;
	} 
	//if((hp = gethostbyname(host)) == NULL){
	if((hp = gethostbyname("localhost")) == NULL){
		perror("gethostbyname");
		return 0;
	}
	//memcpy(hp->h_addr, &addr.sin_addr, hp->h_length);
	memcpy(&addr.sin_addr,hp->h_addr, hp->h_length);
	port16 = port; //long to unsigned short
	addr.sin_port = htons(port); //unsigned short to network byte order
	addr.sin_family = AF_INET;
	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(int));
 
	if(sock == -1){
		perror("setsockopt");
		return 0;
	}
	
	if(connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1){
		perror("connect");
		return 0;
 
	}
	//write(sock,"hi",2); // write(fd, char[]*, len);
	//sockwrite(sock,"hi",2);
	return sock;
}

//static char * fakepostpose = "POST /pose HTTP/1.1\r\nHost: localhost:8080\r\nConnection: keep-alive\r\nContent-Length: 100\r\nOrigin: http://localhost:8080\r\nUser-Agent: Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2272.118 Safari/537.36\r\nContent-type: application/x-www-form-urlencoded\r\nAccept: */*\r\nReferer: http://localhost:8080/SSRClient.html\r\nAccept-Encoding: gzip, deflate\r\nAccept-Language: en-US,en;q=0.8\r\n\r\nposepose=[0, -0.9977955222129822, 0, -0.06639080494642258, -161.7969512939453, 0,284.27691650390625]";
//static char * fakepostsnap = "POST /pose HTTP/1.1\r\nHost: localhost:8080\r\nConnection: keep-alive\r\nContent-Length: 100\r\nOrigin: http://localhost:8080\r\nUser-Agent: Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2272.118 Safari/537.36\r\nContent-type: application/x-www-form-urlencoded\r\nAccept: */*\r\nReferer: http://localhost:8080/SSRClient.html\r\nAccept-Encoding: gzip, deflate\r\nAccept-Language: en-US,en;q=0.8\r\n\r\nposesnapshot=[0, -0.9977955222129822, 0, -0.06639080494642258, -161.7969512939453, 0,284.27691650390625]";
int build_http_post(char *post, char *host, int port, int lencontent, char *useragent, char *origin, char *referer)
{
	int len;
	char *post_fmt;
	post_fmt = "POST /pose HTTP/1.1\r\nHost: %s:%d\r\nConnection: keep-alive\r\nContent-Length: %d\r\nContent-type: application/x-www-form-urlencoded\r\nAccept: */*\r\nAccept-Encoding: gzip, deflate\r\nAccept-Language: en-US,en;q=0.8\r\n\r\n";
	sprintf(post, post_fmt, host, port,lencontent);
	len = strlen(post);
	return len;
}


#define BUFFER_SIZE 32768

//static char *request_line_format = "POST /%s HTTP/1.1\n";
//static char *request_header_format = "Host: %s\nConnection: Keep-Alive\nAccept: */*\nAccept-Language: us-en\nContent-Length: %d\n\n";
//static char *request_line = NULL;
//static char *request_header = NULL;
//char * content_type = "Content_type: application/x-www-form-urlencoded\n";
//char * content_length_format = "Content-Length: %d\n";
int reverse_proxy(char *host, char *port, char *ssr_command, char *key, char *request, int size, char **response) 
{
	int fd, li,lr;
	char *r;
	char buffer[BUFFER_SIZE];
	char content_length[100];
	//if(!request_line)
	//	request_line = malloc(500);
	//sprintf(request_line,request_line_format,ssr_command); //port);

	//if(!request_header)
	//	request_header = malloc(500);
	//sprintf(request_header,request_header_format,"localhost",strlen(request));
    *response = NULL;
	fd = socket_connect(host, atoi(port)); 
	if (fd == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		//freeaddrinfo(result);
	   // WSACleanup();
		//return 1;
		return 0;
	}

	if(fd){
		int lh;
		char *answer, *temp;
		int answermax = BUFFER_SIZE;
		answer = malloc(answermax);

		//POST request-URI HTTP-version
		//Content-Type: mime-type
		//Content-Length: number-of-bytes
		//(other optional request headers)
		//  
		//(URL-encoded query string)

		if(1){
			char post[8192];
			int lenpost;
			int lencontent = size + strlen(key) + 1;
			lenpost = build_http_post(post, host, atoi(port), lencontent, NULL, NULL, NULL);
			sprintf(&post[lenpost],"%s=",key);
			lenpost += strlen(key)+1;
			memcpy(&post[lenpost],request,size);
			lenpost += size;
			memcpy(&post[lenpost],"\r\n",3);
			//printf("%s",post);
			sockwrite(fd,post,lenpost);
		}
		//I learned http uses content-length rather than shutdown signals to end recv loop
		//shutdown(fd, SD_SEND); //shutdown the client's side of the connection (SSR server side can still send)
		memset(buffer,0, BUFFER_SIZE); //we need \0 on the end of each read, so we can safely do strstr below
		r = answer;
		lr = 0;
		int icount = 0;
		//char *content = NULL;
		int lencontent = 0;
		int offcontent = 0;
		while( li = sockread(fd, buffer, BUFFER_SIZE - 1) ){  //subtract one because we need \0 on the end for strstr below
			if(lr + li > answermax ){
				answermax *= 2;
				answer = realloc(answer,answermax);
				r = &answer[lr];
			}
			memcpy(r,buffer,li);
			lr += li;
			r += li;
			memset(buffer,0, BUFFER_SIZE);
			if(!offcontent){
				//first time reading, lets get the Content-Length and \n\n linebreak pointer to data
				if(!lencontent){
					char *cl = strstr(answer,"Content-Length:");
					if(cl){
						cl += strlen("Content-Length:");
						int lc = atoi(cl);
						lencontent = lc;
					}
				}
				if(lencontent && !offcontent){
					char *cd = strstr(answer,"\r\n\r\n"); //linebreak
					if(cd) cd +=4;
					if(!cd){
						cd = strstr(answer,"\n\n");
						if(cd) cd +=2;
					}
					if(cd) offcontent = cd - answer;
				}
			}
			if(lencontent && offcontent)
				if(lr >= offcontent + lencontent) break;
			icount++;
		}
 		shutdown(fd, SHUT_RDWR); 
		//closesocket(fd); 	//might need this
		if(lencontent && offcontent){
			char *finalanswer = malloc(lencontent+1);
			memcpy(finalanswer,&answer[offcontent],lencontent);
			finalanswer[lencontent] = '\0';
			lr = lencontent;
			free(answer);
			answer = finalanswer;
		}
		*response = answer;
	}
	return lr;
}
int sniff_and_foreward(char *ssr_command, char *key, char *data, int size, char **answerstring)
{
	int len;
	double quat4[4];
	double vec3[3];
	Point posexy;
	zone *z;

	//sniff request for map coordinates
	jsonPose2double(quat4,vec3,data);
	posexy.x = vec3[0];
	posexy.y = vec3[1];
	//lookup zone 
	z = find_zone_by_point(posexy);
	if(!z) z = zones;
	if(z){
		ssr *ssri;
		//get the host:port address of the running ssr server to forward to
		ssri = (ssr*)z->ssr;
		//forward request, wait for response
		len = reverse_proxy(ssri->ip, ssri->port, ssr_command, key, data, size, answerstring);
		//if we got a response, return it
	}
	return len;
}

/*zonebalancer version of iterate_post, acts like a load balancer and gateway
1. sniffs request for geographic coords
2. looks up SSR in zone table
3. acts as reverse proxy and forwards request, waits for response
4  copies response to zonebalancer response
*/
static int iterate_post_zb (void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
	const char *filename, const char *content_type,
	const char *transfer_encoding, const char *data, 
	uint64_t off, size_t size)
{
	struct connection_info_struct *con_info = coninfo_cls;
	if(1){
		printf("Key=%s\n",key);
		printf("filename=%s\n",filename);
		printf("content_type=%s\n",content_type);
		printf("transfer_encoding=%s\n",transfer_encoding);
		printf("data=%s\n",data);
	}
	if (0 == strcmp (key, "posepose"))
	{
		if ((size > 0) && (size <= MAXPOSESIZE)) //MAXNAMESIZE
		{
			char *answerstring;
			int len;
			SSR_request ssr_req;

			//answerstring = malloc (MAXANSWERSIZE);
			answerstring = NULL;
			ssr_req.type = SSR_POSEPOSE;
			len = sniff_and_foreward("pose",key,data,size,&answerstring);
			con_info->answerstring = answerstring;  
			con_info->len = len;    
		} 
		else con_info->answerstring = NULL;

		return MHD_NO;
	}
	if (0 == strcmp (key, "posesnapshot"))
	{
		if ((size > 0) && (size <= MAXPOSESIZE)) //MAXNAMESIZE
		{
   			char *answerstring;
			int len;
			SSR_request ssr_req;
			ssr_req.type = SSR_POSESNAPSHOT;
			answerstring = NULL;
			len = sniff_and_foreward("pose",key,data,size,&answerstring);
			con_info->answerstring = answerstring;  
			con_info->len = len;    
		} 
		else con_info->answerstring = NULL;

		return MHD_NO;
	}

	return MHD_YES;
}


// this is the non-zonebalancer , regular SSR response
static int iterate_post (void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
	const char *filename, const char *content_type,
	const char *transfer_encoding, const char *data, 
	uint64_t off, size_t size)
{
	struct connection_info_struct *con_info = coninfo_cls;
	if(1){
		fprintf(stdout,"Key=%s\n",key);
		fprintf(stdout,"filename=%s\n",filename);
		fprintf(stdout,"content_type=%s\n",content_type);
		fprintf(stdout,"transfer_encoding=%s\n",transfer_encoding);
		fprintf(stdout,"data=%s\n",data);
	}
	
	if (0 == strcmp (key, "init_pose"))
	{
		fprintf(stdout,"A");
		if ((size > 0) && (size <= MAXPOSESIZE)) //MAXNAMESIZE
		{
			char *answerstring;
			//double quat4[4], vec3[3];
			SSR_request ssr_req;
			answerstring = malloc (MAXANSWERSIZE);
			fprintf(stdout,"B");
			if (!answerstring) 
				return MHD_NO;
			ssr_req.type = SSR_INITPOSE;
			//in here we should use cson to parse, and our walk_ to gather into ssr_req
			//parse_json_and_gather_ssr_req(data,&ssr_req);
			ssr_req.LOD = 0;
			//jsonPose2double(ssr_req.quat4,ssr_req.vec3,data);
			fprintf(stdout,"C");
			dllFreeWRL_SSRserver_enqueue_request_and_wait(fwctx, &ssr_req);
			fprintf(stdout,"D");
			ssr_req.vec3[0] += ssrleaf.xoff;
			ssr_req.vec3[1] += ssrleaf.yoff;
			ssr_req.vec3[2] += ssrleaf.zoff;
			doublePose2json(ssr_req.quat4,ssr_req.vec3, answerstring, MAXANSWERSIZE);
			fprintf(stdout,"E");
			//_snprintf (answerstring, MAXANSWERSIZE, greetingpage, data);
			con_info->answerstring = answerstring;  
			con_info->len = strlen(answerstring); 
			for(int k=0;k<con_info->len;k++)
				fprintf(stdout,"%c",con_info->answerstring[k]);
			fprintf(stdout," len=%d\n",con_info->len);
		} 
		else con_info->answerstring = NULL;

		return MHD_NO;
	}

	if (0 == strcmp (key, "posepose"))
	{
		if ((size > 0) && (size <= MAXPOSESIZE)) //MAXNAMESIZE
		{
			char *answerstring;
			//double quat4[4], vec3[3];
			SSR_request ssr_req;
			answerstring = malloc (MAXANSWERSIZE);
			if (!answerstring) return MHD_NO;
			ssr_req.type = SSR_POSEPOSE;
			jsonPose2double(ssr_req.quat4,ssr_req.vec3,data);
			ssr_req.vec3[0] -= ssrleaf.xoff;
			ssr_req.vec3[1] -= ssrleaf.yoff;
			ssr_req.vec3[2] -= ssrleaf.zoff;
			dllFreeWRL_SSRserver_enqueue_request_and_wait(fwctx, &ssr_req);
			ssr_req.vec3[0] += ssrleaf.xoff;
			ssr_req.vec3[1] += ssrleaf.yoff;
			ssr_req.vec3[2] += ssrleaf.zoff;
			doublePose2json(ssr_req.quat4,ssr_req.vec3, answerstring, MAXANSWERSIZE);
			//_snprintf (answerstring, MAXANSWERSIZE, greetingpage, data);
			con_info->answerstring = answerstring;  
			con_info->len = strlen(answerstring);  
			  
		} 
		else con_info->answerstring = NULL;

		return MHD_NO;
	}
	if (0 == strcmp (key, "posesnapshot"))
	{
		if ((size > 0) && (size <= MAXPOSESIZE)) //MAXNAMESIZE
		{
   			char *answerstring;
			SSR_request ssr_req;
			ssr_req.type = SSR_POSESNAPSHOT;
			ssr_req.blob = NULL;
			ssr_req.len = 0;
			jsonPose2double(ssr_req.quat4,ssr_req.vec3,data);
			ssr_req.vec3[0] -= ssrleaf.xoff;
			ssr_req.vec3[1] -= ssrleaf.yoff;
			ssr_req.vec3[2] -= ssrleaf.zoff;
			dllFreeWRL_SSRserver_enqueue_request_and_wait(fwctx, &ssr_req);
			con_info->answerstring = ssr_req.blob;
			con_info->len = ssr_req.len;    
		} 
		else con_info->answerstring = NULL;

		return MHD_NO;
	}


	return MHD_YES;
}
void request_completed (void *cls, struct MHD_Connection *connection, 
	void **con_cls,
	enum MHD_RequestTerminationCode toe)
{
	struct connection_info_struct *con_info = *con_cls;

	if (NULL == con_info) return;
	if (con_info->connectiontype == POST)
	{
		MHD_destroy_post_processor (con_info->postprocessor);        
		if (con_info->answerstring) free (con_info->answerstring);
	}
  
	free (con_info);
	*con_cls = NULL;   
}

int haveNasties(char *url){
	// check for nasties 
	//- I don't seem to need any nasty checking:
	//don't want them to fetch ./../../../passowrds.txt or anything above our public folder, 
	// but the .. get cleaned out on the client end, if you trust the clients, 
	// and %20 or %2E (.) get converted to ascii '.', and cleaned out, before this callback
	//on windows, don't want them to fetch C:/passwords.txt, 
	// but it seems impossible anyway, because it comes in as /C:/passwords.txt which is nonsense so file not found
	//putting %08 (backspace, \b) in the url gets in here, and /%08C:/tmp/passwords.html printfs as C:/tmp/passwords.html,
	// but file functions don't back up, so \b stays in filepath and it can't find it
	int urllen, nasties = 0;
	urllen = strlen(url);
	for(int i=0;i<urllen;i++){
		if(i<urllen-1)
			if(url[i]=='.' && url[i+1] == '.') nasties++; 
		if(url[i]==':') nasties++;  
		if(url[i]=='\b') nasties++; //backspace
		//Q. are there other nasties we should check for?
		//Q. have incoming urls converted %20 and %specials to ascii
	}
	//end check for nasties
	return nasties;
}

static int answer_to_connection (void *cls, struct MHD_Connection *connection, 
	const char *url, 
	const char *method, const char *version, 
	const char *upload_data, 
	size_t *upload_data_size, void **con_cls)
{
	if(1){
		printf("\nurl=%s\n",url);
		printf("method=%s\n",method);
		printf("version=%s\n\n",version);
		printf("upload_data=%s\n",upload_data);
	}
	if(NULL == *con_cls) 
	{
		struct connection_info_struct *con_info;

		con_info = malloc (sizeof (struct connection_info_struct));
		if (NULL == con_info) 
			return MHD_NO;
		con_info->answerstring = NULL;
	//If the new request is a POST, the postprocessor must be created now. In addition, the type of the request is stored for convenience.

		if (0 == strcmp (method, "POST")) 
		{      
			if(running_as_zonebalancer)
				con_info->postprocessor = MHD_create_post_processor (connection, POSTBUFFERSIZE, 
					iterate_post_zb, (void*) con_info);   
			else
				con_info->postprocessor = MHD_create_post_processor (connection, POSTBUFFERSIZE, 
					iterate_post, (void*) con_info);   
			if (NULL == con_info->postprocessor) 
			{
				free (con_info); 
				return MHD_NO;
			}
			con_info->connectiontype = POST;
		} 
		else 
			con_info->connectiontype = GET;
		//The address of our structure will both serve as the indicator for successive iterations and to remember the particular details about the connection.

		*con_cls = (void*) con_info; 
		return MHD_YES;
	}
	//The rest of the function will not be executed on the first iteration. A GET request is easily satisfied by sending the question form.
	if (0 == strcmp (method, "GET")) 
	{
		//return send_page (connection, askpage);   
		// maybe valid GET request
		struct MHD_Response * response;
		struct stat ss;
		int fd, ret;
		ssize_t blocksz; //, left2read;
		char *filename = &url[1];
		if (stat(filename, &ss) < 0) {
			printf("load_file_read: could not stat: %s\n", filename);
			return MHD_NO;
		}
		if (!ss.st_size) {
			printf("load_file_read: file is empty %s\n", filename);
			return MHD_NO;
		}
		if (ss.st_size > SSIZE_MAX) {
			/* file is greater that read's max block size: we must make a loop */
			blocksz = SSIZE_MAX;
		} else {
			blocksz = ss.st_size;
		}
		fd = open(filename, O_RDONLY |  O_BINARY | O_BLOCK);
		if (fd < 0) {
			printf("load_file_read: could not open: %s\n", filename);
			return MHD_NO;
		}
		//response = MHD_create_response_from_fd(ss.st_size,fd); //didn't work in win32, complained ERR_CONTENT_LENGTH_MISMATCH in client
		// will be closed when response is destroyed
		//close(fd);

		char *blob = malloc(blocksz);
		read(fd,blob,blocksz);
		//on win32, it works most reliably -without crashing- if I say for it to deep copy the blob, and free its own copy
		//then free my copy here
		response = MHD_create_response_from_data(blocksz,(void*) blob,	MHD_NO, MHD_YES);
		free(blob);
		close(fd);
		ret = MHD_queue_response(connection,MHD_HTTP_OK,response);
		MHD_destroy_response(response);
		return ret;

	} 
	//In case of POST, we invoke the post processor for as long as data keeps incoming, setting *upload_data_size to zero in order to indicate that we have processed�or at least have considered�all of it.
	if (0 == strcmp (method, "POST")) 
	{
		struct connection_info_struct *con_info = *con_cls;
		if (*upload_data_size != 0) 
		{
			MHD_post_process (con_info->postprocessor, upload_data,	
				*upload_data_size);
			*upload_data_size = 0;
			return MHD_YES;
		} 
		else 
			if (NULL != con_info->answerstring) 
				return send_page (connection, con_info->answerstring, con_info->len);
	} 
	//Finally, if they are neither GET nor POST requests, the error page is returned.
	return send_page(connection, errorpage, strlen(errorpage)); 
}

// simple handler - no post guts
static int ahc_echo(void * cls,
	struct MHD_Connection * connection,
	const char * url,
	const char * method,
	const char * version,
	const char * upload_data,
	size_t * upload_data_size,
	void ** ptr) 
{
	static int dummy;
	const char * page = cls;
	struct MHD_Response * response;
	int ret;

	if (!strcmp(method, "POST"))
	{
		//POST handler - we'll come in here for our special goodies:
		// come in with viewpoint pose matrix, leave with screen snapshot and updated pose matrix
		printf("url=%s\n",url);
		printf("method=%s\n",method);
		printf("version=%s\n",version);
		printf("upload_data=%s\n",upload_data);
	}

	if(!strcmp(method, "GET"))
	{
		//GET handler - we'll come in here for our ordinary static pages, glMatrix.js etc

		if (&dummy != *ptr)
		{
			/* The first time only the headers are valid,
				do not respond in the first round... */
			*ptr = &dummy;
			return MHD_YES;
		}
		if (0 != *upload_data_size)
			return MHD_NO; /* upload data in a GET!? */
		*ptr = NULL; /* clear context pointer */
		printf(" %p ",connection);
		if(!strcmp(url,"/") || haveNasties(url)){
			// invalid GET request
			response = MHD_create_response_from_data(strlen(page),
				(void*) page,
				MHD_NO,
				MHD_NO);
		}else{
			// maybe valid GET request
			struct stat ss;
			int fd;
			ssize_t blocksz; //, left2read;
			char *filename = &url[1];
			if (stat(filename, &ss) < 0) {
				printf("load_file_read: could not stat: %s\n", filename);
				return MHD_NO;
			}
			if (!ss.st_size) {
				printf("load_file_read: file is empty %s\n", filename);
				return MHD_NO;
			}
			if (ss.st_size > SSIZE_MAX) {
				/* file is greater that read's max block size: we must make a loop */
				blocksz = SSIZE_MAX;
			} else {
				blocksz = ss.st_size;
			}
			fd = open(filename, O_RDONLY |  O_BINARY | O_BLOCK);
			if (fd < 0) {
				printf("load_file_read: could not open: %s\n", filename);
				return MHD_NO;
			}
			//response = MHD_create_response_from_fd(ss.st_size,fd); //didn't work in win32, complained ERR_CONTENT_LENGTH_MISMATCH in client
			// will be closed when response is destroyed
			//close(fd);

			char *blob = malloc(blocksz);
			read(fd,blob,blocksz);
			//on win32, it works most reliably -without crashing- if I say for it to deep copy the blob, and free its own copy
			//then free my copy here
			response = MHD_create_response_from_data(blocksz,(void*) blob,	MHD_NO, MHD_YES);
			free(blob);
			close(fd);
		}
		ret = MHD_queue_response(connection,MHD_HTTP_OK,response);
		MHD_destroy_response(response);

	}else{
		ret = MHD_NO; /* unexpected method, not GET or POST */
	}
	return ret;
}

// see libmicrohttpd docs on this site:
// http://www.gnu.org/software/libmicrohttpd/
// tutorials: http://www.gnu.org/software/libmicrohttpd/tutorial.html
int main0(int argc, char ** argv) {
/* 
How to run SSR Server
SSRServer.exe 8081 "C:/myscenes/sceneA.x3d" --xoff 123.45 --yoff 345.67 --zoff 789.01 --publicpath "C:\abc\def"
- use a different port for each SSR
- xoff,yoff,zoff are applied to world coordinates going one way, and stripped off going the other
	- so you can get the effect of double precision coordinates 
--publicpath - /public folder that contains SSRClient.html, DragHere.jpg, favicon.ico, gl-matrix.js
	(if not given, ssrserver has a guess for win32 developer running from projectfiles_*)

How to run ZoneBalancer:
SSRServer.exe 8080 --zonebalancer
	- it attempts to read zonebalancer.xml from the folder above the current working directory
		- in xml, it says how many SSRs, and what geographic zones each SSR covers
		- a zone is demarcated with a polygon
	- then your html client will talk to zonebalancer, and zonebalancer will talk to the SSRs
	- currently zonebalancer doesn't launch -or kill- SSRs, you need to do each of those some other way, such as shell script
*/
	struct MHD_Daemon * d;
	char *portstr, *url, *publicpath;
	int iaction;
	if(strstr(argv[0],"projectfiles")){
		//developer folders, use src/SSR/public for Client.html
		char *pf;
		publicpath = strdup(argv[0]);
		pf = strstr(publicpath,"projectfiles");
		strcpy(pf,"src/SSR/public");
	}else{
		//installed, use folder beside ssrserver.exe (PS installer please install public folder with binary distro)
		char *pf;
		publicpath = strdup(argv[0]);
		pf = strstr(publicpath,"SSRserver.exe");
		strcpy(pf,"public");
	}


	if (argc < 2) {
		portstr = "8080";
		printf("%s PORT\n",portstr);
		url = scene_path;
		//return 1;
	}else{
		portstr = argv[1];
		if(argc < 3)
			url = scene_path;
		else{
			url = argv[2];
			if(!strcmp(url,"--zonebalancer")){
				load_polys("..\\zonebalancer.xml");
				running_as_zonebalancer = 1;
				initialize_sockets();
			}
			for(int k=3;k<argc;k++)
			{
				char *arg = argv[k];
				if(!strncmp(arg,"--",2))
					if(!strncmp(&arg[3],"off=",4)){
						if(!strncmp(&arg[2],"x",1)){
							sscanf(&arg[7],"%lf",&ssrleaf.xoff);
						}else if(!strncmp(&arg[2],"y",1)){
							sscanf(&arg[7],"%lf",&ssrleaf.yoff);
						}else if(!strncmp(&arg[2],"z",1)){
							sscanf(&arg[7],"%lf",&ssrleaf.zoff);
						}
					}
					if(!strcmp(arg,"--publicpath")){
						if(argc > k){
							publicpath = strdup(argv[k+1]);
							k++;
						}
					}
			}
		}
	}
	iaction = 2;
	if(running_as_zonebalancer)
		iaction = 2;
	if(iaction == 1) {

		//simple echo of incoming request
		d = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION,
			atoi(portstr),
			NULL,
			NULL,
			&ahc_echo,
			PAGE,
			MHD_OPTION_END);
	}
	if(iaction == 2){
		printf("public path: %s\n",publicpath);
		chdir(publicpath);

		//our special SSRServer request handler
		d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION, //MHD_USE_SELECT_INTERNALLY
			atoi(portstr), //PORT, 
			NULL, 
			NULL,
			&answer_to_connection, 
			PAGE, 
			MHD_OPTION_NOTIFY_COMPLETED, 
			&request_completed, 
			NULL,
			MHD_OPTION_END);
	}
	if (d == NULL)
		return 1;
	if(!running_as_zonebalancer)
		runFW(url);
	printf("Press Enter to stop libmicrohttp deamon and exit:");
	getchar();
	if(!running_as_zonebalancer)
		stopFW();
	MHD_stop_daemon(d);
	return 0;
}


int main(int argc, char ** argv) {
	int iret = main0(argc, argv);
	if(iret){
		printf("Press Enter to exit:");
		getchar();
	}
}