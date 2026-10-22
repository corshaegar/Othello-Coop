META
{
	Parser = Menu;
	Engine = G1;
};

// GothicCoop (Othello 4.4.2.1), ab v40: Eintrag ganz oben im Hauptmenü und Untermenü MENU_COOP.
// Nur zusammen mit der passenden GothicCoop-Fassung einsetzen (system\Autorun und gamemodes\GothicCoop).
// v45: Zuschau-Fenster klein|mittel|gross; v48: Kamera automatisch|nur Shift+F5.
// v58: Versionszeilen (MENUITEM_*_VERSION) - GothicCoop zeigt dort seine Fassung, im Hauptmenü mit Bauzeit.
// v60: MENU_COOP mit 3D-Vorschau rechts (MENUITEM_COOP_PREVIEW, GothicCoop zeichnet die Figur über den Coop-Renderer),
//      Beschriftungen und Werte dafür nach links (COOP_LBL_X/COOP_VAL_X).
// v63b: links abgeschnitten (Nutzer: "erbindung trennen") - Beschriftungen ab 1000, Knöpfe 1000-4200 und 4400-7600,
//       Statuszeilen 400-8192, Spielstand-Zeilen ab 1000 (Othellos eigene Menüs lassen links 1000 frei).
// v65: Hilfetexte zu Gesicht/Statur/Körper/Hautton - GothicCoop weicht nur auf Varianten mit echter Textur aus (Gothics
//      textures.VDF hat für manche nur rote 4x4-Platzhalter).
// v66: Einstellungen -> COOP -> Coop-Kamera: Kamera automatisch|nur Taste|aus, Fenstergröße 15-50 %, Lage, Taste,
//      Automatik ab 20-100 m.
// v67: Einstellungen -> COOP -> Schwierigkeitsgrad: Stufe, Schaden der Spieler/Gegner, Leben der Gegner, Kampf-EP, EP-Reichweite.
// v68: Kampf-EP als zwei Werte: EP allgemein 50-100 %, EP-Anteil Partner 0/50-100 %.
// v70: Leben der Gegner sind echte Lebenspunkte (GothicCoop skaliert HP und Maximum, vor dem Speichern zurück).
// v75: Einstellungen -> COOP -> Abgleich: Quests, Tagebuch, Gegenstände/Truhen, Spielstände gemeinsam/getrennt,
//      Auto-Sicherung, beim Verbinden Host-Stand/eigener Stand (Gothic.ini [GOTHICCOOP] coopSyncQuests, coopSyncLog,
//      coopSyncWorld, coopSyncSaves, coopAutoSave, coopConnectTake; der Host gibt vor). "Verbindung trennen" breiter
//      (Nutzer: Text links und rechts abgeschnitten).
// v55 (Nutzer: "im Hauptmenü statt Mehrspieler der Reiter COOP ... IP eingeben, IP wird gemerkt, Port 9955 einstellbar,
// Name, HeadModel, BodyModel ... Spiel hosten, Spiel beitreten ... Spielstand wählen: Liste der gemeinsamen Sessions,
// optional andere Spielstände ... Verbindung trennen. In den Einstellungen der Reiter COOP: Coop-Kamera, Schwierigkeitsgrad"):
//  - Hauptmenü: Eintrag "COOP" ganz oben (ab v81 zur Laufzeit in Othellos MENU_MAIN eingesetzt).
//  - MENU_COOP: IP, Port, Name (Eingabefelder - Gothic schreibt sie selbst nach Gothic.ini [GOTHICCOOP] coopServer,
//    coopPort, coopName), "Aussehen wie NPC" (Suche, coopLookNpc - GothicCoop sucht die NPC-Instanz und übernimmt deren
//    Mdl_SetVisualBody-Werte), Kopf, Gesicht, Statur, Körper, Hautton (Auswahl, coopHead/coopFace/coopBodyMesh/coopBody/
//    coopSkin), Spiel hosten, Spiel beitreten, Spielstand wählen (MENU_COOP_LOAD), Statuszeilen, Verbindung trennen.
//  - MENU_COOP_LOAD: Liste der gemeinsamen Spielstände (GothicCoop füllt die Zeilen), "Anderen Spielstand laden".
//  - MENU_OPTIONS: der Eintrag "Performance/Qualität" ist (Wunsch des Nutzers) durch "COOP" ersetzt (MENU_OPT_COOP:
//    Coop-Kamera, Schwierigkeitsgrad, Abgleich). Bis v80 per Kopie von Othellos Menü, ab v81 setzt GothicCoop den Eintrag
//    zur Laufzeit ein; Othellos Einstellung perfQualityIndex bleibt, wie sie ist (nur nicht mehr im Menü).
// Die Befehle COOP_* (SEL_ACTION_EXECCOMMANDS) wertet GothicCoop aus (Haken auf zCMenu::HandleSelAction, CoopMenuLobby.cpp).
// Datei in Windows-1252 speichern (Umlaute). Der META-Block muss das erste Wort der Datei sein (zParserExtender, Test v40).

// v81: Othellos MENU_MAIN und MENU_OPTIONS werden NICHT mehr ersetzt (bis v80 stand hier je eine Kopie). GothicCoop hängt
//      MENUITEM_COOPMAIN_COOP vor "Neues Spiel" ein (Othellos Einträge rücken um MENU_MAIN_DY nach unten), dazu
//      MENUITEM_COOPMAIN_VERSION, und setzt MENUITEM_OPT_COOP an die Stelle von MENUITEM_PERF (MENUITEM_PERF_CHOICE fällt weg).
//      Diese Datei enthält nur noch eigene Menüs und Einträge. Nur zusammen mit GothicCoop ab v81 einsetzen.
// v82: Esc-Menü im Spiel: "Hauptmenü" (MENUITEM_COOPINGAME_TITLE, GothicCoop hängt ihn vor "Gothic beenden" in Othellos
//      MENU_MAIN_INGAME ein) mit Abfrage MENU_COOP_TITLE - "Ja" (COOP_TITLE): Coop trennen, zurück zum Startbildschirm.
// v87: Coop über Steam: MENU_COOP_STEAM (Freunde, die Gothic spielen - beitreten/einladen, Spiel hosten, trennen),
//      erreichbar über den Knopf "Steam" in MENU_COOP und Einstellungen -> COOP -> Steam-Freunde (auch im Spiel).
//      Befehle COOP_STEAM_* wertet GothicCoop aus (CoopSteam.cpp).
// v89: MENU_COOP_STEAM mit "Spielstand wählen" (MENU_COOP_LOAD) und "Neues Spiel" (Othellos MENU_NG) - im Spiel sperrt
//      GothicCoop beide mit Hinweis (nur im Hauptmenü).
// v91: Esc-Menü im Spiel: "Spielstand wählen" (MENUITEM_COOPINGAME_LOAD, GothicCoop hängt ihn hinter "Spiel laden"
//      in Othellos MENU_MAIN_INGAME ein) mit derselben Liste wie im Hauptmenü (MENU_COOP_LOAD) - auch über das
//      Steam-Menü im Spiel. Gemeinsames neues Spiel: wählt der Host mit Mitspieler in Othellos MENU_NG eine
//      Schwierigkeit, öffnet GothicCoop MENU_COOP_NG (COOP-Schwierigkeit, "Spiel starten" = COOP_NG_START, "Abbrechen").
// v93: "Spielstand wählen" (MENU_COOP_LOAD) wieder mit der Vorschau aus v77/v78 (bei v81 aus einem älteren Stand übernommen
//      und dabei verloren): Liste 1000-4300, rechts je PC ein Bild mit Namen und 8 Angabezeilen. GothicCoop füllt die Liste
//      aus dem Coop-Archiv (blättern mit Rad/Pfeiltasten), dazu der Schalter MENUITEM_COOPLOAD_FILTER (COOP_LIST_LOCAL).
// v94: Neues Verbindungsmenü (Nutzer: "Neues Spiel findet man nur unter Steam ... dass man dem Spieler joinen kann, ist
//      unklar, da es nur klein in der Mitte angezeigt wird"). MENU_COOP ist die Startseite in einer Spalte: Verbinden (Spiel
//      hosten, Steam-Knopf COOP_STEAM_QUICK - GothicCoop setzt den Text, z. B. ">> Anna hostet - jetzt beitreten <<",
//      Steam-Freunde, Über IP beitreten), Spielen (Neues Spiel = Othellos MENU_NG, Spielstand wählen), Figur und Name,
//      Verbindung trennen. Neu: MENU_COOP_IP (IP, Port, Verbinden) und MENU_COOP_LOOK (Name, Aussehen, 3D-Vorschau).
//      MENU_COOP_STEAM nur noch Freunde (große Zeilen). MENU_COOP_LOAD mit Bildlaufleiste (MENUITEM_COOPLOAD_SCROLL/THUMB).
//      Plan und Befunde: gt-fix\PLAN-VERBINDUNGSMENUE.md.
// v95: Zwei Stufen (Nutzer: "erstmal Aussehen, Name wählen etc. Dann das Verbindungsmenü mit den ganzen Einstellungen",
//      "leicht zu groß"): COOP im Hauptmenü öffnet MENU_COOP_LOOK (Schritt 1: Sprache, Name, Aussehen, Vorschau,
//      "Weiter: Verbinden"), von dort MENU_COOP (Schritt 2: Verbinden, Spielen, COOP-Einstellungen = MENU_OPT_COOP). Knöpfe
//      1500-6692, Höhe 420. Sprache (Gothic.ini coopLang, Auswahl in Schritt 1 und Einstellungen > COOP): jeder eigene
//      Eintrag trägt sein Englisch in userString[2] (Text bzw. Optionen) und userString[3] (Infozeile) - GothicCoop tauscht
//      zur Laufzeit; Einträge, deren Text GothicCoop selbst setzt, haben nur userString[3]. userString[0] bleibt leer
//      (Auswahlfelder nehmen es als Cursor-Textur). Englisch ohne Anführungszeichen (Daedalus-Zeichenketten).
// v96: Sprachzeile in Einstellungen > COOP (Test v95: "die Schrift war zu groß und unten abgeschnitten"): kleine Schrift,
//      Höhe 400, Abstand zum Steam-Knopf darüber (dessen Feld reicht bis MENU_OPT_START_Y + MENU_OPT_DY * 3 + 750).

const int COOP_LBL_X  = 1000;     // Beschriftung links (v63b: wie Othellos Menüs ab 1000, bei 300 links abgeschnitten)
const int COOP_LBL_DX = 2200;
const int COOP_VAL_X  = 3300;    // Wert rechts (Eingabe/Auswahl), endet bei 5800 vor der Vorschau (5950)
const int COOP_VAL_DX = 2500;
const int COOP_Y0     = 1450;    // erste Zeile
const int COOP_DY     = 420;     // Zeilenabstand
const int COOP_ROW_DY = 380;     // Zeilenhöhe

// --- Hauptmenü: nur unser Eintrag COOP - GothicCoop (ab v81) hängt ihn zur Laufzeit in Othellos MENU_MAIN ein ----------

instance MENUITEM_COOPMAIN_COOP(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "COOP";
	text[1] = "Gemeinsam spielen: erst deine Figur, dann Verbindung und gemeinsame Spielstände.";
	posx = 0;
	posy = MENU_MAIN_Y + MENU_MAIN_DY * 1;
	dimx = 8192;
	dimy = 750;
	onselaction[0] = SEL_ACTION_STARTMENU;
	onselaction_s[0] = "MENU_COOP_LOOK";   // v95: erst die Figur
	flags = flags | IT_TXT_CENTER;
	userString[2] = "COOP"; userString[3] = "Play together: first your character, then connection and shared saves.";
};

// --- Untermenü COOP -----------------------------------------------------------------------------------------------------

instance MENU_COOP(C_MENU_DEF)
{
	backpic = MENU_BACK_PIC;
	items[0] = "MENUITEM_COOP_HEADLINE";
	items[1] = "MENUITEM_COOP_STEP2";
	items[2] = "MENUITEM_COOP_STATUS1";
	items[3] = "MENUITEM_COOP_STATUS2";
	items[4] = "MENUITEM_COOP_SECT_CONNECT";
	items[5] = "MENUITEM_COOP_HOST";
	items[6] = "MENUITEM_COOP_STEAMJOIN";
	items[7] = "MENUITEM_COOP_STEAM";
	items[8] = "MENUITEM_COOP_IPJOIN";
	items[9] = "MENUITEM_COOP_SECT_PLAY";
	items[10] = "MENUITEM_COOP_NEWGAME";
	items[11] = "MENUITEM_COOP_LOAD";
	items[12] = "MENUITEM_COOP_SETTINGS";
	items[13] = "MENUITEM_COOP_STOP";
	items[14] = "MENUITEM_COOP_BACK";
	items[15] = "MENUITEM_COOP_VERSION";
	defaultoutgame = 5;
	defaultingame = 5;
	flags = flags | MENU_SHOW_INFO;
};

instance MENUITEM_COOP_HEADLINE(C_MENU_ITEM_DEF)
{
	text[0] = "COOP";
	type = MENU_ITEM_TEXT;
	posx = 0;
	posy = MENU_TITLE_Y - 500;
	dimx = 8192;
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "COOP";
};

// Eingabefelder: Beschriftung links (wählbar, startet das Feld), Feld rechts (Typ MENU_ITEM_INPUT). Gothic liest den Wert beim
// Öffnen aus Gothic.ini und schreibt ihn nach RETURN zurück (zCMenuItemInput::InitMenuItem 0x004D51C0 / Run 0x004D53A0).

