/* DESCHASTRES Thomas
 * RIPOLL Guillaume
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FICHIER_CONF "conf_sporadic"
#define CONF_ERROR 0
#define EXEC_ERROR 1
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
	printf("\n");
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
		printf("Charge de la tâche apériodique = %d => charge du serveur = %d\n", task.charge, Charge);
		return -1;
	}

	Charge -= task.charge;
	D_add(&linked_list, params.srv.Ps, task.charge);
	return 0;
}

//lit le fichier de configuration et remplit les deux tableaux reçus en paramètre
void read_conf()
{
    FILE *conf;
    conf = fopen(FICHIER_CONF, "r+");

    char buffer[64];

    int taches_a_size = 5;
    int taches_p_size = 5;

    //ces deux indices servent à retenir combien de tâches sont déjà présentes dans le tableau
    int num_taches_a = 0;
    int num_taches_p = 0;

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

    while(!feof(conf))
    {
        fscanf(conf, "%s", buffer);

		if(buffer[0] == 0)
		{
			break;
		}
        else
        {
			if(buffer[1] == 'P')
			{
				//on récupère les positions de la virgule et de l'accolade, puisque les nombres peuvent avoir plusieurs chiffres
				poscoma = strchr(buffer, ',') - buffer;
				posbrace = strchr(buffer, '}') - buffer;

				//on récupère la charge et la période/la date de début
				strncpy(charge, buffer + 5, poscoma - 5);
				strncpy(t, buffer + poscoma + 1, posbrace - poscoma + 1);

				//le numéro est contenu dans le deuxième caractère du buffer, à étendre si on a plus de 10 tâches
				num = buffer[2];

				//on stocke les informations des variables provisoires dans le tableau
				params.p[num_taches_p].num = num - '0';
				params.p[num_taches_p].charge = atoi(charge);
				params.p[num_taches_p].curr_charge = atoi(charge);
				params.p[num_taches_p].t = atoi(t);

				num_taches_p++;
			}
			else if(buffer[1] == 'A')
			{
				poscoma = strchr(buffer, ',') - buffer;
				posbrace = strchr(buffer, '}') - buffer;

				strncpy(charge, buffer + 5, poscoma - 5);
				strncpy(t, buffer + poscoma + 1, posbrace - poscoma + 1);

				num = buffer[2];

				params.a[num_taches_a].num = num - '0';
				params.a[num_taches_a].charge = atoi(charge);
				params.a[num_taches_a].curr_charge = atoi(charge);
				params.a[num_taches_a].t = atoi(t);

				num_taches_a++;
			}
			else if(buffer[0] != '#')
			{
				printf("%s\n", buffer);
				error(CONF_ERROR);
			}
		}
        
        //on vide les tableaux de caractères pour éviter de se retrouver après avec des erreurs dues à des tableaux mal vidés
        buffer[0] = '\0';
        charge[0] = '\0';
        t[0] = '\0';
    }

    //*taches_p = realloc(*taches_p, sizeof(p_tache) * num_taches_p);
    //*taches_a = realloc(*taches_a, sizeof(a_tache) * num_taches_a);
    
    params.a_rdy = calloc(num_taches_a, sizeof(int));

//DEBUG
    for(int i = 0; i < num_taches_p; i++)
    {
        printf("TP%d charge = %d t = %d \n", params.p[i].num, params.p[i].charge, params.p[i].t);
    }
    for(int i = 0; i < num_taches_a; i++)
    {
        printf("TA%d charge = %d t = %d \n", params.a[i].num, params.a[i].charge, params.a[i].t);
    }

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
		d += tasks[j].charge * ((m * tasks[k].t / tasks[j].t) +1);
	}
	d /= (m*tasks[k].t);
	return d;
}

/* Test d'ordonnançabilité par Condition Nécessaire Suffisante
 * retourne 0 si ordonnançable, -1 sinon.
*/
char CNS(p_tache *p_tasks, int nb_p_tasks)
{
	for(int i = 0; i<nb_p_tasks; ++i)
		for(int k = 0; k<i; ++k)
			for(int m = 1; m <= i; ++m)
				if(iterate_CNS(i, k, m, p_tasks, nb_p_tasks) <= 1.)
					return 0;
	return -1;
}

//une tâche périodique est disponible si elle a encore de la charge à exécuter
int available(p_tache *p)
{
	
	if(p->curr_charge > 0)
	{
		printf("La tâche TP%d doit encore s'exécuter %d cycles cette période (P = %d)\n", p->num, p->curr_charge, p->t);
//		printf(" -> available\n");
		return 1;
	}
//	printf(" -> unavailable\n");
	return 0;
}

void exec_p(p_tache *p);
void exec_a();

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


p_tache *get_ptask_from_num(int num)
{
	for(int i = 0; i < params.p_size; i++)
	{
		if(params.p[i].num == num)
		{
			return &params.p[i];
		}
	}
}

a_tache* get_atask_from_num(int num)
{
	for(int i = 0; i < params.a_size; i++)
	{
		if(params.a[i].num == num)
		{
			return &params.a[i];
		}
	}
}

void task_ready(int num)
{
	printf("La tâche apériodique TA%d commence ce cycle \n", num);
	//on la ou les ajoute en bout de file
	for(int i = 0; i < params.a_size; i++)
	{
		if(params.a_rdy[i] == 0)
		{
			params.a_rdy[i] = num;
			break;
		}
	}
}

