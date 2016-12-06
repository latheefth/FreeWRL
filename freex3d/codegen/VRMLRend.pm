# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada
# Portions Copyright (C) 1998 Bernhard Reiter
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.
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
package VRML::Rend;

use strict;
use warnings;

# DJTRACK_PICKSENSORS See PointPickSensor
# See WANT_OSC
# used for the X3D Parser only. Return type of node.

our %defaultContainerType = (
	Proto			=>["children"],

	ContourPolyLine2D	=>["children"],
	NurbsCurve		=>["geometry"],
	NurbsCurve2D		=>["children"],
	Contour2D 		=>["trimmingContour"],
	NurbsPositionInterpolator	=>["children"],
	NurbsTrimmedSurface	=>["geometry"],
	CoordinateDouble	=>["children"],
	NurbsOrientationInterpolator	=>["children"],
	NurbsPatchSurface	=>["geometry"],
	NurbsSet		=>["children"],
	NurbsSurfaceInterpolator	=>["children"],
	NurbsSweptSurface	=>["children"],
	NurbsSwungSurface	=>["children"],
	NurbsTextureCoordinate	=>["children"],


	PointPickSensor		=>["children"],
	LinePickSensor		=>["children"],
	PrimitivePickSensor	=>["children"],
	VolumePickSensor	=>["children"],
	OSC_Sensor		=>["children"],

	Arc2D			=>["geometry"],
	ArcClose2D		=>["geometry"],
	Circle2D		=>["geometry"],
	Disk2D			=>["geometry"],
	Polyline2D		=>["geometry"],
	Polypoint2D		=>["geometry"],
	Rectangle2D		=>["geometry"],
	TriangleSet2D		=>["geometry"],

	IndexedQuadSet		=>["geometry"],
	QuadSet			=>["geometry"],
	CADLayer		=>["children"],
	CADFace			=>["children"],
	CADAssembly		=>["children"],
	CADPart			=>["children"],


	Anchor 			=>["children"],
	Appearance 		=>["appearance"],
	AudioClip 		=>["source"],
	Background 		=>["children"],
	Billboard 		=>["children"],
	Box 			=>["geometry"],
	ClipPlane 		=>["children"],
	Collision 		=>["children"],
	Color 			=>["color"],
	ColorInterpolator 	=>["children"],
	ColorRGBA 		=>["color"],
	Cone 			=>["geometry"],
	Coordinate 		=>["coord","skinCoord"],
	FogCoordinate 		=>["fogCoord"],
	CoordinateDeformer 	=>["children"],
	CoordinateInterpolator 	=>["children"],
	CoordinateInterpolator2D 	=>["children"],
	Cylinder 		=>["geometry"],
	CylinderSensor 		=>["children"],
	DirectionalLight 	=>["children"],
	ElevationGrid 		=>["geometry"],
	Extrusion 		=>["geometry"],
	FillProperties		=>["fillProperties"],
	Fog 			=>["children"],
	LocalFog 		=>["children"],
	FontStyle 		=>["fontStyle"],
	GeoCoordinate 		=>["coord"],
	GeoElevationGrid 	=>["geometry"],
	GeoLocation 		=>["children"],
	GeoLOD 			=>["children"],
	GeoMetadata		=>["children"],
	GeoOrigin 		=>["geoOrigin"],
	GeoPositionInterpolator	=>["children"],
	GeoProximitySensor 	=>["children"],
	GeoTouchSensor		=>["children"],
	GeoTransform		=>["children"],
	GeoViewpoint 		=>["children"],
	Group 			=>["children"],
	ViewpointGroup		=>["children"],
	HAnimDisplacer		=>["displacers"],
	HAnimHumanoid		=>["children"],
	HAnimJoint		=>["joints"],
	HAnimSegment		=>["segments"],
	HAnimSite		=>["sites"],
	ImageTexture 		=>["texture"],
	ImageCubeMapTexture 	=>["texture"],
	GeneratedCubeMapTexture	=>["texture"],
	ComposedCubeMapTexture	=>["texture"],
	IndexedFaceSet 		=>["geometry"],
	IndexedLineSet 		=>["geometry"],
	IndexedTriangleFanSet 	=>["geometry"],
	IndexedTriangleSet 	=>["geometry"],
	IndexedTriangleStripSet	=>["geometry"],
	Inline 			=>["children"],
	KeySensor		=>["children"],
	LineSet 		=>["geometry"],
	LineProperties		=>["lineProperties"],
	LineSensor 		=>["children"],
	LoadSensor		=>["children"],
	LOD 			=>["children"],
	Material 		=>["material"],
	TwoSidedMaterial	=>["material"],
	MultiTexture		=>["texture"],
	MultiTextureCoordinate  =>["texCoord"],
	MultiTextureTransform	=>["textureTransform"],
	MovieTexture 		=>["texture","source"], #or source aka SoundSource like AudioClip
	NavigationInfo 		=>["children"],
	Normal 			=>["normal"],
	NormalInterpolator 	=>["children"],
	OrientationInterpolator	=>["children"],
	PickableGroup 		=>["children"],
	PixelTexture 		=>["texture"],
	PlaneSensor 		=>["children"],
	PointLight 		=>["children"],
	PointSet 		=>["geometry"],
	PositionInterpolator 	=>["children"],
	PositionInterpolator2D 	=>["children"],
	ProximitySensor 	=>["children"],
	ScalarInterpolator 	=>["children"],
	Scene 			=>["children"],
	Script 			=>["children"],
	Shape 			=>["children"],
	Sound 			=>["children"],
	Sphere 			=>["geometry"],
	SphereSensor 		=>["children"],
	SpotLight 		=>["children"],
	StaticGroup		=>["children"],
	StringSensor		=>["children"],
	Switch 			=>["children"],
	Teapot 			=>["geometry"],
	Text 			=>["geometry"],
	TextureBackground 	=>["children"],
	TextureCoordinate 	=>["texCoord"],
	TextureCoordinateGenerator  =>["texCoord"],
	TextureTransform 	=>["textureTransform"],
	TextureProperties	=>["textureProperties"],
	TimeSensor 		=>["children"],
	TouchSensor 		=>["children"],
	Transform 		=>["children"],
	TransformSensor		=>["children"],
	TriangleFanSet 		=>["geometry"],
	TriangleSet 		=>["geometry"],
	TriangleStripSet 	=>["geometry"],
	TrimmedSurface 		=>["children"],
	Viewpoint 		=>["children"],
	OrthoViewpoint 		=>["children"],
	VisibilitySensor 	=>["children"],
	WorldInfo 		=>["children"],

	BooleanFilter		=>["children"],
	BooleanSequencer	=>["children"],
	BooleanToggle		=>["children"],
	BooleanTrigger		=>["children"],
	IntegerSequencer	=>["children"],
	IntegerTrigger		=>["children"],
	TimeTrigger		=>["children"],

	ComposedShader		=>["shaders"],
	ProgramShader		=>["shaders"],
	PackagedShader		=>["shaders"],
	FloatVertexAttribute	=>["children"],
	Matrix3VertexAttribute	=>["children"],
	Matrix4VertexAttribute	=>["children"],
	ShaderPart		=>["parts"],
	ShaderProgram		=>["programs"],

	Viewport		=>["viewport"],
	Layer			=>["layers"],
	LayerSet		=>["children"],

	Layout			=>["layout"],
	LayoutGroup		=>["children"],
	LayoutLayer		=>["layers"],
	ScreenFontStyle		=>["fontStyle"],
	ScreenGroup		=>["children"],

	BallJoint		=>["joints"],
	CollidableOffset	=>["collidables","collidable"],
	CollidableShape		=>["collidables","collidable"],
	CollisionCollection	=>["collider"],
	CollisionSensor		=>["children"],
	CollisionSpace		=>["collidables"],
	Contact			=>["children"],
	DoubleAxisHingeJoint	=>["joints"],
	MotorJoint		=>["joints"],
	RigidBody		=>["children"],
	RigidBodyCollection	=>["children"],
	SingleAxisHingeJoint	=>["joints"],
	SliderJoint		=>["joints"],
	UniversalJoint		=>["joints"],

	ColorChaser		=>["children"],
	ColorDamper		=>["children"],
	CoordinateChaser	=>["children"],
	CoordinateDamper	=>["children"],
	OrientationChaser	=>["children"],
	OrientationDamper	=>["children"],
	PositionChaser		=>["children"],
	PositionDamper		=>["children"],
	PositionChaser2D	=>["children"],
	PositionDamper2D	=>["children"],
	ScalarChaser		=>["children"],
	ScalarDamper		=>["children"],
	TexCoordChaser2D	=>["children"],
	TexCoordDamper2D	=>["children"],

	ConeEmitter		=>["emitter"],
	ExplosionEmitter	=>["emitter"],
	PointEmitter		=>["emitter"],
	PolylineEmitter		=>["emitter"],
	SurfaceEmitter		=>["emitter"],
	VolumeEmitter		=>["emitter"],
	WindPhysicsModel	=>["physics"],
	BoundedPhysicsModel	=>["physics"],
	ForcePhysicsModel	=>["physics"],
	ParticleSystem		=>["shape"],

	
	MetadataSet		=>["metadata"],
	MetadataBoolean		=>["metadata"],
	MetadataInteger		=>["metadata"],
	MetadataDouble		=>["metadata"],
	MetadataFloat		=>["metadata"],
	MetadataString		=>["metadata"],

	#could re-direct these to "children" if/when troto target obsolete, and before all these are cut:
	#Mar 2016 changed from =>["FreeWRL_PROTOInterfaceNodes"], to =>["metadata"],
	MetadataSFFloat		=>["metadata"],
	MetadataMFFloat		=>["metadata"],
	MetadataSFRotation	=>["metadata"],
	MetadataMFRotation	=>["metadata"],
	MetadataSFVec3f		=>["metadata"],
	MetadataMFVec3f		=>["metadata"],
	MetadataSFBool		=>["metadata"],
	MetadataMFBool		=>["metadata"],
	MetadataSFInt32		=>["metadata"],
	MetadataMFInt32		=>["metadata"],
	MetadataSFNode		=>["metadata"],
	MetadataMFNode		=>["metadata"],
	MetadataSFColor		=>["metadata"],
	MetadataMFColor		=>["metadata"],
	MetadataSFColorRGBA	=>["metadata"],
	MetadataMFColorRGBA	=>["metadata"],
	MetadataSFTime		=>["metadata"],
	MetadataMFTime		=>["metadata"],
	MetadataSFString	=>["metadata"],
	MetadataMFString	=>["metadata"],
	MetadataSFVec2f		=>["metadata"],
	MetadataMFVec2f		=>["metadata"],
	MetadataSFImage		=>["metadata"],
	MetadataFreeWRLPTR	=>["metadata"],
	MetadataSFVec3d		=>["metadata"],
	MetadataMFVec3d		=>["metadata"],
	MetadataSFDouble	=>["metadata"],
	MetadataMFDouble	=>["metadata"],
	MetadataSFMatrix3f	=>["metadata"],
	MetadataMFMatrix3f	=>["metadata"],
	MetadataSFMatrix3d	=>["metadata"],
	MetadataMFMatrix3d	=>["metadata"],
	MetadataSFMatrix4f	=>["metadata"],
	MetadataMFMatrix4f	=>["metadata"],
	MetadataSFMatrix4d	=>["metadata"],
	MetadataMFMatrix4d	=>["metadata"],
	MetadataSFVec2d		=>["metadata"],
	MetadataMFVec2d		=>["metadata"],
	MetadataSFVec4f		=>["metadata"],
	MetadataMFVec4f		=>["metadata"],
	MetadataSFVec4d		=>["metadata"],
	MetadataMFVec4d		=>["metadata"],


	EaseInEaseOut 	=>["children"],
	SplinePositionInterpolator 	=>["children"],
	SplinePositionInterpolator2D 	=>["children"],
	SplineScalarInterpolator 	=>["children"],
	SquadOrientationInterpolator 	=>["children"],
	DISEntityManager	=>["children"],
	DISEntityTypeMapping	=>["children"],
	EspduTransform		=>["children"],
	ReceiverPdu		=>["children"],
	SignalPdu		=>["children"],
	TransmitterPdu		=>["children"],
	
	ComposedTexture3D	=>["texture"],
	ImageTexture3D		=>["texture"],
	PixelTexture3D		=>["texture"],
	TextureCoordinate3D	=>["texCoord"],
	TextureCoordinate4D	=>["texCoord"],
	TextureTransformMatrix3D =>["textureTransform"],
	TextureTransform3D	=>["textureTransform"],

	OpacityMapVolumeStyle	=>["renderStyle"],
	VolumeData		=>["children"],
	SegmentedVolumeData	=>["children"],
	IsoSurfaceVolumeData	=>["children"],
	BoundaryEnhancementVolumeStyle =>["renderStyle"],
	ComposedVolumeStyle	=>["renderStyle"],
	EdgeEnhancementVolumeStyle =>["renderStyle"],
	ProjectionVolumeStyle	=>["renderStyle"],
	BlendedVolumeStyle	=>["renderStyle"],
	CartoonVolumeStyle	=>["renderStyle"],
	CompositeVolumeStyle	=>["renderStyle"],
	ShadedVolumeStyle	=>["renderStyle"],
	SilhouetteEnhancementVolumeStyle =>["renderStyle"],
	ToneMappedVolumeStyle	=>["renderStyle"],
	
	
	BackdropBackground	=>["children"],
	ImageBackdropBackground	=>["children"],
	CalibratedCameraSensor	=>["children"],
	TrackingSensor		=>["children"],
	Effect			=>["children"],
	EffectPart		=>["parts"],
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

our %RendC = map {($_=>1)} qw/
	Fog
	Background
	TextureBackground
	Box
	Cylinder
	Cone
	Sphere
	IndexedFaceSet
	Teapot
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
	IndexedQuadSet
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
	Text
	LineProperties
	FillProperties
	Material
	TwoSidedMaterial
	ProgramShader
	PackagedShader
	ComposedShader
	PixelTexture
	ImageTexture
	MultiTexture
	MovieTexture
	ComposedCubeMapTexture
	GeneratedCubeMapTexture
	ImageCubeMapTexture
	Sound
	AudioClip
	DirectionalLight
	SpotLight
	PointLight
	HAnimHumanoid
	HAnimJoint
	QuadSet
	NurbsCurve
	NurbsPatchSurface
	NurbsTrimmedSurface
	ComposedTexture3D
	PixelTexture3D
	ImageTexture3D
	
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

our %GenPolyRepC = map {($_=>1)} qw/
	ElevationGrid
	Extrusion
	IndexedFaceSet
	IndexedQuadSet
	IndexedTriangleFanSet
	IndexedTriangleSet
	IndexedTriangleStripSet
	QuadSet
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

our %PrepC = map {($_=>1)} qw/
	HAnimJoint
	HAnimSite
	Viewpoint
	OrthoViewpoint
	Transform
	Billboard
	Group
	Proto
	Inline
	PickableGroup
	PointLight
	SpotLight
	DirectionalLight
	GeoLocation
	GeoViewpoint
	GeoTransform
	CADAssembly
	CADPart
	Viewport
	LayoutGroup
	ScreenGroup
	Layer
	LayoutLayer
	CollidableOffset
	CollidableShape
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

our %FinC = map {($_=>1)} qw/
	GeoLocation
	Transform
	Billboard
	HAnimSite
	HAnimJoint
	GeoTransform
	CADPart
	Viewport
	LayoutGroup
	ScreenGroup
	Layer
	LayoutLayer	
	CollidableOffset
	CollidableShape	
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

our %ChildC = map {($_=>1)} qw/
	HAnimHumanoid
	HAnimJoint
	HAnimSegment
	HAnimSite
	Group
	Proto
	Inline
	ViewpointGroup
	StaticGroup
	PickableGroup
	Billboard
	Transform
	Anchor
	GeoLocation
	GeoTransform
	Switch
	CADLayer
	CADAssembly
	CADPart
	CADFace
	GeoLOD
	LOD
	Collision
	Appearance
	Shape
	Viewport
	LayoutGroup
	ScreenGroup
	LayerSet
	Layer
	LayoutLayer	
	CollidableOffset
	CollidableShape
	VolumeData
	SegmentedVolumeData
	IsoSurfaceVolumeData
	ParticleSystem
/;


#######################################################################
#######################################################################
#######################################################################
#
# Compile --
#
our %CompileC = map {($_=>1)} qw/
	Shape
	ImageCubeMapTexture
	GeneratedCubeMapTexture
	Transform
	Group
	Proto
	Inline
	CADAssembly
	CADPart
	ViewpointGroup
	Material
	TwoSidedMaterial
	IndexedLineSet
	LineSet
	PointSet
	Arc2D
	ArcClose2D
	Circle2D
	Disk2D
	TriangleSet2D
	Rectangle2D
	Polyline2D
	Polypoint2D
	Box
	Cone
	Cylinder
	Sphere
	Teapot
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
	GeoProximitySensor
	GeoTransform
	ComposedShader
	ProgramShader
	PackagedShader
	Effect
	MetadataMFFloat
	MetadataMFRotation
	MetadataMFVec3f
	MetadataMFBool
	MetadataMFInt32
	MetadataMFNode
	MetadataMFColor
	MetadataMFColorRGBA
	MetadataMFTime
	MetadataMFString
	MetadataMFVec2f
	MetadataMFVec3d
	MetadataMFDouble
	MetadataMFMatrix3f
	MetadataMFMatrix3d
	MetadataMFMatrix4f
	MetadataMFMatrix4d
	MetadataMFVec2d
	MetadataMFVec4f
	MetadataMFVec4d
	MetadataSFFloat
	MetadataSFRotation
	MetadataSFVec3f
	MetadataSFBool
	MetadataSFInt32
	MetadataSFNode
	MetadataSFColor
	MetadataSFColorRGBA
	MetadataSFTime
	MetadataSFString
	MetadataSFVec2f
	MetadataSFImage
	MetadataSFVec3d
	MetadataSFDouble
	MetadataSFMatrix3f
	MetadataSFMatrix3d
	MetadataSFMatrix4f
	MetadataSFMatrix4d
	MetadataSFVec2d
	MetadataSFVec4f
	MetadataSFVec4d
	MetadataSet
	MetadataInteger
	MetadataBoolean
	MetadataDouble
	MetadataFloat
	MetadataString

	SpotLight
	PointLight
	DirectionalLight
	NurbsCurve
	NurbsPatchSurface
	NurbsTrimmedSurface
	Layout
	
	CollidableOffset
	CollidableShape
	VolumeData
	SegmentedVolumeData
	IsoSurfaceVolumeData
	
	ParticleSystem
	HAnimJoint
	HAnimSite
	HAnimHumanoid
/;


#######################################################################
#
# ProximityC = following code is run to let proximity sensors send their
# events. This is done in the rendering pass, because the position of
# of the object relative to the viewer is available via the
# modelview transformation matrix.
#

our %ProximityC = map {($_=>1)} qw/
	ProximitySensor
	LOD
	Billboard
	GeoProximitySensor
/;

#######################################################################
#
# OtherC = following code is run for miscalaneous tasks depending
# on what functions you define here and what VF flags you define and
# check against in render_node() and/or switch on in your Other()
#

our %OtherC = map {($_=>1)} qw/
	PointPickSensor
	PickableGroup
	Sphere
	VisibilitySensor
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


our %CollisionC = map {($_=>1)} qw/
	Disk2D
	Rectangle2D
	TriangleSet2D
	Sphere
	Box
	Cone
	Cylinder
	Teapot
	ElevationGrid
	IndexedFaceSet
	IndexedQuadSet
	IndexedTriangleFanSet
	IndexedTriangleSet
	IndexedTriangleStripSet
	QuadSet
	TriangleFanSet
	TriangleStripSet
	TriangleSet
	Extrusion
	Text
	GeoElevationGrid
	NurbsPatchSurface
	NurbsTrimmedSurface	
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

our %RendRayC = map {($_=>1)} qw/
	Box
	Sphere
	Cylinder
	Cone
	Teapot
	GeoElevationGrid
	ElevationGrid
	Text
	Extrusion
	IndexedFaceSet
	IndexedQuadSet
	QuadSet
	IndexedTriangleSet
	IndexedTriangleFanSet
	IndexedTriangleStripSet
	TriangleSet
	TriangleFanSet
	TriangleStripSet
	NurbsPatchSurface
	NurbsTrimmedSurface
/;


#######################################################################
#######################################################################
#######################################################################
#
# Keywords
# a listing of keywords for use in the C VRML parser.
#
#

our %KeywordC = map {($_=>1)} qw/
	COMPONENT
	DEF
	EXPORT
	EXTERNPROTO
	EXTERNBROTO
	FALSE
	IMPORT
	IS
	META
	UNIT
	NULL
	PROFILE
	PROTO
	BROTO
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

our %ComponentC = map {($_=>1)} qw/
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
	Picking
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
	VolumeRendering
/;


#######################################################################
#
# Profiles
# a listing of Profiles for use in the C VRML parser.
#
#

our %ProfileC = map {($_=>1)} qw/
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

our %GEOSpatialKeywordC = map {($_=>1)} qw/
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

# our %PROTOKeywordC = map {($_=>1)} qw/
	# exposedField
	# field
	# eventIn
	# eventOut
	# inputOnly
	# outputOnly
	# inputOutput
	# initializeOnly
# /;

#a few things are relying on a certain order
our @PROTOKeywordC = qw/
	initializeOnly
	inputOnly
	outputOnly
	inputOutput
	field
	eventIn
	eventOut
	exposedField
/;

#######################################################################
#######################################################################
#######################################################################

#
# Texture Boundary Keywords
#

our %TextureBoundaryC = map {($_=>1)} qw/
	CLAMP
	CLAMP_TO_EDGE
	CLAMP_TO_BOUNDARY
	MIRRORED_REPEAT
	REPEAT
/;

#######################################################################
#######################################################################
#######################################################################

#
# Texture MAgnifiation Keywords
#

our %TextureMagnificationC = map {($_=>1)} qw/
	AVG_PIXEL
	DEFAULT
	FASTEST
	NEAREST_PIXEL
	NICEST
/;

#######################################################################
#######################################################################
#######################################################################

#
# Texture Minification Keywords
#

our %TextureMinificationC = map {($_=>1)} qw/
	AVG_PIXEL
	AVG_PIXEL_AVG_MIPMAP
	AVG_PIXEL_NEAREST_MIPMAP
	DEFAULT
	FASTEST
	NEAREST_PIXEL
	NEAREST_PIXEL_AVG_MIPMAP
	NEAREST_PIXEL_NEAREST_MIPMAP
	NICEST
/;

#######################################################################
#######################################################################
#######################################################################

#
# Texture Compression Keywords
#

our %TextureCompressionC = map {($_=>1)} qw/
	DEFAULT
	FASTEST
	HIGH
	LOW
	MEDIUM
	NICEST
/;

#######################################################################
#######################################################################
#######################################################################

our %MultiTextureSourceC = map {($_=>1)} qw/
	DIFFUSE
	SPECULAR
	FACTOR
/;
our %MultiTextureFunctionC = map {($_=>1)} qw/
	COMPLEMENT
	ALPHAREPLICATE
/;


our %MultiTextureModeC = map {($_=>1)} qw/
	MODULATE
	REPLACE
	MODULATE2X
	MODULATE4X
	ADD
	ADDSIGNED
	ADDSIGNED2X
	SUBTRACT
	ADDSMOOTH
	BLENDDIFFUSEALPHA
	BLENDTEXTUREALPHA
	BLENDFACTORALPHA
	BLENDCURRENTALPHA
	MODULATEALPHA_ADDCOLOR
	MODULATEINVALPHA_ADDCOLOR
	MODULATEINVCOLOR_ADDALPHA
	OFF
	SELECTARG1
	SELECTARG2
	DOTPRODUCT3
/;

our %TextureCoordGenModeC = map {($_=>1)} qw/
	SPHERE-REFLECT-LOCAL
	SPHERE-REFLECT
	SPHERE-LOCAL
	SPHERE
	CAMERASPACENORMAL
	CAMERASPACEPOSITION
	CAMERASPACEREFLECTION
	COORD-EYE
	COORD
	NOISE-EYE
	NOISE
/;


#
# X3DSPECIAL Keywords
# a listing of control keywords for use in the XML parser.
#
#

our %X3DSpecialC = map {($_=>1)} qw/
	Scene
	Header
	head
	meta
	ExternProtoDeclare
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
	IMPORT
	EXPORT
/;

1;
