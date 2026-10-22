// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

    // --- v57: Maus in Gothics Menues (20.10.2026) -----------------------------------------------------------------------
    //
    // Nutzer: "im Menue soll die Maus verfuegbar sein, vielleicht hilft da GRawInput der auch fuer die advanced Inventory
    // verwendet wird"; Zeiger: Othellos OTHELLO_CURSOR.TGA. Gilt fuer jedes Gothic-Menue (Hauptmenue, Einstellungen, COOP,
    // Laden/Speichern, Spielmenue mit Esc). Geprueft am Speicherabbild:
    //
    // Gothic selbst (allgemein, G1-Engine): zCMenu::HandleFrame (0x004CF470) -> zCInputCallback::GetInput (0x006FB9D0) holt
    // eine Taste je Bild aus dem Tastenpuffer und schickt bei eingeschalteter Maus (enableMouse=1) zusaetzlich 0x802 (links),
    // 0x804 (rechts), 0x809/0x80A (Rad). zCMenu::HandleEvent (0x004CEE10, Othello leitet ueber +0x9CEE0 weiter) macht daraus
    // RETURN (auf dem gerade markierten Eintrag), ESCAPE, UP, DOWN. Es fehlen Zeiger und "Eintrag unter der Maus". Ausserdem
    // feuert 0x804 in jedem Bild, solange rechts gehalten wird: GetInput vergleicht mit dem Vorzustand von LINKS (0x008DE1B0),
    // und die Knopfzustaende 0x0086CCB8/0x0086CCC0 setzt nur ein Maus-Ereignis (0x004C800A) - ein Rechtsklick konnte also
    // mehrere Menue-Ebenen schliessen.
    //
    // Neu:
    //  - Zeiger frei (nur mit GRawInput, Othello): system\DINPUT.dll setzt den Windows-Zeiger jedes Bild in die Fenstermitte,
    //    ausser ein Rueckruf aus RegisterWrapCheck (Export, stdcall, 0x1000A1F0, std::list) gibt false zurueck (Schleife bei
    //    +0x7D10: erstes false -> nicht zentrieren). Othello meldet dort seine Fenster an (Inventar); wir melden "Menue offen".
    //  - Zeiger zeichnen: eigenes zCView mit OTHELLO_CURSOR.TGA (Speicher von Gothics operator new 0x0054EBA0, wird nie
    //    freigegeben), Kind von screen mit stayOnTop (zCView::InsertItem 0x00701EA0, Flag 0x800 - spaeter geoeffnete Menues
    //    liegen darunter). zCMenu::Render (0x004D0DA0) zeichnet screen; Union ruft Game_MenuLoop erst danach - die neue Lage
    //    erscheint im naechsten Bild. Ohne offenes Menue (Game_Loop) und beim Laden wird es entfernt (RemoveItem 0x007020E0).
    //  - Ueberfahren (nur wenn die Maus sich bewegt): Eintrag unter dem Zeiger (pposx/pposy/psizex/psizey, CalcPixelPos rechnet
    //    absolut) markieren wie zCMenu::NextOption (0x004CEAA0): SetIsActive(0), HandleLeaveItem (vtable +8), m_mainSel,
    //    HandleEnterItem (+4), SetIsActive(1) bei cursorEn; dazu browseSnd wie HandleEvent (zsound vtable +0x28). Ein
    //    Wahlfeld/Regler hinter einem Text mit IT_EFFECTS_NEXT (0x80) markiert den Text (HandleEvent wirkt dann aufs naechste).
    //  - Linksklick: Gothics 0x802 abgeschaltet (Vorzustand 0x008DE1B0 = 1; nur GetInput liest ihn), wir legen selbst eine Taste
    //    in den Tastenpuffer (zCArray<int> 0x0086D2DC wie zCInput_Win32 bei 0x004C7DD1, Vergroessern mit Gothics AllocDelta
    //    0x00417FA0): Eintrag -> RETURN, Auswahlfeld -> SPACE (= rechts mit Umlauf; SPACE ist keyWeapon, laeuft in HandleEvent
    //    den Rohweg), Regler -> LEFT/RIGHT je nach Haelfte. Klick ins Leere tut nichts. GD3D11 (g1_avx2) reicht GetKey durch.
    //  - Rechtsklick: bleibt Gothics 0x804 (= ESCAPE), aber nur einmal je Klick (rechter Zustand nach jedem Menuebild auf 0).
    //  - Rad: bleibt Gothics 0x809/0x80A (hoch/runter).
    //  - Texteingabe (zCMenuItemInput::Run meldet das Eingabefeld als Empfaenger an, GetEnableHandleEvent 0x006FB970 = 1):
    //    kein Markieren; ein Linksklick schickt RETURN (= Eingabe fertig).
    // Gothic.ini [GOTHICCOOP]: coopMenuMouse=0 schaltet alles ab; coopMenuCursor (Textur), coopMenuCursorSize (Pixel, 0 = auto).
    //
    // v86 (Nutzer 22.10.2026: "Wenn eine Option gewaehlt ist, soll diese gewaehlt werden, auch wenn die Maus nicht draufzeigt.
    // Wenn man eine Option z. B. mit Mausrad oder Hoch/Runter waehlt, dann soll Linksklick das auswaehlen, auch wenn die Maus auf
    // etwas anderes zeigt"): Hat der Spieler im aktiven Menue selbst gewaehlt - Rad/Pfeiltasten (m_mainSel aendert sich ohne GT)
    // oder Ueberfahren mit bewegter Maus -, gilt ein Linksklick dem markierten Eintrag, wo der Zeiger auch steht (Wertfeld des
    // markierten Eintrags: wie bisher zurueck/weiter je Haelfte). Schutz: in einem gerade geoeffneten Menue zaehlt die
    // Vorauswahl erst nach eigener Wahl dort - sonst loeste ein schneller zweiter Klick nach "Gothic beenden" das vorgewaehlte
    // "Ja" aus. Ohne eigene Wahl wie bisher: Eintrag unter dem Zeiger, Klick ins Leere tut nichts. Allgemein G1.

    void CoopMenuMousePulse();
    void CoopMenuMouseHide();
    void CoopMenuMouseGameFrame();

