// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

    // --- v67: Schwierigkeitsgrad im Coop (Einstellungen -> COOP -> Schwierigkeitsgrad) ----------------------------------------
    //
    // Nutzer (20.10.2026): "Schwierigkeitsgrad-Einstellungen ... Schaden der Spieler, Schaden der Gegner, Leben der Gegner,
    // Kampf-EP teilen" - "Stufen + Einzelwerte"; der Host gibt vor (Regel "Host fuehrt"). Spawnrate folgt spaeter.
    // v68 (Test v67: "EP nicht ganz korrekt - ich haette gerne zwei Schalter, einmal EP allgemein z. B. 50-100 % und dann EP-Share
    // nochmal 50-100 %"): statt der drei EP-Regeln (beide voll / Haelfte / nur wer toetet) zwei Prozentwerte.
    //
    // Gothic.ini [GOTHICCOOP] (Gothic schreibt die Menue-Wahl selbst, GT belegt vor):
    //   coopDiffPreset 0 Leicht | 1 Normal | 2 Schwer | 3 Sehr schwer | 4 Eigene
    //   coopDmgPlayers 0-5 = 50/75/100/125/150/200 %   Schaden der Spieler an NPCs
    //   coopDmgEnemies 0-6 = 50/75/100/125/150/200/300 %   Schaden von NPCs an den Spielern
    //   coopEnemyHp    0-6 = 50/75/100/125/150/200/300 %   "Leben der Gegner": Spieler-Schaden an NPCs geteilt durch diesen Wert
    //                  (die HP selbst bleiben - nichts davon landet im Spielstand)
    //   coopXpAll      0-5 = 50/60/70/80/90/100 %   v68: "EP allgemein" - alle Kampf-EP (eigene Kills/K.o.)
    //   coopXpPartner  0-6 = 0/50/60/70/80/90/100 %   v68: "EP-Anteil Partner" - EP aus Kills/K.o. des Partners, bezogen auf
    //                  die eigenen (EP allgemein); 0 = nur wer toetet
    //   coopXpRange    0-3 = 20/45/100 m/ueberall: Kill des Partners gibt nur in dieser Entfernung EP (bisher fest 45 m)
    //   (v67 coopXpShare 0/1/2 wird einmal uebernommen: 1 Haelfte -> allgemein 50 %, 2 nur wer toetet -> Partner 0 %)
    // Eine Stufe setzt die drei Kampfwerte (die EP-Regeln nicht); aendert man einen Kampfwert, springt die Stufe auf "Eigene".
    //
    // Wirkung nur, solange Coop laeuft (Host oder Client); jeder PC rechnet mit den Werten des Hosts (Paket COOP_RULES, alle 5 s
    // und bei Aenderung):
    //   - Schaden: Haken oCNpc::OnDamage_Hit (Hooks.cpp) skaliert den HP-Abzug ueber den ChangeAttribute-Haken (CoopDamage.cpp) -
    //     Tod/Bewusstlos entscheidet die Engine wie sonst. Treffer des eigenen Helden rechnet der eigene PC (Client meldet den
    //     Schaden schon skaliert), Treffer am eigenen Helden ebenso (C11).
    //   - Kampf-EP: Othellos Kampf-EP kommen nur aus B_DeathXP und B_UnconciousXP, beide mit der Skript-Konstante
    //     XP_PER_LEVEL_DEAD (10 x Stufe des Opfers; Othello-DAT geprueft, Quest-EP nutzen sie nicht). GT setzt sie dauerhaft auf
    //     "EP allgemein"; waehrend GT einen Kill/K.o. des Partners nachspielt (RemoteNpc.cpp - ZS_Dead/ZS_Unconscious laufen dabei
    //     sofort, siehe CoopReplayDeathDepth) auf allgemein x Partner-Anteil, danach zurueck.
    //   - Partner 0 % / Reichweite: Kill des Partners ohne EP wie bisher ausserhalb der Reichweite (CoopKillNoXp,
    //     RemoteNpc.cpp; CoopFarFromMe nimmt die Reichweite).

    // --- v75: Abgleich-Schalter (Einstellungen -> COOP -> Abgleich) ------------------------------------------------------------
    // Nutzer: "Quests und Tagebuch haben nur einen gemeinsamen Schalter (coopStorySync), keine zwei getrennten. Spielstaende: man
    // kann nur die Auto-Sicherung und das Uebernehmen des Host-Stands abschalten, einen Schalter fuer den ganzen Abgleich gibt es
    // nicht. Gegenstaende und Truhen: gar kein Schalter. Alle stehen nur in GothicCoopConfig.json, nicht im Menue - ich moechte,
    // dass die Schalter in den Einstellungen konfigurierbar sind." Wie der Schwierigkeitsgrad: Gothic.ini [GOTHICCOOP], der Host gibt
    // vor (im selben Paket COOP_RULES). Die json-Werte (coopStorySync, coopAutoSaveMinutes, coopTakeHostStateOnConnect) belegen die
    // Menue-Werte nur beim ersten Start vor.
    //   coopSyncQuests  0 an | 1 aus   Quest-Fortschritt: Skript-Variablen, erzaehlte Dialoge, NPC-Zustaende, Gildenhaltungen, Quest-EP
    //   coopSyncLog     0 an | 1 aus   Tagebuch (Themen, Eintraege, Meldung "Neuer Tagebucheintrag")
    //   coopSyncWorld   0 an | 1 aus   Gegenstaende am Boden und Truhen (CoopWorldSync.cpp)
    //   coopSyncSaves   0 gemeinsam | 1 getrennt   gemeinsames Speichern/Laden, Uebernahme, Archiv, Dateiuebertragung (CoopSaveSync.cpp)
    //   coopAutoSave    0 aus | 1-5 = 5/10/15/20/30 min   gemeinsame Auto-Sicherung des Hosts
    //   coopConnectTake 0 Host-Stand uebernehmen | 1 eigenen Stand behalten   beim Verbinden im Spiel
    bool CoopRuleQuests();
    bool CoopRuleLog();
    bool CoopRuleWorld();
    bool CoopRuleSaves();
    int CoopRuleAutoSaveMinutes();
    bool CoopRuleTakeOnConnect();
    void CoopRulesSendSoon();

    float CoopDiffPlayerFactor();
    float CoopDiffEnemyFactor();
    bool CoopDiffXpNoShare();
    void CoopDiffPartnerXpBegin();
    void CoopDiffPartnerXpEnd();
    float CoopDiffXpRangeCm();
    int CoopDiffHpBase(oCNpc* n);
    void CoopDiffHpFromHost(oCNpc* n, int hpMax, int base);
    void CoopDiffHpSaveBegin();
    void CoopDiffHpSaveEnd();
    void CoopDiffHpWorldGone();
    void CoopDiffDefaults();
    void CoopDiffPulse();
    void CoopDiffMenuPulse();
    void CoopDiffReceive(json& e, const std::string& id);

