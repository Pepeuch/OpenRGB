# D5 NEXT — lot 6C : corrections de l’audit RGBpx

Date : 10 novembre 2026 (Europe/Paris). Branche : `feat/d5next`.
Références : [refonte du lot 6](D5NextRGBpxCppRefactor.md),
[audit du lot 6B](D5NextLot6Audit.md),
[corrélation vidéo/USB](D5NextRGBpxVideoUSBMapping.md) et
[table des paramètres](D5NextRGBpxVideoUSBParameters.csv).

**Décision : GO pour poursuivre l’intégration logicielle et la revue humaine ;
NO-GO pour toute écriture ou validation matérielle.** Les corrections ont été
testées avec des buffers et backends HID simulés. Aucun accès USB/HID réel,
GET/SET réel, lancement de l’application, modification de permissions, commit,
push ou PR n’a été effectué. Les rapports et ressources Aquasuite ne sont pas
ajoutés au dépôt. `aquapx-main/` est inchangé.

## État des constats

| Constat | État après le lot 6C | Preuve / limite |
|---|---|---|
| I1, façade historique divergente | Corrigé dans le code actif | `BuildLightingReport`, `ValidateLightingChange` et leur masque général ont été retirés de `D5NextProtocol.{h,cpp}`. Le test rose refuse la transition sans fabriquer de candidat ; sa branche `HIDIOCSFEATURE` a été supprimée. Les modifications natives utilisent `D5NextRGBpx::BuildPatch` et `AuditPatch`, avec identité, allocation et mode attendu. |
| I2, durée de vie HID | Corrigé pour les chemins examinés | `D5NextHIDHandle`, `unique_ptr` du contrôleur et de l’adaptateur, puis `release()` seulement après insertion réussie. Injection de 23 rangs d’allocation, dont 16 après `hid_open_path` simulé : à chaque exception, chaque ouverture réussie a exactement une fermeture et la réservation est réutilisable. |
| I3, état affiché après refus | Corrigé localement | L’adaptateur D5 n’annonce qu’un mode `Read only`, sans couleur, luminosité ni transition pilotable. Rainbow → Static est refusé par le moteur ; demander l’index Static absent laisse l’index actif à `Read only`, avec zéro SET. L’API OpenRGB commune reste inchangée. |
| B1, propriété et concurrence | Partiellement corrigé, reste bloquant pour le matériel | Un registre processuel réserve le chemin HID et, s’il existe, le numéro de série. Un doublon est refusé avant ouverture. Le verrou de transaction appartient à cette réservation ; il ne bloque pas les autres pompes. Les alias physiques non représentés par le même chemin/série et les clients externes ne sont pas exclus. |
| B2, build complet | Non validé | QMake quitte avec code 3 : `libusb-1.0 development package not found`. Aucun Makefile complet ni linkage de l’application n’a été obtenu. |

### I1 : politique unique et représentation des couleurs

Avant correction, une modification isolée de H en Rainbow avec CRC recalculé
rendait `ValidateLightingChange == OK`, alors que `BuildLightingReport`
retournait `UNSUPPORTED_OPERATION`. Un groupe 6 attaché au strip 0 pouvait aussi
être édité par l’ancien constructeur ; `BuildPatch` le refusait. Les deux
reproductions sont désormais des tests permanents dans `RGBpxTests.cpp` :
`BuildPatch` **et** `AuditPatch` refusent respectivement avec
`UNSUPPORTED_OPERATION` et `PRECONDITION_FAILED`. Le candidat et les sorties
restent intacts en cas de refus. L’ancienne API sans identité ne peut plus être
réintroduite par un appelant de production.

`D5NextProtocol::RGBToHSV` renvoie maintenant `D5NextHSV` avec un champ `value`.
`D5NextLighting::brightness` ne désigne que le master décodé à l’offset 237 ;
la valeur HSV de palette est à l’offset 709. `D5NextRGBpx::RGBToHSV` délègue
directement à la conversion typée. Le décodage historique en lecture et le CRC
restent disponibles pour le diagnostic.

Le test rose historique reste un outil de lecture préparatoire, sans candidat
écrivable : `BuildPlan` et `AuditPlan` vérifient leurs entrées puis refusent
Rainbow → Static. Son option `--write-once` est bloquée par le programme principal,
et `Run` ne contient plus de `HIDIOCSFEATURE`. Aucun nouveau constructeur de
rapport complet n’a été ajouté.

### I2 et B1 : séquence de propriété

