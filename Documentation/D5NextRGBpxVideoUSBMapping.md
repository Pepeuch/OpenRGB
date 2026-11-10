# D5 NEXT — lot 5 : corrélation vidéo Aquasuite / USB

Date : 10 novembre 2026, Europe/Paris. Branche `feat/d5next`.

**Résultat : 13 effets vus, 78 configurations valides, 15 sauvegardes distinctes.**
Le rapprochement de la vidéo et des champs USB attribue **32 couples
effet/paramètre numérique par variation isolée**, **26 couples effet/option**,
et le H/S de la couleur de fond de Blinking. Un 33e couple numérique,
Rainbow.Speed, reste seulement compatible car sa configuration précédente
manque. Les unités physiques et les bornes complètes ne sont pas démontrées.

Le décalage n'est pas fixé à une valeur exacte : les images montrent des valeurs
pendant le glissement, avant le SET correspondant. Une fenêtre opérationnelle
de **21,0 à 22,7 s** entre image d'action et temps USB couvre les observations
retenues. Elle ne sépare pas le décalage d'horloge de la latence de l'application.
Aucune dérive linéaire des horloges n'est quantifiable de manière défendable.

Livrables associés :

- [Événements et correspondances CSV](D5NextRGBpxVideoUSBEvents.csv) : **93 lignes**, un SET par ligne, y compris les15 sauvegardes.
- [Éditions de paramètres CSV](D5NextRGBpxVideoUSBParameters.csv) : **66 lignes**, conservant les répétitions et bascules inverses, soit60 couples effet/contrôle distincts.
- [Référence du lot4](D5NextRGBpxEngineAnalysis.md), complétée ici sans réécrire les preuves antérieures.

## 1. Identification des fichiers et méthode

Les deux enregistrements sont dans `Controllers/AquaComputerD5NextController/` :

| Fichier | Taille | Durée et caractéristiques |
|---|---:|---|
| `d5next test.mp4` | 102249176 octets | 211,300s ; H.264 Main,1958×1694,30images/s,6339images ; time_base1/30000, départ PTS0 |
| `d5next_baseline05.pcapng` | 1877352 octets | 234,382194s ;27746 trames ; USBPcap, interface de capture0 `\\.\USBPcap2` |

Empreintes SHA-256 :

| Fichier | SHA-256 |
|---|---|
| Vidéo | `354aac60ce5f42fabfbd3bf519f3b39d6f5dfbb622565ac0ed196cf48ced0240` |
| PCAPNG05 | `42fcfdcfdbcca49d6fb534eab7b7f11f78f58ae91c4e4ed1fe9ff18df9fa47da` |

FFprobe donne `creation_time=2026-11-10T18:26:03Z`, soit **19:26:03 CET**.
La capture USB va de **19:25:41.868325 à19:29:36.250519 CET**. La différence
des horodatages de départ serait21,131675s, mais ce timestamp MP4 arrondi
ne démontre pas l'instant de la première interaction ni celui de l'encodage.
Les dates de modification, respectivement19:29:34.7185342 et19:30:25.7490547,
n'ont servi qu'à l'inventaire, jamais à la synchronisation.

Les captures01–04 ont aussi été inventoriées : leurs fenêtres sont entre18:09
et18:49 CET, plus tôt que la vidéo. La capture05 contient exactement la même
succession distinctive d'effets, de réglages et d'options que la vidéo, avec
les mêmes valeurs numériques. C'est cette concordance de contenu, et non la
seule proximité des fichiers, qui établit leur association. Des vidéos plus
anciennes et sans nom D5 visibles dans les dossiers usuels n'ont pas été ouvertes.

Outils : FFprobe/FFmpeg9.0.2 portable sous `.build/d5next-video/`, TShark4.6.9,
analyseurs locaux PowerShell. Le téléchargement de l'outil portable a été
distinct de l'analyse des enregistrements ; aucun enregistrement n'a été envoyé
à un service distant. Les analyses vidéo/USB sont entièrement locales.

Une piste AAC stéréo48kHz existante est signalée par FFprobe. Elle n'a été ni
écoutée, ni extraite, ni transcrite ; les extractions d'images utilisent `-an`.
Les271 images locales sont des extractions ciblées et planches de contrôle,
pas un export des6339 images ni une publication de captures d'écran.
Les originaux sont intacts ; images, métadonnées et outils restent dans
`.build/`, hors Git. La vidéo locale a été ajoutée aux exclusions Git du dossier.

