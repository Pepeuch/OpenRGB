# D5 NEXT — Lot 7 : dix effets RGBpx, vidéo et USB

Analyse hors ligne du 11 novembre 2026, branche `feat/d5next`, état de référence
`9b5095af`. Aquasuite X.94, firmware **1027**, configuration **structure 3**.
Aucun accès au périphérique, changement C++, envoi USB, lancement de service,
commit ou publication des acquisitions n'a été effectué.

## 1. Résultat et portée

Les **10 effets recherchés sont sélectionnés dans cette session**. Six apportent
des correspondances fonctionnelles isolées : Rain, Snowfall, Stardust, Sound bars,
Sound slider et Sound shift. Color switch, Bar graph et Sound flash apportent
surtout des états initiaux et des observations de sources. AMBIENTpx apporte
des réglages visibles côté PC et un flux Bulk variable, sans modification
Feature03 correspondant aux réglages effectués après sa sélection.

Le nouveau CSV de paramètres contient **52 correspondances : 49 confirmées et
3 compatibles mais couplées**. Les 49 se décomposent en **28 champs values[],
17 options à masque, 2 filtres et 2 rôles de couleurs H/S**, comptés par couple
effet/réglage, pas par nombre de clics ni par composante de couleur. Ce sont des
attributions documentaires, pas des fonctions C++ activées ou validées en écriture.

