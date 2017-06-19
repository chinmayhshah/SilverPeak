
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
#include <string.h>
#include <netdb.h> //hostent
#include <arpa/inet.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdbool.h>



//#define DEBUGLEVEL

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
#define MAXPORTSIZE 6
#define MAX_COL_SIZE 200

//Worker Threads
#define MAX_WORKER_THREADS 5       

#define NSEC_PER_SEC (1000000000)
#define MSEC_PER_SEC (1000000)
#define DELAY_TICKS (1)
#define ERROR (-1)
#define OK (0)



//Time set for Time out 
struct timeval timeout={0,0};     
                   
//sockets and connection parameters
#define LISTENQ 1000 

#define MAX_IP_ADDRESS 500
#define MAX_SITE_SIZE 1000
#define MIN_SITE_SIZE 5
#define MAX_TOTAL_SITES 10 

#define MAX_HANDLE_IDS 9999 //can be limited to size of the 

typedef enum _ErrorCodes{
		STATUS_OK,
		STATUS_ERROR,
		STATUS_ERROR_FILE_NOT_FOUND,
		STATUS_ERROR_SOCKET_NOT_WRITTEN,
		STATUS_ERROR_REQUEST,
		STATUS_ERROR_GETHOSTBYNAME,
		STATUS_ERROR_TIME
		
}ErrorCodes;


//Handle Status
enum HANDLE_STATUS{		
		IN_QUEUE,
		IN_PROGRESS,
		COMPLETE,
		IN_ERROR
};//different handle status


char handle_status[4][20]={"IN_QUEUE","IN_PROGRESS","COMPLETE","IN_ERROR"};

struct site_content{
	char site_name[MAX_IP_ADDRESS];
	int handle_id;
	struct timespec max_time;
	struct timespec min_time;
	struct timespec total_time;
	struct timespec avg_time;
	int status;
	int site_no;

};

// For input command split 
typedef enum COMMANDLOCATION{
		CommandExtra,//Extra Character
		command_location,//Command type
		handle_id_location,//Handle info
}COMMAND_LC;//Command format



//Message queue implementation with mutex and conditional variables 

struct sites_queue{
	long mtype;
	struct site_content q_site_content;
	
};





enum handle_commands {SHOW_HANDLE,INC_HANDLE,DEC_HANDLE};


/***********************************************************************
@brief
A handle generator ,limit of handle ids(MAX_HANDLE_IDS) 
Input 
	command_type - INC_HANDLE - Increment and return a handle id 
Output
	 A unique new handle id or last handle id in system
***********************************************************************/
int handleGeneration(int command_type){
	static int handle_count=0;
	if(command_type==INC_HANDLE){
		if(++handle_count > MAX_HANDLE_IDS){
			handle_count=1;	
			return handle_count;
		}
		return handle_count;
	}
	else if (command_type==SHOW_HANDLE){
		DEBUG_PRINT("Handle ID value %d",handle_count);
		return handle_count;
	}	
}




pthread_mutex_t shared_array_mutex;
struct content{
	int site_no;
	struct site_content site_data[MAX_TOTAL_SITES+1];
};

struct content shared_array[MAX_HANDLE_IDS+1];//an array used for easy searching of handle ids 

/***********************************************************************
@brief
Add Site contents to the array (shared_array).

Input - new site data  of type  struct site_content
		
Output - add a new site at the location of  handle_id
       - update the number of sites hold by the handle id 
       
       * protected by shared_array_mutex for access of shared_array.
       * It acts more of hash map
***********************************************************************/

