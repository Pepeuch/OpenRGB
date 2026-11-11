# D5 NEXT — lot 7B : catalogue avancé descriptif

11 novembre 2026, branche `feat/d5next`, référence HEAD `e565fa29`.
Référence principale : [inventaire 7A](D5NextLot7EffectsInventory.md).
L'arbre initial ne contenait que ce rapport 7A non suivi ; il a été conservé.
Aucun accès HID/USB réel, changement de permissions, lancement d'OpenRGB,
SettingsSave matériel, commit, push ou PR pendant ce lot.

**Catalogue actualisé : 23 modes sélectionnés, aucun support matériel activé.**
Les dix modes avancés restent refusés par le constructeur de patches, y compris
pour les paramètres dont la variation isolée est documentée. L'adaptateur reste
Read only, la découverte désactivée par défaut, et les propriétaires RAII,
réservations de périphérique et gardes firmware/identité/transactions sont inchangés.

## Audit des preuves

Sources recoupées : `D5NextEffects.cpp`, `D5NextRGBpx.{h,cpp}`, les CSV
[paramètres avancés](D5NextRGBpxAdvancedParameters.csv) et
[événements avancés](D5NextRGBpxAdvancedUSBEvents.csv), les rapports
[Sound/Bulk](D5NextRGBpxStreamingAnalysis.md),
[corrélation06](D5NextRGBpxAdvancedVideoUSBMapping.md),
[mires AMBIENTpx](D5NextAmbientSlideshowAnalysis.md),
[options](D5NextAmbientDisplayOptionsAnalysis.md),
[réglages](D5NextAmbientSettingsAnalysis.md),
[quadrants](D5NextAmbientQuadrantsAnalysis.md) et
[sortie13 LED](D5NextExternal13LEDAnalysis.md).

Recomptage du CSV avancé : **52 attributions**, soit44 variations isolées et5
confirmations par plusieurs observations indépendantes, donc49 confirmées ;
les3 autres restent compatibles avec des variations couplées. Répartition des
49 : **28 values[],17 flags,2 filtres,2 rôles H/S**. Les74 lignes Feature03
du CSV Events couvrent exactement les dix modes avancés, toutes de809 octets
et marquées CRC OK. Les14 configurations du CSV External13LED conservent un
g7 actif et des CRC reçu/calculé égaux.

Les captures brutes citées restent absentes de cet espace : il s'agit d'un
recoupement des données suivies et de leurs preuves de trames, pas d'une nouvelle
analyse indépendante des PCAPNG. Les vecteurs ajoutés sont entièrement synthétiques.

## Métadonnées et distinction des niveaux de preuve

Les champs historiques de `D5NextEffectInfo` sont conservés. `captured` signifie
désormais explicitement **mode sélectionné dans une acquisition** : il vaut vrai
pour les23 effets, faux pour `00 Unused`. `06` reste absent et `00` n'est pas Off.

Une extension minimale ajoute :

- `host_dependency` : `UNKNOWN`, `PROBABLE`, `OBSERVED`. Sound1–4 sont probables
  (flux35 associé, mais corps nul) ; AmbientPx est observé pour le comportement
  lié aux mires et au flux364. Les autres effets restent inconnus, sans déduire
  leur autonomie de l'absence de flux dans une fenêtre de capture.
- `production_editable` : faux pour **toutes** les entrées. Ce marqueur descriptif
  n'accorde aucune permission ; les gardes réelles restent dans le contrôleur.
- `source_filters` : deux descriptions, indices0/1, pour Sound2 seulement.
  Les autres champs inconnus restent absents des tables ; une recherche renvoie
  `nullptr`, sans inventer un rôle à partir d'un zéro stocké.

`D5NextFlagInfo` porte aussi un niveau de preuve, avec une valeur par défaut
`ISOLATED` correspondant aux anciens flags déjà confirmés. Les trois paramètres
couplés restent `COMPATIBLE`. Tous les `range_known` restent faux : les tableaux
de valeurs ne contiennent que des échantillons observés, jamais des bornes UI.
Le booléen historique `requires_pc_transport` est conservé pour compatibilité
source et prudence ; la nouvelle qualification hôte est plus précise. Les
nouveaux champs ont des valeurs par défaut, ce qui conserve les anciens
initialiseurs agrégés ; il faut recompiler les utilisateurs de ces structures,
aucune compatibilité binaire n'est revendiquée.