Les flux sur l'interface vendor **0**, Bulk OUT **0x02**, sont désormais observés :
**327 opérations de 35 octets identiques**, puis **646 opérations de 364 octets,
99 contenus distincts**. Le premier flux ne permet pas de décoder des niveaux
audio ; le second est corrélé à AMBIENTpx, mais son ordre de composantes n'est
pas établi. Voir [l'analyse du streaming](D5NextRGBpxStreamingAnalysis.md).

## 2. Acquisition et identification

Originaux locaux dans `Controllers/AquaComputerD5NextController/` :

| Fichier | Taille exacte | Durée | Informations temporelles |
| --- | ---: | ---: | --- |
| `d5next test2.mp4` | 169 514 149 octets | 395,100 s | `creation_time=2026-11-11T08:10:19Z` ; modification UTC 08:16:54.6505252 |
| `d5next_baseline06.pcapng` | 5 966 316 octets | 397,563060 s | 09:10:18.711879–09:16:56.274939 CET ; modification UTC 08:17:43.0930303 |

SHA-256, recalculés en fin d'analyse et inchangés, dans cet ordre :

```text
40186b6bce1c6fb4512453045c7aba63e696f082480707980baa60b36f3a9c9d
96d5ff30b8a37297fa3f572f2fbb06c876e3673788e483427b5bc72fbea81a0e
```

La vidéo contient H.264, **1958 × 1638, 30 images/s, 11 853 images**, et une piste
AAC signalée par FFprobe. Cette piste n'a été ni extraite, ni écoutée, ni
transcrite. L'analyse d'images utilise `-an`. La capture contient **51 954 trames**,
interface de capture `USBPcap2`, snaplen 65535, ordre chronologique.

L'association repose sur l'ordre distinctif des dix sélections et les variations
isolées décrites ci-dessous. Les dates de modification ne servent pas de preuve
de synchronisation. Les anciennes acquisitions, notamment vidéo `d5next test.mp4`
et capture05, restent des preuves séparées.

Les images, extractions, scripts d'analyse et JSON locaux sont dans
`.build/d5next-lot7/`, ignoré par Git. Aucun payload audio/écran ni rapport complet
privé n'est joint aux livrables.

## 3. Synchronisation propre à cette session

`tUSB` est `frame.time_relative` depuis le début du PCAP ; `tVidéo` est le temps
de lecture du MP4. La différence des métadonnées de départ n'est que **0,288121 s**.
Elle ne synchronise pas les gestes et les écritures différées.

Trois transitions ont été encadrées par extraction ciblée des titres de panneau :

| Action visible | Ancien titre à tVidéo | Nouveau titre à tVidéo | SET Feature03 | tUSB | tUSB − apparition vidéo |
| --- | ---: | ---: | ---: | ---: | ---: |
| Rain → Snowfall | 27,500 | 27,650 | 3259 | 29,175846 | 1,525846–1,675846 s |
| Sound flash → Sound slider | 258,350 | 258,500 | 31365 | 260,005978 | 1,505978–1,655978 s |
| Sound shift → AMBIENTpx | 366,500 | 366,650 | 47291 | 368,168762 | 1,518762–1,668762 s |

Ces trois intervalles se recouvrent : un rapprochement effectif d'environ **1,6 s**
convient aux sélections. Il englobe les délais UI/application/USB et ne mesure pas
à lui seul un décalage d'horloge. Les options ont un délai effectif différent :
Reverse Sound bars est visible ON à 132,5 et 133,0 s, puis OFF à 133,5 s,
pour les écritures 16279/16389 à 133,463085/134,798109 s. Les images entourant
chaque geste sont donc nécessaires. **Aucune dérive d'horloge n'est démontrée.**

Les CSV donnent un échantillon vidéo généralement à `tUSB−0,8 s` et une fenêtre
de recherche `tUSB−1,8 .. tUSB−0,7 s`. Cette fenêtre est un repère de lecture,
pas un intervalle de confiance mesuré pour chaque clic. L'échantillon peut montrer
le curseur en cours de déplacement ou une animation du bouton ; les valeurs USB
ne sont pas présentées comme des nombres nécessairement lisibles sur cette image.
Les allers-retours d'options ont été examinés séparément entre 130 et 144 s.
Les SettingsSave différés ne servent jamais d'ancre de geste.

## 4. Inventaire USB et intégrité

Le descripteur **38** identifie `0C70:F00E`, **bus 2, adresse 5**. La configuration
**40** déclare IF0 vendor (`0xFF`) avec Bulk IN `0x81` et OUT `0x02`, et IF1 HID
avec Interrupt IN `0x83`. Chaque endpoint déclare 64 octets maximum par paquet.
Les tailles ci-dessous sont des charges utiles d'URB complètes, pas des tailles
de paquets physiques individuels. Aucun changement d'adresse de cette identité
n'a été identifié dans la session.

| Échange | Interface / endpoint / sens | Nombre | Taille utile | Références / cadence |
| --- | --- | ---: | ---: | --- |
| GET_DESCRIPTOR device/configuration, SET_CONFIGURATION | device / EP0 | 3 paires | 18, 65, 0 | 37→38, 39→40, 41→42, préambule à t=0 |
| SET_REPORT **Feature03** | IF1 / EP0 / OUT | **74 paires** | **809** | 873→874 à 47291→47292 ; événementiel |
| SET_REPORT **Output02** SettingsSave=2 | IF1 / EP0 / OUT | **23 paires** | **11** | 2993→2994 à 47851→47852 ; événementiel |
| Interrupt **Input01** | IF1 / 0x83 / IN | **394 complétions** | **162** | 81–51875 ; 0,990058 Hz |
| Bulk, préfixe `0A` | IF0 / 0x02 / OUT | **327 paires** | **35** | 19333→19334 à 46921→46922 ; 1,604240 Hz |
| Bulk, préfixe `0B` | IF0 / 0x02 / OUT | **646 paires** | **364** | 46969→46970 à 51951→51952 ; moyenne 21,042369 Hz |

Total cible : **2 934 trames**. Les **1 073 opérations Control/Bulk** sont
appariées par `usb.request_in` et identité IRP, sans perte de complétion observée.
Pour Interrupt IN, il y a **392 paires**, deux complétions sans soumission capturée
(`81`, `303`) et deux soumissions sans complétion capturée (`51565`, `51876`).
Un IRP réutilisé n'est pas une nouvelle preuve d'appariement : la référence de
requête est contrôlée. Il serait incorrect de déduire 394 paires de la seule
égalité des nombres de soumissions et de complétions.

Le CSV d'événements conserve ainsi **1 469 enregistrements USB** (1 465 paires
et ces quatre extrémités), plus **9 observations UI**, soit **1 478 lignes**.
Les champs de trame manquants restent vides. `usb_s` est le temps de soumission,
ou celui de la complétion lorsqu'elle est orpheline. Les statistiques Input01
ci-dessus utilisent les complétions, première à 0,084416 s, dernière à 397,030828 s.

