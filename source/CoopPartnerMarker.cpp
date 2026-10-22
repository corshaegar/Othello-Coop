// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

    // --- v104: Mitspieler-Markierung (22.10.2026) ---------------------------------------------------------------------------
    //
    // Nutzer: "Jetzt moechte ich noch, dass der Mitspieler auch auf Karten angezeigt wird und es eine Art Markierung des
    // Mitspielers gibt, damit man weiss, wo er ist. Oder man sieht den Namen leicht durch die Waende. ... Wenn der Mitspieler im
    // Blickfeld ist, sollte man auch seinen Namen sehen." Gewaehlt: Name ueber dem Kopf, Pfeil am Bildrand, Marke auf der Karte;
    // hinter Waenden blass.
    //
    // Name und Randpfeil (allgemein G1): je Spielbild fuer jede Coop-Figur in der Welt (PlayerNameToNpc) den Kopfpunkt (Position,
    // Oberkante der Huelle bbox3D + 20 cm) mit zCCamera::activeCam->trafoView in Kamerarichtung umrechnen und wie zCCamera::Project
    // (0x0051D7F0: x = xcenter + viewDistanceX * x/z, y = ycenter - viewDistanceY * y/z) auf den Bildschirm legen - als Anteil des
    // Kamera-Ausschnitts (vpData), damit es mit jeder Aufloesung passt. GD3D11 setzt Sichtfeld und Ausschnitt der Engine-Kamera
    // selbst (zCCamera::SetFOV, Haken auf UpdateViewport) und rechnet mit ihrer Projektion - die Werte stimmen mit dem Bild.
    //  - Im Bild: Name ueber dem Kopf. Frei zu sehen (Strahl Kamera -> Kopf oder Brust ohne statische Wand, zCWorld::
    //    TraceRayNearestHit 0x005F46E0, alle 150 ms): voll (ab 25 m mit Entfernung); sonst blass mit Entfernung.
    //  - Ausserhalb (seitlich, oben/unten, hinter der Kamera): am Bildrand "< Name (23 m)", "Name (23 m) >", "^ ...", "v ..."
    //    (hinter einem: links/rechts nach der Seite, fast genau dahinter unten). Pfeile als Schrift, weil Linien unter GD3D11 erst
    //    mit dessen Linien-Durchgang gezeichnet werden.
    //  - Aus: ohne Verbindung, beim Laden, im Dialog (oCInformationManager), bei offenem Inventar und offener Othello-Karte.
    //  Zeichnen wie die Fassungszeile oben links (Plugin.cpp: screen->Print, Font_Old_10_White_Hi.TGA).
    //
    // Marke auf der Karte (nur Othello, OTHELLO_ABI): Othellos Kartenmarken stehen in einer Liste (std::vector bei +0x156324,
    // Eintrag 0x60 Bytes: Text +0, Bild +0x18, Nummer +0x30, Welt-x +0x34, Welt-z +0x38, Farbe ARGB +0x4C). MAP_ADDMARK (+0x64AB0)
    // holt die Lage aus dem Wegpunkt und legt ueber +0xC84A0 an (ecx Nummer, edx Farbe, xmm1/xmm2 Welt-x/z, Stapel: Text, Bild,
    // vier Werte fuer den Bildausschnitt; Aufrufer raeumt). Beim Oeffnen einer Karte (Othellos Ersatz fuer
    // oCViewDocumentMap::UpdatePosition, +0xC83B0) merkt Othello die Karte (+0x14FDC0) und fuellt die Liste im Skript G_OPENMAP,
    // beim Schliessen (+0xC82F0) leert es sie. Gezeichnet wird jedes Bild aus Welt-x/z (+0xC8A64), Ueberfahren zeigt den Text
    // (ohne Bild, wenn der Bildname leer ist), Klick ruft G_USEMARK(Nummer).
    // Hier: ist eine Karte der aktuellen Welt offen (Ebene der Karte +0x204/+0x208 gleich Dateiname der Welt, wie
    // Hlp_IsWorldFileNamed), legt GT je Mitspieler eine Marke an (Nummer 900 + n, Text "Mitspieler: Name", ohne Bild; G_USEMARK
    // kennt die Nummer nicht - ein Klick tut nichts) und setzt ihre Lage jedes Bild neu (wandert mit, blinkt). Vorher Pruefung der
    // Befehlsbytes dieser Stellen; passt die Fassung von OTHELLO_ABI nicht, bleibt die Karte unveraendert.
    //
    // v105: Nutzer (Test v104): "versuche beide Spieler auf der Karte als roten Pfeil anzuzeigen" - mit eigenem Bild ("waehle
    // weiss als die Farbe fuer Transparenz, damit nur der Pfeil sichtbar ist", gt-fix\assets\coop-pfeil.jpg).
    //  - Nur Othello: Gothics eigener Kartenpfeil (oCViewDocumentMap::ViewArrow, Texturen O/RO/R/RU/U/LU/L/LO.TGA, gesetzt in
    //    UpdatePosition 0x00726030) ist unter Othello aus - OTHELLO_ABI hat den Sprung bei 0x007260F1 genullt (74 0A -> 90 90),
    //    der Pfeil wird immer geschlossen (zCViewFX::Close 0x00754170).
    //  - Bild: tools\arrowtex.py macht daraus eine Gothic-Textur (ZTEX, DXT3 128 x 256 mit Mipmaps, Weiss durchsichtig),
    //    die Bytes stehen in CoopArrowTex.h. GT schreibt sie bei Bedarf nach system\GothicCoopArrow.tex und laedt sie wie Gothic
    //    die Vorschaubilder der Spielstaende (oCSavegameInfo::ReloadResources 0x00434380: zfactory->CreateZFile, Exists/Open,
    //    zrenderer->CreateTexture, zCTextureFileFormatInternal::LoadTexture(zFILE&, zCTextureExchange*) 0x005C7D10).
    //  - Lage: je Spieler eine unsichtbare Othello-Marke (Farbe mit Deckkraft 0 - Othello zeichnet Marken mit Ueberblendung
    //    SRCALPHA/INVSRCALPHA und Deckkraft Textur x Farbe; Ueberfahren zeigt weiter den Namen): eigener Held 899, Mitspieler
    //    900 + n. Othello rechnet daraus jedes Bild das Bildschirmrechteck (+0x3C..+0x48; Punkt = Mitte unten). Den Massstab beider
    //    Achsen (fuer die Drehung) liefert eine Hilfsmarke 898 eine Million Einheiten neben dem eigenen Helden (weit ausserhalb des
    //    Bilds) - so stimmt die Richtung auch bei ungleich gestreckter Karte.
    //  - Zeichnen: Othello zeichnet seine Marken in oCGame::Render (Aufruf 0x0063DEE9 -> OTHELLO_ABI+0x11660 -> +0xC9420, nach
    //    zCViewDraw::Render, vor EndFrame). Danach ruft Render oCGame::CheckObjectRoutines (0x00640160, nur dort, unveraendert) -
    //    in diesem Haken zeichnet GT die Pfeile wie zCView::Blit seine Bilder (Z-Puffer aus, Ueberblendung BLEND, bilinear, voller
    //    Ausschnitt; zrenderer->DrawPolySimple mit vier gedrehten Ecken). Groesse 3,5 % der Bildhoehe (v106, vorher 5 %), Spitze in Blickrichtung
    //    (GetAtVectorWorld), eigener Pfeil zuletzt (oben). Bildgroesse aus zCRnd_D3D +0x984/+0x988 (XD3D_SetMode 0x0071FFD2 legt
    //    dort Breite/Hoehe ab; die SDK-Namen sind um ein Feld verschoben - Othello liest dieselben Felder).