#if ENGINE == Engine_G1

    static bool MouseInitDone = false;
    static bool MouseEnabled = true;
    static bool MouseWrapRegistered = false;
    static std::string MouseCursorTex = "OTHELLO_CURSOR.TGA";
    static int MouseCursorSize = 0;
    static zCView* MouseView = NULL;
    static volatile bool MouseMenuOn = false;
    static volatile DWORD MouseMenuTick = 0;
    static int MouseLastX = -1, MouseLastY = -1;
    static bool MouseLeftWas = false;
    static zCMenu* MouseLoggedMenu = NULL;
    static int MouseClicksLogged = 0;
    static const char* MouseHiddenWhy = NULL;   // v58: warum der Zeiger gerade nicht gezeigt wird (einmal je Wechsel ins Log)
    static bool MouseShownLogged = false;
    static int MouseReorderLogged = 0;
    // v86: eigene Wahl im aktiven Menue (Rad/Pfeiltasten/Ueberfahren) - dann gilt der Linksklick dem markierten Eintrag
    static zCMenu* MouseSelMenu = NULL;    // Menue, fuer das MouseSelSeen/MouseUserSel gelten (NULL = kein Menue offen)
    static int MouseSelSeen = -1;          // m_mainSel beim letzten Menuebild
    static DWORD MouseSelSince = 0;        // seit wann dieses Menue aktiv ist
    static bool MouseUserSel = false;
    static int MouseUserSelLogged = 0;
    static unsigned MouseSelectSeq = 0;    // v93: jede Markierung durch die Maus (Liste "Spielstand waehlen": kein Blaettern)
    // v99 (H27c): Tagebuch - siehe MouseListWatch
    static bool MouseSwallowLeft = false;     // nach einem Klick, den GT umsetzt: Gothics Linksklick aus, bis der Knopf los ist
    static zCMenuItem* MouseListRun = NULL;   // Themenliste, deren Schleife (zCMenuItemList::Run) laeuft
    static DWORD MouseListLostSince = 0;      // seit wann sie weder oben liegt noch ein Thema offen ist

    unsigned CoopMenuMouseSelectSeq() {
        return MouseSelectSeq;
    }

    static void MouseLog(const char* fmt, ...) {
        char buf[700];
        va_list ap;
        va_start(ap, fmt);
        _vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        buf[sizeof(buf) - 1] = 0;
        OthelloCompatLog("[%lld s, %s] Men" COOP_UE "-Maus: %s", DiagSeconds(), DiagRole(), buf);
    }

    static void MouseDumpScreen(zCMenu* menu, const char* why);
    static DWORD MouseDiagStart = 0;   // v61: erster Menue-Takt (Start der Lebenslauf-Diagnose, 60 s)

    // GRawInput fragt das jedes Bild ab: false = Zeiger nicht in die Fenstermitte setzen (true laesst andere Rueckrufe,
    // z. B. Othellos Inventar, weiter entscheiden)
    static bool __cdecl MouseWrapCheck() {
        if (MouseMenuOn && GetTickCount() - MouseMenuTick < 500)
            return false;
        // v61-Diagnose ("beim Start nur Hintergrundbild, Hauptmenue erst nach Esc"): GRawInput fragt auch ohne Menue - in der
        // ersten Minute alle 2 s melden, wenn kein Menue aktiv ist (Spiel ohne Welt = Hintergrund-/Schwarzbild)
        if (MouseDiagStart && GetTickCount() - MouseDiagStart < 60000 && !zCMenu::GetActive()) {
            static DWORD last = 0;
            if (GetTickCount() - last > 2000) {
                last = GetTickCount();
                MouseDumpScreen(NULL, "kein Men" COOP_UE " aktiv");
            }
        }
        return true;
    }

    static void MouseInit() {
        if (MouseInitDone)
            return;
        MouseInitDone = true;
        MouseEnabled = IniInt("coopMenuMouse", 1) != 0;
        MouseCursorTex = IniStr("coopMenuCursor", "OTHELLO_CURSOR.TGA");
        MouseCursorSize = IniInt("coopMenuCursorSize", 0);
        if (!MouseEnabled) {
            MouseLog("aus (Gothic.ini [GOTHICCOOP] coopMenuMouse=0)");
            return;
        }
        typedef int(__stdcall* RegisterWrapCheckFn)(bool(__cdecl*)());
        HMODULE h = GetModuleHandleA("DINPUT.DLL");
        RegisterWrapCheckFn reg = h ? (RegisterWrapCheckFn)GetProcAddress(h, "RegisterWrapCheck") : NULL;
        if (reg) {
            reg(&MouseWrapCheck);
            MouseWrapRegistered = true;
        }
        MouseLog("an - Zeiger %s, Gr" COOP_OE "%se %s, GRawInput %s", MouseCursorTex.c_str(),
            "\xDF", MouseCursorSize > 0 ? std::to_string(MouseCursorSize).c_str() : "auto",
            MouseWrapRegistered ? "angemeldet (RegisterWrapCheck)" : "nicht gefunden - Zeiger evtl. in der Fenstermitte gefangen");
    }

    static zCView* MouseCursorView() {
        if (MouseView)
            return MouseView;
        const unsigned int size = sizeof(zCView) + 0x20;
        void* mem = ((void*(__cdecl*)(unsigned int))0x0054EBA0)(size);   // Gothics operator new
        if (!mem)
            return NULL;
        memset(mem, 0, size);
        MouseView = ::new (mem) zCView(0, 0, 256, 256, zTviewID::VIEW_ITEM);
        MouseView->InsertBack(zSTRING(MouseCursorTex.c_str()));
        zTRnd_AlphaBlendFunc blend = zRND_ALPHA_FUNC_BLEND;
        MouseView->SetAlphaBlendFunc(blend);
        MouseView->SetTransparency(255);
        MouseView->SetColor(zCOLOR(255, 255, 255, 255));
        MouseLog("Zeiger angelegt (%s, Textur %s)", MouseCursorTex.c_str(), MouseView->backTex ? "geladen" : "FEHLT");
        return MouseView;
    }

    // Taste in Gothics Tastenpuffer legen - wie zCInput_Win32 (0x004C7DD1); GetInput holt sie im naechsten Bild ab
    static void MousePushKey(int key) {
        int*& data = *(int**)0x0086D2DC;
        int& alloc = *(int*)0x0086D2E0;
        int& num = *(int*)0x0086D2E4;
        if (alloc < num + 1)
            ((void(__thiscall*)(void*, int))0x00417FA0)((void*)0x0086D2DC, alloc < 16 ? 16 : alloc / 2);   // zCArray<int>::AllocDelta
        if (!data || alloc < num + 1)
            return;
        data[num++] = key;
    }

    static bool MouseInRect(zCView* v, int x, int y) {
        return v && v->ondesk && v->psizex > 0 && v->psizey > 0 &&
            x >= v->pposx && y >= v->pposy && x < v->pposx + v->psizex && y < v->pposy + v->psizey;
    }

    // Eintrag unter dem Zeiger: Index des waehlbaren Eintrags; valueIdx = Wahlfeld/Regler dahinter (IT_EFFECTS_NEXT), sonst -1
    static int MouseHit(zCMenu* menu, int x, int y, int& valueIdx) {
        valueIdx = -1;
        const int n = menu->m_listItems.GetNum();
        for (int i = n - 1; i >= 0; i--) {   // spaeter eingefuegt = obenauf
            zCMenuItem* it = menu->m_listItems[i];
            if (!MouseInRect(it, x, y))
                continue;
            if (it->GetIsSelectable())
                return i;
            if (i > 0) {
                zCMenuItem* label = menu->m_listItems[i - 1];
                // Wahlfeld/Regler hinter IT_EFFECTS_NEXT - oder (v61, Test v60: "die Auswahlfelder auch anklickbar, nicht nur der
                // Text") ein Eingabe-/Auswahl-/Reglerfeld in derselben Zeile wie der waehlbare Eintrag davor: die Beschriftung
                // eines Eingabefelds startet es mit SEL_ACTION_STARTITEM (RETURN), hat aber kein IT_EFFECTS_NEXT
                const bool valueType = it->m_parType == MENU_ITEM_INPUT || it->m_parType == MENU_ITEM_CHOICEBOX || it->m_parType == MENU_ITEM_SLIDER;
                const bool sameRow = label && label->psizey > 0 &&
                    it->pposy < label->pposy + label->psizey && label->pposy < it->pposy + it->psizey;
                if (label && label->GetIsSelectable() && ((label->m_parItemFlags & IT_EFFECTS_NEXT) || (valueType && sameRow))) {
                    valueIdx = i;
                    return i - 1;
                }
            }
        }
        return -1;
    }

    // wie zCMenu::NextOption (0x004CEAA0), nur mit festem Ziel
    static void MouseSelect(zCMenu* menu, int idx) {
        const int n = menu->m_listItems.GetNum();
        if (idx < 0 || idx >= n || idx == menu->m_mainSel)
            return;
        MouseSelectSeq++;
        zCMenuItem* now = menu->m_listItems[idx];
        if (!now || !now->GetIsSelectable())
            return;
        typedef int(__thiscall* ItemFn)(zCMenu*, zCMenuItem*);
        const int old = menu->m_mainSel;
        if (old >= 0 && old < n && menu->m_listItems[old]) {
            zCMenuItem* was = menu->m_listItems[old];
            was->SetIsActive(0);
            ((ItemFn)(*(DWORD**)menu)[0x08 / 4])(menu, was);   // HandleLeaveItem
        }
        menu->m_mainSel = idx;
        ((ItemFn)(*(DWORD**)menu)[0x04 / 4])(menu, now);       // HandleEnterItem
        if (menu->cursorEn)
            now->SetIsActive(1);
        // v77 (Nutzer: "Vorschaubild der Savegames direkt anzeigen, wenn man mit der Maus rueber geht"): Gothics Ladefenster
        // (oCMenuSavegame, vtable 0x007D115C) wechselt Bild und Angaben nur in HandleEvent (0x0042C830, also nur per Taste):
        // m_selSlot (+0xCCC) aus dem markierten Eintrag (GetMenuItemSlotNr 0x0042D6E0), bei Wechsel HandleSlotChange (vtable
        // +0x48 - unter Othello OTHELLO_ABI+0xC2DE0, ruft das Original 0x0042C420 und setzt den Schwierigkeitsgrad). Allgemein G1.
        if (*(DWORD*)menu == 0x007D115C) {
            int* selSlot = (int*)((char*)menu + 0xCCC);
            const int oldSlot = *selSlot;
            *(int*)((char*)menu + 0xCD0) = idx;
            typedef int(__thiscall* SlotNrFn)(zCMenu*, zCMenuItem*);
            *selSlot = ((SlotNrFn)0x0042D6E0)(menu, now);
            if (*selSlot != oldSlot) {
                typedef void(__thiscall* SlotChangeFn)(zCMenu*, int);
                ((SlotChangeFn)(*(DWORD**)menu)[0x48 / 4])(menu, oldSlot);
            }
        }
        if (zCMenu::browseSnd && zsound) {
            typedef int(__thiscall* PlayFn)(void*, zCSoundFX*, int);
            ((PlayFn)(*(DWORD**)zsound)[0x28 / 4])(zsound, zCMenu::browseSnd, 0);
        }
    }

    // v62: Auswahlfeld/Regler, den ein Linksklick auf den (markierten) Eintrag sel umschaltet - wie zCMenu::HandleEvent: bei
    // IT_EFFECTS_NEXT das naechste Element, sonst das angeklickte Wertfeld selbst; NULL = RETURN schicken
    static zCMenuItem* MouseToggleTarget(zCMenu* menu, int sel, int valueIdx) {
        const int n = menu->m_listItems.GetNum();
        zCMenuItem* it = menu->m_listItems[sel];
        zCMenuItem* t = NULL;
        if (it && (it->m_parItemFlags & IT_EFFECTS_NEXT) && sel + 1 < n)
            t = menu->m_listItems[sel + 1];
        else if (valueIdx >= 0 && valueIdx < n)
            t = menu->m_listItems[valueIdx];
        if (t && (t->m_parType == MENU_ITEM_CHOICEBOX || t->m_parType == MENU_ITEM_SLIDER))
            return t;
        return NULL;
    }

    // v62: oberster Eingabe-Empfaenger (zCInputCallback::inputList 0x008DE1A4, erster Knoten bei +4 = 0x008DE1A8, Knoten
    // {Daten +0, naechster +4} - SetHandleEventTop 0x006FB880 / GetInput 0x006FB9ED lesen ihn so)
    static zCInputCallback* MouseTopInput() {
        void** node = *(void***)0x008DE1A8;
        return node ? (zCInputCallback*)node[0] : NULL;
    }

    static bool MouseMenuOnTopWas = true;
    static zCInputCallback* MouseTopWas = NULL;
    static int MouseTopLogged = 0;

    // v59: Zeiger als letztes Kind von screen halten. Test v58: "Maus sichtbar, jedoch hinter der Schrift und hinter den
    // Fenstern". zCView::DrawItems (0x006FCBD0) zeichnet die Kinder vom ersten (zList root, screen+0x34) ueber next (+0x08)
    // bis zum letzten; zCView::InsertItem setzt stayOnTop fuer modale Fenster (Flag 0x40, 0x00701ED5) selbst und haengt sie
    // ans Ende (InsertLast) - ein danach geoeffnetes Menue lag also hinter dem Zeiger und wurde darueber gezeichnet.
    // v63: Ende-Zeiger der Kinderliste von screen reparieren. Ursache von "beim Start nur Hintergrundbild, Hauptmenue erst nach
    // Esc" (seit v58) und "kurz zu sehen, dann weg" (v62): zList<zCView> (screen+0x28: Anzahl +0x2C, Ende +0x30, Anfang +0x34,
    // Kette ueber zCView::next +0x08). zCView::InsertItem ohne stayOnTop haengt nach dem letzten normalen Kind an (0x007020D0:
    // new->next = e->next, e->next = new, Anzahl+1) und fuehrt das Ende NICHT nach; mit stayOnTop nimmt es
    // zList::InsertLast (0x00702C40: Ende->next = new, new->next = 0, Ende = new). Der Zeiger (stayOnTop) landete so hinter dem
    // veralteten Ende (Kind [3]) und schnitt alles danach ab - Othellos Versionsleiste und das Fenster des Hauptmenues (blieb
    // "ondesk", fehlte aber in der Kette; Diagnose v62). Allgemeiner Engine-Fehler, trifft jeden, der InsertLast nach normalem
    // Einhaengen benutzt. GT stellt das Ende vor jedem eigenen Einhaengen und je Menuebild auf das echte letzte Kind.
    static int MouseLastFixed = 0;

    static void MouseFixScreenLast() {
        if (!screen)
            return;
        zCView* c = *(zCView**)((char*)screen + 0x34);
        for (int guard = 0; c && c->next && guard < 4096; guard++)
            c = c->next;
        zCView*& last = *(zCView**)((char*)screen + 0x30);
        if (last != c) {
            if (MouseLastFixed < 10) {
                MouseLastFixed++;
                MouseLog("Ende-Zeiger von screen veraltet (%p statt %p) - korrigiert (%d. Mal)", (void*)last, (void*)c, MouseLastFixed);
            }
            last = c;
        }
    }

    static bool MouseIsLastChild(zCView* v) {
        zCView* c = *(zCView**)((char*)screen + 0x34);
        for (int guard = 0; c && c->next && guard < 4096; guard++)
            c = c->next;
        return c == v;
    }

    // v64: markierter Eintrag mit Bereichspruefung. zCMenu::GetActiveItem (0x004D0EF0) liest m_listItems[m_mainSel] (+0xCB8,
    // Index +0xC40) ungeprueft - bei einem Menue ohne Eintraege ist das Feld NULL: Absturz. Test v63: "Bei Steuerung stuerzt das
    // Spiel ab" - Othellos MENU_OTHELLO_CONTROLS hat im Skript keine Eintraege (MENU.DAT: nur backpic; OTHELLO_ABI fuehrt es
    // selbst unter dem Namen, Zeichenkette 0x1011F2D0), GTs Menue-Maus fragte im ersten Bild den aktiven Eintrag ab.
    static zCMenuItem* MouseActiveItem(zCMenu* menu) {
        const int sel = menu->m_mainSel;
        return sel >= 0 && sel < menu->m_listItems.GetNum() ? menu->m_listItems[sel] : NULL;
    }

    // Texteingabe laeuft: zCMenuItemInput::Run (0x004D541D) meldet das Eingabefeld selbst (zCInputCallback bei +4) als
    // Empfaenger an und laesst es oben die Tasten nehmen
    static bool MouseEditing(zCMenu* menu) {
        zCMenuItem* it = MouseActiveItem(menu);
        return it && it->m_parType == MENU_ITEM_INPUT && static_cast<zCInputCallback*>(it)->GetEnableHandleEvent();
    }

    // v92 (Test v91: "Im Tagebuch konnte ich Linksklick nicht verwenden, um den Eintrag anzuzeigen"): Nach der Wahl einer Kategorie
    // laeuft Gothics Themenliste. zCMenuItemList::Run (0x004D8B50) meldet sie mit SetEnableHandleEvent(1) (0x006FB8F0 -> zList-Einfuegen
    // vorn 0x00702B80) als obersten Eingabe-Empfaenger an und dreht eine eigene Schleife ueber zCMenu::HandleFrame, bis m_bLeaveItem
    // (+0x374) gesetzt ist. Ihr HandleEvent (0x004D9640) kennt RETURN und Gothics 0x802 -> ShowContent (0x004D98A0, der Aufruf bei
    // 0x004D96EF geht unter Othello an zUtilities), ESC/0x804 -> zurueck, Rad/Pfeile -> naechstes/voriges Thema. Bis v91 sah GT nur
    // "Eingabe oben ist nicht das Menue" und tat nichts - Gothics Klick kommt aber nur mit Gothics eigener Maus (Steuerung) an.
    // Jetzt: Liste oben -> Gothics Links-Code aus, Linksklick = RETURN (zeigt das markierte Thema). Die Zeilen zeichnet zUtilities
    // (vtable 0x007D2BE8 +0x34 DrawFront -> ZUTILITIES), darum kein Markieren unter dem Zeiger - waehlen per Rad/Pfeiltasten.
    // Allgemein G1 (Gothics Liste); zUtilities nur Othello.
    static zCMenuItem* MouseRunningItem(zCMenu* menu, zCInputCallback* top, int type) {
        if (!menu || !top)
            return NULL;
        for (int i = 0; i < menu->m_listItems.GetNum(); i++) {
            zCMenuItem* it = menu->m_listItems[i];
            if (it && it->m_parType == type && static_cast<zCInputCallback*>(it) == top)
                return it;
        }
        return NULL;
    }

    static zCMenuItem* MouseRunningList(zCMenu* menu, zCInputCallback* top) {
        return MouseRunningItem(menu, top, MENU_ITEM_LISTBOX);
    }

    // v99 (H27c): Test v98 (Host): "Man kann nicht den Eintrag anklicken, und sobald man Esc drueckt, kommt man nicht mehr aus dem
    // Tagebuch raus". Log: Klick auf die Themenliste -> RETURN -> Thema offen (zCMenuItemText, vtable 007D2DF8) -> im selben Moment
    // oben wieder das Menue (oCMenu_Log 007D1FD4), nicht die Liste; danach 45 s nur Klicks auf MENU_ITEM_SEL_MISSIONS_ACT ohne
    // Wirkung. Beim Mitspieler ging es meist (Thema -> Liste -> Menue). Engine (Speicherabbild): zCMenuItemList::Run (0x004D8B50,
    // unveraendert) meldet die Liste an (SetEnableHandleEvent 0x006FB8F0), setzt m_bLeaveItem (+0x374) = 0 und dreht HandleFrame,
    // bis m_bLeaveItem gesetzt ist - das setzt nur ihr HandleEvent (0x004D9640: ESC/0x804; RETURN/0x802 -> ShowContent, unter
    // Othello an ZUTILITIES.DLL+0x24FB0). Verliert die Liste die Anmeldung, laufen alle Tasten ins Menue: Esc setzt dessen
    // m_exitState, das in der Schleife der Liste niemand prueft - man kommt nicht mehr heraus. Wer sie abmeldet: zUtilities beim
    // Schliessen des Themas (Gothics eigenes ShowContent meldet nichts an oder ab). Jetzt: (1) nach einem Klick bleibt Gothics
    // Linksklick aus, bis der Knopf los ist (der gehaltene Knopf schloss das gerade geoeffnete Thema); (2) Thema offen: Linksklick
    // = ESCAPE (der Tastaturweg schliesst es), Gothics 0x802 aus; (3) laeuft die Liste ohne Anmeldung und ohne offenes Thema
    // 300 ms weiter, wird sie wieder vorn angemeldet (SetEnableHandleEvent 0 und 1 - mit 1 allein bliebe sie, wo sie ist).
    // Allgemein G1 (Gothics Liste) + nur Othello (zUtilities).
    static void MouseListWatch(zCMenu* menu, zCInputCallback* top, zCMenuItem* runList, zCMenuItem* runText) {
        if (runList) {
            MouseListRun = runList;
            MouseListLostSince = 0;
            return;
        }
        zCMenuItem* l = MouseListRun;
        if (!l)
            return;
        bool inMenu = false;
        for (int i = 0; i < menu->m_listItems.GetNum() && !inMenu; i++)
            inMenu = menu->m_listItems[i] == l;
        if (!inMenu || l->m_bLeaveItem) {   // anderes Menue oder Schleife regulaer zu Ende
            MouseListRun = NULL;
            MouseListLostSince = 0;
            return;
        }
        if (runText) {   // Thema offen - die Liste wartet zu Recht
            MouseListLostSince = 0;
            return;
        }
        if (!MouseListLostSince) {
            MouseListLostSince = MouseMenuTick ? MouseMenuTick : 1;
            return;
        }
        if (MouseMenuTick - MouseListLostSince < 300)
            return;
        zCInputCallback* cb = static_cast<zCInputCallback*>(l);
        const int had = cb->GetEnableHandleEvent();
        cb->SetEnableHandleEvent(0);
        cb->SetEnableHandleEvent(1);
        MouseListLostSince = 0;
        MouseLog("Tagebuch: Themenliste %s lief ohne Eingabe weiter (oben %p, vtable %08X, angemeldet %d) - wieder vorn angemeldet",
            l->id.ToChar(), (void*)top, top ? *(DWORD*)top : 0, had);
    }

    static void MouseLogMenu(zCMenu* menu, int cw, int ch) {
        if (menu == MouseLoggedMenu)
            return;
        MouseLoggedMenu = menu;
        int onDesk = 0, selectable = 0;
        for (int i = 0; i < menu->m_listItems.GetNum(); i++) {
            zCMenuItem* it = menu->m_listItems[i];
            if (!it)
                continue;
            if (it->ondesk)
                onDesk++;
            if (it->GetIsSelectable())
                selectable++;
        }
        MouseLog("Men" COOP_UE " %s: %d Eintr" COOP_AE "ge, %d sichtbar, %d w" COOP_AE "hlbar; Bild %dx%d, Fenster %dx%d",
            menu->name.ToChar(), menu->m_listItems.GetNum(), onDesk, selectable, screen->psizex, screen->psizey, cw, ch);
    }

    // v60: Diagnose zu "beim Start nur Hintergrundbild, Hauptmenue erst nach Esc" (Test v59) - Kinder von screen und Zustand
    // des Menuefensters (zCView: flags +0x10, ondesk +0x18, backTex +0x3C, isOpen +0xAC, isClosed +0xB0) in den ersten Sekunden
    static void MouseDumpScreen(zCMenu* menu, const char* why) {
        if (!screen)
            return;
        int n = 0;
        std::string list;
        for (zCView* c = *(zCView**)((char*)screen + 0x34); c && n < 40; c = c->next, n++) {
            char b[160];
            const char* tex = c->backTex ? c->backTex->GetObjectName().ToChar() : "-";
            _snprintf(b, sizeof(b), "%s[%d %s fl%X vt%X %d,%d %dx%d]", n ? " " : "", n, tex, c->flags, *(DWORD*)c, c->pposx, c->pposy,
                c->psizex, c->psizey);
            b[sizeof(b) - 1] = 0;
            list += b;
        }
        zCView* w = menu ? (zCView*)menu->m_pWindow : NULL;
        // v62: Eingabe-Empfaenger von oben (Zeiger + vtable; das Menue selbst ist ein zCInputCallback bei +0)
        std::string inputs;
        int k = 0;
        for (void** node = *(void***)0x008DE1A8; node && k < 8; node = (void**)node[1], k++) {
            char b[48];
            _snprintf(b, sizeof(b), "%s%p/%08X", k ? " " : "", node[0], node[0] ? *(DWORD*)node[0] : 0);
            b[sizeof(b) - 1] = 0;
            inputs += b;
        }
        MouseLog("%s: Men" COOP_UE " %s (%p, Fenster %p vt%X auf screen %d, offen %d, zu %d, Alpha %d), Eingabe von oben: %s; %d Kinder von "
            "screen: %s", why, menu ? menu->name.ToChar() : "-", (void*)menu, (void*)w, w ? *(DWORD*)w : 0, w ? w->ondesk : -1,
            w ? *(int*)((char*)w + 0xAC) : -1, w ? *(int*)((char*)w + 0xB0) : -1, w ? w->alpha : -1, inputs.c_str(), n, list.c_str());
    }

    // v60: Zeigerlage (Gothic-Bildpunkte) und linker Knopf fuer die Vorschau (Drehen durch Ziehen)
    bool CoopMenuMouseState(int& x, int& y, bool& left) {
        x = MouseLastX;
        y = MouseLastY;
        left = MouseLeftWas;
        return MouseMenuOn && MouseLastX >= 0 && MouseView && MouseView->ondesk;
    }

    void CoopMenuMouseHide() {
        MouseMenuOn = false;
        MouseSelMenu = NULL;   // v86: naechstes Menue (auch an derselben Adresse) beginnt ohne eigene Wahl
        MouseListRun = NULL;   // v99: ohne Menue laeuft keine Themenliste
        MouseListLostSince = 0;
        if (MouseView && MouseView->ondesk && MouseView->owner) {
            MouseView->owner->RemoveItem(MouseView);
            MouseFixScreenLast();   // v63
        }
    }

    // Game_Loop: im Spiel ohne offenes Menue Zeiger weg (das Spielmenue zeichnet ueber denselben Weg - dort bleibt er)
    void CoopMenuMouseGameFrame() {
        if ((MouseMenuOn || (MouseView && MouseView->ondesk)) && !zCMenu::GetActive())
            CoopMenuMouseHide();
    }

    void CoopMenuMousePulse() {
        MouseInit();
        zCMenu* menu = zCMenu::GetActive();
        if (!MouseEnabled || !menu || !screen || screen->psizex <= 0 || screen->psizey <= 0) {
            CoopMenuMouseHide();
            return;
        }
        const bool wasOn = MouseMenuOn;
        MouseMenuTick = GetTickCount();
        MouseMenuOn = true;
        MouseFixScreenLast();   // v63: Ende-Zeiger je Menuebild richtig halten (auch fuer InsertLast anderer)

        // v60: Start-Diagnose (erstes Menuebild, dann nach 1, 3 und 6 s)
        if (!MouseDiagStart)
            MouseDiagStart = MouseMenuTick;
        const DWORD diagStart = MouseDiagStart;
        static int diagStep = 0;
        // v61: Menue-Lebenslauf in der ersten Minute - Wechsel des aktiven Menues und jeder m_exitState (+0xC24) ungleich 0
        // (1 = zurueck/Esc, 2 = ausfuehren), mit ForceSelAction-Merker (+0xCB4); zCMenu::Run prueft m_exitState nach HandleFrame
        if (MouseMenuTick - diagStart < 60000) {
            static zCMenu* lastMenu = NULL;
            static int lastExit = 0;
            const int ex = *(int*)((char*)menu + 0xC24);
            if (menu != lastMenu) {
                MouseLog("Lebenslauf: aktives Men" COOP_UE " %s (%p), vorher %p, m_exitState %d, inGame %d, ogame %p", menu->name.ToChar(),
                    (void*)menu, (void*)lastMenu, ex, *(int*)0x0086D3E8, (void*)ogame);
                lastMenu = menu;
                lastExit = ex;
            }
            else if (ex != lastExit) {
                MouseLog("Lebenslauf: Men" COOP_UE " %s m_exitState %d -> %d (erzwungen %d)", menu->name.ToChar(), lastExit, ex,
                    *(int*)((char*)menu + 0xCB4));
                lastExit = ex;
            }
        }
        static const DWORD diagAt[] = { 0, 1000, 3000, 6000 };
        if (diagStep < 4 && MouseMenuTick - diagStart >= diagAt[diagStep]) {
            char why[40];
            _snprintf(why, sizeof(why), "Start-Diagnose %d (%u ms)", diagStep + 1, (unsigned)(MouseMenuTick - diagStart));
            why[sizeof(why) - 1] = 0;
            MouseDumpScreen(menu, why);
            diagStep++;
        }

        // Gothics eigene Knoepfe: links aus (wir schicken die Taste selbst), rechts nur einmal je Klick.
        // v62: nur, wenn das Menue (oder sein laufendes Eingabefeld) oberster Eingabe-Empfaenger ist. Test v58-v61: "beim Start
        // nur Hintergrundbild, Hauptmenue erst nach Esc" - erst seit der Maus; das Hauptmenue war dabei aktiv (Lebenslauf v61),
        // ueber ihm lagen zwei Othello-Fenster, und Othello wertet Gothics Linksklick-Code 0x802 in eigenen Eingabe-Behandlern
        // aus (OTHELLO_ABI, 11 Stellen). Liegt ein anderer Empfaenger oben (zCInputCallback::inputList, erster Knoten
        // *0x008DE1A8 {Daten, naechster} - GetInput fragt von dort aus), bekommt er Gothics Klicks wie vor v58.
        // v64: Menue ohne Eintraege (Othellos Steuerung zeichnet und bedient sich selbst) - GT kann darin nichts anklicken und
        // laesst Gothics Klicks wie vor v58 durch
        zCInputCallback* top = MouseTopInput();
        zCMenuItem* activeItem = MouseActiveItem(menu);
        const bool menuOnTop = menu->m_listItems.GetNum() > 0 && (top == static_cast<zCInputCallback*>(menu) ||
            (activeItem && top == static_cast<zCInputCallback*>(activeItem)));
        zCMenuItem* runList = menuOnTop ? NULL : MouseRunningList(menu, top);   // v92: Themenliste (Tagebuch) laeuft
        zCMenuItem* runText = (menuOnTop || runList) ? NULL : MouseRunningItem(menu, top, MENU_ITEM_TEXT);   // v99: Thema offen
        MouseListWatch(menu, top, runList, runText);   // v99 (H27c)
        if (menuOnTop != MouseMenuOnTopWas || top != MouseTopWas) {
            if (MouseTopLogged < 80) {   // v92: 40 statt 20 (Test v91: Grenze vor dem Tagebuch erreicht); v99: 80
                MouseTopLogged++;
                MouseLog("Eingabe oben: %p (vtable %08X) - %s", (void*)top, top ? *(DWORD*)top : 0,
                    menuOnTop ? "Men" COOP_UE " selbst, GT steuert Klicks" : runList ? "Liste im Men" COOP_UE " (Tagebuch), Linksklick = RETURN"
                    : runText ? "Thema offen (Tagebuch), Linksklick = Esc"
                    : "NICHT das Men" COOP_UE " - Gothics Klicks gehen an diesen Empf" COOP_AE "nger");
            }
            MouseMenuOnTopWas = menuOnTop;
            MouseTopWas = top;
        }
        const bool left = *(volatile int*)0x0086CCB8 != 0;
        if (!left)
            MouseSwallowLeft = false;
        if (menuOnTop || runList || runText || MouseSwallowLeft) {   // v99: auch nach eigenem Klick, bis der Knopf los ist
            *(volatile int*)0x008DE1B0 = 1;
            *(volatile int*)0x0086CCC0 = 0;
        }
        const bool click = left && !MouseLeftWas && (menuOnTop || runList || runText);
        MouseLeftWas = left;
        if (click)
            MouseSwallowLeft = true;

        // v58: Fenster vorn auch, wenn das Vordergrundfenster ein Eltern-/Besitzerfenster von hWndApp ist (Renderer-Fenster);
        // ohne Fenster-Handle das von zCInput (GetKey vergleicht GetFocus mit 0x0086D30C)
        HWND w = hWndApp ? hWndApp : *(HWND*)0x0086D30C;
        HWND fg = GetForegroundWindow();
        const bool front = w && fg && (fg == w || GetAncestor(w, GA_ROOT) == fg || GetAncestor(w, GA_ROOTOWNER) == GetAncestor(fg, GA_ROOTOWNER));
        POINT pt;
        RECT rc;
        const char* why = !w ? "kein Spielfenster" : !front ? "Spielfenster nicht vorn" :
            (!GetCursorPos(&pt) || !ScreenToClient(w, &pt) || !GetClientRect(w, &rc)) ? "Zeigerlage nicht lesbar" : NULL;
        const int cw = why ? 0 : rc.right - rc.left, ch = why ? 0 : rc.bottom - rc.top;
        if (!why && (cw <= 0 || ch <= 0 || pt.x < 0 || pt.y < 0 || pt.x >= cw || pt.y >= ch))
            why = "Zeiger au" "\xDF" "erhalb des Fensters";
        if (why) {
            if (MouseView && MouseView->ondesk) {
                screen->RemoveItem(MouseView);
                MouseFixScreenLast();
            }
            if (MouseHiddenWhy != why) {
                MouseHiddenWhy = why;
                MouseLog("Zeiger aus: %s (Spiel %p, vorn %p)", why, (void*)w, (void*)fg);
            }
            return;
        }
        if (MouseHiddenWhy) {
            MouseLog("Zeiger wieder an (vorher: %s)", MouseHiddenWhy);
            MouseHiddenWhy = NULL;
        }
        const int sw = screen->psizex, sh = screen->psizey;
        const int x = pt.x * sw / cw, y = pt.y * sh / ch;
        MouseLogMenu(menu, cw, ch);

        zCView* v = MouseCursorView();
        if (v) {
            MouseFixScreenLast();   // v63: vor jedem InsertLast das echte Ende setzen
            if (!v->ondesk)
                screen->InsertItem(v, 1);   // stayOnTop
            else if (!MouseIsLastChild(v)) {   // v59: ein spaeter eingehaengtes (modales) Fenster liegt dahinter - neu ans Ende
                screen->RemoveItem(v);
                MouseFixScreenLast();
                screen->InsertItem(v, 1);
                if (MouseReorderLogged < 5) {
                    MouseReorderLogged++;
                    MouseLog("Zeiger wieder nach oben gelegt (Men" COOP_UE " %s)", menu->name.ToChar());
                }
            }
            int size = MouseCursorSize > 0 ? MouseCursorSize : sh / 30;
            if (size < 16)
                size = 16;
            if (size > 96)
                size = 96;
            v->SetSize(size * 8192 / sw, size * 8192 / sh);
            v->SetPos(x * 8192 / sw, y * 8192 / sh);
            if (!MouseShownLogged) {
                MouseShownLogged = true;
                MouseLog("Zeiger gezeigt in %s bei %d,%d (Bild %dx%d, Fenster %dx%d, %d Pixel, in screen %s, Textur %s)", menu->name.ToChar(),
                    x, y, sw, sh, cw, ch, size, v->ondesk && v->owner == screen ? "ja" : "NEIN", v->backTex ? "da" : "FEHLT");
            }
        }

        if (!wasOn) {   // Menue gerade aufgegangen: Vorauswahl nicht ueberfahren, solange die Maus ruht
            MouseLastX = x;
            MouseLastY = y;
        }
        const bool moved = x != MouseLastX || y != MouseLastY;
        MouseLastX = x;
        MouseLastY = y;

        // v92: laufende Themenliste - die Menue-Eintraege dahinter nicht markieren (sonst wechselte die Kategorie unter der Liste)
        if (runList) {
            if (click) {
                MousePushKey(0x1C);   // RETURN -> ShowContent des markierten Themas
                if (MouseClicksLogged < 80) {
                    MouseClicksLogged++;
                    MouseLog("Klick in %s auf die Liste %s -> RETURN (Thema anzeigen)", menu->name.ToChar(), runList->id.ToChar());
                }
            }
            return;
        }
        // v99 (H27c): Thema offen - Linksklick schliesst es wie Esc (Tastaturweg); Gothics 0x802 bleibt aus
        if (runText) {
            if (click) {
                MousePushKey(0x01);   // ESCAPE
                if (MouseClicksLogged < 80) {
                    MouseClicksLogged++;
                    MouseLog("Klick in %s auf das Thema %s -> Esc (Thema schlie" "\xDF" "en)", menu->name.ToChar(), runText->id.ToChar());
                }
            }
            return;
        }

        const bool editing = MouseEditing(menu);
        int valueIdx = -1;
        const int hit = editing ? -1 : MouseHit(menu, x, y, valueIdx);

        // v86: eigene Wahl erkennen. Neues aktives Menue = noch keine; m_mainSel aendert sich ohne GT (Rad 0x809/0x80A und
        // Pfeiltasten laufen ueber zCMenu::HandleEvent vor diesem Takt) = gewaehlt - in den ersten 200 ms nicht (Aufbau)
        bool wheel = false;
        if (menu != MouseSelMenu) {
            MouseSelMenu = menu;
            MouseSelSeen = menu->m_mainSel;
            MouseSelSince = MouseMenuTick;
            MouseUserSel = false;
        }
        else if (menu->m_mainSel != MouseSelSeen && CoopLoadListWheel(menu, MouseSelSeen, menu->m_mainSel)) {
            // v95: Liste "Spielstand waehlen" - das Rad blaettert den Inhalt, die Markierung bleibt (CoopMenuLobby.cpp)
            wheel = true;
            MouseSelSeen = menu->m_mainSel;
        }
        else if (menu->m_mainSel != MouseSelSeen) {
            if (MouseSelSeen >= 0 && MouseMenuTick - MouseSelSince > 200 && !MouseUserSel) {
                MouseUserSel = true;
                if (MouseUserSelLogged < 20) {
                    MouseUserSelLogged++;
                    MouseLog("Wahl per Rad/Taste in %s (Eintrag %d) - Linksklick gilt dem markierten Eintrag", menu->name.ToChar(),
                        menu->m_mainSel);
                }
            }
            MouseSelSeen = menu->m_mainSel;
        }
        // v95: Leiste der Liste (Griff ziehen, Schiene anklicken) - Klick und Ziehen gehoeren ihr, kein Ueberfahren dabei
        if (!editing && CoopLoadListBar(menu, x, y, left && (menuOnTop || runList), click)) {
            MouseSelSeen = menu->m_mainSel;
            return;
        }
        if (moved && hit >= 0 && !wheel) {
            if (hit != menu->m_mainSel)
                MouseSelect(menu, hit);
            if (menu->m_mainSel == hit)
                MouseUserSel = true;   // ueberfahren = gewaehlt
            MouseSelSeen = menu->m_mainSel;
        }

        if (!click)
            return;
        if (editing) {
            MousePushKey(0x1C);   // Eingabe fertig
            return;
        }
        // v86: nach eigener Wahl gilt der markierte Eintrag, auch wenn der Zeiger daneben oder auf einem anderen steht
        const int sel = menu->m_mainSel;
        const bool selOk = sel >= 0 && sel < menu->m_listItems.GetNum() && menu->m_listItems[sel] &&
            menu->m_listItems[sel]->GetIsSelectable();
        const bool useSel = MouseUserSel && selOk && hit != sel;
        int target = hit;
        if (useSel) {
            target = sel;
            valueIdx = -1;   // nicht das Wertfeld unter dem Zeiger - das gehoert zu einer anderen Zeile
        }
        else {
            if (hit < 0)
                return;   // Klick ins Leere ohne eigene Wahl
            MouseSelect(menu, hit);
            if (menu->m_mainSel != hit)
                return;
        }
        const char* where = useSel ? (hit < 0 ? " (markiert, Zeiger im Leeren)" : " (markiert, Zeiger auf anderem Eintrag)") : "";
        zCMenuItem* it = menu->m_listItems[target];
        // v62: Auswahlfeld/Regler direkt umschalten wie zCMenu::HandleEvent bei LEFT/RIGHT (vtable +0x30 ToggleValue(Schritt,
        // Umlauf), zCMenuItemChoice 0x004D6F30) - linke Haelfte des Werts zurueck, sonst weiter; Auswahl mit Umlauf, Regler ohne.
        // Test v61: "bei der Modellauswahl nicht mehr davor waehlen, nur das naechste" (bis v61 immer SPACE = weiter)
        zCMenuItem* toggle = MouseToggleTarget(menu, target, valueIdx);
        if (toggle) {
            const bool slider = toggle->m_parType == MENU_ITEM_SLIDER;
            const int step = (valueIdx >= 0 && x < toggle->pposx + toggle->psizex / 2) ? -1 : 1;
            ((int(__thiscall*)(zCMenuItem*, int, int))(*(DWORD**)toggle)[0x30 / 4])(toggle, step, slider ? 0 : 1);
            if (MouseClicksLogged < 80) {
                MouseClicksLogged++;
                MouseLog("Klick in %s auf %s%s%s -> %s", menu->name.ToChar(), it ? it->id.ToChar() : "?", valueIdx >= 0 ? " (Wert)" : "",
                    where, step < 0 ? "zur" COOP_UE "ck" : "weiter");
            }
            return;
        }
        MousePushKey(0x1C);   // RETURN - Gothics HandleEvent wirkt auf den markierten Eintrag (m_mainSel = target)
        if (MouseClicksLogged < 80) {
            MouseClicksLogged++;
            MouseLog("Klick in %s auf %s%s%s -> RETURN", menu->name.ToChar(), it ? it->id.ToChar() : "?", valueIdx >= 0 ? " (Wert)" : "",
                where);
        }
    }

#else
    void CoopMenuMousePulse() {}
    void CoopMenuMouseHide() {}
    void CoopMenuMouseGameFrame() {}
    bool CoopMenuMouseState(int& x, int& y, bool& left) { x = y = -1; left = false; return false; }
    unsigned CoopMenuMouseSelectSeq() { return 0; }
#endif
}
