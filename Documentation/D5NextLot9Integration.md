# D5 NEXT — Lot 9A : intégration native et GUI en lecture seule

Date : 11 novembre 2026. Branche `feat/d5next`, base
`d9d8775e65b85a7478a2b3e35ed1e5f3e20ebc6d`, arbre propre au départ.
Aucun commit, push, paquet Debian ou changement matériel d'éclairage dans ce lot.
Les preuves des lots 7/8 sont réutilisées, sans nouvelle analyse du protocole.

## Résultat

**Compilation complète réussie et GUI réel testé en lecture seule.** Une seule
D5 NEXT est affichée, firmware **1027**, avec les zones **Pump RGB : 6 LED** et
**External RGBpx : 13 LED** pour la configuration actuellement connectée.
Le bouton **Rescan Devices** reconstruit un seul adaptateur avec ces deux zones.
Un profil OpenRGB a été enregistré localement puis chargé dans l'application.
Aucun SET Feature03, Output, SettingsSave ou reset n'a été envoyé.

Le seul mode publié reste **Read only**. Le catalogue descriptif de 23 effets
n'est pas transformé en modes GUI. Les callbacks restent inertes. La découverte
reste absente des builds par défaut et nécessite une option de compilation.

## Modifications exactes

| Fichier | Modification |
|---|---|
| `Controllers/AquaComputerD5NextController/AquaComputerD5NextController.cpp` | Lecture explicite unique Input01/Feature03 ; validation d'identité et de géométrie ; instantané RGB seulement ; getters sans I/O |
| `Controllers/AquaComputerD5NextController/AquaComputerD5NextController.h` | `ReadConfiguration()`, `GetConfiguration()`, `GetFirmware()` et état de lecture distinct de l'autorisation d'écriture |
| `Controllers/AquaComputerD5NextController/D5NextOwnership.h` | Comparaison interne de série, sans getter publiant sa valeur |
| `Controllers/AquaComputerD5NextController/AquaComputerD5NextControllerDetect.cpp` | Lecture avant création de l'adaptateur ; refus et fermeture RAII si validation impossible |
| `Controllers/AquaComputerD5NextController/RGBController_AquaComputerD5Next.cpp` | Zones issues de l'instantané, version réellement lue, indication du recouvrement g7 ; callbacks toujours inertes |
| `OpenRGB.pro` | Option explicite `CONFIG+=d5next_readonly` définissant `OPENRGB_D5NEXT_READ_ONLY_DETECTION` |
| `tests/AquaComputerD5Next/D5NextTests.cpp` | Fixtures de géométrie cohérente et backend Input/série simulé ; attentes de scan/adapter adaptées aux nouvelles lectures |
| `tests/AquaComputerD5Next/DetectionTests.cpp` | Tests du détecteur réellement compilé avec la macro expérimentale, API des zones, JSON de profil, erreurs, concurrence, refus d'écriture après lecture valide et échecs d'allocation |
| `tests/AquaComputerD5Next/ProductionTests.cpp` | Sentinelles interdisant aussi Input/série ; aucune zone inventée pour un contrôleur non initialisé |
| `Documentation/D5NextLot9Integration.md` | Présent rapport et conception du Lot 9B |

Le codec, le catalogue, les transitions et les politiques de patch existantes
ne sont pas modifiés. Aucun fichier `aquapx-main/` n'est modifié.

## Détection et lecture

L'enregistrement utilise toujours `REGISTER_HID_DETECTOR_I`, **0C70:F00E,
interface 1**, sous la macro expérimentale ou dans la cible simulée. Le retour
`DetectedControllers` respecte le DetectionManager actuel. La documentation API
mentionne encore ResourceManager pour cet enregistrement ; l'implémentation
suit les conventions effectives du dépôt.

Séquence, sur un même handle :

1. Vérifier les informations d'énumération, le chemin non vide et, sous Linux,
   les permissions effectives `R_OK | W_OK` nécessaires au backend hidraw HIDAPI.
2. Réserver chemin et série avec `D5NextOwnership`, avant l'ouverture. Un chemin
   ou une série déjà réservé empêche une deuxième ouverture dans ce processus.
3. Ouvrir par le chemin fourni par HIDAPI. Aucun chemin hidraw n'est configuré
   manuellement. Le numéro hidraw peut changer entre deux connexions.
4. Appeler `ReadConfiguration()` sous le mutex transactionnel : vérifier la
   série retournée par HIDAPI contre celle de l'énumération, sans l'afficher.
