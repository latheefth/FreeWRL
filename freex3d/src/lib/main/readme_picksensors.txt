/* POINTING DEVICE SENSOR CONMPONENT
http://www.web3d.org/files/specifications/19775-1/V3.3/Part01/components/pointingsensor.html
- this describes how the pointing device sensors:
 TouchSensor, DragSensors: CylinderSensor, SphereSensor, PlaneSensor, [LineSensor]
 work as seen by the user:
 - dragsensor - the parent's descendent geometry is used only to identify and activate the dragsensor node. 
	After that it's the dragsensor's geometry (Cylinder/Sphere/Plane/Line) (not the parent's descendent geometry) 
	that's intersected with the pointing device bearing/ray to generate trackpoint_changed 
	and translation_ or rotation_changed events in sensor-local coordinates
- touchsensor - generates events in touchsensor-local coordinates

Terminology ([] denotes alternative design not yet implemented April 2014):
<node>-local - coordinates relative to a given node
modelview matrix - transforms coords from the current node-local into the current viewpoint-local
proj matrix - projects points from viewpoint-local 3D to 2D normalized screen viewport (with Z in 0 to 1 range)
pick-proj matrix - special projection matrix that aligns the mousexy (pickpoint) on the viewport center
	- formed in setup_pickray(pick=TRUE,,)
view matrix - the view part of the modelview matrix, generated during setup_viewpoint()
model matrix - the model part of the world modelview matrix, generated during
	traversing the scenegraph in render_node()
world - coordinates at the root node level ie with no transforms applied
	viewport-local -[proj] - viewpoint-local - [view] - world - [model] - node-local
modelview matrix: combined model * view matrices
place - position in the scenegraph transform hierarchy (with DEF/USE, a node can be in multiple places)
geometry_model: the model part of the modelview matrix at a geometry node's place
sensor_model: the model part of the modelview matrix at a sensor node's place
bearing: 2 points (A,B) defining a ray in 3D space going from A in the direction of B
	1. For 2D screen-based pointing devices	like mouse, A is the viewpoint position 0,0,0 
		in viewpoint-local, and for B mousexy is converted in setup_pickray(pick=TRUE,,) 
		to a special pick-proj matrix that centers the viewport on the mousexy, so B is 0,0,-1 in
			pick-viewport-local coordinates	(glu_unproject produces pick-viewport-local)
		[using normal projection matrix for the bearing, then B= mousex,mousey,-1 in 
		normal viewport-local coordinates, and glu_unproject is used to get this bearing
		into world coordinates for the remainder of PointingDeviceSensor activity]
	[2.for a 3D pointing device, it would have its own A,B in world. And then that world bearing 
		can be transformed into geometry-local and sensor-local and intersections with 
		shape-geometry and sensor-geometry calculated. dug9 Sept 2, 2014: FLY keyboard is 6DOF/3D, and not in world]
hit: an intersection between the bearing and geometry that is closer to A (supercedes previous cloest)

As seen by the developer, here's how I (dug9 Apr 2014) think they should work internally, on each frame 
 0. during parsing, when adding a parent to a node, check the node type and if a sensor type, 
		create a SensorEvent (by calling setSensitive(node,parent)) and set the SensorEvent[i].datanode=sensor
		and SensorEvent[i].fromNode = parent
 1. in startofloopnodeupdates() for each sensor node, flag its immediate Group/Transform parent 
	with VF_Sensitive [the sensor's ID]	so when traversing the scenegraph, this 'sensitivity' 
	can be pushed onto a state stack to affect descendent geometry nodes
 2. from the scene root, traverse to the viewpoint to get the view part of the modelview matrix
	-Set up 2 points in viewpoint space: A=0,0,0 (the viewpoint) and B=(mouse x, mouse y, z=-1) to represent the bearing
	-apply a special pick-projection matrix, in setup_pickray(pick=TRUE,,), so that glu_unproject 
		is with respect to the mouse-xy bearing B point being at the center of the viewport 
		[use normal projection, and minimize the use of glu_unproject by transforming viewport-local bearing
		to world during setup, and use world for bearing for all subsequent PointingDeviceSensor activities
		dug9 Sept 2014: or keep in avatar coords, and set up a avatar2pickray affine transform]
 3. from the scene root, in in render_hier(VF_Sensitive) pass, tranverse the scenegraph looking for sensitive 
	group/transform parent nodes, accumulating the model part of the modelview matrix as normal.
	When we find a sensitive parent: 
	a) save the current best-hit parent-node* to a C local variable, and set the current best node* to the 
		current parent	[push the parent's sensor node ID onto a sensor_stack]
	b) save the current hit modelview matrix to a C local variable, and set it to the parent's modelview matrix 
		[snapshot the current modelview matrix, and push onto a sensor_model stack]
	When we find descendent geometry to a sensitive (grand)parent:
	a) use glu_unproject with the pick-proj and current modelview matrix to re-transform the bearing 
		into geometry-local space [invert the cumulative model matrix, and use it to transform 
		world bearing (A,B) into geometry-local]
		-so we have a bearing (A',B') in geometry-local (this requires fewer math ops than 
		transforming all geometry vertices into pick-viewport-local [bearing-world])
	b) intersect the infinite line passing through A' and B' 
		with the geometry to get a list of intersection points n,xyz[n] in geometry-local space, or for 
		well-known geometry sphere,box,cylinder,rectangle2D... compute and test intersections one by one
	c) for each intersection point:
		- filter the intersection point to elliminate if in the -B direction from A (behind the viewpoint)
		- transform using modelview and pick-proj matrix into pick-viewport-local coordinates 
		  [transform using model into bearing-world coordinates]
		- compare the remaining intersection point by distance from A, and choose it over 
			the current one if closer to A (the other is 'occluded') to produce a new hit
	d) if there was a new hit, record some details needed by the particular type of sensor node, (but don't
		generate events yet - we have to search for even closer hits):
		- sensor's parent (Group/Transform) node* [+ Sensor node* ]
		- sensor's parent modelview and pick-proj matrices [sensor's parent model matrix]
		- hit point on decendent geometry surface, in pick-viewport-local [sensor-local]
			(the geometry node may be in descendant transform groups from the sensor's parent, so needs to be 
			transformed up to the Sensor-local system at some point, using 
			the sensor's modelview and pick-proj matrices [sensor_model matrix] we saved)
		- TouchSensor: 
			a) texture coordinates at the hitpoint
			b) normal to surface at hitpoint [transformed into sensor-local system]
  4. once finished searching for sensor nodes, and back in mainloop at the root level, if there 
		was a hit, and the hit is on sensitive geometry, call a function to look up the sensor node* given
		the parent node* in SensorEvents array, and generate events from the sensor node

What's not shown above: 
6. what happens when 2+ sensor nodes apply to the same geometry:
	for example, parent [ sensor1, group [ sensor 2, shape ] ]
	the specs say it's the sensor that's the nearest co-descendent (sibling or higher) to the geometry [sensor2,shape]
	(it doesn't say what happens in a tie ie 2 different sensor nodes are siblings) [sensor1,sensor2,shape]
	(it doesn't say what happens if you DEF a sensornode and USE it in various places)
7. what happens on each frame during a Drag:
	the successful dragsensor node is 'chosen' on the mouse-down
	on mouse-move: intersections with other occluding geometry, and geometry sensitive to other sensor nodes, are ignored, 
	- but intersection with the same dragsensor's geometry are updated for trackpoint_changed eventout
	- but because typically you move the geometry you are dragging, it and the dragsensor are 
		moved on each drag/mousemove, so you can't intersect the bearing with the updated sensor geometry position
		because that would produce net zero movement. Rather you need to 'freeze' the pose of the dragsensor geometry in the scene
		on mousdown, then on each mousemove/drag frame, intersect the new pointer bearing with 
		the original/frozen/mousedown dragsensor geometry pose. Then on mouse-up you unfreeze so next mousedown 
		you are starting fresh on the translated sensor geometry.
8. where things are stored
	The details for the hit are stored in a global hitCursor struct [struct array]
	render_node() has some C local variables which get pushed onto the C call stack when recursing
	[render_node() uses an explicit stack for sensor_model and sensor_parent_node* to apply to descendent geometry]
9. any special conditions for touch devices: none [multiple bearings processed on the same pass, for multi-touch, 
	PlaneSensor.rotation_changed, PlaneSensor.scale_changed, LineSensor.scale_changed events added to specs]

More implemnetation details - these may change, a snapshot April 2014
Keywords:
	Sensitive - all pointingdevice sensors
	HyperSensitive - drag sensors in the middle of a drag
Storage types:
struct currayhit {
	struct X3D_Node *hitNode; // What node hit at that distance? 
	- It's the parent Group or Transform to the Sensor node (the sensor node* gets looked up from parent node*
		in SensorEvents[] array, later in sendSensorEvents())
	GLDOUBLE modelMatrix[16]; // What the matrices were at that node
	- a snapshot of modelview at the sensornode or more precisely it's immediate parent Group or Transform
	- it's the whole modelview matrix
	GLDOUBLE projMatrix[16]; 
	- snapshot of the pick-projection matrix at the same spot
	-- it will include the pick-specific projection matrix aligned to the mousexy in setup_pickray(pick=TRUE,,)
};
global variables:
	struct point_XYZ r1 = {0,0,-1},r2 = {0,0,0},r3 = {0,1,0}; 
		pick-viewport-local axes: r1- along pick-proj axis, r2 viewpoint, r3 y-up axis in case needed
	hyp_save_posn, t_r2 - A - (viewpoint 0,0,0 transformed by modelviewMatrix.inverse() to geometry-local space)
	hyp_save_norm, t_r1 - B - bearing point (viewport 0,0,-1 used with pick-proj bearing-specific projection matrix)
		- norm is not a direction vector, its a point. To get a direction vector: v = (B - A) = (norm - posn)
	ray_save_posn - intersection with scene geometry, in sensor-local coordinates 
		- used in do_CyclinderSensor, do_SphereSensor for computing a radius  on mouse-down
	t_r3 - viewport y-up in case needed
call stacks:
resource thread > parsing > setParent > setSensitive
mainloop > setup_projection > glu_pick
mainloop > render_hier(VF_Sensitive) > render_Node() > upd_ray(), (node)->rendray_<geom> > rayhit()
mainloop > sendSensorEvent > get_hyperHit(), .interptr()={do_TouchSensor / do_<Drag>Sensor /..}
funcion-specific variables:
rayhit()
	Renderfuncs.hp.xyz - current closest hit, in bearing-local system
	RenderFuncs.hitPointDist - current distance to closest hit from viewpoint 0,0,0 to geometry intersection (in viewpoint scale)
	rayHit - snapshot of sensor's modelview matrix to go with closest-hit-so-far
	//rayHitHyper - "
render_node(): - a few variables acting as a stack by using automatic/local C variables in recursive calling
	int srg - saves current renderstate.render_geom
	int sch - saves current count of the hits from a single geometry node before trying to to intersect another geometry
	struct currayhit *srh - saves the current best hit rayph on the call stack
	rayph.modelMatrix- sensor-local snapshot of modelview matrix 
	rayph.viewMatrix - " proj "
		-- this could have been a stack, with a push for each direct parent of a pointingdevice sensor
SIBLING_SENSITIVE(node) - macro that checks if a node is VF_Sensitive and if so, sets up a parent vector
	n->_renderFlags = n->_renderFlags  | VF_Sensitive; - for each parent n, sets the parent sensitive
		(which indirectly sensitizes the siblings)
geometry nodes have virt->rendray_<shapenodetype>() functions called unconditionally on VF_Sensitive pass
	- ie rendray_Box, rendray_Sphere, rendray_Circle2D
	- called whether or not they are sensitized, to find an intersection and determine if they a) occlude 
			or b) succeed other sensitized geometry along the bearing
upd_ray() - keeps bearing A,B transformed to geometry-local A',B' - called after a push or pop to the modelview matrix stack.
rayhit() - accepts intersection of A',B' line with geometry in geometry-local space, 
	- filters if behind viewpoint,
	- sees if it's closer to A' than current, if so 
		- transforms hit xyz from geometry-local into bearing-local (pick-viewport-local) [model] and stores as hp.xyz
		- snapshots the sensor-local modelview and stores as .rayHit, for possible use in get_hyperhit()
get_hyperhit() for DragSensors: transforms the current-frame bearing from bearing-local 
		into the sensor-local coordinate system of the winning sensor, in a generic way that works 
		for all the DragSensor types in their do_<Drag>Sensor function. 
		- ray_save_posn, hyp_save_posn, hyp_save_norm
sendSensorEvents(node,event,button,status) - called from mainloop if there was a any pointing sensor hits
	- looks up the sensor node* from the parent node*
	- calls the do_<sensortype> function ie do_PlaneSensor(,,,)
do_TouchSensor, do_<dragSensorType>: do_CylinderSensor, do_SphereSensor, do_PlaneSensor, [do_LineSensor not in specs, fw exclusive]
	- compute output events and update any carryover sensor state variables
		- these are called from mainloop ie mainloop > sendSensorEvents > do_PlaneSensor
		- because its not called from the scenegraph where the SensorNode is, and because 
			the Sensor evenouts need to be in sensor-local coordinates, the inbound variables
			need to already be in sensor-local coordinates ie posn,norm are in sensor-local


What I think we could do better:
1. world bearing coords:
	Benefits of converting mousexy into world bearing(A,B) once per frame before sensitive pass:
	a) 3D PointingDevices generate bearing(A,B) in world coordinates
		dug9 Aug31, 2014: not really. FLY keyboard is 6DOF/3D, and works in current-viewpoint space, 
			adding relative motions on each tick. If I had a 3D pointing device, I would use it the same way
	b) separate PickingComponent (not yet fully implemented) works in world coordinates
	- so these two can share code if PointingDevice bearing from mouse is in world
	c) minimizes the use of glu_unproject (a compound convenience function with expensive 
		matrix inverse)
		dug9 Aug31, 2014: to do a ray_intersect_geometry you need to transform 2 points into local geometry
			-and that requires a matrix inverse- or you need to transform all the geometry points into
			a stable pickray coordinate system (like avatar collision) - requiring the transform of  lots of points
			IDEA: when compile_transform do the inverse of the transform too - 2 matrix stacks 
			- then for VF_Sensitive you'd matrix multiply down both stacks, use the inverse for transforming
				pick ray points into local, and use modelview (or model if you want global) to transform
				the near-side intersection back into stable pickray coordinates for eventual comparative 
				distance sorting
			- Q. would this be faster than inverting modelview (or model) at each shape (to transform 2 ray points to
				local) or transforming all shape to viewpoint space to intersect there (like collision)? I ask because
				I'm thinking of implementing web3d.org LOOKAT navigation type, which allows you to click on any shape
				(and then your viewpoint transitions to EXAMINE distance near that shape). Unlike Sensitive -which
				sensitize a few nodes- LOOKAT would allow any shape to be picked, so testing (transforming or inverting)
				needs to be done at more shapes. For LOOKAT I'll use the collision approach (transform all shape
				points to avatar/viewpoint space) and it already has a linesegment(A,B)_intersect_shape, 
				with distance sorting for wall penetration detection and correction.
	How:
	after setup_viewpoint and before render_hier(VF_Sensitive), transform the mouse bearing (A,B) 
	from viewpoint to world coordinates using the view matrix. 
	dug9 Aug31, 2014: keep in mind points along the pickray need to be sorted along the pickray 
			(all but the closest are occluded), and that might be simpler math in viewpoint coordinates (distance=z)
			versus world coordinate (A,B) which requires a dot product distance=(C-A)dot(B-A)
2. normal proj matrix (vs glu_pick modified proj matrix)
	Benefits of using normal proj matrix for PointingDeviceSensor:
	a) allows multiple bearings to be processed on the same pass for future multi-touch devices (no 
		competition for the bearing-specific pick-proj matrix)
	b) allows VF_Sensitive pass to be combined with other passes for optimization, if using
		the same transform matrices
	c) allows setup_viewpoint(), setup_pickray() and setup_projection() to be computed once for multiple passes
	d) allows using world coordinates for the bearing because then bearing doesn't depend 
		on a special proj matrix which only glu_project and glu_unproject can make use of,
		and those functions transform to/from viewport space
	How:
	in setup_projection(pick=FALSE,,), and glu_unproject with the mousexy to get B in 
	viewpoint-local, then use view to transform A,B from viewpoint-local to world. View is 
	computed with setup_viewpoint()
	
	dug9 Aug31 2014: extent/Minimum Bounding Boxes (MBB) - if you transform the object shape MBB
		directly into pickray-aligned coordinates in one step with pickray-aligned modelview, then exclusion can be done
		with simple x < max type culling tests. Then once that test gets a hit you can do more expensive
		operations. That's the benefit of pickray-aligned (modified) modelview: faster culling. Otherwise using
		ordinary modelview or model, you have to do another matrix multiply on 8 points to get that MBB into
		pickray-aligned space, or do a more general ray-intersect-unaligned-box which has more math.
	dug9 Sept 2, 2014: if GLU_PROJECT, GLU_UNPROJECT are being used now to transform, then there is already
		a matrix concatonation: modelview and projection. So substituting a pickray-aligned affine
		matrix for the proj matrix won't add matrix multiplies, and since the combined would now be 
		affine, further ops can also be affine, reducing FLOPs. 

3. explicit sensor stack:
	Benefits of an explicit sensor stack in render_node():
	a) code will be easier to read (vs. current 'shuffling' on recusion)
	b) minimizes C call stack size and minimizes memory fragmentation from malloc/free
		by using a pre-allocaed (or infrequently realloced) heap stack
	How: 
	when visiting a VF_sensitive parent (Group/Transform), push parent's modelview [model] transform
	onto a special sensor_viewmodel [sensor_model] stack that works like modelview matrix push and pop stack,
	and push the parents node* onto a sensor_parent stack (and pop them both going back)
3. minimize use of glu_unproject
	glu_unproject is a compound convenience function doing an expensive 4x4 matrix inversion
	Benefits of avoiding glu_unproject:
	a) allows speed optimization by computing inverse once, and using it many times for a given
		modelview [model] matrix
	b) avoids reliance on any projection matrix
	dug9 Sept 2, 2014: 
	  c) without projection matrix, affine matrix ops can be used which cut FLOPs in half
	How:
	a) implement: 
		FW_GL_GETDOUBLEV(GL_MODEL_MATRIX_INVERSE, modelMatrix);
		FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX_INVSERSE, modelviewMatrix);
		and during modelview_push and _pop clear a state flag. Then when we call the above
		we would test if !flag invert & set flag, else: re-use
	b) for bearing, use world coordinates, which elliminates the dependency on pick-bearing-specific projmatrix
		so we don't rely on glu_project/glu_unproject which are the only functions that can use the projMatrix.

	we need the following transform capabilities:
	a) bearing: transform A,B bearing from PointingDevice-local to world. If pointing device is:
		- 2D mouse use pick-proj [normal proj] and view matrix
		- 3D, leave in world coords, or do some uniform scale to world
	b) sensor hits: 
		- transform bearing from bearing-local (pick-viewport-local [world]) to geometry-local
		- transform geometry-local hit xyz and surface normal vector to sensor-local 
			for generating events, DragSensor: for intersecting with sensor geometry,
				using modelview+pick-proj [using model]
	c) DragSensor: snapshot/freeze the Sensor_model transform on mousedown 
		for use during a drag/mousemove
	d) Neither/Non-Sensitized geom nodes: transform geometry xyz hits to bearing-local
		using modelview+pick-proj [model]
	[viewport coordinates are only needed once per frame to project the mouse into bearing-world]

Update May 2015 - dug9
	- we've been using AffinePickMatrix method since fall 2014, 8 months, and with LOOKAT and EXPLORE navigation modes
		and its been working fine.
	- clarifications of some points above:
		- we do our ray-geometry intersections in geometry-local coordinates. That requires us to 
			transform (a copy of) the pick ray into geometry-local as we move down the transform stack in render_node()
			we do that in upd_ray(), and it requires a matrix inverse. Currently we call upd_ray() during descent, and
			also during ascent in render_node(). It could be a stack instead, pushed on the way down and popped on the way back 
			to save an inverse. 
		- We recompute upd_ray() on each vf_sensitive recursion into render_node. But in theory it should/could be just when 
			we've chnaged the modelview transform by passing through a transform (or geotransform or group) node.
			I'm not sure the best place/way to detect that. Perhaps just before ->children(node).
					bool pushed_ray = false
					if(node.type == transform type) pushed_ray = true
					if(pushed_ray) upd_ray_and_push()
					node->children(node)
					if(pushed_ray) pop_ray()
		- then if/when we have an intersection/hit point, we transform it back into bearing-local space for comparison
			with the best-hit so far on the frame (which is closest/not occluded). This transform uses no inverse.
		- One might argue whether it would be easier / somehow better to transform all geometry points 
			into ray/bearing local space instead, requiring no inverse, and simplifying some of the intersection math.
			For drawing, this transform is done on the gpu. I don't know which is better. 
			OpenCL might help, transforming in parallel, analogous to drawing.
		- Or for each transform when compile_transform the 4x4 from translation, rotation, scale, also compute its inverse and
			store with the transform. Then when updating modelview during scengraph traversing, also multiply the inverses 
			to get inverse(modelview). 49 FLOPS in an AFFINE inverse (ie inverting cumulative modelview at each level), 
			vs. 36 in AFFINE matrix multiply. You save 13 FLOPS per transform level during VFSensitve pass, but need to
			pre-compute the inverses, likely once for most, in compile_transform.
		- we have to intersect _all_ geometry, not just sensitive. That's because non-sensitive geometry can occlude.
			we do an extent/MBB (minimum bounding box) intersection test first, but we need the upd_ray() with inverse to do that.
		- LOOKAT and EXPLORE use this intersecting all geometry to find the closest point of all geometry along the ray, 
			even when no sensitive nodes in the scene. They only do it on passes where someone has indicated they 
			want to pick a new location (for viewpoint slerping to) (versus sensitive nodes in scene which cause 
			us to always be checking for hits)
		- for dragsensor nodes, on mouse-down we want to save/snapshot the modelview matrix from viewpoint to sensor-local
			- then use this sensor-local transform to place the sensor-geometry ie cylinder, sphere, plane [,line]
				for intersecting as we move/drag with mousedown
			- since we don't know if or which dragsensor hit will be the winner (on a frame) when visiting a sensor,
				we save it if its the best-so-far and over-write with any subsequent better hits
		- if there are multiple sensors in the same scengraph branch, the one closest to the hit emits the events
			- there could/should be a stack for 'current sensor' pushed when descending, and popping on the way back
	- An alternative to a cluttered render_node() function would be to change virt->children(node) functions
		to accept a function pointer ie children(node,func). Same with normal_children(node,func).
		Then use separate render_node(node), sensor_node(node), and collision_node(node) functions. 
		They are done on separate passes now anyway.
		On the other hand, someone may find a way to combine passes for better efficiency/reduced transform FLOPs per frame.
	- Multitouch - there is currently nothing in the specs. But if there was, it might apply to (modfied / special) touch 
		and drag sensors. And for us that might mean simulataneously or iterating over a list of touches.

Update Dec 17, 2015 dug9:
	I vectorized setup_picking() to allow for multi-touch, but not yet getRayHit() or
		sendSensorEvents > get_hyperhit() > Renderfuncs.hp, .hpp, .modelmatrix etc (or SetCursor(style,touch.ID))
	And added a contenttype_e3dmouse for emulating 3D mouse / HMD that moves 
		the viewpoint (vs the mouse xy), on a drag motion.
	Drag sensor doesn't work with e3dmouse.
	Drag Sensor review:
	
	[WORLD]		< View matrix < .pos/.ori < stereo < pickmatrix < xy=0,0      (remember the pickmatrix aligns Z axis to pickray)
	[COORDS]	> Model matrix > DragSensor

	It's the Model matrix that should be frozen on mouse-down for dragsensors
	- the pickmatrix is updated on each frame or each mouse event
	x currently we are freezing the [View + pos,ori + stereo] with the Model as one modelview matrix
	+ we should be re-concatonating the ViewMatrix to the frozen ModelMatrix on each frame/mouse event
		- that would allow updates to the ViewMatrix on each frame, for 3D mice and HMD IMUs (Head Mounted Display Inertial Measuring Units)

*/
