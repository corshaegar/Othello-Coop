// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

    // --- v60: 3D-Vorschau der eigenen Figur im COOP-Menue (20.10.2026) ---------------------------------------------------
    //
    // Nutzer: "die Modelle fuer die Vorschau erstellen", Wahl "3D-Figur im Menue" (drehbar), dann "setze die Vorschau um".
    // Entwurf und Befund: gt-fix\VORSCHAU-3D.md. Kurz: Gothics Menue kann kein 3D, GD3D11s Inventar-Weg stuerzt im Hauptmenue ab
    // und zeigt nur ein statisches Netz - darum zeichnet der Coop-Renderer (coop6, Export GDX_CoopPreviewDraw) die Figur.
    //
    // Figur ohne Welt (allgemein G1, Werte aus CoopLook.cpp - Listen nur Othello):
    //   zCObject::CreateNewInstance("zCVob") (0x00589500), zCVob::SetVisual("HUMANS.MDS") (0x005D6FA0; GD3D11s SetVisual-Haken
    //   kehrt ohne Spiel/Welt sofort zurueck), dann wie oCNpc::InitModel (0x00695020) und oCNpc::SetHead (0x00694C90):
    //   zCModel::RemoveMeshLibAll (0x00564350), ApplyMeshLib(Statur) (0x00564590), SetMeshLibTexture(Statur, 0, Koerper, "BODY")
    //   und (Statur, 1, Hautton, "BODY") (0x00564150); Kopf: SearchNode(zMDL_NODE_NAME_HEAD 0x00873BF8) (0x00563F80),
    //   zCMorphMesh::Load(Kopf) (0x00586770), SetNodeVisual(Knoten, Kopf, 0) (0x0055FA00), eigener Bezug wieder ab (Release wie
    //   SetHead 0x00694E8E), SetTextureVar(0, Gesicht, "HEAD"), (1, Hautton, "HEAD"), (0, 0, "TEETH") (0x00587A70).
    //   Ruhehaltung: zCModel::StartAni("S_RUN", 0) (0x005611A0); je Menuebild zCModel::AdvanceAnis (0x00562CD0) mit kurz
    //   gesetzter Bildzeit (ztimer 0x008CF1E8: frameTimeFloat +0x04, frameTime +0x10 - AdvanceAnis bricht bei frameTime 0 ab;
    //   danach zurueck). Animations-Ereignisse (Toene, Partikel) laufen nicht: zCModel::DoAniEvents (0x00561B11) kehrt ohne
    //   homeVob->homeWorld (+0xA8) sofort zurueck.
    // Zeichnen: im COOP-Menue der Eintrag MENUITEM_COOP_PREVIEW (COOP_MENU.D v60) bekommt eine Kopie seiner vtable
    //   (zCMenuItemText 0x007D2E04, 34 Eintraege) mit eigenem DrawItems (+0x28 - zCView::DrawItems 0x006FCBD0 ruft ihn fuer
    //   jedes Kind): erst Gothics DrawItems, dann GDX_CoopPreviewDraw(Figur, Rechteck des Eintrags) - mitten im 2D-Teil, ueber
    //   dem Menuehintergrund, unter dem Mauszeiger (der bleibt letztes Kind von screen).
    // Bedienung: Figur dreht sich langsam; Ziehen mit gedrueckter linker Maustaste im Rahmen dreht sie von Hand (Selbstdrehen
    //   danach 3 s aus). Aenderungen an Kopf/Gesicht/Statur/Koerper/Hautton (CoopLookPulse) bauen sie sofort neu.
    // Gothic.ini [GOTHICCOOP]: coopPreview=0 schaltet die Vorschau ab; coopPreviewSpin (Grad/s, Vorgabe 20), coopPreviewYaw
    //   (Startdrehung, Vorgabe 0).

    void CoopPreviewPulse();
    bool CoopMenuMouseState(int& x, int& y, bool& left);

