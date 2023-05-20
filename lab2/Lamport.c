#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <signal.h>

int M;
int N;
int id_memorije;
int *A;
_Atomic int *BROJ;
_Atomic int *ULAZ;


void *proces(void *i);
void KO(int i);
void NKO(int i);

void *proces(void *i){ 
    int id = *((int*)i);
    for (int k=0; k<M; k++){
        KO(id);
        NKO(id);
    }
}

void KO(int i){
    ULAZ[i] = 1;
    int najveci = -1;
    for (int k=0; k<N; k++) {
        if (k == 0){
            najveci = BROJ[0];
        }
        if (najveci < BROJ[k]) {
            najveci = BROJ[k];
        }
    }
    BROJ[i] = najveci + 1;
    ULAZ[i] = 0;

    for (int j=0; j<N; j++){
        while(ULAZ[j] != 0){}
        while(BROJ[j] != 0 && (BROJ[j] < BROJ[i] || (BROJ[j] == BROJ[i] && j < i)) ){}
    }
    *A = *A + 1; 
    printf("Dretva je %2d || A = %d\n", i, *A);
    sleep(1);
}


void NKO(int i){
    BROJ[i] = 0;
}

void brisiA(int sig){
   (void) shmdt((char *) A);
   (void) shmctl(id_memorije, IPC_RMID, NULL);
   exit(0);
}
void brisiBROJ(int sig){
   (void) shmdt((char *) BROJ);
   (void) shmctl(id_memorije, IPC_RMID, NULL);
   exit(0);
}
void brisiULAZ(int sig){
   (void) shmdt((char *) ULAZ);
   (void) shmctl(id_memorije, IPC_RMID, NULL);
   exit(0);
}

int main(){

    printf("\nUnesite željenu vrijednost za M: ");
    scanf("%d", &M);
    printf("Unesite željenu vrijednost za N: ");
    scanf("%d", &N);
    printf("\n");

    pthread_t thr_id[N];

    // ZAJEDNIČKA MEMORIJA - POČETAK
    id_memorije = shmget(IPC_PRIVATE, sizeof(int), 0600);
    if(id_memorije == -1){
        exit(1); 
    }

    A = (int *) shmat(id_memorije, NULL, 0);
    *A = 0;

    _Atomic int (*POM1)[N];
    _Atomic int (*POM2)[N];
    POM1 = (_Atomic int (*)[N])(A + 1);
    BROJ = (_Atomic int *)(POM1 + 1);
    POM2 = (_Atomic int (*)[N])(POM1 + 1);
    ULAZ = (_Atomic int *)(POM2 + 1);

    for(int k=0; k<N; k++){
        BROJ[k] = 0;
        ULAZ[k] = 0;
    }

    signal(SIGINT, brisiA);
    signal(SIGINT, brisiBROJ);
    signal(SIGINT, brisiULAZ);
    // ZAJEDNIČKA MEMORIJA - KRAJ

    int listaID[N];
    for(int i=0; i<N; i++){
        listaID[i] = i;
        if(pthread_create(&thr_id[i], NULL, proces, &listaID[i]) != 0){
            printf("Greska pri stvaranju dretve!\n");
            exit(1);
        }
    }

    for (int i = 0; i < N; i++) {
        pthread_join(thr_id[i], NULL);
    }

    printf("\nZajednička varijabla A na kraju iznosi %d\n\n", *A);

    return 0;
}