instance MENUITEM_COOP_IP(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "IP-Adresse des Hosts";
	text[1] = "RETURN zum Ändern - wird für das nächste Mal gemerkt.";
	fontname = MENU_FONT_SMALL;
	posx = COOP_LBL_X;
	posy = COOP_Y0 + COOP_DY * 0;
	dimx = COOP_LBL_DX;
	dimy = COOP_ROW_DY;
	onselaction[0] = SEL_ACTION_STARTITEM;
	onselaction_s[0] = "MENUITEM_COOP_IP_INP";
	userString[2] = "Host IP address"; userString[3] = "RETURN to change - remembered for next time.";
};

instance MENUITEM_COOP_IP_INP(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_INPUT;
	text[1] = "IP-Adresse eingeben, RETURN zum Übernehmen.";
	fontname = MENU_FONT_SMALL;
	posx = COOP_VAL_X;
	posy = COOP_Y0 + COOP_DY * 0 + 30;
	dimx = COOP_VAL_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "coopServer";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;   // v55b: wie die Auswahlfelder (Text mittig im Kasten)
	userString[3] = "Enter the IP address, RETURN to accept.";
};

instance MENUITEM_COOP_PORT(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Port";
	text[1] = "RETURN zum Ändern - Vorgabe 9955, bei Host und Mitspieler gleich (gilt auch für Spiel hosten).";
	fontname = MENU_FONT_SMALL;
	posx = COOP_LBL_X;
	posy = COOP_Y0 + COOP_DY * 1;
	dimx = COOP_LBL_DX;
	dimy = COOP_ROW_DY;
	onselaction[0] = SEL_ACTION_STARTITEM;
	onselaction_s[0] = "MENUITEM_COOP_PORT_INP";
	userString[2] = "Port"; userString[3] = "RETURN to change - default 9955, same for host and player (also used for hosting).";
};

instance MENUITEM_COOP_PORT_INP(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_INPUT;
	text[1] = "Port eingeben, RETURN zum Übernehmen.";
	fontname = MENU_FONT_SMALL;
	posx = COOP_VAL_X;
	posy = COOP_Y0 + COOP_DY * 1 + 30;
	dimx = COOP_VAL_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "coopPort";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;   // v55b: wie die Auswahlfelder (Text mittig im Kasten)
	userString[3] = "Enter the port, RETURN to accept.";
};

instance MENUITEM_COOP_NAME(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Name";
	text[1] = "RETURN zum Ändern - so heißt deine Figur beim Mitspieler.";
	fontname = MENU_FONT_SMALL;
	posx = COOP_LBL_X;
	posy = COOP_Y0 + COOP_DY * 1;
	dimx = COOP_LBL_DX;
	dimy = COOP_ROW_DY;
	onselaction[0] = SEL_ACTION_STARTITEM;
	onselaction_s[0] = "MENUITEM_COOP_NAME_INP";
	userString[2] = "Name"; userString[3] = "RETURN to change - your character's name for your partner.";
};

instance MENUITEM_COOP_NAME_INP(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_INPUT;
	text[1] = "Name eingeben, RETURN zum Übernehmen.";
	fontname = MENU_FONT_SMALL;
	posx = COOP_VAL_X;
	posy = COOP_Y0 + COOP_DY * 1 + 30;
	dimx = COOP_VAL_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "coopName";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;   // v55b: wie die Auswahlfelder (Text mittig im Kasten)
	userString[3] = "Enter a name, RETURN to accept.";
};

instance MENUITEM_COOP_LOOKNPC(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Aussehen wie NPC";
	text[1] = "RETURN, Namen eingeben (z. B. Gomez) - Kopf, Gesicht, Körper und Hautton werden von diesem NPC übernommen.";
	fontname = MENU_FONT_SMALL;
	posx = COOP_LBL_X;
	posy = COOP_Y0 + COOP_DY * 2;
	dimx = COOP_LBL_DX;
	dimy = COOP_ROW_DY;
	onselaction[0] = SEL_ACTION_STARTITEM;
	onselaction_s[0] = "MENUITEM_COOP_LOOKNPC_INP";
	userString[2] = "Look like NPC"; userString[3] = "RETURN, enter a name (e.g. Gomez) - head, face, body and skin are taken from that NPC.";
};

instance MENUITEM_COOP_LOOKNPC_INP(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_INPUT;
	text[1] = "Name oder Instanz eines NPCs eingeben, RETURN zum Suchen.";
	fontname = MENU_FONT_SMALL;
	posx = COOP_VAL_X;
	posy = COOP_Y0 + COOP_DY * 2 + 30;
	dimx = COOP_VAL_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "coopLookNpc";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;   // v55b: wie die Auswahlfelder (Text mittig im Kasten)
	userString[3] = "Enter an NPC name or instance, RETURN to search.";
};

// Auswahl: Beschriftung mit IT_EFFECTS_NEXT, danach die nicht wählbare Auswahl (Muster aus Gothics Spiel-Optionen).
// Gothic schreibt die Nummer der gewählten Möglichkeit nach Gothic.ini [GOTHICCOOP].

instance MENUITEM_COOP_HEAD(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Kopf";
	text[1] = "Kopfform deines Helden (links/rechts). Gilt für dich und für deine Figur beim Mitspieler.";
	fontname = MENU_FONT_SMALL;
	posx = COOP_LBL_X;
	posy = COOP_Y0 + COOP_DY * 3;
	dimx = COOP_LBL_DX;
	dimy = COOP_ROW_DY;
	onselaction[0] = SEL_ACTION_UNDEF;
	flags = flags | IT_EFFECTS_NEXT;
	userString[2] = "Head"; userString[3] = "Head shape of your hero (left/right). Applies to you and to your character at your partner.";
};

instance MENUITEM_COOP_HEAD_CHOICE(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_CHOICEBOX;
	text[0] = "Zopf|Zopf 2|Kämpfer|Glatze|Dicke Glatze|Dieb|Psioniker|Flail|Flex|Lutter|Pfeiffer|Pymonte|Thomas|Unicorn";
	fontname = MENU_FONT_SMALL;
	posx = COOP_VAL_X;
	posy = COOP_Y0 + COOP_DY * 3 + 30;
	dimx = COOP_VAL_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "coopHead";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "Ponytail|Ponytail 2|Fighter|Bald|Fat bald|Thief|Psionic|Flail|Flex|Lutter|Pfeiffer|Pymonte|Thomas|Unicorn";
};

instance MENUITEM_COOP_FACE(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Gesicht";
	text[1] = "Gesichts-Textur 0-164 (links/rechts). Nicht jedes Gesicht gibt es in jedem Hautton - die Vorschau zeigt, was genommen wird.";
	fontname = MENU_FONT_SMALL;
	posx = COOP_LBL_X;
	posy = COOP_Y0 + COOP_DY * 4;
	dimx = COOP_LBL_DX;
	dimy = COOP_ROW_DY;
	onselaction[0] = SEL_ACTION_UNDEF;
	flags = flags | IT_EFFECTS_NEXT;
	userString[2] = "Face"; userString[3] = "Face texture 0-164 (left/right). Not every face exists for every skin - the preview shows what is used.";
};

instance MENUITEM_COOP_FACE_CHOICE(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_CHOICEBOX;
	text[0] = "0|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25|26|27|28|29|30|31|32|33|34|35|36|37|38|39|40|41|42|43|44|45|46|47|48|49|50|51|52|53|54|55|56|57|58|59|60|61|62|63|64|65|66|67|68|69|70|71|72|73|74|75|76|77|78|79|80|81|82|83|84|85|86|87|88|89|90|91|92|93|94|95|96|97|98|99|100|101|102|103|104|105|106|107|108|109|110|111|112|113|114|115|116|117|118|119|120|121|122|123|124|125|126|127|128|129|130|131|132|133|134|135|136|137|138|139|140|141|142|143|144|145|146|147|148|149|150|151|152|153|154|155|156|157|158|159|160|161|162|163|164";
	fontname = MENU_FONT_SMALL;
	posx = COOP_VAL_X;
	posy = COOP_Y0 + COOP_DY * 4 + 30;
	dimx = COOP_VAL_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "coopFace";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
};

instance MENUITEM_COOP_BODYMESH(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Statur";
	text[1] = "Körperform (links/rechts). Kräftig gibt es nur mit Körper 0-1 und Hautton 2.";
	fontname = MENU_FONT_SMALL;
	posx = COOP_LBL_X;
	posy = COOP_Y0 + COOP_DY * 5;
	dimx = COOP_LBL_DX;
	dimy = COOP_ROW_DY;
	onselaction[0] = SEL_ACTION_UNDEF;
	flags = flags | IT_EFFECTS_NEXT;
	userString[2] = "Build"; userString[3] = "Body shape (left/right). Strong only with body 0-1 and skin 2.";
};

instance MENUITEM_COOP_BODYMESH_CHOICE(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_CHOICEBOX;
	text[0] = "normal|kräftig";
	fontname = MENU_FONT_SMALL;
	posx = COOP_VAL_X;
	posy = COOP_Y0 + COOP_DY * 5 + 30;
	dimx = COOP_VAL_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "coopBodyMesh";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "normal|strong";
};

instance MENUITEM_COOP_BODY(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Körper";
	text[1] = "Körper-Textur 0-14 (links/rechts). Hauttöne 2-4 gibt es nur für die Körper 0-4, ab Körper 5 nur Hautton 1.";
	fontname = MENU_FONT_SMALL;
	posx = COOP_LBL_X;
	posy = COOP_Y0 + COOP_DY * 6;
	dimx = COOP_LBL_DX;
	dimy = COOP_ROW_DY;
	onselaction[0] = SEL_ACTION_UNDEF;
	flags = flags | IT_EFFECTS_NEXT;
	userString[2] = "Body"; userString[3] = "Body texture 0-14 (left/right). Skins 2-4 only for bodies 0-4, from body 5 only skin 1.";
};

instance MENUITEM_COOP_BODY_CHOICE(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_CHOICEBOX;
	text[0] = "0|1|2|3|4|5|6|7|8|9|10|11|12|13|14";
	fontname = MENU_FONT_SMALL;
	posx = COOP_VAL_X;
	posy = COOP_Y0 + COOP_DY * 6 + 30;
	dimx = COOP_VAL_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "coopBody";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
};

instance MENUITEM_COOP_SKIN(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Hautton";
	text[1] = "Hautton 1-4 (links/rechts). Fehlt die Textur für Gesicht oder Körper, nimmt GothicCoop den nächsten vorhandenen.";
	fontname = MENU_FONT_SMALL;
	posx = COOP_LBL_X;
	posy = COOP_Y0 + COOP_DY * 7;
	dimx = COOP_LBL_DX;
	dimy = COOP_ROW_DY;
	onselaction[0] = SEL_ACTION_UNDEF;
	flags = flags | IT_EFFECTS_NEXT;
	userString[2] = "Skin"; userString[3] = "Skin 1-4 (left/right). If the texture for face or body is missing, GothicCoop takes the nearest one.";
};

instance MENUITEM_COOP_SKIN_CHOICE(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_CHOICEBOX;
	text[0] = "1|2|3|4";
	fontname = MENU_FONT_SMALL;
	posx = COOP_VAL_X;
	posy = COOP_Y0 + COOP_DY * 7 + 30;
	dimx = COOP_VAL_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "coopSkin";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
};

// --- v95: Schritt 2 MENU_COOP (eine Spalte, Knöpfe 1500-6692, Höhe 420 - v94 war Nutzer "leicht zu groß") ---------------

const int COOPHUB_X  = 1500;
const int COOPHUB_DX = 5192;
const int COOPHUB_DY = 420;    // Knopfhöhe

instance MENUITEM_COOP_STEP2(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = "Schritt 2 von 2: verbinden und spielen";
	type = MENU_ITEM_TEXT;
	posx = 400;
	posy = 1080;
	dimx = 7392;
	dimy = 300;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
	userString[2] = "Step 2 of 2: connect and play";
};

instance MENUITEM_COOP_STATUS1(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = "Nicht verbunden.";
	type = MENU_ITEM_TEXT;
	posx = 400;
	posy = 1400;
	dimx = 7392;
	dimy = 300;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
};

instance MENUITEM_COOP_STATUS2(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = " ";
	type = MENU_ITEM_TEXT;
	posx = 400;
	posy = 1700;
	dimx = 7392;
	dimy = 300;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
};

instance MENUITEM_COOP_SECT_CONNECT(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = "- Verbinden -";
	type = MENU_ITEM_TEXT;
	posx = 400;
	posy = 2080;
	dimx = 7392;
	dimy = 280;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
	userString[2] = "- Connect -";
};

instance MENUITEM_COOP_HOST(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Spiel hosten";
	text[1] = "Host starten - Steam-Freunde treten mit einem Klick bei, andere über deine IP-Adresse (Port unter Über IP beitreten).";
	posx = COOPHUB_X;
	posy = 2360;
	dimx = COOPHUB_DX;
	dimy = COOPHUB_DY;
	onselaction[0] = SEL_ACTION_EXECCOMMANDS;
	onselaction_s[0] = "COOP_HOST";
	flags = flags | IT_TXT_CENTER;
	userString[2] = "Host game"; userString[3] = "Start a host - Steam friends join with one click, others via your IP address (port under Join via IP).";
};

// GothicCoop setzt den Text: Freund hostet -> ">> Anna hostet: beitreten" (ein Klick), als Host "Steam: Freunde einladen".
instance MENUITEM_COOP_STEAMJOIN(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Steam ...";
	text[1] = "Einem Steam-Freund beitreten, der gerade hostet - ohne IP-Adresse. Als Host: Freunde über Steam einladen.";
	posx = COOPHUB_X;
	posy = 2830;
	dimx = COOPHUB_DX;
	dimy = COOPHUB_DY;
	onselaction[0] = SEL_ACTION_EXECCOMMANDS;
	onselaction_s[0] = "COOP_STEAM_QUICK";
	flags = flags | IT_TXT_CENTER;
	userString[3] = "Join a Steam friend who is hosting - no IP needed. As host: invite friends via Steam.";
};

