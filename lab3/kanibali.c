#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <pthread.h>

int id_memorije;
int *broj_putnika_camca;
char *vrsta_putnika_camca;
int *popunjenost_camca;
char *trenutna_obala;
int *kanibali;
int *misionari;
int *broj_putnik_desna;
char *vrsta_putnika_desna;
int *broj_putnik_lijeva;
char *vrsta_putnika_lijeva;
int *counter_desna;
int *counter_lijeva;
int *broj_kanibala_u_camcu;
int *broj_misionara_u_camcu;
int *camac_vozi;

pthread_mutex_t monitor;
pthread_cond_t red_camac;
pthread_cond_t red_desna;
pthread_cond_t red_lijeva;


void isprazni_camac(){
    *popunjenost_camca = 0;
    *broj_kanibala_u_camcu = 0;
    *broj_misionara_u_camcu = 0;
    pthread_cond_broadcast (&red_camac);
}


void ispisi_ulazak(int id, char vrsta){
    printf("%c%d: ušao u čamac\n", vrsta, id);
}


void dodaj_u_camac(int id, char obala, char vrsta){
    *(broj_putnika_camca + *popunjenost_camca) = id;
    *(vrsta_putnika_camca + *popunjenost_camca) = vrsta;
    *popunjenost_camca += 1;
    if (vrsta == 'M'){
        *broj_misionara_u_camcu += 1;
    } else {
        *broj_kanibala_u_camcu += 1;
    }
    if (obala == 'D' && vrsta == 'M' && *broj_kanibala_u_camcu + 1 <= *broj_misionara_u_camcu){
        pthread_cond_signal(&red_desna);
    } 
    if (obala == 'L' && vrsta == 'M' && *broj_kanibala_u_camcu + 1 <= *broj_misionara_u_camcu){
        pthread_cond_signal(&red_lijeva);
    }
    if (obala == 'D' && vrsta == 'K' && (*broj_kanibala_u_camcu + 1 <= *broj_misionara_u_camcu || *broj_misionara_u_camcu == 0)){
        pthread_cond_signal(&red_desna);
    } 
    if (obala == 'L' && vrsta == 'K' && (*broj_kanibala_u_camcu + 1 <= *broj_misionara_u_camcu || *broj_misionara_u_camcu == 0)){
        pthread_cond_signal(&red_lijeva);
    }
}


void obrisi_element(int id, char obala, char vrsta){
    int indeks;
    if (obala == 'D'){
        for (int i=0; i<*counter_desna; i++){
            if (*(broj_putnik_desna + i) == id && *(vrsta_putnika_desna + i) == vrsta){
                indeks = i;
            }
        }
        for(int i=indeks; i<*counter_desna; i++){
            *(broj_putnik_desna + i) = *(broj_putnik_desna + i + 1);
            *(vrsta_putnika_desna + i) = *(vrsta_putnika_desna + i + 1);
        }
        *counter_desna -= 1;
    } else {
        for (int i=0; i<*counter_lijeva; i++){
            if (*(broj_putnik_lijeva + i) == id && *(vrsta_putnika_lijeva + i) == vrsta){
                indeks = i;
            }
        }
        for(int i=indeks; i<*counter_lijeva; i++){
            *(broj_putnik_lijeva + i) = *(broj_putnik_lijeva + i + 1);
            *(vrsta_putnika_lijeva + i) = *(vrsta_putnika_lijeva + i + 1);
        }
        *counter_lijeva -= 1;
    }
}


void ispisi_obalu(){
    printf("C[%c]={", *trenutna_obala);
    for(int i=0; i<*popunjenost_camca; i++){
        if(i != 0){
            printf(" ");
        }
        printf("%c%d", *(vrsta_putnika_camca + i), *(broj_putnika_camca + i));
    }
    printf("} LO={");
    for(int i=0; i<*counter_lijeva; i++){
        if(i != 0){
            printf(" ");
        }
        printf("%c%d", *(vrsta_putnika_lijeva + i), *(broj_putnik_lijeva + i));
    }
    printf("} DO={");
    for(int i=0; i<*counter_desna; i++){
        if(i != 0){
            printf(" ");
        }
        printf("%c%d", *(vrsta_putnika_desna + i), *(broj_putnik_desna + i));
    }
    printf("}\n\n");
}


