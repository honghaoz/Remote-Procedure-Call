#Remote Procedure Call Library (C++)

###Instructions for program

####Commands:
* make 
	
	This command generates rpc_binder.o rpc_server.o rpc_client.o rpc_helper.o pmap.o files and executable file binder

* make clean
	
	Remove all executable files and linkable files e.g. binder rpc_server.o

* ./binder
		
	Run binder program before run server and client. 
	
	Server will print out its server address and next available port
		
* export BINDER_ADDRESS=“hostname”
* export BINDER_PORT=“#port#”

	To set the env variable, and client will get them from 
	client side

* g++ -c client.cc server.cc

	Generate a client.o server. files
* g++ -L. client.o -lrpc client
		
	Generate an executable file client by using rpc library
* g++ -L. server.o server_functions.o server_function_skels.o -lrpc server -lpthread
		
	Generate an executable file server by using roc library
	please add lpthread flag since our server uses multi-threading to handle

##### Client requests
* ./server
		
	Run server to register itself on binder
* ./client
	
	Run client to execute remote procedure call 

Instruction to run program:
	- place code files in same directory than makefile.
	- execute make in order to compile the c++ files
        - then run the binder to show the ip and port number of binder
	- run the server next to show the ip host and port number of server
	- run client after all binder and servers started


##Notice
 
	If the ip is a local host(e.g. 127.0.0.1) IP instead of a real(external) IP address,
	The program on different machines will not connect successfully since the IP is not real.
	In this case, please to change a machine to run our binder and server
	So that to get a real IP of both binder and server.

##Executable files:

* After make, you will get librpc.a

##Version of makefile:

* GNU Make 3.81

##Version of c++ compiler:

* gcc version 4.6.3 (Ubuntu/Linaro 4.6.3-1ubuntu5)

##Contributor
	
* Chao Chen
* Honghao Zhang