5. Recevoir un Input01 passif, délai maximal 2 s, exactement **162 octets**, ID1.
   Le buffer de 163 octets permet de refuser un Input plus long. Lire le firmware
   BE aux offsets13–14 et exiger **1027 avant tout GET Feature03**.
6. Effectuer un seul GET Feature03 et exiger **809 octets**, ID3, structure BE3,
   CRC USB valide et identité acceptée par le codec existant.
7. Valider les huit allocations, puis conserver uniquement la configuration
   RGB décodée. Le rapport complet incluant l'hydraulique n'est pas conservé
   dans l'adaptateur.
8. Créer le RGBController. Tout échec refuse proprement la découverte et ferme
   le handle par RAII. Il n'y a pas de répétition automatique après erreur.

La lecture est tentée une seule fois par contrôleur. Les getters retournent
l'instantané sans nouvelle lecture. Un nouveau scan détruit l'ancien contrôleur
avant de réserver et lire le périphérique à nouveau. Aucun polling n'est ajouté.
L'absence de série, un firmware différent ou une géométrie non reconnue
entraînent un refus, même si le CRC est correct. `release_number` n'est jamais
une preuve de firmware ou une autorisation.

**Une lecture valide renseigne `layout` et `firmware`, tout en laissant
`write_authorized=false`.** Le chemin de lecture révoque même l'autorisation de
la construction spéciale des tests hors ligne. Les constructions normales
n'ont toujours aucun moyen public d'accorder cette autorisation. Les setters
existants refusent donc avant tout GET/SET, y compris après une lecture valide.
Ni un profil ni un JSON `write_authorized=true` ne changent cette propriété.

### Politique d'allocation de cette version expérimentale

Pour le firmware1027/structure3 uniquement : g0–g5 doivent appartenir au strip0
et rester dans sa capacité documentée de90 positions ; g6/g7 au strip1 dans
ses six positions. Les vérifications utilisent une soustraction après contrôle
du start pour éviter un dépassement de borne. Un effet actif de longueur nulle
ou un ID de mode inconnu est refusé. `00 Unused` reste une entrée distincte de
Off ; `06` reste inconnu.

Les recouvrements entre groupes externes actifs sont refusés. g6 doit couvrir
exactement strip1/start0/count6. Un g7 actif doit couvrir ce même intervalle ;
ce recouvrement connu est accepté pour la lecture uniquement et signalé dans
la description. Une allocation interne partielle différente est refusée.
Cette politique conservative peut refuser des montages pourtant légitimes ;
elle ne prétend pas décrire tous les firmwares ou routages possibles.

## Conformité à RGBControllerAPI.md

Référence locale : [RGBController API](RGBControllerAPI.md), particulièrement
Zones, Modes, Color Mode Values et Device Update Functions. Les structures de
`RGBControllerInterface.h` et les implémentations multizones du dépôt ont
également servi de référence.

| Contrat API | Application dans ce lot |
|---|---|
| Métadonnées | Nom/vendor en anglais, `DEVICE_TYPE_COOLER`, version1027 après lecture ; location HID obtenue automatiquement |
| LEDs contiguës par zone | Pump aux indices0–5 ; External aux indices6–18 pour cette configuration ; `SetupColors()` établit les pointeurs et start indices |
| Tailles fixes | `leds_min = leds_max = leds_count` ; aucun flag de redimensionnement, type, segments ou configuration spécifique modifiable |
| Géométrie | `ZONE_TYPE_LINEAR`, pas de matrice ni de segments inventés ; aucune promesse sur l'ordre physique ou la continuité des19 LED |
| Modes | Une entrée `Read only`, index actif0 ; flags0, `MODE_COLORS_NONE`, aucun paramètre speed/brightness/color ni sauvegarde annoncé |
| Modes par zone | Aucun mode par zone ; index actif de zone -1, `SupportsPerZoneModes()==false` |
| Couleurs | Vecteur de19 couleurs pour respecter la géométrie API ; ce sont des valeurs en RAM, pas une lecture des couleurs instantanées de l'animation |
| Callbacks/profils | Mise à jour de couleur, LED, zone, mode et sauvegarde sans I/O ; configuration JSON héritée sans autorisation matérielle |
| Destruction | `Shutdown()` de l'adaptateur avant destruction du contrôleur et fermeture du handle RAII sous son mutex |

La série est comparée en interne mais n'est pas publiée dans les profils ou le
GUI de cette phase. Cela limite la distinction de plusieurs D5 identiques par
le mécanisme de correspondance des profils ; un futur contrôle actif devra
résoudre ce point sans utiliser une correspondance de profil comme permission.

