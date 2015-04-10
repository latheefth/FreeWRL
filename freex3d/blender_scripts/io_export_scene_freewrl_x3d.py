#!BPY
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# ##### END GPL LICENSE BLOCK #####

# <pep8 compliant>

bl_info = {
    "name": "X3D freewrl",
    "author": "Campbell Barton; freewrl",
    "blender": (2, 57, 0),
    "location": "File > Import-Export",
    "description": "Export X3D for freewrl",
    "warning": "",
    "wiki_url": "http://wiki.blender.org/index.php/Extensions:2.6/Py/"
                "Scripts/Import-Export/Web3D",
    "tracker_url": "",
    "support": 'freewrl',
    "category": "Import-Export"}

# Contributors: "Bart:Campbell Barton (orig); freewrl(mindom,nested)
"""
This script exports to X3D format.

Usage:

Run this script from "File->Export" menu.  A pop-up will ask whether you
want to export only selected or all relevant objects.
Nested: will maintain outliner transform heirarchy in x3d file

Known issues:
	Doesn't handle multiple materials (don't use material indices);<br>
	Doesn't handle multiple UV textures on a single mesh (create a mesh for each texture);<br>
	Can't get the texture array associated with material * not the UV ones;
"""


import math
import os

import bpy
from bpy.props import StringProperty, BoolProperty, EnumProperty, FloatProperty
from bpy_extras.io_utils import (ExportHelper,
                                 path_reference_mode,
                                 )

import mathutils
from mathutils import Matrix
import xml
from xml.dom.minidom import Document, DocumentType, Element, Comment
from xml.sax.saxutils import quoteattr, escape

from bpy_extras.io_utils import create_derived_objects, free_derived_objects

####################################
# Global Variables
####################################

_safeOverwrite = True
extension = ''

##########################################################
# Functions for writing output file
##########################################################
class x3d_class():

    def __init__(self):
        #--- public you can change these ---
        self.writingcolor = 0
        self.writingtexture = 0
        self.writingcoords = 0
        self.proto = 1
        self.matonly = 0
        self.share = 0
        self.billnode = 0
        self.halonode = 0
        self.collnode = 0
        self.tilenode = 0
        self.verbose=2	 # level of verbosity in console 0-none, 1-some, 2-most
        self.cp=3		  # decimals for material color values	 0.000 - 1.000
        self.vp=3		  # decimals for vertex coordinate values  0.000 - n.000
        self.tp=3		  # decimals for texture coordinate values 0.000 - 1.000
        self.it=3
        self.groupwrap = 1 #puts an extra Transform around mesh objects for easy reparenting in fluxstudio
        self.nested = 1    #writes nodes like blender Outliner view - good for animating
        self.polyline2d = 1 #writes circles -which have no face- exported as Polyline2D rather than nothing
        self.scene = None  #for writeData
        self.world = None  # "
        self.constants = {} # - for export flags etc used in writeData
        self.doc = Document() #tired of malformed xml - switching to xml minidom
        self.currentGroupObjects = None #see getChildren and writeGroup - a way to switch from scene.objects to grp.objects

        #--- class private don't touch ---
        self.space='worldspace' #for flat file, else localspace for nested - set in export()
        self.texNames={}   # dictionary of textureNames
        self.matNames={}   # dictionary of materiaNames
        self.meshNames={}   # dictionary of meshNames
        self.grpNames={}    # dictionary of groupNames
        self.lobNames={}    # dictionary of library Object (OB) types (not mat, tex, mesh or grp which go in above with scene items - just Objects from Libraries
        self.indentLevel=0 # keeps track of current indenting
        self.file = None
        self.libraryNode = None  #the Switch node where we'll hide library geometry from the scene graph
        self.libraryIndex = {}   # object.lib to index { <libraryPathName0>:1, <libraryPathName1>:2 ... ie '/mainstreet/Doors.blend':1
        self.filename=None

        self.bNav=0
        self.nodeID=0
        self.namesReserved=[ "Anchor","Appearance","Arc2D","ArcClose2D","AudioClip","Background","Billboard",
            "BooleanFilter","BooleanSequencer","BooleanToggle","BooleanTrigger","Box","Circle2D",
            "Collision","Color","ColorInterpolator","ColorRGBA","component","Cone","connect",
            "Contour2D","ContourPolyline2D","Coordinate","CoordinateDouble","CoordinateInterpolator",
            "CoordinateInterpolator2D","Cylinder","CylinderSensor","DirectionalLight","Disk2D",
            "ElevationGrid","EspduTransform","EXPORT","ExternProtoDeclare","Extrusion","field",
            "fieldValue","FillProperties","Fog","FontStyle","GeoCoordinate","GeoElevationGrid",
            "GeoLocationLocation","GeoLOD","GeoMetadata","GeoOrigin","GeoPositionInterpolator",
            "GeoTouchSensor","GeoViewpoint","Group","HAnimDisplacer","HAnimHumanoid","HAnimJoint",
            "HAnimSegment","HAnimSite","head","ImageTexture","IMPORT","IndexedFaceSet",
            "IndexedLineSet","IndexedTriangleFanSet","IndexedTriangleSet","IndexedTriangleStripSet",
            "Inline","IntegerSequencer","IntegerTrigger","IS","KeySensor","LineProperties","LineSet",
            "LoadSensor","LOD","Material","meta","MetadataDouble","MetadataFloat","MetadataInteger",
            "MetadataSet","MetadataString","MovieTexture","MultiTexture","MultiTextureCoordinate",
            "MultiTextureTransform","NavigationInfo","Normal","NormalInterpolator","NurbsCurve",
            "NurbsCurve2D","NurbsOrientationInterpolator","NurbsPatchSurface",
            "NurbsPositionInterpolator","NurbsSet","NurbsSurfaceInterpolator","NurbsSweptSurface",
            "NurbsSwungSurface","NurbsTextureCoordinate","NurbsTrimmedSurface","OrientationInterpolator",
            "PixelTexture","PlaneSensor","PointLight","PointSet","Polyline2D","Polypoint2D",
            "PositionInterpolator","PositionInterpolator2D","ProtoBody","ProtoDeclare","ProtoInstance",
            "ProtoInterface","ProximitySensor","ReceiverPdu","Rectangle2D","ROUTE","ScalarInterpolator",
            "Scene","Script","Shape","SignalPdu","Sound","Sphere","SphereSensor","SpotLight","StaticGroup",
            "StringSensor","Switch","Text","TextureBackground","TextureCoordinate","TextureCoordinateGenerator",
            "TextureTransform","TimeSensor","TimeTrigger","TouchSensor","Transform","TransmitterPdu",
            "TriangleFanSet","TriangleSet","TriangleSet2D","TriangleStripSet","Viewpoint","VisibilitySensor",
            "WorldInfo","X3D","XvlShell","VertexShader","FragmentShader","MultiShaderAppearance","ShaderAppearance" ]
        self.namesStandard=[ "Empty","Empty.000","Empty.001","Empty.002","Empty.003","Empty.004","Empty.005",
            "Empty.006","Empty.007","Empty.008","Empty.009","Empty.010","Empty.011","Empty.012",
            "Scene.001","Scene.002","Scene.003","Scene.004","Scene.005","Scene.06","Scene.013",
            "Scene.006","Scene.007","Scene.008","Scene.009","Scene.010","Scene.011","Scene.012",
            "World","World.000","World.001","World.002","World.003","World.004","World.005" ]
        self.namesFog=[ "","LINEAR","EXPONENTIAL","" ]
        #not using those commented out, but would be nice if PyAPI had standard list of prefixes for exporters
        self.typePrefix = {
            #Types.ArmatureType:'',
            #Types.BoneType:'',
            #Types.bufferType:'',
            #Types.ButtonType:'',
            bpy.types.Camera:'CA', #Types.CameraType:'CA',
            #Types.constantType:'',
            #Types.CurveType:'',
            #Types.eulerType:'',
            bpy.types.Group:'GR', #Types.GroupType:'GR',
            #Types.IDArrayType:'',
            #Types.IDGroupType:'',
            bpy.types.Image:'IM', #Types.ImageType:'IM',
            #Types.IpoType:'',
            bpy.types.Lamp:'LA', #Types.LampType:'LA',
            bpy.types.Material:'MA', #Types.MaterialType:'MA',
            #Types.matrix_Type:'',
            #Types.MColType:'',
            #Types.MEdgeType:'',
            bpy.types.Mesh:'ME', #Types.MeshType:'ME',
            #Types.MetaballType:'',
            #Types.MFaceType:'',
            #Types.MTexType:'',
            #Types.MVertType:'',
            #Types.NMColType:'',
            #Types.NMeshType:'',
            #Types.NMFaceType:'',
            #Types.NMVertType:'',
            bpy.types.Object:'OB', #Types.ObjectType:'OB',
            #Types.PVertType:'',
            #Types.quaternionType:'',
            #Types.rgbTupleType:'',
            bpy.types.Scene:'SCE', #Types.SceneType:'SCE',
            #Types.Text3dType:'',
            #Types.TextType:'',
            bpy.types.TextCurve:'TX', 
            bpy.types.Texture:'TE', #Types.TextureType:'TE',
            #Types.vectorType:'',
            }

    def setFilename(self,filename):
        self.filename=filename

        if filename.lower().endswith('.x3dz'):
            try:
                import gzip
                self.file = gzip.open(filename, "w")
            except:
                print("failed to import compression modules, exporting uncompressed")
                self.filename = filename[:-1] # remove trailing z

        if self.file == None:
                self.file = open(self.filename, "w")
        
