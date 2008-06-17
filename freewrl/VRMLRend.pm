# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada
# Portions Copyright (C) 1998 Bernhard Reiter
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.
#
# $Id$
#
# Name:        VRMLRend.c
# Description:
#              Fills Hash Variables with "C" Code. They are used by VRMLC.pm
#              to write the C functions-source to render different nodes.
#
#              Certain Abbreviation are used, some are substituted in the
#              writing process in get_rendfunc() [VRMLC.pm].
#              Others are "C-#defines".
#              e.g. for #define glTexCoord2f(a,b) glTexCoord2f(a,b) see gen() [VRMLC.pm]
#
# $Log$
# Revision 1.218  2008/06/17 19:00:27  crc_canada
# Geospatial work - June 17 2008
#
# Revision 1.217  2008/06/13 13:50:49  crc_canada
# Geospatial, SF/MFVec3d support.
#
# Revision 1.216  2008/05/07 15:22:41  crc_canada
# input function modified to better handle files without clear end of line on last line.
#
# Revision 1.215  2008/03/31 20:10:17  crc_canada
# Review texture transparency, use node table to update scenegraph to allow for
# node updating.
#
# Revision 1.214  2007/12/13 20:12:52  crc_canada
# KeySensor and StringSensor
#
# Revision 1.213  2007/12/12 23:24:58  crc_canada
# X3DParser work
#
# Revision 1.212  2007/12/10 19:13:53  crc_canada
# Add parsing for x3dv COMPONENT, EXPORT, IMPORT, META, PROFILE
#
# Revision 1.211  2007/12/08 13:38:17  crc_canada
# first changes for x3dv handling of META, COMPONENT, etc. taga.
#
# Revision 1.210  2007/12/06 21:50:57  crc_canada
# Javascript X3D initializers
#
# Revision 1.209  2007/11/06 20:25:28  crc_canada
# Lighting revisited - pointlights and spotlights should all now work ok
#
# Revision 1.208  2007/08/23 14:01:22  crc_canada
# Initial AudioControl work
#
# Revision 1.207  2007/03/12 20:54:00  crc_canada
# MidiKey started.
#
# Revision 1.206  2007/02/28 20:34:50  crc_canada
# More MIDI work - channelPresent works!
#
# Revision 1.205  2007/01/17 21:29:28  crc_canada
# more X3D XML parsing work.
#
# Revision 1.204  2007/01/12 17:55:27  crc_canada
# more 1.18.11 changes
#
# Revision 1.203  2007/01/11 21:09:21  crc_canada
# new files for X3D parsing
#
# Revision 1.201  2006/11/14 20:16:39  crc_canada
# ReWire work.
#
# Revision 1.200  2006/07/10 14:24:11  crc_canada
# add keywords for PROTO interface fields.
#
# Revision 1.199  2006/05/31 14:52:28  crc_canada
# more changes to code for SAI.
#
# Revision 1.198  2006/05/23 16:15:10  crc_canada
# remove print statements, add more defines for a VRML C parser.
#
# Revision 1.197  2006/05/15 14:05:59  crc_canada
# Various fixes; CVS was down for a week. Multithreading for shape compile
# is the main one.
#
# Revision 1.196  2006/03/01 15:16:57  crc_canada
# Changed include file methodology and some Frustum work.
#
# Revision 1.195  2006/02/28 16:19:42  crc_canada
# BoundingBox
#
# Revision 1.194  2006/01/06 22:05:15  crc_canada
# VisibilitySensor
#
# Revision 1.193  2006/01/03 23:01:22  crc_canada
# EXTERNPROTO and Group sorting bugs fixed.
#
# Revision 1.192  2005/12/21 18:16:40  crc_canada
# Rework Generation code.
#
# Revision 1.191  2005/12/19 21:25:08  crc_canada
# HAnim start
#
# Revision 1.190  2005/12/16 18:07:11  crc_canada
# rearrange perl generation
#
# Revision 1.189  2005/12/16 13:49:23  crc_canada
# updating generation functions.
#
# Revision 1.188  2005/12/15 20:42:01  crc_canada
# CoordinateInterpolator2D PositionInterpolator2D
#
# Revision 1.187  2005/12/15 19:57:58  crc_canada
# Geometry2D nodes complete.
#
# Revision 1.186  2005/12/14 13:51:32  crc_canada
# More Geometry2D nodes.
#
# Revision 1.185  2005/12/13 17:00:29  crc_canada
# Arc2D work.
#.....

