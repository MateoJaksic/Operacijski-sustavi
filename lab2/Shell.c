#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

// inicijalizacija globalne varijable
int signal_checker = 0;

// funkcija za pronalaženje naredbe
char* pronadi_naredbu(char *ulaz) {
    
    char niz[100];
    strcpy(niz, ulaz);

    int i;
    for (i=0; niz[i] != ' '; i++){
        continue;
    }
    niz[i] = '\0';

    char *ptr = &niz[0];
    return ptr;
}

// funkcija za pronalaženje argumenata
char** pronadi_argumente(char *ulaz) {
    char niz[100];
    strcpy(niz, ulaz);

    char *rijeci = strtok(niz, " "); 
    
    int i;
    // pretpostavka da nemamo više od 10 argumenata
    char **argumenti = (char**) malloc(10 * sizeof(char*)); 
    for(i=0; rijeci != NULL; i++){
        argumenti[i] = (char*) malloc(strlen(rijeci) + 1);
        strcpy(argumenti[i], rijeci);
        rijeci = strtok(NULL, " ");
    }

    argumenti[i] = NULL;

    return argumenti;
}

// funkcija za obrada signala 
void obradi_sigint(int sig){
    signal_checker = 1;
	printf("\n");
}


int main(){
    
    int blokada = 0;
    printf("\n");

    // dio za obradu signala SIGINT u mainu
    struct sigaction act;
    act.sa_handler = obradi_sigint;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);


    while(1){

        // početni ispis 
        printf("fsh> ");
        
        // dohvaćanje naredbe
        char ulaz[100];
        fgets(ulaz, 100, stdin);

        // uklanjanje newline znaka iz dohvaćene naredbe
        ulaz[strcspn(ulaz, "\n")] = 0;

        // poziv funkcije za pronalaženje naredbe i manipuliranje podatkom
        char *ptr;
        char naredba[100];
        ptr = pronadi_naredbu(&ulaz[0]);
        strcpy(naredba, ptr);
        char** arg;
        arg = pronadi_argumente(ulaz);
        int i = 0;

        // stvaranje 2D liste u kojoj kao stringove spremamo imena ugrađenih funkcija
        char *ugradene_funkcije[2][100];
        *ugradene_funkcije[0] = "cd";
        *ugradene_funkcije[1] = "exit";

        // provjera imamo li ugrađenu funkciju "cd"
        if (strcmp(naredba, *ugradene_funkcije[0]) == 0){
             
            // provjera postoji li direktorij unesen kao argument
            if (chdir(arg[1]) == -1){
                fprintf(stderr, "cd: The directory '%s' does not exist\n", arg[1]);
            }   

            blokada = 1;
        } 

        // provjera imamo li ugrađenu funkciju "exit"
        else if (strcmp(naredba, *ugradene_funkcije[1]) == 0 || strcmp(naredba, "") == 0 && signal_checker == 0){
            break;
        }

        // ako nije ugrađena funkcija stvori proces dijete i odradi naredbu
        if (blokada == 0 && signal_checker == 0){
            // jezgreni pozivi
            switch(fork()){
                // ako nije stvoren proces
                case -1: 
                    fprintf(stderr, "Nije moguće stvoriti proces.");
                // ako je uspješno stvoren proces
                case 0:
                    setpgid(0, 0);
                    char putanja[50] = "/bin/";
                    strcat(putanja, naredba);
                    execve(putanja, (char * const *)arg, NULL);
                    printf("fsh: Unknown command: %s\n", naredba);
                    exit(0);
            }
            wait(NULL);
        }

        // očisti memoriju
        for (int i = 0; i < 10; i++) {
            arg[i] = NULL;
        }
        for (int i = 0; i < 100; i++) {
            naredba[i] = '\0';
        }
        for (int i = 0; i < 100; i++) {
            ulaz[i] = '\0';
        }
        
        blokada = 0;
        signal_checker = 0;
    }

    printf("\n");

    return 0;
}


/* 

PRIMJER KORIŠTENJA (samo neke naredbe)

pwd - ispis tekućeg kazala
ls - ispis datoteka u trenutnom direktoriju
echo <željeni ispis> - ispis 
touch <ime_datoteke> - stvaranje datoteke
cd <path> - pozicioniranje
exit - izlazak iz shell-a (također kada se unese prazna linija)

=> izvođenjem SIGINT (CTRL + C) se izlazi iz trenutnog programa ukoliko je otvoren,
   ako nije, onda samo prijelazak u sljedeći red

*/