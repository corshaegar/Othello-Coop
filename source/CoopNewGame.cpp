// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

    // --- v91: gemeinsames neues Spiel (22.10.2026) ----------------------------------------------------------------------
    //
    // Nutzer: "Meine Idee ist es, dass der Host mit Diego redet, waehrend der Client zuschaut und auch schon rumlaufen kann. Wenn
    // der Host das Intro skipt, wird dieses auch beim Client geskippt." / "Der Client kann entweder das Intro nicht skippen, oder
    // wenn dieser es doch kann, wird es auch bei beiden geskippt." Plan: gt-fix\PLAN-NEUES-SPIEL.md. Start im Hauptmenue:
    // CoopMenuLobby.cpp (Othellos Schwierigkeit, dann MENU_COOP_NG, LOBBY newgame).
    //
    // Intro (allgemein G1): Othellos STARTUP_WORLD ruft PlayVideo("INTRO.BIK") im Ladevorgang. zCVideoPlayer::Play (0x004694F0)
    // fragt in jedem Bild IsPlaying ueber vtable +0x24 ab - bei oCBinkPlayer (vtable 0x007D0FDC) der Platz 0x007D1000; der
    // Renderer GD3D11 (zBinkPlayer.cpp) legt dort seine Funktion ab (im Speicherabbild 0x718E7A90, in der Datei 0x0043C9B0). Esc ruft
    // vtable +0x20 = zCVideoPlayer::Stop (0x004696A0, unveraendert: Feld +0x20 = 0, Rueckgabe 1). Beim natuerlichen Ende bleibt
    // das Feld 1, nur das Bild-Ende meldet 0. GT legt sich um den vtable-Platz:
    //  - meldet er "zu Ende", obwohl er eben noch lief, und ist Feld +0x20 = 0, wurde abgebrochen -> LOBBY introskip an den Partner.
    //  - kam introskip vom Partner (CoopIntroSkipPeek im Netzfaden - der Hauptfaden steht im Video, Pakete verarbeitet er erst
    //    danach), stoppt GT das Video ueber vtable +0x20. Kommt der Abbruch vor dem Video, endet es gleich beim ersten Bild.
    //  - Der Host gibt einen Abbruch eines Mitspielers an alle weiter.
    // Gilt nur vom gemeinsamen Start bis 5 s nach LoadEnd (das Video blockiert den Spieltakt; danach laeuft Game_Loop).
    //
    // Danach (nur Client): im ersten Spielbild neben den Host (CoopSaveSyncPlaceBesideStart - beide beginnen an Othellos
    // Startpunkt). Diego: 3 Minuten lang melden NPCs dem eigenen Helden keine wichtige Info (Npc_CheckInfo(npc, 1),
    // CoopDialogSync.cpp), bis der Host sein erstes Gespraech beendet hat - Diego geht zum Host, der Client sieht zu.

    // vom Menue bzw. Netzfaden
    void CoopNewGameBegin(bool host, const std::string& run);
    void CoopSaveSyncSetRun(const std::string& run, const char* why);   // CoopSaveSync.cpp (v93)
    void CoopIntroSkipPeek(const ENetEvent& ev);
    // CoopDialogSync.cpp
    bool CoopNewGameInfoBlocked();
    void CoopNewGameDialogEnded(const std::string& by, const std::string& npc);
    // CoopSaveSync.cpp
    void CoopSaveSyncPlaceBesideStart();

