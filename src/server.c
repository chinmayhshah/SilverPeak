
/*******************************************************************************
Author :Chinmay Shah 
File :server.c
Last Edit : 6/14

Server implementation for Client and Server application (SilverPeak Test assignment)
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <pthread.h> // For threading , require change in Makefile -lpthread
#include <semaphore.h> // For using semaphore
#include <string.h>
#include <netdb.h> //hostent
#include <arpa/inet.h>
#include <time.h>



#define DEBUGLEVEL

#ifdef DEBUGLEVEL
	#define DEBUG 1
#else
	#define	DEBUG 0
#endif

#define DEBUG_PRINT(fmt, args...) \
        do { if (DEBUG) fprintf(stderr, "\n %s:%d:%s(): " fmt, __FILE__, \
                                __LINE__, __FUNCTION__, ##args); } while (0)

//size restriction 
#define MAXBUFSIZE 60000  
#define SERV_PORT 8000
#define MAXPORTSIZE 6
#define MAX_COL_SIZE 100

//Worker Threads
#define MAX_WORKER_THREADS 5        

#define NSEC_PER_SEC (1000000000)
#define DELAY_TICKS (1)
#define ERROR (-1)
#define OK (0)



//Time set for Time out 
struct timeval timeout={0,0};     
                   


//sockets and connection parameters
#define LISTENQ 1000 

#define MAX_IP_ADDRESS 100
#define MAX_SITE_SIZE 1000
#define TEST_COUNT 10


#define MAX_HANDLE_IDS 99999

typedef enum _ErrorCodes{
		STATUS_OK,
		STATUS_ERROR,
		STATUS_ERROR_FILE_NOT_FOUND,
		STATUS_ERROR_SOCKET_NOT_WRITTEN,
		STATUS_ERROR_REQUEST,
		STATUS_ERROR_GETHOSTBYNAME,
		STATUS_ERROR_TIME
		
}ErrorCodes;

struct siteContents{
	char site_name[MAX_IP_ADDRESS];
	struct timespec max_time;
	struct timespec min_time;
	struct timespec total_time;
	struct timespec avg_time;
	char status[10];

};

// For input command split 
typedef enum COMMANDLOCATION{
		CommandExtra,//Extra Character
		command_location,//Command type
		handle_id_location,//Handle info
}COMMAND_LC;//Command format








// To find Delta value between start and stop time 
int delta_t(struct timespec *stop, struct timespec *start, struct timespec *delta_t)
{
  int dt_sec=stop->tv_sec - start->tv_sec;
  int dt_nsec=stop->tv_nsec - start->tv_nsec;

  if(dt_sec >= 0)
  {
    if(dt_nsec >= 0)
    {
      delta_t->tv_sec=dt_sec;
      delta_t->tv_nsec=dt_nsec;
    }
    else
    {
      delta_t->tv_sec=dt_sec-1;
      delta_t->tv_nsec=NSEC_PER_SEC+dt_nsec;
    }
  }
  else//why no change in assignment
  {
    if(dt_nsec >= 0)
    {
      delta_t->tv_sec=dt_sec;
      delta_t->tv_nsec=dt_nsec;
    }
    else
    {
      delta_t->tv_sec=dt_sec-1;
      delta_t->tv_nsec=NSEC_PER_SEC+dt_nsec;
    }
  }

  return(OK);
}



/*************************************************************
//Split string on basis of delimiter 
//Assumtion is string is ended by a null character
I/p : splitip - Input string to be parsed 
	  delimiter - delimiter used for parsing 
o/p : splitop - Parsed 2 D array of strings
	  return number of strings parsed 

Referred as previous code limits number of strings parsed 	  
http://stackoverflow.com/questions/20174965/split-a-string-and-store-into-an-array-of-strings
**************************************************************/
int splitString(char *splitip,char *delimiter,char (*splitop)[MAX_COL_SIZE],int maxattr)
{
	int sizeofip=1,i=1;
	char *p=NULL;//token
	char *temp_str = NULL;


	DEBUG_PRINT("value split %d",sizeofip);
	
	if(splitip=="" || delimiter==""){
		DEBUG_PRINT("Error\n");
		return -1;//return -1 on error 
	}
	
	
	p=strtok(splitip,delimiter);//first token string 
	
	//Check other token
	while(p!=NULL && *p!='\n' && sizeofip<maxattr && *p!='\0')
	{
		
		
		temp_str = realloc(*splitop,sizeof(char *)*(sizeofip +1));
		
		if(temp_str == NULL){//if reallocation failed	

			
			//as previous failed , need to free already allocated 
			if(*splitop !=NULL ){
				for (i=0;i<sizeofip;i++)
					free(splitop[i]);
				free(*splitop);	
			}

			return -1;//return -1 on error 
		}
		
		
		//Token Used
		strcat(p,"\0");
		// Set the split o/p pointer
		//allocate size of each string 
		//copy the token tp each string
		memset(splitop[sizeofip],0,sizeof(splitop[sizeofip]));
		strncpy(splitop[sizeofip],p,strlen(p));
		strcat(splitop[sizeofip],"\0");
		DEBUG_PRINT	("%d : %s",sizeofip,splitop[sizeofip]);
		sizeofip++;

		//get next token 
		p=strtok(NULL,delimiter);
		
	}
	//if (sizeofip<maxattr || sizeofip>maxattr){
	if (sizeofip>maxattr+1){
		DEBUG_PRINT("unsuccessful split %d %d",sizeofip,maxattr);
		return -1;
	}	
	else
	{	
		//DEBUG_PRINT("successful split %d %d",sizeofip,maxattr);
		return sizeofip;//Done split and return successful }
	}			
	return sizeofip;	

	
}