Les zones représentent **les allocations des groupes retenus**, pas une mesure
du nombre de LED physiquement branchées. La taille externe est celle de g0 ;
13 n'est pas codé en dur dans l'adaptateur. Les tests vérifient notamment1,7 et13.
Si g0 est Unused ou de longueur nulle, aucune zone externe n'est inventée.
Les autres groupes externes configurés ne prouvent pas la présence de LED
supplémentaires et ne sont pas exposés comme des sorties physiques distinctes.
G7 n'est jamais une troisième zone. Un contrôleur construit sans instantané
valide n'invente plus la zone historique de six LED.

## Compilation et tests hors ligne

Environnement disponible : Ubuntu24.04.5, GCC13.3, Qt5.15.13 ; dépendances déjà
installées, dont libusb1.0.27, HIDAPI0.14 et mbedTLS2.28.8. Aucun paquet installé.

```bash
mkdir -p .build/d5next-lot9/full
cd .build/d5next-lot9/full
qmake ../../../OpenRGB.pro CONFIG+=d5next_readonly
make -j8
```

Application complète compilée et liée : `.build/d5next-lot9/full/openrgb`.
`make -q` final retourne0. La macro expérimentale est présente dans les objets
D5 de cette application ; la macro `OPENRGB_D5NEXT_OFFLINE_TESTS` en est absente.
Les avertissements de dépendances partagées existantes, notamment stb, ne sont
pas présentés comme des erreurs D5 NEXT. Aucune erreur d'intégration/linkage D5.

Scripts exécutés depuis la racine, sans accès matériel :

| Script existant | Normal | ASan/UBSan |
|---|---:|---:|
| `run-rgbpx.sh` | 838897 vérifications | 838897 |
| `run.sh` | 16766 | 16766 avec suppression préexistante |
| `run-production.sh` | 162 ; tous les appels HID interdits | pas d'option sanitizer dans ce script |
| `run-detection.sh` | 972 ; vraie macro, backend simulé | 972 avec suppression préexistante |
| `run-readonly.sh` | 12418 | 12428 |
| `run-speed.sh` | 93351 | 93351 |
| `run-pink.sh` | 274269 puis208733 | 274300 puis208764 |

Chemin des scripts : `tests/AquaComputerD5Next/`. Les variantes utilisent
`--sanitize`, sauf `run.sh --sanitize-known-baseline` pour la passe corrigée du
seul bruit préexistant. Ces nombres sont des **assertions, pas un nombre de
scénarios ou un pourcentage de couverture**. Les différences des outils de
préparation viennent de leur protocole de tests simulés, pas d'essais réels.

`run.sh --sanitize` strict a également été exécuté : il s'arrête sur le défaut
préexistant `RGBControllerInterface.h:287`, `memset(nullptr,0,0)` déclaré nonnull.
`ubsan.supp` n'a pas été modifié ou élargi. Les passes utilisant sa suppression
existante réussissent ; cela ne constitue pas une passe UBSan stricte complète.

La suite de détection couvre notamment : permissions refusées avant ouverture,
identité/interface incorrectes, ouverture refusée, doublons chemin/série,
fermeture et libération des réservations, **65 sites d'allocation dont58 après
ouverture**, firmware inconnu, série différente, Input nul/négatif/court/long ou
d'ID incorrect, Feature court/long/négatif, mauvais ID/CRC/structure, allocations
invalides, recouvrement externe, mode inconnu et deux lectures concurrentes.
Les tests interrogent le RGBController réellement construit et son JSON de
profil :19 LED/couleurs, deux zones, tailles, pointeurs contigus, absence de
resize et de modes par zone. Tous les types de patch g0/g6 restent refusés sans
GET/SET même après connaissance du firmware et de la configuration.

## Essai graphique réel en lecture seule

Essai effectué dans la session graphique locale disponible, `DISPLAY=:0`,
backend Qt xcb/XWayland. Aucun hôte distant supplémentaire n'a été contacté ;
le nom « Pingus » n'a pas été vérifié comme identité d'une autre machine.

Identification locale : **0C70:F00E/interface1**, permissions **0660
root:plugdev**, accès effectif lecture/écriture requis par HIDAPI autorisé pour
l'utilisateur non-root. Série disponible et vérifiée, valeur omise. Aucun
contrôleur concurrent connu ni handle HID/vendor accessible trouvé avant
l'essai ; **379 répertoires fd de processus inaccessibles**, donc absence
universelle de concurrence non démontrée. Aucune permission ou règle udev
modifiée, aucun programme lancé en root.

