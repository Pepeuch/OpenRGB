# D5 NEXT — Windows, lot 2 : changements RGB observés

Date : 10 novembre 2026, Europe/Paris. Branche : `feat/d5next`.
Révision des sources examinées : `3f80b24c35829c52ecb993daa8dd62221ed1be09`.

**Mise à jour :** les sections 1–9 concernent `d5next_baseline02.pcapng`.
La nouvelle capture **`d5next_baseline03.pcapng`**, analysée en section 10,
montre désormais **Rotating Rainbow03 → Fixed01 → couleur choisie → Rainbow03**.
Elle confirme des changements annexes lors du passage Rainbow/fixe et du retour.
La capture **`d5next_baseline04.pcapng`**, analysée en section 11, isole les
changements de luminosité **60 % → 100 % → 0 %** sur le seul octet237 hors CRC.
Le premier 0 % annoncé par l'utilisateur n'y est pas transmis dans un SET visible.

**Résultat : 13 SET Feature `03` complets, tous avec CRC valide et structure 3,
et 6 SET Output `02` portant SettingsSave=2, tous avec CRC valide.** Les commandes
`02` passent par Control EP0, interface 1, avec `wValue=0202` : le choix officiel
**Output**, auparavant incertain, est désormais observé dans cette capture.

L'utilisateur a confirmé avoir volontairement effectué plusieurs changements
dans cette acquisition. La séquence effective est plus large que le scénario
initialement annoncé : couleurs, luminosité, flags et modes `01 → 13 → 01`.
**Pour le contrôleur des six LED (bloc 660–729), aucun mode `03` ni HSV rose
`0580/96/FF` n'est transmis dans les configurations capturées.** Le passage initial Rainbow → Fixed Color et le retour Rainbow
ne sont donc pas observables ici. Aucun résultat visuel rose n'est certifié.

## 1. Acquisition retenue et méthode

Deux captures existent sous `Controllers/AquaComputerD5NextController/` :

| Fichier | Taille | Fenêtre enregistrée, heure de Paris | Dernière modification du fichier, heure de Paris |
|---|---:|---|---|
| `d5next_baseline.pcapng` | 131 588 octets | 18:09:45.149488–18:10:09.002369 | 2026-11-10 18:10:41.9523693 |
| **`d5next_baseline02.pcapng`** | **543 816 octets** | **18:25:46.503792–18:26:54.127553** | **2026-11-10 18:28:23.0868801** |

La seconde capture est la plus récente et contient les écritures recherchées.
Elle comporte **8 144 trames**, sur **67,623761 s**, interface pcapng 0
`\\.\USBPcap2`, encapsulation USBPcap, horodatages à la microseconde.
SHA-256 :
`9ae639a4633b7140ee11fc40d227bdfb03477b5c0da6c5ff65d5e80e71a1893b`.
Les références de trames de ce rapport désignent exclusivement ce fichier original.

Outils : `capinfos` et `tshark` 4.6.9 en lecture de fichiers, calculs PowerShell
hors ligne. Extraction des charges utiles depuis `usb.data_fragment` pour les
SET et depuis les champs de données disponibles pour les Input, sans concaténer
de représentations redondantes. Le setup USB de 8 octets est exclu du rapport HID.
Les calculs CRC ont été recoupés par deux formulations, réfléchie `0xA001` et
normale `0x8005` avec réflexions explicites. Le contrôle `123456789 → B4C8`
a été effectué avant l'analyse.

L'analyseur local et le résumé technique restent dans
`.build/d5next-windows-rgb/`, ignoré par Git. Les deux captures brutes sont
ignorées, restent à leur emplacement et n'ont pas été modifiées. Aucune charge
utile de configuration complète, donnée hydraulique ou identité privée n'est
publiée. Le paquet de commande système ci-dessous est un échange protocolaire
observé, sans code propriétaire.

Aucun accès direct à la pompe, capture nouvelle, commande USB/HID, lancement
du contrôleur expérimental, modification Aquasuite ou C++, commit, push ou PR.

## 2. Identification indépendante et inventaire USB

La trame **38** identifie à nouveau **VID 0C70 / PID F00E**, **bus 2 / adresse 5**.
Cette correspondance a été relue dans la nouvelle capture, sans réutiliser
aveuglément l'adresse du lot 1. `bcdDevice=0100` n'est pas la version firmware.
Les 67 Input `01` de 162 octets contiennent tous `04 03` aux offsets 13–14,
soit **firmware 1027** en BE. Les 13 rapports `03` donnent **structure_id=3**.

La trame **40** déclare configuration 1, alternate settings 0 :

| Interface | Classe | Endpoints | Activité enregistrée |
|---|---|---|---|
| 0 | Vendor Specific `FF` | Bulk IN `81`, Bulk OUT `02`, 64 octets chacun | Aucune |
| 1 | HID `03` | Interrupt IN `83`, 64 octets, `bInterval=5` | Input `01` ; requêtes HID sur EP0 ciblant IF1 |

Il n'y a pas d'endpoint Interrupt OUT déclaré sur IF1. Le descripteur HID
annonce un descripteur de rapports de 70 octets, dont le contenu n'est pas
capturé. Les types Feature/Output ci-dessous proviennent des **transactions
SET_REPORT**, pas d'une supposition tirée du descripteur absent.

