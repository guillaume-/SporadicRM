/* DESCHASTRES Thomas
 * RIPOLL Guillaume
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FICHIER_CONF "conf_sporadic"

#define CONF_ERROR 0

typedef struct
{
    int charge; //temps d'exécution
    int t; //date de début
    int num; //numéro de la tâche
} a_tache;

typedef struct
{
    int charge; //temps d'exécution
    int t; //periode
    int num; //numéro de la tache
} p_tache;


typedef struct sporadic_server{
    int r0;
    int Cs;
    int Ps;
} Server;


void error(int type)
{
    switch(type)
    {
        case CONF_ERROR :
        printf("Erreur dans le fichier de configuration");
        break;

        default :
        printf("Erreur");
        break;
    }
}

//lit le fichier de configuration et remplit les deux tableaux reçus en paramètre
void read_conf(a_tache **taches_a, p_tache **taches_p)
{
    //on ouvre le fichier de config
    FILE *conf;
    conf = fopen(FICHIER_CONF, "r+");

    //on prépare le buffer qui contiendra les lignes qu'on va lire une par une
    char buffer[255];

    //les tableaux auront une taille initiale correspondant à 5 tâches chacun
    int taches_a_size = 5;
    int taches_p_size = 5;

    //ces deux indices servent à retenir combien de tâches sont déjà présentes dans le tableau
    int num_taches_a = 0;
    int num_taches_p = 0;

    //on alloue la taille de chaque tableau
    *taches_a = calloc(taches_a_size, sizeof(a_tache));
    *taches_p = calloc(taches_p_size, sizeof(p_tache));

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
            (*taches_p)[num_taches_p].num = num - '0';
            (*taches_p)[num_taches_p].charge = atoi(charge);
            (*taches_p)[num_taches_p].t = atoi(t);

            num_taches_p++;
        }
        else if(buffer[0] == 'A')
        {
            poscoma = strchr(buffer, ',') - buffer;
            posbrace = strchr(buffer, '}') - buffer;

            strncpy(charge, buffer + 4, poscoma - 4);
            strncpy(t, buffer + poscoma + 1, posbrace - poscoma + 1);

            num = buffer[1];


            (*taches_a)[num_taches_a].num = num - '0';
            (*taches_a)[num_taches_a].charge = atoi(charge);
            (*taches_a)[num_taches_a].t = atoi(t);

            num_taches_a++;
        }
        else
        {
            error(CONF_ERROR);
        }
    }

    fclose(conf);
}

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
    {
        usage(argv[0]);
    }
    parseArgs(argv, &srv);

    a_tache *taches_aperiodiques;
    p_tache *taches_periodiques;

    read_conf(&taches_aperiodiques, &taches_periodiques);

    return 0;
}
