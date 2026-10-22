// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

    // --- v81: COOP-Eintrag zur Laufzeit in Othellos Menues einhaengen --------------------------------------------------------
    //
    // Bis v80 ersetzte COOP_MENU.D Othellos MENU_MAIN und MENU_OPTIONS als ganze Kopie - das war Othello-Skript in unserer Datei,
    // und eine Aenderung Othellos an diesen Menues waere ueberschrieben worden. Nutzer (21.10.2026): "COOP-Eintrag zur Laufzeit in
    // Othellos Hauptmenue und Einstellungsmenue einhaengen, COOP_MENU.D nur noch mit unseren eigenen Menues". Jetzt bleiben
    // Othellos Menues, wie sie sind; GT setzt nach dem Aufbau aus dem Skript seine Eintraege in die Eintragsliste:
    // - MENU_MAIN: MENUITEM_COOPMAIN_COOP vor "Neues Spiel". Othello: NEWGAME, SAVEGAME_LOAD, OPTIONS, INTRO, CREDITS, EXIT auf
    //   MENU_MAIN_Y 2500 + MENU_MAIN_DY 550 * 1..6 - sie ruecken um 550 nach unten (nur, wenn sie genau dort stehen; ein erneut
    //   benutzter, schon verschobener Eintrag bleibt). Dazu MENUITEM_COOPMAIN_VERSION am Ende (Fassungszeile, v58).
    // - MENU_OPTIONS: MENUITEM_OPT_COOP an die Stelle von MENUITEM_PERF (MENU_OPT_START_Y 1600 + MENU_OPT_DY 600 * 6, Wunsch des
    //   Nutzers seit v55: "Performance/Qualitaet" durch COOP ersetzen), MENUITEM_PERF_CHOICE faellt aus der Liste.
    // Engine G1 (Speicherabbild geprueft): zCMenu::SetByScript 0x004CE720 parst die Instanz (zCParser::CreateInstance 0x006E9B60
    // in die Felder ab +0x04, itemID[150] als zSTRING ab +0x58) und legt je nicht leerem itemID[i] mit zCMenuItem::Create
    // 0x004D1970 einen Eintrag am Ende von m_listItems (+0xCB8 {Feld, Platz, Anzahl}) an; gezeichnet und positioniert wird erst in
    // ScreenInit (m_parPosY +0x2D4). Othello hat den Eingang von SetByScript durch einen Sprung nach OTHELLO_ABI+0x9CC60 ersetzt
    // (ruft das Original und haengt sich dabei kurz an 0x004CE7E5) - der Platz in der zCMenu-vtable 0x007D2ACC (+0x40) ist
    // unveraendert 0x004CE720. Aufgerufen wird er von zCMenu::Create 0x004CDFC0 (MENU_MAIN aus CGameManager::Menu) und von
    // zCMenu::HandleSelAction 0x004CF951 (SEL_ACTION_STARTMENU, z. B. MENU_OPTIONS). GT biegt den Platz um und arbeitet nach dem
    // bisherigen Inhalt. Die Eintragsliste waechst nur ueber die Engine (zCArray<zCMenuItem*>::AllocDelta 0x004D14A0) - sonst
    // gaebe ~zCMenu spaeter Speicher frei, den GTs Laufzeit angelegt hat. Weg allgemein G1, Namen und Positionen Othello 4.4.2.1.

