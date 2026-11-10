# D5 NEXT — Windows, lot 1 : analyse passive USB

Date : 10 novembre 2026, Europe/Paris. Branche : `feat/d5next`.
Révision examinée : `3f80b24c35829c52ecb993daa8dd62221ed1be09`.

**Résultat : analyse terminée, preuve protocolaire partielle.** La capture
identifie `0C70:F00E` sur le bus **2**, adresse **5**, et montre 24 rapports
Input `0x01` de 162 octets sur l'interface HID **1**, endpoint **0x83**,
à environ **0,990 Hz**. Le firmware lu dans ces rapports est **1027**.
Aucun GET_REPORT, SET_REPORT, Output ou rapport de configuration n'est présent.
Le CRC de configuration, la structure 3, le transport de `0x02` et la sauvegarde
ne peuvent donc pas être validés par cette capture.

## 1. Périmètre et provenance

L'environnement Aquasuite X.94 et la structure 3 sont ceux annoncés par
l'utilisateur. La capture ne contient pas de métadonnée attribuant les URB à
un processus : elle ne permet pas de distinguer l'interface Aquasuite, son
service et le pilote Windows comme initiateurs des lectures.

Le dépôt et les rapports précédents sont disponibles :

- [Analyse statique Aquasuite](D5NextAquasuiteAnalysis.md), notamment sections 4–6 et 8–10.
- [Rapport initial du contrôleur](AquaComputerD5Next.md).
- [Validation en lecture seule](AquaComputerD5NextReadOnly.md).
- [Préparation du premier essai rose](AquaComputerD5NextPinkPreparation.md).

La seule capture trouvée dans l'inventaire du dépôt est
`Controllers/AquaComputerD5NextController/d5next_baseline.pcapng`.
Elle était non suivie par Git à l'ouverture de ce lot. Elle reste à cet
emplacement, intacte ; `.gitignore` exclut désormais `*.pcap` et `*.pcapng`.
Aucune capture brute ou extraction de charge utile n'est ajoutée à Git.

| Propriété | Valeur |
|---|---|
| Taille du fichier | 131 588 octets |
| SHA-256 | `09fa106c2e41f62504512b94f6e809438c144c4b7e2bbc69a5ebd5f79d3eb8f8` |
| Format / encapsulation | pcapng / USBPcap |
| Interface de capture | `\\.\USBPcap2`, interface pcapng 0 |
| Trames du fichier | 1 202, dont 54 pour la D5 NEXT |
| Début, heure de Paris | 2026-11-10 18:09:45.149488 CET |
| Fin, heure de Paris | 2026-11-10 18:10:09.002369 CET |
| Durée totale | 23,852881 s |
| Horodatage | Résolution microseconde, ordre strictement croissant ou égal |
| Outils de lecture | `capinfos` et `tshark` 4.6.9 ; calculs PowerShell en mémoire |

Toutes les opérations de ce lot ont porté sur des fichiers existants avec
`tshark -r`, `capinfos`, lecture des sources et calculs hors ligne. Aucun
outil OpenRGB ou diagnostic de pompe n'a été exécuté. Aucun accès direct
USB/HID, envoi de commande, changement Aquasuite, reset ou flashage n'a eu lieu.
Le contrôleur C++ n'a pas été modifié. Aucun commit, push ou PR n'a été créé.
Aucun code propriétaire n'est reproduit. Les charges utiles complètes,
identifiants privés et données des autres périphériques sont omis.

## 2. Identification et interfaces

La trame **38** contient le descripteur DEVICE : VID `0x0C70`, PID `0xF00E`,
bus 2, adresse 5, `bcdDevice=0x0100`, EP0 de 64 octets. `bcdDevice` ne vaut
pas identification du firmware ; 1027 est confirmé séparément dans Input `01`.
Les adresses sont celles de cette session et ne sont pas des identifiants stables.

La trame **40** contient un descripteur CONFIGURATION de 65 octets,
configuration 1, deux interfaces, alternate setting 0 :