##########################################################
# Writing nodes routines
##########################################################
    def createDocType(self):
        """
        <!DOCTYPE X3D PUBLIC "ISO//Web3D//DTD X3D 3.0//EN" "http://www.web3d.org/specifications/x3d-3.0.dtd">
        """
        docType = DocumentType("X3D")
        docType.publicId = "ISO//Web3D//DTD X3D 3.0//EN"
        docType.systemId = "http://www.web3d.org/specifications/x3d-3.0.dtd"
        return docType

    def writeHeader(self):
        """
        <X3D version="3.0" profile="Immersive" xmlns:xsd="http://www.w3.org/2001/XMLSchema-instance" xsd:noNamespaceSchemaLocation="http://www.web3d.org/specifications/x3d-3.0.xsd">
        """
        bfile = self.filename.replace('<', '&lt').replace('>', '&gt') # use outfile name
        filepath_quoted = quoteattr(os.path.basename(self.filename)) #file.name))
        blender_ver_quoted = quoteattr('Blender %s' % bpy.app.version_string)

        dt = self.createDocType()
        dt.parent = self.doc
        self.doc.appendChild(dt)
        self.doc.doctype = dt

        root = self.doc.createElement("X3D")
        root.setAttribute("xmlns:xsd","http://www.w3.org/2001/XMLSchema-instance")
        root.setAttribute("xsd:noNamespaceSchemaLocation","http://www.web3d.org/specifications/x3d-3.0.xsd")
        root.setAttribute("version","3.0")
        root.setAttribute("profile","Immersive")
        self.doc.appendChild(root)
        """
        <head>
        <meta name="filename" content="testLibraryLinkToWindow.x3d" />
        <meta name="generator" content="Blender 245" />
        <meta name="translator" content="X3D exporter v1.55 (2006/01/17)" />
        </head>
        """
        head = self.doc.createElement("head")
        meta = self.doc.createElement("meta")
        meta.setAttribute("name","filename")
        meta.setAttribute("content",filepath_quoted) #bfile)
        head.appendChild(meta)
        meta = self.doc.createElement("meta")
        meta.setAttribute("name","generator")
        meta.setAttribute("content",blender_ver_quoted) #"Blender " + bpy.app.version_string)
        head.appendChild(meta)
        meta = self.doc.createElement("meta")
        meta.setAttribute("name","translator")
        meta.setAttribute("content","X3D exporter v1.55 (2006/01/17)")
        head.appendChild(meta)
        root.appendChild(head)
        return root


    # This functionality is poorly defined, disabling for now - campbell
    '''
    def writeInline(self):
            inlines = Blender.Scene.Get()
            allinlines = len(inlines)
            if scene != inlines[0]:
                    return
            else:
                    for i in xrange(allinlines):
                            nameinline=inlines[i].name
                            if (nameinline not in self.namesStandard) and (i > 0):
                                    self.file.write("<Inline DEF=\"%s\" " % (self.cleanStr(nameinline)))
                                    nameinline = nameinline+".x3d"
                                    self.file.write("url=\"%s\" />" % nameinline)
                                    self.file.write("\n\n")


    def writeScript(self):
            textEditor = Blender.Text.Get()
            alltext = len(textEditor)
            for i in xrange(alltext):
                    nametext = textEditor[i].name
                    nlines = textEditor[i].getNLines()
                    if (self.proto == 1):
                            if (nametext == "proto" or nametext == "proto.js" or nametext == "proto.txt") and (nlines != None):
                                    nalllines = len(textEditor[i].asLines())
                                    alllines = textEditor[i].asLines()
                                    for j in xrange(nalllines):
                                            self.writeIndented(alllines[j] + "\n")
                    elif (self.proto == 0):
                            if (nametext == "route" or nametext == "route.js" or nametext == "route.txt") and (nlines != None):
                                    nalllines = len(textEditor[i].asLines())
                                    alllines = textEditor[i].asLines()
                                    for j in xrange(nalllines):
                                            self.writeIndented(alllines[j] + "\n")
            self.writeIndented("\n")
    '''

    def writeViewpoint(self, ob, mat, scene):
        """
        <Viewpoint DEF="Camera" description="Camera" centerOfRotation="0 0 0"
          position="0.00 0.00 0.00" orientation="1.00 0.00 0.00 -1.57" fieldOfView="0.858" />
        """

        lens = ob.data.angle
        vp = self.doc.createElement('Viewpoint')
        vp.setAttribute('description',"%s" % (self.cleanStr(ob.name)))
        vp.setAttribute('centerOfRotation',"0 0 0")
        vp.setAttribute('position',"%3.2f %3.2f %3.2f" % (0.0,0.0,0.0))
        vp.setAttribute('orientation',"%3.2f %3.2f %3.2f %3.2f" % (1.0,0.0,0.0,0.0)) #-1.5708))
        vp.setAttribute('fieldOfView',"%.3f" % (lens))
        return vp

    def writeFog(self, world):
        """
        <Fog DEF="testFog" fogType='LINEAR' color='1 1 1' visibilityRange='50.0'/>
        """
        fog = None
        if world:
            mtype = world.mist_settings.falloff
            mparam = world.mist_settings
            grd = world.horizon_color
            grd0, grd1, grd2 = grd[0], grd[1], grd[2]
        else:
            return fog
        if mparam.use_mist:
            if (mtype == 'LINEAR' or mtype == 'INVERSE_QUADRATIC'):
                mtype = 1 if mtype == 'LINEAR' else 2
                fog = self.doc.createElement('Fog')
                fog.setAttribute('fogType',self.namesFog[mtype])
                fog.setAttribute('color',"%s %s %s" % (round(grd0,self.cp), round(grd1,self.cp), round(grd2,self.cp)))
                fog.setAttribute('visibilityRange',"%s" % round(mparam.depth,self.cp))

        return fog

    def writeNavigationInfo(self, scene):
        """
        <NavigationInfo headlight="true" visibilityLimit="5000.0" type='"EXAMINE" "ANY"' avatarSize="0.25, 1.75, 0.75" />
        """
        nav = self.doc.createElement("NavigationInfo")
        nav.setAttribute('headlight','true')
        nav.setAttribute('visibilityLimit','0.0')
        navtype = ["EXAMINE", "ANY"]

        nav.setAttribute('type','"EXAMINE" "ANY"') #navtype)
        nav.setAttribute('avatarSize','0.15, 1.75, 0.75') #'0.25, 1.75, 0.75') collision dist, eye height, biggest step height
        return nav

    def writeSpotLight(self, ob, mtx, lamp, world):
        """
        <SpotLight DEF="testSpotLight" containerField="children" ambientIntensity="0.7" attenuation="0.8 0.1 0.01" beamWidth="0.25" color="0.8 0.8 0.2" cutOffAngle="0.5" direction="-0.1 0.5 -0.5" global="true" intensity="0.8" location="0.1 0.1 -0.8" on="false" radius="80"/>
        """
        safeName = self.xName(lamp) #self.cleanStr(ob.name)
        if world:
            ambi = world.ambient_color
            #ambi = world.amb
            ambientIntensity = ((float(ambi[0] + ambi[1] + ambi[2]))/3) #/2.5
        else:
            ambi = 0
            ambientIntensity = 0

        # compute cutoff and beamwidth
        intensity=min(lamp.energy/1.75,1.0)
        beamWidth=lamp.spot_size * 0.37;
        cutOffAngle=beamWidth*1.3
        radius = lamp.distance*math.cos(beamWidth)
        lit = self.doc.createElement('SpotLight')
        lit.setAttribute('DEF',"%s" % safeName)
        lit.setAttribute('radius',"%s" % (round(radius,self.cp)))
        lit.setAttribute('ambientIntensity',"%s" % (round(ambientIntensity,self.cp)))
        lit.setAttribute('intensity',"%s" % (round(intensity,self.cp)))
        lit.setAttribute('color',"%s %s %s" % (round(lamp.color[0],self.cp), round(lamp.color[1],self.cp), round(lamp.color[2],self.cp)))
        lit.setAttribute('beamWidth',"%s" % (round(beamWidth,self.cp)))
        lit.setAttribute('cutOffAngle',"%s" % (round(cutOffAngle,self.cp)))
        lit.setAttribute('direction',"%s %s %s" % (0,0,0))
        lit.setAttribute('location',"%s %s %s" % (0,0,0))
        return lit

    def writeDirectionalLight(self, ob, mtx, lamp, world):
        """
        <DirectionalLight DEF="testDirLight" containerField="children" ambientIntensity="0.3" color="0.2 0.2 0.9"
            global="true" intensity="0.6" location="-0.5 0.5 -1"/>
        """
        safeName = self.xName(lamp)
        if world:
            ambi = world.ambient_color
            ambientIntensity = ((float(ambi[0] + ambi[1] + ambi[2]))/3) #/2.5
            print('world ambient ',ambi)
        else:
            ambi = 0
            ambientIntensity = 0
            print('no world - ambient 0')

        intensity=min(lamp.energy/1.75,1.0)
        down = Mathutils.Vector(0.0,0.0,-1.0)
        mtxi = mtx.copy().invert()
        dxyz = mtxi * down
        zero = Mathutils.Vector(0.0,0.0,0.0)
        zero = mtxi * zero
        dxyz = dxyz - zero  #gets rid of translation
        dxyz = dxyz.normalize() #gets rid of scale
        (dx,dy,dz) = (dxyz.x,dxyz.y,dxyz.z)
        lit = self.doc.createElement('DirectionalLight')
        lit.setAttribute('DEF',"%s" % safeName)
        lit.setAttribute('ambientIntensity',"%s" % (round(ambientIntensity,self.cp)))
        lit.setAttribute('intensity',"%s" % (round(intensity,self.cp)))
        lit.setAttribute('color',"%s %s %s" % (round(lamp.color[0],self.cp), round(lamp.color[1],self.cp), round(lamp.color[2],self.cp)))
        lit.setAttribute('direction',"%s %s %s" % (round(dx,4),round(dy,4),round(dz,4))) #so when blender rotations(0,0,0) direction is down
        return lit


    def writePointLight(self, ob, mtx, lamp, world):
        """
        <Transform DEF="OB_Lamp_001" translation="40.841 40.778 5.994" rotation="1.0 0.0 0.0 0.0" scale="1.0 1.0 1.0">
          <PointLight DEF="Lamp_001" ambientIntensity="0.0" color="1.0 1.0 1.0" intensity="0.909" radius="200.0" location="0.0 0.0 0.0" />
        </Transform>
        """
        safeName = self.xName(lamp) #self.cleanStr(ob.name)
        if world:
            ambi = world.ambient_color
            #ambi = world.amb
            ambientIntensity = ((float(ambi[0] + ambi[1] + ambi[2]))/3) #/2.5
        else:
            ambi = 0
            ambientIntensity = 0

        lit = self.doc.createElement('PointLight')
        lit.setAttribute('DEF',"%s" % safeName)
        lit.setAttribute('ambientIntensity',"%s" % (round(ambientIntensity,self.cp)))
        lit.setAttribute('color',"%s %s %s" % (round(lamp.color[0],self.cp), round(lamp.color[1],self.cp), round(lamp.color[2],self.cp)))
        #lit.setAttribute('color',"%s %s %s" % (round(lamp.col[0],self.cp), round(lamp.col[1],self.cp), round(lamp.col[2],self.cp)))
        lit.setAttribute('intensity',"%s" % (round( min(lamp.energy/1.75,1.0) ,self.cp)))
        lit.setAttribute('radius',"%s" % (lamp.distance))
        #lit.setAttribute('radius',"%s" % (lamp.dist))
        lit.setAttribute('location',"%s %s %s" % (0.0,0.0,0.0))
        return lit

    def secureName(self, name):
        name = name + str(self.nodeID)
        self.nodeID=self.nodeID+1
        if len(name) <= 3:
            newname = "_" + str(self.nodeID)
            return "%s" % (newname)
        else:
            for bad in ['"','#',"'",',','.','[','\\',']','{','}']:
                name=name.replace(bad,'_')
            if name in self.namesReserved:
                newname = name[0:3] + "_" + str(self.nodeID)
                return "%s" % (newname)
            elif name[0].isdigit():
                newname = "_" + name + str(self.nodeID)
                return "%s" % (newname)
            else:
                newname = name
                return "%s" % (newname)

    def writeText(self,ob):
        """
        <Transform DEF='OB_HUD_Text' translation='0.0 1.1 0.01'>
         <Shape DEF='Text1' containerField='children'>
          <Appearance containerField='appearance'>
           <Material DEF='BarColor' diffuseColor='0 1 0' />
          </Appearance>
          <Text DEF='HudText' containerField='geometry' string='"This is 3D Text!"' maxExtent='0.000'>
           <FontStyle containerField='fontStyle' family='SANS' style='PLAIN' justify='"MIDDLE" "MIDDLE"' size='.15' spacing='1.000'/>
          </Text>
         </Shape>
        </Transform>
        """
        #Aug 17, 2010 this has not been upgraded to bpy 2.5
        #suggestions:
        #bpy.types.TextCurve
        #bpy.types.TextCurve.body #the string text
        #bpy.types.TextCurve.body_format #booleans for bold,italics,underline
        #byp.types.TextCurve.font  VectorFont
        #bpy.types.TextCurve.spacemode  enum LEFT, CENTRAL, RIGHT, JUSTIFY, FLUSH
        #bpy.types.VectorFont  - VectorFont which has a filename url
        # ... text_size, word_spacing, ul_height, ul_position, offset_x,_y
        #q. how do you get the name of the font?
       
        tcu = ob.data
        shp = self.doc.createElement('Shape')
        meshME = self.xName(tcu)
        print("meshnames=",self.meshNames)
        print("meshMe=",meshME)
        print("in meshnames=",meshME in self.meshNames)
        # look up mesh name, use it if available
        if meshME in self.meshNames:
            shp.setAttribute('USE',meshME) 
            self.meshNames[meshME]+=1
        else:
            self.meshNames[meshME]=1
            shp.setAttribute('DEF',meshME) 

            body = tcu.body
            maters = tcu.materials

            if len(maters) > 0:
                app = self.doc.createElement('Appearance')
                shp.appendChild(app)
                # right now this script can only handle a single material per text
                if True: #len(mi) >= 1:
                    mat=maters[0]
                    if not mat.face_texture:
                        app.appendChild(self.writeMaterial(mat, self.xName(maters[0]), False))
                        if len(maters) > 1:
                            print("Warning: mesh named %s has multiple materials" % meshName)
                            print("Warning: only one material per object handled")

            txt = self.doc.createElement('Text')
            txt.setAttribute('string',body)
            print("size,width,len,spacing",tcu.text_size,tcu.width,len(body),tcu.spacing);
            blender2x3dScaleFudgeFactor = 1.2 #flux
            blender2x3dScaleFudgeFactor = .85 #freewrl
            extent = len(body)*tcu.spacing*blender2x3dScaleFudgeFactor
            txt.setAttribute('maxExtent',"%s" % (round(extent,self.cp)))
            fs = self.doc.createElement('FontStyle')
            fs.setAttribute('containerField',"fontStyle")
            """
            #fs.setAttribute('family',ob.data.Font.name)
            #fs.setAttribute('style',ob.data.
            """
            text_size = tcu.text_size * blender2x3dScaleFudgeFactor
            fs.setAttribute('size',"%s" % (round(text_size,self.cp)))
            al = tcu.spacemode
            ju = "LEFT"
            if al == 'LEFT':
                ju = "LEFT"
            if al == 'CENTRAL':
                ju = "MIDDLE"
            if al == 'RIGHT':
                ju = "RIGHT"
            if al == 'FLUSH':
                ju = "MIDDLE"
            fs.setAttribute('justify',ju)

            txt.appendChild(fs)
            shp.appendChild(txt)

        return shp

    def writeIndexedFaceSet(self, ob, mesh, mtx, world, EXPORT_TRI = False):
        """
        <IndexedFaceSet solid="false"
          texCoordIndex="0, 1, 2, 3, -1, 4, 5, 6, 7, -1, 8, 9, 10, 11, -1, "
          coordIndex="0 3 2 1 -1, 4 6 7 5 -1, 8 4 5 9 -1, ">
          <Coordinate DEF="coord_BowerShingles"
                  point="26.502 6.724 -7.442, 26.502 6.724 10.235,
                           15.023 9.127 10.235, 15.023 9.127 -7.442, 16.675 26.41 0.0,
                           -16.675 26.41 0.0, 16.675 16.523 -11.876, -16.675 16.523 -11.876,
                           16.675 16.523 11.876, -16.675 16.523 11.876" />
          <TextureCoordinate point="1.803 -0.629, 1.807 1.627, -1.14 1.629, -1.143 -0.627, -2.183 0.471,
          -2.183 -3.189, 3.183 -3.189, 3.183 0.471, -2.183 4.132, -2.183 0.471, 3.183 0.471, 3.183 4.132, " />
        </IndexedFaceSet>
        """
        imageMap={}   # set of used images
        sided={}	  # 'one':cnt , 'two':cnt
        vColors={}	# 'multi':1
        meshName = self.xName(ob)

        meshME = self.xName(mesh)
        # tessellation faces may not exist
        #if not mesh.tessfaces and mesh.polygons:
        #    mesh.update(calc_tessface=True)
        #mesh_faces = mesh.tessfaces[:]
        mesh_polys = mesh.polygons
        print("mesh_polys = %d " % len(mesh_polys))
        mesh_vertices = mesh.vertices
        print("mesh_vertices = %d " % len(mesh_vertices))
        mesh_faces = mesh_polys
        if len(mesh_faces) == 0:
            print('sorry mesh has 0 mesh_faces')
            return
        print("hurray mesh has %d mesh_faces" % len(mesh_faces))
        mode = []
