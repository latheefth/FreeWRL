PROGRAMS=cylinderdone lightnign_bolt_test  polyrep_sphere_disp  triangle_disp_sphere conedone elevgrid querty_text_disp cubedone6_final  generaldisp linedone triangle_disp 
HEADERS=LinearAlgebra.h Structs.h Collision.h
LIBOBJS=LinearAlgebra.o
LINKS=LinearAlgebra.h LinearAlgebra.cpp Collision.cpp Collision.h
LINKSPATH=../../../CFuncs/

# Compilation specs
CC=g++
CFLAGS= -g
COMPILER=g++ 
LIBS= -lglut -lGL -lGLU -lm 

%.o: %.cpp $(HEADERS) 
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

all: $(LINKS) $(PROGRAMS)
	@echo ==============================================
	@echo Successfully built: $(PROGRAMS)  

$(PROGRAMS) : % : %.o $(LIBOBJS)
	$(CC) -o $@ $< $(LIBOBJS) $(LIBS)


#Setup symbolic links
LinearAlgebra.h :
	@if [[ ! -a LinearAlgebra.h   ]]; then ln -s $(LINKSPATH)LinearAlgebra.h . ; fi; 

Collision.h :
	@if [[ ! -a Collision.h       ]]; then ln -s $(LINKSPATH)Collision.h .     ; fi; 

LinearAlgebra.cpp :
	@if [[ ! -a LinearAlgebra.cpp ]]; then ln -s $(LINKSPATH)LinearAlgebra.c LinearAlgebra.cpp ; fi; 

Collision.cpp :
	@if [[ ! -a Collision.cpp     ]]; then ln -s $(LINKSPATH)Collision.c Collision.cpp         ; fi; 

clean:
	rm -f $(PROGRAMS) *.o *.cpp~ $(LINKS) 