struct timespec connectSite(char host_address[]){
	//Connect to host_address
	int site_socket_desc=-1; 
	struct timespec start_connectionTime= {0,0};
	struct timespec stop_connectionTime={0,0};
	struct timespec dt_connectionTime={0,0};


	struct sockaddr_in siteSocket;   
	siteSocket.sin_addr.s_addr = inet_addr(host_address);
    siteSocket.sin_family = AF_INET;
    siteSocket.sin_port = htons( 80 );
    


	clock_gettime(CLOCK_REALTIME, &start_connectionTime);
	if ((site_socket_desc= socket(AF_INET , SOCK_STREAM , 0))<0){
	    DEBUG_PRINT("Issue in Creating Socket,Try Again !! %d\n",site_socket_desc);
	    perror("Socket --> Exit ");			        
		return ;
	}
    if(connect(site_socket_desc,(struct sockaddr *)&siteSocket,sizeof(siteSocket))<0){
        DEBUG_PRINT("connect error");
        return ;
    }
    clock_gettime(CLOCK_REALTIME, &stop_connectionTime);
 	close(site_socket_desc);
 	delta_t(&stop_connectionTime, &start_connectionTime, &dt_connectionTime);     
 	return dt_connectionTime;

}

int sendDataToClient (char sendMessage[],int client_sock)
{
	//sprintf(sendMessage,"%s~%s~%s~%s~%s~%s~%s~",sendData.DFCRequestUser,sendData.DFCRequestPass,sendData.DFCRequestCommand,sendData.DFCRequestFile,sendData.DFCData,sendData.DFCRequestFile2,sendData.DFCData2);
	DEBUG_PRINT("Message to Server =>%s",sendMessage);
	write(client_sock,sendMessage,strlen(sendMessage));		
}

