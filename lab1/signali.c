#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

struct timespec t0; // vrijeme pocetka programa

int K_Z[3];
int T_P;
int stog[100];
int vrh_stoga = -1;
int nedovrsen[3];
int odbijen[3];

// postavlja trenutno vrijeme u t0 
void postavi_pocetno_vrijeme(){
	clock_gettime(CLOCK_REALTIME, &t0);
}

// dohvaca vrijeme proteklo od pokretanja programa 
void vrijeme(void){
	struct timespec t;

	clock_gettime(CLOCK_REALTIME, &t);

	t.tv_sec -= t0.tv_sec;
	t.tv_nsec -= t0.tv_nsec;
	if (t.tv_nsec < 0) {
		t.tv_nsec += 1000000000;
		t.tv_sec--;
	}

	printf("%03ld.%03ld:\t", t.tv_sec, t.tv_nsec/1000000);
}

// ispis kao i printf uz dodatak trenutnog vremena na pocetku 
#define PRINTF(format, ...)       \
do {                              \
  vrijeme();                      \
  printf(format, ##__VA_ARGS__);  \
}                                 \
while(0)

// funkcije za obradu signala, navedene ispod main-a 
void obradi_sigusr1(int sig);
void obradi_sigterm(int sig);
void obradi_sigint(int sig);

void zastavice(int stog[], int K_Z[], int T_P, int vrh_stoga){
	if(vrh_stoga == -1){
		PRINTF("K_Z=%d%d%d, T_P=%d, stog: -\n\n", K_Z[0], K_Z[1], K_Z[2], T_P);
	} else {
		PRINTF("K_Z=%d%d%d, T_P=%d, stog: ", K_Z[0], K_Z[1], K_Z[2], T_P);
		if(vrh_stoga == 0){
			printf("%d, reg[%d]\n", stog[0], stog[0]);
		} else{
			for(int i=vrh_stoga; i>-1; i--){
				printf("%d, reg[%d]", stog[i], stog[i]);
				if(i != 0){
					printf("; ");
				} else {
					printf("\n");
				}
			}
		}
		printf("\n");
	}
}

int pop(int stog[], int vrh_stoga){
	int element = stog[vrh_stoga];
	vrh_stoga--;
	return vrh_stoga;
}

int push(int stog[], int vrh_stoga, int element){
	vrh_stoga++;
	stog[vrh_stoga] = element;
	return vrh_stoga;
}

void spavaj(time_t sekundi){
	struct timespec koliko;
	koliko.tv_sec = sekundi;
	koliko.tv_nsec = 0;

	while (nanosleep(&koliko, &koliko) == -1 && errno == EINTR){
		
		if (stog[vrh_stoga] == 2 && nedovrsen[1] == 1 && nedovrsen[2] == 0){
			PRINTF("Nastavlja se obrada prekida razine 2\n");
			T_P = 2;
			vrh_stoga = pop(stog, vrh_stoga);
			zastavice(stog, K_Z, T_P, vrh_stoga);
		}
		if (stog[vrh_stoga] == 1 && nedovrsen[0] == 1 && nedovrsen[1] == 0 && nedovrsen[2] == 0) {
			PRINTF("Nastavlja se obrada prekida razine 1\n");
			T_P = 1;
			vrh_stoga = pop(stog, vrh_stoga);
			zastavice(stog, K_Z, T_P, vrh_stoga);
		}
		if (stog[vrh_stoga] == 0 && nedovrsen[0] == 0 && nedovrsen[1] == 0 && nedovrsen[2] == 0){
			PRINTF("Nastavlja se izvođenje glavnog programa\n");
			T_P = 0;
			vrh_stoga = pop(stog, vrh_stoga);
			zastavice(stog, K_Z, T_P, vrh_stoga);
		}
		if (T_P == 0 && odbijen[2] == 1){
			obradi_sigint(15);
		}
		if (T_P == 0 && odbijen[1] == 1){
			obradi_sigusr1(10);
		}
		if (T_P == 0 && odbijen[0] == 1){
			obradi_sigterm(2);
		}
		if (stog[vrh_stoga] == 0 && nedovrsen[0] == 0 && nedovrsen[1] == 0 && nedovrsen[2] == 0 && vrh_stoga >= 0){
			PRINTF("Nastavlja se izvođenje glavnog programa\n");
			T_P = 0;
			vrh_stoga = pop(stog, vrh_stoga);
			zastavice(stog, K_Z, T_P, vrh_stoga);
		}
		if (T_P == 0 && odbijen[1] == 1){
			obradi_sigusr1(10);
		}
		if (T_P == 0 && odbijen[0] == 1){
			obradi_sigterm(2);
		}
		if (stog[vrh_stoga] == 0 && nedovrsen[0] == 0 && nedovrsen[1] == 0 && nedovrsen[2] == 0 && vrh_stoga >= 0){
			PRINTF("Nastavlja se izvođenje glavnog programa\n");
			T_P = 0;
			vrh_stoga = pop(stog, vrh_stoga);
			zastavice(stog, K_Z, T_P, vrh_stoga);
		}
	}

}

int main(){
	
	postavi_pocetno_vrijeme();
	struct sigaction act;

	// 1. maskiranje signala SIGUSR1 
	act.sa_handler = obradi_sigusr1; // kojom se funkcijom signal obrađuje 
	sigemptyset(&act.sa_mask);
	//sigaddset(&act.sa_mask, SIGTERM); // blokirati i SIGTERM za vrijeme obrade
	act.sa_flags = SA_NODEFER; // naprednije mogućnosti preskočene 
	sigaction(SIGUSR1, &act, NULL); // maskiranje signala preko sučelja OS-a 
	
	// 2. maskiranje signala SIGTERM 
	act.sa_handler = obradi_sigterm;
	sigemptyset(&act.sa_mask);
	sigaction(SIGTERM, &act, NULL);
	
	// 3. maskiranje signala SIGINT 
	act.sa_handler = obradi_sigint;
	sigaction(SIGINT, &act, NULL);

	K_Z[0] = 0;
	K_Z[1] = 0;
	K_Z[2] = 0;
	T_P = 0;

	printf("\n");
	PRINTF("Program s PID=%ld krenuo s radom\n", (long) getpid());
	zastavice(stog, K_Z, T_P, vrh_stoga);
	
	while(1){spavaj(100000000);}

	return 0;
}

void obradi_sigusr1(int sig){
	if (T_P == 2 && K_Z[1] == 0){
		PRINTF("SKLOP: Dogodio se prekid razine 2 ali se on pamti i ne prosljeđuje procesoru\n");
		K_Z[1] = 1;
		odbijen[1] = 1;
		zastavice(stog, K_Z, T_P, vrh_stoga);
	}

	// SCENARIJ KAD JE PREKID MORAO ČEKATI
	if (K_Z[1] == 1 && odbijen[1] == 1 && nedovrsen[1] == 0){
		PRINTF("SKLOP: promijenio se T_P, prosljeđuje prekid razine 2 procesoru\n");
		nedovrsen[1] = 1;
		odbijen[1] = 0;
		vrh_stoga = push(stog, vrh_stoga, 0);
		K_Z[1] = 0;
		PRINTF("Počela obrada razine 2\n");
		T_P = 2;
		zastavice(stog, K_Z, T_P, vrh_stoga);
		spavaj(20);
		nedovrsen[1] = 0;
		PRINTF("Završila obrada prekida razine 2\n");
	}

	// SCENARIJ KAD JE JAČI OD TRENUTNOG
	if (2 > T_P && K_Z[1] == 0){
		if(T_P == 1){
			nedovrsen[0] = 1;
		}
		PRINTF("SKLOP: Dogodio se prekid razine 2 i prosljeđuje se procesoru\n");
		nedovrsen[1] = 1;
		K_Z[1] = 1;
		zastavice(stog, K_Z, T_P, vrh_stoga);
		vrh_stoga = push(stog, vrh_stoga, T_P);
		T_P = 2;
		PRINTF("Počela obrada razine 2\n");
		K_Z[1] = 0;
		zastavice(stog, K_Z, T_P, vrh_stoga);
		spavaj(20);
		nedovrsen[1] = 0;
		PRINTF("Završila obrada prekida razine 2\n");
	}

	// SCENARIJ KAD JE SLABIJI OD TRENUTNOG
	if (2 < T_P && K_Z[1] == 0){
		odbijen[1] = 1; 
		if (T_P == 3){
			nedovrsen[2] = 1;
		}
		PRINTF("SKLOP: Dogodio se prekid razine 2 ali se on pamti i ne prosljeđuje procesoru\n");
		K_Z[1] = 1;
		zastavice(stog, K_Z, T_P, vrh_stoga);
	}
}

void obradi_sigterm(int sig){
	if (T_P == 1 && K_Z[0] == 0){
		PRINTF("SKLOP: Dogodio se prekid razine 1 ali se on pamti i ne prosljeđuje procesoru\n");
		K_Z[0] = 1;
		odbijen[0] = 1;
		zastavice(stog, K_Z, T_P, vrh_stoga);
	}

	// SCENARIJ KAD JE PREKID MORAO ČEKATI
	if (K_Z[0] == 1 && odbijen[0] == 1 && nedovrsen[0] == 0){
		PRINTF("SKLOP: promijenio se T_P, prosljeđuje prekid razine 1 procesoru\n");
		nedovrsen[0] = 1;
		odbijen[0] = 0;
		vrh_stoga = push(stog, vrh_stoga, 0);
		K_Z[0] = 0;
		PRINTF("Počela obrada razine 1\n");
		T_P = 1;
		zastavice(stog, K_Z, T_P, vrh_stoga);
		spavaj(20);
		nedovrsen[0] = 0;
		PRINTF("Završila obrada prekida razine 1\n");
	}

	// SCENARIJ KAD JE JAČI OD TRENUTNOG
	if (1 > T_P && K_Z[0] == 0){
		PRINTF("SKLOP: Dogodio se prekid razine 1 i prosljeđuje se procesoru\n");
		nedovrsen[0] = 1;
		K_Z[0] = 1;
		zastavice(stog, K_Z, T_P, vrh_stoga);
		vrh_stoga = push(stog, vrh_stoga, T_P);
		T_P = 1;
		K_Z[0] = 0;
		PRINTF("Počela obrada razine 1\n");
		zastavice(stog, K_Z, T_P, vrh_stoga);
		spavaj(20);
		nedovrsen[0] = 0;
		PRINTF("Završila obrada prekida razine 1\n");
	}
	
	// SCENARIJ KAD JE SLABIJI OD TRENUTNOG
	if (1 < T_P && K_Z[0] == 0){
		odbijen[0] = 1;
		if (T_P == 2){
			nedovrsen[1] = 1;
		} 
		if (T_P == 3){
			nedovrsen[2] = 1;
		}
		PRINTF("SKLOP: Dogodio se prekid razine 1 ali se on pamti i ne prosljeđuje procesoru\n");
		K_Z[0] = 1;
		zastavice(stog, K_Z, T_P, vrh_stoga);
	}
}

void obradi_sigint(int sig){
	if (T_P == 3 && K_Z[2] == 0){
		PRINTF("SKLOP: Dogodio se prekid razine 3 ali se on pamti i ne prosljeđuje procesoru\n");
		K_Z[2] = 1;
		odbijen[2] = 1;
		zastavice(stog, K_Z, T_P, vrh_stoga);
	}

	// SCENARIJ KAD JE PREKID MORAO ČEKATI
	if (K_Z[2] == 1 && odbijen[2] == 1 && nedovrsen[2] == 0){
		PRINTF("SKLOP: promijenio se T_P, prosljeđuje prekid razine 3 procesoru\n");
		nedovrsen[2] = 1;
		odbijen[2] = 0;
		vrh_stoga = push(stog, vrh_stoga, 0);
		K_Z[2] = 0;
		PRINTF("Počela obrada razine 3\n");
		T_P = 3;
		zastavice(stog, K_Z, T_P, vrh_stoga);
		spavaj(20);
		nedovrsen[2] = 0;
		PRINTF("Završila obrada prekida razine 3\n");
	}

 	// SCENARIJ KAD JE JAČI OD TRENUTNOG (UVIJEK)
	if (3 > T_P && K_Z[2] == 0){
		if(T_P == 1){
			nedovrsen[0] = 1;
		}
		if(T_P == 2){
			nedovrsen[1] = 1;
		}
		PRINTF("SKLOP: Dogodio se prekid razine 3 i prosljeđuje se procesoru\n");
		nedovrsen[2] = 1;
		K_Z[2] = 1;
		zastavice(stog, K_Z, T_P, vrh_stoga);
		vrh_stoga = push(stog, vrh_stoga, T_P);
		T_P = 3;
		PRINTF("Počela obrada razine 3\n");
		K_Z[2] = 0;
		zastavice(stog, K_Z, T_P, vrh_stoga);
		spavaj(20);
		PRINTF("Završila obrada prekida razine 3\n");
		nedovrsen[2] = 0;
	}

	// SCENARIJ KAD JE SLABIJI OD TRENUTNOG (NIKAD)
}
