# D5 NEXT — AMBIENTpx : fonctionnement des réglages, test5 / USB09

Analyse hors ligne du 11 novembre 2026, branche `feat/d5next`. Cette acquisition
est étudiée pour les **fonctions des réglages et la géométrie de l'aperçu**, sans
nouvelle analyse des couleurs. Aucun accès matériel, commande USB/HID, changement
Aquasuite ou C++, commit, push ou PR n'a été effectué.

## Résultat

La vidéo distingue **quatre fonctions UI** : choix du bord, borne de début,
borne de fin et profondeur vers l'intérieur de l'écran. Elle montre **six
changements de bord appliqués**, les quatre bords étant parcourus, et18 lectures
numériques de curseurs. La profondeur50 est visible au bout droit du curseur
simple ; l'aperçu couvre alors approximativement une moitié d'écran.

**Aucun GET_REPORT, Feature03 ou Output02 SettingsSave n'est capturé.** Les
réglages observés ne fournissent donc aucun nouvel offset de configuration HID.
Le flux Bulk364 continue avec le même en-tête ; ses variations ne constituent
pas, à elles seules, un encodage des valeurs des curseurs.

Livrables associés :

- [Opérations USB exhaustives](D5NextAmbientSettingsEvents.csv), **1511 lignes**.
- [Observations UI et fenêtres USB candidates](D5NextAmbientSettingsMapping.csv),
  **31 lignes**, avec niveaux de preuve distincts pour l'UI et l'USB.

## Acquisition et identification

Originaux dans `Controllers/AquaComputerD5NextController/` :

| Fichier | Taille | Durée | Métadonnées |
| --- | ---: | ---: | --- |
| `d5next test5.mp4` | 30 867 357 octets | 85,933300 s | H.264,2106×986,30fps,2578 images ; création MP4 `2026-11-11T09:49:06Z` |
| `d5next_baseline09.pcapng` | 1 520 372 octets | 80,796935 s | 15604 trames ;10:49:15.447915–10:50:36.244850 CET ; USBPcap2 |

SHA-256, vidéo puis PCAP :

```text
fd39575947a28f1a129829d650ba7b1abd36611c0aa51397d7b80b30ab9f1ff0
ec63cdeac4565c79dad3e88b228cee3a121a7b27debaad24fac1b769fd7a91c4
```

La piste AAC signalée n'est ni extraite, ni écoutée, ni transcrite. Les extractions
vidéo ciblées utilisent `-an` et restent dans `.build/d5next-ambient-test5/`,
ignoré par Git. Aucun écran personnel n'est reconstruit à partir de l'USB ;
aucune capture d'écran ni valeur de couleur n'est publiée dans ces livrables.

Les descripteurs **38/40** réidentifient **0C70:F00E, bus2/adresse5** :
IF0 vendor, Bulk IN81/OUT02 ; IF1 HID, Interrupt IN83. Maximum64 octets par paquet
physique ;364 octets désignent la charge utile d'une opération Bulk complète.
Le firmware1027 est constaté dans les80 Input01. **Structure3 et mode18 ne sont
pas relus**, faute de Feature03 dans cette acquisition.

## Synchronisation : ne pas réutiliser les décalages des tests précédents

Les métadonnées placent le début USB **9,447915 s après** le début nominal du
MP4 : `tUSB nominal = tVidéo −9,447915 s`. La vidéo commence donc avant la
capture, contrairement aux tests3/4. Les premières secondes de vidéo n'ont
pas de preuve USB correspondante dans ce fichier.

Ce rapprochement dépend des métadonnées MP4 et ne calibre ni la latence UI,
ni celle du traitement d'écran. Les changements de réglages ne produisent
aucun SET03/Output02 identifiable pour établir un décalage indépendant.
L'aperçu et la source d'écran ne constituent pas une mire temporelle contrôlée.
**Aucune synchronisation fine ni dérive d'horloge n'est démontrée.**

