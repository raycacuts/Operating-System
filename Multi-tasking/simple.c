/*
 * assign2.c
 *
 * Name:
 * Student Number:
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h> 
#include <pthread.h>
#include "train.h"

typedef struct Node{
    pthread_cond_t convar_arrival;
    pthread_cond_t convar_across;
    //pthread_cond_t convar_leave;
    struct TrainInfo* train;
    struct Node* next;
    int waitingForMutex;
}Node;
Node *west; 
Node *east;

pthread_mutex_t lock;
pthread_cond_t convar_leave = PTHREAD_COND_INITIALIZER; 
pthread_cond_t convar_wait_next = PTHREAD_COND_INITIALIZER; 

int isEmpty;
int eastPassed;
int isConflicted;

int len(Node *root){
    int length = 0;
    Node *temp = (Node *)malloc(sizeof(Node));
    for(temp = root; temp != NULL; temp=temp->next){
        length ++;
    }
    return length;
}
Node* add_node(Node *root, Node *new){
    if(root == NULL){
        return new;
    }
    else if(new->train->length < root->train->length){
        new->next = root;
        return new;
    }
    else{
        Node *temp = (Node *)malloc(sizeof(Node));
        for(temp = root; temp->next != NULL; temp=temp->next){
            if(new->train->length < temp->next->train->length){
                new->next = temp->next;
                temp->next = new;
                return root;
            }
        }
        temp->next = new;
        return root;
    }
}
Node* search(Node *root, int id){
    Node *temp = (Node *)malloc(sizeof(Node));
    for(temp = root; temp != NULL; temp=temp->next){
        if(temp->train->trainId == id){
            return temp;
        }
    }
    return NULL;
}
Node* remove_node(Node *root, Node *del){
    if(root->train->trainId == del->train->trainId){
        if(root->next == NULL){
            return NULL;
        }
        else{
            return root->next;
        }
    }
    else{
        Node *temp = (Node *)malloc(sizeof(Node));
        for(temp = root; temp->next != NULL; temp=temp->next){
            if(temp->next->train->trainId == del->train->trainId){
                temp->next = temp->next->next;
                return root;
            }
        }
        return root;
    }
}
void print_node(Node *root){
    if(root == NULL){
        printf("It's empty\n");
    }
    Node *temp = (Node *)malloc(sizeof(Node));
    for(temp = root; temp != NULL; temp=temp->next){
        printf("Train id %d, length: %d, arrival: %d\n", temp->train->trainId, temp->train->length, temp->train->arrival);
    }
    free(temp);
}
/*
 * If you uncomment the following line, some debugging
 * output will be produced.
 *
 * Be sure to comment this line out again before you submit 
 */

/* #define DEBUG	1 */

void ArriveBridge (TrainInfo *train);
void CrossBridge (TrainInfo *train);
void LeaveBridge (TrainInfo *train);

/*
 * This function is started for each thread created by the
 * main thread.  Each thread is given a TrainInfo structure
 * that specifies information about the train the individual 
 * thread is supposed to simulate.
 */
void * Train ( void *arguments )
{
	TrainInfo	*train = (TrainInfo *)arguments;

	/* Sleep to simulate different arrival times */
	usleep (train->length*SLEEP_MULTIPLE);

	ArriveBridge (train);
	CrossBridge  (train);
	LeaveBridge  (train); 

	/* I decided that the paramter structure would be malloc'd 
	 * in the main thread, but the individual threads are responsible
	 * for freeing the memory.
	 *
	 * This way I didn't have to keep an array of parameter pointers
	 * in the main thread.
	 */
	free (train);
	return NULL;
}

/*
 * You will need to add code to this function to ensure that
 * the trains cross the bridge in the correct order.
 */
void ArriveBridge ( TrainInfo *train )
{
	//printf ("Train %2d arrives going %s\n", train->trainId, 
	//		(train->direction == DIRECTION_WEST ? "West" : "East"));
	/* Your code here... */

    
    Node *temp;
    if(train->direction == DIRECTION_EAST){
        temp = search(east, train->trainId);
        temp->train->arrival = 1;
    }
    else{
        temp = search(west, train->trainId);
        temp->train->arrival = 1;
    }
    isEmpty = 1;

    temp->waitingForMutex=1;
    pthread_mutex_lock(&lock);
    pthread_cond_signal(&temp->convar_arrival);
    temp->waitingForMutex=0;
    pthread_cond_wait(&temp->convar_across, &lock);
    
    isEmpty = 0;
    
    //printf("running\n");
    
}

/*
 * Simulate crossing the bridge.  You shouldn't have to change this
 * function.
 */
void CrossBridge ( TrainInfo *train )
{
	//printf ("Train %2d is ON the bridge (%s)\n", train->trainId,
	//		(train->direction == DIRECTION_WEST ? "West" : "East"));
	fflush(stdout);
	
	/* 
	 * This sleep statement simulates the time it takes to 
	 * cross the bridge.  Longer trains take more time.
	 */
	usleep (train->length*SLEEP_MULTIPLE);

	printf ("Train %2d is OFF the bridge(%s)\n", train->trainId, 
			(train->direction == DIRECTION_WEST ? "West" : "East"));
	fflush(stdout);
}