| Catégorie | Opérations / trames | Sens, taille et preuve |
|---|---|---|
| Control standard, préambule | 3 paires, 6 trames | 37–38 GET_DESCRIPTOR DEVICE : 18 octets ; 39–40 GET_DESCRIPTOR CONFIGURATION : 65 octets ; 41–42 SET_CONFIGURATION(1), sans données |
| HID SET_REPORT Feature `03` | **13 opérations, 26 trames** | Hôte → périphérique ; EP0 OUT ; `bmRequestType=21`, `bRequest=09`, `wValue=0303`, `wIndex=1`, `wLength=809` |
| HID SET_REPORT Output `02` | **6 opérations, 12 trames** | Hôte → périphérique ; EP0 OUT ; `21/09`, `wValue=0202`, `wIndex=1`, `wLength=11` |
| HID GET_REPORT | **0** | Aucun GET de configuration ou autre rapport |
| Interrupt IN `83` | **67 complétions + 67 soumissions** | 67 rapports `01`, 162 octets chacun, 10 854 octets utiles |
| Interrupt OUT | **0** | Aucun |
| Bulk IN/OUT | **0 / 0** | Aucun transfert IF0 |
| Rapports `08` / `0C` | **0 / 0** | Absents |

Total D5 NEXT : **178 trames** = 6 + 26 + 12 + 134. Les nombres de trames
ne sont pas des nombres de commandes : chaque SET est compté à sa soumission,
sa complétion est comptée séparément. Tous les SET sont appariés par l'IRP,
l'ordre chronologique et `usb.request_in` dans leur réponse ; leurs réponses
sont chacune la trame suivante. Les IRP sont réutilisées entre opérations.

`usb.data_len=817` pour les SET `03` et `19` pour les SET `02` comprend
**8 octets de setup + respectivement 809 ou 11 octets HID**. Les complétions
de ces SET ne contiennent aucune charge utile. Les latences des SET `03`
vont de 0,996 à 1,171 ms. Tous les statuts D5 sont `USBD_STATUS_SUCCESS` ;
aucun `_ws.malformed` ni `frame.cap_len < frame.len` n'est observé sur la D5.
Cela ne prouve ni l'application visuelle ni une écriture en mémoire non volatile.

Les Input vont de la trame **275**, t=0,793688 s, à **8135**, t=67,453626 s.
Les 66 intervalles ont une moyenne de **1,009999061 s**, min 1,007865 s,
max 1,012118 s, soit **0,990100 rapport/s**. Chaque complétion Input est suivie
d'une nouvelle soumission IN vide : cette soumission n'est pas un Output.
Les lectures en cours aux bords du fichier peuvent être incomplètes.

Le préambule 37–42, à t=0 avec IRP nulle, présente les mêmes caractéristiques
que celui du [lot 1](D5NextWindowsBaseline.md). Son interprétation comme
inventaire reconstruit reste une déduction ; il ne constitue pas une preuve
de réénumération ou d'action Aquasuite réelle à cet instant.
Le SET_CONFIGURATION **standard** 41 n'est pas un SET_REPORT HID.
L'ID HID **02** ne doit pas être confondu avec l'endpoint **Bulk OUT 02** d'IF0.

## 3. Chronologie des configurations et vérification CRC

Temps relatifs au début du fichier, 18:25:46.503792 CET. Tous les offsets sont
décimaux, ID HID inclus en position 0. Dans les tableaux, les valeurs HSV,
flags, modes et CRC sont hexadécimales ; la luminosité générale est décimale.
`H/S/V` correspond à BE16/8/8 aux offsets **706–709**.

**Les 13 configurations ont ID=03, longueur=809, structure BE1–2=0003.**
CRC-16/USB : polynôme `8005`, init `FFFF`, xorout `FFFF`, refin/refout vrais ;
calcul sur **[1,807)**, soit offsets 1–806 inclus, puis stockage **BE807–808**.
La colonne CRC donne **reçu = calculé**, sans exception.

| SET / complétion | t (s) | Mode 663 | Master 237 | Flags BE238–239 | Premier HSV | CRC reçu = calculé |
|---|---:|---|---:|---|---|---|
| 647 / 648 | 3,910297 | 01 | 255 | 0000 | 04BA/C3/FD | AC78 |
| 889 / 890 | 5,669985 | 01 | 255 | 0000 | 053C/D2/FD | F51B |
| 937 / 938 | 7,253279 | 01 | 255 | 0000 | 01D2/A8/FD | 69A6 |
| 1109 / 1110 | 8,593758 | 01 | 255 | 0000 | 00D0/CA/FD | FACB |
| 1869 / 1870 | 14,292717 | 01 | 99 | 0000 | 00D0/CA/FD | 74A2 |
| 2115 / 2116 | 17,509993 | 01 | 156 | 0000 | 00D0/CA/FD | 9C4C |
| 2385 / 2386 | 20,906108 | 01 | 102 | 0000 | 00D0/CA/FD | FDB7 |
| 2937 / 2938 | 26,035546 | 01 | 102 | 0002 | 00D0/CA/FD | BC31 |
| 3201 / 3202 | 31,086598 | 01 | 102 | 0000 | 00D0/CA/FD | FDB7 |
| 3743 / 3744 | 33,560169 | 01 | 102 | 0000 | 0514/CC/FD | BAE6 |
| 4383 / 4384 | 40,306135 | **13** | 102 | 0000 | **0000/00/FF** | A8A4 |
| 5661 / 5662 | 51,300617 | **01** | 102 | 0000 | **0000/FF/FF** | 0F1A |
| 7673 / 7674 | 64,260495 | 01 | 102 | 0000 | 03FF/FF/FF | D03F |

La première configuration visible est **déjà Fixed Color** selon la valeur
`FixedColor=1` identifiée dans l'analyse statique. Sans GET ou SET antérieur,
elle ne permet pas d'établir quels octets ont changé depuis l'état Rainbow
initial. L'ouverture de la page RGBpx n'est pas un événement USB identifiable
de façon autonome dans ce fichier.

Les écritures 647, 889, 937 et 1109 sont quatre états distincts : il ne faut pas
les fusionner en un seul changement de couleur. Idem pour les trois changements
de luminosité 1869, 2115 et 2385. Le rapport 3201 est identique **octet pour octet**
à 2385 : retour à la configuration précédant le changement de flags.

