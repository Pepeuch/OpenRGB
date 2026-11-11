# D5 NEXT — lot 7A : inventaire des effets RGBpx

Date : 11 novembre 2026 (Europe/Paris). Branche `feat/d5next`, HEAD
`e565fa2915a10e14f5e04fa5e4e6db263f0035a2` (`docs(d5next): document
external RGBpx 13 LED USB mapping`). L'arbre et l'index Git étaient propres au
début ; seul ce rapport a été ajouté à la fin de cette analyse. Les sources
et rapports des lots 6, 6B et 6C sont présents. Aucun fichier de production,
matériel ou permission n'a été modifié ; aucune indexation, commit, push ou PR.

**Résultat principal :** le catalogue comporte 24 entrées, dont `00 Unused` et
**23 effets nommés**. Les [événements de la capture05](D5NextRGBpxVideoUSBEvents.csv)
contiennent 13 IDs de `SET_REPORT Feature03` ; les
[événements de la capture06](D5NextRGBpxAdvancedUSBEvents.csv) contiennent
exactement les dix autres. Leur union contient les 23 IDs, sans ID étranger au
catalogue. Il n'existe donc **plus dix effets jamais sélectionnés** dans les
preuves actuellement suivies. Il reste dix effets *absents de la capture05*,
dont plusieurs fonctions sont encore insuffisamment caractérisées. Une
sélection et un CRC correct ne prouvent ni le rendu physique ni la possibilité
de rejouer un état de manière sûre.

## Méthode et niveau de preuve

La liste a été recalculée à partir de `D5NextEffects.cpp`, puis comparée aux
lignes `SET_REPORT Feature03` des deux CSV d'événements, sans prendre le booléen
`captured` du catalogue comme source de vérité. La différence
`catalogue − {00} − capture05` vaut :

```text
0D Bar1, 0F Rain1, 10 Rain2, 11 Rain3, 12 Switch,
14 Sound1, 15 Sound2, 16 Sound3, 17 Sound4, 18 AmbientPx
```

Ces dix IDs sont tous présents dans la capture06. La capture11 apporte en plus
une observation du mode10 sur un groupe externe, sans variation isolée de cet
effet. `06` n'est pas déclaré ; `00 Unused` n'est pas un effet ni une commande
Off. Les noms proches ne sont jamais fusionnés : `0C Ambient` est **Color shift**,
distinct de `18 AmbientPx` ; `0F/10/11` sont trois modes, et `14–17` quatre
modes Sound. Les libellés UI proviennent de la cartographie
[métadonnées/ressources](D5NextRGBpxEngineAnalysis.md) confrontée aux vidéos
documentées ; le nom interne est celui du catalogue C++.

Dans le tableau, `vN` signifie `g6.values[N]`, entier **signé 16 bits big-endian**
à l'offset `682+2N` du Feature03 de 809 octets. Les masques sont les bits du
`g6.flags` BE aux offsets664–665, propres à chaque effet. `H/S` désigne des
composantes de palette HSV ; leur valeur `V`, le master237 et les couleurs des
LED rendues ne sont pas interchangeables. `I` signifie attribution par variation
isolée, `C` compatible mais couplée, `O` état ou mode observé, `D` seulement
déclaré. Les unités et plages UI complètes restent inconnues, même lorsque deux
valeurs numériques ont été lues.

## Inventaire exhaustif des 23 effets nommés

