# D5 NEXT — lot 8 : build complet et validation matérielle Linux

11 novembre 2026, branche `feat/d5next`, HEAD `e565fa29`, avec les changements
locaux7B/7B.1 conservés. **Build complet réussi ; première lecture matérielle
réussie ; premier SET non effectué, préconditions d'écriture non satisfaites.**
Aucun commit, push, reset USB, flashage, modification de configuration USB,
permission système ou règle udev. Aucun programme lancé avec root.

## Compilation complète et tests

Ubuntu24.04.5 LTS, Qt5.15.13. Dépendances déjà présentes :
`libusb-1.0-0-dev`1.0.27, `libhidapi-dev`0.14.0,
`libmbedtls-dev`2.28.8, `qtbase5-dev` et `qttools5-dev-tools`.
Aucune installation effectuée. Les exigences QMake du projet sont satisfaites.

```bash
mkdir -p .build/d5next-lot8/full
cd .build/d5next-lot8/full
qmake ../../../OpenRGB.pro
make -j12
```

La première compilation avec4 jobs a été interrompue pour reprendre avec12 jobs
en conservant les objets. La reprise se termine avec code0 et `linking openrgb`.
Binaire : `.build/d5next-lot8/full/openrgb`,17221192 octets. `ldd` ne signale
aucune bibliothèque manquante ; les backends libusb, hidapi-hidraw et mbedTLS
sont résolus. Aucune correction d'intégration ou de linkage D5 n'a été nécessaire.
Le binaire complet est construit avec les gardes normales et n'a pas été lancé
pour éviter un scan général des autres périphériques.

| Suite | Résultat |
|---|---|
| RGBpx ASan/UBSan | 838897 assertions PASS, sans suppression |
| Production sans macros expérimentales/offline | 162 assertions PASS, backend sentinelle interdisant GET/SET |
| Détection expérimentale simulée ASan/UBSan | 202 assertions PASS,23 injections d'échec dont16 après ouverture ; aucune fuite ni GET/SET |
| Contrôleur natif, ASan/UBSan avec baseline connue | 16765 assertions PASS |
| Contrôleur natif sanitizer strict | Échec préexistant de `RGBControllerInterface.h:287`, pointeur nul dans un `memset` de matrice vide ; distinct du code D5, inchangé |
| Diagnostic lecture seule après extension | 12418 assertions normales PASS ;12428 ASan/UBSan PASS, sans suppression |
| Ancien outil rose, syscalls simulés ASan/UBSan | 274300 /208764 assertions PASS, aucun SET réel |

La différence des comptes du diagnostic vient des wrappers fortifiés testés.
Le défaut sanitizer partagé n'a pas été masqué davantage. La construction et
les protections de production ont passé **avant tout essai matériel**.

## Identification effective et permissions

Découverte automatique par sysfs : un périphérique correspond à
**VID0C70/PIDF00E, interface HID1**, produit `D5 NEXT`, fabricant
`Aqua Computer GmbH & Co. KG`. Nœud courant `/dev/hidraw1`, interface USB
`1-7:1.1`. Numéro de série disponible, valeur volontairement omise.
Le numéro hidraw n'est pas stable après reconnexion : l'outil le résout depuis
l'identité et l'interface à chaque exécution, sans configuration manuelle.
Le chemin topologique et la série disponible permettent de suivre ce montage,
sans en faire une approbation de firmware.

Permissions effectives au moment du test : **0440, root:plugdev**, lecture
autorisée, **écriture refusée** pour l'utilisateur courant. Le descripteur HID
en cache fait70 octets et se décode correctement : Feature03=6464 bits de corps
(808 octets +ID), Input01=1288 bits (161 octets +ID).

Aucun processus connu OpenRGB/aquapx/Aquasuite/coolercontrol/liquidctl trouvé par
le diagnostic. L'inspection des descripteurs `/proc/*/fd` accessibles ne trouve
aucun handle du nœud HID ni du nœud USB correspondant. **379 répertoires fd
restent inaccessibles** : ce résultat ne prouve pas l'absence de clients cachés.
Un verrou coopératif de fichier a été acquis lors de la lecture, sans être
présenté comme une exclusion générale des autres processus.

## Essai réel en lecture seule

L'outil existant `d5next-readonly` a été étendu, compilé et testé : l'option
`--get-feature` demande maintenant, sur le même handle, un **unique Input01
passif** avant le GET. Elle vérifie le framing162/ID01 et le firmware1027 ;
timeout, transfert tronqué, mauvais ID ou firmware inconnu ferment le handle
sans GET et sans répétition. L'option de firmware du helper reste facultative
pour les anciens appelants et ne fournit aucune permission d'écriture.

Commande réellement exécutée, après le build et les protections :

