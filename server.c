/**
 * ============================================================================
 * SERVEUR DE JEU DE DEVINETTE DISTRIBUÉ - Version Professionnelle
 * ============================================================================
 *
 * @file        server.c
 * @brief       Serveur TCP multi-threadé pour jeu de devinette compétitif
 * @author      Opak (Penifana Abdoul-Khader Ouattara)
 * @university  ESATIC (Abidjan, Côte d'Ivoire) & Université Côte d'Azur
 * @course      Master Mobiquité, Big Data et Intégration Système
 * @year        2025-2026
 * @version     2.0 FINAL
 *
 * DESCRIPTION:
 * -----------
 * Ce serveur implémente un jeu de devinette distribué avec les fonctionnalités:
 * - Multi-threading POSIX pour gérer plusieurs clients simultanément
 * - Système de scoring compétitif: Score = 10000 - (tentatives × 100) - temps
 * - Leaderboard persistant des 10 meilleurs scores
 * - Validation stricte des entrées (nom: 3-10 lettres, nombre: 0-100)
 * - Statistiques serveur en temps réel
 * - Gestion propre des signaux (SIGINT, SIGTERM)
 *
 * ARCHITECTURE:
 * ------------
 * Client Python/Web --> Proxy WebSocket --> SERVEUR TCP (ce fichier)
 *
 * COMPILATION:
 * -----------
 * gcc -o server server.c -pthread -Wall -Wextra -O2
 *
 * EXÉCUTION:
 * ---------
 * ./server
 *
 * Le serveur écoute sur le port 8080 par défaut (modifiable via PORT)
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>

/* ============================================================================
 * CONSTANTES DE CONFIGURATION
 * ============================================================================ */
#define PORT                8080        // Port d'écoute du serveur
#define MAX_CLIENTS         30          // Nombre maximum de clients simultanés
#define BUFFER_SIZE         4096        // Taille du buffer de communication
#define MIN_NUMBER          0           // Borne inférieure de la plage
#define MAX_NUMBER          100         // Borne supérieure de la plage
#define MIN_NAME_LENGTH     3           // Longueur minimale du nom (3 lettres)
#define MAX_NAME_LENGTH     11          // Longueur maximale du nom (10 lettres + \0)
#define TOP_SCORES          10          // Nombre de scores dans le leaderboard
#define INITIAL_SCORE       10000       // Score de départ pour le calcul
#define ATTEMPT_PENALTY     100         // Pénalité par tentative

/* ============================================================================
 * STRUCTURES DE DONNÉES
 * ============================================================================ */

/**
 * @struct score_t
 * @brief Structure représentant un score enregistré
 */
typedef struct {
    char name[MAX_NAME_LENGTH];         // Nom du joueur
    int attempts;                        // Nombre de tentatives
    int duration;                        // Durée en secondes
    int score;                           // Score calculé
    time_t timestamp;                    // Timestamp de la partie
} score_t;

/**
 * @struct leaderboard_t
 * @brief Structure du tableau des scores avec mutex pour thread-safety
 */
typedef struct {
    score_t scores[TOP_SCORES];         // Tableau des meilleurs scores
    int count;                           // Nombre de scores enregistrés
    pthread_mutex_t mutex;               // Mutex pour accès concurrent
} leaderboard_t;

/**
 * @struct client_data_t
 * @brief Structure contenant toutes les données d'un client
 */
typedef struct {
    int socket;                          // Socket du client
    int client_id;                       // ID unique du client
    struct sockaddr_in address;          // Adresse IP du client
    int target_number;                   // Nombre à deviner
    int attempts;                        // Compteur de tentatives
    time_t start_time;                   // Heure de début de partie
    char name[MAX_NAME_LENGTH];          // Nom du joueur
} client_data_t;

/**
 * @struct stats_t
 * @brief Statistiques globales du serveur
 */
typedef struct {
    int total_games;                     // Nombre total de parties
    int total_attempts;                  // Nombre total de tentatives
    int best_attempts;                   // Meilleur nombre de tentatives
    float avg_attempts;                  // Moyenne de tentatives
    time_t server_start_time;            // Timestamp de démarrage
    pthread_mutex_t mutex;               // Mutex pour accès concurrent
} stats_t;

