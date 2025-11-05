#!/usr/bin/env python3
"""
============================================================================
CLIENT PYTHON ULTRA-MODERNE - JEU DE DEVINETTE v3.0
============================================================================
Interface terminal MAGNIFIQUE avec parsing JSON !

Auteurs: Opak (Penifana Abdoul-Khader Ouattara) & Bire Ismaël Zie
École: ESATIC & Université Côte d'Azur
Cours: PRAD - TP1 (2025-2026)

Usage: python3 client.py <IP_SERVEUR> [PORT]
============================================================================
"""

import socket
import sys
import signal
import time
import json
from typing import Optional

# Configuration
DEFAULT_PORT = 8080
BUFFER_SIZE = 8192
CONNECTION_TIMEOUT = 30

# ============================================================================
# COULEURS ANSI ULTRA-MODERNES
# ============================================================================
class C:
    """Palette de couleurs professionnelle"""
    # Couleurs de base
    PURPLE = '\033[38;5;141m'
    GREEN = '\033[38;5;156m'
    YELLOW = '\033[38;5;221m'
    RED = '\033[38;5;204m'
    CYAN = '\033[38;5;117m'
    BLUE = '\033[38;5;75m'
    PINK = '\033[38;5;213m'
    ORANGE = '\033[38;5;214m'
    GRAY = '\033[38;5;246m'

    # Modificateurs
    RESET = '\033[0m'
    BOLD = '\033[1m'
    DIM = '\033[2m'

    # Symboles Unicode modernes
    CHECK = '✓'
    CROSS = '✗'
    ARROW = '→'
    STAR = '★'
    FIRE = '🔥'
    ROCKET = '🚀'
    TROPHY = '🏆'
    MEDAL = '🥇'
    GAME = '🎮'
    PARTY = '🎉'

# ============================================================================
# FONCTIONS UTILITAIRES D'INTERFACE
# ============================================================================

def clear():
    """Efface l'écran"""
    print('\033[2J\033[H', end='', flush=True)

def banner():
    """Bannière ultra-moderne avec dégradé"""
    clear()
    print(f"""
{C.PURPLE}{C.BOLD}
    ╔═══════════════════════════════════════════════════════════╗
    ║                                                           ║
    ║        {C.FIRE} JEU DE DEVINETTE v3.0 - CLIENT PYTHON {C.FIRE}        ║
    ║                                                           ║
    ║              {C.CYAN}Interface Ultra-Moderne avec JSON{C.PURPLE}           ║
    ║                                                           ║
    ╠═══════════════════════════════════════════════════════════╣
    ║  {C.PINK}Auteurs{C.PURPLE}: Opak & Bire Ismaël Zie                       ║
    ║  {C.PINK}École{C.PURPLE}  : ESATIC & Université Côte d'Azur              ║
    ╚═══════════════════════════════════════════════════════════╝
{C.RESET}
""")

def box(title, content, color=C.CYAN):
    """Boîte élégante avec bordures"""
    width = 60
    print(f"\n{color}{C.BOLD}╔{'═' * width}╗{C.RESET}")
    print(f"{color}{C.BOLD}║{C.RESET} {title:^{width-2}} {color}{C.BOLD}║{C.RESET}")
    print(f"{color}{C.BOLD}╠{'═' * width}╣{C.RESET}")
    for line in content.split('\n'):
        if line.strip():
            print(f"{color}║{C.RESET} {line:<{width-2}} {color}║{C.RESET}")
    print(f"{color}{C.BOLD}╚{'═' * width}╝{C.RESET}\n")

