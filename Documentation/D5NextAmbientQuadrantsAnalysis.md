# D5 NEXT — AMBIENTpx : mire quatre quadrants, test6 / USB10

Analyse hors ligne du 11 novembre 2026, en continuité des
[réglages test5](D5NextAmbientSettingsAnalysis.md). La mire fixe permet de
confronter les bords Top/Bottom/Left/Right aux groupes de mots du flux, avec
des limites dues aux fenêtres superposées. Aucun accès matériel, commande USB,
changement Aquasuite ou C++, commit, push ou PR n'a été effectué.

## Résultat et niveau de preuve

Les quatre états UI sont observés, dans l'ordre **Top → Bottom → Left → Right**.
Le flux présente les dominantes suivantes pour les mots0–2 puis3–5 :

| Bord | Mots0–2 | Mots3–5 | Conclusion de cette acquisition |
| --- | --- | --- | --- |
| Top | Rouge | Vert | Concordance propre avec la moitié supérieure, gauche→droite |
| Bottom | Bleu | Jaune | Compatible avec la moitié inférieure ; fenêtre Aquasuite sur une partie du bleu |
| Left | Rouge | Bleu | Compatible avec la moitié gauche, haut→bas ; fenêtre sur une partie du bleu |
| Right | Vert | Jaune | Concordance propre avec la moitié droite, haut→bas |

Top etRight apportent deux confrontations propres montrant que les groupes
de mots suivent **l'axe horizontal ou vertical du bord choisi**, à la résolution
de cette mire. Bottom etLeft sont concordants mais partiellement contaminés.
Ce test ne démontre ni le noyau de moyenne des pixels, ni une répartition
uniforme en six bandes exactes, ni l'ordre physique des LED. Dans une zone
uniforme, les mots0,1,2 ne sont pas distinguables entre eux, pas plus que3,4,5.

Le réglage de bord n'introduit **aucun Feature03, Output02 ou changement d'en-tête
Bulk** dans cette capture. Aucun nouvel offset HID n'est attribué.

Livrables : [opérations USB](D5NextAmbientQuadrantsEvents.csv), **1871 lignes** ;
[correspondances et repères](D5NextAmbientQuadrantsMapping.csv), **31 lignes**.

## Sources et empreintes

Originaux vidéo/USB dans `Controllers/AquaComputerD5NextController/` :

| Fichier | Octets | Durée | Informations |
| --- | ---: | ---: | --- |
| `d5next test6.mp4` | 16 903 125 | 38,133300 s | H.264,3840×2160,30fps,1144 images ; création MP4 `2026-11-11T10:09:57Z` |
| `d5next_baseline10.pcapng` | 1 845 124 | 63,181107 s | 14887 trames ;11:09:43.063661–11:10:46.244768 CET ;USBPcap2 |

Mire fournie par l'utilisateur, restée à son emplacement original :

```text
D5NEXT_AMBIENTpx_mire_4_quadrants_4K.png
```

Ce fichier fait29473 octets. Les17 autres PNG du dossier ne sont pas supposés
avoir été utilisés dans cette session. SHA-256, vidéo /PCAP /mire :

```text
8e5cb40a5bb6d7f458c59cafeed7db96795bfd52f97f7a5e6bc4d4e1b221cdaf
dd178781fce66ab9884a727530443fd263d2e4593a01e38e1f1a5a2796d5a03b
6208967e7376cac97ec62eebffbc1df2672bbbdb2dfe6aa2137aab35619cb455
```

La piste AAC signalée n'est ni extraite ni écoutée. Les images ciblées sont
extraites avec `-an` et conservées uniquement dans `.build/d5next-ambient-test6/`,
ignoré. Le PNG extérieur au dépôt est lu, pas modifié ni copié dans Git.

### Vérification de la mire source

Le PNG mesure exactement3840×2160. Lecture de tous les pixels : quatre couleurs
opaques, chacune présente **2073600 fois**. Vérification de la couleur attendue
à chaque coordonnée : **zéro différence** avec les rectangles suivants.

| Rectangle, bornes inclusives | RGB source | Position |
| --- | --- | --- |
| x0–1919,y0–1079 | FF0000 | Haut gauche |
| x1920–3839,y0–1079 | 00FF00 | Haut droite |
| x0–1919,y1080–2159 | 0000FF | Bas gauche |
| x1920–3839,y1080–2159 | FFFF00 | Bas droite |

Cette connaissance du fichier ne rend pas les images vidéo parfaitement pures :
compression, pointeur, fenêtres et changement du mode plein écran doivent être
distingués du PNG source.

## Conditions réellement visibles

La mire est visible en plein écran pendant les changements de bord. Aquasuite
est superposé dans la partie inférieure gauche, avec des déplacements et des
masquages de sa fenêtre. Il recouvre donc une partie d'une région que Bottom
ouLeft peut traiter. Cela explique une contamination possible, sans attribuer
chaque variation à un filtre. Vers33 s, la mire est présentée dans une fenêtre
de visualisation avec d'autres applications visibles : les données tardives
ne sont pas utilisées comme plateaux d'une mire pure plein écran.