#if ENGINE == Engine_G1

#include "CoopArrowTex.h"

    static const int PM_MARK_ID = 900;     // Mitspieler 900 + n
    static const int PM_MARK_SELF = 899;   // eigener Held
    static const int PM_MARK_PROBE = 898;  // Hilfsmarke fuer den Massstab
    static const float PM_PROBE_OFS = 1000000.f;
    static const unsigned int PM_MARK_HIDDEN = 0x00802020u;   // Deckkraft 0 (sonst dunkelrot unter dem Pfeil)

    static void PmLog(const char* fmt, ...) {
        static CoopDiagBudget budget = { 0, 0 };
        if (!CoopDiagAllow(budget, 30))
            return;
        char buf[600];
        va_list ap;
        va_start(ap, fmt);
        _vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        buf[sizeof(buf) - 1] = 0;
        OthelloCompatLog("[%lld s, %s] Mitspieler-Markierung: %s", DiagSeconds(), DiagRole(), buf);
    }

    // --- Othellos Karte ---

    struct PmAbiMap {
        void** doc;     // offene Karte (oCViewDocumentMap) oder NULL
        char** vbeg;    // Markenliste
        char** vend;
        void* add;      // Marke anlegen
    };

    static int PmMapState = 0;   // 0 ungeprueft, 1 erkannt, 2 aus

    static PmAbiMap* PmMapAbi() {
        static PmAbiMap m;
        if (PmMapState)
            return PmMapState == 1 ? &m : NULL;
        PmMapState = 2;
        HMODULE abi = GetModuleHandleW(L"OTHELLO_ABI.DLL");
        if (!abi)
            return NULL;   // ohne Othello keine Karte
        unsigned char* b = (unsigned char*)abi;
        const unsigned int base = (unsigned int)b;
        const bool readable = IsReadableMemory(b + 0x64982, 0x10) && IsReadableMemory(b + 0x64C99, 5) &&
            IsReadableMemory(b + 0xC83DE, 6) && IsReadableMemory(b + 0xC84A0, 6) && IsReadableMemory(b + 0xC8559, 6) &&
            IsReadableMemory(b + 0xC858F, 0x16) && IsReadableMemory(b + 0xC9865, 6);
        const bool ok = readable &&
            b[0xC83DE] == 0x89 && b[0xC83DF] == 0x35 && *(unsigned int*)(b + 0xC83E0) == base + 0x14FDC0 &&   // mov [Karte], esi
            b[0xC8559] == 0x8B && b[0xC855A] == 0x35 && *(unsigned int*)(b + 0xC855B) == base + 0x156328 &&   // mov esi, [Ende]
            b[0xC9865] == 0x8B && b[0xC9866] == 0x3D && *(unsigned int*)(b + 0xC9867) == base + 0x156324 &&   // mov edi, [Anfang]
            memcmp(b + 0xC84A0, "\x55\x8B\xEC\x6A\xFF\x68", 6) == 0 &&
            b[0x64C99] == 0xE8 && base + 0x64C9E + *(unsigned int*)(b + 0x64C9A) == base + 0xC84A0 &&          // MAP_ADDMARK ruft sie
            memcmp(b + 0xC858F, "\x89\x46\x30", 3) == 0 &&       // Nummer +0x30
            memcmp(b + 0xC8595, "\x89\x46\x4C", 3) == 0 &&       // Farbe +0x4C
            memcmp(b + 0xC85A1, "\x0F\x11\x46\x34", 4) == 0 &&   // x/z +0x34/+0x38
            memcmp(b + 0x64982, "\x83\xB8\x08\x02\x00\x00\x00", 7) == 0 &&   // Ebene der Karte: Laenge +0x208
            memcmp(b + 0x6498B, "\x8B\x80\x04\x02\x00\x00", 6) == 0;         //                  Text +0x204
        if (!ok) {
            PmLog("OTHELLO_ABI bei %p: Befehlsbytes der Karte weichen ab - keine Marke auf der Karte", abi);
            return NULL;
        }
        m.doc = (void**)(b + 0x14FDC0);
        m.vbeg = (char**)(b + 0x156324);
        m.vend = (char**)(b + 0x156328);
        m.add = b + 0xC84A0;
        PmMapState = 1;
        PmLog("Othellos Karte erkannt (OTHELLO_ABI bei %p)", abi);
        return &m;
    }

    static bool PmMapOpen() {
        PmAbiMap* m = PmMapAbi();
        return m && *m->doc;
    }

    // std::string von OTHELLO_ABI (MSVC 2015+: 16 Bytes Puffer oder Zeiger, Laenge +0x10, Platz +0x14) - wird nur gelesen
    struct PmAbiStr {
        union {
            char buf[16];
            const char* ptr;
        } u;
        unsigned int size;
        unsigned int cap;
    };

    static void PmAbiStrSet(PmAbiStr& s, const char* text) {
        memset(&s, 0, sizeof(s));
        s.size = (unsigned int)strlen(text);
        if (s.size < 16) {
            memcpy(s.u.buf, text, s.size + 1);
            s.cap = 15;
        }
        else {
            s.u.ptr = text;
            s.cap = s.size;
        }
    }

    static bool PmAbiAddMark(void* fn, int id, unsigned int argb, float x, float z, PmAbiStr* text, PmAbiStr* icon) {
        __try {
            __asm {
                push 0
                push 0
                push 0
                push 0
                push icon
                push text
                mov ecx, id
                mov edx, argb
                movss xmm1, x
                movss xmm2, z
                mov eax, fn
                call eax
                add esp, 0x18
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
        return true;
    }

    static char* PmFindMark(PmAbiMap* m, int id) {
        char* b = *m->vbeg;
        char* e = *m->vend;
        if (!b || e < b || (e - b) % 0x60 != 0 || e - b > 0x60 * 1024)
            return NULL;
        for (char* p = b; p < e; p += 0x60)
            if (*(int*)(p + 0x30) == id)
                return p;
        return NULL;
    }

    static std::string PmName(oCNpc* n) {
        std::string s = n->name[0].ToChar();
        if (s.empty() && PlayerNpcs.count(n))
            s = PlayerNpcs[n].ToChar();
        return s.empty() ? std::string(CoopTr("Mitspieler", "Partner")) : s;
    }

    // Karte der aktuellen Welt offen? (Ebene der Karte wie Hlp_IsWorldFileNamed)
    static bool PmMapOfThisWorld(PmAbiMap* m, const char** level) {
        char* doc = (char*)*m->doc;
        oCWorld* w = ogame ? ogame->GetGameWorld() : NULL;
        if (!doc || !w || !IsReadableMemory(doc + 0x204, 8))
            return false;
        const char* lvl = *(const char**)(doc + 0x204);
        const int len = *(int*)(doc + 0x208);
        if (!lvl || len <= 0 || len > 260 || !IsReadableMemory(lvl, len + 1))
            return false;
        zSTRING wn = w->GetWorldFilename();
        if (_stricmp(lvl, wn.ToChar()) != 0)
            return false;
        if (level)
            *level = lvl;
        return true;
    }

    // Marke anlegen oder Lage setzen; false = OTHELLO_ABI-Aufruf fehlgeschlagen (Karte ab dann aus)
    static bool PmMarkAt(PmAbiMap* m, int id, float x, float z, const std::string& text, const char* level) {
        char* e = PmFindMark(m, id);
        if (!e) {
            PmAbiStr t, i;
            PmAbiStrSet(t, text.c_str());
            PmAbiStrSet(i, "");
            if (!PmAbiAddMark(m->add, id, PM_MARK_HIDDEN, x, z, &t, &i)) {
                PmMapState = 2;
                PmLog("Marke anlegen in OTHELLO_ABI fehlgeschlagen (Ausnahme) - Karte ab jetzt ohne Pfeile");
                return false;
            }
            if (id != PM_MARK_PROBE)
                PmLog("Marke %d fuer %s auf der Karte (%s, %.0f/%.0f)", id, text.c_str(), level, x, z);
            e = PmFindMark(m, id);
            if (!e)
                return true;
        }
        *(float*)(e + 0x34) = x;
        *(float*)(e + 0x38) = z;
        *(unsigned int*)(e + 0x4C) = PM_MARK_HIDDEN;
        return true;
    }

    static void PmMapPulse() {
        PmAbiMap* m = PmMapAbi();
        const char* lvl = NULL;
        if (!m || !PmMapOfThisWorld(m, &lvl))
            return;
        const zVEC3 me = player->GetPositionWorld();
        if (!PmMarkAt(m, PM_MARK_SELF, me.n[0], me.n[2], std::string(CoopTr("Du: ", "You: ")) + MyNickname.ToChar(), lvl) ||
            !PmMarkAt(m, PM_MARK_PROBE, me.n[0] + PM_PROBE_OFS, me.n[2] + PM_PROBE_OFS, std::string(), lvl))
            return;
        int n = 0;
        for (auto& kv : PlayerNameToNpc) {
            const int id = PM_MARK_ID + n++;
            oCNpc* npc = kv.second;
            if (!TpInWorld(npc))
                continue;
            const zVEC3 p = npc->GetPositionWorld();
            if (!PmMarkAt(m, id, p.n[0], p.n[2], std::string(CoopTr("Mitspieler: ", "Partner: ")) + PmName(npc), lvl))
                return;
        }
    }

    // --- roter Pfeil (eigenes Bild) ---

    static zCTexture* PmArrowTex = NULL;
    static int PmArrowTries = 0;
    static long long PmArrowNextTryMs = 0;

    // system\GothicCoopArrow.tex = CoopArrowZtex (nur schreiben, wenn sie fehlt oder abweicht)
    static bool PmArrowFile() {
        const std::string path = CoopSystemFile("GothicCoopArrow.tex");
        if (FILE* f = fopen(path.c_str(), "rb")) {
            std::string have;
            char buf[4096];
            size_t k;
            while ((k = fread(buf, 1, sizeof(buf), f)) > 0 && have.size() <= sizeof(CoopArrowZtex))
                have.append(buf, k);
            fclose(f);
            if (have.size() == sizeof(CoopArrowZtex) && memcmp(have.data(), CoopArrowZtex, sizeof(CoopArrowZtex)) == 0)
                return true;
        }
        FILE* f = fopen(path.c_str(), "wb");
        if (!f)
            return false;
        const bool ok = fwrite(CoopArrowZtex, 1, sizeof(CoopArrowZtex), f) == sizeof(CoopArrowZtex);
        fclose(f);
        return ok;
    }

    static zCTexture* PmArrowLoad() {
        if (!zfactory || !zrenderer)
            return NULL;
        if (!PmArrowFile()) {
            PmLog("Pfeil: system\\GothicCoopArrow.tex nicht schreibbar");
            return NULL;
        }
        zFILE* f = zfactory->CreateZFile(zSTRING("\\SYSTEM\\GOTHICCOOPARROW.TEX"));
        if (!f)
            return NULL;
        zCTexture* tex = NULL;
        const char* why = "Datei nicht gefunden";
        if (f->Exists() && f->Open(false) == 0) {
            why = "Laden fehlgeschlagen";
            zCTextureFileFormatInternal* fmt = zDYNAMIC_CAST<zCTextureFileFormatInternal>(zCObject::CreateNewInstance(zSTRING("zCTextureFileFormatInternal")));
            tex = zrenderer->CreateTexture();
            zCTextureExchange* ex = tex;
            if (tex && (char*)ex - (char*)tex != 0x54) {   // wie ReloadResources (tex + 0x54)
                why = "Texturaufbau weicht ab";
                tex->Release();
                tex = NULL;
            }
            if (!fmt || !tex || !fmt->LoadTexture(*f, ex)) {
                if (tex)
                    tex->Release();
                tex = NULL;
            }
            if (fmt)
                fmt->Release();
            f->Close();
        }
        delete f;
        if (!tex) {
            PmLog("Pfeil: Textur nicht geladen (%s)", why);
            return NULL;
        }
        tex->cacheOutLock = 1;   // nie auslagern (kein Dateiname zum Nachladen)
        PmLog("Pfeil: Textur geladen (%u Bytes, DXT3 128 x 256)", (unsigned)sizeof(CoopArrowZtex));
        return tex;
    }

    static zCTexture* PmArrow() {
        if (PmArrowTex && PmArrowTex->cacheState == zRES_CACHED_IN)
            return PmArrowTex;
        const long long now = GetCurrentMs();
        if (PmArrowTries >= 3 || now < PmArrowNextTryMs)
            return NULL;
        PmArrowTries++;
        PmArrowNextTryMs = now + 5000;
        if (PmArrowTex)
            PmLog("Pfeil: Textur nicht mehr geladen (Zustand %d) - neu", (int)PmArrowTex->cacheState);
        PmArrowTex = PmArrowLoad();
        return PmArrowTex && PmArrowTex->cacheState == zRES_CACHED_IN ? PmArrowTex : NULL;
    }

    // wie zTRndSimpleVertex (0x18 Bytes: x, y, z, u, v, Farbe 0xAARRGGBB)
    struct PmVert {
        float x, y, z, u, v;
        unsigned int color;
    };

    // Punkt einer Marke auf dem Bildschirm (Othello: Rechteck X-a..X+a, Y-b..Y)
    static bool PmMarkScreen(char* e, float& x, float& y) {
        const float x1 = *(float*)(e + 0x3C), y1 = *(float*)(e + 0x40), x2 = *(float*)(e + 0x44), y2 = *(float*)(e + 0x48);
        if (!_finite(x1) || !_finite(x2) || !_finite(y1) || !_finite(y2) || x2 <= x1 || y2 <= y1)
            return false;
        x = 0.5f * (x1 + x2);
        y = y2;
        return true;
    }

    // Pfeil um (cx, cy), Spitze in Richtung (dx, dy) (Bildschirm, y nach unten); das Bild zeigt nach unten (v = 1 = Spitze)
    static void PmDrawArrow(zCTexture* tex, float cx, float cy, float dx, float dy, float len) {
        const float w = 0.5f * len, h = len;
        const float lx[4] = { -0.5f * w, 0.5f * w, 0.5f * w, -0.5f * w };
        const float ly[4] = { -0.5f * h, -0.5f * h, 0.5f * h, 0.5f * h };
        const float tu[4] = { 0.f, 1.f, 1.f, 0.f };
        const float tv[4] = { 0.f, 0.f, 1.f, 1.f };
        PmVert v[4];
        for (int i = 0; i < 4; i++) {
            v[i].x = cx + dy * lx[i] + dx * ly[i];   // Drehung, die (0, 1) auf (dx, dy) legt
            v[i].y = cy - dx * lx[i] + dy * ly[i];
            v[i].z = 1.f;
            v[i].u = tu[i];
            v[i].v = tv[i];
            v[i].color = 0xFFFFFFFFu;
        }
        zrenderer->DrawPolySimple(tex, (zTRndSimpleVertex*)v, 4);
    }

    static bool PmScreenDir(oCNpc* npc, float kx, float kz, float& dx, float& dy) {
        const zVEC3 at = npc->GetAtVectorWorld();
        dx = at.n[0] * kx;
        dy = at.n[2] * kz;
        const float l = sqrtf(dx * dx + dy * dy);
        if (!(l > 1e-6f))
            return false;
        dx /= l;
        dy /= l;
        return true;
    }

    static void PmDrawArrows() {
        if ((!ServerThread && !ClientThread) || !player || IsLoadingLevel || !zrenderer)
            return;
        PmAbiMap* m = PmMapAbi();
        if (!m || !PmMapOfThisWorld(m, NULL))
            return;
        char* self = PmFindMark(m, PM_MARK_SELF);
        char* probe = PmFindMark(m, PM_MARK_PROBE);
        float sx, sy, px, py;
        if (!self || !probe || !PmMarkScreen(self, sx, sy) || !PmMarkScreen(probe, px, py))
            return;
        // Massstab je Achse (Bildpunkte je Welteinheit, mit Vorzeichen) aus der Hilfsmarke
        const float kx = (px - sx) / (*(float*)(probe + 0x34) - *(float*)(self + 0x34));
        const float kz = (py - sy) / (*(float*)(probe + 0x38) - *(float*)(self + 0x38));
        if (!_finite(kx) || !_finite(kz) || fabsf(kx) < 1e-7f || fabsf(kz) < 1e-7f)
            return;
        const int W = *(int*)((char*)zrenderer + 0x984), H = *(int*)((char*)zrenderer + 0x988);
        if (W < 320 || W > 16384 || H < 200 || H > 16384)
            return;
        zCTexture* tex = PmArrow();
        if (!tex)
            return;
        const float len = 0.035f * (float)H;   // v106: Nutzer "die Pfeile etwas kleiner machen auf der Map" (vorher 5 %)
        const int zw = zrenderer->GetZBufferWriteEnabled();
        const zTRnd_ZBufferCmp zc = zrenderer->GetZBufferCompare();
        const zTRnd_AlphaBlendFunc af = zrenderer->GetAlphaBlendFunc();
        const int bf = zrenderer->GetBilerpFilterEnabled();
        zrenderer->SetZBufferWriteEnabled(FALSE);
        zrenderer->SetZBufferCompare(zRND_ZBUFFER_CMP_ALWAYS);
        zrenderer->SetAlphaBlendFunc(zRND_ALPHA_FUNC_BLEND);
        zrenderer->SetBilerpFilterEnabled(TRUE);
        zrenderer->SetViewport(0, 0, W, H);
        float dx, dy, x, y;
        int n = 0;
        for (auto& kv : PlayerNameToNpc) {
            char* e = PmFindMark(m, PM_MARK_ID + n++);
            if (e && TpInWorld(kv.second) && PmMarkScreen(e, x, y) && PmScreenDir(kv.second, kx, kz, dx, dy))
                PmDrawArrow(tex, x, y, dx, dy, len);
        }
        if (PmScreenDir(player, kx, kz, dx, dy))
            PmDrawArrow(tex, sx, sy, dx, dy, len);
        zrenderer->SetBilerpFilterEnabled(bf);
        zrenderer->SetAlphaBlendFunc(af);
        zrenderer->SetZBufferCompare(zc);
        zrenderer->SetZBufferWriteEnabled(zw);
        static bool logged = false;
        if (!logged) {
            logged = true;
            PmLog("Pfeile gezeichnet: Massstab %.6f/%.6f Bildpunkte je Einheit, Bild %d x %d, Pfeil %.0f Bildpunkte", kx, kz, W, H, len);
        }
    }

    // Haken: nach Othellos Kartenmarken (oCGame::Render 0x0063DF06), vor EndFrame - hier die Pfeile
    void __fastcall oCGame_CheckObjectRoutines_Coop(oCGame* _this, void* vtable);
    CInvoke<void(__thiscall*)(oCGame*)> Ivk_oCGame_CheckObjectRoutines_Coop(0x00640160, &oCGame_CheckObjectRoutines_Coop);
    void __fastcall oCGame_CheckObjectRoutines_Coop(oCGame* _this, void* vtable) {
        PmDrawArrows();
        Ivk_oCGame_CheckObjectRoutines_Coop(_this);
    }

    // --- Name ueber dem Kopf / Pfeil am Bildrand ---

    struct PmSight {
        long long at;
        bool free;
    };
    static std::map<oCNpc*, PmSight> PmSightCache;
    static std::set<oCNpc*> PmLogged;

    static zVEC3 PmHead(oCNpc* n) {
        zVEC3 p = n->GetPositionWorld();
        const float top = n->bbox3D.maxs.n[1];
        const float h = top - p.n[1];
        p.n[1] = (h > 30.f && h < 300.f) ? top + 20.f : p.n[1] + 110.f;
        return p;
    }

    // Strahl ohne statische Wand (Laub und Gitter mit Durchsicht zaehlen nicht), endet kurz vor dem Ziel
    static bool PmFreeSight(const zVEC3& from, const zVEC3& to) {
        zCWorld* w = ogame ? ogame->GetGameWorld() : NULL;
        if (!w)
            return true;
        const float dx = to.n[0] - from.n[0], dy = to.n[1] - from.n[1], dz = to.n[2] - from.n[2];
        const float len = sqrtf(dx * dx + dy * dy + dz * dz);
        if (len < 60.f)
            return true;
        const float k = (len - 30.f) / len;
        const zVEC3 ray(dx * k, dy * k, dz * k);
        return !(w->TraceRayNearestHit(from, ray, (zCVob*)NULL, zTRACERAY_STAT_POLY | zTRACERAY_VOB_IGNORE | zTRACERAY_POLY_IGNORE_TRANSP)
            && w->traceRayReport.foundHit);
    }

    static void PmPrint(int x, int y, zSTRING& s, const zCOLOR& c) {
        const int w = screen->FontSize(s);
        const int h = screen->FontY();
        if (x > 8192 - 60 - w) x = 8192 - 60 - w;
        if (x < 60) x = 60;
        if (y > 8192 - 60 - h) y = 8192 - 60 - h;
        if (y < 60) y = 60;
        screen->SetFontColor(c);
        screen->Print(x, y, s);
    }

    static void PmDrawPartner(zCCamera* cam, oCNpc* n, long long now) {
        const zVEC3 head = PmHead(n);
        const zVEC3 at = n->GetPositionWorld();
        const zVEC3 me = player->GetPositionWorld();
        const float ddx = at.n[0] - me.n[0], ddy = at.n[1] - me.n[1], ddz = at.n[2] - me.n[2];
        const int meters = (int)(sqrtf(ddx * ddx + ddy * ddy + ddz * ddz) / 100.f + 0.5f);
        const std::string name = PmName(n);
        char dist[32];
        _snprintf(dist, sizeof(dist), " (%d m)", meters);
        dist[sizeof(dist) - 1] = 0;
        if (!PmLogged.count(n)) {
            PmLogged.insert(n);
            PmLog("zeigt %s (%d m entfernt)", name.c_str(), meters);
        }

        const zVEC3 v = cam->trafoView * head;   // Kamerarichtung: x rechts, y oben, z vorn
        const zTViewportData& vp = cam->vpData;
        float nx = 0.5f, ny = 0.5f;   // Anteil am Bild, links/oben = 0
        bool inView = false;
        if (v.n[2] > 20.f && vp.xdim > 0 && vp.ydim > 0) {
            const float px = vp.xcenter + cam->viewDistanceX * v.n[0] / v.n[2];
            const float py = vp.ycenter - cam->viewDistanceY * v.n[1] / v.n[2];
            nx = (px - (float)vp.xmin) / (float)vp.xdim;
            ny = (py - (float)vp.ymin) / (float)vp.ydim;
            inView = nx > 0.02f && nx < 0.98f && ny > 0.04f && ny < 0.96f;
        }
        const int fy = screen->FontY();

        if (inView) {
            PmSight& s = PmSightCache[n];
            if (now - s.at > 150 || now < s.at) {
                s.at = now;
                const zVEC3 cp(cam->trafoViewInv.v[0][3], cam->trafoViewInv.v[1][3], cam->trafoViewInv.v[2][3]);
                const zVEC3 eye(head.n[0], head.n[1] - 30.f, head.n[2]);
                const zVEC3 chest(at.n[0], at.n[1] + 30.f, at.n[2]);
                s.free = PmFreeSight(cp, eye) || PmFreeSight(cp, chest);
            }
            zSTRING t(name.c_str());
            if (!s.free || meters >= 25)
                t = t + zSTRING(dist);
            const int w = screen->FontSize(t);
            PmPrint((int)(nx * 8192.f) - w / 2, (int)(ny * 8192.f) - fy, t,
                s.free ? zCOLOR(255, 235, 160, 255) : zCOLOR(185, 185, 185, 150));
            return;
        }

        // Richtung vom Bildmittelpunkt (y nach unten)
        float dx, dy;
        if (v.n[2] > 20.f) {
            dx = nx - 0.5f;
            dy = ny - 0.5f;
        }
        else {
            const float back = 20.f - v.n[2];   // > 0: hinter der Kamera
            if (fabsf(v.n[0]) < 0.3f * back) {   // fast genau dahinter: unten
                dx = 0.5f * v.n[0] / back;
                dy = 1.f;
            }
            else {
                dx = v.n[0] > 0.f ? 1.f : -1.f;
                dy = -0.5f * v.n[1] / (fabsf(v.n[0]) + 1.f);
                if (dy > 0.4f) dy = 0.4f;
                if (dy < -0.4f) dy = -0.4f;
            }
        }
        const float lx = 0.485f, ly = 0.46f;
        const float tx = fabsf(dx) > 1e-4f ? lx / fabsf(dx) : 1e9f;
        const float ty = fabsf(dy) > 1e-4f ? ly / fabsf(dy) : 1e9f;
        const float t = tx < ty ? tx : ty;
        const int X = (int)((0.5f + dx * t) * 8192.f);
        const int Y = (int)((0.5f + dy * t) * 8192.f);
        zSTRING label;
        int x, y;
        if (tx <= ty && dx < 0.f) {
            label = zSTRING("< ") + zSTRING(name.c_str()) + zSTRING(dist);
            x = X;
            y = Y - fy / 2;
        }
        else if (tx <= ty) {
            label = zSTRING(name.c_str()) + zSTRING(dist) + zSTRING(" >");
            x = X - screen->FontSize(label);
            y = Y - fy / 2;
        }
        else if (dy < 0.f) {
            label = zSTRING("^ ") + zSTRING(name.c_str()) + zSTRING(dist);
            x = X - screen->FontSize(label) / 2;
            y = Y;
        }
        else {
            label = zSTRING("v ") + zSTRING(name.c_str()) + zSTRING(dist);
            x = X - screen->FontSize(label) / 2;
            y = Y - fy;
        }
        PmPrint(x, y, label, zCOLOR(255, 235, 160, 230));
    }

    static bool PmHudOff() {
        if (!ogame || !player || IsLoadingLevel || !ogame->GetGameWorld() || !screen)
            return true;
        oCInformationManager& im = oCInformationManager::GetInformationManager();
        if (im.Npc && !im.IsDone)
            return true;   // Dialog
        if (player->inventory2.IsOpen())
            return true;
        return PmMapOpen();   // auf der Karte zeigt die Marke den Mitspieler
    }

    // Plugin.cpp, LoadBegin: Zeiger der alten Welt verwerfen
    void CoopPartnerMarkerWorldGone() {
        PmSightCache.clear();
        PmLogged.clear();
    }

    // Plugin.cpp, Game_Loop
    void CoopPartnerMarkerPulse() {
        if (!ServerThread && !ClientThread)
            return;
        if (!player || IsLoadingLevel)
            return;
        PmMapPulse();
        if (PmHudOff())
            return;
        zCCamera* cam = zCCamera::activeCam;
        if (!cam)
            return;
        const long long now = GetCurrentMs();
        if (PmSightCache.size() > 64)
            PmSightCache.clear();
        zSTRING font = screen->GetFontName();
        const zCOLOR color = screen->fontColor;
        screen->SetFont(zSTRING("Font_Old_10_White_Hi.TGA"));
        for (auto& kv : PlayerNameToNpc)
            if (TpInWorld(kv.second))
                PmDrawPartner(cam, kv.second, now);
        screen->SetFont(font);
        screen->SetFontColor(color);
    }

#else
    void CoopPartnerMarkerWorldGone() {}
    void CoopPartnerMarkerPulse() {}
#endif
}
