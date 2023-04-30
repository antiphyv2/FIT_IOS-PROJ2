#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

//Deklarace sdilenych promennych a semaforu
FILE *output_file;


sem_t *output_sem;
sem_t *mutex;
sem_t *fronta_dopisy;
sem_t *fronta_baliky;
sem_t *fronta_peneznisluzby;

long pocet_zakazniku;
long pocet_uredniku;
long max_cekani_na_postu;
long max_delka_prestavky;
long max_uzavreno_pro_nove;

typedef struct shared {
    int citac_akci;
    bool post_open;
    int pocet_lidi_dopisy;
    int pocet_lidi_baliky;
    int pocet_lidi_peneznisluzby;
} shared_mem;

shared_mem* memory_sh;

//Funkce pro otevreni souboru a nastaveni bufferu
bool open_file(){

    output_file = fopen("proj2.out", "w");
    if(output_file == NULL){
        fprintf(stderr, "Error handling proj2.out");
        return false;
    }

    setbuf(output_file, NULL);
    return true;
}

//Funkce pro zpracovani vstupnich argumentu
int argument_parsing(char* argv[]){

    char *end_ptr;
    pocet_zakazniku = strtol(argv[1], &end_ptr, 10);
    if(*end_ptr != 0){
        return -1;
    }
    pocet_uredniku = strtol(argv[2], &end_ptr, 10);
    if(*end_ptr != 0){
        return -1;
    }
    max_cekani_na_postu = strtol(argv[3], &end_ptr, 10);
    if(*end_ptr != 0){
        return -1;
    }
    max_delka_prestavky = strtol(argv[4], &end_ptr, 10);
    if(*end_ptr != 0){
        return -1;
    }
    max_uzavreno_pro_nove = strtol(argv[5], &end_ptr, 10);
    if(*end_ptr != 0){
        return -1;
    }

    if(pocet_zakazniku < 0){
        return -1;
    }
    if(pocet_uredniku < 1){
        return -1;
    }
    if(max_cekani_na_postu < 0 || max_cekani_na_postu > 10000){
        return -1;
    }

    if(max_delka_prestavky < 0 || max_delka_prestavky > 100){
        return -1;
    }

    if(max_uzavreno_pro_nove < 0 || max_uzavreno_pro_nove > 10000){
        return -1;
    }

    return 0;
}
//Funkce pro vytvareni sdilenne pameti
void shared_mem_init(){
    memory_sh = mmap(NULL, sizeof(shared_mem), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    memory_sh->citac_akci = 0;
    memory_sh->post_open = true;
    memory_sh->pocet_lidi_dopisy = 0;
    memory_sh->pocet_lidi_baliky = 0;
    memory_sh->pocet_lidi_peneznisluzby = 0;
}

//Funkce pro inicializaci semaforu
void semaphore_init(){
    output_sem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    if(output_sem == MAP_FAILED){
        fclose(output_file);
        exit(1);
    }
    sem_init(output_sem, 1, 1);
    fronta_dopisy = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    if(fronta_dopisy == MAP_FAILED){
        munmap(output_sem, sizeof(sem_t));
        fclose(output_file);
        exit(1);
    }
    sem_init(fronta_dopisy, 1, 0);
    fronta_baliky = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    if(fronta_baliky == MAP_FAILED){
        munmap(output_sem, sizeof(sem_t));
        munmap(fronta_dopisy, sizeof(sem_t));
        fclose(output_file);
        exit(1);
    }
    sem_init(fronta_baliky, 1, 0);
    fronta_peneznisluzby = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    if(fronta_peneznisluzby == MAP_FAILED){
        munmap(output_sem, sizeof(sem_t));
        munmap(fronta_dopisy, sizeof(sem_t));
        munmap(fronta_baliky, sizeof(sem_t));
        fclose(output_file);
        exit(1);
    }
    sem_init(fronta_peneznisluzby, 1, 0);
    mutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    if(mutex == MAP_FAILED){
        munmap(output_sem, sizeof(sem_t));
        munmap(fronta_dopisy, sizeof(sem_t));
        munmap(fronta_baliky, sizeof(sem_t));
        munmap(fronta_peneznisluzby, sizeof(sem_t));
        fclose(output_file);
        exit(1);
    }
    sem_init(mutex, 1, 1);
}

//Funkce pro uklizeni pameti
void shared_clean(){
    sem_destroy(output_sem);
    sem_destroy(mutex);
    sem_destroy(fronta_baliky);
    sem_destroy(fronta_dopisy);
    sem_destroy(fronta_peneznisluzby);
    munmap(output_sem, sizeof(sem_t));
    munmap(fronta_baliky, sizeof(sem_t));
    munmap(fronta_dopisy, sizeof(sem_t));
    munmap(fronta_peneznisluzby, sizeof(sem_t));
    munmap(mutex, sizeof(sem_t));
    munmap(memory_sh, sizeof(shared_mem));
    
}

void proces_zakaznik(int idZ){
    sem_wait(output_sem);
    fprintf(output_file,"%d: Z %d: started\n", ++(memory_sh->citac_akci), idZ);
    sem_post(output_sem); 

    srand(time(NULL)+ idZ);
    int customer_waiting_time = rand() % max_cekani_na_postu + 1;
    usleep(customer_waiting_time * 1000);

    sem_wait(mutex);
    if(memory_sh->post_open == false){
        sem_post(mutex);
        sem_wait(output_sem);
        fprintf(output_file,"%d: Z %d: going home\n", ++(memory_sh->citac_akci), idZ);
        sem_post(output_sem); 
        exit(0);
    }

    srand(time(NULL) * getpid());
    int Cislo_prepazky = rand() % 3 + 1;

    switch (Cislo_prepazky)
    {
    case 1:
        ++(memory_sh->pocet_lidi_dopisy);
        sem_wait(output_sem);
        fprintf(output_file,"%d: Z %d: entering office for a service %d\n", ++(memory_sh->citac_akci), idZ, Cislo_prepazky);
        sem_post(output_sem); 
        sem_post(mutex);
        sem_wait(fronta_dopisy);
        break;
    case 2:
        ++(memory_sh->pocet_lidi_baliky);
        sem_wait(output_sem);
        fprintf(output_file,"%d: Z %d: entering office for a service %d\n", ++(memory_sh->citac_akci), idZ, Cislo_prepazky);
        sem_post(output_sem); 
        sem_post(mutex);
        sem_wait(fronta_baliky);
        break;
    case 3:
        ++(memory_sh->pocet_lidi_peneznisluzby);
        sem_wait(output_sem);
        fprintf(output_file,"%d: Z %d: entering office for a service %d\n", ++(memory_sh->citac_akci), idZ, Cislo_prepazky);
        sem_post(output_sem); 
        sem_post(mutex);
        sem_wait(fronta_peneznisluzby);
        break;
    default:
        break;
    }

    sem_wait(output_sem);
    fprintf(output_file,"%d: Z %d: called by office worker\n", ++(memory_sh->citac_akci), idZ);
    sem_post(output_sem); 

    srand(time(NULL) + idZ);
    int wait_for_sync = rand() % 10 + 1;
    usleep(wait_for_sync * 1000);

    sem_wait(output_sem);
    fprintf(output_file,"%d: Z %d: going home\n", ++(memory_sh->citac_akci), idZ);
    sem_post(output_sem); 
    exit(0);
}

//Funkce pro nahodny vyber neprazdne fronty
int vyberfrontu (){
    
    int pocet_nepraznych_front = 0;
    if (memory_sh->pocet_lidi_dopisy > 0) {
        pocet_nepraznych_front++;
    }
    if (memory_sh->pocet_lidi_baliky > 0) {
        pocet_nepraznych_front++;
    }
    if (memory_sh->pocet_lidi_peneznisluzby > 0) {
        pocet_nepraznych_front++;
    }

    srand(time(NULL) * getpid());
    int nahodne_cislo = rand() % (pocet_nepraznych_front + 1);
    int rada_k_obsluze = 0;
    
    if (memory_sh->pocet_lidi_dopisy > 0) {
        nahodne_cislo--;
        if (nahodne_cislo == 0) {
            rada_k_obsluze = 1;
        }
    }
    if (memory_sh->pocet_lidi_baliky > 0) {
        nahodne_cislo--;
        if (nahodne_cislo == 0) {
            rada_k_obsluze = 2;
        }
    }
    if (memory_sh->pocet_lidi_peneznisluzby > 0) {
        nahodne_cislo--;
        if (nahodne_cislo == 0) {
            rada_k_obsluze = 3;
        }
    }

    return rada_k_obsluze;
}

void proces_urednik(int idU){
    sem_wait(output_sem);
    fprintf(output_file,"%d: U %d: started\n", ++(memory_sh->citac_akci), idU);
    sem_post(output_sem); 

    while(true){
        sem_wait(mutex);
        int obsluhovana_rada = vyberfrontu();
        sem_post(mutex);

        if(obsluhovana_rada > 0){
            sem_wait(output_sem);
            fprintf(output_file,"%d: U %d: serving a service of type %d\n", ++(memory_sh->citac_akci), idU, obsluhovana_rada);
            sem_post(output_sem); 

            if(obsluhovana_rada == 1){
                memory_sh->pocet_lidi_dopisy-= 1;
                sem_post(fronta_dopisy);
            } else if (obsluhovana_rada == 2){
                memory_sh->pocet_lidi_baliky-= 1;
                sem_post(fronta_baliky);
            } else if (obsluhovana_rada == 3){
                memory_sh->pocet_lidi_peneznisluzby-= 1;
                sem_post(fronta_peneznisluzby);
            }

            srand(time(NULL) * getpid());
            int cekani_po_sluzbe = rand() % 10 + 1;
            usleep(cekani_po_sluzbe * 1000);

            sem_wait(output_sem);
            fprintf(output_file,"%d: U %d: service finished\n", ++(memory_sh->citac_akci), idU);
            sem_post(output_sem);
            continue; 
        }

        if (memory_sh->pocet_lidi_baliky == 0 && memory_sh->pocet_lidi_dopisy == 0 && memory_sh->pocet_lidi_peneznisluzby == 0 && memory_sh->post_open == true){
            sem_wait(output_sem);
            fprintf(output_file,"%d: U %d: taking break\n", ++(memory_sh->citac_akci), idU);
            sem_post(output_sem); 

            srand(time(NULL) * getpid() * idU);
            int cas_pauza = rand() % max_delka_prestavky + 1;
            usleep(cas_pauza * 1000);

            sem_wait(output_sem);
            fprintf(output_file,"%d: U %d: break finished\n", ++(memory_sh->citac_akci), idU);
            sem_post(output_sem); 
            continue;
        }
        if(memory_sh->post_open == false && memory_sh->pocet_lidi_baliky == 0 && memory_sh->pocet_lidi_dopisy == 0 && memory_sh->pocet_lidi_peneznisluzby == 0){
            sem_wait(output_sem);
            fprintf(output_file,"%d: U %d: going home\n", ++(memory_sh->citac_akci), idU);
            sem_post(output_sem);
            exit(0);
        }
        
    }
}

int main(int argc, char* argv[]){
    
    if (!open_file()){
        return 1;
    }

    if(argc != 6){
        fprintf(stderr, "Bad amount of arguments.");
        fclose(output_file);
        return 1;
    }

    if(argument_parsing(argv) == -1){
        fprintf(stderr, "Invalid number in argument.");
        fclose(output_file);
        return 1;
    }

    shared_mem_init();
    semaphore_init();

    for (int i = 0; i < pocet_zakazniku; i++) {
        pid_t pid = fork(); 
        if (pid == 0) {
            proces_zakaznik(i+1);
            exit(0);
        } else if (pid < 0) {
            fprintf(stderr, "Cannot create a child process.");
            fclose(output_file);
            exit (1);
        }
    }

    for (int i = 0; i < pocet_uredniku; i++) {
        pid_t pid = fork(); 
        if (pid == 0) {
            proces_urednik(i+1);
            exit(0);
        } else if (pid < 0) {
            fprintf(stderr, "Cannot create a child process.");
            fclose(output_file);
            exit (1);
        }
    }
    srand(time(NULL) * getpid());
    long cas_do_uzavreni = (rand() % (max_uzavreno_pro_nove/2 + 1)) + (max_uzavreno_pro_nove/2);
    usleep(cas_do_uzavreni*1000);

    sem_wait(mutex);
    fprintf(output_file,"%d: closing\n", ++(memory_sh->citac_akci));
    memory_sh->post_open = false;
    sem_post(mutex); 

    while(wait(NULL) > 0);

    shared_clean();
    fclose(output_file);
    exit (0);
}