##        uv_layer = mesh.uv_layers.active.data
##        for poly in me.polygons:
##            #print("Polygon index: %d, length: %d" % (poly.index, poly.loop_total))
##            # range is used here to show how the polygons reference loops,
##            # for convenience 'poly.loop_indices' can be used instead.
##            for loop_index in range(poly.loop_start, poly.loop_start + poly.loop_total):
##                print("    Vertex: %d" % me.loops[loop_index].vertex_index)
##                print("    UV: %r" % uv_layer[loop_index].uv)
        
        mesh_active_uv_texture = None
        if len(mesh.uv_textures) > 0:
            mesh_active_uv_texture = mesh.uv_textures[0]
##        if mesh_active_uv_texture:
##            for face in mesh_active_uv_texture.data:
##            #for face in mesh.faces:
##                if face.halo and 'HALO' not in mode:
##                    mode += ['HALO']
##                if face.billboard and 'BILLBOARD' not in mode:
##                    mode += ['BILLBOARD']
##                if face.object_color and 'OBJECT_COLOR' not in mode:
##                    mode += ['OBJECT_COLOR']
##                if face.collision and 'COLLISION' not in mode:
##                    mode += ['COLLISION']
        top = self.doc.createElement('stub')
        cur = top
##        if 'HALO' in mode and self.halonode == 0:
##            cur = self.doc.createElement('Billboard')
##            cur.setAttribute('axisOfRotation','0 0 0')
##            top.appendChild(cur)
##            self.halonode = 1
##        elif 'BILLBOARD' in mode and self.billnode == 0:
##            cur = self.doc.createElement('Billboard')
##            cur.setAttribute('axisOfRotation','0 1 0')
##            top.appendChild(cur)
##            self.billnode = 1
##        elif 'OBJECT_COLOR' in mode and self.matonly == 0:
##            self.matonly = 1
##        elif 'COLLISION' not in mode and self.collnode == 0:
##            """
##            <Collision DEF="myC" containerField="children" enabled="false" >
##                <!--Add children nodes here-->
##            </Collision>
##            """
##
##            cur = self.doc.createElement('Collision')
##            cur.setAttribute('enabled','false')
##            top.appendChild(cur)
##
##            self.collnode = 1

        nIFSCnt=self.countIFSSetsNeeded(mesh, imageMap, sided, vColors)

        if nIFSCnt > 1:
            grp = self.doc.createElement('Group')
            grp.setAttribute('DEF','G_'+meshName)
            cur.appendChild(grp)

        if 'two' in sided and sided['two'] > 0:
            bTwoSided=1
        else:
            bTwoSided=0
        bTwoSided = mesh.show_double_sided
        shp = self.doc.createElement('Shape')
        cur.appendChild(shp)
        # look up mesh name, use it if available
        if meshME in self.meshNames:
            shp.setAttribute('USE',meshME)
            self.meshNames[meshME]+=1
        else:
            shp.setAttribute('DEF',meshME)
            self.meshNames[meshME]=1
            maters = mesh.materials
            print('materials=')
            print(maters)
            print('len materials=')
            print(len(maters))
            hasImageTexture=0
            issmooth=0

            if len(maters) > 0 or mesh_active_uv_texture:
                app = self.doc.createElement('Appearance')
                shp.appendChild(app)
                # right now this script can only handle a single material per mesh.
                if len(maters) >= 1:
                    mat=maters[0]
                    #if not mat.active_texture: #mat.face_texture:
                    if True:
                        app.appendChild(self.writeMaterial(mat, self.xName(maters[0]), world))
                        if len(maters) > 1:
                            print("Warning: mesh named %s has multiple materials" % meshName)
                            print("Warning: only one material per object handled")

                    #-- textures
                    face = None
                    if mesh_active_uv_texture:
                        for face in mesh_active_uv_texture.data:
                            if (hasImageTexture == 0) and (face.image):
                                app.appendChild(self.writeImageTexture(face.image))
                                hasImageTexture=1  # keep track of face texture
                                break
                    if self.tilenode == 1 and face and face.image:
                        ttr = doc.createElement('TextureTransform')
                        ttr.setAttribute('scale',"%s %s" % (face.image.xrep, face.image.yrep))
                        app.appendChild(ttr)
                        self.tilenode = 0

            #-- IndexedFaceSet or IndexedLineSet

            # user selected BOUNDS=1, SOLID=3, SHARED=4, or TEXTURE=5
            ifStyle="IndexedFaceSet"
            # look up mesh name, use it if available
            if True: #<<Sep3 end
                ifs = self.doc.createElement('IndexedFaceSet')
                shp.appendChild(ifs)

                if bTwoSided == 1:
                    ifs.setAttribute('solid','false')
                else:
                    ifs.setAttribute('solid','true')

                for face in mesh_faces:
                    if face.use_smooth:
                        issmooth=1
                        break
                if issmooth==1:
                    creaseAngle=(mesh.autosmooth_angle)*(math.pi/180.0)
                    ifs.setAttribute('creaseAngle',"%s" % (round(creaseAngle,self.cp)))

                #--- output textureCoordinates if UV texture used
                if mesh_active_uv_texture:
                    if self.matonly == 1 and self.share == 1:
                        ifs.setAttribute('colorPerVertex','false')
                    elif hasImageTexture == 1:
                        ifs.setAttribute('texCoordIndex',self.writeTextureCoordinates(mesh,0))
                #--- output coordinates
                ifs.setAttribute('coordIndex',self.writeCoordinates(ob, mesh, meshName,0,EXPORT_TRI))
                ifs.appendChild(self.writeCoordinates(ob, mesh, meshName,1,EXPORT_TRI))

                #--- output textureCoordinates if UV texture used
                #print('mesh.active_uv_texture=',mesh_active_uv_texture)
                if mesh_active_uv_texture:
                    if hasImageTexture == 1:
                        ifs.appendChild(self.writeTextureCoordinates(mesh,1))
                    elif self.matonly == 1 and self.share == 1:
                        ifs.appendChild(self.writeFaceColors(mesh))
                #--- output vertexColors
        self.matonly = 0
        self.share = 0

        cur = top.firstChild #python docs 8.6.2.2 Node object
        return cur

    def writeCoordinates(self, ob, mesh, meshName, writingcoords, EXPORT_TRI = False):
        # writingcoords
        #   = 0 means write the coordinate indexes
        #   = 1 means write the vertices
        """
        <IndexedFaceSet solid="false"
          texCoordIndex="0, 1, 2, 3, -1, 4, 5, 6, 7, -1, 8, 9, 10, 11, -1, "
          coordIndex="0 3 2 1 -1, 4 6 7 5 -1, 8 4 5 9 -1, ">
          <Coordinate DEF="coord_BowerShingles"
                  point="26.502 6.724 -7.442, 26.502 6.724 10.235,
                           15.023 9.127 10.235, 15.023 9.127 -7.442, 16.675 26.41 0.0,
                           -16.675 26.41 0.0, 16.675 16.523 -11.876, -16.675 16.523 -11.876,
                           16.675 16.523 11.876, -16.675 16.523 11.876" />
          <TextureCoordinate point="1.803 -0.629, 1.807 1.627, -1.14 1.629, -1.143 -0.627, -2.183 0.471,
          -2.183 -3.189, 3.183 -3.189, 3.183 0.471, -2.183 4.132, -2.183 0.471, 3.183 0.471, 3.183 4.132, " />
        </IndexedFaceSet>
        """

        if writingcoords == 0:
            #-- indexes
            cistr = ""
            nn = 0
            
            for poly in mesh.polygons:
                fv = poly.vertices
                fe = poly.edge_keys
                for loop_index in range(poly.loop_start, poly.loop_start + poly.loop_total):
                    #print("    Vertex: %d" % mesh.loops[loop_index].vertex_index)
                    #print("    UV: %r" % uv_layer[loop_index].uv)
                    cistr = cistr + "%i " % mesh.loops[loop_index].vertex_index
                cistr = cistr + "-1 "
            return cistr
        else:
            #-- vertices
            crd = self.doc.createElement('Coordinate')
            crd.setAttribute('DEF','coord_'+meshName)
            ptstr = ""
            nn = 0
            for v in mesh.vertices:
                if nn > 0:
                    ptstr = ptstr + ", "
                nn += 1
                ptstr =  ptstr + ("%.6f %.6f %.6f" % tuple(v.co))
            crd.setAttribute('point',ptstr)
            return crd


    def writeTextureCoordinates(self, mesh, writingtexture):
        """
        <IndexedFaceSet solid="false"
          texCoordIndex="0, 1, 2, 3, -1, 4, 5, 6, 7, -1, 8, 9, 10, 11, -1, "
          coordIndex="0 3 2 1 -1, 4 6 7 5 -1, 8 4 5 9 -1, ">
          <Coordinate DEF="coord_BowerShingles"
                  point="26.502 6.724 -7.442, 26.502 6.724 10.235,
                           15.023 9.127 10.235, 15.023 9.127 -7.442, 16.675 26.41 0.0,
                           -16.675 26.41 0.0, 16.675 16.523 -11.876, -16.675 16.523 -11.876,
                           16.675 16.523 11.876, -16.675 16.523 11.876" />
          <TextureCoordinate point="1.803 -0.629, 1.807 1.627, -1.14 1.629, -1.143 -0.627, -2.183 0.471,
          -2.183 -3.189, 3.183 -3.189, 3.183 0.471, -2.183 4.132, -2.183 0.471, 3.183 0.471, 3.183 4.132, " />
        </IndexedFaceSet>
        """

        if writingtexture == 0:
            #-- indexes
            cistr = ""
            nn = 0
            
            for poly in mesh.polygons:
                fv = poly.vertices
                fe = poly.edge_keys
                for loop_index in range(poly.loop_start, poly.loop_start + poly.loop_total):
                    #print("    Vertex: %d" % mesh.loops[loop_index].vertex_index)
                    #print("    UV: %r" % uv_layer[loop_index].uv)
                    cistr = cistr + "%i " % loop_index #uv_layer[loop_index].uv
                cistr = cistr + "-1 "
            return cistr
        else:
            #-- vertices
            uv_layer = mesh.uv_layers.active.data
            tpt = self.doc.createElement('TextureCoordinate')
            tpstr = ""
            for poly in mesh.polygons:
                fv = poly.vertices
                fe = poly.edge_keys
                for loop_index in range(poly.loop_start, poly.loop_start + poly.loop_total):
                    #print("    Vertex: %d" % mesh.loops[loop_index].vertex_index)
                    #print("    UV: %r" % uv_layer[loop_index].uv)
                    uv = uv_layer[loop_index].uv
                    tpstr = tpstr + "%s %s," % (round(uv[0],self.tp), round(uv[1],self.tp))
            tpt.setAttribute('point',tpstr)
            return tpt

    def writeFaceColors(self, mesh):
        col = None
        if mesh.active_vertex_color:
            col = self.doc.createElement('Color')
            clstr = ""
            nn = 0
            for face in mesh.active_vertex_color.data:
                c = face.color1
                if nn > 0:
                    clstr = clstr + ", "
                nn += 1
                if self.verbose > 2:
                    print("Debug: face.col r=%d g=%d b=%d" % (c.r, c.g, c.b))
                aColor = self.rgbToFS(c)
                clstr = clstr + aColor
            col.setAttribute('color',clstr)
        return col

    def writePolyline2D(self, ob, mesh, mtx, world):
        """
        http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/geometry2D.html#Polyline2D
        <Polyline2D lineSegments='x y, x y, x y ...'/>
        """

        meshME = self.xName(mesh)
        mesh_polys = mesh.polygons
        print("mesh_polys = %d " % len(mesh_polys))
        mesh_vertices = mesh.vertices
        mesh_edges = mesh.edges
        mesh_loops = mesh.loops
        print("mesh_vertices = %d " % len(mesh_vertices))
        print("mesh_loops = %d " % len(mesh_loops))
        print("mesh_edges = %d " % len(mesh_edges))
        mesh_faces = mesh_polys

        #organize edges into contiguous polylines
        if(len(mesh.edges) < 1):
            return
        first = cur = mesh.edges[0]
        cur.select = True #use select to flag a reversed edge
        cur.hide = False #flag if we've already used it once
        seq = [cur]
        k = 1
        more = True
        while more:
            more = False
            for edge in mesh.edges:
                if edge != cur and  not edge.hide:
                    for i in range(2):
                        if cur.vertices[k] == edge.vertices[i]:
                            seq.append(edge)
                            cur = edge
                            k = 1 - i
                            cur.hide = True
                            cur.select = False
                            if k:
                                cur.select = True
                            if cur != first:
                                more = True
                            break