int add_site(struct site_content *new_site_results){
	
	static const struct site_content EmptyStruct;
	
	//search for same handle ids initially and clear them 
	pthread_mutex_lock(&shared_array_mutex);	
		int handle_id,site_no;
		handle_id=new_site_results->handle_id;	
		site_no=shared_array[handle_id].site_no;
		if(site_no>MAX_TOTAL_SITES){//overflow of total site numbers 
			site_no=0;
		}
		shared_array[handle_id].site_data[site_no]= EmptyStruct;
		shared_array[handle_id].site_data[site_no]=*(new_site_results);
		shared_array[handle_id].site_data[site_no].site_no=site_no;
			
		shared_array[handle_id].site_no=site_no+1;
	pthread_mutex_unlock(&shared_array_mutex);	
	DEBUG_PRINT("\n \
		     Site Name  = %s\n  \
	         Status  = %d \n \
	         site_no = %d \n \
	         site_no(queue element) = %d \n \
	          ",shared_array[handle_id].site_data[site_no].site_name
	          ,shared_array[handle_id].site_data[site_no].status
	          ,shared_array[handle_id].site_no       
	          ,shared_array[handle_id].site_data[site_no].site_no
	         );
	
	DEBUG_PRINT("\nShared data  %d handle %d ",shared_array[handle_id].site_data[site_no].site_no,handle_id);
	 return site_no+1;         

}
/***********************************************************************
@brief
Update Site contents to the array (shared_array).

Input -New status or data of type  struct site_content
		
Output 
       -Update site  status or data of a struct site_content
 		using handle id and site_no
       
       * protected by shared_array_mutex for access of shared_array
***********************************************************************/

void update_site(struct site_content *new_site_results){
	
	static const struct site_content EmptyStruct;
	int handle_id,site_no,site_locn=0,i=0;
	//search for same handle ids initially and clear them 
	
	pthread_mutex_lock(&shared_array_mutex);	
		handle_id=new_site_results->handle_id;	

		site_no=shared_array[handle_id].site_no-1;/// total sites stored in mapping to handle_id
		for(i=0;i<=site_no;i++){
			if(strcmp(shared_array[handle_id].site_data[i].site_name,new_site_results->site_name)==0){
				shared_array[handle_id].site_data[i]=*(new_site_results);
			}	
		}	
	pthread_mutex_unlock(&shared_array_mutex);	

}



/***********************************************************************
@brief
Implements the display of all the handle ids  or handle id with required format 

Input - handle_id
		0 - No handle Id 
		<number> - A particular handle id 
Output - a character array of information in following format for handle id or all handle ids
		s_no|h_id|site name|avg|min|max|status
***********************************************************************/

char * showHandleStatus(int handle_id){
	int i=0,h;
	char conversion[MAX_COL_SIZE];
	char *message_string=(char *)calloc(MAXBUFSIZE,sizeof(char));
	int s_no=0;
	int end = handleGeneration(SHOW_HANDLE);
	if (end==0 || handle_id > end){//for no handle id in system
			strncpy(message_string,"No Handle IDs Found",strlen("No Handle IDs Found"));	
	}else{
	 	if(handle_id==0){//no handle input 				
		strcat(message_string,"\n-------pingSites(Time(ms))-------- \n");			
			strcat(message_string,"\ns_no|h_id|site name|avg|min|max|status\n");			
			s_no=0;
			for(h=0;h<=end;h++){//for alll handle ids in system
				for(i=0;i<shared_array[h].site_no;i++){
					s_no++;
					sprintf(conversion,"%d. %d %s %d %d %d %s \n",
							s_no,
							h,
							shared_array[h].site_data[i].site_name,
				          	(int)(shared_array[h].site_data[i].avg_time.tv_nsec/1000000), 
				          	(int)(shared_array[h].site_data[i].min_time.tv_nsec/1000000),	      
				          	(int)(shared_array[h].site_data[i].max_time.tv_nsec/1000000),
				        	handle_status[shared_array[h].site_data[i].status]);	
					strcat(message_string,conversion);
				}
			}	
			strcat(message_string,"\0");
			
		}		
		else{
			DEBUG_PRINT("Handle Id to be searched %d %d",handle_id,shared_array[handle_id].site_no);		
				for(i=0;i<shared_array[handle_id].site_no;i++){			
					sprintf(conversion,"%d. %d %s %d %d %d %s \n",
								i+1,
								handle_id,
								shared_array[handle_id].site_data[i].site_name,
					          	(int)(shared_array[handle_id].site_data[i].avg_time.tv_nsec/1000000), 
					          	(int)(shared_array[handle_id].site_data[i].min_time.tv_nsec/1000000),	      
					          	(int)(shared_array[handle_id].site_data[i].max_time.tv_nsec/1000000),
					        	handle_status[shared_array[handle_id].site_data[i].status]);	
						strcat(message_string,conversion);
					}
					
				strcat(message_string,"\0");
		
		}
	}	

	DEBUG_PRINT("showHandles Message %s ",message_string);
	return message_string;
}

