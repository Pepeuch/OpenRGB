# D5 NEXT — préparation du premier essai rose fixe

Date : 10 novembre 2026, Europe/Paris. Branche : `feat/d5next`.

**Préparation terminée, sans écriture matérielle.** Le firmware réel a été lu
passivement : **1027 (`0x0403`)**. Une lecture fraîche de `0x03` a confirmé les
809 octets, le CRC `0xEFD1` et l'état Rainbow communiqué par l'utilisateur. La
configuration rose construite en mémoire aurait le CRC **`0xF860`**, avec
exactement six octets différents. Aucun firmware n'a été approuvé pour écrire.

## Audit réalisé

Les sources, les rapports des lots 1/2 et le protocole fourni dans
`aquapx-main/re/PROTOCOL.md` ont été relus. Le codec, le contrôleur matériel,
l'adaptateur OpenRGB et les barrières de détection existants n'ont pas été
modifiés. L'outil ajouté est distinct de l'application et de sa découverte.

Une seule exécution matérielle a été effectuée, dans le mode par défaut de
simulation, avec confirmation de disponibilité d'aquasuite :

```sh
.build/d5next-pink/d5next-pink-test --recovery-aquasuite-confirmed
```

Cette commande a découvert automatiquement `0C70:F00E`, interface HID `1`.
Le nœud observé était `/dev/hidraw1`, `0440 root:plugdev`, accessible en lecture.
Cette permission avait changé depuis le lot 2 ; l'agent n'a installé aucune
règle ni modifié les permissions. Le chemin n'a pas été fourni manuellement.

La transaction a utilisé un seul handle `O_RDONLY`, un verrou `flock` non
bloquant et un mutex couvrant toute la préparation. La trace confirme :

1. Ouverture du nœud en lecture seule et acquisition du verrou.
2. `HIDIOCGRAWINFO`, avec contrôle USB/VID/PID et identité du nœud ouvert.
3. Un seul `poll`, limité à 2 secondes, et une seule lecture passive de
   **162 octets**, rapport Input `0x01`.
4. Un seul `GET_FEATURE(0x03)`, retournant **809 octets**.
5. Fermeture du handle. **Zéro SET_FEATURE, Output, sauvegarde ou reset.**

Il n'y a eu ni boucle RGB, ni nouvelle tentative après erreur, ni restauration.
Le rapport complet et sa copie candidate n'ont jamais été écrits sur disque.
La trace utilise le format brut des appels `read`/`ioctl`, qui journalise leurs
paramètres numériques et non le contenu des buffers contenant des informations
privées. Le résumé ne contient ni numéro de série ni réglage hydraulique.

L'exécution a précédé le durcissement final du blocage sur inventaire de processus
incomplet et sur échec de publication de l'audit. Ces barrières supplémentaires
ont ensuite été validées hors ligne ; aucune seconde lecture matérielle n'a été
effectuée pour les retester. Le chemin de simulation et les octets audités sont
identiques.

## État initial et différentiel exact

L'état initial lu est identique à celui annoncé par l'utilisateur : luminosité
255, activation `0x00`, effet `0x03`, hue 0, saturation 255. L'utilisateur a
confirmé visuellement Rainbow ; l'agent n'a pas effectué d'observation visuelle
indépendante.

La couleur `#FF69B4` correspond à RGB `(255,105,180)`, soit OpenRGB
`0x00B469FF`. Le codec existant produit H=1408 (`0x0580`), S=150 (`0x96`), V=255.
Le sélecteur prévu est Static `0x01`. Luminosité et activation restent inchangées.

| Offset décimal, ID inclus | Champ | Initial | Copie rose |
| ---: | --- | --- | --- |
| 237 | Luminosité | `FF` | `FF`, inchangé |
| 239 | Activation | `00` | `00`, inchangé |
| **663** | Effet | **`03`** | **`01`** |
| **706** | Hue, octet fort | **`00`** | **`05`** |
| **707** | Hue, octet faible | **`00`** | **`80`** |
| **708** | Saturation | **`FF`** | **`96`** |
| **807** | CRC, octet fort | **`EF`** | **`F8`** |
| **808** | CRC, octet faible | **`D1`** | **`60`** |

**Nombre de différences : 6. Offsets : `663, 706, 707, 708, 807, 808`.**