```bash
.build/d5next-readonly/d5next-readonly --get-feature
```

Résultat : ouverture **O_RDONLY**, vérification de l'identité et du nœud,
verrou coopératif, Input01 reçu et firmware **1027 réellement lu** aux octets13–14,
puis **un seul GET Feature03**. Réception **809 octets**, ID03, structure BE3,
CRC reçu **4CD4**, CRC calculé **4CD4**, validation OK ; fermeture du handle.
Il ne s'agit pas d'une déduction depuis `bcdDevice` ou du seul CRC.
Master RGB255, flags strip0000. Aucun SET, Output ou SettingsSave envoyé.

Les huit groupes ont été décodés à partir de la configuration fraîche :

| Groupe | Strip | Start | Count | Mode | Observation |
|---|---:|---:|---:|---|---|
| 0 | 0 | 0 | **13** | 02 Breathing | Sortie externe, conforme à l'allocation13 documentée |
| 1 | 0 | 13 | 20 | 10 Rain2 | Configuration externe supplémentaire ; ne prouve pas20 LED physiques connectées |
| 2 | 0 | 33 | 15 | 0B Marquee2 | État lu uniquement |
| 3 | 0 | 48 | 15 | 04 Blink1 | État lu uniquement |
| 4 | 0 | 63 | 15 | 04 Blink1 | État lu uniquement |
| 5 | 0 | 78 | 12 | 04 Blink1 | État lu uniquement |
| 6 | 1 | 0 | **6** | 02 Breathing | Six positions internes, flags0000 ; HSV0 H1343/S255/V255 |
| 7 | 1 | 0 | **6** | 02 Breathing | Actif et recouvrant g6, flags0000 ; HSV0 H1279/S255/V255 |

Le CRC est identique à celui de l'état final documenté de capture11 ; **aucune
égalité des rapports complets n'en est déduite**. Les allocations et modes lus
constituent la comparaison utile avec cette référence. La priorité de rendu
entre g6/g7 reste inconnue. La lecture ne prouve pas le rendu physique ni la
présence électrique de toutes les positions externes déclarées.

Des instantanés hwmon en cache ont été lus avant et pendant le diagnostic,
uniquement `*_input` et labels. Une vitesse de pompe non nulle est présente.
Aucun contrôle PWM, offset, courbe ou seuil n'a été lu ou modifié. Ces
instantanés ne remplacent pas une surveillance continue et visuelle du circuit
pour un futur SET. Aucune valeur de télémétrie ni série brute n'est publiée ici.

## État du détecteur natif

Le détecteur D5 réel a aussi été compilé dans une sonde locale avec
`OPENRGB_D5NEXT_READ_ONLY_DETECTION`, le backend **réel hidapi-hidraw** et la
construction normale du contrôleur. La sonde limite l'énumération à0C70:F00E/IF1.
Elle réutilise uniquement le shim de registre/logs des tests pour éviter de
démarrer le gestionnaire général et les autres détecteurs OpenRGB ; le backend
HID et le contrôle `access()` ne sont pas simulés.

Résultat réel : **1 interface correspondante,0 adaptateur créé**. Le garde
`access(R_OK|W_OK)` refuse avant `hid_open_path`, car hidapi ouvre en O_RDWR.
Il s'agit d'un blocage de permissions, pas d'une absence du périphérique.
Le diagnostic séparé réussit grâce à O_RDONLY. Aucun Scan Devices de
l'application complète n'a été exécuté, et la détection reste désactivée par
défaut dans son binaire normal. Source/binaire de la sonde et logs de build :
`.build/d5next-lot8/`, hors suivi Git.

## Premier changement RGB et essai externe : arrêt avant SET

**Nombre de SET matériels :0.** Le premier test RGB n'a pas été exécuté :

1. Absence de droit d'écriture sur le nœud HID, également bloquante pour hidapi.
2. Exclusivité contre les processus inaccessibles non établie.
3. Surveillance continue et possibilité d'intervention immédiate sur le circuit
   non validées dans cette session ; les instantanés hwmon ne suffisent pas.
4. Recouvrement g6/g7 réel, priorité inconnue : préserver les octets de g7 ne
   suffit pas à prédire le rendu de g6.

Il n'existe donc pas de comparaison avant/après un SET ni de lecture de retour
après écriture. Le rapport frais n'a pas été utilisé pour fabriquer ou rejouer
un ancien état complet. Aucun outil d'écriture supplémentaire ni override des
gardes de production n'a été ajouté pour contourner ces préconditions.

