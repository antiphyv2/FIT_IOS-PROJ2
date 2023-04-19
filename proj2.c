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
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>


FILE *output_file;


sem_t *mutex;
sem_t *urednik;
sem_t *zakaznik;
sem_t *urednik_done;
sem_t *zakaznik_done;

long pocet_zakazniku;
long pocet_uredniku;
long max_cekani_na_postu;
long max_delka_prestavky;
long max_uzavreno_pro_nove;

int *counter_action;


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
    //printf("%ld, %ld, %ld, %ld, %ld", NZ, NU, TZ, TF, F);

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
    counter_action = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    *counter_action = 0;
}
void semaphore_init(){
    mutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    sem_init(mutex, 1, 1);
    
}

void shared_clean(){
    munmap(counter_action, sizeof(int));
    munmap(mutex, sizeof(sem_t));
    sem_destroy(mutex);
}

void proces_zakaznik(int ZID){
    sem_wait(mutex);
    fprintf(output_file,"%d: Zakaznik %d started.\n", ++(*counter_action), ZID);
    sem_post(mutex); 
}

void proces_urednik(int UID){
    sem_wait(mutex);
    fprintf(output_file,"%d: Urednik %d started.\n", ++(*counter_action), UID);
    sem_post(mutex); 
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
    fprintf(output_file,"%d: Closing.", ++(*counter_action));

    for (int i = 0; i < pocet_zakazniku + pocet_uredniku; i++) {
        wait(NULL);
    }

    shared_clean();
    fclose(output_file);
    printf("All child processes have exited.\n");

    exit (0);
}