Les instantanés Aquasuite montrent :

- **Top** encore appliqué à6,5 s ; Top reste affiché à7,75 s malgré l'ouverture
  précédente de la liste ; **Bottom** est appliqué à8,0 s.
- Bottom encore appliqué à14,0 s ; **Left** à14,5 s, encore visible à18 s.
- Left encore appliqué à21,5 s ; **Right** à22,0 s, encore visible à27 s.
- Reverse direction visiblement **OFF** ; Strips Brightness affiche **50 %**.
  Ce nombre n'est pas une lecture de master237.
- Les curseurs de région et de profondeur sont visibles aux extrémités dans
  les instantanés examinés ; leurs valeurs numériques ne sont pas relues ici.
  Les effets d'autres valeurs de profondeur ou d'intervalle ne sont pas généralisés.

Le fond Windows de l'aperçu de réglage reste visuellement semblable pendant
que la mire est affichée autour d'Aquasuite. **Ce fond n'est pas utilisé comme
preuve d'une capture live du contenu traité.** L'indicateur de rectangle demeure
utile pour lire le bord choisi, mais ne mesure pas les pixels réellement intégrés.

## Synchronisation par séquence distinctive

L'écart nominal des métadonnées vaut
`tUSB ≈ tVidéo +13,936339 s`. Il ne suffit pas à mesurer la réaction logicielle.
Cette fois, trois changements de bord donnent des motifs USB distinctifs.

Les mots sont lus selon l'ordre de composantes GRB établi précédemment. Pour
reproduire les repères, chaque mot est classé avec les seuils suivants :

- R : R>60, R>1,8G etR>1,8B ; G : G>60, G>1,8R etG>1,8B.
- B : B>60, B>1,4G etB>1,4R.
- Y : G>60, R>60 etB<0,25×min(G,R).
- Sinon : non classé. Ce sont des seuils d'analyse, pas des règles du protocole.

| Application visible | Fenêtre vidéo ancien→nouveau | Première opération au motif complet | tUSB | Différence effective |
| --- | --- | --- | ---: | --- |
| Top→Bottom | 7,75–8,00 s | **6517→6518**, BBBYYY | 23,805608 | 15,805608–16,055608 s |
| Bottom→Left | 14,00–14,50 s | **7714→7715**, RRRBBB | 29,993431 | 15,493431–15,993431 s |
| Left→Right | 21,50–22,00 s | **9279→9280**, GGGYYY | 37,583222 | 15,583222–16,083222 s |

Les intervalles se recouvrent autour de15,9 s. Ce rapprochement effectif inclut
le temps nécessaire pour atteindre les dominantes selon les seuils : **ce n'est
ni un décalage pur d'horloge, ni le premier paquet de réaction, ni un délai
contractuel du moteur**. Aucune dérive d'horloge n'est démontrée. La concordance
des trois motifs valide l'association de la vidéo et du PCAP sans imposer le
seul horodatage MP4.

Le motif Top RRRGGG existe déjà avant le début de la vidéo ; son premier paquet
n'est donc pas une ancre d'interaction filmée. Les échantillons de plateaux
ci-dessous sont associés à des états, pas à une image strictement synchrone.

## Preuves spatiales et offsets

Les indices de mots sont à base zéro, après l'en-tête4 octets. Les composantes
observées restent G=5+4i,R=6+4i,B=7+4i pouri0..5. Le premier octet de chaque mot
reste nul et sans rôle sémantique établi.

| Bord | Paire /tUSB | Motif des six mots | Limite |
| --- | --- | --- | --- |
| Top | **5750→5751 /20,017931 s** | RRR /GGG | Région supérieure non recouverte par la fenêtre dans les images concernées |
| Bottom | **6540→6541 /24,022540 s** | BBB /YYY | Bleu partiellement recouvert par Aquasuite |
| Left | **7976→7977 /32,020788 s** | RRR /BBB | Bas de la moitié gauche partiellement recouvert |
| Right | **9322→9323 /38,022727 s** | GGG /YYY | Moitié droite non recouverte par la fenêtre dans les images concernées |

Exemples minimaux des seules régions de mire non masquées :

| Mot /offsets | Top,5750 | Right,9322 |
| --- | --- | --- |
| 0 /4–7 | `00 00 F5 00` | `00 D6 01 00` |
| 1 /8–11 | `00 00 F6 00` | `00 D6 01 00` |
| 2 /12–15 | `00 00 F6 00` | `00 D6 01 00` |
| 3 /16–19 | `00 D4 00 00` | `00 DB FD 00` |
| 4 /20–23 | `00 D8 00 00` | `00 DB FD 00` |
| 5 /24–27 | `00 D8 00 00` | `00 DB FD 00` |

