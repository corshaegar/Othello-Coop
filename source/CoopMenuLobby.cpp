// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

    // --- v40: Mehrspieler im Hauptmenue (19.10.2026) ---------------------------------------------------------------------
    //
    // Nutzer: "im Hauptmenue einen neuen Eintrag fuer Multiplayer, wo man direkt waehlen kann zwischen Host und Client, dann
    // wartet man auf eine Verbindung, und wenn diese zustande kommt, kann man einen Spielstand waehlen, der dann bei beiden
    // geladen wird. Es soll dann immer der Held von dem Spielstand jeweils beim Client und Host geladen werden." Entscheidungen:
    // Eintrag ganz oben, nur gemeinsame Staende, Neues Spiel spaeter, Adresse weiter aus GothicCoopConfig.json.
    // Entwurf und Pruefung am Speicherabbild: MENUE-ENTWURF.md.
    //
    // Menue: COOP_MENU.D (zParserExtender, Parser = Menu) ersetzt MENU_MAIN und legt MENU_COOP an. Die Eintraege rufen
    // SEL_ACTION_EXECCOMMANDS mit COOP_HOST / COOP_CLIENT / COOP_STOP auf; zCMenu::HandleSelAction (0x004CF6E0, vtable +0x3C,
    // von Othello und den anderen Plugins unberuehrt) wird hier abgefangen. Fall 7 (EXECCOMMANDS) gibt 0 zurueck - das Menue
    // bleibt offen.
    // Takt: Union ersetzt zCMenu::Render (0x004D0DA0, vtable +0x1C) in allen sechs Menue-vtables durch SHW32+0x59C00 und ruft
    // dort Game_MenuLoop - auch fuer das einfache zCMenu (vtable 0x007D2ACC) unseres Untermenues.
    // Laden: Othellos Hauptmenue schliesst mit menuAction "OTHELLO_SAVEGAME_LOAD"; CGameManager::Menu (0x004279F0) ruft dann
    // Read_Savegame(menu_load_savegame->GetSelectedSlot()) (0x0042843C; GetSelectedSlot 0x0042D840 = m_selSlot +0xCCC).
    //   Host:   waehlt im Ladefenster; der Haken prueft vorher, ob der Mitspieler zu dieser Kennung einen eigenen Helden hat
    //           (nur gemeinsame Staende). Danach wie bisher: Game_LoadBegin_SaveGame -> LOAD_REQ an den Client.
    //   Client: LOAD_REQ kommt im Menue an; die Uebernahme (v19, Host-Welt + eigener Held aus COOPHERO.TXT) holt bei Bedarf die
    //           Dateien und schliesst dann das Menue selbst: m_selSlot setzen, zCMenu::ForceSelAction(SEL_ACTION_CLOSE,
    //           "OTHELLO_SAVEGAME_LOAD") (0x004CF150) auf dem obersten Menue. CLOSE laeuft ueber jede Menue-Ebene zurueck
    //           (zCMenu::Run gibt m_exitState == GO zurueck, der STARTMENU-Fall gibt das weiter).
    // Im Hauptmenue gibt es noch keine Spielsitzung (ogame = NULL; erst Read_Savegame legt sie an) und keinen Helden. Deshalb
    // verarbeitet die Wartephase nur ihre eigenen Pakete (SYNC_LOBBY), den Speicher-Abgleich (LOAD_REQ, FILE_*) und den
    // Log-Versand; alles andere wird verworfen und nach dem Laden ohnehin neu abgeglichen (WORLD_REQUEST, RESYNC, Reinit).

    // Ganz am Ende von LoadEnd (Plugin.cpp): Wartephase beenden.
    void CoopLobbyAfterLoad();
    bool CoopRuleSaves();   // v75: CoopDifficulty.cpp - Spielstaende gemeinsam (Regel des Hosts)
    // v77: Vorschau in der Liste "Spielstand waehlen" (CoopSavePreview.cpp)
    void CoopPreviewPrefetch();
    void CoopSavePreviewPulse(zCMenu* menu);
    void CoopPreviewReceive(json& e, const std::string& from, const std::string& op);
    void CoopPreviewReset();

    // v58: Fassung in den Versionszeilen der Menues (Game_MenuLoop, auch im Spiel ueber Esc)
    void CoopVersionMenuPulse();
    // v59: Esc im Hauptmenue ohne geladenes Spiel abfangen (sonst schwarzer Bildschirm)
    void CoopMainMenuEscGuard();
    // v87: CoopSteam.cpp - "Spiel beitreten" (IP) vergisst das letzte Steam-Ziel; Anzeige des Steam-Ziels (leer = IP)
    void CoopSteamForgetLast();
    std::string CoopSteamTargetText();
    bool CoopSteamAvailable();   // v88: Steam angemeldet
    std::string CoopSteamQuickText();   // v94: Text des Steam-Knopfs auf der COOP-Startseite

#if ENGINE == Engine_G1

