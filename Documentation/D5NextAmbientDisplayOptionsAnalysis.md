# D5 NEXT — AMBIENTpx : test4, capture08 et options d'affichage

Complément du 11 novembre 2026 au
[test3 sur mires](D5NextAmbientSlideshowAnalysis.md). Analyse exclusivement hors
ligne, sans modification C++, Aquasuite ou matérielle. Les preuves antérieures
ne sont pas remplacées.

## Acquisition

Fichiers dans `Controllers/AquaComputerD5NextController/` :

| Fichier | Octets | Durée | Informations |
| --- | ---: | ---: | --- |
| `d5next test4.mp4` | 9 434 859 | 83,600000 s | H.264, 3840×2160, 30fps,2508 images ; création MP4 `2026-11-11T09:28:41Z` |
| `d5next_baseline08.pcapng` | 2 019 976 | 109,635905 s | 13112 trames, USBPcap2 ;10:28:32.594668–10:30:22.230573 CET |

SHA-256, vidéo puis PCAP :

```text
41b41ada71e55d2be9cb58f155cd7657756ffffbb9ce37b979129c580c0b99e3
3d2a50762be0fe23730a33ccb602f1098e9d871a1e877a16ea942dddbede6b3a
```

La piste AAC signalée n'est pas consultée. Les images sont des extractions vidéo
locales avec `-an` ; aucune illustration du bureau ou identité d'écran n'est publiée.

## Options réellement visibles et corrélation

La vidéo contient trois passages des mires, séparés par l'affichage du panneau
AMBIENTpx. Les instantanés établissent :

| tVidéo | Constat | Qualification |
| ---: | --- | --- |
| 31 et32 s | Display area **Bottom** | Confirmé visuellement ; association au passage précédent compatible, pas un état lu au début de ce passage |
| 33 s | Liste ouverte, Bottom encore sélectionné, curseur sur Left | Le survol n'est pas encore un changement appliqué |
| **33,5 s** | Display area **Left**, rectangle d'aperçu à gauche | Confirmé visuellement avant le deuxième passage |
| 58 et60 s | **Left** encore affiché ; menu ouvert à60 | Confirmé visuellement après le deuxième passage |
| **61 s** | Display area **Right**, rectangle d'aperçu à droite | Confirmé visuellement avant le troisième passage |

Le libellé **Strips Brightness38 %** est lisible lors de ces apparitions ; il
ne s'agit pas d'une lecture de master237 sur USB. Le curseur Brightness du
panneau AMBIENTpx est à droite, sans valeur numérique lisible ; les curseurs
Desktop range ne sont pas testés isolément. Reverse est visible OFF. Ne pas
transformer ces positions en pourcentages ou en règle géométrique non mesurée.

Les métadonnées donnent un écart nominal `tUSB−tVidéo=8,405332 s`. Les trois
séquences de couleurs et de bandes identifient la session, mais le délai de
traitement et les transitions varient. Les exemples ci-dessous se rapprochent
par leurs mires, pas par une horloge unique calibrée. Leurs différences
`tUSB−tVidéo-échantillon` vont de7,512499 à9,227224 s : ce ne sont pas des délais
de commande mesurés, puisque l'échantillon est situé à l'intérieur d'un plateau.
Le décalage13,75 s du test3 n'est pas réutilisé. **Aucune dérive d'horloge établie.**

## Inventaire et contrôles USB

Descripteurs38/40 : identité**0C70:F00E, bus2/adresse5**, IF0 vendor Bulk IN81/
OUT02, IF1 HID Interrupt IN83. Maximum64 octets par paquet physique. Aucun
changement d'adresse identifié, statut d'échec cible, paquet tronqué ou
`_ws.malformed` relevé.

| Opération | Nombre | Références |
| --- | ---: | --- |
| Énumération standard | 3 paires | 37→38,39→40,41→42 |
| Bulk OUT02 IF0 | **3265 paires de364 octets** | 85→86,t0,015504 à13111→13112,t109,635171 |
| Input01 IF1,Interrupt IN83 | **108 complétions de162 octets** | 181–13009 ; firmware1027 ;0,990044 Hz |
| HID Feature03,Output02,GET_REPORT,Interrupt OUT,Bulk IN | **0** | Aucune configuration ou sauvegarde capturée |

Total cible **6752 trames**. Input01 comprend106 paires, complétions181/293 sans
soumission capturée, soumissions12742/13010 sans complétion capturée. Le
[CSV exhaustif](D5NextAmbientDisplayOptionsEvents.csv) conserve **3378 lignes** :
3374 paires et4 extrémités. Appariement par référence de requête et IRP ; les
complétions ne sont pas recomptées comme des écritures. Structure et mode ne sont
pas relus dans cette capture ; aucun CRC03/02 ne peut être testé en leur absence.