Une suite possible, après résolution explicite de ces points, serait un unique
paramètre de Breathing02 déjà isolé (par exemple Speed60→49 **uniquement si une
nouvelle lecture fraîche confirme l'état de départ60**), avec audit indépendant
des offsets et CRC, surveillance et lecture de contrôle. Ce rapport ne fournit
pas un candidat d'écriture préparé à partir d'un état supposé.

Le groupe0 est identifié et ses13 positions relues. Aucun test RGB externe
n'est lancé : la politique de patch reste limitée à g6, le test interne n'a
pas eu lieu, et les recouvrements/routages ne sont pas suffisamment validés.

## Données locales et décision

Le diagnostic ne sauvegarde aucun rapport complet ni numéro de série. Les logs
privés contiennent les instantanés de télémétrie et résumés RGB, sous `.build/`
exclu de Git, créés avec umask077. Les seuls changements spécifiques au lot8
sont l'extension en lecture seule du diagnostic/helper, ses tests de syscalls
remplacés et ce rapport ; les changements7B/7B.1 préexistants sont conservés.

**GO lecture seule Linux confirmé :** build complet, identification réelle,
firmware1027, lecture809/structure3/CRC et huit groupes décodés, sans root ni
modification du refroidissement. **Arrêt avant écriture RGB.** Les droits udev
adaptés, l'exclusion des concurrents, la surveillance et le traitement des
recouvrements doivent être validés avant de reprendre le premier SET. Aucune
permission n'a été modifiée automatiquement.

## Lot 8B : validation après correction udev

11 novembre 2026. L'utilisateur a corrigé les permissions avant ce lot ;
aucune règle ni permission n'a été modifiée par les sondes. Identité relue :
**0C70:F00E, interface HID1**, nœud courant `/dev/hidraw1`, topologie
`1-7:1.1`. Permissions effectives **0660 root:plugdev**, utilisateur membre de
plugdev ; les vérifications d'accès lecture **et** écriture réussissent.
L'ancien blocage de permissions du lot8 est donc levé.

### Sonde native HIDAPI réelle

La même sonde `.build/d5next-lot8/native-discovery`, compilée avec
`OPENRGB_D5NEXT_READ_ONLY_DETECTION` et liée à `libhidapi-hidraw.so.0`, a été
exécutée sur la seule identité D5 NEXT. Comme au lot8, le shim porte uniquement
sur le registre et les logs du gestionnaire ; le détecteur, le contrôleur,
l'adaptateur et le backend HID sont réels. Aucun scan général d'OpenRGB lancé.

Résultat : **1 interface correspondante,1 adaptateur créé** :
`Aqua Computer D5 NEXT`, mode **Read only**. L'adaptateur est détruit avant la
libération de l'énumération HID et la fin du programme.

Une trace `strace -f` locale, avec arguments `ioctl/read/write` en forme brute
pour éviter de journaliser les buffers, confirme :

| Opération sur le nœud HID pendant la découverte | Résultat |
|---|---|
| Ouverture | Une seule, `O_RDWR|O_CLOEXEC`, réussie |
| ioctl | Un seul : `0x80044801`, `HIDIOCGRDESCSIZE`, taille du descripteur en cache noyau |
| GET Feature / SET Feature | **0 /0** |
| `read` / `write` sur le handle HID | **0 /0** |
| Fermeture | Une seule, `close(.../dev/hidraw1)=0` |

`HIDIOCGRDESCSIZE` est une interrogation de métadonnées hidraw, pas un GET_REPORT
USB. L'ouverture O_RDWR exigée par hidapi n'a déclenché aucune configuration,
écriture Output, sauvegarde ni lecture Feature. La création/destruction normale
de l'adaptateur réel et la fermeture du handle sont ainsi constatées, sans
déduire ces résultats de la seule existence d'un destructeur C++.

### Lecture réelle après fermeture de la découverte

Le diagnostic existant a ensuite été exécuté une seule fois avec
`--get-feature`, également sous trace locale. Il ouvre un **autre handle
O_RDONLY**, acquiert son verrou coopératif, vérifie l'identité, reçoit un
**Input01 passif de162 octets**, puis effectue **un GET Feature03**.

- Firmware **1027** réellement lu dans Input01.
- Feature03 : **809 octets, ID03, structure3**, validation OK.
- CRC reçu **0x4CD4**, calculé **0x4CD4**.
- Master255, flags strip0000.
- g0 : strip0,start0,count**13**,mode02 Breathing,flags0000,HSV0 H1279/S255/V255.
- g6 : strip1,start0,count**6**,mode02 Breathing,flags0000,HSV0 H1343/S255/V255.
- g7 : strip1,start0,count**6**,mode02 Breathing,flags0000,HSV0 H1279/S255/V255.

Les allocations et ces champs RGB concordent avec la lecture du lot8. Leur CRC
commun ne constitue pas une preuve d'identité des rapports complets. Le groupe7
est toujours actif et recouvre g6 ; aucune priorité de rendu n'est déduite.

La trace du handle de diagnostic contient un ioctl d'identité
`0x80084803` (`HIDIOCGRAWINFO`), un seul ioctl Feature GET `0xC3294807`, un seul
`read` Input01 et une fermeture réussie. **Aucun SET Feature, aucune écriture
sur le handle HID ni Output02/SettingsSave**, aucun reset ou changement USB.
Les deux programmes se terminent avec code0 et leurs handles sont fermés.

### Concurrence, données locales et conditions du premier SET

Les processus connus recherchés par le diagnostic ne sont pas présents.
L'inspection des fd accessibles ne trouve aucun handle concurrent du nœud HID
ni du nœud USB, avant les sondes ; une nouvelle inspection après leur sortie
ne trouve aucun handle résiduel accessible. **380 répertoires de fd de processus
étaient inaccessibles lors du contrôle initial**. L'absence de clients visibles
et le verrou coopératif ne prouvent donc pas l'exclusivité générale, notamment
contre un service non visible, un accès vendor/libusb ou une autre activité de
configuration par le pilote noyau.

Les traces et logs restent privés dans `.build/d5next-lot8b/`, exclu de Git ;
les buffers USB, la série brute et les valeurs de télémétrie ne sont pas ajoutés
au dépôt. Aucun code ni garde de production modifié pour ce lot, aucune
installation, aucun lancement en root, commit ou push. Les suites simulées et
le build du lot8 servent de références ; aucune reconstruction sans changement
de source n'a été nécessaire.

**GO découverte HIDAPI et lecture seule confirmé. NO-GO écriture pour8B :
nombre total de SET de ce lot=0.** Les permissions ne bloquent plus un futur
essai. Avant un unique SET RGB limité à g6, il reste à réunir :

1. Une **autorisation distincte d'écriture** et un moyen de récupération
   Aquasuite à nouveau vérifié ; ne pas présumer l'absence de persistance de03
   sans SettingsSave.
2. Un niveau de confiance suffisant sur l'exclusion des clients HID/USB et
   activités de configuration concurrents, au-delà des seuls fd accessibles
   et du verrou coopératif.
3. Une surveillance effective du refroidissement pendant l'opération : vitesse
   de pompe et température pertinentes, confirmation visuelle du circuit,
   opérateur en mesure d'interrompre immédiatement. Les instantanés du diagnostic
   ne remplacent pas cette surveillance.
4. Une décision explicite sur le recouvrement g6/g7, sans désactiver ou modifier
   automatiquement g7. La préservation de ses octets n'établit pas que le rendu
   du changement g6 sera visible sur les six LED.
5. Un outil séparé, désactivé par défaut, gardant une transaction sérialisée et
   lisant un **nouveau rapport frais** immédiatement avant l'opération. Identité,
   firmware, structure, CRC, allocation et mode doivent être revérifiés.
6. Une demande documentée sans transition de mode, avec audit indépendant de
   tous les809 octets. Exemple conditionnel : Breathing02 Speed60→49 seulement
   si la lecture fraîche confirme60 ; seuls les octets682–683 et807–808 seraient
   autorisés à différer. Tous les autres champs, y compris hydrauliques, sources,
   bindings, autres groupes et données opaques, doivent rester identiques.
7. Au maximum un SET, sans Output/SettingsSave, puis une lecture de contrôle si
   prise en charge ; divergence ou anomalie implique arrêt, sans répétition ni
   restauration aveugle d'un ancien rapport complet.

Ces conditions sont celles d'un futur essai. Elles ne sont pas converties en
override des gardes normales ni en préparation d'une écriture dans ce lot.

## Lot 8C : préparation du premier SET interne

État du premier essai : **dry-run matériel réussi, SET unique expressément
autorisé et réalisé, lecture de contrôle exacte ; circuit normal confirmé par
l'opérateur, différence de vitesse lumineuse non établie visuellement**.
Référence de métadonnées : [catalogue7B](D5NextLot7AdvancedCatalog.md).

### Outil séparé et barrières

`tools/AquaComputerD5Next/compile-speed.sh` compile un exécutable dédié
`.build/d5next-speed/d5next-speed-test`. Il n'est ni lié à OpenRGB normal ni
conditionné par une macro de simulation. Sans option, il est en dry-run.
`--dry-run` est explicite ; l'envoi exige une autre invocation avec
`--send-once`, un jeton d'approbation issu du dry-run, `--operator-confirmed`
et deux limites de refroidissement choisies par l'opérateur.

L'outil découvre la seule identité0C70:F00E/HID1, exige une série non vide,
compare la série sysfs à `HIDIOCGRAWUNIQ` sur le handle ouvert, revérifie
l'interface et l'inode du nœud, le descripteur, Input01/firmware1027, puis lit
Feature03 frais. Le dry-run ouvre O_RDONLY ; seul le mode d'envoi explicitement
autorisé peut ouvrir O_RDWR. Les programmes root sont refusés.

La campagne exige les allocations observées des huit groupes : g0 start0/count13,
g1 13/20,g2 33/15,g3 48/15,g4 63/15,g5 78/12 sur strip0 ; g6/g7 start0/count6
sur strip1 et tous deux en mode02. Le Speed initial de g6 doit être60. Les
valeurs ne sont pas généralisées en politique d'allocation pour OpenRGB.

`D5NextSpeedTest::Plan` demande un patch Breathing.Speed49 au codec existant,
puis construit **indépendamment** un buffer attendu : copie intégrale du rapport
frais, octets682–683=`0031`, CRC recalculé par une formulation bit à bit distincte
de CRC++. La comparaison porte sur809 octets ; tout écart hors de682–683/807–808
est refusé. Aucune transition, allocation, valeur master, donnée opaque, source,
paramètre hydraulique ou octet de g7 ne peut être modifié par ce plan.

Le reçu privé du dry-run lie la série, le SHA-256 du rapport complet et un jeton
aléatoire. Il ne contient **aucun rapport complet**. L'envoi doit retrouver la
même série et l'empreinte du **nouveau** rapport lu : il ne réutilise pas un ancien
buffer. Un marqueur de tentative est créé exclusivement et synchronisé avant
l'exécution autorisée ; il empêche les nouvelles tentatives automatiques, même
après une erreur. Un verrou de campagne et un flock du handle empêchent les
instances coopérantes de cette sonde de se chevaucher.

Un seul appel `HIDIOCSFEATURE(809)` existe dans le transport dédié, lui-même
limité à une invocation. Aucune fonction Output02, SettingsSave, reset ou
restauration n'est liée à cet outil. Une erreur SET ou lecture de contrôle
provoque un arrêt sans répétition ; une divergence de809 octets après SET
est signalée par ses offsets, sans publication de données opaques.

### Surveillance et limites

Les labels `Pump speed` et `Coolant temp` doivent être disponibles sous hwmon.
Seuls leurs `*_input` en cache sont lus. Le mode d'envoi surveille à250 ms,
avec5 secondes d'observation avant la transaction et10 secondes après une
lecture de contrôle exacte, sans nouvelles commandes HID durant ces fenêtres.
Une alarme, une perte de capteur ou SIGINT/SIGTERM interdit les commandes
suivantes. Le PID est affiché pour l'interruption ; **un ioctl déjà soumis ne
peut pas être rappelé**. L'opérateur reste responsable de l'observation du
circuit et de l'intervention prévue en cas d'anomalie.

Les processus connus et les handles accessibles des nœuds HID **et vendor USB**
sont contrôlés avant ouverture, avant la lecture et avant le SET. Aucun client
accessible n'a été trouvé pendant le dry-run ;380 répertoires fd restent
inaccessibles. Ce contrôle et les locks coopératifs ne constituent pas une
preuve d'exclusivité générale. La confirmation de l'arrêt des autres services
et de la récupération Aquasuite reste nécessaire.

### Résultat du dry-run réel

Commande exécutée :

```bash
bash tools/AquaComputerD5Next/compile-speed.sh
.build/d5next-speed/d5next-speed-test --dry-run
```

Série comparée sans affichage ; firmware1027 lu dans Input01. Feature03 frais
ID03/809/structure3 valide, CRC reçu et calculé **4CD4**. Allocations contrôlées,
g6 Breathing02, Speed **60**, g7 actif et recouvrant : le changement g6 peut
être masqué visuellement, sa priorité n'est pas établie.

| Offset | Avant | Candidat | Rôle |
|---|---|---|---|
| 682 | 00 | 00 | Octet haut Speed, inchangé |
| **683** | **3C** | **31** | Speed60→49 |
| **807** | **4C** | **F3** | CRC haut |
| **808** | **D4** | **3A** | CRC bas |

**Trois octets changeraient ;806 resteraient identiques.** Le CRC candidat
calculé est **F33A**. La trace locale confirme **un GET Feature et zéro SET** ;
le handle est fermé à la fin. Les instantanés en cache avant/après ne montrent
pas d'alarme ; leurs valeurs restent privées. Le jeton et le reçu restent sous
`.build/d5next-lot8c/`, hors Git, droits privés. Le reçu ne vaut pas accord de
l'opérateur et la tentative d'envoi n'a pas été consommée.

Tests hors ligne : `tests/AquaComputerD5Next/run-speed.sh --sanitize`,
**25971 assertions PASS ASan/UBSan**, transport synthétique sans backend HID.
Ils couvrent le hash SHA-256 connu,16 buffers opaques synthétiques, les809 octets,
CRC indépendant, tailles, allocations, mode/Speed incorrects, sorties intactes,
dry-run, digest périmé, alarmes avant/après SET, transferts courts/incertains et
lecture de retour différente. Au maximum un SET simulé, sans répétition.

Après examen du résultat, l'opérateur a expressément autorisé le SET unique
sous réserve des préconditions et confirmé les logiciels arrêtés, le circuit
normal, la surveillance et l'intervention immédiate, la récupération Aquasuite
et l'acceptation du masquage g7. Température maximale choisie : **40 °C**.
Le seuil minimal de pompe a ensuite été fourni : **3000 tr/min**. L'opérateur
a précisé vouloir toucher uniquement au RGBpx ; il lui a été confirmé que
ces deux limites ne sont que des conditions d'arrêt locales à la sonde, et
ne modifient ni consigne de pompe ni protection thermique du périphérique.

### Exécution matérielle unique et lecture de contrôle

Après ces confirmations, invocation distincte avec le reçu privé :

```bash
.build/d5next-speed/d5next-speed-test --send-once \
  --approve <jeton-prive-du-dry-run> --operator-confirmed \
  --min-pump-rpm 3000 --max-coolant-mc 40000
```

L'identité et la série ont été revérifiées sur le handle, le firmware1027 lu
dans Input01. Aucun client concurrent connu ou handle accessible n'a été
détecté ;380 répertoires fd restent inaccessibles, limite expressément signalée
à l'opérateur avant son accord. Les cinq secondes de surveillance initiale ont
passé. Une **nouvelle lecture fraîche** a produit le même SHA-256 que le reçu
du dry-run ; le plan et l'audit indépendant ont été recalculés depuis ce rapport.

**Un SET Feature03 a été envoyé et un seul**, complété avec809 octets.
La nouvelle lecture de contrôle a validé ID03/809/structure3/CRC et a été
comparée **sur les809 octets** au candidat exact : correspondance intégrale.

| Champ | Avant, lecture fraîche | Après, lecture de contrôle |
|---|---|---|
| Mode g6 | 02 Breathing | 02 Breathing, inchangé |
| Speed g6,682–683 | `00 3C` (60) | `00 31` (49) |
| CRC807–808 | `4C D4` | `F3 3A` |
| Tous les autres octets | Référence fraîche | Identiques,806 octets |

Les seules différences sont donc **683,807,808**. G7, g0, master, allocations,
sources, bindings, paramètres hydrauliques/thermiques et champs opaques restent
identiques dans les rapports avant/après. Aucune transition ni restauration.

La trace locale de l'invocation confirme une ouverture HID O_RDWR, **2 GET
Feature** (frais et contrôle), **1 SET Feature** (`0xC3294806`,retour`0x329`),
une fermeture réussie, **0 write sur le handle HID**. Aucun Output02/SettingsSave,
reset ou autre écriture matérielle. Les traces utilisent des arguments bruts
pour ne pas extraire les buffers ; elles ne sont pas publiées dans Git.

La surveillance en cache a continué pendant les dix secondes après lecture de
contrôle, sans commande HID supplémentaire. Aucun seuil franchi, capteur perdu
ou signal d'interruption ; régime de pompe au-dessus de3000 et température
sous40 °C avant et après. Les valeurs mesurées restent dans le log privé.
Le programme termine avec code0, handle fermé, et **marqueur de tentative
consommé** : aucune autre exécution d'envoi ni répétition n'est permise par
cette campagne. Les commandes matérielles sont arrêtées.

L'opérateur confirme ensuite le fonctionnement normal du circuit. Il indique
que les éclairages sont actifs et demande une couleur bleue pour distinguer
visuellement le groupe ; aucune différence visuelle de vitesse n'est donc
comptée comme confirmée. La persistance de03 sans SettingsSave n'est pas testée.
Aucune sauvegarde ni restauration automatique ne découle de ce succès.

## Essais supplémentaires explicitement autorisés : teintes g6, g0, g7

Chaque ligne ci-dessous a fait l'objet d'une demande distincte, d'un nouveau
dry-run réel, de la présentation du différentiel puis d'un accord explicite
après examen. Chacune utilise son propre reçu lié à la série et au SHA-256 du
rapport frais, et son marqueur consommable. **Un seul SET par essai**, aucune
répétition, aucun SettingsSave. La surveillance locale reste3000 tr/min/40 °C ;
aucun de ces seuils n'est enregistré comme consigne de pompe ou protection.

L'outil dédié propose `--blue`, `--external-blue` ou `--group7-yellow`,
mutuellement exclusifs. Les champs ciblés et les états initiaux sont stricts ;
les autres805 ou806 octets doivent être identiques au rapport frais. Le
réencodage du seul groupe70 octets est confronté à une copie attendue construite
indépendamment, où seuls les deux octets de teinte et le CRC peuvent différer.
Pour ces teintes, l'outil utilise le layout/documentation des captures et un
plan expérimental explicite, **sans étendre la politique normale `BuildPatch`**
ni les zones ou modes pilotables de l'adaptateur OpenRGB. G0 etg7 restent
refusés par cette politique de production.

### G6 : passage au bleu, confirmé visuellement

Mode02 Breathing, Speed49 après l'essai initial, HSV0 initial H1343/S255/V255.
Le dry-run, puis une nouvelle lecture fraîche identique, donnent :

| Offsets modifiés | Avant | Après accepté |
|---|---|---|
| 706–707, HSV0.H g6 | `05 3F` (1343) | `04 00` (1024, bleu #0000FF) |
| 807–808, CRC | `F3 3A` | `9A A7` |

Quatre octets changent,805 sont identiques. Mode, Speed49, S/V255, master,
allocations, g0/g7 et champs hydrauliques/opaques préservés. Un SET complété809,
lecture de contrôle exacte809, code0 ; surveillance initiale5 s et finale10 s
sans alarme, handle fermé. L'opérateur confirme : **les LED de la pompe sont
bleues**. G6 a donc une influence visible dans cette configuration avec g7 actif,
sans preuve d'une règle générale de priorité.

### G0 : les13 LED externes au bleu, confirmé visuellement

Après clarification, l'opérateur désigne les13 LED externes, et non g7.
G0 est en Breathing02, strip0,start0,count13 ; g1 commence à13, donc aucun
recouvrement g0/g1 dans l'allocation courante. HSV0 initial H1279/S255/V255.
La capture11 établit déjà une variation isolée de cette teinte sur g0.

| Offset modifié | Avant | Après accepté |
|---|---|---|
| 287, HSV0.H bas g0 | `FF` | `00` ; H286–287=`0400`, bleu #0000FF |
| 807–808, CRC | `9A A7` | `D9 93` |

Trois octets changent,806 sont identiques. G6 bleu et g7 restent inchangés,
ainsi que le mode, toutes les allocations et autres paramètres. Un SET
complété809, lecture de contrôle exacte809, code0 et surveillance sans alarme.
L'opérateur confirme que cet éclairage est **également correct**. Les deux
passages au bleu sont donc validés visuellement, séparément.

### G7 : test jaune, accepté mais sans effet visible

À la demande de l'opérateur, le projet de chenillard est mis en attente pour
tester g7. G7 est en Breathing02, strip1,start0,count6, comme g6 ; il recouvre
toujours les six positions internes. Son HSV0 initial est H1279/S255/V255.
Ce test est **exploratoire** : le layout70 octets identifie son champ de teinte,
mais aucune édition visible de g7 n'avait été confirmée par les captures.
L'autorisation ne supposait pas de rôle physique connu pour ce groupe.

| Offsets modifiés | Avant | Après accepté |
|---|---|---|
| 776–777, HSV0.H g7 | `04 FF` (1279) | `01 00` (256, jaune #FFFF00) |
| 807–808, CRC | `D9 93` | `2B FD` |

Quatre octets changent,805 sont identiques. Les teintes bleues de g0/g6, leurs
vitesses et les autres paramètres restent intacts. Un SET complété809,
lecture de contrôle exacte809, code0 ; surveillance sans alarme et handle fermé.
Réponse opérateur : **aucun changement visible**. Le jaune est accepté dans
la configuration ; son rendu n'est pas observé. Cela ne prouve ni que g7 soit
toujours inutilisé, ni une priorité universelle de g6, ni une seconde zone
physique. Aucun test supplémentaire, désactivation de groupe ou changement
d'allocation n'est déclenché pour forcer une conclusion.

### Bilan technique de ces essais

Les traces locales de chaque envoi montrent2 GET Feature (frais et contrôle),
1 SET Feature, zéro `write` sur le handle HID et une fermeture réussie. Aucun
Output02, reset ou sauvegarde. Chaque dry-run avait1 GET et0 SET. Les traces,
reçus, jetons et mesures restent sous `.build/`, privés et exclus de Git.
Les marqueurs des quatre campagnes sont consommés : Speed g6, bleu g6, bleu g0,
jaune g7. **Total des essais matériels autorisés effectués :4 SET distincts**,
sans aucune boucle RGB. Aucun autre SET n'est effectué sans nouvelle demande,
préparation et autorisation correspondantes.

Les tests de l'outil dédié sont étendus aux quatre actions : **93351 assertions
PASS ASan/UBSan** sur transport synthétique, incluant les états de départ,
sorties intactes, préservation complète, CRC indépendant, erreurs de transfert,
alarmes et lecture de contrôle divergente. Ces capacités expérimentales restent
hors de l'application normale ; aucune activation de découverte par défaut.

État matériel final relu : g0 bleu, g6 bleu, g7 configuré jaune sans effet visible,
g6 Speed49, modes Breathing02 conservés, CRC03 **2BFD**. Aucun réglage hydraulique,
ventilateur, source de commande, protection thermique ou master modifié.

## Chenillard demandé : un point rose unique sur19 LED

L'opérateur souhaite un seul point rose sur fond bleu parcourant successivement
les six LED de la pompe et les13 externes, et autorise les effets fabricant.
Il confirme préférer cette continuité à deux chenillards indépendants.
**Demande mise en attente pendant le test g7 ; aucune configuration de
chenillard n'a été envoyée.**

Les captures attribuent les valeurs et flags de Sequence07 et Scanner08,
mais pas tous les rôles de palette, le réglage correspondant à exactement
une LED ni une synchronisation entre strip1 etstrip0. Les transitions
initialisent plusieurs champs conjointement. La présence d'un ID d'effet ou
d'un curseur Speed ne suffit pas à fabriquer un préréglage rose/bleu valide
et une trajectoire continue de19 positions.

Le flux AMBIENTpx observé sur l'interface vendor0 décrit six mots `00 G R B`,
mais les deux06 d'en-tête et le routage général entre groupes ne sont pas
résolus. La capture11 avec13 LED ne contient aucun Bulk et ne complète pas
cette preuve. Aucun encodeur actif ou flux construit par hypothèse n'est ajouté.

Pour poursuivre avec une preuve ciblée :

1. Vérifier dans Aquasuite si un effet fabricant peut effectivement produire
   un point unique continu entre les deux sorties, puis capturer son choix,
   son fond bleu, sa couleur rose et son éventuel mécanisme de synchronisation,
   chaque réglage séparément. Si la fonction n'existe pas dans l'UI, ne pas
   assimiler deux effets autonomes à cette trajectoire.
2. Si un flux PC est nécessaire, capturer AMBIENTpx d'abord sur g0 seul avec
   count13 et une mire synthétique, puis sur g6 seul avec count6, puis les deux
   activés séparément. Comparer en-têtes, nombre et routage des mots pour
   départager les deux06, sans toucher aux paramètres du circuit.
3. Construire et tester des fixtures hors ligne de ce transport avant toute
   nouvelle émission. Une éventuelle reprise active nécessitera son propre
   dry-run, audit et autorisation ; les reçus de teinte ne peuvent pas servir
   pour l'animation.

## Consolidation Git avant GUI

11 novembre 2026 : nouvelle vérification exclusivement hors matériel avant
indexation et publication sur le fork `Pepeuch/OpenRGB`. L'inventaire réel
contient20 fichiers légitimes :8 pour le catalogue, les tests et rapports7A–7B.1 ;
12 pour les diagnostics, tests expérimentaux et le présent rapport8–8C et suites.
Les scripts de compilation seuls ont aussi été exécutés, jamais leurs outils
matériels. Aucun raccordement GUI n'est commencé.

| Vérification rejouée | Résultat |
|---|---|
| Catalogue/RGBpx normal et ASan/UBSan | 838897 assertions PASS, dont608 sur le catalogue C++ lié |
| Contrôleur natif normal et sanitizer avec baseline connue | 16765 assertions PASS |
| Gardes de production | 162 assertions PASS, appels HID interdits par les sentinelles |
| Détection simulée normale et ASan/UBSan | 202 assertions PASS ;23 échecs d'allocation injectés |
| Diagnostic lecture seule normal /ASan/UBSan | 12418 /12428 assertions PASS, syscalls remplacés |
| Outil expérimental, quatre actions, normal et ASan/UBSan | 93351 assertions PASS, transport synthétique |
| Ancien outil rose normal /ASan/UBSan | Production274269 /274300, identité synthétique208733 /208764 assertions PASS |
| Build complet | QMake puis `make -j12` réussis dans `.build/d5next-lot8/full`, aucun objet périmé, application non lancée |

Le sanitizer natif strict a également été rejoué et quitte avec code1 sur le
défaut préexistant `RGBControllerInterface.h:287`. La suppression connue et
l'API partagée restent inchangées ; ce défaut n'est pas présenté comme corrigé.
Les nombres sont des assertions, pas des cas indépendants ni une couverture
mesurée. Les logs de consolidation restent sous `.build/`.

La découverte normale est toujours désactivée par défaut et l'adaptateur reste
Read only. Les outils d'envoi restent séparés, en dry-run par défaut, sans lien
depuis l'application normale. Les reçus, séries, jetons, empreintes privées de
configuration, traces et journaux matériels sont exclus des fichiers publiés.
