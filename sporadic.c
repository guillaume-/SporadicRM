/* DESCHASTRES Thomas
 * RIPOLL Guillaume
 * gcc -lm
*/
#include <math.h> // ceil
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FICHIER_CONF "conf_sporadic"
#define CONF_ERROR 0
#define EXEC_ERROR 1
#define NUM_CYCLES 30
#define DEFAULT_MAX_CYCLES 50

typedef char bool;

typedef struct
{
	int curr_charge;
	int charge; //temps d'exécution
	int t; //date de début/période
	int num; //numéro de la tâche
} a_tache, p_tache;

typedef struct
{
	int r0;
	int Cs;
	int Ps;
} Server;

typedef struct params_serv
{
	a_tache *a;
	p_tache *p;
	int a_size;
	int p_size;
	Server srv;
	int *a_rdy;
	int curr_cycle;
} Param;

typedef struct structDelay
{
	int delay;
	int charge;
	struct structDelay *next;
} Delay, *Delays;

Param params;
int max_cycles = DEFAULT_MAX_CYCLES;

void error(int type)
{
	switch(type)
	{
		case CONF_ERROR :
			printf("Erreur dans le fichier de configuration");
			break;		
		case EXEC_ERROR :
			printf("Une tâche n'a pas pû s'exécuter en entier durant sa période");
			break;
		default :
			printf("Erreur");
			break;
	}
}

void usage(char *progname)
{
	printf("\
	usage : %s r0 Cs Ps\n\
	r0 = temps de démarrage\n\
	Cs = capacité maximale du serveur\n\
	Ps = période du serveur (= Ds deadline)\n", progname);
	exit(EXIT_FAILURE);
}

void parse_args(char *argv[])
{
	params.srv.r0 = (int) strtol(argv[1], (char **)NULL, 10);
	params.srv.Cs = (int) strtol(argv[2], (char **)NULL, 10);
	params.srv.Ps = (int) strtol(argv[3], (char **)NULL, 10);
}

// add an element at the end
void D_add(Delays *list, int delay, int charge)
{
	Delays l = *list;
	if(l == NULL)
		l = malloc(sizeof(Delay));
	else
	{
		for(l = (*list); l->next!=NULL; l = l->next);
		l->next = malloc(sizeof(Delay));
		l = l->next;
	}	
	l->delay = delay;
	l->charge = charge;
	l->next = NULL;
}

// delete the first element
void D_del(Delays *list)
{
	Delays l = *list;
	if(l != NULL)
	{
		if(l->next == NULL)
		{
			free(l);
			*list = NULL;
		}
		else
		{
			l = l->next;
			free(*list);
			*list = l;
		}
	}
}

void D_close(Delays *list)
{
	while((*list) != NULL)
		D_del(list);
}

// to call at each new task
int D_update(Delays *list, int delay_reduce)
{
	int chargeUp = 0;
	for(Delays l = *list; l != NULL; l = l->next){
		l->delay -= delay_reduce;
		if(l->delay == 0)
		{
			chargeUp += l->charge;
			D_del(list); // first to desappear are first on the list
		}
	}
	return chargeUp;
}

/* Une tâche apériodique a la priorité du serveur.
 * La charge du serveur diminue au lancement d'une tâche apériodique de sa charge, mais augmente après ce lancement avec un délai de serveur.periode cycles
 * A n'appeler que lors du lancement d'une tâche apériodique
 * Code à include dans cycle
*/
int chck_charge(bool finish, a_tache task)
{
	// ALLOCATIONS
	static int Charge = -1; // Charge serveur à un temps donné
	static Delays linked_list = NULL;

	if(Charge == -1) // Init with constant element
		Charge = params.srv.Cs;

	// INCREMENTATION DU TEMPS
	Charge += D_update(&linked_list, 1);

	// Libérer proprement la mémoire
	if(finish) 
		D_close(&linked_list);

	// TASK CHOICE
	if(Charge < task.charge) // charge du serveur inférieure à la charge de la tâche, erreur à gérer par la suite
	{
		printf("Charge de la tâche apériodique = %d > charge du serveur = %d\n", task.charge, Charge);
		return -1;
	}

	Charge -= task.charge;
	D_add(&linked_list, params.srv.Ps, task.charge);
	return 0;
}

