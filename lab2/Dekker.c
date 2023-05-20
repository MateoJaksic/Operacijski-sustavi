#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <stdatomic.h>
#include <string.h>

int M;
int *A;
_Atomic int *pravo; 
int (*C)[2];
_Atomic int *zastavica;
int id_memorije;

void proces(int i);
void KO(int i, int j);
void NKO(int i, int j);

void proces(int i){
    for(int k=0; k<M; k++){
        KO(i, 1-i); 
        NKO(i, 1-i);
    }
    exit(0);
}

void KO(int i, int j){
    zastavica[i] = 1;
    while(zastavica[j] != 0){
        if(*pravo == j){
            zastavica[i] = 0;
            while(*pravo == j){
                continue;
            }
            zastavica[i] = 1;
        }
    }
    *A = *A + 1;
    printf("Proces = %d || A = %d\n", i, *A);
    sleep(1);
}

void NKO(int i, int j){
    *pravo = j;
    zastavica[i] = 0;
}

void brisiA(int sig){
   (void) shmdt((char *) A);
   (void) shmctl(id_memorije, IPC_RMID, NULL);
   exit(0);
}

void brisiPravo(int sig){
   (void) shmdt((char *) pravo);
   (void) shmctl(id_memorije, IPC_RMID, NULL);
   exit(0);
}

void brisiZastavice(int sig){
   (void) shmdt((char *) zastavica);
   (void) shmctl(id_memorije, IPC_RMID, NULL);
   exit(0);
}



int main(){

    printf("\nUnesite željenu vrijednost za M: ");
    scanf("%d", &M);
    printf("\n");

    pid_t p[2];
    int i;

    id_memorije = shmget(IPC_PRIVATE, sizeof(int), 0600);
    
    if(id_memorije == -1){
        exit(1); 
    }

    A = (int *) shmat(id_memorije, NULL, 0);
    *A = 0;

    pravo = (_Atomic int *) A + 1;
    *pravo = -1;

    C = (int (*)[2])(pravo + 1);
    *C[0] = 0;
    *C[1] = 0;

    zastavica = (_Atomic int *)(C + 1);
    zastavica[0] = 0;
    zastavica[1] = 0;


    signal(SIGINT, brisiA);
    signal(SIGINT, brisiPravo);
    signal(SIGINT, brisiZastavice);

    for(i=0; i<2; i++){
        switch (p[i] = fork()){
            case -1:
                fprintf(stderr, "Nije moguće stvoriti proces.");
                exit(1);
            case 0:
                proces(i);
        }
    }

    while(i--) {
        wait(NULL);
    }

    printf("\nZajednička varijabla A na kraju iznosi %d\n\n", *A);

    return 0;
}