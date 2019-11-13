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

void handleMSGPertes( int sig ) {

    reenvoie = 1;
}

void handlerStop( int sig ) {

    continu = 0;
}

int main () {

    messages_initialiser_attente();

    requete_t req;
    req.corps.dossard = getpid();
    req.corps.etat = EN_COURSE;
    req.type = PC_COURSE;

    reponse_t rep;
    ack_t ack;

    int id = msgget( 12, 0666);

    if ( id == -1 ) {

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

    sigaction( SIGINT, &act, NULL );

    act.sa_handler = handleMSGPertes;
    sigemptyset( &( act.sa_mask) );
    act.sa_flags = 0;

    sigaction( SIGALRM, &act, NULL );

    continu = 1;

    do {

        msgsnd( id, &req, sizeof( struct corps_requete ), 0);

        alarm( ATTENTE_AVANT_PERTE );

        while ( msgrcv( id, &rep, sizeof( struct corps_reponse), getpid(), IPC_NOWAIT ) <= 0 ) {

            sleep(1);

            if ( reenvoie ) {

                printf("Le message a été perdu... ré-envoie\n");
                msgsnd( id, &req, sizeof( struct corps_requete ), 0);
                reenvoie = 0;

                alarm(ATTENTE_AVANT_PERTE);
            }
        }

        alarm(0);

        messages_afficher_reponse( &rep);
        messages_afficher_parcours( &rep);

        messages_attendre_tour();
    } while ( continu && rep.corps.etat != DECANILLE && rep.corps.etat != ARRIVE );

    if ( continu == 0 ) {

        req.corps.etat = ABANDON;
        msgsnd( id, &req, sizeof( struct requete), 0);
        msgrcv( id, &ack, sizeof( ack_t), getpid(), 0);

        messages_afficher_erreur( ack.ack );
    }

    exit(0);
}