Tous les statuts présents des trames cibles sont `SUCCESS`. Aucun paquet cible
tronqué (`cap_len < len`) ni signalement `_ws.malformed` n'a été relevé.
Absents : GET_REPORT HID, SET Feature02, Interrupt OUT, Bulk IN et autres types
de transfert cibles ; aucun rapport HID08/0C identifié. Ce constat concerne cette
capture, pas toutes les capacités du matériel.

Les Feature03 utilisent `bmRequestType=21`, `bRequest=09`, `wValue=0303`,
`wIndex=1`, `wLength=809`. **74/74** : ID03, longueur809, structure BE1–2=3,
CRC-16/USB correct sur **1–806**, stockage BE807–808. Vérification par deux
formulations du CRC, vecteur ASCII `123456789 → B4C8` : poly8005/refletA001,
initFFFF, RefIn/RefOut vrais, xoroutFFFF. Le firmware1027 est constaté dans les
394 Input01, pas supposé être un champ du Feature03.

Les **huit groupes de 70 octets** sont décodés à chaque configuration, soit
592 états. Après le premier état capturé, **seul le groupe6 change** : aucun
changement hors RGB, de master237 ou de flags238–239. Master reste **203**,
UI affiche80 % ; aucun arrondi universel n'est déduit de cet état. Les autres
groupes sont inchangés : six groupes externes modes03/08/0B/04/04/04,
et groupe7 mode00. Aucun groupe externe audio/AMBIENTpx n'est configuré ici.

## 5. Catalogue des dix effets réellement sélectionnés

Indices à base zéro ; toutes les lignes concernent le groupe6 interne :
`strip=1, start=0, count=6`, base **660**. L'état antérieur au premier Feature03
est absent ; il ne faut pas fabriquer une transition vers sa trame873.

| ID | Nom interne / UI | Première trame | tUSB | values[0..11] à cette trame, état observé seulement |
| --- | --- | ---: | ---: | --- |
| 0F | Rain1 / Rain | 873 | 6,645683 | 40,4,25,30,0,0,0,0,0,0,0,0 |
| 10 | Rain2 / Snowfall | 3259 | 29,175846 | 50,3,10,15,0,0,0,0,0,0,0,0 |
| 11 | Rain3 / Stardust | 5605 | 49,411292 | 25,3,60,15,0,0,0,0,0,0,0,0 |
| 12 | Switch / Color switch | 8047 | 71,003087 | 1,0,25,50,100,100,100,100,0,0,0,0 |
| 0D | Bar1 / Bar graph | 9675 | 81,473886 | 0,100,0,1,50,100,200,0,0,0,0,0 |
| 15 | Sound2 / Sound bars | 11165 | 94,074299 | 0,100,0,1,25,50,75,5,0,0,0,0 |
| 14 | Sound1 / Sound flash | 25941 | 222,985770 | 0,0,0,0,0,0,0,0,0,0,0,0 |
| 16 | Sound3 / Sound slider | 31365 | 260,005978 | 0,0,0,0,5,5,5,5,10,0,0,0 |
| 17 | Sound4 / Sound shift | 40821 | 324,073211 | 10,2,2,10,50,0,0,0,0,0,0,0 |
| 18 | AmbientPx / AMBIENTpx | 47291 | 368,168762 | 0,0,0,0,0,0,0,0,0,0,0,0 |

Ces initialisations sont **couplées** : mode, flags, paramètres et parfois palette
ou source changent ensemble. Ce ne sont ni des presets universels ni des plages
UI. Les différences complètes, y compris les octets CRC, figurent dans le CSV.