void ispisi_dolazak(int id, char obala, char vrsta){
    if (obala == 'D'){
        printf("%c%d: došao na desnu obalu\n", vrsta, id);
    } else {
        printf("%c%d: došao na lijevu obalu\n", vrsta, id);
    }
}


void dodaj_na_obalu(int id, char obala, char vrsta){
    if (obala == 'D'){
        *(broj_putnik_desna + *counter_desna) = id;
        *(vrsta_putnika_desna + *counter_desna) = vrsta;
        *counter_desna += 1;
    } else {
        *(broj_putnik_lijeva + *counter_lijeva) = id;
        *(vrsta_putnika_lijeva + *counter_lijeva) = vrsta;
        *counter_lijeva += 1;
    }

    
}

char odredi_stranu(){
    int nasumicni_broj;
    srand(time(0));
    nasumicni_broj = rand() % ((1-0)+1);
    if (nasumicni_broj == 0){
        return 'D';
    } else {
        return 'L';
    }
}

void *fja_misionar(){
    int id = *misionari;
    *misionari += 1;
    char obala = odredi_stranu();

    pthread_mutex_lock (&monitor);
    dodaj_na_obalu(id, obala, 'M');

    ispisi_dolazak(id, obala, 'M');
    ispisi_obalu();

    
    while (*broj_misionara_u_camcu + 1 < *broj_kanibala_u_camcu || *popunjenost_camca == 7 || *trenutna_obala != obala || *camac_vozi != 0){
        if (obala == 'D'){
            pthread_cond_wait (&red_desna, &monitor);
        } else {
            pthread_cond_wait (&red_lijeva, &monitor);
        }
    }


    obrisi_element(id, obala, 'M');
    dodaj_u_camac(id, obala, 'M');
    ispisi_ulazak(id, 'M');
    ispisi_obalu();

    pthread_mutex_unlock (&monitor);

    pthread_mutex_lock (&monitor);

    if (*popunjenost_camca != 0){
        pthread_cond_wait (&red_camac, &monitor);
    }

    pthread_mutex_unlock (&monitor);
}


void *fja_kanibal(){
    int id = *kanibali;
    *kanibali += 1;
    char obala = odredi_stranu();

    pthread_mutex_lock (&monitor);
    dodaj_na_obalu(id, obala, 'K');    

    ispisi_dolazak(id, obala, 'K');
    ispisi_obalu();

    while ((*broj_kanibala_u_camcu + 1 <= *broj_misionara_u_camcu && *broj_misionara_u_camcu == 0) || (*broj_kanibala_u_camcu + 1 > *broj_misionara_u_camcu && *broj_misionara_u_camcu != 0) || *popunjenost_camca == 7 || *trenutna_obala != obala || *camac_vozi != 0){
        if (obala == 'D'){
            pthread_cond_wait (&red_desna, &monitor);
        } else {
            pthread_cond_wait (&red_lijeva, &monitor);
        }
    }

    obrisi_element(id, obala, 'K');
    dodaj_u_camac(id, obala, 'K');
    ispisi_ulazak(id, 'K');
    ispisi_obalu();

    pthread_mutex_unlock (&monitor);

    pthread_mutex_lock (&monitor);

    if (*popunjenost_camca != 0){
        pthread_cond_wait (&red_camac, &monitor);
    }

    pthread_mutex_unlock (&monitor);
}

