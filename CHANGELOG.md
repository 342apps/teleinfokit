# Release notes

## v2.2.80c509

### Firmware TeleInfoKit V2.2

Cette version de firmware met surtout l'accent sur la fiabilisation et simplification de la connexion WiFi, et sur la compatibilité avec la version 2026.4 de Home Assistant.

### Nouveautés

- Qrcode affiché à l'écran pour la connexion au réseau WiFi hotspot de configuration
- Nom de réseau wifi hotspot spécifique au module
- Affichage du total de consommation et du maximum sur les 24 dernières heures
- Mécanismes de reconnexion automatique au réseau WiFi en cas de perte ou d'absence, en fonctionnement ou lors du démarrage. Fixe [#30](https://github.com/342apps/teleinfokit/issues/30)

### Corrections

- Mise à jour du message de MQTT Auto Discovery pour compatibilité Home Assistant 2026.4. Merci @Damiendhn ! Fixe [#22](https://github.com/342apps/teleinfokit/issues/22)
- Robustification et prise en compte des index TEMPO pour le calcul des index pour le graphe affiché à l'écran. Merci @Damiendhn !

### New Contributors

@Damiendhn

## v2.1.0.390806

### Firmware TeleInfoKit V2.1

Cette version apporte bon nombre d'améliorations et de corrections qui visent à rendre le TeleInfoKit plus simple à configurer et intégrer à Home Assistant. La connexion MQTT a également été améliorée.

### Nouveautés

- La page configuration évolue pour intégrer le mode triphasé. Ce mode permet l'auto-déclaration des sensors associés, qui évite leur configuration manuelle.
- Un plus grand nombre de sensors est auto-déclaré dans Home Assistant
- La mise à jour de la configuration se reflète immédiatement dans Home Assistant
- Amélioration des messages de démarrage à l'écran
- Configuration stockée dans un format étendu pour de futurs besoins potentiels

### Corrections

- Les données des étiquettes TIC contenant un + sont maintenant remontées en remplaçant le + par _ (causait des 'malformed packet' sur MQTT)
- La connexion MQTT est rendue plus robuste notamment lors des phases d'auto-déclaration des sensors. (causait des 'client xxx has exceeded timeout')
- Fiabilise l'enregistrement de la configuration qui pouvait ne pas être prise en compte dans certains cas

### Remarque générale

Cette version remonte toutes les trames émises par le compteur, sans filtrage, comme le faisait déjà le firmware v2.0.0.312588. Aucune donnée additionnelle n'est à attendre avec cette version (Hormis les étiquettes TIC qui comportent un symbole `+`, corrigé dans cette version).

### New Contributors

@pyrech made their first contribution in #20 [Fix typo in MQTT discovery]

## v2.0.0.312588

### Firmware TeleInfoKit V2

Nouvelle version majeure pour le boitier TeleInfoKit, qui arrive avec une nouvelle version hardware du boîtier (v4).
Il s'agit d'une ré-écriture quasi totale pour simplifier le code, mettre à jour des librairies et apporter une meilleure stabilité.

### Nouveautés

- Gestion TIC mode Historique ET Standard, configurable via portail web
- Auto-discovery dans Home Assistant via MQTT
- Mots de passe hotspot wifi et firmware update unique par device
- Mode test pour vérifier le signal TIC sans configuration wifi

### Rétro-compatibilité

- Mise à jour possible en wifi depuis les firmwares v1.x
- Compatible avec les boitiers précédents (v3) [sans lecture TIC Standard]

### Breaking changes

- ⚠️ Topic MQTT modifié : contient maintenant l'ID du device (teleinfokit/data/-> teleinfokit-xxxxxx/data/)
- ⚠️ Page web contenant le graphe supprimée
- ⚠️ API HTTP REST supprimée

L'API et la page web sont supprimées depuis la version 2.0 du firmware pour des raisons de simplification et d'optimisation du code, et du peu d'utilité que ces fonctionnalités apportaient.

## v1.0.0.6bbff8

### Firmware TeleInfoKit V1

Une première version "majeure" avec des nouvelles fonctionnalités significatives :

- Fonctionnement génériques : remonte toutes les données TIC dans le topic teleinfokit/data (voir doc mqtt)
- Gestion des abonnements triphasé dans le dashboard web et dans les données MQTT
- Données d'index envoyés avec le flag retained

### Autres modifications

- Nouvel écran de démarrage
- Phase de démarrage accélérée
- Intégration librairie TIC pour adaptation aux besoins de ce firmware

### Rétro-compatibilité

Ce firmware est rétro-compatible avec les configurations mises en place pour les versions précédentes. Les données MQTT "classiques" sont toujours envoyées, les données génériques sont remontées en plus dans le topic teleinfokit/data. Cela permet de garder la configuration en place, ou d'exploiter les nouvelles données si besoin.

### New Contributors

@welcoMattic made their first contribution in #8

## v0.6.353c8f
Updates

Corrige l'histogramme sur le dashboard web quand on est en simple tarif (BASE). Merci à @NlsMyFox !

## v0.5.0b1deb

### Updates

Avoid activating wifi access point and reboot while wifi not found instead

## v0.4.4558e2

### Updates

- Add "base - simple tarif" handling when user has no "Heure Creuse/Heure Pleine" subscription
- Set user and password lenght to 32 characters long

## v0.3.df7b2a

### Updates

Add possibility to reset configuration at startup
Dashboard improvements
Fix index refresh when delays to send data != 0

## v0.3.9d8996

### Updates

- Protect dashboard and api with password
- Settings: delay between data pushes
- Fix holes/shifts in history

## v0.2.dc0c51

### Updates

- base mqtt topic is teleinfokit
- improved navigation
- hardware v2

## v0.1.4071f6

Initial release