## 2. Synchronisation, latence et niveau de confiance

Les changements d'effet03→13→01→02→0C→05→04→0B→07→08→09→0A→0E,
les infobulles44/22/14/79/38 de Swiping rainbow et les bascules d'options
fournissent des signatures beaucoup plus distinctives qu'une sauvegarde02.
Le panneau filmé est celui du groupe de pompe, sixLED ; seuls les champs du
contrôleur6 changent dans les78 rapports. Le nom personnel d'un périphérique
ou son numéro de série n'est pas nécessaire à cette corrélation.

Quelques ancres vérifiées par images successives : les bornes vidéo sont
volontairement élargies d'environ une image quand l'extraction tombe entre PTS.

| Événement distinctif | Transition visible, temps vidéo(s) | SET USB / temps(s) | Écart apparent USB−image(s) |
|---|---|---|---|
| Rotating→Swiping rainbow | Après15,600, avant15,733 | 5027 /37,830145 | Environ22,10–22,23 |
| Point speed affiche44 | Infobulle lisible à18,400 | 5423 /40,463810 | 22,063810 à cette image, pas un instant de clic |
| Passage Sequence | Entre124,200 et124,300 | 17455 /145,711521 | Environ21,41–21,51 |
| Passage Flame | Entre200,800 et200,900 | 26427 /222,307683 | Environ21,41–21,51 |

Une image intermédiaire montre déjà Blinking à83,520s, avant son SET105,820243s,
soit un écart apparent d'au moins22,3s. Les variations ne suivent donc pas une
simple progression régulière entre les ancres. Elles mêlent prévisualisation,
sélection de mode, glissement et envoi différé. Ajuster une droite et annoncer
une dérive enppm serait trompeur avec ces seules observations.

**Méthode retenue :** une fenêtre vidéo prudente `[tUSB−22,7 ; tUSB−21,0]`
sert au rapprochement initial ; l'effet, le libellé, l'état de l'option ou la
valeur d'infobulle et l'ordre des actions désambiguïsent ensuite les opérations.
Les fenêtres de deux actions proches peuvent se chevaucher : le temps seul
ne suffit jamais. Le CSV conserve aussi le temps d'une image effectivement
examinée, qui peut montrer le réglage pendant le geste ou dans son voisinage.
Ce temps ne prétend pas mesurer le début du clic ou le relâchement.

Niveaux utilisés dans les CSV :

- `confirme_isole` : variation fonctionnelle isolée et correspondance visuelle ; haute confiance dans le champ, temps du geste conservé comme plage.
- `confirme_selection_champs_couples` : choix d'effet reconnaissable ; plusieurs champs changent ensemble. Le rôle individuel de chaque valeur initialisée n'est pas confirmé par ce seul SET.
- `compatible_premier_etat` : contrôle visible compatible, mais pas de configuration USB précédente pour établir la différence.
- `confirme_usb_uniquement` : sauvegarde réellement envoyée, sans clic de sauvegarde attribué dans la vidéo.

## 3. Inventaire USB et audit des huit contrôleurs

Le descripteur38 réidentifie **0C70:F00E, bus2/adresse5**. La configuration40
déclare IF0 Bulk IN81/OUT02 et IF1 HID Interrupt IN83. Les requêtes de classe
capturées ciblent **IF1 sur EP0** ; l'adresse n'a pas été supposée à partir
de la session précédente.

| Type D5 NEXT | Nombre d'opérations | Nombre de trames et contenu |
|---|---:|---|
| Préambule Control standard | 3 | 6 trames37–42 ; descripteurs et SET_CONFIGURATION, mêmes réserves que le lot1 |
| SET_REPORT Feature03 | **78** | **156 trames** ; `21/09`, `wValue=0303`, `wIndex=1`,809 octets HID |
| SET_REPORT Output02 | **15** | **30 trames** ; `21/09`, `wValue=0202`, `wIndex=1`,11 octets HID |
| Interrupt IN83 / Input01 | **233 rapports** | **466 trames**,233 complétions de162 octets et233 nouvelles soumissions vides |
| GET_REPORT, Interrupt OUT, Bulk IN/OUT, rapports08/0C | **0** | Aucun dans le trafic D5 |

