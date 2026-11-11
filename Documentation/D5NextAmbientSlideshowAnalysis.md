# D5 NEXT — AMBIENTpx : mires du test3 et capture07 remplacées

Analyse hors ligne du 11 novembre 2026. Cette note concerne **uniquement la
reprise validée par l'utilisateur**, identifiée par les empreintes ci-dessous.
Le premier test interrompu, dont la vidéo et le PCAP portaient les mêmes noms,
est exclu des conclusions. Aucun accès USB, modification C++, changement
Aquasuite, commit ou push n'a été effectué.

## Acquisitions et périmètre

Originaux dans `Controllers/AquaComputerD5NextController/` :

| Fichier | Octets | Durée | Métadonnées |
| --- | ---: | ---: | --- |
| `d5next test3.mp4` | 2 445 443 | 38,300000 s | H.264, 3840×2160, 30fps, 1149 images ; création MP4 `2026-11-11T09:24:35Z` |
| `d5next_baseline07.pcapng` | 1 437 208 | 69,276088 s | 9287 trames ; USBPcap2 ; 10:24:20.916067–10:25:30.192155 CET |

SHA-256 respectifs :

```text
491e94eed2d05139d2fbe6cc11827f84865bb58b1607b71f17257ceab5b723f4
d8fc93de506f504ab809fd95a317f4fd29f41ca6ac0d3e725fe9e70566a9f174
```

La vidéo montre des mires synthétiques : déplacements d'une bande rouge sur fond
noir, six bandes horizontales puis verticales, gris et aplats magenta/cyan/jaune/
blanc/bleu/vert/rouge. Une piste AAC existe, mais n'a été ni extraite ni écoutée.
Les images sont extraites avec `-an`, restent locales et ignorées par Git.
Le panneau Aquasuite n'est pas lisible dans cette vidéo : Display area, réglages
de traitement et luminosités ne sont pas mesurés ici.

L'association vidéo/PCAP est confirmée par l'ordre des mires et les réponses
spatiales et colorées du flux. Les dates seules ne servent pas de synchronisation.
Le [test4/capture08](D5NextAmbientDisplayOptionsAnalysis.md), enregistré ensuite,
apporte une vérification supplémentaire avec les options Display area visibles.

## Inventaire USB et contrôles

Descripteurs38/40 : **0C70:F00E, bus2/adresse5**, IF0 vendor Bulk IN81/OUT02,
IF1 HID Interrupt IN83 ; endpoints maximum64 octets par paquet physique.
Pas de changement d'adresse identifié. **4278 trames cibles**, dont :

| Transfert | Nombre | Preuves |
| --- | ---: | --- |
| Énumération standard | 3 paires | 37→38, 39→40, 41→42 |
| Bulk OUT02 IF0 | **2067 paires de364 octets** | 85→86 à9285→9286 ; tUSB0,000443–69,261178 s |
| Interrupt IN83 IF1, Input01 | **69 complétions de162 octets** | 89–9185 ; firmware1027 ; environ0,990063 Hz |
| SET/GET HID, Feature03, Output02, autres transferts cibles | **0** | Aucun rapport de configuration ou SettingsSave capturé |

Les soumissions Bulk sont appariées par `usb.request_in` et identité IRP.
L'Interrupt comprend67 paires, complétions89/243 sans soumission capturée,
et soumissions8885/9186 sans complétion capturée. Le
[CSV exhaustif des opérations](D5NextAmbientSlideshowEvents.csv) contient donc
**2141 lignes** : 2137 paires et4 extrémités. Aucun double comptage des complétions.

Tous les statuts cibles sont `SUCCESS` ; aucun paquet tronqué ni `_ws.malformed`
relevé. CRC03/02 : **non vérifiables ici, rapports absents**. Le firmware1027 est
lu dans les Input01 ; structure3 et mode18 ne sont pas relus dans cette session.
Les preuves de configuration des lots antérieurs restent séparées.

## Framing et fréquence

Les **2067 paquets** portent le préfixe `0B 06 06 00` et présentent **425 contenus
distincts**. Les seules positions variables restent5–7,9–11,13–15,17–19,21–23,25–27.
Octets4,8,12,16,20,24 et28–363 sont nuls. Cela reproduit la structure observée
dans la capture06 : six mots variables de4 octets après un en-tête4 octets.

Cadence moyenne **29,829311 Hz**, médiane des intervalles0,031262 s, minimum
0,029906 s, maximum0,063243 s. Latences URB0,507–1,048 ms. Aucun arrêt du flux
ni réponse Bulk IN n'est observé. Les variations d'images ne sont pas des
SettingsSave ; aucun rapport HID n'est nécessairement émis à chaque mire.

CRC-16/USB final testé sur0–361 et1–361, comparé au motBE362–363 : **0/2067**
correspondances dans les deux cas. Ce constat ne prouve pas l'absence de tout
autre mécanisme d'intégrité. L'octet0B n'est pas un identifiant HID dans ce transport.

## Synchronisation et limites temporelles

Le décalage nominal des métadonnées est `tUSB ≈ tVidéo +14,083933 s`. Ce n'est
pas une calibration. Les changements de mire ont été repérés à30fps et rapprochés
de variations distinctives des composantes USB. Pour rendre les repères
reproductibles, le tableau utilise des **franchissements de seuils**, pas une
prétendue première trame de réaction. C1/C2/C3 désignent les moyennes des six
octets de même position dans les mots, avant attribution aux couleurs.

