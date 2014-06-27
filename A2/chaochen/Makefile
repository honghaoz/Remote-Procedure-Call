all: stringClient stringServer

stringClient:
	g++ -o stringClient stringClient.cc -pthread

stringServer:
	g++ -o stringServer stringServer.cc -pthread

clean:
	rm -f *.o stringClient stringServer