| ID | Nom Aquasuite / nom C++ | Feature03 groupe6 | Paramètres attribués | Flags confirmés | Couleurs et palettes confirmées | Hôte / niveau |
|---|---|---|---|---|---|---|
| `01` | Single color / `FixedColor` | 05 ; aussi11 | Aucun `vN` | Aucun | HSV0 couleur principale H/S observée ; H isolé sur Pump dans11 ; V non isolé | Pas de flux hôte observé ; O |
| `02` | Breathing / `Breathing` | 05 ; aussi11 | `v0` Speed, `v1` Intensity, `v2` Delay max., `v3` Delay min. (I) | Aucun | HSV0.H isolé sur Pump et Strip dans11 ; S/V non isolés | Pas de flux hôte observé ; I |
| `03` | Rotating rainbow / `Rainbow` | 05 | `v1` Color range (I) ; `v0` Speed (C : premier état déjà personnalisé) | `0002` Reverse direction (I) | Aucune édition de palette isolée | Pas de flux hôte observé ; I/C |
| `04` | Blinking / `Blink1` | 05 | `v0` Speed (I) | `0002` Fade in, `0004` Fade out, `0008` Random color, `0010` Slide colors (I) | HSV0.H/S = Background (I) ; V non isolé | Pas de flux hôte observé ; I |
| `05` | Color change / `Blink2` | 05 | `v0` Speed (I) | `0004` Fade, `0008` Random color, `0010` Slide colors (I) | Rôles/index de palette non isolés | Pas de flux hôte observé ; I |
| `07` | Sequence / `Sequential` | 05 | `v0` Speed, `v1` Smoothness, `v3` Delay after, `v4` Delay before (I) | `0002` Reverse, `0004` Fade, `0008` Random (I) | Rôles/index non isolés | Pas de flux hôte observé ; I |
| `08` | Scanner / `Visor1` | 05 | `v0` Speed, `v1` Smoothness, `v2` Width (I) | `0002` Reverse, `0008` Random, `0020` Color mode 2, `0040` Color change, `0080` Circular (I) | Rôles/index non isolés | Pas de flux hôte observé ; I |
| `09` | Laser / `Visor2` | 05 | `v0` Speed, `v1` Smoothness, `v2` Width (I, confirmés séparément de08) | Mêmes cinq masques que08, confirmés séparément (I) | Rôles/index non isolés | Pas de flux hôte observé ; I |
| `0A` | Wave / `Marquee1` | 05 | `v0` Speed, `v1` Smoothness, `v2` Width (I) | `0002` Reverse, `0008` Random, `0080` Circular (I) | Rôles/index non isolés | Pas de flux hôte observé ; I |
| `0B` | Color sequence / `Marquee2` | 05 | `v0` Speed, `v1` Smoothness, **`v4` Color change speed** (I) | `0002` Reverse, `0008` Random (I) | Rôles/index non isolés | Pas de flux hôte observé ; I |
| `0C` | Color shift / `Ambient` | 05 | `v0` Speed, `v1` Color range, `v2` Total area (I) | Aucun | Rôles/index non isolés | Aucun lien écran démontré ; I |
| `0D` | Bar graph / `Bar1` | 06, trame9675 | Aucun champ isolé ; source et seuils inconnus | Aucun | Sélection seulement ; rôles inconnus | Source de données non affectée dans la vidéo ; O |
| `0E` | Flame / `Candle` | 05 | **`v0` Intensity**, pas Speed (I) | Aucun | Rôles/index non isolés | Pas de flux hôte observé ; I |
| `0F` | Rain / `Rain1` | 06, trame873 | `v0` Drop speed, `v1` Drop items, `v2` Drop size, `v3` Drop smoothness (I) | `0002` Reverse, `0008` Random color, `0010` Snow (I) | Background et Color visibles ; index/H/S/V non isolés | Aucun flux hôte associé ; I |
| `10` | Snowfall / `Rain2` | 06, trame3259 ; état externe en11 | Mêmes quatre contrôles `v0–v3`, **variés séparément** (I) | Mêmes trois masques, **variés séparément** (I) | Background/Color non isolés | Aucun flux hôte associé ; I |
| `11` | Stardust / `Rain3` | 06, trame5605 | Mêmes quatre contrôles `v0–v3`, **variés séparément** (I) | Mêmes trois masques, **variés séparément** (I) | Background/Color non isolés | Aucun flux hôte associé ; I |
| `12` | Color switch / `Switch` | 06, trame8047 | Aucun seuil ou binding attribué isolément | Aucun | Rôles inconnus | Source de données non affectée ; O |
| `13` | Swiping rainbow / `RainbowShift` | 05 | `v0` Point speed, `v1` Smoothness, `v2` Point size, `v3` Color change speed, `v4` Color range (I) | Aucun | Rôles/index non isolés | Pas de flux hôte observé ; I |
| `14` | Sound flash / `Sound1` | 06, trame25941 | `v0–v11` observés à0 ; aucun rôle isolé | Aucun | Background, quatre couleurs visibles ; indices non isolés | Bulk35 associé aux panneaux audio, corps nul ; dépendance fonctionnelle non prouvée ; O |
| `15` | Sound bars / `Sound2` | 06, trame11165 | `v0` minimum source, `v4` première limite, `v7` Peak hold (I) ; `v1` maximum, `v3` nombre de limites, `v5` seconde limite (C) ; filtres source0/1 (I) | `0001` Fade ranges, `0002` Show bar, `0004` Show ranges, `0008` Reverse, `0020` Show peak (I) | HSV1.H/S couleur de pic et HSV2.H/S première plage (I) ; V et autres rôles inconnus | Source signée20 observée ; Bulk35 nul ; I/C |
| `16` | Sound slider / `Sound3` | 06, trame31365 | `v0–v3` sous-modes 1–4, `v4–v7` vitesses 1–4, `v8` Rotate color (I) | Aucun isolé | Aucun rôle isolé | Bulk35 nul ; menus Effect2–4 non parcourus entièrement ; I |
| `17` | Sound shift / `Sound4` | 06, trame40821 | `v0` Rotate color, `v2` Effect2 length, `v3` Idle speed, `v4` Activity speed (I) ; `v1` Effect1 length (C) | `0001` Reverse, `0002` Effect1 Random, `0004` Effect2 Random (I) | Aucun rôle isolé | Bulk35 nul ; I/C |
| `18` | AMBIENTpx / `AmbientPx` | 06, trame47291 | `v0–v11=0` à la sélection ; aucun réglage UI attribué à un offset Feature03 | Aucun ; Reverse UI ne change pas le dernier Feature03 | Flux Bulk364 : six mots actifs `00 G R B` liés à des mires synthétiques ; ordre physique et routage général inconnus | Traitement/flux PC observé ; O + corrélation du contenu |