# used for the X3D Parser only. Return type of node.
%defaultContainerType = (
	ContourPolyLine2D 	=>geometry,
	NurbsTrimmedSurface	=>geometry,
	MidiControl		=>children,
	MidiKey			=>children,


	Arc2D			=>geometry,
	ArcClose2D		=>geometry,
	Circle2D		=>geometry,
	Disk2D			=>geometry,
	Polyline2D		=>geometry,
	Polypoint2D		=>geometry,
	Rectangle2D		=>geometry,
	TriangleSet2D		=>geometry,
	
	Anchor 			=>children,
	Appearance 		=>appearance,
	AudioClip 		=>source,
	AudioControl		=>children,
	Background 		=>children,
	Billboard 		=>children,
	Box 			=>geometry,
	Collision 		=>children,
	Color 			=>color,
	ColorInterpolator 	=>children,
	ColorRGBA 		=>color,
	Cone 			=>geometry,
	Contour2D 		=>geometry,
	Coordinate 		=>coord,
	CoordinateDeformer 	=>children,
	CoordinateInterpolator 	=>children,
	CoordinateInterpolator2D 	=>children,
	Cylinder 		=>geometry,
	CylinderSensor 		=>children,
	DirectionalLight 	=>children,
	ElevationGrid 		=>geometry,
	Extrusion 		=>geometry,
	FillProperties		=>fillProperties,
	Fog 			=>children,
	FontStyle 		=>fontStyle,
	GeoCoordinate 		=>children,
	GeoElevationGrid 	=>geometry,
	GeoLocation 		=>children,
	GeoLOD 			=>children,
	GeoMetadata		=>children,
	GeoOrigin 		=>geoOrigin,
	GeoPositionInterpolator	=>children,
	GeoTouchSensor		=>children,
	GeoViewpoint 		=>children,
	Group 			=>children,
	HAnimDisplacer		=>children,
	HAnimHumanoid		=>children,
	HAnimJoint		=>joints,
	HAnimSegment		=>segments,
	HAnimSite		=>sites,
	ImageTexture 		=>texture,
	IndexedFaceSet 		=>geometry,
	IndexedLineSet 		=>geometry,
	IndexedTriangleFanSet 	=>geometry,
	IndexedTriangleSet 	=>geometry,
	IndexedTriangleStripSet	=>geometry,
	Inline 			=>children,
	InlineLoadControl 	=>children,
	KeySensor		=>children,
	LineSet 		=>geometry,
	LineProperties		=>lineProperties,
	LoadSensor		=>children,
	LOD 			=>children,
	Material 		=>material,
	MultiTexture		=>texture,
	MultiTextureCoordinate  =>texCoord,
	MultiTextureTransform	=>textureTransform,
	MovieTexture 		=>texture,
	NavigationInfo 		=>children,
	Normal 			=>normal,
	NormalInterpolator 	=>children,
	NurbsCurve2D 		=>geometry,
	NurbsCurve 		=>geometry,
	NurbsGroup 		=>children,
	NurbsPositionInterpolator=>children,
	NurbsSurface 		=>children,
	NurbsTextureSurface 	=>children,
	OrientationInterpolator	=>children,
	PixelTexture 		=>texture,
	PlaneSensor 		=>children,
	PointLight 		=>children,
	PointSet 		=>geometry,
	PositionInterpolator 	=>children,
	PositionInterpolator2D 	=>children,
	ProximitySensor 	=>children,
	ScalarInterpolator 	=>children,
	Scene 			=>children,
	Script 			=>children,
	Shape 			=>children,
	Sound 			=>children,
	Sphere 			=>geometry,
	SphereSensor 		=>children,
	SpotLight 		=>children,
	StaticGroup		=>children,
	StringSensor		=>children,
	Switch 			=>children,
	Text 			=>geometry,
	TextureBackground 	=>children,
	TextureCoordinate 	=>texCoord,
	TextureCoordinateGenerator  =>texCoord,
	TextureTransform 	=>textureTransform,
	TimeSensor 		=>children,
	TouchSensor 		=>children,
	Transform 		=>children,
	TriangleFanSet 		=>geometry,
	TriangleSet 		=>geometry,
	TriangleStripSet 	=>geometry,
	TrimmedSurface 		=>children,
	Viewpoint 		=>children,
	VisibilitySensor 	=>children,
	WorldInfo 		=>children,

	BooleanFilter		=>children,
	BooleanSequencer	=>children,
	BooleanToggle		=>children,
	BooleanTrigger		=>children,
	IntegerSequencer	=>children,
	IntegerTrigger		=>children,
	TimeTrigger		=>children,
);