void exec_p(p_tache *p)
{
    p->curr_charge--;
}

void exec_a()
{
    a_tache* tache = get_atask_from_num(params.a_rdy[0]);
    tache->curr_charge--;
    chck_charge(0, *tache);
}


int check_tasks()
{
    int i;

    //on vérifie pour toutes les tâches périodiques si leur période recommence
    //si c'est le cas on leur rend leur charge totale à exécuter
    for(i = 0; i < params.p_size; i++)
    {
        //si le cycle courant est un multiple de la période de la tâche courante
        //alors la tâche récupère sa charge courante
        if((params.curr_cycle - params.srv.r0) % params.p[i].t == 0  && params.curr_cycle != 0 + params.srv.r0)
        {
            //on vérifie que la tâche a bien eu le temps de faire toute son exécution
            //si c'est pas le cas baaah, c'est une erreur
            if(params.p[i].curr_charge > 0)
            {
                printf("Cycle %d : la tache TP%d a encore %d temps d'exécution à faire cette période\n", params.curr_cycle, params.p[i].num, params.p[i].charge);
                return 1;
            }
            else
            {
				printf("La tache TP%d a de nouveau du temps d'exécution à effectuer\n", params.p[i].num);
                params.p[i].curr_charge = params.p[i].charge;
            }
        }
    }
 
    //on vérifie si la première tâche apériodique (si elle existe)
    //de la liste des a_taches prêtes a encore de la charge à exécuter
    if(params.a_rdy[0] != 0)
    {
        //sinon on la sort de la file
        if(get_atask_from_num(params.a_rdy[0])->curr_charge == 0)
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
            task_ready(params.a[i].num);
        }
        else if(params.srv.r0 > params.a[i].t && params.srv.r0 == params.curr_cycle)
        {
			task_ready(params.a[i].num);
		}
    }
    
    return 0;
}

int cycle()
{
    if(check_tasks())
    {
		printf("Impossible d'effectuer un ordonnancement correct \n");
		return -1;
	}

    //on regarde si on a une tache périodique à lancer
    int p_prio = get_tache_prio();

    if(params.a_rdy[0] > 0)
    {
        printf("\nLes tâches aperiodiques attendant d'être exécutées sont : \n");
        int a = 0;			

        for(int i = 0; i < params.a_size; i++)
        {
            if(params.a_rdy[i] > 0)
            {
               a = params.a_rdy[i];
               printf("TA%d : temps d'exécution restant = %d\n\n", get_atask_from_num(a)->num, get_atask_from_num(a)->curr_charge);
            }
        }
    }

    //si c'est le cas on vérifie que sa priorité est effectivement plus grande que celle du serveur
    if(p_prio != -1)
    {
        p_tache *prio = get_ptask_from_num(p_prio);
		
		if(params.a_rdy[0] > 0)
		{
			a_tache *a = get_atask_from_num(params.a_rdy[0]);
			
			// Si le serveur a la priorité et a une tâche à lancer il la lance
			if((prio->t > params.srv.Ps) && a->curr_charge > 0)
			{
				printf("Le serveur a la priorité ce cycle donc on exécute la tâche apériodique TA%d à qui il reste %d à exécuter \n", a->num, a->curr_charge);
				exec_a();
			}
			else
			{
				printf("\nLe serveur n'a pas la priorité, on éxécute TP%d\n", prio->num);
				exec_p(prio);
			}
		}
        //sinon on lance la tâche périodique
        else
        {
            printf("\nLe serveur n'a pas de tâche apériodique à exécuter : on exécute TP%d\n", prio->num);

            exec_p(prio);
        }
    }
    else if(params.a_rdy[0] > 0 && get_atask_from_num(params.a_rdy[0])->curr_charge > 0 )
    {
		printf("Pas de tâche périodique à exécuter : le serveur exécute TA%d\n", get_atask_from_num(params.a_rdy[0])->num);
        exec_a();
    }
    else
    {
        printf("Rien à faire ce cycle\n");
        printf("Ordonnancement trouvé en %d cycles\n ", params.curr_cycle - 1 - params.srv.r0);
        
        return 1;
    }
    
    return 0;
}

int main(int argc, char *argv[])
{
    if(argc != 4)
    {
        usage(argv[0]);
    }
    parse_args(argv);
	
//    a_tache *taches_aperiodiques;
//    p_tache *taches_periodiques;

//    params.a = taches_aperiodiques;
//    params.p = taches_periodiques;

    params.curr_cycle = params.srv.r0;

    read_conf();

	if(CNS(params.p, params.p_size) == -1)
	{
		printf("Non ordonnançable.\n");
		return 1;
	}
	else
	{
		printf("CNS vérifiée : ordonnançable.\n");
	}

    int success = 0;

    while(!success && params.curr_cycle < max_cycles)
    {
        printf("\n\n----------CYCLE NUMERO %d---------- \n\n", params.curr_cycle);
        success = cycle();
        params.curr_cycle++;
    }

    if(!success)
    {
        printf("Pas trouvé d'ordonnancement en moins de %d cycles", max_cycles - params.srv.r0);
    }
    
    free(params.a);
    free(params.p);
	free(params.a_rdy);
	
    return 0;
}