//http://www.rowleydownload.co.uk/avr/documentation/index.htm?http://www.rowleydownload.co.uk/avr/documentation/ctl_message_queues.htm
void * workerThreadImplementation(){

	char *hostname = "www.colorado.edu";//sample input
	//Convert the hostname to hostIPaddress 
    char host_address[MAX_IP_ADDRESS];
    struct hostent *hostnet;
    struct in_addr **addresslist;
    int i;
    struct siteContents dequeue_site;
    struct timespec redt_connectionTime;

    DEBUG_PRINT("Worker Thread implementation");      
    //Dequeue the Host name here      
    if ((hostnet = gethostbyname(hostname)) == NULL) 
    {        
        perror("Socket --> Exit ");	
        return;
    }
    
    addresslist = (struct in_addr **) hostnet->h_addr_list;
     
    for(i = 0; addresslist[i] != NULL; i++) 
    {
        strcpy(host_address,inet_ntoa(*addresslist[i]));
    }
    //Conversion complete
	
	dequeue_site.min_time.tv_nsec=9999999999999;
	

	for(i=0;i<10;i++){
		
		redt_connectionTime=connectSite(host_address);
		DEBUG_PRINT("Time(ns) = %ld ",redt_connectionTime.tv_nsec);          
		if(redt_connectionTime.tv_nsec > dequeue_site.max_time.tv_nsec ){
			dequeue_site.max_time.tv_nsec = redt_connectionTime.tv_nsec;
		}
		if(redt_connectionTime.tv_nsec <= dequeue_site.min_time.tv_nsec){
			dequeue_site.min_time.tv_nsec = redt_connectionTime.tv_nsec;
		}
		dequeue_site.total_time.tv_nsec += redt_connectionTime.tv_nsec;

	}	
	dequeue_site.avg_time.tv_nsec = dequeue_site.total_time.tv_nsec/10;
	//Printing seconds 
	DEBUG_PRINT("\n MAX Time(ns) = %ld\n \
		         MIN Time(ns) = %ld\n \
		         TOTAL Time(ns) = %ld\n \
		         AVG Time(ns) = %ld\n \
		         ",dequeue_site.max_time.tv_nsec 
		          ,dequeue_site.min_time.tv_nsec
		          ,dequeue_site.total_time.tv_nsec
		          ,dequeue_site.avg_time.tv_nsec
		         );

}




void pingSitesCommand(char site_list[]){
	//enqueue the sites to message queue 
	DEBUG_PRINT("site List%s",site_list);


}

/*
A handle generator ,limit of handle ids(MAX_HANDLE_IDS) 
*/
int handle_generation(){
	static int handle_count=0;
	if(handle_count > MAX_HANDLE_IDS){
		handle_count=0;	
	}
	return handle_count++;
}


int commandAnalysis(char inputCommand[]){

		int total_attr_commands=0;
		char (*action)[MAX_COL_SIZE];

		char message_bkp[MAXBUFSIZE];//store message from client 

		if ((action=calloc(MAX_COL_SIZE,sizeof(action)))){	
			DEBUG_PRINT("Malloc allocated");
			
			if((total_attr_commands=splitString(inputCommand," ",action,4))<0)
			{
				DEBUG_PRINT("Error in Split \n\r");			
			}
			DEBUG_PRINT("%d",total_attr_commands);
			DEBUG_PRINT("Command Type %s =>%s ",action[1],action[2]);
			if ((strncmp(action[command_location],"pingSites",strlen("pingSites"))==0)){
					
					DEBUG_PRINT("Inside pingSites");
					pingSitesCommand(action[handle_id_location]);
			}
			else if ((strncmp(action[command_location],"showHandles",strlen("showHandles")))==0){
					DEBUG_PRINT("Inside showHandles");
	  		}
			else if ((strncmp(action[command_location],"showHandleStatus",strlen("showHandleStatus")))==0){
					DEBUG_PRINT("Inside showHandleStatus");	
			}
			else if ((strncmp(action[command_location],"exit",strlen("exit")))==0){			
	  			
			}
	  		else
	  		{	
	  			return -1;
	  		}
		}
		else{
			DEBUG_PRINT("not allocated");
		}

		if (action!=NULL){

			DEBUG_PRINT("De alloaction");
			free(action);//clear  the request recieved 
		}

}


//To correct handling of client connections 
void *client_connections(void *client_sock_id){
	
	int i=0,total_attr_commands=0;
	//int thread_sock = (int*)(client_sock_id);	
	int thread_sock = (intptr_t)(client_sock_id);	
	ssize_t read_bytes=0;
	char message_client[MAXBUFSIZE];//store message from client 
	
	//Wait for command from Client 
	//Recieve the message from client  and reurn back to client 
	//while(exitFlag){
		if((read_bytes =recv(thread_sock,message_client,MAXBUFSIZE,0))>0){
			DEBUG_PRINT("%s Message length%d\n",message_client,(int)strlen(message_client) );
			commandAnalysis(message_client);
			
			sendDataToClient("Command received",thread_sock);
		}
	//}		
}





