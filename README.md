# Teleinfokit

An ESP01 based module that reads data from french energy meters and send them to a MQTT server. 

As this is mainly intended for french people, the rest of this file will be in french, as well as all the documentation.

## Description

Le firmware proposé ici est prévu pour fonctionner sur le module TeleInfoKit qui permet de collecter les données de télé-information émises par un compteur électrique de type Linky ou non-communicant. Les données sont disponibles via l'écran du module et depuis un dashboard accessible depuis un navigateur internet.

Les données sont également envoyées en temps réel sur un serveur MQTT pour être exploitées ensuite par un système domotique tel [Home Assistant](https://www.home-assistant.io/) ou un logiciel de monitoring comme [Grafana](https://www.home-assistant.io/).

Un [guide d'utilisation détaillé](./doc/user-guide.md) du module et son firmware est disponible dans les sources du projet.

## Envoi des données en temps réel vers un serveur MQTT

Les données collectées depuis le bus de téléinformation du compteur (Linky ou anciens modèles) sont envoyées en temps réel sur un serveur MQTT (si configuré). La structure des messages et les topics utilisés sont décrits dans la [documentation mqtt](./doc/mqtt.md).

## Affichage intégré au module TeleInfoKit

Un afficheur intégré au module TeleInfoKit restitue un historique de consommation sur 24h, la consommation instantanée et d'autre informations décrites dans la documentation.

## Dashboard web

Un dashboard sous la forme d'une page web est également accessible depuis un navigateur en se connectant au module via son adresse IP.

**TODO Image**

Un graphe de la consommation instantanée et l'historique sur 24 est affiché. Des informations complémentaires sont disponibles au bas du dashboard.

## APIs

Une série d'APIs est mise à disposition pour accéder aux données autrement que par MQTT. Le dashboard utilise ces APIs dans son fonctionnement. Les [spécifications au format OpenAPI](./doc/TeleInfoKit-data.v1.yaml) sont disponibles dans les sources.

## Mises à jour OTA

Le firmware peut être mis à jour en mode OTA (Over The Air). Cela passe par la connexion wifi, sans besoin de matériel supplémentaire pour flasher le firmware.

## Hardware

Le module TeleInfoKit s'architecture autour du module ESP-01 basé sur le chip [ESP8266 d'Espressif](https://www.espressif.com/en/products/socs/esp8266). Le firmware disponible ici est prévu pour être compilé pour cette plateforme.