/* ============================================================================
 * VARIABLES GLOBALES
 * ============================================================================ */
static int server_socket = -1;                              // Socket serveur
static int active_clients = 0;                              // Clients connectés
static int total_clients_served = 0;                        // Total clients
static pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
static stats_t global_stats = {0, 0, 999999, 0.0, 0, PTHREAD_MUTEX_INITIALIZER};
static leaderboard_t leaderboard = {.count = 0, .mutex = PTHREAD_MUTEX_INITIALIZER};

/* ============================================================================
 * PROTOTYPES DES FONCTIONS
 * ============================================================================ */
void handle_signal(int sig);
void log_message(const char *level, const char *message);
int send_message(int socket, const char *message);
int receive_message(int socket, char *buffer, int size);
int validate_name(const char *name);
void update_stats(int attempts);
int calculate_score(int attempts, int duration);
void add_to_leaderboard(const char *name, int attempts, int duration, int score);
void display_server_stats(int socket);
void display_leaderboard(int socket);
void *handle_client(void *arg);

/* ============================================================================
 * IMPLÉMENTATION DES FONCTIONS
 * ============================================================================ */

/**
 * @brief Gestionnaire de signaux pour arrêt propre du serveur
 * @param sig Numéro du signal reçu
 */
void handle_signal(int sig) {
    char msg[100];
    snprintf(msg, sizeof(msg), "Signal %d reçu, arrêt du serveur", sig);
    log_message("SHUTDOWN", msg);

    if (server_socket >= 0) {
        close(server_socket);
    }

    pthread_mutex_destroy(&clients_mutex);
    pthread_mutex_destroy(&global_stats.mutex);
    pthread_mutex_destroy(&leaderboard.mutex);

    printf("\n✅ Serveur arrêté proprement\n\n");
    exit(0);
}

/**
 * @brief Affiche un message de log avec timestamp et niveau
 * @param level Niveau du log (INFO, SUCCESS, WARNING, ERROR)
 * @param message Message à afficher
 */
void log_message(const char *level, const char *message) {
    time_t now = time(NULL);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    // Couleurs ANSI selon le niveau
    const char *color = "\033[0m";
    if (strcmp(level, "SUCCESS") == 0) color = "\033[32m";      // Vert
    else if (strcmp(level, "ERROR") == 0) color = "\033[31m";   // Rouge
    else if (strcmp(level, "WARNING") == 0) color = "\033[33m"; // Jaune
    else if (strcmp(level, "INFO") == 0) color = "\033[36m";    // Cyan

    printf("%s[%s] [%-8s]\033[0m %s\n", color, timestamp, level, message);
}

/**
 * @brief Envoie un message au client via socket
 * @param socket Socket du client
 * @param message Message à envoyer
 * @return 0 si succès, -1 si erreur
 */
int send_message(int socket, const char *message) {
    size_t len = strlen(message);
    ssize_t sent = send(socket, message, len, MSG_NOSIGNAL);
    return (sent == (ssize_t)len) ? 0 : -1;
}

/**
 * @brief Reçoit un message du client avec nettoyage des retours à la ligne
 * @param socket Socket du client
 * @param buffer Buffer de réception
 * @param size Taille du buffer
 * @return Nombre d'octets reçus, -1 si erreur
 */
int receive_message(int socket, char *buffer, int size) {
    memset(buffer, 0, size);
    ssize_t bytes_received = recv(socket, buffer, size - 1, 0);

    if (bytes_received <= 0) {
        return -1;
    }

    // Nettoyer les retours à la ligne
    buffer[strcspn(buffer, "\r\n")] = '\0';

    return bytes_received;
}

/**
 * @brief Valide le nom du joueur selon les règles strictes
 * @param name Nom à valider
 * @return 1 si valide, 0 sinon
 *
 * Règles de validation:
 * - Longueur: 3 à 10 caractères
 * - Uniquement des lettres (a-z, A-Z)
 * - Pas de chiffres, espaces ou caractères spéciaux
 */
