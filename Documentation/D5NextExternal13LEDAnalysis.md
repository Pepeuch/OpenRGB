# D5 NEXT — sortie RGBpx, contrôleur externe de13 LED : test7 / USB11

Analyse hors ligne du 11 novembre 2026, branche `feat/d5next`, après l'archive
documentaire `c806a6ac`. L'utilisateur indique avoir ajouté13 LED sur la sortie,
les observer indépendantes des LED de la pompe, et avoir passé la sortie en RGBpx.
Ces indications sont distinguées ci-dessous des preuves USB et de la vidéo écran.
Aucun accès matériel, envoi HID, changement Aquasuite/C++, compilation, commit
ou push n'est effectué pour cette analyse.

## Résultats principaux

- Le **LED Controller1** affiché sous **Strip** correspond au **groupe0**, base240,
  `strip=0`. Il est déjà configuré `start=0,count=13` dans le premier Feature03.
- Le groupe **Base** affiché sous **Pump** correspond au **groupe6**, base660,
  `strip=1,start=0,count=6`. Ses changements n'altèrent pas le groupe0, et
  réciproquement. Cela établit l'indépendance des **champs de configuration**.
- Un **second groupe Pump**, g7, est aussi actif et couvre les mêmes six positions
  logiques. Il n'est pas modifié. Ne pas le considérer comme inutilisé et ne pas
  en déduire un deuxième ensemble physique de six LED.
- Les gestes de déplacement/redimensionnement confirment **start241** et
  **count242**, deux octets distincts du groupe0. Un déplacement crée un
  recouvrement complet avec une partie de la plage de g1 ; la priorité de rendu
  n'est pas déterminée.
- **14 Feature03**, tous809 octets/structure3/CRC valide, et **7 Output02
  SettingsSave=2**, CRC valide. **Aucun Bulk** : cette capture ne précise pas
  le rôle des deux06 de l'en-tête AMBIENTpx.

Livrables : [toutes les opérations USB](D5NextExternal13LEDEvents.csv),119 lignes,
et [chronologie des configurations](D5NextExternal13LEDMapping.csv),14 lignes.

## Acquisition et identification

Originaux locaux dans `Controllers/AquaComputerD5NextController/` :

| Fichier | Octets | Durée | Informations |
| --- | ---: | ---: | --- |
| `d5next test7.mp4` | 34444316 | 78,533300 s | H.264,1920×1080,30fps,2356 images ; création MP4 `2026-11-11T12:51:00Z` |
| `d5next_baseline11.pcapng` | 998104 | 93,924727 s | 14598 trames ;13:50:46.329745–13:52:20.254472 CET ;USBPcap2 |

SHA-256 respectifs :

```text
d987a06d32f46f092da96325ff5d63cb34ea5e9542d144bef8e6b04ff1e6f185
494f1b4df09a7cf09048f73c15cf5e8587b783937fb04f0ea8d8184da3a46a2f
```

La piste AAC signalée n'est pas consultée. Les extractions avec `-an` et les
rapports complets intermédiaires restent dans `.build/d5next-external13/`,
ignoré par Git. Aucun rapport complet809 ni mesure hydraulique n'est publié.
Les données conservées dans les CSV sont les différences RGB et leurs CRC.

Descripteurs38/40 : **0C70:F00E, bus2/adresse5**, IF0 vendor Bulk IN81/OUT02,
IF1 HID Interrupt IN83 ; maximum64 octets par paquet physique. Aucun changement
d'adresse identifié. Firmware **1027** lu dans93 Input01 ; structure **3** relue
dans les14 Feature03. Le nom UI « Controller1 », l'interface HID1 et le champ
`strip=1` ne désignent pas la même chose.

## Sortie RGBpx : portée de l'observation

La vidéo commence par l'ouverture de la page RGBpx. À3 s, les sections Strip et
Pump sont visibles, sans la bannière aquabus rencontrée dans certaines sessions
antérieures. Le réglage est donc cohérent avec la déclaration de l'utilisateur
d'une sortie passée en RGBpx.

**La bascule System aquabus→RGBpx n'est pas filmée ni isolée dans ce PCAP.** Il
n'y a pas de paire de rapports avant/après ce réglage. Les seuls contrôles
applicatifs capturés sont les Feature03 et SettingsSave décrits plus loin.
Le premier rapport contient déjà l'allocation13 ; il ne documente pas sa
création ou le branchement physique. Le SET_CONFIGURATION USB standard41→42
du préambule ne doit pas être interprété comme une activation RGBpx.