Les valeurs brutes Bottom/Left contaminées ne sont pas publiées ; les références
exactes et dominantes sont conservées dans le CSV et vérifiables dans le PCAP
local. Les amplitudes ne sont pas les255 de la mire : aucune loi de gain,
correction, gamma ou compensation n'est déduite.

La séparation **0–2 versus3–5** correspond à gauche/droite pour Top et à haut/bas
pour Right. Cela renforce le modèle de sélection longitudinale du test5 au-delà
du seul rectangle UI. Il reste impossible avec quatre zones uniformes de décider
si les trois mots d'un même triplet sont permutés, et de mesurer leurs positions
individuelles ou leurs zones de moyenne. Reverse est OFF : aucune conclusion
sur une permutation activée par Reverse ou sur le câblage des LED.

## Inventaire USB exhaustif et contrôles

Descripteurs38/40 : **0C70:F00E, bus2/adresse5**, IF0 vendor avec Bulk IN81/
OUT02, IF1 HID avec Interrupt IN83 ; maximum64 octets par paquet physique.
Aucun changement d'adresse identifié. **3738 trames cibles** sur14887.

| Échange | Nombre | Références |
| --- | ---: | --- |
| Énumération standard | 3 paires | 37→38,39→40,41→42 |
| Bulk OUT02 IF0 | **1804 paires de364 octets** | 85→86,t0,025641 à14884→14885,t63,179804 |
| Input01 IF1,Interrupt IN83 | **62 complétions de162 octets** | 658–14742 ;firmware1027 ;0,990031 Hz |
| HID GET/SET_REPORT,Feature03,Output02 | **0** | Aucune configuration ou SettingsSave capturé |
| Bulk IN,Interrupt OUT,format35 et autres transferts cibles | **0** | Absents de cette acquisition |

Appariement Bulk par référence de requête et IRP :1804/1804. Input01 comprend
60 paires, complétions658/1122 sans soumission capturée et soumissions14537/
14743 sans complétion capturée. Le CSV Events contient **1867 paires et4
extrémités**, soit1871 lignes, sans recomptage des complétions.

Tous les statuts cibles sont SUCCESS. Aucun paquet cible tronqué ou signalé
`_ws.malformed` relevé. Le firmware1027 est lu dans les62 Input01 ; **structure3,
mode18 et paramètres de configuration ne sont pas relus**, les rapports03 étant
absents. Aucun CRC03/02 ne peut être vérifié dans cette capture.

Le Bulk comporte **376 contenus distincts**, en-tête constant **0B 06 06 00**.
Seuls les18 octets des composantes des six mots varient ; octets4,8,12,16,20,24
et28–363 nuls. Les deux06 restent indissociables : groupe et nombre de LED ne
peuvent toujours pas être attribués à ces deux positions.

Cadence moyenne28,549187 Hz ; médiane des intervalles0,031272 s ; minimum
0,029813 etmaximum1,068294 s ;3 intervalles dépassent500 ms. Latences URB
0,502–1,029 ms. Ces mesures ne définissent pas une cadence obligatoire.
Les deux tests CRC-16/USB final (portées0–361 et1–361, motBE362–363) échouent
sur1804/1804 paquets ; aucune autre forme d'intégrité n'est exclue.

Bulk endpoint02 reste distinct de l'Output HID02. L'absence d'Output02 ne
permet aucune conclusion de persistance EEPROM ou de préférences PC.

## Apport au futur pilote et questions restantes

Ce lot ajoute **une confrontation spatiale pour chacun des quatre bords**,
dont2 propres et2 partiellement masquées, et3 repères de transition. Il
n'ajoute aucun champ values[],flag de configuration ou commande. Les59 puis49
correspondances des lots précédents ne sont ni modifiées ni recomptées.

Pour des tests hors ligne futurs, la mire source fournit un cas synthétique
de sélection par moitié et de bascule d'axe. Elle ne suffit pas pour implémenter
le moteur Aquasuite : il manque la découpe précise de chaque mot, la règle
de moyenne, la profondeur et l'intervalle réellement appliqués aux pixels,
Reverse, les gains, le routage de plusieurs groupes et le sens des deux06.
Le transport et les barrières d'écriture ne sont pas modifiés dans ce lot.

Prochaine acquisition utile : déplacer Aquasuite sur un **autre écran que la
mire**, conserver des réglages chiffrés, enregistrer des plateaux longs pour
chaque bord, puis utiliser des bandes identifiables individuellement sur les
deux axes. Pour préciser la profondeur, déplacer une frontière colorée
perpendiculaire au bord, sans déplacer les autres réglages. Ces expériences
sont proposées à l'utilisateur, pas exécutées ou automatiquement autorisées.

Outils : TShark/capinfos4.6.9, FFmpeg/FFprobe9.0.2, scripts PowerShell et lecteur
PNG .NET en lecture seule. Aucun code propriétaire ou Go copié ; recherches
aquapx/OpenRGB#2111 créditées avec les lots précédents. Originaux et extractions
restent locaux et ignorés. **Arrêt après analyse, sans écriture matérielle.**