def table(headers, rows, color=C.PURPLE):
    """Tableau élégant"""
    col_widths = [max(len(str(row[i])) for row in [headers] + rows) + 2 for i in range(len(headers))]
    total_width = sum(col_widths) + len(headers) + 1

    print(f"\n{color}{C.BOLD}╔{'═' * total_width}╗{C.RESET}")

    # En-têtes
    header_line = f"{color}{C.BOLD}║{C.RESET}"
    for i, header in enumerate(headers):
        header_line += f" {C.BOLD}{header:^{col_widths[i]-2}}{C.RESET} {color}{C.BOLD}│{C.RESET}" if i < len(headers) - 1 else f" {C.BOLD}{header:^{col_widths[i]-2}}{C.RESET} {color}{C.BOLD}║{C.RESET}"
    print(header_line)

    print(f"{color}{C.BOLD}╠{'═' * total_width}╣{C.RESET}")

    # Lignes
    for row in rows:
        row_line = f"{color}║{C.RESET}"
        for i, cell in enumerate(row):
            cell_str = str(cell)
            row_line += f" {cell_str:^{col_widths[i]-2}} {color}│{C.RESET}" if i < len(row) - 1 else f" {cell_str:^{col_widths[i]-2}} {color}║{C.RESET}"
        print(row_line)

    print(f"{color}{C.BOLD}╚{'═' * total_width}╝{C.RESET}\n")

def spinner(text, duration=1.5):
    """Animation de chargement moderne"""
    frames = ['⠋', '⠙', '⠹', '⠸', '⠼', '⠴', '⠦', '⠧', '⠇', '⠏']
    end_time = time.time() + duration
    i = 0
    while time.time() < end_time:
        print(f'\r{C.CYAN}{frames[i % len(frames)]}{C.RESET} {text}', end='', flush=True)
        time.sleep(0.08)
        i += 1
    print(f'\r{C.GREEN}{C.CHECK}{C.RESET} {text}')

def celebrate():
    """Animation de victoire épique"""
    print(f"\n{C.YELLOW}{C.BOLD}")
    print("    ╔═══════════════════════════════════════════════╗")
    print("    ║                                               ║")
    print(f"    ║          {C.PARTY} VICTOIRE ÉPIQUE ! {C.PARTY}              ║")
    print("    ║                                               ║")
    print("    ╚═══════════════════════════════════════════════╝")
    print(f"{C.RESET}\n")
    time.sleep(1)

def prompt(text):
    """Prompt élégant"""
    return input(f"{C.PURPLE}{C.ARROW}{C.RESET} {text}").strip()

# ============================================================================
# CLASSE PRINCIPALE DU CLIENT
# ============================================================================