//lit le fichier de configuration et remplit les deux tableaux reçus en paramètre
void read_conf()
{
	//on ouvre le fichier de config
	FILE *conf;
	conf = fopen(FICHIER_CONF, "r+");

	//on prépare le buffer qui contiendra les lignes qu'on va lire une par une
	char buffer[64];
	//les tableaux auront une taille initiale correspondant à 5 tâches chacun
	int taches_a_size = 5;
	int taches_p_size = 5;
	//ces deux indices servent à retenir combien de tâches sont déjà présentes dans le tableau
	int num_taches_a = 0;
	int num_taches_p = 0;

	//on alloue la taille de chaque tableau
	params.a = (a_tache*)calloc(taches_a_size, sizeof(a_tache));
	params.p = (a_tache*)calloc(taches_p_size, sizeof(p_tache));

	//contiendront provisoirement les informations relatives à la tâche en train d'être lue dans le fichier de config
	char charge[3];
	char t[3];
	char num;
	//on retient les positions des séparateurs des attributs de la tâche
	// la tâche est sous la forme 'Tx={y,z}' donc on retient la position de la virgule et de l'accolade fermante
	int poscoma = 0;
	int posbrace = 0;

	//tant qu'on a pas lu l'entièreté du fichier de conf
	while(!feof(conf))
	{
		//on lit une ligne
		fscanf(conf, "%s", buffer);

		if(buffer[0] == 'P')
		{
			//on récupère les positions de la virgule et de l'accolade, puisque les nombres peuvent avoir plusieurs chiffres
			poscoma = strchr(buffer, ',') - buffer;
			posbrace = strchr(buffer, '}') - buffer;

			//on récupère la charge et la période/la date de début
			strncpy(charge, buffer + 4, poscoma - 4);
			strncpy(t, buffer + poscoma + 1, posbrace - poscoma + 1);

			//le numéro est contenu dans le deuxième caractère du buffer, à étendre si on a plus de 10 tâches
			num = buffer[1];

			//on stocke les informations des variables provisoires dans le tableau
			params.p[num_taches_p].num = num - '0';
			params.p[num_taches_p].charge = atoi(charge);
			params.p[num_taches_p].curr_charge = atoi(charge);
			params.p[num_taches_p].t = atoi(t);

			num_taches_p++;
		}
		else if(buffer[0] == 'A')
		{
			poscoma = strchr(buffer, ',') - buffer;
			posbrace = strchr(buffer, '}') - buffer;

			strncpy(charge, buffer + 4, poscoma - 4);
			strncpy(t, buffer + poscoma + 1, posbrace - poscoma + 1);

			num = buffer[1];

			params.a[num_taches_a].num = num - '0';
			params.a[num_taches_a].charge = atoi(charge);
			params.a[num_taches_a].curr_charge = atoi(charge);
			params.a[num_taches_a].t = atoi(t);
			num_taches_a++;
		}
		else if(buffer[0] != 0 && buffer[0] != '#')
		{
			error(CONF_ERROR);
		}
		buffer[0] = '\0';
		charge[0] = '\0';
		t[0] = '\0';
	}
	//*taches_p = realloc(*taches_p, sizeof(p_tache) * num_taches_p);
	//*taches_a = realloc(*taches_a, sizeof(a_tache) * num_taches_a);
	params.a_rdy = calloc(num_taches_a, sizeof(int));
//	for(int i = 0; i < num_taches_p; i++)
//	{
//		printf("T : P%d charge = %d t = %d \n", params.p[i].num, params.p[i].charge, params.p[i].t);
//	}
//	for(int i = 0; i < num_taches_a; i++)
//	{
//		printf("T : A%d charge = %d t = %d \n", params.a[i].num, params.a[i].charge, params.a[i].t);
//	}
	params.a_size = num_taches_a;
	params.p_size = num_taches_p;
	fclose(conf);
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

//une tâche périodique est disponible si elle a encore de la charge à exécuter
int available(p_tache *p)
{
	//on décide quelle tâche a la priorité
	//de base on dit que c'est le serveur
	int max_prio = params.srv.Ps;
	if(p->curr_charge > 0)
		return 1;
	return 0;
	printf("P%d a une charge de : %d (P = %d) \n", p->num, p->curr_charge, p->t);
	if(p->curr_charge > 0)
	{
//		printf(" -> available\n");
		return 1;
	}
//	printf(" -> unavailable\n");
	return 0;
}

int get_tache_prio()
{
	/*On parcourt la liste des tâches périodiques disponibles pour
		 * trouver laquelle a la plus grande priorité
		 */
	int i;
	p_tache prio;

	int found = 0;

	for(i = 0; i < params.p_size; i++)
	{
		//il faut bien sûr que la tâche n'ait pas encore été exécutée pendant cette période
		if(available(&(params.p[i])))
		{
			//si on a pas encore trouvé de tâche disponible, alors on met celle qu'on vient de trouver
			if(!found)
			{
				prio = params.p[i];
				found = 1;
			}
			else
			//sinon on regarde si elle a une plus grande priorité que celle qu'on a actuellement
				if(params.p[i].t < prio.t)
				{
					prio = params.p[i];
				}
		}
	}
	
	//si on a trouvé une tâche disponible on la renvoie
	if(found)
	{
		return prio.num;
	}
	return -1;
}

void exec_p(p_tache *p)
{
	p->curr_charge--;
}

void exec_a()
{
	int tache = params.a_rdy[0];
	params.a[tache].curr_charge--;
	chck_charge(0, params.a[tache]);
}

p_tache *get_ptask_from_num(int num)
{
	int i;
	//on vérifie pour toutes les tâches périodiques si leur période recommence
	//si c'est le cas on leur rend leur charge totale à exécuter
	for(i = 0; i < params.p_size; i++)
	{
		//si le cycle courant est un multiple de la période de la tâche courante
		//alors la tâche récupère sa charge courante
		if(params.curr_cycle % params.p[i].t == 0)
		{
			//on vérifie que la tâche a bien eu le temps de faire toute son exécution
			//si c'est pas le cas baaah, c'est une erreur
			if(params.p[i].curr_charge > 0)
			{
				error(EXEC_ERROR);
			}
			else
			{
				params.p[i].curr_charge = params.p[i].charge;
			}
		}
	}
	
	//on vérifie si la première tâche apériodique (si elle existe) 
	//de la liste des a_taches prêtes a encore de la charge à exécuter
	if(params.a_rdy[0] != 0)
	{
		//sinon on la sort de la file
		if(params.a[params.a_rdy[0]].curr_charge == 0)
		{
			//on parcours la liste des tâches apériodiques prêtes à être exécutées
			//et on bouge chacune d'elles à la case d'avant (on fait défiler quoi)
			for(i = 1; i < params.a_size; i++)
			{
				//si on est arrivé au bout de la file on sort
				if(params.a_rdy[i] == 0)
					break;
				params.a_rdy[i - 1] = params.a_rdy[i];
			}
		}
	}
}

void check_tasks();
a_tache* get_atask_from_num(int num)
{
	check_tasks();
	//on regarde si on a une tache périodique à lancer
	int p_prio = get_tache_prio(params);

	//si c'est le cas on vérifie que sa priorité est effectivement plus grande que celle du serveur
	if(p_prio != -1)
	{
		// Si le serveur a la priorité et a une tâche à lancer il la lance
		if((params.p[p_prio].t > params.srv.Ps) && params.a_rdy[0] > 0)
		{
			exec_a();
		}
		//sinon on lance la tâche périodique
		else
		{
			exec_p(&(params.p[p_prio]));
		}
	}
	for(int i = 0; i < params.a_size; i++)
	{
		if(params.a[i].num == num)
		{
			return &params.a[i];
		}
	}
	return NULL;
}

void check_tasks()
{
	int i;

	//on vérifie pour toutes les tâches périodiques si leur période recommence
	//si c'est le cas on leur rend leur charge totale à exécuter
	for(i = 0; i < params.p_size; i++)
	{
		//si le cycle courant est un multiple de la période de la tâche courante
		//alors la tâche récupère sa charge courante
		if(params.curr_cycle % params.p[i].t == 0 && params.curr_cycle != 0)
		{
			//on vérifie que la tâche a bien eu le temps de faire toute son exécution
			//si c'est pas le cas baaah, c'est une erreur
			if(params.p[i].curr_charge > 0)
			{
				printf("Cycle %d : la tache %d a encore %d charge à exécuter\n", params.curr_cycle, params.p[i].num, params.p[i].charge);
			}
			else
			{
				params.p[i].curr_charge = params.p[i].charge;
			}
		}
	}

	//on vérifie si la première tâche apériodique (si elle existe)
	//de la liste des a_taches prêtes a encore de la charge à exécuter
	if(params.a_rdy[0] != 0)
	{
		//sinon on la sort de la file
		if(params.a[params.a_rdy[0]].curr_charge == 0)
		{
			params.a_rdy[0] = 0;

			//on parcours la liste des tâches apériodiques prêtes à être exécutées
			//et on bouge chacune d'elles à la case d'avant (on fait défiler quoi)
			for(i = 1; i < params.a_size; i++)
			{
				//si on est arrivé au bout de la file on sort
				if(params.a_rdy[i] == 0)
				{
					break;
				}
				params.a_rdy[i - 1] = params.a_rdy[i];
				params.a_rdy[i] = 0;
			}
		}
	}

	// on vérifie dans la liste des tâches apériodiques la ou lesquelles
	// sont supposées commencer lors de ce cycle
	for(i = 0; i < params.a_size; i++)
	{
		if(params.a[i].t == params.curr_cycle)
		{
			printf("La tâche apériodique A%d commence ce cycle \n", params.a[i].num);
			//on la ou les ajoute en bout de file
			for(i = 0; i < params.a_size; i++)
			{
				if(params.a_rdy[i] == 0)
				{
					params.a_rdy[i] = params.a[i].num;
					break;
				}
			}
		}
	}
}

int cycle()
{
	check_tasks();

	//on regarde si on a une tache périodique à lancer
	int p_prio = get_tache_prio();

	if(params.a_rdy[0] > 0)
	{
		printf("Les tâches aperiodiques attendant d'être exécutées sont : \n");
		int a = 0;
		for(int i = 0; i < params.a_size; i++)
		{
			if(params.a_rdy[i] > 0)
			{
			   a = params.a_rdy[a];
			   printf("A%d : charge = %d\n", params.a[a].num, params.a[a].curr_charge);
			}
		}
	}

	//si c'est le cas on vérifie que sa priorité est effectivement plus grande que celle du serveur
	if(p_prio != -1)
	{
		p_tache *prio = get_ptask_from_num(p_prio);
		// Si le serveur a la priorité et a une tâche à lancer il la lance
		if((prio->t > params.srv.Ps) && params.a_rdy[0] > 0 && params.a[params.a_rdy[0]].curr_charge > 0)
		{
			printf("Le serveur a la priorité ce cycle donc on exécute la tâche apériodique %d à qui il reste %d à exécuter \n", params.a_rdy[0], params.a[params.a_rdy[0]].curr_charge);
			exec_a();
		}
		//sinon on lance la tâche périodique
		else
		{
			printf("\nOn a une tâche à exécuter avant le serveur : %d\n", prio->num);

			exec_p(prio);
		}
	}
	else if(params.a_rdy[0] > 0)
	{
		exec_a();
	}
	else
	{
		printf("Rien à faire ce cycle\n");
		printf("Ordonnancement trouvé en %d cycles\n ", params.curr_cycle - 1);
		return 1;
	}
	return 0;
}

int main(int argc, char *argv[])
{
	// Server initialisation
	if(argc != 4)
		usage(argv[0]);
	parse_args(argv);
	read_conf();
	/*if(CNS(params.p, params.p_size) == -1)
	{
		printf("Non ordonnançable.\n");
		return 1;
	}*/
//	a_tache *taches_aperiodiques;
//	p_tache *taches_periodiques;
//	params.a = taches_aperiodiques;
//	params.p = taches_periodiques;
	params.curr_cycle = 0;

	int success = 0;
	while(!success && params.curr_cycle < max_cycles)
	{
		printf("\n----------CYCLE NUMERO %d---------- \n\n", params.curr_cycle);
		success = cycle();
		params.curr_cycle++;
	}
	if(!success)
	{
		printf("Pas trouvé d'ordonnancement en moins de %d cycles", max_cycles);
	}
	return 0;
}
