import sys, random, time

N = int(sys.argv[1])    # broj procesa
M = int(sys.argv[2])    # broj okvira
disk = [0 for i in range(N)]
okvir = [[0, 0, -1] for i in range(M)]
tablica = [[[int(0) for k in range(16)] for j in range(16)] for i in range(N)]  

def generiraj_nasumicnu_adresu():
    x= random.randint(0, 1024-1) 
    if x % 2 == 0:
        return hex(x)
    else:
        return hex(x+1)
    #return hex(510)

def dohvati_sadrzaj(p,x):
    y = dohvati_fizicku_adresu(p,x)
    if type(y) == str:
        y = '0b' + (bin(int(y, 16))[2:].zfill(16))[0:10]
        y = int(y,2)   
    i = disk[okvir[y][0]]
    return i

def zapisi_vrijednosti(p,x,i):
    y = dohvati_fizicku_adresu(p, x)
    if type(y) == str:
        y = '0b' + (bin(int(y, 16))[2:].zfill(16))[0:10]
        y = int(y,2) 
    disk[okvir[y][0]] = i

def dohvati_fizicku_adresu(p,x):
    x = bin(int(x, 16))[2:].zfill(16)
    x = x[6:]
    indeks_tablice_stranicenja = int(x[0:4], 2)
    pomak_unutar_fizickog_okvira = x[4:]
    
    if (tablica[p][indeks_tablice_stranicenja][5] == 0):
        print("\tPromašaj!")
        najmanji_za_lru = 0
        for o in range(len(okvir)):
            if okvir[o][2] < okvir[najmanji_za_lru][2]:
                najmanji_za_lru = o
      
        if t == 0:
            dodijeljen_okvir = hex(0)
            dodijeljen_okvir_za_ispis = '0x0000'
            print(f"\t\tDodijeljen okvir {dodijeljen_okvir_za_ispis}")
        elif (okvir[najmanji_za_lru][0] == 0 and okvir[najmanji_za_lru][1] == 0 and okvir[najmanji_za_lru][2] == -1):
            dodijeljen_okvir = hex(0 + 1 * najmanji_za_lru)
            dodijeljen_okvir_za_ispis = '0x' + hex(najmanji_za_lru)[2:].zfill(4)
            print(f"\t\tDodijeljen okvir {dodijeljen_okvir_za_ispis}")
        else: 
            adresa_izbacene = '0x' + okvir[najmanji_za_lru][1][2:].zfill(4)
            print(f"\t\tIzbacujem stranicu {adresa_izbacene} iz procesa {okvir[najmanji_za_lru][0]}")
            lru_izbacene = '0x' + hex(okvir[najmanji_za_lru][2])[2:].zfill(4)
            print(f"\t\tLRU izbacene stranice: {lru_izbacene}")
            dodijeljen_okvir = hex(0 + 1 * najmanji_za_lru)
            dodijeljen_okvir_za_ispis = '0x' + hex(najmanji_za_lru)[2:].zfill(4)
            print(f"\t\tDodijeljen okvir {dodijeljen_okvir_za_ispis}")
            prosli_proces = okvir[int(dodijeljen_okvir, 16)][0]
            prosla_stranica = int(bin(int(okvir[int(dodijeljen_okvir, 16)][1],16))[2:].zfill(16)[:-6], 2)
            for indeks in range(16):
                tablica[prosli_proces][prosla_stranica][indeks] = 0

        okvir[najmanji_za_lru][0] = p
        okvir[najmanji_za_lru][1] = x[0:4] + '000000'
        okvir[najmanji_za_lru][1] = int(okvir[najmanji_za_lru][1], 2)
        okvir[najmanji_za_lru][1] = hex(okvir[najmanji_za_lru][1])
        okvir[najmanji_za_lru][2] = t 

        dodijeljen_okvir = bin(int(dodijeljen_okvir,16))
        fizicka_adresa = '0x' + hex(int(dodijeljen_okvir + str(pomak_unutar_fizickog_okvira), 2))[2:].zfill(4)        
        print(f"\tFiz. adresa: {fizicka_adresa}")
        
        zapis_za_tablicu = '0x' + hex(int(str(bin(najmanji_za_lru)[2:].zfill(10) + '1' + str(bin(okvir[najmanji_za_lru][2])[2:].zfill(5))),2))[2:].zfill(4)
        print(f"\tZapis tablice: {zapis_za_tablicu}")
        
        lru_metapodatak = bin(okvir[najmanji_za_lru][2])[2:].zfill(5)
        lru_metapodatak = [int(znamenka) for znamenka in lru_metapodatak]
        for i in range(5):
            tablica[p][indeks_tablice_stranicenja][i] = lru_metapodatak[i]
        
        tablica[p][indeks_tablice_stranicenja][5] = 1
        
        fiz_adr_metapodatak = (dodijeljen_okvir + str(pomak_unutar_fizickog_okvira))[2:].zfill(10)
        fiz_adr_metapodatak = [int(znamenka) for znamenka in fiz_adr_metapodatak]
        for i in range(10):
            tablica[p][indeks_tablice_stranicenja][6+i] = fiz_adr_metapodatak[i]

        sadrzaj_adrese = disk[p]
        print(f"\tSadrzaj adrese: {sadrzaj_adrese}")

        return fizicka_adresa
    else: 
        dodijeljen_okvir = tablica[p][indeks_tablice_stranicenja][6:]
        dodijeljen_okvir = [str(znamenka) for znamenka in dodijeljen_okvir]
        dodijeljen_okvir = ''.join(dodijeljen_okvir)
        dodijeljen_okvir = hex(int(dodijeljen_okvir, 2))
        fizicka_adresa = dodijeljen_okvir
        return fizicka_adresa

def main():
    global t 
    t = 0

    while(True):
        for p in range(0, N):
            x = generiraj_nasumicnu_adresu()
            x_za_ispis = '0x' + x[2:].zfill(4)
            print("\n---------------------------")
            print(f"proces: {p}")
            print(f"\tt: {t}")
            print(f"\tlog. adresa: {x_za_ispis}")
            i = dohvati_sadrzaj(p, x)
            i += 1
            zapisi_vrijednosti(p,x,i)
            if t == 31:
                t = 0
                for proces in range(N):
                    for indeks in range(16):
                        for i in range(5):
                            tablica[proces][indeks][i] = [int(0) for j in range(5)]
            else:
                t = t + 1
            time.sleep(1)

main()