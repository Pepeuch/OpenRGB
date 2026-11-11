# D5 NEXT — Lot 7 : échanges Bulk associés aux effets PC

Preuve : `d5next_baseline06.pcapng`, session vidéo `d5next test2.mp4`.
Firmware1027, structure3. Analyse exclusivement hors ligne ; aucun nouveau
transport n'est implémenté. Inventaire, synchronisation et CSV dans
[le rapport principal](D5NextRGBpxAdvancedVideoUSBMapping.md).

## 1. Transport réellement observé

Descripteurs38/40 : `0C70:F00E`, bus2/adresse5, interface0 vendor,
Bulk OUT02 et IN81 (paquet maximum64). **973 soumissions OUT**, chacune appariée
à une complétion `SUCCESS` par référence de requête et IRP. **Aucun Bulk IN**.
Les complétions ne portent pas de réponse applicative ; un succès URB ne prouve
pas une interprétation du contenu par le moteur RGBpx.

Les latences soumission→complétion vont de **0,058 à0,778 ms**. Les tailles35 et364
désignent des charges utiles d'opérations complètes, pas des paquets physiques64.
Les octets initiaux0A/0B sont des discriminateurs de **charge Bulk** ; rien ne
permet de les appeler rapports HID. L'Output HID02 SettingsSave reste un transfert
Control EP0 IF1 distinct, même si son numéro ressemble à celui de l'endpoint OUT.

| Format observé | Nombre | Première paire / temps USB | Dernière paire / temps USB | Contenus distincts |
| --- | ---: | --- | --- | ---: |
| 35 octets, préfixe `0A 00 00 00` | 327 | 19333→19334 /163,047567 s | 46921→46922 /366,259010 s | **1** |
| 364 octets, préfixe `0B 06 06 00` | 646 | 46969→46970 /366,889915 s | 51951→51952 /397,542357 s | **99** |

Le CSV AdvancedUSBEvents contient toutes les références exactes et signale si le
contenu est identique au précédent de même taille. Aucun payload dynamique ni
empreinte de contenu privé n'y est publié.

## 2. Format35 : candidat de mise à jour audio, mais corps constant nul

Observations byte à byte sur **327/327 opérations** :

| Zone | Observation | Qualification |
| --- | --- | --- |
| Octet0 | 0A constant | Confirmé par répétition |
| Octets1–32 | 32 octets **tous nuls** | Confirmé par comparaison exhaustive |
| Octets33–34 | Correspondent au CRC-16/USB BE du corps1–32 | 327/327 vérifiés |
| Compteur variable | Aucun champ ne varie | Non observé |

Le calcul incluant aussi l'octet0 ne correspond pas au CRC final. Le même
algorithme CRC a été validé par le vecteur `123456789 → B4C8` et les rapports03/02.
Ce découpage est un constat de framing ; **32 octets ne prouvent ni huit float32,
ni16 niveaux16 bits, ni un spectre ou une table RGB**. Aucun ordre de canaux,
encodage numérique ou niveau audio non nul ne peut être établi sur un corps nul.

Fréquence : intervalle moyen **0,623347985 s**, min0,604059, max0,632986,
soit **1,604240 opérations/s** entre première et dernière soumission. Il n'y a
aucune variation de contenu à corréler à une bande audio ou à une couleur.

Relation au dernier mode Feature03 connu :

| Mode groupe6 | Nombre35 | Constat |
| --- | ---: | --- |
| Sound bars15 | 97 | Début alors que Low apparaît dans le menu source vidéo entre161 et162 s |
| Sound flash14 | 59 | Flux inchangé malgré l'affectation progressive des quatre sources PC |
| Sound slider16 | 103 | Flux inchangé pendant variations de neuf paramètres et des sous-modes |
| Sound shift17 | 68 | Flux inchangé jusqu'à la préparation d'AMBIENTpx |

Ce sont des **envois périodiques associés aux panneaux audio**. Ils ne prouvent
pas que l'acquisition audio produise des niveaux exploitables pendant la session.
Source silencieuse, absence d'échantillons, banque de valeurs réservée, rafraîchissement
ou autre commande restent des possibilités, pas des conclusions. Le menu UI
Low/High/Volume ne prouve pas à lui seul l'exécution d'une FFT ni la présence d'un
signal sonore. La piste AAC n'a pas été consultée pour résoudre ce point.

## 3. Format364 : contenu variable corrélé à AMBIENTpx

Les646 opérations partagent le préfixe4 octets **`0B 06 06 00`**.
Les seules positions variables sont :

```text
5,6,7, 9,10,11, 13,14,15, 17,18,19, 21,22,23, 25,26,27
```

