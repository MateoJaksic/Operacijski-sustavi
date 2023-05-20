#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <string.h>
#include <semaphore.h>
#include <time.h>
#include <stdbool.h>

int id_memorije;
sem_t *slobodna_mjesta; // slobodna mjesta u čekaonici
sem_t *frizerka_slobodna; // frizerka je slobodna
sem_t *cekaonica; // ima klijenata u čekaonici
sem_t *id_klijenta; // dohvaćam id klijenta
sem_t *klijente_daj_id; // postavljamo id klijenta
sem_t *radi_frizuru; // frizerka radi, pa klijent može ispisati da mu ju radi
sem_t *inicijalizacija; // inicijalizacija da klijent ne dode prije frizerke
int *identifikator; // integer kojeg koristim da klijent kaze frizerki koji je broj



void klijent(int id, int velicina_cekaonice){
    int vrijednost;
    
    sem_getvalue(slobodna_mjesta, &vrijednost);
    if(vrijednost > 0){

        sem_wait(slobodna_mjesta);          // zauzimam slobodno mjesto
        printf("        Klijent(%d): Želim na frizuru\n", id);
        printf("        Klijent(%d): Ulazim u čekaonicu (%d)\n", id, velicina_cekaonice-vrijednost+1);
        sem_post(cekaonica);                // počinjem čekam, budem signalizirao frizerki
        sem_wait(frizerka_slobodna);        // čekam dok frizerka nije slobodna
        sem_post(slobodna_mjesta);          // frizerka je slobodna, pa moje mjesto postaje slobodno
        sem_wait(klijente_daj_id);          // čekamo da možemo mijenjati identifikatorski broj
        *identifikator = id;                // zapisujem frizerki svoj id
        sem_post(id_klijenta);              // dajem znak frizerki da sam zapisao svoj broj
        sem_wait(radi_frizuru);             // sjeo sam, čekam da frizerka krene raditi frizuru
    
        printf("        Klijent(%d): Frizerka mi radi frizuru\n", id);

    }  else {
        printf("        Klijent(%d): Želim na frizuru\n", id);
        printf("        Klijent(%d): Nema mjesta u čekaonici, vratit ću se sutra\n", id);
    }
}



void frizerka(int radno_vrijeme){
    time_t pocetno_vrijeme, trenutno_vrijeme;
    int netko_ceka;
    bool frizerka_spava = true;
    int ostaje_prekovremeno;

    printf("Frizerka: Otvaram salon\n");
    printf("Frizerka: Postavljam znak OTVORENO\n");
    sem_post(frizerka_slobodna);
    time(&pocetno_vrijeme);
    printf("Frizerka: Spavam dok klijenti ne dođu\n");
    sem_post(inicijalizacija);

    do {
        sem_getvalue(cekaonica, &netko_ceka);
        if (frizerka_spava == false && netko_ceka > 0){
            sem_wait(cekaonica);                                                       
            sem_post(klijente_daj_id);
            sem_wait(id_klijenta);
            printf("Frizerka: Idem raditi na klijentu %d\n", *identifikator);
            sem_post(radi_frizuru);
            sleep(3);
            printf("Frizerka: Klijent %d gotov\n", *identifikator);
            sem_post(frizerka_slobodna);
        }
        
        sem_getvalue(cekaonica, &netko_ceka);
        if (frizerka_spava == true && netko_ceka > 0){
            frizerka_spava = false;
            sem_wait(cekaonica);                                                       
            sem_post(klijente_daj_id);
            sem_wait(id_klijenta);
            printf("Frizerka: Idem raditi na klijentu %d\n", *identifikator);
            sem_post(radi_frizuru);
            sleep(3);
            printf("Frizerka: Klijent %d gotov\n", *identifikator);
            sem_post(frizerka_slobodna);
        }

        sem_getvalue(cekaonica, &netko_ceka);
        if (frizerka_spava == false && netko_ceka == 0){
            frizerka_spava = true;
            printf("Frizerka: Spavam dok klijenti ne dođu\n");
        }

        sem_getvalue(cekaonica, &netko_ceka);
        sem_getvalue(frizerka_slobodna, &ostaje_prekovremeno);
        time(&trenutno_vrijeme);          
    } while (difftime(trenutno_vrijeme, pocetno_vrijeme) < radno_vrijeme || netko_ceka > 0 || ostaje_prekovremeno == 0);
    
    
    printf("Frizerka: Postavljam znak ZATVORENO\n");
    printf("Frizerka: Zatvaram salon\n");
    int velicina_cekaonice;
    sem_getvalue(slobodna_mjesta, &velicina_cekaonice);
    for (int i=velicina_cekaonice; i>0; i--){
        sem_wait(slobodna_mjesta);
    }

}