#######################################################################
#######################################################################
#######################################################################
#
# Rend --
#  actually render the node
#
#

# Rend = real rendering - rend_geom is true; this is for things that
#	actually affect triangles/lines on the screen.
#
# All of these will have a render_xxx name associated with them.

%RendC = map {($_=>1)} qw/
	NavigationInfo
	Fog
	Background
	TextureBackground
	Box 
	Cylinder 
	Cone 
	Sphere 
	IndexedFaceSet 
	Extrusion 
	ElevationGrid 
	Arc2D 
	ArcClose2D 
	Circle2D 
	Disk2D 
	Polyline2D 
	Polypoint2D 
	Rectangle2D 
	TriangleSet2D 
	IndexedTriangleFanSet 
	IndexedTriangleSet 
	IndexedTriangleStripSet 
	TriangleFanSet 
	TriangleStripSet 
	TriangleSet 
	LineSet 
	IndexedLineSet 
	PointSet 
	GeoElevationGrid 
	LoadSensor 
	TextureCoordinateGenerator 
	TextureCoordinate 
	Text 
	LineProperties 
	FillProperties 
	Material 
	PixelTexture 
	ImageTexture 
	MultiTexture 
	MovieTexture 
	Sound 
	AudioControl
	AudioClip 
	DirectionalLight 
	HAnimHumanoid
	HAnimJoint
/;

#######################################################################
#######################################################################
#######################################################################

#
# GenPolyRep
#  code for generating internal polygonal representations
#  of some nodes (ElevationGrid, Text, Extrusion and IndexedFaceSet)
#
# 

%GenPolyRepC = map {($_=>1)} qw/
	ElevationGrid
	Extrusion
	IndexedFaceSet
	IndexedTriangleFanSet
	IndexedTriangleSet
	IndexedTriangleStripSet
	TriangleFanSet
	TriangleStripSet
	TriangleSet
	Text
	GeoElevationGrid
/;

#######################################################################
#######################################################################
#######################################################################
#
# Prep --
#  Prepare for rendering a node - e.g. for transforms, do the transform
#  but not the children.

%PrepC = map {($_=>1)} qw/
	HAnimJoint
	HAnimSite
	Viewpoint
	Transform
	Billboard
	Group
	MidiControl
	MidiKey
	PointLight
	SpotLight
	GeoLocation
	GeoViewpoint
/;

#######################################################################
#######################################################################
#######################################################################
#
# Fin --
#  Finish the rendering i.e. restore matrices and whatever to the
#  original state.
#
#

%FinC = map {($_=>1)} qw/
	GeoLocation
	Transform
	Billboard
	HAnimSite
	HAnimJoint
/;


#######################################################################
#######################################################################
#######################################################################
#
# Child --
#  Render the actual children of the node.
#
#

# Render children (real child nodes, not e.g. appearance/geometry)
%ChildC = map {($_=>1)} qw/
	HAnimHumanoid
	HAnimJoint
	HAnimSegment
	HAnimSite
	Group
	StaticGroup
	Billboard
	Transform
	Anchor
	GeoLocation
	Inline
	InlineLoadControl
	Switch
	GeoLOD
	LOD
	Collision
	Appearance
	Shape
	VisibilitySensor
/;


#######################################################################
#######################################################################
#######################################################################
#
# Compile --
#
%CompileC = map {($_=>1)} qw/
	IndexedLineSet
	LineSet
	Arc2D
	ArcClose2D
	Circle2D
	Disk2D
	TriangleSet2D
	Rectangle2D
	Box
	Cone
	Cylinder
	Sphere
	GeoLocation
	GeoCoordinate
	GeoElevationGrid
	GeoLocation
	GeoLOD
	GeoMetadata
	GeoOrigin
	GeoPositionInterpolator
	GeoTouchSensor
	GeoViewpoint	
