#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "threads.h"

/***********************************************************************/
/*      Generic Extensible Array                                       */
/***********************************************************************/

vector* init_vector(unsigned int num_elements, unsigned int element_size)
{
    vector* v = (vector*) malloc(sizeof(vector));
    v->elements = (void*) malloc(element_size * num_elements);
    v->total_elements = num_elements; v->occupied_elements = 0;
    v->element_size = element_size;
    return v;
}

void memcpy_safe(vector* vec, void* memory, unsigned int number_of_elements)
{
    unsigned int free_elements = vec->total_elements - vec->occupied_elements;
    if(number_of_elements > free_elements)
    {
        if(number_of_elements + vec->occupied_elements > 2 * vec->total_elements)
        {
            vec->elements = realloc(vec->elements, 2 * vec->element_size * (vec->occupied_elements + number_of_elements));
            vec->total_elements = 2 * (vec->occupied_elements + number_of_elements);
        }
        else
        {
            vec->elements = realloc(vec->elements, 2 * vec->element_size * vec->total_elements);
            vec->total_elements = 2 * vec->total_elements;
        }
    }

    memcpy(((char*)vec->elements) + (vec->occupied_elements * vec->element_size), (char*)memory, vec->element_size * number_of_elements);
    vec->occupied_elements += number_of_elements;
}

void destroy_vector(vector* vec)
{
    free(vec->elements);
    free(vec); vec = NULL;
}





/***********************************************************************/
/*      Generic Queue Interface                                        */
/***********************************************************************/

// Queue Data Structure - http://en.wikipedia.org/wiki/Queue_(data_structure)

void enqueue(queue* q, void* object)
{
	queue_node* node = (queue_node*) malloc(sizeof(queue_node));
	node->object = object;
	if(q->first == NULL && q->last == NULL)
	{
		q->first = node;
		q->last = node;
	}

	else
	{
		q->last->next = node;
		q->last = node;
	}
	
    q->size += 1;
	node->next = NULL;
}

// Recall that this will return a void pointer
// you must catch it, and free it manually
// the actual queue_node however is freed in this function
void* dequeue(queue* q)
{	
	void* pointer = NULL;
	if(q->first == NULL) pointer = (void*) NULL;
	pointer = q->first->object;
	queue_node* temp = q->first;
	if(q->first == q->last)
	{ q->first = NULL; q->last = NULL; }
	else{ q->first = (q->first)->next; }
	free(temp); temp = NULL;
    q->size -= 1;
	return pointer;	
}

queue* initialize_queue()
{
    queue* q = (queue *) malloc(sizeof(queue));
	q->first = NULL;
	q->last = NULL;
    q->size = 0;
    return q;
}

int queue_empty(const queue* q)
{
	return (q->first == NULL);
}


queue* queue_join(queue* p1, queue* p2)
{
	queue* join = (queue*)malloc(sizeof(queue));
	join->first = p1->first;
	join->last = p2->last;
	p1->last->next = p2->first;
	return join;
}



/***********************************************************************/
/*      Sorted Linked List Interfaces                                  */
/***********************************************************************/

sorted_linked_list* init_sorted_linked_list()
{
    sorted_linked_list* lst = (sorted_linked_list*) malloc(sizeof(sorted_linked_list));
    if (lst == NULL) { fprintf(stderr, "Out of memory creating a new Sorted Linked List!\n"); return NULL; }
    lst->length = 0; lst->head = NULL; return lst;
}


void destroy_sorted_linked_list(sorted_linked_list* lst)
{
    sorted_linked_list_node* temp = NULL;
    sorted_linked_list_node* node = lst->head;
    if(node != NULL)
    {
        temp = node; node = node->next;
        free(temp->data); free(temp);
    }

    free(lst); lst = NULL;
}


void print_sorted_linked_list(sorted_linked_list* lst)
{
    //printf("List Contains %d Elements\n-----------------------\n", lst->length);
    sorted_linked_list_node* cur = lst->head;
    if(lst->length != 0)
    {
        while(cur != NULL) { //printf("%d -> ", cur->index); 
			cur = cur->next;}

        //printf("NULL\n");        
    }
}


