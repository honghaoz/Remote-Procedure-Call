all: Client Server Binder

Client:
	g++ -Wall -o client client1.cc rpc_helper.cc rpc_client.cc pmap.cc

Server:
	g++ -Wall -o server server.cc server_functions.cc server_function_skels.cc rpc_helper.cc pmap.cc rpc_server.cc -lpthread

Binder:
	g++ -Wall -o binder binder.cc rpc_helper.cc rpc_binder.cc pmap.cc