int main(){

    // odabir dafaultnog ili custom rasporeda
    int odabir;
    printf("\nŽelite li defaultni ili custom raspored? (defaultni raspored = 0, custom raspored = 1)\nVaš odabir je: ");
    scanf("%d", &odabir);
    
    printf("-----------------------------\n");
    
    if (odabir == 0){
        int velicina_cekaonice = 3;
        int broj_klijenata = 7;
        int radno_vrijeme = 30; // u sekundama
        int vremena_dolazaka[broj_klijenata+1];
        vremena_dolazaka[0] = 0; // frizerka
        vremena_dolazaka[1] = 0; // klijent 1
        vremena_dolazaka[2] = 0; // klijent 2
        vremena_dolazaka[3] = 0; // ...
        vremena_dolazaka[4] = 0;
        vremena_dolazaka[5] = 0;
        vremena_dolazaka[6] = 16;
        vremena_dolazaka[7] = 22; // klijent 7
        pid_t proces[broj_klijenata+1];

        // ZAJEDNIČKA MEMORIJA
        id_memorije = shmget(IPC_PRIVATE, sizeof(sem_t), 0600);
        slobodna_mjesta = shmat(id_memorije, NULL, 0);
        shmctl(id_memorije, IPC_RMID, NULL);
        sem_init(slobodna_mjesta, 1, velicina_cekaonice);

        cekaonica = (sem_t *) slobodna_mjesta + sizeof(sem_t *);
        sem_init(cekaonica, 1, 0);

        id_klijenta = (sem_t *) cekaonica + sizeof(sem_t *);
        sem_init(id_klijenta, 1, 0);

        frizerka_slobodna = (sem_t *) id_klijenta + sizeof(sem_t *);
        sem_init(frizerka_slobodna, 1, 0);

        radi_frizuru = (sem_t*) frizerka_slobodna + sizeof(sem_t *);
        sem_init(radi_frizuru, 1, 0);

        inicijalizacija = (sem_t *) radi_frizuru + sizeof(sem_t *);
        sem_init(inicijalizacija, 1, 0);

        klijente_daj_id = (sem_t *) inicijalizacija + sizeof(sem_t *);
        sem_init(klijente_daj_id, 1, 0);

        identifikator = (int *) klijente_daj_id + sizeof(sem_t *);
        *identifikator = -1; 
        // KRAJ ZAJEDNIČKE MEMORIJE


        // OBRADA RADNOG VREMENA
        time_t pocetno_vrijeme, trenutno_vrijeme, provjeri_vrijeme, ucitano_vrijeme;

        int i = 0;
        int ostaje_prekovremeno;
        int netko_ceka;
        time(&pocetno_vrijeme);
        do {

            if (i <= broj_klijenata && i != 0){
                sleep(vremena_dolazaka[i] - vremena_dolazaka[i-1]);
                time(&provjeri_vrijeme);
                switch (proces[i] = fork()){
                    case -1:
                        fprintf(stderr, "Nije moguće stvoriti proces za klijenta.");
                        exit(1);
                    case 0:
                        klijent(i, velicina_cekaonice);
                        exit(EXIT_SUCCESS);
                }
                i++; 
            }
            
            if (i == 0) {
                switch (proces[i] = fork()){
                    case -1:
                        fprintf(stderr, "Nije moguće stvoriti proces za frizerku.");
                        exit(1);
                    case 0:
                        frizerka(radno_vrijeme);
                        exit(EXIT_SUCCESS);
                }   
                i++;
                sem_wait(inicijalizacija);
            }
            
            time(&trenutno_vrijeme);
            sem_getvalue(frizerka_slobodna, &ostaje_prekovremeno);
            sem_getvalue(slobodna_mjesta, &netko_ceka);
        } while (difftime(trenutno_vrijeme, pocetno_vrijeme) < radno_vrijeme || ostaje_prekovremeno == 0 || netko_ceka > 0);

        while(i--){
            wait(NULL);
        }

    } else {

        int velicina_cekaonice;
        int broj_slobodnih_mjesta;
        printf("Unesite veličinu čekaonice: ");
        scanf("%d", &velicina_cekaonice);

        int broj_klijenata;
        printf("Unesite broj klijenata: ");
        scanf("%d", &broj_klijenata);

        int radno_vrijeme; // u sekundama
        printf("Unesite radno vrijeme: ");
        scanf("%d", &radno_vrijeme);

        int vremena_dolazaka[broj_klijenata+1];
        for(int i=0; i<broj_klijenata+1; i++){
            if (i==0){
                vremena_dolazaka[i] = 0; // frizerka
            } else {
                printf("Unesite vrijeme dolaska %d. klijenta: ", i);
                scanf("%d", &vremena_dolazaka[i]);
            }
        }
        
        pid_t proces[broj_klijenata+1];

        printf("------------------------------------------\n");

        // ZAJEDNIČKA MEMORIJA
        id_memorije = shmget(IPC_PRIVATE, sizeof(sem_t), 0600);
        slobodna_mjesta = shmat(id_memorije, NULL, 0);
        shmctl(id_memorije, IPC_RMID, NULL);
        sem_init(slobodna_mjesta, 1, velicina_cekaonice);

        cekaonica = (sem_t *) slobodna_mjesta + sizeof(sem_t *);
        sem_init(cekaonica, 1, 0);

        id_klijenta = (sem_t *) cekaonica + sizeof(sem_t *);
        sem_init(id_klijenta, 1, 0);

        frizerka_slobodna = (sem_t *) id_klijenta + sizeof(sem_t *);
        sem_init(frizerka_slobodna, 1, 0);

        radi_frizuru = (sem_t*) frizerka_slobodna + sizeof(sem_t *);
        sem_init(radi_frizuru, 1, 0);

        inicijalizacija = (sem_t *) radi_frizuru + sizeof(sem_t *);
        sem_init(inicijalizacija, 1, 0);

        klijente_daj_id = (sem_t *) inicijalizacija + sizeof(sem_t *);
        sem_init(klijente_daj_id, 1, 0);

        identifikator = (int *) klijente_daj_id + sizeof(sem_t *);
        *identifikator = -1; 
        // KRAJ ZAJEDNIČKE MEMORIJE

        // OBRADA RADNOG VREMENA
        time_t pocetno_vrijeme, trenutno_vrijeme, provjeri_vrijeme, ucitano_vrijeme;

        int i = 0;
        int ostaje_prekovremeno;
        int netko_ceka;
        time(&pocetno_vrijeme);
        do {
            time(&trenutno_vrijeme);  
            if (difftime(trenutno_vrijeme, pocetno_vrijeme) >= radno_vrijeme){
                sem_getvalue(slobodna_mjesta, &broj_slobodnih_mjesta);
                for (int i=broj_slobodnih_mjesta; i>0; i--){
                    sem_wait(slobodna_mjesta);
                }
            }

            if (i <= broj_klijenata && i != 0){
                sleep(vremena_dolazaka[i] - vremena_dolazaka[i-1]);
                time(&provjeri_vrijeme);
                switch (proces[i] = fork()){
                    case -1:
                        fprintf(stderr, "Nije moguće stvoriti proces za klijenta.");
                        exit(1);
                    case 0:
                        klijent(i, velicina_cekaonice);
                        exit(EXIT_SUCCESS);
                }
                i++; 
            }
            
            if (i == 0) {
                switch (proces[i] = fork()){
                    case -1:
                        fprintf(stderr, "Nije moguće stvoriti proces za frizerku.");
                        exit(1);
                    case 0:
                        frizerka(radno_vrijeme);
                        exit(EXIT_SUCCESS);
                }   
                i++;
                sem_wait(inicijalizacija);
            }
            
            time(&trenutno_vrijeme);
            sem_getvalue(frizerka_slobodna, &ostaje_prekovremeno);
            sem_getvalue(slobodna_mjesta, &netko_ceka);
        } while (difftime(trenutno_vrijeme, pocetno_vrijeme) < radno_vrijeme || ostaje_prekovremeno == 0 || netko_ceka > 0);

        while(i--){
            wait(NULL);
        }
        
    }

    return 0;
}