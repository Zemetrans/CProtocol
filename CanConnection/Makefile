all: ConCli ConServ
ConCli: ConnectionClient.c
	gcc ConnectionClient.c -o ConCli
ConServ: ConnectionServer.c
	gcc ConnectionServer.c -o ConServ

clean:
	rm -f ConCli
	rm -f ConServ
	rm -f *~