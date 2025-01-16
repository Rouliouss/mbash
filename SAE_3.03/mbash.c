#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h> 
#define MAXLI 2048 // taille max d'une ligne (en caractères), taille par défaut de la plupart des tableaux
char *environ[] = {"TERM=xterm", "DISPLAY=:1", NULL}; // tableau d'une case valant toujours NULL, nécessaire pour execve
char *args[] = {NULL}; // tableau d'arguments transmis à execve
char cmd[MAXLI]; // commande en entree
char cwd[MAXLI]; // chemin du current working directory cwd
static char exec_path[MAXLI]; // chemin de l'executable 
char *history[MAXLI]; // tableau contenant l'historique de commandes
int nb_history; // nombre de commandes dans history
int history_index = -1;

// Structure pour sauvegarder les paramètres du terminal
struct termios orig_termios;

void print_history();
void mbash();
void StringVarEnv();
const char *getExecutable(const char *cmd);
void welcome();
void handle(char *input);
void disable_raw_mode();
void enable_raw_mode();

// Désactive le mode raw du terminal
void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

// Active le mode raw du terminal
void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);
    
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

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


void handle(char *input) {
    int index = 0;  // Index utilisé pour parcourir et modifier la chaîne 'input'
    char c;
    input[0] = '\0';  // Initialise la chaîne input comme étant vide

    while (1) {  // Boucle infinie pour attendre les entrées utilisateur
        c = getchar();  // Lit un caractère à la fois

        if (c == 27) {  // Vérifie si c'est un caractère d'échappement (début de la séquence des flèches)
            char seq[3];  // Tableau pour stocker la séquence d'échappement
            seq[0] = getchar();  // Récupère le premier caractère de la séquence (généralement '[')
            seq[1] = getchar();  // Récupère le second caractère de la séquence (soit 'A' pour flèche haut, soit 'B' pour flèche bas)

            if (seq[0] == '[') {  // Si la séquence commence par '['
                if (seq[1] == 'A') {  // Si c'est une flèche haut
                    if (nb_history > 0) {  // Si l'historique n'est pas vide
                        if (history_index == -1) {  // Si aucun historique n'est sélectionné
                            history_index = nb_history - 1;  // Sélectionne la dernière commande de l'historique
                        } else if (history_index > 0) {  // Si l'index de l'historique est valide
                            history_index--;  // Passe à la commande précédente dans l'historique
                        }

                        // Efface la ligne actuelle et réaffiche le prompt
                        printf("\r\033[K");
                        printf("\033[1;32mmbash\033[1;37m:\033[1;34m%s\033[0m$ ", cwd);

                        // Copie la commande de l'historique dans l'input et l'affiche à l'écran
                        strcpy(input, history[history_index]);
                        printf("%s", input);
                        index = strlen(input);  // Met à jour l'index en fonction de la longueur de la commande chargée
                    }
                } else if (seq[1] == 'B') {  // Si c'est une flèche bas
                    if (history_index < nb_history - 1) {  // Si on n'est pas déjà à la fin de l'historique
                        history_index++;  // Passe à la commande suivante dans l'historique

                        // Efface la ligne actuelle et réaffiche le prompt
                        printf("\r\033[K");
                        printf("\033[1;32mmbash\033[1;37m:\033[1;34m%s\033[0m$ ", cwd);

                        // Copie la commande de l'historique dans l'input et l'affiche à l'écran
                        strcpy(input, history[history_index]);
                        printf("%s", input);
                        index = strlen(input);  // Met à jour l'index
                    } else {  // Si on est à la fin de l'historique
                        history_index = nb_history;  // Réinitialise l'index d'historique pour vider l'input

                        // Efface la ligne actuelle et réaffiche le prompt
                        printf("\r\033[K");
                        printf("\033[1;32mmbash\033[1;37m:\033[1;34m%s\033[0m$ ", cwd);

                        input[0] = '\0';  // Vide l'input
                        index = 0;  // Réinitialise l'index
                    }
                }
            }
        } else if (c == 127) {  // Si c'est la touche Backspace (code 127)
            if (index > 0) {  // Si l'index est supérieur à 0 (il y a des caractères à effacer)
                index--;  // Décrémenter l'index
                input[index] = '\0';  // Supprime le dernier caractère de l'input
                printf("\b \b");  // Efface visuellement le caractère à l'écran (retour arrière et espace)
            }
        } else if (c == '\n') {  // Si c'est la touche "Entrée" (fin de la commande)
            input[index] = '\0';  // Ajoute le caractère de fin de chaîne
            printf("\n");  // Affiche un saut de ligne
            return;  // Retourne de la fonction pour valider l'input
        } else if (c == 4) {  // Si c'est la touche Ctrl + D
            printf("\n");
            exit(0);  // Quitte le shell
        } else if (c == 12) {  // Si c'est la touche Ctrl + L
            printf("\033[H\033[J");  // Efface l'écran
            printf("\033[1;32mmbash\033[1;37m:\033[1;34m%s\033[0m$ ", cwd);  // Réaffiche le prompt
            index = 0;  // Réinitialise l'index de l'input
            input[0] = '\0';  // Réinitialise la chaîne d'input
        } else if (c == 3) {  // Si c'est la touche Ctrl + C
            printf("\n");
            printf("\033[1;32mmbash\033[1;37m:\033[1;34m%s\033[0m$ ", cwd);  // Affiche le prompt après avoir intercepé Ctrl + C
            index = 0;  // Réinitialise l'index de l'input
            input[0] = '\0';  // Vide l'input
        } else if (c >= 32 && c <= 126) {  // Si c'est un caractère imprimable (de l'espace à ~)
            if (index < MAXLI - 1) {  // Si l'index n'a pas dépassé la capacité maximale de l'input
                input[index++] = c;  // Ajoute le caractère à l'input et incrémente l'index
                input[index] = '\0';  // Met à jour la chaîne de caractères avec un terminateur null
                printf("%c", c);  // Affiche le caractère à l'écran
            }
        }
    }
}