/*
 * Add code here to make the bridge available to waiting
 * trains...
 */
void LeaveBridge ( TrainInfo *train )
{
    if(train->direction == DIRECTION_EAST){
        pthread_cond_signal(&convar_leave);
        east=remove_node(east,east);
    }
    else{
        pthread_cond_signal(&convar_leave);
        west=remove_node(west, west);
    }
    
    isEmpty = 1;
    pthread_mutex_unlock(&lock);
    pthread_exit(NULL);
}

int main ( int argc, char *argv[] )
{
    east = (Node *)malloc(sizeof(Node));
    east = NULL;
    west = (Node *)malloc(sizeof(Node));
    west = NULL;
   
    int passed = 0;
    int trainPassing = DIRECTION_NONE;
    isEmpty = 1;
    eastPassed = 0;
    isConflicted = 0;
    
	int		trainCount = 0;
	char 		*filename = NULL;
	pthread_t	*tids;
	int		i;
    
		
	/* Parse the arguments */
	if ( argc < 2 )
	{
		printf ("Usage: part1 n {filename}\n\t\tn is number of trains\n");
		printf ("\t\tfilename is input file to use (optional)\n");
		exit(0);
	}
	
	if ( argc >= 2 )
	{
		trainCount = atoi(argv[1]);
	}
	if ( argc == 3 )
	{
		filename = argv[2];
        
	}	
	initTrain(filename);
	
	/*
	 * Since the number of trains to simulate is specified on the command
	 * line, we need to malloc space to store the thread ids of each train
	 * thread.
	 */
	tids = (pthread_t *) malloc(sizeof(pthread_t)*trainCount);
	
	/*
	 * Create all the train threads pass them the information about
	 * length and direction as a TrainInfo structure
	 */
    int count = 0;
	for (i=0;i<trainCount;i++)
	{
		TrainInfo *info = createTrain();
        if(info == NULL)
            break;
		count++;
		printf ("Train %2d headed %s length is %d\n", info->trainId,
			(info->direction == DIRECTION_WEST ? "West" : "East"),
			info->length );
        

        Node *new = (Node*)malloc(sizeof(Node));
        new->train = info;
        pthread_cond_t convar1 = PTHREAD_COND_INITIALIZER;
        pthread_cond_t convar2 = PTHREAD_COND_INITIALIZER;
        //pthread_cond_t convar3 = PTHREAD_COND_INITIALIZER;
        new->convar_across = convar1;
        new->convar_arrival = convar2;
        //new->convar_leave = convar3;
        
        if(info->direction == DIRECTION_WEST){
            west = add_node(west, new);
        }
        
        else if(info->direction == DIRECTION_EAST){
            east = add_node(east, new);
        }
        
		if ( pthread_create (&tids[i],0, Train, (void *)info) != 0 )
		{
			printf ("Failed creation of Train.\n");
			exit(0);
		}
	}
//    printf("\n");
//    print_node(east);
//    printf("\n");
//    print_node(west);
//    printf("\n");
    trainCount = count;
    pthread_mutex_lock(&lock);
    Node *next = (Node *)malloc(sizeof(Node));
    while(passed < trainCount){
        
        //find the next train
        if(east == NULL) //if east waiting list is empty
        {
            next = west;
        }
        else if(west == NULL)// if west waiting list is empty
        {
            next = east;
        }
        // conflict is detected at first time
        else if(east->train->arrival && west->train->arrival && eastPassed == 0)//if both east and west trains are waiting
        {
            printf("conflicted\n");
            next = east;
            if(trainPassing == DIRECTION_EAST)
                eastPassed = 1;
            else
                eastPassed = 0;
        }
        else if(east->train->arrival && west->train->arrival && eastPassed == 1){
            next = west;
            eastPassed = 0;
        }
        else// if only one direction train is waiting
        {
            if(east->train->length < west->train->length){
                next = east;
            }
            else{
                next = west;
            }
        }

        if(next!=NULL) {
            int releaseLock=0;
            if (next->waitingForMutex == 1) {
                pthread_mutex_unlock(&lock);
                releaseLock=1;
            }
            while(next->waitingForMutex == 1){}
            if(releaseLock==1){pthread_mutex_lock(&lock);}
        }
        
        //signal the next waiting train to across
        if(next->train->arrival == 1){
            
                //signal the train to across
            trainPassing = next->train->direction;
                //printf("passing directio: %d\n", trainPassing);
            pthread_cond_signal(&next->convar_across);
                
            
            //wait until the train on bridge to leave
            pthread_cond_wait(&convar_leave, &lock);
            passed++;
        }
        else{
            //wait the next train to arrive
            pthread_cond_wait(&next->convar_arrival, &lock);
        }
        
    }
    pthread_mutex_unlock(&lock);
	/*
	 * This code waits for all train threads to terminate
	 */
	for (i=0;i<trainCount;i++)
	{
		pthread_join (tids[i], NULL);
	}
	
	free(tids);
	return 0;
}