Le flux contient **676 contenus distincts**. Préfixe constant `0B 06 06 00` ;
mêmes18 positions variables dans les six mots ; premiers octets des mots et
suffixe28–363 toujours nuls. Aucun format35 observé. CRC-16/USB final sur0–361
ou1–361 : **0/3265 correspondances** dans chaque portée testée, pas preuve
d'absence de tout autre mécanisme d'intégrité.

Cadence moyenne29,775679 Hz ; médiane des intervalles0,031297 s ; minimum
0,029938, maximum1,076923 s. La pause maximale empêche d'en faire une cadence
garantie. Le flux existe avant la vidéo et continue après ; aucun arrêt final
du moteur ni redémarrage contrôlé n'est démontré.

## Confrontation des trois passages

Chaque mire verticale comporte rouge,vert,bleu,jaune,cyan,magenta de gauche à
droite. Les trois opérations suivantes sont des exemples des fenêtres associées,
pas une mesure du délai entre l'image échantillonnée et le paquet :

| Passage | Vidéo : exemple / fenêtre de mire verticale | Paire USB / tUSB |
| --- | --- | --- |
| 1, Bottom observé ensuite | 14,8 /14,166667–15,666667 s | **2905→2906 /24,027224** |
| 2, Left observé avant/après | 46,8 /46,600000–47,600000 s | **6091→6092 /55,002910** |
| 3, Right observé avant | 72,5 /72,100000–73,233333 s | **9137→9138 /80,012499** |

Seuls les six mots des mires synthétiques sont donnés ici :

| Mot/bande | 2905 | 6091 | 9137 |
| --- | --- | --- | --- |
| 0 rouge | `00 05 F7 00` | `00 0F AA 00` | `00 0E AF 00` |
| 1 vert | `00 CE 10 00` | `00 BD 41 06` | `00 A7 00 06` |
| 2 bleu | `00 10 0B 97` | `00 00 3E 96` | `00 06 08 8C` |
| 3 jaune | `00 C6 E9 0D` | `00 C5 FE 00` | `00 B2 C6 03` |
| 4 cyan | `00 D8 25 8F` | `00 C6 48 93` | `00 AE 07 86` |
| 5 magenta | `00 20 DB A0` | `00 10 EE 99` | `00 0E B2 8C` |

Les primaires et combinaisons confortent **00 G R B** dans les trois passages.
Le [CSV de correspondances](D5NextAmbientDisplayOptionsMapping.csv) contient
**22 lignes** :18 mots de mires et4 états UI. Les dominantes des six bandes
restent dans le même ordre dans ces exemples.

**Cela n'établit pas que Left et Right sont équivalents**, ni qu'ils ignorent le
réglage : profondeur, rectangle, moyennes, filtres et transitions ne sont pas
isolés. La mire vidéo n'est pas une mire générée par Codex au cours de l'analyse.
L'algorithme d'intégration spatiale n'a pas été reconstitué ; en particulier,
une loi de rotation ou d'inversion systématique des six mots n'est pas démontrée.

## Bilan pour le futur pilote

Les options **Bottom/Left/Right sont constatées visuellement**. Aucun offset
Feature03, flag HID, commande SettingsSave ou changement d'en-tête Bulk ne les
accompagne dans cette capture. Elles relèvent de réglages visibles côté PC ; leur
lieu de sauvegarde et leur traduction exacte en échantillons restent inconnus.

Résultat cumulé avec le test3 : **5332 opérations Bulk**, toutes appariées,
trois positions de composantes attribuées dans chacun des six mots observés,
quatre exemples de mires verticales concordants. Cela n'ajoute aucun nouveau
paramètre values[] au catalogue. Les deux06 restent inconnus, l'octet initial
de chaque mot n'est pas identifié comme alpha, et les amplitudes ne définissent
pas une conversion RGB brute garantie.

Prochaine expérience utile : filmer le panneau et la mire ensemble ; garder
le même affichage, isoler une zone puis une profondeur faible explicitement
chiffrée ; faire varier une région connue à la fois et tester Reverse séparément.
Comparer les six mots pendant des plateaux longs, avec activation/arrêt explicites.
Ce sont des expériences proposées à l'utilisateur, **aucune écriture matérielle
n'est autorisée ou effectuée par ce rapport**.

Intermédiaires ignorés : `.build/d5next-ambient-test4/`. TShark/capinfos4.6.9,
FFmpeg/FFprobe9.0.2, contrôles PowerShell hors ligne. Aucun code propriétaire,
Go ou tiers copié/modifié ; recherches aquapx et OpenRGB#2111 créditées avec les
lots précédents. Aucun test C++, accès matériel, commit ou push.