/***********************************************************************
@brief
Implements the display of all the handle ids for "showHandles commands
(return all the unique handle ids on the server)
Input - None
Output - a character array of handle ids 
***********************************************************************/

char *showHandles(){
	int i=0;
	char conversion[MAX_COL_SIZE];
	char *message_string=(char *)calloc(MAXBUFSIZE,sizeof(char));
	int end = handleGeneration(SHOW_HANDLE);
	if (end==0){
		strncpy(message_string,"No Handle IDs Found",strlen("No Handle IDs Found"));	

	}else{

		for(i=1;i<=end;i++){
			sprintf(conversion,"%d \n",i);
			strcat(message_string,conversion);
		}
		strcat(message_string,"\0");
		
	}	
	DEBUG_PRINT("showHandles Message %s ",message_string);
	return message_string;

}

/*************************************************************
@brief
Calulate the difference between two time stamps
Input 
1) start and stop timings 
Ouput 
1) delta or difference of timings in nsec

**************************************************************/

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
@brief
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
		//allocate size of each string and copy the token tp each string
		memset(splitop[sizeofip],0,sizeof(splitop[sizeofip]));
		strncpy(splitop[sizeofip],p,strlen(p));
		strcat(splitop[sizeofip],"\0");
		DEBUG_PRINT	("%d : %s",sizeofip,splitop[sizeofip]);
		sizeofip++;

		//get next token 
		p=strtok(NULL,delimiter);
		
	}
	if (sizeofip>maxattr+1){
		DEBUG_PRINT("unsuccessful split %d %d",sizeofip,maxattr);
		return -1;
	}	
	else
	{	
		return sizeofip;//Done split and return successful }
	}			
	return sizeofip;	

	
}

/*

/*************************************************************
@brief
Calculate timing parameters of the site to be tested 
Input -
		host_address- the address of the site to be tested 
Ouput 
	- return of timing parameters of type "struct timespec"

**********************************************************/
struct timespec connectSite(char host_address[],char *name){
	//Connect to host_address
	int site_socket_desc=-1; 
	struct timespec start_connectionTime= {0,0};
	struct timespec stop_connectionTime={0,0};
	struct timespec dt_connectionTime={0,0};

	//set up the socket for paramters 
	struct sockaddr_in siteSocket;   
	siteSocket.sin_addr.s_addr = inet_addr(host_address);
    siteSocket.sin_family = AF_INET;
    siteSocket.sin_port = htons( 80 );
    
    static const struct timespec EmptyStruct;

	clock_gettime(CLOCK_REALTIME, &start_connectionTime);//note start time 
	if ((site_socket_desc= socket(AF_INET , SOCK_STREAM , 0))<0){
	    DEBUG_PRINT("Issue in Creating Socket,Try Again !! %d\n",site_socket_desc);
	    perror("Socket --> Exit ");			        
		return EmptyStruct;
	}
    if(connect(site_socket_desc,(struct sockaddr *)&siteSocket,sizeof(siteSocket))<0){
    	perror(host_address);
        DEBUG_PRINT("connect error %s",name);
        return EmptyStruct;
    }
    clock_gettime(CLOCK_REALTIME, &stop_connectionTime);//note stop time 
 	close(site_socket_desc);
 	delta_t(&stop_connectionTime, &start_connectionTime, &dt_connectionTime);     
 	return dt_connectionTime;//return difference of time 

}