int validate_name(const char *name) {
    if (!name || name[0] == '\0') {
        return 0;
    }

    size_t len = strlen(name);

    // Vérifier la longueur
    if (len < MIN_NAME_LENGTH || len >= MAX_NAME_LENGTH) {
        return 0;
    }

    // Vérifier que ce sont uniquement des lettres
    for (size_t i = 0; i < len; i++) {
        if (!isalpha((unsigned char)name[i])) {
            return 0;
        }
    }

    return 1;
}

/**
 * @brief Met à jour les statistiques globales du serveur
 * @param attempts Nombre de tentatives de la partie terminée
 */
void update_stats(int attempts) {
    pthread_mutex_lock(&global_stats.mutex);

    global_stats.total_games++;
    global_stats.total_attempts += attempts;
    global_stats.avg_attempts = (float)global_stats.total_attempts / global_stats.total_games;

    if (attempts < global_stats.best_attempts) {
        global_stats.best_attempts = attempts;
    }

    pthread_mutex_unlock(&global_stats.mutex);
}

/**
 * @brief Calcule le score d'un joueur
 * @param attempts Nombre de tentatives
 * @param duration Durée en secondes
 * @return Score calculé (peut être négatif si trop de tentatives)
 *
 * Formule: Score = 10000 - (tentatives × 100) - temps
 */
int calculate_score(int attempts, int duration) {
    int score = INITIAL_SCORE - (attempts * ATTEMPT_PENALTY) - duration;
    return (score > 0) ? score : 0;
}

/**
 * @brief Ajoute un score au leaderboard avec insertion triée
 * @param name Nom du joueur
 * @param attempts Nombre de tentatives
 * @param duration Durée en secondes
 * @param score Score calculé
 *
 * Le leaderboard est trié par score décroissant
 */
void add_to_leaderboard(const char *name, int attempts, int duration, int score) {
    pthread_mutex_lock(&leaderboard.mutex);

    // Créer le nouveau score
    score_t new_score;
    strncpy(new_score.name, name, MAX_NAME_LENGTH - 1);
    new_score.name[MAX_NAME_LENGTH - 1] = '\0';
    new_score.attempts = attempts;
    new_score.duration = duration;
    new_score.score = score;
    new_score.timestamp = time(NULL);

    // Trouver la position d'insertion (tri par score décroissant)
    int insert_pos = -1;
    for (int i = 0; i < leaderboard.count && i < TOP_SCORES; i++) {
        if (score > leaderboard.scores[i].score) {
            insert_pos = i;
            break;
        }
    }

    // Si pas dans le top mais il reste de la place
    if (insert_pos == -1 && leaderboard.count < TOP_SCORES) {
        insert_pos = leaderboard.count;
    }

    // Insérer le score
    if (insert_pos != -1) {
        // Décaler les scores inférieurs
        int end_pos = (leaderboard.count < TOP_SCORES) ? leaderboard.count : TOP_SCORES - 1;
        for (int i = end_pos; i > insert_pos; i--) {
            leaderboard.scores[i] = leaderboard.scores[i - 1];
        }

        leaderboard.scores[insert_pos] = new_score;

        if (leaderboard.count < TOP_SCORES) {
            leaderboard.count++;
        }
    }

    pthread_mutex_unlock(&leaderboard.mutex);
}

/**
 * @brief Affiche les statistiques du serveur au client
 * @param socket Socket du client
 */