##        if meshME == 'ME_Circle_Eemhaven':
##            print('Eemhave mesh.edges:')
##            for edge in mesh.edges:
##                print("%d %d" % (edge.vertices[0],edge.vertices[1]))
##            print('Eemhave seq:')
##            for edge in seq:
##                print("&d &d" % (edge.vertices[0],edge.vertices[1]))

        shp = self.doc.createElement('Shape')
        # look up mesh name, use it if available
        if meshME in self.meshNames:
            shp.setAttribute('USE',meshME)
            self.meshNames[meshME]+=1
        else:
            shp.setAttribute('DEF',meshME)
            self.meshNames[meshME]=1
            maters = mesh.materials
            print('materials=')
            print(maters)
            print('len materials=')
            print(len(maters))
            hasImageTexture=0
            issmooth=0

            if len(maters) > 0:
                app = self.doc.createElement('Appearance')
                shp.appendChild(app)
                # right now this script can only handle a single material per mesh.
                if len(maters) >= 1:
                    mat=maters[0]
                    app.appendChild(self.writeMaterial(mat, self.xName(maters[0]), world))
                    if len(maters) > 1:
                        print("Warning: mesh named %s has multiple materials" % meshName)
                        print("Warning: only one material per object handled")


            ifs = self.doc.createElement('Polyline2D')
            #--- output coordinates
            cstr = ""
            last = seq[len(seq)-1]
            for edge in seq:
                k = 0
                if edge.select:
                    k = 1
                vidx = edge.vertices[k] #first point of each edge
                vx = mesh_vertices[vidx]
                cstr = cstr + "%s %s," % (round(vx.co[0],self.tp), round(vx.co[1],self.tp))
                if edge == last and last != seq[0]:
                    #if not closed polygon, write out last point
                    vidx = edge.vertices[1-k] #last point of last edge
                    vx = mesh_vertices[vidx]
                    cstr = cstr + "%s %s," % (round(vx.co[0],self.tp), round(vx.co[1],self.tp))
            ifs.setAttribute('lineSegments',cstr)
            shp.appendChild(ifs)
        for edge in mesh_edges:
            edge.select = False
            edge.hide = False
        return shp


    def writeMaterial(self, mat, matName, world):
        """
        <Appearance>
                <Material DEF="MA_MatBlackShingles" diffuseColor="0.23 0.2 0.2" specularColor="0.01 0.01 0.01" emissiveColor="0.0 0.0 0.0"
                        ambientIntensity="0.104" shininess="0.002" transparency="0.0" />
                <ImageTexture DEF="ShingleBlack_jpg" url="ShingleBlack.jpg" />
        </Appearance>
        """
        mt = self.doc.createElement("Material")
        # look up material name, use it if available
        if matName in self.matNames:
            mt.setAttribute('USE',matName)
            self.matNames[matName]+=1
            return mt

        self.matNames[matName]=1

        ambient = mat.ambient/3
        diffuseR, diffuseG, diffuseB = tuple(mat.diffuse_color)
        if world:
            ambi = world.ambient_color
            ambi0, ambi1, ambi2 = (ambi[0]*mat.ambient)*2, (ambi[1]*mat.ambient)*2, (ambi[2]*mat.ambient)*2
        else:
            ambi0, ambi1, ambi2 = 0, 0, 0
        emisR, emisG, emisB = (diffuseR*mat.emit+ambi0)/2, (diffuseG*mat.emit+ambi1)/2, (diffuseB*mat.emit+ambi2)/2

        shininess = mat.specular_hardness/512.0
        #shininess = mat.hard/512.0
        specR = (mat.specular_color[0]+0.001)/(1.25/(mat.specular_intensity+0.001))
        specG = (mat.specular_color[1]+0.001)/(1.25/(mat.specular_intensity+0.001))
        specB = (mat.specular_color[2]+0.001)/(1.25/(mat.specular_intensity+0.001))
        transp = 1-mat.alpha
        if mat.use_shadeless:
            ambient = 1
            shine = 1
            specR = emitR = diffuseR
            specG = emitG = diffuseG
            specB = emitB = diffuseB
        mt.setAttribute('DEF',matName)
        mt.setAttribute('diffuseColor',"%s %s %s" % (round(diffuseR,self.cp), round(diffuseG,self.cp), round(diffuseB,self.cp)))
        mt.setAttribute('specularColor',"%s %s %s" % (round(specR,self.cp), round(specG,self.cp), round(specB,self.cp)))
        mt.setAttribute('emissiveColor',"%s %s %s" % (round(emisR,self.cp), round(emisG,self.cp), round(emisB,self.cp)))
        mt.setAttribute('ambientIntensity',"%s" % (round(ambient,self.cp)))
        mt.setAttribute('shininess',"%s" % (round(shininess,self.cp)))
        mt.setAttribute('transparency',"%s" % (round(transp,self.cp)))
        return mt

    def writeImageTexture(self, image):
        """
        <Shape DEF="BowerShingles" >
                <Appearance>
                        <Material DEF="MA_MatBlackShingles" diffuseColor="0.23 0.2 0.2" specularColor="0.01 0.01 0.01" emissiveColor="0.0 0.0 0.0"
                                ambientIntensity="0.104" shininess="0.002" transparency="0.0" />
                        <ImageTexture DEF="ShingleBlack_jpg" url="ShingleBlack.jpg" />
                </Appearance>
                <IndexedFaceSet solid="false" ...
        """
        tex = self.doc.createElement("ImageTexture")
        name = self.xName(image)
        filename = image.filepath.split('/')[-1].split('\\')[-1]
        if name in self.texNames:
            tex.setAttribute('USE',name)
            self.texNames[name] += 1
            return tex
        else:
            tex.setAttribute('DEF',name)
            tex.setAttribute('url',filename)
            self.texNames[name] = 1
        return tex

    def writeBackground(self, world, alltextures):
        """
        <Background groundColor="0.057 0.221 0.4" skyColor="0.057 0.221 0.4" />
        """
        bg = None
        if world:	worldname = world.name
        else:		return bg
        blending = (world.use_sky_blend, world.use_sky_paper, world.use_sky_real)
        grd = world.horizon_color
        grd0, grd1, grd2 = grd[0], grd[1], grd[2]
        sky = world.zenith_color
        sky0, sky1, sky2 = sky[0], sky[1], sky[2]
        mix0, mix1, mix2 = grd[0]+sky[0], grd[1]+sky[1], grd[2]+sky[2]
        mix0, mix1, mix2 = mix0/2, mix1/2, mix2/2
        bg = self.doc.createElement("Background")
        # http://www.web3d.org/x3d/specifications/ISO-IEC-FDIS-19775-1.2-X3D-AbstractSpecification/index.html
        #> 24.Environmental Effects 24.2.1 Background
        if worldname not in self.namesStandard:
            bg.setAttribute('DEF',self.secureName(worldname))
        # No Skytype - just Hor color
        if blending == (False,False,False): #0
            bg.setAttribute('groundColor',"%s %s %s" % (round(grd0,self.cp), round(grd1,self.cp), round(grd2,self.cp)))
            bg.setAttribute('skyColor',"%s %s %s" % (round(grd0,self.cp), round(grd1,self.cp), round(grd2,self.cp)))
        # Blend Gradient
        elif blending == (True,False,False): #1:
            bg.setAttribute('groundColor',"%s %s %s" % (round(grd0,self.cp), round(grd1,self.cp), round(grd2,self.cp)))
            bg.setAttribute("groundAngle","1.57, 1.57")
            bg.setAttribute('skyColor',"%s %s %s" % (round(sky0,self.cp), round(sky1,self.cp), round(sky2,self.cp)))

            bg.setAttribute('skyAngle',"1.57, 1.57")
        # Blend+Real Gradient Inverse
        elif blending == (True,False,True): #3:
            bg.setAttribute('groundColor',"%s %s %s" % (round(sky0,self.cp), round(sky1,self.cp), round(sky2,self.cp)))
            bg.setAttribute('groundAngle',"1.57, 1.57")
            bg.setAttribute('skyColor',"%s %s %s" % (round(grd0,self.cp), round(grd1,self.cp), round(grd2,self.cp)))
            bg.setAttribute('skyAngle',"1.57, 1.57")
        # Paper - just Zen Color
        elif blending == (False,True,False): #4:
            bg.setAttribute('groundColor',"%s %s %s" % (round(sky0,self.cp), round(sky1,self.cp), round(sky2,self.cp)))
            bg.setAttribute('skyColor',"%s %s %s" % (round(sky0,self.cp), round(sky1,self.cp), round(sky2,self.cp)))
        # Blend+Real+Paper - komplex gradient
        elif blending == (True,True,True): #7:
            bg.setAttribute('groundColor',"%s %s %s" % (round(sky0,self.cp), round(sky1,self.cp), round(sky2,self.cp)))

            bg.setAttribute('groundAngle',"1.57, 1.57")
            bg.setAttribute('skyColor',"%s %s %s" % (round(sky0,self.cp), round(sky1,self.cp), round(sky2,self.cp)))
            bg.setAttribute('skyAngle',"1.57, 1.57")

        # Any Other two colors
        else:
            bg.setAttribute('groundColor',"%s %s %s" % (round(grd0,self.cp), round(grd1,self.cp), round(grd2,self.cp)))
            bg.setAttribute('skyColor',"%s %s %s" % (round(sky0,self.cp), round(sky1,self.cp), round(sky2,self.cp)))

        alltexture = len(alltextures)
        sidenames = ["back","top","front","left","right","back"]
        for i in range(alltexture):
            tex = alltextures[i]

            if tex.type != 'IMAGE' or tex.image == None:
                continue

            namemat = tex.name
            pic = tex.image
            if (pic != None):
                for side in sidenames:
                    if side == namemat:
                        uri = pic.filename.split('/')[-1].split('\\')[-1]
                        attnam = side+'Url'
                        bg.setAttribute(attnam,uri)
        return bg
    
    def writeGroup(self,parent):
        #assumes you want to export your blender groups in the current x3d file, not #url to another x3d
        #our usual tactic: first instance of group gets DEF, subsequent get USE
        grp = parent.dupli_group
        return self.writeGrp(grp)

    def writeGrp(self,grp):
        #assumes you want to export your blender groups in the current x3d file, not #url to another x3d
        #our usual tactic: first instance of group gets DEF, subsequent get USE
        gp = self.doc.createElement('Group')
        name = self.xName(grp) #grp.name #do we need cleanstr? it works without
        if name in self.grpNames:
            gp.setAttribute('USE',name) #'GR_'+self.cleanStr(name))
            self.grpNames[name] += 1
        else:
            gp.setAttribute('DEF',name) #'GR_'+self.cleanStr(name))
            #this grp ob has matrix from source scene (ie library) not parent Empty, and
            #using writeNode on all group nodes will double write because some group members are children of other group members and
            #writeNode does all children when nested=1
            #if grp is not a library group ie current scene group then individual nodes will be double DEFd.
            #first DEF each node into the library
            nested = self.nested #turn off nesting to write library nodes flat
            self.nested = 0
            for ob in list(grp.objects):
                obname = self.xName(ob,grp=1)
                if obname in self.lobNames:
                    self.lobNames[obname] += 1
                else:
                    nd = self.writeNode(ob,grp=1)
                    self.libraryNode.appendChild(nd)
                    self.lobNames[obname] = 1
            self.nested = nested

            #then add them all flat into a group
            for ob in list(grp.objects):
                trans = self.doc.createElement('Transform')
                obname = self.xName(ob,grp=1)
                trans.setAttribute('USE',obname)
                gp.appendChild(trans)
            self.grpNames[name] = 1
            self.libraryNode.appendChild(gp)
            gp = self.writeGrp(grp) #group DEFd lib - should USE now
        return gp


