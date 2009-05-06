.SUFFIXES: .o .c .cpp

.cpp.o:
	$(CXX) -c -fPIC $(INCLUDE) $(DEFLIST) -o $@ $<
	
.c.o:
	$(CC) -c -fPIC $(INCLUDE) $(DEFLIST) -o $@ $<
	
CXX	= g++
CC	= gcc
DEBUG = -g
DEFLIST = -DUNIX -DLINUX -DMULTITHREAD -D_REENTRANT
INCLUDE = -I/usr/local/include -I../ipp_base -I/opt/intel/ipp/5.3.4.080/ia32/include
LIB     = -L/usr/local/lib -L/opt/intel/ipp/5.3.4.080/ia32/sharedlib
LIBS	= -lippcore -lguide -lipps -lippsc -lpthread -ldl -lnsl
LINKS	= -shared


PROJECT	= ../../_bin/libg729.so
OBJECTS = ipp_g729.o decg729.o encg729.o owng729.o uscg729i.o vadg729.o ../ipp_base/aux_tbls.o

$(PROJECT): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LINKS) $(LIB) $(LIBS) -o $@

%.o: %.c
	$(CC) -c -fPIC $(DEBUG) $(INCLUDE) $(DEFLIST) -o $@ $<
	
ipp_g729.o: ipp_g729.cpp
	$(CXX) -c -fPIC $(DEBUG) $(INCLUDE) $(DEFLIST) -o $@ $<
	
clean:
	rm -f $(PROJECT) *.o


