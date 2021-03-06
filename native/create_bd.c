/**
 * \file create_bd.c
 * \brief Pars le dociment et stock toutes les valeurs en memoire.
 * \author Mickael PURET
 * \date 21 mars 2011
 *
 */

#include "create_bd.h"
#include "erreur.h"
#include "use_bd.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

/**
 * \fn static FILE * open_file(char * path)
 * \brief ouvre un fichier et retourne un pointeur sur une structure FILE.
 *
 * \param chaine de caractères indiquant le arc du fichier à ouvrir.
 * \return Un pointeur sur une structure FILE.
 */
static FILE * open_file(char * path){
    FILE * file;
    if( (file = fopen(path, "rt")) == NULL ) fatalerreur("open_file : ouverture du fichier imposible");
    return file;
}

/**
 * \fn static char * readLine(FILE * fichier)
 * \brief lit une ligne d'au maximum 1024 caractères et la retourne sous forme d'une chaine alloué sur le tas.
 *
 * \param Un pointeur sur une structure FILE valide.
 * \return une chaine de caractères.
 */
static char * readLine(FILE * fichier) {
    char buf[1024];
    char * result = strdup(fgets(buf, 1024, fichier));
    return result;
}

/**
 * \fn static void ajout_parametre(Donnee * data, FILE * file)
 * \brief Lit la ligne contenant les parametres du programme (temps d'execution, nombre de lieux, nombre d'arc) et les stock dans la vaiable data.
 *
 * \param Un pointeur sur une structure Donnee.
 * \param Un pointeur sur une structure FILE.
 */
static void ajout_parametre(Donnee * data, FILE * file){
    fscanf(file, "%d;%d;%d", &data->temps_execution, &data->nb_lieux_total, &data->nb_arcs);
    /* les nombres d'elements doivent contenir le nombre exacte*/
    data->nb_arcs++;
    data->nb_lieux_total++;
}

/**
 * \fn static void ajout_lieux(Donnee * data, FILE * file)
 * \brief Ajoute l'identifiant, le nom, le nombre d'arcs et l'interet de chaques lieu dans la table Lieux.
 *
 * \param Un pointeur sur une structure Donnee.
 * \param Un pointeur sur une structure FILE.
 */
static void ajout_lieux(Donnee * data, FILE * file){
    char nom_lieu[1024];
    int num_lieu, intret_lieu, id = 0;

    /* allocation du tableau de type Lieu*/
    if(data->lieux == NULL) if((data->lieux = (Lieu*)malloc(data->nb_lieux_total*sizeof(Lieu))) == NULL)  fatalerreur("ajout_lieux : la creation du table lieux a echoué\n");

    /* initialisation des tableau lieux et interet*/
    while(fscanf(file, "%d;%d;%s\n", &num_lieu, &intret_lieu, nom_lieu) == 3){
        data->lieux[num_lieu].interet = intret_lieu;
        data->lieux[num_lieu].nom = strdup(nom_lieu);
        data->lieux[num_lieu].nb_arc = 0;
        data->lieux[num_lieu].id = id++;
    }
}

/**
 * \fn static void ajout_arcs(Donnee * data, FILE * file)
 * \brief Ajoute les arcs(lieu de depart, distance, insecurité, lieu d'arrivé) dans la variable data. C'est information contenue dans le tableau map sont classé en fonction de le lieu de depart, de l'interet du lieu d'arrivé (decroissant), de la distance (croissant) puis de l'insecurité (croissant). Les doublons et les dominés ont été suprimé.
 *
 * \param Un pointeur sur une structure Donnee.
 * \param Un pointeur sur une structure FILE.
 */