void display_server_stats(int socket) {
    char msg[2048];
    char buffer[256];

    pthread_mutex_lock(&global_stats.mutex);

    time_t now = time(NULL);
    int uptime = (int)difftime(now, global_stats.server_start_time);
    int hours = uptime / 3600;
    int minutes = (uptime % 3600) / 60;
    int seconds = uptime % 60;

    snprintf(msg, sizeof(msg),
        "\n╔═══════════════════════════════════════════════════╗\n"
        "║        📊 STATISTIQUES DU SERVEUR                 ║\n"
        "╠═══════════════════════════════════════════════════╣\n");

    snprintf(buffer, sizeof(buffer),
        "║ ⏱️  Uptime             : %02dh %02dm %02ds              ║\n",
        hours, minutes, seconds);
    strcat(msg, buffer);

    snprintf(buffer, sizeof(buffer),
        "║ 👥 Clients actifs      : %-25d║\n", active_clients);
    strcat(msg, buffer);

    snprintf(buffer, sizeof(buffer),
        "║ 📈 Total servis        : %-25d║\n", total_clients_served);
    strcat(msg, buffer);

    snprintf(buffer, sizeof(buffer),
        "║ 🎮 Parties jouées      : %-25d║\n", global_stats.total_games);
    strcat(msg, buffer);

    snprintf(buffer, sizeof(buffer),
        "║ 🏆 Meilleur (tentatives): %-25d║\n",
        (global_stats.best_attempts == 999999) ? 0 : global_stats.best_attempts);
    strcat(msg, buffer);

    snprintf(buffer, sizeof(buffer),
        "║ 📊 Moyenne tentatives  : %-24.1f║\n", global_stats.avg_attempts);
    strcat(msg, buffer);

    strcat(msg, "╚═══════════════════════════════════════════════════╝\n");

    pthread_mutex_unlock(&global_stats.mutex);

    send_message(socket, msg);
}

/**
 * @brief Affiche le leaderboard au client sous forme de tableau élégant
 * @param socket Socket du client
 */
void display_leaderboard(int socket) {
    char msg[4096];
    char buffer[256];

    pthread_mutex_lock(&leaderboard.mutex);

    snprintf(msg, sizeof(msg),
        "\n╔═══════════════════════════════════════════════════╗\n"
        "║           🏆 TOP %d MEILLEURS SCORES 🏆            ║\n"
        "╠═══════════════════════════════════════════════════╣\n"
        "║  #  │ Joueur     │ Score  │ Essais │ Temps      ║\n"
        "╠═════╪════════════╪════════╪════════╪════════════╣\n",
        TOP_SCORES);

    if (leaderboard.count == 0) {
        strcat(msg,
            "║              Aucun score enregistré               ║\n");
    } else {
        for (int i = 0; i < leaderboard.count; i++) {
            const char *medal = "   ";
            if (i == 0) medal = "🥇";
            else if (i == 1) medal = "🥈";
            else if (i == 2) medal = "🥉";

            snprintf(buffer, sizeof(buffer),
                "║ %s │ %-10s │ %6d │ %6d │ %7ds   ║\n",
                medal,
                leaderboard.scores[i].name,
                leaderboard.scores[i].score,
                leaderboard.scores[i].attempts,
                leaderboard.scores[i].duration);
            strcat(msg, buffer);
        }
    }

    strcat(msg, "╚═══════════════════════════════════════════════════╝\n");

    pthread_mutex_unlock(&leaderboard.mutex);

    send_message(socket, msg);
}

/**
 * @brief Fonction principale de gestion d'un client (exécutée dans un thread)
 * @param arg Pointeur vers client_data_t
 * @return NULL
 *
 * Cycle de vie:
 * 1. Afficher les stats serveur et leaderboard
 * 2. Demander et valider le nom du joueur (3-10 lettres uniquement)
 * 3. Générer le nombre aléatoire à deviner (0-100)
 * 4. Boucle de jeu: recevoir tentatives, envoyer indices (Grand/Petit)
 * 5. Victoire: calculer score, mettre à jour leaderboard
 * 6. Nettoyage et fermeture
 */
