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
#define NB_DEPS 5
#define NB_RSRC 5

typedef char bool;
struct rsrc;
struct task;

typedef struct util
{
	struct task *p;
	struct rsrc *r;
	int duree;
	int cycle_appel;
} utilisation;

typedef struct rsrc
{
	bool is_used;
	struct task *used_by;
} ressource;

typedef struct task
{
	int r0; //cycle de début
	int curr_charge;
	int charge; //temps d'exécution
	int p; //période
	int num; //numéro de la tâche
	int deps[NB_DEPS];//pour l'instant on dit qu'une tâche ne peut pas avoir plus de 5 dépendances
	int nb_deps;
	int nb_rsrc;
	utilisation util_ressources[NB_RSRC]; // ressources
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
	int *a_rdy; // ready to execute <=> not yet executed
	int curr_cycle;
	int nb_ressources;
	int blocage_max;
	Server srv;
	ressource ressources[NB_RSRC];
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
			printf("Erreur lors de l'exécution des cycles");
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
	params.srv.r0 = sscanf("%d", argv[1]);
	params.srv.Cs = sscanf("%d", argv[2]);
	params.srv.Ps = sscanf("%d", argv[3]);
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
 * La charge du serveur diminue au lancement d'une tâche apériodique de sa charge,
 * mais augmente après ce lancement avec un délai de serveur.periode cycles
 * A n'appeler que lors du lancement d'une tâche apériodique
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

void adjust_params()
{
	int periodes[params.p_size];
	int max_r_zero[params.p_size];
	
	for(int i = 0; i < params.p_size; i++)
	{
		periodes[i] = 0;
		max_r_zero[i] = 0;
	}
	
	int cur = 0;
	
	//pour chaque tache périodique
	for(int i = 0; i < params.p_size; i++)
	{
		//on va regarder si on a déjà enregistré des tâches ayant cette période
		for(int j = 0; j < params.p_size; j++)
		{
			//si non on l'écrit dans la première case vide qu'on rencontre
			if(periodes[j] == 0)
			{
				periodes[j] = params.p[i].p;
				cur++;
				break;
			}
			if(periodes[j] == params.p[i].p)
			{
				break;
			}
		}
	}

	//pour chaque période enregistrée
	for(int i = 0; i < cur; i++)
	{
		//on parcoure le tableau des tâches pour trouver le r0 maximum pour une période
		for(int j = 0; j < params.p_size; j++)
		{
			if(params.p[j].p == periodes[i] && params.p[j].r0 > max_r_zero[i])
			{
				max_r_zero[i] = params.p[j].r0;
			}
		}
	}

	//enfin on traverse le tableau des tâches pour mettre toutes les tâches de même période au même r0
	for(int i = 0; i < cur; i++)
	{
		for(int j = 0; j < params.p_size; j++)
		{
			if(params.p[j].p == periodes[i])
			{
				params.p[j].r0 = max_r_zero[i];
			}
		}
	}
}

void acces_ressource(p_tache *p, ressource *r)
{
	r->used_by = p;
	r->is_used = 1;
}

void liberer_ressource(p_tache *p, ressource *r)
{
	r->used_by = NULL;
	r->is_used = 0;
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
	char type = ' ';

	params.a = (a_tache*)calloc(taches_a_size, sizeof(a_tache));
	params.p = (a_tache*)calloc(taches_p_size, sizeof(p_tache));

	if(conf == NULL)
	{
		perror("ERR");
		exit(EXIT_FAILURE);
	}
	while(!feof(conf))
	{
		fscanf(conf, "%s", buffer);

		if(buffer[0] == 0)
		{
			break;
		}
		if(buffer[0] != '#')
		{
			if(sscanf(buffer, "T%c", &type))
			{
				if(type == 'P')
				{
					int posdep = 0;
					p_tache *p = &(params.p[num_taches_p]);
					
					//scanne la string et met directement les paramètres dans la tâche
					sscanf(buffer, "TP%d={%d,%d,%d}%n", &p->num, &p->charge, &p->p, &p->r0, &posdep);
					
					p->curr_charge = p->charge;
					
					//ici on parcoure les dépendances et on remplit le tableau de dépendances de la tâche
					if(buffer[posdep] == '[')
					{
						int nb_deps = ((int)strlen(buffer + posdep) - 1)/4;
						
						for(int i = 0; i < nb_deps; i++)
						{
							//TPnum={charge,période,r0}[TPX,TPY...]
							//^buffer			posdep^  ^ posnum
							char* posnum = buffer + posdep + 1 + 4 * i;
							sscanf(posnum, "TP%d", &p->deps[i]);
							p->nb_deps = nb_deps;
						}
					}
					
					num_taches_p++;
				}
				else if(type == 'A')
				{
					a_tache *a = &params.a[num_taches_a];
					sscanf(buffer, "TA%d={%d,%d,%d}", &a->num, &a->charge, &a->p, &a->r0);
					
					a->curr_charge = a->charge;
					
					num_taches_a++;
				}
				else
				{
					error(CONF_ERROR);
				}
			}			
		}
	}
	params.a_rdy = calloc(num_taches_a, sizeof(int));

//DEBUG
	for(int i = 0; i < num_taches_p; i++)
	{
		printf("TP%d charge = %d p = %d r0 = %d\n", params.p[i].num, params.p[i].charge, params.p[i].p, params.p[i].r0);
	}
	for(int i = 0; i < num_taches_a; i++)
	{
		printf("TA%d charge = %d r0 = %d\n", params.a[i].num, params.a[i].charge, params.a[i].p);
	}
	params.a_size = num_taches_a;
	params.p_size = num_taches_p;
	fclose(conf);
}

/* iterate_CNS
 * calcule une itération du théorème de Lehoczky et al.
*/
double iterate_CNS(int i, int k, int m, p_tache *tasks, int nb_tasks)
{
	double d = 0.;
	for(int j = 0; j <= i; ++j){
		d += tasks[j].charge * ((m * tasks[k].p / tasks[j].p) +1);
	}
	d /= (m * tasks[k].p);
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

p_tache *get_ptask_from_num(int num)
{
	for(int i = 0; i < params.p_size; i++)
	{
		if(params.p[i].num == num)
		{
			return &params.p[i];
		}
	}
	return 0;
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
	return 0;
}

//une tâche périodique est disponible si elle a encore de la charge à exécuter
int available(p_tache *p)
{
	if(p->curr_charge > 0 && params.curr_cycle >= p->r0)
	{
		if(p->nb_deps > 0)
		{
			p_tache *dep;
			for(int i = 0; i < p->nb_deps; i++)
			{
				dep = get_ptask_from_num(p->deps[i]);
				
				if(dep->curr_charge > 0)
				{
					printf("TP%d ne peut pas s'exécuter car elle dépend de TP%d\n", p->num, dep->num);
					return 0;
				}
			}
		}
		return 1;
	}
	return 0;
}

/* Take a priority task considering RM sporadic, and check rsrc, update the task if needed
 * Detects interblocked by ressource processes
 * returns 1 if periodic, 0 else
*/
bool get_task_considering_rsrc(struct task **t){
	return 1;
}

void exec_p(p_tache *p)
{
	p->curr_charge--;
}

void exec_a(p_tache *a)
{
	a->curr_charge--;
	chck_charge(0, *a);
}

void exec_t(struct task *t)
{
	struct task *tRsrc = t;
	bool periodic = get_task_considering_rsrc(&tRsrc);
	char letter = periodic ? 'P' : 'A';
	if(t != tRsrc)
	{
		printf("La priorité des ressources fait executer T%c%d\n", letter, tRsrc->num);
	}
	if(periodic)
		exec_p(tRsrc);
	else
		exec_a(tRsrc);
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
		printf("La tâche TP%d doit encore s'exécuter %d cycles cette période (P = %d)\n", params.p[i].num, params.p[i].curr_charge, params.p[i].p);
		
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
			if(params.p[i].p < prio.p)
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

/* Called for aperiodic tasks with a num task as parameter */
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

//prend en paramètre une tâche p et vérifie si une nouvelle période commence pour cette tâche
int cycles_avant_periode(p_tache *p)
{	
	//si on enlève le r0 du serveur et de la tâche on obtient le numéro 
	//du cycle pour la tâche qu'on traite
	int cycle_relatif = params.curr_cycle - params.srv.r0 - p->r0;

	if(cycle_relatif - p->p < 0)
	{
		return -1;
	}
	if(cycle_relatif % p->p != 0)
	{
		return cycle_relatif % p->p;
	}
	return 0;
}

int check_tasks()
{
	int i;
		
	// on vérifie dans la liste des tâches apériodiques la ou lesquelles
	// sont supposées commencer lors de ce cycle
	for(i = 0; i < params.a_size; i++)
	{
		if(params.a[i].r0 == params.curr_cycle)
		{
			task_ready(params.a[i].num);
		}
		else if(params.srv.r0 > params.a[i].r0 && params.srv.r0 == params.curr_cycle)
		{
			task_ready(params.a[i].num);
		}
	}

	//on vérifie pour toutes les tâches périodiques si leur période recommence
	//si c'est le cas on leur rend leur charge totale à exécuter
	for(i = 0; i < params.p_size; i++)
	{
		if(params.curr_cycle >= params.p[i].r0)
		{
			//si le cycle courant est un multiple de la période de la tâche courante
			//alors la tâche récupère sa charge courante
			int c = cycles_avant_periode(&params.p[i]);
			
			if(c == 0 && params.curr_cycle != 0 + params.srv.r0)
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
		
		if(!prio)
		{
			error(EXEC_ERROR);
			return -1;
		}
		
		if(params.a_rdy[0] > 0)
		{
			a_tache *a = get_atask_from_num(params.a_rdy[0]);
			
			// Si le serveur a la priorité et a une tâche à lancer il la lance
			if((prio->p > params.srv.Ps) && a->curr_charge > 0)
			{
				printf("Le serveur a la priorité ce cycle donc on exécute la tâche apériodique TA%d à qui il reste %d à exécuter \n", a->num, a->curr_charge);
				exec_t(get_atask_from_num(params.a_rdy[0]));
			}
			else
			{
				printf("\nLe serveur n'a pas la priorité, on éxécute TP%d\n", prio->num);
				exec_t(prio);
			}
		}
		//sinon on lance la tâche périodique
		else
		{
			printf("\nLe serveur n'a pas de tâche apériodique à exécuter : on exécute TP%d\n", prio->num);

			exec_t(prio);
		}
	}
	else if(params.a_rdy[0] > 0 && get_atask_from_num(params.a_rdy[0])->curr_charge > 0 )
	{
		printf("Pas de tâche périodique à exécuter : le serveur exécute TA%d\n", get_atask_from_num(params.a_rdy[0])->num);
		exec_t(get_atask_from_num(params.a_rdy[0]));
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
	
	params.curr_cycle = params.srv.r0;

	read_conf();
	
	/*if(conf == NULL)
	{
		perror("ERR");
		exit(EXIT_FAILURE);
	}*/

	adjust_params();

	if(CNS(params.p, params.p_size) == -1)
	{
		printf("Non ordonnançable.\n");
		//return 1;
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
	if(success == -1)
	{
		printf("Erreur d'exécution, arrêt du programme.\n");
	}

	free(params.a);
	free(params.p);
	free(params.a_rdy);
	return 0;
}
