/*
 * main.c
 *
 * A simple program to illustrate the use of the GNU Readline library
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

#define max_dir 256
#define max_size 5

int num_bgtasks = 0;

typedef struct bgtask bgtask;
struct bgtask{
	int pid;
	char status;
	char cmd[max_dir];
};
struct bgtask tasks[5];

// add a new task into tasks, pid == -1 means the position is open
void add_task(int pid, char cmd[]){
	for(int i = 0; i < max_size; i++){
			if(tasks[i].pid == -1){
				tasks[i].pid = pid;
				tasks[i].status = 'R'; //initialize status as R
				strncpy(tasks[i].cmd, cmd, (strlen(cmd)+1));
				num_bgtasks++;
				break;
			}
		}
}
// delete a task refered with bgid, which is tasks array index
void delete_task(int bgid){
	printf("%d: %s has terminated, pid: %d\n", 
		bgid, tasks[bgid].cmd, tasks[bgid].pid);
	tasks[bgid].pid = -1;
    num_bgtasks--;
}
// print all tasks
void print_tasks(){
	for(int i = 0; i < max_size; i++){
		if(tasks[i].pid != -1){
			printf("%d[%c]: %s\n", i, tasks[i].status, tasks[i].cmd);
		}
	}
	printf("Total background jobs: %d\n", num_bgtasks);
}
int main (int argc, char * argv[])
{
    //initialize bglist array of size 5
    for(int i = 0; i < max_size; i++){
    	tasks[i].pid = -1;
    }
	for (;;)
	{
        // check if any child processes terminated
        if(num_bgtasks > 0){
				int terminated = waitpid(0, NULL, WNOHANG);
                //check until no zombie child process is found
				while(terminated > 0){
					for(int i = 0; i < max_size; i++){
						if(tasks[i].pid == terminated){
							delete_task(i);
						}
					}
					terminated = waitpid(0, NULL, WNOHANG);
				}
        }
        // get current directory and prompt
        char buf[max_dir];
        getcwd(&buf[0], (size_t)max_dir);
        strncat(buf, ">", 2);
		char 	*cmd = readline (buf);
        
		//extract command line into argv
		argv[0] = strtok(cmd, " ");
		int i = 0;
		while(argv[i] != NULL){
			argv[i+1] = strtok(NULL, " ");
			i++;
		}
        
        //check if child process is needed to be created
        if(strcmp(argv[0], "bg") == 0 || 
             (strcmp(argv[0], "cd") != 0 &&
              strcmp(argv[0], "pwd") != 0 &&
              strcmp(argv[0], "bgkill") != 0 &&
              strcmp(argv[0], "bglist") != 0 &&
              strcmp(argv[0], "stop") != 0 &&
              strcmp(argv[0], "start") != 0)){
            
            
            int p = fork();
            // check if the process failed
            if(p < 0){
                printf("Failed to create process!\n");
                return 0;
            }
            // child process
            if(p == 0){
                // check if it is a background job
                if(strcmp(argv[0], "bg") == 0){
                    argv++;
                    if(num_bgtasks < 5){
                        execvp(argv[0], argv);
                    }
                    else{
                        return 0;
                    }
                }
                else{
                    execvp(argv[0], argv);
                } 
            }
            // parent process
            else{
                if(strcmp(argv[0], "bg") == 0){
                    // check if the job list is full
                    if(num_bgtasks < 5){
					   argv++;

                        // copy the command line into lcmd
					   char lcmd[max_dir];
					   strncpy(lcmd, argv[0], (strlen(argv[0])+1));
					   int i = 1;
					   while(argv[i] != NULL){
						  strncat(lcmd, " ", 2);
						  strncat(lcmd, argv[i], (strlen(argv[i])+1));
						  i++;
					   }
                        add_task(p, lcmd); // add the process into tasks
                    }
                    else{
                        printf("Jobs full, can not create new process!\n");
                    }
                }
                else{
                    // wait until child process teminated if it is not a background job
                    waitpid(p, NULL, 0);
                }
            }
        }
        // if no child process needed to be created
        else{
            if(strcmp(argv[0], "bgkill") == 0){
				int bgid = atoi(argv[1]);
                // check if the input number in the range
				if(bgid >= 0 && bgid < 5){
                    // check if there is any process in the list
                    if(tasks[bgid].pid > 0){
                        kill(tasks[bgid].pid, SIGKILL); // kill the process
				        delete_task(bgid); // remove the process from the list
                    }
				    else{
                        printf("No such process!\n");
                    }
				}
				else{
				    printf("Out of range!\n");
				}
            }
            else if(strcmp(argv[0], "bglist") == 0){
					print_tasks(); // print the list
            }
            else if(strcmp(argv[0], "stop") == 0){
                int bgid = atoi(argv[1]);
                // check if the process is stopped alread
                if(tasks[bgid].status == 'S'){
                    printf("The job is stopped already!\n");
                }
                else{
                    kill(tasks[bgid].pid, SIGSTOP);// stop the process
                    tasks[bgid].status = 'S';// change the status
                }
            }
            else if(strcmp(argv[0], "start") == 0){
                int bgid = atoi(argv[1]);
                if(tasks[bgid].status == 'R'){
                    printf("The job is running already!\n");
                }
                else{
                    kill(tasks[bgid].pid, SIGCONT);
                    tasks[bgid].status = 'R';
                }
            }
            else if(strcmp(argv[0], "pwd") == 0){
				char buf[max_dir];
				getcwd(&buf[0], (size_t)max_dir);// get current directory into buf	
				printf("%s\n", buf);
			 }
            else if(strcmp(argv[0], "cd") == 0){
                // check if no input after cd
				if(argv[1] == NULL){
				    chdir(getenv("HOME"));
                }
                else{
				    chdir(argv[1]);
                }
			}
        }
		free (cmd);
        sleep(1);
	}	
	return 0;
}