instance MENUITEM_COOP_STEAM(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Steam-Freunde ...";
	text[1] = "Alle Steam-Freunde, die Gothic spielen: beitreten, wenn sie hosten - einladen, wenn du hostest.";
	posx = COOPHUB_X;
	posy = 3300;
	dimx = COOPHUB_DX;
	dimy = COOPHUB_DY;
	onselaction[0] = SEL_ACTION_STARTMENU;
	onselaction_s[0] = "MENU_COOP_STEAM";
	flags = flags | IT_TXT_CENTER;
	userString[2] = "Steam friends ..."; userString[3] = "All Steam friends playing Gothic: join if they host - invite if you host.";
};

instance MENUITEM_COOP_IPJOIN(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Über IP beitreten ...";
	text[1] = "IP-Adresse und Port des Hosts eingeben und verbinden (ohne Steam).";
	posx = COOPHUB_X;
	posy = 3770;
	dimx = COOPHUB_DX;
	dimy = COOPHUB_DY;
	onselaction[0] = SEL_ACTION_STARTMENU;
	onselaction_s[0] = "MENU_COOP_IP";
	flags = flags | IT_TXT_CENTER;
	userString[2] = "Join via IP ..."; userString[3] = "Enter the host's IP address and port and connect (without Steam).";
};

instance MENUITEM_COOP_SECT_PLAY(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = "- Spielen -";
	type = MENU_ITEM_TEXT;
	posx = 400;
	posy = 4330;
	dimx = 7392;
	dimy = 280;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
	userString[2] = "- Play -";
};

// GothicCoop setzt den Text je nach Rolle ("Neues Spiel (gemeinsam)", beim Mitspieler "Neues Spiel: startet der Host").
instance MENUITEM_COOP_NEWGAME(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Neues Spiel";
	text[1] = "Neues Abenteuer (Othellos Auswahl). Als Host mit Mitspieler starten beide - danach die COOP-Schwierigkeit.";
	posx = COOPHUB_X;
	posy = 4610;
	dimx = COOPHUB_DX;
	dimy = COOPHUB_DY;
	onselaction[0] = SEL_ACTION_STARTMENU;
	onselaction_s[0] = "MENU_NG";
	flags = flags | IT_TXT_CENTER;
	userString[3] = "New adventure (Othello's choice). As host with a player both start - then the COOP difficulty.";
};

instance MENUITEM_COOP_LOAD(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Spielstand wählen";
	text[1] = "Gemeinsamen Spielstand wählen - der Host wählt, beide laden ihn, jeder mit seinem Helden.";
	posx = COOPHUB_X;
	posy = 5080;
	dimx = COOPHUB_DX;
	dimy = COOPHUB_DY;
	onselaction[0] = SEL_ACTION_STARTMENU;
	onselaction_s[0] = "MENU_COOP_LOAD";
	flags = flags | IT_TXT_CENTER;
	userString[3] = "Choose a shared save - the host chooses, both load it, each with their own hero.";
};

instance MENUITEM_COOP_SETTINGS(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "COOP-Einstellungen ...";
	text[1] = "Kamera, Schwierigkeitsgrad, Abgleich, Steam und Sprache - der Host stellt vor dem Start ein.";
	posx = COOPHUB_X;
	posy = 5640;
	dimx = COOPHUB_DX;
	dimy = COOPHUB_DY;
	onselaction[0] = SEL_ACTION_STARTMENU;
	onselaction_s[0] = "MENU_OPT_COOP";
	flags = flags | IT_TXT_CENTER;
	userString[2] = "COOP settings ..."; userString[3] = "Camera, difficulty, sync, Steam and language - the host sets them before starting.";
};

instance MENUITEM_COOP_STOP(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Verbindung trennen";
	text[1] = "Host beenden bzw. die Verbindung zum Host trennen - danach wieder wie am Anfang.";
	posx = COOPHUB_X;
	posy = 6110;
	dimx = COOPHUB_DX;
	dimy = COOPHUB_DY;
	onselaction[0] = SEL_ACTION_EXECCOMMANDS;
	onselaction_s[0] = "COOP_STOP";
	flags = flags | IT_TXT_CENTER;
	userString[2] = "Disconnect"; userString[3] = "Stop the host or disconnect from the host - then back to the start.";
};

instance MENUITEM_COOP_BACK(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "ZURÜCK";
	text[1] = "Zurück zu deiner Figur (Schritt 1).";
	posx = COOPHUB_X;
	posy = 6580;
	dimx = COOPHUB_DX;
	dimy = COOPHUB_DY;
	onselaction[0] = SEL_ACTION_BACK;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "BACK"; userString[3] = "Back to your character (step 1).";
};

// --- v94: Über IP beitreten -------------------------------------------------------------------------------------------

instance MENU_COOP_IP(C_MENU_DEF)
{
	backpic = MENU_BACK_PIC;
	items[0] = "MENUITEM_COOPIP_HEADLINE";
	items[1] = "MENUITEM_COOP_IP";
	items[2] = "MENUITEM_COOP_IP_INP";
	items[3] = "MENUITEM_COOP_PORT";
	items[4] = "MENUITEM_COOP_PORT_INP";
	items[5] = "MENUITEM_COOPIP_HINT1";
	items[6] = "MENUITEM_COOPIP_HINT2";
	items[7] = "MENUITEM_COOP_CLIENT";
	items[8] = "MENUITEM_COOPIP_STATUS1";
	items[9] = "MENUITEM_COOPIP_STATUS2";
	items[10] = "MENUITEM_COOPIP_STOP";
	items[11] = "MENUITEM_COOPIP_BACK";
	items[12] = "MENUITEM_COOPIP_VERSION";
	defaultoutgame = 7;
	defaultingame = 7;
	flags = flags | MENU_SHOW_INFO;
};

instance MENUITEM_COOPIP_HEADLINE(C_MENU_ITEM_DEF)
{
	text[0] = "ÜBER IP";
	type = MENU_ITEM_TEXT;
	posx = 0;
	posy = MENU_TITLE_Y - 500;
	dimx = 8192;
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "VIA IP";
};

instance MENUITEM_COOPIP_HINT1(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = "Der Host nennt dir seine IP-Adresse - über das Internet muss er den Port im Router freigeben.";
	type = MENU_ITEM_TEXT;
	posx = 400;
	posy = 2450;
	dimx = 7392;
	dimy = 300;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
	userString[2] = "The host tells you their IP address - over the internet they must open the port in the router.";
};

instance MENUITEM_COOPIP_HINT2(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = "Steam-Freunde brauchen das nicht (COOP > Steam-Knopf). Der Port gilt auch für Spiel hosten.";
	type = MENU_ITEM_TEXT;
	posx = 400;
	posy = 2770;
	dimx = 7392;
	dimy = 300;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
	userString[2] = "Steam friends do not need this (COOP > Steam button). The port is also used for hosting.";
};

instance MENUITEM_COOP_CLIENT(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Verbinden";
	text[1] = "Mit dem Host unter dieser IP-Adresse verbinden - danach wählt er den Spielstand oder startet ein neues Spiel.";
	posx = COOPHUB_X;
	posy = 3300;
	dimx = COOPHUB_DX;
	dimy = COOPHUB_DY;
	onselaction[0] = SEL_ACTION_EXECCOMMANDS;
	onselaction_s[0] = "COOP_CLIENT";
	flags = flags | IT_TXT_CENTER;
	userString[2] = "Connect"; userString[3] = "Connect to the host at this IP address - then they choose the save or start a new game.";
};

instance MENUITEM_COOPIP_STATUS1(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = "Nicht verbunden.";
	type = MENU_ITEM_TEXT;
	posx = 400;
	posy = 4000;
	dimx = 7392;
	dimy = 330;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
};

instance MENUITEM_COOPIP_STATUS2(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = " ";
	type = MENU_ITEM_TEXT;
	posx = 400;
	posy = 4330;
	dimx = 7392;
	dimy = 330;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
};

instance MENUITEM_COOPIP_STOP(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Verbindung trennen";
	text[1] = "Die Verbindung zum Host trennen bzw. den Host beenden.";
	posx = COOPHUB_X;
	posy = 4900;
	dimx = COOPHUB_DX;
	dimy = COOPHUB_DY;
	onselaction[0] = SEL_ACTION_EXECCOMMANDS;
	onselaction_s[0] = "COOP_STOP";
	flags = flags | IT_TXT_CENTER;
	userString[2] = "Disconnect"; userString[3] = "Disconnect from the host or stop the host.";
};

instance MENUITEM_COOPIP_BACK(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "ZURÜCK";
	posx = COOPHUB_X;
	posy = 5420;
	dimx = COOPHUB_DX;
	dimy = COOPHUB_DY;
	onselaction[0] = SEL_ACTION_BACK;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "BACK";
};

// --- v95: Schritt 1 MENU_COOP_LOOK - Sprache, Name, Aussehen, 3D-Vorschau (MENUITEM_COOP_PREVIEW), Weiter -------------------

instance MENU_COOP_LOOK(C_MENU_DEF)
{
	backpic = MENU_BACK_PIC;
	items[0] = "MENUITEM_COOPLOOK_HEADLINE";
	items[1] = "MENUITEM_COOPLOOK_STEP";
	items[2] = "MENUITEM_COOPLOOK_LANG";
	items[3] = "MENUITEM_COOPLOOK_LANG_CHOICE";
	items[4] = "MENUITEM_COOP_NAME";
	items[5] = "MENUITEM_COOP_NAME_INP";
	items[6] = "MENUITEM_COOP_LOOKNPC";
	items[7] = "MENUITEM_COOP_LOOKNPC_INP";
	items[8] = "MENUITEM_COOP_HEAD";
	items[9] = "MENUITEM_COOP_HEAD_CHOICE";
	items[10] = "MENUITEM_COOP_FACE";
	items[11] = "MENUITEM_COOP_FACE_CHOICE";
	items[12] = "MENUITEM_COOP_BODYMESH";
	items[13] = "MENUITEM_COOP_BODYMESH_CHOICE";
	items[14] = "MENUITEM_COOP_BODY";
	items[15] = "MENUITEM_COOP_BODY_CHOICE";
	items[16] = "MENUITEM_COOP_SKIN";
	items[17] = "MENUITEM_COOP_SKIN_CHOICE";
	items[18] = "MENUITEM_COOPLOOK_NOTE";
	items[19] = "MENUITEM_COOPLOOK_NEXT";
	items[20] = "MENUITEM_COOPLOOK_BACK";
	items[21] = "MENUITEM_COOPLOOK_VERSION";
	items[22] = "MENUITEM_COOP_PREVIEW";
	defaultoutgame = 19;
	defaultingame = 19;
	flags = flags | MENU_SHOW_INFO;
};

instance MENUITEM_COOPLOOK_HEADLINE(C_MENU_ITEM_DEF)
{
	text[0] = "DEINE FIGUR";
	type = MENU_ITEM_TEXT;
	posx = 0;
	posy = MENU_TITLE_Y - 500;
	dimx = 8192;
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "YOUR CHARACTER";
};

instance MENUITEM_COOPLOOK_STEP(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = "COOP - Schritt 1 von 2";
	type = MENU_ITEM_TEXT;
	posx = 400;
	posy = 1080;
	dimx = 7392;
	dimy = 300;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
	userString[2] = "COOP - step 1 of 2";
};

instance MENUITEM_COOPLOOK_LANG(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Sprache / Language";
	text[1] = "Sprache der Coop-Menüs und -Meldungen (links/rechts): Deutsch oder English.";
	fontname = MENU_FONT_SMALL;
	posx = COOP_LBL_X;
	posy = COOP_Y0 + COOP_DY * 0;
	dimx = COOP_LBL_DX;
	dimy = COOP_ROW_DY;
	onselaction[0] = SEL_ACTION_UNDEF;
	flags = flags | IT_EFFECTS_NEXT;
	userString[2] = "Sprache / Language"; userString[3] = "Language of the coop menus and messages (left/right): Deutsch or English.";
};

instance MENUITEM_COOPLOOK_LANG_CHOICE(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_CHOICEBOX;
	text[0] = "Deutsch|English";
	fontname = MENU_FONT_SMALL;
	posx = COOP_VAL_X;
	posy = COOP_Y0 + COOP_DY * 0 + 30;
	dimx = COOP_VAL_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "coopLang";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
};

// GothicCoop zeigt hier das Ergebnis der NPC-Suche.
instance MENUITEM_COOPLOOK_NOTE(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = " ";
	type = MENU_ITEM_TEXT;
	posx = 400;
	posy = 4900;
	dimx = 7392;
	dimy = 330;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
};

instance MENUITEM_COOPLOOK_NEXT(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Weiter: Verbinden >>";
	text[1] = "Zu Schritt 2: hosten, beitreten, neues Spiel oder Spielstand wählen.";
	posx = COOPHUB_X;
	posy = 5450;
	dimx = COOPHUB_DX;
	dimy = COOPHUB_DY;
	onselaction[0] = SEL_ACTION_STARTMENU;
	onselaction_s[0] = "MENU_COOP";
	flags = flags | IT_TXT_CENTER;
	userString[2] = "Next: connect >>"; userString[3] = "To step 2: host, join, new game or choose a save.";
};

instance MENUITEM_COOPLOOK_BACK(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "ZURÜCK";
	text[1] = "Zurück zum Hauptmenü.";
	posx = COOPHUB_X;
	posy = 5920;
	dimx = COOPHUB_DX;
	dimy = COOPHUB_DY;
	onselaction[0] = SEL_ACTION_BACK;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "BACK"; userString[3] = "Back to the main menu.";
};

