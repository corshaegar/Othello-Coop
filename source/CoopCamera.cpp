// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

    // --- Zuschau-Fenster beim Mitspieler (v44-v48) ---------------------------------------------------------------------------
    //
    // Nutzer (19.10.2026): "Eine Kamera, die beim Mitspieler angebracht ist und diesen verfolgt" - "mit Hilfe dieser Kamera
    // oben rechts ein Fenster, in dem man dem Mitspieler zuschauen kann" - "die Groesse des Fensters direkt in den
    // Mehrspieler-Optionen einstellen".
    //
    // v48 (Nutzer: "nimm dieses Vollbild einfach raus. Man soll mit Shift+F5 einfach die Kamera abschalten koennen, eigentlich
    // war gedacht, dass die automatisch angeht, sobald der ausserhalb des Radius ist - beide Optionen bei Mehrspieler waehlbar"):
    //  - Die Vollbild-Kamera (v44: Kamera-KI des Helden auf die Figur, zCAICamera::SetTarget) ist entfernt. Sie stuerzte beim
    //    Zurueckschalten jedes Mal ab - Ursache K4: string::Combine bekam fuer %s einen rohen const char* statt eines
    //    Union-string (20 Bytes) und kopierte Muell (COOP-FIXES K4). Nichts setzt das Kamera-Ziel oder die Bewegungssperre
    //    des Helden mehr; CoopCamWatching() ist immer false.
    //  - Shift+F5 (coopCamKey): Fenster an -> naechster Mitspieler -> aus. Ohne Coop-Renderer nur ein Hinweis.
    //  - Automatik (Mehrspieler-Menue "Kamera: automatisch | nur Shift+F5", Gothic.ini [GOTHICCOOP] camWindowMode 0/1, Vorgabe
    //    0): geht der naechste Mitspieler ueber den Sync-Radius hinaus (BROADCAST_DISTANCE 45 m, GothicCoopConfig.json
    //    "coopCamWindowAutoDistance"), oeffnet sich das Fenster; kommt er wieder naeher (85 % des Radius), schliesst es sich -
    //    nur wenn die Automatik es geoeffnet hat. Gewechselt wird erst nach 1 s im neuen Zustand. Shift+F5 geht immer vor:
    //    von Hand geschlossen bleibt es zu, bis der Mitspieler einmal wieder in der Naehe war.
    //
    // Das Fenster: GT rechnet jedes Bild eine Verfolgerkamera hinter der Figur (3 m hinter, 1,1 m ueber, Blick auf die Figur,
    // geglaettet, Strahl zCWorld::TraceRayNearestHit 0x005F46E0 gegen Waende) und gibt sie mit Fensterlage an den Coop-GD3D11
    // (Export GDX_CoopSecondView, gt-fix\gd3d11\coop-zweitbild.patch, GD3D11-UMBAU.md). Der eigene Held bleibt spielbar.
    // Groesse: "Zuschau-Fenster klein|mittel|gross" (COOP_MENU.D, Gothic.ini camWindowSize 0/1/2 - zCOption::ReadInt
    // 0x0045CDB0, GD3D11 reicht fremde Eintraege durch). Bildrate: "coopCamWindowEveryNth" (Standard 2 = jedes zweite Bild).
    // Weit weg ist die Figur nur in der Welt, weil der Verfolgungspunkt (I1, coopFollowPartner) sie einsetzt.
    //
    // v66 (Nutzer: "Coop-Kamera: Option Kamera aus, feinere Groesse, Lage des Fensters, eigene Taste, Automatik-Abstand"):
    // Einstellungen -> COOP -> Coop-Kamera, alles in Gothic.ini [GOTHICCOOP] (Gothic schreibt die Auswahl selbst):
    //  - camWindowMode 0 automatisch | 1 nur Taste | 2 aus (aus: Taste tut nichts und bleibt dem Spiel, offenes Fenster zu)
    //  - camWindowWidth 0-7 = 15-50 % der Bildbreite (ersetzt camWindowSize 0/1/2 = 20/27/36 %; einmalig uebernommen)
    //  - camWindowPos 0 oben rechts | 1 oben links | 2 unten rechts | 3 unten links (unten 7,5 % Rand ueber Gothics Leisten)
    //  - camWindowKey: Eintrag aus CamKeys (Shift+F5, F5, F11, Einfg/Pos1/Ende mit und ohne Shift; ersetzt "coopCamKey" aus der json)
    //  - camWindowDist: Automatik ab 20/30/45/60/80/100 m (ersetzt "coopCamWindowAutoDistance")
    // Der Renderer setzt die Fensterhoehe = Breite x Bildhoehe/Bildbreite in Pixeln - im Bildanteil also so hoch wie breit;
    // ragt es hinaus, schiebt er es zurueck (RenderCoopSecondView).

