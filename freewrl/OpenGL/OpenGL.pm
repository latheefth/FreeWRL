# Originally generated by generate.p, but now handled by hand.
#
# John Stewart
#
	package VRML::OpenGL;
	$Version = 0.5;

	require Exporter;
	require DynaLoader;
	@ISA = qw(Exporter DynaLoader);
	@EXPORT = qw(
		Button1
		Button1Mask
		Button2
		Button2Mask
		Button3
		Button3Mask
		ButtonMotionMask
		ButtonPress
		ButtonPressMask 
		ButtonRelease
		ButtonReleaseMask
		ConfigureNotify
		ControlMask
		ExposureMask
		GLX_BLUE_SIZE
		GLX_DEPTH_SIZE
		GLX_DOUBLEBUFFER
		GLX_GREEN_SIZE
		GLX_RED_SIZE
		GLX_RGBA
		GL_AMBIENT
		GL_BACK
		GL_BACK_LEFT
		GL_BACK_RIGHT
		GL_BLEND
		GL_COLOR_BUFFER_BIT
		GL_CULL_FACE
		GL_DEPTH_BUFFER_BIT
		GL_DEPTH_TEST
		GL_DIFFUSE
		GL_FALSE
		GL_FLAT
		GL_FRONT_AND_BACK
		GL_LEQUAL
                GL_LEFT
		GL_LIGHT0
		GL_LIGHT1
		GL_LIGHT2
		GL_LIGHT3
		GL_LIGHT4
		GL_LIGHT5
		GL_LIGHT6
		GL_LIGHT7
		GL_LIGHT8
		GL_LIGHT9
		GL_LIGHTING
		GL_LIGHT_MODEL_LOCAL_VIEWER
		GL_LIGHT_MODEL_TWO_SIDE
		GL_LIGHT_MODEL_TWO_SIDE
		GL_MODELVIEW
		GL_MODELVIEW_MATRIX
		GL_MODELVIEW_STACK_DEPTH
		GL_NICEST
		GL_NORMALIZE
		GL_ONE_MINUS_SRC_ALPHA
		GL_PACK_ALIGNMENT
		GL_PERSPECTIVE_CORRECTION_HINT
		GL_POLYGON_OFFSET_EXT
		GL_POSITION
		GL_PROJECTION
		GL_PROJECTION_STACK_DEPTH
		GL_RENDER
		GL_RGB
		GL_RGBA
		GL_SELECT
		GL_SHININESS
		GL_SMOOTH
		GL_SPECULAR
		GL_SRC_ALPHA
		GL_TRUE
		GL_UNPACK_ALIGNMENT
		GL_UNSIGNED_BYTE
		GL_VIEWPORT
		KeyPress
		KeyPressMask
		KeyRelease
		KeyReleaseMask
		MotionNotify
		PointerMotionMask
		ShiftMask
		StructureNotifyMask
		XPending
		glBlendFunc
		glClear
		glClearColor
		glDepthFunc
		glDisable
		glDrawBuffer
		glEnable
		glGetDoublev
		glGetIntegerv
		glHint
		glLightModeli
		glLightfv
		glLoadIdentity
		glMaterialf
		glMatrixMode
		glMultMatrixd
		glPixelStorei
		glPolygonMode
		glPopMatrix
		glPushMatrix
		glReadPixels
		glRenderMode
		glSelectBuffer
		glShadeModel
		glViewport
		glXDestroyContext
		glXSwapBuffers
		glpOpenGLInitialize
		glpOpenWindow
		glpXNextEvent
		gluPerspective
		glupPickMatrix
		get_triangulator
		glGenTexture
		glPrintError
	);
	bootstrap VRML::OpenGL;