`D5NextOwnership::Acquire` construit une réservation pour le chemin fourni par
le détecteur et la série HID éventuelle. Le mutex du registre ne couvre que
l’insertion/retrait des clés, jamais les opérations HID. Une exception pendant
l’insertion de la série retire aussi le chemin ; un objet non réservé ne retire
aucune clé. Une série absente conserve la détection en lecture seule, mais
empêche même la construction de test d’autoriser une écriture.

Le détecteur garde successivement la réservation, le handle ouvert, le contrôleur
et l’adaptateur dans des propriétaires RAII. Après `push_back`, le vecteur prend
l’adaptateur. Le destructeur de l’adaptateur arrête son worker avant de détruire
le contrôleur ; celui-ci ferme le handle sous le mutex de la réservation, puis
libère cette dernière. Une allocation ratée à l’une de ces étapes ne laisse ni
handle ni réservation. Les tests couvrent également ouverture refusée, doublon
de chemin, alias de série, disparition/réapparition et deux pompes distinctes
sur deux threads.

Cette propriété est **interne au processus et dépend des métadonnées HID**.
L’unicité physique d’un numéro de série n’a pas été vérifiée sur plusieurs
appareils réels ; une collision ferait ignorer prudemment la seconde pompe.
Deux chemins alias sans série commune ne peuvent pas être reliés avec certitude
hors matériel. Aucun client d’un autre processus n’est exclu par ce registre,
et un verrou de fichier coopératif ne suffit pas. Ces limites restent bloquantes
avant toute écriture matérielle.

### I3 : état OpenRGB

`RGBController::SetActiveMode` change son index avant de demander l’application
et ne renvoie pas de résultat. Le défaut ne pouvait donc être corrigé par un
simple refus dans `DeviceUpdateMode`. L’adaptateur D5 annonce désormais
uniquement `Read only` et ses callbacks n’appellent aucun chemin HID. Le test
construit un rapport synthétique Rainbow, refuse une demande Static par le
moteur, puis confirme que l’adaptateur n’expose pas Static et ne publie pas
cet état. Aucun rappel réentrant ou changement de l’API partagée n’est requis.

## Fichiers et fonctions touchés

| Zone | Changement |
|---|---|
| `D5NextProtocol.{h,cpp}`, `D5NextRGBpx.{h,cpp}` | Suppression des constructeurs/auditeurs historiques ; type HSV explicite. Le codec RGBpx, ses contrôles CRC/BE et sa politique de patch ne sont pas refondus. |
| `D5NextOwnership.{h,cpp}` (nouveaux) | Handle HID RAII, réservation chemin/série, mutex par périphérique réservé. |
| `AquaComputerD5NextController.{h,cpp}` | Prend la réservation et le handle par transfert explicite ; ferme le handle sous verrou ; la construction normale conserve `layout=UNKNOWN`, firmware=0, écriture interdite. La construction à firmware connu n’existe que sous `OPENRGB_D5NEXT_OFFLINE_TESTS` et exige une série. |
| `AquaComputerD5NextControllerDetect.cpp` | Acquisition avant `hid_open_path`, ownership des allocations, insertion sûre. Macro de détection toujours conditionnée par l’option expérimentale ou le test hors ligne. |
| `RGBController_AquaComputerD5Next.{h,cpp}` | Propriété RAII du contrôleur, seul mode non pilotable `Read only`, callbacks inertes. |
| `tools/AquaComputerD5Next/D5NextPinkTest.cpp` | Retrait de la construction historique et de l’appel d’écriture irréalisable ; refus explicite de transition. |
| `tests/AquaComputerD5Next/` | Régressions de politique, vecteur littéral de 70 octets, couleur de fond Blink1, multi-instance, état refusé, injection de pannes et macro réelle de détection simulée. |

`OpenRGB.pro` inclut récursivement les `.cpp` de `Controllers/`, ce qui inclut
`D5NextOwnership.cpp` et `D5NextEffects.cpp`. `D5NextTests.pro` les inclut
explicitement pour ses trois configurations. Le build du diagnostic et celui
du programme rose compilent aussi avec les interfaces modifiées.

## Tests et portée des chiffres

| Vérification hors matériel | Avant 6C | Après 6C | Portée |
|---|---:|---:|---|
| Codec RGBpx + CSV | 742 189 assertions | **744 636 assertions** | Vecteur BE16/signed16/bindings/HSV littéral, refus I1, Blink1.Background H/S, CRC et préservation des 809 octets. |
| Contrôleur natif + scan simulé | 45 033 assertions | **16 765 assertions** | L’ancienne batterie massive du constructeur divergent a été retirée ; le chiffre inférieur n’est pas une régression de couverture mesurée. Transactions fake HID, réservations, deux pompes, refus Rainbow → Static, scans répétés. |
| Gardes de production | 158 assertions | **162 assertions** | Aucune macro de détection/test ; toute tentative de GET/SET dans le backend fake échoue immédiatement. |
| Détection expérimentale sans macro offline | absent | **202 assertions** | Macro `OPENRGB_D5NEXT_READ_ONLY_DETECTION` réelle, permission simulée, 23 rangs d’allocation, 16 après ouverture, fermeture/réservation vérifiées, aucun GET/SET. |
| Diagnostic lecture seule | 8 264 assertions | **8 264 assertions** | Syscalls remplacés, aucun hidraw réel. |
| Test rose, production / identité synthétique | 274 269 / 208 733 assertions | **274 300 / 208 764 assertions** | Syscalls remplacés, aucune écriture. |

