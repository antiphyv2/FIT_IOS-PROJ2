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


FILE *output_file;


sem_t *output_sem;
sem_t *urednik;
sem_t *zakaznik;
sem_t *urednik_done;
sem_t *zakaznik_done;

long pocet_zakazniku;
long pocet_uredniku;
long max_cekani_na_postu;
long max_delka_prestavky;
long max_uzavreno_pro_nove;

typedef struct shared {
    int counter_action;
    bool post_open;
} shared_mem;

shared_mem* memory_sh;
bool open_file(){

    output_file = fopen("proj2.out", "w");
    if(output_file == NULL){
        fprintf(stderr, "Error handling proj2.out");
        return false;
    }

    setbuf(output_file, NULL);
    return true;
}

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

    if(pocet_zakazniku < 1 || pocet_uredniku < 1){
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

void shared_counter_init(){
    memory_sh = mmap(NULL, sizeof(shared_mem), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    memory_sh->counter_action = 0;
    memory_sh->post_open = true;
}
void semaphore_init(){
    output_sem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    sem_init(output_sem, 1, 1);
    
}

void shared_clean(){
    munmap(output_sem, sizeof(sem_t));
    munmap(memory_sh, sizeof(sem_t));
    sem_destroy(output_sem);
}

void proces_zakaznik(int idZ){
    sem_wait(output_sem);
    fprintf(output_file,"%d: Z %d started.\n", ++(memory_sh->counter_action), idZ);
    sem_post(output_sem); 

    if(memory_sh->post_open == false){
        sem_wait(output_sem);
        fprintf(output_file,"%d: Z %d going home.\n", ++(memory_sh->counter_action), idZ);
        sem_post(output_sem); 
        exit(0);
    }

    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned int seed = tv.tv_usec;

    srand(seed);
    int Cislo_prepazky = rand() % 3 + 1;
    sem_wait(output_sem);
    fprintf(output_file,"%d: Z %d entering office for a service %d.\n", ++(memory_sh->counter_action), idZ, Cislo_prepazky);
    sem_post(output_sem); 

}

void proces_urednik(int idU){
    sem_wait(output_sem);
    fprintf(output_file,"%d: U %d started.\n", ++(memory_sh->counter_action), idU);
    sem_post(output_sem); 
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

    srand(time(0));

    shared_counter_init();
    semaphore_init();

    for (int i = 0; i < pocet_zakazniku; i++) {
        pid_t pid = fork(); 
        if (pid == 0) {
            //printf("Child process %d with PID %d\n", i + 1, getpid());
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
            //printf("Child process %d with PID %d\n", i + 1, getpid());
            exit(0);
        } else if (pid < 0) {
            fprintf(stderr, "Cannot create a child process.");
            fclose(output_file);
            exit (1);
        }
    }
   
    long waiting_time = max_uzavreno_pro_nove + (rand() % (max_uzavreno_pro_nove + 1));
    usleep(waiting_time);
    fprintf(output_file,"%d: Closing.\n", ++(memory_sh->counter_action));
    memory_sh->post_open = false;

    for (int i = 0; i < pocet_zakazniku + pocet_uredniku; i++) {
        wait(NULL);
    }

    shared_clean();
    fclose(output_file);
    
    printf("All child processes have exited.\n");

    exit (0);
}