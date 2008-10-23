/*******************************************************************
 Copyright (C) 2008 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*******************************************************************

	X3D Programmable Shaders Component

*********************************************************************/

#include "headers.h"
#include "installdir.h"

/* which shader is running?? */
GLuint globalCurrentShader = 0;

/*********************************************************************/

void compile_ComposedShader (struct X3D_ComposedShader *node) {
	#ifdef GL_VERSION_2_0
		/* an array of text pointers, should contain shader source */
		GLchar **vertShaderSource;
		GLchar **fragShaderSource;
	
		vertShaderSource = MALLOC(sizeof(GLchar*) * node->parts.n); 
		fragShaderSource = MALLOC(sizeof(GLchar*) * node->parts.n);
	
		/* do we have anything to compile? */
		int haveVertShaderText = FALSE; 
		int haveFragShaderText = FALSE; 
	
		/* set this up... set it to FALSE if there are problems */
		node->isValid = TRUE;
	
		/* we support only GLSL here */
		if (strcmp(node->language->strptr,"GLSL")) {
			ConsoleMessage ("Shaders: support only GLSL shading language, got :%s:",node->language->strptr);
			node->isValid = FALSE;
		} else {
			int i;
			struct X3D_ShaderPart *part;
			
			/* ok so far, go through the parts */
			for (i=0; i<node->parts.n; i++) {
				part = (struct X3D_ShaderPart *) node->parts.p[i];
				vertShaderSource[i] = "";
				fragShaderSource[i] = "";
	
				if (part!=NULL) {
					if (part->_nodeType == NODE_ShaderPart) {
						/* compile this part */
	
						if (!((strcmp (part->type->strptr,"VERTEX")) && (strcmp(part->type->strptr,"FRAGMENT")))) {
							char *myText = NULL;
							char filename[1000];
							char firstBytes[4];
							
	
							if (getValidFileFromUrl (filename, part->__parenturl->strptr, &part->url, firstBytes) ) {
								myText = readInputString(filename); 
							} else {
								ConsoleMessage ("error reading ShaderPart");
								myText = "";
							}
	
							/* assign this text to VERTEX or FRAGMENT buffers */
							if (!strcmp(part->type->strptr,"VERTEX")) {
								vertShaderSource[i] = myText;
								haveVertShaderText = TRUE;
							} else {
								fragShaderSource[i] = myText;
								haveFragShaderText = TRUE;
							}
	
							#ifdef VERBOSE
							printf ("Shader text %s\n",myText);
							#endif
	
						} else {
							ConsoleMessage ("ShaderPart, invalid Type, got \"%s\"",part->type->strptr);
							node->isValid = FALSE;
						}
					} else {
						ConsoleMessage ("ComposedShader, expected ShaderPart, got \"%s\"",stringNodeType(part->_nodeType));
						node->isValid = FALSE;
					}
				}
	
			}
		}
	
	#define MAX_INFO_LOG_SIZE 512
	
		/* compile shader, if things are ok to here */
		if (node->isValid) {
			GLint success;
			GLuint myProgram = glCreateProgram();
			GLuint myVertexShader = 0;
			GLuint myFragmentShader= 0;
			
	
			if (haveVertShaderText) {
				myVertexShader = glCreateShader(GL_VERTEX_SHADER);	
				glShaderSource(myVertexShader, node->parts.n, (const GLchar **) vertShaderSource, NULL);
				glCompileShader(myVertexShader);
				glGetShaderiv(myVertexShader, GL_COMPILE_STATUS, &success);
				if (!success) {
					GLchar infoLog[MAX_INFO_LOG_SIZE];
					glGetShaderInfoLog(myFragmentShader, MAX_INFO_LOG_SIZE, NULL, infoLog);
					printf ("problem with VERTEX shader: %s\n",infoLog);
					node->isValid = FALSE;
				} else {
					glAttachShader(myProgram, myVertexShader);
				}
			}
			
			if (haveFragShaderText) {	
				myFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);	
				glShaderSource(myFragmentShader, node->parts.n,(const GLchar **)  fragShaderSource, NULL);
				glCompileShader(myFragmentShader);
	
				glGetShaderiv(myFragmentShader, GL_COMPILE_STATUS, &success);
				if (!success) {
					GLchar infoLog[MAX_INFO_LOG_SIZE];
					glGetShaderInfoLog(myFragmentShader, MAX_INFO_LOG_SIZE, NULL, infoLog);
					printf ("problem with FRAGMENT shader: %s\n",infoLog);
					node->isValid = FALSE;
				} else {
					glAttachShader(myProgram, myFragmentShader);
				}
			}
	
			/* link the shader parts together */
			glLinkProgram(myProgram);
			glGetProgramiv(myProgram, GL_LINK_STATUS, &success);
			if (!success) {
				GLchar infoLog[MAX_INFO_LOG_SIZE];
				glGetProgramInfoLog(myFragmentShader, MAX_INFO_LOG_SIZE, NULL, infoLog);
				printf ("problem with Shader Program link: %s\n",infoLog);
				node->isValid = FALSE;
			}
	
			/* does the program get a thumbs up? */	
			glValidateProgram (myProgram);
			glGetProgramiv(myProgram, GL_VALIDATE_STATUS, &success);
			if (!success) {
				GLchar infoLog[MAX_INFO_LOG_SIZE];
				glGetProgramInfoLog(myFragmentShader, MAX_INFO_LOG_SIZE, NULL, infoLog);
				printf ("problem with Shader Program Validate: %s\n",infoLog);
				node->isValid = FALSE;
			}
	
			if (node->__shaderIDS.n == 0) {
				node->__shaderIDS.n = 1;
				node->__shaderIDS.p = MALLOC(sizeof (GLuint));
				node->__shaderIDS.p[0] = (void *)myProgram;
			}
		}

		MARK_NODE_COMPILED
	#endif	
}
void compile_PackagedShader (struct X3D_PackagedShader *node) {
	#ifdef GL_VERSION_2_0
		printf ("compileing PackagedShader\n");
		MARK_NODE_COMPILED
	#endif
}
void compile_ProgramShader (struct X3D_ProgramShader *node) {
	#ifdef GL_VERSION_2_0
		printf ("compileing ProgramShader\n");
		MARK_NODE_COMPILED
	#endif
}


/*****************************************************************/
void render_ComposedShader (struct X3D_ComposedShader *node) {
	#ifdef GL_VERSION_2_0
		COMPILE_IF_REQUIRED
		if (node->isValid) {
			if (node->__shaderIDS.n != 0) {
				globalCurrentShader = (GLuint) node->__shaderIDS.p[0];
				glUseProgram(globalCurrentShader);
			}
		}
	#endif
}
void render_PackagedShader (struct X3D_PackagedShader *node) {
	#ifdef GL_VERSION_2_0
	COMPILE_IF_REQUIRED
	
	printf ("rendering PackagedShader\n");
	#endif
}
void render_ProgramShader (struct X3D_ProgramShader *node) {
	#ifdef GL_VERSION_2_0
	COMPILE_IF_REQUIRED
	#endif
}