Total : **658 trames D5**. Les233 Input indiquent le firmware1027 aux offsets
13–14 BE. Les78 configurations ont ID03,809 octets, structure3 BE1–2 et
**78 CRC reçus = calculés**. Vérification par deux formulations CRC-16/USB,
calcul sur1–806, stockage BE807–808. Les15 rapports02 ont aussi un CRC valide.
Les statuts USB D5 sont tous réussis ; aucun `_ws.malformed` ou paquet tronqué
au sens cap_len<len n'est détecté. La capture ne fournit pas une garantie
indépendante d'absence de pertes ou d'application visuelle par le firmware.

Les huit blocs de70 octets ont été décodés pour chaque écriture : strip/start/
count/mode, flags16BE, source signée et deux filtres, deux bindings, douze
int16BE et sixHSV. Entre configurations successives :

- **Seul le groupe6 change.** Les groupes0–5 et7 sont conservés intégralement.
- **Aucun octet hors RGB et CRC ne change** ; aucun réglage hydraulique n'est publié.
- Master237 reste **203**, flags strip238–239 restent0000 ; la vidéo affiche80%. Cette observation de quantification ne suffit pas à imposer une règle d'arrondi universelle80%→203.
- Le groupe6 reste strip1/start0/count6, source.id=-1, filtres10/15. Les changements de bindings sont associés aux changements de mode ; aucune borne de binding ou source n'est modifiée isolément par l'utilisateur.

Le CSV inclut toutes les différences fonctionnelles, le groupe modifié, le CRC
reçu/calculé et la structure. Les complétions sont appariées par IRP et
`usb.request_in`, pas par une hypothèse «trame suivante». Aucun transfert n'est
compté deux fois. Sans rapport précédent, les champs de3541 sont un état
initial observé, pas une liste de différences reconstruites.

## 4. Corrélations et offsets désormais attribués

Extraits représentatifs ; les93 opérations et leurs références exactes sont
dans [le CSV des événements](D5NextRGBpxVideoUSBEvents.csv). Les valeurs
avant/après sont USB ; `ui_after_read` distingue celles également lisibles
dans une infobulle de celles seulement décodées dans le rapport.

| Vidéo, image ou voisinage(s) | Action visible | Trame USB | Champ modifié | Avant→après | Confiance |
|---|---|---:|---|---|---|
| 3,033 | Speed, Rotating rainbow | 3541 | values[0],682–683 | Avant USB absent→51 | Compatible |
| 5,333 | Color range, Rotating rainbow | 3795 | values[1],684–685 | 100→99 | Isolée ;99 lu en USB, pas dans une infobulle |
| 7,567 | Reverse direction activé | 3999 | flags664–665, masque0002 | 0000→0002 | Isolée |
| 18,400 | Point speed44 | 5423 | values[0],682–683 | 35→44 | Isolée + infobulle |
| 20,133 | Point smoothness22 | 5691 | values[1],684–685 | 10→22 | Isolée + infobulle |
| 21,600 | Point size14 | 5907 | values[2],686–687 | 5→14 | Isolée + infobulle |
| 23,367 | Color change speed79 | 6241 | values[3],688–689 | 90→79 | Isolée + infobulle |
| 25,033 | Color range38 | 6423 | values[4],690–691 | 25→38 | Isolée + infobulle |
| 42,667 | Breathing.Intensity71 | 9039 | values[1],684–685 | 80→71 | Isolée + infobulle |
| 44,133 | Delay max. brightness22 | 9307 | values[2],686–687 | 5→22 | Isolée + infobulle ; unité inconnue |
| 54,033 | Color shift.Total area36 | 10613 | values[2],686–687 | 25→36 | Isolée + infobulle |
| 68,700 | Color change.Fade désactivé | 12123 | flags, masque0004 | 001C→0018 | Isolée |
| 88,400 | Blinking.Speed61 | 14213 | values[0],682–683 | 40→61 | Isolée + infobulle |
| 102,145 | Couleur Background, Blinking | 15205 | HSV[0].H/S,706–708 | 0000/00→0283/8E ; V=0F préservé | Isolée ; couleur RGB exacte non lisible |
| 111,700 | Color sequence.Color change speed39 | 16409 | **values[4]**,690–691 | 80→39 | Isolée + infobulle |
| 130,167 | Sequence.Delay after sequence43 | 18449 | values[3],688–689 | 10→43 | Isolée + infobulle |
| 135,667 | Sequence.Reverse direction activé | 19087 | flags, masque0002 | 0008→000A | Isolée |
| 137,567 | Sequence.Fade activé | 19311 | flags, masque0004 | 000A→000E | Isolée |
| 148,233 | Scanner.Width41 | 20865 | values[2],686–687 | 20→41 | Isolée + infobulle |
| 154,333 | Scanner.Color mode2 activé | 21217 | flags, masque0020 | 004A→006A | Isolée |
| 187,267 | Wave.Width47 | 25123 | values[2],686–687 | 20→47 | Isolée + infobulle |
| 205,967 | Flame.Intensity67 | 26967 | **values[0]**,682–683 | 40→67 | Isolée + infobulle |