void *fja_camac(){
    int prethodna_popunjenost = -1;

    do {
        
        if (*misionari >= 100 && *kanibali >= 200){
            if (*trenutna_obala == 'D'){
                pthread_cond_broadcast (&red_desna);
            } else {
                pthread_cond_broadcast (&red_lijeva);
            }
        }
        
        if(*popunjenost_camca >= 3){
            
            if(*popunjenost_camca == 6){
                pthread_mutex_lock (&monitor);
                printf("C: šest putnika ukrcana, polazim za jednu sekundu\n");
                ispisi_obalu();
                pthread_mutex_unlock (&monitor);
            }
            if(*popunjenost_camca == 5){
                pthread_mutex_lock (&monitor);
                printf("C: pet putnika ukrcana, polazim za jednu sekundu\n");
                ispisi_obalu();
                pthread_mutex_unlock (&monitor);
            }
            if(*popunjenost_camca == 4){
                pthread_mutex_lock (&monitor);
                printf("C: četiri putnika ukrcana, polazim za jednu sekundu\n");
                ispisi_obalu();
                pthread_mutex_unlock (&monitor);
            }
            if(*popunjenost_camca == 3){
                pthread_mutex_lock (&monitor);
                printf("C: tri putnika ukrcana, polazim za jednu sekundu\n");
                ispisi_obalu();
                pthread_mutex_unlock (&monitor);
            }

            sleep(1);
        
            if (*trenutna_obala == 'D'){
                pthread_mutex_lock (&monitor);
                printf("C: prevozim s desne na lijevu obalu:");
                *camac_vozi = 1;
                for(int i=0; i<*popunjenost_camca; i++){
                    printf(" %c%d", *(vrsta_putnika_camca + i), *(broj_putnika_camca + i));
                }
                printf("\n\n");
                pthread_mutex_unlock (&monitor);
            } else {
                pthread_mutex_lock (&monitor);
                printf("C: prevozim s lijeve na desnu obalu:");
                *camac_vozi = 1;
                for(int i=0; i<*popunjenost_camca; i++){
                    printf(" %c%d", *(vrsta_putnika_camca + i), *(broj_putnika_camca + i));
                }
                printf("\n\n");
                pthread_mutex_unlock (&monitor);
            }
            sleep(2);
            if (*trenutna_obala == 'D'){
                pthread_mutex_lock (&monitor);
                printf("C: prevezao s desne na lijevu obalu:");
                *camac_vozi = 0;
                for(int i=0; i<*popunjenost_camca; i++){
                    printf(" %c%d", *(vrsta_putnika_camca + i), *(broj_putnika_camca + i));
                }
                printf("\n");
                printf("C: prazan na lijevoj obali\n");
                isprazni_camac();
                *trenutna_obala = 'L';
                ispisi_obalu();
                pthread_mutex_unlock (&monitor);
                pthread_cond_broadcast (&red_lijeva);
            } else {
                pthread_mutex_lock (&monitor);
                printf("C: prevezao s lijeve na desnu obalu:");
                *camac_vozi = 0;
                for(int i=0; i<*popunjenost_camca; i++){
                    printf(" %c%d", *(vrsta_putnika_camca + i), *(broj_putnika_camca + i));
                }
                printf("\n");
                printf("C: prazan na desnoj obali\n");
                isprazni_camac();
                *trenutna_obala = 'D';
                ispisi_obalu();
                pthread_mutex_unlock (&monitor);
                pthread_cond_broadcast (&red_desna);
            }
        }

    } while (1);
}

void *fja_pomocna(){
    int k=0;
    int m=0;
    pthread_t d_kanibali[200];
    pthread_t d_misionari[100]; 

    do {
        
        // napravi misionara i kanibala
        if(pthread_create(&d_misionari[m], NULL, fja_misionar, NULL) != 0){
            printf("Pogreška prilikom stvaranja dretve\n");
            exit(1);
        }
        m++;
        sleep(1);
        if(pthread_create(&d_kanibali[k], NULL, fja_kanibal, NULL) != 0){
            printf("Pogreška prilikom stvaranja dretve\n");
            exit(1);
        }
        k++;
        
        sleep(1);

        // napravi kanibala
        if(pthread_create(&d_kanibali[k], NULL, fja_kanibal, NULL) != 0){
            printf("Pogreška prilikom stvaranja dretve\n");
            exit(1);
        }
        k++;
        
        sleep(1);        

    } while( k<200 && m<100); 

    for(int kk=0; kk<200; kk++){
        pthread_join(d_kanibali[kk], NULL);
    }
    for(int mm=0; mm<100; mm++){ 
        pthread_join(d_misionari[mm], NULL);
    }

}

