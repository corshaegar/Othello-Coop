// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

    // --- v102: Teleport-Rune zum Mitspieler (22.10.2026) ------------------------------------------------------------------
    //
    // Nutzer: "Ich moechte jetzt ein neues Item einfuehren. Eine Teleportrune. Diese teleportiert einen zum Mitspieler. Jeder
    // Spieler startet mit einer Teleport-Rune. Es wird am Anfang jedes Spiel-Ladens geprueft, ob der Spieler sie im Inventar hat,
    // und wenn nicht, wird sie eingefuegt. Der Wert sollte 0 sein. ... Die Teleport-Animation sollte durchgefuehrt werden. Wir
    // machen erstmal Manakosten 0."
    //
    // So funktionieren Teleport-Runen (G1-Modkit, Othello-DAT): Rune = Item mit spell = SPL_TELEPORTn. Beim Wirken laedt die
    // Engine auf (oCSpell::Invest 0x0047CD40: erst "Mana > 0?", dann je time_per_mana 750 ms CallScriptInvestedMana 0x0047D930
    // -> Spell_ProcessMana -> Spell_Logic_TeleportN, ab dem zweiten Schritt ChangeAttribute(Mana, -1)). Ab 5 investiertem Mana
    // (SPL_SENDCAST_TELEPORT) ruft die Logik AI_Teleport(self, "Wegpunkt") (External 0x00650460: EV_BEAMTO -> oCNpc::BeamTo
    // 0x00693B10, nur ueber Namen) und AI_PlayAni(self, "T_HEASHOOT_2_STAND"). Othello sperrt alle fuenf, solange man in Xardas'
    // altem Turm ist; Othellos eigene Runen 6-12 haben keinen Zauber, sie schalten Marken auf der Karte frei (MAP_TELEPORT).
    //
    // Hier (allgemein G1, Rune in system\Autorun\COOP_ITEMS.D): ITRU_COOP_TELEPORT mit spell = SPL_TELEPORT5.
    //  - Mana 0: Haken auf oCSpell::Invest - zaubert der eigene Held mit dieser Rune (oCMag_Book::GetSpellItem(oCSpell*)
    //    0x00472E40), hat er waehrenddessen mindestens 1 Mana (sonst bricht die Engine ab), danach wieder genau den alten Wert.
    //  - Ziel: das External AI_TELEPORT wird umgeleitet (Symbol-Funktionszeiger wie CoopDialogSync.cpp). Waehrend dieses Wirkens
    //    nimmt GT die Parameter vom Stapel und setzt den Helden im naechsten Spielbild neben den Mitspieler (CoopPlaceBesideHost,
    //    CoopSaveSync.cpp: freie Stelle mit Boden, ohne Wand/Wasser; sonst oCVob::SearchNpcPosition an seiner Stelle), Bewegung
    //    zuruecksetzen wie nach dem Laden (PendingMoveReset). Alle anderen AI_Teleport-Aufrufe gehen unveraendert ans Original.
    //  - Mitspieler: anvisierte Coop-Figur, sonst beim Mitspieler die Figur des Hosts, sonst die erste (nach Name).
    //  - Startausstattung: 3 s und 15 s nach jedem Laden (auch neues Spiel, Weltwechsel; nach dem Wiederherstellen des eigenen
    //    Helden, CoopSaveSync.cpp) - ohne Rune im Inventar (oCNpc::IsInInv(int, int) 0x006A4E80) eine anlegen (PutInInv 0x006A4FF0,
    //    unter WorldSyncApplying: kein Welt-Abgleich).
    //  - Beim Partner spielt GT den Zauber nur mit einem Aufladeschritt nach (RemoteNpc::UpdateSpellCasts) - die Logik teleportiert
    //    erst ab 5 Mana, die Figur springt dort also nicht.
    // Engine-Stellen (Speicherabbild): alle unveraendert.

