# D5 NEXT — lot 6B : audit critique de la refonte RGBpx

Date : 10 novembre 2026, Europe/Paris. Branche `feat/d5next`.
HEAD : `35227a93bb5635b919fed026cefeceeef2e6df0d`.
Référence : [rapport du lot 6](D5NextRGBpxCppRefactor.md).

**Verdict : protections de production maintenues, revue corrective nécessaire.**
Trois problèmes ont été reproduits dans les interfaces existantes : divergence
de la façade historique, fuite de handle sur exception et état de mode annoncé
après une transition refusée. Une quatrième reproduction confirme la perte de
mise à jour entre deux instances du même périphérique simulé, risque déjà connu
qui reste bloquant avant toute activation matérielle. La compilation complète
reste non validée à cause des dépendances de développement absentes.

Ces constats ne démontrent pas un contournement des gardes de production :
**aucun GET/SET n’est accessible par les constructions normales examinées**.
Le nouveau codec passe les contrôles supplémentaires de bornes et préservation.
Cela ne constitue ni une validation du firmware réel ni une autorisation de SET.

**Aucun C++ existant modifié, aucun fichier indexé, commit, push ou PR. Aucun
accès matériel, lancement d’OpenRGB/Aquasuite, reset, flashage ou changement de
permissions.** Les sondes supplémentaires utilisent des fichiers texte locaux
sous `.build/d5next-lot6b/`, compilés avec un backend HID entièrement simulé.
Le présent audit est aussi réalisé avec **Codex** ; il ne remplace pas une revue
humaine indépendante du code assisté du lot 6.

## 1. Arbre réel et inclusion des sources

Avant cet audit : **12 fichiers suivis modifiés et 10 fichiers non suivis**.
Le diff ordinaire ne contient que les12 premiers. Aucun changement n’était
indexé. Le travail du lot6 n’était donc pas un commit supplémentaire sur HEAD.

Fichiers suivis modifiés, tous dans le périmètre annoncé :

| Répertoire | Fichiers |
|---|---|
| `Controllers/AquaComputerD5NextController/` | `AquaComputerD5NextController.{h,cpp}`, `D5NextProtocol.{h,cpp}`, `RGBController_AquaComputerD5Next.cpp` |
| `tests/AquaComputerD5Next/` | `D5NextTests.cpp`, `D5NextTests.pro`, `PinkTests.cpp`, `ReadOnlyTests.cpp` |
| `tools/AquaComputerD5Next/` | `D5NextDiagnostic.cpp`, `D5NextPinkMain.cpp`, `build.sh` |

Inventaire complet des fichiers non suivis du lot6 :

| Fichier | Inclusion effective ou fonction |
|---|---|
| `Controllers/AquaComputerD5NextController/D5NextEffects.cpp` | **Présent**, explicitement dans `D5NextTests.pro:32`, `run-rgbpx.sh:25` et le build du diagnostic ; symbole `D5NextEffects::Count()` présent dans l’exécutable de tests de production |
| `Controllers/AquaComputerD5NextController/D5NextRGBpx.cpp` | Tests natifs/RGBpx et diagnostic |
| `Controllers/AquaComputerD5NextController/D5NextRGBpx.h` | Modèles, catalogue et interface de patch |
| `Controllers/AquaComputerD5NextController/D5NextSystemCommands.cpp` | Tests natifs/RGBpx ; symbole d’encodage présent au linkage |
| `Controllers/AquaComputerD5NextController/D5NextSystemCommands.h` | Interface d’encodage hors ligne |
| `tests/AquaComputerD5Next/ProductionTests.cpp` | Cible QMake `CONFIG+=d5next_production`, sans les deux macros de découverte/test |
| `tests/AquaComputerD5Next/RGBpxTests.cpp` | Suite pure compilée par `run-rgbpx.sh` |
| `tests/AquaComputerD5Next/run-production.sh` | Lance uniquement les tests avec sentinelles HID en mémoire |
| `tests/AquaComputerD5Next/run-rgbpx.sh` | Compile et lance le codec sans backend HID |
| `Documentation/D5NextRGBpxCppRefactor.md` | Rapport du lot6 |

