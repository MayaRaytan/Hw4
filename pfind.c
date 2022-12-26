#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <threads.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#define True 1

typedef struct directory {
    char name[PATH_MAX];
    struct directory* next;
}directory;

typedef struct queue{
    /* number of items in queue*/
    int len;
    directory *head;
    directory *tail;
}queue;



static int num_of_created_threads;
char *T;
static queue* q;
int num_of_threads;
mtx_t lock;
mtx_t lock_found_files;
mtx_t lock_waiting_threads;
static int found_files;
cnd_t all_threads_created;
cnd_t not_empty;
cnd_t active_thread;
/* currently working thread */
int curr_id = 0;
/* maximum id of sleeping thread */
int max_id = 0;
int waiting_threads = 0;


int perror_exit_1();
void perror_found_files();
directory* dequeue();
void enqueue(char* dir_name);
int is_directory(char *path);
int has_execute_read_permissions(char *path_name);
void iterate_over_directory(directory* d);
int search(void* idx);


directory* dequeue(){
    /* queue is empty */
    if (q->len == 0){
        return NULL;
    }
    directory *keep_head;
    keep_head = q->head;
    q->head = ((directory*)q->head)->next;
    q->len--;
    return keep_head;
}


void enqueue(char* dir_name){
    directory* d;
    d = (directory*) malloc(sizeof(directory));
    if (d == NULL){
        perror_exit_1();
    }
    strcpy(d->name, dir_name);

    if (q->len == 0){
        q->head = d;
        q->len = 1;
    }
    else {
        ((directory *) q->tail)->next = d;
        q->len++;
    }
    q->tail = d;
}


int perror_exit_1(){
    perror("");
    exit(1);
}


void perror_found_files(){
    printf("Done searching, found %d files\n", found_files);
}




int is_directory(char *path) {
    struct stat buff;
    if ((stat(path, &buff) == 0) && S_ISDIR(buff.st_mode)){
        return 1;
    }
    return 0;
}


int has_execute_read_permissions(char *path_name){
    if ((access(path_name, R_OK) == -1) || (access(path_name, X_OK) == -1)) {
        printf("Directory %s: Permission denied. \n", path_name);
        return 0;
    }
    return 1;
}


void iterate_over_directory(directory* d){
    DIR *dirp;
    struct dirent *dp;
    char *path;
    char *dname;

    dname = d->name;
    free(d);

    if ((dirp = opendir(dname)) == NULL) {
        perror_found_files();
        return;
    }

    while ((dp = readdir(dirp)) != NULL) {
            /* pointers to itself / to parent */
            if ((strcmp(dp->d_name, ".") == 0) || (strcmp(dp->d_name, "..") == 0)){
                continue;
            }

            /* get new full path */
            path = strcat(dname, dp->d_name);

            /* a directory that can be searched */
            if (is_directory(path)){
                if (has_execute_read_permissions(path)){
                    mtx_lock(&lock);
                    enqueue(path);
                    mtx_unlock(&lock);
                    cnd_signal(&not_empty);
                }
            }

            /* It's a file */
            else{
                if (strstr(dp->d_name, T) == 0){
                    mtx_lock(&lock_found_files);
                    found_files++;
                    mtx_unlock(&lock_found_files);
                    printf("%s\n", path);
                }
            }
    }

    closedir(dirp);
}


int search(void* idx){
    directory *d;
    int sleep_time;
    sleep_time = *(int*)idx;

    while (True) {
        mtx_lock(&lock);

        while (num_of_created_threads != num_of_threads) {
            waiting_threads++;
            cnd_wait(&all_threads_created, &lock);
            waiting_threads--;
        }
        /* all threads created, main signals to start searching */

        while (q->len == 0) {
            sleep_time = max_id % num_of_threads;
            max_id++;
            max_id = max_id % num_of_threads;
            waiting_threads++;
            cnd_wait(&not_empty, &lock);
            waiting_threads--;
            curr_id++;
            curr_id = curr_id % num_of_threads;
        }

        while (sleep_time != curr_id){
            waiting_threads++;
            cnd_wait(&active_thread, &lock);
            waiting_threads--;
        }

        /* critical part */
        d = dequeue();
        mtx_unlock(&lock);

        iterate_over_directory(d);

        cnd_signal(&active_thread);

    }
    return 1;
}


int main(int argc, char *argv[]){
    char *root_directory;
    int found_files, rc;
    size_t i;

    if (argc == 4){
        root_directory = argv[1];
        T = argv[2];
        num_of_threads = atoi(argv[3]);

        if (!is_directory(root_directory) || !has_execute_read_permissions(root_directory)){
            perror_exit_1();
        }

        /* create directories queue */
        q = (queue*)malloc(sizeof(queue));
        if (q == NULL) {
            perror_exit_1();
        }

        /* add root directory to queue */
        enqueue(root_directory);

        /* initialize mutex */
        mtx_init(&lock, mtx_plain);

        /* initialize condition */
        cnd_init(&all_threads_created);
        cnd_init(&not_empty);
        cnd_init(&active_thread);

        thrd_t thread_ids[num_of_threads];
        /* create searching threads */
        for (i = 0; i < num_of_threads; i++){
            rc = thrd_create(&thread_ids[i], search, (void *)i);
            if (rc != thrd_success) {
                perror_exit_1();
            }
            num_of_created_threads++;
        }
        cnd_signal(&all_threads_created);

        if (waiting_threads == num_of_threads){
            exit(0);
        }
//        /* wait till all threads finish */
//        for (i = 0; i < num_of_threads; i++){
//            rc = thrd_join(thread_ids[i], NULL);
//            if (rc != thrd_success) {
//                perror_exit_1();
//            }
//        }

        mtx_destroy(&lock);
        cnd_destroy(&all_threads_created);
        cnd_destroy(&not_empty);
        free(q);
        printf("Done searching, found %d files\n", found_files);
        thrd_exit(0);
    }

    /* wrong number of arguments*/
    else{
        perror_exit_1();
    }
}