Ces nombres sont des **assertions exécutées**, pas autant de scénarios
indépendants. La suite du codec parcourt notamment les 65 536 valeurs 16 bits
et les 809 positions de rapport ; beaucoup d’assertions partagent donc le même
générateur. Les vecteurs littéraux et le CSV apportent un oracle distinct pour
les champs importants. `AuditPatch` reconstruit encore le résultat attendu via
`BuildPatch` ; il faut maintenir une revue indépendante avant une écriture.

Mesure `gcov -b` après exécution de la suite pure RGBpx avec GCC 13.3.0,
`-O0 --coverage` : `D5NextRGBpx.cpp` **94,66 % des 206 lignes**, **98,82 % des
169 branches exécutées**, **82,84 % des 169 branches prises au moins une fois**.
`D5NextEffects.cpp` : 100 % des 30 lignes ; `D5NextSystemCommands.cpp` : 100 %
des 11 lignes. La même exécution pure ne couvre que 41,46 % des 82 lignes de
`D5NextProtocol.cpp`, car elle ne teste pas son ancien décodeur ni tous les
résultats de framing ; les autres suites le sollicitent, mais aucune couverture
fusionnée n’est revendiquée. La couverture de la découverte et du cycle de vie
n’a pas été chiffrée en pourcentage : les 23 rangs d’allocation sont des essais,
pas une métrique de branches.

ASan/UBSan : suites RGBpx, diagnostic, rose et détection expérimentale passent.
La suite native avec sanitizer strict s’arrête encore sur le défaut préexistant
`RGBControllerInterface.h:287` (`nonnull-attribute`, buffer vide). Avec la
suppression existante propre à ce fichier, elle passe avec 16 765 assertions.
Cette suppression n’a pas été élargie ; `RGBControllerInterface.h` n’a pas été
modifié. Le défaut strict reste à corriger séparément.

## Compilation complète et risques restants

Environnement : Ubuntu 24.04.5 LTS, QMake 3.1, Qt 5.15.13. Qt de développement
est installé. `pkg-config` ne trouve ni `libusb-1.0`, ni `hidapi-hidraw`,
`hidapi-libusb` ou `hidapi`; les paquets de développement `libusb-1.0-0-dev`,
`libhidapi-dev` et `libmbedtls-dev` ne sont pas installés. QMake lancé dans
`.build/d5next-lot6c/full/` quitte avec code 3 sur libusb avant tout Makefile.
Le build complet et son linkage restent **non validés**. Proposition à examiner
et exécuter séparément par l’utilisateur, sans installation automatique ici :

```bash
sudo apt update
sudo apt install libusb-1.0-0-dev libhidapi-dev libmbedtls-dev
mkdir -p .build/d5next-full
cd .build/d5next-full
qmake ../../OpenRGB.pro
make -j2
```

La compatibilité des en-têtes `mbedtls` 2.28 fournis par Ubuntu 24.04 avec
l’ensemble de ce dépôt n’a pas été vérifiée : QMake s’arrête avant ce stade.
La compilation des composants D5 et des outils séparés a réussi ; elle ne
remplace pas le build complet.

La détection réelle est toujours désactivée par défaut. Le détecteur n’envoie
aucun rapport pendant le scan ; l’adaptateur n’a pas de callback d’écriture.
La construction normale du contrôleur refuse tout Feature avant même un GET.
Le seul chemin `hid_send_feature_report` reste derrière les gardes de firmware,
layout, propriété, lecture fraîche et audit. Aucun Save 0x02, Output, reset ou
répétition n’a été ajouté. L’identité réelle du firmware, l’exclusivité contre
Aquasuite ou un autre client, la sûreté des transitions et la persistance du
rapport 0x03 ne sont pas validées ici. **Toute écriture matérielle reste NO-GO.**

L’intégration logicielle peut continuer sur cette base, sous réserve du build
complet et d’une revue humaine des métadonnées de propriété, du cycle de vie
OpenRGB et des dépendances avant toute activation de la détection hors essais.