Pour l’application principale, [OpenRGB.pro](../OpenRGB.pro#L99), lignes99–105,
recense récursivement les en-têtes et sources de `Controllers/`, puis ajoute
`CONTROLLER_CPP` à `SOURCES` ligne210. Les trois nouveaux noms de `.cpp` sont
uniques dans cet arbre et ne portent aucun suffixe d’exclusion par OS.
Cette inclusion est établie statiquement ; **elle ne remplace pas un linkage
réussi de l’application complète**, non obtenu ici.

`D5NextEffects.cpp` n’est donc ni absent ni oublié du build. Son absence d’un
`git diff --stat` s’explique par son état non suivi. Pour la revue suivante,
il faut conserver l’ensemble des dix fichiers avec le diff des douze autres,
sans les indexer pendant cet audit.

`git ls-files` ne contient aucun fichier sous `aquasuite/` ou `aquapx-main/`,
aucune capture PCAP/PCAPNG, vidéo, dump de processus, Prefetch ou paquet firmware
privé repéré. Les nouveaux fichiers sont uniquement des sources, scripts et
documentation. Aucun contenu Aquasuite ni rapport USB réel complet n’y est
ajouté. Aucun changement involontaire hors de ce périmètre n’a été constaté.

## 2. Constats bloquants

### B1 — Exclusivité insuffisante avant une future écriture matérielle

**Sources :** [contrôleur natif](../Controllers/AquaComputerD5NextController/AquaComputerD5NextController.cpp#L69),
lignes69–77 et151–159 ; mutex membre dans
[son en-tête](../Controllers/AquaComputerD5NextController/AquaComputerD5NextController.h#L36),
lignes36–44. Le verrou protège une instance, sans registre partagé par identité
physique ni exclusion interprocessus.

**Reproduction :** deux handles simulés distincts représentant la même pompe,
deux instances, une requête master153 et une requête Off. Une barrière fait
terminer les deux lectures du même état initial avant les écritures. Les deux
appels retournent OK ; le résultat observé est master153/Off0 : la demande Off
est perdue. L’autre ordre conduirait à master255/Off2 et perdrait le master.
Il suffit d’un entrelacement GET-A, GET-B, SET-A, SET-B ; aucune hypothèse de
transfert USB physiquement simultané n’est nécessaire.

**Portée :** risque préexistant, déjà mentionné dans les lots1/6, confirmé ici
sans matériel. Il ne rend pas la construction de production actuelle capable
d’écrire. Il empêche de considérer le RMW comme sûr dès qu’une autorisation
matérielle serait ajoutée ; un autre client pourrait également modifier des
octets hydrauliques entre lecture et écriture.

**Correction proposée :** une transaction et une propriété de handle uniques
par périphérique physique dans le processus, avec refus des doublons et une
politique d’exclusivité contre les autres clients avant tout GET/SET. Un simple
flock coopératif ne prouve pas l’exclusion des clients qui ne le respectent pas.
Ne pas lever les gardes en ajoutant seulement1027 à une liste d’approbation.

### B2 — Compilation complète et linkage de l’application non validés

**Sources :** [OpenRGB.pro](../OpenRGB.pro#L484), lignes484–487 et550–565.
L’application exige libusb et choisit un fournisseur hidapi via pkg-config.

**Reproduction effectuée :** QMake3.1/Qt5.15.13, dans
`.build/d5next-lot6b/full/`, commande
`qmake /home/pepeuch/Documents/vscode/OpenRGB/OpenRGB.pro`.
Code de sortie **3**, erreur : `libusb-1.0 development package not found`.
La génération s’arrête avant Makefile : aucune compilation complète ni édition
de liens de l’application n’a pu démarrer. Aucun binaire OpenRGB n’a été lancé.

Les recherches pkg-config confirment aussi l’absence de `hidapi-hotplug-hidraw`,
`hidapi-hidraw`, `hidapi-libusb` et `hidapi`. Les tests isolés n’utilisent aucun
de ces backends, ce qui explique leur réussite sans ces paquets. Les autres
dépendances de l’application ne sont pas déclarées validées par cette tentative.

**Correction proposée :** fournir les dépendances de développement habituelles
de l’environnement Linux puis relancer QMake et le build complet, sans exécuter
l’application. Aucune installation système n’a été effectuée. Ce blocage
d’environnement n’est pas présenté comme une erreur de compilation propre au D5.

## 3. Constats importants

### I1 — Façade historique : politiques divergentes du nouveau moteur

**Sources :** [D5NextProtocol.cpp](../Controllers/AquaComputerD5NextController/D5NextProtocol.cpp#L163),
lignes163–194 et197–249, contre
[D5NextRGBpx.cpp](../Controllers/AquaComputerD5NextController/D5NextRGBpx.cpp#L222),
lignes222–240 et305–334.

**Reproductions sur buffers synthétiques :**

1. En Rainbow inchangé, modifier seulement H, recalculer le CRC :
   `ValidateLightingChange` retourne **OK**, alors que `BuildLightingReport`
   refuse exactement cette demande avec `UNSUPPORTED_OPERATION`.
2. Un groupe6 FixedColor alloué à **strip0** au lieu du strip interne1 reste
   éditable par `BuildLightingReport`. Le nouveau `BuildPatch` refuse la même
   allocation avec `PRECONDITION_FAILED`.

L’ancienne API n’exige pas non plus de firmware ni d’identité. L’hypothèse de
format passée en argument ne fournit pas ces preuves. Elle reste accessible
dans l’en-tête de production et conserve plusieurs représentations ambiguës :
`D5NextLighting.brightness` est le master décodé, mais `RGBToHSV` utilise ce même
membre pour retourner la valeur du RGB converti.

**Impact :** source de régression si un futur appelant réutilise cette façade
plutôt que le moteur contrôlé. Le chemin natif actuel utilise le nouveau moteur,
et le test rose historique est bloqué : **aucun contournement de production
n’a été démontré**. La limitation est décrite dans le lot6, mais elle laisse
deux contrats de construction/audit incompatibles dans le code livré.

**Correction proposée :** retirer la construction/audit historique de l’API de
production ou la déléguer au nouveau moteur en exigeant identité, allocation
et requête précise. Isoler les comparaisons historiques dans les tests. Extraire
la conversion couleur vers un retour HSV explicite pour supprimer le double
sens de brightness. Ajouter ces deux reproductions aux tests permanents.

Le premier cas est une divergence créée par le durcissement du constructeur
au lot6 sans durcissement équivalent de son audit. L’absence de contrôle
d’allocation/firmware de l’ancienne façade est héritée du prototype.

### I2 — Handle non libéré si une allocation échoue pendant la détection

**Sources :** [détecteur](../Controllers/AquaComputerD5NextController/AquaComputerD5NextControllerDetect.cpp#L49),
lignes49–57 ; [constructeur natif](../Controllers/AquaComputerD5NextController/AquaComputerD5NextController.cpp#L15),
lignes15–22 ; [constructeur de l’adaptateur](../Controllers/AquaComputerD5NextController/RGBController_AquaComputerD5Next.cpp#L19),
lignes19–38.

**Reproduction :** le stub d’ouverture réussit, puis l’allocation suivante
déclenche `std::bad_alloc`. Le détecteur propage l’exception avec un delta de
**une ouverture et zéro fermeture**. Reproduction réussie aussi dans la branche
expérimentale de lecture seule, avec son contrôle access remplacé en mémoire.
Le chemin normal ferme correctement ; ce sont les chemins d’exception qui
manquent de propriété RAII.

Le détecteur n’a pas été modifié au lot6 : cette fuite est héritée du prototype,
elle n’est pas introduite par la refonte.

Les allocations du contrôleur, les chaînes, la construction de l’adaptateur et
l’insertion dans le vecteur sont susceptibles de lever. Le gestionnaire de
détection attrape les exceptions de coroutine
([DetectionManager.cpp](../DetectionManager.cpp#L650), lignes650–660) : il ne
faut pas supposer qu’un arrêt immédiat du processus nettoiera ces ressources.

**Correction proposée :** garder un propriétaire RAII du handle dès son
ouverture ; transférer sa propriété seulement après construction réussie du
contrôleur, puis garder contrôleur/adaptateur protégés jusqu’à l’insertion
réussie dans le résultat. Tester les échecs à chaque étape et pas seulement
`hid_open_path == nullptr`. Aucun correctif C++ appliqué ici.

### I3 — L’état OpenRGB peut annoncer une transition effectivement refusée

**Sources :** [adaptateur](../Controllers/AquaComputerD5NextController/RGBController_AquaComputerD5Next.cpp#L99),
lignes99–104 et128–160 ;
[SetActiveMode](../RGBController/RGBController.cpp#L1456), lignes1456–1487 ;
[refus natif](../Controllers/AquaComputerD5NextController/AquaComputerD5NextController.cpp#L95),
lignes95–99.

**Reproduction :** construction prototype hors lecture seule avec un rapport
simulé Rainbow, appel de `SetActiveMode(1)` puis attente du callback. Le GET
simulé a lieu, la transition est refusée, aucun SET n’a lieu, mais l’API retourne
mode actif **Static**, alors que le rapport simulé reste **Rainbow**. Le warning
ne rétablit pas l’état de l’interface. `active_mode=-1` à l’initialisation
empêche un Off initial, sans résoudre ce cas ultérieur.

Il s’agit d’une régression d’intégration liée aux nouveaux refus du lot6 ;
la publication anticipée du mode par la classe de base existait déjà.

**Portée :** la découverte réelle expérimentale construit l’adaptateur Read only
et ne présente pas cette liste. Le défaut concerne la branche prototype avec
modes ; il devra être résolu avant de l’exposer comme contrôle opérationnel.

**Correction proposée :** maintenir l’exposition en lecture seule tant que les
transitions sont refusées ; ensuite adopter et publier un état réellement lu
et prévoir un retour d’erreur ou une resynchronisation différée. Ne pas écrire
simplement dans les champs de base depuis un callback tenant déjà AccessMutex
en lecture : la discipline de verrouillage doit être conservée.

### I4 — Les suites ne couvrent pas toutes les garanties annoncées

**Sources :** [ProductionTests.cpp](../tests/AquaComputerD5Next/ProductionTests.cpp#L17),
lignes17–18 et57–102 ; [RGBpxTests.cpp](../tests/AquaComputerD5Next/RGBpxTests.cpp#L58),
lignes58–109,116–130,362–368 et503–548 ;
[test concurrent natif](../tests/AquaComputerD5Next/D5NextTests.cpp#L528), lignes528–559.

**Justification et contrôles :**

- La suite de158 assertions interdit par `#error` la macro de découverte
  expérimentale. Elle ne teste donc pas cette combinaison de compilation.
  Une sonde supplémentaire de cet audit la compile **sans** macro de simulation,
  remplace le backend et access, puis vérifie trois scans et callbacks/JSON :
  GET=0, SET=0, trois fermetures. Cette preuve supplémentaire n’est pas encore
  un test permanent du dépôt.
- Les200 transactions concurrentes existantes utilisent **une seule instance**.
  Elles ne peuvent pas détecter B1. Aucune injection d’exception ne couvre I2.
- Les scripts nomment des gardes de profil, mais les tests exercent des setters
  et callbacks, pas un chargement complet par ProfileManager. La preuve de
  non-écriture repose aussi sur l’analyse des membres privés et des callbacks.
- Les fixtures et codecs reprennent les mêmes formules de positions de groupes,
  bindings et couleurs. Le roundtrip est utile, sans constituer à lui seul
  une preuve indépendante de la cartographie. Les captures Windows restent
  absentes de cet espace ; les CSV documentés sont la référence disponible.
- Pour les lignes numériques et flags, le CSV est réellement rejoué avec
  préservation intégrale. Pour **Blink1.Background**, la branche545–548 ne
  vérifie que les métadonnées ; elle ne rejoue pas l’édition H/S décrite.
  L’expression « chaque édition isolée » du rapport lot6 est donc trop large.
- Les gardes normales sont testées dans leur état initial où layout et
  autorisation bloquent ensemble ; cela ne démontre pas chaque garde isolément.
- Dans [PinkTests.cpp](../tests/AquaComputerD5Next/PinkTests.cpp#L273), lignes273–308,
  les scénarios nommés échec/rapport court de SET sont maintenant arrêtés avant
  le SET par le refus du plan. C’est approprié pour le retrait de l’ancien essai,
  mais leurs PASS ne valident plus l’ancien writer encore présent au source.
  Les erreurs de transfert du nouveau contrôleur sont, elles, testées dans la
  suite native : il ne faut pas confondre ces deux couvertures.

**Correction proposée :** transformer les reproductions I1–I3/B1 en tests de
régression, conserver une cible pour la découverte expérimentale sans macro
de simulation, ajouter le patch Background, et vérifier des petits vecteurs
RGB/binding littéraux revus à partir des preuves Windows. Tester les décisions
d’autorisation indépendamment sans introduire un moyen public de les lever.
Ajouter un scénario de profil réel dans un environnement sans backend matériel.

## 4. Améliorations

### A1 — Audit de patch dépendant du constructeur qu’il est censé vérifier

**Source :** [AuditPatch](../Controllers/AquaComputerD5NextController/D5NextRGBpx.cpp#L364),
lignes364–382 ; construction du masque aux lignes243–355 du même fichier.

Le résultat attendu est produit en rappelant `BuildPatch`. C’est efficace pour
rejeter un candidat altéré après sa construction, mais une erreur commune
d’offset ou de masque du constructeur serait acceptée par cet audit. Il ne
faut pas présenter les deux appels comme deux implémentations indépendantes.
Les tests à offsets littéraux et l’inspection du code apportent une vérification
distincte ; aucune mauvaise position n’a été constatée dans le nouveau moteur.

**Proposition :** vérifier indépendamment les champs ciblés et le périmètre
canonique de bits RGB modifiables, puis contrôler le diff et le CRC sans
réutiliser tout le constructeur. Ne pas élargir ce périmètre à un bloc complet.

### A2 — Suppression UBSan plus large qu’une seule ligne, défaut hérité

**Sources :** [ubsan.supp](../tests/AquaComputerD5Next/ubsan.supp#L5), ligne5 ;
[script](../tests/AquaComputerD5Next/run.sh#L13), lignes13–20 ;
[matrix_map_type](../RGBController/RGBControllerInterface.h#L275), lignes275–287.

Le lancement strict échoue à nouveau sur `memset(nullptr,0,0)` dans le fichier
partagé, ligne287. Le lancement avec baseline passe. Le motif de suppression
est **tout diagnostic nonnull-attribute attribué à ce fichier d’en-tête**,
pas exclusivement cette fonction ou cette ligne. Il n’exclut pas tous les
sanitizers du code D5, mais peut masquer un autre diagnostic de même catégorie
dans cet en-tête. Les scripts héritent aussi des options sanitizer de l’appelant.

**Proposition :** garder le résultat strict visible, faire corriger ce défaut
préexistant dans une revue distincte et retirer la suppression ensuite. Entre
temps documenter son véritable périmètre et nettoyer les options héritées pour
les mesures reproductibles. Aucune modification de l’API partagée ici.

### A3 — Simplifier les contrats de requête et la dépendance historique

**Sources :** [requête de patch](../Controllers/AquaComputerD5NextController/D5NextRGBpx.h#L148),
lignes148–160 ; [conversion](../Controllers/AquaComputerD5NextController/D5NextRGBpx.cpp#L72),
lignes72–78 ; [validation historique](../Controllers/AquaComputerD5NextController/D5NextProtocol.cpp#L52),
lignes52–54.

La requête regroupe les champs de six actions, dont plusieurs sont ignorés selon
kind et dont V n’est actuellement autorisé par aucun rôle de couleur. La
conversion moderne retourne le bon HSV, mais passe encore par l’objet historique
dont brightness change de sens. La structure3 est validée à deux endroits avec
une constante et un littéral distincts. Ces points ne produisent pas de défaut
reproduit sur Linux ; ils compliquent la revue et les évolutions.

**Proposition :** demandes explicites ou fabriques nommées avec initialisation
complète, conversion HSV indépendante, une définition de layout et une seule
politique d’édition. Les entiers de futurs JSON doivent être validés avant
construction des requêtes ; le schéma de base n’est pas une autorisation.

## 5. Points conformes et preuve des gardes

### C1 — Framing, CRC, BE signé et préservation du nouveau codec

Dans [D5NextProtocol.cpp](../Controllers/AquaComputerD5NextController/D5NextProtocol.cpp#L31),
lignes31–56, pointeur, taille809 et ID03 précèdent l’accès aux octets ; le CRC
porte sur1–806 et est lu BE807–808 ; la structure3 est contrôlée.
Dans [D5NextRGBpx.cpp](../Controllers/AquaComputerD5NextController/D5NextRGBpx.cpp#L19),
lignes19–69, les limites sont contrôlées par soustraction avant offset+1,
sans débordement d’addition. Le signe est restauré arithmétiquement et les
écritures refusent les valeurs non représentables avant réduction.

Les positions des huit groupes et deux bindings concordent avec les rapports
Windows. Douze paramètres et source sont signés ; H est BE16, S/V sur8bits.
Sur la cible Linux/GCC examinée, aucun débordement ou comportement indéfini
nouveau n’a été mis en évidence dans ces codecs. Les pointeurs du catalogue
désignent des objets statiques const à durée de vie suffisante.

Le préparateur copie le rapport existant, conserve les octets opaques et ne
projette aucune structure C++ sur le buffer. Il refuse groupe externe, modes
inconnus et transitions. Il sépare master, flags16 de strip, flags du groupe et
V. Les masques ont des noms propres aux effets : Fade in04 n’est pas Reverse03.
Les rôles de palette inconnus et V ne sont pas édités. Les retours sur erreur
précèdent la copie de sortie, y compris dans les tests de chevauchement.

### C2 — Chemins HID et autorisation séparée de la connaissance du layout

| Chemin | Appel possible | Préconditions / état actuel |
|---|---|---|
| Constructeur normal natif | Aucun Feature | layout inconnu, firmware0, write_authorized=false, lignes15–22 |
| SetLighting / SetOff → UpdateLighting | GET03, puis éventuellement SET03 | Mutex, garde lignes77–80, firmware dans ReadFresh, validation/politique/audit avant envoi |
| ApplyRGBpxPatch | GET03, puis éventuellement SET03 | Même garde avant lecture aux lignes153–156 |
| ReadFresh | `hid_get_feature_report`, ligne186 | Privé ; les deux appelants sont gardés ; erreur de longueur bloque l’instance |
| SendChanged | `hid_send_feature_report`, ligne201 | Privé, après préparation/audit ; no-op sans SET, erreur bloque l’instance |
| Détecteur normal | Aucun open/GET/SET | Corps et enregistrement désactivés par défaut |
| Détection expérimentale | `access`, `hid_open_path`, puis close | Constructeur normal à deux arguments, adaptateur Read only ; aucun Feature dans le scan |
| Callbacks/profils/JSON Read only | Aucun Feature | Retour de l’adaptateur avant traitement du mode ; callbacks spécifiques et SaveMode de base inertes |
| Destruction | `hid_close` | Shutdown du worker avant suppression du contrôleur ; fermeture normale sans save/restauration |
| Diagnostic séparé | O_RDONLY, HIDIOCGRAWINFO, GET03 unique explicite | Outils compilés seulement ; tests de syscalls remplacés |
| Outil rose séparé | GET03 dans Run ; ancien SET encore dans le source | CLI --write-once refusé avant découverte ; BuildPlan échoue sur la transition ; politique firmware réelle vide |
| Encodeur SettingsSave | Aucun transport | Buffer Output02 hors ligne, aucun appel d’envoi |

Les seuls changements de layout/firmware/autorisation du contrôleur sont dans
son constructeur réservé à `OPENRGB_D5NEXT_OFFLINE_TESTS`, lignes25–37.
Aucun setter, champ de configuration JSON ou propriété du profil n’atteint ces
membres privés. La découverte expérimentale ne choisit pas ce constructeur et
stocke aussi read_only=true dans l’adaptateur. Les valeurs déclarées dans
`D5NextIdentity` ne sont jamais converties automatiquement en autorisation.

Aucun `hid_write`, envoi Output, reset ou commande système n’est appelé par le
contrôleur. L’ancien `HIDIOCSFEATURE` du test rose reste au source ligne338,
mais les trois blocages précités le rendent inaccessible dans le programme
normal actuel. Pour une réduction future du code mort, il peut être retiré
séparément plutôt que réactivé indirectement.

Les futurs schémas JSON exigent toujours leurs propres contrôles : le setter
de base [RGBController.cpp](../RGBController/RGBController.cpp#L1675),
lignes1675–1721 et1776–1828, ne constitue pas une validation complète des types
et dépendances. Ses callbacks tiennent déjà AccessMutex et DeviceMutex ;
appeler un getter verrouillant AccessMutex depuis eux risque un interblocage.
Aucun schéma éditable D5 ni ce type de callback n’est ajouté dans le lot6.

### C3 — Provenance, dépendances et conventions

Les données du catalogue concordent avec le CSV :33 paramètres dont32 isolés,
26 associations de flags, pas de mode06 ; Candle.v0 est Intensity et
Marquee2.Color change speed est v4. Les primaires, le layout et le paquet
SettingsSave concordent avec les documents Windows. Il s’agit d’un recoupement
avec ces preuves documentées, pas d’une nouvelle lecture des captures absentes.

Pas de dépendance nouvelle de production : CRC++ et JSON sont déjà dans OpenRGB.
Les classes séparent transport, codec et adaptateur ; les structures sont des
données, les tableaux fixes sont des tableaux C et les tests sont hors de
`Controllers/`. Le catalogue286 lignes, codec383 lignes et contrôleur215 lignes
restent d’une taille révisable, avec les simplifications A3 à considérer.

Aucun code Go traduit, fonction propriétaire décompilée ou ressource Aquasuite
n’a été identifié dans les changements examinés. Les données de protocole sont
attribuées à [hirparak/aquapx](https://github.com/hirparak/aquapx), aux
contributeurs [OpenRGB2111](https://gitlab.com/CalcProgrammer1/OpenRGB/-/issues/2111)
et aux analyses Windows de cette branche. Les SPDX suivent GPL-2.0-or-later.
Ce contrôle de provenance n’est pas une détermination juridique ni une assertion
de développement en clean room.

## 6. Tests rejoués, couverture et limites

Les commandes suivantes ont été exécutées, sans accès à une pompe :

```sh
bash tests/AquaComputerD5Next/run.sh
bash tests/AquaComputerD5Next/run-rgbpx.sh
bash tests/AquaComputerD5Next/run-production.sh
bash tests/AquaComputerD5Next/run-readonly.sh
bash tests/AquaComputerD5Next/run-pink.sh
env -u UBSAN_OPTIONS -u ASAN_OPTIONS bash tests/AquaComputerD5Next/run-rgbpx.sh --sanitize
env -u UBSAN_OPTIONS -u ASAN_OPTIONS bash tests/AquaComputerD5Next/run-readonly.sh --sanitize
env -u UBSAN_OPTIONS -u ASAN_OPTIONS bash tests/AquaComputerD5Next/run-pink.sh --sanitize
env -u UBSAN_OPTIONS -u ASAN_OPTIONS bash tests/AquaComputerD5Next/run.sh --sanitize
env -u UBSAN_OPTIONS -u ASAN_OPTIONS bash tests/AquaComputerD5Next/run.sh --sanitize-known-baseline
bash tools/AquaComputerD5Next/build.sh
bash tools/AquaComputerD5Next/build-pink.sh
```

| Suite | Résultat de ce lot6B |
|---|---|
| Native simulée | PASS,45 033 assertions |
| RGBpx | PASS,742 189 assertions |
| Production sans macros de test/découverte | PASS,158 assertions |
| Lecture seule | PASS,8 264 assertions |
| Rose, production / synthétique | PASS,274 269 /208 733 assertions |
| ASan/UBSan RGBpx et lecture seule sans suppression | PASS,742 189 /8 264 assertions |
| ASan/UBSan rose sans suppression | PASS,274 300 /208 764 assertions |
| Native sanitizer strict | **ÉCHEC**, code1, défaut hérité ligne287 |
| Native sanitizer avec baseline annoncée | PASS,45 033 assertions |
| Compilation des deux outils dédiés | PASS ; aucun outil lancé |
| Configuration du build OpenRGB complet | **ÉCHEC**, code3, libusb de développement absent |

Les nombres sont des **assertions exécutées**, pas autant de scénarios, de
branches ou de propriétés indépendantes. Dans RGBpx,524288 assertions viennent
de la boucle des65536 représentations16bits et131070 du refus des65535 autres
firmwares : **88,30 %** du total pour ces deux boucles. Ces tests sont utiles
et exhaustifs sur leur domaine ; ils ne couvrent pas tout le pilote.

Une compilation séparée avec GCC13 `--coverage -O0 -g` et exécution de la suite
RGBpx existante donne, pour **D5NextRGBpx.cpp seulement** :

| Mesure gcov | Résultat |
|---|---|
| Lignes exécutables exécutées | 197/209 =94,26 % |
| Branches exécutées | 97,66 % selon gcov |
| Branches prises au moins une fois | 138/171 =80,70 % |

Ce n’est pas une couverture de l’application, du backend, des profils ou du
firmware. Parmi les lignes non exécutées : sorties output nul/taille invalide,
kind inattendu et erreurs de candidat d’audit. D’autres branches sont
inaccessibles par la politique actuelle : V interdit, valeurs de paramètres
déjà limitées aux short du catalogue, seconde validation de structure redondante.
Un pourcentage inférieur à100 % n’identifie donc pas automatiquement un défaut.

L’oracle CRC des fixtures est distinct de CRC++, et les tests natifs utilisent
aussi une formulation non réfléchie ; le vecteur B4C8 et le paquet34C6 sont
contrôlés. Les assertions de bytes BE et les vérifications de préservation sont
plus fortes qu’un simple encode/decode. Le recoupement avec le CSV reste
nécessaire, sans lui attribuer une preuve matérielle supplémentaire.

Sondes supplémentaires de cet audit, toutes locales et hors suivi Git :

- `codec-probe.txt` / `codec-probe` : divergences I1, sorties inchangées,
  tailles0/808/810/UINT_MAX, firmware hors16bits, groupe/mode inattendus,
  kinds6/7, valeurs extrêmes, chevauchement sur refus ; **2000 cas à motif
  pseudo-aléatoire** vérifient un patch de paramètre et tous les octets restants,
  avec CRC de référence. Exécuté sous ASan/UBSan sans suppression.
- `integration-probe.txt` et `build-integration-probes.py` : branche de découverte
  expérimentale sans macro de simulation, backend/access remplacés ; trois
  scans/fermetures, callbacks et demandes JSON, GET=0/SET=0 ; injection bad_alloc
  reproduisant I2. Une variante de simulation reproduit I3 et B1.
- `coverage/` : instrumentation gcov de la suite existante, pas un nouveau jeu
  de fixtures ni une modification des sources.

Reproduction locale des sondes déjà préparées :

```sh
UBSAN_OPTIONS=halt_on_error=1 .build/d5next-lot6b/codec-probe
python3 .build/d5next-lot6b/build-integration-probes.py
```

Le constructeur autorisé de la variante de simulation est lié exclusivement
aux symboles HID de la sonde. Les octets sont artificiels ; aucun report privé
n’est chargé. `ldd` de la sonde expérimentale ne montre que les bibliothèques
standard ; une trace `strace -f -e trace=%file,ioctl` ne contient aucun accès
hidraw, USB ou sysfs de découverte et aucun ioctl HID/USB. Les sources et
journaux de ces reproductions restent dans `.build/d5next-lot6b/`.

## 7. Revue upstream et décision

[CONTRIBUTING.md](../CONTRIBUTING.md#L73), lignes73–75, demande la responsabilité
et la revue manuelle du développeur humain et indique que les soumissions
générées par IA sont généralement refusées. Le lot6 et cet audit sont assistés
par Codex. Une seconde lecture humaine des interfaces, offsets, domaines
d’édition, verrouillages et licences est indispensable avant contribution ;
aucune acceptation upstream n’est présumée ni l’assistance dissimulée.

L’ordre de travail proposé est : corriger/quarantainer la façade historique,
fermer les chemins d’exception, aligner état publié et refus des opérations,
ajouter les régressions manquantes, puis obtenir une compilation complète.
Toute activation matérielle doit en outre résoudre B1, établir l’identité
réellement lue et une autorisation distincte. Le présent audit ne fournit
aucune option d’activation ou commande vers la pompe.

Les62 fichiers sources, scripts et documents présents dans l’instantané initial
ont été contrôlés par empreintes ; l’index Git est inchangé. Le seul ajout hors
`.build/` de cet audit est **ce rapport**. Les captures et ressources tierces
restent hors suivi. Les constats décrivent une fondation encore expérimentale,
pas un support matériel validé. **Arrêt après audit ; NO-GO matériel maintenu.**