## 6. Matrice fonctionnelle et chronologie des variations isolées

Layout conservé : flags BE664–665 ; source signée BE666–667 ; filtres668/669 ;
bindings670–681 ; `values[k]` signé BE à **682+2k** ; HSV[i] à **706+4i**
(H BE16, S8, V8). Le fait que les échantillons soient positifs ne change pas
le type signé du layout. Les unités et limites UI ne sont pas extrapolées.

### Rain, Snowfall, Stardust

| Réglage | Champ / offsets | Rain : trame, avant→après | Snowfall | Stardust |
| --- | --- | --- | --- | --- |
| Drop speed | v0 / 682–683 | 1225 :40→20 | 3527 :50→64 | 6293 :25→61 |
| Drop items | v1 / 684–685 | 1479 :4→3 | 3681 :3→2 | 6319 :3→2 |
| Drop size | v2 / 686–687 | 1771 :25→57 | 3989 :10→38 | 6561 :60→28 |
| Drop smoothness | v3 / 688–689 | 2051 :30→63 | 4299 :15→50 | 6829 :15→64 |

Chaque ligne est isolée dans le rapport USB. Les deux opérations6293/6319,
séparées de0,488076 s, restent deux écritures distinctes.

| Option | Masque | Rain | Snowfall | Stardust |
| --- | --- | --- | --- | --- |
| Reverse direction | 0002 | 2269 :0000→0002 | 4425 :0010→0012 | 7015 :0000→0002 |
| Random color | 0008 | 2369 :0002→000A | 4545 :0012→001A | 7139 :0002→000A |
| Snow | 0010 | 2451 :000A→001A | 4641 :001A→000A | 7205 :000A→001A |

Les labels Background et Color sont visibles, mais aucune modification isolée
de leurs HSV n'a lieu ici. Les états de palette à la sélection ne suffisent pas
à attribuer expérimentalement leurs indices. v4–v11 restent non attribués.

### Sound bars, ID15

| Réglage | Champ / offsets | Preuves | Qualification |
| --- | --- | --- | --- |
| Première limite | v4 /690–691 | 11875 :25→55 ; 12605 :28→32 ; 12779 :32→19 | Confirmé isolé |
| Peak hold time | v7 /696–697 | 18529 :5→10 | Confirmé isolé, unité inconnue |
| Minimum source | v0 /682–683 | 23767 :0→2 | Confirmé isolé |
| Maximum source | v1 /684–685 | 24645 :100→20, v5 aussi55→20 | Compatible couplé |
| Nombre de limites | v3 /688–689 | 12207 :1→2, seuils/palette réorganisés | Compatible couplé |
| Seconde limite | v5 /692–693 | 12207 :50→55 et 24645 :55→20 | Compatible couplé |
| Rising values | filtre0 /668 | 23159 :1→38 | Confirmé isolé |
| Falling values | filtre1 /669 | 23309 :1→58 | Confirmé isolé |
| Couleur première plage | HSV2 H/S /714–716 | 13753 :H0000→04F7, SFF→85, VFF préservé | Confirmé isolé H/S |
| Couleur de pic | HSV1 H/S /710–712 | 18243 :H0000→04B9, S00→F0, VFF préservé | Confirmé isolé H/S |

L'insertion12207 change simultanément v3, v4 (55→28), v5 et H deHSV3/HSV4.
Elle ne démontre ni le nombre maximal de limites, ni un algorithme général de
répartition. Le clamp24645 montre un effet secondaire à préserver dans de futurs
tests ; il ne justifie pas d'en faire une règle universelle pour tous les effets.
Background/HSV0 et les autres plages restent compatibles avec la palette de
sélection, sans variation isolée. Le rôle de V n'a pas été testé.

**Les masques Sound bars ne sont pas ceux des Rain** :