// --- Spielstand wählen (GothicCoop füllt die Zeilen; v93: aus dem Coop-Archiv, blättern) --------------------------------------------------------------------

instance MENU_COOP_LOAD(C_MENU_DEF)
{
	backpic = MENU_BACK_PIC;
	items[0] = "MENUITEM_COOPLOAD_HEADLINE";
	items[1] = "MENUITEM_COOPLOAD_ROW0";
	items[2] = "MENUITEM_COOPLOAD_ROW1";
	items[3] = "MENUITEM_COOPLOAD_ROW2";
	items[4] = "MENUITEM_COOPLOAD_ROW3";
	items[5] = "MENUITEM_COOPLOAD_ROW4";
	items[6] = "MENUITEM_COOPLOAD_ROW5";
	items[7] = "MENUITEM_COOPLOAD_ROW6";
	items[8] = "MENUITEM_COOPLOAD_ROW7";
	items[9] = "MENUITEM_COOPLOAD_ROW8";
	items[10] = "MENUITEM_COOPLOAD_ROW9";
	items[11] = "MENUITEM_COOPLOAD_OTHER";
	items[12] = "MENUITEM_COOPLOAD_FILTER";
	items[13] = "MENUITEM_COOPLOAD_STATUS";
	items[14] = "MENUITEM_COOPLOAD_BACK";
	items[15] = "MENUITEM_COOPLOAD_VERSION";
	items[16] = "MENUITEM_COOPLOAD_PIC_HOST";
	items[17] = "MENUITEM_COOPLOAD_PIC_PARTNER";
	items[18] = "MENUITEM_COOPLOAD_NAME_HOST";
	items[19] = "MENUITEM_COOPLOAD_NAME_PARTNER";
	items[20] = "MENUITEM_COOPLOAD_INFO0";
	items[21] = "MENUITEM_COOPLOAD_INFO1";
	items[22] = "MENUITEM_COOPLOAD_INFO2";
	items[23] = "MENUITEM_COOPLOAD_INFO3";
	items[24] = "MENUITEM_COOPLOAD_INFO4";
	items[25] = "MENUITEM_COOPLOAD_INFO5";
	items[26] = "MENUITEM_COOPLOAD_INFO6";
	items[27] = "MENUITEM_COOPLOAD_INFO7";
	items[28] = "MENUITEM_COOPLOAD_SCROLL";
	items[29] = "MENUITEM_COOPLOAD_THUMB";
	defaultoutgame = 1;
	defaultingame = 1;
	flags = flags | MENU_SHOW_INFO;
};

instance MENUITEM_COOPLOAD_HEADLINE(C_MENU_ITEM_DEF)
{
	text[0] = "Spielstand wählen";
	type = MENU_ITEM_TEXT;
	posx = 0;
	posy = MENU_TITLE_Y - 500;
	dimx = 8192;
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
};

const int COOPLOAD_Y0 = 1450;
const int COOPLOAD_DY = 430;
const int COOPLOAD_ROW_DX = 3300;   // v77: Liste 1000-4300, rechts die Vorschau

instance MENUITEM_COOPLOAD_ROW0(C_MENU_ITEM_DEF) { backpic = MENU_ITEM_BACK_PIC; text[0] = " "; text[1] = "Diesen gemeinsamen Spielstand laden."; fontname = MENU_FONT_SMALL; posx = 1000; posy = COOPLOAD_Y0 + COOPLOAD_DY * 0; dimx = COOPLOAD_ROW_DX; dimy = 400; onselaction[0] = SEL_ACTION_EXECCOMMANDS; onselaction_s[0] = "COOP_ROW_0"; userString[3] = "Load this shared save."; };
instance MENUITEM_COOPLOAD_ROW1(C_MENU_ITEM_DEF) { backpic = MENU_ITEM_BACK_PIC; text[0] = " "; text[1] = "Diesen gemeinsamen Spielstand laden."; fontname = MENU_FONT_SMALL; posx = 1000; posy = COOPLOAD_Y0 + COOPLOAD_DY * 1; dimx = COOPLOAD_ROW_DX; dimy = 400; onselaction[0] = SEL_ACTION_EXECCOMMANDS; onselaction_s[0] = "COOP_ROW_1"; userString[3] = "Load this shared save."; };
instance MENUITEM_COOPLOAD_ROW2(C_MENU_ITEM_DEF) { backpic = MENU_ITEM_BACK_PIC; text[0] = " "; text[1] = "Diesen gemeinsamen Spielstand laden."; fontname = MENU_FONT_SMALL; posx = 1000; posy = COOPLOAD_Y0 + COOPLOAD_DY * 2; dimx = COOPLOAD_ROW_DX; dimy = 400; onselaction[0] = SEL_ACTION_EXECCOMMANDS; onselaction_s[0] = "COOP_ROW_2"; userString[3] = "Load this shared save."; };
instance MENUITEM_COOPLOAD_ROW3(C_MENU_ITEM_DEF) { backpic = MENU_ITEM_BACK_PIC; text[0] = " "; text[1] = "Diesen gemeinsamen Spielstand laden."; fontname = MENU_FONT_SMALL; posx = 1000; posy = COOPLOAD_Y0 + COOPLOAD_DY * 3; dimx = COOPLOAD_ROW_DX; dimy = 400; onselaction[0] = SEL_ACTION_EXECCOMMANDS; onselaction_s[0] = "COOP_ROW_3"; userString[3] = "Load this shared save."; };
instance MENUITEM_COOPLOAD_ROW4(C_MENU_ITEM_DEF) { backpic = MENU_ITEM_BACK_PIC; text[0] = " "; text[1] = "Diesen gemeinsamen Spielstand laden."; fontname = MENU_FONT_SMALL; posx = 1000; posy = COOPLOAD_Y0 + COOPLOAD_DY * 4; dimx = COOPLOAD_ROW_DX; dimy = 400; onselaction[0] = SEL_ACTION_EXECCOMMANDS; onselaction_s[0] = "COOP_ROW_4"; userString[3] = "Load this shared save."; };
instance MENUITEM_COOPLOAD_ROW5(C_MENU_ITEM_DEF) { backpic = MENU_ITEM_BACK_PIC; text[0] = " "; text[1] = "Diesen gemeinsamen Spielstand laden."; fontname = MENU_FONT_SMALL; posx = 1000; posy = COOPLOAD_Y0 + COOPLOAD_DY * 5; dimx = COOPLOAD_ROW_DX; dimy = 400; onselaction[0] = SEL_ACTION_EXECCOMMANDS; onselaction_s[0] = "COOP_ROW_5"; userString[3] = "Load this shared save."; };
instance MENUITEM_COOPLOAD_ROW6(C_MENU_ITEM_DEF) { backpic = MENU_ITEM_BACK_PIC; text[0] = " "; text[1] = "Diesen gemeinsamen Spielstand laden."; fontname = MENU_FONT_SMALL; posx = 1000; posy = COOPLOAD_Y0 + COOPLOAD_DY * 6; dimx = COOPLOAD_ROW_DX; dimy = 400; onselaction[0] = SEL_ACTION_EXECCOMMANDS; onselaction_s[0] = "COOP_ROW_6"; userString[3] = "Load this shared save."; };
instance MENUITEM_COOPLOAD_ROW7(C_MENU_ITEM_DEF) { backpic = MENU_ITEM_BACK_PIC; text[0] = " "; text[1] = "Diesen gemeinsamen Spielstand laden."; fontname = MENU_FONT_SMALL; posx = 1000; posy = COOPLOAD_Y0 + COOPLOAD_DY * 7; dimx = COOPLOAD_ROW_DX; dimy = 400; onselaction[0] = SEL_ACTION_EXECCOMMANDS; onselaction_s[0] = "COOP_ROW_7"; userString[3] = "Load this shared save."; };
instance MENUITEM_COOPLOAD_ROW8(C_MENU_ITEM_DEF) { backpic = MENU_ITEM_BACK_PIC; text[0] = " "; text[1] = "Diesen gemeinsamen Spielstand laden."; fontname = MENU_FONT_SMALL; posx = 1000; posy = COOPLOAD_Y0 + COOPLOAD_DY * 8; dimx = COOPLOAD_ROW_DX; dimy = 400; onselaction[0] = SEL_ACTION_EXECCOMMANDS; onselaction_s[0] = "COOP_ROW_8"; userString[3] = "Load this shared save."; };
instance MENUITEM_COOPLOAD_ROW9(C_MENU_ITEM_DEF) { backpic = MENU_ITEM_BACK_PIC; text[0] = " "; text[1] = "Diesen gemeinsamen Spielstand laden."; fontname = MENU_FONT_SMALL; posx = 1000; posy = COOPLOAD_Y0 + COOPLOAD_DY * 9; dimx = COOPLOAD_ROW_DX; dimy = 400; onselaction[0] = SEL_ACTION_EXECCOMMANDS; onselaction_s[0] = "COOP_ROW_9"; userString[3] = "Load this shared save."; };

instance MENUITEM_COOPLOAD_OTHER(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	fontname = MENU_FONT_SMALL;   // v93: links, rechts der Schalter "Lokale Stände"
	text[0] = "Anderen Spielstand laden ...";
	text[1] = "Einen beliebigen Spielstand laden - der Mitspieler nimmt seinen Helden, der zeitlich am nächsten gespeichert wurde.";
	posx = 1000;
	posy = COOPLOAD_Y0 + COOPLOAD_DY * 10 + 150;
	dimx = COOPLOAD_ROW_DX;
	dimy = 400;
	onselaction[0] = SEL_ACTION_STARTMENU;
	onselaction_s[0] = "MENU_SAVEGAME_LOAD";
	flags = flags | IT_TXT_CENTER;
	userString[2] = "Load another save ..."; userString[3] = "Load any save - your partner takes the hero saved closest in time.";
};

instance MENUITEM_COOPLOAD_STATUS(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = " ";
	type = MENU_ITEM_TEXT;
	posx = 0;
	posy = COOPLOAD_Y0 + COOPLOAD_DY * 11 + 300;
	dimx = 8192;
	dimy = 350;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
};

instance MENUITEM_COOPLOAD_BACK(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "ZURÜCK";
	posx = 1000;
	posy = COOPLOAD_Y0 + COOPLOAD_DY * 12 + 400;
	dimx = 6192;
	dimy = 600;
	onselaction[0] = SEL_ACTION_BACK;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "BACK";
};

// v77: Vorschau rechts (GothicCoop setzt Bild und Text). Bild quadratisch in Menü-Einheiten = 16:9 auf einem 16:9-Bild
// (wie Gothics MENUITEM_LOADSAVE_THUMBPIC: backpic leer, nicht wählbar).
const int COOPPV_X1 = 4450;   // Host
const int COOPPV_X2 = 6100;   // Mitspieler
const int COOPPV_DX = 1500;
const int COOPPV_Y0 = 1450;   // Bild
const int COOPPV_YN = 3000;   // Name
const int COOPPV_YI = 3330;   // erste Zeile
const int COOPPV_DY = 290;
const int COOPPV_W  = 3150;   // v78: Zeilen über beide Bilder (4450-7600)

instance MENUITEM_COOPLOAD_PIC_HOST(C_MENU_ITEM_DEF) { backpic = ""; text[0] = " "; type = MENU_ITEM_TEXT; fontname = MENU_FONT_SMALL; posx = COOPPV_X1; posy = COOPPV_Y0; dimx = COOPPV_DX; dimy = COOPPV_DX; flags = flags & ~IT_SELECTABLE; flags = flags | IT_TXT_CENTER; };
instance MENUITEM_COOPLOAD_PIC_PARTNER(C_MENU_ITEM_DEF) { backpic = ""; text[0] = " "; type = MENU_ITEM_TEXT; fontname = MENU_FONT_SMALL; posx = COOPPV_X2; posy = COOPPV_Y0; dimx = COOPPV_DX; dimy = COOPPV_DX; flags = flags & ~IT_SELECTABLE; flags = flags | IT_TXT_CENTER; };
instance MENUITEM_COOPLOAD_NAME_HOST(C_MENU_ITEM_DEF) { text[0] = " "; type = MENU_ITEM_TEXT; fontname = MENU_FONT_SMALL; posx = COOPPV_X1; posy = COOPPV_YN; dimx = COOPPV_DX; dimy = 300; flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER; };
instance MENUITEM_COOPLOAD_NAME_PARTNER(C_MENU_ITEM_DEF) { text[0] = " "; type = MENU_ITEM_TEXT; fontname = MENU_FONT_SMALL; posx = COOPPV_X2; posy = COOPPV_YN; dimx = COOPPV_DX; dimy = 300; flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER; };
instance MENUITEM_COOPLOAD_INFO0(C_MENU_ITEM_DEF) { text[0] = " "; type = MENU_ITEM_TEXT; fontname = MENU_FONT_SMALL; posx = COOPPV_X1; posy = COOPPV_YI + COOPPV_DY * 0; dimx = COOPPV_W; dimy = 280; flags = IT_CHROMAKEYED | IT_TRANSPARENT; };
instance MENUITEM_COOPLOAD_INFO1(C_MENU_ITEM_DEF) { text[0] = " "; type = MENU_ITEM_TEXT; fontname = MENU_FONT_SMALL; posx = COOPPV_X1; posy = COOPPV_YI + COOPPV_DY * 1; dimx = COOPPV_W; dimy = 280; flags = IT_CHROMAKEYED | IT_TRANSPARENT; };
instance MENUITEM_COOPLOAD_INFO2(C_MENU_ITEM_DEF) { text[0] = " "; type = MENU_ITEM_TEXT; fontname = MENU_FONT_SMALL; posx = COOPPV_X1; posy = COOPPV_YI + COOPPV_DY * 2; dimx = COOPPV_W; dimy = 280; flags = IT_CHROMAKEYED | IT_TRANSPARENT; };
instance MENUITEM_COOPLOAD_INFO3(C_MENU_ITEM_DEF) { text[0] = " "; type = MENU_ITEM_TEXT; fontname = MENU_FONT_SMALL; posx = COOPPV_X1; posy = COOPPV_YI + COOPPV_DY * 3; dimx = COOPPV_W; dimy = 280; flags = IT_CHROMAKEYED | IT_TRANSPARENT; };
instance MENUITEM_COOPLOAD_INFO4(C_MENU_ITEM_DEF) { text[0] = " "; type = MENU_ITEM_TEXT; fontname = MENU_FONT_SMALL; posx = COOPPV_X1; posy = COOPPV_YI + COOPPV_DY * 4; dimx = COOPPV_W; dimy = 280; flags = IT_CHROMAKEYED | IT_TRANSPARENT; };
instance MENUITEM_COOPLOAD_INFO5(C_MENU_ITEM_DEF) { text[0] = " "; type = MENU_ITEM_TEXT; fontname = MENU_FONT_SMALL; posx = COOPPV_X1; posy = COOPPV_YI + COOPPV_DY * 5; dimx = COOPPV_W; dimy = 280; flags = IT_CHROMAKEYED | IT_TRANSPARENT; };
instance MENUITEM_COOPLOAD_INFO6(C_MENU_ITEM_DEF) { text[0] = " "; type = MENU_ITEM_TEXT; fontname = MENU_FONT_SMALL; posx = COOPPV_X1; posy = COOPPV_YI + COOPPV_DY * 6; dimx = COOPPV_W; dimy = 280; flags = IT_CHROMAKEYED | IT_TRANSPARENT; };
instance MENUITEM_COOPLOAD_INFO7(C_MENU_ITEM_DEF) { text[0] = " "; type = MENU_ITEM_TEXT; fontname = MENU_FONT_SMALL; posx = COOPPV_X1; posy = COOPPV_YI + COOPPV_DY * 7; dimx = COOPPV_W; dimy = 280; flags = IT_CHROMAKEYED | IT_TRANSPARENT; };