| Mode | Ajouts confirmés | Couplés / inconnus conservés |
|---|---|---|
| `0D Bar1` | Sélection9675 | Source réelle, seuils, bindings, flags et palettes non isolés |
| `0F Rain1` | v0 speed, v1 items, v2 size, v3 smoothness ; flags0002 Reverse,0008 Random,0010 Snow | Palette Background/Color et v4–v11 non attribués |
| `10 Rain2` | Mêmes quatre champs et trois masques, confirmés séparément | Aucun transfert implicite de palette depuis Rain1 |
| `11 Rain3` | Mêmes quatre champs et trois masques, confirmés séparément | Palette et autres values inconnus |
| `12 Switch` | Sélection8047 | Source, seuils, flags, bindings, palettes inconnus |
| `14 Sound1` | Sélection25941 ; hôte probable | Aucune attribution de values/flags/couleurs isolée ; sources UI sans Feature03 supplémentaire |
| `15 Sound2` | v0 minimum, v4 première limite, v7 Peak hold ; filtres0 Rising/1 Falling ; HSV1 pic/HSV2 première plage H/S ; flags0001 Fade ranges,0002 Show bar,0004 Show ranges,0008 Reverse,0020 Peak | v1 maximum, v3 nombre de limites, v5 seconde limite **COMPATIBLE** ; V, fond, autres seuils et règles de clamp non généralisés |
| `16 Sound3` | v0–v3 sous-modes1–4, v4–v7 vitesses1–4, v8 Rotate color | Les échantillons ne décrivent pas une plage continue ; menus2–4 incomplets ; v9–v11 et palettes inconnus |
| `17 Sound4` | v0 Rotate, v2 Effect2 length, v3 Idle, v4 Activity ; flags0001 Reverse,0002 Random1,0004 Random2 | v1 Effect1 length non ajouté : pas d'attribution isolée dans les52 lignes ; autres values et palettes inconnus |
| `18 AmbientPx` | Sélection47291 ; hôte observé | Aucun offset Feature03 ajouté pour les réglages vidéo ; transport/routage général non implémentés |

Le catalogue totalise maintenant **64 descriptions values[]** (33 anciennes +31
nouvelles, dont28 isolées et3 couplées), **43 associations effet/flag**, **4 rôles
de couleur** et **2 filtres**. Les noms des nouvelles descriptions suivent le
CSV, y compris ses libellés français. Une trame mémorisée est la première
référence du couple ; les autres références restent dans le CSV.

## Métadonnées et politique d'édition

Avant ce lot, `BuildPatch` utilisait `captured` comme l'un de ses filtres.
Actualiser ce seul booléen aurait permis des patches sur Rain, Bar1 ou Switch.
Une fonction locale `HasReviewedPatchPolicy` énumère désormais explicitement les
**13 anciens modes du lot6**, en plus des gardes existantes. Le catalogue seul
ne peut donc pas étendre cette politique. Les dix nouveaux modes refusent tous
les types de patch avec `UNSUPPORTED_OPERATION`, ainsi que leur audit ; la
sortie de l'appelant reste intacte.

Cette liste permet seulement les anciens calculs hors ligne et transactions
simulées ; elle n'est pas une liste de firmwares autorisés à écrire. La
construction normale du contrôleur refuse toujours l'accès Feature avant GET.
Aucun nouveau contrôle n'est exposé dans l'adaptateur OpenRGB. Les attributions
couplées, les unités, les nombres de couleurs et les bornes inconnues ne sont
pas transformés en commandes éditables.

## Tests exécutés et indépendance des oracles

| Vérification | Résultat |
|---|---|
| `run-rgbpx.sh` normal (7B.1) | **838897 assertions, PASS** |
| `run-rgbpx.sh --sanitize` (7B.1) | **838897 assertions, PASS ASan/UBSan**, sans suppression |
| `run.sh` | **16765 assertions, PASS**, backend HID uniquement en mémoire |
| `run-production.sh` | **162 assertions, PASS**, macros de test/détection absentes et appels Feature interdits |
| `run-detection.sh` | **202 assertions, PASS**, vraie macro expérimentale, syscalls/backend simulés,23 échecs d'allocation dont16 après ouverture ; aucun GET/SET |
| `run.sh --sanitize-known-baseline` | PASS16765, suppression existante du fichier partagé uniquement |
| `run.sh --sanitize` strict | Échec préexistant `RGBControllerInterface.h:287`, argument nul à `memset` pour matrice vide ; API partagée et suppression inchangées |
| Diagnostic séparé `tools/AquaComputerD5Next/build.sh` | Compilation réussie, programme non exécuté |