int main (int argc, char * argv[] ){

	int i=0;
	pthread_t client_thread;
	char listen_port[MAXPORTSIZE];
	int *mult_sock=NULL;//to alloacte the client socket descriptor
	pthread_t worker_thread[MAX_WORKER_THREADS];
	int worker_thread_count=0;

	int server_sock,client_sock;        //This will be our socket
	struct sockaddr_in server, client; //"Internet socket address structure"
	unsigned int remote_length;         //length of the sockaddr_in structure
	int nbytes;                        //number of bytes we receive in our message


	if (argc < 2)
	{
		printf("USAGE:  <Server_IP> <Server_port>\n");
		exit(1);
	}
	else
	{
		printf("USAGE IP %s :PORT %s \n",argv[1],argv[2]);		
	}

	//Set up the server
	/******************
	  This code populates the sockaddr_in struct with
	  the information about our socket
	 ******************/
	bzero(&server,sizeof(server));                    //zero the struct
	server.sin_family = AF_INET;                   //address family
	strcpy(listen_port,argv[2]);
	
	//Check if Port is present or not (additional Check as ports under 1024 are reservered in most cases )
	if (strcmp(listen_port,"")){
		DEBUG_PRINT("Port %s",listen_port);
		if(atoi(listen_port) <=1024){
			printf("\nPort Number less than 1024!! Check input Port\n");
			exit(-1);
		}
		else
		{
			server.sin_port = htons(atoi(listen_port));        		//htons() sets the port # to network byte order
		}	
		
	}
	else
	{	
		printf("\nPort Number not found !! Try again \n");
		exit(-1);
	}	

	server.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine
	remote_length = sizeof(struct sockaddr_in);    //size of client packet 
	
	//Causes the system to create a generic socket of type TCP (strean)
	if ((server_sock =socket(AF_INET,SOCK_STREAM,0)) < 0){
		DEBUG_PRINT("unable to create tcp socket");
		exit(-1);
	}

	/******************
	  Bind socket created to the local address 
	  and port we've supplied in the sockaddr_in struct
	 ******************/
	if (bind(server_sock, (struct sockaddr *)&server, sizeof(server)) < 0){
		close(server_sock);
		printf("unable to bind socket\n");
		perror("Bind Socket");
		exit(-1);
	}
	//
	if (listen(server_sock,LISTENQ)<0)
	{
		close(server_sock);
		perror("LISTEN");
		exit(-1);
	}

	
	//Spawn MAX_WORKER_THREADS
	for(i=0;i<MAX_WORKER_THREADS;i++){
			//Create the pthread 
			//if ((pthread_create(&client_thread,NULL,client_connections,(void *)(intptr_t)(client_sock)))<0){
			if ((pthread_create(&worker_thread[i],NULL,workerThreadImplementation,(void *)(intptr_t)i))<0){
				perror("Worker Thread not created");					
			}	
			else
			{
				worker_thread_count++;
			}
	}

	DEBUG_PRINT("Server is running wait for connections");
	while(1){
		//Accept incoming connections and spawing pthreads for each client 	 
		client_sock=0;
		while((client_sock = accept(server_sock,(struct sockaddr *) &client, (socklen_t *)&remote_length))){
			if(client_sock<0){	
				perror("accept  request failed");
				exit(-1);
				close(server_sock);
			}
			//DEBUG_PRINT("connection accepted  %d \n",(int)client_sock);	
			//mult_sock = (int *)malloc(1);
			/*if (mult_sock== NULL)//allocate a space of 1 
			{
				perror("Malloc mult_sock unsuccessful");
				close(server_sock);
				exit(-1);
			}*/
			DEBUG_PRINT("Malloc successfully\n");
			//bzero(mult_sock,sizeof(mult_sock));
			//*mult_sock = client_sock;

			//DEBUG_PRINT("connection accepted  %d \n",*mult_sock);	
			
			//Create the pthread 
			if ((pthread_create(&client_thread,NULL,client_connections,(void *)(intptr_t)(client_sock)))<0){
				close(server_sock);
				perror("Thread not created");
				exit(-1);

			}			
			//Free of Client Socket 
			free(mult_sock);
			DEBUG_PRINT("Free of Client Socket");

		}	
	}
	
	for(i=0;i<worker_thread_count;i++){
		if(pthread_join(worker_thread[i], NULL) == 0)
				 printf("Worker Thread done\n");
				else
				 perror("Worker Thread Join");
	}
				 	
	if (client_sock < 0)
	{
		perror("Accept Failure");
		close(server_sock);
		exit(-1);
	}
	//Close 
	close(server_sock);
	
}