// v93: Schalter "Lokale Stände" (COOP_LIST_LOCAL): zusätzlich die eigenen Stände des Durchlaufs, die der Mitspieler nicht
//      hat (GothicCoop setzt den Text "Lokale Stände: an/aus").
instance MENUITEM_COOPLOAD_FILTER(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Lokale Stände: aus";
	text[1] = "Auch eigene Stände dieses Durchlaufs zeigen, die der Mitspieler nicht hat (z. B. allein gespeichert).";
	fontname = MENU_FONT_SMALL;
	posx = COOPPV_X1;
	posy = COOPLOAD_Y0 + COOPLOAD_DY * 10 + 150;
	dimx = COOPPV_W;
	dimy = 400;
	onselaction[0] = SEL_ACTION_EXECCOMMANDS;
	onselaction_s[0] = "COOP_LIST_LOCAL";
	flags = flags | IT_TXT_CENTER;
	userString[3] = "Also show own saves of this playthrough that your partner does not have (e.g. saved alone).";
};

// v94: Bildlaufleiste (Nutzer: "verdeutlichen, wo genau wir uns befinden ... vielleicht mit einer Scrollbar"): Schiene
//      zwischen Liste (bis 4300) und Vorschau (ab 4450), GothicCoop setzt Lage und Höhe des Griffs (Anteil der sichtbaren
//      Zeilen). Texturen aus Gothics textures.VDF. Nicht wählbar.
instance MENUITEM_COOPLOAD_SCROLL(C_MENU_ITEM_DEF) { backpic = "menu_choice_back.tga"; text[0] = " "; type = MENU_ITEM_TEXT; fontname = MENU_FONT_SMALL; posx = 4320; posy = COOPLOAD_Y0; dimx = 100; dimy = COOPLOAD_DY * 9 + 400; flags = flags & ~IT_SELECTABLE; };
instance MENUITEM_COOPLOAD_THUMB(C_MENU_ITEM_DEF) { backpic = "menu_slider_pos.tga"; text[0] = " "; type = MENU_ITEM_TEXT; fontname = MENU_FONT_SMALL; posx = 4320; posy = COOPLOAD_Y0; dimx = 100; dimy = COOPLOAD_DY * 9 + 400; flags = flags & ~IT_SELECTABLE; };

// --- Einstellungen: nur unser Eintrag COOP - GothicCoop (ab v81) setzt ihn zur Laufzeit an die Stelle von MENUITEM_PERF ---

instance MENUITEM_OPT_COOP(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "COOP";
	text[1] = "Coop-Kamera, Schwierigkeitsgrad, Abgleich (Quests, Tagebuch, Gegenstände, Spielstände), Steam, Sprache.";
	posx = 0;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 6;
	dimx = 8192;
	dimy = 750;
	onselaction[0] = SEL_ACTION_STARTMENU;
	onselaction_s[0] = "MENU_OPT_COOP";
	flags = flags | IT_TXT_CENTER;
	userString[2] = "COOP"; userString[3] = "Coop camera, difficulty, sync (quests, diary, items, saves), Steam, language.";
};

instance MENU_OPT_COOP(C_MENU_DEF)
{
	backpic = MENU_BACK_PIC;
	items[0] = "MENUITEM_OPTCOOP_HEADLINE";
	items[1] = "MENUITEM_OPTCOOP_CAM";
	items[2] = "MENUITEM_OPTCOOP_DIFF";
	items[3] = "MENUITEM_OPTCOOP_SYNC";
	items[4] = "MENUITEM_OPTCOOP_STEAM";
	items[5] = "MENUITEM_OPTCOOP_LANG";
	items[6] = "MENUITEM_OPTCOOP_LANG_CHOICE";
	items[7] = "MENUITEM_OPTCOOP_BACK";
	items[8] = "MENUITEM_OPTCOOP_VERSION";
	defaultoutgame = 1;
	defaultingame = 1;
	flags = flags | MENU_SHOW_INFO;
};

instance MENUITEM_OPTCOOP_HEADLINE(C_MENU_ITEM_DEF)
{
	text[0] = "COOP";
	type = MENU_ITEM_TEXT;
	posx = 0;
	posy = MENU_TITLE_Y - 400;
	dimx = 8192;
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "COOP";
};

instance MENUITEM_OPTCOOP_CAM(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Coop-Kamera";
	text[1] = "Zuschau-Fenster: an/aus, Größe, Lage, Taste und wann es von selbst aufgeht.";
	posx = 0;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 0;
	dimx = 8192;
	dimy = 750;
	onselaction[0] = SEL_ACTION_STARTMENU;
	onselaction_s[0] = "MENU_OPT_COOP_CAM";
	flags = flags | IT_TXT_CENTER;
	userString[2] = "Coop camera"; userString[3] = "Watch window: on/off, size, position, key and when it opens by itself.";
};

instance MENUITEM_OPTCOOP_DIFF(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Schwierigkeitsgrad";
	text[1] = "Stufe, Schaden der Spieler und Gegner, Leben der Gegner, EP - der Host gibt vor.";
	posx = 0;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 1;
	dimx = 8192;
	dimy = 750;
	onselaction[0] = SEL_ACTION_STARTMENU;
	onselaction_s[0] = "MENU_OPT_COOP_DIFF";
	flags = flags | IT_TXT_CENTER;
	userString[2] = "Difficulty"; userString[3] = "Level, damage of players and enemies, enemy health, XP - set by the host.";
};

instance MENUITEM_OPTCOOP_SYNC(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Abgleich";
	text[1] = "Was beide teilen: Quests, Tagebuch, Gegenstände und Truhen, Spielstände - der Host gibt vor.";
	posx = 0;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 2;
	dimx = 8192;
	dimy = 750;
	onselaction[0] = SEL_ACTION_STARTMENU;
	onselaction_s[0] = "MENU_OPT_COOP_SYNC";
	flags = flags | IT_TXT_CENTER;
	userString[2] = "Sync"; userString[3] = "What both share: quests, diary, items and chests, saves - set by the host.";
};

instance MENUITEM_OPTCOOP_STEAM(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Steam-Freunde";
	text[1] = "Coop über Steam: Freunden beitreten, die hosten, Freunde einladen, hosten oder trennen - auch im Spiel.";
	posx = 0;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 3;
	dimx = 8192;
	dimy = 750;
	onselaction[0] = SEL_ACTION_STARTMENU;
	onselaction_s[0] = "MENU_COOP_STEAM";
	flags = flags | IT_TXT_CENTER;
	userString[2] = "Steam friends"; userString[3] = "Coop via Steam: join friends who host, invite friends, host or disconnect - also in game.";
};

// v95: Sprache der Coop-Menüs und -Meldungen (Gothic.ini coopLang) - auch im Spiel umstellbar
instance MENUITEM_OPTCOOP_LANG(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Sprache / Language";
	text[1] = "Sprache der Coop-Menüs und -Meldungen (links/rechts): Deutsch oder English.";
	fontname = MENU_FONT_SMALL;
	posx = 1000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 4 + 250;
	dimx = 3800;
	dimy = 400;
	onselaction[0] = SEL_ACTION_UNDEF;
	flags = flags | IT_EFFECTS_NEXT;
	userString[2] = "Sprache / Language"; userString[3] = "Language of the coop menus and messages (left/right): Deutsch or English.";
};

instance MENUITEM_OPTCOOP_LANG_CHOICE(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_CHOICEBOX;
	text[0] = "Deutsch|English";
	fontname = MENU_FONT_SMALL;
	posx = 5000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 4 + 250 + 25;
	dimx = MENU_SLIDER_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "coopLang";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
	
};

instance MENUITEM_OPTCOOP_BACK(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "ZURÜCK";
	posx = 1000;
	posy = MENU_BACK_Y;
	dimx = 6192;
	dimy = MENU_OPT_DY;
	onselaction[0] = SEL_ACTION_BACK;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "BACK";
};

// Coop-Kamera (v45/v48, v66: Modus mit "aus", Breite 15-50 %, Lage, Taste, Automatik-Abstand) - Gothic.ini [GOTHICCOOP]
// camWindowMode 0-2, camWindowWidth 0-7, camWindowPos 0-3, camWindowKey 0-8, camWindowDist 0-5 (GothicCoop belegt sie vor).
instance MENU_OPT_COOP_CAM(C_MENU_DEF)
{
	backpic = MENU_BACK_PIC;
	items[0] = "MENUITEM_OPTCOOPCAM_HEADLINE";
	items[1] = "MENUITEM_COOP_CAMMODE";
	items[2] = "MENUITEM_COOP_CAMMODE_CHOICE";
	items[3] = "MENUITEM_COOP_WINDOW";
	items[4] = "MENUITEM_COOP_WINDOW_CHOICE";
	items[5] = "MENUITEM_COOP_WINPOS";
	items[6] = "MENUITEM_COOP_WINPOS_CHOICE";
	items[7] = "MENUITEM_COOP_CAMKEY";
	items[8] = "MENUITEM_COOP_CAMKEY_CHOICE";
	items[9] = "MENUITEM_COOP_CAMDIST";
	items[10] = "MENUITEM_COOP_CAMDIST_CHOICE";
	items[11] = "MENUITEM_OPTCOOPCAM_NOTE";
	items[12] = "MENUITEM_OPTCOOP_BACK";
	items[13] = "MENUITEM_OPTCOOPCAM_VERSION";
	defaultoutgame = 1;
	defaultingame = 1;
	flags = flags | MENU_SHOW_INFO;
};

instance MENUITEM_OPTCOOPCAM_HEADLINE(C_MENU_ITEM_DEF)
{
	text[0] = "Coop-Kamera";
	type = MENU_ITEM_TEXT;
	posx = 0;
	posy = MENU_TITLE_Y - 400;
	dimx = 8192;
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "Coop camera";
};

instance MENUITEM_COOP_CAMMODE(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Kamera";
	text[1] = "Automatisch: Fenster geht auf, wenn der Mitspieler weiter weg ist als unten eingestellt. Nur Taste: nur von Hand. Aus: kein Fenster.";
	posx = 1000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 0;
	dimx = 4000;
	dimy = 600;
	onselaction[0] = SEL_ACTION_UNDEF;
	flags = flags | IT_EFFECTS_NEXT;
	userString[2] = "Camera"; userString[3] = "Automatic: the window opens when your partner is farther away than set below. Key only: manual. Off: no window.";
};

instance MENUITEM_COOP_CAMMODE_CHOICE(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_CHOICEBOX;
	text[0] = "automatisch|nur Taste|aus";
	fontname = MENU_FONT_SMALL;
	posx = 5000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 0 + MENU_CHOICE_YPLUS;
	dimx = MENU_SLIDER_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "camWindowMode";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "automatic|key only|off";
};

instance MENUITEM_COOP_WINDOW(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Fenstergröße";
	text[1] = "Breite des Fensters, in dem man dem Mitspieler zuschaut, in Prozent der Bildbreite (braucht den Coop-Renderer).";
	posx = 1000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 1;
	dimx = 4000;
	dimy = 600;
	onselaction[0] = SEL_ACTION_UNDEF;
	flags = flags | IT_EFFECTS_NEXT;
	userString[2] = "Window size"; userString[3] = "Width of the window showing your partner, in percent of the screen width (needs the coop renderer).";
};

instance MENUITEM_COOP_WINDOW_CHOICE(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_CHOICEBOX;
	text[0] = "15 %|20 %|25 %|30 %|35 %|40 %|45 %|50 %";
	fontname = MENU_FONT_SMALL;
	posx = 5000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 1 + MENU_CHOICE_YPLUS;
	dimx = MENU_SLIDER_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "camWindowWidth";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
};