##########################################################
# export routine
##########################################################

    def writeData(self,ob):
        ob_mat = ob.matrix_world
        objType=ob.type
        objName=ob.name
        self.matonly = 0
        scene = self.scene
        world = self.world

        dataNode = None

        if objType == "CAMERA":
            dataNode = self.writeViewpoint(ob, ob_mat, scene)
        elif objType == "TEXT":
            print(33,objName,objType)
            dataNode = self.writeText(ob)
        elif objType in ("MESH", "CURVE", "SURF"): #, "Text") :
            if  self.constants['EXPORT_APPLY_MODIFIERS'] or objType != 'MESH':
                me = ob.create_mesh(EXPORT_APPLY_MODIFIERS, 'PREVIEW')
            else:
                me = ob.data
            if 'circle' in ob.data.name.lower() and self.polyline2d:
                print('Exporting Circle as Polyline2D')
                dataNode = self.writePolyline2D(ob,me,ob_mat,world)
            else:
                dataNode = self.writeIndexedFaceSet(ob, me, ob_mat, world, EXPORT_TRI = self.constants['EXPORT_TRI'])

            if me != ob.data:
                bpy.data.meshes.remove(me)
            
        elif objType == "LAMP":
            data= ob.data
            datatype=data.type
            if datatype == 'POINT':
                dataNode = self.writePointLight(ob, ob_mat, data, world)
            elif datatype == 'SPOT':
                dataNode = self.writeSpotLight(ob, ob_mat, data, world)
            elif datatype == 'SUN':
                pass #directional lights in x3d apply only to siblings and their descendents in transform heirarchy
                   #so directional light and its ob handled together in writeNode so as not to bury it in a transform wrapper
            else:
                #Area, Hemi
                dataNode = self.writeDirectionalLight(ob, ob_mat, data, world)
        elif objType == "EMPTY":
            if ob.dupli_type == 'GROUP':
                dataNode = self.writeGroup(ob)
        else:
            #print "Info: Ignoring [%s], object type [%s] not handle yet" % (object.name,object.getType)
            dataNode = None

        return dataNode




    def getChildren(self,parent):
        #this is a simple version which is inefficient for a v.large scene O(n*n*n)
        #in theory you could pre-process into a dict of lists stored by parent
        # ie preprocess: kidlist={}, kidlist[ob] = [] O(n), kidlist[ob.parent].append(ob) O(n?dict lookup)
        #          then: getChildren = kidlist[ob]
        # this method orphans scene graph chunks separated from scene root by unselected items
        searchlist = self.scene.objects
        if self.currentGroupObjects != None:
            searchlist = self.currentGroupObjects
        kidlist = []
        for ob in searchlist:
            if ob.parent == parent:
                kidlist = kidlist + [ob]
        return kidlist

    def writeTransform(self,ob,grp=0):
        """
        <Transform DEF="OB_Window2pane106"
          translation="-6.000000 0.000000 0.000000"
          scale="1.000000 1.000000 1.000000"
          rotation="-1.000000 0.000000 0.000000 1.570796">
          <Shape>
              <--... data ...-->
          </Shape>
          <--...children list...-->
          <Transform DEF="OB_WindowSubFrame" >
          </Transform>
          <Transform DEF="OB_WindowSubGlass" >
          </Transform>
        </Transform>
        """


        mtx = ob.matrix_world.copy()
        thisName = self.xName(ob,grp)

        if self.nested:
            if ob.parent != None:
                pmat = ob.parent.matrix_world.copy().transpose()
                mtx = mtx.transpose() * pmat.invert()
                mtx = mtx.transpose()

        loc, rot, sca = mtx.decompose()
        rot = rot.to_axis_angle()
        rot = rot[0][:] + (rot[1], )

