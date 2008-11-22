MAKE    = make 
DOXYGEN = d:/programme/doxygen/bin/doxygen.exe

# Makefiles containing rules for librarys
PROJECT_LIBS = tools

# Makefiles containing rules for executables
PROJECT_EXES = server

.PHONY: help build build_lib build_exe clean test all doku $(PROJECT_LIBS) $(PROJECT_EXES)

build :
	$(MAKE) build_lib && $(MAKE) build_exe

build_lib : $(PROJECT_LIBS)

build_exe : $(PROJECT_EXES)

clean : $(PROJECT_LIBS) $(PROJECT_EXES)

test : $(PROJECT_LIBS) $(PROJECT_EXES)

all : clean build 

doku :
	$(DOXYGEN) 

help :
	echo build build_lib build_exe clean test all doku
    
# rules for a project
$(PROJECT_LIBS) $(PROJECT_EXES) :
	$(MAKE) -C $@ $(MAKECMDGOALS)