// Insertion with equal indices always places the new item after the one already in list
void insert_into_sorted_linked_list(sorted_linked_list* list_to_insert_into, int index, void* data)
{
    sorted_linked_list_node* new_node = (sorted_linked_list_node*) malloc(sizeof(sorted_linked_list_node));
    new_node->index = index; new_node->data = data; new_node->next = NULL;

    list_to_insert_into->length += 1;
    if(list_to_insert_into->head == NULL)
    { list_to_insert_into->head = new_node; return; }

    sorted_linked_list_node* cur = list_to_insert_into->head;
    sorted_linked_list_node* prev = NULL;
    while(cur != NULL)
    {
        if(index < cur->index)
        {
            new_node->next = cur;
            if(prev != NULL) { prev->next = new_node; }
            else { list_to_insert_into->head = new_node; }
            return;
        }
        prev = cur; cur = cur->next;
    }

    prev->next = new_node;
}


void* remove_from_sorted_linked_list(sorted_linked_list* list, int index)
{
    sorted_linked_list_node* cur = list->head;
    sorted_linked_list_node* prev = NULL; void* data = NULL;
    while(cur != NULL)
    {
        if(cur->index == index)
        {
            list->length -= 1;
            data = cur->data;

            if(prev == NULL) { list->head = cur->next; }
            else { prev->next = cur->next; }
            free(cur); return data;
        }
        prev = cur;
        cur = cur->next;
    }
    return data;
}


void* pop_top(sorted_linked_list* list)
{ 
    if(list->head == NULL) return NULL;
    return remove_from_sorted_linked_list(list, (list->head)->index);
}


int peek_top(sorted_linked_list* list)
{
    if(list->head == NULL) return 0;
    else return list->head->index;
}

/***********************************************************************/
/*      Thread Pool Interfaces                                         */
/***********************************************************************/

spec_thread* new_spec_thread(int thread_id, threadpool* p)
{
    spec_thread* c = (spec_thread*) malloc(sizeof(spec_thread));
    pthread_mutex_init(&c->thread_lock, NULL);
    pthread_cond_init(&c->thread_cond, NULL);
    c->pool = p; c->work = NULL; c->busy = 0;
    c->shutdown = 0; c->thread_id = thread_id; 
    pthread_create(&c->thread, NULL, do_work, c);
    return c;
}

threadpool* create_threadpool(unsigned int num_threads_in_pool)
{
    if (num_threads_in_pool <= 0) return NULL;

    threadpool *pool = (threadpool*) malloc(sizeof(threadpool));
    pool->num_threads = num_threads_in_pool; pool->qsize = 0;
    pool->pending_job_requests = initialize_queue();
    pool->completed_threads = initialize_queue();
    pool->free_threads = initialize_queue();
    
    spec_thread* temp;
    pool->busy_threads = (void**) malloc(sizeof(temp) * num_threads_in_pool);
    pool->shutdown = 0;

    //initialize mutex and condition variables.  
    pthread_mutex_init(&pool->pending_job_requests_lock, NULL);
    pthread_mutex_init(&pool->completed_threads_lock, NULL);
    pthread_cond_init(&pool->pending_job_requests_cond, NULL);
    //pthread_cond_init(&pool->completed_threads_cond, NULL);
    
    //make threads
    int i;
    for (i = 0; i < num_threads_in_pool; i++)
    {   
        spec_thread* th = new_spec_thread(i, pool);
        enqueue(pool->free_threads, (void*) th); 
    }
  
    return pool;
}

