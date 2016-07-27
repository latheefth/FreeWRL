
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


/*******************************************************************

	X3D Environmental Effects Component

*********************************************************************/

/*
FOG - state as of July 2016: 
	- our conformance page says local_fog and fog_coordinates are unimplemented
	- and render_Fog (for global fog) in Bindable.c is in ifndef:
		#ifndef GL_ES_VERSION_2_0 // this should be handled in material shader
	Unclear if global Fog is working 

	http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/lighting.html#LightingModel
	- Use of fog and fog coords in lighting eqn.
	http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/enveffects.html#FogSemantics
	- meaning of fog nodes

	VRML2 fog examples:
		http://www.web3d.org/x3d-resources/content/examples/Vrml2.0Sourcebook/Chapter23-Fog/
		http://www.web3d.org/x3d/content/examples/Vrml2.0Sourcebook/Chapter23-Fog/Figure23.3cExponentialFogVisibility20.x3d
		- Octaga shows light fog increasing exponentially with distance (but shows ground white)
		- InstantReality shows heavy fog increasing exponentially with distance (then bombs on exit)
		- Vivaty - medium " works well
		- freewrl desktop win32 - no fog effect
	X3D fog in kelp forest example:
		http://x3dgraphics.com/examples/X3dForWebAuthors/Chapter11-LightingEnvironmentalEffects/
		http://x3dgraphics.com/examples/X3dForWebAuthors/Chapter11-LightingEnvironmentalEffects/Fog-KelpForestMain.x3d
		- freewrl crashes loading scene - win32 desktop develop branch July 27, 2016
		- fog (and scene) looks great in Octaga, InstantReality, Vivaty (although Octaga crashes if left running 8min)

	Non-X3D links:
		http://www.mbsoftworks.sk/index.php?page=tutorials&series=1&tutorial=15
		- fog shader with linear and exponential
*/