Aucun bit, offset ou commande de sélection aquabus/RGBpx n'est donc attribué.
L'indépendance visuelle du matériel est rapportée par l'utilisateur ; la vidéo
filme Aquasuite, pas une mesure du rendu physique, du courant ou du câblage.

## Cartographie des huit groupes

Base d'un groupe g : `240+70g`. Les quatre premiers octets sont strip,start,
count,mode. Indices internes à base zéro ; plages affichables à base un.
Les états initial et final de l'allocation sont identiques :

| Groupe /base | Strip logique | Start | Count | Plage logique | Mode initial→final | Correspondance UI |
| --- | ---: | ---: | ---: | --- | --- | --- |
| g0 /240 | 0 | 0 | **13** | 0–12 | 02→02 | **Strip /LED Controller1**, confirmé par ses changements |
| g1 /310 | 0 | 13 | 20 | 13–32 | 10→10 | Contrôleur externe suivant, non modifié |
| g2 /380 | 0 | 33 | 15 | 33–47 | 0B→0B | Externe, non modifié |
| g3 /450 | 0 | 48 | 15 | 48–62 | 04→04 | Externe, non modifié |
| g4 /520 | 0 | 63 | 15 | 63–77 | 04→04 | Externe, non modifié |
| g5 /590 | 0 | 78 | 12 | 78–89 | 04→04 | Externe, non modifié |
| g6 /660 | 1 | 0 | **6** | 0–5 | 01→02 | **Pump /Base**, confirmé par ses changements |
| g7 /730 | 1 | 0 | **6** | 0–5 | 02→02 | Second groupe Pump, compatible avec « anneaux » ; non varié |

Les six groupes Strip allouent90 positions logiques. **Cela ne prouve pas que90
LED physiques soient branchées** :13 sont déclarées connectées par l'utilisateur.
La plage de g0 correspond alors aux positions UI1–13. Deux groupes partageant
les mêmes six positions Pump ne prouvent pas12 LED physiques.

Le dialogue « Strip name » visible vers8 s et les noms affichés sont des éléments
UI ; aucune écriture03 correspondante à ce renommage n'est identifiée. Aucun
stockage ou encodage de ces chaînes dans le rapport HID n'est attribué.

## Synchronisation propre à test7

Métadonnées : `tUSB nominal = tVidéo +13,670255 s`. Trois changements de titre
de panneau sont encadrés par images ciblées :

| Action visible | Ancien état à tVidéo | Nouvel état à tVidéo | Feature03 /tUSB | Différence effective possible |
| --- | ---: | ---: | --- | --- |
| Base : Breathing→Single color | 18,0 | 18,5 | 5629 /32,167520 | 13,667520–14,167520 s |
| Base : Single color→Breathing | 27,0 | 27,5 | 6661 /41,044434 | 13,544434–14,044434 s |
| LED Controller1 : Breathing→Single color | 41,5 | 42,0 | 8553 /55,344055 | 13,344055–13,844055 s |

Ces intervalles se recouvrent et sont compatibles avec le rapprochement nominal.
Ils incluent les délais UI/application, pas seulement les horloges. Aucune dérive
ni latence contractuelle n'est déduite. L'état antérieur à5629 n'existe pas dans
les rapports USB capturés, même si son titre est visible dans la vidéo.

Le CSV propose généralement une image à `tUSB−13,4 s`, repère de lecture légèrement
après l'apparition attendue. Ce n'est pas un instant exact d'application. Cela
compte notamment pour les écritures12 puis13, séparées de0,536904 s : la valeur12
est bien présente sur USB, mais l'image associée peut déjà afficher13. Ces deux
opérations restent distinctes ; aucun nombre illisible n'est inventé dans l'UI.

## Chronologie des configurations et vérification CRC

Chaque Feature03 est Control EP0 OUT, IF1, type **Feature**, ID03, `wValue=0303`,
809 octets. CRC-16/USB sur1–806, stockage BE807–808, structure BE1–2=3.
Les14 CRC sont vérifiés par deux formulations, avec le vecteur
`123456789 → B4C8` : poly8005/refletA001,initFFFF,xoroutFFFF,RefIn/RefOut vrais.