| Option | Masque | Trames et flags |
| --- | --- | --- |
| Reverse direction | **0008** | 16279 :0006→000E ; 16389 :000E→0006 |
| Show bar | 0002 | 16439 :0006→0004 ; 16453 :0004→0006 |
| Show ranges | 0004 | 16613 :0006→0002 ; 16695 :0002→0006 |
| Fade ranges | **0001** | 16795 :0006→0007 ; 16841 :0007→0006 |
| Show peak value | 0020 | 16953 :0006→0026 |

Les captures vidéo rapprochées130–144 s permettent de distinguer Reverse de
Fade. Une image tardive de la séquence pouvait conduire à les inverser.

### Sound slider, ID16

| Réglage | Champ / offsets | Écritures isolées |
| --- | --- | --- |
| Effect1 sous-mode | v0 /682–683 | 35831 :0→1 ; 38443 :1→5 ; 38891 :5→4 ; 39137 :4→3 ; 39437 :3→2 ; 39645 :2→1 ; 39773 :1→0 |
| Effect2 sous-mode | v1 /684–685 | 36149 :0→2 |
| Effect3 sous-mode | v2 /686–687 | 36531 :0→3 |
| Effect4 sous-mode | v3 /688–689 | 36859 :0→4 |
| Effect1 Speed | v4 /690–691 | 37387 :5→3 |
| Effect2 Speed | v5 /692–693 | 37579 :5→3 |
| Effect3 Speed | v6 /694–695 | 37793 :5→3 |
| Effect4 Speed | v7 /696–697 | 37959 :5→3 |
| Rotate color | v8 /698–699 | 31807 :10→58 |

Les six choix visibles pour Effect1 sont : **0 Outwards from center,
1 Inwards to center (A), 2 Inwards to center (B), 3 From the left,
4 From the right, 5 All LEDs**. Cela rejoint l'énumération déclarée antérieurement
dans les métadonnées, désormais confrontée à la séquence USB. Les trois autres
menus n'ont pas chacun parcouru leurs six choix. Aucune option flags ni couleur
HSV n'est modifiée isolément ici. v9–v11 restent non attribués.

### Sound shift, ID17

| Réglage | Champ / offsets | Trame et avant→après |
| --- | --- | --- |
| Rotate color | v0 /682–683 | 41121 :10→43 |
| **Effect2** Effect length | v2 /686–687 | 45473 :2→6 |
| Idle speed | v3 /688–689 | 45643 :10→43 |
| Activity speed | v4 /690–691 | 45869 :50→25 |
| Effect1 Random color | flags mask0002 | 41471 :0006→0004 |
| Effect2 Random color | flags mask0004 | 44657 :0004→0000 ; 44733 :0000→0004 |
| Reverse direction | flags mask0001 | 45927 :0004→0005 |

v1=2 est compatible avec Effect1 Effect length, mais ce curseur n'est pas varié
isolément : **pas de confirmation supplémentaire comptée**. Couleurs et fond
ne sont pas variés ; v5–v11 restent sans rôle démontré.

### Color switch, Bar graph, Sound flash, AMBIENTpx

Color switch8047 et Bar graph9675 montrent un panneau **Data source non affectée**.
Aucune variation isolée des seuils, couleurs, flags ou bindings n'est enregistrée.
Ne pas importer automatiquement la sémantique de Sound bars dans Bar graph.

Sound flash25941 initialise v0–v11 à0 et affiche Background, quatre couleurs et
quatre sources. À225 s, une source est affectée ; à250 s, les quatre sources
affichées sont VolumeR, VolumeL, VolumeL+R, Mid. **Aucun Feature03 supplémentaire**
entre25941 et31365 : l'affectation visible ne fournit pas quatre nouveaux offsets
de configuration. Les rôles des HSV de sélection restent non isolés.

AMBIENTpx47291 initialise v0–v11 et flags à0. Puis la vidéo montre :

| Échantillon vidéo | État / action constatée | Feature03 ultérieur |
| --- | --- | --- |
| 367 s | Display initial, Display area Top | Aucun après47291 |
| 370 /374 s | Autre Display choisi ; zone Bottom à374 | Aucun |
| 378 /382 s | Left puis Right | Aucun |
| 386 s | Brightness et Desktop range déplacés ; tooltip61 sur un curseur Desktop range | Aucun |
| 390 s | Autre curseur Desktop range déplacé | Aucun |
| 394 s | Reverse direction ON, OFF à390 | Aucun |