static void ajout_arcs(Donnee * data, FILE * file){
    int nb_element = data->nb_arcs / data->nb_lieux_total + 1;
    Arc **ch_tmp;
    int i = 0, l1, l2, distance, insecurite;
    int * nb_element_lieu;

    /* allocation du tableau map a deux dimantions. cette table grandi en fonction de nb_element et du nombre d'arcs pour un lieu*/
    data->map = (Arc ***)malloc(data->nb_lieux_total*sizeof(Arc**));
    if(data->map == NULL) fatalerreur("ajout_arcs : echec de l'allocation de map lv1");

    for(i = 0;  i < data->nb_lieux_total; ++i){
        data->map[i] = (Arc**)malloc(nb_element*sizeof(Arc*));
        if(data->map[i] == NULL) fatalerreur("ajout_arcs : echec de l'allocation de map lv2");
    }

    /* allocation d'un tableau temporaire servent a gere l'alloaction du nombre d'arcs de chaque lieu.*/
    nb_element_lieu = (int *)malloc((data->nb_lieux_total)*sizeof(int));
    if(nb_element_lieu == NULL) fatalerreur("ajout_arcs : echec de l'allocation de nb_element_lieu");

    /* initialisation de cette table a deux dimantions (nombre d'element x nombre d'element max)*/
    for(i = 0; i < data->nb_lieux_total; ++i)
        nb_element_lieu[i] = 0;

    /* lecture du fichier ligne a ligne et traitement des informations*/
    while(fscanf(file, "%d;%d;%d;%d\n", &l1, &l2, &distance, &insecurite) == 4){
        /*si le nombre d'arcs enregistre est superieur ou egale au nombre d'element disponible, on augmante le nombre delements de nb_element*/
        if(data->lieux[l1].nb_arc >= nb_element_lieu[l1]){
            ch_tmp = (Arc**)realloc(data->map[l1], (nb_element + nb_element_lieu[l1])*sizeof(Arc*));
            if(ch_tmp == NULL) fatalerreur("ajout_arcs : echec de l'allocation de map lv2");
            data->map[l1] = ch_tmp;

            nb_element_lieu[l1] += nb_element; /*gaumentation du nombre d'elements possible*/
        }

        data->map[l1][data->lieux[l1].nb_arc] = (Arc *)malloc(sizeof(Arc));
        if(data->map[l1][data->lieux[l1].nb_arc] == NULL) fatalerreur("ajout_arcs : allocation de l'arc");
        /* on instrit l'arc a son lieu de depart*/
        data->map[l1][data->lieux[l1].nb_arc]->depart = &data->lieux[l1];
        data->map[l1][data->lieux[l1].nb_arc]->insecurite = insecurite;
        data->map[l1][data->lieux[l1].nb_arc]->distance = distance;
        data->map[l1][data->lieux[l1].nb_arc]->destination = &data->lieux[l2];
        data->lieux[l1].nb_arc++; /*gaumentation du nombre d'arcs enregirte*/

        /*si le nombre d'arcs enregistre est superieur ou egale au nombre d'element disponible, on augmante le nombre delements de nb_element*/
        if(data->lieux[l2].nb_arc >= nb_element_lieu[l2]){
            ch_tmp = (Arc**)realloc(data->map[l2], (nb_element + nb_element_lieu[l2])*sizeof(Arc*));
            if(ch_tmp == NULL) fatalerreur("ajout_arcs : echec de l'allocation de map lv2");
            data->map[l2] = ch_tmp;

            nb_element_lieu[l2] += nb_element; /*gaumentation du nombre d'elements possible*/
        }

        data->map[l2][data->lieux[l2].nb_arc] = (Arc *)malloc(sizeof(Arc));
        if(data->map[l2][data->lieux[l2].nb_arc] == NULL) fatalerreur("ajout_arcs : allocation de l'arc");
        /* on instrit l'arc a son lieu de d'arrive*/
        data->map[l2][data->lieux[l2].nb_arc]->depart = &data->lieux[l2];
        data->map[l2][data->lieux[l2].nb_arc]->insecurite = insecurite;
        data->map[l2][data->lieux[l2].nb_arc]->distance = distance;
        data->map[l2][data->lieux[l2].nb_arc]->destination = &data->lieux[l1];
        data->lieux[l2].nb_arc++; /*gaumentation du nombre d'arcs enregirte*/
    }

    /* sauvgarde du nombre d'arc dans la table definiassant les lieux et tri les arcs de map*/
    free(nb_element_lieu);
}

/**
 * \fn int epure_map(Donnee *data,int id_lieu)
 * \brief supprime les arcs domminés du lieu.
 *
 * \param Un pointeur sur une structure Donnee.
 * \param l'indentifiant du lieu.
 * \return le nombre d'arcs restant en depart du lieu.
 */