| Interface | Classe | Endpoints déclarés | Trafic observé après le préambule |
|---|---|---|---|
| 0 | Vendor Specific `0xFF` | Bulk IN `0x81`, Bulk OUT `0x02`, 64 octets chacun, `bInterval=0` | Aucun |
| 1 | HID `0x03`, sous-classe/protocole 0 | Interrupt IN `0x83`, 64 octets, `bInterval=5` | Input `0x01` |

Le descripteur HID annonce HID 1.11 et un descripteur de rapports de **70 octets**.
Ce dernier n'est pas inclus : aucune requête de descripteur `0x22` n'est présente.
Les types et tailles des rapports absents ne peuvent pas être redéduits ici.
L'interface 1 n'annonce aucun endpoint Interrupt OUT. L'endpoint `0x02` de
l'interface 0 est **Bulk OUT**, sans rapport avec l'ID HID `0x02`.

La capture représente des URB : une complétion de 162 octets avec un endpoint
déclaré à 64 octets ne décrit pas un paquet USB physique unique de 162 octets.
`bInterval=5` est une valeur du descripteur, pas la période des rapports mesurée.
La vitesse USB effective et les tentatives de polling sans données ne sont pas
établies par cette analyse.

## 3. Isolement reproductible

Filtre Wireshark/TShark couvrant toutes les trames du périphérique identifié :

```text
usb.bus_id == 2 && usb.device_address == 5
```

La recherche `usb.idVendor == 0x0c70 && usb.idProduct == 0xf00e` retrouve la
trame d'identification 38 ; ces champs de descripteur ne sont pas portés par
chaque transfert. Utiliser ensuite le couple bus/adresse pour cette capture.
Tous les numéros de trame ci-dessous se réfèrent au fichier original.

Exemple de lecture locale sans produire de copie brute :

```powershell
& 'C:\Program Files\Wireshark\tshark.exe' `
  -r 'Controllers\AquaComputerD5NextController\d5next_baseline.pcapng' `
  -Y 'usb.bus_id == 2 && usb.device_address == 5' `
  -T fields -e frame.number -e frame.time_relative -e usb.endpoint_address `
  -e usb.transfer_type -e usb.irp_info -e usb.data_len -e _ws.col.Info
```

Pour les seuls rapports reçus, ajouter
`&& usb.transfer_type == 1 && usb.data_len > 0` au filtre.
Dans le champ **USBPcap** `usb.transfer_type`, Interrupt vaut 1 et Control 2 ;
ces nombres ne sont pas les bits de type `bmAttributes` des endpoints.

## 4. Inventaire et fréquences

| Échanges D5 NEXT | Trames / opérations | Données utiles | Cadence |
|---|---|---|---|
| Control EP0, préambule | 6 trames, 3 paires 37–38 / 39–40 / 41–42 | DEVICE 18 octets ; CONFIGURATION 65 octets ; SET_CONFIGURATION sans données | Événement initial, pas de fréquence périodique |
| Interrupt IN `0x83` | 48 trames : 24 complétions avec données et 24 soumissions vides | 24 × 162 = 3 888 octets, ID `01` | 0,990184 rapport/s sur les intervalles |
| Interrupt OUT | 0 | Aucun | 0 observé |
| Bulk IN/OUT sur IF0 | 0 | Aucun | 0 observé |
| HID GET_REPORT / SET_REPORT sur EP0 | 0 / 0 | Aucun | 0 observé |

Les 78 premières trames de la capture entière ont le même horodatage initial
et des IRP ID nuls ; elles décrivent plusieurs périphériques. Les trames D5
37–42 présentent les mêmes caractéristiques. C'est **compatible avec un
préambule d'inventaire reconstruit par la capture** ; cette interprétation
reste une déduction. Ces entrées ne prouvent ni une réénumération réellement
survenue à cet instant ni une commande lancée par Aquasuite.

Les complétions Input sont les trames :