sub Button1 () {1}
sub Button1Mask () {(1<<8)}
sub Button1MotionMask () {(1<<8)}
sub Button2 () {2}
sub Button2Mask () {(1<<9)}
sub Button2MotionMask () {(1<<9)}
sub Button3 () {3}
sub Button3Mask () {(1<<10)}
sub Button3MotionMask () {(1<<10)}
sub ButtonMotionMask () {(1<<13)}
sub ButtonPress () {4}
sub ButtonPressMask () {(1<<2)}
sub ButtonRelease () {5}
sub ButtonReleaseMask () {(1<<3)}
sub ConfigureNotify () {22}
sub ControlMask () {(1<<2)}
sub ExposureMask () {(1<<15)}
sub GLX_BLUE_SIZE () {10}
sub GLX_DEPTH_SIZE () {12}
sub GLX_DOUBLEBUFFER () {5}
sub GLX_GREEN_SIZE () {9}
sub GLX_RED_SIZE () {8}
sub GLX_RGBA () {4}
sub GL_AMBIENT () {0x1200}
sub GL_AMBIENT_AND_DIFFUSE () {0x1602}
sub GL_BACK () {0x0405}
sub GL_BACK_LEFT () {0x0402}
sub GL_BACK_RIGHT () {0x0403}
sub GL_BLEND () {0x0BE2}
sub GL_BLEND_COLOR_EXT () {0x8005}
sub GL_BLEND_DST () {0x0BE0}
sub GL_BLEND_EQUATION_EXT () {0x8009}
sub GL_BLEND_SRC () {0x0BE1}
sub GL_COLOR_BUFFER_BIT () {0x00004000}
sub GL_CULL_FACE () {0x0B44}
sub GL_CULL_FACE_MODE () {0x0B45}
sub GL_DEPTH_BUFFER_BIT () {0x00000100}
sub GL_DEPTH_TEST () {0x0B71}
sub GL_DIFFUSE () {0x1201}
sub GL_FALSE () {0}
sub GL_FLAT () {0x1D00}
sub GL_FRONT_AND_BACK () {0x0408}
sub GL_LEFT () {0x0406}
sub GL_LEQUAL () {0x0203}
sub GL_LIGHT0 () {0x4000}
sub GL_LIGHT1 () {0x4001}
sub GL_LIGHT2 () {0x4002}
sub GL_LIGHT3 () {0x4003}
sub GL_LIGHT4 () {0x4004}
sub GL_LIGHT5 () {0x4005}
sub GL_LIGHT6 () {0x4006}
sub GL_LIGHT7 () {0x4007}
sub GL_LIGHTING () {0x0B50}
sub GL_LIGHTING_BIT () {0x00000040}
sub GL_LIGHT_MODEL_LOCAL_VIEWER () {0x0B51}
sub GL_LIGHT_MODEL_TWO_SIDE () {0x0B52}
sub GL_MODELVIEW () {0x1700}
sub GL_MODELVIEW_MATRIX () {0x0BA6}
sub GL_MODELVIEW_STACK_DEPTH () {0x0BA3}
sub GL_NICEST () {0x1102}
sub GL_NORMALIZE () {0x0BA1}
sub GL_ONE_MINUS_SRC_ALPHA () {0x0303}
sub GL_PACK_ALIGNMENT () {0x0D05}
sub GL_PERSPECTIVE_CORRECTION_HINT () {0x0C50}
sub GL_POLYGON_OFFSET_EXT () {0x8037}
sub GL_POSITION () {0x1203}
sub GL_PROJECTION () {0x1701}
sub GL_PROJECTION_MATRIX () {0x0BA7}
sub GL_PROJECTION_STACK_DEPTH () {0x0BA4}
sub GL_RENDER () {0x1C00}
sub GL_RENDERER () {0x1F01}
sub GL_RENDER_MODE () {0x0C40}
sub GL_RGB () {0x1907}
sub GL_RGBA () {4}
sub GL_SELECT () {0x1C02}
sub GL_SHININESS () {0x1601}
sub GL_SMOOTH () {0x1D01}
sub GL_SPECULAR () {0x1202}
sub GL_SRC_ALPHA () {0x0302}
sub GL_SRC_ALPHA_SATURATE () {0x0308}
sub GL_TRUE () {1}
sub GL_UNPACK_ALIGNMENT () {0x0CF5}
sub GL_UNSIGNED_BYTE () {0x1401}
sub GL_VIEWPORT () {0x0BA2}
sub GL_VIEWPORT_BIT () {0x00000800}
sub KeyPress () {2}
sub KeyPressMask () {(1<<0)}
sub KeyRelease () {3}
sub KeyReleaseMask () {(1<<1)}
sub MotionNotify () {6}
sub PointerMotionMask () {(1<<6)}
sub ShiftMask () {(1<<0)}
sub StructureNotifyMask () {(1<<17)}

%window_defaults=(
                'x'     => 0,
                'y'     => 0,
                'width' => 500,
                'height'=> 500,
                'parent'=> 0,
                'mask'  => StructureNotifyMask,
		'fs'	=> 0,
		'shutter' => 0,
		'wintitle' => "",
		'cmap' => 1,
                'attributes'=> [ GLX_RGBA, GL_TRUE],
        );
sub glpOpenWindow {
        # default values
        my(%a) = @_;
        my(%p) = %window_defaults;
        foreach $k (keys(%a)){
                defined($p{$k}) || warn "Not a valid parameter to glpOpenWindow: `$k'
\n";
                # print "parameter $k now ",$a{$k}," was ",$p{$k},"\n";
                $p{$k} = $a{$k};
        }
        glpcOpenWindow($p{x},$p{y},$p{width},$p{height},
                       $p{parent},$p{fs},$p{shutter},$p{'mask'},$p{wintitle},
			$p{'cmap'}, @{$p{attributes}});
}
1;

