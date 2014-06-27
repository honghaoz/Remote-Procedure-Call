#include <iostream>
#include <pthread.h>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
using namespace std;

/***************** establish *************************
 Purpose: establish the connection socket on server
          side
 
 return: (integer) the socket fd or -1 as error occurs
 ********************************************************/
int establish(unsigned short portnum){
    struct sockaddr_in sa;
    struct hostent *hp;
    char myname[100+1];
    int status;
    
    memset(&sa, 0, sizeof(struct sockaddr_in));
    gethostname(myname,100);
    hp = gethostbyname(myname);
    if(hp==NULL){
        cout<<"wrong host name"<<endl;
        return -1;
    }
    sa.sin_family = hp->h_addrtype;
    sa.sin_port = htons(portnum); // bind to the next available port
    sa.sin_addr.s_addr = INADDR_ANY;// bind to the ip
    
    
    int socketfd;
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfd < 0){
        cout<<"socket error"<<endl;
        exit(-1);
    }
    int yes = 1;
    status = setsockopt(socketfd,SOL_SOCKET,SO_REUSEADDR, &yes, sizeof(int));
    status = bind(socketfd, (struct sockaddr*)&sa,sizeof(struct sockaddr_in));
    if(status < 0){
        cout<<"bind error"<<endl;
        close(socketfd);
        exit(-1);
    }
    
    status = listen(socketfd, 5);
    if(status < 0){
        cout<<"listen error"<<endl;
    }
    
    int length = sizeof(sa);
    getsockname(socketfd,(struct sockaddr*)&sa, (socklen_t*)&length);
    cout<<"SERVER_ADDRESS "<<myname<<endl;
    cout<<"SERVER_PORT "<<ntohs(sa.sin_port)<<endl;
    
    return socketfd;
}

/***************** get_connection *************************
 Purpose: connect to client which send request to server
 
 return: (integer) next available socket file descriptor
 ********************************************************/
int get_connection(int s){
    int t;
    if((t = accept(s,NULL,NULL)) < 0){
        cerr<<"connection faild!"<<endl;
    }
    return t;
}

/***************** TitleCase *************************
 Purpose: consume a string get change it to title case
          string
 
 return: (void)
 ********************************************************/
void TitleCase(char* message){
    for(int i = 0; i < strlen(message);i++){
        if(i==0 && message!=NULL){
            *message = toupper(*message);// change the first char to upper case
        }
        else if(*(message+i-1) == ' ' || *(message+i-1)=='\t'){
            *(message+i) = toupper(*(message+i));// first char after a space to
                                                 // to upper case
        }
        else{
            *(message+i) = tolower(*(message+i));
        }
    }
}


/***************** main *************************
 Purpose: the main function of stringServer
 
 return:
 *************************************************/
int main(){
    int sockfd,t,maxfd;
    fd_set master, readfds;//for select() function
    
    if((sockfd=establish(0))<0){//set 0 to dynamically use an available port
        cout<<"establish error"<<endl;
        exit(-1);
    }
    
    FD_ZERO(&master);
    FD_ZERO(&readfds);
    FD_SET(sockfd, &master);
    maxfd = sockfd;
    
    int newsocketfd;
    
    int status,status2;
    
    while(true){
        
        readfds = master;
        if(select(maxfd+1, &readfds,NULL,NULL,NULL) == -1){//use selete to do unblocked multiplex
            cout<<"Select ERROR!"<<endl;
            exit(-1);
        }
        for(int i=0;i < maxfd+1;i++){
            if(FD_ISSET(i,&readfds)){
                if(i==sockfd){
                    newsocketfd = get_connection(sockfd);
                    FD_SET(newsocketfd,&master);
                    if(newsocketfd > maxfd){
                        maxfd = newsocketfd;
                    }
                }
                else{
                    char* buffer = new char[2048];
                    int length = recv(i,buffer,100*(sizeof(char)),0);//receiving message from client
                    if(length)
                        cout<<"received message: '"<<buffer<<"' from client fd "<<i<<endl;
                    else{
                        FD_CLR(i,&master);
                    }
                    TitleCase(buffer);
                    status = send(i,buffer,length,0);//send back message to client
                    delete [] buffer;
                    string empty = "";
                    buffer = strndup(empty.c_str(),empty.size()+1);
                    delete [] buffer;
                    buffer = NULL;
                }
            }
        }
    }
    return 0;
}