Le CSV Mapping indique pour chaque échantillon une fenêtre pratique de recherche
de±1 s autour du temps nominal. Ce n'est **pas un intervalle de confiance
mesuré**, ni la preuve que toutes les opérations de la fenêtre proviennent de
ce geste. Les bornes de trames sont des candidates, pas une première trame de
réaction. Les résultats fonctionnels ci-dessous reposent d'abord sur les gestes
et les changements du rectangle visibles dans la vidéo.

## Les réglages et leurs fonctions

Le libellé **Desktop range** couvre deux contrôles différents : un curseur à
deux poignées et, en dessous, un curseur simple. « Profondeur » est ici un nom
descriptif déduit de l'aperçu, pas le nom d'un champ propriétaire identifié.

| Contrôle | Fonction visible | Qualification | Ce qui n'est pas établi |
| --- | --- | --- | --- |
| Display area | Ancrer la zone à Top,Bottom,Left ouRight | Confirmé par plusieurs sélections et aperçus | Encodage d'une préférence PC ou d'une commande cachée |
| Desktop range, poignée de début | Déplacer le début de l'intervalle **le long du bord** | Confirmé par variations UI et rectangle | Offset HID/Bulk, index du premier échantillon |
| Desktop range, poignée de fin | Déplacer la fin de ce même intervalle | Confirmé par variations UI et rectangle | Borne haute logicielle, règle de franchissement entre poignées |
| Curseur simple sous Desktop range | Modifier l'épaisseur de la zone **perpendiculairement au bord**, vers l'intérieur | Confirmé par variations isolées dans l'UI, notamment Top etRight | Formule exacte d'intégration des pixels, borne basse non testée |

Top etBottom donnent un intervalle longitudinal horizontal. Left etRight donnent
un intervalle longitudinal vertical. Le curseur simple contrôle respectivement
la hauteur ou la largeur depuis le bord sélectionné. Les poignées ne règlent
donc pas le nombre de LED ni leur luminosité dans cette observation.

### Modèle géométrique descriptif

Avec x/y normalisés dans l'aperçu, origine en haut à gauche, a etb les bornes
longitudinales et d la profondeur **normalisée du rectangle affiché** :

| Bord | Intervalle horizontal de l'aperçu | Intervalle vertical de l'aperçu |
| --- | --- | --- |
| Top | [a,b] | [0,d] |
| Bottom | [a,b] | [1−d,1] |
| Left | [0,d] | [a,b] |
| Right | [1−d,1] | [a,b] |

Ce tableau formalise qualitativement les rectangles observés. Il **ne démontre
pas** que le moteur échantillonne uniformément ce rectangle, ni que ses six mots
USB se répartissent selon cette formule. Il ne transforme pas les nombres UI
en coordonnées physiques garanties. L'aperçu et les tooltips peuvent aussi se
mettre à jour à des instants différents pendant un déplacement rapide.

La valeur **50** est lisible à l'extrémité droite du curseur de profondeur
(20 s enTop,32 s enLeft), avec un rectangle proche d'une demi-hauteur ou
demi-largeur. Cela est compatible avec une profondeur exprimée en pourcentage
de la dimension totale. **Ne pas assimiler une position maximale de ce curseur
à une couverture de100 % de l'écran.** La borne basse et la linéarité complète
ne sont pas mesurées dans ce test.

## Chronologie des bords et des valeurs lisibles

Les temps sont des échantillons de vidéo, pas des instants exacts d'application.
Une liste ouverte ou un élément survolé ne compte pas comme changement appliqué.