CRC initial reçu/calculé : `0xEFD1` / `0xEFD1`. CRC de la copie calculé sur
`[1,807)` : `0xF860`, stocké big-endian. Les **803 autres octets** restent
strictement identiques à la lecture fraîche, y compris tous les octets opaques
et les paramètres d'effet 674–689. L'audit compare les 809 octets, valide le CRC
des deux rapports, puis exige cette modification rose exacte et son masque
restreint. Même les offsets RGB 237/239 sont interdits de modification pour cet
essai puisqu'ils sont déjà à la valeur requise.

Ce candidat provient exclusivement d'une copie du rapport fraîchement reçu. Il
n'est ni reconstruit depuis des zéros, ni chargé depuis un ancien dump. Une
configuration initiale différente du Rainbow confirmé est refusée et nécessite
une nouvelle revue. Le CRC `0xF860` décrit cette observation ; il n'est pas une
constante à rejouer lors d'un futur essai. Toute invocation doit relire et
recalculer son propre candidat.

Le résultat visuel attendu est un rose uniforme sur les six LED intégrées.
**Il n'a pas été testé**, et aucun contrôle individuel des LED n'est revendiqué.

## Firmware et preuves de compatibilité

La version **1027** provient des octets 13–14 big-endian du rapport périodique
Input `0x01`, pas de `release_number` ou de `bcdDevice`. La longueur et l'ID de ce
rapport ont été vérifiés. Ce champ correspond à celui que le pilote installé
`aquacomputer_d5next` emploie pour sa version firmware et son interface debugfs.
Le pilote et sa source installée avaient été examinés au lot 2. La documentation
[du pilote Linux](https://docs.kernel.org/hwmon/aquacomputer_d5next.html) confirme
que cette information est normalement exposée dans debugfs ; ce répertoire
reste inaccessible à l'utilisateur, sans tentative d'élévation.

| Preuve disponible | Conclusion permise | Conclusion non permise |
| --- | --- | --- |
| Version passive 1027, ID/longueur Input vérifiés | Identification du firmware observé | Approbation automatique des offsets de configuration |
| Descripteur et GET de 809 octets | Framing observé de `0x03` | Tous les champs du rapport sont compris |
| CRC reçu/calculé `EFD1` | Couverture/ordre CRC cohérents avec le codec sur cette lecture | Innocuité d'une écriture de configuration |
| Effet `03` et Rainbow confirmé visuellement | Indice concret pour le sélecteur dans cet état | Validation de Static, de la fenêtre HSV ou des six LED sur 1027 |
| Référence aquapx avec offsets RGB | Hypothèse de travail attribuée et testable | Contrat firmware/layout établi pour 1027 |

La recherche a inclus les références publiques de l'[issue OpenRGB 2111](https://gitlab.com/CalcProgrammer1/OpenRGB/-/issues/2111),
le [manuel constructeur](https://aquacomputer.de/tl_files/aquacomputer/downloads/manuals/D5_NEXT_english.pdf)
et le [changelog constructeur consulté](https://forum.aquacomputer.de/weitere-foren/history-produktank-ndingungen/110868-aquasuite-changelog-english/).
Ces documents consultés n'apportent pas de contrat des offsets RGB pour le
firmware 1027. Une tentative de récupération de la capture rouge référencée dans
l'issue a reçu HTTP 403 : aucune capture n'a pu être analysée et aucune identité
firmware n'en a été déduite. Les sources aquapx locales ne précisent pas une
matrice de compatibilité par firmware. Aucun code aquapx n'a été exécuté ou copié.

**État : lecture cohérente observée sur 1027 ; compatibilité d'écriture non
validée.** La politique de production refuse toujours toutes les versions,
y compris 1027. Il n'existe pas d'option `--force`, de paramètre firmware manuel
ou d'approbation implicite par CRC/longueur/descripteur.

## Concurrence, permissions et blocages

Aucun processus connu de contrôle ni handle lisible pointant vers le nœud HID
ou son nœud USB correspondant n'a été trouvé dans l'inventaire. Toutefois,
**398 répertoires de descripteurs de processus n'étaient pas inspectables**.
L'absence de clients cachés/non coopératifs n'est donc pas démontrée.

Le mutex sérialise un objet transaction ; l'objet ne permet qu'une tentative.
`flock` reste détenu sur le même handle pendant la lecture passive, le GET,
l'audit et le SET unique éventuel. Il ne protège pas les logiciels qui ignorent
ce verrou, les autres backends ou les opérations de contrôle du pilote noyau.
Les attributs PWM/offsets/contrôles hwmon ne sont pas lus, car certaines lectures
de contrôles peuvent elles-mêmes demander un rapport de configuration.

Les blocages d'écriture conservés ou ajoutés sont :

- Aucun firmware/layout approuvé en production.
- Inventaire de handles incomplet ; cet état empêche désormais le SET éventuel.
- Nœud `0440`, sans permission d'ouverture `O_RDWR`. L'outil exige cette
  permission en mode écriture, même si certains ioctls peuvent techniquement
  fonctionner sur un handle de lecture. Il ne cherche pas à contourner ce choix.
- Confirmation de disponibilité d'aquasuite requise pour l'option d'écriture.
- État initial, framing, ID, CRC et audit strict obligatoires.
- Échec d'affichage/flush de l'audit : abandon avant SET.

L'existence d'un moyen de récupération et l'absence de sauvegarde `0x02` ne
prouvent pas que `0x03` est volatil. Une persistance interne reste possible.
La récupération sans effets sur les autres réglages n'a pas été démontrée.
Ces risques doivent être revus explicitement avant tout premier essai autorisé.

## Récupération et arrêt : procédure proposée, non exécutée

L'utilisateur confirme qu'aquasuite fonctionne avec cette pompe. Le manuel
constructeur, section 12, place les réglages d'éclairage dans RGBpx, décrit un
contrôleur dédié aux six LED intégrées et indique leur indisponibilité dans le
menu physique. Il n'y a donc pas de procédure inventée consistant à remettre
Rainbow avec les boutons de la pompe.

Pour une anomalie limitée à l'éclairage, avec pompe fonctionnant normalement :

1. Terminer l'outil et s'assurer que son handle est fermé ; ne pas le relancer.
2. Décider séparément de la récupération opérateur dans aquasuite. Si elle est
   autorisée, sélectionner D5 NEXT → RGBpx → contrôleur des LED intégrées,
   remettre uniquement l'effet Rainbow et sa luminosité précédente. Ne pas
   importer de profil complet, ouvrir les réglages hydrauliques, réinitialiser
   l'appareil ou mettre à jour le firmware.
3. Vérifier visuellement l'éclairage et le fonctionnement de la pompe. Cette
   procédure n'a pas été répétée dans ce lot puisqu'elle modifierait le matériel.

**Limite essentielle :** aquasuite peut envoyer ses propres commandes de
sauvegarde et modifier la configuration persistante. Son accès confirmé fournit
une solution opérateur possible, pas une récupération garantie sans rapport
`0x02`. Si l'interdiction de sauvegarde doit également couvrir cette récupération,
la voie aquasuite n'est pas démontrée conforme et doit être revue avant un GO.
Il n'y a aucune restauration automatique ni rejeu d'un ancien rapport `0x03`.
Un retour d'aquasuite peut aussi dépendre d'une communication USB encore valide.

Pour une anomalie de pompe/refroidissement, une alarme ou une température qui
augmente anormalement : arrêter immédiatement la charge et l'essai, ne pas
envoyer d'autre commande RGB, et suivre la procédure d'arrêt sûre du PC/du
circuit. Ne pas poursuivre une charge avec un refroidissement défaillant et ne
pas couper seulement la pompe en laissant cette charge active. Aucun reset USB,
cycle d'alimentation présenté comme restauration, essai de firmware ou nouvelle
tentative automatique ne fait partie de cette procédure.

Un SET retournant une erreur ou une longueur inattendue devra être considéré
comme **résultat matériel incertain**, sans seconde tentative ni restauration.

## Procédure du futur essai, après nouvelle validation

1. Faire revoir la compatibilité firmware 1027/layout, l'exclusivité d'accès,
   les permissions et le plan de récupération, y compris sa persistance.
   Ne pas ajouter 1027 à une liste d'approbation sur la seule base de ce rapport.
2. Préparer aquasuite comme solution opérateur, puis fermer ses clients/services
   de contrôle et les autres logiciels susceptibles de toucher à la pompe avant
   la transaction. Résoudre les parties inaccessibles de l'inventaire par une
   méthode examinée et autorisée ; aucune élévation automatique n'est prévue.
3. Mettre le PC au repos et vérifier le fonctionnement normal du circuit. Le
   snapshot de préparation était **3867 tr/min, 29,160 °C** ; il ne constitue ni
   une consigne ni une limite universelle. Définir les critères d'arrêt à partir
   du fonctionnement habituel et des alarmes de cette installation.
4. Exécuter une seule transaction autorisée. Elle doit refaire la découverte,
   les précontrôles, l'identification passive, le GET frais et l'audit. Vérifier
   que seuls les quatre octets RGB prévus et les deux CRC changent. Ne pas
   réutiliser la copie calculée aujourd'hui.
5. Observer immédiatement les six LED : rose uniforme, sans animation. Vérifier
   le régime de pompe, les alarmes et la température via l'afficheur ou des
   snapshots hwmon en cache, sans lecteur HID concurrent. Recontrôler après un
   court délai au repos ; aucune boucle RGB ou lecture USB intensive n'est prévue.
6. À la première anomalie ou incertitude, appliquer l'arrêt ci-dessus. Un succès
   de transfert seul ne valide pas le fonctionnement hydraulique ni la portée
   exacte des paramètres. Une relecture de confirmation éventuelle devra être
   comprise explicitement dans l'autorisation suivante ; elle n'est pas
   automatiquement ajoutée après le SET par cet outil.

## Commandes et état de l'outil

Compilation, sans exécution matérielle :

```sh
bash tools/AquaComputerD5Next/build-pink.sh
```

Le mode par défaut ne fait qu'une simulation avec lectures réelles contrôlées.
L'option `--recovery-aquasuite-confirmed` atteste uniquement la disponibilité
opérateur ; elle ne valide ni le firmware ni le protocole de récupération.

**Commande réservée au futur lot autorisé, non exécutée :**

```sh
.build/d5next-pink/d5next-pink-test --write-once --recovery-aquasuite-confirmed
```

Cette commande reste bloquée avec le code et les permissions actuels. Elle
n'est pas un GO et ne permet pas de contourner les contrôles. Une fois toutes
les barrières réellement satisfaites et revues dans un lot distinct, le chemin
préparé contient un seul `HIDIOCSFEATURE(809)` pour `0x03`, sous le même verrou,
après un nouvel audit et son flush. Aucun rapport `0x02`, Output, reset, rejeu,
readback ou deuxième SET ne suit cet appel, succès ou échec.

## Validation hors ligne et inventaire

```sh
bash tests/AquaComputerD5Next/run-pink.sh
bash tests/AquaComputerD5Next/run-pink.sh --sanitize
```

Les tests utilisent exclusivement des configurations artificielles et remplacent
les appels de périphérique à l'édition de liens, y compris leurs variantes
fortifiées libc. Ils couvrent :

- 256 buffers opaques synthétiques, conservation de tous les autres octets,
  différence CRC constante `0x17B1`, refus de chaque altération du candidat,
  limites de taille, pointeurs invalides et changement de l'état initial.
- Tous les **65 536** identifiants firmware 16 bits refusés par la politique de
  production, y compris l'identité synthétique réservée aux tests.
- Permissions, verrou, identité du nœud, statut passif, timeout, CRC, erreurs de
  GET, récupération non confirmée, inventaire incomplet, perte de permission,
  échec de publication de l'audit et SET incomplet/incertain simulé.
- Un seul handle verrouillé, au plus une lecture Input, un GET et un SET fictif,
  fermeture garantie, refus de seconde invocation et appels concurrents sur un
  même objet sérialisés. Les seuls SET atteints appartiennent à ce simulateur,
  avec l'identité fictive `0xFA01`, jamais au matériel.

Résultats : **281 759 vérifications** pour la politique de production et
**216 245** pour le modèle synthétique, toutes réussies. ASan/UBSan passent
**sans suppression** : 281 790 / 216 276 vérifications ; les appels fortifiés
ajoutent ici des contrôles aux wrappers. La compilation normale utilise
`-Wall -Wextra -Wpedantic -Werror` et réussit. Aucune application OpenRGB modifiée
n'a été lancée et ses fonctions de détection/écriture n'ont pas été activées.

Fichiers ajoutés :

- `tools/AquaComputerD5Next/D5NextPinkTest.h`, `D5NextPinkTest.cpp`,
  `D5NextPinkMain.cpp`, `build-pink.sh`.
- `tests/AquaComputerD5Next/PinkTests.cpp`, `run-pink.sh`.
- Ce rapport ; le rapport historique du lot 2 reçoit seulement un lien vers lui.

Les preuves locales sont sous `.build/d5next-pink/`, ignoré par Git :
`preparation-summary.txt`, `preparation.trace`, `tests-final.log` et
`tests-final-sanitized.log`. Le répertoire est privé (`0700`), le résumé et la
trace matériels sont `0600`. Aucun dump brut n'est sauvegardé ou ajouté aux
fixtures. `aquapx-main/` est inchangé ; aucun commit, push ou PR n'a été créé.

**Fin du lot : préparation uniquement. Aucun SET_FEATURE réel n'a été envoyé.
Une nouvelle validation des préconditions reste nécessaire avant tout essai rose.**