Les nombres sont des assertions, pas autant de scénarios indépendants ; aucune
nouvelle mesure de couverture en pourcentage n'est revendiquée. Le build complet
d'OpenRGB n'a pas été validé par ce lot.

Les52 lignes du CSV avancé sont confrontées aux noms, indices, masques, premières
trames et niveaux de preuve C++. Des oracles **littéraux séparés** couvrent les
28 paramètres isolés (ID, index, offset exact, avant/après), les17 masques, les
deux filtres et les deux rôles H/S. Les tableaux produits depuis les faits CSV
ne constituent donc pas l'unique oracle. Les champs signés incluent les octets
littéraux `80 00 → -32768` et `FF FE → -2`, en plus du parcours existant de toutes
les valeurs16 bits.

Les tests décodent des rapports synthétiques à offsets littéraux, réencodent le
groupe70 octets, comparent les70 octets puis les809 octets et le CRC avec un
calcul bit à bit indépendant. Les suites existantes conservent les tests de
longueurs, sorties intactes, overlap, corruption à chacune des809 positions et
refus des firmwares inconnus. Les dix modes avancés sont refusés pour **chaque
type de patch**, ce qui protège aussi les trois paramètres couplés.

`tests/AquaComputerD5Next/DocumentaryBulk.h` est limité au code de test : aucun
include de production, encodeur ou transport. Son lecteur35 n'accepte que la
forme observée `0A +32 zéros +CRC`, vérifiée avec un CRC d'oracle indépendant ;
un corps non nul reste inconnu et est refusé. Son lecteur364 accepte uniquement
l'en-tête observé `0B 06 06 00`, six mots `00 G R B` et le suffixe nul observé.
Les tests utilisent des primaires synthétiques et vérifient tailles, marqueurs,
octets réservés et sortie intacte sur erreur. Cela décrit les captures, pas un
codec universel : le zéro n'est pas appelé alpha et les deux06 ne sont pas
interprétés comme un nombre de LED.

## Groupes0/6/7 et montage externe

La capture11 distingue `g0` (Strip,13 LED déclarées,strip0,start0,count13) et
`g6` (Pump,six LED,strip1,start0,count6). `g7` est aussi actif,mode02,et recouvre
les six positions logiques de g6 ; son rendu et sa priorité sont inconnus.
Les états ne sont pas généralisés à tous les firmwares. Les tests synthétiques
relisent ces trois groupes et refusent toujours les patches des groupes0/7.
Le routage de production reste limité à g6 ; le commentaire décrivant g7 comme
inutilisé a été corrigé. Sa présence n'est pas une preuve de sécurité d'une
future écriture sur un groupe recouvrant : la priorité reste à établir.

Le passage aquabus → RGBpx est confirmé par le contexte utilisateur et l'état
de la sortie au début de capture11. Son changement n'est pas isolé sur USB :
aucun bit de bascule n'est ajouté au catalogue. La capture11 ne contient aucun
Bulk ; elle ne permet donc pas de remplacer l'en-tête AMBIENTpx06/06 par13/19.

## Acquisitions ciblées nécessaires avant une implémentation active

Ces propositions nécessitent un **GO matériel distinct**. Dans chaque session,
capturer le premier état complet du mode, laisser les initialisations se
stabiliser, puis faire une seule modification RGB ; comparer les huit groupes,
CRC et champs opaques. Aucun changement de source de commande hydraulique,
de courbe de pompe ou de protection thermique. Une source température choisie
ci-dessous est exclusivement une source de couleur dans le panneau RGBpx.

| Cible | État initial → final proposé | Preuve recherchée |
|---|---|---|
| Bar1 et Switch, séparément | Data source None → température d'eau D5, si offerte dans la liste RGB | Source signée et éventuels bindings/seuils, sans copier la sémantique Sound2 |
| Rain1/2/3, séparément | Background rouge `#FF0000` stabilisé → vert `#00FF00`, paramètres et flags constants | Indice HSV, H/S/V, absence de changement master ou autres palettes |
| Sound2 | Maximum100 →80, puis autre séquence80 →20 avec seconde limite initiale55, si accepté par l'UI | Variation isolée de v1 puis éventuel clamp de v5 ; distinguer couplage et règle générale |
| Sound3 | Effect2 sous-mode0 →5, autres menus fixes, si proposé | Confirmer le menu2 sans extrapoler les six choix du menu1 |
| Sound4 | Effect1 length2 →6, si accepté | Identifier v1 indépendamment de v2 (Effect2) |
| Audio | Source PC stable, silence → signal synthétique gauche à50 %, puis silence | Déterminer si le corps32 du Bulk35 quitte zéro ; aucun algorithme audio à déduire d'un flux nul |
| AMBIENTpx | Mire six régions connues, Reverse OFF →ON, autres réglages fixes | Ordre des mots et effet PC, sans attribuer un flag Feature03 arbitraire |
| Sortie13 LED et AMBIENTpx | Nombre puis groupe variés dans deux sessions distinctes, contenu synthétique actif | Rôle des deux06, routage externe/interne ; observer les groupes recouvrants sans en supposer la priorité |