instance MENUITEM_COOP_WINPOS(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Lage";
	text[1] = "In welcher Ecke das Zuschau-Fenster steht.";
	posx = 1000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 2;
	dimx = 4000;
	dimy = 600;
	onselaction[0] = SEL_ACTION_UNDEF;
	flags = flags | IT_EFFECTS_NEXT;
	userString[2] = "Position"; userString[3] = "Which corner the watch window is in.";
};

instance MENUITEM_COOP_WINPOS_CHOICE(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_CHOICEBOX;
	text[0] = "oben rechts|oben links|unten rechts|unten links";
	fontname = MENU_FONT_SMALL;
	posx = 5000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 2 + MENU_CHOICE_YPLUS;
	dimx = MENU_SLIDER_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "camWindowPos";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "top right|top left|bottom right|bottom left";
};

instance MENUITEM_COOP_CAMKEY(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Taste";
	text[1] = "Fenster an, nächster Mitspieler, aus. F6-F10 und F12 belegen GothicCoop bzw. Othello schon.";
	posx = 1000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 3;
	dimx = 4000;
	dimy = 600;
	onselaction[0] = SEL_ACTION_UNDEF;
	flags = flags | IT_EFFECTS_NEXT;
	userString[2] = "Key"; userString[3] = "Window on, next player, off. F6-F10 and F12 are already used by GothicCoop or Othello.";
};

instance MENUITEM_COOP_CAMKEY_CHOICE(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_CHOICEBOX;
	text[0] = "Shift+F5|F5|F11|Shift+Einfg|Einfg|Shift+Pos1|Pos1|Shift+Ende|Ende";
	fontname = MENU_FONT_SMALL;
	posx = 5000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 3 + MENU_CHOICE_YPLUS;
	dimx = MENU_SLIDER_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "camWindowKey";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "Shift+F5|F5|F11|Shift+Ins|Ins|Shift+Home|Home|Shift+End|End";
};

instance MENUITEM_COOP_CAMDIST(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Automatik ab";
	text[1] = "Ab dieser Entfernung zum Mitspieler geht das Fenster bei Kamera automatisch auf (zu bei 85 % davon).";
	posx = 1000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 4;
	dimx = 4000;
	dimy = 600;
	onselaction[0] = SEL_ACTION_UNDEF;
	flags = flags | IT_EFFECTS_NEXT;
	userString[2] = "Automatic from"; userString[3] = "From this distance to your partner the window opens automatically (closes at 85 % of it).";
};

instance MENUITEM_COOP_CAMDIST_CHOICE(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_CHOICEBOX;
	text[0] = "20 m|30 m|45 m|60 m|80 m|100 m";
	fontname = MENU_FONT_SMALL;
	posx = 5000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 4 + MENU_CHOICE_YPLUS;
	dimx = MENU_SLIDER_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "camWindowDist";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
};

instance MENUITEM_OPTCOOPCAM_NOTE(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = "Das Fenster zeigt den Mitspieler von hinten. Gilt sofort, auch im laufenden Spiel.";
	type = MENU_ITEM_TEXT;
	posx = 0;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 6;
	dimx = 8192;
	dimy = 350;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
	userString[2] = "The window shows your partner from behind. Applies immediately, also in game.";
};

// Schwierigkeitsgrad (v67): Gothic.ini [GOTHICCOOP] coopDiffPreset 0-4, coopDmgPlayers 0-5, coopDmgEnemies 0-6, coopEnemyHp 0-6,
// v68: coopXpAll 0-5 (EP allgemein 50-100 %), coopXpPartner 0-6 (EP-Anteil Partner 0/50-100 %), coopXpRange 0-3 (GothicCoop belegt vor). Eine Stufe setzt die drei Kampfwerte, ein geaenderter Kampfwert setzt
// die Stufe auf "Eigene" (GothicCoop, CoopDifficulty.cpp). Wirkt nur im Coop; der Host gibt vor. Die Auswahlfelder heissen
// MENUITEM_OPTDIFF_* - GothicCoop liest sie nach einer Stufenwahl neu ein.
instance MENU_OPT_COOP_DIFF(C_MENU_DEF)
{
	backpic = MENU_BACK_PIC;
	items[0] = "MENUITEM_OPTCOOPDIFF_HEADLINE";
	items[1] = "MENUITEM_COOPDIFF_PRESET";
	items[2] = "MENUITEM_OPTDIFF_PRESET_CHOICE";
	items[3] = "MENUITEM_COOPDIFF_DMGPLAYERS";
	items[4] = "MENUITEM_OPTDIFF_DMGPLAYERS_CHOICE";
	items[5] = "MENUITEM_COOPDIFF_DMGENEMIES";
	items[6] = "MENUITEM_OPTDIFF_DMGENEMIES_CHOICE";
	items[7] = "MENUITEM_COOPDIFF_ENEMYHP";
	items[8] = "MENUITEM_OPTDIFF_ENEMYHP_CHOICE";
	items[9] = "MENUITEM_COOPDIFF_XPALL";
	items[10] = "MENUITEM_OPTDIFF_XPALL_CHOICE";
	items[11] = "MENUITEM_COOPDIFF_XPPARTNER";
	items[12] = "MENUITEM_OPTDIFF_XPPARTNER_CHOICE";
	items[13] = "MENUITEM_COOPDIFF_XPRANGE";
	items[14] = "MENUITEM_OPTDIFF_XPRANGE_CHOICE";
	items[15] = "MENUITEM_OPTCOOPDIFF_NOTE";
	items[16] = "MENUITEM_OPTCOOP_BACK";
	items[17] = "MENUITEM_OPTCOOPDIFF_VERSION";
	defaultoutgame = 1;
	defaultingame = 1;
	flags = flags | MENU_SHOW_INFO;
};

instance MENUITEM_OPTCOOPDIFF_HEADLINE(C_MENU_ITEM_DEF)
{
	text[0] = "Schwierigkeitsgrad";
	type = MENU_ITEM_TEXT;
	posx = 0;
	posy = MENU_TITLE_Y - 400;
	dimx = 8192;
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "Difficulty";
};

instance MENUITEM_COOPDIFF_PRESET(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Stufe";
	text[1] = "Leicht, Normal, Schwer, Sehr schwer setzen die drei Werte darunter. Ändert man einen davon, gilt Eigene.";
	posx = 1000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 0;
	dimx = 4000;
	dimy = 600;
	onselaction[0] = SEL_ACTION_UNDEF;
	flags = flags | IT_EFFECTS_NEXT;
	userString[2] = "Level"; userString[3] = "Easy, Normal, Hard, Very hard set the three values below. Changing one of them makes it Custom.";
};

instance MENUITEM_OPTDIFF_PRESET_CHOICE(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_CHOICEBOX;
	text[0] = "Leicht|Normal|Schwer|Sehr schwer|Eigene";
	fontname = MENU_FONT_SMALL;
	posx = 5000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 0 + MENU_CHOICE_YPLUS;
	dimx = MENU_SLIDER_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "coopDiffPreset";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "Easy|Normal|Hard|Very hard|Custom";
};

instance MENUITEM_COOPDIFF_DMGPLAYERS(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Schaden der Spieler";
	text[1] = "Wie stark die Spieler Gegner und andere NPCs treffen.";
	posx = 1000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 1;
	dimx = 4000;
	dimy = 600;
	onselaction[0] = SEL_ACTION_UNDEF;
	flags = flags | IT_EFFECTS_NEXT;
	userString[2] = "Player damage"; userString[3] = "How hard players hit enemies and other NPCs.";
};

instance MENUITEM_OPTDIFF_DMGPLAYERS_CHOICE(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_CHOICEBOX;
	text[0] = "50 %|75 %|100 %|125 %|150 %|200 %";
	fontname = MENU_FONT_SMALL;
	posx = 5000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 1 + MENU_CHOICE_YPLUS;
	dimx = MENU_SLIDER_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "coopDmgPlayers";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
};

instance MENUITEM_COOPDIFF_DMGENEMIES(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Schaden der Gegner";
	text[1] = "Wie stark Gegner und andere NPCs die Spieler treffen.";
	posx = 1000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 2;
	dimx = 4000;
	dimy = 600;
	onselaction[0] = SEL_ACTION_UNDEF;
	flags = flags | IT_EFFECTS_NEXT;
	userString[2] = "Enemy damage"; userString[3] = "How hard enemies and other NPCs hit the players.";
};

instance MENUITEM_OPTDIFF_DMGENEMIES_CHOICE(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_CHOICEBOX;
	text[0] = "50 %|75 %|100 %|125 %|150 %|200 %|300 %";
	fontname = MENU_FONT_SMALL;
	posx = 5000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 2 + MENU_CHOICE_YPLUS;
	dimx = MENU_SLIDER_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "coopDmgEnemies";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
};

instance MENUITEM_COOPDIFF_ENEMYHP(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Leben der Gegner";
	text[1] = "Lebenspunkte der Gegner (HP und Maximum, im Coop). In Spielstände kommen immer die normalen Werte.";
	posx = 1000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 3;
	dimx = 4000;
	dimy = 600;
	onselaction[0] = SEL_ACTION_UNDEF;
	flags = flags | IT_EFFECTS_NEXT;
	userString[2] = "Enemy health"; userString[3] = "Hit points of enemies (HP and maximum, in coop). Saves always get the normal values.";
};

instance MENUITEM_OPTDIFF_ENEMYHP_CHOICE(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_CHOICEBOX;
	text[0] = "50 %|75 %|100 %|125 %|150 %|200 %|300 %";
	fontname = MENU_FONT_SMALL;
	posx = 5000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 3 + MENU_CHOICE_YPLUS;
	dimx = MENU_SLIDER_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "coopEnemyHp";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
};

instance MENUITEM_COOPDIFF_XPALL(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "EP allgemein";
	text[1] = "Anteil der Kampf-EP (Kill, erster Sieg über Menschen) für jeden Spieler. Quest-EP bleiben voll.";
	posx = 1000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 4;
	dimx = 4000;
	dimy = 600;
	onselaction[0] = SEL_ACTION_UNDEF;
	flags = flags | IT_EFFECTS_NEXT;
	userString[2] = "XP overall"; userString[3] = "Share of combat XP (kill, first win over humans) for every player. Quest XP stays full.";
};

instance MENUITEM_OPTDIFF_XPALL_CHOICE(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_CHOICEBOX;
	text[0] = "50 %|60 %|70 %|80 %|90 %|100 %";
	fontname = MENU_FONT_SMALL;
	posx = 5000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 4 + MENU_CHOICE_YPLUS;
	dimx = MENU_SLIDER_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "coopXpAll";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
};

instance MENUITEM_COOPDIFF_XPPARTNER(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "EP-Anteil Partner";
	text[1] = "EP, die man für Kills und Siege des Partners bekommt, bezogen auf EP allgemein. 0 % = nur wer tötet.";
	posx = 1000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 5;
	dimx = 4000;
	dimy = 600;
	onselaction[0] = SEL_ACTION_UNDEF;
	flags = flags | IT_EFFECTS_NEXT;
	userString[2] = "Partner XP share"; userString[3] = "XP for your partner's kills and wins, relative to XP overall. 0 % = only the killer.";
};

instance MENUITEM_OPTDIFF_XPPARTNER_CHOICE(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_CHOICEBOX;
	text[0] = "0 %|50 %|60 %|70 %|80 %|90 %|100 %";
	fontname = MENU_FONT_SMALL;
	posx = 5000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 5 + MENU_CHOICE_YPLUS;
	dimx = MENU_SLIDER_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "coopXpPartner";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
};

instance MENUITEM_COOPDIFF_XPRANGE(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "EP-Reichweite";
	text[1] = "Kills und Siege des Partners geben nur EP, wenn er höchstens so weit entfernt ist.";
	posx = 1000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 6;
	dimx = 4000;
	dimy = 600;
	onselaction[0] = SEL_ACTION_UNDEF;
	flags = flags | IT_EFFECTS_NEXT;
	userString[2] = "XP range"; userString[3] = "Your partner's kills and wins only give XP if they are at most this far away.";
};

instance MENUITEM_OPTDIFF_XPRANGE_CHOICE(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_CHOICEBOX;
	text[0] = "20 m|45 m|100 m|überall";
	fontname = MENU_FONT_SMALL;
	posx = 5000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 6 + MENU_CHOICE_YPLUS;
	dimx = MENU_SLIDER_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "coopXpRange";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "20 m|45 m|100 m|anywhere";
};

instance MENUITEM_OPTCOOPDIFF_NOTE(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = "Gilt im Coop für alle Spieler, der Host gibt sie vor.";
	type = MENU_ITEM_TEXT;
	posx = 0;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 7;
	dimx = 8192;
	dimy = 350;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
};