/*************************************************************
@brief
Send data to Client on socket
Input -
		sendMessage- Message to be send 
		client_sock - Socket for communication 
Ouput 
	- return of suceess 

**********************************************************/

int sendDataToClient (char sendMessage[],int client_sock)
{
	DEBUG_PRINT("Message to Client =>%s",sendMessage);
	write(client_sock,sendMessage,strlen(sendMessage));		
	return 1;
}


//Mutex for protection of message queue
pthread_mutex_t queue_mutex;
pthread_mutex_t dequeue_mutex;

/*************************************************************
@brief
Implementatio of Worker thread 
Input -  Dequeue site_content from message queue

Output -Performs the timing calculations by creating a socket between 

Initail Ref: http://www.rowleydownload.co.uk/avr/documentation/index.htm?http://www.rowleydownload.co.uk/avr/documentation/ctl_message_queues.htm
****************************************************************/

void * workerThreadImplementation(){
	//Convert the hostname to hostIPaddress 
    char host_address[MAX_IP_ADDRESS];
    struct hostent *hostnet;
    struct in_addr **addresslist;
    int i=0;
    struct sites_queue s_queue;
    
    struct timespec redt_connectionTime;	
    //Message Queue
    int msqid;
    key_t key;
  
    //Prep for Dequeue the Host name here      
	if ((key = ftok("server.c",'B')) <0) {  /* same key as server.c */
		  perror("ftok");
    }

    if ((msqid = msgget(key, 0644)) < 0) { /* connect to the queue */
        perror("msgget");      
    }    

    while(1){// runs continuously 
    	bool skip_ping=true;
    	DEBUG_PRINT("Worker Thread implementation");      
		//Lock Mutex before dequeue from queue
		pthread_mutex_lock(&dequeue_mutex);	
    	
    	if(msgrcv(msqid, &s_queue, sizeof (struct site_content), 0, 0)<0 || !skip_ping) {
            perror("msgrcv");        
            s_queue.q_site_content.status=IN_ERROR;
            skip_ping=false;
        }    
        pthread_mutex_unlock(&dequeue_mutex);

        struct site_content dequeue_site= s_queue.q_site_content;
      	dequeue_site.status = IN_PROGRESS;
		DEBUG_PRINT("Update site to be in IN_PROGRESS %s ",dequeue_site.site_name);				
		update_site(&dequeue_site); //Update in Progress status
		
       
	    if ((hostnet = gethostbyname(dequeue_site.site_name)) == NULL || !skip_ping) 
	    {        
	    	DEBUG_PRINT("Unreachable site \n");
	        perror("hostnet");	
	        skip_ping=false;
	    }		    


	    if(skip_ping){
		    addresslist = (struct in_addr **) hostnet->h_addr_list;
		     
		    for(i = 0; addresslist[i] != NULL; i++) 
		    {
		        strcpy(host_address,inet_ntoa(*addresslist[i]));
		    }
		    //Conversion complete

			//Clearing data before calculations 
			dequeue_site.min_time.tv_sec=0;
			dequeue_site.max_time.tv_sec=0;
			dequeue_site.total_time.tv_sec=0;
			dequeue_site.avg_time.tv_sec=0;

			dequeue_site.min_time.tv_nsec=999999999;// a high max value 
			dequeue_site.max_time.tv_nsec=0;
			dequeue_site.total_time.tv_nsec=0;
			dequeue_site.avg_time.tv_nsec=0;
			
			//Running the connection for ten times and calulating in nano secs	
			for(i=0;i<10;i++){				
				redt_connectionTime=connectSite(host_address,dequeue_site.site_name);
				  
				if(redt_connectionTime.tv_nsec > dequeue_site.max_time.tv_nsec ){
					dequeue_site.max_time.tv_nsec = redt_connectionTime.tv_nsec;
					dequeue_site.max_time.tv_sec = redt_connectionTime.tv_sec;
				}
				if(redt_connectionTime.tv_nsec <= dequeue_site.min_time.tv_nsec){
					dequeue_site.min_time.tv_nsec = redt_connectionTime.tv_nsec;
					dequeue_site.min_time.tv_sec = redt_connectionTime.tv_sec;
				}
				dequeue_site.total_time.tv_nsec += redt_connectionTime.tv_nsec;
				dequeue_site.total_time.tv_sec += redt_connectionTime.tv_sec;

			}	
			dequeue_site.avg_time.tv_nsec = dequeue_site.total_time.tv_nsec/10;
			dequeue_site.avg_time.tv_sec = dequeue_site.total_time.tv_sec/10;
			DEBUG_PRINT("Update site to be in COMPLETE %s handle_id %d",dequeue_site.site_name,dequeue_site.handle_id);		
			dequeue_site.status = COMPLETE;
			
		}else
		{
			DEBUG_PRINT("Update site to be in IN_ERROR %s ",dequeue_site.site_name);		
			DEBUG_PRINT("Could not connect , thus error");		
			dequeue_site.status = IN_ERROR;
		}

		
		update_site(&dequeue_site);// Update status
	}	

}