Le champ flags du dernier Feature03 reste donc0 malgré Reverse visible ON.
Ne pas attribuer à AMBIENTpx le masque0001 de Sound shift. Les réglages sont
visibles dans l'UI côté PC ; leur représentation éventuelle dans le flux et
leur lieu de persistance ne sont pas déterminés. Les valeurs non lisibles restent
non renseignées, et aucune identité d'écran ou image personnelle n'est publiée.

## 7. Sources, bindings, sauvegardes et flux

La source signée reste `-1` jusqu'à Sound bars11165, passe alors à **20 décimal
(0014)** avec les filtres10/15→1/1, puis revient à−1 à25941. Le20 correspond
au `SOUND_CONTROLLER_ID` déclaré dans les métadonnées antérieures ; il ne code
pas à lui seul le choix Low/High/Volume d'un menu PC, et n'est pas un rapport HID14.
À161 s le menu est vide, à162 s Low est affiché : **pas de nouveau Feature03**
à ce moment, mais début du Bulk35 à163,047567 s. D'autres sources sont ensuite
sélectionnées dans les panneaux audio sans variation03 correspondante.

Les bindings sont décodés pour chaque état. Pour g6, binding2 reste
`x1=0,x2=100,y1=0,y2=255`. Binding1 commence `0,100,1,100`, passe à
`0,100,0,255` à8047, puis `0,100,0,100` à9675 et `0,100,0,255` à11165.
Ces changements accompagnent les modes : **aucune fonction UI de binding isolée**
n'est attribuée. Les filtres668/669 sont les deux exceptions isolées décrites plus haut.

Les **23 Output02** sont Control EP0 IF1, `wValue=0202`, pas Bulk02 et pas
Feature02. ID02, 11 octets, SettingsSave=2 :

```text
02 00 00 00 02 00 00 00 00 34 C6
```

**23/23 CRC corrects**, CRC-16/USB sur1–8, BE9–10=`34C6`. Les références et
la dernière configuration précédente sont dans le CSV. Délai depuis celle-ci :
**2,926244–3,840752 s**, moyenne3,756896 s. Deux cas sont sensiblement plus
courts que3,83 s : 8505 après8047 (2,926244 s), 12115 après11875 (3,018544 s).
Le nombre23 contre74 prouve qu'une sauvegarde n'est pas observée après chaque
écriture. Aucun contrat de temporisation ni effet EEPROM n'est déduit.

Pour le Bulk : voir le rapport dédié. Le passage de format35 à364 précède
Feature18 de1,278847 s. Le dernier mode écrit est encore Sound shift pendant
les40 premières opérations364 ; la sélection AMBIENTpx est alors visible dans
le dialogue vidéo. Il peut s'agir d'une préparation ou prévisualisation côté PC.
Cela **ne prouve pas une exécution audio/vidéo simultanée**. Aucun chevauchement
des deux formats Bulk, multiplexage démontré ou autre groupe configuré Sound/
AMBIENTpx n'est observé. Une coexistence possible n'est pas réfutée pour autant.

## 8. Validation, comparaison et conséquences

Méthode : TShark/capinfos **4.6.9**, FFmpeg/FFprobe **9.0.2**, scripts PowerShell
locaux ; lecture des descripteurs avant filtrage bus/adresse ; appariement des
URB ; double calculCRC03 ; validationCRC02 ; comparaison intégrale des809 octets
successifs ; décodage de8 groupes ; extraction ciblée et relecture des options
rapides. Les livrables ne contiennent que des différences RGB, preuves de trames,
tailles, en-têtes minimaux et agrégats. Aucun test matériel ni test C++ n'a été lancé.