#if ENGINE == Engine_G1

    typedef int(__thiscall* CoopVideoFn)(void*);

    static const DWORD INTRO_VT_ISPLAYING = 0x007D1000;   // oCBinkPlayer-vtable 0x007D0FDC + 0x24
    static const DWORD INTRO_VT_STOP = 0x007D0FFC;        // + 0x20, zCVideoPlayer::Stop
    static const DWORD INTRO_STOP_FUNC = 0x004696A0;

    static CoopVideoFn IntroOrigIsPlaying = NULL;
    static volatile LONG IntroSyncOn = 0;       // gemeinsamer Start bis kurz nach LoadEnd (Netzfaden liest mit)
    static volatile LONG IntroRemoteSkip = 0;   // Partner hat abgebrochen (vom Netzfaden gesetzt)
    static bool IntroWasPlaying = false;
    static bool IntroSkipSent = false;
    static bool IntroSeen = false;

    static bool NewGameActive = false;
    static bool NewGameHost = false;
    static long long NewGameBeginMs = 0;
    static long long NewGameLoadEndMs = 0;
    static bool NewGamePlacePending = false;
    static long long NewGameInfoUntil = 0;   // Client: bis dahin keine wichtige Info fuer den eigenen Helden (0 = aus)

    static void NewGameLog(const char* fmt, ...) {
        char buf[1024];
        va_list ap;
        va_start(ap, fmt);
        _vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        buf[sizeof(buf) - 1] = 0;
        OthelloCompatLog("[%lld s, %s] Neues Spiel: %s", DiagSeconds(), DiagRole(), buf);
    }

    static void IntroSendSkip(const char* why) {
        if (IntroSkipSent || (!ServerThread && !ClientThread))
            return;
        IntroSkipSent = true;
        json j;
        j["type"] = SYNC_LOBBY;
        j["id"] = MyselfId.ToChar();
        j["op"] = "introskip";
        CoopTraceCount(TRACE_SENT, SYNC_LOBBY);
        ReadyToSendJsons.enqueue(j);
        NewGameLog("Intro %s - Partner auch", why);
    }

    // vtable-Platz IsPlaying (thiscall ohne Argumente: this in ecx, fastcall mit unbenutztem edx passt)
    static int __fastcall IntroIsPlaying(void* self, void*) {
        int r = IntroOrigIsPlaying ? IntroOrigIsPlaying(self) : 0;
        if (!IntroSyncOn || !self)
            return r;
        if (r && !IntroWasPlaying && !IntroSeen) {
            IntroSeen = true;
            NewGameLog("Intro l" "\xE4" "uft (Video-Spieler %p)", self);
        }
        if (r && IntroRemoteSkip) {
            ((CoopVideoFn)(*(DWORD**)self)[0x20 / 4])(self);   // Stop wie Esc
            IntroWasPlaying = false;
            NewGameLog("Intro vom Partner " "\xFC" "bersprungen - hier auch beendet");
            if (ServerThread)
                IntroSendSkip("eines Mitspielers an alle weitergegeben");
            return 0;
        }
        if (IntroWasPlaying && !r) {
            if (*(int*)((char*)self + 0x20) == 0)
                IntroSendSkip("hier " "\xFC" "bersprungen (Esc)");
            else
                NewGameLog("Intro zu Ende gelaufen");
        }
        IntroWasPlaying = r != 0;
        return r;
    }

    static void IntroHookInstall() {
        DWORD* slot = (DWORD*)INTRO_VT_ISPLAYING;
        if (*slot == (DWORD)&IntroIsPlaying)
            return;
        if (*(DWORD*)INTRO_VT_STOP != INTRO_STOP_FUNC) {   // nicht die erwartete vtable - nichts anfassen
            NewGameLog("Intro-Abgleich aus: vtable-Platz 0x%08X ist nicht zCVideoPlayer::Stop (%08X)", INTRO_VT_STOP, *(DWORD*)INTRO_VT_STOP);
            return;
        }
        DWORD old = 0;
        if (!VirtualProtect(slot, 4, PAGE_EXECUTE_READWRITE, &old)) {
            NewGameLog("Intro-Abgleich aus: VirtualProtect fehlgeschlagen (%lu)", GetLastError());
            return;
        }
        IntroOrigIsPlaying = (CoopVideoFn)*slot;
        *slot = (DWORD)&IntroIsPlaying;
        VirtualProtect(slot, 4, old, &old);
        NewGameLog("Intro-Abgleich: vtable-Platz 0x%08X (IsPlaying) umgelegt, vorher %p", INTRO_VT_ISPLAYING, (void*)IntroOrigIsPlaying);
    }

    void CoopIntroSkipPeek(const ENetEvent& ev) {
        if (!IntroSyncOn || ev.type != ENET_EVENT_TYPE_RECEIVE || !ev.packet || ev.packet->dataLength > 200)
            return;
        // BSON speichert Zeichenketten roh - "introskip" (Wert von op) genuegt, voll gelesen wird das Paket im Spieltakt
        const char* d = (const char*)ev.packet->data;
        const size_t n = ev.packet->dataLength;
        for (size_t i = 0; i + 9 <= n; i++)
            if (memcmp(d + i, "introskip", 9) == 0) {
                InterlockedExchange(&IntroRemoteSkip, 1);
                return;
            }
    }

    static std::string NewGameRun;   // v93: Durchlauf des gemeinsamen Spiels (vom Host), gilt ab LoadEnd

    void CoopNewGameBegin(bool host, const std::string& run) {
        NewGameRun = run;
        IntroHookInstall();
        IntroWasPlaying = IntroSkipSent = IntroSeen = false;
        InterlockedExchange(&IntroRemoteSkip, 0);
        InterlockedExchange(&IntroSyncOn, 1);
        NewGameActive = true;
        NewGameHost = host;
        NewGameBeginMs = GetCurrentMs();
        NewGameLoadEndMs = 0;
        NewGamePlacePending = false;
        NewGameInfoUntil = 0;
        NewGameLog("gemeinsamer Start als %s - Intro-Abgleich an", host ? "Host" : "Mitspieler");
    }

    // UnionHooks: Game_LoadEnd_NewGame (nach LoadEnd)
    void CoopNewGameLoadEnd() {
        if (!NewGameActive)
            return;
        NewGameLoadEndMs = GetCurrentMs();
        if (!NewGameRun.empty())   // v93: Game_LoadBegin_NewGame hat den Durchlauf geleert - jetzt der gemeinsame
            CoopSaveSyncSetRun(NewGameRun, "gemeinsames neues Spiel");
        const bool client = ClientThread && !ServerThread;
        if (!NewGameHost && client) {
            NewGamePlacePending = true;
            NewGameInfoUntil = NewGameLoadEndMs + 180000;
        }
        NewGameLog("Welt geladen nach %lld s%s", (NewGameLoadEndMs - NewGameBeginMs) / 1000,
            NewGamePlacePending ? " - setze mich neben den Host, 3 Minuten keine wichtigen Infos (der Host spricht zuerst)" : "");
    }

    // UnionHooks: Game_Loop
    void CoopNewGamePulse() {
        if (!NewGameActive)
            return;
        const long long now = GetCurrentMs();
        if (IntroSyncOn && NewGameLoadEndMs && now > NewGameLoadEndMs + 5000) {
            InterlockedExchange(&IntroSyncOn, 0);
            InterlockedExchange(&IntroRemoteSkip, 0);
            NewGameLog("Intro-Abgleich aus%s", IntroSeen ? "" : " (kein Intro gesehen)");
        }
        if (NewGamePlacePending && player && !IsLoadingLevel) {
            NewGamePlacePending = false;
            CoopSaveSyncPlaceBesideStart();
        }
        if (NewGameInfoUntil && now >= NewGameInfoUntil) {
            NewGameInfoUntil = 0;
            NewGameLog("3 Minuten vorbei - NPCs melden dem eigenen Helden wieder wichtige Infos");
        }
        if (!IntroSyncOn && !NewGamePlacePending && !NewGameInfoUntil)
            NewGameActive = false;
    }

    bool CoopNewGameInfoBlocked() {
        if (!NewGameInfoUntil)
            return false;
        if (!ClientThread || ServerThread) {   // Verbindung weg - nicht mehr warten
            NewGameInfoUntil = 0;
            return false;
        }
        return GetCurrentMs() < NewGameInfoUntil;
    }

    void CoopNewGameDialogEnded(const std::string& by, const std::string& npc) {
        if (!NewGameInfoUntil)
            return;
        NewGameInfoUntil = 0;
        NewGameLog("%s hat das erste Gespr" "\xE4" "ch beendet (%s) - NPCs melden dem eigenen Helden wieder wichtige Infos", by.c_str(), npc.c_str());
    }

#else
    void CoopNewGameBegin(bool, const std::string&) {}
    void CoopIntroSkipPeek(const ENetEvent&) {}
    void CoopNewGameLoadEnd() {}
    void CoopNewGamePulse() {}
    bool CoopNewGameInfoBlocked() { return false; }
    void CoopNewGameDialogEnded(const std::string&, const std::string&) {}
#endif
}
