#Makefile for comipling  file
#@chinmay.shah@colorado.edu
#reference: 1)http://mrbook.org/blog/tutorials/make/
#			2)http://mercury.pr.erau.edu/~siewerts/cec450/code/example-sync/

# the compiler to be used	
#
CC=gcc 

#flags used during compilation
CFLAGS= -std=c++11 -L/usr/include -I/usr/include/boost 
LDFLAGS= -lboost_filesystem -lboost_system -lboost_thread 

PRODUCT=ssfi

BINARY =bin
OBJECTLOC =obj
SOURCELOC =src


all: $(PRODUCT)

clean:
	-rm -f $(OBJECTLOC)/*.o *.NEW *~ *.d
	-rm -f $(BINARY)/$(PRODUCT) 


OBJECTS = $(OBJECTLOC)/listtopWords.o \
		  $(OBJECTLOC)/SearchManager.o \
		  $(OBJECTLOC)/workerManager.o \
		  $(OBJECTLOC)/SSFI.o
	  
$(PRODUCT):$(OBJECTS)
		$(CC) $(CFLAGS) $(OBJECTS) $(LDFLAGS) -o $(BINARY)/$(PRODUCT)

$(OBJECTLOC)/listtopWords.o: $(SOURCELOC)/listtopWords.cpp
		$(CC) $(CFLAGS) -c $^ $(LDFLAGS) -o $@

$(OBJECTLOC)/SearchManager.o: $(SOURCELOC)/SearchManager.cpp
		$(CC) $(CFLAGS) -c $^ $(LDFLAGS) -o $@		

$(OBJECTLOC)/workerManager.o: $(SOURCELOC)/workerManager.cpp
		$(CC) $(CFLAGS) -c $^ $(LDFLAGS) -o $@		

$(OBJECTLOC)/SSFI.o: $(SOURCELOC)/SSFI.cpp
		$(CC) $(CFLAGS) -c $^ $(LDFLAGS) -o $@				