void *handle_client(void *arg) {
    client_data_t *client = (client_data_t *)arg;
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    // Mise à jour des compteurs
    pthread_mutex_lock(&clients_mutex);
    active_clients++;
    total_clients_served++;
    pthread_mutex_unlock(&clients_mutex);

    // Log de connexion
    snprintf(buffer, sizeof(buffer),
        "Client #%d connecté depuis %s",
        client->client_id,
        inet_ntoa(client->address.sin_addr));
    log_message("INFO", buffer);

    // ========================================================================
    // ÉTAPE 1: AFFICHER LES STATISTIQUES ET LEADERBOARD
    // ========================================================================
    display_server_stats(client->socket);
    display_leaderboard(client->socket);

    // ========================================================================
    // ÉTAPE 2: DEMANDER ET VALIDER LE NOM DU JOUEUR
    // ========================================================================
    send_message(client->socket,
        "\n╔═══════════════════════════════════════════════════╗\n"
        "║              🎮 BIENVENUE AU JEU ! 🎮             ║\n"
        "╠═══════════════════════════════════════════════════╣\n"
        "║ 🎯 Devinez le nombre entre 0 et 100              ║\n"
        "║ 🏆 Score = 10000 - (tentatives × 100) - temps    ║\n"
        "║ 📛 Nom: 3-10 lettres (a-z, A-Z) uniquement       ║\n"
        "╚═══════════════════════════════════════════════════╝\n\n"
        "📝 Entrez votre nom (3-10 lettres): ");

    // Boucle de validation du nom
    int name_validated = 0;
    int name_attempts = 0;
    const int MAX_NAME_ATTEMPTS = 5;

    while (!name_validated && name_attempts < MAX_NAME_ATTEMPTS) {
        if (receive_message(client->socket, buffer, BUFFER_SIZE) <= 0) {
            log_message("WARNING", "Client déconnecté pendant la saisie du nom");
            goto cleanup;
        }

        name_attempts++;

        // Validation stricte du nom
        if (validate_name(buffer)) {
            strncpy(client->name, buffer, MAX_NAME_LENGTH - 1);
            client->name[MAX_NAME_LENGTH - 1] = '\0';
            name_validated = 1;

            snprintf(response, sizeof(response),
                "✅ Bienvenue %s ! Préparation de votre partie...\n\n", client->name);
            send_message(client->socket, response);

            snprintf(buffer, sizeof(buffer),
                "Client #%d: Nom validé '%s'", client->client_id, client->name);
            log_message("SUCCESS", buffer);
        } else {
            send_message(client->socket,
                "❌ Nom invalide !\n"
                "   • Longueur: 3-10 caractères\n"
                "   • Uniquement des lettres (a-z, A-Z)\n"
                "   • Pas de chiffres, espaces ou caractères spéciaux\n\n"
                "📝 Réessayez: ");
        }
    }

    if (!name_validated) {
        send_message(client->socket,
            "❌ Trop de tentatives invalides. Déconnexion.\n");
        goto cleanup;
    }

    // ========================================================================
    // ÉTAPE 3: GÉNÉRER LE NOMBRE ALÉATOIRE ET INITIALISER LA PARTIE
    // ========================================================================
    client->target_number = (rand() % (MAX_NUMBER - MIN_NUMBER + 1)) + MIN_NUMBER;
    client->attempts = 0;
    client->start_time = time(NULL);

    snprintf(buffer, sizeof(buffer),
        "Client #%d - %s: Partie démarrée (cible: %d)",
        client->client_id, client->name, client->target_number);
    log_message("INFO", buffer);

    // Message de début de partie
    snprintf(response, sizeof(response),
        "╔═══════════════════════════════════════════════════╗\n"
        "║              🎯 DÉBUT DE LA PARTIE                ║\n"
        "╠═══════════════════════════════════════════════════╣\n"
        "║ Joueur          : %-31s║\n"
        "║ Plage           : %d - %-27d║\n"
        "║ Commandes       : 'stats' | 'quit'               ║\n"
        "╚═══════════════════════════════════════════════════╝\n\n"
        "🎲 Devinez le nombre entre %d et %d\n\n",
        client->name,
        MIN_NUMBER, MAX_NUMBER,
        MIN_NUMBER, MAX_NUMBER);
    send_message(client->socket, response);

    // ========================================================================
    // ÉTAPE 4: BOUCLE DE JEU PRINCIPALE
    // ========================================================================
    while (1) {
        if (receive_message(client->socket, buffer, BUFFER_SIZE) <= 0) {
            log_message("WARNING", "Client déconnecté");
            break;
        }

        client->attempts++;

        // Commande QUIT
        if (strcasecmp(buffer, "quit") == 0) {
            send_message(client->socket, "\n👋 Au revoir ! Merci d'avoir joué\n");
            break;
        }

        // Commande STATS
        if (strcasecmp(buffer, "stats") == 0) {
            display_server_stats(client->socket);
            display_leaderboard(client->socket);
            client->attempts--; // Ne pas compter comme tentative
            continue;
        }

        // Validation de l'entrée (nombre entier)
        char *endptr;
        errno = 0;
        long guess = strtol(buffer, &endptr, 10);

        if (errno != 0 || *endptr != '\0' || endptr == buffer) {
            send_message(client->socket, "❌ Entrez un nombre entier valide\n");
            client->attempts--;
            continue;
        }

        // Vérification de la plage
        if (guess < MIN_NUMBER || guess > MAX_NUMBER) {
            snprintf(response, sizeof(response),
                "❌ Le nombre doit être entre %d et %d\n",
                MIN_NUMBER, MAX_NUMBER);
            send_message(client->socket, response);
            client->attempts--;
            continue;
        }

        // Log de tentative
        snprintf(buffer, sizeof(buffer),
            "Client #%d - %s: Tentative %d → %ld (cible: %d)",
            client->client_id, client->name, client->attempts,
            guess, client->target_number);
        log_message("INFO", buffer);

        // ====================================================================
        // COMPARAISON ET RÉPONSE
        // ====================================================================
        if (guess > client->target_number) {
            snprintf(response, sizeof(response),
                "📉 Trop grand ! (Tentative #%d)\n", client->attempts);
            send_message(client->socket, response);

        } else if (guess < client->target_number) {
            snprintf(response, sizeof(response),
                "📈 Trop petit ! (Tentative #%d)\n", client->attempts);
            send_message(client->socket, response);

        } else {
            // ================================================================
            // VICTOIRE ! CALCUL DU SCORE ET MISE À JOUR DU LEADERBOARD
            // ================================================================
            time_t end_time = time(NULL);
            int duration = (int)difftime(end_time, client->start_time);
            int score = calculate_score(client->attempts, duration);

            // Message de victoire avec tableau élégant
            snprintf(response, sizeof(response),
                "\n╔═══════════════════════════════════════════════════╗\n"
                "║              🎉 FÉLICITATIONS ! 🎉                ║\n"
                "╠═══════════════════════════════════════════════════╣\n"
                "║ Joueur          : %-31s║\n"
                "║ Nombre trouvé   : %-31d║\n"
                "║ Tentatives      : %-31d║\n"
                "║ Temps écoulé    : %-28ds ║\n"
                "║ Score final     : %-31d║\n"
                "╠═══════════════════════════════════════════════════╣\n"
                "║ Calcul: 10000 - (%d × 100) - %d = %d             ║\n"
                "╚═══════════════════════════════════════════════════╝\n",
                client->name,
                client->target_number,
                client->attempts,
                duration,
                score,
                client->attempts, duration, score);
            send_message(client->socket, response);

            // Mise à jour des statistiques et leaderboard
            update_stats(client->attempts);
            add_to_leaderboard(client->name, client->attempts, duration, score);

            // Afficher le nouveau leaderboard
            send_message(client->socket, "\n");
            display_leaderboard(client->socket);

            // Log de victoire
            snprintf(buffer, sizeof(buffer),
                "Client #%d - %s: VICTOIRE en %d tentatives (%ds) - Score: %d",
                client->client_id, client->name, client->attempts,
                duration, score);
            log_message("SUCCESS", buffer);

            break;
        }
    }

cleanup:
    // ========================================================================
    // NETTOYAGE ET DÉCONNEXION
    // ========================================================================
    snprintf(buffer, sizeof(buffer),
        "Client #%d - %s: Déconnexion",
        client->client_id,
        client->name[0] ? client->name : "Anonyme");
    log_message("INFO", buffer);

    close(client->socket);

    pthread_mutex_lock(&clients_mutex);
    active_clients--;
    pthread_mutex_unlock(&clients_mutex);

    free(client);
    pthread_exit(NULL);
}

