# Guide d'utilisation TeleInfoKit

## Démarrage et configuration

Lors du premier démarrage du module il est nécessaire de saisir les informations de connexion au réseau Wifi et au serveur MQTT.

Brancher le connecteur usb à une source d'alimentation 5V (chargeur USB classique), puis attendre jusqu'à l'affichage de l'écran suivant :

```
Hotspot Wifi: TeleInfoKit
192.168.4.1
```

Un réseau Wifi portant le SSID `TeleInfoKit` a été créé. A l'aide d'un smartphone, se connecter à ce réseau Wifi. Le mot de passe du hotspot wifi est `givememylinkydata`.

**TODO** captures d'écran du portail
**TODO** procédure depuis un ordinateur

## Messages MQTT

### Structure des topics

## Ecrans

### Navigation

### Ecran #1 : Historique

### Ecran #2 : Puissance / Intensite

### Ecran #3 : Index compteurs

### Ecran #4 : Informations compteur

### Ecran #5 : Réseau

### Ecran #6 : Réinitialisation

### Ecran #7 : Eteint

## Réinitialisation des paramètres Wifi

Pour modifier ou supprimer les paramètres de connexion (Wifi et serveur MQTT), les étapes sont les suivantes :

* Naviguer jusqu'à l'écran 6 
```
Reinitialisation ?
Appui long pour reset...
```
* Effectuer un appui long
* L'écran va afficher 
```
Appui long pour confirmer
Appui court pour annuler
```
* Pour revenir en arrière et annuler la procédure, effecture un clic rapide ou attendre 10s.
* Pour confirmer la réinitialisation, effectuer un nouveau clic long.
* L'écran va afficher successivement
```
Reinitialisation en cours
```
puis
```
Redemarrage
```
Le module a supprimé ses informations de connexion et va démarrer avec sa configuration d'origine, c'est à dire sans identifiants Wifi enregistrés.

Pour enregistrer de nouveaux paramètres de connexion, voir la section [Démarrage et Configuration](#démarrage-et-configuration).