| Paire USB | tUSB | Groupe concerné /différence fonctionnelle | CRC |
| --- | ---: | --- | --- |
| 5629→5630 | 32,167520 | Premier état complet : g0 Breathing13 LED ;g6 Single color6 LED | 3F7F |
| 5885→5886 | 34,497618 | g6 HSV0.H :0000→04BE | ECA1 |
| 6661→6662 | 41,044434 | g6 mode01→02, bindings/paramètres/couleur réinitialisés ensemble | 25F1 |
| 7423→7426 | 46,164324 | g6 HSV0.H :0080→053F | 4CD4 |
| 8553→8554 | 55,344055 | g0 mode02→01, bindings/paramètres/couleur réinitialisés ensemble | 2F4F |
| 9239→9240 | 61,804814 | g0 mode01→02, initialisations inverses observées | B6D1 |
| 9713→9716 | 64,934109 | g0 HSV0.H :0080→04FF | 4CD4 |
| 11045→11046 | 71,012855 | g0 start0→16, count13 conservé | A360 |
| 11563→11568 | 74,577071 | g0 start16→0 | 4CD4 |
| 12179→12180 | 79,696915 | g0 start0→5, count13→8 | 5FCE |
| 12475→12476 | 81,934223 | g0 start5→0, count8→13 | 4CD4 |
| 13121→13122 | 85,508167 | g0 count13→5 | 9660 |
| 13601→13602 | 88,801940 | g0 count5→12, état intermédiaire | F783 |
| 13653→13654 | 89,338844 | g0 count12→13 | 4CD4 |

Dans les13 comparaisons successives, seuls les blocs **g0 oug6 et le CRC** changent.
Aucun autre champ, paramètre hydraulique, master237 ou flags238–239 ne change.
Master reste255 et flags0000. Les autres groupes sont préservés octet par octet.

Les rapports7423,9713,11563,12475,13653 sont **identiques sur les809 octets**,
vérification indépendante par empreinte complète en local. La conclusion ne
repose pas sur leur seul CRC commun4CD4. Il y a10 configurations distinctes.
Le rapport complet et son empreinte ne sont pas publiés.

### Même organisation des effets sur les groupes externe et Pump

| Champ | g0,base240 | g6,base660 |
| --- | --- | --- |
| Strip /Start /Count /Mode | 240 /241 /242 /243 | 660 /661 /662 /663 |
| Flags BE16 | 244–245 | 664–665 |
| Binding1.y1/y2 | 254 /255 | 674 /675 |
| Binding2.y1/y2 | 260 /261 | 680 /681 |
| values[0..3],BE16 | 262–269 | 682–689 |
| HSV0 H BE16,S,V | 286–287,288,289 | 706–707,708,709 |

Sur les transitions **Single color01↔Breathing02 observées**, les valeurs de
bindings y1/y2 passent0/255↔1/100, les quatre values passent0,0,0,0↔60,80,5,20.
La sélection Breathing initialise H à0080 ; Single color initialise H à0000.
Ces changements sont couplés au choix d'effet et ne constituent pas des
presets universels. Les trois modifications de teinte isolées5885,7423,9713
préservent S=FF,V=FF et tous les autres octets hors H/CRC.

Cela confirme le même layout70 octets sur deux groupes distincts, sans démontrer
la validité de toute transition, source ou valeur sur la sortie externe. Le
rapport du test ne change aucune politique d'écriture du codec.

## Allocation : déplacement, longueur et recouvrements

Start etCount sont **deux octets**, pas un entier16 bits. Les manipulations
visibles du rectangle LED Controller1 correspondent à :

| Trame | Start /Count après écriture | Plage logique g0 | Interprétation |
| --- | --- | --- | --- |
| 9713 | 0 /13 | 0–12 | État de référence,13 LED déclarées |
| 11045 | 16 /13 | 16–28 | Déplacement complet ; longueur conservée |
| 11563 | 0 /13 | 0–12 | Retour au début |
| 12179 | 5 /8 | 5–12 | Début déplacé, fin logique12 conservée ; modification couplée |
| 12475 | 0 /13 | 0–12 | Restauration des deux champs |
| 13121 | 0 /5 | 0–4 | Réduction par la fin, start conservé |
| 13601 | 0 /12 | 0–11 | Agrandissement intermédiaire |
| 13653 | 0 /13 | 0–12 | Retour à13 |

À11045, g1 reste `start=13,count=20`, soit13–32 : **les13 positions16–28 de g0
sont aussi allouées à g1**. Les rectangles sont sur des lignes UI distinctes,
mais ne désignent pas nécessairement des LED physiques différentes. À12179,
le mouvement conserve au contraire la frontière avec g1, sans recouvrement.
Ces gestes ne déplacent pas automatiquement les autres groupes.

G6 etg7 partagent aussi la même plage Pump0–5 dans toutes les configurations.
L'effet affiché en dernier, une priorité de groupe, un mélange, un masquage ou
une règle d'activation ne sont **pas déterminés** par cette capture. Un CRC valide
ne résout pas la priorité de rendu. Le déplacement de g0 au-delà des13 LED
déclarées ne prouve pas que d'autres LED soient connectées ou aient réagi.