/**
 * @brief Fonction principale du serveur
 * @return EXIT_SUCCESS ou EXIT_FAILURE
 */
int main(void) {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_counter = 0;

    // Initialisation du générateur aléatoire
    srand((unsigned int)time(NULL));
    global_stats.server_start_time = time(NULL);

    // Configuration des gestionnaires de signaux
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGPIPE, SIG_IGN); // Ignorer SIGPIPE

    // Bannière de démarrage
    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║   🎮 SERVEUR JEU DE DEVINETTE MULTI-THREADÉ v2.0 🎮   ║\n");
    printf("║        Architecture Client-Serveur Professionnelle     ║\n");
    printf("╠════════════════════════════════════════════════════════╣\n");
    printf("║  Auteur : Opak (Penifana Abdoul-Khader Ouattara)      ║\n");
    printf("║  École  : ESATIC & Université Côte d'Azur             ║\n");
    printf("║  Cours  : PRAD - TP1 (Architecture Distribuée)        ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n\n");

    // Création du socket serveur
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("❌ Erreur de création du socket");
        exit(EXIT_FAILURE);
    }

    // Option pour réutiliser le port immédiatement
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("❌ Erreur setsockopt");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Configuration de l'adresse du serveur
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Écoute sur toutes les interfaces
    server_addr.sin_port = htons(PORT);

    // Liaison du socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("❌ Erreur de liaison du port");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Mise en écoute
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("❌ Erreur de mise en écoute");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Affichage des informations de démarrage
    log_message("SUCCESS", "Serveur démarré avec succès");
    printf("📡 Port d'écoute        : %d\n", PORT);
    printf("👥 Clients max          : %d\n", MAX_CLIENTS);
    printf("🎯 Plage de nombres     : %d - %d\n", MIN_NUMBER, MAX_NUMBER);
    printf("🏆 Top scores           : %d\n", TOP_SCORES);
    printf("📊 Score initial        : %d pts\n", INITIAL_SCORE);
    printf("⚡ Pénalité/tentative   : %d pts\n", ATTEMPT_PENALTY);
    printf("\n");
    log_message("INFO", "En attente de connexions clients...");
    printf("\n");

    // ========================================================================
    // BOUCLE PRINCIPALE DU SERVEUR
    // ========================================================================
    while (1) {
        // Allocation de la structure client
        client_data_t *client = malloc(sizeof(client_data_t));
        if (!client) {
            log_message("ERROR", "Erreur d'allocation mémoire pour client");
            continue;
        }
        memset(client, 0, sizeof(client_data_t));

        // Accepter la connexion
        client->socket = accept(server_socket,
                                (struct sockaddr *)&client->address,
                                &client_len);

        if (client->socket < 0) {
            log_message("ERROR", "Erreur d'acceptation de connexion");
            free(client);
            continue;
        }

        // Assigner un ID unique
        client->client_id = ++client_counter;

        // Vérifier le nombre maximum de clients
        pthread_mutex_lock(&clients_mutex);
        int current = active_clients;
        pthread_mutex_unlock(&clients_mutex);

        if (current >= MAX_CLIENTS) {
            log_message("WARNING", "Nombre maximum de clients atteint");
            send_message(client->socket,
                "⛔ Serveur plein ! Maximum de clients atteint.\n"
                "Veuillez réessayer dans quelques instants.\n");
            close(client->socket);
            free(client);
            continue;
        }

        // Créer un thread pour gérer le client
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void *)client) != 0) {
            log_message("ERROR", "Erreur de création du thread");
            close(client->socket);
            free(client);
            continue;
        }

        // Détacher le thread (libération automatique des ressources)
        pthread_detach(thread_id);
    }

    // Nettoyage (code jamais atteint sauf signal)
    close(server_socket);
    return EXIT_SUCCESS;
}