// --- v91: Gemeinsames neues Spiel ---------------------------------------------------------------------------------------
// GothicCoop öffnet MENU_COOP_NG, wenn der Host mit Mitspieler in Othellos MENU_NG eine Schwierigkeit wählt. Die COOP-Werte
// sind dieselben Einträge wie in MENU_OPT_COOP_DIFF (Gothic.ini [GOTHICCOOP], GothicCoop liest sie; die Hinweiszeile nennt
// Othellos Wahl). "Spiel starten" (COOP_NG_START): beide starten mit Othellos Schwierigkeit. "Abbrechen": zurück zu Othellos Auswahl.
instance MENU_COOP_NG(C_MENU_DEF)
{
	backpic = MENU_BACK_PIC;
	items[0] = "MENUITEM_COOPNG_HEADLINE";
	items[1] = "MENUITEM_COOPDIFF_PRESET";
	items[2] = "MENUITEM_OPTDIFF_PRESET_CHOICE";
	items[3] = "MENUITEM_COOPDIFF_DMGPLAYERS";
	items[4] = "MENUITEM_OPTDIFF_DMGPLAYERS_CHOICE";
	items[5] = "MENUITEM_COOPDIFF_DMGENEMIES";
	items[6] = "MENUITEM_OPTDIFF_DMGENEMIES_CHOICE";
	items[7] = "MENUITEM_COOPDIFF_ENEMYHP";
	items[8] = "MENUITEM_OPTDIFF_ENEMYHP_CHOICE";
	items[9] = "MENUITEM_COOPDIFF_XPALL";
	items[10] = "MENUITEM_OPTDIFF_XPALL_CHOICE";
	items[11] = "MENUITEM_COOPDIFF_XPPARTNER";
	items[12] = "MENUITEM_OPTDIFF_XPPARTNER_CHOICE";
	items[13] = "MENUITEM_COOPDIFF_XPRANGE";
	items[14] = "MENUITEM_OPTDIFF_XPRANGE_CHOICE";
	items[15] = "MENUITEM_COOPNG_NOTE";
	items[16] = "MENUITEM_COOPNG_START";
	items[17] = "MENUITEM_COOPNG_BACK";
	items[18] = "MENUITEM_COOPNG_VERSION";
	defaultoutgame = 16;
	defaultingame = 16;
	flags = flags | MENU_SHOW_INFO;
};

instance MENUITEM_COOPNG_HEADLINE(C_MENU_ITEM_DEF)
{
	text[0] = "Gemeinsames neues Spiel";
	type = MENU_ITEM_TEXT;
	posx = 0;
	posy = MENU_TITLE_Y - 400;
	dimx = 8192;
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "New game together";
};

instance MENUITEM_COOPNG_NOTE(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = "COOP-Schwierigkeit - gilt für beide, der Host gibt sie vor.";
	type = MENU_ITEM_TEXT;
	posx = 0;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 7;
	dimx = 8192;
	dimy = 350;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
};

instance MENUITEM_COOPNG_START(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Spiel starten";
	text[1] = "Beide starten das neue Spiel mit dieser Schwierigkeit - der Host spricht zuerst mit Diego.";
	posx = 700;
	posy = MENU_BACK_Y;
	dimx = 3600;
	dimy = MENU_OPT_DY;
	onselaction[0] = SEL_ACTION_EXECCOMMANDS;
	onselaction_s[0] = "COOP_NG_START";
	flags = flags | IT_TXT_CENTER;
	userString[2] = "Start game"; userString[3] = "Both start the new game with this difficulty - the host talks to Diego first.";
};

instance MENUITEM_COOPNG_BACK(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Abbrechen";
	text[1] = "Zurück zu Othellos Auswahl der Schwierigkeit - nichts startet.";
	posx = 4500;
	posy = MENU_BACK_Y;
	dimx = 3000;
	dimy = MENU_OPT_DY;
	onselaction[0] = SEL_ACTION_BACK;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "Cancel"; userString[3] = "Back to Othello's difficulty choice - nothing starts.";
};

instance MENUITEM_COOPNG_VERSION(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = "GothicCoop";
	type = MENU_ITEM_TEXT;
	posx = 5300;
	posy = 150;
	dimx = 2800;
	dimy = 300;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
};

// v75: Abgleich - Gothic.ini [GOTHICCOOP] coopSyncQuests/coopSyncLog/coopSyncWorld 0 an 1 aus, coopSyncSaves 0 gemeinsam
// 1 getrennt, coopAutoSave 0 aus 1-5 = 5/10/15/20/30 min, coopConnectTake 0 Host-Stand 1 eigener Stand (GothicCoop belegt vor).
instance MENU_OPT_COOP_SYNC(C_MENU_DEF)
{
	backpic = MENU_BACK_PIC;
	items[0] = "MENUITEM_OPTCOOPSYNC_HEADLINE";
	items[1] = "MENUITEM_COOPSYNC_QUESTS";
	items[2] = "MENUITEM_OPTSYNC_QUESTS_CHOICE";
	items[3] = "MENUITEM_COOPSYNC_LOG";
	items[4] = "MENUITEM_OPTSYNC_LOG_CHOICE";
	items[5] = "MENUITEM_COOPSYNC_WORLD";
	items[6] = "MENUITEM_OPTSYNC_WORLD_CHOICE";
	items[7] = "MENUITEM_COOPSYNC_SAVES";
	items[8] = "MENUITEM_OPTSYNC_SAVES_CHOICE";
	items[9] = "MENUITEM_COOPSYNC_AUTOSAVE";
	items[10] = "MENUITEM_OPTSYNC_AUTOSAVE_CHOICE";
	items[11] = "MENUITEM_COOPSYNC_CONNECT";
	items[12] = "MENUITEM_OPTSYNC_CONNECT_CHOICE";
	items[13] = "MENUITEM_OPTCOOPSYNC_NOTE";
	items[14] = "MENUITEM_OPTCOOP_BACK";
	items[15] = "MENUITEM_OPTCOOPSYNC_VERSION";
	defaultoutgame = 1;
	defaultingame = 1;
	flags = flags | MENU_SHOW_INFO;
};

instance MENUITEM_OPTCOOPSYNC_HEADLINE(C_MENU_ITEM_DEF)
{
	text[0] = "Abgleich";
	type = MENU_ITEM_TEXT;
	posx = 0;
	posy = MENU_TITLE_Y - 400;
	dimx = 8192;
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "Sync";
};

instance MENUITEM_COOPSYNC_QUESTS(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Quests";
	text[1] = "Quest-Fortschritt: erzählte Dialoge, Variablen, NPC-Zustände, Quest-EP - beide auf demselben Stand.";
	posx = 1000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 0;
	dimx = 4000;
	dimy = 600;
	onselaction[0] = SEL_ACTION_UNDEF;
	flags = flags | IT_EFFECTS_NEXT;
	userString[2] = "Quests"; userString[3] = "Quest progress: told dialogues, variables, NPC states, quest XP - both on the same state.";
};

instance MENUITEM_OPTSYNC_QUESTS_CHOICE(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_CHOICEBOX;
	text[0] = "an|aus";
	fontname = MENU_FONT_SMALL;
	posx = 5000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 0 + MENU_CHOICE_YPLUS;
	dimx = MENU_SLIDER_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "coopSyncQuests";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "on|off";
};

instance MENUITEM_COOPSYNC_LOG(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Tagebuch";
	text[1] = "Tagebuch-Themen und -Einträge des einen erscheinen auch beim anderen.";
	posx = 1000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 1;
	dimx = 4000;
	dimy = 600;
	onselaction[0] = SEL_ACTION_UNDEF;
	flags = flags | IT_EFFECTS_NEXT;
	userString[2] = "Diary"; userString[3] = "Diary topics and entries of one also appear for the other.";
};

instance MENUITEM_OPTSYNC_LOG_CHOICE(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_CHOICEBOX;
	text[0] = "an|aus";
	fontname = MENU_FONT_SMALL;
	posx = 5000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 1 + MENU_CHOICE_YPLUS;
	dimx = MENU_SLIDER_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "coopSyncLog";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "on|off";
};

instance MENUITEM_COOPSYNC_WORLD(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Gegenstände/Truhen";
	text[1] = "Truhen und Gegenstände am Boden: der Host führt den Bestand, was einer nimmt, ist für beide weg.";
	posx = 1000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 2;
	dimx = 4000;
	dimy = 600;
	onselaction[0] = SEL_ACTION_UNDEF;
	flags = flags | IT_EFFECTS_NEXT;
	userString[2] = "Items/chests"; userString[3] = "Chests and items on the ground: the host keeps the stock, what one takes is gone for both.";
};

instance MENUITEM_OPTSYNC_WORLD_CHOICE(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_CHOICEBOX;
	text[0] = "an|aus";
	fontname = MENU_FONT_SMALL;
	posx = 5000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 2 + MENU_CHOICE_YPLUS;
	dimx = MENU_SLIDER_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "coopSyncWorld";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "on|off";
};

instance MENUITEM_COOPSYNC_SAVES(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Spielstände";
	text[1] = "Gemeinsam: zusammen speichern und laden, Abgleich-Sicherungen. Getrennt: jeder speichert und lädt selbst.";
	posx = 1000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 3;
	dimx = 4000;
	dimy = 600;
	onselaction[0] = SEL_ACTION_UNDEF;
	flags = flags | IT_EFFECTS_NEXT;
	userString[2] = "Saves"; userString[3] = "Shared: save and load together, sync backups. Separate: everyone saves and loads alone.";
};

instance MENUITEM_OPTSYNC_SAVES_CHOICE(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_CHOICEBOX;
	text[0] = "gemeinsam|getrennt";
	fontname = MENU_FONT_SMALL;
	posx = 5000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 3 + MENU_CHOICE_YPLUS;
	dimx = MENU_SLIDER_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "coopSyncSaves";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "shared|separate";
};

instance MENUITEM_COOPSYNC_AUTOSAVE(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Auto-Sicherung";
	text[1] = "Gemeinsame Sicherung des Hosts in festen Abständen (reservierte Slots 6-9), der Mitspieler speichert mit.";
	posx = 1000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 4;
	dimx = 4000;
	dimy = 600;
	onselaction[0] = SEL_ACTION_UNDEF;
	flags = flags | IT_EFFECTS_NEXT;
	userString[2] = "Autosave"; userString[3] = "Shared host backup at fixed intervals (reserved slots 6-9), the player saves along.";
};

instance MENUITEM_OPTSYNC_AUTOSAVE_CHOICE(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_CHOICEBOX;
	text[0] = "aus|5 min|10 min|15 min|20 min|30 min";
	fontname = MENU_FONT_SMALL;
	posx = 5000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 4 + MENU_CHOICE_YPLUS;
	dimx = MENU_SLIDER_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "coopAutoSave";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "off|5 min|10 min|15 min|20 min|30 min";
};

instance MENUITEM_COOPSYNC_CONNECT(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Beim Verbinden";
	text[1] = "Verbindet sich ein Mitspieler im Spiel: Stand des Hosts übernehmen (eigener Held bleibt) oder eigenen behalten.";
	posx = 1000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 5;
	dimx = 4000;
	dimy = 600;
	onselaction[0] = SEL_ACTION_UNDEF;
	flags = flags | IT_EFFECTS_NEXT;
	userString[2] = "On connect"; userString[3] = "When a player connects in game: take the host's state (own hero kept) or keep your own.";
};

instance MENUITEM_OPTSYNC_CONNECT_CHOICE(C_MENU_ITEM_DEF)
{
	backpic = MENU_CHOICE_BACK_PIC;
	type = MENU_ITEM_CHOICEBOX;
	text[0] = "Host-Stand|eigener Stand";
	fontname = MENU_FONT_SMALL;
	posx = 5000;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 5 + MENU_CHOICE_YPLUS;
	dimx = MENU_SLIDER_DX;
	dimy = MENU_CHOICE_DY;
	onchgsetoption = "coopConnectTake";
	onchgsetoptionsection = "GOTHICCOOP";
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "host state|own state";
};

instance MENUITEM_OPTCOOPSYNC_NOTE(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = "Gilt im Coop für alle Spieler, der Host gibt sie vor.";
	type = MENU_ITEM_TEXT;
	posx = 0;
	posy = MENU_OPT_START_Y + MENU_OPT_DY * 7;
	dimx = 8192;
	dimy = 350;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
};

// --- v82: Esc-Menü im Spiel - "Hauptmenü" (zurück zum Startbildschirm, ohne Neustart) ----------------------------------

instance MENUITEM_COOPINGAME_TITLE(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Hauptmenü";
	text[1] = "Spiel verlassen und zum Hauptmenü zurückkehren - eine Coop-Verbindung wird getrennt.";
	posx = 0;
	posy = MENU_MAIN_Y + MENU_MAIN_DY * 5;
	dimx = 8192;
	dimy = 750;
	onselaction[0] = SEL_ACTION_STARTMENU;
	onselaction_s[0] = "MENU_COOP_TITLE";
	flags = flags | IT_TXT_CENTER;
	userString[2] = "Main menu"; userString[3] = "Leave the game and return to the main menu - a coop connection is closed.";
};

// v91: "Spielstand wählen" im Esc-Menü - GothicCoop hängt ihn hinter "Spiel laden" ein (die Einträge darunter rücken um
//      MENU_MAIN_DY nach unten; die Lage setzt GothicCoop). Liste wie im Hauptmenü: GothicCoop füllt MENU_COOP_LOAD.
instance MENUITEM_COOPINGAME_LOAD(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Spielstand wählen";
	text[1] = "Gemeinsame Coop-Spielstände - der Host lädt für beide, als Mitspieler geht der Wunsch an den Host.";
	posx = 0;
	posy = MENU_MAIN_Y + MENU_MAIN_DY * 3;
	dimx = 8192;
	dimy = 750;
	onselaction[0] = SEL_ACTION_STARTMENU;
	onselaction_s[0] = "MENU_COOP_LOAD";
	flags = flags | IT_TXT_CENTER;
	userString[2] = "Choose save"; userString[3] = "Shared coop saves - the host loads for both, as player the request goes to the host.";
};

instance MENU_COOP_TITLE(C_MENU_DEF)
{
	backpic = MENU_BACK_PIC;
	items[0] = "MENUITEM_COOP_TITLE_HEADLINE";
	items[1] = "MENUITEM_COOP_TITLE_YES";
	items[2] = "MENUITEM_COOP_TITLE_NO";
	defaultoutgame = 2;
	defaultingame = 2;
	flags = flags | MENU_SHOW_INFO;
};

