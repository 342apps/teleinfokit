# Gestion de la configuration : lecture, écriture et migration

## 1. Fichiers de configuration

- **Versions ≤ 2.0.0**  
  La configuration est stockée dans le fichier `/config.dat` selon la structure `ConfStruct_V200`.
- **Versions > 2.0.0**  
  La configuration est stockée dans le fichier `/ext_config.dat` selon la structure étendue `ConfStruct`.

## 2. Structures de configuration

- **ConfStruct_V200** (pour `/config.dat`)
  ```cpp
  typedef struct {
    bool mode_tic_standard;
    char mqtt_server[40];
    char mqtt_port[6];
    char mqtt_server_username[32];
    char mqtt_server_password[32];
    char data_transmission_period[10];
  } ConfStruct_V200;
  ```

- **ConfStruct** (pour `/ext_config.dat`)
  ```cpp
  typedef struct {
    bool mode_tic_standard;
    char mqtt_server[40];
    char mqtt_port[6];
    char mqtt_server_username[32];
    char mqtt_server_password[32];
    char data_transmission_period[10];
    // Champs ajoutés en v2.1
    bool mode_triphase;
    char version[20];
    // Champs génériques pour le futur
    bool bool_conf[10];
    char string_conf[30][40];
    int int_conf[30];
  } ConfStruct;
  ```

### Champs génériques dans la nouvelle structure

La structure `ConfStruct` introduit des champs génériques :

```cpp
bool bool_conf[10];
char string_conf[30][40];
int int_conf[30];
```

Ces tableaux permettent d’ajouter facilement de nouveaux paramètres de configuration dans les futures versions du firmware, sans modifier la structure de base ni casser la compatibilité avec les anciennes configurations.

Chaque tableau peut stocker respectivement : des booléens, des chaînes de caractères ou des entiers, utilisables pour tout nouveau besoin de configuration (options avancées, paramètres utilisateurs, extensions, etc.).

**Avantage :**  
Grâce à ces champs génériques, il est possible d’étendre la configuration sans devoir migrer à chaque fois la structure, ce qui facilite la maintenance et l’évolution du firmware tout en assurant la compatibilité ascendante.

## 3. Lecture de la configuration

La fonction `readConfig()` gère la lecture et la migration :

- **Si `/ext_config.dat` existe**  
  → Lecture directe dans la structure `ConfStruct`.

- **Sinon, si `/config.dat` existe**  
  → Lecture dans la structure `ConfStruct_V200`, puis migration :  
  Les champs communs sont copiés dans la nouvelle structure `ConfStruct`.  
  Les nouveaux champs sont initialisés à des valeurs par défaut (ex : `mode_triphase = false`, `version = V200`).

- **Sinon**  
  → Aucun fichier de configuration trouvé, la configuration par défaut est utilisée.

## 4. Migration automatique

Lorsqu’un firmware > 2.0.0 détecte un fichier `/config.dat` (ancienne version), il :
- Lit l’ancienne structure (`ConfStruct_V200`)
- Copie les valeurs dans la nouvelle structure (`ConfStruct`)
- Initialise les nouveaux champs à des valeurs par défaut
- Sauvegarde la configuration au prochain enregistrement dans `/ext_config.dat`

Ce mécanisme permet de conserver les paramètres utilisateurs lors d’une mise à jour du firmware, sans perte de configuration.

## 5. Sauvegarde de la configuration

Lors d’une modification (via le portail web ou autre), la fonction `saveParamCallback()` :
- Met à jour la structure `ConfStruct` avec les nouvelles valeurs
- Écrit la structure complète dans `/ext_config.dat` (fichier de la nouvelle version)

## 6. Résumé du flux

1. **Au démarrage**  
   → `readConfig()` tente de lire `/ext_config.dat`  
   → Sinon, tente `/config.dat` et migre si besoin

2. **À la sauvegarde**  
   → Toujours écrit dans `/ext_config.dat` (nouveau format)

3. **Après migration**  
   → L’ancien fichier `/config.dat` peut être supprimé manuellement ou lors d’une réinitialisation.

---

**Ce mécanisme garantit la compatibilité ascendante et la migration automatique de la configuration lors des mises à jour du firmware.**