Les images de voisinage ne sont pas publiées ; elles restent locales. Le champ
CSV `video_sample_s` permet de retrouver le point examiné dans l'original.
Les changements du même contrôle répétés sont conservés, notamment Speed
Color change40→56→40 et Intensity Flame50→40→67.

### Matrice des23 effets nommés et correspondances values[]

Le catalogue vient des métadonnées X.94 du lot4 ; cette vidéo ne prouve pas
à elle seule la disponibilité matérielle des23 effets. Les plages ci-dessous
sont des **valeurs distinctes observées**, pas des minima/maxima garantis.
Les libellés sont ceux visibles dans Aquasuite. `vN` désigne values[N].

| ID / effet | Contrôles numériques attribués, valeurs observées | Premiers v0..v4 capturés dans cette session | Limites |
|---|---|---|---|
| 01 FixedColor / Single color | Aucun curseur propre isolé | 0,0,0,0,0 | Choix de mode seulement ; couleur non éditée ici |
| 02 Breathing | v0 Speed60→49 ; v1 Intensity80→71 ; v2 Delay max5→22 ; v3 Delay min20→31 | 60,80,5,20,0 | Quatre paramètres attribués ; unités des délais inconnues |
| 03 Rainbow / Rotating rainbow | v0 Speed51 compatible ; v1 Color range100→99 | **51**,100,0,0,0 | Premier état déjà personnalisé, avant USB absent |
| 04 Blink1 / Blinking | v0 Speed40→61 | 40,1,0,0,0 | v1=1 compatible avec un compteur de couleurs, non isolé |
| 05 Blink2 / Color change | v0 Speed40→56→40 | 40,2,0,0,0 | v1=2 compatible avec un compteur de couleurs, non isolé |
| 07 Sequential / Sequence | v0 Speed25→56 ; v1 Smoothness30→17 ; v3 Delay after10→43 ; v4 Delay before5→27 | 25,30,2,10,5 | v2=2 non attribué par variation isolée |
| 08 Visor1 / Scanner | v0 Speed25→48 ; v1 Smoothness40→53 ; v2 Width20→41 | 25,40,20,0,0 | Width n'est pas démontré en nombre deLED |
| 09 Visor2 / Laser | v0 Speed25→41 ; v1 Smoothness40→29 ; v2 Width20→30 | 25,40,20,0,0 | Même indices que08, confirmés séparément |
| 0A Marquee1 / Wave | v0 Speed25→53 ; v1 Smoothness40→26 ; v2 Width20→47 | 25,40,20,1,0 | v3=1 non attribué |
| 0B Marquee2 / Color sequence | v0 Speed30→31 ; v1 Smoothness40→50 ; **v4 Color change speed80→39** | 30,40,1,2,80 | v2/v3 non attribués ; pas de réutilisation de v3 de13 |
| 0C Ambient / Color shift | v0 Speed30→42 ; v1 Color range50→38 ; v2 Total area25→36 | 30,50,25,0,0 | Aucun lien écran observé pour cet effet |
| 0D Bar1 / Bar graph | Inconnus | Non capturé05 | Source, seuils et couleurs non isolés |
| 0E Candle / Flame | **v0 Intensity50→40→67** | 50,0,0,0,0 | v0 n'est pas Speed ici |
| 0F Rain1 / Rain | Inconnus | Non capturé05 | Aucun contrôle corrélé |
| 10 Rain2 / Snowfall | Inconnus | Non capturé05 | Idem |
| 11 Rain3 / Stardust | Inconnus | Non capturé05 | Idem |
| 12 Switch / Color switch | Inconnus | Non capturé05 | Source et seuils inconnus |
| 13 RainbowShift / Swiping rainbow | v0 Point speed35→44 ; v1 Point smoothness10→22 ; v2 Point size5→14 ; v3 Color change speed90→79 ; v4 Color range25→38 | 35,10,5,90,25 | Cinq paramètres attribués séparément |
| 14 Sound1 / Sound flash | Inconnus | Non capturé05 | Audio non sélectionné |
| 15 Sound2 / Sound bars | Inconnus | Non capturé05 | Idem |
| 16 Sound3 / Sound slider | Inconnus | Non capturé05 | Idem |
| 17 Sound4 / Sound shift | Inconnus | Non capturé05 | Idem |
| 18 AmbientPx / AMBIENTpx | Inconnus | Non capturé05 | Vidéo réactive non sélectionnée |