int epure_map(Donnee *data,int id_lieu){
    int nbre_arc = nb_arc(data, id_lieu);
    int id_arc, id_key = 0, id_cpy = 1;

    int arc_interet, arc_distance, arc_insecurite;

    /*recuperation des valeurs de la clef*/
    int key_interet = interet_destination(data, id_lieu, id_key);
    int key_distance = distance_arc(data, id_lieu, id_key);
    int key_insecurite = insecurite_arc(data, id_lieu, id_key);

    /*lecture de tous le tableau*/
    for(id_arc = 1 ; id_arc < nbre_arc ; ++id_arc){
        /*si l'arc n'existe pas, on prends le suivant*/
        while((id_arc != nbre_arc)&&(pointeur_map_arc(data, id_lieu, id_arc) == NULL)) id_arc++;

        /*recuperation des valeurs de l'arc*/
        arc_interet = interet_destination(data, id_lieu, id_arc);
        arc_distance = distance_arc(data, id_lieu, id_arc);
        arc_insecurite = insecurite_arc(data, id_lieu, id_arc);

        /*si l'arc est domine, on le suprime*/
        if((arc_interet == key_interet)&&(arc_distance >= key_distance)&&(arc_insecurite >= key_insecurite)){
            spr_pointeur_map_arc(data, id_lieu, id_arc);
            maj_pointeur_map_arc(data, id_lieu, id_arc, NULL);
        }
        else{
            /* l'inetere de la clef est differnts de celui de l'arc, on deplace la clef de un*/
            if(arc_interet < key_interet){
                /*si l'arc n'existe pas, on prends le suivant*/
                while((id_key != nbre_arc)&&(pointeur_map_arc(data, id_lieu, id_key) == NULL)) ++id_key;

                /*recuperation des valeurs de la clef*/
                key_interet = interet_destination(data, id_lieu, id_key);
                key_distance = distance_arc(data, id_lieu, id_key);
                key_insecurite = insecurite_arc(data, id_lieu, id_key);
            }

            /* s'il y a des arc de supprimes, on comble les trous*/
            if(id_arc != id_cpy){
                if(pointeur_map_arc(data, id_lieu, id_cpy) != NULL) spr_pointeur_map_arc(data, id_lieu, id_cpy);
                maj_pointeur_map_arc(data, id_lieu, id_cpy, pointeur_map_arc(data, id_lieu, id_arc));
                maj_pointeur_map_arc(data, id_lieu, id_arc, NULL);

                ++id_cpy;
            }
        }
    }
    return id_cpy;
}

/**
 * \fn static int position_arc(const void *p1, const void *p2).
 * \brief indique si l'arc p1 doit se trouver avant l'arc p2 selon :
 * p1.destination.interet >= p2.destination.interet et
 * p1.distance <= p2.distance et
 * p2.insecurité <= p2.insecurité.
 *
 * \param pointeur sur l'arc 1.
 * \param pointeur sur l'arc 2.
 */
static int position_arc(const void *p1, const void *p2){
    Arc *a = *((Arc* const*)p1);
    Arc *b = *((Arc* const*)p2);

    /*comparaison*/
    if(a->destination->interet > b->destination->interet)
        return -1;

    if(a->destination->interet == b->destination->interet){
        if(a->distance < b->distance)
            return -1;
        if(a->distance == b->distance)
            if(a->insecurite <= b->insecurite)
                return -1;
    }
    return 1;
}

static int interet_decroissant(const void *interet1, const void *interet2){
    int a = ((Interet_lieu const*)interet1)->interet;
    int b = ((Interet_lieu const*)interet2)->interet;
    return b - a;
}

static void creat_interet(Donnee *data){
    int i;

    data->table_interet = (Interet_lieu*)malloc(data->nb_lieux_total*sizeof(Interet_lieu));
    if(data->table_interet == NULL) fatalerreur("creat_interet : echec de l'allocation de la table table_interet");

    for(i = 0; i < data->nb_lieux_total; ++i){
        data->table_interet[i].interet = data->lieux[i].interet;
        data->table_interet[i].lieu = &data->lieux[i];
        printf("creat_interet : table_interet %d id %d\n", data->table_interet[i].interet, data->table_interet[i].lieu->id);
    }
}

