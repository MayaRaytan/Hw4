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


int perror_exit_1();
directory* dequeue(queue q);
void enqueue(queue* q, directory* d);


directory* dequeue(queue q){
    directory *keep_head;
    keep_head = q->head;
    q->head = ((directory*)q->head)->next;
    q->len--;
    return keep_head;
}

void enqueue(queue* q, directory* d){
    ((directory*)q->tail)->next = d;
    q->tail = d;
    q->len++;
}

int perror_exit_1(){
    perror("");
    exit(1);
}

int main(int argc, char *argv[]){
    char *root_directory;
    char *T;
    int num_of_threads, i, found_files;
    queue* q;
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
        d = (directory*) malloc(sizeof(directory));
        d->name = (d->name, root_directory);
        enqueue(q,d);

        for (i = 0; i < num_of_threads; i++){
            thrd_t thread_id;
            int rc = thrd_create(&thread_id, thread_func, (void *)"hello");
            if (rc != thrd_success) {
            }
        }
    }



    }

    /* wrong number of arguments*/
    else{
        perror_exit_1();
    }
}