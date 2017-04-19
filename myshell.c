#include <sys/wait.h>
#include<sys/types.h>
#include<sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include<pwd.h>
#include<grp.h>
#include<string.h>
#include<time.h>
#include<libgen.h>

int lsh_cd(char **args);
int lsh_exit(char **args);
int lsh_pwd(char **args);
int ls(char **args);

char *builtin_str[] = {
  "cd",
  "exit",
  "pwd",
  "ls"
};

int (*builtin_func[]) (char **) = {
  &lsh_cd,
  &lsh_exit,
  &lsh_pwd,
  &ls
};

int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

int lsh_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("lsh");
    }
  }
  return 1;
}

int lsh_pwd(char **args)
{
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("%s\n", cwd);
    return 1;
}

int lsh_exit(char **args)
{
  return 0;
}

int ls(char** args)
{
//	if (args < 2)
//	{
//		perror("Shell filename");
//	}

	struct stat buf;
	if (lstat(args[1], &buf) < 0)
	{
		perror("can't open file");
		return 1;
	}

	unsigned char type;
	if (S_ISDIR(buf.st_mode)) type = 'd';
	else if (S_ISREG(buf.st_mode)) type = '-';
	else if (S_ISLNK(buf.st_mode)) type = 'l';
	else type = '?';

	unsigned short rights = buf.st_mode & (S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH);
	
	unsigned int link = buf.st_nlink;
	
	char username[50];
	
	struct passwd *user = getpwuid(buf.st_uid);
	strncpy(username, user -> pw_name, 49);
	username[49] = '\0';
	
	char groupname[50];
	struct group *grp = getgrgid(buf.st_gid);
	strncpy(groupname, grp -> gr_name, 49);
	groupname[49] = '\0';
	
	unsigned int size = buf.st_size;
	
	char time[50];
	strncpy(time, ctime(&buf.st_atime), 50);
	time[49] = '\0';
	int len = strlen(time);
	if (time[len-1] == '\n') time[len-1] = '\0';
	
	printf("%c %3o %u %s %s %u %s %s\n", type, rights, link, username, groupname, size, time, basename(args[1]));
	
	if(access(args[1], R_OK) == 0) printf("Read OK\t");
	else printf("No Read\t");
	if(access(args[1], W_OK) == 0) printf("Write OK\t");
	else printf("No Write\t");
	if(access(args[1], X_OK) == 0) printf("Execute OK\t");
	else printf("No Execute\t");
	printf("\n");    
}

int lsh_launch(char **args)
{
  pid_t pid, wpid;
  int status;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("lsh");
  } else {
    // Parent process
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}


int lsh_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < lsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return lsh_launch(args);
}

#define LSH_RL_BUFSIZE 1024
char *lsh_read_line(void)
{
  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // Read a character
    c = getchar();

    // If we hit EOF, replace it with a null character and return.
    if (c == EOF || c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += LSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

void lsh_loop(void)
{
  char *line;
  char **args;
  int status;

  do {
    printf("> ");
    line = lsh_read_line();
    args = lsh_split_line(line);
    status = lsh_execute(args);

    free(line);
    free(args);
  } while (status);
}

int main(int argc, char **argv)
{

  lsh_loop();


  return EXIT_SUCCESS;
}

