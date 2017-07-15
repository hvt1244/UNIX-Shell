#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "tokenizer.h"

/* Convenience macro to silence compiler warnings about unused function parameters. */
#define unused __attribute__((unused))

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;
/* Process group id for the shell */
pid_t shell_pgid;

int cmd_exit(struct tokens *tokens);
int cmd_help(struct tokens *tokens);
int cmd_pwd(struct tokens *tokens);
int cmd_cd(struct tokens *tokens);
int IO_redirect(struct tokens *tokens);
int IO_token_idx(struct tokens *tokens, char* c);

/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens *tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t; 

fun_desc_t cmd_table[] = {
  {cmd_help, "?", "show this help menu"},
  {cmd_exit, "exit", "exit the command shell"},
  {cmd_pwd, "pwd", "present working directory"},
  {cmd_cd, "cd", "change directory"}

};

/* Prints a helpful description for the given command */
int cmd_help(unused struct tokens *tokens) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Exits this shell */
int cmd_exit(unused struct tokens *tokens) {
  exit(0);
}

int cmd_pwd(unused struct tokens *tokens){
  char pwd [1024];
  getcwd(pwd,sizeof(pwd));
  printf("%s\n",pwd);
  return 1;
}

int cmd_cd( struct tokens *tokens1){

 
  char* a;
  if(chdir(&(tokens1->tokens)[4])==-1)
    printf("cannot find the directory\n");


  return 1;
}
/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  return -1;
}

int IO_token_idx(struct tokens *tokens, char* c)
{
	size_t no_tokens; 
	no_tokens = (tokens_get_length(tokens));
	for (int i=0; i< no_tokens-1 && (tokens->tokens)[i]; i++){
		if (strncmp((tokens->tokens)[i], c, 1) == 0)
			return i;
	}
	return 0;
}

int IO_redirect(struct tokens *tokens2)
{
	
  int i,j,fd,n;
  int in_index,out_index;
  n = (tokens_get_length(tokens2));

  in_index = IO_token_idx(tokens2, "<");
  out_index = IO_token_idx(tokens2, ">");
  
  //printf ("Index I & O: %d\t %d\t",in_index, out_index);


	//fd = fopen("sample.txt",O_RDONLY);
	//if(fd < 0)
	//{
		//printf("----------Can not open file------------");
		//return 0;
	//}
  ////------------Input Redirection---------------------/////////
	if(in_index!=0){
		if ((tokens2->tokens)[in_index+1] == NULL || strcmp((tokens2->tokens)[in_index+1],">")==0 || strcmp((tokens2->tokens)[in_index+1],"<")==0){
			printf ("Syntax error \n");
			return -1;
		}
		printf("\n->%s\n",(tokens2->tokens)[in_index+1]);
		if ((fd = open((tokens2->tokens)[in_index+1],O_RDONLY))	<0){

			printf ("No such file or directory %s\n,%d", (tokens2->tokens)[in_index+1],__LINE__);
			return -1;
		  }
		  							          printf("before execv2 \n");


		  dup2(fd, 0);// STDIN_FILENO);
		  close(fd);
		  for(j=in_index;j<n-2 && (tokens2->tokens)[j+2];++j){
			(tokens2->tokens)[j]=(tokens2->tokens)[j+2];
			free ((tokens2->tokens)[j]);
			}
			(tokens2->tokens)[j] = NULL;

		}

	 ////------------Output Redirection---------------------/////////

	if(out_index!=0)
	{

			if ((tokens2->tokens)[out_index+1] == NULL || strcmp((tokens2->tokens)[in_index+1],">")==0 || strcmp((tokens2->tokens)[in_index+1],"<")==0)
			{
				printf ("Syntax error \n");
				return -1;
			}
			printf("\n->%s\n",(tokens2->tokens)[out_index+1]);
			fd = open((tokens2->tokens)[out_index+1],O_WRONLY);//O_WRONLY| O_TRUNC | S_IRGRP | S_IROTH | S_IWUSR | O_CREAT);
			if(fd < 0)
			{
				printf ("No such file or directory1 %s\n , %d", (tokens2->tokens)[out_index+1],__LINE__);
				printf (" Inside %s\n",(tokens2->tokens)[out_index+1]);
				          printf("before execv2 \n");

				return -1;
			}
			//printf (" Inside %s\n",(tokens2->tokens)[out_index+1]);

			dup2(fd, 1);//STDOUT_FILENO);
			close(fd);

			for(j=out_index;j<n-2 && (tokens2->tokens)[j+2];++j)
			{
				(tokens2->tokens)[j]=(tokens2->tokens)[j+2];
				free ((tokens2->tokens)[j]);
			}
			(tokens2->tokens)[j] = NULL;

		}


return 0;
}



/* Intialization procedures for this shell */
void init_shell() {
  /* Our shell is connected to standard input. */
  shell_terminal = STDIN_FILENO;

  /* Check if we are running interactively */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive) {
    /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
     * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
     * foreground, we'll receive a SIGCONT. */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Saves the shell's process id */
    shell_pgid = getpid();

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Save the current termios to a variable, so it can be restored later. */
    tcgetattr(shell_terminal, &shell_tmodes);
  }
}

int main(unused int argc,  char *argv[]) {
  init_shell();
  char* path;
  //char *path_token;
  struct tokens *tokenised,*path_token;
  static char line[4096];
  char concat_s[1024];
  int i,line_num = 0;
    struct stat file_stat;


  /* Please only print shell prompts when standard input is not a tty */
  if (shell_is_interactive)
    fprintf(stdout, "%d: ", line_num);
     i=0;

  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    struct tokens *tokens = tokenize(line);

    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tokens, 0));

    if (fundex >= 0) {
      cmd_table[fundex].fun(tokens);
    } 

    else {
				
      /* REPLACE this to run commands as programs. */
      pid_t pid = fork();
      if (pid == 0){

		if(IO_redirect(tokens)<0){
					 printf("exiting");
			  exit(0);
		  }
		  		  	//		          printf("before execv1 \n");

		      path = getenv ("PATH"); 
 

		path_token = tokenize(path);      
		i=0;
        execv((tokens->tokens)[0],&(tokens->tokens)[0]);


        while((path_token->tokens)[i]){
                     // printf ("4\n");

          strcpy(concat_s,(path_token->tokens)[i]);
          strcat(concat_s,"/");
          strcat(concat_s,(tokens->tokens)[0]);
                   //   printf ("%d\n",i);

          if(stat(concat_s,&file_stat) == 0){


            execv(concat_s,&(tokens->tokens)[0]);
            perror("Error");
            exit(1);
            
          }
          i++;
        }
  // printf("after execv3 \n");

        execv((tokens->tokens)[0],&(tokens->tokens)[0]);

        if (execv((tokens->tokens)[0],&(tokens->tokens)[0]) < 0) {

           fprintf(stderr, "%s : Command not found\n", tokens[0]);
          exit(0);
          }
      }

    }

    

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}