Les octets4,8,12,16,20,24 sont constamment nuls ; la zone28–363 est nulle.
Entre3 et21 octets sont non nuls par opération, en comptant l'en-tête.
Cela décrit **six mots de4 octets contenant un zéro puis trois composantes
susceptibles de varier**, après un en-tête4 octets, et un suffixe nul.

La longueur se décompose arithmétiquement en `4 + 90×4`. Le maximum90 LED connu
du layout et les six LED du groupe6 rendent une table de couleurs plausible.
**Cette compatibilité n'établit pas un codec, l'ordre RGB/BGR, un canal alpha,
un numéro de groupe, une quantité de LED ou une longueur codée dans l'en-tête.**
Les deux octets06 restent identiques et constants : leur rôle ne peut pas être
séparé expérimentalement. Les mots sont décrits structurellement, sans reconstruire
les couleurs d'un bureau ou exporter les valeurs de pixels.

Aucun compteur variable distinct n'est identifié. Aucun CRC-16/USB final ne
correspond aux deux portées testées : bytes0..361 ou1..361, comparées au mot
finalBE362–363, **0/646 correspondances**. Cela ne prouve pas l'absence de toute
autre intégrité ou d'un framing différent ; cela distingue simplement ce format
des rapports03/02 et du candidat0A validés ici.

Cadence sur toute la fenêtre : **21,042369 Hz**, intervalle moyen0,047523166 s,
minimum0,029984 s, maximum **1,094199 s**. Médiane **0,031307 s**, environ31,94 Hz.
Sur645 intervalles, **596 <60 ms, 10 >100 ms, dont9 >500 ms**. La moyenne21 Hz
ne décrit donc pas un cadenceur fixe : des séquences rapprochées alternent avec
des pauses. Ni30, ni32, ni60 Hz ne doit être traité comme une garantie de protocole.

La vidéo montre un choix d'affichage et les zones Top/Bottom/Left/Right, puis des
réglages Brightness, Desktop range et Reverse. Il n'y a **aucun Feature03 après
47291**, malgré ces changements visibles. Le flux364 varie dans cette période :
un calcul de contenu côté PC est compatible avec les observations. Il n'existe
pas de variation de mire connue permettant d'attribuer chaque composante.
Les événements UI du CSV donnent seulement les fenêtres et trames Bulk candidates,
sans inventer une première trame de réaction ni un lien causal pour chaque curseur.

## 4. Transition, concurrence et limites temporelles

Dernier35 à366,259010 s ; premier364 à366,889915 s : écart **0,630905 s**.
**Aucun chevauchement des deux formats n'est observé.** Le premier364 précède
le SET Feature03 AMBIENTpx47291 (368,168762 s) de **1,278847 s**.

**40 opérations364** surviennent alors que le dernier mode écrit est encore
Sound shift17 ; les **606 suivantes** après la sélection03 deAMBIENTpx18.
Le dialogue vidéo affiche déjà le choix AMBIENTpx pendant cette transition.
Une préparation/prévisualisation logicielle explique potentiellement ce décalage,
sans prouver une exécution simultanée des moteurs. Il ne faut pas attribuer les
40 premières opérations à l'algorithme Sound shift sur le seul dernier mode03.

Les groupes0–5 externes restent en effets03/08/0B/04/04/04, groupe7 en00.
Il n'y a pas deux groupes distincts configurés respectivement Sound et AMBIENTpx.
La coexistence audio/vidéo, le multiplexage et l'exclusion mutuelle ne sont donc
**ni démontrés ni réfutés**. Les deux formats utilisent le même endpoint, mais
ce fait seul ne démontre pas un multiplexage applicatif.

Le début du35 et la commutation vers364 sont capturés. Le flux364 se poursuit
jusqu'à la fin de la capture ; l'arrêt de l'enregistrement vidéo n'est pas une
désactivation du moteur. Aucun arrêt final puis reprise contrôlée n'est observé.
Input01 reste de la télémétrie à environ1 Hz, et SettingsSave une commande
ponctuelle ; aucun des deux n'est le streaming de contenu.

## 5. Ce que l'on peut préparer ensuite

Les métadonnées déjà documentées du service D5 distinguent WinUSB, réglages et
liens audio/vidéo, ainsi que des dates de mise à jour audio/LED. Elles sont
compatibles avec ces observations d'interface0. La présence de composants FFT
ou capture d'écran ne prouve pas leur exécution dans cette session. Aucun
contournement, patch ou code propriétaire n'a été utilisé ici.