Les confirmations des lignes01–13 proviennent surtout du
[CSV du lot5](D5NextRGBpxVideoUSBParameters.csv) ; celles des lignes0F–17 du
[CSV avancé](D5NextRGBpxAdvancedParameters.csv), qui contient 52 couples
effet/contrôle, dont **49 confirmés et3 seulement compatibles**. Les variantes
Sound1–4 ne partagent pas implicitement leurs masques : Reverse vaut `0002` pour
Rain, `0008` pour Sound bars et `0001` pour Sound shift. Une palette enregistrée
dans le rapport n'est pas une preuve que toutes ses entrées soient éditables ni
que leurs six positions correspondent chacune à une LED physique.
Sur les 120 emplacements `values[]` théoriques des dix effets de la capture06,
28 sont attribués par variation isolée,3 seulement par compatibilité couplée
et89 n'ont pas de rôle attribué. Cela ne signifie pas89 fonctionnalités
manquantes : plusieurs emplacements peuvent rester inutilisés.

## Transactions USB et données réellement accessibles dans cet espace

Les fichiers bruts `d5next_baseline05/06/07/08/09/10/11.pcapng` et les MP4
correspondants, cités dans les rapports, **ne sont pas présents** dans le dossier
du contrôleur, dans `.build/`, dans les fichiers parcourus de `/home/pepeuch`
ni sur le point de montage utilisateur visible. Les PCAP trouvés ailleurs dans
le répertoire personnel ne sont pas identifiés D5 NEXT et n'ont pas été ouverts.
Aucune copie de capture n'a donc été créée, et ce lot **ne revendique pas de
nouvelle vérification indépendante des charges USB brutes**. Les données
disponibles sont les rapports et CSV suivis, avec trames, tailles, différences
fonctionnelles, offsets et résultats CRC, sans rapports complets privés.

Le CSV de la capture06 contient 74 lignes `SET_REPORT Feature03` de809 octets,
réparties sur les dix IDs du tableau, chacune marquée CRC OK ; 23 lignes
`SET_REPORT Output02` de11 octets, également CRC OK ; 394 complétions Input01
de162 octets, dont le firmware1027 a été lu dans l'analyse antérieure ; et
973 opérations Bulk OUT sur l'interface vendor0. Les 327 charges de35 octets
préfixées `0A` ont un corps32 octets nul et un CRC-16/USB sur1–32. Les646
charges de364 octets préfixées `0B` varient pendant AMBIENTpx. Ces préfixes Bulk
ne sont pas des IDs de rapport HID. Aucun GET Feature03 n'est relevé dans cette
session. Le CRC Feature03 protège le rapport entier ; les octets807–808 de CRC
ne sont jamais des paramètres d'effet. Les 23 Output02 sont des sauvegardes
distinctes, non une écriture après chaque changement.

Le [rapport du streaming](D5NextRGBpxStreamingAnalysis.md) rattache le flux35
constant aux panneaux Sound et le flux364 variable au parcours AMBIENTpx. Les
[mires07](D5NextAmbientSlideshowAnalysis.md) et
[options08](D5NextAmbientDisplayOptionsAnalysis.md) permettent ensuite de
confirmer l'ordre des composantes **`00 G R B`** dans les six mots actifs ; les
[réglages09](D5NextAmbientSettingsAnalysis.md) et
[quadrants10](D5NextAmbientQuadrantsAnalysis.md) confrontent zones et bords.
Ces captures supplémentaires ne contiennent pas de nouveau Feature03 ; les
valeurs des réglages restent sans encodage HID attribué. Les deux `06` de
l'en-tête Bulk, l'ordre physique des six mots, la règle d'échantillonnage, les
gains et la persistance des préférences PC ne sont pas résolus. Pour l'audio,
aucun corps35 non nul n'est documenté : le contenu dynamique et l'autonomie des
quatre effets ne sont pas démontrés.

