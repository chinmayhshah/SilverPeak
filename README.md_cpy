Super Simple File Indexer 
-------------------------

 A super Simple File Indexer implemented using multi-threaded text file indexing application 
 printing ten most repeated words in descending order with their counts in the following format

<word>	<count>

For example :

Top 10 Words of Directory are 
 
Word1 549
Word2 294
Word3 193
..........
..........


BUILD
-----

Navigate to "SolidFire/SolidFire/" folder
Run "make" the executable "ssfi" will be created in 'bin' folder


USAGE 
-----
./bin/<Executable> -t <Threads>  < directoryPath>

For example
$ ./bin/ssfi ~/Desktop/SolidFire/SolidFire/

By default, the application creates ten threads and crawls the directory path(provided in command line) to search for ".txt" files 

To specify custom threads, use "-t" option, for example

For example
$ ./ssfi -t 10 ~/Desktop/SolidFire/testDir/


Structure of directory 
---------------------
	SolidFire
		--SolidFire
			--bin — Location of executable file
			--src — Source files written
			--obj — object files for each source files
			--include — Headers files written
			--doc — Documentation of requirements ..etc
			Makefile — to build the application
			ReadMe.md — Read Me documentation for general guidelines and information
	
		--<TestFiles and Directories>		
		
Configuration for default values
----------------------------------

Some of the important default parameters set SSFIdefaulttypes.hpp
(i) DEFAULT_FILE_INDEXER ".txt" - can be used to set another type of file 
(ii) DEFAULT_WORKER_THREADS - Default threads can be changed 


Debugging 
---------

Enable Debugging - Uncomment //#define DEBUGLEVEL
or 
GDB command example

$ gdb --args ./bin/ssfi ../../SolidFire/largeFiledir/

Assumptions
----------- 

1) A default set of worker threads spawned if custom worker threads aren't inputted by user
2) Scenarios where Non-Alphanumeric elements acting as delimiter would result in non-English words
	For example, I'm are converted to I m and considered as different words.
3) Maximum performance can be obtained by running on multi-core instance allowing threads to run on different cores.	
4) Default directory isn't available and need to specify as input
4) Only one Flag -t supported and path supported without a flag 


Implementation 
--------------

WorkerManager.hpp - Manages methods and tasks of Worker thread
SearchManager.hpp - Manages methods and tasks of Search thread
SynchronizedQueue.hpp - Manages methods to provide synchronization between Search thread and Worker threads
SSFI.hpp - Spawns Threads and Deliver results for Top 10 (defalut) Words in directory
listtopWords.cpp - Driver of implementation 

Each class provides its implementation in detail . 


Testing
-------


All commands executed from SolidFire/SolidFire

1) For a large Single File
$ ./bin/ssfi -t 100 ../../SolidFire/largeFiledir/

2) For a small Single File
$ ./bin/ssfi -t 10 ../../SolidFire/lessThanTen/

3) For blank or no text file directory 
$ ./bin/ssfi -t 10 ../../SolidFire/blankDir/

4) For multiple Files and directories
$ ./bin/ssfi -t 10 ../../SolidFire/

Error Scenarios

$ ./bin/ssfi -t 100 ../../SolidFire/testFile.txt
$ ./bin/ssfi -t 10 ../../SolidFire/SolidFi
$ ./bin/ssfi -n 100 ../../SolidFire/SolidFire/ 
$ ./bin/ssfi 


AUTHOR
------
Chinmay Shah (chinmay.shah@colorado.edu)