#if ENGINE == Engine_G1

    static int TpRuneIdx = -2;               // Symbol ITRU_COOP_TELEPORT (-1: COOP_ITEMS.D fehlt)
    static bool TpCasting = false;           // gerade wirkt der eigene Held die Coop-Rune (innerhalb von oCSpell::Invest)
    static bool TpPending = false;           // im naechsten Spielbild zum Mitspieler
    static bool TpExtInstalled = false;
    typedef int(__cdecl* CoopTpExternal)();
    static CoopTpExternal TpOrigAiTeleport = NULL;
    static long long TpEnsureAt1 = 0, TpEnsureAt2 = 0;

    static void TpLog(const char* fmt, ...) {
        static CoopDiagBudget budget = { 0, 0 };
        if (!CoopDiagAllow(budget, 30))
            return;
        char buf[600];
        va_list ap;
        va_start(ap, fmt);
        _vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        buf[sizeof(buf) - 1] = 0;
        OthelloCompatLog("[%lld s, %s] Teleport-Rune: %s", DiagSeconds(), DiagRole(), buf);
    }

    static int TpRune() {
        if (TpRuneIdx == -2 && parser) {
            TpRuneIdx = parser->GetIndex(zSTRING("ITRU_COOP_TELEPORT"));
            if (TpRuneIdx < 0)
                OthelloCompatLog("Teleport-Rune: ITRU_COOP_TELEPORT fehlt im Skript (system\\Autorun\\COOP_ITEMS.D nicht da?) - Rune aus");
        }
        return TpRuneIdx;
    }

    // Coop-Rune im Zauberbuch des eigenen Helden zu diesem Zauber (sonst NULL)
    static oCItem* TpCoopRuneItem(oCSpell* spell) {
        if (!spell || !player || TpRune() < 0)
            return NULL;
        oCMag_Book* book = player->GetSpellBook();
        oCItem* it = book ? book->GetSpellItem(spell) : NULL;
        return it && it->GetInstance() == TpRuneIdx ? it : NULL;
    }

    static bool TpIsCoopRune(oCSpell* spell) {
        return spell && spell->spellCasterNpc == player && TpCoopRuneItem(spell);
    }

    // Haken: Aufladen - fuer die Coop-Rune ohne Manakosten
    int __fastcall oCSpell_Invest_Coop(oCSpell* _this, void* vtable);
    CInvoke<int(__thiscall*)(oCSpell*)> Ivk_oCSpell_Invest_Coop(0x0047CD40, &oCSpell_Invest_Coop);
    int __fastcall oCSpell_Invest_Coop(oCSpell* _this, void* vtable) {
        if (!TpIsCoopRune(_this))
            return Ivk_oCSpell_Invest_Coop(_this);
        const int atr = (_this->spellEnergy >= 0 && _this->spellEnergy < NPC_ATR_MAX) ? _this->spellEnergy : NPC_ATR_MANA;
        const int saved = player->GetAttribute(atr);
        if (saved < 1)
            player->SetAttribute(atr, 1);   // die Engine laedt nur mit Mana > 0 auf
        TpCasting = true;
        const int r = Ivk_oCSpell_Invest_Coop(_this);
        TpCasting = false;
        if (player->GetAttribute(atr) != saved)
            player->SetAttribute(atr, saved);   // Manakosten 0
        return r;
    }

    // v103: Nutzer (Test v102): "Beim Auswaehlen steht der Text 'Teleport ins Sumpflager'". Den Namen beim Wechsel im Zauberkreis
    // zeigt oCMag_Book::DoPerFrame (0x004726F6: spells[gewaehlt]->GetName(), dann kurz auf den Bildschirm); oCSpell::GetName
    // (0x0047E1B0, unveraendert) liest Gothics Namenstabelle nach der Zauber-ID - fuer SPL_TELEPORT5 Othellos Teleport ins
    // Sumpflager. Fuer die Coop-Rune jetzt ihr eigener Name (Item-Name aus COOP_ITEMS.D). Rueckgabe zSTRING per Wert: Aufrufer legt
    // den Platz an und schiebt ihn als (versteckten) ersten Parameter, Funktion endet mit ret 4 - hier ausdruecklich so deklariert.
    zSTRING* __fastcall oCSpell_GetName_Coop(oCSpell* _this, void* vtable, zSTRING* ret);
    CInvoke<zSTRING*(__thiscall*)(oCSpell*, zSTRING*)> Ivk_oCSpell_GetName_Coop(0x0047E1B0, &oCSpell_GetName_Coop);
    zSTRING* __fastcall oCSpell_GetName_Coop(oCSpell* _this, void* vtable, zSTRING* ret) {
        zSTRING* r = Ivk_oCSpell_GetName_Coop(_this, ret);
        oCItem* it = r ? TpCoopRuneItem(_this) : NULL;
        if (it && it->name.Length() > 0)
            *r = it->name;
        return r;
    }

    // External AI_Teleport(self, waypoint) - beim Wirken der Coop-Rune: Ziel Mitspieler
    static int __cdecl CoopExtAiTeleport() {
        zCParser* p = zCParser::GetParser();
        if (!TpCasting || !p || p != parser)
            return TpOrigAiTeleport ? TpOrigAiTeleport() : 0;
        zSTRING wp;
        p->GetParameter(wp);   // letzter Parameter zuerst
        oCNpc* self = (oCNpc*)p->GetInstance();
        if (self == player) {
            TpPending = true;
            TpLog("Rune gewirkt - Ziel Mitspieler statt %s", wp.ToChar());
        }
        else
            TpLog("AI_Teleport(%s) beim Wirken der Rune fuer einen anderen NPC - nicht ausgefuehrt", wp.ToChar());
        return 0;
    }

    static void TpInstallExternal() {
        if (TpExtInstalled || !parser)
            return;
        TpExtInstalled = true;
        zCPar_Symbol* s = parser->GetSymbol(zSTRING("AI_TELEPORT"));
        if (!s || s->type != zPAR_TYPE_FUNC || !(s->flags & zPAR_FLAG_EXTERNAL) || !s->single_intdata) {
            OthelloCompatLog("Teleport-Rune: External AI_TELEPORT nicht gefunden - Rune teleportiert wie Teleport5");
            return;
        }
        if ((CoopTpExternal)s->single_intdata == &CoopExtAiTeleport)
            return;
        TpOrigAiTeleport = (CoopTpExternal)s->single_intdata;
        s->single_intdata = (int)&CoopExtAiTeleport;
        OthelloCompatLog("Teleport-Rune: External AI_TELEPORT umgeleitet (Original %p)", TpOrigAiTeleport);
    }

    static bool TpInWorld(oCNpc* n) {
        return n && n != player && n->GetHomeWorld() && n->vobLeafList.GetNum() > 0 && n->GetModel();
    }

    // Ziel: anvisierte Coop-Figur, sonst (beim Mitspieler) die Figur des Hosts, sonst die erste nach Name
    static oCNpc* TpPartner() {
        if (!ServerThread && !ClientThread)
            return NULL;
        oCNpc* focus = player ? player->GetFocusNpc() : NULL;
        if (focus && PlayerNpcs.count(focus) && TpInWorld(focus))
            return focus;
        if (ClientThread && !ServerThread) {
            auto h = PlayerNameToNpc.find(string("HOST"));
            if (h != PlayerNameToNpc.end() && TpInWorld(h->second))
                return h->second;
        }
        for (auto& kv : PlayerNameToNpc)
            if (TpInWorld(kv.second))
                return kv.second;
        return NULL;
    }

    static void TpDo() {
        oCNpc* t = TpPartner();
        if (!t) {
            ChatLog(CoopTr("Teleport: kein Mitspieler da.", "Teleport: no partner here."), zCOLOR(255, 200, 80, 255));
            TpLog("kein Mitspieler in der Welt (%s) - Lage unveraendert", (ServerThread || ClientThread) ? "verbunden" : "ohne Coop");
            return;
        }
        const zVEC3 from = player->GetPositionWorld();
        const zVEC3 at = t->GetPositionWorld();
        std::string how;
        PendingMoveReset = true;   // Bewegung im naechsten Bild zuruecksetzen (wie nach dem Laden, CoopSaveSync.cpp)
        bool ok = CoopPlaceBesideHost(at, t->GetAtVectorWorld(), how, "den Mitspieler");
        if (!ok) {
            zVEC3 p = at;
            if (player->SearchNpcPosition(p)) {
                player->SetCollDet(FALSE);
                player->SetPositionWorld(p);
                player->SetCollDet(TRUE);
                char b[160];
                _snprintf(b, sizeof(b), "direkt beim Mitspieler (%s; %.0f/%.0f/%.0f)", how.c_str(), p.n[0], p.n[1], p.n[2]);
                b[sizeof(b) - 1] = 0;
                how = b;
                ok = true;
            }
        }
        if (!ok) {
            PendingMoveReset = false;
            ChatLog(CoopTr("Teleport: beim Mitspieler ist kein Platz.", "Teleport: no room next to your partner."), zCOLOR(255, 200, 80, 255));
        }
        TpLog("zu %s (%.1f m entfernt): %s", PlayerNpcs.count(t) ? PlayerNpcs[t].ToChar() : "?", (at - from).LengthApprox() / 100.f, how.c_str());
    }

    // --- v106: eigenes Aussehen (Nutzer: "Ersetze die Textur fuer die Rune mit diesem Bild", gt-fix\assets\coop-rune.jpg) ---
    // ItRn_TP04.3ds nutzt Sammeltexturen vieler Gegenstaende (ITMI_GEMS, RUBY_GOLD), alle G1-Runen die gemeinsame ITARRUNES -
    // eine eigene Optik braucht ein eigenes Modell. tools\runetex.py baut aus Gothics ITAR_RUNE_17.MRM (flacher Stein: Quadrat
    // oben/unten, vier Spitzen) und dem Bild das Modell ITRU_COOP_TELEPORT.MRM (Material/Textur COOPRUNES, Texturkoordinaten auf
    // die eigene Textur) und COOPRUNES-C.TEX (DXT3 256 x 128: Quadrat mit "F", Spitzen aus den goldenen Dreiecken des Bilds);
    // die Bytes stehen in CoopRuneAssets.h. GT legt beide beim Start in Gothics Ordner fuer vorkompilierte Modelle/Texturen
    // (zoptions DIR_COMPILED_MESHES / DIR_COMPILED_TEXTURES, nur wenn sie fehlen oder abweichen - wie der Kartenpfeil v105, den
    // Gothic aus system\ laedt), laedt das Modell zur Probe (zCVisual::LoadVisual 0x005DA1F0) und setzt erst dann die Konstante
    // COOP_RUNE_VISUAL aus COOP_ITEMS.D (v106; dort vorbelegt "ItRn_TP04.3ds") auf das eigene Modell - die Instanz liest sie bei
    // jedem Anlegen. Schon vorhandene Runen im eigenen Inventar bekommen das Modell nach dem Laden (TpEnsureRune). Geht etwas
    // schief, bleibt Othellos Aussehen. Zeitpunkt: Spielstart, jedes Menuebild bis erledigt, vor jedem Laden.
