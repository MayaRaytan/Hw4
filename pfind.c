#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <limits.h>

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



int perror_exit_1();
directory* dequeue(queue q);
void enqueue(queue* q, directory* d);


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

void enqueue(queue* q, directory* d){
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

void create_head_directory(char* dir_name){
    d = (directory*) malloc(sizeof(directory));
    if (d == NULL){
        perror_exit_1();
    }
    d->name = (d->name, dir_name);
    enqueue(q,d);
}

int search(){
    while (num_of_created_threads != num_of_threads){}
    /* all threads created, main signals to start searching */

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
        create_head_directory(root_directory);

        /* create searching threads */
        for (i = 0; i < num_of_threads; i++){
            thrd_t thread_id;
            int rc = thrd_create(&thread_id, search, (void *)"hello");
            if (rc != thrd_success) {
                perror_exit_1();
            }
            num_of_created_threads++;
        }

    }


    /* wrong number of arguments*/
    else{
        perror_exit_1();
    }
}