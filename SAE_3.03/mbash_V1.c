#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#define MAXLI 2048
char *environ[] = {NULL};
char cmd[MAXLI];
char path[MAXLI];
int pathidx;
void mbash();
const char *getExecutable(const char *cmd);

int main(int argc, char** argv) {
  while (1) {
    
    printf("mbash $ \n" "commande: ");
    fgets(cmd, MAXLI, stdin);
    cmd[strcspn(cmd, "\n")] = '\0';
    mbash(cmd);
  }
  return 0;
}

void mbash() {
    if (strlen(cmd) == 0) {
        return;
    }
    char *args[MAXLI / 2 + 1];
    int i = 0;

    args[i] = strtok(cmd, " ");
    while (args[i] != NULL) {
        i++;
        args[i] = strtok(NULL, " ");
    }
    args[i] = NULL;

    const char *Executable = getExecutable(args[0]);
    if (Executable == NULL) {
        fprintf(stderr, "Commande non reconnue : %s\n", args[0]);
        return;
    }

    pid_t pid = fork();

    if (pid == 0) {
        if (execve(Executable, args, environ) == -1) {
            perror("Erreur execve");
            exit(EXIT_FAILURE);
        }
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
    } else {
        perror("Erreur fork");
    }
}

    const char *getExecutable(const char *cmd) {
    if (strcmp(cmd, "ls") == 0) {
        return "/bin/ls";
    } else if (strcmp(cmd, "echo") == 0) {
        return "/bin/echo";
    } else if (strcmp(cmd, "pwd") == 0) {
        return "/bin/pwd";
    } else if (strcmp(cmd, "cat") == 0) {
        return "/bin/cat";
    } else if (strcmp(cmd, "date") == 0) {
        return "/bin/date";
    } else {
        return NULL;
    }
}