Pour de futures fixtures et une contribution aquapx, conserver le framing minimal,
les règles de CRC vérifiées et les mesures de cadence comme **observations**.
Avant un encodeur actif : mire synthétique connue, canaux audio stimulés séparément,
variation du nombre et du groupe de LED, captures d'activation/arrêt et coexistence.
Il manque encore la sémantique du corps0A, des deux06, des composantes0B, leurs
préconditions et la politique d'arbitrage des deux sources. Un générateur supposé
RGB ou de niveaux audio ne serait pas justifié par ce lot.

**Streaming variable observé : oui, format364. Audio dynamique décodé : non.
Aucune écriture matérielle ni modification C++ autorisée par cette analyse.**

## Complément du 11 novembre 2026 : nouvelles mires test3 et test4

Les conclusions ci-dessus décrivent la capture06 du Lot7. Les acquisitions
suivantes apportent une preuve qui y manquait : des mires synthétiques connues.
Le [test3/capture07 remplacés](D5NextAmbientSlideshowAnalysis.md) et le
[test4/capture08](D5NextAmbientDisplayOptionsAnalysis.md) confrontent désormais
les six mots actifs à des bandes primaires et combinées ainsi qu'à des aplats.
La correspondance des composantes **`00 G R B`** est confirmée pour ces mots,
par plusieurs observations indépendantes. L'octet00 n'est toujours pas attribué
à un canal alpha ; les deux06 de l'en-tête, le routage général, les gains et
la géométrie complète des options d'affichage restent à établir.

Ces nouvelles captures contiennent respectivement2067 et3265 opérations Bulk364,
sans Feature03 ni Output02. Bottom/Left/Right sont visibles dans test4, sans
rapport HID associé. Le premier test3 interrompu est exclu ; seules les empreintes
des nouveaux rapports identifient les enregistrements utilisés. Aucun C++ ou
transport actif n'est ajouté, et les barrières d'écriture restent inchangées.

### Test5 / capture09 : fonctions de réglage, sans nouvelle analyse des couleurs

Le [rapport des réglages](D5NextAmbientSettingsAnalysis.md) distingue le choix
du bord, les deux bornes longitudinales et la profondeur vers l'intérieur de
l'écran, à partir des gestes et rectangles d'aperçu. Six transitions de bord
et18 valeurs de curseurs sont documentées. La profondeur50 est visible à
l'extrémité du curseur simple, avec un aperçu proche d'un demi-écran : position
maximale ne signifie donc pas nécessairement couverture de tout l'écran.

La capture contient1426 Bulk364 et80 Input01, sans Feature03 ni Output02.
Aucun offset de réglage HID n'est attribué. La géométrie du véritable calcul
de pixels, le découpage des six valeurs, Reverse et la persistance restent
à tester. Le rapprochement temporel de cette vidéo avec le PCAP est nominal,
fondé sur les métadonnées ; il ne réutilise pas la synchronisation des mires.

### Test6 / capture10 : confrontation à quatre quadrants connus

La [mire quatre quadrants](D5NextAmbientQuadrantsAnalysis.md), fournie en PNG
et vérifiée pixel par pixel, permet de confronter les changements de bord au
flux : Top produit RRR/GGG,Bottom BBB/YYY,Left RRR/BBB etRight GGG/YYY pour
les mots0–2/3–5. Top etRight donnent des observations propres ; Bottom etLeft
sont partiellement contaminés par la fenêtre Aquasuite superposée à la mire.
L'axe horizontal/vertical et l'ordre entre les deux triplets sont ainsi
étayés, sans résoudre l'ordre interne des triplets ni la règle de moyenne.

La capture contient1804 Bulk364 et62 Input01, sans Feature03 ni Output02.
Trois repères par motifs de dominantes complètent la synchronisation nominale.
Le fond de l'aperçu Windows n'est pas traité comme une preuve de capture live.
Aucun nouveau paramètre HID ni écriture matérielle n'est ajouté.

### Test7 / capture11 :13 LED externes, sans flux PC observé

Le [rapport de la sortie13 LED](D5NextExternal13LEDAnalysis.md) distingue le
LED Controller1 externe (groupe0,strip0,start0,count13) du groupe Base de la
pompe (groupe6,strip1,count6). Les changements de configuration de l'un préservent
l'autre ; les offsets241/242 sont confrontés aux déplacements/redimensionnements.
Le second groupe Pump7 est actif et recouvre les mêmes six positions que g6.

Cette capture contient14 Feature03 valides,7 Output02 et93 Input01, **aucun Bulk**.
Elle ne résout donc pas les deux06 de l'en-tête AMBIENTpx. La bascule System
aquabus/RGBpx annoncée par l'utilisateur n'y est pas isolée avant/après. Aucune
commande d'activation de sortie, priorité de recouvrement ou nouvelle écriture
matérielle n'est déduite ou implémentée.
