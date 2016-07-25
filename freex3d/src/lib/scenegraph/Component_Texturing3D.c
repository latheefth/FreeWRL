/*


X3D Texturing3D Component

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
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "../opengl/OpenGL_Utils.h"
#include "../opengl/Textures.h"
#include "../scenegraph/Component_Shape.h"
#include "../scenegraph/RenderFuncs.h"

/*
http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/texture3D.html

Texturing3D > Volumetric Image Formats
http://www.volumesoffun.com/voldat-format/
https://graphics.stanford.edu/data/voldata/
http://paulbourke.net/dataformats/pvl/
http://www.ncbi.nlm.nih.gov/pmc/articles/PMC2954506/#!po=59.5238
https://www.blender.org/manual/render/blender_render/textures/types/volume/voxel_data.html

http://paulbourke.net/dataformats/volumetric/
-Simplest 3d texture file if you write your own images
http://www.web3d.org/x3d/content/examples/Basic/VolumeRendering/
- Volumetric Rendering Samples use nrrd format
- http://teem.sourceforge.net/index.html
-- Nrrd lib LGPL

What is Texturing3D used for? a few links:
https://msdn.microsoft.com/en-us/library/windows/desktop/ff476906(v=vs.85).aspx
- difference between composed slices and volume image
http://docs.nvidia.com/gameworks/content/gameworkslibrary/graphicssamples/opengl_samples/texturearrayterrainsample.htm
- an application for rendering terrain



*/