class GameClient:
    """Client de jeu avec parsing JSON"""

    def __init__(self, host: str, port: int):
        self.host = host
        self.port = port
        self.socket: Optional[socket.socket] = None
        self.connected = False
        self.player_name = ""

    def connect(self) -> bool:
        """Connexion au serveur"""
        try:
            spinner(f"Connexion à {self.host}:{self.port}")
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(CONNECTION_TIMEOUT)
            self.socket.connect((self.host, self.port))
            self.connected = True
            print(f"{C.GREEN}{C.CHECK} Connecté avec succès !{C.RESET}\n")
            return True
        except Exception as e:
            print(f"\n{C.RED}{C.CROSS} Erreur: {e}{C.RESET}\n")
            return False

    def send(self, message: str) -> bool:
        """Envoyer un message"""
        try:
            self.socket.sendall(f"{message}\n".encode('utf-8'))
            return True
        except:
            return False

    def receive_json(self, timeout: float = 2.0) -> Optional[dict]:
        """Recevoir et parser le JSON"""
        try:
            old_timeout = self.socket.gettimeout()
            self.socket.settimeout(timeout)

            data = self.socket.recv(BUFFER_SIZE)

            self.socket.settimeout(old_timeout)

            if not data:
                return None

            json_str = data.decode('utf-8').strip()
            return json.loads(json_str)
        except json.JSONDecodeError:
            return None
        except:
            return None

    def display_stats(self, data: dict):
        """Afficher les stats du serveur de manière élégante et dynamique"""
        # Conversion de l'uptime en format lisible
        uptime_sec = data['uptime']
        hours = uptime_sec // 3600
        minutes = (uptime_sec % 3600) // 60
        seconds = uptime_sec % 60
        uptime_str = f"{hours:02d}h {minutes:02d}m {seconds:02d}s"

        # Création d'une barre de progression pour la moyenne de tentatives
        avg = data['avg_attempts']
        max_attempts = 20  # Max pour la visualisation
        bar_length = int((avg / max_attempts) * 20) if avg < max_attempts else 20
        bar = f"{C.GREEN}{'█' * bar_length}{C.GRAY}{'░' * (20 - bar_length)}{C.RESET}"

        print(f"\n{C.CYAN}{C.BOLD}╔{'═' * 62}╗{C.RESET}")
        print(f"{C.CYAN}{C.BOLD}║{C.RESET} {C.FIRE} {'STATISTIQUES DU SERVEUR':^56} {C.FIRE} {C.CYAN}{C.BOLD}║{C.RESET}")
        print(f"{C.CYAN}{C.BOLD}╠{'═' * 62}╣{C.RESET}")

        # Ligne 1 : Uptime et Clients actifs
        print(f"{C.CYAN}║{C.RESET}  {C.YELLOW}⏱️  Uptime{C.RESET}              : {C.BOLD}{uptime_str}{C.RESET}                  {C.CYAN}║{C.RESET}")
        print(f"{C.CYAN}║{C.RESET}  {C.GREEN}👥 Clients actifs{C.RESET}      : {C.BOLD}{data['active_clients']}{C.RESET}                                 {C.CYAN}║{C.RESET}")
        print(f"{C.CYAN}║{C.RESET}  {C.BLUE}📈 Total servi{C.RESET}         : {C.BOLD}{data['total_served']}{C.RESET}                                 {C.CYAN}║{C.RESET}")
        print(f"{C.CYAN}║{C.RESET}  {C.PURPLE}🎮 Parties jouées{C.RESET}      : {C.BOLD}{data['total_games']}{C.RESET}                                 {C.CYAN}║{C.RESET}")

        best = data['best_attempts'] if data['best_attempts'] != 0 else "Aucun"
        print(f"{C.CYAN}║{C.RESET}  {C.ORANGE}🏆 Meilleur (tentatives){C.RESET}: {C.BOLD}{best}{C.RESET}                             {C.CYAN}║{C.RESET}")

        print(f"{C.CYAN}║{C.RESET}  {C.PINK}📊 Moyenne tentatives{C.RESET}  : {C.BOLD}{avg:.1f}{C.RESET}                              {C.CYAN}║{C.RESET}")
        print(f"{C.CYAN}║{C.RESET}     {bar}                                {C.CYAN}║{C.RESET}")
        print(f"{C.CYAN}{C.BOLD}╚{'═' * 62}╝{C.RESET}\n")

    def display_leaderboard(self, data: dict):
        """Afficher le leaderboard avec un tableau ultra-moderne et dynamique"""
        if data['count'] == 0:
            print(f"\n{C.GRAY}{C.DIM}┌{'─' * 60}┐{C.RESET}")
            print(f"{C.GRAY}{C.DIM}│{'Aucun score enregistré':^60}│{C.RESET}")
            print(f"{C.GRAY}{C.DIM}└{'─' * 60}┘{C.RESET}\n")
            return

        print(f"\n{C.YELLOW}{C.BOLD}{'═' * 70}{C.RESET}")
        print(f"{C.YELLOW}{C.BOLD} {C.TROPHY}  TOP {data['count']} MEILLEURS SCORES - HALL OF FAME  {C.TROPHY}{C.RESET}")
        print(f"{C.YELLOW}{C.BOLD}{'═' * 70}{C.RESET}\n")

        # En-tête du tableau
        print(f"{C.YELLOW}{C.BOLD}╔{'═' * 5}╦{'═' * 15}╦{'═' * 12}╦{'═' * 10}╦{'═' * 11}╗{C.RESET}")
        print(f"{C.YELLOW}{C.BOLD}║{C.RESET} {'🏅':^3} {C.YELLOW}{C.BOLD}║{C.RESET} {'Joueur':^13} {C.YELLOW}{C.BOLD}║{C.RESET} {'Score':^10} {C.YELLOW}{C.BOLD}║{C.RESET} {'Essais':^8} {C.YELLOW}{C.BOLD}║{C.RESET} {'Temps':^9} {C.YELLOW}{C.BOLD}║{C.RESET}")
        print(f"{C.YELLOW}{C.BOLD}╠{'═' * 5}╬{'═' * 15}╬{'═' * 12}╬{'═' * 10}╬{'═' * 11}╣{C.RESET}")

        # Lignes du tableau avec couleurs dynamiques
        for score in data['scores']:
            rank = score['rank']

            # Médaille et couleur selon le rang
            if rank == 1:
                medal = C.MEDAL
                color = C.YELLOW
            elif rank == 2:
                medal = '🥈'
                color = C.GRAY
            elif rank == 3:
                medal = '🥉'
                color = C.ORANGE
            else:
                medal = f"#{rank}"
                color = C.RESET

            # Barre de visualisation du score
            score_val = score['score']
            max_score = 10000
            bar_length = int((score_val / max_score) * 8)
            score_bar = f"{C.GREEN}{'█' * bar_length}{C.GRAY}{'░' * (8 - bar_length)}{C.RESET}"

            # Affichage de la ligne
            print(f"{C.YELLOW}║{C.RESET} {medal:^3} {C.YELLOW}║{C.RESET} {color}{C.BOLD}{score['name']:^13}{C.RESET} {C.YELLOW}║{C.RESET} {C.GREEN}{C.BOLD}{score_val:>6}{C.RESET} pts {C.YELLOW}║{C.RESET} {C.CYAN}{score['attempts']:^8}{C.RESET} {C.YELLOW}║{C.RESET} {C.PURPLE}{score['duration']:>5}s{C.RESET}   {C.YELLOW}║{C.RESET}")

            # Barre de visualisation
            if rank <= 3:
                print(f"{C.YELLOW}║{C.RESET}     {C.YELLOW}║{C.RESET}               {C.YELLOW}║{C.RESET} {score_bar}  {C.YELLOW}║{C.RESET}          {C.YELLOW}║{C.RESET}           {C.YELLOW}║{C.RESET}")

            # Séparateur entre les lignes (sauf dernière)
            if rank < data['count']:
                print(f"{C.YELLOW}╠{'─' * 5}╬{'─' * 15}╬{'─' * 12}╬{'─' * 10}╬{'─' * 11}╣{C.RESET}")

        print(f"{C.YELLOW}{C.BOLD}╚{'═' * 5}╩{'═' * 15}╩{'═' * 12}╩{'═' * 10}╩{'═' * 11}╝{C.RESET}\n")

    def play_game(self) -> bool:
        """Boucle de jeu principale"""
        while self.connected:
            data = self.receive_json()

            if not data:
                print(f"{C.RED}{C.CROSS} Connexion perdue{C.RESET}")
                return False

            msg_type = data.get('type')

            # STATS
            if msg_type == 'stats':
                self.display_stats(data)

            # LEADERBOARD
            elif msg_type == 'leaderboard':
                self.display_leaderboard(data)

            # PROMPT (demande de nom)
            elif msg_type == 'prompt':
                print(f"{C.PURPLE}{data['message']}{C.RESET}")
                name = prompt("")
                self.send(name)

            # NOM ACCEPTÉ
            elif msg_type == 'name_accepted':
                self.player_name = data['name']
                print(f"{C.GREEN}{C.CHECK} Bienvenue {C.BOLD}{self.player_name}{C.RESET}{C.GREEN} !{C.RESET}\n")

            # DÉBUT DE PARTIE
            elif msg_type == 'game_start':
                box(
                    f"{C.GAME} DÉBUT DE LA PARTIE",
                    f"Joueur : {C.BOLD}{data['player']}{C.RESET}\n"
                    f"Plage  : {data['min']} - {data['max']}\n\n"
                    f"{C.CYAN}💡 Commandes: stats | quit{C.RESET}",
                    C.PURPLE
                )

                # Boucle de tentatives
                while True:
                    guess = prompt("Votre nombre ")
                    if not guess:
                        continue

                    self.send(guess)
                    response = self.receive_json()

                    if not response:
                        break

                    resp_type = response.get('type')

                    # INDICE
                    if resp_type == 'hint':
                        direction = response['direction']
                        attempts = response['attempts']

                        if direction == 'grand':
                            print(f"{C.ORANGE}📉 Trop grand ! {C.GRAY}(Tentative #{attempts}){C.RESET}")
                        else:
                            print(f"{C.BLUE}📈 Trop petit ! {C.GRAY}(Tentative #{attempts}){C.RESET}")

                    # VICTOIRE !
                    elif resp_type == 'victory':
                        celebrate()

                        box(
                            f"{C.PARTY} FÉLICITATIONS {C.PARTY}",
                            f"Joueur    : {C.BOLD}{response['player']}{C.RESET}\n"
                            f"Nombre    : {response['number']}\n"
                            f"Tentatives: {response['attempts']}\n"
                            f"Temps     : {response['duration']}s\n"
                            f"Score     : {C.YELLOW}{C.BOLD}{response['score']} points{C.RESET}",
                            C.GREEN
                        )

                        # Attendre le nouveau leaderboard
                        time.sleep(0.5)
                        lb_data = self.receive_json(timeout=3.0)
                        if lb_data and lb_data.get('type') == 'leaderboard':
                            self.display_leaderboard(lb_data)

                        return self.ask_retry()

                    # STATS
                    elif resp_type == 'stats':
                        self.display_stats(response)

                    # ERREUR
                    elif resp_type == 'error':
                        print(f"{C.RED}{C.CROSS} {response['message']}{C.RESET}")

                    # BYE
                    elif resp_type == 'bye':
                        print(f"{C.YELLOW}👋 {response['message']}{C.RESET}")
                        return False

            # ERREUR
            elif msg_type == 'error':
                print(f"{C.RED}{C.CROSS} {data['message']}{C.RESET}")
                return False

    def ask_retry(self) -> bool:
        """Demander si rejouer"""
        print(f"\n{C.PURPLE}Rejouer avec le même nom ? (o/n){C.RESET}")
        choice = prompt("").lower()
        return choice in ['o', 'oui', 'y', 'yes']

    def run(self):
        """Exécution principale"""
        box(
            f"{C.ROCKET} BIENVENUE",
            f"Score = 10000 - (essais × 100) - temps\n"
            f"Plage: 0-100 | Nom: 3-10 lettres (a-z)",
            C.PURPLE
        )

        if not self.connect():
            return

        while True:
            retry = self.play_game()
            if not retry:
                break

            # Reconnexion
            print(f"\n{C.CYAN}Reconnexion...{C.RESET}\n")
            self.disconnect()
            time.sleep(1)

            if not self.connect():
                break

            # Attendre stats et leaderboard
            self.receive_json(timeout=2.0)
            self.receive_json(timeout=2.0)

            # Envoyer le nom automatiquement
            self.send(self.player_name)

    def disconnect(self):
        """Déconnexion propre"""
        if self.socket:
            try:
                self.socket.close()
            except:
                pass
        self.connected = False

# ============================================================================
# PROGRAMME PRINCIPAL
# ============================================================================

def signal_handler(signum, frame):
    print(f"\n{C.YELLOW}Interruption{C.RESET}")
    sys.exit(0)

def main():
    signal.signal(signal.SIGINT, signal_handler)

    banner()

    if len(sys.argv) < 2:
        print(f"{C.RED}Usage: python3 client.py <IP_SERVEUR> [PORT]{C.RESET}")
        print(f"{C.GRAY}Exemple: python3 client.py 192.168.1.105{C.RESET}\n")
        sys.exit(1)

    host = sys.argv[1]
    port = int(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_PORT

    client = GameClient(host, port)

    try:
        client.run()
    except Exception as e:
        print(f"{C.RED}{C.CROSS} Erreur: {e}{C.RESET}")
    finally:
        client.disconnect()
        print(f"\n{C.PURPLE}{C.BOLD}Merci d'avoir joué ! À bientôt {C.FIRE}{C.RESET}\n")

if __name__ == "__main__":
    main()