Pour chaque ligne capturée, v5..v11 restent0. Leur rôle ne devient pas «inutilisé»
pour autant. Le premier état après choix de mode est un **profil observé**,
pas une valeur de fabrique : Aquasuite peut initialiser/restaurer des réglages.
La session ne réalise pas le test de mémoire de mode avec retour à un ancien
effet personnalisé. Les champs initialisés ensemble à un changement de mode
restent distincts des35 éditions numériques enregistrées.

### Masques d'options observés, groupe6 flags BE664–665

| Masque | Option et effets effectivement corrélés | Trames de preuve |
|---|---|---|
| 0002 | Reverse direction :03,07,08,09,0A,0B ; **Fade in :04** | 3999/4007 ;19087 ;21203 ;23413 ;25517 ;16771 ;14253 |
| 0004 | Fade :05,07 ; **Fade out :04** | 12123/12133 ;19311 ;14341 |
| 0008 | Random color :04,05,07,08,09,0A,0B | 14411 ;11939/12781 ;19051 ;21017 ;23055 ;25597 ;16647 |
| 0010 | Slide colors :04,05 | 14493 ;12015/12805 |
| 0020 | Color mode2 :08,09 | 21217 ;23445 |
| 0040 | Color change :08,09 | 20977 ;23025 |
| 0080 | Circular :08,09,0A | 21315 ;23569 ;25681 |

Chaque bascule isolée change seulement le champ flags et le CRC. Les trente
bascules donnent26 couples effet/option distincts. **Un masque n'a pas un
nom universel :0002 signifie Reverse dans plusieurs effets mais Fade in dans
Blinking.** Le bit0001 et les bits hauts n'ont pas été isolés. Ces flags de
contrôleur ne sont pas les flags de strip238–239.

### Couleurs, palettes et changement de mode

Dans Blinking, la commande Background correspond bien à HSV[0],706–709 :
la modification15205 change H/S et préserve V=15. Les couleurs d'effet visibles
commencent ensuite dans d'autres entrées de palette ; leur nombre n'est pas
déduit du nombre de champs stockés. Aucun changement isolé de nombre de couleurs,
de V, de source ou de binding n'est réalisé dans05.

L'activation de Random color fait disparaître certaines palettes dans l'UI et
remonte les commandes suivantes sans changement de leur rôle. C'est pourquoi
la corrélation ne repose pas sur une coordonnéeY fixe d'un bouton.
Les transitions de mode initialisent simultanément bindings, paramètres,
flags et couleurs : ce sont des actions de sélection distinctes, pas autant
de curseurs modifiés manuellement. Les différences complètes restent dans
`all_functional_differences` du CSV.

## 5. Sauvegardes Output02 analysées séparément

Les quinze commandes sont identiques : ID02, commande BE32=2, argument0,
CRC BE=34C6 sur les8 octets de corps. Transport : **Control EP0, IF1,
SET_REPORT Output, wValue0202**, sans Interrupt OUT ou Bulk OUT02.

Trames de soumission : **4643,6761,7755,11189,12671,13287,13973,14845,
15585,17273,19913,21877,23967,26327,27463**. Les complétions, temps et derniers
SET03 associés sont tous inclus dans le CSV. Les délais depuis le dernier SET03
vont de **3,606782 à3,838525s** ; la majorité est proche de3,83s.

Une seule sauvegarde peut suivre plusieurs réglages. Par exemple, les quatre
éditions Breathing8805/9039/9307/9511 ne sont pas quatre sauvegardes ; le prochain
Output11189 survient après le changement Color shift9935 et ses trois réglages.
Aucun bouton de sauvegarde de la vidéo n'est attribué arbitrairement à02.
Ni un contrat de temporisation, ni une persistance EEPROM/flash ne sont prouvés.

## 6. Incertitudes réduites et prochaines captures utiles

Les captures01–04 fournissaient le layout et les différences brutes ;05 permet
de nommer les curseurs et options cités ci-dessus. Restent principalement :

