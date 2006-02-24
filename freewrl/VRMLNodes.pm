#
# $Id$
#
# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada.
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

package VRML::NodeType;

#########################################################
# The routines below implement the browser object interface.


# The following VRML:: variables are used
BEGIN {
								# Script output file name (set with -psout)
  $VRML::script_out_file = "" unless
	defined $VRML::script_out_file ;
  $VRML::script_out_open = 0;	# True if output file is open
								# True if script output file is
								# currently selected
  $VRML::script_out_selected = 0 ;
  $VRML::LAST_HANDLE = 0 ;		# Filehandle in use before script call
}


#JAS use Data::Dumper ;

package VRML::NodeType;

########################################################################

my $protono;

{
    # XXX When this changes, change Scene.pm: VRML::Scene::newp too --
    # the members must correspond.
    sub new {
		my($type, $name, $fields) = @_;
		my $this = bless {
						  Name => $name,
						  Defaults => {},
						  EventOuts => {},
						  EventIns => {},
						  Fields => {},
						  FieldKinds => {},
						  FieldTypes => {}
						 },$type;

		my $t;
		for (keys %$fields) {
			if (ref $fields->{$_}[1] eq "ARRAY") {
				push @{$this->{Defaults}{$_}}, @{$fields->{$_}[1]};
			} else {
				$this->{Defaults}{$_} = $fields->{$_}[1];
			}
			$this->{FieldTypes}{$_} = $fields->{$_}[0];
			$t = $fields->{$_}[2];
			if (!defined $t) {
				die("Missing field or event type for $_ in $name");
			}

			if ($t =~ /in$/i) {
				$this->{EventIns}{$_} = $_;
			} elsif ($t =~ /out$/i) {
				$this->{EventOuts}{$_} = $_;
			} elsif ($t =~ /^exposed/) {
				## in case the 'set_' prefix or '_changed' suffix isn't
				## used for exposedFields in routes
				$this->{EventIns}{$_} = $_;
				$this->{EventOuts}{$_} = $_;
			}
			$this->{FieldKinds}{$_} = $t;
		}
		return $this;
    }
}


# commented out - for using occlusion to do frustum culling.
#	Shape
#	StaticGroup
#	Transform
%VRML::Nodes::occludeEvents = map {($_,1)} qw/
	VisibilitySensor
/;


%VRML::Nodes::bindable = map {($_,1)} qw/
 Viewpoint
 Background
 TextureBackground
 NavigationInfo
 Fog
 GeoViewpoint
/;



%VRML::Nodes::X3DUrlObject = map {($_=>1)} qw/
	ImageTexture
	MovieTexture
	Inline
	Script
	AudioClip
	/;

%VRML::Nodes::X3DFaceGeometry = map {($_=>1)} qw/
	ElevationGrid
	IndexedFaceSet
	IndexedTriangleFanSet
	IndexedTriangleSet
	IndexedTriangleStripSet
	TriangleFanSet
	TriangleStripSet
	TriangleSet
	/;

%VRML::Nodes::X3DCG_IndexedTriangleStripSet = map {($_=>1)} qw/
	color
	coord
	creaseAngle
	metadata
	normal
	ccw
	normalPerVertex
	solid
	index
	/;
%VRML::Nodes::X3DCG_IndexedTriangleSet = map {($_=>1)} qw/
	color
	coord
	metadata
	normal
	texCoord
	ccw
	colorPerVertex
	normalPerVertex
	solid
	index
	/;

%VRML::Nodes::X3DCG_IndexedTriangleFanSet = map {($_=>1)} qw/
	color
	coord
	metadata
	normal
	texCoord
	ccw
	colorPerVertex
	normalPerVertex
	solid
	index
	/;

%VRML::Nodes::X3DCG_TriangleFanSet = map {($_=>1)} qw/
	color
	coord
	metadata
	normal
	texCoord
	ccw
	colorPerVertex
	normalPerVertex
	solid
	fanCount
	/;

%VRML::Nodes::X3DCG_TriangleSet = map {($_=>1)} qw/
	color
	coord
	metadata
	normal
	texCoord
	ccw
	colorPerVertex
	normalPerVertex
	solid
	/;

%VRML::Nodes::X3DCG_TriangleStripSet = map {($_=>1)} qw/
	color
	coord
	metadata
	normal
	stripCount
	texCoord
	ccw
	colorPerVertex
	normalPerVertex
	solid
	/;


%VRML::Nodes::X3DCG_ElevationGrid = map {($_=>1)} qw/
	set_height
	color
	normal
	texCoord
	height
	ccw
	colorPerVertex
	creaseAngle
	normalPerVertex
	solid
	xDimension
	xSpacing
	zDimension
	zSpacing
	/;

%VRML::Nodes::X3DCG_IndexedFaceSet = map {($_=>1)} qw/
	color
	coord
	creaseAngle
	metadata
	normal
	texCoord
	ccw
	colorIndex
	colorPerVertex
	convex
	coordIndex
	normalIndex
	normalPerVertex
	solid
	texCoordIndex
	/;


%VRML::Nodes::visible = map {($_=>1)} qw/
	Box
	Cone
	Sphere
	IndexedFaceSet
	ElevationGrid
	GeoElevationGrid
	Extrusion
	IndexedLineSet
	Background
	TextureBackground
	PointLight
	Fog
	DirectionalLight
	SpotLight
        /;

# nodes that are valid geometry fields.
%VRML::Nodes::geometry = map {($_=>1)} qw/
	Arc2D
	ArcClose2D
	Circle2D
	Disk2D
	Polyline2D
	Polypoint2D
	Rectangle2D
	TriangleSet2D
	Box
	Cone
	Sphere
	Cylinder
	IndexedFaceSet
	ElevationGrid
	GeoElevationGrid
	Extrusion
	IndexedLineSet
	LineSet
	PointSet
	Text
	IndexedTriangleFanSet
	IndexedTriangleSet
	IndexedTriangleStripSet
	TriangleFanSet
	TriangleSet
	TriangleStripSet
        /;

# nodes that are valid at the top; not all nodes
# can reside at the top of the scenegraph.
# see http://www.web3d.org/x3d/specifications/ISO-IEC-19775-X3DAbstractSpecification/
%VRML::Nodes::topNodes = map {($_=>1)} qw/
	Fog
	GeoViewpoint
	NavigationInfo
	Viewpoint
	Background
	TextureBackground
	Inline
	StaticGroup
	Shape
	Anchor
	Billboard
	Collision
	EspduTransform
	GeoLocation
	GeoLOD
	Group
	HAnimJoint
	HAnimSegment
	HAnimSite
	LOD
	Switch
	Transform
	NurbsSet
	NurbsOrientationInterpolator
	NurbsPositionInterpolator
	NurbsSurfaceInterpolator
	HAnimHumanoid
	ReceiverPdu
	SignalPdu
	TransmitterPdu
	ColorInterpolator
	CoordinateInterpolator
	CoordinateInterpolator2D
	GeoPositionInterpolator
	NormalInterpolator
	OrientationInterpolator
	PositionInterpolator
	PositionInterpolator2D
	ScalarInterpolator
	DirectionalLight
	PointLight
	SpotLight
	Script
	TimeSensor
	Collision
	ProximitySensor
	VisibilitySensor
	KeySensor
	StringSensor
	LoadSensor
	CylinderSensor
	PlaneSensor
	SphereSensor
	GeoTouchSensor
	TouchSensor
	Sound
	TimeSensor
	AudioClip
	MovieTexture
	BooleanSequencer
	IntegerSequencer
	BooleanTrigger
	IntegerTrigger
	TimeTrigger
	BooleanFilter
	BooleanToggle
	GeoMetadata
	WorldInfo
	PROFILE
	COMPONENT
	META
        /;

# nodes that are valid children fields. Check out the X3DChildNode
# section of:
#ISO-IEC-19775-IS-X3DAbstractSpecification/Part01/concepts.html#f-Objecthierarchy
%VRML::Nodes::children = map {($_=>1)} qw/
	Fog
	GeoViewpoint
	NavigationInfo
	Viewpoint
	Background
	TextureBackground
	Inline
	StaticGroup
	Shape
	Anchor
	Billboard
	Collision
	EspduTransform
	GeoLocation
	GeoLOD
	Group
	HAnimJoint
	HAnimSegment
	HAnimSite
	LOD
	Switch
	Transform
	NurbsSet
	NurbsOrientationInterpolator
	NurbsPositionInterpolator
	NurbsSurfaceInterpolator
	HanimHumanoid
	ReceiverPdu
	SignalPdu
	TransmitterPdu
	ColorInterpolator
	CoordinateInterpolator
	CoordinateInterpolator2D
	GeoPositionInterpolator
	NormalInterpolator
	OrientationInterpolator
	PositionInterpolator
	PositionInterpolator2D
	ScalarInterpolator
	DirectionalLight
	PointLight
	SpotLight
	Script
	TimeSensor
	Collision
	ProximitySensor
	VisibilitySensor
	KeySensor
	StringSensor
	LoadSensor
	CylinderSensor
	PlaneSensor
	SphereSensor
	GeoTouchSensor
	TouchSensor
	Sound
	TimeSensor
	BooleanSequencer
	IntegerSequencer
	BooleanTrigger
	IntegerTrigger
	TimeTrigger
	BooleanFilter
	BooleanToggle
	GeoMetadata
	WorldInfo
        /;