/*************************************************************
@brief
Implements the "pingSites command" .It splits the multiple sites and enques them on message queue
Input - site_list - list of sites for client 
		- client_handle_id - handle associated with this command


Output - Enqueue site_content from message queue
	  - Also adds each site to a shared memory between threads to access them using handle id 

****************************************************************/
int msgid;
void pingSitesCommand(char site_list[],int client_handle_id ){
	//enqueue the sites to message queue 
	
	int total_attr_commands=0,i=0;
	char site_list_copy[MAX_SITE_SIZE+1];
	static const struct site_content EmptyStruct;
	int site_loc=0;
	struct sites_queue s_queue;

	//clear before back uo
	memset(site_list_copy,0,(MAX_SITE_SIZE+1));
	strncpy(site_list_copy,site_list,strlen(site_list));
	
	//split into different sites
	char (*list_sites)[MAX_COL_SIZE];
	
	if ((list_sites=calloc(MAX_COL_SIZE,sizeof(list_sites)))){	
		DEBUG_PRINT("site List %s \n %s ",site_list_copy,site_list);	
		if((total_attr_commands=splitString(site_list_copy,",",list_sites,MAX_TOTAL_SITES))<0){
			perror("Split");
		}
		
			//add all the sites to the queue 
			for(i=1;i<total_attr_commands;i++){					

				if(strcmp(list_sites[i],"")!=0 && (strlen(list_sites[i])>MIN_SITE_SIZE)){//check if site is present or error				
				
					strcpy(s_queue.q_site_content.site_name,list_sites[i]);			
					DEBUG_PRINT("%s i value %d \n",s_queue.q_site_content.site_name,i );
					s_queue.mtype=1;
					s_queue.q_site_content.handle_id=client_handle_id;
					s_queue.q_site_content.status=IN_QUEUE;	
					site_loc=add_site(&s_queue.q_site_content);
					//s_queue.q_site_content.site_no=site_loc;									
					pthread_mutex_lock(&queue_mutex);			
					
					if (msgsnd(msgid, &s_queue, sizeof(struct site_content), 0) == -1) 
				            perror("msgsnd");
				    pthread_mutex_unlock(&queue_mutex);   		   				    

				}else{
					DEBUG_PRINT("Error in Site");
				}     
			}       
			
	}
	DEBUG_PRINT("Exit pingSites Command handle id %d => %d \n",client_handle_id,i );
	
}


