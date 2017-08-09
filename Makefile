bridge: bridge.c station.c
	gcc bridge.c -o bridge -lpthread
	gcc station.c -o station 