## 4. Différences exhaustives entre configurations successives

Chaque ligne compare les **809 octets**, pas seulement les champs connus.
Les changements de CRC sont distingués des données fonctionnelles.
Aucun autre octet ne change entre les paires ci-dessous.

| Transition | Différences hors CRC, `offset: avant→après` | CRC807–808 avant→après |
|---|---|---|
| Avant 647 → 647 | Non calculable : état précédent absent | Inconnu→AC78 |
| 647 → 889 | 706:04→05 ; 707:BA→3C ; 708:C3→D2 | AC78→F51B |
| 889 → 937 | 706:05→01 ; 707:3C→D2 ; 708:D2→A8 | F51B→69A6 |
| 937 → 1109 | 706:01→00 ; 707:D2→D0 ; 708:A8→CA | 69A6→FACB |
| 1109 → 1869 | 237:FF→63 | FACB→74A2 |
| 1869 → 2115 | 237:63→9C | 74A2→9C4C |
| 2115 → 2385 | 237:9C→66 | 9C4C→FDB7 |
| 2385 → 2937 | 239:00→02 | FDB7→BC31 |
| 2937 → 3201 | 239:02→00 | BC31→FDB7 |
| 3201 → 3743 | 706:00→05 ; 707:D0→14 ; 708:CA→CC | FDB7→BAE6 |
| 3743 → 4383 | **663:01→13 ; 674:00→01 ; 675:FF→64 ; 683:00→23 ; 685:00→0A ; 687:00→05 ; 689:00→5A ; 691:00→19 ; 706:05→00 ; 707:14→00 ; 708:CC→00 ; 709:FD→FF ; 710:00→01 ; 712:00→FF ; 713:00→FF** | BAE6→A8A4 |
| 4383 → 5661 | **663:13→01 ; 674:01→00 ; 675:64→FF ; 683:23→00 ; 685:0A→00 ; 687:05→00 ; 689:5A→00 ; 691:19→00 ; 708:00→FF ; 710:01→00 ; 712:FF→00 ; 713:FF→00** | A8A4→0F1A |
| 5661 → 7673 | 706:00→03 ; 707:00→FF | 0F1A→D03F |

Le bloc 660–729 commence toujours par strip=1, début=0, nombre=6 ; les flags
BE664–665 restent `0000`. Le reste du bloc varie aux offsets explicités.
Les octets hors `237`, `239`, bloc `660–729` et CRC restent identiques sur
toute la série. Cela documente la conservation entre ces écritures officielles,
sans prouver l'identité avec un état antérieur à la première écriture.

Le changement de mode 3743→4383 touche **15 octets hors CRC**, et le retour
4383→5661 en touche **12**. Il inclut paramètres, V et deuxième entrée HSV.
Ce sont des observations concrètes de changements associés au mode ; la capture
n'établit pas que chacun est requis par le firmware ni le nom du mode `13`.
Le retour `13→01` ne restaure pas la première couleur `0514/CC/FD` : il installe
`0000/FF/FF`. Il ne faut donc pas assimiler ce retour à une restauration d'état.

## 5. HSV complet, luminosité et flags

Les six entrées de quatre octets commencent à 706, 710, 714, 718, 722 et 726.
Les métadonnées précédemment étudiées les définissent comme HSV ; le nombre
d'entrées ne prouve pas une association individuelle à chaque LED physique.

| Configurations | HSV1, 706–709 | HSV2, 710–713 | HSV3–6, 714–729 |
|---|---|---|---|
| 647 | 04BA/C3/FD | 0000/00/00 | Tous 0000/00/00 |
| 889 | 053C/D2/FD | 0000/00/00 | Tous 0000/00/00 |
| 937 | 01D2/A8/FD | 0000/00/00 | Tous 0000/00/00 |
| 1109, 1869, 2115, 2385, 2937, 3201 | 00D0/CA/FD | 0000/00/00 | Tous 0000/00/00 |
| 3743 | 0514/CC/FD | 0000/00/00 | Tous 0000/00/00 |
| 4383, mode 13 | 0000/00/FF | **0100/FF/FF** | Tous 0000/00/00 |
| 5661 | 0000/FF/FF | 0000/00/00 | Tous 0000/00/00 |
| 7673 | 03FF/FF/FF | 0000/00/00 | Tous 0000/00/00 |

H est un BE16 ; valeurs décimales successives de HSV1 : 1210, 1340, 466, 208,
1300, 0, 0, 1023, selon les groupes ci-dessus. S et V sont des octets distincts.
V1 vaut **253** jusqu'à 3743 inclus, puis **255** dès 4383. Pendant les changements
de master 255→99→156→102, le V1 reste 253 : **master237 et V709 ne sont pas
interchangeables**. Le master final 102 correspond à 40 % de 255 ; 99 et 156
correspondent à environ 38,8 % et 61,2 %. Il s'agit de rapports d'échelle,
sans preuve des pourcentages affichés par l'interface.

Les flags **BE238–239** passent de `0000` à `0002` en 2937 puis reviennent à
`0000` en 3201. Cela concorde avec le bit Off décrit antérieurement par aquapx,
mais aucun journal visuel ne confirme l'extinction. L'octet 238 reste nul ;
la signification des autres bits ne peut pas être déduite de cette acquisition.

## 6. Rapport 02 et sauvegarde

Les six paquets sont identiques, 11 octets ID compris :

```text
02 00 00 00 02 00 00 00 00 34 C6
```