#define COOP_AE "\xE4"
#define COOP_OE "\xF6"
#define COOP_UE "\xFC"

    struct CoopLobbyPeer {
        long long since = 0;          // verbunden seit
        bool hello = false;
        bool verOk = false;
        bool haveKnown = false;
        std::string ver, othello;
        std::set<std::string> have;   // Kennungen der Host-Staende, zu denen dieser Client einen eigenen Helden hat
        std::set<std::string> runs;   // v93: Durchlaeufe, die dieser Client kennt
        std::string nick;             // v93: Spielername (Durchlauf-Kennung)
    };

    struct CoopLobbyState {
        bool active = false;          // Wartephase laeuft (bis zum Laden)
        int role = 0;                 // 1 Host, 2 Client (zuletzt im Menue gewaehlt)
        // Host
        std::map<std::string, CoopLobbyPeer> peers;
        std::set<std::string> listSids;   // v93: eigene gemeinsame Staende (Archiv und Slots), dem Mitspieler geschickt
        // v93: im Spiel (beide Rollen) - was der Partner von meinen Staenden hat und welche Durchlaeufe er kennt
        std::set<std::string> gameHave, gameRuns;
        bool gameHaveKnown = false;
        long long gameAskMs = 0;
        // Client
        bool helloSent = false, hostHello = false, hostVerOk = false, hostInGame = false;
        bool hostTakes = false;       // v71: Host spielt schon und schickt den laufenden Stand
        std::string hostWait;         // v71: warum der Host die gemeinsame Sicherung verschiebt
        std::string hostVer, hostOthello;
        long long connectedMs = 0;    // Client: Verbindung steht seit
        bool clientFailed = false, clientLost = false;
        bool loadPending = false, allowLoad = false, loading = false;
        int loadSlot = -1;
        // v91: gemeinsames neues Spiel - Othellos Auswahl des Hosts (CLOSE-Text); unser eigenes ForceSelAction durchlassen
        std::string ngAct;
        std::string ngRun;            // v93: Durchlauf des Hosts
        bool allowNewGame = false;
        // v55: Spielstand-Liste (MENU_COOP_LOAD) - Zeile -> Stand; Host laedt eine Zeile ueber ForceSelAction im naechsten Takt
        // v93: Staende aus dem Coop-Archiv (CoopSaveSyncListJoint), sichtbar 10 Zeilen ab rowTop - rows = sichtbarer Ausschnitt
        std::vector<CoopListEntry> rows;
        std::vector<CoopListEntry> allRows;
        int rowTop = 0;
        int localRows = 0;            // v94: davon nur lokal (Schalter "Lokale Staende")
        std::string hostNick;         // v93: Client - Spielername des Hosts
        int hostRowSlot = -1;
        bool hostRowLoad = false;
        long long rowsMs = 0;
        zCMenu* rowsMenu = NULL;
        // Anzeige
        std::string note;             // Hinweis (z. B. gesperrte Auswahl), einige Sekunden sichtbar
        long long noteUntil = 0;
        std::string lastLog1, lastLog2;
        zCMenu* shownMenu = NULL;
        std::string shown1, shown2;
        std::string shownQuick;       // v94: Text des Steam-Knopfs (Log bei Wechsel)
        long long shownMs = 0;
    };

    static CoopLobbyState Lobby;
    static bool ListShowLocal = false;   // v93: Liste "Spielstand waehlen" - auch lokale Staende des Durchlaufs (Schalter)
    static bool LobbyOpenSteamMenu = false;   // v94: Steam-Knopf als Host - MENU_COOP_STEAM verschachtelt oeffnen (CoopSteam.cpp)
    // v91: Host - Othellos Schwierigkeit gewaehlt, MENU_COOP_NG folgt (zCMenu_HandleSelAction_Coop oeffnet es verschachtelt)
    static std::string NgPendingAct;
    static std::string NgShownAct;   // Othellos Text, solange MENU_COOP_NG offen ist (Hinweiszeile)
    static bool NgStartChosen = false;
    static bool LobbyMenuLoopLogged = false;
    static bool LobbyMainChecked = false;

    static void LobbyLog(const char* fmt, ...) {
        char buf[1024];
        va_list ap;
        va_start(ap, fmt);
        _vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        buf[sizeof(buf) - 1] = 0;
        OthelloCompatLog("[%lld s, %s] Mehrspieler-Menue: %s", DiagSeconds(), DiagRole(), buf);
    }

    static std::string LobbyUpper(const zSTRING& s) {
        std::string r = s.ToChar();
        for (auto& c : r)
            c = (char)toupper((unsigned char)c);
        return r;
    }

    // Othello-Skriptstand: Zeitstempel und Datengroesse aus dem VDF-Kopf (gleich auf jedem PC mit derselben Datei).
    static std::string LobbyOthelloStamp() {
        static std::string stamp;
        if (!stamp.empty())
            return stamp;
        std::string root = GothicExeFolderPath;
        const size_t p = root.find_last_of("\\/");
        if (p != std::string::npos)
            root = root.substr(0, p);
        const char* files[] = { "\\Data\\ScriptsOthello4.0_de.vdf", "\\Data\\ScriptsOthello4.0.vdf" };
        for (const char* name : files) {
            FILE* f = fopen((root + name).c_str(), "rb");
            unsigned char hdr[296];
            char part[64] = "-";
            if (f) {
                if (fread(hdr, 1, sizeof(hdr), f) == sizeof(hdr)) {
                    unsigned int ts = 0, size = 0;
                    memcpy(&ts, hdr + 280, 4);
                    memcpy(&size, hdr + 284, 4);
                    _snprintf(part, sizeof(part), "%08X/%u", ts, size);
                    part[sizeof(part) - 1] = 0;
                }
                fclose(f);
            }
            stamp += (stamp.empty() ? "" : ";") + std::string(part);
        }
        return stamp;
    }

    static void LobbySend(json& j) {
        j["type"] = SYNC_LOBBY;
        j["id"] = MyselfId.ToChar();
        CoopTraceCount(TRACE_SENT, SYNC_LOBBY);
        ReadyToSendJsons.enqueue(j);
    }

    static void LobbySendHello() {
        json j;
        j["op"] = "hello";
        j["ver"] = COOP_OTHELLO_FASSUNG;
        j["coop"] = COOP_VERSION;
        j["othello"] = LobbyOthelloStamp();
        j["pc"] = CoopSaveSyncPcId();   // v76: feste PC-Kennung (Spielstaende gehoeren dem PC, nicht der Rolle)
        j["nick"] = MyNickname.ToChar();   // v93: fuer die Durchlauf-Kennung (Datum_Host_Mitspieler)
        LobbySend(j);
    }

    // Kennungen der eigenen gemeinsamen Staende an den Partner - er sagt, zu welchen er einen eigenen Helden hat (op "have").
    // v93: aus dem Coop-Archiv und den Slots (CoopSaveSyncListJoint), nicht nur aus Othellos 15 Slots; auch im Spiel, beide Rollen.
    static void LobbySendSlots() {
        std::vector<CoopListEntry> own;
        CoopSaveSyncListJoint(own);
        Lobby.listSids.clear();
        json list = json::array();
        for (auto const& e : own) {
            if (e.sid.empty())
                continue;
            Lobby.listSids.insert(e.sid);
            json x;
            x["slot"] = e.slot;
            x["sid"] = e.sid;
            list.push_back(x);
        }
        json j;
        j["op"] = "slots";
        j["slots"] = list;
        LobbySend(j);
        LobbyLog("%s: %u eigene gemeinsame St" COOP_AE "nde an den Partner geschickt", ServerThread ? "Host" : "Client", (unsigned)Lobby.listSids.size());
    }

    // Antwort auf "slots": welche dieser Kennungen ich habe, dazu meine Durchlaeufe (fuer "lokale Staende" beim Partner)
    static void LobbyAnswerSlots(json& e) {
        json have = json::array();
        int n = 0, k = 0;
        if (e.contains("slots") && e["slots"].is_array()) {
            for (auto& s : e["slots"]) {
                if (!s.contains("sid") || !s["sid"].is_string())
                    continue;
                n++;
                const std::string sid = s["sid"].get<std::string>();
                if (CoopSaveSyncHasOwnHero(sid)) {
                    have.push_back(sid);
                    k++;
                }
            }
        }
        std::set<std::string> runs;
        CoopSaveSyncKnownRuns(runs);
        json r = json::array();
        for (auto const& x : runs)
            r.push_back(x);
        json j;
        j["op"] = "have";
        j["sids"] = have;
        j["runs"] = r;
        LobbySend(j);
        LobbyLog("zu %d von %d St" COOP_AE "nden des Partners einen eigenen Helden, %u Durchl" COOP_AE "ufe gemeldet", k, n, (unsigned)runs.size());
    }

    static void LobbyReadHave(json& e, std::set<std::string>& have, std::set<std::string>& runs) {
        have.clear();
        runs.clear();
        if (e.contains("sids") && e["sids"].is_array())
            for (auto& s : e["sids"])
                if (s.is_string())
                    have.insert(s.get<std::string>());
        if (e.contains("runs") && e["runs"].is_array())
            for (auto& s : e["runs"])
                if (s.is_string())
                    runs.insert(s.get<std::string>());
    }

    static bool LobbyPeersReady(std::string& why) {
        if (!ServerThread || CoopServerState != 1) { why = CoopTr("Der Host l" COOP_AE "uft noch nicht.", "The host is not running yet."); return false; }
        if (Lobby.peers.empty()) { why = CoopTr("Noch kein Mitspieler verbunden.", "No player connected yet."); return false; }
        for (auto& p : Lobby.peers) {
            if (!p.second.hello && GetCurrentMs() > p.second.since + 8000) {
                why = p.first + CoopTr(" antwortet nicht auf den Men" COOP_UE "-Abgleich - Fassung vor v40 oder schon im Spiel?", " does not answer the menu handshake - version before v40 or already in game?");
                return false;
            }
            if (!p.second.hello) { why = p.first + CoopTr(" ist verbunden, Abgleich l" COOP_AE "uft noch.", " is connected, handshake running."); return false; }
            if (!p.second.verOk) { why = p.first + CoopTr(" hat Fassung ", " has version ") + p.second.ver + CoopTr(", hier ", ", here ") + COOP_OTHELLO_FASSUNG + CoopTr(" - bitte gleiche Fassung einspielen.", " - please install the same version."); return false; }
            if (!p.second.haveKnown) { why = CoopTr("Pr" COOP_UE "fe die gemeinsamen Spielst" COOP_AE "nde bei ", "Checking shared saves with ") + p.first + " ..."; return false; }
        }
        return true;
    }

    static void LobbyNote(const std::string& text, zCMenu* menu) {
        Lobby.note = text;
        Lobby.noteUntil = GetCurrentMs() + 8000;
        LobbyLog("Hinweis: %s", text.c_str());
        if (menu)
            menu->PrintInformation(zSTRING(text.c_str()));
    }

    // --- Pakete --------------------------------------------------------------------------------------------------

    static void LobbyReceive(json& e, const std::string& from) {
        const std::string op = e.contains("op") && e["op"].is_string() ? e["op"].get<std::string>() : std::string();
        CoopTraceCount(TRACE_RECV, SYNC_LOBBY);
        if (op == "hello") {
            const std::string ver = e.contains("ver") && e["ver"].is_string() ? e["ver"].get<std::string>() : std::string("?");
            const int coop = e.contains("coop") && e["coop"].is_number() ? e["coop"].get<int>() : -1;
            const std::string oth = e.contains("othello") && e["othello"].is_string() ? e["othello"].get<std::string>() : std::string("?");
            const bool ok = ver == COOP_OTHELLO_FASSUNG && coop == COOP_VERSION;
            if (e.contains("pc") && e["pc"].is_string())   // v76
                CoopSaveSyncNotePc(e["pc"].get<std::string>(), from);
            const std::string nick = e.contains("nick") && e["nick"].is_string() ? e["nick"].get<std::string>() : std::string();   // v93
            CoopSaveSyncNotePartnerNick(nick);
            if (ServerThread) {
                CoopLobbyPeer& p = Lobby.peers[from];
                p.hello = true;
                p.ver = ver;
                p.othello = oth;
                p.verOk = ok;
                p.nick = nick;
                LobbyLog("Host: %s meldet Fassung %s (GT %d), Othello %s - %s%s", from.c_str(), ver.c_str(), coop, oth.c_str(),
                    ok ? "passt" : "PASST NICHT", oth != LobbyOthelloStamp() ? " (Othello-Skripte verschieden!)" : "");
                if (ok)
                    LobbySendSlots();
            }
            else if (ClientThread) {
                Lobby.hostHello = true;
                Lobby.hostVer = ver;
                Lobby.hostOthello = oth;
                Lobby.hostVerOk = ok;
                Lobby.hostInGame = e.contains("ingame");
                Lobby.hostTakes = Lobby.hostInGame && e.contains("take");
                Lobby.hostWait.clear();
                Lobby.hostNick = nick;
                LobbyLog("Client: Host meldet Fassung %s (GT %d), Othello %s - %s%s%s", ver.c_str(), coop, oth.c_str(),
                    ok ? "passt" : "PASST NICHT", oth != LobbyOthelloStamp() ? " (Othello-Skripte verschieden!)" : "",
                    Lobby.hostTakes ? ", Host ist schon im Spiel und schickt den laufenden Stand" : (Lobby.hostInGame ? ", Host ist schon im Spiel" : ""));
                if (!Lobby.helloSent && !(MyselfId == "_player_")) {
                    LobbySendHello();
                    Lobby.helloSent = true;
                }
            }
            return;
        }
        if (op == "pv" || op == "pvd") {   // v77: Vorschau eines Stands (Bild und Angaben des Mitspielers)
            CoopPreviewReceive(e, from, op);
            return;
        }
        if (op == "newgame" && ClientThread && !ServerThread) {   // v91: Host startet ein neues Spiel - mit derselben Schwierigkeit mit
            const std::string act = e.contains("act") && e["act"].is_string() ? e["act"].get<std::string>() : std::string();
            std::string up = act;
            for (auto& c : up)
                c = (char)toupper((unsigned char)c);
            if (up.compare(0, 12, "OTHELLO_NEW_") != 0 && up != "NEW_GAME") {
                LobbyLog("Client: neues Spiel des Hosts mit unbekannter Auswahl \"%s\" - nicht mitgestartet", act.c_str());
                return;
            }
            Lobby.ngAct = act;
            Lobby.ngRun = e.contains("run") && e["run"].is_string() ? e["run"].get<std::string>() : std::string();   // v93: Durchlauf
            if (Lobby.ngRun.find_first_of("\\/:*?\"<>|") != std::string::npos || Lobby.ngRun.find("..") != std::string::npos)
                Lobby.ngRun.clear();
            LobbyLog("Client: der Host startet ein neues Spiel (%s, Durchlauf %s) - starte mit", act.c_str(), Lobby.ngRun.c_str());
            return;
        }
        if (op == "introskip")   // v91: im Menue ohne Bedeutung (wirkt im Netzfaden, CoopNewGame.cpp)
            return;
        if (op == "wait" && ClientThread && !ServerThread) {   // v71: Host verschiebt die gemeinsame Sicherung
            Lobby.hostWait = e.contains("why") && e["why"].is_string() ? e["why"].get<std::string>() : std::string("beschaeftigt");
            LobbyLog("Client: Host verschiebt die gemeinsame Sicherung (%s) - neuer Versuch in 1 Minute", Lobby.hostWait.c_str());
            return;
        }
        if (op == "slots" && ClientThread && !ServerThread) {
            LobbyAnswerSlots(e);   // v93: mit den eigenen Durchlaeufen
            return;
        }
        if (op == "have" && ServerThread) {
            CoopLobbyPeer& p = Lobby.peers[from];
            LobbyReadHave(e, p.have, p.runs);
            p.haveKnown = true;
            LobbyLog("Host: %s hat zu %u von %u St" COOP_AE "nden einen eigenen Helden, kennt %u Durchl" COOP_AE "ufe", from.c_str(),
                (unsigned)p.have.size(), (unsigned)Lobby.listSids.size(), (unsigned)p.runs.size());
            return;
        }
    }

    static bool LobbyParse(ENetEvent& ev, json& j) {
        if (!ev.packet)
            return false;
        bool ok = true;
        try {
            const std::uint8_t* d = (const std::uint8_t*)ev.packet->data;
            std::vector<std::uint8_t> bytes(d, d + ev.packet->dataLength);
            j = json::from_bson(bytes);
        }
        catch (std::exception& ex) {
            CoopErrorLog("Mehrspieler-Menue", ex.what(), "Paket lesen");
            ok = false;
        }
        enet_packet_destroy(ev.packet);
        ev.packet = NULL;
        return ok && j.contains("type") && j["type"].is_number();
    }

    static void LobbyDispatch(json& j, const std::string& from) {
        const int type = j["type"].get<int>();
        if (type == SYNC_LOBBY)
            LobbyReceive(j, from);
        else if (CoopSaveSyncHandles(type)) {   // LOAD_REQ, FILE_* (Uebernahme beim Client)
            CoopTraceCount(TRACE_RECV, type);
            CoopSaveSyncReceive(j, from);
        }
        else if (CoopLogShipHandles(type)) {
            CoopTraceCount(TRACE_RECV, type);
            CoopLogShipReceive(j, from);
        }
        else if (type == SYNC_NPC_LEAD) {   // v72: Kampf-Uebergabe im Hauptmenue ablehnen (der Host behaelt den NPC)
            CoopLeadReceive(j, from);
        }
        // alles andere braucht eine Welt - verwerfen (nach dem Laden gleichen WORLD_REQUEST, RESYNC und Reinit neu ab)
    }

    static void LobbyServerEvent(ENetEvent& ev) {
        switch (ev.type) {
        case ENET_EVENT_TYPE_CONNECT:
        {
            // wie ProcessServerPacket, aber ohne Figur (noch keine Welt): die legt das erste Paket nach dem Laden an
            string name = string::Combine("FRIEND_%i", GetFreePlayerId());
            json j;
            j["id"] = "HOST";
            j["type"] = SYNC_PLAYER_NAME;
            j["name"] = name.ToChar();
            j["connectId"] = ev.peer->connectID;
            ReadyToSendJsons.enqueue(j);
            CoopSaveSyncPeerJoined();
            auto d = new PeerData();
            d->name = name;
            ev.peer->data = d;
            CoopLobbyPeer np;
            np.since = GetCurrentMs();
            Lobby.peers[std::string(name.ToChar())] = np;
            LobbySendHello();
            LobbyLog("Host: %s hat sich verbunden", name.ToChar());
            break;
        }
        case ENET_EVENT_TYPE_RECEIVE:
        {
            PeerData* pd = (PeerData*)ev.peer->data;
            json j;
            if (!LobbyParse(ev, j) || !pd)
                break;
            const std::string from = pd->name.ToChar();
            j["id"] = from;
            LobbyDispatch(j, from);
            break;
        }
        case ENET_EVENT_TYPE_DISCONNECT:
        {
            PeerData* pd = (PeerData*)ev.peer->data;
            if (!pd)
                break;
            const std::string from = pd->name.ToChar();
            Lobby.peers.erase(from);
            CoopSaveSyncPeerLeft();
            ev.peer->data = NULL;
            LobbyLog("Host: %s hat die Verbindung getrennt", from.c_str());
            break;
        }
        default:
            break;
        }
    }

    static void LobbyClientEvent(ENetEvent& ev) {
        if (ev.type != ENET_EVENT_TYPE_RECEIVE) {
            if (ev.packet)
                enet_packet_destroy(ev.packet);
            return;
        }
        CurrentPing = ev.peer->roundTripTime;
        json j;
        if (!LobbyParse(ev, j))
            return;
        const int type = j["type"].get<int>();
        const std::string from = j.contains("id") && j["id"].is_string() ? j["id"].get<std::string>() : std::string("HOST");
        if (type == SYNC_PLAYER_NAME) {
            if (j.contains("connectId") && j["connectId"].get<enet_uint32>() == ev.peer->connectID && j.contains("name")) {
                MyselfId = j["name"].get<std::string>().c_str();
                LobbyLog("Client: vom Host als %s angemeldet", MyselfId.ToChar());
                if (!Lobby.helloSent) {
                    LobbySendHello();
                    Lobby.helloSent = true;
                }
            }
            return;
        }
        LobbyDispatch(j, from);
    }

    static void LobbyProcessPackets() {
        while (!ReadyToBeReceivedPackets.isEmpty()) {
            ENetEvent ev = ReadyToBeReceivedPackets.dequeue();
            // wie PacketProcessorLoop (v39): Ereignisse eines endenden Fadens verwerfen, ohne den Peer anzufassen
            const bool ending = (ClientThread && !ServerThread && (CoopClientState >= 3 || CoopClientStop))
                || (ServerThread && (CoopServerStop || CoopServerState == 5));
            if (ending || (!ServerThread && !ClientThread)) {
                if (ev.type == ENET_EVENT_TYPE_RECEIVE && ev.packet)
                    enet_packet_destroy(ev.packet);
                continue;
            }
            if (ServerThread)
                LobbyServerEvent(ev);
            else
                LobbyClientEvent(ev);
        }
    }

    // --- Befehle aus MENU_COOP ---------------------------------------------------------------------------------------

    static void LobbyResetClient() {
        Lobby.helloSent = Lobby.hostHello = Lobby.hostVerOk = Lobby.hostInGame = Lobby.hostTakes = false;
        Lobby.hostWait.clear();
        Lobby.connectedMs = 0;
        Lobby.hostVer.clear();
        Lobby.hostOthello.clear();
        Lobby.clientFailed = Lobby.clientLost = false;
        Lobby.loadPending = Lobby.allowLoad = Lobby.loading = false;
        Lobby.loadSlot = -1;
        Lobby.ngAct.clear();
        Lobby.allowNewGame = false;
    }

    static void LobbyCommand(const std::string& cmd, zCMenu* menu) {
        LobbyLog("Befehl %s (Host %s, Client %s, Zustand Client %ld / Host %ld)", cmd.c_str(), ServerThread ? "an" : "aus",
            ClientThread ? "an" : "aus", (long)CoopClientState, (long)CoopServerState);
        if (cmd == "COOP_HOST") {
            Lobby.active = true;
            Lobby.role = 1;
            CoopSaveSyncSetMenu(true);
            if (ServerThread) {
                LobbyNote(CoopTr("Der Host l" COOP_AE "uft schon.", "The host is already running."), menu);
                return;
            }
            if (ClientThread) {   // wie Shift+F1 (v39): Client beenden, dann Host
                CoopPendingRole = 1;
                CoopClientStop = 1;
                LobbyLog("Wechsel zum Host - die Client-Verbindung wird beendet");
                return;
            }
            Lobby.peers.clear();
            CoopStartServer();
            LobbyLog("Host gestartet (Port %d), Sitzung %p, Spielstand-Verwaltung %p", ConnectionPort, (void*)ogame,
                (void*)(gameMan ? gameMan->savegameManager : NULL));
            return;
        }
        if (cmd == "COOP_CLIENT") {
            Lobby.active = true;
            Lobby.role = 2;
            CoopSaveSyncSetMenu(true);
            if (ServerThread) {
                if (!Lobby.peers.empty() || CoopSaveSyncPeers() > 0) {
                    LobbyNote(CoopTr("Als Host ist ein Mitspieler verbunden - erst \"Verbindung trennen\".", "A player is connected to your host - \"Disconnect\" first."), menu);
                    return;
                }
                CoopPendingRole = 2;
                CoopServerStop = 1;
                LobbyResetClient();
                LobbyLog("Wechsel zum Client - der Host wird beendet");
                return;
            }
            if (ClientThread) {
                LobbyNote(CoopClientState == 2 ? CoopTr("Schon mit dem Host verbunden.", "Already connected to the host.") : CoopTr("Verbindung wird noch aufgebaut ...", "Still connecting ..."), menu);
                return;
            }
            LobbyResetClient();
            CoopSteamForgetLast();   // v87: "Spiel beitreten" ist IP - Shift+F2 danach auch
            CoopStartClient();   // v55: liest IP/Port/Name aus dem COOP-Menue (CoopSettingsRefresh)
            LobbyLog("Client verbindet mit %s:%d als %s, Sitzung %p, Spielstand-Verwaltung %p", CoopServerIp.c_str(), ConnectionPort,
                MyNickname.ToChar(), (void*)ogame, (void*)(gameMan ? gameMan->savegameManager : NULL));
            return;
        }
        if (cmd.compare(0, 9, "COOP_ROW_") == 0) {   // v55: Zeile der Spielstand-Liste
            const int n = atoi(cmd.c_str() + 9);
            std::string why;
            if (!ServerThread) {
                LobbyNote(ClientThread ? CoopTr("Den Spielstand w" COOP_AE "hlt der Host.", "The host chooses the save.") : CoopTr("Erst \"Spiel hosten\" oder beitreten (Steam/IP).", "First \"Host game\" or join (Steam/IP)."), menu);
                return;
            }
            if (n < 0 || n >= (int)Lobby.rows.size()) {
                LobbyNote(CoopTr("Leere Zeile.", "Empty row."), menu);
                return;
            }
            if (!LobbyPeersReady(why)) {
                LobbyNote(why, menu);
                return;
            }
            // v93: Stand aus dem Coop-Archiv - liegt er in keinem Slot, kommt er jetzt in einen Coop-Slot (vorher gesichert)
            const CoopListEntry& e = Lobby.rows[n];
            const int slot = e.sid.empty() ? e.slot : CoopSaveSyncPrepareSlot(e.sid, why);
            if (slot < 1) {
                LobbyNote(CoopTr("Stand nicht ladbar (", "Cannot load save (") + why + ").", menu);
                return;
            }
            Lobby.hostRowSlot = slot;
            LobbyLog("Host: Zeile %d = %s (Slot %d) gew" COOP_AE "hlt - lade im n" COOP_AE "chsten Takt, der Mitspieler l" COOP_AE "dt mit", n,
                e.sid.empty() ? "lokaler Stand" : e.sid.c_str(), slot);
            return;
        }
        if (cmd == "COOP_STOP") {
            CoopPendingRole = 0;
            if (ClientThread)
                CoopClientStop = 1;
            if (ServerThread)
                CoopServerStop = 1;   // im Menue auch mit Mitspieler (ENet trennt sie); CoopServerEnded setzt die Zahl zurueck
            Lobby.peers.clear();
            LobbyResetClient();
            Lobby.role = 0;
            LobbyNote(CoopTr("Verbindung getrennt.", "Disconnected."), menu);
            return;
        }
        LobbyNote(CoopTr("Unbekannter Befehl ", "Unknown command ") + cmd, menu);
    }

    void CoopTitleRequest(zCMenu* menu);   // v82: CoopTitle.cpp - "Hauptmenue" im Esc-Menue
    bool CoopSteamMenuCommand(const std::string& cmd, zCMenu* menu);   // v87: CoopSteam.cpp - MENU_COOP_STEAM (auch im Spiel)
    bool CoopSaveSyncAskLoad(int slot);   // v90: CoopSaveSync.cpp - Client im Spiel: erst den Host laden lassen (einmal laden)
    void CoopMenuCloseRequest();          // v90: CoopTitle.cpp - Menues im naechsten Menuebild schliessen, ohne zu laden
    void CoopSaveSyncLoadSoon(int slot);  // v91: CoopSaveSync.cpp - Laden nach dem Schliessen der Menues (Host bzw. ohne Coop)
    void CoopNewGameBegin(bool host, const std::string& run);   // v91: CoopNewGame.cpp - gemeinsamer Start; v93 mit Durchlauf

    // v91: "Spielstand waehlen" im Esc-Menue (MENU_COOP_LOAD im Spiel) - Zeile gewaehlt
    // v93: Staende aus dem Coop-Archiv ueber die Kennung (Mitspieler: Ladewunsch; Host/allein: in einen Slot legen und laden)
    static void LobbyIngameRow(int n, zCMenu* menu) {
        if (n < 0 || n >= (int)Lobby.rows.size()) {
            LobbyNote(CoopTr("Leere Zeile.", "Empty row."), menu);
            return;
        }
        const CoopListEntry e = Lobby.rows[n];
        if (ClientThread && !ServerThread && !e.sid.empty() && CoopSaveSyncAskLoadSid(e.sid)) {   // wie v90: der Host laedt, hier einmal mit
            CoopMenuCloseRequest();
            LobbyNote(CoopTr("Ladewunsch an den Host geschickt - es wird gleich einmal f" COOP_UE "r beide geladen.", "Load request sent to the host - it will load once for both."), menu);
            return;
        }
        std::string why;
        const int slot = e.sid.empty() ? e.slot : CoopSaveSyncPrepareSlot(e.sid, why);
        if (slot < 1) {
            LobbyNote(CoopTr("Stand nicht ladbar (", "Cannot load save (") + why + ").", menu);
            return;
        }
        CoopSaveSyncLoadSoon(slot);   // Host (der Mitspieler laedt mit), ohne Coop bzw. in der Pause nur fuer mich
        CoopMenuCloseRequest();
        LobbyLog("Esc-Men" COOP_UE ": Zeile %d = %s (Slot %d) - lade nach dem Schlie" "\xDF" "en der Men" COOP_UE "s", n,
            e.sid.empty() ? "lokaler Stand" : e.sid.c_str(), slot);
    }

    // --- Haken auf zCMenu::HandleSelAction ----------------------------------------------------------------------------
    // Rueckgabe true: selbst behandelt, result = Rueckgabe an zCMenu::HandleAction (0 = Menue bleibt offen).
    static bool LobbyHandleSelAction(zCMenu* menu, int action, const zSTRING& s, int& result) {
        const std::string arg = LobbyUpper(s);
        result = 0;
        if (action == SEL_ACTION_EXECCOMMANDS && arg == "COOP_TITLE") {   // v82: "Ja" in MENU_COOP_TITLE (Esc-Menue im Spiel)
            CoopTitleRequest(menu);
            return true;
        }
        if (action == SEL_ACTION_EXECCOMMANDS && arg.compare(0, 11, "COOP_STEAM_") == 0) {   // v87: Steam-Menue, auch im Spiel
            CoopSteamMenuCommand(arg, menu);
            return true;
        }
        if (player && action == SEL_ACTION_EXECCOMMANDS && arg.compare(0, 9, "COOP_ROW_") == 0) {   // v91: Liste im Esc-Menue
            LobbyIngameRow(atoi(arg.c_str() + 9), menu);
            return true;
        }
        if (action == SEL_ACTION_EXECCOMMANDS && arg == "COOP_LIST_LOCAL") {   // v93: Schalter "Auch lokale Staende" (Haupt- und Esc-Menue)
            ListShowLocal = !ListShowLocal;
            Lobby.rowTop = 0;
            Lobby.rowsMs = 0;   // Liste im naechsten Bild neu
            LobbyLog("Liste: lokale St" COOP_AE "nde %s", ListShowLocal ? "an" : "aus");
            return true;
        }
        // v91: "Spiel starten" in MENU_COOP_NG - merken und das Menue wie ZURUECK schliessen (HandleSelAction-Fall 1, 0x004CFBB3:
        // m_exitState +0xC24 = 1, Rueckgabe 1); den Rest macht LobbyNewGameMenu nach dem verschachtelten Lauf
        if (action == SEL_ACTION_EXECCOMMANDS && arg == "COOP_NG_START") {
            if (menu && !NgShownAct.empty()) {
                NgStartChosen = true;
                *(int*)((char*)menu + 0xC24) = 1;
                result = 1;
            }
            return true;
        }
        if (action == SEL_ACTION_EXECCOMMANDS && arg.compare(0, 5, "COOP_") == 0) {
            if (player) {   // nur ausserhalb des Spiels (MENU_COOP gibt es nur im Hauptmenue)
                LobbyLog("Befehl %s im Spiel ignoriert", arg.c_str());
                return true;
            }
            LobbyCommand(arg, menu);
            return true;
        }
        // v89: Steam-Menue im Spiel (Einstellungen > COOP > Steam-Freunde) - Neues Spiel nur im Hauptmenue. v91: die Spielstand-
        // Liste geht jetzt auch im Spiel (CoopIngameLoadListPulse)
        if (player && action == SEL_ACTION_STARTMENU && menu && LobbyUpper(menu->name) == "MENU_COOP_STEAM" && arg == "MENU_NG") {
            LobbyNote(CoopTr("Neues Spiel nur im Hauptmen" COOP_UE " (Esc > Hauptmen" COOP_UE ").", "New game only from the main menu (Esc > Main menu)."), menu);
            return true;
        }
        // v55: "Spielstand waehlen" erst nach "Spiel hosten" bzw. "Spiel beitreten" (vorher ist der Eintrag leer)
        // v94: der Eintrag steht immer da (COOP-Startseite) - ohne Verbindung nur der Hinweis
        if (!player && action == SEL_ACTION_STARTMENU && arg == "MENU_COOP_LOAD" && !ServerThread && !ClientThread) {
            LobbyNote(CoopTr("Erst \"Spiel hosten\" oder beitreten (Steam/IP) - dann w" COOP_AE "hlt der Host den Stand f" COOP_UE "r beide.", "First \"Host game\" or join (Steam/IP) - then the host picks the save for both."), menu);
            return true;
        }
        // v94: "Neues Spiel" (COOP-Startseite, Othellos Hauptmenue) als verbundener Mitspieler - gleich sagen, wer startet (bisher
        // erst nach Othellos Auswahl im CLOSE-Fall unten)
        if (!player && action == SEL_ACTION_STARTMENU && arg == "MENU_NG" && ClientThread && !ServerThread && Lobby.active) {
            LobbyNote(CoopTr("Verbunden: das neue Spiel startet der Host (du startest mit) - oder erst \"Verbindung trennen\".", "Connected: the host starts the new game (you join in) - or \"Disconnect\" first."), menu);
            return true;
        }
        // v90 (Test v89: "es hat zweimal geladen beim Client"): Client waehlt im Spiel im Ladefenster einen gemeinsamen Stand -
        // nicht selbst laden, sondern den Host fragen (CoopSaveSyncAskLoad); Menues ohne Laden schliessen
        if (player && action == SEL_ACTION_CLOSE && (arg == "SAVEGAME_LOAD" || arg == "OTHELLO_SAVEGAME_LOAD") && ClientThread && !ServerThread) {
            int slot = -1;
            if (menu && *(DWORD*)menu == 0x007D115C)   // oCMenuSavegame: m_selSlot +0xCCC
                slot = *(int*)((char*)menu + 0xCCC);
            else if (gameMan && gameMan->menu_load_savegame)
                slot = gameMan->menu_load_savegame->m_selSlot;
            if (CoopSaveSyncAskLoad(slot)) {
                CoopMenuCloseRequest();
                LobbyNote(CoopTr("Ladewunsch an den Host geschickt - es wird gleich einmal f" COOP_UE "r beide geladen.", "Load request sent to the host - it will load once for both."), menu);
                return true;
            }
        }
        if (player || !Lobby.active)
            return false;   // im Spiel und ohne Wartephase: alles wie bisher
        const bool host = ServerThread != NULL;
        const bool client = ClientThread != NULL && !host;
        std::string why;

        if (action == SEL_ACTION_STARTMENU && arg == "MENU_SAVEGAME_LOAD" && menu &&
            (LobbyUpper(menu->name) == "MENU_COOP_LOAD" || LobbyUpper(menu->name) == "MENU_COOP")) {
            if (client && !CoopRuleSaves())
                return false;   // v75: Spielstaende getrennt - der Client laedt selbst
            if (client) {
                LobbyNote(CoopTr("Den Spielstand w" COOP_AE "hlt der Host.", "The host chooses the save."), menu);
                return true;
            }
            if (!LobbyPeersReady(why)) {
                LobbyNote(why, menu);
                return true;
            }
            return false;   // Ladefenster oeffnen; die Wahl prueft der CLOSE-Fall
        }
        if (action != SEL_ACTION_CLOSE)
            return false;

        const bool load = arg == "OTHELLO_SAVEGAME_LOAD" || arg == "SAVEGAME_LOAD";
        const bool newGame = arg.compare(0, 12, "OTHELLO_NEW_") == 0 || arg == "NEW_GAME";
        if (!load && !newGame)
            return false;   // z. B. Beenden
        if (client) {
            if (load && Lobby.allowLoad) {   // unser eigenes ForceSelAction nach dem Ladeauftrag des Hosts
                Lobby.allowLoad = false;
                LobbyLog("Client: Men" COOP_UE " schlie" "\xDF" "t, lade Slot %d", Lobby.loadSlot);
                return false;
            }
            if (newGame && Lobby.allowNewGame) {   // v91: unser eigenes ForceSelAction nach "newgame" des Hosts
                Lobby.allowNewGame = false;
                CoopNewGameBegin(false, Lobby.ngRun);
                LobbyLog("Client: Men" COOP_UE " schlie" "\xDF" "t - neues Spiel wie der Host (%s)", s.ToChar());
                return false;
            }
            if (load && !CoopRuleSaves()) {   // v75: Spielstaende getrennt (Einstellung des Hosts) - eigener Stand, verbunden bleiben
                LobbyLog("Client: Spielst" COOP_AE "nde getrennt - l" COOP_AE "dt selbst einen Stand (bleibt verbunden)");
                return false;
            }
            LobbyNote(newGame ? CoopTr("Verbunden: das neue Spiel startet der Host - oder erst \"Verbindung trennen\".", "Connected: the host starts the new game - or \"Disconnect\" first.")
                              : CoopTr("Verbunden: den Spielstand w" COOP_AE "hlt der Host - oder erst \"Verbindung trennen\".", "Connected: the host chooses the save - or \"Disconnect\" first."), menu);
            return true;
        }
        if (!host || (Lobby.peers.empty() && CoopSaveSyncPeers() <= 0))
            return false;   // Host ohne Mitspieler: wie bisher (verbindet sich jemand spaeter, gilt der Ablauf im Spiel)
        if (newGame) {   // v91: gemeinsames neues Spiel - erst die COOP-Schwierigkeit (MENU_COOP_NG), dann starten beide
            if (!LobbyPeersReady(why)) {
                LobbyNote(why, menu);
                return true;
            }
            NgPendingAct = s.ToChar();
            LobbyLog("Host: Othellos Auswahl %s - COOP-Schwierigkeit (MENU_COOP_NG) folgt", NgPendingAct.c_str());
            return true;
        }
        if (load && Lobby.hostRowLoad) {   // v55: unser ForceSelAction nach einer Zeile der Spielstand-Liste
            Lobby.hostRowLoad = false;
            Lobby.loading = true;
            return false;
        }
        if (!LobbyPeersReady(why)) {
            LobbyNote(why, menu);
            return true;
        }
        // v55 (Nutzer: "optional kann man auch Savegames laden"): auch Staende ohne gemeinsame Kennung bzw. ohne eigenen Helden
        // des Mitspielers - der Mitspieler nimmt dann seinen Helden, der zeitlich am naechsten gespeichert wurde (CoopSaveSync.cpp).
        const int slot = (gameMan && gameMan->menu_load_savegame) ? gameMan->menu_load_savegame->m_selSlot : -1;
        const std::string sid = slot >= 0 ? CoopSaveSyncSlotSid(slot) : std::string();
        std::string without;
        for (auto& p : Lobby.peers)
            if (sid.empty() || !p.second.have.count(sid))
                without += (without.empty() ? "" : ", ") + p.first;
        LobbyLog("Host: lade Slot %d (%s) - der Mitspieler l" COOP_AE "dt mit%s%s", slot, sid.empty() ? "ohne Coop-Kennung" : sid.c_str(),
            without.empty() ? ", jeder mit seinem Helden" : " - ohne eigenen Helden dazu: ", without.c_str());
        Lobby.loading = true;
        return false;
    }

    // v55: nach einem Eingabefeld des COOP-Menues - Werte uebernehmen, bei "Aussehen wie NPC" suchen (CoopLook.cpp)
    static void LobbyAfterInput(zCMenu* menu, const std::string& id) {
        if (id == "MENUITEM_COOP_LOOKNPC_INP") {
            const std::string note = CoopLookSearchFromMenu();
            if (!note.empty())
                LobbyNote(note, menu);
            return;
        }
        if (id == "MENUITEM_COOP_IP_INP" || id == "MENUITEM_COOP_PORT_INP" || id == "MENUITEM_COOP_NAME_INP") {
            CoopSettingsRefresh(true);
            char b[200];
            _snprintf(b, sizeof(b), CoopTr("Gemerkt: Host %s:%d, Name %s.", "Saved: host %s:%d, name %s."), CoopServerIp.c_str(), ConnectionPort, MyNickname.ToChar());
            b[sizeof(b) - 1] = 0;
            LobbyNote(b, menu);
        }
    }

    int __fastcall zCMenu_HandleSelAction_Coop(zCMenu* _this, void* vtable, int action, const zSTRING& s, zCMenuItem* item);
    CInvoke<int(__thiscall*)(zCMenu*, int, const zSTRING&, zCMenuItem*)> Ivk_zCMenu_HandleSelAction_Coop(0x004CF6E0, &zCMenu_HandleSelAction_Coop);
    // v91: Host hat Othellos Schwierigkeit gewaehlt - MENU_COOP_NG (COOP-Schwierigkeit fuer beide) verschachtelt laufen lassen wie
    // einen STARTMENU-Eintrag (HandleSelAction-Fall 2, 0x004CF8EF: Untermenue holen bzw. anlegen, vtable +0x10 Run; der Eintrag
    // wird dort nicht benutzt). "Spiel starten" (COOP_NG_START) merkt NgStartChosen und schliesst es wie ZURUECK. Danach Othellos
    // CLOSE wie gewaehlt (Fall 4, 0x004CFB23: schreibt den Text als menuAction in die Optionen - CGameManager::Menu wertet ihn
    // aus); der Mitspieler bekommt denselben Text (LOBBY newgame) und schliesst seine Menues damit. Ohne MENU_COOP_NG im
    // Menue-Skript (aelteres COOP_MENU.D) startet es gleich. Abbrechen: zurueck zu Othellos Auswahl.
    static int LobbyNewGameMenu(zCMenu* menu, const std::string& act, zCMenuItem* item) {
        zCParser* mp = zCMenu::GetParser();
        if (mp && mp->GetSymbol(zSTRING("MENU_COOP_NG"))) {
            NgStartChosen = false;
            NgShownAct = act;
            Ivk_zCMenu_HandleSelAction_Coop(menu, SEL_ACTION_STARTMENU, zSTRING("MENU_COOP_NG"), NULL);
            NgShownAct.clear();
            if (!NgStartChosen) {
                LobbyLog("Host: neues Spiel abgebrochen (MENU_COOP_NG) - zur" COOP_UE "ck zu Othellos Auswahl");
                return 0;
            }
            NgStartChosen = false;
        }
        else
            LobbyLog("Host: MENU_COOP_NG fehlt im Men" COOP_UE "-Skript (COOP_MENU.D vor v91?) - starte ohne COOP-Schwierigkeit");
        std::string why;
        if (!LobbyPeersReady(why)) {
            LobbyNote(why, menu);
            return 0;
        }
        // v93: neuer Durchlauf "JJJJ-MM-TT_hh-mm-ss_<Host>_<Mitspieler>" - beide legen die Staende dieses Spiels darunter ab
        const std::string run = CoopSaveSyncNewRun(Lobby.peers.empty() ? std::string() : Lobby.peers.begin()->second.nick);
        json j;
        j["op"] = "newgame";
        j["act"] = act;
        j["run"] = run;
        LobbySend(j);
        Lobby.loading = true;
        CoopNewGameBegin(true, run);
        LobbyLog("Host: neues Spiel gemeinsam (%s, Durchlauf %s) - der Mitspieler startet mit", act.c_str(), run.c_str());
        return Ivk_zCMenu_HandleSelAction_Coop(menu, SEL_ACTION_CLOSE, zSTRING(act.c_str()), item);
    }

    int __fastcall zCMenu_HandleSelAction_Coop(zCMenu* _this, void* vtable, int action, const zSTRING& s, zCMenuItem* item) {
        int result = 0;
        bool handled = false;
        try {
            handled = LobbyHandleSelAction(_this, action, s, result);
            if (handled && !NgPendingAct.empty()) {   // v91: Othellos "Neues Spiel" beim Host mit Mitspieler
                const std::string act = NgPendingAct;
                NgPendingAct.clear();
                result = LobbyNewGameMenu(_this, act, item);
            }
            // v94: Steam-Knopf der COOP-Startseite als Host ("Freunde ueber Steam einladen ...") - Steam-Freunde verschachtelt wie
            // ein STARTMENU-Eintrag (Fall 2, 0x004CF8EF; wie MENU_COOP_NG), Ergebnis wie bei einem normalen Untermenue zurueck
            if (handled && LobbyOpenSteamMenu) {
                LobbyOpenSteamMenu = false;
                zCParser* mp = zCMenu::GetParser();
                if (mp && mp->GetSymbol(zSTRING("MENU_COOP_STEAM")))
                    result = Ivk_zCMenu_HandleSelAction_Coop(_this, SEL_ACTION_STARTMENU, zSTRING("MENU_COOP_STEAM"), NULL);
            }
        }
        catch (std::exception& e) {
            CoopErrorLog("Mehrspieler-Menue", e.what(), "HandleSelAction");
        }
        catch (...) {
            CoopErrorLog("Mehrspieler-Menue", "Zugriffsfehler (SEH) oder unbekannte Ausnahme", "HandleSelAction");
        }
        if (handled)
            return result;
        const int r = Ivk_zCMenu_HandleSelAction_Coop(_this, action, s, item);
        if (action == SEL_ACTION_STARTITEM) {   // v55: Eingabefeld fertig (zCMenuItemInput::Run hat Gothic.ini geschrieben)
            try {
                LobbyAfterInput(_this, LobbyUpper(s));
            }
            catch (...) {
                CoopErrorLog("Mehrspieler-Menue", "Zugriffsfehler (SEH) oder unbekannte Ausnahme", "Eingabefeld");
            }
        }
        return r;
    }

    static void LobbyIngameSlots(json& e, const std::string& op);   // v93: weiter unten

    // Aus ProcessCoopPacket (im Spiel): ein Client im Hauptmenue meldet sich - "Host spielt schon" zurueckmelden.
    // v71: bei gleicher Fassung bekommt er den laufenden Stand (gemeinsame Sicherung, dann Ladeauftrag - CoopSaveSync.cpp).
    void CoopLobbyReceiveInGame(json& e, const std::string& id) {
        CoopTraceCount(TRACE_RECV, SYNC_LOBBY);
        const std::string op = e.contains("op") && e["op"].is_string() ? e["op"].get<std::string>() : std::string();
        if ((op == "pv" || op == "pvd") && !e.contains("by")) {   // v92: Vorschau in der Liste im Esc-Menue (CoopSavePreview.cpp)
            CoopPreviewReceive(e, id, op);
            return;
        }
        if ((op == "slots" || op == "have") && !e.contains("by")) {   // v93: welche Staende beide haben (Liste im Esc-Menue)
            LobbyIngameSlots(e, op);
            return;
        }
        if (op != "hello" || !ServerThread || e.contains("by"))
            return;
        const std::string ver = e.contains("ver") && e["ver"].is_string() ? e["ver"].get<std::string>() : std::string("?");
        const int coop = e.contains("coop") && e["coop"].is_number() ? e["coop"].get<int>() : -1;
        const bool ok = ver == COOP_OTHELLO_FASSUNG && coop == COOP_VERSION;
        const bool take = ok && CoopRuleSaves();   // v75: bei getrennten Spielstaenden kein gemeinsamer Stand
        if (e.contains("pc") && e["pc"].is_string())   // v76
            CoopSaveSyncNotePc(e["pc"].get<std::string>(), id);
        json j;
        j["op"] = "hello";
        j["ver"] = COOP_OTHELLO_FASSUNG;
        j["coop"] = COOP_VERSION;
        j["othello"] = LobbyOthelloStamp();
        j["pc"] = CoopSaveSyncPcId();   // v76
        j["ingame"] = 1;
        if (take)
            j["take"] = 1;
        LobbySend(j);
        LobbyLog("Host im Spiel: %s meldet sich aus dem Hauptmen" COOP_UE " (Fassung %s) - %s", id.c_str(), ver.c_str(),
            take ? "er bekommt den laufenden Stand" : (ok ? "Spielst" COOP_AE "nde getrennt, nur \"Host spielt schon\" gemeldet"
                                                          : "andere Fassung, nur \"Host spielt schon\" gemeldet"));
        if (take)
            CoopSaveSyncMenuPeerWantsState(id);
    }

    // Aus CoopDoLoad (CoopSaveSync.cpp): im Hauptmenue nicht direkt laden, sondern das Menue schliessen lassen.
    bool CoopLobbyLoadFromMenu(int slot) {
        if (player || !Lobby.active || !ClientThread || ServerThread)
            return false;
        Lobby.loadPending = true;
        Lobby.loadSlot = slot;
        LobbyLog("Client: Stand liegt in Slot %d bereit - schlie" "\xDF" "e das Men" COOP_UE " und lade", slot);
        return true;
    }

    // --- Anzeige -----------------------------------------------------------------------------------------------------

    static void LobbyStatus(std::string& l1, std::string& l2) {
        char b[400];
        l1.clear();
        l2.clear();
        const std::string steam = CoopSteamTargetText();   // v87: Verbindung ueber Steam - Name statt IP:Port
        std::string ip = CoopServerIp;   // v55: aus dem COOP-Menue
        if (steam.empty()) {
            _snprintf(b, sizeof(b), "%s:%d", CoopServerIp.c_str(), ConnectionPort);
            b[sizeof(b) - 1] = 0;
            ip = b;
        }
        else
            ip = steam;
        if (ServerThread) {
            std::string why;
            if (CoopServerState != 1) {
                l1 = CoopTr("Host startet ...", "Host starting ...");
            }
            else if (Lobby.peers.empty()) {
                // v88: mit Steam nimmt der Host beides an (Test v87: "Oben stand immer noch etwas mit Port")
                if (CoopSteamAvailable())
                    _snprintf(b, sizeof(b), CoopTr("Host l" COOP_AE "uft (Steam und Port %d) - warte auf einen Mitspieler ...", "Hosting (Steam and port %d) - waiting for a player ..."), ConnectionPort);
                else
                    _snprintf(b, sizeof(b), CoopTr("Host l" COOP_AE "uft (Port %d) - warte auf einen Mitspieler ...", "Hosting (port %d) - waiting for a player ..."), ConnectionPort);
                b[sizeof(b) - 1] = 0;
                l1 = b;
                l2 = CoopTr("Der Mitspieler tritt unter COOP bei: Steam-Knopf (du hostest) oder \"\xDC" "ber IP beitreten\".", "Your partner joins via COOP: Steam button or \"Join via IP\".");   // v87, v94
            }
            else if (!LobbyPeersReady(why)) {
                l1 = why;
            }
            else {
                const auto& p = *Lobby.peers.begin();
                int both = 0;
                for (auto& s : Lobby.listSids)   // v93: Staende aus dem Coop-Archiv
                    if (p.second.have.count(s))
                        both++;
                _snprintf(b, sizeof(b), CoopTr("%s verbunden (Fassung %s). Gemeinsame St" COOP_AE "nde: %d.", "%s connected (version %s). Shared saves: %d."), p.first.c_str(), p.second.ver.c_str(), both);
                b[sizeof(b) - 1] = 0;
                l1 = b;
                l2 = both > 0 ? CoopTr("Jetzt \"Spielstand w" COOP_AE "hlen\" - oder \"Neues Spiel\" (beide starten).", "Now \"Choose save\" - or \"New game\" (both start).")   // v91
                              : CoopTr("Kein gemeinsamer Stand - \"Spielstand w" COOP_AE "hlen\" (anderen laden) oder \"Neues Spiel\".", "No shared save - \"Choose save\" (load another) or \"New game\".");
                if (!CoopRuleSaves())   // v75
                    l2 = CoopTr("Spielst" COOP_AE "nde getrennt (Einstellungen > COOP > Abgleich): der Mitspieler l" COOP_AE "dt nicht mit.", "Separate saves (Settings > COOP > Sync): your partner does not load along.");
                if (p.second.othello != LobbyOthelloStamp())
                    l2 = CoopTr("Achtung: Othello-Skripte unterscheiden sich! ", "Warning: Othello scripts differ! ") + l2;
            }
        }
        else if (ClientThread) {
            if (CoopClientState <= 1) {
                _snprintf(b, sizeof(b), CoopTr("Verbinde mit %s ...", "Connecting to %s ..."), ip.c_str());
                b[sizeof(b) - 1] = 0;
                l1 = b;
            }
            else if (CoopClientState != 2) {
                l1 = CoopTr("Verbindung beendet ...", "Connection closed ...");
            }
            else if (MyselfId == "_player_" || !Lobby.hostHello) {
                l1 = CoopTr("Verbunden - Abgleich mit dem Host l" COOP_AE "uft ...", "Connected - handshake with the host ...");
                if (Lobby.connectedMs && GetCurrentMs() > Lobby.connectedMs + 8000)
                    l2 = CoopTr("Keine Antwort - hat der Host eine Fassung vor v40?", "No answer - does the host run a version before v40?");
            }
            else if (!Lobby.hostVerOk) {
                l1 = CoopTr("Der Host hat Fassung ", "The host has version ") + Lobby.hostVer + CoopTr(", hier ", ", here ") + COOP_OTHELLO_FASSUNG + CoopTr(" - bitte gleiche Fassung einspielen.", " - please install the same version.");
            }
            else if (Lobby.hostInGame && !Lobby.hostTakes) {
                l1 = CoopTr("Der Host spielt schon.", "The host is already playing.");
                l2 = CoopRuleSaves() ? CoopTr("\"Verbindung trennen\", einen Stand laden und im Spiel mit Shift+F2 verbinden.", "\"Disconnect\", load a save and connect in game with Shift+F2.")
                    : CoopTr("Spielst" COOP_AE "nde getrennt: selbst einen Stand laden (\"Spiel laden\") - die Verbindung bleibt.", "Separate saves: load a save yourself (\"Load game\") - you stay connected.");   // v75
            }
            else if (Lobby.loadPending || Lobby.loading) {
                l1 = CoopTr("Lade den gemeinsamen Spielstand ...", "Loading the shared save ...");
            }
            else {
                const std::string st = CoopSaveSyncMenuStatus();
                if (!st.empty())
                    l1 = st;
                else if (Lobby.hostInGame) {   // v71: Host spielt schon und schickt den laufenden Stand
                    l1 = CoopTr("Der Host spielt schon - er legt eine gemeinsame Sicherung an ...", "The host is already playing - creating a shared save ...");
                    l2 = Lobby.hostWait.empty() ? std::string(CoopTr("Danach wird sie hier geladen (eigener Held: der zuletzt gespeicherte).", "Then it is loaded here (your hero: the last one saved)."))
                                                : CoopTr("Der Host ist gerade besch" COOP_AE "ftigt (", "The host is busy (") + Lobby.hostWait + CoopTr(") - neuer Versuch in 1 Minute.", ") - retrying in 1 minute.");
                }
                else {
                    _snprintf(b, sizeof(b), CoopTr("Verbunden mit dem Host als %s (Fassung %s).", "Connected to the host as %s (version %s)."), MyselfId.ToChar(), Lobby.hostVer.c_str());
                    b[sizeof(b) - 1] = 0;
                    l1 = b;
                    l2 = CoopRuleSaves() ? CoopTr("Warte, bis der Host einen Spielstand w" COOP_AE "hlt oder ein neues Spiel startet.", "Wait until the host chooses a save or starts a new game.")   // v75, v91
                        : CoopTr("Spielst" COOP_AE "nde getrennt (Einstellung des Hosts): selbst einen Stand laden, im Spiel mit Shift+F2 verbinden.", "Separate saves (host setting): load a save yourself, connect in game with Shift+F2.");
                }
                if (Lobby.hostOthello != LobbyOthelloStamp() && l2.empty())
                    l2 = CoopTr("Achtung: Othello-Skripte unterscheiden sich!", "Warning: Othello scripts differ!");
            }
        }
        else if (CoopPendingRole) {
            l1 = CoopPendingRole == 1 ? CoopTr("Wechsel zum Host ...", "Switching to host ...") : CoopTr("Wechsel zum Client ...", "Switching to client ...");
        }
        else if (Lobby.clientFailed) {
            _snprintf(b, sizeof(b), CoopTr("Keine Verbindung zu %s.", "No connection to %s."), ip.c_str());
            b[sizeof(b) - 1] = 0;
            l1 = b;
            l2 = CoopTr("Erneut: Steam-Knopf bzw. \"\xDC" "ber IP beitreten\" > Verbinden.", "Retry: Steam button or \"Join via IP\" > Connect.");   // v94
        }
        else if (Lobby.clientLost) {
            l1 = CoopTr("Verbindung zum Host verloren.", "Lost connection to the host.");
            l2 = CoopTr("Erneut: Steam-Knopf bzw. \"\xDC" "ber IP beitreten\" > Verbinden.", "Retry: Steam button or \"Join via IP\" > Connect.");
        }
        else {
            l1 = CoopTr("Nicht verbunden.", "Not connected.");
        }
        if (!Lobby.note.empty() && GetCurrentMs() < Lobby.noteUntil)
            l2 = Lobby.note;
    }

    // Text eines Eintrags setzen - nur fuer Texteintraege (vtable zCMenuItemText 0x007D2E04, SetText +0x40 = 0x004D6430).
    // v78: Text auf die Breite des Eintrags kuerzen ("..."). Gothic setzt Menues in der Aufloesung aus Gothic.ini (hier 640x480) -
    // die kleine Schrift (font_old_10_white) fasst in 1500 Menue-Einheiten nur etwa 15 Zeichen, laengere Texte schnitt Gothic mitten
    // im Wort ab (Test v77: "Kapitel 1 T", "en Spielstand lad"). zCView::FontSize (0x006FFE80) misst in den Einheiten des Eintrags
    // selbst (8192 = seine Breite); ohne Schrift liefert es 0 (dann bleibt der Text).
    // v96 (Test v95: "oft wurde der Text nur als '...' angezeigt - zuerst auf Deutsch, dann nach dem Umstellen auf Englisch"):
    // FontSize rechnet ueber die aktuelle Bildpunkt-Breite des Eintrags (zCView::anx 0x006FC3A0: Breite * 8192 / psizex +0x58) -
    // beim Oeffnen eines Menues sind viele Eintraege noch nicht im Fenster (Menue-Maus v95: MENU_COOP_LOOK 4 von 23 "ondesk"),
    // ihre Breite stimmt dann nicht, und die Sprach-Umstellung setzte die Texte genau dann einmal - es blieb "...". Jetzt misst GT
    // mit der Schrift selbst (zCFont::GetFontX 0x006E0210, wie FontSize) gegen die Breite aus dem Skript (m_parDimX * Bildbreite /
    // 8192 - das Menuefenster deckt das Bild). Ohne Schrift oder Breite bleibt der Text, wie er ist.
    static int LobbyFitLogged = 0;

    static zCFont* LobbyItemFont(zCMenuItem* item) {
        return item->m_pFont ? item->m_pFont : item->font;   // m_pFont +0x360 (normale Schrift), sonst zCView::font +0x60
    }

    static int LobbyItemPx(zCMenuItem* item) {
        if (!screen || screen->psizex <= 0 || item->m_parDimX <= 0)
            return 0;
        return (int)((long long)item->m_parDimX * screen->psizex / 8192) * 94 / 100;   // etwas Rand
    }

    static int LobbyTextPx(zCFont* font, const std::string& s) {
        zSTRING z(s.c_str());
        return font->GetFontX(z);
    }

    // passt der Text in den Eintrag? (true, wenn es sich nicht messen laesst)
    static bool LobbyFits(zCMenuItem* item, const std::string& s) {
        zCFont* font = item ? LobbyItemFont(item) : NULL;
        const int px = item ? LobbyItemPx(item) : 0;
        return !font || px <= 0 || LobbyTextPx(font, s) <= px;
    }

    static std::string LobbyFitText(zCMenuItem* item, const std::string& text) {
        if (!item || text.size() < 4 || LobbyFits(item, text))
            return text;
        zCFont* font = LobbyItemFont(item);
        const int px = LobbyItemPx(item);
        size_t lo = 0, hi = text.size() - 1;   // laengster Anfang, der mit "..." passt
        while (lo < hi) {
            const size_t mid = (lo + hi + 1) / 2;
            if (LobbyTextPx(font, text.substr(0, mid) + "...") <= px)
                lo = mid;
            else
                hi = mid - 1;
        }
        std::string s = text.substr(0, lo);
        while (!s.empty() && (s[s.size() - 1] == ' ' || s[s.size() - 1] == ','))
            s.erase(s.size() - 1);
        if (LobbyFitLogged++ < 15)
            OthelloCompatLog("[%lld s, %s] Mehrspieler-Menue: Text gek" COOP_UE "rzt in %s (%d von %d Bildpunkten): \"%s\" -> \"%s...\"",
                DiagSeconds(), DiagRole(), item->id.ToChar(), LobbyTextPx(font, text), px, text.c_str(), s.c_str());
        return s + "...";
    }

    static void LobbySetItemText(zCMenuItem* item, const std::string& text) {
        if (!item || *(DWORD*)item != 0x007D2E04)
            return;
        typedef void(__thiscall* SetTextFn)(zCMenuItem*, const zSTRING&, int, int);
        const std::string fit = LobbyFitText(item, text);
        zSTRING z(fit.empty() ? " " : fit.c_str());
        ((SetTextFn)0x004D6430)(item, z, 0, 1);
    }

    static zCMenuItem* LobbyFindItem(zCMenu* menu, const char* id) {
        if (!menu)
            return NULL;
        for (int i = 0; i < menu->m_listItems.GetNum(); i++) {
            zCMenuItem* it = menu->m_listItems[i];
            if (it && LobbyUpper(it->id) == id)
                return it;
        }
        return NULL;
    }

    // v55: Spielstand-Liste (Host): Slots mit Coop-Kennung, neueste zuerst, hoechstens 10. Spaeter "Anderen Spielstand laden".
    // v93 (Nutzer: "wir haben doch unser eigenes Sicherungssystem und nicht Othellos Speicherplaetze ... nur die Spielstaende, die bei
    // beiden vorhanden sind ... auch lokale Sicherungen, die aus diesem Playthrough stammten"): Staende aus dem Coop-Archiv und den
    // Slots (CoopSaveSyncListJoint), alle, neueste zuerst, 10 Zeilen sichtbar ab Lobby.rowTop. Mit Partner nur, was beide haben
    // (seine Antwort "have"); Schalter "Auch lokale Staende": dazu die eigenen Staende des Durchlaufs (im Spiel der aktuelle, im
    // Hauptmenue die Durchlaeufe, die der Partner auch kennt). Ohne Partner alle eigenen. Lobby.rows/texts = sichtbarer Ausschnitt.
    static const int LOBBY_ROWS = 10;

    static std::string LobbyRowText(const CoopListEntry& e, bool partner) {
        // v78: Zeile fasst etwa 34 Zeichen - Name, Datum nur, wenn es nicht schon im Namen steht, kurz "TT.MM. hh:mm"
        std::string label = e.name.empty() || e.name == "---" ? std::string(CoopTr("Gespeichert", "Saved")) : e.name;
        std::string text = label;
        if (e.time.size() >= 16 && label.find(e.time.substr(0, 10)) == std::string::npos)
            text += "  " + e.time.substr(0, 6) + " " + e.time.substr(11, 5);
        if (partner && !e.both)
            text += e.sid.empty() ? CoopTr("  (lokal)", "  (local)") : CoopTr("  (nur hier)", "  (only here)");
        return text;
    }

    // Partner-Stand fuer die Liste: im Hauptmenue (Host) der erste Mitspieler, im Spiel die Antwort auf LobbySendSlots
    static bool LobbyPartnerList(const std::set<std::string>*& have, const std::set<std::string>*& runs, bool& known) {
        have = runs = NULL;
        known = false;
        if (!player) {
            if (!ServerThread || Lobby.peers.empty())
                return false;
            const CoopLobbyPeer& p = Lobby.peers.begin()->second;
            have = &p.have;
            runs = &p.runs;
            known = p.haveKnown;
            return true;
        }
        if (!ServerThread && !ClientThread)
            return false;
        if (!Lobby.gameHaveKnown && Lobby.gameAskMs && GetCurrentMs() > Lobby.gameAskMs + 6000)
            return false;   // keine Antwort (aeltere Fassung?) - alle eigenen Staende zeigen
        have = &Lobby.gameHave;
        runs = &Lobby.gameRuns;
        known = Lobby.gameHaveKnown;
        return true;
    }

    static void LobbyBuildRows(std::vector<std::string>& texts) {
        std::vector<CoopListEntry> own;
        CoopSaveSyncListJoint(own);
        const std::set<std::string>* have = NULL;
        const std::set<std::string>* runs = NULL;
        bool known = false;
        const bool partner = LobbyPartnerList(have, runs, known);
        const std::string cur = CoopSaveSyncCurrentRun();
        Lobby.allRows.clear();
        Lobby.localRows = 0;
        for (auto& e : own) {
            e.both = partner && known && !e.sid.empty() && have->count(e.sid) > 0;
            bool show = true;
            if (partner) {
                if (!known)
                    show = false;   // Antwort des Partners steht noch aus
                else if (!e.both) {
                    show = ListShowLocal && !e.run.empty() && (player ? e.run == cur : runs->count(e.run) > 0);
                    if (show)
                        Lobby.localRows++;   // v94: Anzeige am Schalter
                }
            }
            if (show)
                Lobby.allRows.push_back(e);
        }
        const int total = (int)Lobby.allRows.size();
        if (Lobby.rowTop > total - LOBBY_ROWS)
            Lobby.rowTop = total - LOBBY_ROWS;
        if (Lobby.rowTop < 0)
            Lobby.rowTop = 0;
        Lobby.rows.clear();
        texts.clear();
        for (int i = Lobby.rowTop; i < total && i < Lobby.rowTop + LOBBY_ROWS; i++) {
            Lobby.rows.push_back(Lobby.allRows[i]);
            texts.push_back(LobbyRowText(Lobby.allRows[i], partner));
        }
    }

    // v93: Schalter "Auch lokale Staende" (MENUITEM_COOPLOAD_FILTER, COOP_MENU-v93.D)
    // v94 (Test v93: "als ich lokale Kopien angemacht habe, konnte ich es nicht mehr ausschalten"): der Schalter wirkte (Log: an/aus
    // im Wechsel), nur die Anzeige blieb auf "an" - SetText (0x004D6430) aendert m_parText[0] nicht, der Vergleich damit sah beim
    // Ausschalten den Skripttext "aus" und setzte nichts. Jetzt immer setzen, dazu die Zahl der zusaetzlichen Zeilen.
    static void LobbySetListFilter(zCMenu* menu) {
        std::string t = ListShowLocal ? CoopTr("Lokale St" COOP_AE "nde: an", "Local saves: on") : CoopTr("Lokale St" COOP_AE "nde: aus", "Local saves: off");
        if (ListShowLocal)
            t += " (+" + std::to_string(Lobby.localRows) + ")";
        LobbySetItemText(LobbyFindItem(menu, "MENUITEM_COOPLOAD_FILTER"), t);
    }

    // v94 (Nutzer: "verdeutlichen, wo genau wir uns befinden bei den Speicherstaenden, vielleicht mit einer Scrollbar"): Griff
    // MENUITEM_COOPLOAD_THUMB auf der Schiene MENUITEM_COOPLOAD_SCROLL (COOP_MENU-v94.D, zwischen Liste und Vorschau), Hoehe =
    // Anteil der sichtbaren Zeilen, Lage = rowTop. zCMenuItem ist ein zCView: zCView::SetPos (0x006FDA10, vposx/vposy +0x40/+0x44)
    // und SetSize (0x006FDC00, +0x48/+0x4C) in Menue-Einheiten des Elternfensters, beide unveraendert; m_parPosY/m_parDimY mit,
    // damit zCMenuItem::InsertInWin (0x004D2B40, ruft SetPos mit m_parPosX/Y) beim naechsten Oeffnen dieselbe Lage nimmt.
    static int LobbyScrollLogged = 0;

    static void LobbySetScrollBar(zCMenu* menu) {
        zCMenuItem* track = LobbyFindItem(menu, "MENUITEM_COOPLOAD_SCROLL");
        zCMenuItem* thumb = LobbyFindItem(menu, "MENUITEM_COOPLOAD_THUMB");
        if (!track || !thumb)
            return;   // Menue-Skript vor v94
        const int total = (int)Lobby.allRows.size();
        const int y0 = track->m_parPosY, h = track->m_parDimY;
        int th = h, ty = y0;
        if (total > LOBBY_ROWS && h > 0) {
            th = h * LOBBY_ROWS / total;
            if (th < 400)
                th = 400;
            if (th > h)
                th = h;
            ty = y0 + (int)((long long)(h - th) * Lobby.rowTop / (total - LOBBY_ROWS));
        }
        if (thumb->m_parPosY == ty && thumb->m_parDimY == th)
            return;
        thumb->m_parPosY = ty;
        thumb->m_parDimY = th;
        typedef void(__thiscall* ViewIntFn)(zCMenuItem*, int, int);
        ((ViewIntFn)0x006FDA10)(thumb, thumb->m_parPosX, ty);   // zCView::SetPos
        ((ViewIntFn)0x006FDC00)(thumb, thumb->m_parDimX, th);   // zCView::SetSize
        if (LobbyScrollLogged++ < 12)
            LobbyLog("Bildlaufleiste: Zeilen %d-%d von %d - Griff y %d, H" COOP_OE "he %d (Schiene %d+%d)", Lobby.rowTop + 1,
                Lobby.rowTop + LOBBY_ROWS < total ? Lobby.rowTop + LOBBY_ROWS : total, total, ty, th, y0, h);
    }

    // v93: Ueberschrift mit Ausschnitt ("1-10 von 13"), wenn mehr Staende da sind als Zeilen
    static void LobbySetListHeadline(zCMenu* menu) {
        const int total = (int)Lobby.allRows.size();
        std::string h = CoopTr("Spielstand w" COOP_AE "hlen", "Choose save");
        if (total > LOBBY_ROWS)
            h += "  (" + std::to_string(Lobby.rowTop + 1) + "-" + std::to_string(Lobby.rowTop + LOBBY_ROWS < total ? Lobby.rowTop + LOBBY_ROWS : total) +
                CoopTr(" von ", " of ") + std::to_string(total) + ")";
        LobbySetItemText(LobbyFindItem(menu, "MENUITEM_COOPLOAD_HEADLINE"), h);   // v94: immer (wie der Schalter)
        LobbySetScrollBar(menu);   // v94
    }

    // v93 (Nutzer: "Koennen wir mehr Eintraege einfuegen und das Ganze scrollbar nach unten erweitern?"): Blaettern wie in einer
    // Liste. Gothic bewegt die Markierung mit Rad (0x809/0x80A) und Pfeiltasten selbst (zCMenu::HandleEvent, vor diesem Takt in
    // GetInput). Geht sie von der letzten sichtbaren Zeile nach unten aus dem Zeilenblock (auf "Anderen Spielstand laden") bzw. von
    // der ersten nach oben (Gothic springt dann ans Ende, auf ZURUECK) und gibt es dort noch Staende, verschiebt GT den Ausschnitt
    // um eine Zeile und setzt die Markierung zurueck (wie zCMenu::NextOption 0x004CEAA0: SetIsActive, HandleLeaveItem vtable +8,
    // m_mainSel, HandleEnterItem +4). Markierungen der Menue-Maus (Ueberfahren) zaehlen nicht - CoopMenuMouseSelectSeq.
    unsigned CoopMenuMouseSelectSeq();   // CoopMenuMouse.cpp
    static zCMenu* LobbyListLastMenu = NULL;
    static int LobbyListLastSel = -1;
    static unsigned LobbyListMouseSeq = 0;
    static int LobbyListScrollLogged = 0;

    static int LobbyItemIndex(zCMenu* menu, const char* id) {
        for (int i = 0; i < menu->m_listItems.GetNum(); i++) {
            zCMenuItem* it = menu->m_listItems[i];
            if (it && LobbyUpper(it->id) == id)
                return i;
        }
        return -1;
    }

    static void LobbySelectItem(zCMenu* menu, int idx) {
        const int n = menu->m_listItems.GetNum();
        if (idx < 0 || idx >= n || idx == menu->m_mainSel || !menu->m_listItems[idx])
            return;
        typedef int(__thiscall* ItemFn)(zCMenu*, zCMenuItem*);
        const int old = menu->m_mainSel;
        if (old >= 0 && old < n && menu->m_listItems[old]) {
            zCMenuItem* was = menu->m_listItems[old];
            was->SetIsActive(0);
            ((ItemFn)(*(DWORD**)menu)[0x08 / 4])(menu, was);   // HandleLeaveItem
        }
        menu->m_mainSel = idx;
        zCMenuItem* now = menu->m_listItems[idx];
        ((ItemFn)(*(DWORD**)menu)[0x04 / 4])(menu, now);       // HandleEnterItem
        if (menu->cursorEn)
            now->SetIsActive(1);
    }

    // true = Ausschnitt verschoben (Zeilen gleich neu setzen)
    static bool LobbyListScroll(zCMenu* menu) {
        const int r0 = LobbyItemIndex(menu, "MENUITEM_COOPLOAD_ROW0");
        const int rl = LobbyItemIndex(menu, "MENUITEM_COOPLOAD_ROW9");
        const int sel = menu->m_mainSel;
        const unsigned seq = CoopMenuMouseSelectSeq();
        bool scrolled = false;
        if (menu == LobbyListLastMenu && seq == LobbyListMouseSeq && r0 >= 0 && rl > r0 && sel != LobbyListLastSel &&
            (sel < r0 || sel > rl)) {
            const int total = (int)Lobby.allRows.size();
            if (LobbyListLastSel == rl && Lobby.rowTop + LOBBY_ROWS < total) {
                Lobby.rowTop++;
                LobbySelectItem(menu, rl);
                scrolled = true;
            }
            else if (LobbyListLastSel == r0 && Lobby.rowTop > 0) {
                Lobby.rowTop--;
                LobbySelectItem(menu, r0);
                scrolled = true;
            }
            if (scrolled && LobbyListScrollLogged++ < 10)
                LobbyLog("Liste geblaettert: Zeilen %d-%d von %d", Lobby.rowTop + 1, Lobby.rowTop + LOBBY_ROWS < total ? Lobby.rowTop + LOBBY_ROWS : total, total);
        }
        LobbyListLastMenu = menu;
        LobbyListLastSel = menu->m_mainSel;
        LobbyListMouseSeq = seq;
        return scrolled;
    }

    // --- v95: Liste per Rad und Leiste (CoopMenuMouse.cpp ruft beides vor dem Ueberfahren) ------------------------------------
    // Test v94: "beim Scrollen der Spielstaende hat die Maus genervt, weil es dann nicht richtig scrollen konnte - entweder wir
    // scrollen nur die Leiste oder beim Scrollen soll die Maus nicht dazwischenfunken", "gut, wenn man die Scrollleiste ziehen
    // kann". Gothic macht aus dem Rad 0x809/0x80A hoch/runter (zCInputCallback::GetInput 0x006FB9D0, Raddelta ueber
    // zCInput::GetMousePos - unter Othello umgeleitet, darum hier nicht selbst abgefragt) - die Markierung wanderte erst bis zur
    // letzten Zeile, und bewegte sich die Maus dabei, setzte das Ueberfahren sie zurueck (kein Blaettern). Jetzt: hat sich die
    // Markierung ohne GT um eine Zeile bewegt und ist keine Pfeiltaste gedrueckt (= Rad), blaettert der Inhalt und die Markierung
    // bleibt (unter dem Zeiger). Pfeiltasten wie bisher (LobbyListScroll). Allgemein G1.
    static int LobbyWheelLogged = 0;

    // true = verbraucht (Inhalt geblaettert bzw. oben gehalten, Markierung zurueck auf from)
    bool CoopLoadListWheel(zCMenu* menu, int from, int to) {
        if (!menu || LobbyUpper(menu->name) != "MENU_COOP_LOAD")
            return false;
        if ((GetAsyncKeyState(VK_UP) & 0x8000) || (GetAsyncKeyState(VK_DOWN) & 0x8000) ||
            (zinput && (zinput->KeyPressed(KEY_UPARROW) || zinput->KeyPressed(KEY_DOWNARROW))))
            return false;   // Pfeiltaste: Markierung wandert, am Rand blaettert LobbyListScroll
        const int r0 = LobbyItemIndex(menu, "MENUITEM_COOPLOAD_ROW0");
        const int rl = LobbyItemIndex(menu, "MENUITEM_COOPLOAD_ROW9");
        if (r0 < 0 || rl <= r0 || from < r0 || from > rl)
            return false;
        int dir = 0;
        if (from == r0 && to > rl + 1)
            dir = -1;   // oberste Zeile, Rad hoch: Gothic springt ans Ende (ZURUECK)
        else if (to == from + 1)
            dir = 1;
        else if (to == from - 1)
            dir = -1;
        if (!dir)
            return false;
        const int total = (int)Lobby.allRows.size();
        if (dir > 0 && Lobby.rowTop + LOBBY_ROWS >= total)
            return false;   // ganz unten: die Markierung geht weiter zu den Knoepfen
        if (dir < 0 && Lobby.rowTop <= 0) {
            if (to <= rl)
                return false;
            LobbySelectItem(menu, from);   // ganz oben bleiben statt auf ZURUECK zu springen
            LobbyListLastSel = from;
            return true;
        }
        Lobby.rowTop += dir;
        LobbySelectItem(menu, from);
        LobbyListLastSel = from;
        Lobby.rowsMs = 0;   // Zeilen im naechsten Bild neu
        if (LobbyWheelLogged++ < 10)
            LobbyLog("Liste per Rad: Zeilen %d-%d von %d", Lobby.rowTop + 1, Lobby.rowTop + LOBBY_ROWS < total ? Lobby.rowTop + LOBBY_ROWS : total, total);
        return true;
    }

    // Leiste: Griff ziehen (Taste gehalten) oder Klick auf die Schiene springt dorthin. x/y in Bildpunkten wie zCView pposx/pposy.
    // Rueckgabe true = Zeiger auf der Leiste bzw. Ziehen laeuft - der Klick gehoert der Leiste (sonst loeste ein Klick ins Leere nach
    // eigener Wahl den markierten Stand aus, v86).
    static bool LobbyBarDrag = false;
    static int LobbyBarGrab = 0;
    static int LobbyBarLogged = 0;

    bool CoopLoadListBar(zCMenu* menu, int x, int y, bool left, bool pressed) {
        if (!menu || LobbyUpper(menu->name) != "MENU_COOP_LOAD") {
            LobbyBarDrag = false;
            return false;
        }
        zCMenuItem* track = LobbyFindItem(menu, "MENUITEM_COOPLOAD_SCROLL");
        zCMenuItem* thumb = LobbyFindItem(menu, "MENUITEM_COOPLOAD_THUMB");
        if (!track || !thumb || !track->ondesk || track->psizey <= 0) {
            LobbyBarDrag = false;
            return false;
        }
        const int pad = track->psizex < 12 ? 12 - track->psizex / 2 : 4;   // schmale Leiste: etwas breiter greifen
        const bool over = x >= track->pposx - pad && x < track->pposx + track->psizex + pad && y >= track->pposy - 4 &&
            y < track->pposy + track->psizey + 4;
        if (!left) {
            LobbyBarDrag = false;
            return false;
        }
        const int total = (int)Lobby.allRows.size();
        if (pressed) {
            if (!over)
                return false;
            if (total <= LOBBY_ROWS)
                return true;   // nichts zu blaettern, Klick trotzdem verbraucht
            LobbyBarDrag = true;
            const bool onThumb = y >= thumb->pposy && y < thumb->pposy + thumb->psizey;
            LobbyBarGrab = onThumb ? y - thumb->pposy : thumb->psizey / 2;   // Schiene: Griffmitte unter den Zeiger
        }
        if (!LobbyBarDrag)
            return false;
        const int range = track->psizey - thumb->psizey;
        int top = 0;
        if (range > 0 && total > LOBBY_ROWS) {
            const int pos = y - LobbyBarGrab - track->pposy;
            top = (int)(((long long)pos * (total - LOBBY_ROWS) + range / 2) / range);
        }
        if (top < 0)
            top = 0;
        if (top > total - LOBBY_ROWS)
            top = total - LOBBY_ROWS;
        if (top != Lobby.rowTop) {
            Lobby.rowTop = top;
            Lobby.rowsMs = 0;
            if (LobbyBarLogged++ < 10)
                LobbyLog("Leiste gezogen: Zeilen %d-%d von %d", top + 1, top + LOBBY_ROWS, total);
        }
        return true;
    }

    // --- v95: Sprache der Menue-Texte -------------------------------------------------------------------------------------------
    // COOP_MENU.D (ab v95): jeder eigene Eintrag traegt sein Englisch selbst - userString[2] = Text bzw. Optionen eines
    // Auswahlfelds, userString[3] = Infozeile (m_parUserString +0x300, je 0x14; die Engine liest im Menue-Code nur userString[0],
    // bei Auswahlfeldern als Cursor-Textur, zCMenuItemChoice::InsertInWin 0x004D77FF). Bei coopLang=1 tauscht GT im aktiven Menue:
    //   Text: zCMenuItemText::SetText (0x004D6430) - m_parText[0] behaelt das Deutsch aus dem Skript (zurueck damit);
    //   Auswahlfeld: m_listLines[0] (+0x37C) - zCMenuItemChoice::GetNumOptions/GetText (0x004D7C10/0x004D7DF0) lesen die Optionen
    //     ueber zCMenuItem::GetText(0) (0x004D3530) daraus -, dann DrawFront (vtable +0x58);
    //   Infozeile: m_parText[1] (+0x124) - zCMenuItem::SetIsActive (0x004D4C50) gibt sie an zCMenu::PrintInformation (0x004CFE50);
    //     das Deutsch merkt sich GT beim ersten Tausch.
    // Eintraege, deren Text GT selbst setzt (Status, Zeilen, Knoepfe der Startseite), haben nur userString[3]. Laeuft auch fuer
    // Othellos Menues (unsere eingehaengten Eintraege), Othellos eigene Eintraege haben keine userString[2]/[3]. Allgemein G1.
    struct CoopLangItem {
        int lang = -1;
        std::string shown0;
        std::string de1;
        bool hasDe1 = false;
    };
    static std::map<zCMenuItem*, CoopLangItem> LangItems;
    static zCMenu* LangMenu = NULL;
    static DWORD LangMenuMs = 0;
    static int LangLastLogged = -1;

    static std::string LangLine0(zCMenuItem* it) {
        return it->m_listLines.GetNum() > 0 ? std::string(it->m_listLines[0].ToChar()) : std::string();
    }

    void CoopLangMenuPulse() {
        zCMenu* menu = zCMenu::GetActive();
        if (!menu)
            return;
        const DWORD now = GetTickCount();
        if (menu == LangMenu && now - LangMenuMs < 200)
            return;
        LangMenu = menu;
        LangMenuMs = now;
        typedef void(__thiscall* VoidFn)(zCMenuItem*);
        int changed = 0;
        for (int i = 0; i < menu->m_listItems.GetNum(); i++) {
            zCMenuItem* it = menu->m_listItems[i];
            if (!it)
                continue;
            const zSTRING& en0 = it->m_parUserString[2];
            const zSTRING& en1 = it->m_parUserString[3];
            const bool has0 = en0.Length() > 0, has1 = en1.Length() > 0;
            if (!has0 && !has1)
                continue;
            CoopLangItem& st = LangItems[it];
            if (has1) {
                if (!st.hasDe1) {
                    st.de1 = it->m_parText[1].ToChar();
                    st.hasDe1 = true;
                }
                const std::string want1 = CoopLangEn ? std::string(en1.ToChar()) : st.de1;
                if (want1 != it->m_parText[1].ToChar()) {
                    it->m_parText[1] = want1.c_str();
                    changed++;
                }
            }
            if (!has0)
                continue;
            const std::string want0 = CoopLangEn ? std::string(en0.ToChar()) : std::string(it->m_parText[0].ToChar());
            const DWORD vt = *(DWORD*)it;
            if (vt == 0x007D2C54) {   // Auswahlfeld
                if (it->m_listLines.GetNum() > 0 && LangLine0(it) != want0) {
                    it->m_listLines[0] = want0.c_str();
                    ((VoidFn)(*(DWORD**)it)[0x58 / 4])(it);   // DrawFront
                    changed++;
                }
            }
            else if (vt == 0x007D2E04) {   // Text - v96: erst, wenn der Eintrag im Fenster ist (vorher stimmt seine Breite nicht)
                if (!it->ondesk)
                    continue;
                if (st.lang != CoopLangEn || LangLine0(it) != st.shown0) {
                    LobbySetItemText(it, want0);
                    st.shown0 = LangLine0(it);
                    st.lang = CoopLangEn;
                    changed++;
                }
            }
        }
        if (changed && LangLastLogged != CoopLangEn) {
            LangLastLogged = CoopLangEn;
            LobbyLog("Sprache: Men" COOP_UE " %s auf %s gestellt (%d Texte)", menu->name.ToChar(), CoopLangEn ? "English" : "Deutsch", changed);
        }
    }

    // v95: Text auf Zeilen umbrechen, die in den Eintrag passen (Woerter; zu lange Woerter und die letzte Zeile mit "...")
    static std::vector<std::string> LobbyWrapText(zCMenuItem* item, const std::string& text, int maxLines) {
        std::vector<std::string> out;
        if (!item || maxLines <= 0)
            return out;
        std::string rest = text;   // v96: gemessen wie LobbyFitText (Schrift gegen Skriptbreite)
        while (!rest.empty() && (int)out.size() < maxLines) {
            if ((int)out.size() == maxLines - 1 || LobbyFits(item, rest)) {
                out.push_back(LobbyFitText(item, rest));
                break;
            }
            size_t cut = std::string::npos;   // laengster Anfang bis zu einem Leerzeichen, der passt
            for (size_t p = rest.find(' '); p != std::string::npos; p = rest.find(' ', p + 1)) {
                if (!LobbyFits(item, rest.substr(0, p)))
                    break;
                cut = p;
            }
            if (cut == std::string::npos) {
                out.push_back(LobbyFitText(item, rest));
                break;
            }
            out.push_back(rest.substr(0, cut));
            rest = "  " + rest.substr(cut + 1);
        }
        return out;
    }

    static void LobbyShowLoadList(zCMenu* menu, const std::string& l1, const std::string& l2) {
        const long long now = GetCurrentMs();
        static long long lastMs = 0;   // v93: neu geoeffnet -> wieder oben anfangen
        if (now > lastMs + 1500)
            Lobby.rowTop = 0;
        lastMs = now;
        if (LobbyListScroll(menu))     // v93: geblaettert -> Zeilen sofort neu
            Lobby.rowsMs = 0;
        if (menu == Lobby.rowsMenu && now < Lobby.rowsMs + 2000)
            return;
        Lobby.rowsMenu = menu;
        Lobby.rowsMs = now;
        std::vector<std::string> texts;
        std::string status;
        if (ServerThread) {
            LobbyBuildRows(texts);
            CoopPreviewPrefetch();   // v77: Bilder/Angaben des Mitspielers fuer alle Zeilen vorab
            std::string why;
            if (!LobbyPeersReady(why))
                status = why;
            else if (texts.empty())   // v93
                status = ListShowLocal ? CoopTr("Kein Stand - \"Anderen Spielstand laden\".", "No save - \"Load another save\".")
                                       : CoopTr("Kein gemeinsamer Stand - \"Lokale St" COOP_AE "nde\" oder \"Anderen Spielstand laden\".", "No shared save - \"Local saves\" or \"Load another save\".");
            else
                status = CoopTr("Zeile w" COOP_AE "hlen - beide laden ihn, jeder mit seinem Helden.", "Pick a row - both load it, each with their own hero.");
        }
        else {
            Lobby.rows.clear();
            status = ClientThread ? CoopTr("Den Spielstand w" COOP_AE "hlt der Host - bitte warten.", "The host chooses the save - please wait.") : CoopTr("Nicht verbunden.", "Not connected.");
            if (!l1.empty() && ClientThread)
                status += "  " + l1;
        }
        if (!Lobby.note.empty() && now < Lobby.noteUntil)
            status = Lobby.note;
        for (int i = 0; i < 10; i++) {
            char id[40];
            _snprintf(id, sizeof(id), "MENUITEM_COOPLOAD_ROW%d", i);
            id[sizeof(id) - 1] = 0;
            LobbySetItemText(LobbyFindItem(menu, id), i < (int)texts.size() ? texts[i] : std::string());
        }
        LobbySetItemText(LobbyFindItem(menu, "MENUITEM_COOPLOAD_STATUS"), status);
        LobbySetListHeadline(menu);   // v93
        LobbySetListFilter(menu);
    }

    // v91: Spielstand-Liste im Esc-Menue (Game_MenuLoop, nur im Spiel) - Zeilen wie im Hauptmenue, fuer Host, Mitspieler und allein
    void CoopIngameLoadListPulse() {
        if (!player)
            return;
        zCMenu* menu = zCMenu::GetActive();
        if (!menu || LobbyUpper(menu->name) != "MENU_COOP_LOAD")
            return;
        const long long now = GetCurrentMs();
        // v92: neu geoeffnet (Gothic legt das Menue nur einmal an - gleicher Zeiger): Bilder neu laden, Partner neu fragen
        // v93: oben anfangen, dem Partner die eigenen Kennungen schicken (er antwortet mit "have", CoopLobbyReceiveInGame)
        static long long lastPulseMs = 0;
        if (now > lastPulseMs + 1500) {
            CoopPreviewReset();
            Lobby.rowsMs = 0;
            Lobby.rowTop = 0;
            if (ServerThread || ClientThread) {
                Lobby.gameHaveKnown = false;
                Lobby.gameAskMs = now;
                LobbySendSlots();
            }
        }
        lastPulseMs = now;
        if (LobbyListScroll(menu))   // v93: geblaettert -> Zeilen sofort neu
            Lobby.rowsMs = 0;
        if (menu != Lobby.rowsMenu || now >= Lobby.rowsMs + 2000) {
            Lobby.rowsMenu = menu;
            Lobby.rowsMs = now;
            std::vector<std::string> texts;
            LobbyBuildRows(texts);
            const std::set<std::string>* have = NULL;
            const std::set<std::string>* runs = NULL;
            bool known = false;
            const bool partner = LobbyPartnerList(have, runs, known);
            const bool coop = ServerThread || ClientThread;
            std::string status;
            if (partner && !known)
                status = CoopTr("Pr" COOP_UE "fe die St" COOP_AE "nde beim Mitspieler ...", "Checking your partner's saves ...");
            else if (coop && !partner)
                status = CoopTr("Der Mitspieler antwortet nicht (Fassung?) - alle eigenen St" COOP_AE "nde.", "Your partner does not answer (version?) - all own saves.");
            else if (texts.empty())
                status = !partner ? CoopTr("Noch kein Coop-Stand - \"Anderen Spielstand laden\".", "No coop save yet - \"Load another save\".")
                    : ListShowLocal ? CoopTr("Kein Stand dieses Durchlaufs - \"Anderen Spielstand laden\".", "No save of this playthrough - \"Load another save\".")
                    : CoopTr("Kein gemeinsamer Stand - \"Lokale St" COOP_AE "nde\" oder \"Anderen Spielstand laden\".", "No shared save - \"Local saves\" or \"Load another save\".");
            else
                status = (ClientThread && !ServerThread) ? CoopTr("Zeile w" COOP_AE "hlen - der Host l" COOP_AE "dt den Stand f" COOP_UE "r beide.", "Pick a row - the host loads it for both.")
                    : ServerThread ? CoopTr("Zeile w" COOP_AE "hlen - beide laden ihn, jeder mit seinem Helden.", "Pick a row - both load it, each with their own hero.")
                    : CoopTr("Zeile w" COOP_AE "hlen - l" COOP_AE "dt den Stand.", "Pick a row - loads the save.");
            if (!Lobby.note.empty() && now < Lobby.noteUntil)
                status = Lobby.note;
            for (int i = 0; i < 10; i++) {
                char id[40];
                _snprintf(id, sizeof(id), "MENUITEM_COOPLOAD_ROW%d", i);
                id[sizeof(id) - 1] = 0;
                LobbySetItemText(LobbyFindItem(menu, id), i < (int)texts.size() ? texts[i] : std::string());
            }
            LobbySetItemText(LobbyFindItem(menu, "MENUITEM_COOPLOAD_STATUS"), status);
            LobbySetListHeadline(menu);
            LobbySetListFilter(menu);
        }
        CoopSavePreviewPulse(menu);   // Bilder und Angaben beider Seiten (v92)
    }

    // v93: im Spiel die "slots"/"have"-Abfrage der Liste (beide Rollen) - aus CoopLobbyReceiveInGame
    static void LobbyIngameSlots(json& e, const std::string& op) {
        if (op == "slots") {
            LobbyAnswerSlots(e);
            return;
        }
        LobbyReadHave(e, Lobby.gameHave, Lobby.gameRuns);
        Lobby.gameHaveKnown = true;
        Lobby.rowsMs = 0;   // Liste gleich neu
        LobbyLog("Liste im Spiel: Partner hat %u meiner St" COOP_AE "nde, kennt %u Durchl" COOP_AE "ufe", (unsigned)Lobby.gameHave.size(),
            (unsigned)Lobby.gameRuns.size());
    }

    // v91: MENU_COOP_NG - welche Othello-Schwierigkeit gewaehlt ist und ob der Mitspieler bereit ist
    static const char* LobbyNgDiffName(const std::string& act) {
        std::string up = act;
        for (auto& c : up)
            c = (char)toupper((unsigned char)c);
        if (up.find("VERY_HARD") != std::string::npos) return CoopTr("Sehr schwer", "Very hard");
        if (up.find("HARD") != std::string::npos) return CoopTr("Schwer", "Hard");
        if (up.find("EASY") != std::string::npos) return CoopTr("Leicht", "Easy");
        if (up.find("NORMAL") != std::string::npos) return "Normal";
        return CoopTr("wie gew" COOP_AE "hlt", "as chosen");
    }

    static void LobbyShowNgNote(zCMenu* menu) {
        static long long nextMs = 0;
        const long long now = GetCurrentMs();
        if (now < nextMs)
            return;
        nextMs = now + 1000;
        std::string why, text;
        if (!LobbyPeersReady(why))
            text = why;
        else
            text = std::string("Othello: ") + LobbyNgDiffName(NgShownAct) + CoopTr(". Diese COOP-Werte gelten f" COOP_UE "r beide.", ". These COOP values apply to both.");
        if (!Lobby.note.empty() && now < Lobby.noteUntil)
            text = Lobby.note;
        LobbySetItemText(LobbyFindItem(menu, "MENUITEM_COOPNG_NOTE"), text);
    }

    static void LobbyShowStatus() {
        std::string l1, l2;
        LobbyStatus(l1, l2);
        if (l1 != Lobby.lastLog1 || l2 != Lobby.lastLog2) {
            Lobby.lastLog1 = l1;
            Lobby.lastLog2 = l2;
            LobbyLog("Status: %s%s%s", l1.c_str(), l2.empty() ? "" : " | ", l2.c_str());
        }
        // v71: laufender Fortschritt nur in der Anzeige (Test v69: 48 s stand "(10 Dateien)" still - der Mitspieler trennte)
        if (ClientThread && !ServerThread && (l1.compare(0, 14, "Hole den Stand") == 0 || l1.compare(0, 16, "Fetching the sav") == 0))
            l1 += CoopSaveSyncMenuProgress();
        zCMenu* menu = zCMenu::GetActive();
        if (!menu)
            return;
        const std::string mname = LobbyUpper(menu->name);
        if (mname == "MENU_COOP_LOAD") {
            LobbyShowLoadList(menu, l1, l2);
            CoopSavePreviewPulse(menu);   // v77: markierte Zeile -> Bilder und Angaben (jedes Bild, sofort beim Ueberfahren)
            return;
        }
        if (mname == "MENU_COOP_NG") {   // v91: Hinweiszeile - Othellos Wahl, Mitspieler bereit?
            LobbyShowNgNote(menu);
            return;
        }
        const long long now = GetCurrentMs();
        // v94: Unterseiten der COOP-Startseite (COOP_MENU-v94.D) - "Ueber IP beitreten" mit Status, "Figur und Name" mit Hinweis
        if (mname == "MENU_COOP_IP" || mname == "MENU_COOP_LOOK") {
            const bool ip = mname == "MENU_COOP_IP";
            const std::string note = !Lobby.note.empty() && now < Lobby.noteUntil ? Lobby.note
                : std::string(CoopTr("Die Vorschau zeigt deine Figur so, wie der Mitspieler sie sieht.", "The preview shows your character as your partner sees it."));
            const std::string key = ip ? l1 + "|" + l2 : note;
            if (menu == Lobby.shownMenu && key == Lobby.shown1 && now < Lobby.shownMs + 2000)
                return;
            if (ip) {
                LobbySetItemText(LobbyFindItem(menu, "MENUITEM_COOPIP_STATUS1"), l1);
                LobbySetItemText(LobbyFindItem(menu, "MENUITEM_COOPIP_STATUS2"), l2);
            }
            else
                LobbySetItemText(LobbyFindItem(menu, "MENUITEM_COOPLOOK_NOTE"), note);
            Lobby.shownMenu = menu;
            Lobby.shown1 = key;
            Lobby.shownMs = now;
            return;
        }
        if (mname != "MENU_COOP")
            return;
        // v94 (Test v93: "Neues Spiel findet man nur unter Steam ... dass man dem Spieler joinen kann, ist unklar, da es nur klein in
        // der Mitte angezeigt wird"): Startseite mit Steam-Knopf (Freund hostet -> ein Klick), Neues Spiel und Spielstand waehlen;
        // GT setzt deren Text je nach Rolle (Plan gt-fix\PLAN-VERBINDUNGSMENUE.md)
        const bool host = ServerThread != NULL, client = ClientThread != NULL && !host;
        const std::string quick = CoopSteamQuickText();
        const std::string ng = client ? CoopTr("Neues Spiel: startet der Host", "New game: host starts") : (host && !Lobby.peers.empty()) ? CoopTr("Neues Spiel (gemeinsam)", "New game (together)") : CoopTr("Neues Spiel", "New game");
        const std::string ld = client ? CoopTr("Spielstand: w" COOP_AE "hlt der Host", "Save: host chooses") : CoopTr("Spielstand w" COOP_AE "hlen", "Choose save");
        const std::string key = l2 + "|" + quick + "|" + ng + "|" + ld;
        if (menu == Lobby.shownMenu && l1 == Lobby.shown1 && key == Lobby.shown2 && now < Lobby.shownMs + 2000)
            return;
        LobbySetItemText(LobbyFindItem(menu, "MENUITEM_COOP_STATUS1"), l1);
        LobbySetItemText(LobbyFindItem(menu, "MENUITEM_COOP_STATUS2"), l2);
        // v55: "Spielstand waehlen" erschien erst nach "Spiel hosten" / "Spiel beitreten"; v94: immer (ohne Verbindung mit Hinweis)
        LobbySetItemText(LobbyFindItem(menu, "MENUITEM_COOP_LOAD"), ld);
        LobbySetItemText(LobbyFindItem(menu, "MENUITEM_COOP_NEWGAME"), ng);
        LobbySetItemText(LobbyFindItem(menu, "MENUITEM_COOP_STEAMJOIN"), quick);
        if (quick != Lobby.shownQuick) {
            Lobby.shownQuick = quick;
            LobbyLog("Steam-Knopf: %s", quick.c_str());
        }
        Lobby.shownMenu = menu;
        Lobby.shown1 = l1;
        Lobby.shown2 = key;
        Lobby.shownMs = now;
    }

    // --- v58: Fassung im Menue -------------------------------------------------------------------------------------------
    // COOP_MENU.D (ab v58) hat in MENU_MAIN die Zeile MENUITEM_COOPMAIN_VERSION (ueber Gothics Infozeile; MENU_MAIN ist auch das
    // Esc-Menue im Spiel) und in den COOP-Menues je eine Zeile MENUITEM_*COOP*_VERSION oben rechts. GT schreibt "GothicCoop vNN"
    // hinein, im Hauptmenue mit Bauzeit. Mit einem aelteren Menue-Skript fehlen die Zeilen - dann nichts; mit einer aelteren DLL
    // bleibt der Text aus dem Skript ("GothicCoop").
    static std::map<zCMenuItem*, long long> VersionSetMs;
    static bool VersionLogged = false;

    void CoopVersionMenuPulse() {
        zCMenu* menu = zCMenu::GetActive();
        if (!menu)
            return;
        const long long now = GetCurrentMs();
        for (int i = 0; i < menu->m_listItems.GetNum(); i++) {
            zCMenuItem* it = menu->m_listItems[i];
            if (!it)
                continue;
            const std::string id = LobbyUpper(it->id);
            if (id.size() < 17 || id.compare(id.size() - 8, 8, "_VERSION") != 0 || id.find("COOP") == std::string::npos)
                continue;
            const bool main = id == "MENUITEM_COOPMAIN_VERSION";
            const std::string want = main ? CoopVersionName() + CoopTr("  -  Stand ", "  -  build ") + CoopBuildStamp() : CoopVersionName();
            if (want == it->m_parText[0].ToChar())
                continue;
            long long& last = VersionSetMs[it];
            if (now < last + 1000)
                continue;
            last = now;
            LobbySetItemText(it, want);
            if (!VersionLogged && main) {
                VersionLogged = true;
                LobbyLog("Versionszeile im Hauptmen" COOP_UE ": %s", want.c_str());
            }
        }
    }

    // --- v59: Esc im Hauptmenue ohne Spiel ------------------------------------------------------------------------------
    // Test v58: "manchmal Blackscreen, z. B. ... ESC ... nochmal ESC und es war wieder das Hauptmenue". Esc (Taste oder seit
    // v57 Rechtsklick) setzt in zCMenu::HandleEvent m_exitState (+0xC24) = 1 (BACK); zCMenu::Run (0x004CED79) prueft das nach
    // HandleFrame und schliesst das Menue. Ohne geladenes Spiel (ogame NULL, zCMenu::inGameMenu 0x0086D3E8 = 0) bleibt dann
    // ein Spiel ohne Welt - schwarz, bis Esc das Menue wieder oeffnet (Gothic-Verhalten, allgemein G1). Der Menue-Takt laeuft
    // in HandleFrame nach HandleEvent und vor dieser Pruefung: GT setzt m_exitState dort zurueck, wenn es aus Esc kommt.
    // Nicht bei ForceSelAction (+0xCB4 = 1; CLOSE setzt 2, nur BACK 1) - Laden, Neues Spiel und Beenden laufen ueber CLOSE.
    static int MainEscBlocked = 0;

    // v62: "kein Spiel geladen" = kein Held (player). ogame ist im Hauptmenue schon angelegt (Lebenslauf v61: ogame 08DD6EF0) -
    // bis v61 griff der Abfang darum nie.
    void CoopMainMenuEscGuard() {
        zCMenu* menu = zCMenu::GetActive();
        if (!menu || player || *(int*)0x0086D3E8)
            return;
        int& exitState = *(int*)((char*)menu + 0xC24);
        if (exitState != 1 || *(int*)((char*)menu + 0xCB4) != 0 || LobbyUpper(menu->name) != "MENU_MAIN")
            return;
        exitState = 0;
        if (++MainEscBlocked <= 5)
            LobbyLog("Esc im Hauptmen" COOP_UE " ohne geladenes Spiel abgefangen (sonst schwarzer Bildschirm) - %d. Mal", MainEscBlocked);
    }

    // Einmal im Hauptmenue: ist unser Eintrag da (COOP_MENU.D geladen)?
    static void LobbyCheckMainMenu() {
        if (LobbyMainChecked)
            return;
        zCMenu* menu = zCMenu::GetActive();
        if (!menu || LobbyUpper(menu->name) != "MENU_MAIN")
            return;
        LobbyMainChecked = true;
        const bool found = LobbyFindItem(menu, "MENUITEM_COOPMAIN_COOP") != NULL;
        LobbyLog("Hauptmen" COOP_UE ": Eintrag COOP %s (%d Eintr" COOP_AE "ge), Othello-Skripte %s", found ? "vorhanden" : "FEHLT - COOP_MENU.D nicht geladen?",
            menu->m_listItems.GetNum(), LobbyOthelloStamp().c_str());
    }

    // --- Takt (Game_MenuLoop) ------------------------------------------------------------------------------------------

    void CoopLobbyPulse() {
        if (player)
            return;   // im Spiel: Pakete verarbeitet Game_Loop
        CurrentMs = GetCurrentMs();
        if (!LobbyMenuLoopLogged) {
            LobbyMenuLoopLogged = true;
            LobbyLog("Men" COOP_UE "-Takt l" COOP_AE "uft (Game_MenuLoop au" "\xDF" "erhalb des Spiels)");
        }
        LobbyCheckMainMenu();
        CoopLookPulse();   // v55: Name/Adresse/Aussehen aus dem COOP-Menue (Gothic.ini) uebernehmen
        if (!Lobby.active) {
            // v55b: Statuszeilen auch vor "Spiel hosten"/"Spiel beitreten" - sonst stand ein Hinweis (z. B. Ergebnis der NPC-Suche)
            // nur in Gothics Infozeile, die das Menue gleich wieder ueberschreibt (Test v55: Suche lief, Ergebnis unsichtbar)
            LobbyShowStatus();
            return;
        }
        PluginState = "Lobby";
        // v55: Host hat eine Zeile der Spielstand-Liste gewaehlt - Menue wie beim Client mit OTHELLO_SAVEGAME_LOAD schliessen
        if (Lobby.hostRowSlot >= 0 && ServerThread && !Lobby.loading) {
            zCMenu* menu = zCMenu::GetActive();
            const int slot = Lobby.hostRowSlot;
            Lobby.hostRowSlot = -1;
            if (menu && gameMan && gameMan->menu_load_savegame) {
                gameMan->menu_load_savegame->m_selSlot = slot;
                Lobby.hostRowLoad = true;
                LobbyLog("Host: schlie" "\xDF" "e Men" COOP_UE " %s mit OTHELLO_SAVEGAME_LOAD (Slot %d)", LobbyUpper(menu->name).c_str(), slot);
                menu->ForceSelAction(SEL_ACTION_CLOSE, zSTRING("OTHELLO_SAVEGAME_LOAD"), NULL);
            }
        }
        // Faeden: Fehlschlag/Trennung merken, bevor CoopNetPulse aufraeumt
        if (ClientThread && !ServerThread) {
            if (CoopClientState == 3)
                Lobby.clientFailed = true;
            else if (CoopClientState == 4)
                Lobby.clientLost = true;
            else if (CoopClientState == 2 && !Lobby.connectedMs)
                Lobby.connectedMs = GetCurrentMs();
        }
        LobbyProcessPackets();
        static long long netLogMs = 0;   // v71: Netz-Messung auch im Hauptmenue (dort lief im Test v69 die langsame Uebertragung)
        if ((ServerThread || ClientThread) && CurrentMs >= netLogMs + 30000) {
            netLogMs = CurrentMs;
            DiagNetSummary();
        }
        CoopNetPulse();   // v39: beendete Faeden aufraeumen, gewuenschte Rolle starten
        if (!ServerThread)
            Lobby.peers.clear();
        if (!ClientThread && !CoopPendingRole) {
            Lobby.helloSent = Lobby.hostHello = Lobby.hostVerOk = Lobby.hostInGame = Lobby.hostTakes = false;
            Lobby.loadPending = Lobby.allowLoad = false;
            Lobby.ngAct.clear();
            Lobby.allowNewGame = false;
            Lobby.connectedMs = 0;
        }
        CoopLogShipPulse();
        if (ClientThread && !ServerThread && CoopClientState == 2 && !Lobby.loading) {
            CoopSaveSyncMenuPulse();   // Ladeauftrag des Hosts: Dateien holen, Slot vorbereiten (ruft CoopLobbyLoadFromMenu)
            if (Lobby.loadPending) {
                zCMenu* menu = zCMenu::GetActive();
                if (menu && gameMan && gameMan->menu_load_savegame) {
                    gameMan->menu_load_savegame->m_selSlot = Lobby.loadSlot;
                    Lobby.loadPending = false;
                    Lobby.allowLoad = true;
                    Lobby.loading = true;
                    LobbyLog("Client: schlie" "\xDF" "e Men" COOP_UE " %s mit OTHELLO_SAVEGAME_LOAD (Slot %d)", LobbyUpper(menu->name).c_str(), Lobby.loadSlot);
                    menu->ForceSelAction(SEL_ACTION_CLOSE, zSTRING("OTHELLO_SAVEGAME_LOAD"), NULL);
                }
            }
            // v91: neues Spiel des Hosts - Menues mit Othellos Text schliessen wie beim Laden (CLOSE schreibt menuAction)
            else if (!Lobby.ngAct.empty()) {
                zCMenu* menu = zCMenu::GetActive();
                if (menu) {
                    const std::string act = Lobby.ngAct;
                    Lobby.ngAct.clear();
                    Lobby.allowNewGame = true;
                    Lobby.loading = true;
                    LobbyLog("Client: schlie" "\xDF" "e Men" COOP_UE " %s mit %s (neues Spiel des Hosts)", LobbyUpper(menu->name).c_str(), act.c_str());
                    menu->ForceSelAction(SEL_ACTION_CLOSE, zSTRING(act.c_str()), NULL);
                }
            }
        }
        LobbyShowStatus();
        PluginState = "Menu";
    }

    void CoopLobbyAfterLoad() {
        if (!Lobby.active)
            return;
        const int role = Lobby.role;
        const bool host = ServerThread != NULL, client = ClientThread != NULL && !host;
        Lobby = CoopLobbyState();
        CoopSaveSyncSetMenu(false);
        CoopPreviewReset();   // v77: Vorschaubilder freigeben
        if (host || client)
            CoopLookApplyOwn("gemeinsam geladen");
        if (client)
            CoopSaveSyncSkipConnectTake();   // gemeinsamer Stand ist schon geladen - kein "Stand des Hosts uebernehmen" nach dem Verbinden
        LobbyLog("Wartephase beendet - Spiel geladen (gew" COOP_AE "hlt: %s, l" COOP_AE "uft als %s, Kennung %s)", role == 1 ? "Host" : (role == 2 ? "Client" : "-"),
            host ? "Host" : (client ? "Client" : "Einzelspieler"), MyselfId.ToChar());
    }

#else
    void CoopLobbyPulse() {}
    void CoopLobbyAfterLoad() {}
    void CoopVersionMenuPulse() {}
    void CoopMainMenuEscGuard() {}
    void CoopLobbyReceiveInGame(json&, const std::string&) {}
    void CoopIngameLoadListPulse() {}
    void CoopLangMenuPulse() {}
#endif
}
