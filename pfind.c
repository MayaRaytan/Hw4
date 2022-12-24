#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

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
queue* q;
int num_of_threads;
mtx_t lock;



int perror_exit_1();
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

void create_tail_directory(char* dir_name){
    d = (directory*) malloc(sizeof(directory));
    if (d == NULL){
        perror_exit_1();
    }
    d->name = (d->name, dir_name);
    enqueue(d);
}

int is_directory(const char *path) {
    struct stat buff;
    if (stat(path, &buff) != 0)
        return 0;
    return S_ISDIR(buff.st_mode);
}

int has_execute_read_permissions(char *dir_name){
    int res;
    res = access(dir_name, R_OK);
    if (res != -1){
        printf("Directory %s: Permission denied. \n", dir_name);
        return 0;
    }
    res = access(dir_name, X_OK);
    if (res != -1){
        printf("Directory %s: Permission denied. \n", dir_name);
        return 0;
    }
    return 1;
}

int iterate_over_directory(directory* d){
    DIR *dirp;
    struct dirent *dp;

    if ((dirp = opendir(d)) == NULL) {

        perror_exit_1();
    }

    do {
        if ((dp = readdir(dirp)) != NULL) {
            /* pointers to itself / to parent */
            if ((strcmp(dp->d_name, '.') == 0) or (strcmp(dp->d_name, '..') == 0)){
                continue;
            }

            /* a directory that can be searched */
            if (is_directory(dp)){
                if (has_execute_read_permissions(dp->d_name)){
                    create_tail_directory(dp->d_name);
                }
            }

            /* a file */
            else{

            }



        }
    } while (dp != NULL);

    closedir(dirp);
}

int search(){
    directory *d;


    while (num_of_created_threads != num_of_threads){}
    /* all threads created, main signals to start searching */

    /* critical part */
    rc = mtx_lock(&lock);
    if (rc != thrd_success) {
        perror_exit_1();
    }

    while (q->len == 0){
        /* go out????????????????????? */
    }

    d = dequeue();

    iterate_over_directory(d);

    rc = mtx_unlock(&lock);
    if (rc != thrd_success) {

    }

}



int main(int argc, char *argv[]){
    char *root_directory;
    int i, found_files;
    directory* d;

    if (argc == 4){
        root_directory = argv[1];
        T = argv[2];
        num_of_threads = atoi(argv[3]);

        found_files = 0;

        /* validate that root directory can be searched */
        /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
        if (root_directory){}

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
            printf("ERROR in mtx_init()\n");
            exit(-1);
        }


        /* create searching threads */
        for (i = 0; i < num_of_threads; i++){
            thrd_t thread_id;
            int rc = thrd_create(&thread_id, search, (void *)"hello");
            if (rc != thrd_success) {
                perror_exit_1();
            }
            num_of_created_threads++;
        }
        mtx_destroy(&lock);
    }


    /* wrong number of arguments*/
    else{
        perror_exit_1();
    }
}