Découpage cohérent avec les métadonnées de commandes système précédentes :
ID=02 ; commande BE32 aux offsets 1–4 = **2, SettingsSave** ; argument BE32
aux offsets 5–8 = **0** ; CRC BE9–10 = **34C6**. Le CRC-16/USB calculé sur
les **8 octets 1–8**, ID exclu, vaut **34C6 dans les six cas**.
Aucune autre commande système n'est présente.

**Transport observé : Control SET_REPORT Output**, EP0 OUT, interface 1,
`bmRequestType=21`, `bRequest=09`, **`wValue=0202`**, `wIndex=1`, `wLength=11`.
Il ne s'agit ni d'un SET Feature `0302`, ni d'un Interrupt OUT, ni d'un Bulk OUT.
Ces transactions ne permettent pas de déterminer quelle API Windows précise
a été appelée en amont, par exemple `HidD_SetOutputReport` ou une autre voie.

| Output / complétion | t (s) | Dernier SET03 précédent | Délai depuis sa soumission (s) | Écritures depuis le précédent Output02 ou début |
|---|---:|---:|---:|---|
| 1669 / 1670 | 12,419983 | 1109 | 3,826225 | 647, 889, 937, 1109 |
| 2929 / 2930 | 24,733862 | 2385 | 3,827754 | 1869, 2115, 2385 |
| 3053 / 3054 | 29,737230 | 2937 | 3,701684 | 2937 |
| 4357 / 4358 | 37,389051 | 3743 | 3,828882 | 3201, 3743 |
| 4731 / 4732 | 44,142988 | 4383 | 3,836853 | 4383 |
| 6239 / 6240 | 55,146278 | 5661 | 3,845661 | 5661 |

Les six complétions réussissent. Il n'y a donc **pas une commande de sauvegarde
immédiate après chaque écriture** : certains groupes de plusieurs SET03 sont
suivis d'un seul Output02. Le comportement est **compatible avec une sauvegarde
différée après inactivité**, généralement autour de 3,8 s dans cette capture.
C'est une inférence chronologique, pas la reconstruction du timer officiel.
Le délai de 3,701684 s doit être conservé, sans imposer artificiellement 3,8 s.

Le dernier SET03, 7673, n'a pas de sauvegarde ultérieure visible : la capture
s'arrête seulement **3,363266 s** après sa soumission. Ce délai est inférieur
aux six délais observés ; l'absence peut donc résulter de la fin d'acquisition.
Elle ne prouve ni omission de sauvegarde ni traitement spécial de cette couleur.

L'identification SettingsSave est étayée par l'ID, le payload, le CRC et le nom
de commande dans l'analyse statique. **Aucune persistance EEPROM/flash n'est
démontrée** : aucun redémarrage, cycle d'alimentation ou readback postérieur
contrôlé n'a été réalisé. Le rôle exact du firmware lors d'un SET03 sans 02
reste également inconnu.

## 7. Comparaison Aquasuite / aquapx / OpenRGB C++

Références locales : [analyse statique, sections 5, 6 et 8](D5NextAquasuiteAnalysis.md),
[préparation rose](AquaComputerD5NextPinkPreparation.md),
[rapport du prototype](AquaComputerD5Next.md), et sources actuelles
`D5NextProtocol.{h,cpp}`, `AquaComputerD5NextController{,Detect}.cpp`,
`tools/AquaComputerD5Next/D5NextPinkTest.cpp`.
Le répertoire `aquapx-main/` cité historiquement n'est pas présent dans cet
espace Windows : la colonne aquapx reprend les observations sourcées des
rapports antérieurs, **sans nouvelle vérification de son code ni exécution**.

| Point | Aquasuite, capture actuelle | aquapx selon les rapports antérieurs | OpenRGB C++ actuel / conséquence |
|---|---|---|---|
| Transport configuration | SET Feature03, 809 octets, IF1/EP0 | Feature03, 809 | `hid_get_feature_report` puis `hid_send_feature_report` ; SET concordant, GET non observé chez Aquasuite ici |
| CRC et structure | 13/13 CRC USB BE valides, structure3 | Même CRC/framing ; pas de matrice firmware démontrée | Même couverture CRC ; `ValidateReport` ne contrôle pas structure_id, limite déjà identifiée |
| Static / Rainbow | Mode01 visible ; aucun mode03 | 01/03 documentés | 01/03 définis ; Static corroboré par association avec les métadonnées ; aucun retour Rainbow validé ici |
| H/S | Modifiés uniquement en 706–708 lors des changements de couleur en mode01 | Même offsets | Même offsets ; valeur rose exacte non observée |
| V de HSV | 709 varie FD→FF au changement de mode | V709 préservé, modèle de luminosité simplifié | V709 préservé et absent de `D5NextLighting` ; master237 reçoit V RGB combiné à la luminosité demandée |
| Master | 237 varie indépendamment du V709 | 237 modifié | Offset concordant ; équivalence de rendu non établie |
| Flags | BE238–239, `0000↔0002` | 239 traité 0/2 | 239 remplacé par 0/2 ; préservation correcte de tous les autres bits non garantie par ce modèle |
| Changement de mode | Paramètres 674–691, V709 et HSV2 aussi modifiés sur `01↔13` | Modèle de patch ciblé décrit précédemment | Masque actuel limité à 237/239/663/706/707/708 ; il ne reproduit pas ces transitions officielles |
| Sauvegarde | **Output02 sur EP0**, commande2/argument0/CRC34C6, différée par groupes | **SendFeatureReport02**, même paquet décrit | Aucun02 ; divergence aquapx/transport officiel désormais constatée |
| Persistance sans02 | Inconnue | NoSave présenté comme non persistant | L'absence de02 ne prouve pas une absence de persistance |
| IF0 | Aucun Bulk | Rôle évoqué, non établi | Aucun chemin séparé à justifier par cette capture |