##        loc= mtx.translation_part() #Part()
##        sca= mtx.scale_part() #Part()
##        quat = mtx.to_quat() #Quat()
##        rot= quat.axis

        trans = self.doc.createElement("Transform")
        trans.setAttribute('DEF',thisName)
        trans.setAttribute('translation','%.6f %.6f %.6f'%(loc[0], loc[1], loc[2]))
        trans.setAttribute('scale','%.6f %.6f %.6f'%(sca[0], sca[1], sca[2]))
        trans.setAttribute('rotation','%.6f %.6f %.6f %.6f'%(rot[0], rot[1], rot[2], rot[3])) #quat.angle))
        return trans

    def writeNode(self,ob,grp=0):
        """
        1) write transform
        2) write data
        3) if NESTED write children (recursively)
        """

        self.indentLevel +=1
        inLib = not ob.library is None
        name = self.xName(ob,grp)
        if name in self.lobNames:
            trans = self.doc.createElement('Transform')
            trans.setAttribute('USE',name)
            self.lobNames[name] += 1
        else:
            #print('>>writenode ',ob.name,grp,self.indentLevel,inLib)
            sun = ob.type == "LAMP"
            if sun:
                sun = ob.data.type == 'SUN'
            if sun:
                trans = self.writeDirectionalLight(ob, ob.matrix, ob.data, self.world)
                #note: children lost - don't parent to your sun
            else:
                trans = self.writeTransform(ob,grp)
                viz = ob.is_visible(self.scene)
                if viz:
                    dataNode = self.writeData(ob)
                    if dataNode != None:
                        trans.appendChild(dataNode)
                if self.nested:
                    kids = self.getChildren(ob)
                    for c in kids:
                        trans.appendChild(self.writeNode(c,grp))
            #print('<<writenode',ob.name)
        self.indentLevel -= 1
        return trans

    def writeLibSwitch(self):
        """
        <Switch DEF="library_nodes" containerField="children"  whichChoice="-1">
          <!--Add children nodes here-->
        </Switch>
        """
        lib = self.doc.createElement('Switch')
        lib.setAttribute('DEF','library_nodes')
        lib.setAttribute('whichChoice','-1') #makes invisible, the default
        return lib




    def export(self, scene, world, context, alltextures,\
            EXPORT_APPLY_MODIFIERS = False,\
            EXPORT_TRI=				False,\
            EXPORT_SELECTED = False,\
            EXPORT_NESTED = True,\
            EXPORT_POLYLINE2D = True\
            ):

        print("Info: starting X3D export to " + self.filename + "...")
        root = self.writeHeader()
        x3dscene = self.doc.createElement("Scene")
        root.appendChild(x3dscene)
        matw = self.doc.createElement("Transform")
        matw.setAttribute('DEF','MATWORLD')
        """ rotation="-1.000000 0.000000 0.000000 1.570796"> """
        matw.setAttribute('rotation',"-1.000000 0.000000 0.000000 1.570796")
        x3dscene.appendChild(matw)
        x3dscene = matw


        # self.writeScript()
        nav = self.writeNavigationInfo(scene)
        bac = self.writeBackground(world, alltextures)
        fog = self.writeFog(world)
        if nav != None: x3dscene.appendChild(nav)
        if bac != None: x3dscene.appendChild(bac)
        if fog != None: x3dscene.appendChild(fog)

        self.proto = 0
        self.scene = scene
        self.world = world
        self.constants['EXPORT_APPLY_MODIFIERS'] = EXPORT_APPLY_MODIFIERS  #used in writeData
        self.constants['EXPORT_TRI'] = EXPORT_TRI


