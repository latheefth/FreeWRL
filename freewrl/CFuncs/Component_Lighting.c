/*******************************************************************
 Copyright (C) 2005, 2006 John Stewart, Ayla Khan, Sarah Dumoulin, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*******************************************************************

	X3D Lighting Component

*********************************************************************/

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <math.h>
#ifdef AQUA
#include <gl.h>
#include <glu.h>
#include <glext.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#endif

#include "Structs.h"
#include "headers.h"
#include "installdir.h"


void render_DirectionalLight (struct X3D_DirectionalLight *node) {
	/* NOTE: This is called by the Group Children code
	 * at the correct point (in the beginning of the rendering
	 * of the children. We just turn the light on right now.
	 */

	if(node->on) {
		int light = nextlight();
		if(light >= 0) {
			float vec[4];
			/* glEnable(light); */
			lightState(light-GL_LIGHT0,TRUE);
			vec[0] = -((node->direction).c[0]);
			vec[1] = -((node->direction).c[1]);
			vec[2] = -((node->direction).c[2]);
			vec[3] = 0;
			glLightfv(light, GL_POSITION, vec);
			vec[0] = ((node->color).c[0]) * (node->intensity) /*cget*/;
			vec[1] = ((node->color).c[1]) * (node->intensity) /*cget*/;
			vec[2] = ((node->color).c[2]) * (node->intensity) /*cget*/;
			vec[3] = 1;
			glLightfv(light, GL_DIFFUSE, vec);
			glLightfv(light, GL_SPECULAR, vec);

			/* Aubrey Jaffer */
			vec[0] = ((node->color).c[0]) * (node->ambientIntensity) /*cget*/;
			vec[1] = ((node->color).c[1]) * (node->ambientIntensity) /*cget*/;
			vec[2] = ((node->color).c[2]) * (node->ambientIntensity) /*cget*/;

			glLightfv(light, GL_AMBIENT, vec);
		}
	}
}



void light_PointLight (struct X3D_PointLight *node) {

                if(((node->on))) {
                        int light = nextlight();
                        if(light >= 0) {
                                float vec[4];
                                glEnable(light);
                                vec[0] = ((node->direction).c[0]);
                                vec[1] = ((node->direction).c[1]);
                                vec[2] = ((node->direction).c[2]);
                                vec[3] = 1;
                                glLightfv(light, GL_SPOT_DIRECTION, vec);
                                vec[0] = ((node->location).c[0]);
                                vec[1] = ((node->location).c[1]);
                                vec[2] = ((node->location).c[2]);
                                vec[3] = 1;
                                glLightfv(light, GL_POSITION, vec);

                                glLightf(light, GL_CONSTANT_ATTENUATION,
                                        ((node->attenuation).c[0]));
                                glLightf(light, GL_LINEAR_ATTENUATION,
                                        ((node->attenuation).c[1]));
                                glLightf(light, GL_QUADRATIC_ATTENUATION,
                                        ((node->attenuation).c[2]));


                                vec[0] = ((node->color).c[0]) * (node->intensity) /*cget*/;
                                vec[1] = ((node->color).c[1]) * (node->intensity) /*cget*/;
                                vec[2] = ((node->color).c[2]) * (node->intensity) /*cget*/;
                                vec[3] = 1;
                                glLightfv(light, GL_DIFFUSE, vec);
                                glLightfv(light, GL_SPECULAR, vec);

                                /* Aubrey Jaffer */
                                vec[0] = ((node->color).c[0]) * (node->ambientIntensity) /*cget*/;
                                vec[1] = ((node->color).c[1]) * (node->ambientIntensity) /*cget*/;
                                vec[2] = ((node->color).c[2]) * (node->ambientIntensity) /*cget*/;
                                glLightfv(light, GL_AMBIENT, vec);

                                /* XXX */
                                glLightf(light, GL_SPOT_CUTOFF, 180);
                        }
                }
}


void light_SpotLight (struct X3D_SpotLight *node) {
                if(((node->on))) {
                        int light = nextlight();
                        if(light >= 0) {
                                float vec[4];
                                /* glEnable(light); */                                lightState(light-GL_LIGHT0,TRUE);
               
                                vec[0] = ((node->direction).c[0]);
                                vec[1] = ((node->direction).c[1]);
                                vec[2] = ((node->direction).c[2]);
                                vec[3] = 1;
                                glLightfv(light, GL_SPOT_DIRECTION, vec);
                                vec[0] = ((node->location).c[0]);
                                vec[1] = ((node->location).c[1]);
                                vec[2] = ((node->location).c[2]);
                                vec[3] = 1;
                                glLightfv(light, GL_POSITION, vec);
        
                                glLightf(light, GL_CONSTANT_ATTENUATION,
                                        ((node->attenuation).c[0]));
                                glLightf(light, GL_LINEAR_ATTENUATION,
                                        ((node->attenuation).c[1]));
                                glLightf(light, GL_QUADRATIC_ATTENUATION,
                                        ((node->attenuation).c[2]));
        
        
                                vec[0] = ((node->color).c[0]) * (node->intensity) /*cget*/;
                                vec[1] = ((node->color).c[1]) * (node->intensity) /*cget*/;
                                vec[2] = ((node->color).c[2]) * (node->intensity) /*cget*/;
                                vec[3] = 1; 
                                glLightfv(light, GL_DIFFUSE, vec);
                                glLightfv(light, GL_SPECULAR, vec);

                                /* Aubrey Jaffer */
                                vec[0] = ((node->color).c[0]) * (node->ambientIntensity) /*cget*/;
                                vec[1] = ((node->color).c[1]) * (node->ambientIntensity) /*cget*/;
                                vec[2] = ((node->color).c[2]) * (node->ambientIntensity) /*cget*/;

                                glLightfv(light, GL_AMBIENT, vec);

                                /* XXX */
                                glLightf(light, GL_SPOT_EXPONENT,
                                        0.5/((node->beamWidth) /*cget*/+0.1));
                                glLightf(light, GL_SPOT_CUTOFF,
                                        (node->cutOffAngle) /*cget*//3.1415926536*180);
                        }
                }
        }
