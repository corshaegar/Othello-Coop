// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

    // --- v82: "Hauptmenue" im Esc-Menue - zurueck zum Startbildschirm ohne Neustart ----------------------------------------
    //
    // Nutzer (21.10.2026): "Fuege noch die Option Hauptmenue ein, wenn man ESC drueckt im Spiel" - gewaehlt: zurueck zum
    // Startbildschirm, ohne Neustart ("wenn man richtig clean ins Hauptmenue kommt, waere natuerlich noch besser"); "Neues Spiel"
    // gehoert nicht ins Esc-Menue. Gothic 1 kennt den Weg nicht. Was die Engine als "Spiel laeuft" ansieht (G1, geprueft):
    // - ogame->GetCamera() != NULL (zCSession::camera +0x0C). CGameManager::Run (0x00424EA0) ruft Menu(0) so lange, bis eine
    //   Kamera da ist; der Menue-Eintritt (0x00425C0C) haelt nur dann das Spiel an und setzt zCMenu::FrameCallback (0x0086D3F4)
    //   auf 0x00425C90 = Welt hinter dem Menue zeichnen (darum laeuft Game_Loop bei offenem Spielmenue); jeder Frame genauso.
    // - zCMenu::inGameMenu (0x0086D3E8): 0 = Startmenue (Run setzt es um Menu(0) herum, ScreenInit blendet danach ein/aus).
    // - oCNpc::player: oCGame::LoadSavegame setzt ihn selbst auf NULL (0x0063C592), oCGame::LoadWorld (0x0063E530) raeumt die
    //   alte Welt selbst ab (Spawnliste leeren, Welt verwerfen, neu laden) - ein Laden ohne Held und Kamera ist der Startfall.
    // Ablauf: (1) "Ja" in MENU_COOP_TITLE (COOP_MENU.D) -> Coop trennen (wie "Verbindung trennen"), alle Menues schliessen;
    // (2) im Spiel warten, bis die Netzfaeden beendet und aufgeraeumt sind, Figuren der Mitspieler aus der Welt nehmen;
    // (3) Ton aus, Startbild (STARTSCREEN.TGA, wie Othellos Startbildschirm) hinter das Menue, player = NULL, Kamera = NULL,
    // inGameMenu = 0 und CGameManager::Menu(0) wie Run, bis ein Spiel geladen ist oder Beenden gewaehlt wurde. Laden, Neues
    // Spiel, COOP (Lobby wie nach dem Start, player NULL) und Beenden laufen ueber Gothics eigene Wege. Wird nichts geladen,
    // kommen Held und Kamera zurueck (sauberes Ende mit der alten Welt). Aus Game_Loop wie GTs Laden aus der F9-Liste.
    //
    // v84 (Test v83: "es ist abgestürzt nach klicken auf hauptmenü bzw. ist es hängengeblieben", Nutzer: "meine theorie ist dass es
    // geklappt hat aber das hauptmenü nicht angezeigt wurde" - stimmt laut Log: Startmenue aufgebaut, Klicks und Esc liefen):
    // - Kein Bild: zCMenu::Render (0x004D0DA0) ruft bei gesetztem zCMenu::FrameCallback (0x0086D3F4) NUR diesen; 0x00425C90
    //   zeichnet nur mit Kamera (gameSession->GetCamera, sonst ret) - ohne Kamera wurde gar nichts gezeichnet, das letzte Bild
    //   blieb stehen. Gothic setzt den Rueckruf im Menue-Eintritt (0x00425BE0, zCMenu-Rueckruf 0x0086D3EC) nur MIT Kamera und
    //   loescht ihn im Menue-Austritt (0x00425CC0, Rueckruf 0x0086D3F0) nur OHNE Kamera (0x00425D54) - unsere Menues schlossen
    //   noch mit Kamera. Jetzt vor Menu(0) FrameCallback = 0 wie dieser Austritt; Render zeichnet dann screen selbst
    //   (Vid_Clear, BeginFrame, screen->DrawItems, EndFrame, Vid_Blit) wie nach dem Start.
    // - "Ja" musste zweimal geklickt werden: zCMenu::Run (0x004CED79-0x004CED8C) setzt m_exitState (+0xC24) wieder auf 0, wenn
    //   die Eintragsaktion (vtable +0x38) 0 liefert - ein ForceSelAction aus der Aktion heraus wurde sofort ueberschrieben (Log
    //   "m_exitState 2 -> 0 (erzwungen 1)"). Jetzt schliesst GT die Menues im naechsten Menuebild (Game_MenuLoop), wie beim
    //   Laden aus der COOP-Liste (CoopMenuLobby.cpp).
    //
    // v85 (Test v84: "das war schon besser. Jedoch hat sich das Hintergrundbild nicht geaendert und die Menues haben sich
    // uebereinander gestapelt"):
    // - Nur Othello: zCView::Blit (0x006FC8C0, vtable 0x007DEA20 +0x24) springt nach OTHELLO_ABI+0x83240. Ist dort der Merker
    //   OTHELLO_ABI+0x137BDF gesetzt, zeichnet Othello nur Bilder einer festen Liste (BAR_XP.TGA, SAVING.TGA, PROGRESS*.TGA ...),
    //   Ansichten ohne Bild (Texte) weiter. Gesetzt wird er in OTHELLO_ABI+0x83610 genau um Unions Aufruf der Plugin-Takte
    //   (Haken an SHW32+0x58630, Ziel SHW32+0x567F0) - gedacht fuer Lade-/Speicherbilder aus einem Game_Loop. Das Startmenue
    //   laeuft in GTs Game_Loop, also mit Merker: Startbild und Menuebilder fehlten, und weil nichts den Bildschirm deckte,
    //   blieben die Texte frueherer Bilder stehen ("gestapelt"). Jetzt: Merker fuer die Dauer des Startmenues 0, danach der
    //   alte Wert. Adresse aus Othellos Befehl selbst gelesen (cmp byte ptr [Merker], 0 an +0x83240), sonst nichts aendern.
    // - "Ja" weiterhin zweimal (Log v84 "2 -> 0 (erzwungen 1)"): zCMenu::HandleAction (0x004CF500) ruft nach der Eintragsaktion
    //   HandleFrame(0) (0x004CF5FF) - darin lief GTs Game_MenuLoop, das ForceSelAction kam also noch innerhalb der Aktion und
    //   Run setzte m_exitState danach auf 0. Jetzt erst, wenn m_exitState des obersten Menues 0 ist (Run hat zurueckgesetzt).

