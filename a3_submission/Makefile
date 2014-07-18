CXX = g++
EXECS = binder
###############################################
.PHONY : all clean

all: 
	${CXX} -c binder.cc rpc_helper.cc rpc_binder.cc pmap.cc rpc_client.cc rpc_server.cc
	${CXX} -o binder binder.o rpc_helper.o rpc_binder.o pmap.o
	ar rcs librpc.a rpc_helper.o rpc_binder.o pmap.o rpc_client.o rpc_server.o
	#rm -f rpc_helper.o rpc_binder.o pmap.o rpc_client.o rpc_server.o binder.o
clean:
	rm -f *.d *.o ${EXECS}
