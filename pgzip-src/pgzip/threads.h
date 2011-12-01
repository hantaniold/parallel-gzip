#ifndef THREADS
#define THREADS


/***********************************************************************/
/*      Extensible Array Types and Interfaces                          */
/***********************************************************************/

typedef struct vector
{
    void* elements;
    unsigned int element_size;
    unsigned int total_elements;
    unsigned int occupied_elements;
} vector;

vector* init_vector(unsigned int num_elements, unsigned int element_size);
void memcpy_safe(vector* vec, void* memory, unsigned int number_of_elements);
void destroy_vector(vector* vec);


/***********************************************************************/
/*      Queue Types and Interfaces                                     */
/***********************************************************************/

typedef struct _queue_node
{
	struct _queue_node* next;
	void* object;
} queue_node;


typedef struct _queue
{
	queue_node* first;
	queue_node* last;
    int size;
} queue;

void enqueue(queue *q, void* object);

// Recall that this will return a void pointer
// you must catch it, and free it manually
// the actual queue_node however is freed in this function
void* dequeue(queue *q);
queue* initialize_queue();
int queue_empty(const queue* q);


/***********************************************************************/
/*      Sorted Linked List Types and Interfaces                        */
/***********************************************************************/


typedef struct sorted_linked_list_node
{
    int index;
    void* data;
    struct sorted_linked_list_node* next;
} sorted_linked_list_node;


typedef struct sorted_linked_list
{
    unsigned int length;
    sorted_linked_list_node* head; 
} sorted_linked_list;


sorted_linked_list* init_sorted_linked_list();
void destroy_sorted_linked_list(sorted_linked_list* lst);
void print_sorted_linked_list(sorted_linked_list* lst);


void insert_into_sorted_linked_list(sorted_linked_list* list_to_insert_into, int index, void* data);
void* remove_from_sorted_linked_list(sorted_linked_list* list, int index);
void* pop_top(sorted_linked_list* list);
int peek_top(sorted_linked_list* list);

/***********************************************************************/
/*      Thread Pool Types and Interfaces                               */
/***********************************************************************/


typedef struct work_st
{
    void* (*routine) (void*);
    void* arg;
} work_t;


typedef struct _threadpool_st {
    int num_threads;            //number of active threads
    int qsize;                  //number in the queue
    
	pthread_mutex_t pending_job_requests_lock;
	pthread_cond_t pending_job_requests_cond;
    queue* pending_job_requests;

    pthread_mutex_t completed_threads_lock;		//lock on the queue list
    //pthread_cond_t completed_threads_cond;	//non empty condition variable
   	queue* completed_threads;
    
    queue* free_threads;
    void** busy_threads;
    
    int shutdown;
} threadpool;


typedef struct _spec_thread
{
    int busy;
	threadpool* pool;
	int shutdown;
	int	thread_id;
    pthread_t thread;
    pthread_mutex_t thread_lock;
    pthread_cond_t thread_cond;
    void* work; 
} spec_thread;

threadpool* create_threadpool(unsigned int num_threads_in_pool);
void dispatch(threadpool* from_me, void* (*dispatch_to_here) (void*), void *arg);
void* do_work(void* p);

#endif
