
/*******************************************************************************
Author :Chinmay Shah 
File :client.c
Last Edit : 6/14

Client implementation for Client and Server application (SilverPeak Test assignment)
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



//#define DEBUGLEVEL

#ifdef DEBUGLEVEL
	#define DEBUG 1
#else
	#define	DEBUG 0
#endif

#define DEBUG_PRINT(fmt, args...) \
        do { if (DEBUG) fprintf(stderr, "\n %s:%d:%s(): " fmt, __FILE__, \
                                __LINE__, __FUNCTION__, ##args); } while (0)

#define LOCAL_ADDRESS 127.0.0.1
#define SEVER_PORT 8000        


//size restriction 
#define MAXBUFSIZE 60000
#define MAX_COMMAND_SIZE 1000
#define MAX_COL_SIZE 100
#define MAX_TOTAL_SIZES 20  // add restriction on number of sites on client         

//Socket parameters for Client
struct sockaddr_in remote;              //"Internet socket address structure"
struct sockaddr *remoteaddr;
struct sockaddr_in from_addr;
int addr_length = sizeof(struct sockaddr);



// For input command split 
typedef enum COMMANDLOCATION{
		CommandExtra,//Extra Character
		command_location,//Command type
		handle_id,//Handle info
	}COMMAND_LC;//Command format


int sendcommandToServer (char *sendMessage,int size,int socketID)
{
	//write(socketID,sendMessage,sizeof(sendMessage));	
	write(socketID,sendMessage,size);	
	return 1;
} 

void rcvdataFromServer(int socketID){
	ssize_t read_bytes;
	char message_server[MAXBUFSIZE];
	bzero(message_server,sizeof(message_server));
	if((read_bytes =recv(socketID,message_server,sizeof(message_server),0))>0){

		DEBUG_PRINT("Read Bytes %d",(int)read_bytes);	
		DEBUG_PRINT("Message from Server => %s \n",message_server );

		if ((strlen(message_server)>0) && (message_server[strlen(message_server)-1]=='\n')){
				message_server[strlen(message_server)-1]='\0';
		}
		//check if error message 
		if (!strncmp(message_server,"Error",strlen("Error")))
		{
			printf("\n%s\n",message_server);
		}else
		{
			printf("%s\n",message_server);
		}	
	}
		
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

int serverConnection(){

	//For Socket
		int client_sock;
		struct sockaddr_in server;
	//Create a client_sock 
		if ((client_sock= socket(AF_INET , SOCK_STREAM , 0))<0){
	                printf("Issue in Creating Socket,Try Again !! %d\n",client_sock);
			        perror("Socket --> Exit ");			        		  
		    		exit(-1);
		}
		DEBUG_PRINT("Socket Created");
		
		server.sin_addr.s_addr = inet_addr("127.0.0.1");
		server.sin_family = AF_INET;
		server.sin_port = htons(SEVER_PORT); //htons() sets the port # to network byte order
			  
		//Connect to remote server	  	
		if (connect(client_sock , (struct sockaddr *)&server,sizeof(server)) < 0)
	    {
	        perror("\nConnect failed. Error");
	        
	        return -1;
	    }
	    return client_sock;	   
}



void helpOptions(){
	char command[MAX_COMMAND_SIZE];//Local command storage 
	printf("\n pingSites <site> - To ping sites from server <site1,site2,site3> <max ten sites>");
	printf("\n showHandles - to show all handles of the server");
	printf("\n showHandleStatus <handle> -to show the status of <handle> , by default for all handles");
	printf("\n exit -Exit client \n");
}


int commandAnalysis(char command[MAX_COMMAND_SIZE]){

	char (*action)[MAX_COL_SIZE];
	int total_attr_commands=0;
	//Split the command from user , check sanity of commands 
		if ((action=calloc(MAX_COL_SIZE,sizeof(action)))){	
			
			if((total_attr_commands=splitString(command," ",action,4)>0)) {
   				DEBUG_PRINT("Total Commands >0  => %d",total_attr_commands);
		  	}	
			else
			{
				printf("Error in Command Split\n");
				return -1;	
			}
		}
		else
		{
			perror("Allocation for command ");
		}
		DEBUG_PRINT("Command Type %s =>%s ",action[1],action[2]);
		if ((strncmp(action[command_location],"pingSites",strlen("pingSites"))==0)){
					DEBUG_PRINT("Inside pingSites");
		}
		else if ((strncmp(action[command_location],"showHandles",strlen("showHandles")))==0){
				DEBUG_PRINT("Inside showHandles");
  		}
		else if ((strncmp(action[command_location],"showHandleStatus",strlen("showHandleStatus")))==0){
				DEBUG_PRINT("Inside showHandleStatus");	
		}
		else if ((strncmp(action[command_location],"exit",strlen("exit")))==0){			
  			
		}
		else if ((strncmp(action[command_location],"help",strlen("help")))==0){			
  			helpOptions();	
  			return -1;
		}
  		else
  		{	
  			helpOptions();		  			
  			return -1;
  		}	
  		return 1;
}




		
int main (int argc, char * argv[] ){

	char command[MAX_COMMAND_SIZE];
	char command_to_send[MAX_COMMAND_SIZE];
	//Input of filename for config 
	if (argc >= 4 ){
		printf("USAGE:  \n");
		//exit(1);
	}

	DEBUG_PRINT("%s\n",argv[1] );
	while(1){

		//wait for input from user 
		fgets(command,MAX_COMMAND_SIZE,stdin);
		if ((strlen(command)>0) && (command[strlen(command)-1]=='\n')){
				command[strlen(command)-1]='\0';
		}	
		strcat(command,"\0");
		DEBUG_PRINT("Command %s",command);
		strncpy(command_to_send,command,sizeof(command));
		strcat(command_to_send,"\0");
		//check sanity of command
		if(commandAnalysis(command)>=0){
		
			int client_sock=serverConnection();

		    if(client_sock){
		    	DEBUG_PRINT("Connected Socket %d",client_sock);
		    	DEBUG_PRINT("Command to send %s",command_to_send);
			    if(sendcommandToServer(command_to_send,strlen(command_to_send),client_sock)<0){
			    	DEBUG_PRINT("Connected Socket %d",client_sock);
			    }

		    }

		    rcvdataFromServer(client_sock);

		    close(client_sock);
		 }   
	}    
}