| tVidéo | État/action visible | Lecture technique |
| ---: | --- | --- |
| 14 s | Top, intervalle aux extrémités | État initial après masquage de Wireshark |
| 18 /19 /19,5 /20 s | Profondeur32 /17 /44 /50 | Épaisseur depuis le haut, intervalle longitudinal conservé |
| 22 s | Menu ouvert, Top encore sélectionné | Pas encore une sélection Bottom prouvée |
| 24 s | **Bottom** appliqué | Premier changement de bord |
| 26 s | Profondeur49 enBottom | Rectangle ancré en bas |
| 28 s | Menu ouvert, Bottom encore sélectionné | Ne pas dater Left au seul menu |
| 30 s | **Left** appliqué | Deuxième changement |
| 32 s | Profondeur50 enLeft | Épaisseur vers la droite depuis le bord gauche |
| 36 s | **Right** appliqué, profondeur44 | Troisième changement ; plusieurs éléments ont évolué depuis32 s |
| 38 s | Profondeur33 enRight | Rectangle moins large, intervalle longitudinal conservé |
| 40 s | Profondeur revenue au bout droit | Valeur numérique non relue à cet instant |
| 49 s | Borne de fin77 enRight | Rectangle limité verticalement ; autre borne déjà déplacée |
| 52 s | **Left** appliqué avec intervalle restreint | Quatrième changement |
| 54 /56 s | Borne de début1 /borne de fin85 | Limitation verticale de la zone gauche |
| 60 s | **Bottom** appliqué | Cinquième changement ; intervalle maintenant horizontal |
| 62 /64 /66 /68 s | Fin52 /début29 /début0 /fin86 | Recadrages horizontaux du rectangle inférieur |
| 72 s | **Top** appliqué, borne de fin81 | Sixième changement |
| 78 /82 s | Borne de début51 /40 | Zone très étroite puis déplacement/élargissement selon l'autre borne |
| 84 s | Poignées revenues aux extrémités | Valeur de fin non relue ; pas de test explicite de largeur nulle |

Les18 lectures numériques sont8 lectures de profondeur et10 de bornes. Elles
ne prouvent pas18 paramètres distincts : ce sont des échantillons des mêmes
trois grandeurs numériques. Une valeur maximale100 de la borne de fin n'est
pas annoncée sans tooltip lu à cette extrémité dans cette analyse.

### Contrôles non exercés

- **Brightness AMBIENTpx** est un curseur distinct situé au-dessus de Desktop
  range, visible à droite dans les échantillons ; aucune variation isolée n'est
  retenue. Sa loi de gain n'est pas étudiée ici.
- **Strips Brightness** affiche38 % ; ce libellé UI n'est pas une lecture de
  master237 dans le PCAP. Aucun changement03 ne permet de le relire.
- **Reverse direction** reste visiblement OFF dans les échantillons examinés.
  Pas de permutation USB, sens physique de LED ou masque de flags attribué.
- Display, Background, effets audio et coexistence audio/vidéo ne font pas
  l'objet d'une nouvelle attribution dans ce test.

## Inventaire USB et relation aux gestes

**3018 trames cibles**, toutes les opérations terminées observées sont SUCCESS :

| Échange | Nombre | Références |
| --- | ---: | --- |
| Énumération standard | 3 paires | 37→38,39→40,41→42 |
| Bulk OUT02 IF0 | **1426 paires**,364 octets | 85→86,t0,023874 à15597→15598,t80,774250 |
| Interrupt IN83,Input01 | **80 complétions**,162 octets | 195–15511 ; environ0,990088 Hz |
| GET_REPORT /SET_REPORT HID,Feature03,Output02 | **0** | Aucune lecture/écriture de configuration ou sauvegarde |
| Bulk IN,Interrupt OUT,autres transferts cibles | **0** | Absents de cette capture |

Les1426 Bulk sont appariés par référence de requête et identité IRP. Input01
comprend78 paires, complétions195/381 sans soumission capturée et soumissions
15274/15512 sans complétion capturée. Le CSV conserve **1507 paires et4
extrémités**, soit1511 lignes ; chaque trame cible y figure une seule fois.
Aucun paquet cible tronqué ou `_ws.malformed` n'a été relevé.

Le flux conserve l'en-tête **0B 06 06 00**, les six mots actifs et le suffixe
observés précédemment. **261 contenus distincts**,365 changements par rapport
au paquet précédent ;81 opérations ont un corps entièrement nul. Ces variations
restent des faits de transport : elles ne désignent pas des valeurs de réglage
ni une erreur de capture à elles seules. Aucun payload ou pixel n'est exporté.