```text
129, 349, 473, 493, 621, 623, 625, 635, 835, 915, 919, 1001,
1003, 1005, 1007, 1009, 1011, 1039, 1089, 1121, 1123, 1193, 1195, 1201
```

Chaque complétion (`usb.irp_info=0x01`) est immédiatement suivie d'une
soumission IN vide (`0x00`, numéro de trame +1). La soumission vide est une
demande de lecture par l'hôte, **pas un rapport Output envoyé à la pompe**.
Les IRP sont réutilisées : une complétion clôt la lecture précédente et une
nouvelle soumission relance la lecture. Par exemple, 130 précède la complétion
473 de même IRP ; 350 précède 493. Les lectures correspondant aux complétions
129 et 349 ont été soumises avant la fenêtre visible ; les soumissions 1196 et
1202 n'ont pas de complétion ultérieure dans le fichier. Il ne faut donc pas
compter 48 rapports ni apparier naïvement chaque complétion avec la trame suivante.

Première complétion : **t=0,624768 s** ; dernière : **t=23,852777 s**.
Sur les 23 intervalles : minimum **1,007924 s**, moyenne **1,009913435 s**,
maximum **1,012086 s**. Cadence calculée : `23 / (23,852777 - 0,624768)`.
Le quotient `24 / 23,852881 = 1,006168 rapport/s` sur toute la fenêtre dépend
des bords et n'est pas l'estimation retenue de la période.

Les 54 trames D5 ont `USBD_STATUS_SUCCESS`. Aucun paquet tronqué au sens
`frame.cap_len < frame.len`, ni indication `_ws.malformed`, n'a été trouvé pour
ce périphérique. L'absence de statistiques de perte dans le fichier ne permet
pas de garantir l'exhaustivité de la capture.

## 5. Rapports HID, CRC et commandes

| ID | Preuve dans cette capture | Conclusion |
|---|---|---|
| `0x01` | 24 charges utiles de 162 octets, toutes préfixées par `01` | Input sur IF1/EP83. Octets 13–14, offsets depuis l'ID en position 0 : `04 03`, soit BE 1027, dans les 24 rapports |
| `0x02` | Absent | Aucun Output ni commande système/sauvegarde à décoder |
| `0x03` | Absent | Aucun GET/SET Feature de configuration, taille et CRC non vérifiables ici |
| `0x08` | Absent | Aucun rapport de noms observé |
| `0x0C` | Absent | Aucun rapport d'affichage observé |

**CRC de configuration : non évaluable, zéro échantillon.** Le codec C++
`D5NextProtocol::ValidateReport` attend pour `03` 809 octets, un CRC-16/USB
(polynôme `0x8005`, init/xorout `0xFFFF`, entrée/sortie réfléchies), calculé sur
les offsets **1 à 806 inclus**, ID exclu, puis stocké BE aux offsets **807–808**.
Aucun rapport capturé ne permet de tester cette hypothèse. Le CRC `EFD1`
des rapports antérieurs n'est pas une observation de ce lot. Aucun CRC n'est
attribué arbitrairement au rapport de télémétrie `01` ou aux descripteurs USB.

Seules les requêtes Control suivantes sont représentées :

| Trames | Décodage | Portée de la preuve |
|---|---|---|
| 37–38 | Standard GET_DESCRIPTOR DEVICE, `bmRequestType=80`, `bRequest=06`, type `01`, index 0, longueur 18 | Identification, préambule décrit plus haut |
| 39–40 | Standard GET_DESCRIPTOR CONFIGURATION, `80/06`, type `02`, index 0, longueur 65 | Descripteurs USB, pas lecture du rapport HID `02` ou `03` |
| 41–42 | Standard SET_CONFIGURATION, `00/09`, configuration 1, `wIndex=0`, `wLength=0` | Sélection de configuration USB représentée dans le préambule ; pas SET_REPORT de paramètres D5 |

