/* DESCHASTRES Thomas
 * RIPOLL Guillaume
 * gcc -lm
*/
#include <math.h> // ceil
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
	int charge;	// temps d'exécution
	int t;		// période
	int num; 
} a_tache, p_tache;

typedef struct
{
	int r0;
	int Cs;
	int Ps;
} Server;

void parse_args(char *argv[], Server *srv)
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

/* iterate_CNS
 * calcule une itération du théorème de Lehoczky et al.
*/
double iterate_CNS(int i, int k, int m, 
				   p_tache *tasks, int nb_tasks)
{
	double d = 0.;
	for(int j = 0; j<=i; ++j){
		d += tasks[j].charge * ceil( (m*tasks[k].t) / tasks[j].t);
	}
	d /= (m*tasks[k].t);
	return d;
}

/* Test d'ordonnançabilité par Condition Nécessaire Suffisante
 * retourne 0 si ordonnançable, -1 sinon.
*/
char CNS(p_tache *p_tasks, int nb_p_tasks)
{
	int sigma = 0;
	char is_i_schedulable;
	for(int i = 0; i<nb_p_tasks; ++i)
	{
		for(int k = 0; k<i; ++k)
		{
			is_i_schedulable = -1;
			for(int m = 1; m<i+1; ++m)
			{
				if(iterate_CNS(i, k, m, p_tasks, nb_p_tasks) <= 1.)
				{
					is_i_schedulable = 1;
					break;
				}
			}
			if(is_i_schedulable == 1)
				break;
		}
		if(is_i_schedulable != 1)
			return -1;
	}
}

int main(int argc, char *argv[])
{
	Server srv;
	if(argc != 4)
		usage(argv[0]);
	parse_args(argv, &srv);

	return 0;
}