#nodes that are valid appearance fields.
%VRML::Nodes::appearance = map {($_=>1)} qw/
	Appearance
        /;

#nodes that are valid normal fields.
%VRML::Nodes::normal = map {($_=>1)} qw/
	Normal	
        /;

#nodes that are valid material fields.
%VRML::Nodes::fontStyle = map {($_=>1)} qw/
	FontStyle
        /;

#nodes that are valid material fields.
%VRML::Nodes::material = map {($_=>1)} qw/
	Material
        /;

#nodes that are valid X3DColorNode color fields.
%VRML::Nodes::color = map {($_=>1)} qw/
	Color
	ColorRGBA
        /;

#nodes that are valid textureTransform fields.
%VRML::Nodes::textureTransform = map {($_=>1)} qw/
	TextureTransform
        /;

# nodes that are valid texture fields.
%VRML::Nodes::texture = map {($_=>1)} qw/
	ImageTexture
	PixelTexture
	MovieTexture
	MultiTexture
        /;

# nodes that are valid coord fields.
%VRML::Nodes::coord = map {($_=>1)} qw/
	Coordinate
	/;

# nodes that are valid texcoord fields.
%VRML::Nodes::texCoord = map {($_=>1)} qw/
	TextureCoordinate
	MultiTextureCoordinate
	TextureCoordinateGenerator
	/;

# nodes that are valid fillProperties fields.
%VRML::Nodes::fillProperties = map {($_=>1)} qw/
	FillProperties
	/;

# nodes that are valid lineProperties fields.
%VRML::Nodes::lineProperties = map {($_=>1)} qw/
	LineProperties
	/;

# nodes that are valid sound source fields.
%VRML::Nodes::source = map {($_=>1)} qw/
	AudioClip
	/;

# nodes that are just ignored for now.
%VRML::Nodes::skipthese = map {($_=>1)} qw/
	X3D
	Metadata
	MetadataDouble
	MetadataInteger
	MetadataString
	MetadataFloat
	MetadataSet
	/;

# fields of ROUTE statements in X3D:
%VRML::Nodes::routefields = map{($_=>1)} qw/
	toField
	toNode
	fromField
	fromNode
	/;

# initevents are used in event propagation as the "First" events to run.
# check out add_first subroutine, and event propagation to see what happens.

%VRML::Nodes::initevents = map {($_,1)} qw/
 TimeSensor
 ProximitySensor
 Collision
 MovieTexture
 AudioClip
 VisibilitySensor
/;

# What are the transformation-hierarchy child nodes?
%VRML::Nodes::Transchildren = qw(
 Transform	children
 Group		children
 StaticGroup	children
 Billboard	children
 Anchor		children
 Collision	children
 GeoLocation	children
 InlineLoadControl children

);

%VRML::Nodes::siblingsensitive = map {($_, 1)} qw/
 TouchSensor
 GeoTouchSensor
 PlaneSensor
 CylinderSensor
 SphereSensor
/;

%VRML::Nodes::sensitive = map {($_, 1)} qw/
 ProximitySensor
 Anchor
/;


# used for the X3D Parser only. Return type of node.
%X3D::X3DNodes::defaultContainerType = (
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
	GeoOrigin 		=>children,
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
);