Une configuration privée distincte sous `.build/d5next-lot9/gui-config/`
désactive tous les détecteurs sauf D5 NEXT. La liste a été contrôlée après sa
mise à jour par le DetectionManager : seul D5 NEXT reste activé. Aucun client,
plugin utilisateur, profil utilisateur ou serveur SDK existant n'est chargé ;
`--noautoconnect` évite la connexion à un serveur local.

Des DSO **temporaires et locales à `.build/`**, sans modification du binaire
OpenRGB, ont servi à instrumenter l'essai :

- garde HIDAPI limitant l'énumération au VID/PID/interface D5 et les ouvertures
  au chemin ainsi découvert ; refus systématique des appels SET/Output ;
  backend réel hidraw conservé pour les lectures et métadonnées ; ouverture
  vendor/libusb interdite dans le processus de test ;
- observateur Qt utilisant les vrais widgets de l'application, sélectionnant
  les deux zones, cliquant une fois sur le vrai bouton `ButtonRescan` et
  fermant le GUI proprement ; aucun accès au contrôleur depuis cet observateur ;
- contrôle `strace` des ouvertures/fermetures et des requêtes ioctl numériques,
  arguments des ioctl non décodés pour ne pas enregistrer de rapport privé.

La garde ne remplace pas la preuve des protections de production : ces
protections sont testées séparément avec les vrais objets compilés. Le résultat
ci-dessous constate **zéro tentative bloquée**, pas simplement un SET tenté
puis masqué par l'instrumentation. Les sources des DSO, captures de fenêtre,
profils et journaux locaux restent exclus du suivi Git dans `.build/`.

### Résultats observés

| Élément | Résultat |
|---|---|
| Input01 | 162 octets, ID01, firmware1027 |
| Feature03 | 809 octets, ID03, structure3 ; CRC reçu/calculé **2BFD**, valide |
| g0 | strip0, start0, count13, mode02 Breathing |
| g6 | strip1, start0, count6, mode02 Breathing |
| g7 | strip1, start0, count6, mode02 ; recouvre g6, priorité toujours inconnue |
| GUI | Une page `Aqua Computer D5 NEXT`, mode unique `Read only` ; zones `Pump RGB` et `External RGBpx` |
| Capacités affichées | Sélection individuelle de LED et application de couleurs désactivées ; vitesse/luminosité désactivées ; `Saving Not Supported` |
| Profil réel | Enregistrement local puis chargement avec `--profile lot9-readonly`, nom actif observé dans le GUI ;19 LED/couleurs, tailles6/13 dans son JSON |
| Rescan réel | Ancien handle fermé, nouveau handle ouvert, nouvelle lecture ; toujours une seule page et les deux mêmes zones |
| Session automatisée démarrage + rescan | **2 ouvertures, 2 fermetures, 2 Input01, 2 GET Feature03, 0 tentative SET/Output bloquée** |
| Contrôle ioctl de cette session | **2 `HIDIOCGFEATURE(809)`, 0 `HIDIOCSFEATURE(809)`** |
| Arrêt | Fermeture propre du GUI et libération des handles ; aucune instance de test laissée ouverte |

Le CRC n'est pas employé comme preuve de compatibilité à lui seul ni comme
preuve cryptographique de conservation de tous les octets. Le firmware, la
structure, l'identité et les allocations ont été vérifiés séparément.
L'essai n'évalue pas un changement de rendu lumineux : il n'en demande aucun.

### Incident d'environnement graphique, résolu

Le premier démarrage a terminé la lecture et créé le profil local, puis a
échoué avant affichage sur `__libc_pthread_init / GLIBC_PRIVATE` : les variables
GTK héritées de VS Code Snap chargeaient des bibliothèques de `/snap/core20/`.
Aucune erreur USB ni tentative d'écriture n'a été relevée. Pour cette terminaison
anormale, la fermeture du fd relève du noyau ; la destruction C++ n'est pas
revendiquée comme observée.