1. **Premier Speed Rainbow et limites UI :** capture ciblée d'un état initial complet puis d'une seule vitesse, avec infobulle avant/après. Pour les durées/largeurs, relever aussi toute unité affichée ; ne pas traduire22 en22ms ou22s sans preuve.
2. **Palette :** sous un effet stable, ajouter/retirer exactement une couleur, puis modifier seulement son V. Cela départagera les compteurs candidats et l'index de début de palette selon l'effet, sans toucher au master.
3. **Sources et bindings :** source RGB interne unique puis une seule borne ou un seul filtre. Les modèles sont connus mais aucune variation isolée de ces champs n'apparaît ici.
4. **Mémoire de mode :** personnaliser un seul paramètre, quitter l'effet puis y revenir dans des captures distinctes. Les premiers états de05 ne suffisent pas à décider preset/restauration.
5. **Dix effets non sélectionnés :** Bar1, Rain1–3, Switch, Sound1–4, AmbientPx. Pour audio/écran, un lot distinct doit autoriser explicitement une source synthétique ; aucune activation ou acquisition de contenu n'est effectuée par cette analyse.

Pour une meilleure mesure temporelle, un futur enregistrement peut inclure un
repère commun volontaire clairement identifiable, puis une longue pause avant
et après une action isolée. Il doit permettre de séparer le début du geste,
sa fin, la modification UI et l'envoi USB. Le présent lot ne justifie pas de
resynchroniser les fichiers ou d'en modifier les timestamps.

## 7. Format des CSV, vérification et bilan quantifié

CSV UTF-8, séparateur virgule, champs entre guillemets, **point décimal** pour
les temps. Les noms des contrôles sont ceux de l'UI anglaise. Un index de groupe
ou values[] est **zéro-indexé** ; les offsets comprennent l'ID HID en position0.
Les flags/CRC sont hexadécimaux ; H/S/V du tableau d'événements sont explicités
quand ils sont hexadécimaux. Les données numériques des différences
fonctionnelles sont décimales. Une case avant vide/non_capture signifie absence
de preuve, pas zéro.

Le CSV d'événements contient aussi `completion_frame`, `crc_received`,
`crc_calculated`, `structure`, `last_config_frame` et `save_delay_s`.
`ui_after_read` n'est renseigné pour un curseur que lorsqu'une valeur numérique
a été effectivement lue dans une infobulle ; autrement le nombre provient du
rapport USB. La précision de `video_sample_s` est celle d'une image examinée,
pas celle de l'instant d'interaction. Les images privées ne sont pas nécessaires
pour exploiter les champs USB mais doivent rester locales pour une revue vidéo.

Vérifications : lecture FFprobe, ancres visuelles réparties sur la durée,
contrôle ID/taille/structure/CRC, décodage des624 enregistrements de groupe,
comparaison exhaustive des77 paires de configurations, séparation des15 Output,
et contrôle des empreintes originales. Aucun test C++, contrôleur ou outil de
diagnostic matériel n'est exécuté.

| Mesure | Résultat |
|---|---:|
| Effets sélectionnés/présents visuellement dans cette vidéo | **13/23** |
| Configurations03 complètes, structure3, CRC conformes | **78/78** |
| Sauvegardes02 distinctes, CRC conformes | **15/15** |
| Éditions numériques / bascules / couleur / sélections de mode | **35 /30 /1 /12** |
| Couples effet/paramètre numérique attribués par variation isolée | **32** |
| Couple numérique seulement compatible, premier état sans avant USB | **1** |
| Couples effet/option confirmés / masques distincts concernés | **26 /7** |
| Association palette supplémentaire confirmée | **1**, Blinking.Background→HSV[0].H/S |
| Total des couples effet/contrôle distincts dans le CSV de paramètres | **60**, dont59 confirmés et1 compatible |
| Couples effet/slot values[] encore sans attribution | **243 sur276 slots potentiels**, certains pouvant être inactifs |
| Effets sans sélection dans05 | **10** |

Le dénominateur276 est23 effets×12 emplacements stockés, **pas276 réglages
utiles démontrés**. Les33 attributions numériques comprennent le cas compatible ;
les243 autres emplacements restent sans nom de fonction établi, même s'ils
valent0. Les bornes complètes, les unités physiques, les sources audio/vidéo et
les garanties de persistance restent inconnues.

**Arrêt après cette analyse. Aucun C++ modifié, aucune commande USB/HID,
aucune écriture matérielle, aucun fichier propriétaire modifié, aucun commit,
push ou PR. Aucune approbation d'écriture n'est déduite de la corrélation.**