/;


#######################################################################
#
# ChangedC - when the fields change, the following code is run before
# rendering for caching the data.
#
#

%ChangedC = map {($_=>1)} qw/
	Group
        Inline
	Transform
	StaticGroup
	Billboard
	Anchor
	Collision
	GeoCollision
	InlineLoadControl
	HAnimSite
/;

#######################################################################
#
# ProximityC = following code is run to let proximity sensors send their
# events. This is done in the rendering pass, because the position of
# of the object relative to the viewer is available via the
# modelview transformation matrix.
#

%ProximityC = map {($_=>1)} qw/
	ProximitySensor
/;


#######################################################################
#
# CollisionC = following code is run to do collision detection
#
# In collision nodes:
#    if enabled:
#       if no proxy:
#           passes rendering to its children
#       else (proxy)
#           passes rendering to its proxy
#    else
#       does nothing.
#
# In normal nodes:
#    uses gl modelview matrix to determine distance from viewer and
# angle from viewer. ...
#
#
#	       /* the shape of the avatar is a cylinder */
#	       /*                                           */
#	       /*           |                               */
#	       /*           |                               */
#	       /*           |--|                            */
#	       /*           | width                         */
#	       /*        ---|---       -                    */
#	       /*        |     |       |                    */
#	       /*    ----|() ()| - --- | ---- y=0           */
#	       /*        |  \  | |     |                    */
#	       /*     -  | \ / | |head | height             */
#	       /*    step|     | |     |                    */
#	       /*     -  |--|--| -     -                    */
#	       /*           |                               */
#	       /*           |                               */
#	       /*           x,z=0                           */


%CollisionC = map {($_=>1)} qw/
	Disk2D
	Rectangle2D
	TriangleSet2D
	Sphere
	Box
	Cone
	Cylinder
	ElevationGrid
	IndexedFaceSet
	IndexedTriangleFanSet
	IndexedTriangleSet
	IndexedTriangleStripSet
	TriangleFanSet
	TriangleStripSet
	TriangleSet
	Extrusion
	Text
	GeoElevationGrid
/;

#######################################################################
#######################################################################
#######################################################################
#
# RendRay --
#  code for checking whether a ray (defined by mouse pointer)
#  intersects with the geometry of the primitive.
#
#

# Y axis rotation around an unit vector:
# alpha = angle between Y and vec, theta = rotation angle
#  1. in X plane ->
#   Y = Y - sin(alpha) * (1-cos(theta))
#   X = sin(alpha) * sin(theta)
#
#
# How to find out the orientation from two vectors (we are allowed
# to assume no negative scales)
#  1. Y -> Y' -> around any vector on the plane midway between the
#                two vectors
#     Z -> Z' -> around any vector ""
#
# -> intersection.
#
# The plane is the midway normal between the two vectors
# (if the two vectors are the same, it is the vector).


# found in the C code:
# Distance to zero as function of ratio is
# sqrt(
#	((1-r)t_r1.x + r t_r2.x)**2 +
#	((1-r)t_r1.y + r t_r2.y)**2 +
#	((1-r)t_r1.z + r t_r2.z)**2
# ) == radius
# Therefore,
# radius ** 2 == ... ** 2
# and
# radius ** 2 =
# 	(1-r)**2 * (t_r1.x**2 + t_r1.y**2 + t_r1.z**2) +
#       2*(r*(1-r)) * (t_r1.x*t_r2.x + t_r1.y*t_r2.y + t_r1.z*t_r2.z) +
#       r**2 (t_r2.x**2 ...)
# Let's name tr1sq, tr2sq, tr1tr2 and then we have
# radius ** 2 =  (1-r)**2 * tr1sq + 2 * r * (1-r) tr1tr2 + r**2 tr2sq
# = (tr1sq - 2*tr1tr2 + tr2sq) r**2 + 2 * r * (tr1tr2 - tr1sq) + tr1sq
#
# I.e.
#
# (tr1sq - 2*tr1tr2 + tr2sq) r**2 + 2 * r * (tr1tr2 - tr1sq) +
#	(tr1sq - radius**2) == 0
#
# I.e. second degree eq. a r**2 + b r + c == 0 where
#  a = tr1sq - 2*tr1tr2 + tr2sq
#  b = 2*(tr1tr2 - tr1sq)
#  c = (tr1sq-radius**2)
#
#
# Cylinder: first test the caps, then against infinite cylinder.