#if ENGINE == Engine_G1

    // Tasten, die weder GT noch Othello/Gothic schon belegen: GT nimmt F6 (Abgleich), F7 (Testmarke), F8/F9 (Sicherungen),
    // Shift+F1-F4 und Shift+F11 (Testlauf), Othello F10/F12 (Schnellspeichern/-laden); Gothic.ini [KEYS] belegt Entf und
    // Bild ab (Seitwaerts) - Einfg, Pos1 und Ende sind dort frei.
    struct CamKeyDef { const char* name; int key; bool shift; };
    static const CamKeyDef CamKeys[] = {
        { "Shift+F5", KEY_F5, true }, { "F5", KEY_F5, false }, { "F11", KEY_F11, false },
        { "Shift+Einfg", KEY_INSERT, true }, { "Einfg", KEY_INSERT, false }, { "Shift+Pos1", KEY_HOME, true },
        { "Pos1", KEY_HOME, false }, { "Shift+Ende", KEY_END, true }, { "Ende", KEY_END, false } };
    static const int CamKeyCount = sizeof(CamKeys) / sizeof(CamKeys[0]);
    static const int CamDistM[] = { 20, 30, 45, 60, 80, 100 };
    static const int CamDistCount = sizeof(CamDistM) / sizeof(CamDistM[0]);
    static const char* CamPosName[] = { "oben rechts", "oben links", "unten rechts", "unten links" };

    static int CamKey = 0;
    static int CamKeyIdx = -1;            // v66: Gothic.ini [GOTHICCOOP] camWindowKey

    static void CamLog(const char* fmt, ...) {
        char buf[600];
        va_list ap;
        va_start(ap, fmt);
        _vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        buf[sizeof(buf) - 1] = 0;
        OthelloCompatLog("[%lld s, %s] Kamera: %s", DiagSeconds(), DiagRole(), buf);
    }

    // v48: keine Vollbild-Kamera mehr - der Spawn-Manager-Haken (CoopFarSync.cpp) braucht keinen zweiten Mittelpunkt
    bool CoopCamWatching() {
        return false;
    }

    // v47: Shift+F5 genau einmal je Druck. zinput->KeyToggled meldet eine gehaltene Taste nach der Wiederholzeit erneut:
    // zCInput_Win32::GetKey 0x004C88A0 setzt bei Tastenwiederholung keytoggle[] (0x0086CED0) wieder (+0x148/+0x152), KeyToggled
    // 0x004C87B0 liest und loescht nur. Jetzt Flanke von KeyPressed (Zustand 0x0086CCC8, nur aus DirectInput-Ereignissen) und
    // 400 ms Sperre. Jedes Bild aufrufen, damit der Zustand stimmt.
    static bool CamKeyWas = false;
    static long long CamKeyLastMs = 0;

    static const char* CamKeyName() {
        return CamKeys[CamKeyIdx >= 0 && CamKeyIdx < CamKeyCount ? CamKeyIdx : 0].name;
    }

    static bool CamFigUsable(oCNpc* f) {
        return f && f != player && f->GetHomeWorld() && f->vobLeafList.GetNum() > 0 && f->GetModel();
    }

    // Figuren der Mitspieler in fester Reihenfolge (nach GT-Name)
    static std::vector<std::pair<std::string, oCNpc*>> CamFigures() {
        std::vector<std::pair<std::string, oCNpc*>> v;
        for (auto& kv : PlayerNpcs)
            if (CamFigUsable(kv.first))
                v.push_back({ std::string(kv.second.ToChar()), kv.first });
        std::sort(v.begin(), v.end(), [](const std::pair<std::string, oCNpc*>& a, const std::pair<std::string, oCNpc*>& b) {
            return a.first < b.first;
        });
        return v;
    }

    // --- Zuschau-Fenster ----------------------------------------------------------------------------------------------------

    typedef void(__cdecl* CoopGdxSecondViewFn)(int enabled, const float* camToWorld, float x, float y, float w, int everyNth);
    static CoopGdxSecondViewFn WinGdx = NULL;
    static bool WinGdxLooked = false;
    static oCNpc* WinFig = NULL;          // Figur im Fenster (NULL = Fenster aus)
    static std::string WinFigName;
    static bool WinSentOn = false;        // Renderer hat das Fenster gerade an
    static zVEC3 WinCamPos;
    static bool WinCamPosValid = false;
    static long long WinLastMs = 0;
    static int WinSize = -1;              // v66: Gothic.ini [GOTHICCOOP] camWindowWidth 0-7 (15-50 %), je Sekunde neu gelesen
    static int WinMode = -1;              // Gothic.ini [GOTHICCOOP] camWindowMode: 0 automatisch, 1 nur Taste, v66: 2 aus
    static int WinPos = 0;                // v66: camWindowPos 0-3
    static int WinDistIdx = -1;           // v66: camWindowDist
    static long long WinOptNextMs = 0;
    static int WinEveryNth = 0;
    static bool WinAutoOutside = false;   // v48: Automatik - Mitspieler gilt als ausserhalb des Radius
    static bool WinAutoOpened = false;    // v48: das Fenster hat die Automatik geoeffnet (nur dann schliesst sie es auch)
    static long long WinAutoSinceMs = 0;  // v48: seit wann der neue Zustand anliegt (Wechsel erst nach 1 s)
    static long long WinAutoNextMs = 0;
    static float WinAutoDist = 0.f;

    static CoopGdxSecondViewFn WinRenderer() {
        if (WinGdxLooked)
            return WinGdx;
        WinGdxLooked = true;
        static const char* mods[] = { "g1_avx2.dll", "g1_avx.dll", "g1.dll", "g1a.dll" };
        bool found = false;
        for (const char* m : mods) {
            HMODULE h = GetModuleHandleA(m);
            if (!h)
                continue;
            found = true;
            WinGdx = (CoopGdxSecondViewFn)GetProcAddress(h, "GDX_CoopSecondView");
            // v47: Fassung dazu (GDX_GetVersionString, z. B. "v17.9.7-c83a424-coop5") - zeigt im Log, welcher Bau laeuft
            typedef const char*(__cdecl* GdxVersionFn)();
            GdxVersionFn ver = (GdxVersionFn)GetProcAddress(h, "GDX_GetVersionString");
            const char* vs = ver ? ver() : NULL;
            CamLog("Renderer %s %s %s", m, vs ? vs : "(Fassung unbekannt)", WinGdx ? "mit GDX_CoopSecondView - Zuschau-Fenster verfuegbar"
                : "ohne GDX_CoopSecondView (Original-GD3D11) - kein Zuschau-Fenster");
            break;
        }
        if (!found)
            CamLog("kein GD3D11-Renderer geladen - kein Zuschau-Fenster");
        return WinGdx;
    }

    // v66: Vorgaben nach Gothic.ini schreiben, damit das Menue sie zeigt (auch schon im Hauptmenue, aus CoopSettingsDefaults).
    // camWindowWidth einmalig aus dem alten camWindowSize: klein 20 % -> 1, mittel 27 % -> 2 (25 %), gross 36 % -> 4 (35 %).
    void CoopCamDefaults() {
        if (!zoptions)
            return;
        const zSTRING sec("GOTHICCOOP");
        if (!zoptions->EntryExists(sec, "camWindowWidth")) {
            int w = 2;
            if (zoptions->EntryExists(sec, "camWindowSize")) {
                const int old = zoptions->ReadInt(sec, "camWindowSize", 1);
                w = old <= 0 ? 1 : old == 1 ? 2 : 4;
            }
            zoptions->WriteInt(sec, "camWindowWidth", w, 0);
        }
        if (!zoptions->EntryExists(sec, "camWindowMode"))
            zoptions->WriteInt(sec, "camWindowMode", 0, 0);
        if (!zoptions->EntryExists(sec, "camWindowPos"))
            zoptions->WriteInt(sec, "camWindowPos", 0, 0);
        if (!zoptions->EntryExists(sec, "camWindowKey"))
            zoptions->WriteInt(sec, "camWindowKey", 0, 0);   // Shift+F5 wie bisher
        if (!zoptions->EntryExists(sec, "camWindowDist"))
            zoptions->WriteInt(sec, "camWindowDist", 2, 0);  // 45 m = Sync-Radius wie bisher
    }

    static void WinOff(const char* why);

    // Einstellungen aus Einstellungen -> COOP -> Coop-Kamera (Gothic schreibt die Wahl nach Gothic.ini), je Sekunde neu gelesen
    static void WinReadOptions() {
        const long long now = GetCurrentMs();
        if (WinSize >= 0 && WinMode >= 0 && now < WinOptNextMs)
            return;
        WinOptNextMs = now + 1000;
        int s = 2, mode = 0, pos = 0, key = 0, dist = 2;
        if (zoptions) {
            CoopCamDefaults();
            const zSTRING sec("GOTHICCOOP");
            s = zoptions->ReadInt(sec, "camWindowWidth", 2);
            mode = zoptions->ReadInt(sec, "camWindowMode", 0);
            pos = zoptions->ReadInt(sec, "camWindowPos", 0);
            key = zoptions->ReadInt(sec, "camWindowKey", 0);
            dist = zoptions->ReadInt(sec, "camWindowDist", 2);
        }
        if (s < 0 || s > 7) s = 2;
        if (mode < 0 || mode > 2) mode = 0;
        if (pos < 0 || pos > 3) pos = 0;
        if (key < 0 || key >= CamKeyCount) key = 0;
        if (dist < 0 || dist >= CamDistCount) dist = 2;
        const bool first = WinMode < 0;
        if (first || s != WinSize || pos != WinPos)
            CamLog("Zuschau-Fenster: %d %% der Bildbreite, %s", 15 + 5 * s, CamPosName[pos]);
        if (first || key != CamKeyIdx) {
            CamLog("Zuschau-Fenster: Taste %s", CamKeys[key].name);
            CamKeyWas = false;
        }
        if (first || mode != WinMode || dist != WinDistIdx)
            CamLog("Zuschau-Fenster: %s", mode == 0 ? (std::string("automatisch ab ") + std::to_string(CamDistM[dist]) + " m (und Taste)").c_str()
                : mode == 1 ? "nur mit der Taste" : "aus (Taste bleibt dem Spiel)");
        if (mode == 2 && WinMode != 2 && WinFig)
            WinOff("Kamera in den Einstellungen ausgeschaltet");
        WinSize = s;
        WinMode = mode;
        WinPos = pos;
        CamKeyIdx = key;
        CamKey = CamKeys[key].key;
        WinDistIdx = dist;
        WinAutoDist = CamDistM[dist] * 100.f;
    }

    int CoopCamKey() {
        WinReadOptions();
        return CamKey;
    }

    // Taste aus camWindowKey: mit Shift nur bei gedrueckter Shift-Taste, ohne Shift nur ohne. Bei "Kamera aus" bleibt sie dem
    // Spiel (nichts verbrauchen).
    bool CoopCamKeyHit(bool shift) {
        WinReadOptions();
        if (!zinput || WinMode == 2) {
            CamKeyWas = false;
            return false;
        }
        const CamKeyDef& k = CamKeys[CamKeyIdx];
        const bool mods = k.shift ? shift : !shift;
        if (mods)
            zinput->KeyToggled(k.key);   // Merker verbrauchen (die Taste gehoert in dieser Lage GT)
        const bool down = zinput->KeyPressed(k.key) != 0;
        const bool edge = down && !CamKeyWas;
        CamKeyWas = down;
        if (!edge || !mods)
            return false;
        const long long now = GetCurrentMs();
        if (now - CamKeyLastMs < 400)
            return false;
        CamKeyLastMs = now;
        return true;
    }

    // Fensterbreite in Bildanteilen (v66: 15-50 %)
    static float WinWidth() {
        WinReadOptions();
        return 0.15f + 0.05f * WinSize;
    }

    // Verfolgerkamera hinter der Figur -> Matrix wie zCVob::trafoObjToWorld (Zeile fuer Zeile; Spalten rechts, oben, Blick,
    // Lage - Gothic ist linkshaendig: rechts = oben x Blick)
    static void WinCamera(oCNpc* fig, float m[16]) {
        const zVEC3 p = fig->GetPositionWorld();
        const zVEC3 at = fig->GetAtVectorWorld();
        float ax = at.n[0], az = at.n[2];
        const float al = sqrtf(ax * ax + az * az);
        if (al < 0.01f) { ax = 0.f; az = 1.f; }
        else { ax /= al; az /= al; }
        const zVEC3 look(p.n[0], p.n[1] + 40.f, p.n[2]);
        zVEC3 want(p.n[0] - ax * 300.f, p.n[1] + 110.f, p.n[2] - az * 300.f);
        // Wand zwischen Figur und Kamera: davor bleiben
        if (zCWorld* w = ogame->GetGameWorld()) {
            const zVEC3 ray = want - look;
            if (w->TraceRayNearestHit(look, ray, (zCVob*)NULL, zTRACERAY_STAT_POLY | zTRACERAY_VOB_IGNORE | zTRACERAY_POLY_IGNORE_TRANSP)
                && w->traceRayReport.foundHit) {
                const zVEC3 h = w->traceRayReport.foundIntersection;
                const float rl = sqrtf(ray.n[0] * ray.n[0] + ray.n[1] * ray.n[1] + ray.n[2] * ray.n[2]);
                const float hl = sqrtf((h.n[0] - look.n[0]) * (h.n[0] - look.n[0]) + (h.n[1] - look.n[1]) * (h.n[1] - look.n[1])
                    + (h.n[2] - look.n[2]) * (h.n[2] - look.n[2]));
                if (rl > 1.f) {
                    const float t = (hl > 40.f ? hl - 30.f : 10.f) / rl;
                    want = zVEC3(look.n[0] + ray.n[0] * t, look.n[1] + ray.n[1] * t, look.n[2] + ray.n[2] * t);
                }
            }
        }
        // glaetten (Figur bewegt sich in Netz-Schritten)
        const long long now = GetCurrentMs();
        float dt = WinLastMs ? (now - WinLastMs) / 1000.f : 1.f;
        WinLastMs = now;
        if (dt < 0.f) dt = 0.f;
        if (dt > 0.5f) dt = 0.5f;
        const float jx = want.n[0] - WinCamPos.n[0], jy = want.n[1] - WinCamPos.n[1], jz = want.n[2] - WinCamPos.n[2];
        if (!WinCamPosValid || jx * jx + jy * jy + jz * jz > 2000.f * 2000.f)
            WinCamPos = want;
        else {
            const float k = 1.f - expf(-dt * 6.f);
            WinCamPos = zVEC3(WinCamPos.n[0] + jx * k, WinCamPos.n[1] + jy * k, WinCamPos.n[2] + jz * k);
        }
        WinCamPosValid = true;
        float dx = look.n[0] - WinCamPos.n[0], dy = look.n[1] - WinCamPos.n[1], dz = look.n[2] - WinCamPos.n[2];
        float dl = sqrtf(dx * dx + dy * dy + dz * dz);
        if (dl < 1.f) { dx = ax; dy = 0.f; dz = az; dl = 1.f; }
        dx /= dl; dy /= dl; dz /= dl;
        // rechts = (0,1,0) x d
        float rx = dz, ry = 0.f, rz = -dx;
        const float rl = sqrtf(rx * rx + rz * rz);
        if (rl < 0.001f) { rx = 1.f; rz = 0.f; }
        else { rx /= rl; rz /= rl; }
        // oben = d x rechts
        const float ux = dy * rz - dz * ry, uy = dz * rx - dx * rz, uz = dx * ry - dy * rx;
        m[0] = rx;  m[1] = ux;  m[2] = dx;   m[3] = WinCamPos.n[0];
        m[4] = ry;  m[5] = uy;  m[6] = dy;   m[7] = WinCamPos.n[1];
        m[8] = rz;  m[9] = uz;  m[10] = dz;  m[11] = WinCamPos.n[2];
        m[12] = 0.f; m[13] = 0.f; m[14] = 0.f; m[15] = 1.f;
    }

    static void WinOff(const char* why) {
        if (WinSentOn && WinGdx)
            WinGdx(0, NULL, 0.f, 0.f, 0.f, 1);
        WinSentOn = false;
        if (WinFig) {
            CamLog("Zuschau-Fenster aus (%s)", why);
            WinFig = NULL;
        }
        WinAutoOpened = false;
        WinCamPosValid = false;
    }

    // why: "Shift+F5" oder Text der Automatik (nur fuer Chat und Log)
    static bool WinStart(oCNpc* fig, const std::string& name, const char* why) {
        if (!WinRenderer())
            return false;
        WinFig = fig;
        WinFigName = name;
        WinCamPosValid = false;
        if (!WinEveryNth) {
            WinEveryNth = 2;
            if (CoopConfig.contains("coopCamWindowEveryNth") && CoopConfig["coopCamWindowEveryNth"].is_number_integer())
                WinEveryNth = CoopConfig["coopCamWindowEveryNth"].get<int>();
            if (WinEveryNth < 1) WinEveryNth = 1;
        }
        // K4: fuer %s nur Union-string-Objekte uebergeben, nie const char*
        ChatLog(string::Combine(CoopTr("Zuschau-Fenster: %s (%s) - %s: n" "\xE4" "chster Mitspieler bzw. aus.", "Watch window: %s (%s) - %s: next player or off."),
            string(name.c_str()), string(why), string(CamKeyName())));
        CamLog("Zuschau-Fenster bei %s (%s, jedes %d. Bild neu)", name.c_str(), why, WinEveryNth);
        return true;
    }

    bool CoopCamWindowOn() {
        return WinFig != NULL;
    }

    bool CoopCamWindowSent() {   // der Renderer hat das Fenster (ab dem Bild nach dem Einschalten)
        return WinFig != NULL && WinSentOn;
    }

    bool CoopCamWindowAvailable() {
        return WinRenderer() != NULL;
    }

    static void WinAutoReset() {
        WinAutoOutside = false;
        WinAutoSinceMs = 0;
    }

    void CoopCamStop(const char* why) {
        WinOff(why);
        WinAutoReset();   // v48: danach entscheidet die Automatik neu (z. B. nach dem Laden)
    }

    // v48: Speichern beruehrt das Fenster nicht mehr (die Vollbild-Kamera musste vorher die Bewegungssperre loesen)
    void CoopCamSuspendForSave() {}

    // aus CoopDeleteNpc / oCWorld::RemoveVob: die Figur verschwindet - Fenster aus
    void CoopCamNpcGone(oCNpc* npc) {
        if (npc && npc == WinFig)
            WinOff("Figur des Mitspielers entfernt");
    }

    // Shift+F5: aus -> Fenster beim ersten Mitspieler -> (naechster Mitspieler) -> aus
    void CoopCamToggle() {
        if (!player || !ogame || IsLoadingLevel)
            return;
        if (!WinRenderer()) {
            ChatLog(CoopTr("Zuschau-Fenster: braucht den Coop-Renderer (GD3D11 coop) - hier l" "\xE4" "uft ein anderer.", "Watch window: needs the coop renderer (GD3D11 coop) - another one is running."), zCOLOR(255, 200, 80, 255));
            return;
        }
        auto figs = CamFigures();
        oCNpc* next = NULL;
        std::string nextName;
        bool afterCurrent = WinFig == NULL;
        for (auto& f : figs) {
            if (afterCurrent) {
                next = f.second;
                nextName = f.first;
                break;
            }
            if (f.second == WinFig)
                afterCurrent = true;
        }
        if (!next) {
            if (WinFig)
                WinOff(CamKeyName());
            else
                ChatLog(CoopTr("Zuschau-Fenster: kein Mitspieler in der Welt.", "Watch window: no player in the world."), zCOLOR(255, 200, 80, 255));
            return;
        }
        WinStart(next, nextName, CamKeyName());
        WinAutoOpened = false;   // von Hand - die Automatik schliesst es nicht
    }

    // v48: Automatik - Fenster auf, sobald der naechste Mitspieler ausserhalb des Radius ist, zu, wenn er wieder nah ist
    static void WinAutoPulse() {
        if (WinMode != 0) {
            WinAutoReset();
            return;
        }
        if (IsLoadingLevel || IsSavingGame || !player || !ogame || !ogame->GetGameWorld() || !WinRenderer())
            return;
        const long long now = GetCurrentMs();
        if (now < WinAutoNextMs)
            return;
        WinAutoNextMs = now + 250;
        if (WinAutoDist < 500.f)
            WinAutoDist = (float)BROADCAST_DISTANCE;   // v66: aus camWindowDist (WinReadOptions)
        auto figs = CamFigures();
        if (figs.empty()) {
            WinAutoReset();
            return;
        }
        oCNpc* nearest = NULL;
        std::string nearestName;
        float best = 0.f;
        const zVEC3 me = player->GetPositionWorld();
        for (auto& f : figs) {
            const zVEC3 p = f.second->GetPositionWorld();
            const float dx = p.n[0] - me.n[0], dy = p.n[1] - me.n[1], dz = p.n[2] - me.n[2];
            const float d = sqrtf(dx * dx + dy * dy + dz * dz);
            if (!nearest || d < best) {
                nearest = f.second;
                nearestName = f.first;
                best = d;
            }
        }
        const bool outside = WinAutoOutside ? best > WinAutoDist * 0.85f : best > WinAutoDist;
        if (outside == WinAutoOutside) {
            WinAutoSinceMs = 0;
            return;
        }
        if (!WinAutoSinceMs) {
            WinAutoSinceMs = now;
            return;
        }
        if (now - WinAutoSinceMs < 1000)
            return;
        WinAutoSinceMs = 0;
        WinAutoOutside = outside;
        char why[80];
        _snprintf(why, sizeof(why), "automatisch, %.0f m entfernt", best / 100.f);
        why[sizeof(why) - 1] = 0;
        if (outside && !WinFig) {
            if (WinStart(nearest, nearestName, why))
                WinAutoOpened = true;
        }
        else if (!outside && WinFig && WinAutoOpened) {
            WinOff(why);
        }
    }

    // Zuschau-Fenster je Bild - Kamera rechnen und an den Renderer geben
    static void WinPulse() {
        if (!WinFig)
            return;
        if (IsLoadingLevel || !player || !ogame || !ogame->GetGameWorld()) {
            WinOff("Laden");
            return;
        }
        if (!PlayerNpcs.count(WinFig) || !CamFigUsable(WinFig)) {
            WinOff("Mitspieler nicht mehr in der Welt");
            ChatLog(CoopTr("Zuschau-Fenster aus: Mitspieler nicht mehr in der Welt.", "Watch window off: player left the world."), zCOLOR(255, 200, 80, 255));
            return;
        }
        float m[16];
        WinCamera(WinFig, m);
        const float w = WinWidth();
        // v66: Lage - rechts/links 1,2 % Rand, oben 3 % (unter der Fassungszeile oben links), unten 7,5 % (ueber Gothics Leisten);
        // die Hoehe ist im Bildanteil gleich der Breite (Renderer)
        const float x = (WinPos == 1 || WinPos == 3) ? 0.012f : 1.0f - w - 0.012f;
        const float y = WinPos >= 2 ? 1.0f - w - 0.075f : 0.03f;
        WinGdx(1, m, x, y < 0.f ? 0.f : y, w, WinEveryNth);
        WinSentOn = true;
    }

    // jedes Bild aus Game_Loop
    void CoopCamPulse() {
        CoopCamKey();
        WinReadOptions();
        WinAutoPulse();   // v48
        WinPulse();
    }

    // v44: fuer den Coop-Testlauf (v48: keine Vollbild-Kamera mehr)
    oCNpc* CoopCamTarget() {
        return NULL;
    }

#else
    void CoopCamDefaults() {}
    bool CoopCamWatching() { return false; }
    int CoopCamKey() { return 0; }
    bool CoopCamKeyHit(bool) { return false; }
    void CoopCamStop(const char*) {}
    void CoopCamSuspendForSave() {}
    void CoopCamNpcGone(oCNpc*) {}
    void CoopCamToggle() {}
    void CoopCamPulse() {}
    oCNpc* CoopCamTarget() { return NULL; }
    bool CoopCamWindowOn() { return false; }
    bool CoopCamWindowSent() { return false; }
    bool CoopCamWindowAvailable() { return false; }
#endif
}
