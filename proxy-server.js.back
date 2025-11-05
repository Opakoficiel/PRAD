#!/usr/bin/env node
/**
 * Serveur Proxy WebSocket pour le jeu de devinette distribué v2.0 FINAL
 * Fait le pont entre les clients WebSocket (navigateur) et le serveur TCP (C)
 *
 * Auteur: Opak - ESATIC 2025
 * Version: 2.0 FINAL
 */

const WebSocket = require("ws");
const net = require("net");

// Configuration
const WEBSOCKET_PORT = 8081;
const TCP_SERVER_HOST = "localhost";
const TCP_SERVER_PORT = 8080;

// Compteurs et statistiques
let activeConnections = 0;
let totalConnections = 0;

// Couleurs pour les logs
const colors = {
  reset: "\x1b[0m",
  bright: "\x1b[1m",
  green: "\x1b[32m",
  yellow: "\x1b[33m",
  blue: "\x1b[34m",
  red: "\x1b[31m",
  cyan: "\x1b[36m",
  magenta: "\x1b[35m",
};

// Fonction de logging avec timestamp
function log(level, message, color = colors.reset) {
  const timestamp = new Date().toISOString().replace("T", " ").substr(0, 19);
  console.log(`${color}[${timestamp}] [${level}]${colors.reset} ${message}`);
}

// Bannière de démarrage
function displayBanner() {
  console.log(
    "\n" +
      colors.cyan +
      "╔══════════════════════════════════════════════════════╗",
  );
  console.log("║     PROXY WEBSOCKET → TCP v2.0 FINAL                 ║");
  console.log("║     Bridge pour Client Web → Serveur C               ║");
  console.log("║     🏆 Avec Système de Scoring Compétitif 🏆        ║");
  console.log(
    "╚══════════════════════════════════════════════════════╝" +
      colors.reset +
      "\n",
  );
}

// Créer le serveur WebSocket
const wss = new WebSocket.Server({
  port: WEBSOCKET_PORT,
  perMessageDeflate: false,
});

displayBanner();
log(
  "SUCCESS",
  `Serveur WebSocket démarré sur le port ${WEBSOCKET_PORT}`,
  colors.green,
);
log(
  "INFO",
  `Prêt à se connecter au serveur TCP: ${TCP_SERVER_HOST}:${TCP_SERVER_PORT}`,
  colors.blue,
);
log("INFO", "En attente de connexions WebSocket...", colors.blue);
console.log("");

// Gestion des connexions WebSocket
wss.on("connection", (ws, req) => {
  activeConnections++;
  totalConnections++;
  const clientId = totalConnections;
  const clientIP = req.socket.remoteAddress;

  log(
    "CONNECT",
    `Client WS #${clientId} connecté depuis ${clientIP}`,
    colors.green,
  );
  log(
    "INFO",
    `Connexions WebSocket actives: ${activeConnections}`,
    colors.cyan,
  );

  // Créer une connexion TCP vers le serveur C
  const tcpClient = new net.Socket();
  let tcpConnected = false;

  // Connexion au serveur TCP
  tcpClient.connect(TCP_SERVER_PORT, TCP_SERVER_HOST, () => {
    tcpConnected = true;
    log(
      "TCP",
      `Client #${clientId}: Connexion TCP établie avec le serveur C`,
      colors.blue,
    );
  });

  // Recevoir les données du serveur TCP
  tcpClient.on("data", (data) => {
    const message = data.toString("utf8");

    // Log plus concis pour les longs messages
    const preview =
      message.length > 100 ? message.substring(0, 100) + "..." : message;
    const lines = preview.split("\n").length;
    log(
      "TCP→WS",
      `Client #${clientId}: ${lines} lignes (${message.length} bytes)`,
      colors.yellow,
    );

    // Envoyer au client WebSocket
    if (ws.readyState === WebSocket.OPEN) {
      ws.send(message);
    }
  });

  // Recevoir les messages du client WebSocket
  ws.on("message", (message) => {
    const msg = message.toString("utf8").trim();
    log("WS→TCP", `Client #${clientId}: "${msg}"`, colors.cyan);

    // Envoyer au serveur TCP
    if (tcpConnected) {
      const dataToSend = msg.endsWith("\n") ? msg : msg + "\n";
      tcpClient.write(dataToSend);
    } else {
      log(
        "ERROR",
        `Client #${clientId}: TCP non connecté, impossible d'envoyer`,
        colors.red,
      );
      if (ws.readyState === WebSocket.OPEN) {
        ws.send("❌ ERREUR: Connexion au serveur TCP non établie\n");
      }
    }
  });

  // Gestion de la fermeture WebSocket
  ws.on("close", (code, reason) => {
    activeConnections--;
    const reasonText = reason ? reason.toString() : "aucune raison";
    log(
      "DISCONNECT",
      `Client WS #${clientId} déconnecté (code: ${code}, raison: ${reasonText})`,
      colors.yellow,
    );
    log(
      "INFO",
      `Connexions WebSocket actives: ${activeConnections}`,
      colors.cyan,
    );

    // Fermer la connexion TCP
    if (tcpConnected) {
      tcpClient.end();
    }
  });

  // Gestion des erreurs WebSocket
  ws.on("error", (error) => {
    log("ERROR", `Client WS #${clientId}: ${error.message}`, colors.red);
  });

  // Gestion de la fermeture TCP
  tcpClient.on("close", () => {
    tcpConnected = false;
    log(
      "TCP",
      `Client #${clientId}: Connexion TCP fermée par le serveur`,
      colors.yellow,
    );

    // Fermer le WebSocket si encore ouvert
    if (ws.readyState === WebSocket.OPEN) {
      ws.close(1000, "TCP connection closed by server");
    }
  });

  // Gestion des erreurs TCP
  tcpClient.on("error", (error) => {
    log(
      "ERROR",
      `Client #${clientId}: TCP error: ${error.message}`,
      colors.red,
    );

    // Informer le client WebSocket
    if (ws.readyState === WebSocket.OPEN) {
      const errorMsg = `❌ ERREUR de connexion au serveur: ${error.message}\n`;
      ws.send(errorMsg);
      ws.close(1011, "TCP connection error");
    }
  });

  // Timeout de connexion TCP
  tcpClient.setTimeout(60000); // 60 secondes
  tcpClient.on("timeout", () => {
    log("TIMEOUT", `Client #${clientId}: TCP timeout (60s)`, colors.red);
    tcpClient.end();
  });
});

