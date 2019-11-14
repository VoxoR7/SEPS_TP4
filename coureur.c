#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "messages.h"

#define ATTENTE_AVANT_PERTE 12

short continu, reenvoie;

void handleMSGPertes( int sig ) { /* en cas de perte de message, on demande le ré-envoie d'un message */

    reenvoie = 1;
}

void handlerStop( int sig ) { /* pour le ^C */

    continu = 0;
}

int main () {

    messages_initialiser_attente();

    requete_t req;
    req.corps.dossard = getpid(); /* initialisation de la requete */
    req.corps.etat = EN_COURSE;
    req.type = PC_COURSE;

    reponse_t rep;
    ack_t ack;

    int id = msgget( 12, 0666); /* on recupere l'id de la BAL */

    if ( id == -1 ) { /* on verifie que la BAL existe bien */

        if ( errno == ENOENT )
            printf("La cle de la boite au lettre n'existe pas\n");
        else
            printf("Une erreur inatendu est survenu\n");
        exit(1);
    }

    struct sigaction act;

    act.sa_handler = handlerStop;
    sigemptyset( &( act.sa_mask) );
    act.sa_flags = 0;

    sigaction( SIGINT, &act, NULL ); /* capture du controle C */

    act.sa_handler = handleMSGPertes;
    sigemptyset( &( act.sa_mask) );
    act.sa_flags = 0;

    sigaction( SIGALRM, &act, NULL ); /* nous permet de detecter les pertes de message */

    continu = 1;

    do {

        if ( msgsnd( id, &req, sizeof( struct corps_requete ), 0)) {/* envoie du premier message */

            printf("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
            exit(2);
        }

        alarm( ATTENTE_AVANT_PERTE ); /* delai avant que l'on considére que le message a été perdu */

        while ( msgrcv( id, &rep, sizeof( struct corps_reponse), getpid(), IPC_NOWAIT ) <= 0 ) { /* tant que le serveur nous a pas repondu */

            sleep(1); /* on test la reponse du serveur toutes les 1 secondes */

            if ( reenvoie ) { /* si l'alarme s'est déclanché, le message est considérer comme perdu, on renvoie donc le message */

                printf("Le message a été perdu... ré-envoie\n");
                msgsnd( id, &req, sizeof( struct corps_requete ), 0);
                reenvoie = 0;

                alarm(ATTENTE_AVANT_PERTE);
            }
        }

        alarm(0); /* arret de l'alarme */

        messages_afficher_reponse( &rep);
        messages_afficher_parcours( &rep);

        messages_attendre_tour();
    } while ( continu && rep.corps.etat != DECANILLE && rep.corps.etat != ARRIVE );

    if ( continu == 0 ) { /* si on a abandoné */

        req.corps.etat = ABANDON;
        if ( msgsnd( id, &req, sizeof( struct requete), 0)) { /* on envoie la requete d'abandon au serveur */

            printf("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
            exit(2);
        }

        msgrcv( id, &ack, sizeof( ack_t), getpid(), 0); /* on attent la confirmation du serveur et on quitte */

        messages_afficher_erreur( ack.ack );
    }

    exit(0);
}