void ispis_legende(){
    // legenda
    printf("\nLegenda: M-misionar, K-kanibal, C-čamac,\n");
    printf("         LO-lijeva obala, DO-desna obala\n");
    printf("         L-lijevo, D-desno\n\n");

    // inicijalizacija camca
    printf("C: prazan na desnoj obali\n");
    printf("C[D]={} LO={} DO={}\n\n");
}


int main(){

    pthread_mutex_init (&monitor, NULL);
    pthread_cond_init (&red_camac, NULL);
    pthread_cond_init (&red_desna, NULL);
    pthread_cond_init (&red_lijeva, NULL);

    // ZAJEDNIČKA MEMORIJA
    id_memorije = shmget(IPC_PRIVATE, 7*sizeof(int), 0600);
    broj_putnika_camca = shmat(id_memorije, NULL, 0);
    shmctl(*broj_putnika_camca, IPC_RMID, NULL);
    for(int i=0; i<7; i++){
        *(broj_putnika_camca + i) = -1;
    }
    
    vrsta_putnika_camca = (char *) broj_putnika_camca + 7 * sizeof(int *);
    for(int i=0; i<7; i++){
        *(vrsta_putnika_camca + i) = 'X';
    }

    popunjenost_camca = (int *) vrsta_putnika_camca + 7 * sizeof(char *);
    *popunjenost_camca = 0;

    trenutna_obala = (char *) popunjenost_camca + sizeof(int *);
    *trenutna_obala = 'D';

    kanibali = (int *) trenutna_obala + sizeof(char *);
    *kanibali = 1;

    misionari = (int *) kanibali + sizeof(int *);
    *misionari = 1;

    broj_putnik_desna = (int *) misionari + sizeof(int *);
    for(int i=0; i<30; i++){
        *(broj_putnik_desna + i) = -1;
    }
    
    vrsta_putnika_desna = (char *) broj_putnik_desna + 30 * sizeof(int *);
    for(int i=0; i<30; i++){
        *(vrsta_putnika_desna + i) = 'X';
    }

    broj_putnik_lijeva = (int *) vrsta_putnika_desna + 30 * sizeof(char *);
    for(int i=0; i<30; i++){
        *(broj_putnik_lijeva + i) = -1;
    }

    vrsta_putnika_lijeva = (char *) broj_putnik_lijeva + 30 * sizeof(int *);
    for(int i=0; i<30; i++){
        *(vrsta_putnika_desna + i) = 'X';
    }

    counter_desna = (int *) vrsta_putnika_lijeva + 30 * sizeof(char *);
    *counter_desna = 0;

    counter_lijeva = (int *) counter_desna + sizeof(int *);
    *counter_lijeva = 0;

    broj_kanibala_u_camcu = (int *) counter_lijeva + sizeof(int *);
    *broj_kanibala_u_camcu = 0;

    broj_misionara_u_camcu = (int *) broj_kanibala_u_camcu + sizeof(int *);
    *broj_misionara_u_camcu = 0;

    camac_vozi = (int *) broj_misionara_u_camcu + sizeof(int *);
    *camac_vozi = 0;
    // KRAJ ZAJEDNIČKE MEMORIJE

    ispis_legende();

    pthread_t d_camac;  
    if(pthread_create(&d_camac, NULL, fja_camac, NULL) != 0){
        printf("Pogreška prilikom stvaranja dretve\n");
    }

    pthread_t d_pomocna; 
    if(pthread_create(&d_pomocna, NULL, fja_pomocna, NULL) != 0){
        printf("Pogreška prilikom stvaranja dretve\n");
    }

    pthread_join(d_camac, NULL);
    pthread_join(d_pomocna, NULL);

    pthread_mutex_destroy (&monitor);
    pthread_cond_destroy (&red_camac);
    pthread_cond_destroy (&red_desna);
    pthread_cond_destroy (&red_lijeva);

}