#if ENGINE == Engine_G1

    typedef void(__thiscall* CoopSetByScriptFn)(zCMenu*, const zSTRING&);
    static CoopSetByScriptFn CoopOrigSetByScript = NULL;

    static void InjectLog(const char* fmt, ...) {
        char buf[600];
        va_list ap;
        va_start(ap, fmt);
        _vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        buf[sizeof(buf) - 1] = 0;
        OthelloCompatLog("[%lld s, %s] Menue-Einhaengen: %s", DiagSeconds(), DiagRole(), buf);
    }

    // zCArray<zCMenuItem*>: Feld, Platz, Anzahl
    struct InjectArray {
        zCMenuItem** items;
        int alloc;
        int num;
    };

    static InjectArray& InjectList(zCMenu* menu) {
        return *(InjectArray*)&menu->m_listItems;
    }

    static int InjectFind(zCMenu* menu, const char* id) {
        InjectArray& a = InjectList(menu);
        for (int i = 0; i < a.num; i++)
            if (a.items[i] && LobbyUpper(a.items[i]->id) == id)
                return i;
        return -1;
    }

    // Eintrag aus unserem Menue-Skript - nur, wenn die Instanz dort steht (ohne sie legte zCMenuItem::Create einen leeren an)
    static zCMenuItem* InjectCreate(const char* id) {
        zCParser* par = zCMenu::GetParser();
        if (!par || !par->GetSymbol(zSTRING(id)))
            return NULL;
        return zCMenuItem::Create(zSTRING(id));
    }

    static bool InjectInsert(zCMenu* menu, int pos, zCMenuItem* item) {
        InjectArray& a = InjectList(menu);
        if (pos < 0 || pos > a.num)
            return false;
        if (a.alloc < a.num + 1)
            ((void(__thiscall*)(void*, int))0x004D14A0)(&menu->m_listItems, 16);   // zCArray<zCMenuItem*>::AllocDelta
        if (a.alloc < a.num + 1 || !a.items)
            return false;
        for (int i = a.num; i > pos; i--)
            a.items[i] = a.items[i - 1];
        a.items[pos] = item;
        a.num++;
        // itemID (Skriptnamen) gleich halten - Othellos Listen haben keine Luecken, Index = Listenplatz
        for (int i = 149; i > pos; i--)
            menu->itemID[i] = menu->itemID[i - 1];
        menu->itemID[pos] = item->id;
        return true;
    }

    static void InjectRemove(zCMenu* menu, int pos) {
        InjectArray& a = InjectList(menu);
        if (pos < 0 || pos >= a.num)
            return;
        for (int i = pos; i < a.num - 1; i++)
            a.items[i] = a.items[i + 1];
        a.num--;
        for (int i = pos; i < 149; i++)
            menu->itemID[i] = menu->itemID[i + 1];
        menu->itemID[149] = zSTRING("");
    }

    static const int INJECT_MAIN_Y = 2500, INJECT_MAIN_DY = 550;
    static const int INJECT_OPT_Y = 1600, INJECT_OPT_DY = 600;
    static const char* const InjectMainItems[] = { "MENUITEM_MAIN_NEWGAME", "MENUITEM_MAIN_SAVEGAME_LOAD", "MENUITEM_MAIN_OPTIONS",
        "MENUITEM_MAIN_INTRO", "MENUITEM_MAIN_CREDITS", "MENUITEM_MAIN_EXIT" };

    static void InjectMain(zCMenu* menu) {
        if (InjectFind(menu, "MENUITEM_COOPMAIN_COOP") >= 0)
            return;   // schon da (erneut aufgebaut, oder ein altes COOP_MENU.D mit eigenem MENU_MAIN)
        InjectArray& a = InjectList(menu);
        const int first = InjectFind(menu, InjectMainItems[0]);
        if (first < 0) {
            InjectLog("Hauptmen\xFC ohne %s (%d Eintr\xE4ge) - COOP nicht eingeh\xE4ngt", InjectMainItems[0], a.num);
            return;
        }
        zCMenuItem* items[6];
        for (int k = 0; k < 6; k++) {
            const int i = InjectFind(menu, InjectMainItems[k]);
            items[k] = i >= 0 ? a.items[i] : NULL;
            const int y = items[k] ? items[k]->m_parPosY : 0;
            const int orig = INJECT_MAIN_Y + INJECT_MAIN_DY * (k + 1);
            if (!items[k] || (y != orig && y != orig + INJECT_MAIN_DY)) {
                InjectLog("Hauptmen\xFC anders als erwartet (%s %s, y %d statt %d) - COOP nicht eingeh\xE4ngt", InjectMainItems[k],
                    items[k] ? "da" : "fehlt", y, orig);
                return;
            }
        }
        zCMenuItem* coop = InjectCreate("MENUITEM_COOPMAIN_COOP");
        if (!coop) {
            InjectLog("MENUITEM_COOPMAIN_COOP fehlt im Men\xFC-Skript (COOP_MENU.D nicht geladen?) - Hauptmen\xFC unver\xE4ndert");
            return;
        }
        if (!InjectInsert(menu, first, coop)) {
            InjectLog("Hauptmen\xFC: Eintrag lie\xDF sich nicht einf\xFCgen (%d/%d) - unver\xE4ndert", a.num, a.alloc);
            return;
        }
        int moved = 0;
        for (int k = 0; k < 6; k++) {
            const int orig = INJECT_MAIN_Y + INJECT_MAIN_DY * (k + 1);
            if (items[k]->m_parPosY == orig) {
                items[k]->m_parPosY = orig + INJECT_MAIN_DY;
                moved++;
            }
        }
        bool version = InjectFind(menu, "MENUITEM_COOPMAIN_VERSION") >= 0;
        if (!version) {
            zCMenuItem* v = InjectCreate("MENUITEM_COOPMAIN_VERSION");
            version = v && InjectInsert(menu, a.num, v);
        }
        static int logged = 0;
        if (logged++ < 5)
            InjectLog("Hauptmen\xFC: COOP vor \"Neues Spiel\" eingeh\xE4ngt (Platz %d), %d Eintr\xE4ge Othellos um %d nach unten, "
                "Versionszeile %s - jetzt %d Eintr\xE4ge", first, moved, INJECT_MAIN_DY, version ? "da" : "FEHLT", a.num);
    }

    static void InjectOptions(zCMenu* menu) {
        if (InjectFind(menu, "MENUITEM_OPT_COOP") >= 0)
            return;
        InjectArray& a = InjectList(menu);
        const int perf = InjectFind(menu, "MENUITEM_PERF");
        const int want = INJECT_OPT_Y + INJECT_OPT_DY * 6;
        if (perf < 0 || a.items[perf]->m_parPosY != want) {
            InjectLog("Einstellungen anders als erwartet (MENUITEM_PERF %s, y %d statt %d) - COOP nicht eingeh\xE4ngt",
                perf < 0 ? "fehlt" : "da", perf < 0 ? 0 : a.items[perf]->m_parPosY, want);
            return;
        }
        zCMenuItem* coop = InjectCreate("MENUITEM_OPT_COOP");
        if (!coop) {
            InjectLog("MENUITEM_OPT_COOP fehlt im Men\xFC-Skript (COOP_MENU.D nicht geladen?) - Einstellungen unver\xE4ndert");
            return;
        }
        a.items[perf] = coop;
        menu->itemID[perf] = coop->id;
        const int choice = InjectFind(menu, "MENUITEM_PERF_CHOICE");
        if (choice >= 0)
            InjectRemove(menu, choice);
        static int logged = 0;
        if (logged++ < 5)
            InjectLog("Einstellungen: COOP statt \"Performance/Qualit\xE4t\" (Platz %d)%s - jetzt %d Eintr\xE4ge", perf,
                choice >= 0 ? ", Auswahlfeld entfernt" : "", a.num);
    }

    // v82: Esc-Menue im Spiel (Othello MENU_MAIN_INGAME: RESUME, LOAD_INGAME, SAVE, OPTIONS_INGAME, EXIT_INGAME) - "Hauptmenue"
    // (MENUITEM_COOPINGAME_TITLE, CoopTitle.cpp) an die Stelle von "Gothic beenden", das um 550 nach unten rueckt.
    // v83: v82 erwartete EXIT_INGAME auf 2500 + 550 * 5 = 5250; Othello 4.4.2.1 setzt es auf 5525 (Log 21.10.2026, 23:41: "y 5525
    // statt 5250") - der Eintrag wurde nie eingehaengt, Nutzer: "hab kein hauptmenü gesehen". Jetzt gilt die Stelle, an der
    // EXIT_INGAME beim ersten Aufbau steht; ein schon verschobener (wieder benutzter) Eintrag steht 550 tiefer und bleibt.
    // v91 (Nutzer: "wie im Verbindungsmenue gibt's ja dieses Spielstand waehlen. Das brauchen wir noch im Spiel bei Esc"): dazu
    // "Spielstand waehlen" (MENUITEM_COOPINGAME_LOAD -> MENU_COOP_LOAD) hinter "Spiel laden". Lagen aus Othellos erstem Aufbau
    // (Eintraege werden wiederverwendet und behalten unsere Lage): Eintraege zwischen "Spiel laden" und "Gothic beenden" +550,
    // "Hauptmenue" an Othellos Stelle von "Gothic beenden" +550, "Gothic beenden" +1100 (Othello 4.4.2.1: 5525 -> 6625).
    static std::map<std::string, int> IngameOrigY;

    static int IngameOrig(zCMenuItem* it) {
        const std::string id = LobbyUpper(it->id);
        auto f = IngameOrigY.find(id);
        if (f != IngameOrigY.end())
            return f->second;
        IngameOrigY[id] = it->m_parPosY;
        return it->m_parPosY;
    }

    static void InjectIngame(zCMenu* menu) {
        InjectArray& a = InjectList(menu);
        int exit = InjectFind(menu, "MENUITEM_MAIN_EXIT_INGAME");
        int load = InjectFind(menu, "MENUITEM_MAIN_SAVEGAME_LOAD_INGAME");
        if (exit < 0 || load < 0) {
            static int warned = 0;
            if (warned++ < 5)
                InjectLog("Esc-Men\xFC anders als erwartet (EXIT_INGAME %s, SAVEGAME_LOAD_INGAME %s) - nichts eingeh\xE4ngt",
                    exit < 0 ? "fehlt" : "da", load < 0 ? "fehlt" : "da");
            return;
        }
        for (int i = 0; i < a.num; i++)   // Othellos Lagen beim ersten Aufbau merken (unsere Eintraege kommen danach)
            if (a.items[i] && LobbyUpper(a.items[i]->id).compare(0, 19, "MENUITEM_COOPINGAME") != 0)
                IngameOrig(a.items[i]);
        const int exitY = IngameOrig(a.items[exit]);
        const int loadY = IngameOrig(a.items[load]);
        if (exitY < INJECT_MAIN_Y || exitY > 8192 - 4 * INJECT_MAIN_DY || loadY >= exitY) {
            static int warned = 0;
            if (warned++ < 5)
                InjectLog("Esc-Men\xFC anders als erwartet (Spiel laden y %d, Gothic beenden y %d) - nichts eingeh\xE4ngt", loadY, exitY);
            return;
        }
        zCMenuItem* title = NULL;
        zCMenuItem* coopLoad = NULL;
        int t = InjectFind(menu, "MENUITEM_COOPINGAME_TITLE");
        if (t < 0) {
            title = InjectCreate("MENUITEM_COOPINGAME_TITLE");
            if (title && !InjectInsert(menu, exit, title))
                title = NULL;
        }
        else
            title = a.items[t];
        load = InjectFind(menu, "MENUITEM_MAIN_SAVEGAME_LOAD_INGAME");
        int l = InjectFind(menu, "MENUITEM_COOPINGAME_LOAD");
        if (l < 0) {
            coopLoad = InjectCreate("MENUITEM_COOPINGAME_LOAD");
            if (coopLoad && !InjectInsert(menu, load + 1, coopLoad))
                coopLoad = NULL;
            // Vorauswahl ist ein Listenplatz (defaultingame/defaultoutgame) - beim selben Eintrag lassen
            if (coopLoad) {
                if (menu->defaultInGame > load)
                    menu->defaultInGame++;
                if (menu->defaultOutGame > load)
                    menu->defaultOutGame++;
            }
        }
        else
            coopLoad = a.items[l];
        const int dy = INJECT_MAIN_DY;
        const int shift = coopLoad ? dy : 0;
        for (int i = 0; i < a.num; i++) {
            zCMenuItem* it = a.items[i];
            if (!it)
                continue;
            const std::string id = LobbyUpper(it->id);
            if (it == title)
                it->m_parPosY = exitY + shift;   // im Skript MENU_MAIN_Y + DY * 5 - zaehlt nicht
            else if (it == coopLoad)
                it->m_parPosY = loadY + dy;
            else if (id == "MENUITEM_MAIN_EXIT_INGAME")
                it->m_parPosY = exitY + shift + (title ? dy : 0);
            else {
                const int o = IngameOrig(it);
                it->m_parPosY = (o > loadY && o < exitY) ? o + shift : o;
            }
        }
        static int logged = 0;
        if (logged++ < 5)
            InjectLog("Esc-Men\xFC: \"Spielstand w\xE4hlen\" %s, \"Hauptmen\xFC\" %s (Spiel laden y %d, Gothic beenden jetzt y %d) - jetzt %d Eintr\xE4ge",
                coopLoad ? "eingeh\xE4ngt" : "FEHLT (COOP_MENU.D zu alt?)", title ? "eingeh\xE4ngt" : "FEHLT", loadY,
                a.items[InjectFind(menu, "MENUITEM_MAIN_EXIT_INGAME")]->m_parPosY, a.num);
    }

    static void __fastcall CoopSetByScriptHook(zCMenu* menu, void* edx, const zSTRING& name) {
        CoopOrigSetByScript(menu, name);
        try {
            if (!menu)
                return;
            const std::string n = LobbyUpper(name);
            if (n == "MENU_MAIN")
                InjectMain(menu);
            else if (n == "MENU_OPTIONS")
                InjectOptions(menu);
            else if (n == "MENU_MAIN_INGAME")
                InjectIngame(menu);
        }
        catch (...) {
            InjectLog("Ausnahme beim Einh\xE4ngen in %s - Men\xFC bleibt, wie Othello es aufgebaut hat", name.ToChar());
        }
    }

    // Plugin.cpp, Game_Entry: vor dem ersten Menue
    void CoopMenuInjectInstall() {
        static bool done = false;
        if (done)
            return;
        done = true;
        DWORD* const vtable = (DWORD*)0x007D2ACC;   // zCMenu-vtable: +0x00 HandleEvent 0x004CEE10, +0x3C HandleSelAction 0x004CF6E0
        DWORD* const slot = vtable + 0x40 / 4;
        if (!IsReadableMemory(vtable, 0x48) || vtable[0] != 0x004CEE10 || vtable[0x3C / 4] != 0x004CF6E0) {
            InjectLog("zCMenu-vtable nicht erkannt (%08X) - COOP-Eintr\xE4ge werden nicht eingeh\xE4ngt",
                IsReadableMemory(vtable, 4) ? vtable[0] : 0);
            return;
        }
        const DWORD before = *slot;
        DWORD old = 0;
        if (!VirtualProtect(slot, 4, PAGE_READWRITE, &old)) {
            InjectLog("VirtualProtect f\xFCr den SetByScript-Platz fehlgeschlagen (%lu)", GetLastError());
            return;
        }
        CoopOrigSetByScript = (CoopSetByScriptFn)before;
        *slot = (DWORD)&CoopSetByScriptHook;
        VirtualProtect(slot, 4, old, &old);
        char mod[96];
        CoopModuleOf((const void*)before, mod, sizeof(mod));
        InjectLog("zCMenu::SetByScript (vtable 007D2B0C) eingehakt, bisher %08X (%s)%s", before, mod,
            before == 0x004CE720 ? "" : " - NICHT das Original, wird weitergereicht");
    }

#else
    void CoopMenuInjectInstall() {}
#endif
}
