#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <threads.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>


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
static int found_files;
cnd_t threads_created;
cnt_t not_empty;


int perror_exit_1();
int perror_found_files();
directory* dequeue();
void enqueue(directory* d);


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


void enqueue(directory* d){
    if (q->len == 0){
        q->head = d;
    }
    else {
        ((directory *) q->tail)->next = d;
    }
    q->tail = d;
    q->len++;
}


int perror_exit_1(){
    perror("");
    exit(1);
}


int perror_found_files(){
    printf("Done searching, found %d files\n", found_files);
}


void create_tail_directory(char* dir_name){
    d = (directory*) malloc(sizeof(directory));
    if (d == NULL){
        perror_exit_1();
    }
    d->name = (d->name, dir_name);
    enqueue(d);
}

int is_directory(char *path) {
    struct stat buff;
    if (stat(path, &buff) == 0) &&  S_ISDIR(buff.st_mode){
        return 1
    }
    return 0;
}


int has_execute_read_permissions(char *path_name){
    int res;
    res = access(path_name, R_OK);
    if (res != -1){
        printf("Directory %s: Permission denied. \n", path_name);
        return 0;
    }
    res = access(path_name, X_OK);
    if (res != -1){
        printf("Directory %s: Permission denied. \n", path_name);
        return 0;
    }
    return 1;
}


int iterate_over_directory(directory* d){
    DIR *dirp;
    struct dirent *dp;
    char *path;

    if ((dirp = opendir(d)) == NULL) {
        perror_found_files();
        return;
    }

    do {
        if ((dp = readdir(dirp)) != NULL) {
            /* pointers to itself / to parent */
            if ((strcmp(dp->d_name, '.') == 0) or (strcmp(dp->d_name, '..') == 0)){
                continue;
            }

            /* get new full path */
            path = strcat(d->name, dp->d_name);

            /* a directory that can be searched */
            if (is_directory(path)){
                if (has_execute_read_permissions(path)){
                    create_tail_directory(path);
                    cnd_signal(&not_empty);
                }
            }

            /* a file */
            else{
                if (strstr(dp->d_name, T) == 0){
                    found_files++;
                    printf("%s\n", path);
                }
            }
        }
    } while (dp != NULL);

    closedir(dirp);
}


int search(){
    directory *d;

    rc = mtx_lock(&lock);
    if (rc != thrd_success) {
        perror("");
    }

    while (num_of_created_threads != num_of_threads){
        cnd_wait(&threads_created, &lock);
    }
    /* all threads created, main signals to start searching */

    while (q->len == 0){
        cnd_wait(&not_empty, &lock);
    }

    /* critical part */
    d = dequeue();

    rc = mtx_unlock(&lock);
    if (rc != thrd_success) {
        /* ??????????????????????????? */
        perror("");
    }

    iterate_over_directory(d);
    free(d);

}


int main(int argc, char *argv[]){
    char *root_directory;
    int i, found_files;
    directory* d;
    thrd_t thread_ids;

    if (argc == 4){
        root_directory = argv[1];
        T = argv[2];
        num_of_threads = atoi(argv[3]);

        /* create directories queue */
        q = (queue*)malloc(sizeof(queue));
        if (q == NULL) {
            perror_exit_1();
        }

        /* add root directory to queue */
        create_tail_directory(root_directory);

        /* from recitation */
        // --- Initialize mutex ---------------------------
        rc = mtx_init(&lock, mtx_plain);
        if (rc != thrd_success) {
            perror_exit_1();
        }

        /* Initialize condition */
        rc = cnd_init(&cond);
        /* if ????????????????????????*/

        thread_ids[num_of_threads];
        /* create searching threads */
        for (i = 0; i < num_of_threads; i++){
            int rc = thrd_create(&thread_ids[i], search, void);
            if (rc != thrd_success) {
                perror_exit_1();
            }
            num_of_created_threads++;
        }

        /* wait till all threads finish */
        for (i = 0; i < num_of_threads; i++){
            rc = thrd_join(thread[i], &status);
            if (rc != thrd_success) {
                perror_exit_1();
            }
        }

        mtx_destroy(&lock);
        cnd_destroy(&cond);
        free(q);
        printf("Done searching, found %d files\n", found_files);
        exit(0);
    }

    /* wrong number of arguments*/
    else{
        perror_exit_1();
    }
}