| Transition vidéo | tVidéo | Repère USB / tUSB | Critère local | Différence effective |
| --- | ---: | --- | --- | ---: |
| Gris→magenta | 21,000000 | 3879 /34,895515 | C2>145 | 13,895515 s |
| Magenta→cyan | 23,466667 | 4035 /37,017306 | C1>25 | 13,550639 s |
| Cyan→jaune | 25,300000 | 4191 /39,170571 | C2>50 | 13,870571 s |
| Jaune→blanc | 27,500000 | 4397 /41,228010 | C3>20 | 13,728010 s |
| Blanc→bleu | 30,200000 | 4627 /43,851035 | C1<170 | 13,651035 s |
| Bleu→vert | 32,000000 | 4755 /45,907645 | C1>20 | 13,907645 s |
| Vert→rouge | 34,033333 | 4875 /47,775994 | C2>30 | 13,742661 s |
| Rouge→noir | 36,500000 | 5029 /50,177091 | C2<180 | 13,677091 s |

Chaque seuil est cherché dans la fenêtre locale de la transition, pas sur toute
la capture. Moyenne des différences13,752896 s, étendue13,550639–13,907645 s.
Elles incluent capture vidéo, traitement logiciel et seuil choisi. **Aucune dérive
d'horloge ni durée de filtre n'est établie.** Les images et paquets d'un plateau
peuvent être associés à la même mire sans être exactement synchrones.
Le [CSV des correspondances](D5NextAmbientSlideshowMapping.csv) conserve21 lignes :
8 repères temporels,6 mots de mire verticale,7 exemples d'aplats.

## Résultat nouveau : composantes et ordre spatial observé

La mire verticale est visible entre16,133333 et18,500000 s. Les six bandes sont,
de gauche à droite, rouge, vert, bleu, jaune, cyan, magenta. La trame **3695→3696**,
tUSB32,057986 s, fournit les mots suivants, limités à cette mire synthétique :

| Mot | Offsets absolus | Octets | Lecture G/R/B | Bande correspondante |
| ---: | --- | --- | --- | --- |
| 0 | 4–7 | `00 03 E2 00` | 3/226/0 | Rouge |
| 1 | 8–11 | `00 B7 0A 00` | 183/10/0 | Vert |
| 2 | 12–15 | `00 0D 04 8D` | 13/4/141 | Bleu |
| 3 | 16–19 | `00 B3 D1 0C` | 179/209/12 | Jaune |
| 4 | 20–23 | `00 C6 1C 83` | 198/28/131 | Cyan |
| 5 | 24–27 | `00 1C CB 96` | 28/203/150 | Magenta |

Les trois primaires, leurs combinaisons et les aplats à d'autres instants
établissent la correspondance **`00 G R B`**, pour les six mots actifs de ce
flux. Confirmé par plusieurs observations indépendantes, et conforté par
le test4. Pour le mot d'indice i : **G=5+4i, R=6+4i, B=7+4i**, i=0..5.
Ce résultat porte sur les composantes liées à l'écran dans la charge USB, pas
sur une mesure physique des LED de la pompe.

Contrôles indépendants avec le premier mot : magenta3961=`00 0F EC 98`,
cyan4183=`00 C5 14 93`, jaune4381=`00 C5 E7 00`, blanc4569=`00 C4 E7 91`,
bleu4729=`00 06 0E 9E`, vert4855=`00 C6 01 0F`, rouge5019=`00 17 F0 00`.
Les dominantes sont concordantes. **Les amplitudes ne sont pas les RGB exacts
de la mire** : gain, correction, moyenne spatiale et transitions ne sont pas
séparés. Le blanc n'est notamment pas trois255.

L'ordre des six mots suit celui des six bandes dans cette configuration.
Cela ne démontre ni l'ordre physique des LED, ni un Reverse général, ni
l'algorithme d'échantillonnage de toutes les zones. L'octet00 de chaque mot n'a
pas de rôle établi : ne pas le baptiser alpha. Les deux06 de l'en-tête restent
indissociables ; groupe6 et nombre6 sont des hypothèses compatibles, pas des
champs dont la fonction est prouvée. La capacité arithmétique `4+90×4` ne
constitue toujours pas une validation d'envoi à90 LED.

## Bilan et suites

**Trois positions de composantes identifiées**, ordre relatif de six mots
confronté à une mire connue,425 contenus distincts ; **aucun nouveau champ
values[], flag ou commande de sauvegarde** identifié. Les59 correspondances
anciennes et49 du Lot7 restent inchangées ; ce résultat enrichit le transport,
pas le nombre de paramètres d'effet déjà comptés.

Analyse : TShark/capinfos4.6.9, FFmpeg/FFprobe9.0.2, appariement URB et contrôles
hors ligne PowerShell. Intermédiaires dans `.build/d5next-ambient-slideshow-valid/`.
Originaux, images et extractions restent ignorés. Aucun contenu d'écran personnel
n'a été reconstruit à partir de l'USB ; seules les valeurs minimales associées
aux mires synthétiques figurent dans les tableaux.

Pour de futures fixtures C++/aquapx : vecteurs synthétiques de six mots GRB,
préservation de l'en-tête et du suffixe observés, séparation du transport Bulk
et des rapports HID. Avant un émetteur actif, il reste à établir les deux06,
l'allocation des groupes, les gains et filtres, le routage, les préconditions,
le comportement arrêt/reprise et les autres nombres de LED. Les recherches
initiales aquapx et OpenRGB#2111 restent créditées ; aucune copie de code Go
ou propriétaire. **Aucune écriture matérielle autorisée par cette analyse.**