Le C++ conserve un layout inconnu en construction normale, sa détection est
conditionnelle et aucun firmware réel n'est approuvé pour écrire. La capture
ne justifie pas la levée automatique de ces protections.

### Candidat rose #FF69B4

La conversion actuelle du codec pour `(255,105,180)` produit Static01,
**H=0580 (1408), S=96 (150), V RGB=FF (255)**. À luminosité demandée 100 %,
le contrôleur place 255 en **237**, mais **préserve le V du rapport en 709**.
Il ne force donc pas nécessairement le HSV complet `0580/96/FF`.

Aucun des 13 rapports ne contient même le couple H/S `0580/96` ; il est
impossible de certifier l'arrondi d'Aquasuite pour ce rose à partir de cette
capture. Les couleurs et luminosités multiples sont analysables, sans les
réétiqueter comme le rose exact annoncé dans le scénario initial.

Exemple **calculé hors ligne**, non envoyé : en partant de 647, master=FF,
flags=0000 et mode=01 sont déjà les valeurs demandées ; remplacer seulement
706–708 par `05 80 96` comme le codec laisserait **V709=FD**. Le CRC recalculé
serait **7BB5**, et non F860. Ce calcul illustre la dépendance au rapport de
départ ; ce n'est ni une configuration recommandée ni un résultat Aquasuite.

Le CRC historique **F860** ne concerne que le candidat bâti sur l'ancienne
lecture **EFD1**, décrit dans le rapport de préparation. Aucun rapport de
configuration actuel ne porte EFD1 ou F860. Les octets opaques de l'ancienne
lecture ne sont pas disponibles ici pour une comparaison intégrale historique.
Ne pas rejouer ce CRC, ni reconstruire une configuration à partir des tableaux.

## 8. Incertitudes et recommandations avant un premier SET du pilote

Incertitudes restantes :

- Aucun GET03, état Rainbow initial complet, SET mode03, ou rose exact dans ce
  fichier ; le changement d'ouverture de page n'est pas horodatable séparément.
- Nom et sémantique exacte du mode13 non établis ; les effets des paramètres
  modifiés et les obligations de réinitialisation par mode restent inconnus.
- Aucun enregistrement visuel synchronisé des six LED ; palette et rendu
  physique ne peuvent pas être assimilés sans preuve supplémentaire.
- Statut USB réussi et CRC correct ne prouvent ni l'application sémantique,
  ni la relecture fidèle, ni la persistance. Pas de GET de contrôle après SET.
- Délai de sauvegarde seulement inféré ; dernier SET proche de la fin du fichier.
- Initiateur Windows exact non fourni par USBPcap : le contexte utilisateur
  attribue les changements à Aquasuite X.94, sans journal de processus par URB.
- Le fichier ne fournit pas de statistiques garantissant l'absence de pertes.

Recommandations pour un lot ultérieur, **sans exécution ni autorisation ici** :

1. Faire revoir le transport désormais démontré : SET Feature03 et **SET
   Output02** sont deux types différents. Ne pas reprendre le choix Feature02
   d'aquapx ; ne pas ajouter une sauvegarde automatique au prototype sur cette
   seule base. Une politique de sauvegarde doit être définie séparément.
2. Établir par une preuve complémentaire la transition **Rainbow03 → Fixed01**
   avec configurations complètes avant/après et paramètres annexes. Les
   transitions `01↔13` observées montrent que le changement du seul sélecteur
   n'est pas une reproduction générale du comportement officiel.
3. Valider explicitement **structure3**, firmware1027, taille, ID et CRC avant
   toute future écriture. Ne pas confondre concordance de layout et approbation
   matérielle. Conserver lecture fraîche, refus des préconditions inconnues,
   audit des 809 octets et absence de rejeu d'un ancien buffer.
4. Clarifier la gestion séparée de **master237 et V709**, des flags16 et de la
   palette. La préservation de V=FD rend insuffisant le seul contrôle master=FF
   pour revendiquer le HSV rose FF attendu. Ne pas élargir le masque d'écriture
   à tous les octets modifiés ici sans comprendre leurs rôles.
5. Obtenir une observation du rose exact et de son rendu sur six LED avant de
   conclure à l'équivalence Aquasuite/OpenRGB. Chaque future configuration
   devra avoir un CRC recalculé sur ses propres octets ; F860 n'est pas un modèle.
6. Définir séparément l'exclusivité des accès, les critères d'arrêt et la
   récupération par l'utilisateur avant un éventuel essai autorisé. Aucune
   boucle, retry, sauvegarde, reset ou restauration automatique n'est validé
   par la présente analyse.

## 9. Reproduction locale des observations

Filtre d'identification dans cette capture :

```text
usb.idVendor == 0x0c70 && usb.idProduct == 0xf00e
```

Il trouve le descripteur 38. Pour les échanges de la session identifiée :

```text
usb.bus_id == 2 && usb.device_address == 5
```

Exemple d'inventaire des SET, sans publier les buffers de configuration :

```powershell
& 'C:\Program Files\Wireshark\tshark.exe' `
  -r 'Controllers\AquaComputerD5NextController\d5next_baseline02.pcapng' `
  -Y 'usb.bus_id == 2 && usb.device_address == 5 && usbhid.setup.bRequest == 9' `
  -T fields -e frame.number -e frame.time_relative -e usb.irp_id `
  -e usb.bmRequestType -e usbhid.setup.wValue -e usbhid.setup.ReportType `
  -e usbhid.setup.ReportID -e usbhid.setup.wIndex -e usbhid.setup.wLength