Après ce préambule, seules des lectures Interrupt IN sont visibles. Aucun
HID GET_REPORT (`bRequest=01` de classe), HID SET_REPORT (`bRequest=09` de classe),
rapport Output ou commande SettingsSave n'est observé. Le `bRequest=09`
**standard** de la trame 41 ne doit pas être confondu avec HID SET_REPORT.
L'ambiguïté Feature/Output de `02` reste entière ; aucune séquence, charge utile
de commande, condition de sauvegarde ou propriété de persistance n'est extrapolée.

## 6. Comparaison aux rapports et au C++

| Élément précédent | Confrontation à la capture |
|---|---|
| VID/PID, IF1 HID | Confirmés par 38/40 ; adresse de session Windows 2.5 |
| Input `01`, 162 octets, firmware BE13–14=1027 | Confirmés par les 24 rapports |
| Firmware 1027 / structure 3 déclarés dans Aquasuite X.94 | Firmware confirmé ; structure 3 non relue, faute de rapport `03` |
| Feature `03` 809 octets, CRC, offsets RGB | Aucune nouvelle preuve ; pas de configuration capturée |
| `02` Output 11 octets et SettingsSave=2 dans l'analyse statique | Non observés ; le descripteur de rapports lui-même est absent |
| `08` 773 octets et `0C` 1 025 octets dans les rapports précédents | Non vérifiés par cette capture |
| Hypothèse d'un canal RGB sur IF0 | IF0 Bulk présente, aucun trafic ; son rôle effectif n'est pas établi |

Sources C++ examinées dans `Controllers/AquaComputerD5NextController/` :
`AquaComputerD5NextControllerDetect.cpp`, `AquaComputerD5NextController.cpp`,
`D5NextProtocol.h` et `D5NextProtocol.cpp`. Le filtre VID/PID/interface du
détecteur concorde. Le contrôleur décrit un GET Feature `03`, validation,
modification ciblée puis SET Feature `03`, sans `02`. **Cette séquence n'apparaît
pas dans la capture** et ne reçoit donc aucune validation Windows nouvelle.
Le layout demeure inconnu en construction normale et la détection demeure
conditionnelle ; aucune barrière n'a été modifiée ou levée.

La capture ne permet de juger ni les effets RGB, ni le masque des octets
modifiables, ni l'ordre GET/SET/sauvegarde, ni la persistance sans sauvegarde.
L'absence d'écriture pendant environ 24 secondes ne prouve pas qu'Aquasuite
n'écrit jamais en régime de repos ou au démarrage.

## 7. Prochaines expériences proposées — non exécutées

1. Examiner en priorité d'autres captures déjà enregistrées, si disponibles,
   incluant le descripteur HID de rapports et des lectures de configuration.
   Réidentifier le bus et l'adresse dans chaque capture.
2. Dans un lot passif distinct, enregistrer une fenêtre de repos plus longue,
   sans changer de réglage ni relancer Aquasuite/service. Noter les heures,
   pages déjà ouvertes et contexte utilisateur pour faciliter l'attribution.
   Cela peut révéler des lectures périodiques ; leur présence n'est pas garantie.
3. Si un futur fichier contient `03`, relever longueur, ID, interface et
   direction, lire la structure BE1–2, puis tester le CRC sur chaque rapport
   complet avec le périmètre exact ci-dessus. Publier comptes et références de
   trames ; conserver les buffers de configuration privés.
4. Pour résoudre le transport de `02`, examiner une capture existante montrant
   un véritable envoi. Relever type de rapport dans `wValue` et `wIndex` si
   SET_REPORT, ou endpoint si Output, puis corréler aux événements documentés.
   Toute expérience provoquant sauvegarde, changement de mode, relance du
   service ou autre modification sort du présent lot et demanderait un périmètre
   distinct ; aucune manipulation de ce type n'est exécutée ni prescrite ici.

**Arrêt après cette analyse passive.** Le livrable conserve les absences et
incertitudes comme telles ; aucune commande manquante n'a été reconstruite.