#if ENGINE == Engine_G1

    static int TitleState = 0;            // 0 nichts, 1 gewaehlt (Coop wird getrennt, Menues schliessen)
    static bool TitleClosePending = false; // v84: Menues im naechsten Menuebild schliessen (CoopTitleMenuPulse)
    static long long TitleSinceMs = 0;
    static zCView* TitleBack = NULL;

    static void TitleLog(const char* fmt, ...) {
        char buf[600];
        va_list ap;
        va_start(ap, fmt);
        _vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        buf[sizeof(buf) - 1] = 0;
        OthelloCompatLog("[%lld s, %s] Hauptmen\xFC: %s", DiagSeconds(), DiagRole(), buf);
    }

    // Startbild wie Othellos Startbildschirm, ganz hinten (vor dem Menue eingehaengt, Menues und Zeiger liegen darueber)
    static zCView* TitleBackView() {
        if (TitleBack)
            return TitleBack;
        const unsigned int size = sizeof(zCView) + 0x20;
        void* mem = ((void*(__cdecl*)(unsigned int))0x0054EBA0)(size);   // Gothics operator new (wie der Menue-Zeiger)
        if (!mem)
            return NULL;
        memset(mem, 0, size);
        TitleBack = ::new (mem) zCView(0, 0, 8192, 8192, zTviewID::VIEW_ITEM);
        TitleBack->InsertBack(zSTRING("STARTSCREEN.TGA"));
        TitleBack->SetColor(zCOLOR(255, 255, 255, 255));
        return TitleBack;
    }

    // v85: Othellos Bildfilter-Merker (nur Othello) - aus dem Befehl "cmp byte ptr [Merker], 0" am Eingang von Othellos Blit
    static unsigned char* TitleOthelloBlitFilter() {
        HMODULE abi = GetModuleHandleW(L"OTHELLO_ABI.DLL");
        if (!abi)
            return NULL;
        const unsigned char* code = (const unsigned char*)abi + 0x83240;
        if (!IsReadableMemory(code, 7) || code[0] != 0x80 || code[1] != 0x3D || code[6] != 0x00)
            return NULL;
        unsigned char* flag = *(unsigned char* const*)(code + 2);
        if (flag != (unsigned char*)abi + 0x137BDF || !IsReadableMemory(flag, 1))
            return NULL;
        return flag;
    }

    // CoopMenuLobby.cpp (LobbyHandleSelAction): "Ja" in MENU_COOP_TITLE
    void CoopTitleRequest(zCMenu* menu) {
        if (!player || TitleState) {
            TitleClosePending = true;
            return;
        }
        const bool coop = ClientThread || ServerThread;
        TitleLog("Spiel verlassen gew\xE4hlt%s", coop ? " - Coop-Verbindung wird getrennt" : "");
        if (coop) {
            CoopPendingRole = 0;
            if (ClientThread)
                CoopClientStop = 1;
            if (ServerThread)
                CoopServerStop = 1;
            Lobby.peers.clear();
            LobbyResetClient();
            Lobby.role = 0;
        }
        TitleState = 1;
        TitleSinceMs = GetCurrentMs();
        TitleClosePending = true;   // alle Menues zu (im naechsten Menuebild), Spiel laeuft noch
    }

    // v90: alle Menues im naechsten Menuebild schliessen, ohne etwas auszuloesen (CoopMenuLobby.cpp: Ladewunsch des Clients)
    void CoopMenuCloseRequest() {
        TitleClosePending = true;
    }

    // UnionHooks.cpp, Game_MenuLoop: CLOSE auf dem obersten Menue - laeuft ueber jede Menue-Ebene zurueck (STARTMENU gibt GO weiter)
    void CoopTitleMenuPulse() {
        if (!TitleClosePending)
            return;
        zCMenu* menu = zCMenu::GetActive();
        if (!menu || *(int*)((char*)menu + 0xC24) != 0)
            return;   // v85: noch in der Eintragsaktion (HandleAction -> HandleFrame(0)) - Run setzt m_exitState gleich auf 0
        TitleClosePending = false;
        menu->ForceSelAction(SEL_ACTION_CLOSE, zSTRING("RESUME_GAME"), NULL);
    }

    // Figuren der Mitspieler sofort aus der Welt (sonst blieben Zeiger auf Objekte der alten Welt in GTs Listen)
    static int TitleRemoveFigures() {
        int n = 0;
        for (auto& kv : SyncNpcs) {
            RemoteNpc* r = kv.second;
            if (r && r->npc && IsCoopPlayer(kv.first)) {
                r->DestroyNpc();
                n++;
            }
        }
        SyncNpcs.clear();
        PlayerNpcs.clear();
        PlayerNameToNpc.clear();
        BroadcastNpcs.clear();
        return n;
    }

    static void TitleRun() {
        CGameManager* gm = gameMan;
        if (!gm || !ogame) {
            TitleLog("kein Spielmanager - abgebrochen");
            return;
        }
        if (zsound)
            zsound->StopAllSounds();
        zCView* back = TitleBackView();
        if (back && screen && !back->ondesk) {
            MouseFixScreenLast();   // v63: Ende-Zeiger der Kinderliste vor dem Einhaengen richtig setzen
            screen->InsertItem(back, 0);
        }
        oCNpc* oldPlayer = oCNpc::player;
        zCCamera* oldCam = ogame->camera;
        oCNpc::player = NULL;
        ogame->camera = NULL;
        int& inGameMenu = *(int*)0x0086D3E8;
        inGameMenu = 0;
        void*& frameCallback = *(void**)0x0086D3F4;   // v84: zCMenu::FrameCallback - ohne Kamera zeichnet 0x00425C90 nichts
        void* const oldFrameCallback = frameCallback;
        frameCallback = NULL;
        unsigned char* const blitFilter = TitleOthelloBlitFilter();   // v85: nur Othello - sonst fehlen Startbild und Menuebilder
        const unsigned char oldBlitFilter = blitFilter ? *blitFilter : 0;
        if (blitFilter)
            *blitFilter = 0;
        TitleLog("Startmen\xFC (Welt bleibt bis zum n\xE4" "chsten Laden im Speicher, Held und Kamera aus; Bild-R\xFC" "ckruf %08X -> 0; "
            "Othellos Bildfilter %s; Startbild %s, %dx%d)", (unsigned)(uintptr_t)oldFrameCallback,
            blitFilter ? (oldBlitFilter ? "1 -> 0" : "war schon 0") : "nicht gefunden",
            back && back->backTex ? "geladen" : "FEHLT", back ? back->psizex : 0, back ? back->psizey : 0);
        int rounds = 0;
        while (!gm->exitGame && rounds < 100000) {
            rounds++;
            ((void(__thiscall*)(CGameManager*, int))0x004279F0)(gm, 0);   // CGameManager::Menu(0) wie Run (0x00424EB9)
            if (ogame && ogame->camera)
                break;
        }
        inGameMenu = 1;
        if (blitFilter)
            *blitFilter = oldBlitFilter;   // Othello setzt ihn nach seinem Aufruf ohnehin selbst auf 0
        if (back && back->ondesk && back->owner) {
            back->owner->RemoveItem(back);
            MouseFixScreenLast();
        }
        if (ogame && ogame->camera) {
            TitleLog("Spiel aus dem Startmen\xFC geladen (%d Men\xFC-Durchl\xE4ufe)", rounds);
            return;
        }
        // Nichts geladen (Beenden): alter Zustand zurueck, damit Gothic mit der alten Welt sauber endet
        if (ogame)
            ogame->camera = oldCam;
        oCNpc::player = oldPlayer;
        frameCallback = oldFrameCallback;
        TitleLog("Startmen\xFC ohne Laden verlassen (%s) - Held und Kamera zur\xFC" "ck", gm->exitGame ? "Beenden" : "unbekannt");
    }

    // Plugin.cpp, Game_Loop (vor CoopSaveSyncPulse). true = Startmenue lief, in diesem Frame nichts mehr tun.
    bool CoopTitlePulse() {
        if (!TitleState)
            return false;
        if (!player || IsLoadingLevel || IsSavingGame || zCMenu::GetActive())
            return false;   // waehrend Laden/Speichern oder bei offenem Menue warten
        const long long waited = GetCurrentMs() - TitleSinceMs;
        if (TitleState == 1) {
            if ((ClientThread || ServerThread) && waited < 8000)
                return false;   // CoopNetPulse raeumt die beendeten Faeden auf
            if (ClientThread || ServerThread)
                TitleLog("Netzf\xE4" "den nach %lld ms noch nicht beendet - weiter", waited);
            const int figs = TitleRemoveFigures();
            TitleLog("Coop beendet nach %lld ms, %d Figur(en) entfernt", waited, figs);
        }
        TitleState = 0;
        TitleRun();
        return true;
    }

#else
    void CoopTitleRequest(zCMenu*) {}
    void CoopMenuCloseRequest() {}
    void CoopTitleMenuPulse() {}
    bool CoopTitlePulse() { return false; }
#endif
}