Cadence moyenne **17,646977 Hz**, médiane des intervalles0,031327 s ; min0,029983,
max1,107849 s. **41 intervalles dépassent500 ms**. Les rafales rapprochées et
pauses expliquent l'écart à la moyenne proche de30 Hz des autres tests ; leur
cause et un lien avec chaque glissement de curseur ne sont pas établis.
Latence soumission→complétion :0,501–0,983 ms. Aucun format35 observé.

CRC03/02 : non vérifiables, rapports absents. Les deux tests d'un CRC-16/USB
final sur le Bulk364 (portées0–361 et1–361, motBE362–363) ne correspondent
sur aucun des1426 paquets. Ce n'est pas une preuve d'absence de toute autre
intégrité. Bulk endpoint02 n'est toujours pas le rapport HID Output02.

Exemples de fenêtres **nominales**, reproduits du CSV :

| Observation vidéo | Fenêtre tUSB nominale±1 s | Trames Bulk candidates | Qualification |
| --- | --- | --- | --- |
| 18 s,profondeur32 | 7,552085–9,552085 | 1935–2247 | Métadonnées seulement |
| 38 s,profondeur33 | 27,552085–29,552085 | 6179–6435 | Métadonnées seulement |
| 49 s,borne de fin77 | 38,552085–40,552085 | 8365–8503 | Métadonnées seulement |
| 62 s,borne de fin52 | 51,552085–53,552085 | 10537–10735 | Métadonnées seulement |
| 78 s,borne de début51 | 67,552085–69,552085 | 12951–13149 | Métadonnées seulement |

Ces tableaux ne démontrent pas que le Bulk encode un rectangle ni des coordonnées.
Les gestes modifient un aperçu et possiblement le contenu échantillonné ; les
variations de charge peuvent rester couplées à ces effets logiciels. L'absence
de Feature03 montre seulement qu'aucune telle écriture n'est nécessairement
visible pour ces manipulations dans cette capture. Le lieu de persistance PC
et une éventuelle mémoire du périphérique ne sont pas déterminés.

## Bilan et conséquences pour OpenRGB

**Quatre fonctions UI caractérisées**, quatre bords observés, six transitions
appliquées,18 valeurs numériques lues. **Zéro nouvel offset HID, zéro nouvelle
commande de sauvegarde et aucune loi de couleur ajoutée.** Les paramètres
confirmés des lots précédents ne sont ni recomptés ni modifiés.

Pour une future architecture, le choix de bord, l'intervalle et la profondeur
peuvent être décrits dans une couche de sélection de région côté PC, séparée du
codec de configuration809 octets et du transport de contenu. Le schéma géométrique
ci-dessus fournit un modèle à tester hors ligne ; il ne constitue pas une
reproduction du calcul Aquasuite. Aucun champ values[],binding ou flag existant
ne doit être réaffecté à ces réglages sur la base de ce test.

Il reste à tester : coordonnées exactes et unités, profondeur minimale, intervalle
vide/inversé et contraintes entre poignées, découpage de la région en six valeurs,
Reverse, gain Brightness et persistance PC. Une capture ultérieure peut conserver
une mire fixe connue, filmer le panneau et cette mire ensemble, effectuer un
réglage à la fois et laisser des plateaux longs ; cela réduirait l'ambiguïté
temporelle et les couplages. Ce sont des propositions, pas des opérations
matérielles autorisées ou lancées par Codex.

Méthode : TShark/capinfos4.6.9, FFmpeg/FFprobe9.0.2, lecture vidéo ciblée et
contrôles PowerShell hors ligne. Références :
[streaming initial](D5NextRGBpxStreamingAnalysis.md),
[mires test3](D5NextAmbientSlideshowAnalysis.md),
[options test4](D5NextAmbientDisplayOptionsAnalysis.md), et protections du Lot6.
Les recherches initiales aquapx/OpenRGB#2111 restent créditées. Aucun code
propriétaire, Go ou tiers repris. **Arrêt après analyse ; aucune écriture
matérielle ni activation de transport dans ce lot.**