# For cone, this is most difficult. We have
# sqrt(
#	((1-r)t_r1.x + r t_r2.x)**2 +
#	((1-r)t_r1.z + r t_r2.z)**2
# ) == radius*( -( (1-r)t_r1.y + r t_r2.y )/(2*h)+0.5)
# == radius * ( -( r*(t_r2.y - t_r1.y) + t_r1.y )/(2*h)+0.5)
# == radius * ( -r*(t_r2.y-t_r1.y)/(2*h) + 0.5 - t_r1.y/(2*h))

#
# Other side: r*r*(

%RendRayC = map {($_=>1)} qw/
	Box
	Sphere
	Cylinder
	Cone
	GeoElevationGrid
	ElevationGrid
	Text
	Extrusion
	IndexedFaceSet
	IndexedTriangleSet
	IndexedTriangleFanSet
	IndexedTriangleStripSet
	TriangleSet
	TriangleFanSet
	TriangleStripSet
/;

#######################################################################
#######################################################################
#######################################################################
#
# Keywords
# a listing of keywords for use in the C VRML parser.
#
# 

%KeywordC = map {($_=>1)} qw/
	COMPONENT
	DEF
	EXPORT
	EXTERNPROTO
	FALSE
	IMPORT
	IS
	META
	NULL
	PROFILE
	PROTO
	ROUTE
	TO
	TRUE
	USE
	inputOnly
	outputOnly
	inputOutput
	initializeOnly
	exposedField
	field
	eventIn
	eventOut
/;


#######################################################################
#
# Components 
# a listing of Components for use in the C VRML parser.
#
# 

%ComponentC = map {($_=>1)} qw/
	CADGeometry
	Core
	CubeMapTexturing
	DIS
	EnvironmentalEffects
	EnvironmentalSensor
	EventUtilities
	Followers
	Geometry2D
	Geometry3D
	Geospatial
	Grouping
	H-Anim
	Interpolation
	KeyDeviceSensor
	Layering
	Layout
	Lighting
	Navigation
	Networking
	NURBS
	ParticleSystems
	PickingSensor
	PointDeviceSensor
	Shaders
	Rendering
	RigidBodyPhysics
	Scripting
	Shape
	Sound
	Text
	Texturing
	Texturing3D
	Time
/;


#######################################################################
#
# Profiles 
# a listing of Profiles for use in the C VRML parser.
#
# 

%ProfileC = map {($_=>1)} qw/
	CADInterchange
	Core
	Full
	Immersive
	Interactive
	Interchange
	MPEG-4
/;

#######################################################################
#######################################################################
#######################################################################

#
# GEOSpatialKeywords
# a listing of Geospatial Elipsoid keywords.
#
# 

%GEOSpatialKeywordC = map {($_=>1)} qw/
	AA
	AM
	AN
	BN
	BR
	CC
	CD
	EA
	EB
	EC
	ED
	EE
	EF
	FA
	GC
	GCC
	GCC
	GD
	GDC
	GDC
	HE
	HO
	ID
	IN
	KA
	RF
	SA
	UTM
	WD
	WE
	WGS84
	coordinateSystem
	copyright
	dataFormat
	dataUrl
	date
	description
	ellipsoid
	extent
	horizontalDatum
	metadataFormat
	originator
	resolution
	title
	verticalDatum
/;

#######################################################################
#######################################################################
#######################################################################

#
# PROTOKeywords
# a listing of PROTO define keywords for use in the C VRML parser.
#
# 

%PROTOKeywordC = map {($_=>1)} qw/
	exposedField
	field
	eventIn
	eventOut
	inputOnly
	outputOnly
	inputOutput
	initializeOnly
/;

#######################################################################
#######################################################################
#######################################################################

#
# X3DSPECIAL Keywords
# a listing of control keywords for use in the XML parser.
#
# 

%X3DSpecialC = map {($_=>1)} qw/
	Scene
	Header
	head
	meta
	ProtoDeclare
	ProtoInterface
	ProtoInstance
	ProtoBody
	ROUTE
	IS
	connect
	X3D
	field
	fieldValue
	component
	import
	export
/;
1;