/*************************************************************
@brief
Checks the sanity of each command from client and performs appropriate task
Input - inputCommand - command from  client 

Output -Sanity check for support of command
       -Split input command into command_type and addiional required input 

****************************************************************/
char *commandAnalysis(char inputCommand[]){

		int total_attr_commands=0,handle_id=0;
		char (*action)[MAX_COL_SIZE];

		char message_bkp[MAXBUFSIZE];//store message from client 
		char *reply_string=(char *)malloc(sizeof(char)*MAXBUFSIZE);

		if ((action=calloc(MAX_COL_SIZE,sizeof(action)))){	
			DEBUG_PRINT("Malloc allocated");
			
			if((total_attr_commands=splitString(inputCommand," ",action,4))<0)
			{
				DEBUG_PRINT("Error in Split \n\r");			
			}
			DEBUG_PRINT("%d",total_attr_commands);
			
			if ((strncmp(action[command_location],"pingSites",strlen(action[command_location]))==0)){
					
					DEBUG_PRINT("Inside pingSites");					
					handle_id=handleGeneration(INC_HANDLE);					
					pingSitesCommand(action[handle_id_location],handle_id);
					sprintf(reply_string,"%d",handle_id);
					DEBUG_PRINT("%s",reply_string);						
			}
			else if ((strncmp(action[command_location],"showHandles",strlen(action[command_location])))==0){
					DEBUG_PRINT("Inside showHandles");
					reply_string=showHandles();
	  		}
			else if ((strncmp(action[command_location],"showHandleStatus",strlen(action[command_location])))==0){
					DEBUG_PRINT("Inside showHandleStatus");	
					if(strcmp(action[handle_id_location],"")==0){
						reply_string=showHandleStatus(0);
					}else{
						DEBUG_PRINT("Inside showHandleStatus %d ",atoi(action[handle_id_location]));	
						reply_string=showHandleStatus(atoi(action[handle_id_location]));
					}
			}
			else if ((strncmp(action[command_location],"exit",strlen("exit")))==0){			
	  			//strcpy(reply_string,"exit");
	  			DEBUG_PRINT("EXIT");
	  			return NULL;
			}
	  		else
	  		{	
	  			strcpy(reply_string,"Unsupported Command");
	  		}
	  		
		}
		else{
			DEBUG_PRINT("not allocated");
		}

		if (action!=NULL){

			DEBUG_PRINT("De alloaction");
			free(action);//clear  the request recieved 
		}

		return reply_string;
}
/*************************************************************
@brief
Implementatio of Client thread and runs continously till exit command
Input   -client_sock_id - socket id associated with each client connected 
		-Receives message strings (commands) from client
Output -Provides handle id for each PingSites Command
	    - CommandAnalysis used to perform other task  

****************************************************************/
//To correct handling of client connections 
void *client_connections(void *client_sock_id){
	
	int i=0,total_attr_commands=0;
	int thread_sock = (intptr_t)(client_sock_id);	
	ssize_t read_bytes=0;
	char message_client[MAXBUFSIZE];//store message from client 
	char *handle_string;
	
	//Wait for command from Client 
	//Recieve the message from client  and reurn back to client 
	do {
		memset(message_client,0,MAXBUFSIZE);
		if((read_bytes =recv(thread_sock,message_client,MAXBUFSIZE,0))>0){
			DEBUG_PRINT("%s Message length%d\n",message_client,(int)strlen(message_client) );
			handle_string=commandAnalysis(message_client);

			if(handle_string){
				DEBUG_PRINT("Command is not NULL");
				sendDataToClient(handle_string,thread_sock);
			}else{
				DEBUG_PRINT("Command is NULL");
			}	
		}
	}while(handle_string!=NULL );
	
	if(handle_string){
		free(handle_string);
	}	
	if (thread_sock){	
		DEBUG_PRINT("Break While loop and closing socket");
		close(thread_sock);
	}

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

	
	

	//Create the queue and work on it 
	key_t key;
	if((key= ftok("server.c",'B')) <0){
		perror("ftok");
		close(server_sock);
		exit(-1);
	}

	if((msgid==msgget(key,0644|IPC_CREAT))<0){
		perror("msgget");
		close(server_sock);
		exit(-1);
	}
            	
	//Spawn MAX_WORKER_THREADS
	for(i=0;i<MAX_WORKER_THREADS;i++){
			//Create the pthread 
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
			DEBUG_PRINT("Malloc successfully\n");
			
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
	
	//Wait for worker threads to complete and join
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

	//Remove the message queue 
    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        perror("msgctl");
        exit(-1);
    }

	//Close 
	close(server_sock);
	
}
