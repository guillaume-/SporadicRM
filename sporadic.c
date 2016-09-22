/* DESCHASTRES Thomas
 * RIPOLL Guillaume
*/
#include <stdio.h>
#include <stdlib.h>

typedef struct sporadic_server{
	char r0;
	char Cs;
	char Ps;
} Server;

void parseArgs(char *argv[], Server *srv)
{
	srv->r0 = (int) strtol(argv[1], (char **)NULL, 10);
	srv->Cs = (int) strtol(argv[2], (char **)NULL, 10);
	srv->Ps = (int) strtol(argv[3], (char **)NULL, 10);
}

void usage(char * progname)
{
	printf("\
usage : %s r0 Cs Ps\n\
r0 = temps de démarrage\n\
Cs = capacité maximale du serveur\n\
Ps = période du serveur (= Ds deadline)\n", progname);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	Server srv;
	if(argc != 4)
		usage(argv[0]);
	parseArgs(argv, &srv);

	return 0;
}