#if ENGINE == Engine_G1

    typedef int(__cdecl* CoopGdxPreviewDrawFn)(void* vob, float x, float y, float w, float h, float yawDeg, float camDist, float camHeight);

    static bool PrevInitDone = false;
    static bool PrevEnabled = true;
    static CoopGdxPreviewDrawFn PrevDraw = NULL;
    static zCVob* PrevVob = NULL;
    static zCModel* PrevModel = NULL;
    static std::string PrevKey;
    static float PrevYaw = 0.0f;
    static float PrevSpin = 20.0f;
    static float PrevCamDist = 380.0f, PrevCamHeight = 0.0f;
    static DWORD PrevTick = 0;
    static DWORD PrevManualUntil = 0;
    static int PrevDragX = -1;
    static bool PrevMenuOpen = false;
    static DWORD PrevVtable[35];        // [0] = RTTI wie vor der Engine-vtable, [1..34] = Eintraege
    static DWORD PrevOrigDrawItems = 0;
    static bool PrevVtableReady = false;
    static int PrevDrawFails = 0;
    static bool PrevDrawnLogged = false;

    static void PrevLog(const char* fmt, ...) {
        char buf[700];
        va_list ap;
        va_start(ap, fmt);
        _vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        buf[sizeof(buf) - 1] = 0;
        OthelloCompatLog("[%lld s, %s] Vorschau: %s", DiagSeconds(), DiagRole(), buf);
    }

    static void PrevInit() {
        if (PrevInitDone)
            return;
        PrevInitDone = true;
        PrevEnabled = IniInt("coopPreview", 1) != 0;
        PrevSpin = (float)IniInt("coopPreviewSpin", 20);
        PrevYaw = (float)IniInt("coopPreviewYaw", 0);
        if (!PrevEnabled) {
            PrevLog("aus (Gothic.ini [GOTHICCOOP] coopPreview=0)");
            return;
        }
        static const char* mods[] = { "g1_avx2.dll", "g1_avx.dll", "g1.dll", "g1a.dll" };
        const char* found = NULL;
        for (const char* m : mods) {
            HMODULE h = GetModuleHandleA(m);
            if (!h)
                continue;
            found = m;
            PrevDraw = (CoopGdxPreviewDrawFn)GetProcAddress(h, "GDX_CoopPreviewDraw");
            break;
        }
        PrevLog("Renderer %s - %s", found ? found : "kein GD3D11",
            PrevDraw ? "mit GDX_CoopPreviewDraw (coop6)" : "ohne GDX_CoopPreviewDraw - keine Vorschau (Coop-Renderer ab coop6 noetig)");
    }

    // Figur aufbauen bzw. an das gewaehlte Aussehen anpassen (wie oCNpc::InitModel + SetHead)
    static bool PrevBuild() {
        if (!PrevVob) {
            PrevVob = zDYNAMIC_CAST<zCVob>(zCObject::CreateNewInstance(zSTRING("zCVob")));
            if (!PrevVob) {
                PrevLog("zCVob nicht angelegt");
                PrevEnabled = false;
                return false;
            }
            PrevVob->SetVisual(zSTRING("HUMANS.MDS"));
            PrevModel = zDYNAMIC_CAST<zCModel>(PrevVob->visual);
            if (!PrevModel) {
                PrevLog("HUMANS.MDS nicht geladen");
                PrevEnabled = false;
                return false;
            }
            PrevModel->StartAni(zSTRING("S_RUN"), 0);
        }
        const std::string key = MyHeadMesh + "|" + std::to_string(MyHeadVarNr) + "|" + MyBodyMesh + "|" + std::to_string(MyBodyTextVarNr) +
            "|" + std::to_string(MySkin);
        if (key == PrevKey)
            return true;
        PrevKey = key;
        zSTRING body(MyBodyMesh.c_str());
        zSTRING bodyPart("BODY");
        PrevModel->RemoveMeshLibAll();
        PrevModel->ApplyMeshLib(body);
        PrevModel->SetMeshLibTexture(body, 0, MyBodyTextVarNr, &bodyPart);
        PrevModel->SetMeshLibTexture(body, 1, MySkin, &bodyPart);
        zCModelNodeInst* headNode = PrevModel->SearchNode(*(const zSTRING*)0x00873BF8);   // zMDL_NODE_NAME_HEAD
        bool head = false;
        if (headNode) {
            zCMorphMesh* mm = zCMorphMesh::Load(zSTRING(MyHeadMesh.c_str()));
            if (mm) {
                PrevModel->SetNodeVisual(headNode, mm, 0);
                zSTRING headPart("HEAD"), teethPart("TEETH");
                mm->SetTextureVar(0, MyHeadVarNr, &headPart);
                mm->SetTextureVar(1, MySkin, &headPart);
                mm->SetTextureVar(0, 0, &teethPart);
                mm->Release();   // SetNodeVisual haelt den Kopf selbst (wie oCNpc::SetHead)
                head = true;
            }
        }
        // Kamera nach der Groesse der Figur (Hoehe ins Bild, 30 Grad senkrecht im Renderer). v61 (Test v60: "der Kopf wurde
        // bei der Vorschau leicht abgeschnitten"): die Box des Modells (166 cm) enthaelt den Kopf nicht - er haengt als
        // Knoten-Visual am Kopfknoten; oben 30 cm dazu und 20 % Rand statt 12 %
        zTBBox3D bb = PrevModel->GetBBox3D();
        const float bottom = bb.mins.n[VY], top = bb.maxs.n[VY] + 30.0f;
        const float hgt = top - bottom;
        if (hgt > 50.0f && hgt < 1000.0f) {
            PrevCamHeight = (top + bottom) * 0.5f;
            PrevCamDist = hgt * 0.5f / 0.2679f * 1.20f;   // tan(15 Grad)
        }
        PrevLog("Figur: Kopf %s (%s), Gesicht %d, Statur %s, Koerper %d, Hautton %d; Hoehe %.0f cm, Kamera %.0f cm",
            MyHeadMesh.c_str(), head ? "geladen" : "FEHLT", MyHeadVarNr, MyBodyMesh.c_str(), MyBodyTextVarNr, MySkin + 1, hgt, PrevCamDist);
        return true;
    }

    // Animation weiterschalten: Gothics Bildzeit kurz auf die eigene setzen (im Menue laeuft ztimer nicht verlaesslich)
    // AdvanceAnis enthaelt fremde Eingriffe (Othello: CalcTransBlending-Vorbau OTHELLO_ABI+0xC6A00 bei 0x005637F5, Trampoline
    // bei 0x005632D4/0x005636C9/0x00563A65) - laufen fuer jede Figur im Spiel; ohne Welt nicht erprobt. Ein Zugriffsfehler
    // schaltet nur die Animation ab (Figur steht dann still), statt das Spiel zu beenden.
    static bool PrevAnimOff = false;

    static void PrevAnimate(DWORD dtMs) {
        if (!PrevModel || PrevAnimOff)
            return;
        char* timer = (char*)0x008CF1E8;
        float& frameTimeFloat = *(float*)(timer + 0x04);
        unsigned long& frameTime = *(unsigned long*)(timer + 0x10);
        const float oldF = frameTimeFloat;
        const unsigned long oldT = frameTime;
        frameTime = dtMs;
        frameTimeFloat = (float)dtMs;
        try {
            PrevModel->AdvanceAnis();
        }
        catch (...) {
            PrevAnimOff = true;
            PrevLog("Animation ohne Welt fehlgeschlagen (Zugriffsfehler in zCModel::AdvanceAnis) - Figur steht still");
        }
        frameTime = oldT;
        frameTimeFloat = oldF;
    }

    static void __fastcall PrevDrawItems(zCView* self, void*) {
        ((void(__thiscall*)(zCView*))PrevOrigDrawItems)(self);
        if (!PrevDraw || !PrevVob || !PrevMenuOpen)
            return;
        try {
            const int r = PrevDraw(PrevVob, (float)self->pposx, (float)self->pposy, (float)self->psizex, (float)self->psizey, PrevYaw,
                PrevCamDist, PrevCamHeight);
            if (!r && PrevDrawFails < 3) {
                PrevDrawFails++;
                PrevLog("Renderer zeichnete nichts (Modell/Netz fehlt?) - %d. Mal", PrevDrawFails);
            }
            else if (r && !PrevDrawnLogged) {
                PrevDrawnLogged = true;
                PrevLog("gezeichnet bei %d,%d (%dx%d Bildpunkte)", self->pposx, self->pposy, self->psizex, self->psizey);
            }
        }
        catch (...) {
            PrevDraw = NULL;
            CoopErrorLog("Vorschau", "Zugriffsfehler (SEH) oder unbekannte Ausnahme im Renderer - Vorschau aus", PluginState.c_str());
        }
    }

    // Eintrag MENUITEM_COOP_PREVIEW mit der eigenen vtable versehen (einmal je Eintrag)
    static void PrevHookItem(zCMenuItem* it) {
        DWORD* vt = *(DWORD**)it;
        if (vt == &PrevVtable[1])
            return;
        if (vt != (DWORD*)0x007D2E04) {
            static bool logged = false;
            if (!logged) {
                logged = true;
                PrevLog("Eintrag MENUITEM_COOP_PREVIEW hat unerwartete vtable %p - keine Vorschau", (void*)vt);
            }
            return;
        }
        LobbySetItemText(it, " ");   // Hinweistext aus dem Skript weg (geht nur mit der Engine-vtable)
        if (!PrevVtableReady) {
            memcpy(PrevVtable, vt - 1, sizeof(PrevVtable));
            PrevOrigDrawItems = PrevVtable[1 + 0x28 / 4];
            PrevVtable[1 + 0x28 / 4] = (DWORD)&PrevDrawItems;
            PrevVtableReady = true;
        }
        *(DWORD**)it = &PrevVtable[1];
        PrevLog("Vorschau-Rahmen eingehakt (DrawItems %08X)", PrevOrigDrawItems);
    }

    void CoopPreviewPulse() {
        zCMenu* menu = zCMenu::GetActive();
        // v94: Figur und Vorschau stehen jetzt in "Figur und Name" (MENU_COOP_LOOK, COOP_MENU-v94.D); MENU_COOP fuer aeltere Skripte
        const std::string mname = menu ? LobbyUpper(menu->name) : std::string();
        PrevMenuOpen = mname == "MENU_COOP_LOOK" || mname == "MENU_COOP";
        if (!PrevMenuOpen)
            return;
        PrevInit();
        if (!PrevEnabled || !PrevDraw)
            return;
        zCMenuItem* it = LobbyFindItem(menu, "MENUITEM_COOP_PREVIEW");
        if (!it)
            return;   // altes Menue-Skript ohne Rahmen
        try {
            if (!PrevBuild())
                return;
        }
        catch (...) {
            PrevEnabled = false;
            PrevLog("Figur ohne Welt nicht aufzubauen (Zugriffsfehler) - Vorschau aus");
            return;
        }
        PrevHookItem(it);

        const DWORD now = GetTickCount();
        DWORD dt = PrevTick ? now - PrevTick : 16;
        PrevTick = now;
        if (dt > 100)
            dt = 100;
        if (dt < 1)
            dt = 1;
        PrevAnimate(dt);

        // Drehen: Ziehen im Rahmen, sonst langsam von selbst
        int mx = -1, my = -1;
        bool left = false;
        const bool mouse = CoopMenuMouseState(mx, my, left);
        const bool inside = mouse && mx >= it->pposx && my >= it->pposy && mx < it->pposx + it->psizex && my < it->pposy + it->psizey;
        if (left && (inside || PrevDragX >= 0)) {
            if (PrevDragX >= 0)
                PrevYaw += (float)(mx - PrevDragX) * 0.5f;
            PrevDragX = mx;
            PrevManualUntil = now + 3000;
        }
        else {
            PrevDragX = -1;
            if (now > PrevManualUntil)
                PrevYaw += PrevSpin * (float)dt / 1000.0f;
        }
        while (PrevYaw >= 360.0f)
            PrevYaw -= 360.0f;
        while (PrevYaw < 0.0f)
            PrevYaw += 360.0f;
    }

#else
    void CoopPreviewPulse() {}
#endif
}