Les limites UI, unités, états de départ des clamps, mémoire de mode, persistance,
exclusivité contre les logiciels concurrents et comportement des recouvrements
restent à valider avant une éventuelle implémentation active. **GO pour la revue
logicielle de ces métadonnées ; NO-GO matériel maintenu.**

## Vérification 7B.1 : catalogue effectivement compilé

L'inspection Git retrouve les cinq fichiers suivis modifiés du lot7B :
`D5NextEffects.cpp`, `D5NextRGBpx.{h,cpp}`, `RGBpxTests.cpp` et `run-rgbpx.sh`.
Les rapports7A/7B et `DocumentaryBulk.h` sont non suivis et conservés.
Les métadonnées annoncées sont présentes dans le code de production, sans
correction supplémentaire nécessaire :

| Déclaration réelle dans D5NextEffects.cpp | Paramètres | Flags | Autres métadonnées |
|---|---:|---:|---|
| `Rain1_parameters` ligne195, `Rain1_flags` ligne202 | 4 isolés | 3 | Référencés par l'entrée0F ligne332 |
| `Rain2_parameters` ligne212, `Rain2_flags` ligne219 | 4 isolés | 3 | Entrée10 ligne333 |
| `Rain3_parameters` ligne229, `Rain3_flags` ligne236 | 4 isolés | 3 | Entrée11 ligne334 |
| `Sound2_parameters` ligne250, `Sound2_flags` ligne269 | 3 isolés +3 couplés | 5 | `Sound2_filters` ligne259 (2 filtres), `Sound2_colors` ligne264 (2 rôles H/S), entrée15 ligne338 |
| `Sound3_parameters` ligne286 | 9 isolés | 0 | Entrée16 ligne339 |
| `Sound4_parameters` ligne302, `Sound4_flags` ligne309 | 4 isolés | 3 | Entrée17 ligne340 |

Total avancé : **31 paramètres,17 flags,2 filtres,2 rôles de couleur**.
La table `effects[]` commence ligne316 et contient les pointeurs et les comptes
effectifs. Elle déclare aussi les qualifications hôte : Sound1–4 `PROBABLE`,
AmbientPx `OBSERVED` (ligne341), les autres `UNKNOWN`. Les types et champs sont
dans `D5NextRGBpx.h`. `Find()` ligne354 parcourt cette table ; `Parameter()`
ligne366 et `Flag()` ligne382 parcourent les membres de l'entrée trouvée.
`Color()` ligne398 et les accès `source_filters` utilisent ces mêmes entrées.

`TestLinkedCatalogueIntegrity()` interroge maintenant ces fonctions et la table
liée, **sans ouvrir de CSV ni construire de fixture**. Il vérifie les indices
attendus de chaque mode, l'unicité, les comptes exacts, l'identité des pointeurs
renvoyés par `Find`/`At`/`Parameter`/`Flag`/`Color`, les échantillons des deux
filtres, les niveaux isolé/couplé, les qualifications hôte et l'absence de support
éditable de production. Les indices ou masques non décrits doivent rester absents.
Ce contrôle s'exécute dans la suite et peut être lancé seul après compilation :

```bash
bash tests/AquaComputerD5Next/run-rgbpx.sh
.build/d5next-rgbpx/rgbpx-tests --catalogue-integrity
```

Résultat isolé : **608 assertions PASS**, avec le résumé31/28/3/17/2/2.
La suite entière passe avec **838897 assertions**, normale et ASan/UBSan.
La cible de production repassée donne162 assertions PASS, appels HID interdits.
Le script compile explicitement le vrai `D5NextEffects.cpp` avec le codec ;
`nm -C` sur le binaire sanitizer confirme les symboles définis `Find`,
`Parameter`, `Flag` et `Color`. Il ne s'agit pas d'un catalogue de remplacement
fourni par le test. Les assertions du lot7B étaient838289 avant cet ajout.

Seuls le test et ce rapport sont modifiés pour7B.1. Le catalogue, la politique
restrictive, les zones, les transitions et les protections matérielles ne sont
pas modifiés. Aucun accès matériel, commit ou push.
