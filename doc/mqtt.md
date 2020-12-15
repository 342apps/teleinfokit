# Messages MQTT

Une fois les paramétrages correctement effectués, le module va envoyer régulièrement les données de consommation vers des topics MQTT.

Le topic de base pour tous les messages envoyés par le module est `teleinfokit/`.

## Démarrage

Au démarrage du module, une fois la connexion wifi effectuée, un message de log est envoyé. Cela permet de s'assurer que le paramétrage est correct et que le module fonctionne.

Le topic et la payload sont les suivants :

|Topic|Payload|
|--|--|
|`teleinfokit/log`|`Startup`|

## Informations statiques

Après le démarrage et avoir reçu les premières trames de téléinformation du compteur, 2 messages sont envoyés avec le flag **retain = true**.

|Topic|Payload|Exemple|Trame téléinformation|
|--|--|--|--|
|`teleinfokit/adc0`|Adresse du compteur|`062769678471`|ADC0|
|`teleinfokit/isousc`|Intensité souscrite (A)|`45`|ISOUSC|

## Données de consommation temps réel

Une fois le module démarré, les informations de consommation et les index sont envoyés en temps réel. Un message n'est envoyé que si la valeur a changé.

|Topic|Payload|Exemple|Trame téléinformation|
|--|--|--|--|
|`teleinfokit/iinst`|Intensité instantanée : Courant efficace (en A)|`7`|IINST|
|`teleinfokit/papp`|Puissance apparente (en VA)|`1580`|PAPP|
|`teleinfokit/imax`|Intensité maximale appelée *|`90`|IMAX|
|`teleinfokit/hp`|Index heures pleines|`24784235`|HPHP|
|`teleinfokit/hc`|Index heures creuses|`14582447`|HCHC|
|`teleinfokit/ptec`|Période tarifaire en cours|`HC..`|PTEC|

*Note* L’intensité maximale «IMAX» est toujours égale à 90A dans le cas de ce compteur monophasé ([voir informations Enedis](https://www.enedis.fr/sites/default/files/Enedis-NOI-CPT_54E.pdf)).

## Identifiant client MQTT

L'identifiant de client MQTT utilisé par le module correspond à l'ID du chip ESP8266 du module TeleInfoKit.