instance MENUITEM_COOP_TITLE_HEADLINE(C_MENU_ITEM_DEF)
{
	text[0] = "Zum Hauptmenü zurückkehren?";
	type = MENU_ITEM_TEXT;
	posx = 0;
	posy = 3400;
	dimx = 8192;
	dimy = 500;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
	userString[2] = "Return to the main menu?";
};

instance MENUITEM_COOP_TITLE_YES(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Ja";
	text[1] = "Nicht gespeicherter Fortschritt geht verloren, eine Coop-Verbindung wird getrennt.";
	posx = 0;
	posy = 4400;
	dimx = 8192;
	dimy = 500;
	onselaction[0] = SEL_ACTION_EXECCOMMANDS;
	onselaction_s[0] = "COOP_TITLE";
	flags = flags | IT_TXT_CENTER;
	userString[2] = "Yes"; userString[3] = "Unsaved progress is lost, a coop connection is closed.";
};

instance MENUITEM_COOP_TITLE_NO(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Nein";
	text[1] = "Weiterspielen.";
	posx = 0;
	posy = 5000;
	dimx = 8192;
	dimy = 500;
	onselaction[0] = SEL_ACTION_BACK;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "No"; userString[3] = "Keep playing.";
};

// --- v87: Coop über Steam (GothicCoop füllt Status und Freundeszeilen, CoopSteam.cpp) ------------------------------------

const int COOPSTEAM_Y0 = 2050;   // v94: große Schrift (bis v93 klein, 2100/430)
const int COOPSTEAM_DY = 520;

instance MENU_COOP_STEAM(C_MENU_DEF)
{
	backpic = MENU_BACK_PIC;
	items[0] = "MENUITEM_COOPSTEAM_HEADLINE";
	items[1] = "MENUITEM_COOPSTEAM_STATUS1";
	items[2] = "MENUITEM_COOPSTEAM_STATUS2";
	items[3] = "MENUITEM_COOPSTEAM_ROW0";
	items[4] = "MENUITEM_COOPSTEAM_ROW1";
	items[5] = "MENUITEM_COOPSTEAM_ROW2";
	items[6] = "MENUITEM_COOPSTEAM_ROW3";
	items[7] = "MENUITEM_COOPSTEAM_ROW4";
	items[8] = "MENUITEM_COOPSTEAM_ROW5";
	items[9] = "MENUITEM_COOPSTEAM_HOST";
	items[10] = "MENUITEM_COOPSTEAM_STOP";
	items[11] = "MENUITEM_COOPSTEAM_NET1";
	items[12] = "MENUITEM_COOPSTEAM_NET2";
	items[13] = "MENUITEM_COOPSTEAM_BACK";
	items[14] = "MENUITEM_COOPSTEAM_VERSION";
	defaultoutgame = 3;
	defaultingame = 3;
	flags = flags | MENU_SHOW_INFO;
};

instance MENUITEM_COOPSTEAM_HEADLINE(C_MENU_ITEM_DEF)
{
	text[0] = "STEAM-FREUNDE";
	type = MENU_ITEM_TEXT;
	posx = 0;
	posy = MENU_TITLE_Y - 500;
	dimx = 8192;
	flags = flags & ~IT_SELECTABLE;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "STEAM FRIENDS";
};

instance MENUITEM_COOPSTEAM_STATUS1(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = "Steam wird geprüft ...";
	type = MENU_ITEM_TEXT;
	posx = 400;
	posy = 1250;
	dimx = 7392;
	dimy = 350;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
};

instance MENUITEM_COOPSTEAM_STATUS2(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = " ";
	type = MENU_ITEM_TEXT;
	posx = 400;
	posy = 1600;
	dimx = 7392;
	dimy = 350;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
};

instance MENUITEM_COOPSTEAM_ROW0(C_MENU_ITEM_DEF) { backpic = MENU_ITEM_BACK_PIC; text[0] = " "; text[1] = "Hostet er: ein Klick tritt bei. Hostest du: ein Klick lädt ihn ein (er nimmt in Steam an, Gothic muss laufen)."; posx = 1000; posy = COOPSTEAM_Y0 + COOPSTEAM_DY * 0; dimx = 6192; dimy = 480; onselaction[0] = SEL_ACTION_EXECCOMMANDS; onselaction_s[0] = "COOP_STEAM_ROW_0"; userString[3] = "If they host: one click joins. If you host: one click invites them (they accept in Steam, Gothic must run)."; };
instance MENUITEM_COOPSTEAM_ROW1(C_MENU_ITEM_DEF) { backpic = MENU_ITEM_BACK_PIC; text[0] = " "; text[1] = "Hostet er: ein Klick tritt bei. Hostest du: ein Klick lädt ihn ein (er nimmt in Steam an, Gothic muss laufen)."; posx = 1000; posy = COOPSTEAM_Y0 + COOPSTEAM_DY * 1; dimx = 6192; dimy = 480; onselaction[0] = SEL_ACTION_EXECCOMMANDS; onselaction_s[0] = "COOP_STEAM_ROW_1"; userString[3] = "If they host: one click joins. If you host: one click invites them (they accept in Steam, Gothic must run)."; };
instance MENUITEM_COOPSTEAM_ROW2(C_MENU_ITEM_DEF) { backpic = MENU_ITEM_BACK_PIC; text[0] = " "; text[1] = "Hostet er: ein Klick tritt bei. Hostest du: ein Klick lädt ihn ein (er nimmt in Steam an, Gothic muss laufen)."; posx = 1000; posy = COOPSTEAM_Y0 + COOPSTEAM_DY * 2; dimx = 6192; dimy = 480; onselaction[0] = SEL_ACTION_EXECCOMMANDS; onselaction_s[0] = "COOP_STEAM_ROW_2"; userString[3] = "If they host: one click joins. If you host: one click invites them (they accept in Steam, Gothic must run)."; };
instance MENUITEM_COOPSTEAM_ROW3(C_MENU_ITEM_DEF) { backpic = MENU_ITEM_BACK_PIC; text[0] = " "; text[1] = "Hostet er: ein Klick tritt bei. Hostest du: ein Klick lädt ihn ein (er nimmt in Steam an, Gothic muss laufen)."; posx = 1000; posy = COOPSTEAM_Y0 + COOPSTEAM_DY * 3; dimx = 6192; dimy = 480; onselaction[0] = SEL_ACTION_EXECCOMMANDS; onselaction_s[0] = "COOP_STEAM_ROW_3"; userString[3] = "If they host: one click joins. If you host: one click invites them (they accept in Steam, Gothic must run)."; };
instance MENUITEM_COOPSTEAM_ROW4(C_MENU_ITEM_DEF) { backpic = MENU_ITEM_BACK_PIC; text[0] = " "; text[1] = "Hostet er: ein Klick tritt bei. Hostest du: ein Klick lädt ihn ein (er nimmt in Steam an, Gothic muss laufen)."; posx = 1000; posy = COOPSTEAM_Y0 + COOPSTEAM_DY * 4; dimx = 6192; dimy = 480; onselaction[0] = SEL_ACTION_EXECCOMMANDS; onselaction_s[0] = "COOP_STEAM_ROW_4"; userString[3] = "If they host: one click joins. If you host: one click invites them (they accept in Steam, Gothic must run)."; };
instance MENUITEM_COOPSTEAM_ROW5(C_MENU_ITEM_DEF) { backpic = MENU_ITEM_BACK_PIC; text[0] = " "; text[1] = "Hostet er: ein Klick tritt bei. Hostest du: ein Klick lädt ihn ein (er nimmt in Steam an, Gothic muss laufen)."; posx = 1000; posy = COOPSTEAM_Y0 + COOPSTEAM_DY * 5; dimx = 6192; dimy = 480; onselaction[0] = SEL_ACTION_EXECCOMMANDS; onselaction_s[0] = "COOP_STEAM_ROW_5"; userString[3] = "If they host: one click joins. If you host: one click invites them (they accept in Steam, Gothic must run)."; };

instance MENUITEM_COOPSTEAM_HOST(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Spiel hosten";
	text[1] = "Host starten - Freunde treten über Steam bei (hier oder über die Steam-Freundesliste).";
	posx = 500;
	posy = 5300;
	dimx = 3000;
	dimy = 600;
	onselaction[0] = SEL_ACTION_EXECCOMMANDS;
	onselaction_s[0] = "COOP_STEAM_HOST";
	flags = flags | IT_TXT_CENTER;
	userString[2] = "Host game"; userString[3] = "Start a host - friends join via Steam (here or via the Steam friends list).";
};

instance MENUITEM_COOPSTEAM_STOP(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "Verbindung trennen";
	text[1] = "Host bzw. Verbindung beenden (im Spiel als Host nur ohne verbundene Mitspieler).";
	posx = 3700;
	posy = 5300;
	dimx = 4400;   // wie in MENU_COOP (v75: bei 3200 abgeschnitten)
	dimy = 600;
	onselaction[0] = SEL_ACTION_EXECCOMMANDS;
	onselaction_s[0] = "COOP_STEAM_STOP";
	flags = flags | IT_TXT_CENTER;
	userString[2] = "Disconnect"; userString[3] = "Stop the host or connection (in game as host only without connected players).";
};

instance MENUITEM_COOPSTEAM_NET1(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = " ";
	type = MENU_ITEM_TEXT;
	posx = 400;
	posy = 6000;
	dimx = 7392;
	dimy = 350;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
};

instance MENUITEM_COOPSTEAM_NET2(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = " ";
	type = MENU_ITEM_TEXT;
	posx = 400;
	posy = 6350;
	dimx = 7392;
	dimy = 350;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
};

instance MENUITEM_COOPSTEAM_BACK(C_MENU_ITEM_DEF)
{
	backpic = MENU_ITEM_BACK_PIC;
	text[0] = "ZURÜCK";
	posx = 2596;
	posy = 7050;
	dimx = 3000;
	dimy = 600;
	onselaction[0] = SEL_ACTION_BACK;
	flags = flags | IT_TXT_CENTER;
	userString[2] = "BACK";
};

instance MENUITEM_COOPSTEAM_VERSION(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = "GothicCoop";
	type = MENU_ITEM_TEXT;
	posx = 5300;
	posy = 150;
	dimx = 2800;
	dimy = 300;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
};

// --- v58: Versionszeilen ----------------------------------------------------------------------------------------------
// GothicCoop schreibt "GothicCoop vNN" hinein, im Hauptmenü (auch Esc-Menü im Spiel) mit Bauzeit. Nicht wählbar.
// Hauptmenü: zwischen dem letzten Eintrag (bis y 7100) und Gothics Infozeile (MENU_INFO_Y 7780). Sonst oben rechts
// (Überschriften ab MENU_TITLE_Y - 500 = 700).
instance MENUITEM_COOPMAIN_VERSION(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = "GothicCoop";
	type = MENU_ITEM_TEXT;
	posx = 0;
	posy = 7380;
	dimx = 8192;
	dimy = 300;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
};

instance MENUITEM_COOP_VERSION(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = "GothicCoop";
	type = MENU_ITEM_TEXT;
	posx = 5300;
	posy = 150;
	dimx = 2800;
	dimy = 300;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
};

instance MENUITEM_COOPLOAD_VERSION(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = "GothicCoop";
	type = MENU_ITEM_TEXT;
	posx = 5300;
	posy = 150;
	dimx = 2800;
	dimy = 300;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
};

instance MENUITEM_OPTCOOP_VERSION(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = "GothicCoop";
	type = MENU_ITEM_TEXT;
	posx = 5300;
	posy = 150;
	dimx = 2800;
	dimy = 300;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
};

instance MENUITEM_OPTCOOPCAM_VERSION(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = "GothicCoop";
	type = MENU_ITEM_TEXT;
	posx = 5300;
	posy = 150;
	dimx = 2800;
	dimy = 300;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
};

instance MENUITEM_OPTCOOPDIFF_VERSION(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = "GothicCoop";
	type = MENU_ITEM_TEXT;
	posx = 5300;
	posy = 150;
	dimx = 2800;
	dimy = 300;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
};

instance MENUITEM_OPTCOOPSYNC_VERSION(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = "GothicCoop";
	type = MENU_ITEM_TEXT;
	posx = 5300;
	posy = 150;
	dimx = 2800;
	dimy = 300;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
};

instance MENUITEM_COOPIP_VERSION(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = "GothicCoop";
	type = MENU_ITEM_TEXT;
	posx = 5300;
	posy = 150;
	dimx = 2800;
	dimy = 300;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
};

instance MENUITEM_COOPLOOK_VERSION(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = "GothicCoop";
	type = MENU_ITEM_TEXT;
	posx = 5300;
	posy = 150;
	dimx = 2800;
	dimy = 300;
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
};

// --- v60: 3D-Vorschau der eigenen Figur (v94: in MENU_COOP_LOOK) ------------------------------------------------------------------------------
// GothicCoop hängt sich in das Zeichnen dieses Eintrags (DrawItems) und lässt den Coop-Renderer (ab coop6) die Figur
// mit dem gewählten Aussehen hineinzeichnen. Ohne Coop-Renderer bleibt der Hinweistext stehen. Nicht wählbar.
instance MENUITEM_COOP_PREVIEW(C_MENU_ITEM_DEF)
{
	fontname = MENU_FONT_SMALL;
	text[0] = "Vorschau nur mit Coop-Renderer (coop6)";
	type = MENU_ITEM_TEXT;
	posx = 5950;
	posy = COOP_Y0 - 100;
	dimx = 2050;
	dimy = COOP_DY * 8 + 100;   // v95: neben den 8 Zeilen von Schritt 1
	flags = IT_CHROMAKEYED | IT_TRANSPARENT | IT_TXT_CENTER;
};
