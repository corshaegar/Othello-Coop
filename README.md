# Othello Coop

Koop-Modus für **Gothic 1** mit der Mod **Othello 4.4.2.1** – zu zweit durch die Kolonie.
Grundlage ist [Gothic Together / GothicCoop](https://gitlab.com/gothiccoop/gothiccoop) (Union-Plugin), angepasst an Othello.

*English below.*

> **Alpha (Version 0.1.0).** Getestet mit zwei Spielern (Host + ein Mitspieler) und der deutschen Othello-Fassung.
> Vorher die Spielstände sichern.

## Was geht

- **Der Host führt die Welt:** Truhen, Gegenstände, Leichen, Spawns, Tagesablauf der NPCs.
- **Story gemeinsam:** Dialoge (für den anderen sichtbar), Quest-Fortschritt, Tagebuch, Kapitel und Quest-EP für beide.
- **Kampf:** NPCs greifen auch den Mitspieler an; wer kämpft, dessen PC führt den Kampf. Schwierigkeitsgrad einstellbar.
- **Gemeinsame Spielstände:** Speichern und Laden zusammen, gemeinsames neues Spiel, Liste mit Vorschau.
- **COOP-Menü** im Hauptmenü: Figur wählen, Spiel eröffnen oder beitreten – per IP oder über **Steam**.
- Zuschau-Fenster mit der Sicht des Mitspielers, eigenes Aussehen der Figur, Menü auf Deutsch oder Englisch.

## Voraussetzungen

- Gothic 1 mit **Othello 4.4.2.1** (bringt Union und zParserExtender mit)
- GD3D11-Renderer – die ZIP ersetzt `system\GD3D11\Bin\g1_avx2.dll` durch eine Coop-Fassung
- **Alle Spieler brauchen dieselbe Version.**
- Verbindung per IP: Der Host gibt **UDP-Port 9955** frei (im Menü änderbar). Per Steam: befreundete Konten, die
  Gothic besitzen; das Spiel muss beim Beitreten schon laufen.

## Installation

1. Die ZIP aus den [Releases](https://github.com/corshaegar/Othello-Coop/releases) laden.
2. `system\GD3D11\Bin\g1_avx2.dll` sichern (wird ersetzt).
3. Den Inhalt der ZIP in den Gothic-Hauptordner ziehen (dort, wo der Ordner `system` liegt) und überschreiben.
4. In `system\Autorun` darf nur **eine** GothicCoop-DLL liegen: `Gothic1Coop.dll`. Ältere Fassungen mit anderem Namen
   (z. B. `GothicCoop*.dll`) entfernen.

Inhalt der ZIP:

| Datei | Zweck |
|---|---|
| `system\Autorun\Gothic1Coop.dll` | das Coop-Plugin |
| `system\Autorun\COOP_MENU.D`, `PC_MULTIPLAYER_HERO.D` | COOP-Menü und Mitspieler-Figur |
| `system\GD3D11\Bin\g1_avx2.dll` | Renderer mit Zuschau-Fenster und Figur-Vorschau |
| `GothicCoop-Lizenzen\` | Lizenztexte und Herkunft |

## Spielen

Hauptmenü → **COOP** → Figur einstellen → *Weiter: Verbinden* → *Spiel hosten* oder beitreten (*Über IP beitreten …*
bzw. *Steam …*). Danach *Neues Spiel* oder *Spielstand wählen*.
Unter *COOP-Einstellungen* (auch *Einstellungen → COOP*) liegen Sprache, Schwierigkeitsgrad, Coop-Kamera und Abgleich
(der Host gibt vor).
**Shift+F5** schaltet das Zuschau-Fenster. Das Protokoll steht in `system\GothicCoopOthello.log`.

## Bekannte Grenzen

- Skript-Trigger, die beide Helden auslösen, können beim Host doppelt wirken (z. B. doppelter Spawn).
- Beschwörungen des Mitspielers und einige Skript-Gegenstände entstehen nur bei ihm.
- Quest-Gegenstände bleiben bei dem, der sie abgibt.

## Quellcode bauen

`source\` ist der vollständige Quellcode der DLL in dieser Version, `scripts\` die beiden `.D`-Dateien.
Bauen mit den Visual Studio 2019 Build Tools:

```
cd source
msbuild GothicCoop.vcxproj /p:Configuration="G1 MD Release" /p:Platform=Win32 /p:PreBuildEventUseInBuild=False /p:PostBuildEventUseInBuild=False
```

Ergebnis `source\Bin\GothicCoop.dll` → als `system\Autorun\Gothic1Coop.dll` einsetzen. Den Renderer baut man aus
GD3D11 v17.9.7 mit `licenses\GD3D11-coop6.patch` (Anleitung in `licenses\GD3D11-HERKUNFT.txt`).

## Lizenz

| Teil | Lizenz |
|---|---|
| Coop-Plugin, Menü-Skripte und alle Änderungen (geänderte Fassung von Gothic Together) | [GNU AGPL-3.0](LICENSE) |
| Renderer `g1_avx2.dll` (geänderte Fassung von [GD3D11](https://github.com/Kirides/GD3D11)) | [GNU GPL-3.0](licenses/GD3D11-GPL-3.0.txt), Änderungen in [`GD3D11-coop6.patch`](licenses/GD3D11-coop6.patch) |
| ENet 1.3.17, nlohmann/json (in der DLL) | MIT, [`licenses/`](licenses) |
| Union-SDK, DirectX-Header in `source\SDK` | unverändert aus Gothic Together, jeweils eigene Bedingungen |

Keine Gewährleistung (AGPL-3.0/GPL-3.0, Abschnitte 15 und 16). Wer die ZIP weitergibt, muss den Quellcode mitgeben oder auf
dieses Repository verweisen – auch für Mitspieler über das Netzwerk (AGPL-3.0, Abschnitt 13).
Gothic und Othello sind nicht enthalten und gehören ihren Rechteinhabern. Kein offizielles Projekt von Piranha Bytes,
THQ Nordic oder dem Othello-Team.

---

## English

Co-op mode for **Gothic 1** with the **Othello 4.4.2.1** mod, based on
[Gothic Together / GothicCoop](https://gitlab.com/gothiccoop/gothiccoop) (Union plugin). **Alpha** (version 0.1.0),
tested with two players and the German Othello release. Back up your saves first.

**Features:** the host runs the world (chests, items, corpses, spawns, NPC routines); shared story (dialogues,
quests, diary, chapters, quest XP); NPCs fight both players, combat handled by the fighting player's PC; adjustable
difficulty; shared saves and a shared new game; COOP main menu to host or join via IP or **Steam**; spectator window
showing your partner's view; custom character look; German or English menus.

**Requirements:** Gothic 1 with Othello 4.4.2.1, the GD3D11 renderer, the **same version for all players**. Direct
connections need **UDP port 9955** open on the host; Steam joins need befriended accounts that own Gothic.

**Install:** download the zip from [Releases](https://github.com/corshaegar/Othello-Coop/releases), back up
`system\GD3D11\Bin\g1_avx2.dll`, extract the zip into the Gothic main folder (the one containing `system`) and
overwrite. Keep only **one** GothicCoop DLL (`Gothic1Coop.dll`) in `system\Autorun`.

**Play:** main menu → **COOP** → set up your character → *Weiter: Verbinden* → *Spiel hosten* (host) or join via IP or
Steam; then start a new game or pick a save. Switch the menus to English under *COOP-Einstellungen* (also
*Settings → COOP*), where difficulty, camera and sync live too. **Shift+F5** toggles the spectator window. Log: `system\GothicCoopOthello.log`.

**Source and license:** `source\` is the complete source of the DLL (build command above), `scripts\` holds the menu
scripts. The plugin and all changes are licensed under the [GNU AGPL-3.0](LICENSE); the renderer is a modified GD3D11
under GPL-3.0 (patch in [`licenses/`](licenses)); ENet and nlohmann/json are MIT. No warranty. If you share the zip,
share the source too. Gothic and Othello are not included. Not affiliated with Piranha Bytes, THQ Nordic or the Othello
team.