int main(int argc, char** argv) {
    welcome();
    enable_raw_mode(); // Active le mode raw

    while (1) { // boucle de l'appli
        getcwd(cwd, sizeof(cwd)); // getcwd() stocke le pwd dans la variable en parametre
        printf("\033[1;32mmbash\033[1;37m:\033[1;34m%s\033[0m$ ", cwd); // %string
        handle(cmd); // gestion des fleches haut et bas
        
        if (strlen(cmd) > 0) {
            history[nb_history] = strdup(cmd); // duplique le string de la commande pour l'ajouter dans le tableau d'historique
            nb_history++; // incrémente le nombre de commandes
        }
        mbash(); // appel de la commande avec mbash
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

    StringVarEnv();

    // cd
    if (strcmp(args[0], "cd") == 0) { // strcmp pour comparer deux strings
        if (args[1] == NULL) { // quand on utilise cd sans argument on retourne au home directory
            chdir(getenv("HOME")); // chdir pour changer de repertoire courant, getenv pour récupérer une variable
        } else {
            chdir(args[1]); // chdir pour changer de repertoire courant
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
        fprintf(stderr, "%s : commande introuvable\n", args[0]);
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

void StringVarEnv() {
    for (int i = 0; args[i] != NULL; i++) { // boucle pour chaque argument
        // Vérifie si l'argument commence par '$'
        if (args[i][0] == '$') {
            if (strcmp(args[i], "$$") == 0) { // Si l'argument est exactement "$$"
                // Remplace "$$" par le PID du processus en cours
                pid_t pid = getpid();
                char pid_str[MAXLI];
                snprintf(pid_str, sizeof(pid_str), "%d", pid); // Convertit le PID en chaîne
                args[i] = pid_str; // Remplace l'argument par le PID
            } else {
                // Sinon, remplace les variables d'environnement classiques
                char *VarEnv = args[i] + 1; // On enlève le '$' pour récupérer la variable
                char *Val = getenv(VarEnv); // On récupère la valeur de la variable d'environnement

                if (Val != NULL) {
                    args[i] = Val; // Remplace l'argument par la valeur de la variable d'environnement
                }
            }
        }
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