La [capture11 du montage externe](D5NextExternal13LEDAnalysis.md), signalée
par l'utilisateur, documente **13 LED supplémentaires** sur la sortie passée
en RGBpx. Le contrôleur `Strip / LED Controller1` est `g0`, `strip=0`,
`start=0,count=13`; `Pump / Base` est `g6`, `strip=1,start=0,count=6`.
`g7`, également sur `strip=1,start=0,count=6`, est **actif en mode02** dans
ce premier état, contrairement à l'ancienne hypothèse d'un groupe7 inutilisé.
Les 14 Feature03 et7 Output02 de la capture11 sont documentés avec CRC valide ;
aucun Bulk n'y est observé. Les modifications g0/g6 sont séparées dans les
comparaisons successives, sans démontrer la priorité de rendu entre g6/g7 ni
entre groupes externes recouvrants. La bascule **aquabus → RGBpx n'est pas
filmée ni isolée** : la sortie est déjà dans cet état au premier Feature03.
Aucun offset ou rapport de bascule ne peut être attribué à partir de cette
capture. Les 13 LED déclarées ne doivent pas être confondues avec les 90
positions logiques possibles des groupes externes.

## Écarts entre le catalogue C++ et les preuves plus récentes

1. `D5NextEffects.cpp` garde `captured=false`, aucun paramètre, flag ou rôle de
   couleur pour les dix effets de la capture06. C'était cohérent avec le lot5 ;
   c'est désormais **périmé comme inventaire descriptif**. `BuildPatch` refuse
   donc leurs éditions, ce qui reste une barrière prudente. Aucun champ C++ n'est
   modifié dans ce lot.
2. `requires_pc_transport=true` pour Sound1–4 et AmbientPx confond une exigence
   fonctionnelle avec une hypothèse de catalogue : Bulk35 est associé aux
   panneaux audio mais nul ; les preuves ne démontrent pas quel signal audio
   serait nécessaire à chaque rendu. Pour AMBIENTpx, le flux variable lié aux
   mires justifie une dépendance PC pour le comportement capturé, sans codec
   général ni preuve de persistance. Une future révision devrait distinguer
   `observé`, `probable` et `inconnu`.
3. Les documents du lot4/lot5 et du lot6 décrivent l'état de leurs propres
   acquisitions : leurs mentions « non capturé » ou « transport non établi »
   ne décrivent plus l'état actuel. La capture11 invalide aussi l'hypothèse
   documentaire d'un g7 toujours inutilisé. Ces documents historiques ne sont
   pas réécrits.
4. La [liste des 23 IDs](D5NextRGBpxEngineAnalysis.md) est conservée : aucun
   effet supplémentaire n'est justifié par des chaînes de ressources partagées
   entre appareils. La découverte, l'adaptateur Read only, la propriété RAII,
   les refus de doublons et les barrières firmware/CRC/GET/SET restent intacts.

## Captures complémentaires proposées, non effectuées

Une future campagne, **après autorisation matérielle distincte**, doit démarrer
la capture avant la sélection du mode, obtenir un premier Feature03 dans le mode
cible, laisser une pause, puis changer **une seule commande RGBpx**. Pour chaque
ligne, conserver le rapport frais précédent, comparer les809 octets et les huit
groupes, décoder les int16 BE, vérifier le CRC et enregistrer séparément les
Output02 différés. Aucun changement de courbe de pompe, de ventilateur, de
seuil thermique ou de réglage hydraulique. Le mode antérieur et les
initialisations couplées lors de la sélection ne servent pas de preuve d'un
paramètre isolé. Les valeurs UI proposées ci-dessous sont des consignes de
capture, **pas des plages validées** ; si un choix n'existe pas ou si l'état
initial diffère, consigner l'écart et ne pas substituer silencieusement une
autre commande.