%VRML::Nodes = (
	###################################################################################

	#		Time Component

	###################################################################################

	TimeSensor => new VRML::NodeType("TimeSensor", {
						cycleInterval => [SFTime, 1, exposedField],
						enabled => [SFBool, 1, exposedField],
						loop => [SFBool, 0, exposedField],
						startTime => [SFTime, 0, exposedField],
						stopTime => [SFTime, 0, exposedField],
						cycleTime => [SFTime, -1, eventOut],
						fraction_changed => [SFFloat, 0.0, eventOut],
						isActive => [SFBool, 0, eventOut],
						time => [SFTime, -1, eventOut],
						resumeTime => [SFTime,0,exposedField],
						pauseTime => [SFTime,0,exposedField],
						isPaused => [SFTime,0,eventOut],
						 # time that we were initialized at
						 __inittime => [SFTime, 0, field],
						# cycleTimer flag.
						__ctflag =>[SFTime, 10, exposedField]
					   }),


	###################################################################################

	#		Networking Component

	###################################################################################

	Anchor => new VRML::NodeType("Anchor", {
						addChildren => [MFNode, undef, eventIn],
						removeChildren => [MFNode, undef, eventIn],
						children => [MFNode, [], exposedField],
						description => [SFString, "", exposedField],
						parameter => [MFString, [], exposedField],
						url => [MFString, [], exposedField],
						bboxCenter => [SFVec3f, [0, 0, 0], field],
						bboxSize => [SFVec3f, [-1, -1, -1], field],
						__parenturl =>[SFString,"",field],
					   }),


	Inline => new VRML::NodeType("Inline", {
						url => [MFString, [], exposedField],
						bboxCenter => [SFVec3f, [0, 0, 0], field],
						bboxSize => [SFVec3f, [-1, -1, -1], field],
						load => [SFBool,0,field],
                                                __children => [MFNode, [], exposedField],
						__loadstatus =>[SFInt32,0,field],
						__parenturl =>[SFString,"",field],
					   }),

	LoadSensor => new VRML::NodeType("LoadSensor", {
						enabled => [SFBool,1,exposedField],
						timeOut  => [SFTime,0,exposedField],
						watchList => [MFNode, [], exposedField],
						isActive  => [SFBool,0,eventOut],
						isLoaded  => [SFBool,0,eventOut],
						loadTime  => [SFTime,0,eventOut],
						progress  => [SFFloat,0,eventOut],
						__loading => [SFBool,0,field],		# current internal status
						__finishedloading => [SFBool,0,field],	# current internal status
						__StartLoadTime => [SFTime,0,eventOut], # time we started loading...
					}),

	###################################################################################

	#		Grouping Component

	###################################################################################

	Group => new VRML::NodeType("Group", {
						addChildren => [MFNode, undef, eventIn],
						removeChildren => [MFNode, undef, eventIn],
						children => [MFNode, [], exposedField],
						bboxCenter => [SFVec3f, [0, 0, 0], field],
						bboxSize => [SFVec3f, [-1, -1, -1], field],
						 __isProto => [SFInt32, 0, field], # tell renderer that this is a proto...
					   }),

	StaticGroup => new VRML::NodeType("StaticGroup", {
						children => [MFNode, [], exposedField],
						bboxCenter => [SFVec3f, [0, 0, 0], field],
						bboxSize => [SFVec3f, [-1, -1, -1], field],
						 __transparency => [SFInt32, -1, field], # display list for transparencies
						 __solid => [SFInt32, -1, field],	 # display list for solid geoms.
						 __OccludeNumber =>[SFInt32,-1,field], # for Occlusion tests.
					   }),

	Switch => new VRML::NodeType("Switch", {
					 	addChildren => [MFNode, undef, eventIn],
						removeChildren => [MFNode, undef, eventIn],
						choice => [MFNode, [], exposedField],
						whichChoice => [SFInt32, -1, exposedField],
						 bboxCenter => [SFVec3f, [0, 0, 0], field],
						 bboxSize => [SFVec3f, [-1, -1, -1], field],
					   }),

	Transform => new VRML::NodeType ("Transform", {
						 addChildren => [MFNode, undef, eventIn],
						 removeChildren => [MFNode, undef, eventIn],
						 center => [SFVec3f, [0, 0, 0], exposedField],
						 children => [MFNode, [], exposedField],
						 rotation => [SFRotation, [0, 0, 1, 0], exposedField],
						 scale => [SFVec3f, [1, 1, 1], exposedField],
						 scaleOrientation => [SFRotation, [0, 0, 1, 0], exposedField],
						 translation => [SFVec3f, [0, 0, 0], exposedField],
						 bboxCenter => [SFVec3f, [0, 0, 0], field],
						 bboxSize => [SFVec3f, [-1, -1, -1], field],
						 __OccludeNumber =>[SFInt32,-1,field], # for Occlusion tests.

						 # fields for reducing redundant calls
						 __do_center => [SFInt32, 0, field],
						 __do_trans => [SFInt32, 0, field],
						 __do_rotation => [SFInt32, 0, field],
						 __do_scaleO => [SFInt32, 0, field],
						 __do_scale => [SFInt32, 0, field],
						}),

	WorldInfo => new VRML::NodeType("WorldInfo", {
						info => [MFString, [], field],
						title => [SFString, "", field]
					   }),

	###################################################################################

	#		Rendering Component

	###################################################################################

	Color => new VRML::NodeType("Color", { color => [MFColor, [], exposedField], }),

	ColorRGBA => new VRML::NodeType("ColorRGBA", { color => [MFColorRGBA, [], exposedField], }),

	Coordinate => new VRML::NodeType("Coordinate", { point => [MFVec3f, [], exposedField] }),

	IndexedLineSet => new VRML::NodeType("IndexedLineSet", {
						set_colorIndex => [MFInt32, undef, eventIn],
						set_coordIndex => [MFInt32, undef, eventIn],
						color => [SFNode, NULL, exposedField],
						coord => [SFNode, NULL, exposedField],
						colorIndex => [MFInt32, [], field],
						colorPerVertex => [SFBool, 1, field],
						coordIndex => [MFInt32, [], field]
					   }),

	IndexedTriangleFanSet => new VRML::NodeType("IndexedTriangleFanSet", {
						set_colorIndex => [MFInt32, undef, eventIn],
						set_coordIndex => [MFInt32, undef, eventIn],
						set_normalIndex => [MFInt32, undef, eventIn],
						set_texCoordIndex => [MFInt32, undef, eventIn],
						color => [SFNode, NULL, exposedField],
						coord => [SFNode, NULL, exposedField],
						normal => [SFNode, NULL, exposedField],
						texCoord => [SFNode, NULL, exposedField],
						ccw => [SFBool, 1, field],
						colorIndex => [MFInt32, [], field],
						colorPerVertex => [SFBool, 1, field],
						convex => [SFBool, 1, field],
						coordIndex => [MFInt32, [], field],
						creaseAngle => [SFFloat, 0, field],
						normalIndex => [MFInt32, [], field],
						normalPerVertex => [SFBool, 1, field],
						solid => [SFBool, 1, field],
						texCoordIndex => [MFInt32, [], field],
						index => [MFInt32, [], field],
						fanCount => [MFInt32, [], field],
						stripCount => [MFInt32, [], field],

						set_height => [MFFloat, undef, eventIn],
						height => [MFFloat, [], field],
						xDimension => [SFInt32, 0, field],
						xSpacing => [SFFloat, 1.0, field],
						zDimension => [SFInt32, 0, field],
						zSpacing => [SFFloat, 1.0, field],
						__PolyStreamed => [SFBool, 0, field],
						}),

	IndexedTriangleSet => new VRML::NodeType("IndexedTriangleSet", {
						set_colorIndex => [MFInt32, undef, eventIn],
						set_coordIndex => [MFInt32, undef, eventIn],
						set_normalIndex => [MFInt32, undef, eventIn],
						set_texCoordIndex => [MFInt32, undef, eventIn],
						color => [SFNode, NULL, exposedField],
						coord => [SFNode, NULL, exposedField],
						normal => [SFNode, NULL, exposedField],
						texCoord => [SFNode, NULL, exposedField],
						ccw => [SFBool, 1, field],
						colorIndex => [MFInt32, [], field],
						colorPerVertex => [SFBool, 1, field],
						convex => [SFBool, 1, field],
						coordIndex => [MFInt32, [], field],
						creaseAngle => [SFFloat, 0, field],
						normalIndex => [MFInt32, [], field],
						normalPerVertex => [SFBool, 1, field],
						solid => [SFBool, 1, field],
						texCoordIndex => [MFInt32, [], field],
						index => [MFInt32, [], field],
						fanCount => [MFInt32, [], field],
						stripCount => [MFInt32, [], field],

						set_height => [MFFloat, undef, eventIn],
						height => [MFFloat, [], field],
						xDimension => [SFInt32, 0, field],
						xSpacing => [SFFloat, 1.0, field],
						zDimension => [SFInt32, 0, field],
						zSpacing => [SFFloat, 1.0, field],
						__PolyStreamed => [SFBool, 0, field],
						}),

	IndexedTriangleStripSet => new VRML::NodeType("IndexedTriangleStripSet", {
						set_colorIndex => [MFInt32, undef, eventIn],
						set_coordIndex => [MFInt32, undef, eventIn],
						set_normalIndex => [MFInt32, undef, eventIn],
						set_texCoordIndex => [MFInt32, undef, eventIn],
						color => [SFNode, NULL, exposedField],
						coord => [SFNode, NULL, exposedField],
						normal => [SFNode, NULL, exposedField],
						texCoord => [SFNode, NULL, exposedField],
						ccw => [SFBool, 1, field],
						colorIndex => [MFInt32, [], field],
						colorPerVertex => [SFBool, 1, field],
						convex => [SFBool, 1, field],
						coordIndex => [MFInt32, [], field],
						creaseAngle => [SFFloat, 0, field],
						normalIndex => [MFInt32, [], field],
						normalPerVertex => [SFBool, 1, field],
						solid => [SFBool, 1, field],
						texCoordIndex => [MFInt32, [], field],
						index => [MFInt32, [], field],
						fanCount => [MFInt32, [], field],
						stripCount => [MFInt32, [], field],

						set_height => [MFFloat, undef, eventIn],
						height => [MFFloat, [], field],
						xDimension => [SFInt32, 0, field],
						xSpacing => [SFFloat, 1.0, field],
						zDimension => [SFInt32, 0, field],
						zSpacing => [SFFloat, 1.0, field],
						__PolyStreamed => [SFBool, 0, field],
						}),

	LineSet => new VRML::NodeType("LineSet", {
						color => [SFNode, NULL, exposedField],
						coord => [SFNode, NULL, exposedField],
						vertexCount => [MFInt32,[],exposedField],
						__points  =>[FreeWRLPTR,0,field],
					   }),

	Normal => new VRML::NodeType("Normal", { vector => [MFVec3f, [], exposedField] }),

	PointSet => new VRML::NodeType("PointSet", {
						color => [SFNode, NULL, exposedField],
						coord => [SFNode, NULL, exposedField]
					   }),

	TriangleFanSet => new VRML::NodeType("TriangleFanSet", {
						set_colorIndex => [MFInt32, undef, eventIn],
						set_coordIndex => [MFInt32, undef, eventIn],
						set_normalIndex => [MFInt32, undef, eventIn],
						set_texCoordIndex => [MFInt32, undef, eventIn],
						color => [SFNode, NULL, exposedField],
						coord => [SFNode, NULL, exposedField],
						normal => [SFNode, NULL, exposedField],
						texCoord => [SFNode, NULL, exposedField],
						ccw => [SFBool, 1, field],
						colorIndex => [MFInt32, [], field],
						colorPerVertex => [SFBool, 1, field],
						convex => [SFBool, 1, field],
						coordIndex => [MFInt32, [], field],
						creaseAngle => [SFFloat, 0, field],
						normalIndex => [MFInt32, [], field],
						normalPerVertex => [SFBool, 1, field],
						solid => [SFBool, 1, field],
						texCoordIndex => [MFInt32, [], field],
						index => [MFInt32, [], field],
						fanCount => [MFInt32, [], field],
						stripCount => [MFInt32, [], field],

						set_height => [MFFloat, undef, eventIn],
						height => [MFFloat, [], field],
						xDimension => [SFInt32, 0, field],
						xSpacing => [SFFloat, 1.0, field],
						zDimension => [SFInt32, 0, field],
						zSpacing => [SFFloat, 1.0, field],
						__PolyStreamed => [SFBool, 0, field],
						}),

	TriangleStripSet => new VRML::NodeType("TriangleStripSet", {
						set_colorIndex => [MFInt32, undef, eventIn],
						set_coordIndex => [MFInt32, undef, eventIn],
						set_normalIndex => [MFInt32, undef, eventIn],
						set_texCoordIndex => [MFInt32, undef, eventIn],
						color => [SFNode, NULL, exposedField],
						coord => [SFNode, NULL, exposedField],
						normal => [SFNode, NULL, exposedField],
						texCoord => [SFNode, NULL, exposedField],
						ccw => [SFBool, 1, field],
						colorIndex => [MFInt32, [], field],
						colorPerVertex => [SFBool, 1, field],
						convex => [SFBool, 1, field],
						coordIndex => [MFInt32, [], field],
						creaseAngle => [SFFloat, 0, field],
						normalIndex => [MFInt32, [], field],
						normalPerVertex => [SFBool, 1, field],
						solid => [SFBool, 1, field],
						texCoordIndex => [MFInt32, [], field],
						index => [MFInt32, [], field],
						fanCount => [MFInt32, [], field],
						stripCount => [MFInt32, [], field],

						set_height => [MFFloat, undef, eventIn],
						height => [MFFloat, [], field],
						xDimension => [SFInt32, 0, field],
						xSpacing => [SFFloat, 1.0, field],
						zDimension => [SFInt32, 0, field],
						zSpacing => [SFFloat, 1.0, field],
						__PolyStreamed => [SFBool, 0, field],
						}),

	TriangleSet => new VRML::NodeType("TriangleSet", {
						set_colorIndex => [MFInt32, undef, eventIn],
						set_coordIndex => [MFInt32, undef, eventIn],
						set_normalIndex => [MFInt32, undef, eventIn],
						set_texCoordIndex => [MFInt32, undef, eventIn],
						color => [SFNode, NULL, exposedField],
						coord => [SFNode, NULL, exposedField],
						normal => [SFNode, NULL, exposedField],
						texCoord => [SFNode, NULL, exposedField],
						ccw => [SFBool, 1, field],
						colorIndex => [MFInt32, [], field],
						colorPerVertex => [SFBool, 1, field],
						convex => [SFBool, 1, field],
						coordIndex => [MFInt32, [], field],
						creaseAngle => [SFFloat, 0, field],
						normalIndex => [MFInt32, [], field],
						normalPerVertex => [SFBool, 1, field],
						solid => [SFBool, 1, field],
						texCoordIndex => [MFInt32, [], field],
						index => [MFInt32, [], field],
						fanCount => [MFInt32, [], field],
						stripCount => [MFInt32, [], field],

						set_height => [MFFloat, undef, eventIn],
						height => [MFFloat, [], field],
						xDimension => [SFInt32, 0, field],
						xSpacing => [SFFloat, 1.0, field],
						zDimension => [SFInt32, 0, field],
						zSpacing => [SFFloat, 1.0, field],
						__PolyStreamed => [SFBool, 0, field],
						}),


	###################################################################################

#			Shape Component

	###################################################################################

	Appearance => new VRML::NodeType ("Appearance", {
						 material => [SFNode, NULL, exposedField],
						 texture => [SFNode, NULL, exposedField],
						 textureTransform => [SFNode, NULL, exposedField],
						 lineProperties => [SFNode, NULL, exposedField],
						 fillProperties => [SFNode, NULL, exposedField],
						}),

	FillProperties => new VRML::NodeType ("FillProperties", {
						filled => [SFBool, 1, exposedField],
						hatchColor => [SFColor, [1,1,1], exposedField],
						hatched => [SFBool, 1, exposedField],
						hatchStyle => [SFInt32, 1, exposedField],
						}),

	LineProperties => new VRML::NodeType ("LineProperties", {
						applied => [SFBool, 1, exposedField],
						linetype => [SFInt32, 1, exposedField],
						linewidthScaleFactor => [SFFloat, 0, exposedField],
						}),

	Material => new VRML::NodeType ("Material", {
						 ambientIntensity => [SFFloat, 0.2, exposedField],
						 diffuseColor => [SFColor, [0.8, 0.8, 0.8], exposedField],
						 emissiveColor => [SFColor, [0, 0, 0], exposedField],
						 shininess => [SFFloat, 0.2, exposedField],
						 specularColor => [SFColor, [0, 0, 0], exposedField],
						 transparency => [SFFloat, 0, exposedField]
						}),

	Shape => new VRML::NodeType ("Shape", {
						 appearance => [SFNode, NULL, exposedField],
						 geometry => [SFNode, NULL, exposedField],
						 bboxCenter => [SFVec3f, [0, 0, 0], field],
						 bboxSize => [SFVec3f, [-1, -1, -1], field],
						 __OccludeNumber =>[SFInt32,-1,field], # for Occlusion tests.
						}),


	###################################################################################

	#		Geometry3D Component

	###################################################################################

	Box => new VRML::NodeType("Box", { 	size => [SFVec3f, [2, 2, 2], field],
						solid => [SFBool, 1, field],
						__points  =>[FreeWRLPTR,0,field],
					   }),

	Cone => new VRML::NodeType ("Cone", {
						 bottomRadius => [SFFloat, 1.0, field],
						 height => [SFFloat, 2.0, field],
						 side => [SFBool, 1, field],
						 bottom => [SFBool, 1, field],
						solid => [SFBool, 1, field],
						 __sidepoints =>[FreeWRLPTR,0,field],
						 __botpoints =>[FreeWRLPTR,0,field],
						 __normals =>[FreeWRLPTR,0,field],
						}),

	Cylinder => new VRML::NodeType ("Cylinder", {
						 bottom => [SFBool, 1, field],
						 height => [SFFloat, 2.0, field],
						 radius => [SFFloat, 1.0, field],
						 side => [SFBool, 1, field],
						 top => [SFBool, 1, field],
						solid => [SFBool, 1, field],
						 __points =>[FreeWRLPTR,0,field],
						 __normals =>[FreeWRLPTR,0,field],
						}),

	ElevationGrid => new VRML::NodeType("ElevationGrid", {
						set_colorIndex => [MFInt32, undef, eventIn],
						set_coordIndex => [MFInt32, undef, eventIn],
						set_normalIndex => [MFInt32, undef, eventIn],
						set_texCoordIndex => [MFInt32, undef, eventIn],
						color => [SFNode, NULL, exposedField],
						coord => [SFNode, NULL, exposedField],
						normal => [SFNode, NULL, exposedField],
						texCoord => [SFNode, NULL, exposedField],
						ccw => [SFBool, 1, field],
						colorIndex => [MFInt32, [], field],
						colorPerVertex => [SFBool, 1, field],
						convex => [SFBool, 1, field],
						coordIndex => [MFInt32, [], field],
						creaseAngle => [SFFloat, 0, field],
						normalIndex => [MFInt32, [], field],
						normalPerVertex => [SFBool, 1, field],
						solid => [SFBool, 1, field],
						texCoordIndex => [MFInt32, [], field],
						index => [MFInt32, [], field],
						fanCount => [MFInt32, [], field],
						stripCount => [MFInt32, [], field],

						set_height => [MFFloat, undef, eventIn],
						height => [MFFloat, [], field],
						xDimension => [SFInt32, 0, field],
						xSpacing => [SFFloat, 1.0, field],
						zDimension => [SFInt32, 0, field],
						zSpacing => [SFFloat, 1.0, field],
						__PolyStreamed => [SFBool, 0, field],
					   }),

	Extrusion => new VRML::NodeType("Extrusion", {
						set_crossSection => [MFVec2f, undef, eventIn],
						set_orientation => [MFRotation, undef, eventIn],
						set_scale => [MFVec2f, undef, eventIn],
						set_spine => [MFVec3f, undef, eventIn],
						beginCap => [SFBool, 1, field],
						ccw => [SFBool, 1, field],
						convex => [SFBool, 1, field],
						creaseAngle => [SFFloat, 0, field],
						crossSection => [MFVec2f, [[1, 1],[1, -1],[-1, -1],
									   [-1, 1],[1, 1]], field],
						endCap => [SFBool, 1, field],
						orientation => [MFRotation, [[0, 0, 1, 0]],field],

						scale => [MFVec2f, [[1, 1]], field],
						solid => [SFBool, 1, field],
						spine => [MFVec3f, [[0, 0, 0],[0, 1, 0]], field]
					   }),

	IndexedFaceSet => new VRML::NodeType("IndexedFaceSet", {
						set_colorIndex => [MFInt32, undef, eventIn],
						set_coordIndex => [MFInt32, undef, eventIn],
						set_normalIndex => [MFInt32, undef, eventIn],
						set_texCoordIndex => [MFInt32, undef, eventIn],
						color => [SFNode, NULL, exposedField],
						coord => [SFNode, NULL, exposedField],
						normal => [SFNode, NULL, exposedField],
						texCoord => [SFNode, NULL, exposedField],
						ccw => [SFBool, 1, field],
						colorIndex => [MFInt32, [], field],
						colorPerVertex => [SFBool, 1, field],
						convex => [SFBool, 1, field],
						coordIndex => [MFInt32, [], field],
						creaseAngle => [SFFloat, 0, field],
						normalIndex => [MFInt32, [], field],
						normalPerVertex => [SFBool, 1, field],
						solid => [SFBool, 1, field],
						texCoordIndex => [MFInt32, [], field],
						index => [MFInt32, [], field],
						fanCount => [MFInt32, [], field],
						stripCount => [MFInt32, [], field],

						set_height => [MFFloat, undef, eventIn],
						height => [MFFloat, [], field],
						xDimension => [SFInt32, 0, field],
						xSpacing => [SFFloat, 1.0, field],
						zDimension => [SFInt32, 0, field],
						zSpacing => [SFFloat, 1.0, field],
						__PolyStreamed => [SFBool, 0, field],
					   }),

	Sphere => new VRML::NodeType("Sphere",
					   { 	radius => [SFFloat, 1.0, field],
						solid => [SFBool, 1, field],
						 __points =>[FreeWRLPTR,0,field],
 					   }),


	###################################################################################

	#		Geometry 2D Component

	###################################################################################

	Arc2D => new VRML::NodeType("Arc2D", {
					    	endAngle => [SFFloat, 1.5707, field],
					    	radius => [SFFloat, 1.0, field],
					    	startAngle => [SFFloat, 0.0, field],
						__points  =>[FreeWRLPTR,0,field],
						__numPoints =>[SFInt32,0,field],
 					   }),

	ArcClose2D => new VRML::NodeType("ArcClose2D", {
						closureType => [SFString,"PIE",field],
					    	endAngle => [SFFloat, 1.5707, field],
					    	radius => [SFFloat, 1.0, field],
						solid => [SFBool, 0, field],
					    	startAngle => [SFFloat, 0.0, field],
						__points  =>[FreeWRLPTR,0,field],
						__numPoints =>[SFInt32,0,field],
 					   }),


	Circle2D => new VRML::NodeType("Circle2D", {
					    	radius => [SFFloat, 1.0, field],
						__points  =>[FreeWRLPTR,0,field],
						__numPoints =>[SFInt32,0,field],
 					   }),

	Disk2D => new VRML::NodeType("Disk2D", {
					    	innerRadius => [SFFloat, 0.0, field],
					    	outerRadius => [SFFloat, 1.0, field],
						solid => [SFBool, 0, field],
						__points  =>[FreeWRLPTR,0,field],
						__texCoords  =>[FreeWRLPTR,0,field],
						__numPoints =>[SFInt32,0,field],
						__simpleDisk => [SFBool,0,field],
 					   }),

	Polyline2D => new VRML::NodeType("Polyline2D", {
					    	lineSegments => [MFVec2f, [], field],
 					   }),

	Polypoint2D => new VRML::NodeType("Polypoint2D", {
					    	point => [MFVec2f, [], exposedField],
 					   }),

	Rectangle2D => new VRML::NodeType("Rectangle2D", {
					    	size => [SFVec2f, [2.0, 2.0], field],
						solid => [SFBool, 0, field],
						__points  =>[FreeWRLPTR,0,field],
						__numPoints =>[SFInt32,0,field],
 					   }),


	TriangleSet2D => new VRML::NodeType("TriangleSet2D", {
					    	vertices => [MFVec2f, [], exposedField],
						solid => [SFBool, 0, field],
						__texCoords  =>[FreeWRLPTR,0,field],
 					   }),

	###################################################################################

	#		Text Component

	###################################################################################

	Text => new VRML::NodeType ("Text", {
						 string => [MFString, [], exposedField],
						 fontStyle => [SFNode, NULL, exposedField],
						 length => [MFFloat, [], exposedField],
						solid => [SFBool, 1, field],
						 maxExtent => [SFFloat, 0, exposedField],
						 __rendersub => [SFInt32, 0, exposedField] # Function ptr hack
						}),

	FontStyle => new VRML::NodeType("FontStyle", {
						family => [MFString, ["SERIF"], field],
						horizontal => [SFBool, 1, field],
						justify => [MFString, ["BEGIN"], field],
						language => [SFString, "", field],
						leftToRight => [SFBool, 1, field],
						size => [SFFloat, 1.0, field],
						spacing => [SFFloat, 1.0, field],
						style => [SFString, "PLAIN", field],
						topToBottom => [SFBool, 1, field]
					   }), 

	###################################################################################

	#		Sound Component

	###################################################################################

	AudioClip => new VRML::NodeType("AudioClip", {
						description => [SFString, "", exposedField],
						loop =>	[SFBool, 0, exposedField],
						pitch => [SFFloat, 1.0, exposedField],
						startTime => [SFTime, 0, exposedField],
						stopTime => [SFTime, 0, exposedField],
						url => [MFString, [], exposedField],
						duration_changed => [SFTime, -1, eventOut],
						isActive => [SFBool, 0, eventOut],

						pauseTime => [SFTime,0,exposedField],
						resumeTime => [SFTime,0,exposedField],
						elapsedTime => [SFTime,0,eventOut],
						isPaused => [SFBool,0,eventOut],

						# parent url, gets replaced at node build time
						__parenturl =>[SFString,"",field],

						# internal sequence number
						__sourceNumber => [SFInt32, -1, field],
						# local name, as received on system
						__localFileName => [FreeWRLPTR, 0,field],
						# time that we were initialized at
						__inittime => [SFTime, 0, field],
					   }),

	Sound => new VRML::NodeType("Sound", {
						direction => [SFVec3f, [0, 0, 1], exposedField],
						intensity => [SFFloat, 1.0, exposedField],
						location => [SFVec3f, [0, 0, 0], exposedField],
						maxBack => [SFFloat, 10.0, exposedField],
						maxFront => [SFFloat, 10.0, exposedField],
						minBack => [SFFloat, 1.0, exposedField],
						minFront => [SFFloat, 1.0, exposedField],
						priority => [SFFloat, 0, exposedField],
						source => [SFNode, NULL, exposedField],
						spatialize => [SFBool,1, field]
					   }),

	###################################################################################

	#		Lighting Component

	###################################################################################

	DirectionalLight => new VRML::NodeType("DirectionalLight", {
						ambientIntensity => [SFFloat, 0, exposedField],
						color => [SFColor, [1, 1, 1], exposedField],
						direction => [SFVec3f, [0, 0, -1], exposedField],
						intensity => [SFFloat, 1.0, exposedField],
						on => [SFBool, 1, exposedField]
					   }),

	PointLight => new VRML::NodeType("PointLight", {
						ambientIntensity => [SFFloat, 0, exposedField],
						attenuation => [SFVec3f, [1, 0, 0], exposedField],
						color => [SFColor, [1, 1, 1], exposedField],
						intensity => [SFFloat, 1.0, exposedField],
						location => [SFVec3f, [0, 0, 0], exposedField],
						on => [SFBool, 1, exposedField],
						radius => [SFFloat, 100.0, exposedField],
						##not in the spec
						direction => [SFVec3f, [0, 0, -1.0], exposedField]
					   }),

	SpotLight => new VRML::NodeType("SpotLight", {
						ambientIntensity => [SFFloat, 0, exposedField],
						attenuation => [SFVec3f, [1, 0, 0], exposedField],
						beamWidth => [SFFloat, 1.570796, exposedField],
						color => [SFColor, [1, 1, 1], exposedField],
						cutOffAngle => [SFFloat, 0.785398, exposedField],
						direction => [SFVec3f, [0, 0, -1], exposedField],
						intensity => [SFFloat, 1.0, exposedField],
						location => [SFVec3f, [0, 0, 0], exposedField],
						on => [SFBool, 1, exposedField],
						radius => [SFFloat, 100.0, exposedField]
					   }),

	###################################################################################

	#		Texturing Component

	###################################################################################

	ImageTexture => new VRML::NodeType("ImageTexture", {
						url => [MFString, [], exposedField],
						repeatS => [SFBool, 1, field],
						repeatT => [SFBool, 1, field],
						__texture => [SFInt32, 0, field],
						__parenturl =>[SFString,"",field],
					   }),

	MovieTexture => new VRML::NodeType ("MovieTexture", {
						 loop => [SFBool, 0, exposedField],
						 speed => [SFFloat, 1.0, exposedField],
						 startTime => [SFTime, 0, exposedField],
						 stopTime => [SFTime, 0, exposedField],
						 url => [MFString, [""], exposedField],
						 repeatS => [SFBool, 1, field],
						 repeatT => [SFBool, 1, field],
						 duration_changed => [SFTime, -1, eventOut],
						 isActive => [SFBool, 0, eventOut],
						resumeTime => [SFTime,0,exposedField],
						pauseTime => [SFTime,0,exposedField],
						elapsedTime => [SFTime,0,eventOut],
						isPaused => [SFTime,0,eventOut],
						 # has the URL changed???
						 __oldurl => [MFString, [""], field],
						 # initial texture number
						 __texture0_ => [SFInt32, 0, field],
						 # last texture number
						 __texture1_ => [SFInt32, 0, field],
						 # which texture number is used
						 __ctex => [SFInt32, 0, field],
						 # time that we were initialized at
						 __inittime => [SFTime, 0, field],
						 # internal sequence number
						 __sourceNumber => [SFInt32, -1, field],
						# parent url, gets replaced at node build time
						__parenturl =>[SFString,"",field],
						}),


	MultiTexture => new VRML::NodeType("MultiTexture", {
						alpha =>[SFFloat, 1, exposedField],
						color =>[SFColor,[1,1,1],exposedField],
						function =>[MFString,[],exposedField],
						mode =>[MFString,[],exposedField],
						source =>[MFString,[],exposedField],
						texture=>[MFNode,undef,exposedField],
						__params => [FreeWRLPTR, 0, field],
					   }),

	MultiTextureCoordinate => new VRML::NodeType("MultiTextureCoordinate", {
						texCoord =>[MFNode,undef,exposedField],
					   }),

	MultiTextureTransform => new VRML::NodeType("MultiTextureTransform", {
						textureTransform=>[MFNode,undef,exposedField],
					   }),

	PixelTexture => new VRML::NodeType("PixelTexture", {
						image => [SFImage, "0, 0, 0", exposedField],
						repeatS => [SFBool, 1, field],
						repeatT => [SFBool, 1, field],
						__texture => [SFInt32, 0, field],
						__parenturl =>[SFString,"",field],
					   }),

	TextureCoordinate => new VRML::NodeType("TextureCoordinate", { 
						point => [MFVec2f, [], exposedField],
						__compiledpoint => [MFVec2f, [], field],
					 }),

	TextureCoordinateGenerator => new VRML::NodeType("TextureCoordinateGenerator", { 
						parameter => [MFFloat, [], exposedField],
						mode => [SFString,"SPHERE",exposedField],
						__compiledmode => [SFInt32,0,field],
					 }),

	TextureTransform => new VRML::NodeType ("TextureTransform", {
						 center => [SFVec2f, [0, 0], exposedField],
						 rotation => [SFFloat, 0, exposedField],
						 scale => [SFVec2f, [1, 1], exposedField],
						 translation => [SFVec2f, [0, 0], exposedField]
						}),


	###################################################################################

	#		Interpolation Component

	###################################################################################

	ColorInterpolator => new VRML::NodeType("ColorInterpolator", {
			set_fraction => [SFFloat, undef, eventIn],
			key => [MFFloat, [], exposedField],
			keyValue => [MFColor, [], exposedField],
			value_changed => [SFColor, [0, 0, 0], eventOut],
		   }),

	############################################################################33
	# CoordinateInterpolators and NormalInterpolators use almost the same thing;
	# look at the _type field.

	CoordinateInterpolator => new VRML::NodeType("CoordinateInterpolator", {
			set_fraction => [SFFloat, undef, eventIn],
			key => [MFFloat, [], exposedField],
			keyValue => [MFVec3f, [], exposedField],
			value_changed => [MFVec3f, [], eventOut],
			_type => [SFInt32, 0, exposedField], #1 means dont normalize
		}),

	NormalInterpolator => new VRML::NodeType("NormalInterpolator", {
			set_fraction => [SFFloat, undef, eventIn],
			key => [MFFloat, [], exposedField],
			keyValue => [MFVec3f, [], exposedField],
			value_changed => [MFVec3f, [], eventOut],
			_type => [SFInt32, 1, exposedField], #1 means normalize
		}),

	OrientationInterpolator => new VRML::NodeType("OrientationInterpolator", {
			set_fraction => [SFFloat, undef, eventIn],
			key => [MFFloat, [], exposedField],
			keyValue => [MFRotation, [], exposedField],
			value_changed => [SFRotation, [0, 0, 1, 0], eventOut]
		   }),

	PositionInterpolator => new VRML::NodeType("PositionInterpolator", {
			set_fraction => [SFFloat, undef, eventIn],
			key => [MFFloat, [], exposedField],
			keyValue => [MFVec3f, [], exposedField],
			value_changed => [SFVec3f, [0, 0, 0], eventOut],
		   }),


	ScalarInterpolator => new VRML::NodeType("ScalarInterpolator", {
			set_fraction => [SFFloat, undef, eventIn],
			key => [MFFloat, [], exposedField],
			keyValue => [MFFloat, [], exposedField],
			value_changed => [SFFloat, 0.0, eventOut]
		   }),

	CoordinateInterpolator2D => new VRML::NodeType("CoordinateInterpolator2D", {
			set_fraction => [SFFloat, undef, eventIn],
			key => [MFFloat, [], exposedField],
			keyValue => [MFVec2f, [], exposedField],
			value_changed => [MFVec2f, [], eventOut],
		}),

	PositionInterpolator2D => new VRML::NodeType("PositionInterpolator2D", {
			set_fraction => [SFFloat, undef, eventIn],
			key => [MFFloat, [], exposedField],
			keyValue => [MFVec2f, [], exposedField],
			value_changed => [SFVec2f, [0, 0, 0], eventOut],
		}),


	###################################################################################

	#		Pointing Device Component

	###################################################################################

	TouchSensor => new VRML::NodeType("TouchSensor", {
						enabled => [SFBool, 1, exposedField],
						hitNormal_changed => [SFVec3f, [0, 0, 0], eventOut],
						hitPoint_changed => [SFVec3f, [0, 0, 0], eventOut],
						hitTexCoord_changed => [SFVec2f, [0, 0], eventOut],
						isActive => [SFBool, 0, eventOut],
						isOver => [SFBool, 0, eventOut],
						description => [SFString, "", field],
						touchTime => [SFTime, -1, eventOut]
					   }),

	PlaneSensor => new VRML::NodeType("PlaneSensor", {
						autoOffset => [SFBool, 1, exposedField],
						enabled => [SFBool, 1, exposedField],
						maxPosition => [SFVec2f, [-1, -1], exposedField],
						minPosition => [SFVec2f, [0, 0], exposedField],
						offset => [SFVec3f, [0, 0, 0], exposedField],
						isActive => [SFBool, 0, eventOut],
						isOver => [SFBool, 0, eventOut],
						description => [SFString, "", field],
						trackPoint_changed => [SFVec3f, [0, 0, 0], eventOut],
						translation_changed => [SFVec3f, [0, 0, 0], eventOut],
						# where we are at a press...
						_origPoint => [SFVec3f, [0, 0, 0], field],
					   }),

	SphereSensor => new VRML::NodeType("SphereSensor", {
						autoOffset => [SFBool, 1, exposedField],
						enabled => [SFBool, 1, exposedField],
						offset => [SFRotation, [0, 1, 0, 0], exposedField],
						isActive => [SFBool, 0, eventOut],
						rotation_changed => [SFRotation, [0, 0, 1, 0], eventOut],
						trackPoint_changed => [SFVec3f, [0, 0, 0], eventOut],
						isOver => [SFBool, 0, eventOut],
						description => [SFString, "", field],
						# where we are at a press...
						_origPoint => [SFVec3f, [0, 0, 0], field],
						_radius => [SFFloat, 0, field],
					   }),

	CylinderSensor => new VRML::NodeType("CylinderSensor", {
						autoOffset => [SFBool, 1, exposedField],
						diskAngle => [SFFloat, 0.262, exposedField],
						enabled => [SFBool, 1, exposedField],
						maxAngle => [SFFloat, -1.0, exposedField],
						minAngle => [SFFloat, 0, exposedField],
						offset => [SFFloat, 0, exposedField],
						isActive => [SFBool, 0, eventOut],
						isOver => [SFBool, 0, eventOut],
						description => [SFString, "", field],
						rotation_changed => [SFRotation, [0, 0, 1, 0], eventOut],
						trackPoint_changed => [SFVec3f, [0, 0, 0], eventOut],
						# where we are at a press...
						_origPoint => [SFVec3f, [0, 0, 0], field],
						_radius => [SFFloat, 0, field],
					   }),


	###################################################################################

	#		Key Device Component

	###################################################################################

	# KeySensor
	# StringSensor

	###################################################################################

	#		Environmental Sensor Component

	###################################################################################


	ProximitySensor => new VRML::NodeType("ProximitySensor", {
						center => [SFVec3f, [0, 0, 0], exposedField],
						size => [SFVec3f, [0, 0, 0], exposedField],
						enabled => [SFBool, 1, exposedField],
						isActive => [SFBool, 0, eventOut],
						position_changed => [SFVec3f, [0, 0, 0], eventOut],
						orientation_changed => [SFRotation, [0, 0, 1, 0], eventOut],
						enterTime => [SFTime, -1, eventOut],
						exitTime => [SFTime, -1, eventOut],
						centerOfRotation_changed =>[SFVec3f, [0,0,0], eventOut],

						# These fields are used for the info.
						__hit => [SFInt32, 0, exposedField],
						__t1 => [SFVec3f, [10000000, 0, 0], exposedField],
						__t2 => [SFRotation, [0, 1, 0, 0], exposedField]
					   }),

	VisibilitySensor => new VRML::NodeType("VisibilitySensor", {
						center => [SFVec3f, [0, 0, 0], exposedField],
						enabled => [SFBool, 1, exposedField],
						size => [SFVec3f, [0, 0, 0], exposedField],
						enterTime => [SFTime, -1, eventOut],
						exitTime => [SFTime, -1, eventOut],
						isActive => [SFBool, 0, eventOut],
						 __OccludeNumber =>[SFInt32,-1,field], 	# for Occlusion tests.
						__points  =>[FreeWRLPTR,0,field],	# for Occlude Box.
						__Samples =>[SFInt32,0,field],		# Occlude samples from last pass
					   }),



	###################################################################################

	#		Navigation Component

	###################################################################################

	LOD => new VRML::NodeType("LOD", {
						 addChildren => [MFNode, undef, eventIn],
						 removeChildren => [MFNode, undef, eventIn],
						level => [MFNode, [], exposedField],
						center => [SFVec3f, [0, 0, 0],  field],
						range => [MFFloat, [], field],
						_selected =>[SFInt32,0,field],
					   }),

	Billboard => new VRML::NodeType("Billboard", {
						addChildren => [MFNode, undef, eventIn],
						removeChildren => [MFNode, undef, eventIn],
						axisOfRotation => [SFVec3f, [0, 1, 0], exposedField],
						children => [MFNode, [], exposedField],
						bboxCenter => [SFVec3f, [0, 0, 0], field],
						bboxSize => [SFVec3f, [-1, -1, -1], field]
					   }),

	Collision => new VRML::NodeType("Collision", {
						addChildren => [MFNode, undef, eventIn],
						removeChildren => [MFNode, undef, eventIn],
						children => [MFNode, [], exposedField],
						collide => [SFBool, 1, exposedField],
						bboxCenter => [SFVec3f, [0, 0, 0], field],
						bboxSize => [SFVec3f, [-1, -1, -1], field],
						proxy => [SFNode, NULL, field],
						collideTime => [SFTime, -1, eventOut],
						# return info for collisions
						# bit 0 : collision or not
						# bit 1: changed from previous of not
						__hit => [SFInt32, 0, exposedField]
					   }),

	Viewpoint => new VRML::NodeType("Viewpoint", {
						set_bind => [SFBool, undef, eventIn],
						fieldOfView => [SFFloat, 0.785398, exposedField],
						jump => [SFBool, 1, exposedField],
						orientation => [SFRotation, [0, 0, 1, 0], exposedField],
						position => [SFVec3f,[0, 0, 10], exposedField],
						description => [SFString, "", field],
						bindTime => [SFTime, -1, eventOut],
						isBound => [SFBool, 0, eventOut],
						centerOfRotation =>[SFVec3f, [0,0,0], exposedField],
					   }),

	NavigationInfo => new VRML::NodeType("NavigationInfo", {
						set_bind => [SFBool, undef, eventIn],
						avatarSize => [MFFloat, [0.25, 1.6, 0.75], exposedField],
						headlight => [SFBool, 1, exposedField],
						speed => [SFFloat, 1.0, exposedField],
						type => [MFString, ["WALK", "ANY"], exposedField],
						visibilityLimit => [SFFloat, 0, exposedField],
						isBound => [SFBool, 0, eventOut],
						transitionType => [MFString, [],exposedField],
						bindTime => [SFTime, -1, eventOut],
					   }),

	###################################################################################

	#		Environmental Effects Component

	###################################################################################

	Fog => new VRML::NodeType("Fog", {
						set_bind => [SFBool, undef, eventIn],
						color => [SFColor, [1, 1, 1], exposedField],
						fogType => [SFString, "LINEAR", exposedField],
						visibilityRange => [SFFloat, 0, exposedField],
						isBound => [SFBool, 0, eventOut]
					   }),

	TextureBackground => new VRML::NodeType("TextureBackground", {
						set_bind => [SFBool, undef, eventIn],
						groundAngle => [MFFloat, [], exposedField],
						groundColor => [MFColor, [], exposedField],
						skyAngle => [MFFloat, [], exposedField],
						skyColor => [MFColor, [[0, 0, 0]], exposedField],
						bindTime => [SFTime,0,eventOut],
						isBound => [SFBool, 0, eventOut],
						__parenturl =>[SFString,"",field],
						__points =>[FreeWRLPTR,0,field],
						__colours =>[FreeWRLPTR,0,field],
						__quadcount => [SFInt32,0,field],
						__BGNumber => [SFInt32,-1,field], # for ordering backgrounds for binding

						frontTexture=>[SFNode,NULL,exposedField],
						backTexture=>[SFNode,NULL,exposedField],
						topTexture=>[SFNode,NULL,exposedField],
						bottomTexture=>[SFNode,NULL,exposedField],
						leftTexture=>[SFNode,NULL,exposedField],
						rightTexture=>[SFNode,NULL,exposedField],
						transparency=> [MFFloat,[0],exposedField],
					   }),

	Background => new VRML::NodeType("Background", {
						set_bind => [SFBool, undef, eventIn],
						groundAngle => [MFFloat, [], exposedField],
						groundColor => [MFColor, [], exposedField],
						skyAngle => [MFFloat, [], exposedField],
						skyColor => [MFColor, [[0, 0, 0]], exposedField],
						bindTime => [SFTime,0,eventOut],
						isBound => [SFBool, 0, eventOut],
						__parenturl =>[SFString,"",field],
						__points =>[FreeWRLPTR,0,field],
						__colours =>[FreeWRLPTR,0,field],
						__quadcount => [SFInt32,0,field],
						__BGNumber => [SFInt32,-1,field], # for ordering backgrounds for binding

						(map {(
							   $_.Url => [MFString, [], exposedField],
							   # OpenGL texture number
							   __texture.$_ => [SFInt32, 0, exposedField],
							  )} qw/back front top bottom left right/),
					   }),

	###################################################################################

	#		Geospatial Component

	###################################################################################


	GeoCoordinate => new VRML::NodeType("GeoCoordinate", {
						geoOrigin => [SFNode, NULL, field],
						geoSystem => [MFString,["GD","WE"],field],
						point => [MFString,[],field],
					}),

	GeoElevationGrid => new VRML::NodeType("GeoElevationGrid", {
						set_height => [MFFloat, undef, eventIn],
						set_YScale => [MFFloat, undef, eventIn],
						color => [SFNode, NULL, exposedField],
						normal => [SFNode, NULL, exposedField],
						texCoord => [SFNode, NULL, exposedField],
						ccw => [SFBool,1,field],
						colorPerVertex => [SFBool, 1, field],
						creaseAngle => [SFFloat, 0, field],
						geoOrigin => [SFNode, NULL, field],
						geoSystem => [MFString,["GD","WE"],field],
						geoGridOrigin => [SFString,"0 0 0",field],
						height => [MFFloat, [], field],
						normalPerVertex => [SFBool, 1, field],
						solid => [SFBool, 1, field],
						xDimension => [SFInt32, 0, field],
						xSpacing => [SFString, "1.0", field],
						yScale => [SFFloat, 1.0, field],
						zDimension => [SFInt32, 0, field],
						zSpacing => [SFString, "1.0", field]
					}),

	GeoLOD => new VRML::NodeType("GeoLOD", {
						center => [SFString,"",field],
						child1Url =>[MFString,[],field],
						child2Url =>[MFString,[],field],
						child3Url =>[MFString,[],field],
						child4Url =>[MFString,[],field],
						geoOrigin => [SFNode, NULL, field],
						geoSystem => [MFString,["GD","WE"],field],
						range => [SFFloat,10.0,field],
						rootUrl => [MFString,[],field],
						rootNode => [MFNode,[],field],
						children => [MFNode,[],eventOut],
					}),


	GeoMetadata=> new VRML::NodeType("GeoMetadata", {
						data => [MFNode,[],exposedField],
						summary => [MFString,[],exposedField],
						url => [MFString,[],exposedField],
					}),

	GeoPositionInterpolator=> new VRML::NodeType("GeoPositionInterpolator", {
						set_fraction => [SFFloat,undef,eventIn],
						geoOrigin => [SFNode, NULL, field],
						geoSystem => [MFString,["GD","WE"],field],
						key => [MFFloat,[],field],
						keyValue => [MFString,[],field],
						geovalue_changed => [SFString,"",eventOut],
						value_changed => [SFVec3f,undef,eventOut],
					}),


	GeoTouchSensor=> new VRML::NodeType("GeoTouchSensor", {
						enabled => [SFBool,1,exposedField],
						geoOrigin => [SFNode, NULL, field],
						geoSystem => [MFString,["GD","WE"],field],
						hitNormal_changed => [SFVec3f, [0, 0, 0], eventOut],
						hitPoint_changed => [SFVec3f, [0, 0, 0], eventOut],
						hitTexCoord_changed => [SFVec2f, [0, 0], eventOut],
						hitGeoCoord_changed => [SFString,"",eventOut],
						isActive => [SFBool, 0, eventOut],
						isOver => [SFBool, 0, eventOut],
						description => [SFString, "", field],
						touchTime => [SFTime, -1, eventOut]
					}),

	GeoViewpoint => new VRML::NodeType("GeoViewpoint", {
						set_bind => [SFBool, undef, eventIn],
						set_orientation => [SFString, undef, eventIn],
						set_position => [SFString, undef, eventIn],
						fieldOfView => [SFFloat, 0.785398, exposedField],
						headlight => [SFBool, 1, exposedField],
						jump => [SFBool, 1, exposedField],
						navType => [MFString, ["EXAMINE","ANY"],exposedField],
						description => [SFString, "", field],
						geoOrigin => [SFNode, NULL, field],
						geoSystem => [MFString,["GD","WE"],field],
						orientation => [SFRotation, [0, 0, 1, 0], field],
						position => [SFString,"0, 0, 100000", field],
						speedFactor => [SFFloat,1.0,field],
						bindTime => [SFTime, -1, eventOut],
						isBound => [SFBool, 0, eventOut],

						# "compiled" versions of strings above
						__position => [SFVec3f,[0, 0, 100000], field],
						__geoSystem => [SFInt32,0,field],
					   }),

	GeoOrigin => new VRML::NodeType("GeoOrigin", {
						geoSystem => [MFString,["GD","WE"],exposedField],
						geoCoords => [SFString,"",exposedField],
						rotateYUp => [SFBool,0,field],

						# these are now static in CFuncs/GeoVRML.c
						# "compiled" versions of strings above
						#__geoCoords => [SFVec3f,[0, 0, 0], field],
						#__geoSystem => [SFInt32,0,field],
					}),

	GeoLocation => new VRML::NodeType("GeoLocation", {
						geoCoords => [SFString,"",exposedField],
						children => [MFNode, [], field],
						geoOrigin => [SFNode, NULL, field],
						geoSystem => [MFString,["GD","WE"],field],
						bboxCenter => [SFVec3f, [0, 0, 0], field],
						bboxSize => [SFVec3f, [-1, -1, -1], field],

						# "compiled" versions of strings above
						__geoCoords => [SFVec3f,[0, 0, 0], field],
						__geoSystem => [SFInt32,0,field],

					}),


	###################################################################################

	#		H-Anim Component

	###################################################################################

	HAnimDisplacer => new VRML::NodeType("HAnimDisplacer", {
						coordIndex => [MFInt32, [], exposedField],
						displacements => [MFVec3f, [], exposedField],
						name => [SFString, "", exposedField],
						weight => [SFFloat, 0.0, expoeseField],
					}),

	HAnimHumanoid => new VRML::NodeType("HAnimHumanoid", {
						center => [SFVec3f, [0, 0, 0], exposedField],
						info => [MFString, [],exposedField],
						joints => [MFNode,[],exposedField],
						name => [SFString, "", exposedField],
						rotation => [SFRotation,[0,0,1,0], exposedField],
						scale => [SFVec3f,[1,1,1],exposedField],
						scaleOrientation => [SFRotation, [0, 0, 1, 0], exposedField],
						segments => [MFNode,[],exposedField],
						sites => [MFNode,[],exposedField],
						skeleton => [MFNode,[],exposedField],
						skin => [MFNode,[],exposedField],
						skinCoord => [SFNode, NULL, exposedField],
						skinNormal => [SFNode, NULL, exposedField],
						translation => [SFVec3f, [0, 0, 0], exposedField],
						version => [SFString,"",exposedField],
						viewpoints => [MFNode,[],exposedField],
						bboxCenter => [SFVec3f, [0, 0, 0], field],
						bboxSize => [SFVec3f, [-1, -1, -1], field],
					}),

	HAnimJoint => new VRML::NodeType("HAnimJoint", {

						addChildren => [MFNode, undef, eventIn],
						removeChildren => [MFNode, undef, eventIn],
						children => [MFNode, [], exposedField],
						center => [SFVec3f, [0, 0, 0], exposedField],
						children => [MFNode, [], exposedField],
						rotation => [SFRotation, [0, 0, 1, 0], exposedField],
						scale => [SFVec3f, [1, 1, 1], exposedField],
						scaleOrientation => [SFRotation, [0, 0, 1, 0], exposedField],
						translation => [SFVec3f, [0, 0, 0], exposedField],
						displacers => [MFNode, [], exposedField],
						limitOrientation => [SFRotation, [0, 0, 1, 0], exposedField],
						llimit => [MFFloat,[],exposedField],
						name => [SFString, "", exposedField],
						skinCoordIndex => [MFInt32,[],exposedField],
						skinCoordWeight => [MFFloat,[],exposedField],
						stiffness => [MFFloat,[0,0,0],exposedField],
						ulimit => [MFFloat,[],exposedField],
						bboxCenter => [SFVec3f, [0, 0, 0], field],
						bboxSize => [SFVec3f, [-1, -1, -1], field],

						 # fields for reducing redundant calls
						 __do_center => [SFInt32, 0, field],
						 __do_trans => [SFInt32, 0, field],
						 __do_rotation => [SFInt32, 0, field],
						 __do_scaleO => [SFInt32, 0, field],
						 __do_scale => [SFInt32, 0, field],
					}),

	HAnimSegment => new VRML::NodeType("HAnimSegment", {
						addChildren => [MFNode, undef, eventIn],
						removeChildren => [MFNode, undef, eventIn],
						children => [MFNode, [], exposedField],
						name => [SFString, "", exposedField],
						bboxCenter => [SFVec3f, [0, 0, 0], field],
						bboxSize => [SFVec3f, [-1, -1, -1], field],
						centerOfMass => [SFVec3f, [0, 0, 0], exposedField],
						coord => [SFNode, NULL, exposedField],
						displacers => [MFNode,[],exposedField],
						mass => [SFFloat, 0, exposedField],
						momentsOfInertia =>[MFFloat, [0, 0, 0, 0, 0, 0, 0, 0, 0],exposedField],
					}),



	HAnimSite => new VRML::NodeType("HAnimSite", {
						addChildren => [MFNode, undef, eventIn],
						removeChildren => [MFNode, undef, eventIn],
						children => [MFNode, [], exposedField],
						name => [SFString, "", exposedField],
						bboxCenter => [SFVec3f, [0, 0, 0], field],
						bboxSize => [SFVec3f, [-1, -1, -1], field],
						center => [SFVec3f, [0, 0, 0], exposedField],
						children => [MFNode, [], exposedField],
						rotation => [SFRotation, [0, 0, 1, 0], exposedField],
						scale => [SFVec3f, [1, 1, 1], exposedField],
						scaleOrientation => [SFRotation, [0, 0, 1, 0], exposedField],
						translation => [SFVec3f, [0, 0, 0], exposedField],

						 # fields for reducing redundant calls
						 __do_center => [SFInt32, 0, field],
						 __do_trans => [SFInt32, 0, field],
						 __do_rotation => [SFInt32, 0, field],
						 __do_scaleO => [SFInt32, 0, field],
						 __do_scale => [SFInt32, 0, field],
					}),


	###################################################################################

	#		NURBS Component

	###################################################################################

	Contour2D => new VRML::NodeType("Contour2D", {
						addChildren => [MFNode, undef, eventIn],
						removeChildren => [MFNode, undef, eventIn],
						children => [MFNode, [], exposedField],
					}),


	ContourPolyLine2D =>
	new VRML::NodeType("ContourPolyline2D",
					{
					}
					),

	NurbsCurve =>
	new VRML::NodeType("NurbsCurve",
					{
						controlPoint =>[MFVec3f,[],exposedField],
						weight => [MFFloat,[],exposedField],
						tessellation => [SFInt32,0,exposedField],
						knot => [MFFloat,[],field],
						order => [SFInt32,3,field],
					}
					),

	NurbsCurve2D =>
	new VRML::NodeType("NurbsCurve2D",
					{
						controlPoint =>[MFVec2f,[],exposedField],
						weight => [MFFloat,[],exposedField],
						tessellation => [SFInt32,0,exposedField],
						knot => [MFFloat,[],field],
						order => [SFInt32,3,field],
					}
					),

	NurbsGroup =>
	new VRML::NodeType("NurbsGroup",
					{
						addChildren => [MFNode, undef, eventIn],
						removeChildren => [MFNode, undef, eventIn],
						children => [MFNode, [], exposedField],
						tessellationScale => [SFFloat,1.0,exposedField],
						bboxCenter => [SFVec3f, [0, 0, 0], field],
						bboxSize => [SFVec3f, [-1, -1, -1], field],
					}
					),

	NurbsPositionInterpolator =>
	new VRML::NodeType("NurbsPositionInterpolator",
					{

						set_fraction => [SFFloat,undef,eventIn],
						dimension => [SFInt32,0,exposedField],
						keyValue => [MFVec3f,[],exposedField],
						keyWeight => [MFFloat,[],exposedField],
						knot => [MFFloat,[],exposedField],
						order => [SFInt32,[],exposedField],
						value_changed => [SFVec3f,undef,eventOut],
					}
					),

	NurbsSurface =>
	new VRML::NodeType("NurbsSurface",
					{
						controlPoint =>[MFVec3f,[],exposedField],
						texCoord => [SFNode, NULL, exposedField],
						uTessellation => [SFInt32,0,exposedField],
						vTessellation => [SFInt32,0,exposedField],
						weight => [MFFloat,[],exposedField],
						ccw => [SFBool,1,field],

						knot => [MFFloat,[],field],
						order => [SFInt32,3,field],
					}
					),
	NurbsTextureSurface =>
	new VRML::NodeType("NurbsTextureSurface",
					{
					}
					),

	NurbsTrimmedSurface =>
	new VRML::NodeType("NurbsTrimmedSurface",
					{
					}
					),


	###################################################################################

	#		Scripting Component

	###################################################################################
	Script =>
	new VRML::NodeType("Script",
					   {
						url => [MFString, [], exposedField],
						directOutput => [SFBool, 0, field],
						mustEvaluate => [SFBool, 0, field],
					   },
					  ),

	###################################################################################

	#		EventUtilities Component

	###################################################################################

	###################################################################################

	#		Old VRML nodes.

	###################################################################################
	# Internal structures, to store def and use in the right way

	InlineLoadControl =>
	new VRML::NodeType("InlineLoadControl",
					{
						load =>[SFBool,TRUE,exposedField],
						url => [MFString,[],exposedField],
						bboxCenter => [SFVec3f, [0, 0, 0], field],
						bboxSize => [SFVec3f, [-1, -1, -1], field],
						children => [MFNode, [], eventOut],
						__loadstatus =>[SFInt32,0,field],
						__parenturl =>[SFString,"",field],
					}
					),


); 


1;

