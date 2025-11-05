# 🎮 Jeu de Devinette Distribué - PRAD TP1

## 📋 Description

Projet de **jeu de devinette multi-joueurs compétitif** développé dans le cadre du cours PRAD (Programmation Réseaux et Applications Distribuées) à l'ESATIC.

Architecture client-serveur distribuée avec système de scoring, leaderboard et support multi-clients simultanés.

## 👥 Auteurs

- **Opak** (Penifana Abdoul-Khader Ouattara)
- **Bire Ismaël Zie**

**Formation:** Master Mobiquité, Big Data et Intégration Système
**Établissements:** ESATIC (Abidjan, Côte d'Ivoire) & Université Côte d'Azur
**Année:** 2025-2026

---

## 🏗️ Architecture du Système

```
┌─────────────────┐
│  Client Python  │ ──┐
│  (Terminal CLI) │   │
└─────────────────┘   │
                      ├──> [TCP Port 8080] ──> ┌─────────────────────┐
┌─────────────────┐   │                        │   Serveur C (TCP)   │
│   Client Web    │   │                        │   Multi-threadé     │
│  (HTML/JS/CSS)  │ ──┼──> [WebSocket 8081]──> │    + Leaderboard    │
└─────────────────┘      └> [Proxy Node.js]    └─────────────────────┘
                              (Bridge WS→TCP)
```

### Composants

1. **Serveur Backend** (`server.c`)
   - Langage: C avec POSIX threads
   - Port: 8080 (TCP)
   - Protocole: JSON
   - Support: 30 clients simultanés max

2. **Proxy WebSocket** (`proxy-server.js`)
   - Langage: Node.js
   - Bridge: WebSocket (8081) → TCP (8080)
   - Logging détaillé avec couleurs

3. **Client Python** (`client.py`)
   - Interface terminal ultra-moderne
   - Connexion TCP directe au serveur
   - Animations et couleurs ANSI

4. **Client Web** (`index.html`)
   - Design Glassmorphism moderne
   - WebSocket via proxy
   - Responsive + animations

---

## 🎯 Règles du Jeu

| Paramètre | Valeur |
|-----------|--------|
| **Plage de nombres** | 0 - 100 (fixe) |
| **Format du nom** | 3-10 lettres (a-z, A-Z uniquement) |
| **Score initial** | 10000 points |
| **Pénalité/tentative** | -100 points |
| **Pénalité temps** | -1 point/seconde |
| **Leaderboard** | Top 10 scores |
| **Commandes spéciales** | `stats`, `quit` |

### Formule de Score

```
Score = 10000 - (tentatives × 100) - temps_en_secondes
```

**Exemple:** 5 tentatives en 30 secondes = 10000 - 500 - 30 = **9470 points**

---

## 🚀 Installation et Utilisation

### Prérequis

- **GCC** (compilateur C avec support pthread)
- **Python 3.7+**
- **Node.js 12+** avec npm
- **Navigateur moderne** (Chrome, Firefox, Edge)

### 1️⃣ Compilation du Serveur C

```bash
gcc -o server server.c -pthread -Wall -Wextra -O2
```

### 2️⃣ Installation des Dépendances Node.js

```bash
npm install ws
```

### 3️⃣ Démarrage du Système

#### Terminal 1: Serveur C
```bash
./server
```
**Sortie attendue:**
```
╔════════════════════════════════════════════════════════╗
║   🎮 SERVEUR JEU DE DEVINETTE MULTI-THREADÉ v2.0 🎮   ║
╚════════════════════════════════════════════════════════╝

✅ Serveur démarré avec succès
📡 Port d'écoute        : 8080
👥 Clients max          : 30
🎯 Plage de nombres     : 0 - 100
```

#### Terminal 2: Proxy WebSocket
```bash
node proxy-server.js
```
**Sortie attendue:**
```
╔══════════════════════════════════════════════════════╗
║     PROXY WEBSOCKET → TCP v2.0 FINAL                 ║
╚══════════════════════════════════════════════════════╝

✅ Serveur WebSocket démarré sur le port 8081
📡 Les clients web peuvent se connecter à:
   ws://localhost:8081
```

#### Terminal 3: Client Python (optionnel)
```bash
python3 client.py localhost 8080
```

#### Navigateur: Client Web
```
Ouvrir index.html dans un navigateur
Se connecter à ws://localhost:8081
```

---

## 📡 Protocole de Communication JSON

Le serveur communique en JSON pour garantir la compatibilité avec tous les clients.

### Messages Serveur → Client

#### 1. Statistiques du Serveur
```json
{
  "type": "stats",
  "uptime": 3600,
  "active_clients": 5,
  "total_served": 42,
  "total_games": 38,
  "best_attempts": 3,
  "avg_attempts": 7.2
}
```

#### 2. Leaderboard
```json
{
  "type": "leaderboard",
  "count": 3,
  "scores": [
    {"rank": 1, "name": "Alice", "score": 9750, "attempts": 2, "duration": 10},
    {"rank": 2, "name": "Bob", "score": 9500, "attempts": 4, "duration": 15},
    {"rank": 3, "name": "Charlie", "score": 9200, "attempts": 6, "duration": 20}
  ]
}
```

#### 3. Prompt (demande d'entrée)
```json
{
  "type": "prompt",
  "message": "Entrez votre nom (3-10 lettres, a-z uniquement)"
}
```

#### 4. Nom Accepté
```json
{
  "type": "name_accepted",
  "name": "Alice"
}
```

#### 5. Début de Partie
```json
{
  "type": "game_start",
  "player": "Alice",
  "min": 0,
  "max": 100
}
```

#### 6. Indice
```json
{
  "type": "hint",
  "direction": "grand",  // ou "petit"
  "attempts": 3
}
```

#### 7. Victoire
```json
{
  "type": "victory",
  "player": "Alice",
  "number": 42,
  "attempts": 5,
  "duration": 30,
  "score": 9470
}
```

#### 8. Erreur
```json
{
  "type": "error",
  "message": "Nom invalide ! Longueur: 3-10 lettres (a-z, A-Z uniquement)"
}
```

#### 9. Au Revoir
```json
{
  "type": "bye",
  "message": "Au revoir ! Merci d'avoir joue"
}
```

### Messages Client → Serveur

Les clients envoient du **texte brut** :
- Nom du joueur (ex: `Alice`)
- Nombre deviné (ex: `42`)
- Commandes spéciales : `stats`, `quit`

---

## 🛠️ Fonctionnalités Techniques

### Serveur C (server.c)

✅ **Multi-threading POSIX**
- Thread dédié par client
- Mutex pour thread-safety (leaderboard, stats globales)
- Détachement automatique des threads

✅ **Validation Stricte**
- Noms: 3-10 lettres uniquement (regex: `[a-zA-Z]{3,10}`)
- Nombres: 0-100 uniquement
- Max 5 tentatives pour le nom

✅ **Gestion Propre**
- Signaux SIGINT/SIGTERM capturés
- Fermeture propre des sockets
- Libération mémoire automatique

✅ **Système de Scoring**
- Calcul: `10000 - (essais × 100) - temps`
- Leaderboard trié automatiquement
- Persistance en mémoire (top 10)

### Proxy WebSocket (proxy-server.js)

✅ **Bridge Bidirectionnel**
- Conversion WebSocket ↔ TCP transparente
- 1 connexion TCP par client WebSocket
- Timeout 60 secondes

✅ **Logging Professionnel**
- Timestamp sur chaque log
- Couleurs ANSI pour lisibilité
- Statistiques périodiques (60s)

### Client Python (client.py)

✅ **Interface Moderne**
- Couleurs ANSI 256 couleurs
- Animations (spinner, célébration)
- Tableaux Unicode élégants

✅ **Parsing JSON**
- Gestion de tous les types de messages
- Reconnexion automatique pour rejouer
- Affichage leaderboard formaté

### Client Web (index.html)

✅ **Design Glassmorphism**
- Dégradés modernes
- Backdrop blur effects
- Animations CSS fluides

✅ **Fonctionnalités**
- Timer temps réel
- Compteur tentatives
- Leaderboard dynamique avec médailles 🥇🥈🥉
- Animation confettis à la victoire (80 particules)

---

## 📊 Exemple de Session de Jeu

```
CLIENT                           SERVEUR

Connexion TCP
    ├──────────────────────>     [Accepte connexion]
    <──────────────────────      {"type":"stats",...}
    <──────────────────────      {"type":"leaderboard",...}
    <──────────────────────      {"type":"prompt",...}

"Alice"
    ├──────────────────────>     [Valide nom]
    <──────────────────────      {"type":"name_accepted","name":"Alice"}
    <──────────────────────      {"type":"game_start","min":0,"max":100}

"50"                              [Nombre cible: 73]
    ├──────────────────────>
    <──────────────────────      {"type":"hint","direction":"petit","attempts":1}

"80"
    ├──────────────────────>
    <──────────────────────      {"type":"hint","direction":"grand","attempts":2}

"73"
    ├──────────────────────>
    <──────────────────────      {"type":"victory",...,"score":9780}
    <──────────────────────      {"type":"leaderboard",...}
```

---

## 🔧 Dépannage

### Problème: Port 8080 déjà utilisé

```bash
# Trouver le processus
lsof -i :8080

# Tuer le processus
kill -9 <PID>
```

### Problème: Client web ne se connecte pas

1. Vérifier que le serveur C est démarré
2. Vérifier que le proxy Node.js est démarré
3. Ouvrir la console développeur (F12) pour voir les erreurs WebSocket

### Problème: Erreurs de compilation

```bash
# Installer les headers pthread (Linux Debian/Ubuntu)
sudo apt-get install build-essential

# Vérifier la version GCC
gcc --version  # Doit être >= 4.8
```

---

## 📝 Fichiers du Projet

```
PRAD/
├── server.c              # Serveur TCP multi-threadé (C)
├── client.py             # Client terminal (Python)
├── index.html            # Client web (HTML/CSS/JS)
├── proxy-server.js       # Proxy WebSocket→TCP (Node.js)
├── README.md             # Documentation complète
└── server                # Binaire compilé (généré)
```

---

## 🎓 Concepts Pédagogiques Abordés

- ✅ Sockets TCP/IP
- ✅ Multi-threading POSIX (pthread)
- ✅ Synchronisation par mutex
- ✅ Protocole JSON
- ✅ WebSocket
- ✅ Architecture client-serveur
- ✅ Gestion de sessions multiples
- ✅ Validation d'entrées utilisateur
- ✅ Gestion de signaux UNIX

---

## 📜 Licence

Projet académique - ESATIC & Université Côte d'Azur 2025-2026

---

## 🙏 Remerciements

- Professeurs du cours PRAD
- ESATIC & Université Côte d'Azur
- Communauté open-source (Node.js, WebSocket)

---

**Bon jeu ! 🎮🏆**
