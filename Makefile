all: Client Server Binder

Client:
	g++ -Wall -o client1 client1.cc rpc_helper.cc rpc_client.cc pmap.cc
	g++ -Wall -o client2 client2.cc rpc_helper.cc rpc_client.cc pmap.cc
	g++ -Wall -o client_cacheCall client_cacheCall.cc rpc_helper.cc rpc_client.cc pmap.cc

Server:
	g++ -Wall -o server server.cc server_functions.cc server_function_skels.cc rpc_helper.cc pmap.cc rpc_server.cc -lpthread
	g++ -Wall -o server2 server2.cc server_functions.cc server_function_skels.cc rpc_helper.cc pmap.cc rpc_server.cc -lpthread
	g++ -Wall -o server3 server3.cc server_functions.cc server_function_skels.cc rpc_helper.cc pmap.cc rpc_server.cc -lpthread
	g++ -Wall -o server4 server4.cc server_functions.cc server_function_skels.cc rpc_helper.cc pmap.cc rpc_server.cc -lpthread

	
Binder:
	g++ -Wall -o binder binder.cc rpc_helper.cc rpc_binder.cc pmap.cc