#if ENGINE == Engine_G1

    static const int DiffPlayerPct[] = { 50, 75, 100, 125, 150, 200 };
    static const int DiffEnemyPct[] = { 50, 75, 100, 125, 150, 200, 300 };
    static const int DiffXpAllPct[] = { 50, 60, 70, 80, 90, 100 };
    static const int DiffXpPartnerPct[] = { 0, 50, 60, 70, 80, 90, 100 };
    static const char* DiffPresetName[] = { "Leicht", "Normal", "Schwer", "Sehr schwer", "Eigene" };
    static const char* DiffRangeName[] = { "20 m", "45 m", "100 m", COOP_UE "berall" };
    static const float DiffRangeCm[] = { 2000.f, 4500.f, 10000.f, -1.f };
    // Stufen: Indizes in DiffPlayerPct / DiffEnemyPct (Schaden Spieler, Schaden Gegner, Leben Gegner)
    static const int DiffPresetIdx[4][3] = { { 3, 1, 1 }, { 2, 2, 2 }, { 2, 4, 4 }, { 2, 5, 5 } };

    static const int SyncAutoMin[] = { 0, 5, 10, 15, 20, 30 };   // v75: coopAutoSave

    struct DiffRules {
        int preset = 1, dp = 2, de = 2, hp = 2, xa = 5, xp = 6, xr = 1;   // Indizes
        int sq = 0, sl = 0, sw = 0, ss = 0, sa = 2, st = 0;              // v75: Abgleich (0 = an / gemeinsam / uebernehmen)
        bool SameDifficulty(const DiffRules& o) const {
            return preset == o.preset && dp == o.dp && de == o.de && hp == o.hp && xa == o.xa && xp == o.xp && xr == o.xr;
        }
        bool SameSync(const DiffRules& o) const {
            return sq == o.sq && sl == o.sl && sw == o.sw && ss == o.ss && sa == o.sa && st == o.st;
        }
        bool operator==(const DiffRules& o) const { return SameDifficulty(o) && SameSync(o); }
        bool operator!=(const DiffRules& o) const { return !(*this == o); }
    };

    static DiffRules DiffOwn;                 // aus Gothic.ini (je Sekunde)
    static bool DiffOwnRead = false;
    static DiffRules DiffHost;                // Client: vom Host (Netz-Faden schreibt, Spiel-Faden liest)
    static volatile bool DiffHostValid = false;
    static volatile bool DiffHostChanged = false;
    static DiffRules DiffLastEff;
    static bool DiffLastEffValid = false;
    static long long DiffNextReadMs = 0, DiffNextSendMs = 0;
    static int DiffXpOrig = -1;               // XP_PER_LEVEL_DEAD aus dem Skript (Othello/G1: 10)
    static int DiffXpOwnPct = 100;            // zuletzt gesetzter Anteil (EP allgemein)
    static int DiffPartnerDepth = 0;

    static void DiffLog(const char* fmt, ...) {
        char buf[600];
        va_list ap;
        va_start(ap, fmt);
        _vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        buf[sizeof(buf) - 1] = 0;
        OthelloCompatLog("[%lld s, %s] Schwierigkeit: %s", DiagSeconds(), DiagRole(), buf);
    }

    static int DiffClamp(int v, int n, int def) {
        return v >= 0 && v < n ? v : def;
    }

    // Kampfwerte der Stufe (ausser "Eigene")
    static DiffRules DiffNormalized(DiffRules r) {
        r.preset = DiffClamp(r.preset, 5, 1);
        r.dp = DiffClamp(r.dp, 6, 2);
        r.de = DiffClamp(r.de, 7, 2);
        r.hp = DiffClamp(r.hp, 7, 2);
        r.xa = DiffClamp(r.xa, 6, 5);
        r.xp = DiffClamp(r.xp, 7, 6);
        r.xr = DiffClamp(r.xr, 4, 1);
        r.sq = DiffClamp(r.sq, 2, 0);   // v75
        r.sl = DiffClamp(r.sl, 2, 0);
        r.sw = DiffClamp(r.sw, 2, 0);
        r.ss = DiffClamp(r.ss, 2, 0);
        r.sa = DiffClamp(r.sa, 6, 2);
        r.st = DiffClamp(r.st, 2, 0);
        if (r.preset < 4) {
            r.dp = DiffPresetIdx[r.preset][0];
            r.de = DiffPresetIdx[r.preset][1];
            r.hp = DiffPresetIdx[r.preset][2];
        }
        return r;
    }

    void CoopDiffDefaults() {
        if (!zoptions)
            return;
        const zSTRING sec("GOTHICCOOP");
        if (!zoptions->EntryExists(sec, "coopDiffPreset")) zoptions->WriteInt(sec, "coopDiffPreset", 1, 0);
        if (!zoptions->EntryExists(sec, "coopDmgPlayers")) zoptions->WriteInt(sec, "coopDmgPlayers", 2, 0);
        if (!zoptions->EntryExists(sec, "coopDmgEnemies")) zoptions->WriteInt(sec, "coopDmgEnemies", 2, 0);
        if (!zoptions->EntryExists(sec, "coopEnemyHp")) zoptions->WriteInt(sec, "coopEnemyHp", 2, 0);
        if (!zoptions->EntryExists(sec, "coopXpAll") || !zoptions->EntryExists(sec, "coopXpPartner")) {
            // v68: einmal aus der v67-Regel uebernehmen
            const int old = zoptions->EntryExists(sec, "coopXpShare") ? zoptions->ReadInt(sec, "coopXpShare", 0) : 0;
            if (!zoptions->EntryExists(sec, "coopXpAll")) zoptions->WriteInt(sec, "coopXpAll", old == 1 ? 0 : 5, 0);
            if (!zoptions->EntryExists(sec, "coopXpPartner")) zoptions->WriteInt(sec, "coopXpPartner", old == 2 ? 0 : 6, 0);
        }
        if (!zoptions->EntryExists(sec, "coopXpRange")) zoptions->WriteInt(sec, "coopXpRange", 1, 0);
        // v75: Abgleich - beim ersten Start aus GothicCoopConfig.json vorbelegt (danach gilt das Menue)
        auto jsonBool = [](const char* k, bool def) {
            return CoopConfig.contains(k) && CoopConfig[k].is_boolean() ? CoopConfig[k].get<bool>() : def;
        };
        const bool story = jsonBool("coopStorySync", true);
        if (!zoptions->EntryExists(sec, "coopSyncQuests")) zoptions->WriteInt(sec, "coopSyncQuests", story ? 0 : 1, 0);
        if (!zoptions->EntryExists(sec, "coopSyncLog")) zoptions->WriteInt(sec, "coopSyncLog", story ? 0 : 1, 0);
        if (!zoptions->EntryExists(sec, "coopSyncWorld")) zoptions->WriteInt(sec, "coopSyncWorld", 0, 0);
        if (!zoptions->EntryExists(sec, "coopSyncSaves")) zoptions->WriteInt(sec, "coopSyncSaves", 0, 0);
        if (!zoptions->EntryExists(sec, "coopAutoSave")) {
            int m = 10;
            if (CoopConfig.contains("coopAutoSaveMinutes") && CoopConfig["coopAutoSaveMinutes"].is_number())
                m = CoopConfig["coopAutoSaveMinutes"].get<int>();
            const int idx = m <= 0 ? 0 : m <= 5 ? 1 : m <= 10 ? 2 : m <= 15 ? 3 : m <= 20 ? 4 : 5;
            zoptions->WriteInt(sec, "coopAutoSave", idx, 0);
        }
        if (!zoptions->EntryExists(sec, "coopConnectTake"))
            zoptions->WriteInt(sec, "coopConnectTake", jsonBool("coopTakeHostStateOnConnect", true) ? 0 : 1, 0);
    }

    // roh aus Gothic.ini (ohne Stufe anzuwenden) - fuer das Menue
    static DiffRules DiffReadIniRaw() {
        DiffRules r;
        if (!zoptions)
            return r;
        CoopDiffDefaults();
        const zSTRING sec("GOTHICCOOP");
        r.preset = zoptions->ReadInt(sec, "coopDiffPreset", 1);
        r.dp = zoptions->ReadInt(sec, "coopDmgPlayers", 2);
        r.de = zoptions->ReadInt(sec, "coopDmgEnemies", 2);
        r.hp = zoptions->ReadInt(sec, "coopEnemyHp", 2);
        r.xa = zoptions->ReadInt(sec, "coopXpAll", 5);
        r.xp = zoptions->ReadInt(sec, "coopXpPartner", 6);
        r.xr = zoptions->ReadInt(sec, "coopXpRange", 1);
        r.sq = zoptions->ReadInt(sec, "coopSyncQuests", 0);   // v75
        r.sl = zoptions->ReadInt(sec, "coopSyncLog", 0);
        r.sw = zoptions->ReadInt(sec, "coopSyncWorld", 0);
        r.ss = zoptions->ReadInt(sec, "coopSyncSaves", 0);
        r.sa = zoptions->ReadInt(sec, "coopAutoSave", 2);
        r.st = zoptions->ReadInt(sec, "coopConnectTake", 0);
        return r;
    }

    static void DiffReadOwn(bool force) {
        const long long now = GetCurrentMs();
        if (DiffOwnRead && !force && now < DiffNextReadMs)
            return;
        DiffNextReadMs = now + 1000;
        DiffOwn = DiffNormalized(DiffReadIniRaw());
        DiffOwnRead = true;
    }

    static bool DiffCoopOn() {
        return ServerThread || ClientThread;
    }

    static const DiffRules& DiffEff() {
        if (ClientThread && !ServerThread && DiffHostValid)
            return DiffHost;
        if (!DiffOwnRead)
            DiffReadOwn(true);
        return DiffOwn;
    }

    // v95: en = englisch (Anzeige bei coopLang=1); das Log bekommt immer Deutsch
    static const char* DiffPresetNameEn[] = { "Easy", "Normal", "Hard", "Very hard", "Custom" };

    static std::string DiffDescribe(const DiffRules& r, bool en = false) {
        char b[300];
        if (en)
            _snprintf(b, sizeof(b), "%s - player damage %d %%, enemy damage %d %%, enemy health %d %%, XP %d %%, partner XP %d %% (within %s)",
                DiffPresetNameEn[r.preset], DiffPlayerPct[r.dp], DiffEnemyPct[r.de], DiffEnemyPct[r.hp], DiffXpAllPct[r.xa],
                DiffXpPartnerPct[r.xp], r.xr == 3 ? "any range" : DiffRangeName[r.xr]);
        else
            _snprintf(b, sizeof(b), "%s - Schaden Spieler %d %%, Gegner %d %%, Leben der Gegner %d %%, EP allgemein %d %%, "
                "EP-Anteil Partner %d %% (bis %s)", DiffPresetName[r.preset], DiffPlayerPct[r.dp], DiffEnemyPct[r.de], DiffEnemyPct[r.hp],
                DiffXpAllPct[r.xa], DiffXpPartnerPct[r.xp], DiffRangeName[r.xr]);
        b[sizeof(b) - 1] = 0;
        return b;
    }

    // v75: Abgleich-Regeln (im Coop die des Hosts)
    static std::string SyncDescribe(const DiffRules& r, bool en = false) {
        char b[300];
        const std::string autoMin = std::to_string(SyncAutoMin[r.sa]) + " min";
        if (en)
            _snprintf(b, sizeof(b), "quests %s, diary %s, items/chests %s, saves %s, autosave %s, on connect %s", r.sq ? "off" : "on",
                r.sl ? "off" : "on", r.sw ? "off" : "on", r.ss ? "separate" : "shared", r.sa ? autoMin.c_str() : "off",
                r.st ? "own state" : "host state");
        else
            _snprintf(b, sizeof(b), "Quests %s, Tagebuch %s, Gegenst" COOP_AE "nde/Truhen %s, Spielst" COOP_AE "nde %s, Auto-Sicherung %s, "
                "beim Verbinden %s", r.sq ? "aus" : "an", r.sl ? "aus" : "an", r.sw ? "aus" : "an", r.ss ? "getrennt" : "gemeinsam",
                r.sa ? autoMin.c_str() : "aus", r.st ? "eigener Stand" : "Host-Stand");
        b[sizeof(b) - 1] = 0;
        return b;
    }

    bool CoopRuleQuests() { return DiffEff().sq == 0; }
    bool CoopRuleLog() { return DiffEff().sl == 0; }
    bool CoopRuleWorld() { return DiffEff().sw == 0; }
    bool CoopRuleSaves() { return DiffEff().ss == 0; }
    int CoopRuleAutoSaveMinutes() { return SyncAutoMin[DiffEff().sa]; }
    bool CoopRuleTakeOnConnect() { return DiffEff().st == 0; }
    void CoopRulesSendSoon() { DiffNextSendMs = 0; }   // Host: neuer Mitspieler - Regeln gleich schicken

    // Faktor fuer den Schaden des eigenen Helden an einem NPC (Schaden der Spieler). v70: "Leben der Gegner" sind echte
    // Lebenspunkte (DiffHp... unten) - nicht mehr ueber den Schaden gerechnet.
    float CoopDiffPlayerFactor() {
        if (!DiffCoopOn())
            return 1.f;
        return DiffPlayerPct[DiffEff().dp] / 100.f;
    }

    // Faktor fuer den Schaden eines NPCs am eigenen Helden
    float CoopDiffEnemyFactor() {
        if (!DiffCoopOn())
            return 1.f;
        return DiffEnemyPct[DiffEff().de] / 100.f;
    }

    // EP-Anteil Partner 0 %: Kills/K.o. des Partners ohne EP (wie ausserhalb der Reichweite)
    bool CoopDiffXpNoShare() {
        return DiffCoopOn() && DiffXpPartnerPct[DiffEff().xp] == 0;
    }

    // Reichweite fuer EP aus Kills des Partners in cm, < 0 = ueberall
    float CoopDiffXpRangeCm() {
        return DiffCoopOn() ? DiffRangeCm[DiffEff().xr] : (float)BROADCAST_DISTANCE;
    }

    // Kampf-EP: XP_PER_LEVEL_DEAD (wird in B_DeathXP/B_UnconciousXP per PUSHVAR gelesen) auf pct % des Skriptwerts (gerundet;
    // EP = Wert x Stufe des Opfers - bei 10 also 10-%-Schritte genau, allgemein x Partner auf 10 % gerundet)
    static void DiffSetXpConst(int pct, const char* why) {
        if (!parser)
            return;
        zCPar_Symbol* s = parser->GetSymbol("XP_PER_LEVEL_DEAD");
        if (!s)
            return;
        int cur = 0;
        s->GetValue(cur, 0);
        if (DiffXpOrig < 0) {
            DiffXpOrig = cur;
            DiffLog("Skript-Konstante XP_PER_LEVEL_DEAD = %d (Kampf-EP = Wert x Stufe des Opfers)", cur);
        }
        const int want = (DiffXpOrig * pct + 50) / 100;
        if (cur != want) {
            s->SetValue(want, 0);
            static CoopDiagBudget budget = { 0, 0 };
            if (!why || CoopDiagAllow(budget, 20))
                DiffLog("Kampf-EP: XP_PER_LEVEL_DEAD %d -> %d (%d %%%s%s)", cur, want, pct, why ? ", " : "", why ? why : "");
        }
    }

    // Kill/K.o. des Partners wird nachgespielt (RemoteNpc.cpp): Kampf-EP = allgemein x Partner-Anteil
    void CoopDiffPartnerXpBegin() {
        if (!DiffCoopOn() || DiffPartnerDepth++ > 0)
            return;
        const DiffRules& r = DiffEff();
        DiffSetXpConst(DiffXpAllPct[r.xa] * DiffXpPartnerPct[r.xp] / 100, "Kill des Partners");
    }

    void CoopDiffPartnerXpEnd() {
        if (DiffPartnerDepth <= 0)
            return;
        if (--DiffPartnerDepth == 0)
            DiffSetXpConst(DiffXpOwnPct, "zurueck");
    }

    // --- v70: Leben der Gegner als echte Lebenspunkte ------------------------------------------------------------------------
    // Test v69: "das Leben der Gegner wird z. B. nicht angepasst nach Einstellung, die haben 800" - bis v69 teilte GT nur den
    // Schaden der Spieler. Jetzt setzt der Host HP und HP-Maximum der NPCs (nicht Held/Figuren/Begleiter) in Welt und
    // Spawn-Liste auf Grundwert x Faktor und merkt sich den Grundwert (DiffHp). Aendert ein Skript das Maximum, gilt das als neuer
    // Grundwert. Der Client uebernimmt die Werte mit den HP-Paketen des Hosts, die den Grundwert mitbringen ("hb").
    // Nichts davon darf in einen Spielstand: Game_SaveBegin setzt alle gemerkten NPCs auf den Grundwert zurueck, Game_SaveEnd
    // wieder auf den Faktor. Ohne Coop (Faktor 1, Client getrennt) gehen alle auf den Grundwert, beim Laden wird die Liste
    // verworfen (die NPCs kommen mit Grundwerten aus dem Spielstand).
    struct DiffHpEntry { int base; float f; };
    static std::map<oCNpc*, DiffHpEntry> DiffHp;
    static std::vector<std::pair<oCNpc*, float>> DiffHpSaved;
    static long long DiffHpNextMs = 0;

    static int DiffRoundI(float v) {
        return (int)floorf(v + 0.5f);
    }

    static bool DiffHpEligible(oCNpc* n) {
        if (!n || n == player || PlayerNpcs.count(n))
            return false;
        if (CoopLeadIsRemote(n))
            return false;   // v72: fuehrt gerade ein Client im Kampf - HP kommen von dort (schon skaliert)
        static int pm = -2, mm = -2;
        if (pm == -2) {
            pm = GetPartyMemberID();
            zCPar_Symbol* s = parser ? parser->GetSymbol("AIV_MM_PARTYMEMBER") : NULL;
            mm = 23;
            if (s)
                s->GetValue(mm, 0);
        }
        if ((pm >= 0 && pm < 50 && n->aiscriptvars[pm]) || (mm >= 0 && mm < 50 && n->aiscriptvars[mm]))
            return false;   // Begleiter/Beschwoerungen der Spieler
        return true;
    }

    // HP-Maximum = Grundwert x f, HP im selben Verhaeltnis (1 und 0 bleiben)
    static void DiffHpSet(oCNpc* n, DiffHpEntry& e, float f) {
        if (fabsf(e.f - f) < 0.001f)
            return;
        const int hp = n->attribute[NPC_ATR_HITPOINTS];
        const int newMax = (std::max)(1, DiffRoundI(e.base * f));
        int newHp = hp;
        if (hp > 1)
            newHp = (std::min)(newMax, (std::max)(1, DiffRoundI(hp * f / e.f)));
        n->SetAttribute(NPC_ATR_HITPOINTSMAX, newMax);
        n->SetAttribute(NPC_ATR_HITPOINTS, newHp);
        e.f = f;
    }

    static void DiffHpCollect(std::set<oCNpc*>& all, std::vector<oCNpc*>& world) {
        oCWorld* w = ogame ? ogame->GetGameWorld() : NULL;
        for (auto* l = w ? CoopWorldNpcListFirst(w) : NULL; l; l = l->next)
            if (l->data) {
                all.insert(l->data);
                world.push_back(l->data);
            }
        oCSpawnManager* sm = ogame ? ogame->spawnman : NULL;
        if (sm)
            for (int k = 0; k < sm->spawnList.GetNum(); k++)
                if (oCSpawnManager::oSSpawnNode* node = sm->spawnList[k])
                    if (node->npc)
                        all.insert(node->npc);
    }

    // Host: je Sekunde alle NPCs auf den Faktor bringen (target 1 = zurueck auf den Grundwert)
    static void DiffHpHostPulse(float target) {
        const bool unity = fabsf(target - 1.f) < 0.001f;
        if (unity && DiffHp.empty())
            return;
        std::set<oCNpc*> all;
        std::vector<oCNpc*> world;
        DiffHpCollect(all, world);
        for (auto it = DiffHp.begin(); it != DiffHp.end();)
            it = all.count(it->first) ? std::next(it) : DiffHp.erase(it);   // geloeschte NPCs vergessen
        int changed = 0, rebased = 0;
        for (oCNpc* n : world) {
            if (!DiffHpEligible(n) || n->IsDead())
                continue;
            const int curMax = n->attribute[NPC_ATR_HITPOINTSMAX];
            if (curMax <= 0)
                continue;
            auto it = DiffHp.find(n);
            if (it != DiffHp.end() && curMax != (std::max)(1, DiffRoundI(it->second.base * it->second.f))) {
                it->second = { curMax, 1.f };   // ein Skript hat das Maximum gesetzt - neuer Grundwert
                rebased++;
            }
            if (it == DiffHp.end()) {
                if (unity)
                    continue;
                it = DiffHp.emplace(n, DiffHpEntry{ curMax, 1.f }).first;
            }
            if (fabsf(it->second.f - target) >= 0.001f) {
                DiffHpSet(n, it->second, target);
                changed++;
            }
            if (unity)
                DiffHp.erase(it);
        }
        if (unity)   // aus der Welt genommene (Spawn-Liste) ebenfalls zurueck
            for (auto it = DiffHp.begin(); it != DiffHp.end();) {
                DiffHpSet(it->first, it->second, 1.f);
                it = DiffHp.erase(it);
                changed++;
            }
        static CoopDiagBudget budget = { 0, 0 };
        if ((changed || rebased) && CoopDiagAllow(budget, 10))
            DiffLog("Leben der Gegner: %d NPCs auf %.0f %% gesetzt%s (%d gemerkt)", changed, target * 100.f,
                rebased ? ", Skript-Aenderungen als Grundwert uebernommen" : "", (int)DiffHp.size());
    }

    // Host (LocalNpc, Paket HP): Grundwert des NPCs, wenn GT ihn skaliert hat, sonst -1
    int CoopDiffHpBase(oCNpc* n) {
        auto it = DiffHp.find(n);
        return it != DiffHp.end() && fabsf(it->second.f - 1.f) >= 0.001f ? it->second.base : -1;
    }

    // Client (RemoteNpc::UpdateHp): Werte vom Host - Grundwert merken, damit vor dem Speichern zurueckgesetzt werden kann
    void CoopDiffHpFromHost(oCNpc* n, int hpMax, int base) {
        if (!n)
            return;
        if (base > 0 && hpMax > 0 && base != hpMax)
            DiffHp[n] = { base, (float)hpMax / (float)base };
        else
            DiffHp.erase(n);
    }

    // vor dem Speichern: alle gemerkten NPCs auf den Grundwert (Host und Client)
    void CoopDiffHpSaveBegin() {
        DiffHpSaved.clear();
        if (DiffHp.empty() || !ogame)
            return;
        std::set<oCNpc*> all;
        std::vector<oCNpc*> world;
        DiffHpCollect(all, world);
        for (auto it = DiffHp.begin(); it != DiffHp.end();) {
            if (!all.count(it->first)) {
                it = DiffHp.erase(it);
                continue;
            }
            const float f = it->second.f;
            if (fabsf(f - 1.f) >= 0.001f) {
                DiffHpSet(it->first, it->second, 1.f);
                DiffHpSaved.push_back({ it->first, f });
            }
            ++it;
        }
        DiffLog("Leben der Gegner: vor dem Speichern %d NPCs auf den Grundwert", (int)DiffHpSaved.size());
    }

    // nach dem Speichern: wieder auf den Faktor
    void CoopDiffHpSaveEnd() {
        for (auto& p : DiffHpSaved) {
            auto it = DiffHp.find(p.first);
            if (it != DiffHp.end())
                DiffHpSet(p.first, it->second, p.second);
        }
        DiffHpSaved.clear();
    }

    // Ladebeginn (Spielstand, neues Spiel, Levelwechsel): erst alle gemerkten NPCs auf den Grundwert - beim Levelwechsel speichert
    // Gothic die verlassene Welt in den Zwischenstand, ohne dass Game_SaveBegin laeuft -, dann die Liste verwerfen (die NPCs
    // der neuen Welt kommen mit Grundwerten)
    void CoopDiffHpWorldGone() {
        if (!DiffHp.empty() && ogame && ogame->GetGameWorld()) {
            CoopDiffHpSaveBegin();
            if (!DiffHpSaved.empty())
                DiffLog("Leben der Gegner: Ladebeginn - %d NPCs auf den Grundwert", (int)DiffHpSaved.size());
        }
        DiffHp.clear();
        DiffHpSaved.clear();
    }

    static void DiffSend() {
        const DiffRules& r = DiffOwn;
        json j;
        j["type"] = SYNC_COOP_RULES;
        j["id"] = "HOST";   // nur der Host sendet; der Client prueft den Absender
        j["p"] = r.preset;
        j["dp"] = r.dp;
        j["de"] = r.de;
        j["hp"] = r.hp;
        j["xa"] = r.xa;
        j["xp"] = r.xp;
        j["xr"] = r.xr;
        j["sq"] = r.sq;   // v75: Abgleich
        j["sl"] = r.sl;
        j["sw"] = r.sw;
        j["ss"] = r.ss;
        j["sa"] = r.sa;
        j["st"] = r.st;
        CoopTraceCount(TRACE_SENT, SYNC_COOP_RULES);
        ReadyToSendJsons.enqueue(j);
    }

    // Netz-Faden (ProcessCoopPacket): Regeln des Hosts
    void CoopDiffReceive(json& e, const std::string& id) {
        CoopTraceCount(TRACE_RECV, SYNC_COOP_RULES);
        if (ServerThread || !ClientThread || id != "HOST")
            return;
        DiffRules r;
        auto geti = [&](const char* k, int def) { return e.contains(k) && e[k].is_number_integer() ? e[k].get<int>() : def; };
        r.preset = geti("p", 1);
        r.dp = geti("dp", 2);
        r.de = geti("de", 2);
        r.hp = geti("hp", 2);
        r.xa = geti("xa", 5);
        r.xp = geti("xp", 6);
        r.xr = geti("xr", 1);
        r.sq = geti("sq", 0);   // v75
        r.sl = geti("sl", 0);
        r.sw = geti("sw", 0);
        r.ss = geti("ss", 0);
        r.sa = geti("sa", 2);
        r.st = geti("st", 0);
        r = DiffNormalized(r);
        if (!DiffHostValid || r != DiffHost) {
            DiffHost = r;
            DiffHostChanged = true;
        }
        DiffHostValid = true;
    }

    // je Bild aus Game_Loop und Game_MenuLoop
    void CoopDiffPulse() {
        if (!zoptions)
            return;
        DiffReadOwn(false);
        if (!ClientThread)
            DiffHostValid = false;
        const long long now = GetCurrentMs();
        const DiffRules eff = DiffEff();
        const bool on = DiffCoopOn();
        // Kampf-EP (EP allgemein) nur im Coop (je Sekunde geprueft; nicht waehrend ein Kill des Partners laeuft)
        static long long nextXpMs = 0;
        if (now >= nextXpMs && DiffPartnerDepth == 0) {
            nextXpMs = now + 1000;
            DiffXpOwnPct = on ? DiffXpAllPct[eff.xa] : 100;
            DiffSetXpConst(DiffXpOwnPct, NULL);
        }
        // v70: Leben der Gegner - der Host skaliert, ohne Coop (auch Client getrennt) zurueck auf den Grundwert
        if (now >= DiffHpNextMs && player && ogame && !IsLoadingLevel && !IsSavingGame) {
            DiffHpNextMs = now + 1000;
            if (ServerThread)
                DiffHpHostPulse(DiffEnemyPct[eff.hp] / 100.f);
            else if (!ClientThread)
                DiffHpHostPulse(1.f);
        }
        static bool lastOn = false;
        const bool changed = !DiffLastEffValid || eff != DiffLastEff || on != lastOn;
        const bool diffChanged = !DiffLastEffValid || !eff.SameDifficulty(DiffLastEff) || on != lastOn;
        const bool syncChanged = !DiffLastEffValid || !eff.SameSync(DiffLastEff) || on != lastOn;
        lastOn = on;
        if (changed || DiffHostChanged) {
            const std::string d = DiffDescribe(eff);
            const std::string s = SyncDescribe(eff);   // v75
            const char* src = ClientThread && !ServerThread ? (DiffHostValid ? "vom Host: " : "eigene (bis die Werte des Hosts kommen): ")
                : ServerThread ? "eigene (Host, gilt fuer alle): " : "eingestellt (ohne Coop wirkungslos): ";
            if (diffChanged || DiffHostChanged)
                DiffLog("%s%s", src, d.c_str());
            if (syncChanged || DiffHostChanged)
                OthelloCompatLog("[%lld s, %s] Abgleich: %s%s", DiagSeconds(), DiagRole(), src, s.c_str());
            if (on && player && DiffLastEffValid) {
                if (diffChanged)
                    ChatLog(string::Combine(CoopTr("Schwierigkeitsgrad%s: %s", "Difficulty%s: %s"), string(ClientThread && !ServerThread ? " (Host)" : ""),
                        string(DiffDescribe(eff, CoopLangEn != 0).c_str())));
                if (syncChanged)
                    ChatLog(string::Combine(CoopTr("Abgleich%s: %s", "Sync%s: %s"), string(ClientThread && !ServerThread ? " (Host)" : ""),
                        string(SyncDescribe(eff, CoopLangEn != 0).c_str())));
            }
            DiffLastEff = eff;
            DiffLastEffValid = true;
            DiffHostChanged = false;
            if (ServerThread)
                DiffNextSendMs = 0;   // gleich senden
        }
        if (ServerThread && now >= DiffNextSendMs) {
            DiffNextSendMs = now + 5000;
            DiffSend();
        }
    }

    // --- Menue MENU_OPT_COOP_DIFF: Stufe <-> Einzelwerte, Hinweiszeile -------------------------------------------------------

    static bool DiffMenuOpen = false;
    static DiffRules DiffMenuLast;
    static long long DiffMenuNoteMs = 0;

    // Auswahlfelder neu aus Gothic.ini lesen und zeichnen (wie LookRefreshChoices, CoopLook.cpp)
    static void DiffRefreshChoices(zCMenu* menu) {
        typedef void(__thiscall* VoidFn)(zCMenuItem*);
        for (int i = 0; i < menu->m_listItems.GetNum(); i++) {
            zCMenuItem* it = menu->m_listItems[i];
            if (!it || *(DWORD*)it != 0x007D2C54)
                continue;
            if (LobbyUpper(it->id).compare(0, 17, "MENUITEM_OPTDIFF_") != 0)
                continue;
            ((VoidFn)0x004D7F70)(it);                          // zCMenuItemChoice::InitMenuItem
            ((VoidFn)(*(DWORD**)it)[0x58 / 4])(it);            // DrawFront
        }
    }

    static void DiffWriteIni(const DiffRules& r) {
        const zSTRING sec("GOTHICCOOP");
        zoptions->WriteInt(sec, "coopDiffPreset", r.preset, 0);
        zoptions->WriteInt(sec, "coopDmgPlayers", r.dp, 0);
        zoptions->WriteInt(sec, "coopDmgEnemies", r.de, 0);
        zoptions->WriteInt(sec, "coopEnemyHp", r.hp, 0);
    }

    // v75: Menue MENU_OPT_COOP_SYNC - Hinweiszeile (wer die Werte vorgibt)
    static void SyncMenuNotePulse(zCMenu* menu) {
        static long long nextMs = 0;
        const long long now = GetCurrentMs();
        if (now < nextMs)
            return;
        nextMs = now + 500;
        DiffReadOwn(true);   // Aenderung im Menue gleich wirksam (Host sendet sie dann)
        zCMenuItem* note = LobbyFindItem(menu, "MENUITEM_OPTCOOPSYNC_NOTE");
        if (!note)
            return;
        // v95: englisch nach coopLang; immer setzen (SetText aendert m_parText[0] nicht - der Vergleich damit liess alte Texte stehen)
        std::string text;
        if (ClientThread && !ServerThread)
            text = DiffHostValid ? CoopTr("Es gilt die Einstellung des Hosts: ", "Host settings apply: ") + SyncDescribe(DiffHost, CoopLangEn != 0)
                : std::string(CoopTr("Verbunden - die Einstellung des Hosts kommt gleich.", "Connected - the host's settings are coming."));
        else if (ServerThread)
            text = CoopTr("Du bist Host - gilt f" COOP_UE "r alle, auch im laufenden Spiel.", "You are host - applies to everyone, also in game.");
        else
            text = CoopTr("Gilt im Coop f" COOP_UE "r alle Spieler, der Host gibt sie vor.", "Applies to all players in coop, set by the host.");
        LobbySetItemText(note, text);
    }

    void CoopDiffMenuPulse() {
        zCMenu* menu = zCMenu::GetActive();
        if (menu && zoptions && LobbyUpper(menu->name) == "MENU_OPT_COOP_SYNC")
            SyncMenuNotePulse(menu);   // v75
        // v91: dieselben Eintraege im Menue "Gemeinsames neues Spiel" (MENU_COOP_NG, CoopMenuLobby.cpp; dort eigene Hinweiszeile)
        if (!menu || !zoptions || (LobbyUpper(menu->name) != "MENU_OPT_COOP_DIFF" && LobbyUpper(menu->name) != "MENU_COOP_NG")) {
            DiffMenuOpen = false;
            return;
        }
        DiffRules cur = DiffReadIniRaw();
        if (!DiffMenuOpen) {
            DiffMenuOpen = true;
            // beim Oeffnen: Kampfwerte einer Stufe ins Menue (falls Gothic.ini abweicht)
            const DiffRules n = DiffNormalized(cur);
            if (n.preset < 4 && (n.dp != cur.dp || n.de != cur.de || n.hp != cur.hp)) {
                DiffWriteIni(n);
                DiffRefreshChoices(menu);
                cur = n;
            }
            DiffMenuLast = cur;
            DiffMenuNoteMs = 0;
        }
        else if (cur != DiffMenuLast) {
            if (cur.preset != DiffMenuLast.preset && cur.preset >= 0 && cur.preset < 4) {
                DiffRules n = DiffNormalized(cur);   // Stufe gewaehlt: ihre Kampfwerte eintragen
                DiffWriteIni(n);
                DiffRefreshChoices(menu);
                cur = n;
                DiffLog("Menue: Stufe %s gewaehlt", DiffPresetName[n.preset]);
            }
            else if (cur.preset >= 0 && cur.preset < 4 && (cur.dp != DiffMenuLast.dp || cur.de != DiffMenuLast.de || cur.hp != DiffMenuLast.hp)) {
                cur.preset = 4;                      // Kampfwert von Hand geaendert: Stufe "Eigene"
                DiffWriteIni(cur);
                DiffRefreshChoices(menu);
                DiffLog("Menue: Kampfwert geaendert - Stufe Eigene");
            }
            DiffMenuLast = cur;
            DiffReadOwn(true);
            DiffMenuNoteMs = 0;
        }
        const long long now = GetCurrentMs();
        if (now < DiffMenuNoteMs)
            return;
        DiffMenuNoteMs = now + 500;
        zCMenuItem* note = LobbyFindItem(menu, "MENUITEM_OPTCOOPDIFF_NOTE");
        if (!note)
            return;
        std::string text;   // v95: wie SyncMenuNotePulse (englisch, immer setzen)
        if (ClientThread && !ServerThread)
            text = DiffHostValid ? CoopTr("Es gilt die Einstellung des Hosts: ", "Host settings apply: ") + DiffDescribe(DiffHost, CoopLangEn != 0)
                : std::string(CoopTr("Verbunden - die Einstellung des Hosts kommt gleich.", "Connected - the host's settings are coming."));
        else if (ServerThread)
            text = CoopTr("Du bist Host - diese Einstellung gilt f" COOP_UE "r alle Spieler.", "You are host - these settings apply to all players.");
        else
            text = CoopTr("Gilt im Coop f" COOP_UE "r alle Spieler, der Host gibt sie vor.", "Applies to all players in coop, set by the host.");
        LobbySetItemText(note, text);
    }

#else
    bool CoopRuleQuests() { return true; }
    bool CoopRuleLog() { return true; }
    bool CoopRuleWorld() { return true; }
    bool CoopRuleSaves() { return true; }
    int CoopRuleAutoSaveMinutes() { return 10; }
    bool CoopRuleTakeOnConnect() { return true; }
    void CoopRulesSendSoon() {}
    float CoopDiffPlayerFactor() { return 1.f; }
    float CoopDiffEnemyFactor() { return 1.f; }
    bool CoopDiffXpNoShare() { return false; }
    void CoopDiffPartnerXpBegin() {}
    void CoopDiffPartnerXpEnd() {}
    float CoopDiffXpRangeCm() { return (float)BROADCAST_DISTANCE; }
    int CoopDiffHpBase(oCNpc*) { return -1; }
    void CoopDiffHpFromHost(oCNpc*, int, int) {}
    void CoopDiffHpSaveBegin() {}
    void CoopDiffHpSaveEnd() {}
    void CoopDiffHpWorldGone() {}
    void CoopDiffDefaults() {}
    void CoopDiffPulse() {}
    void CoopDiffMenuPulse() {}
    void CoopDiffReceive(json&, const std::string&) {}
#endif
}