/* This function is the work function of the thread */
void* do_work(void* arg)
{
    spec_thread* c = (spec_thread*) arg;
    while(1)
    {
		pthread_mutex_lock(&(c->thread_lock));
        while(c->shutdown == 0 && c->busy == 0)
            pthread_cond_wait(&(c->thread_cond),&(c->thread_lock));

	    if(c->shutdown)
        { pthread_mutex_unlock(&(c->thread_lock)); pthread_exit(NULL); }
        
        work_t* job = (work_t*) c->work;
        (job->routine) (job->arg); c->busy = 0;
        
        pthread_mutex_unlock(&(c->thread_lock));
        
        pthread_mutex_lock(&(c->pool->completed_threads_lock));
        enqueue(c->pool->completed_threads, &(c->thread_id));
        pthread_mutex_unlock(&(c->pool->completed_threads_lock));
       // printf("Sent Signal \n");
        
        pthread_cond_signal(&(c->pool->pending_job_requests_cond));
    }

    return NULL;
}


void dispatch(threadpool* pl, void* (*dispatch_to_here) (void*), void *arg)
{
    //printf("Hello Wordl!");
    work_t* wrk = (work_t*) malloc(sizeof(work_t));
    wrk->arg = arg; wrk->routine = dispatch_to_here;
    pthread_mutex_lock(&(pl->pending_job_requests_lock));
    enqueue(pl->pending_job_requests, wrk);
    pthread_mutex_unlock(&(pl->pending_job_requests_lock));
    pthread_cond_signal(&(pl->pending_job_requests_cond));
    //printf("Done!\n");
}



/***********************************************************************/
/*      IO Compression Thread Information (Gzip Specific)              */
/***********************************************************************/


/*typedef struct thread_compression_block
{
    unsigned char inbuf[INBUFSIZ + INBUF_EXTRA];
    unsigned char outbuf[OUTBUFSIZ + OUTBUF_EXTRA];
    unsigned short d_buf[DIST_BUFSIZE];
    unsigned char window [2L*WSIZE];
    unsigned short tab_prefix[1L << BITS];
    unsigned int chunk;
} thread_compression_block;*/


/*IOCompressThreadInfo* init_IOCompressThreadInfo(int input_fd, int output_fd)
{
    IOCompressThreadInfo* info = (IOCompressThreadInfo*) malloc(sizeof(IOCompressThreadInfo));
    if(info == NULL) { fprintf(stderr, "Out of memory creating a new Info Struct for Compression!\n"); return NULL; }

    info->processed_blocks = init_sorted_linked_list();
    if(info->processed_blocks == NULL) { free(info); return NULL; }

    info->next_block_to_write = 1;
    info->blocks_read = 0;

    //initialize mutex and condition variables.  
    if(pthread_mutex_init(&(info->output_block_lock),NULL)) { fprintf(stderr, "Mutex initiation error!\n"); return NULL; }
    if(pthread_mutex_init(&(info->input_block_lock), NULL)) { fprintf(stderr, "Mutex initiation error!\n"); return NULL; }
    if(pthread_cond_init(&(info->take_io_action),NULL)) { fprintf(stderr, "CV initiation error!\n"); return NULL; }

    info->input_fd = input_fd; info->output_fd = output_fd;
    return info;
}*/


/***********************************************************************/
/*      Main Sample Code Usage Samples                                 */
/***********************************************************************/


/*// Sample Code Demonstrating Sorted Linked List Usage
int main()
{
    sorted_linked_list* lst = init_sorted_linked_list();
    sorted_linked_list_node* node = pop_top(lst);
    remove_from_sorted_linked_list(lst, 423);    
    print_sorted_linked_list(lst);    
    insert_into_sorted_linked_list(lst, 43, NULL);
    print_sorted_linked_list(lst);
    insert_into_sorted_linked_list(lst, 25, NULL);
    insert_into_sorted_linked_list(lst, 3, NULL);
    insert_into_sorted_linked_list(lst, 55, NULL);
    remove_from_sorted_linked_list(lst, 55);
    pop_top(lst);
    print_sorted_linked_list(lst);
    destroy_sorted_linked_list(lst);
}

// Sample Code Demonstrating Pthread Usage
void* hello(void* _notused)
{
    printf("Hello From Thread %u\n", (unsigned int)pthread_self());
    sleep(15); return NULL;
}

int main()
{
    _threadpool* pool = create_threadpool(15); int i;
    for(i = 0; i != 50; i++) { dispatch(pool, hello, NULL); }
    destroy_threadpool(pool); return 0;
}*/