```

Pour les complétions, inclure aussi les trames sans champ setup, avec le filtre
bus/adresse et `usb.transfer_type == 2`, puis suivre `usb.request_in`, IRP,
`usb.irp_info` et `usb.usbd_status`. Le champ USBPcap `transfer_type=2` signifie
Control ; ne pas le confondre avec le code Bulk de `bmAttributes` d'un endpoint.

**Analyse arrêtée après rédaction et vérification. Aucune écriture matérielle
n'est autorisée par ce rapport.**

## 10. Complément : Rotating Rainbow → fixe → rose → Rotating Rainbow

### Nouvelle capture et inventaire indépendant

Fichier : `Controllers/AquaComputerD5NextController/d5next_baseline03.pcapng`.
Taille : **259 188 octets** ; **3 847 trames** ; durée **47,688168 s**.
Fenêtre enregistrée : **2026-11-10 18:44:35.580534–18:45:23.268702 CET**.
Dernière modification du fichier : 18:45:36.0133562 CET.
SHA-256 :
`c179cce19b2f6599278d3a216e62e180a427e7da88470900fa58ad9098b8ba0f`.
Tous les numéros de trame de cette section désignent **cette nouvelle capture**.
Les temps sont relatifs à son début, pas à celui de la capture précédente.

La trame **38** réidentifie `0C70:F00E`, **bus 2, adresse 5**. La trame **40**
confirme IF0 vendor/Bulk81–02 et IF1 HID/Interrupt83. Les échanges HID Control
ciblent explicitement **IF1**. Aucun changement d'adresse n'est observé.

| Catégorie D5 NEXT | Opérations et trames | Contenu |
|---|---|---|
| Préambule standard EP0 | 3 paires, 6 trames 37–42 | Descripteurs DEVICE/CONFIGURATION et SET_CONFIGURATION ; mêmes réserves de provenance qu'en section 2 |
| SET Feature03, EP0 OUT | **4 opérations / 8 trames** | `21/09`, `wValue=0303`, `wIndex=1`, `wLength=809` |
| SET Output02, EP0 OUT | **3 opérations / 6 trames** | `21/09`, `wValue=0202`, `wIndex=1`, `wLength=11` |
| Interrupt IN83 | **47 rapports / 94 trames** | 47 complétions de 162 octets et 47 nouvelles soumissions vides ; ID01, firmware BE13–14=1027 dans tous les rapports |
| GET_REPORT / Interrupt OUT / Bulk IN / Bulk OUT / rapports08–0C | **0** | Aucun |

Total : **114 trames D5 NEXT**. Les Input vont de **236**, t=0,904030 s,
à **3778**, t=47,364081 s ; cadence **0,990098 Hz**, intervalle moyen
1,010001109 s, min 1,007893 s, max 1,012155 s. Tous les statuts D5 sont réussis,
sans `_ws.malformed` ou troncature de trame détectée. Des SET HID vers l'adresse
**13** existent ailleurs dans le fichier : ils ne font pas partie de la D5 NEXT
et ont été exclus de l'analyse.

### Chronologie, configurations complètes et CRC

Les quatre rapports ont **ID03, 809 octets, structure_id BE1–2=0003**.
Les deux formulations du CRC de la section 1 retrouvent, pour chacun, le CRC
reçu BE807–808 sur les offsets 1–806. **Quatre CRC valides sur quatre.**
Le master237 reste **102 (`66`, soit 40 % de 255)** et les flags BE238–239
restent **0000** pendant toute la séquence. Le bloc660–665 reste
`01 00 06 [mode] 00 00` : strip1, début0, six LED, flags du groupe nuls.

| SET / complétion | t (s) | État observé | Mode663 | HSV1, BE706–709 | CRC reçu = calculé |
|---|---:|---|---|---|---|
| **1110 / 1111** | **8,592078** | Passage en Rotating Rainbow, selon le scénario utilisateur | **03** | 0000/FF/FF | **1430** |
| **2376 / 2377** | **23,336999** | Passage en fixe ; HSV1 rouge nominal | **01** | 0000/FF/FF | **0F1A** |
| **2666 / 2667** | **25,148750** | Changement de couleur, rose selon l'utilisateur | **01** | **04FD/C4/FF** | **C3DD** |
| **3284 / 3285** | **37,452211** | Retour Rotating Rainbow ; HSV1 réinitialisé | **03** | **0000/FF/FF** | **1430** |

Le nom « Rotating Rainbow » provient de la manipulation décrite par l'utilisateur ;
la valeur **03** concorde avec Rainbow dans les métadonnées et le C++ précédemment
examinés. Les transactions ne contiennent pas le libellé de l'interface graphique.
L'état avant 1110 n'est pas lu : les octets changés lors de ce premier passage
ne peuvent pas être reconstruits à partir de la seule nouvelle capture.

Dans les quatre configurations, **HSV2–6, offsets710–729, valent tous
0000/00/00**. Le V1 en709 reste **FF** ; contrairement à l'acquisition précédente,
aucun V=FD n'est observé. Les complétions des quatre SET arrivent respectivement
après **0,930 ; 0,940 ; 1,018 ; 1,116 ms**. Une complétion USB réussie ne remplace
pas une relecture de configuration ou une observation visuelle synchronisée.

### Différences exhaustives, octet par octet

Comparaison intégrale des 809 octets. Les CRC figurent à part ; **aucun autre
octet ne change** entre les configurations successives.

| Transition | Différences hors CRC, `offset: avant→après` | CRC807–808 |
|---|---|---|
| Avant1110 → 1110 | Non calculable, état précédent absent | Inconnu→1430 |
| **1110 → 2376**, Rainbow→fixe | **663:03→01 ; 674:01→00 ; 675:64→FF ; 683:32→00 ; 685:64→00** | 1430→0F1A |
| **2376 → 2666**, couleur | **706:00→04 ; 707:00→FD ; 708:FF→C4** | 0F1A→C3DD |
| **2666 → 3284**, fixe→Rainbow | **663:01→03 ; 674:00→01 ; 675:FF→64 ; 683:00→32 ; 685:00→64 ; 706:04→00 ; 707:FD→00 ; 708:C4→FF** | C3DD→1430 |

La configuration finale **3284 est identique sur les 809 octets à 1110**,
pas seulement par son CRC. Le retour Rainbow rétablit donc l'état Rainbow
enregistré au début de cette séquence, y compris le HSV1 rouge nominal.
Les valeurs annexes sont `674–675=0164`, `683=32`, `685=64` en Rainbow et
`674–675=00FF`, `683=00`, `685=00` en fixe. Leur rôle fonctionnel précis reste
indéterminé : cette capture prouve leur modification par Aquasuite, pas leur
caractère obligatoire pour le firmware.

### Sauvegardes corrélées

Les trois Output02 contiennent le même paquet qu'en section 6 :
`02 00 00 00 02 00 00 00 00 34 C6`. Commande BE32=2, argument=0,
CRC-16/USB BE=**34C6**, recalculé et valide dans les trois cas.
Le transport est explicitement **Control SET_REPORT Output02, EP0, IF1**,
`wValue=0202`, et non Feature02 ou Bulk OUT02.

| Output / complétion | t (s) | Dernier SET03 | Délai depuis ce SET (s) | Association chronologique |
|---|---:|---:|---:|---|
| **1428 / 1429** | 12,422416 | 1110 | **3,830338** | Après le premier Rainbow |
| **2774 / 2775** | 28,973123 | 2666 | **3,824373** | Après fixe2376 puis couleur2666 ; un seul Output02 visible pour les deux écritures |
| **3456 / 3457** | 41,285403 | 3284 | **3,833192** | Après le retour Rainbow |

Toutes les complétions sont réussies. La séquence conforte l'hypothèse d'une
sauvegarde différée après la dernière modification : l'écriture de couleur
intervient **1,811751 s** après le passage en fixe, avant la commande02 commune.
Il ne faut pas transformer cette corrélation en contrat de timer ou en preuve
de persistance EEPROM. Aucun GET_REPORT ou essai de persistance n'est présent.

### Conséquences pour le candidat rose et le pilote

La couleur choisie par l'utilisateur dans ce nouveau dump correspond à
**H=04FD (1277), S=C4 (196), V=FF (255)** dans le premier HSV. Elle diffère
du candidat historique **#FF69B4**, calculé par notre codec comme
**0580/96/FF**. La capture ne prouve donc toujours pas la conversion exacte
d'Aquasuite pour #FF69B4. La luminosité générale reste 40 %, même si V1 est à
255 ; elle ne correspond pas au master100 % du candidat historique.

La nouvelle preuve concerne directement la transition qui manquait au lot
précédent : **Rainbow03→Fixed01 modifie cinq octets hors CRC**, et non le seul
sélecteur663. Le C++ actuel préserve 674, 675, 683 et685 : il ne reproduirait
donc pas la transition officielle observée. Le retour Rainbow remet aussi
H/S à0000/FF, alors que la branche Rainbow du C++ préserve H/S.
Cela ne prouve pas que le C++ échouerait visuellement, mais son équivalence
avec les écritures Aquasuite **n'est pas établie**.

Le changement de couleur **à mode fixe déjà établi**, 2376→2666, ne touche en
revanche que H/S706–708 et le CRC. Il confirme ces offsets et la conservation
de V709, du master, des flags et de toute la palette restante pour cette action.
Il ne doit pas être fusionné avec le SET de changement de mode précédent.

Les réserves de comparaison aquapx en section 7 restent applicables. Cette
nouvelle acquisition confirme le framing/CRC de03 et **Output02**, sans
valider le transport Feature02 décrit historiquement par aquapx. Aucun code
tiers supplémentaire n'a été lu, importé ou exécuté dans ce complément.

Avant tout futur SET du pilote, il reste à faire revoir les octets annexes de
transition03↔01, leur nécessité et la politique de préservation/réinitialisation,
ainsi que les préconditions master/V/flags et la conversion du rose exact.
Ne pas étendre automatiquement le masque du codec à partir d'une seule séquence.
Le CRC historique F860 ne s'applique à aucun rapport observé ici ; chaque
configuration future demanderait son propre calcul sur un état fraîchement lu.

Le complément a été analysé avec le même analyseur hors ligne, sortie locale
ignorée `.build/d5next-windows-rgb/summary03.json`. La capture est inchangée,
ignorée par Git, et son empreinte a été vérifiée. **Aucun C++ modifié, aucune
commande matérielle, aucune approbation de SET, aucun commit/push/PR.
Arrêt après cette analyse.**

## 11. Complément : luminosité 0 → 60 → 100 → 0 %

### Acquisition et inventaire

Scénario annoncé par l'utilisateur : premier réglage à 0 %, puis 60 %, 100 %,
puis retour à 0 %. La nouvelle acquisition est
`Controllers/AquaComputerD5NextController/d5next_baseline04.pcapng` :
**138 400 octets**, **2 052 trames**, durée **20,350470 s**.
Fenêtre : **2026-11-10 18:49:18.878126–18:49:39.228596 CET**.
Dernière modification du fichier : 18:49:51.6879499 CET.
SHA-256 :
`6fe8b9c55bfa307e3a29f3e71066037abedcfff017dea7d50745753a85110a00`.
Tous les numéros de trame de cette section se rapportent **uniquement à04** ;
les temps sont relatifs au début de ce fichier.

Le descripteur DEVICE **38** réidentifie `0C70:F00E`, **bus2/adresse5**.
Le descripteur CONFIGURATION **40** confirme IF0 vendor/Bulk81–02 et
IF1 HID/Interrupt83. Les requêtes HID visent explicitement **IF1 sur EP0**.

| Catégorie D5 NEXT | Opérations / trames | Preuve |
|---|---|---|
| Préambule standard | 3 paires / 6 trames, 37–42 | GET_DESCRIPTOR DEVICE/CONFIGURATION et SET_CONFIGURATION, mêmes réserves sur le préambule qu'en section2 |
| SET Feature03 | **3 / 6** | Hôte→pompe, `bmRequestType=21`, `bRequest=09`, `wValue=0303`, `wIndex=1`, `wLength=809` |
| SET Output02 | **2 / 4** | Hôte→pompe, `21/09`, `wValue=0202`, `wIndex=1`, `wLength=11` |
| Interrupt IN83 | **20 rapports / 40 trames** | 20 complétions de162 octets et20 nouvelles soumissions vides ; ID01 et firmware BE13–14=1027 dans tous les rapports |
| GET_REPORT / Interrupt OUT / Bulk IN / Bulk OUT / rapports08–0C | **0** | Absents |

Total D5 : **56 trames**. Les SET d'autres adresses, notamment13, sont exclus.
Les Input vont de **157**, t=0,410207 s, à **1965**, t=19,598225 s,
à **0,990201 Hz** ; intervalle moyen1,009895684 s, min1,007913 s,
max1,012094 s. Tous les statuts D5 sont réussis ; aucune troncature ou
indication `_ws.malformed` n'est détectée.

### Luminosité, CRC et comparaison intégrale

Les trois rapports03 sont complets : **809 octets, ID03, structure BE1–2=3**.
Les trois CRC-16/USB reçus BE807–808 correspondent au calcul sur les
offsets1–806 avec les deux formulations de contrôle utilisées précédemment.

| SET / complétion | t (s) | Master237, hex / décimal | Association au scénario | CRC reçu = calculé |
|---|---:|---|---|---|
| **951 / 952** | **7,378641** | **99 / 153** | **60 %**, `153/255=0,60` | **FCDE** |
| **1287 / 1290** | **12,505519** | **FF / 255** | **100 %** | **134C** |
| **1607 / 1608** | **16,326422** | **00 / 0** | **0 % final** | **FBA2** |

Le **0 % initial n'a pas de SET03 visible** et aucun GET ne révèle l'état
précédant951. Il peut avoir été réglé avant la fenêtre, ou ne pas avoir donné
lieu à une écriture visible ; la capture ne tranche pas. Il ne faut ni inventer
une quatrième configuration ni prendre la dernière configuration comme preuve
de la première.

Comparaison exhaustive des809 octets :

| Transition | Différences hors CRC | CRC807–808 avant→après |
|---|---|---|
| État initial annoncé→951 | Non calculable, configuration initiale absente | Inconnu→FCDE |
| **951→1287** | **237:99→FF uniquement** | FCDE→134C |
| **1287→1607** | **237:FF→00 uniquement** | 134C→FBA2 |

Tous les autres octets sont identiques : **flags BE238–239=0000**, groupe660–665
`01 00 06 03 00 00` (modeRainbow03), premier HSV706–709 **0000/FF/FF**, et
cinq entrées suivantes710–729 toutes **0000/00/00**. Aucun paramètre annexe de
mode ni aucune couleur ne change. **V709 reste255 même à master0.**
Le mode reste03 : cette acquisition isole la luminosité en Rainbow, sans
preuve supplémentaire de son traitement en mode fixe.

Les complétions arrivent après1,055 ms, 1,045 ms et1,078 ms. Celle du SET1287
est **1290**, pas1288 : l'appariement a été vérifié par IRP et `usb.request_in`.
Les trames intercalées ne constituent pas une opération HID supplémentaire.

### Sauvegardes et portée pour le pilote

Les deux Output02 portent le paquet système déjà observé,
`02 00 00 00 02 00 00 00 00 34 C6` : **SettingsSave=2**, argument0,
CRC sur1–8 **34C6 valide**, EP0/IF1, type **Output**, `wValue=0202`.

| Output / complétion | t (s) | Dernier SET03 | Délai (s) |
|---|---:|---:|---:|
| **1095 / 1096** | 11,206632 | 951, 60 % | **3,827991** |
| **2047 / 2048** | 20,158128 | 1607, 0 % | **3,831706** |

Aucun Output02 ne sépare le100 % du0 % final. Ces deux SET sont espacés de
**3,820903 s**, légèrement moins que les délais de sauvegarde constatés ici.
L'ensemble reste compatible avec un report de sauvegarde après une nouvelle
modification, sans démontrer le fonctionnement exact du timer. Aucune
persistance EEPROM ou absence de persistance avant02 n'est établie.

Cette capture confirme que le réglage de luminosité décrit par l'utilisateur
agit sur **master237 à l'échelle0–255**, indépendamment du V709 et des flags.
Dans cette opération, **0 % n'utilise pas le flag0002** vu en section4 :
le réglage master0 et l'opération Off du prototype ne produisent pas le même
rapport. Leur équivalence visuelle ou leur interaction avec le firmware n'est
pas démontrée par les seules trames.

Le chemin Rainbow du contrôleur C++ modifie le master237 sans changer le HSV,
ce qui concorde avec ces différences de luminosité pour des flags initiaux0000.
Il demeure distinct du passage de mode03↔01 examiné en section10 et de
`SetOff()`, qui choisit le flag02. Ne pas déduire une validation générale du
pilote ou une autorisation de SET de cette concordance ciblée.

Le résumé local est `.build/d5next-windows-rgb/summary04.json`, ignoré par Git.
Analyse TShark et calculs exclusivement hors ligne ; capture intacte et ignorée.
**Aucun C++ modifié, accès matériel, commande envoyée, commit, push ou PR.
Arrêt après mise à jour du rapport ; aucune écriture automatiquement approuvée.**