// Gestion des erreurs du serveur WebSocket
wss.on("error", (error) => {
  log("ERROR", `Serveur WebSocket: ${error.message}`, colors.red);
});

// Afficher les statistiques toutes les 60 secondes
setInterval(() => {
  if (activeConnections > 0) {
    log(
      "STATS",
      `📊 Connexions actives: ${activeConnections} | Total servi: ${totalConnections}`,
      colors.magenta,
    );
  }
}, 60000);

// Gestion de l'arrêt propre
process.on("SIGINT", () => {
  console.log("\n");
  log("SHUTDOWN", "Signal SIGINT reçu, arrêt du proxy...", colors.yellow);

  // Fermer toutes les connexions WebSocket
  wss.clients.forEach((ws) => {
    if (ws.readyState === WebSocket.OPEN) {
      ws.close(1001, "Server shutting down");
    }
  });

  wss.close(() => {
    log("SUCCESS", "Serveur proxy arrêté proprement", colors.green);
    process.exit(0);
  });

  // Forcer l'arrêt après 5 secondes
  setTimeout(() => {
    log("WARNING", "Arrêt forcé après timeout", colors.yellow);
    process.exit(0);
  }, 5000);
});

process.on("SIGTERM", () => {
  console.log("\n");
  log("SHUTDOWN", "Signal SIGTERM reçu, arrêt...", colors.yellow);
  process.exit(0);
});

// Gestion des erreurs non capturées
process.on("uncaughtException", (error) => {
  log("FATAL", `Exception non gérée: ${error.message}`, colors.red);
  console.error(error.stack);
});

process.on("unhandledRejection", (reason, promise) => {
  log("FATAL", `Promise rejetée non gérée: ${reason}`, colors.red);
});

// Message final
console.log(colors.green + "✅ Proxy WebSocket opérationnel!\n" + colors.reset);
console.log(colors.cyan + "📡 Les clients web peuvent se connecter à:");
console.log(`   ws://localhost:${WEBSOCKET_PORT}\n` + colors.reset);
console.log(
  colors.yellow +
    "⚠️  Assurez-vous que le serveur C est démarré sur le port " +
    TCP_SERVER_PORT +
    "\n" +
    colors.reset,
);
console.log(colors.magenta + "💡 Configuration:");
console.log(`   - Plage: 0-100 (fixe)`);
console.log(`   - Noms: 3-10 lettres uniquement`);
console.log(`   - Score: 10000 - (essais×100) - temps`);
console.log(`   - Prix: 🏆 Plat offert au 1er!\n` + colors.reset);