| Effet | État initial contrôlé → modification unique | Événement attendu à rechercher | Contrôle contre une attribution trompeuse |
|---|---|---|---|
| `0D` Bar graph | Mode0D stabilisé, Data source **None → température d'eau D5**, uniquement si proposé dans la liste RGB | Feature03 : source signée, filtres/bindings/values éventuels ; éventuellement aucun SET | Vérifier que le capteur n'est choisi que comme **source de couleur** ; aucun réglage de pompe, seuil ou courbe ; noter les valeurs UI exactes. |
| `0F` Rain | Même mode et mêmes v0–v3/flags ; Background `#FF0000 → #00FF00`, après avoir d'abord établi l'état rouge | Feature03 : identifier quel HSV change, et si H/S/V changent seuls | Ne pas déduire l'index de Background de son nom ; comparer palette entière, master et flags. |
| `10` Snowfall | Même procédure rouge → vert, avec mode10 et état initial propre à10 | Feature03 du groupe concerné ; HSV, CRC | Ne pas transférer la correspondance de0F sans édition isolée ; contrôler le groupe externe si présent. |
| `11` Stardust | Même procédure rouge → vert, mode11 stabilisé | Feature03 HSV, ou absence de SET à documenter | Vérifier que l'option Snow et les autres flags restent constants. |
| `12` Color switch | Mode12 stabilisé, Data source **None → température d'eau D5** si proposée comme source RGB | Feature03 : source signée, bindings et seuils ; ou absence de SET | Ne pas emprunter les offsets de Bar graph ou Sound bars ; comparer les deux rapports du même mode. |
| `14` Sound flash | Source UI **None → Volume L** si proposée, avec signal audio synthétique silencieux avant/après la sélection | Feature03 et Bulk35 : présence/absence, contenu constant ou variable | Séparer changement de source, choix d'effet et émission audio ; ne pas publier de signal personnel. |
| `15` Sound bars | État vérifié `v1=100`, seconde limite `v5=55` ; Maximum source **100→80**, puis, dans une capture distincte, **80→20** si l'UI accepte | Feature03 : `v1` et éventuel clamp de `v5` | Les deux étapes sont comparées séparément ; ne pas attribuer le changement couplé de12207 à la seule limite. |
| `16` Sound slider | Mode16 stabilisé, Effect2 sous-mode **0 → 5 (All LEDs)** si offert | Feature03 : `v1` et éventuelles initialisations annexes | `v0` d'Effect1 ne vaut pas preuve pour Effect2 ; conserver v0/v2/v3/v4–v8. |
| `17` Sound shift | Mode17 stabilisé, Effect1 length **2 → 6** si l'UI l'accepte | Feature03 : `v1` candidat BE, autre champ éventuel | `v2` appartient à Effect2 ; ne pas recopier son attribution sur Effect1. |
| `18` AMBIENTpx | Mire synthétique à quatre quadrants, Display area fixe, Reverse **OFF → ON** | Bulk364 avant/après ; rechercher aussi tout Feature03/Output02, y compris leur absence | Enregistrer une séquence stable sans fenêtre sur la mire ; distinguer permutation de mots, traitement PC et changement d'en-tête. |

Pour Sound flash, une **seconde** acquisition seulement après la première
pourrait appliquer un son synthétique contrôlé `0 → 50 %` sur le canal gauche,
à source inchangée, et vérifier si le corps32 du Bulk35 quitte zéro. Aucun
algorithme d'analyse audio ou commande Linux n'en découle aujourd'hui. Pour
AMBIENTpx, varier ensuite le nombre de LED et le groupe dans des sessions
distinctes afin de résoudre les deux `06` de l'en-tête ; la capture11 sans Bulk
n'apporte pas cette comparaison. Les captures brutes, écrans et éventuels
rapports complets resteraient privés et hors Git.

## Possibilités sans nouvelle acquisition et découpage proposé

**Lot 7B, hors matériel :** revoir humainement les 49 attributions confirmées
et3 couplées du CSV avancé ; préparer des fixtures synthétiques et tests de
non-régression pour les modes0D–18, les masques propres à chaque effet, les
int16 signés et la préservation des champs opaques. Mettre à jour ensuite le
catalogue descriptif avec niveaux de preuve distincts, après revue, sans
brancher de nouveaux modes pilotables sur l'adaptateur Read only. Une fixture
Bulk364 de six mots `00 G R B` peut tester le *décodage documentaire* hors ligne,
sans encodeur actif ni supposition sur le routage des LED.

**Lot 7C, uniquement sous nouveau GO matériel :** acquérir les dix séquences
ciblées ci-dessus, en commençant par les états surtout limités à une sélection
(`0D`, `12`, `14`, `18`) et par les rôles de palette des trois Rain,
avec sources audio/vidéo synthétiques et contrôle explicite du montage13 LED.
Comparer d'abord les nouvelles preuves aux CSV existants, réviser les champs
couplés et l'exclusivité des logiciels concurrents, puis décider séparément de
toute implémentation active. Le présent lot s'arrête à l'analyse hors ligne :
**NO-GO pour toute commande matérielle et toute écriture RGBpx.**
