#include <iostream>
#include <pthread.h>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
using namespace std;

string sentence;
volatile bool newMessage;
pthread_mutex_t mutex;

/***************** connect *************************
 Purpose: connect to server by using hostname and 
          port number
 
 return: (integer) the socket fd or -1 as error occurs
 ********************************************************/
int connect(char* hostname, char* portnumber){
    struct addrinfo host_info;
    struct addrinfo* host_info_list;
    int status;
    
    memset(&host_info, 0, sizeof host_info);
    
    host_info.ai_family = AF_UNSPEC; //IP version not specified.
    host_info.ai_socktype = SOCK_STREAM;//SOCK_STREAM is for TCP
    status = getaddrinfo(hostname,portnumber, &host_info, &host_info_list);
    if(status){
        cerr<<"Cannot Get Address Information!"<<endl;
        exit(-1);
    }
    
    int socketfd;
    socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
    if(socketfd == -1){
        cerr<<"Socket Error"<<endl;
        exit(-1);
    }
    
    
    status = connect(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if(status == -1){
        cerr<<"connection error!"<<endl;
        exit(-1);
    }
    
    return socketfd;
}


/***************** Input ********************************
 Purpose: read standard input by reading thread
 
 return: (void*)
 ********************************************************/
void * Input(void* arg){
    while(true){
        if(!newMessage){
            pthread_mutex_lock(&mutex);
            getline(cin,sentence);
            newMessage = true;
            pthread_mutex_unlock(&mutex);
        }
    }
}


/***************** sending_and_receiving ******************
 Purpose: send and receive message to/from server side
 
 return: (void)
 ********************************************************/
void sending_and_receiving(int sockfd){
    int message_size;
    while(true){
        if(newMessage){
            pthread_mutex_lock(&mutex);
            message_size = sentence.size();
            char* message = strndup(sentence.c_str(),sentence.size()+1);
            send(sockfd,message,sentence.size()+1,0);//send message
            delete[] message;
            newMessage = false;
            pthread_mutex_unlock(&mutex);
            char* server_message = (char*)malloc(message_size+1);
            sleep(2);
            recv(sockfd,server_message,message_size+1,0);//receive message
            cout<<"Server: "<<server_message<<endl;
            delete[] server_message;
        }
    }
}

/***************** main *************************
 Purpose: the main function of stringClient
 
 return:
 *************************************************/
int main(){
    pthread_t thread;
    sentence = "";
    newMessage = false;
    int sockfd;
    if(pthread_create(&thread,NULL,&Input,NULL)){//create a pthread to read standard input
        cerr<<"error!"<<endl;
        return -1;
    }
    
    char* host = getenv("SERVER_ADDRESS");//get the server hostname from env
    if(host == NULL){
        cerr<<"host is null"<<endl;
        exit(-1);
    }
    char* portnum = getenv("SERVER_PORT");//get the server socket port from env
    if(portnum == NULL){
        cerr<<"port number is NULL!"<<endl;
        exit(1);
    }
    sockfd = connect(host,portnum);
    sending_and_receiving(sockfd);

    close(sockfd);//close the socket
    
    return 0;
}