Références conservées : [baseline Windows](D5NextWindowsBaseline.md),
[captures02–04](D5NextWindowsRGBAnalysis.md),
[moteur et métadonnées](D5NextRGBpxEngineAnalysis.md),
[première corrélation](D5NextRGBpxVideoUSBMapping.md), les CSV Events/Parameters
du Lot5, [refonte C++](D5NextRGBpxCppRefactor.md) et
[corrections Lot6](D5NextLot6Corrections.md). Les **59 correspondances confirmées
antérieures ne sont pas modifiées** (65 lignes confirmées dans l'ancien CSV,
59 couples effet/contrôle distincts, répétitions comprises dans les65 lignes).
Les49 nouvelles portent sur d'autres IDs :
**108 correspondances confirmées documentées au total**, sans équivalence avec
108 capacités matérielles testées. Les23 IDs du catalogue restent distincts de
la preuve de rendu physique de leurs effets : cette vidéo filme Aquasuite.

Pour une future évolution OpenRGB, conserver la séparation configuration809 /
systèmeOutput02 / télémétrieInput01 / transports PC vendor. Étendre d'abord les
descriptions et fixtures synthétiques d'effets, avec préconditions par effet,
préservation des champs inconnus et audit des différences. Le masque Reverse
dépend de l'effet (Rain0002, Sound bars0008, Sound shift0001). Les quatre menus
Sound slider sont des enums, leurs quatre Speed sont d'autres champs. Les
sources PC ne peuvent pas être implémentées comme un simple patch03.

Le catalogue/auditeur existant du Lot6 et ses barrières d'écriture restent la
référence actuelle ; ce lot **ne les étend pas**. Ni encodeur Bulk audio/vidéo,
ni moteur de rendu, ni sauvegarde automatique ne sont prêts à être activés à
partir de ces observations. La structure364 reste à établir par stimulations
synthétiques connues, pas par déduction d'un format RGB d'après la longueur.

Les recherches initiales d'**aquapx**, les contributeurs de l'issue **OpenRGB
#2111**, et les validations indépendantes avec Aquasuite sont crédités. Une future
contribution à aquapx peut reprendre les tableaux de champs par effet, les masques,
les preuves de séparation des transports et des fixtures entièrement synthétiques.
Aucun code Go ou propriétaire n'a été repris ; aucun dossier tiers n'a été modifié.

## 9. Incertitudes et prochaines captures ciblées

Sur120 emplacements `values[]` théoriques pour ces10 effets, **28 sont attribués
par variation isolée, 3 seulement compatibles et 89 sans rôle attribué**. Les89
incluent possiblement des champs inutilisés : ce n'est pas un nombre de fonctions
manquantes. Les drapeaux non variés, bornes UI, unités, bindings et palettes
non isolées restent inconnus indépendamment de ce décompte.

Expériences proposées à l'utilisateur, sans exécution ni autorisation matérielle
automatique par ce rapport :

1. **Source synthétique audio connue**, amplitude nulle puis paliers distincts
   gauche/droite et bande unique ; capturer l'activation et la désactivation.
   Déterminer si le corps32 octets du flux0A quitte son état nul et quel canal varie.
2. **AMBIENTpx sur mire synthétique**, un seul réglage à la fois : zones, Reverse,
   luminosité, six régions de couleurs connues ; relever gestes et valeurs.
   Établir les composantes et le sens des octets sans capturer un bureau privé.
3. **Bar graph / Color switch avec source affectée**, seuils et bindings isolés ;
   puis Effect1 length de Sound shift et les couleurs de chaque effet Sound.
4. **Coexistence explicite**, deux groupes dédiés audio et vidéo, sources
   synthétiques, activation/arrêt séparés ; comparer aux flux présents ici sans
   supposer une mutualisation ni une impossibilité matérielle.
5. Après documentation de ces résultats, préparer sous Linux les fixtures et
   tests hors ligne avant toute proposition séparée de validation matérielle.

Les captures brutes et images restent locales et ignorées. **Arrêt après analyse :
NO-GO pour une écriture matérielle ou une modification C++ dans ce lot.**
