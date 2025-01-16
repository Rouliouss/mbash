// gcc mbash.c -o mbash

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/wait.h>
#define MAXLI 2048 // taille max d'une ligne (en caractères), taille par défaut de la plupart des tableaux
char *environ[] = {NULL}; // tableau d'une case valant toujours NULL, nécessaire pour execve
char *args[] = {NULL}; // tableau d'arguments transmis à execve
char cmd[MAXLI]; // commande en entree
char cwd[MAXLI]; // chemin du current working directory cwd
static char exec_path[MAXLI]; // chemin de l'executable 
char *history[MAXLI]; // tableau contenant l'historique de commandes
int nb_history; // nombre de commandes dans history
void print_history(); 
void mbash();
const char *getExecutable(const char *cmd);
void welcome() {
      system("clear"); // seul appel a system() de l'application
      printf( 
            "#############################################################\n"
            "#                                                           #\n"
            "#                   █████                       █████    ©  #\n"
            "#                  ░░███                       ░░███        #\n"
            "#   █████████████   ░███████   ██████    █████  ░███████    #\n"
            "#  ░░███░░███░░███  ░███░░███ ░░░░░███  ███░░   ░███░░███   #\n"
            "#   ░███ ░███ ░███  ░███ ░███  ███████ ░░█████  ░███ ░███   #\n"
            "#   ░███ ░███ ░███  ░███ ░███ ███░░███  ░░░░███ ░███ ░███   #\n"
            "#   █████░███ █████ ████████ ░░████████ ██████  ████ █████  #\n"
            "#  ░░░░░ ░░░ ░░░░░ ░░░░░░░░   ░░░░░░░░ ░░░░░░  ░░░░ ░░░░░   #\n"
						"#                                                           #\n"
            "#                 ANDRÉ Jules - DENIS Oscar                 #\n"
						"#                                                           #\n"
            "#############################################################\n"); // Message de bienvenue
}

int main(int argc, char** argv) {
  welcome();
  while (1) { // boucle de l'appli
    getcwd(cwd, sizeof(cwd)); // getcwd() stocke le pwd dans la variable en parametre
		printf("\033[1;32mmbash\033[1;37m:\033[1;34m%s\033[0m$ ", cwd); // %string
    fgets(cmd, MAXLI, stdin); // met stdin dans la variable cmd
    strtok(cmd, "\n"); // enlève le \n saut de ligne final de l'input
    history[nb_history] = strdup(cmd); // duplique le string de la commande pour l'ajouter dans le tableau d'historique
    nb_history++; // incrémente le nombre de commandes
    mbash(cmd); // appel de la commande avec mbash
  }
  return 0;
}

void mbash() {
    if (strlen(cmd) == 0) { // si la commande est vide
        return;
    }

    int i = 0;

    args[i] = strtok(cmd, " "); // enlève l'espace à la fin de l'argument
    while (args[i] != NULL) { // enlève l'espace final de chaque argument
        i++;
        args[i] = strtok(NULL, " ");
    }
    args[i] = NULL;

  // cd
  if (strcmp(args[0], "cd") == 0) { // strcmp pour comparer deux strings
    if (args[1] == NULL) { // quand on utilise cd sans argument on retourne au home directory
      chdir(getenv("HOME")); // chdir pour changer de repertoire courant, getenv pour récupérer une variable
    } else {
      chdir(args[1]);
    }
    return;
  }

  // history
  if (strcmp(args[0], "history") == 0) { // strcmp pour comparer deux strings
        print_history(); // Affiche l'historique de commandes
        return;
    }

    const char *Executable = getExecutable(args[0]); // getExec sur le premier argument (la commande donc)
    if (Executable == NULL) { // si la commande est vide
        printf("Commande non reconnue : %s\n", args[0]);
        return;
    }


    pid_t pid = fork(); // on fork pour executer ailleurs

    if (pid == 0) { // si c'est le proc fils, executer la commande
        execve(Executable, args, environ);
        exit(1); // tue le proc quand il a fini
    } else if (pid > 0) { // si c'est le proc parent, attendre la fin du fils
        int status;
        waitpid(pid, &status, 0); // attend que le fils finisse
    }
}


const char *getExecutable(const char *cmd) { // méthode pour trouver l'executable correspondant parmis les chemins du $PATH 

		
		char *path_env = getenv("PATH"); // récupère le PATH
		char *path_copy = strdup(path_env); // duplique la chaine
		char *dir = strtok(path_copy, ":"); // récupère le dernier repertoire du PATH
		
		while (dir != NULL) {
			snprintf(exec_path, sizeof(exec_path), "%s/%s", dir, cmd); // concatene les deux string dir et cmd dans exec_path

			if (access(exec_path, X_OK) == 0) { // vérifie si le programme à l'autorisation X (exécuter) sur répertoire/commande si il existe
				return exec_path; // le chemin est valide alors on le return
			}
			
			dir = strtok(NULL, ":"); // passe au prochain repertoire
		}

		return NULL; // l'executable n'est dans aucun repertoire du PATH
}

void print_history() { 
  for (int i = 0; i < nb_history; i++) {
    printf(" %5d  %s\n", i + 1, history[i]); // Affiche l'historique de commande (5d pour 5 espaces) 
  }
}