Le GUI a ensuite été lancé avec `QT_STYLE_OVERRIDE=Fusion`, `QT_QPA_PLATFORM=xcb`
et sans les variables héritées `GTK_PATH`, `GTK_EXE_PREFIX`,
`GTK_IM_MODULE_FILE`, `GIO_MODULE_DIR`, `GDK_PIXBUF_MODULE_FILE`,
`GDK_PIXBUF_MODULEDIR`, `GTK_MODULES`, `SNAP_LIBRARY_PATH`. Aucun changement
système n'a été effectué. Une première session visible avec chargement du profil
s'est fermée proprement :1 ouverture/fermeture,1 Input/GET,0 tentative bloquée.
La session instrumentée avec rescan décrite ci-dessus a ensuite réussi.
Au total, ces essais d'application ont effectué **quatre lectures Input/Feature**,
espacées entre les démarrages et le rescan, sans répétition après erreur USB.

## Limites et chemin minimal du Lot 9B

Le GUI expose une géométrie de configuration et un instantané, pas un moteur
RGB actif. Une modification extérieure n'est visible qu'après un nouveau scan.
Les modes inconnus, les firmwares autres que1027 et les allocations ambiguës
sont refusés. La présence physique de13 LED est issue du montage déjà validé,
pas d'une capacité d'auto-mesure inventée. Le basculement aquabus/RGBpx n'est
ni détecté par un bit supposé ni commandé. Aucun Direct Mode, streaming,
transition d'effet ou chenillard continu sur19 LED n'est ajouté.

Le chemin logiciel proposé pour 9B est le suivant ; **il n'est pas raccordé
aux callbacks de cette version** :

1. Définir une requête de teinte Breathing02 typée et bornée, distincte des
   métadonnées du catalogue. Le routage est explicite : zone Pump→g6,
   zone External→g0. Ne jamais déduire le groupe d'un nom libre de profil.
   Résoudre l'identité de plusieurs pompes et traiter le recouvrement g6/g7
   sans modifier g7.
2. Sortir les politiques pures, spécifiques g0/g6, des outils matériels du
   Lot8 vers un composant testable partagé avant tout branchement GUI.
   Le patch générique actuel refuse g0 : changer une autorisation seule ne
   suffit pas et cette restriction n'est pas levée ici. Le catalogue `Find()`
   n'accorde aucune permission.
3. Pour cette première extension, accepter seulement une variation de teinte
   du premier rôle couleur en mode02 déjà actif, avec S/V et flags compatibles
   vérifiés. Pas de transition ni de changement implicite de saturation,
   luminosité, master, vitesse ou allocation. Les demandes de couleurs qui
   exigent une opération non validée doivent être refusées explicitement.
   Respecter le format RGBColor API **0x00BBGGRR** et les helpers R/G/B ;
   le bleu est `0x00FF0000`, avant conversion HSV documentée.
4. Sérialiser tout le cycle sur le handle propriétaire, vérifier de nouveau
   identité/série/firmware1027, contrôler les concurrents accessibles et les
   préconditions opérateur. Un verrou de processus ne prouve pas l'exclusivité
   des autres logiciels HID/vendor. L'instantané de découverte ne sert jamais
   de rapport à réécrire.
5. Lire un Feature03 **frais**, valider809/ID3/structure3/CRC et les allocations,
   exiger mode02 et état attendu, copier ce rapport, modifier uniquement H0
   du groupe visé. Pour g6, whitelist **706–707 +807–808** ; pour g0,
   **286–287 +807–808**. Comparer indépendamment les809 octets et refuser
   toute différence ailleurs, notamment hydraulique/thermique/opaque.
6. Préparer un dry-run contrôlable, puis une autorisation d'écriture propre à
   cette opération et à ce périphérique. Elle ne doit provenir ni du JSON,
   ni d'un profil, ni d'une sélection de mode GUI ou de l'existence du catalogue.
   Conserver la surveillance et le moyen de récupération opérateur validés
   au Lot8 ; ne pas utiliser une restauration automatique d'un ancien rapport.
7. Autoriser au plus un SET après validation distincte, sans répétition ni
   SettingsSave. Effectuer une lecture de contrôle et comparer au résultat
   attendu ; invalider l'opération en cas d'erreur ou de divergence. Ne pas
   réessayer automatiquement et ne pas prétendre qu'une absence de Save
   garantit l'absence de tout effet non volatil.
8. Ajouter les modes/couleurs API seulement pour les fonctions effectivement
   raccordées et validées, avec tests des profils, callbacks asynchrones,
   requêtes concurrentes, autorisations absentes, modifications étrangères,
   readback et erreurs. Les sélecteurs de couleur arbitraires ou effets
   supplémentaires nécessitent leurs propres politiques et validations.

**Décision : Lot9A validé pour la découverte expérimentale et la représentation
GUI en lecture seule. Le raccordement actif GUI→SET demeure NO-GO dans ce lot.**