#include "CoopRuneAssets.h"

    static int TpLookState = 0;   // 0 offen, 1 eigenes Modell aktiv, 2 aus (Othellos Aussehen)
    static zCVisual* TpLookVisual = NULL;
    static const char* TP_LOOK_VISUAL = "ITRU_COOP_TELEPORT.3DS";

    static bool TpLookWrite(int dirKind, const char* name, const unsigned char* data, size_t n, std::string& where) {
        if (!zoptions)
            return false;
        std::string rel = zoptions->GetDirString((zTOptionPaths)dirKind).ToChar();
        if (rel.empty())
            return false;
        if (rel[0] != '\\' && rel[0] != '/')
            rel = "\\" + rel;
        if (rel[rel.size() - 1] != '\\' && rel[rel.size() - 1] != '/')
            rel += "\\";
        const std::string dir = CoopGameRoot() + rel;
        for (size_t p = CoopGameRoot().size() + 1; p < dir.size(); p++)   // Ordner der Reihe nach anlegen
            if (dir[p] == '\\' || dir[p] == '/')
                CreateDirectoryA(dir.substr(0, p).c_str(), NULL);
        where = dir + name;
        if (FILE* f = fopen(where.c_str(), "rb")) {
            std::string have;
            char buf[4096];
            size_t k;
            while ((k = fread(buf, 1, sizeof(buf), f)) > 0 && have.size() <= n)
                have.append(buf, k);
            fclose(f);
            if (have.size() == n && memcmp(have.data(), data, n) == 0)
                return true;
        }
        FILE* f = fopen(where.c_str(), "wb");
        if (!f)
            return false;
        const bool ok = fwrite(data, 1, n, f) == n;
        fclose(f);
        return ok;
    }

    void CoopRuneLookInit() {
        if (TpLookState || !parser || !zoptions || !zrenderer)
            return;
        zCPar_Symbol* sym = parser->GetSymbol(zSTRING("COOP_RUNE_VISUAL"));
        if (!sym || sym->type != zPAR_TYPE_STRING || !sym->stringdata) {
            TpLookState = 2;
            OthelloCompatLog("Teleport-Rune: COOP_RUNE_VISUAL fehlt (COOP_ITEMS.D vor v106) - Rune behaelt Othellos Aussehen");
            return;
        }
        std::string mesh, tex;
        if (!TpLookWrite(DIR_COMPILED_TEXTURES, "COOPRUNES-C.TEX", CoopRuneTex, sizeof(CoopRuneTex), tex) ||
            !TpLookWrite(DIR_COMPILED_MESHES, "ITRU_COOP_TELEPORT.MRM", CoopRuneMrm, sizeof(CoopRuneMrm), mesh)) {
            TpLookState = 2;
            OthelloCompatLog("Teleport-Rune: Modell/Textur nicht schreibbar (%s, %s) - Othellos Aussehen", mesh.c_str(), tex.c_str());
            return;
        }
        TpLookVisual = zCVisual::LoadVisual(zSTRING(TP_LOOK_VISUAL));
        if (!TpLookVisual) {
            TpLookState = 2;
            OthelloCompatLog("Teleport-Rune: %s nicht ladbar (%s) - Othellos Aussehen", TP_LOOK_VISUAL, mesh.c_str());
            return;
        }
        sym->SetValue(zSTRING(TP_LOOK_VISUAL), 0);
        TpLookState = 1;
        OthelloCompatLog("Teleport-Rune: eigenes Aussehen aktiv (%s, %s)", mesh.c_str(), tex.c_str());
    }

    static void TpEnsureRune(const char* why) {
        if (!player || player->IsDead() || TpRune() < 0)
            return;
        if (oCItem* have = player->IsInInv(TpRuneIdx, 1)) {
            if (TpLookState == 1 && TpLookVisual && have->GetVisual() != TpLookVisual) {
                have->SetVisual(TpLookVisual);
                TpLog("%s: Rune im Inventar bekommt das eigene Aussehen", why);
            }
            return;
        }
        WorldSyncApplying++;
        oCItem* it = CreateCoopItem(TpRuneIdx);
        oCItem* in = it ? player->PutInInv(it) : NULL;
        WorldSyncApplying--;
        TpLog("%s: keine Rune im Inventar - %s", why, in ? "eine angelegt" : "Anlegen FEHLGESCHLAGEN");
    }

    // Plugin.cpp, LoadEnd: nach jedem Laden pruefen (nach dem Wiederherstellen des eigenen Helden)
    void CoopTeleportLoadEnd() {
        const long long now = GetCurrentMs();
        TpEnsureAt1 = now + 3000;
        TpEnsureAt2 = now + 15000;
        TpPending = false;
        TpCasting = false;
    }

    // Plugin.cpp, Game_Loop
    void CoopTeleportPulse() {
        if (!player || IsLoadingLevel)
            return;
        TpInstallExternal();
        if (TpPending) {
            TpPending = false;
            TpDo();
        }
        const long long now = GetCurrentMs();
        if (TpEnsureAt1 && now >= TpEnsureAt1) {
            TpEnsureAt1 = 0;
            TpEnsureRune("nach dem Laden");
        }
        if (TpEnsureAt2 && now >= TpEnsureAt2) {
            TpEnsureAt2 = 0;
            TpEnsureRune("15 s nach dem Laden");
        }
    }

#else
    void CoopTeleportLoadEnd() {}
    void CoopTeleportPulse() {}
    void CoopRuneLookInit() {}
#endif
}
