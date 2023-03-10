#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#define True 1

typedef struct directory{
    char name[PATH_MAX];
    struct directory* next;
}directory;

typedef struct queue_dir{
    /* number of items in queue*/
    int len;
    directory *head;
    directory *tail;
}queue_dir;

typedef struct cv_node{
    cnd_t my_cnd;
    struct cv_node* next;
}cv_node;

typedef struct queue_cv{
    /* number of items in queue*/
    int len;
    cv_node *head;
    cv_node *tail;
}queue_cv;




int num_of_created_threads = 0;
char *T;
static queue_dir* q_dir;
static queue_cv* q_cv;
int num_of_threads;
mtx_t lock;
mtx_t lock_cv;
mtx_t lock_found_files;
int found_files = 0;
cnd_t exit_sign;
int errors = 0;


int perror_exit_1();
directory* dequeue_dir();
cv_node* dequeue_cv();
void enqueue_dir(char* dir_name);
void enqueue_cv(cv_node* cv);
int is_directory(char *path);
int has_execute_read_permissions(char *path_name);
void iterate_over_directory(directory* d);
void signal_all_threads();
int search(void* idx);
void malloc_q_cv();
void malloc_q_dir();




directory* dequeue_dir(){
    directory *keep_head;
    /* queue is empty */
    if (q_dir->len == 0){
        return NULL;
    }
    keep_head = q_dir->head;
    q_dir->head = ((directory*)q_dir->head)->next;
    q_dir->len--;
    return keep_head;
}

cv_node* dequeue_cv(){
    cv_node *keep_head;
    /* queue is empty */
    if (q_cv->len == 0){
        return NULL;
    }
    keep_head = q_cv->head;
    q_cv->head = ((cv_node*)q_cv->head)->next;
    q_cv->len--;
    return keep_head;
}


void enqueue_dir(char* dir_name){
    directory* d;
    d = (directory*) malloc(sizeof(directory));
    if (d == NULL){
        perror_exit_1();
    }
    strcpy(d->name, dir_name);

    if (q_dir->len == 0){
        q_dir->head = d;
        q_dir->len = 1;
    }
    else {
        ((directory *) q_dir->tail)->next = d;
        q_dir->len++;
    }
    q_dir->tail = d;
}

void enqueue_cv(cv_node* cv){
    if (q_cv->len == 0){
        q_cv->head = cv;
        q_cv->len = 1;
    }
    else {
        ((cv_node*) q_cv->tail)->next = cv;
        q_cv->len++;
    }
    q_cv->tail = cv;
}


int perror_exit_1(){
    perror("");
    exit(1);
}



int is_directory(char *path) {
    struct stat buff;
    if (stat(path, &buff) == -1){
        perror("");
        errors = 1;
    }
    if (S_ISDIR(buff.st_mode)){
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
    char path[PATH_MAX];
    char dname[PATH_MAX];
    cv_node *cv;

    strcpy(dname, d->name);
    free(d);

    if ((dirp = opendir(dname)) == NULL) {
        perror("");
        errors = 1;
        return;
    }

    while ((dp = readdir(dirp)) != NULL) {
            /* pointers to itself / to parent */
            if ((strcmp(dp->d_name, ".") == 0) || (strcmp(dp->d_name, "..") == 0)){
                continue;
            }

            /* get new full path */
            strcpy(path, dname);
            strcat(path, "/");
            strcat(path, dp->d_name);


            /* a directory that can be searched */
            if (is_directory(path)){
                if (has_execute_read_permissions(path)){
                    mtx_lock(&lock);
                    enqueue_dir(path);
                    mtx_unlock(&lock);

                    mtx_lock(&lock_cv);
                    cv = dequeue_cv();
                    /* if there are sleeping threads */
                    if (cv != NULL){
                        cnd_signal(&cv->my_cnd);
                    }
                    mtx_unlock(&lock_cv);

                }
            }

            /* It's a file */
            else{
                if (strstr(dp->d_name, T) != NULL){

                    mtx_lock(&lock_found_files);
                    found_files++;
                    mtx_unlock(&lock_found_files);
                }
            }
    }

    closedir(dirp);
}

void malloc_q_cv(){
    q_cv = (queue_cv *) malloc(sizeof(queue_cv));
    if (q_cv == NULL) {
        perror_exit_1();
    }
    q_cv->len = 0;
}

void malloc_q_dir(){
    q_dir = (queue_dir *) malloc(sizeof(queue_dir));
    if (q_dir == NULL) {
        perror_exit_1();
    }
    q_dir->len = 0;
}


void signal_all_threads(){
    cv_node *cv;
    while (q_cv->len > 0){
        cv = dequeue_cv();
        cnd_signal(&cv->my_cnd);
        cnd_destroy(&cv->my_cnd);
        free(cv);
    }
    free(q_cv);
}


int search(void* node){
    directory *d;
    cv_node *cv;
    cv = (cv_node *)node;
    cnd_init(&cv->my_cnd);


    while (True) {
        mtx_lock(&lock);
        while (q_dir->len == 0 || num_of_created_threads != num_of_threads) {
            mtx_lock(&lock_cv);

            /* I'm the last awake thread and there are no directories to search */
            if (q_cv->len == num_of_threads - 1){
                signal_all_threads();
                cnd_signal(&exit_sign);
            }

            enqueue_cv(cv);
            mtx_unlock(&lock_cv);
            cnd_wait(&cv->my_cnd, &lock);
            /* thread woke up by signal_all_threads just to exit */
            if (q_dir->len == 0){thrd_exit(0);}
        }
        d = dequeue_dir();
        mtx_unlock(&lock);
        iterate_over_directory(d);
    }
    return 1;
}


int main(int argc, char *argv[]){
    char *root_directory;
    int rc;
    int i;
    cv_node *cv;

    if (argc == 4){
        root_directory = argv[1];
        T = argv[2];
        num_of_threads = atoi(argv[3]);

        printf("%d", num_of_threads);

        if (!is_directory(root_directory) || !has_execute_read_permissions(root_directory)){
            perror_exit_1();
        }

        /* create queues */
        malloc_q_dir();
        malloc_q_cv();

        /* add root directory to queue */
        enqueue_dir(root_directory);

        /* initialize mutex */
        mtx_init(&lock, mtx_plain);
        mtx_init(&lock_cv, mtx_plain);
        mtx_init(&lock_found_files, mtx_plain);

        thrd_t thread_ids[num_of_threads];
        /* create searching threads */
        for (i = 0; i < num_of_threads; i++){
            cv = (cv_node *) malloc(sizeof(cv_node));
            if (cv == NULL){
                perror_exit_1();
            }

            rc = thrd_create(&thread_ids[i], search, (void *)cv);
            if (rc != thrd_success) {
                perror_exit_1();
            }
            printf("%d", i);
        }
        printf("finished creating threads");
        num_of_created_threads = num_of_threads;

        cnd_init(&exit_sign);
        cnd_wait(&exit_sign, &lock);

        free(q_dir);
        mtx_destroy(&lock);
        mtx_destroy(&lock_cv);
        mtx_destroy(&lock_found_files);
        printf("Done searching, found %d files\n", found_files);

        /* 0 if all threads didn't get errors, otherwise 1*/
        exit(errors);
    }

    /* wrong number of arguments*/
    else{
        perror_exit_1();
    }
}