static void create_index(Donnee *data, int id_lieu){
    int i, last_lieu;
    Arc **map = data->map[id_lieu];
    if(data->index_lieu == NULL){
        data->index_lieu = (Index_arc***)malloc(data->nb_lieux_total * sizeof(Index_arc**));
        if(data->index_lieu == NULL)
            fatalerreur("create_index : creation de la table d'index impossible");

        for(i = 0; i < data->nb_lieux_total; ++i)
            data->index_lieu[i] = NULL;
    }

    if(data->index_lieu[id_lieu] == NULL){
        data->index_lieu[id_lieu] = (Index_arc**)malloc(data->nb_lieux_total * sizeof(Index_arc*));
        if(data->index_lieu[id_lieu] == NULL)
            fatalerreur("create_index : creation de la table d'index impossible");

        //memset(data->index_lieu[id_lieu], NULL, data->nb_lieux_total * sizeof(Index_arc*));
        for(i = 0; i < data->nb_lieux_total; ++i)
            data->index_lieu[id_lieu][i] = NULL;
    }

    data->index_lieu[id_lieu][map[0]->destination->id] = (Index_arc*)malloc(sizeof(Index_arc));
    data->index_lieu[id_lieu][map[0]->destination->id]->id_arc = 0;
    last_lieu = 0;

    for(i = 1; i < data->lieux[id_lieu].nb_arc; ++i){
        if(map[last_lieu]->destination != map[i]->destination){
            data->index_lieu[id_lieu][map[i]->destination->id] = (Index_arc*)malloc(sizeof(Index_arc));
            data->index_lieu[id_lieu][map[i]->destination->id]->id_arc = i;

            data->index_lieu[id_lieu][map[last_lieu]->destination->id]->nb_arc = i - last_lieu;

            last_lieu = i;
        }

    }

    data->index_lieu[id_lieu][map[last_lieu]->destination->id]->nb_arc = i - last_lieu;
}

/**
 * \fn static void ajout_arcs(Donnee * data, FILE * file)
 * \brief Ouvre le fichier et coordonne les differantes fonctions parmetant le stockage des informations en memoire.
 *
 * \param Une chaine de caractères designant le fichier contenant les données.
 */
Donnee * main_create_db(char * path){
    char * line ;
    int i;
    Donnee * data;

    /*allocation de la structure contenent toutes les donnees et initialisation*/
    data = (Donnee *)malloc(sizeof(Donnee));

    data->temps_execution = 0;
    data->nb_lieux_total = 0;
    data->nb_arcs = 0;
    data->lieux = NULL;
    data->index_lieu = NULL;
    data->map = NULL;
    data->solution = NULL;
    data->table_interet = NULL;

    /*ouvreture du fichier*/
    FILE * file = open_file(path);

    /*parcour de tous le fichier*/
    while(!feof(file)){
        /*lit les gigne une a une*/
        line = readLine(file);

        /*si la ligne commance par un # on utilise la fonction corespondane*/
        if(line[0] == '#'){
            if( (strcasecmp(line + 1, "Parametres\n") == 0) ) ajout_parametre(data, file);
            else if( (strcasecmp(line + 1, "Lieux\n") == 0) ) ajout_lieux(data, file);
                else if( (strcasecmp(line + 1, "Arcs\n") == 0) ) ajout_arcs(data, file);
                    else fatalerreur("main_creat_db : aucun traitement pour cette ligne\n");
        }
        free(line);
    }

    creat_interet(data);
    qsort(data->table_interet, data->nb_lieux_total, sizeof(Interet_lieu), interet_decroissant);

    for( i = 0; i < data->nb_lieux_total; ++i){
        /* tri des arcs*/
        //quicksort_map(data, i, 0, data->lieux[i].nb_arc-1);
        qsort(data->map[i], data->lieux[i].nb_arc, sizeof(Arc*), position_arc);

        /*supression des arcs domine*/
        data->lieux[i].nb_arc = epure_map(data, i);

        create_index(data, i);
    }
    return data;
}

/**
 * \fn static void free_db(Donnee * data)
 * \brief supprime toutes les données du tas.
 *
 * \param la tructure Donnée aloué sur le tas.
 */
void free_db(Donnee * data){
    int i,j;

    /*liberation de toutes les tables*/
    for(i = 0; i < data->nb_lieux_total; ++i){
        free(data->lieux[i].nom);

        for(j = 0; j < data->nb_lieux_total; ++j)
            if(data->index_lieu[i][j] != NULL)
                free(data->index_lieu[i][j]);

        for(j = 0; j < data->lieux[i].nb_arc; ++j)
            free(data->map[i][j]);

        free(data->index_lieu[i]);
        free(data->map[i]);
    }
    /*liberation de leur entet*/
    free(data->table_interet);
    free(data->index_lieu);
    free(data->lieux);
    free(data->map);
    free(data);

}