## Inventaire USB et sauvegardes

**234 trames cibles**, toutes de statut SUCCESS, sans troncature ni
`_ws.malformed` relevé :

| Échange | Nombre | Références |
| --- | ---: | --- |
| Énumération standard | 3 paires | 37→38,39→40,41→42 |
| SET_REPORT Feature03,EP0 IF1 OUT | 14 paires | Tableau chronologique ci-dessus |
| SET_REPORT Output02,EP0 IF1 OUT | 7 paires | Tableau ci-dessous |
| Interrupt IN83,Input01 | 93 complétions de162 octets | 227–14517 ;0,990099 Hz |
| GET_REPORT,HID autres IDs,Interrupt OUT,Bulk IN/OUT | 0 | Absents du périphérique cible |

Toutes les opérations Control sont appariées par référence de requête et IRP.
Input01 comprend91 paires, complétions227/493 sans soumission capturée et
soumissions14286/14518 sans complétion capturée. Le CSV conserve **115 paires
et4 extrémités**,119 lignes, chaque trame cible une seule fois.

Les7 commandes sont bien **Output02**, pas Feature02 ou Bulk endpoint02 :
`wValue=0202`,interface1,11 octets,SettingsSave=2. CRC-16/USB sur1–8,
stockage BE9–10=`34C6`, correct7/7. Commande minimale observée :

```text
02 00 00 00 02 00 00 00 00 34 C6
```

| Paire Output02 | tUSB | Dernier Feature03 précédent | Délai |
| --- | ---: | ---: | ---: |
| 6345→6346 | 38,344375 | 5885 | 3,846757 s |
| 7313→7314 | 44,871818 | 6661 | 3,827384 s |
| 7807→7808 | 49,175106 | 7423 | 3,010782 s |
| 8947→8948 | 59,170765 | 8553 | 3,826710 s |
| 10255→10256 | 68,766110 | 9713 | 3,832001 s |
| 12053→12054 | 78,403066 | 11563 | 3,825995 s |
| 14465→14466 | 93,163617 | 13653 | 3,824773 s |

Pas de commande après chaque modification ; délai non contractuel et aucune
persistance EEPROM déduite. Les effets01/02 observés n'exigent pas de flux PC
capturé ici ; **l'absence de Bulk ne prouve pas que le passage en RGBpx interdise
les flux audio/vidéo**. Aucun en-tête0B avec13 ou19 LED ne peut être étudié dans
ce PCAP, puisqu'il n'existe aucune opération Bulk.

## Conséquences pour la reprise Linux

Deux champs d'allocation externes sont désormais confrontés aux gestes sur g0 ;
le layout d'effet et la séparation de g0/g6 sont validés sur cette configuration
firmware1027/structure3. Cela ne constitue pas une nouvelle autorisation de SET.

Le C++ actuel est seulement lu pour comparaison : `D5NextRGBpx::BuildPatch`
refuse les groupes autres que le groupe interne6 et ne propose pas de changement
d'allocation. Ces barrières restent intactes. Le commentaire évoquant un groupe7
inutilisé ne décrit pas les nouveaux états : **g7 est actif ici**. Les futures
fixtures devront couvrir ce cas et les recouvrements, sans supposer leur priorité.

Propositions avant tout développement actif : fixtures synthétiques de groupes0/
6/7, préservation de tous les champs hors requête, déplacements avec frontière
conservée, refus ou politique explicite pour les recouvrements. Une capture
séparée filmant **System et la bascule aquabus/RGBpx**, avec un avant/après
complet, serait nécessaire pour attribuer son bit ou transport. Pour résoudre
les deux06 du Bulk AMBIENTpx, il faudra un test PC de contenu effectivement actif,
avec groupe et nombre de LED variés séparément ; ce lot n'en apporte pas la preuve.

Références : [moteur RGBpx](D5NextRGBpxEngineAnalysis.md),
[refonte C++](D5NextRGBpxCppRefactor.md),
[corrections Lot6](D5NextLot6Corrections.md),
[campagnes avancées](D5NextRGBpxAdvancedVideoUSBMapping.md) et
[streaming](D5NextRGBpxStreamingAnalysis.md). Recherches initiales aquapx et
contributeurs OpenRGB#2111 crédités ; aucun code Go ou propriétaire repris.
TShark/capinfos4.6.9,FFmpeg/FFprobe9.0.2 et PowerShell hors ligne. Les preuves
antérieures ne sont pas réécrites. **Arrêt après analyse, sans écriture matérielle.**