##        # COPIED FROM OBJ EXPORTER
##        if EXPORT_APPLY_MODIFIERS:
##            temp_mesh_name = '~tmp-mesh'
##
##            # Get the container mesh. - used for applying modifiers and non mesh objects.
##            containerMesh = meshName = tempMesh = None
##            for meshName in NMesh.GetNames():
##                if meshName.startswith(temp_mesh_name):
##                    tempMesh = Mesh.Get(meshName)
##                    if not tempMesh.users:
##                        containerMesh = tempMesh
##            if not containerMesh:
##                containerMesh = Mesh.New(temp_mesh_name)
##        # --------------------------

        self.selectedList = []
        if EXPORT_SELECTED:
            self.selectedList = scene.objects.context
        else:
            self.selectedList = list(scene.objects)

        self.nested = EXPORT_NESTED
        self.polyline2d = EXPORT_POLYLINE2D
        self.indentLevel = 0
        self.libraryNode = self.writeLibSwitch()
        x3dscene.appendChild(self.libraryNode)

        if self.nested:
            #heirarchical scene graph style x3d with local transforms useful for animating
            #do all library nodes
            nested = self.nested
            self.nested = 0
            for ob_main in self.selectedList:
                if ob_main.library != None:
                    self.libraryNode.appendChild(self.writeNode(ob_main))
                    self.lobNames[self.cleanStr(ob_main.name)] = 1
            self.nested = nested
            #do all groups
            #grps = context.main.groups
            grps = context.blend_data.groups
            for g in grps:
                self.writeGrp(g) #throw away reference

            for ob_main in self.selectedList:
                rootnode = False
                if ob_main.parent == None: #with nested, just start with the root nodes
                    rootnode = True
                else:
                    if not ob_main.parent in self.selectedList: #orphaned by getChildren
                        rootnode = True
                if rootnode:
                    #print('rootnode >>',ob_main.name)
                    nd = self.writeNode(ob_main)
                    #print('<< rootnode',ob_main.name)
                    if nd != None:
                        x3dscene.appendChild(nd)
                    else:
                        print('no node for root ob=',ob_main.name)
        else:
            #default flat file style - all transforms global
            for ob_main in self.selectedList:
                nd = self.writeNode(ob_main)
                if nd != None:
                    x3dscene.appendChild(nd) #with flat, do everything
                else:
                    print('no node for ob=',ob_main.name)


        #self.file.write("\n</Scene>\n</X3D>")

        #start > over-ride on minidom to get X3D style MFSTRING ie '"EXAMINE" "ANY"' without &quot
        #X3DEdit from www.web3d.org passes the reverse syntax "'EXAMINE' 'ANY'" which also passes through xmlwriter, but
        #gets load errors in FluxPlayer and is not the standard way its written so other ones may have trouble parsing, so
        #this hack provides a way to pass lists of strings to the minidom and have them formatted the normal x3d way.
        #if you have a better approach please apply it and delete the following, and replace the [,] list approach in writeNavigationInfo

        #strategy: call node.setAttribute('type',["EXAMINE","ANY"]) ie with value as a list of strings instead of a string
        #then over-ride minidom element writer to detect when its a list and write in X3D MFSTRING style: '"EXAMINE" "ANY"'
        # xml.dom.minidom._write_data is here for reference (from python25 minidom.py) but could also be over-ridden see below

        def _write_data_orig(writer, data):
            "Writes datachars to writer."
            data = data.replace("&", "&amp;").replace("<", "&lt;")
            data = data.replace("\"", "&quot;").replace(">", "&gt;")
            writer.write(data)

        # xml.dom.minidom.element.writexml() - we are going to over-ride with the following
        def writexml_strlists(self, writer, indent="", addindent="", newl=""):
            # indent = current indentation
            # addindent = indentation to add to higher levels
            # newl = newline string
            writer.write(indent+"<" + self.tagName)

            attrs = self._get_attributes()
            a_names = list(attrs.keys())
            a_names.sort()

            for a_name in a_names:
                #added MFSTRING section '"EXAMINE" "ANY"'
                atval = attrs[a_name].value
                if isinstance(atval,list) or isinstance(atval,tuple):
                    writer.write(" %s='" % a_name)
                    nn = 0
                    for s in atval:
                        if nn > 0:
                            writer.write(" ")
                        nn += 1
                        writer.write('"')
                        _write_data(writer, s)
                        writer.write('"')
                    writer.write("'")
                else:
                    #<end MFSTRING section
                    writer.write(" %s=\"" % a_name)
                    _write_data(writer, attrs[a_name].value)
                    writer.write("\"")
            if self.childNodes:
                writer.write(">%s"%(newl))
                for node in self.childNodes:
                    node.writexml(writer,indent+addindent,addindent,newl)
                writer.write("%s</%s>%s" % (indent,self.tagName,newl))
            else:
                writer.write("/>%s"%(newl))

        def _write_data(writer, data):
            "Writes datachars to writer."
            data = data.replace("&", "&amp;").replace("<", "&lt;")
            #data = data.replace("\"", "&quot;")
            data = data.replace(">", "&gt;")
            writer.write(data)
        def writexml(self, writer, indent="", addindent="", newl=""):
            # indent = current indentation
            # addindent = indentation to add to higher levels
            # newl = newline string
            writer.write(indent+"<" + self.tagName)

            attrs = self._get_attributes()
            a_names = list(attrs.keys())
            a_names.sort()

            for a_name in a_names:
                writer.write(" %s='" % a_name)
                _write_data(writer, attrs[a_name].value)
                writer.write("'")
            if self.childNodes:
                writer.write(">%s"%(newl))
                for node in self.childNodes:
                    node.writexml(writer,indent+addindent,addindent,newl)
                writer.write("%s</%s>%s" % (indent,self.tagName,newl))
            else:
                writer.write("/>%s"%(newl))

        #xml.dom.minidom._write_data = _write_data  #don't need to override now
        xml.dom.minidom.Element.writexml = writexml
        # < finished over-riding minidom

        self.doc.writexml(self.file,"","\t","\n","UTF-8") #"  ","\n")
        self.file.close()
        self.doc.unlink()

        if EXPORT_APPLY_MODIFIERS:
            if containerMesh:
                containerMesh.verts = None

        self.cleanup()

##########################################################
# Utility methods
##########################################################

    def cleanup(self):
        self.file.close()
        self.texNames={}
        self.matNames={}
        self.indentLevel=0
        print("Info: finished X3D export to %s\n" % self.filename)

    def xName(self, thing,grp=0 ):
        """
        generates unique export name from thing.name and thing.lib
        thing - one of: Object, Scene, Group, Mesh, Material, Image, Texture, Text - anything with .lib attribute
        thing.lib - as in the blender py scripting API docs - the path name to the library .blend ie if File > Append or Link > Link
        exmaple1: in current scene thing.name = 'Mesh.001' thing.lib = None -> name = ME_Mesh_001
        example2: in lib thing.name='Mesh.001' thing.lib= '/mainstreet/Doors.blend' (only library blend referenced)
          -> name = 'ME_Mesh_001_L1'
        """
        name = self.cleanStr(thing.name)
        if not thing.library is None:
            if thing.library not in self.libraryIndex:
                self.libraryIndex[thing.library] = len(self.libraryIndex)+1
            libdex = self.libraryIndex[thing.library]
            name = name + '_L'+str(libdex)
        if type(thing) in self.typePrefix:
            name = self.typePrefix[type(thing)]+'_' + name
        if grp:
            name = 'G'+name
        return name


    def cleanStr(self, name, prefix='rsvd_'):
        """cleanStr(name,prefix) - try to create a valid VRML DEF name from object name"""

        newName=name[:]
        if len(newName) == 0:
            self.nNodeID+=1
            return "%s%d" % (prefix, self.nNodeID)

        if newName in self.namesReserved:
            newName='%s%s' % (prefix,newName)

        if newName[0].isdigit():
            newName='%s%s' % ('_',newName)

        for bad in [' ','"','#',"'",',','.','[','\\',']','{','}']:
            newName=newName.replace(bad,'_')
        return newName

    def countIFSSetsNeeded(self, mesh, imageMap, sided, vColors):
        """
        countIFFSetsNeeded() - should look at a blender mesh to determine
        how many VRML IndexFaceSets or IndexLineSets are needed.  A
        new mesh created under the following conditions:

         o - split by UV Textures / one per mesh
         o - split by face, one sided and two sided
         o - split by smooth and flat faces
         o - split when faces only have 2 vertices * needs to be an IndexLineSet
        """

        imageNameMap={}
        faceMap={}
        nFaceIndx=0

        mesh_active_uv_texture = None
        if len(mesh.uv_textures) > 0:
            mesh_active_uv_texture = mesh.uv_textures[0]
        if mesh_active_uv_texture:
            for face in mesh_active_uv_texture.data:
                sidename='one'
##                sidename=''
##                #print('face.twoside=',face.twoside)
##                if face.twoside:
##                    sidename='two'
##                else:
##                    sidename='one'

                if sidename in sided:
                    sided[sidename]+=1
                else:
                    sided[sidename]=1

                image = face.image
                if image:
                    faceName="%s_%s" % (face.image.name, sidename);
                    try:
                        imageMap[faceName].append(face)
                    except:
                        imageMap[faceName]=[face.image.name,sidename,face]

            if self.verbose > 2:
                for faceName in imageMap.keys():
                    ifs=imageMap[faceName]
                    print("Debug: faceName=%s image=%s, solid=%s facecnt=%d" % \
                        (faceName, ifs[0], ifs[1], len(ifs)-2))

        else:
            #just materials? then one flag on buttons > Edit context > Mesh panel > Double Sided button
            if mesh.show_double_sided:
                sided['two'] = 1
        return len(imageMap)

    def faceToString(self,face):

        print("Debug: face.flag=0x%x (bitflags)" % face.flag)
        if face.sel:
            print("Debug: face.sel=true")

        print("Debug: face.mode=0x%x (bitflags)" % face.mode)
        if face.mode & Mesh.FaceModes.TWOSIDE:
            print("Debug: face.mode twosided")

        print("Debug: face.transp=0x%x (enum)" % face.transp)
        if face.transp == Mesh.FaceTranspModes.SOLID:
            print("Debug: face.transp.SOLID")

        if face.image:
            print("Debug: face.image=%s" % face.image.name)
        print("Debug: face.materialIndex=%d" % face.materialIndex)

    def getVertexColorByIndx(self, mesh, indx):
        c = None
        for face in mesh.faces:
            j=0
            for vertex in face.v:
                if vertex.index == indx:
                    c=face.col[j]
                    break
                j=j+1
            if c: break
        return c

    def meshToString(self,mesh):
        print("Debug: mesh.hasVertexUV=%d" % mesh.vertexColors)
        print("Debug: mesh.faceUV=%d" % mesh.faceUV)
        print("Debug: mesh.hasVertexColours=%d" % mesh.hasVertexColours())
        print("Debug: mesh.verts=%d" % len(mesh.verts))
        print("Debug: mesh.faces=%d" % len(mesh.faces))
        print("Debug: mesh.materials=%d" % len(mesh.materials))

    def rgbToFS(self, c):
        s="%s %s %s" % (
            round(c.r/255.0,self.cp),
            round(c.g/255.0,self.cp),
            round(c.b/255.0,self.cp))
        return s

    # swap Y and Z to handle axis difference between Blender and VRML
    #------------------------------------------------------------------------
    def rotatePointForVRML(self, v):
        x = v[0]
        y = v[2]
        z = -v[1]

        vrmlPoint=[x, y, z]
        return vrmlPoint

    # For writing well formed VRML code
    #------------------------------------------------------------------------
    def writeIndented(self, s, inc=0):
        if inc < 1:
            self.indentLevel = self.indentLevel + inc

        spaces=""
        for x in range(self.indentLevel):
            spaces = spaces + "\t"
        self.file.write(spaces + s)

        if inc > 0:
            self.indentLevel = self.indentLevel + inc

    # Converts a Euler to three new Quaternions
    # Angles of Euler are passed in as radians
    #------------------------------------------------------------------------
    def eulerToQuaternions(self, x, y, z):
        Qx = [math.cos(x/2), math.sin(x/2), 0, 0]
        Qy = [math.cos(y/2), 0, math.sin(y/2), 0]
        Qz = [math.cos(z/2), 0, 0, math.sin(z/2)]

        quaternionVec=[Qx,Qy,Qz]
        return quaternionVec

    # Multiply two Quaternions together to get a new Quaternion
    #------------------------------------------------------------------------
    def multiplyQuaternions(self, Q1, Q2):
        result = [((Q1[0] * Q2[0]) - (Q1[1] * Q2[1]) - (Q1[2] * Q2[2]) - (Q1[3] * Q2[3])),
                ((Q1[0] * Q2[1]) + (Q1[1] * Q2[0]) + (Q1[2] * Q2[3]) - (Q1[3] * Q2[2])),
                ((Q1[0] * Q2[2]) + (Q1[2] * Q2[0]) + (Q1[3] * Q2[1]) - (Q1[1] * Q2[3])),
                ((Q1[0] * Q2[3]) + (Q1[3] * Q2[0]) + (Q1[1] * Q2[2]) - (Q1[2] * Q2[1]))]

        return result

    # Convert a Quaternion to an Angle Axis (ax, ay, az, angle)
    # angle is in radians
    #------------------------------------------------------------------------
    def quaternionToAngleAxis(self, Qf):
        scale = math.pow(Qf[1],2) + math.pow(Qf[2],2) + math.pow(Qf[3],2)
        ax = Qf[1]
        ay = Qf[2]
        az = Qf[3]

        if scale > .0001:
            ax/=scale
            ay/=scale
            az/=scale

        angle = 2 * math.acos(Qf[0])

        result = [ax, ay, az, angle]
        return result

##########################################################
# Callbacks, needed before Main
##########################################################

def x3d_export(filepath,
    context,
    EXPORT_APPLY_MODIFIERS=False,
    EXPORT_TRI=False,
    EXPORT_GZIP=False,
    EXPORT_SELECTED=False,
    EXPORT_NESTED=True,
    EXPORT_POLYLINE2D=True):

    #filename = "C:\\Users\\higha_000\\Documents\\untitled.x3d"
    filename = filepath
    if EXPORT_GZIP:
        if not filename.lower().endswith('.x3dz'):
            filename = '.'.join(filename.split('.')[:-1]) + '.x3dz'
    else:
        if not filename.lower().endswith('.x3d'):
            filename = '.'.join(filename.split('.')[:-1]) + '.x3d'


    scene = context.scene
    world = scene.world
    # XXX these are global textures while .Get() returned only scene's?
    alltextures = bpy.data.textures
    # alltextures = Blender.Texture.Get()
    print('hi from x3d_export befor initializing class')
    print('filename=',filename)
    wrlexport=x3d_class()
    wrlexport.setFilename(filename)    
    wrlexport.export(\
        scene,\
        world,\
        context,\
        alltextures,\
        \
        EXPORT_APPLY_MODIFIERS = EXPORT_APPLY_MODIFIERS,\
        EXPORT_TRI = EXPORT_TRI,\
        EXPORT_SELECTED = EXPORT_SELECTED,\
        EXPORT_NESTED = EXPORT_NESTED,\
        EXPORT_POLYLINE2D = EXPORT_POLYLINE2D\
        )


from bpy.props import *

class ExportX3DfreeWRL(bpy.types.Operator, ExportHelper):
    """Export selection to Extensible 3D file (.x3d)"""
    bl_idname = "export_scene_fw.x3d"
    bl_label = 'Export X3D freeWRL'

    filename_ext = ".x3d"
    filter_glob = StringProperty(default="*.x3d", options={'HIDDEN'})

    use_selection = BoolProperty(
            name="Selection Only",
            description="Export selected objects only",
            default=False,
            )
    use_mesh_modifiers = BoolProperty(
            name="Apply Modifiers",
            description="Use transformed mesh data from each object",
            default=False,
            )
    use_triangulate = BoolProperty(
            name="Triangulate",
            description="Write quads into 'IndexedTriangleSet'",
            default=False,
            )
    use_normals = BoolProperty(
            name="Normals",
            description="Write normals with geometry",
            default=False,
            )
    use_compress = BoolProperty(
            name="Compress",
            description="Compress the exported file",
            default=False,
            )
    use_hierarchy = BoolProperty(
            name="Hierarchy",
            description="Export parent child relationships",
            default=True,
            )
    name_decorations = BoolProperty(
            name="Name decorations",
            description=("Add prefixes to the names of exported nodes to "
                         "indicate their type"),
            default=False,
            )
    use_polyline2d = BoolProperty(
            name="Polyline2D",
            description="Export Circles as Polyline2D (vs none)",
            default=True,
            )

    path_mode = path_reference_mode


    def execute(self, context):
        x3d_export(self.properties.filepath, context, self.properties.use_mesh_modifiers,
                   self.properties.use_triangulate, self.properties.use_compress,
                   self.properties.use_selection, self.properties.use_hierarchy,self.properties.use_polyline2d)
        return {'FINISHED'}



def menu_func_export(self, context):
    self.layout.operator(ExportX3DfreeWRL.bl_idname,
                         text="X3D for freeWRL (.x3d)")


def register():
    bpy.utils.register_module(__name__)
    bpy.types.INFO_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_module(__name__)
    bpy.types.INFO_MT_file_export.remove(menu_func_export)

# NOTES
# - blender version is hardcoded

if __name__ == "__main__":
    register()
