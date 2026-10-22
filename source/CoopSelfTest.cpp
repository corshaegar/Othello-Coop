// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

    // --- Coop-Testlauf (v37/v38, Wunsch des Nutzers: "Testset mit 20-30 Funktionen ... breites Testergebnis") -------------
    //
    // Shift + CoopSimKey (Standard F11) startet einen Testlauf mit 28 Schritten, nochmal Shift+F11 bricht ab. Je Schritt:
    //   1. der Partner meldet seinen Ausgangsstand (Frage "pre"),
    //   2. dieser PC handelt wie im Spiel (Skripte/Externals wie in Dialogen, Engine-Funktionen wie beim Spielen),
    //   3. nach einer Wartezeit meldet der Partner, was er jetzt sieht (Frage "post"),
    //   4. dieser PC bewertet: OK / FEHLER / uebersprungen / bekannte Luecke.
    // Ergebnis je Schritt im Chat und im Log ("Coop-Test 07/28 OK ..."), am Ende eine Zusammenfassung. Paket 46 SELFTEST.
    // v38 (Nutzer nach Absturz in Schritt 12/13 von v37: "erst NPCs einfuegen, bevor du irgendwas toetest"): der Lauf legt sich
    // seine Ziele selbst an (Molerat fuer Haltung/Treffer/Tod, Fleischwanze fuer "ein Schlag toetet") und fasst keine
    // Welt-NPCs mehr an. Fallenlassen/Aufheben (direkte DoDropVob/DoTakeVob am Spieler - dabei stuerzte der Client in v37 ab)
    // ist aus dem Lauf genommen und wird von Hand getestet.
    // v42 (Nutzer: "diesmal stehen die Charaktere weiter entfernt, ruhig noch ein paar mehr Tests"): 32 Schritte. Neu:
    // Verfolgungspunkt beim Partner (meine Figur in seiner Welt, NPCs um mich aktiv), Test-Molerat beim Partner gefuehrt (Host:
    // gesendet, Client: RemoteNpc mit Modell) und an derselben Stelle, Toeten mit EP-Regel (fern: Partner-EP gleich, nah: mehr),
    // Test-Molerat bleibt 5 s spaeter tot. Am Start: Abstand zur Figur des Partners (fern/nah).
    // v43 (Nutzer: "alle Hooks in einem Testlauf durchgehen, dann sehen wir die veraenderten Funktionen"; "NPC-Routinen ausserhalb
    // des Radius nicht perfekt"): 34 Schritte. Neu: Engine-Aenderungen wie beim Partner (Fingerabdruck der Selbstpruefung je
    // Verursacher, CoopAudit.cpp - bei Abweichung schreiben beide die ganze Liste ins Log) und NPCs um mich beim Partner (Name,
    // Lage, KI-Zustand, abgeglichen ja/nein - nah und fern ueber den Verfolgungspunkt). Ein Urteil, das mit "--" beginnt, zaehlt
    // als uebersprungen (z. B. Test-Molerat beim Partner schon tot).
    // v44: 36 Schritte - Kamera beim Mitspieler (folgt der Figur, Held steht, NPCs um den Helden bleiben aktiv) und zurueck.
    // v45: 37 Schritte - Zuschau-Fenster (nur mit dem Coop-GD3D11, sonst uebersprungen).
    // v48: 36 Schritte - Vollbild-Kamera entfernt (K1-Schritte raus), dafuer "Zuschau-Fenster aus".
    // Veraendert den Spielstand (Tagebuch, EP, Test-NPCs, Zeit) - nur auf einem Test-Spielstand benutzen. Einmal am Host und
    // einmal am Client starten. Nicht automatisch (im Chat genannt): Zielwechsel (C3), Wiederbeleben (C5), Dialoge (D3-D6),
    // Speichern/Laden (F), Fallenlassen/Aufheben (E8/E2). Alle benutzten Engine-Funktionen nutzt GT schon (COOP-FIXES D12b).

#if ENGINE == Engine_G1

    json CoopAuditSummaryJson(bool refresh);    // v43: CoopAudit.cpp
    void CoopAuditDumpToLog(const char* why);

    enum { CT_START, CT_PRE, CT_WAIT, CT_POST, CT_GAP };

    struct CoopTestDef {
        const char* name;
        const char* ref;
        int waitMs;
        int flags;                                                   // 1 nur Host, 2 bekannte Luecke
        bool (*prep)(std::string& why);                              // Ziel suchen; false = uebersprungen
        json (*query)();                                             // Frage an den Partner (vorher und nachher)
        bool (*act)(std::string& why);                               // Handlung; false = uebersprungen
        bool (*judge)(const json& pre, const json& post, std::string& d);
        void (*cleanup)();
    };

    struct CoopTestRun {
        bool on = false;
        std::string tag;
        size_t idx = 0;
        int phase = CT_START;
        long long deadline = 0;
        json pre, post;
        bool havePre = false, havePost = false;
        int ok = 0, fail = 0, skip = 0, gap = 0;
        std::vector<std::string> fails;
        // Zustand zwischen den Schritten
        int guildOld1 = -1, guildOld2 = -1, guildNew = -1;
        std::string wpName;
        std::string molName;          // Test-Molerat (Schritt 7)
        std::string bugName;          // Test-Fleischwanze (Schritt 12)
        oCMobContainer* chest = NULL;
        zVEC3 chestPos;
        oCItem* weapon = NULL;
        oCItem* armor = NULL;
        int hpOld = 0, strOld = 0;
        std::string marker;
        int itemsBefore = -1;
        // v42: grosse Entfernung
        float partnerDist = -1.f;     // Abstand zur Figur des Partners beim Toeten des Test-Molerats
        bool farKill = false;
        int myActive = -1;            // aktive NPCs um mich (hier) - Vergleich mit dem Partner
        int camActiveBefore = -1;     // v44: aktive NPCs um den eigenen Helden, bevor die Kamera zum Mitspieler geht
    };
    static CoopTestRun CT;

    static const int CT_G1 = 20, CT_G2 = 35;   // GIL_TROLL / GIL_GOLEM (Othello) - begegnen sich kaum

    static void CTLog(const char* fmt, ...) {
        char buf[600];
        va_list ap;
        va_start(ap, fmt);
        _vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        buf[sizeof(buf) - 1] = 0;
        OthelloCompatLog("[%lld s, %s] Coop-Test: %s", DiagSeconds(), DiagRole(), buf);
    }

    static void CTSay(const char* fmt, ...) {
        char buf[400];
        va_list ap;
        va_start(ap, fmt);
        _vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        buf[sizeof(buf) - 1] = 0;
        ChatLog(string(buf));
        CTLog("%s", buf);
    }

    static json CTVec(const zVEC3& v) {
        return json::array({ v.n[0], v.n[1], v.n[2] });
    }

    static zVEC3 CTVecOf(const json& j) {
        return zVEC3(j.at(0).get<float>(), j.at(1).get<float>(), j.at(2).get<float>());
    }

    static int CTInt(const json& o, const char* a, const char* b = NULL, int def = -999) {
        try {
            if (!o.contains(a))
                return def;
            const json& x = b ? o.at(a) : o;
            if (b && !x.contains(b))
                return def;
            const json& v = b ? x.at(b) : x.at(a);
            return v.is_number() ? v.get<int>() : def;
        }
        catch (...) {
            return def;
        }
    }

    static std::string CTStr(const json& o, const char* a, const char* b) {
        try {
            if (o.contains(a) && o.at(a).contains(b) && o.at(a).at(b).is_string())
                return o.at(a).at(b).get<std::string>();
        }
        catch (...) {}
        return std::string();
    }

    static int CTGuild(int a, int b) {
        oCGuilds* g = ogame ? ogame->GetGuilds() : NULL;
        if (!g || !g->attitude || a >= g->size || b >= g->size)
            return -1;
        return g->attitude[b * g->size + a];
    }

    static int CTCountNpcs(const std::string& inst, const zVEC3& p, float r) {
        const int idx = parser->GetIndex(zSTRING(inst.c_str()));
        int n = 0;
        for (auto* l = CoopWorldNpcListFirst(ogame->GetGameWorld()); l; l = l->next) {
            oCNpc* npc = l->data;
            if (npc && npc->GetInstance() == idx && !npc->IsDead() && CoopDist(npc->GetPositionWorld(), p) <= r)
                n++;
        }
        return n;
    }

    // naechster lebender, vom Host nachtraeglich erzeugter NPC (GT-Name "<Objekt>-DYNAMIC-<Wegpunkt>-n") dieser Instanz
    static oCNpc* CTFindDynamic(const char* inst) {
        oCNpc* best = NULL;
        float bestD = 3000.f;
        const int idx = parser->GetIndex(zSTRING(inst));
        for (auto* l = CoopWorldNpcListFirst(ogame->GetGameWorld()); l; l = l->next) {
            oCNpc* npc = l->data;
            if (!npc || npc->GetInstance() != idx || npc->IsDead())
                continue;
            if (DialogNpcName(npc).find("-DYNAMIC-") == std::string::npos)
                continue;
            const float d = CoopDist(npc->GetPositionWorld(), player->GetPositionWorld());
            if (d < bestD) {
                bestD = d;
                best = npc;
            }
        }
        return best;
    }

    static int CTCountItems(const std::string& inst, const zVEC3& p, float r) {
        const int idx = parser->GetIndex(zSTRING(inst.c_str()));
        if (idx < 0)
            return -1;
        zCArray<zCVob*> vobs = CollectVobsInRadius(p, (int)r);
        int n = 0;
        for (int i = 0; i < vobs.GetNumInList(); i++) {
            oCItem* it = zDYNAMIC_CAST<oCItem>(vobs.GetSafe(i));
            if (it && it->GetInstance() == idx && CoopItemLiegtFrei(it) && CoopDist(it->GetPositionWorld(), p) <= r)
                n += CoopItemAmount(it);
        }
        return n;
    }

    static oCMobContainer* CTNearestChest(const zVEC3& p, float r) {
        zCArray<zCVob*> vobs = CollectVobsInRadius(p, (int)r);
        oCMobContainer* best = NULL;
        float bestD = r;
        for (int i = 0; i < vobs.GetNumInList(); i++) {
            oCMobContainer* c = zDYNAMIC_CAST<oCMobContainer>(vobs.GetSafe(i));
            if (!c)
                continue;
            const float d = CoopDist(c->GetPositionWorld(), p);
            if (d < bestD) {
                bestD = d;
                best = c;
            }
        }
        return best;
    }

    // v42: lebende, gerade aktive NPCs (in der Welt) im Umkreis von p - ohne Helden und Coop-Figuren
    static int CTCountActive(const zVEC3& p, float r) {
        int n = 0;
        for (auto* l = CoopWorldNpcListFirst(ogame->GetGameWorld()); l; l = l->next) {
            oCNpc* npc = l->data;
            if (!npc || npc == player || PlayerNpcs.count(npc) || npc->IsDead() || npc->vobLeafList.GetNum() == 0)
                continue;
            if (CoopDist(npc->GetPositionWorld(), p) < r)
                n++;
        }
        return n;
    }

    // v42: Abstand zur naechsten Figur eines Mitspielers (hier)
    static float CTPartnerDist() {
        float best = -1.f;
        for (auto& kv : PlayerNpcs) {
            if (!kv.first)
                continue;
            zVEC3 p = kv.first->GetPositionWorld();
            auto it = SyncNpcs.find(kv.second);
            if (it != SyncNpcs.end() && it->second && it->second->lastPositionFromServer)
                p = *it->second->lastPositionFromServer;
            const float d = CoopDist(p, player->GetPositionWorld());
            if (best < 0 || d < best)
                best = d;
        }
        return best;
    }

    static std::string CTReceivedLogFile(const std::string& from) {
        if (!ServerThread)
            return "system\\GothicCoopOthelloHost.log";
        return from == "FRIEND_1" ? "system\\GothicCoopOthelloClient.log" : "system\\GothicCoopOthelloClient_" + from + ".log";
    }

    // --- Partner: melden, was er sieht ---------------------------------------------------------------------------------
    static json CoopTestObserve(const json& q, const std::string& from) {
        json o = json::object();
        if (q.contains("topic")) {
            oCLogTopic* tp = DialogFindTopic(q["topic"].get<std::string>());
            const std::string tag = q.contains("tag") ? q["tag"].get<std::string>() : std::string();
            json t;
            t["ex"] = tp ? 1 : 0;
            if (tp) {
                int n = 0, has = 0;
                for (auto* e = tp->m_lstEntries.GetNextInList(); e; e = e->GetNextInList())
                    if (oCLogEntry* le = e->GetData()) {
                        n++;
                        if (!tag.empty() && strstr(le->m_strDescription.ToChar(), tag.c_str()))
                            has = 1;
                    }
                t["st"] = (int)tp->m_enuStatus;
                t["n"] = n;
                t["tag"] = has;
            }
            o["topic"] = t;
        }
        if (q.contains("exp"))
            o["exp"] = player ? player->experience_points : -1;
        if (q.contains("guild"))
            o["guild"] = json::array({ CTGuild(CT_G1, CT_G2), CTGuild(CT_G2, CT_G1) });
        if (q.contains("count")) {   // NPCs einer Instanz um die Stelle des Ausloesers (der Partner kann woanders stehen)
            const json& c = q["count"];
            o["count"] = CTCountNpcs(c["inst"].get<std::string>(), CTVecOf(c["p"]), 2500.f);
        }
        if (q.contains("npc")) {
            oCNpc* b = DialogNpcByName(q["npc"].get<std::string>());
            json x;
            x["found"] = b ? 1 : 0;
            if (b) {
                x["hp"] = b->GetAttribute(NPC_ATR_HITPOINTS);
                x["dead"] = b->IsDead() ? 1 : 0;
                x["att"] = b->attitude;
            }
            o["npc"] = x;
        }
        if (q.contains("chest")) {
            const json& c = q["chest"];
            oCMobContainer* ch = FindChest(CTVecOf(c["p"]));
            if (!ch)
                o["chest"] = -1;
            else {
                CoopBestand b = ChestBestand(ch);
                auto f = b.find(c["inst"].get<std::string>());
                o["chest"] = f == b.end() ? 0 : f->second;
            }
        }
        if (q.contains("items")) {
            const json& c = q["items"];
            o["items"] = CTCountItems(c["inst"].get<std::string>(), CTVecOf(c["p"]), 2500.f);
        }
        if (q.contains("fig")) {
            oCNpc* f = DialogFigureOf(q["fig"].get<std::string>());
            json x;
            x["found"] = f ? 1 : 0;
            if (f) {
                x["p"] = CTVec(f->GetPositionWorld());
                x["wm"] = f->GetWeaponMode();
                oCItem* w = f->GetEquippedMeleeWeapon();
                x["melee"] = w ? std::string(w->GetInstanceName().ToChar()) : std::string();
                oCItem* a = f->GetEquippedArmor();
                x["armor"] = a ? std::string(a->GetInstanceName().ToChar()) : std::string();
                x["hp"] = f->GetAttribute(NPC_ATR_HITPOINTS);
            }
            o["fig"] = x;
        }
        if (q.contains("hero")) {
            json x;
            x["str"] = player->GetAttribute(NPC_ATR_STRENGTH);
            x["dex"] = player->GetAttribute(NPC_ATR_DEXTERITY);
            x["lvl"] = player->level;
            o["hero"] = x;
        }
        if (q.contains("time")) {
            int day = 0, h = 0, m = 0;
            ogame->GetTime(day, h, m);
            json x;
            x["day"] = day;
            x["min"] = h * 60 + m;
            x["ver"] = COOP_OTHELLO_FASSUNG;
            x["world"] = std::string(ogame->GetGameWorld()->GetWorldFilename().ToChar());
            o["time"] = x;
        }
        if (q.contains("far")) {   // v42: Verfolgungspunkt - sieht der Partner meine Figur und die NPCs um mich?
            const zVEC3 p = CTVecOf(q["far"]["p"]);
            json x;
            x["fol"] = CoopFollowActive() ? 1 : 0;
            x["dist"] = CoopDist(player->GetPositionWorld(), p);
            oCNpc* f = DialogFigureOf(from);
            x["fig"] = f ? 1 : 0;
            x["figact"] = (f && f->vobLeafList.GetNum() > 0) ? 1 : 0;
            x["act"] = CTCountActive(p, (float)BROADCAST_DISTANCE);
            o["far"] = x;
        }
        if (q.contains("npcx")) {   // v42: Test-NPC beim Partner: Lage, in der Welt, vom Host gesendet bzw. vom Host gefuehrt
            const std::string name = q["npcx"].get<std::string>();
            oCNpc* b = DialogNpcByName(name);
            json x;
            x["found"] = b ? 1 : 0;
            if (b) {
                x["p"] = CTVec(b->GetPositionWorld());
                x["act"] = b->vobLeafList.GetNum() > 0 ? 1 : 0;
                x["dead"] = b->IsDead() ? 1 : 0;
                x["hp"] = b->GetAttribute(NPC_ATR_HITPOINTS);
            }
            if (ServerThread)
                x["sent"] = BroadcastNpcs.count(string(name.c_str())) ? 1 : 0;
            else {
                auto it = SyncNpcs.find(string(name.c_str()));
                x["synced"] = (it != SyncNpcs.end() && it->second && !it->second->destroyed && it->second->hasModel) ? 1 : 0;
            }
            x["host"] = ServerThread ? 1 : 0;
            o["npcx"] = x;
        }
        if (q.contains("audit")) {   // v43: Fingerabdruck der Selbstpruefung (hoechstens alle 10 s neu abgetastet)
            static long long lastScan = -100000;
            const bool refresh = CurrentMs - lastScan > 10000;
            if (refresh)
                lastScan = CurrentMs;
            json x = CoopAuditSummaryJson(refresh);
            if (x.value("h", std::string()) != q["audit"].value("h", std::string()) ||
                x.value("rh", std::string()) != q["audit"].value("rh", std::string()))
                CoopAuditDumpToLog("Testlauf: weicht vom Partner ab");
            o["audit"] = x;
        }
        if (q.contains("near")) {   // v43: NPCs um die Stelle des Ausloesers - Name, Lage, KI-Zustand, in der Welt, abgeglichen
            const zVEC3 p = CTVecOf(q["near"]["p"]);
            const float r = q["near"].value("r", 2000.f);
            json arr = json::array();
            int cnt = 0;
            for (auto* l = CoopWorldNpcListFirst(ogame->GetGameWorld()); l; l = l->next) {
                oCNpc* npc = l->data;
                if (!npc || npc == player || PlayerNpcs.count(npc) || npc->IsDead())
                    continue;
                auto nm = NpcToUniqueNameList.find(npc);
                if (nm == NpcToUniqueNameList.end() || CoopDist(npc->GetPositionWorld(), p) >= r)
                    continue;
                cnt++;
                if (arr.size() >= 40)
                    continue;
                json n;
                n["n"] = std::string(nm->second.ToChar());
                n["p"] = CTVec(npc->GetPositionWorld());
                n["s"] = std::string(npc->state.curState.name.ToChar());
                n["a"] = npc->vobLeafList.GetNum() > 0 ? 1 : 0;
                if (ServerThread)
                    n["l"] = BroadcastNpcs.count(nm->second) ? 1 : 0;
                else {
                    auto it = SyncNpcs.find(nm->second);
                    n["l"] = (it != SyncNpcs.end() && it->second && !it->second->destroyed && it->second->hasModel) ? 1 : 0;
                }
                arr.push_back(n);
            }
            json x;
            x["l"] = arr;
            x["cnt"] = cnt;
            x["host"] = ServerThread ? 1 : 0;
            o["near"] = x;
        }
        if (q.contains("logmark")) {
            const std::string mark = q["logmark"].get<std::string>();
            int has = 0;
            if (!mark.empty())
                if (FILE* f = fopen(CTReceivedLogFile(from).c_str(), "rb")) {
                    std::string all;
                    char buf[8192];
                    size_t got;
                    while ((got = fread(buf, 1, sizeof(buf), f)) > 0)
                        all.append(buf, got);
                    fclose(f);
                    has = all.find(mark) != std::string::npos ? 1 : 0;
                }
            o["logmark"] = has;
        }
        return o;
    }

    // --- Schritte ------------------------------------------------------------------------------------------------------

    static json QTopic() { json q; q["topic"] = "Coop-Test"; q["tag"] = CT.tag; return q; }
    static json QExp() { json q; q["exp"] = 1; return q; }
    static json QGuild() { json q; q["guild"] = 1; return q; }
    static json QCountMol() { json q; q["count"]["inst"] = "MOLERAT"; q["count"]["p"] = CTVec(player->GetPositionWorld()); return q; }
    static json QCountBug() { json q; q["count"]["inst"] = "MEATBUG"; q["count"]["p"] = CTVec(player->GetPositionWorld()); return q; }
    static json QMol() { json q; q["npc"] = CT.molName; return q; }
    static json QBug() { json q; q["npc"] = CT.bugName; return q; }
    static json QChest() { json q; q["chest"]["p"] = CTVec(CT.chestPos); q["chest"]["inst"] = "ITFOAPPLE"; return q; }
    static json QItems() { json q; q["items"]["inst"] = "ITFOAPPLE"; q["items"]["p"] = CTVec(player->GetPositionWorld()); return q; }
    static json QFig() { json q; q["fig"] = DialogMe(); return q; }
    static json QHero() { json q; q["hero"] = 1; return q; }
    static json QTime() { json q; q["time"] = 1; return q; }
    static json QLog() { json q; q["logmark"] = CT.marker; return q; }

    static bool ANone(std::string&) { return true; }

    // 01 Tagebuch: Thema + Eintrag (wie aus einem Dialog: Schnappschuss, B_LogEntry, Unterschied senden)
    static bool A01(std::string& why) {
        zSTRING topic("Coop-Test");
        CoopDlgSnap a, b;
        DlgSnapshot(a);
        if (!DialogFindTopic("Coop-Test")) {
            CoopParserExternal create = DialogExternal("LOG_CREATETOPIC");
            CoopParserExternal status = DialogExternal("LOG_SETTOPICSTATUS");
            if (!create || !status) {
                why = "LOG_CREATETOPIC fehlt";
                return false;
            }
            DialogPushString(&topic);
            DialogPushInt(0);   // LOG_MISSION
            create();
            DialogPushString(&topic);
            DialogPushInt(1);   // LOG_RUNNING
            status();
        }
        zSTRING entry(("Testlauf " + CT.tag + " von " + DialogMe()).c_str());
        if (!DialogScriptLogEntry(topic, entry))
            if (CoopParserExternal add = DialogExternal("LOG_ADDENTRY")) {
                DialogPushString(&topic);
                DialogPushString(&entry);
                add();
            }
        DlgSnapshot(b);
        DlgSendDiff(a, b, std::string(), "Coop-Test");
        StoryAbsorb(a, b);
        return true;
    }
    static bool J01(const json&, const json& post, std::string& d) {
        const int ex = CTInt(post, "topic", "ex"), tag = CTInt(post, "topic", "tag");
        d = std::string("Partner: Thema ") + (ex == 1 ? "da" : "fehlt") + ", Eintrag " + (tag == 1 ? "da" : "fehlt");
        return ex == 1 && tag == 1;
    }

    // 02 Erfahrung +10 wie aus einem Dialog (B_GiveXP beim Partner)
    static bool A02(std::string& why) {
        CoopDlgSnap a, b;
        DlgSnapshot(a);
        if (!DialogScriptGiveXP(10)) {
            why = "B_GiveXP fehlt";
            return false;
        }
        DlgSnapshot(b);
        DlgSendDiff(a, b, std::string(), "Coop-Test");
        StoryAbsorb(a, b);
        return true;
    }
    static bool J02(const json& pre, const json& post, std::string& d) {
        const int a = CTInt(pre, "exp"), b = CTInt(post, "exp");
        char buf[96];
        _snprintf(buf, sizeof(buf), "Partner-EP %d -> %d (soll +10)", a, b);
        d = buf;
        return a != -999 && b - a == 10;
    }

    // 03/04 Themenstatus (Story-Takt)
    static bool CTSetStatus(int st, std::string& why) {
        zSTRING topic("Coop-Test");
        CoopParserExternal status = DialogExternal("LOG_SETTOPICSTATUS");
        if (!status || !DialogFindTopic("Coop-Test")) {
            why = "Thema fehlt (Schritt 1)";
            return false;
        }
        DialogPushString(&topic);
        DialogPushInt(st);
        status();
        return true;
    }
    static bool A03(std::string& why) { return CTSetStatus(2, why); }
    static bool A04(std::string& why) { return CTSetStatus(1, why); }
    static bool CTJudgeStatus(const json& post, int want, std::string& d) {
        const int st = CTInt(post, "topic", "st");
        char buf[64];
        _snprintf(buf, sizeof(buf), "Partner-Status %d (soll %d)", st, want);
        d = buf;
        return st == want;
    }
    static bool J03(const json&, const json& post, std::string& d) { return CTJudgeStatus(post, 2, d); }
    static bool J04(const json&, const json& post, std::string& d) { return CTJudgeStatus(post, 1, d); }

    // 05/06 Gildenhaltung Troll/Golem (Story-Takt D9)
    static bool PGuild(std::string& why) {
        CT.guildOld1 = CTGuild(CT_G1, CT_G2);
        CT.guildOld2 = CTGuild(CT_G2, CT_G1);
        if (CT.guildOld1 < 0 || CT.guildOld2 < 0) {
            why = "Gildentabelle nicht lesbar";
            return false;
        }
        return true;
    }
    static bool AGuild(std::string&) {
        oCGuilds* g = ogame->GetGuilds();
        CT.guildNew = CT.guildOld1 == 2 ? 1 : 2;
        g->SetAttitude(CT_G1, CT_G2, (unsigned char)CT.guildNew);
        g->SetAttitude(CT_G2, CT_G1, (unsigned char)CT.guildNew);
        return true;
    }
    static bool PGuildBack(std::string& why) {
        if (CT.guildOld1 < 0) {
            why = "kein Wert aus dem Schritt davor";
            return false;
        }
        return true;
    }
    static bool AGuildBack(std::string&) {
        oCGuilds* g = ogame->GetGuilds();
        g->SetAttitude(CT_G1, CT_G2, (unsigned char)CT.guildOld1);
        g->SetAttitude(CT_G2, CT_G1, (unsigned char)CT.guildOld2);
        return true;
    }
    static bool CTJudgeGuild(const json& post, std::string& d) {
        int p1 = -999, p2 = -999;
        try {
            p1 = post.at("guild").at(0).get<int>();
            p2 = post.at("guild").at(1).get<int>();
        }
        catch (...) {}
        const int m1 = CTGuild(CT_G1, CT_G2), m2 = CTGuild(CT_G2, CT_G1);
        char buf[96];
        _snprintf(buf, sizeof(buf), "Partner %d/%d, hier %d/%d", p1, p2, m1, m2);
        d = buf;
        return p1 == m1 && p2 == m2;
    }
    static bool JGuild(const json&, const json& post, std::string& d) { return CTJudgeGuild(post, d) && CTGuild(CT_G1, CT_G2) == CT.guildNew; }
    static bool JGuildBack(const json&, const json& post, std::string& d) { return CTJudgeGuild(post, d) && CTGuild(CT_G1, CT_G2) == CT.guildOld1; }

    // 07/12 Test-NPC einfuegen (Wld_InsertNpc ueber den Host, D13) - der Lauf legt seine Ziele selbst an
    static bool CTPrepSpawn(const char* inst, std::string& why) {
        zCWaypoint* wp = ogame->GetGameWorld() && ogame->GetGameWorld()->wayNet
            ? ogame->GetGameWorld()->wayNet->GetNearestWaypoint(player->GetPositionWorld()) : NULL;
        if (!wp || !DialogExternal("WLD_INSERTNPC") || parser->GetIndex(zSTRING(inst)) < 0) {
            why = std::string("kein Wegpunkt / WLD_INSERTNPC / ") + inst;
            return false;
        }
        CT.wpName = wp->GetName().ToChar();
        return true;
    }
    static void CTSpawn(const char* inst) {
        zSTRING wpName(CT.wpName.c_str());
        DialogPushInt(parser->GetIndex(zSTRING(inst)));
        DialogPushString(&wpName);
        DialogExternal("WLD_INSERTNPC")();
    }
    static bool CTJudgeSpawn(const json& pre, const json& post, const char* what, std::string& name, const char* inst,
        std::string& d) {
        const int a = CTInt(pre, "count"), b = CTInt(post, "count");
        oCNpc* mine = CTFindDynamic(inst);
        name = mine ? DialogNpcName(mine) : std::string();
        char buf[200];
        _snprintf(buf, sizeof(buf), "%s beim Partner %d -> %d (soll +1), Name hier %s", what, a, b,
            name.empty() ? "-" : name.c_str());
        d = buf;
        return a >= 0 && b == a + 1 && !name.empty();
    }
    static bool P07(std::string& why) { return CTPrepSpawn("MOLERAT", why); }
    static bool A07(std::string&) { CTSpawn("MOLERAT"); return true; }
    static bool J07(const json& pre, const json& post, std::string& d) {
        return CTJudgeSpawn(pre, post, "Molerats", CT.molName, "MOLERAT", d);
    }
    static bool P12(std::string& why) { return CTPrepSpawn("MEATBUG", why); }
    static bool A12(std::string&) { CTSpawn("MEATBUG"); return true; }
    static bool J12(const json& pre, const json& post, std::string& d) {
        return CTJudgeSpawn(pre, post, "Fleischwanzen", CT.bugName, "MEATBUG", d);
    }

    // Haltung der Test-Fleischwanze (Story-Takt D9): freundlich, dann neutral. v39: an der harmlosen Fleischwanze statt am
    // Molerat - in v38 griff der Molerat bei einem Lauf vom Client an (der Story-Takt traegt nur die feste Haltung zum Host,
    // die voruebergehende blieb dort feindlich). Der Molerat wird jetzt gleich nach dem Einfuegen getroffen und getoetet.
    static bool PMol(std::string& why) {
        oCNpc* m = CT.molName.empty() ? NULL : DialogNpcByName(CT.molName);
        if (!m || m->IsDead()) {
            why = "kein lebender Test-Molerat aus dem Schritt davor";
            return false;
        }
        return true;
    }
    static bool PBugAlive(std::string& why) {
        oCNpc* b = CT.bugName.empty() ? NULL : DialogNpcByName(CT.bugName);
        if (!b || b->IsDead()) {
            why = "keine lebende Test-Fleischwanze aus dem Schritt davor";
            return false;
        }
        return true;
    }
    static bool CTSetBugAtt(int att) {
        oCNpc* m = DialogNpcByName(CT.bugName);
        m->SetAttitude(att);
        m->SetTmpAttitude(att);
        return true;
    }
    static bool A08(std::string&) { return CTSetBugAtt(3); }   // ATT_FRIENDLY
    static bool A09(std::string&) { return CTSetBugAtt(2); }   // ATT_NEUTRAL
    static bool CTJudgeBugAtt(const json& post, int want, std::string& d) {
        const int a = CTInt(post, "npc", "att");
        char buf[160];
        _snprintf(buf, sizeof(buf), "%s beim Partner Haltung %d (soll %d)", CT.bugName.c_str(), a, want);
        d = buf;
        return a == want;
    }
    static bool J08(const json&, const json& post, std::string& d) { return CTJudgeBugAtt(post, 3, d); }
    static bool J09(const json&, const json& post, std::string& d) { return CTJudgeBugAtt(post, 2, d); }

    // 10/11/13 Treffer und Tod laufen ueber den Host, HP und Tod kommen zurueck (C1)
    static bool CTHit(const std::string& name, float dmg, std::string& why) {
        oCNpc* b = DialogNpcByName(name);
        if (!b || b->IsDead()) {
            why = "Test-NPC weg oder schon tot";
            return false;
        }
        b->GetEM(false)->OnDamage(b, player, dmg, 4 /* DAM_EDGE */, b->GetPositionWorld());
        return true;
    }
    static bool A10(std::string& why) { return CTHit(CT.molName, 2.f, why); }        // Othello: mindestens 12
    static bool J10(const json& pre, const json& post, std::string& d) {
        oCNpc* b = DialogNpcByName(CT.molName);
        const int mine = b ? b->GetAttribute(NPC_ATR_HITPOINTS) : -1;
        const int a = CTInt(pre, "npc", "hp"), p = CTInt(post, "npc", "hp");
        char buf[160];
        _snprintf(buf, sizeof(buf), "%s: Partner-HP %d -> %d, hier %d", CT.molName.c_str(), a, p, mine);
        d = buf;
        return p >= 0 && p == mine && p < a;
    }
    static bool CTJudgeDead(const json& post, const std::string& name, std::string& d) {
        const int dead = CTInt(post, "npc", "dead"), found = CTInt(post, "npc", "found");
        d = name + " beim Partner " + (found == 1 ? (dead == 1 ? "tot" : "lebt noch") : "nicht gefunden");
        return dead == 1;
    }
    // v42: Toeten mit EP-Regel - weit weg (> BROADCAST_DISTANCE) bekommt der Partner keine Kill-EP (I2), in der Naehe teilt ihr sie
    static json QMolExp() { json q; q["npc"] = CT.molName; q["exp"] = 1; return q; }
    static bool A11(std::string& why) {
        CT.partnerDist = CTPartnerDist();
        CT.farKill = CT.partnerDist > (float)BROADCAST_DISTANCE;
        return CTHit(CT.molName, 10000.f, why);
    }
    static bool J11(const json& pre, const json& post, std::string& d) {
        const bool dead = CTJudgeDead(post, CT.molName, d);
        const int a = CTInt(pre, "exp"), b = CTInt(post, "exp");
        char buf[200];
        _snprintf(buf, sizeof(buf), "; Abstand %.0f m (%s), Partner-EP %d -> %d (soll %s)", CT.partnerDist / 100.f,
            CT.farKill ? "fern" : "nah", a, b, CT.farKill ? "gleich" : "mehr");
        d += buf;
        const bool xpOk = a != -999 && b != -999 && (CT.farKill ? b == a : b > a);
        return dead && xpOk;
    }

    // v42: Verfolgungspunkt - meine Figur ist beim Partner in der Welt, die NPCs um mich laufen dort
    static json QFar() { json q; q["far"]["p"] = CTVec(player->GetPositionWorld()); return q; }
    static bool AFar(std::string&) {
        CT.myActive = CTCountActive(player->GetPositionWorld(), (float)BROADCAST_DISTANCE);
        return true;
    }
    static bool JFar(const json&, const json& post, std::string& d) {
        const int fol = CTInt(post, "far", "fol"), fig = CTInt(post, "far", "fig"), figact = CTInt(post, "far", "figact");
        const int act = CTInt(post, "far", "act");
        float dist = -1.f;
        try { dist = post.at("far").at("dist").get<float>(); } catch (...) {}
        char buf[240];
        _snprintf(buf, sizeof(buf), "Abstand %.0f m (%s), Verfolgungspunkt beim Partner %s, meine Figur %s, NPCs um mich: hier %d aktiv, beim Partner %d",
            dist / 100.f, dist > (float)BROADCAST_DISTANCE ? "fern" : "nah", fol == 1 ? "an" : "aus",
            fig == 1 ? (figact == 1 ? "in der Welt" : "da, aber nicht in der Welt") : "fehlt", CT.myActive, act);
        d = buf;
        if (fig != 1 || figact != 1)
            return false;
        if (dist > (float)BROADCAST_DISTANCE)   // fern: Verfolgungspunkt muss an sein und NPCs um mich aktiv halten
            return fol == 1 && (CT.myActive <= 0 || act > 0);
        return true;
    }

    // v43: Engine-Aenderungen wie beim Partner - Fingerabdruck der Selbstpruefung (CoopAudit.cpp) je Verursacher-Modul
    static json QAudit() {
        json mine = CoopAuditSummaryJson(false);
        json q;
        q["audit"]["h"] = mine.value("h", std::string());
        q["audit"]["rh"] = mine.value("rh", std::string());
        return q;
    }
    static bool JAudit(const json&, const json& post, std::string& d) {
        if (!post.contains("audit") || post["audit"].value("ok", 0) != 1) {
            d = "Partner hat keine Selbstpruefung (" + (post.contains("audit") ? post["audit"].value("err", std::string("?")) : std::string("keine Antwort")) + ")";
            return false;
        }
        const json& p = post["audit"];
        json m = CoopAuditSummaryJson(true);
        const bool same = m.value("h", std::string()) == p.value("h", std::string()) && m.value("rh", std::string()) == p.value("rh", std::string());
        char buf[300];
        _snprintf(buf, sizeof(buf), "hier %d Stellen .text / %d .rdata, GT-Haken %d/%d - Partner %d / %d, %d/%d; Fingerabdruck %s",
            m.value("n", -1), m.value("rn", -1), m.value("ha", -1), m.value("ht", -1), p.value("n", -1), p.value("rn", -1),
            p.value("ha", -1), p.value("ht", -1), same ? "gleich" : "VERSCHIEDEN");
        buf[sizeof(buf) - 1] = 0;
        d = buf;
        // Unterschiede je Verursacher
        std::string diff;
        const json none = json::object();
        for (const char* sec : { "o", "ro" }) {
            const json& mo = m.contains(sec) ? m[sec] : none;
            const json& po = p.contains(sec) ? p[sec] : none;
            std::set<std::string> keys;
            for (auto it = mo.begin(); it != mo.end(); ++it) keys.insert(it.key());
            for (auto it = po.begin(); it != po.end(); ++it) keys.insert(it.key());
            for (auto& k : keys) {
                const int a = mo.value(k, 0), b = po.value(k, 0);
                if (a != b) {
                    char x[160];
                    _snprintf(x, sizeof(x), "%s%s%s hier %d/Partner %d", diff.empty() ? "" : ", ", k.c_str(),
                        strcmp(sec, "ro") == 0 ? " (.rdata)" : "", a, b);
                    x[sizeof(x) - 1] = 0;
                    diff += x;
                }
            }
        }
        if (!diff.empty())
            d += "; abweichend: " + diff;
        if (!same) {
            CoopAuditDumpToLog("Testlauf: weicht vom Partner ab");
            d += " - beide Listen im Log (HL-Zeilen, tools\\auditnames.py)";
        }
        return same && m.value("ha", -1) == m.value("ht", -2) && p.value("ha", -1) == p.value("ht", -2);
    }

    // v43: NPCs um mich beim Partner - nah direkt, fern ueber den Verfolgungspunkt (Nutzer: "NPC-Routinen ausserhalb des Radius
    // nicht perfekt"). Gleich benannt, Lage, KI-Zustand auf beiden Seiten, beim Partner abgeglichen (Host: gesendet,
    // Client: vom Host gefuehrt - sonst sperrt der StartAni-Haken seine Animationen und er steht still).
    static const float CT_NEAR_R = 2000.f;
    static json QNear() { json q; q["near"]["p"] = CTVec(player->GetPositionWorld()); q["near"]["r"] = CT_NEAR_R; return q; }
    static bool JNear(const json&, const json& post, std::string& d) {
        if (!post.contains("near") || !post["near"].contains("l")) {
            d = "Partner meldet keine NPC-Liste";
            return false;
        }
        const json& pl = post["near"]["l"];
        const bool partnerHost = post["near"].value("host", 0) == 1;
        struct Row { std::string n, mine, theirs; float dev; };
        std::vector<Row> rows;
        int here = 0, missing = 0, matched = 0, led = 0, partnerActive = 0;
        std::vector<float> devs;
        for (auto& x : pl) {
            if (x.value("a", 0) == 1)
                partnerActive++;
            if (x.value("l", 0) == 1)
                led++;
        }
        for (auto* l = CoopWorldNpcListFirst(ogame->GetGameWorld()); l; l = l->next) {
            oCNpc* npc = l->data;
            if (!npc || npc == player || PlayerNpcs.count(npc) || npc->IsDead() || npc->vobLeafList.GetNum() == 0)
                continue;
            auto nm = NpcToUniqueNameList.find(npc);
            if (nm == NpcToUniqueNameList.end() || CoopDist(npc->GetPositionWorld(), player->GetPositionWorld()) >= CT_NEAR_R)
                continue;
            here++;
            const std::string name = nm->second.ToChar();
            const json* hit = NULL;
            for (auto& x : pl)
                if (x.value("n", std::string()) == name) {
                    hit = &x;
                    break;
                }
            if (!hit || (*hit).value("a", 0) != 1) {
                missing++;
                rows.push_back({ name, npc->state.curState.name.ToChar(), hit ? "nicht in der Welt" : "fehlt", 1e9f });
                continue;
            }
            matched++;
            const float dev = CoopDist(CTVecOf((*hit)["p"]), npc->GetPositionWorld());
            devs.push_back(dev);
            rows.push_back({ name, npc->state.curState.name.ToChar(), (*hit).value("s", std::string()) +
                ((*hit).value("l", 0) == 1 ? "" : (partnerHost ? ", nicht gesendet" : ", NICHT gefuehrt")), dev });
        }
        if (here == 0) {
            d = "-- keine NPCs in 20 m um mich";
            return false;
        }
        std::sort(devs.begin(), devs.end());
        const float median = devs.empty() ? -1.f : devs[devs.size() / 2];
        const float worst = devs.empty() ? -1.f : devs.back();
        const float dist = CTPartnerDist();
        char buf[400];
        _snprintf(buf, sizeof(buf), "Abstand %.0f m (%s): hier %d NPCs in 20 m, beim Partner %d (in der Welt %d, %s %d); fehlen/nicht in"
            " der Welt %d; Lage-Abweichung Mitte %.0f cm, groesste %.0f cm", dist / 100.f, dist > (float)BROADCAST_DISTANCE ? "fern" : "nah",
            here, post["near"].value("cnt", 0), partnerActive, partnerHost ? "gesendet" : "vom Host gefuehrt", led, missing, median, worst);
        buf[sizeof(buf) - 1] = 0;
        d = buf;
        // die 8 groessten Abweichungen mit KI-Zustand auf beiden Seiten ins Log
        std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) { return a.dev > b.dev; });
        for (size_t i = 0; i < rows.size() && i < 8; i++)
            CTLog("  NPC %s: %s - hier %s, beim Partner %s", rows[i].n.c_str(),
                rows[i].dev >= 1e8f ? "fehlt dort" : (std::to_string((int)rows[i].dev) + " cm").c_str(), rows[i].mine.c_str(),
                rows[i].theirs.c_str());
        const int partnerNamed = (int)pl.size();
        const bool ledOk = partnerNamed == 0 || led * 5 >= partnerNamed * 4;   // mind. 80 % abgeglichen
        return missing * 5 <= here && median >= 0 && median < 300.f && ledOk;
    }

    // v42: Test-NPC beim Partner vom Host gefuehrt (Host: gesendet, Client: RemoteNpc mit Modell) und an derselben Stelle
    static json QMolX() { json q; q["npcx"] = CT.molName; return q; }
    static bool JMolSync(const json&, const json& post, std::string& d) {
        const int found = CTInt(post, "npcx", "found"), act = CTInt(post, "npcx", "act"), host = CTInt(post, "npcx", "host");
        const int sent = CTInt(post, "npcx", "sent"), synced = CTInt(post, "npcx", "synced");
        if (found == 1 && CTInt(post, "npcx", "dead") == 1) {   // v43: z. B. von Wachen getoetet (Test v42, WP_INTRO_WI06)
            d = "-- nicht bewertbar: " + CT.molName + " beim Partner schon tot (z. B. von Wachen getoetet)";
            return false;
        }
        char buf[200];
        if (host == 1)
            _snprintf(buf, sizeof(buf), "%s beim Host %s, %s, %s", CT.molName.c_str(), found == 1 ? "da" : "fehlt",
                act == 1 ? "in der Welt" : "nicht in der Welt", sent == 1 ? "wird gesendet" : "wird NICHT gesendet");
        else
            _snprintf(buf, sizeof(buf), "%s beim Client %s, %s, %s", CT.molName.c_str(), found == 1 ? "da" : "fehlt",
                act == 1 ? "in der Welt" : "nicht in der Welt", synced == 1 ? "vom Host gefuehrt" : "NICHT vom Host gefuehrt");
        d = buf;
        // Host als Partner: er muss den NPC senden (nahe meiner Figur). Ich als Host: der Client muss ihn fuehren.
        return found == 1 && act == 1 && (host == 1 ? sent == 1 : synced == 1);
    }
    static bool JMolPos(const json&, const json& post, std::string& d) {
        oCNpc* m = DialogNpcByName(CT.molName);
        if (CTInt(post, "npcx", "found") == 1 && CTInt(post, "npcx", "dead") == 1) {   // v43
            d = "-- nicht bewertbar: " + CT.molName + " beim Partner schon tot";
            return false;
        }
        if (!m || CTInt(post, "npcx", "found") != 1 || !post["npcx"].contains("p")) {
            d = "Test-Molerat fehlt hier oder beim Partner";
            return false;
        }
        const float dist = CoopDist(CTVecOf(post["npcx"]["p"]), m->GetPositionWorld());
        char buf[120];
        _snprintf(buf, sizeof(buf), "%s: Lage beim Partner %.0f cm von hier (soll < 500)", CT.molName.c_str(), dist);
        d = buf;
        return dist < 500.f;
    }

    // v45: Zuschau-Fenster (K2) - nur mit dem Coop-GD3D11 (Export GDX_CoopSecondView). v48: ohne Vollbild-Schritte (K1 entfernt)
    static bool PWin(std::string& why) {
        if (!CoopCamWindowAvailable()) {
            why = "Renderer ohne Zuschau-Fenster (Original-GD3D11)";
            return false;
        }
        if (CoopCamWindowOn())
            CoopCamStop("Coop-Test: Fenster-Schritt beginnt neu");   // z. B. von der Automatik geoeffnet
        for (auto& kv : PlayerNpcs)
            if (kv.first && kv.first->vobLeafList.GetNum() > 0)
                return true;
        why = "keine Figur eines Mitspielers in der Welt";
        return false;
    }
    static bool AWin(std::string& why) {
        CoopCamToggle();
        if (!CoopCamWindowOn()) {
            why = "Zuschau-Fenster liess sich nicht einschalten";
            return false;
        }
        return true;
    }
    static bool JWin(const json&, const json&, std::string& d) {
        const bool on = CoopCamWindowSent();
        d = on ? "Fenster an (Renderer bekommt die Verfolgerkamera je Bild) - Bild selbst von Hand pruefen" : "Fenster wieder aus";
        return on;
    }

    // v48: Shift+F5 schaltet das Fenster wieder aus (bei einem Mitspieler; mehrere: reihum bis aus)
    static bool PWinOff(std::string& why) {
        if (!CoopCamWindowOn()) {
            why = "Zuschau-Fenster war nicht an";
            return false;
        }
        return true;
    }
    static bool AWinOff(std::string& why) {
        for (int i = 0; i < 8 && CoopCamWindowOn(); i++)
            CoopCamToggle();
        if (CoopCamWindowOn()) {
            why = "Zuschau-Fenster liess sich nicht ausschalten";
            return false;
        }
        return true;
    }
    static bool JWinOff(const json&, const json&, std::string& d) {
        const bool off = !CoopCamWindowOn();
        d = off ? "Fenster aus, Kamera und Held unberuehrt" : "Fenster wieder an (Automatik?)";
        return off && !player->IsMovLock();
    }

    // v42: tot bleibt tot - kein Wiederaufstehen mit HP 1 (I3)
    static bool PMolDead(std::string& why) {
        if (CT.molName.empty()) {
            why = "kein Test-Molerat";
            return false;
        }
        return true;
    }
    static bool JMolStillDead(const json&, const json& post, std::string& d) {
        const int dead = CTInt(post, "npc", "dead"), hp = CTInt(post, "npc", "hp"), found = CTInt(post, "npc", "found");
        char buf[160];
        _snprintf(buf, sizeof(buf), "%s beim Partner 5 s spaeter %s (HP %d)", CT.molName.c_str(),
            found == 1 ? (dead == 1 ? "tot" : "LEBT") : "nicht mehr da", hp);
        d = buf;
        return found != 1 || dead == 1;   // weggeraeumt zaehlt als tot
    }
    static bool A13(std::string& why) { return CTHit(CT.bugName, 2.f, why); }        // 5 HP: ein Schlag toetet
    static bool J13(const json&, const json& post, std::string& d) { return CTJudgeDead(post, CT.bugName, d); }

    // 14/15 Truhe: Apfel hinein / heraus (Truhen-Haken wie Quickloot und Skripte, E1)
    static bool P14(std::string& why) {
        CT.chest = CTNearestChest(player->GetPositionWorld(), 1000.f);
        if (!CT.chest) {
            why = "keine Truhe in 10 m";
            return false;
        }
        CT.chestPos = CT.chest->GetPositionWorld();
        return true;
    }
    static bool A14(std::string& why) {
        oCItem* it = CreateCoopItem(parser->GetIndex(zSTRING("ITFOAPPLE")));
        if (!it) {
            why = "Apfel nicht erzeugt";
            return false;
        }
        CT.chest->Insert(it);
        it->Release();
        return true;
    }
    static bool CTJudgeChest(const json& pre, const json& post, int delta, std::string& d) {
        const int a = CTInt(pre, "chest"), b = CTInt(post, "chest");
        char buf[96];
        _snprintf(buf, sizeof(buf), "Aepfel in der Truhe beim Partner %d -> %d (soll %+d)", a, b, delta);
        d = buf;
        return a >= 0 && b == a + delta;
    }
    static bool J14(const json& pre, const json& post, std::string& d) { return CTJudgeChest(pre, post, 1, d); }
    static bool P15(std::string& why) {
        if (!CT.chest || FindChest(CT.chestPos) != CT.chest) {
            why = "keine Truhe aus Schritt 14";
            return false;
        }
        return true;
    }
    static bool A15(std::string& why) {
        for (zCListSort<oCItem>* n = CT.chest->containList.next; n; n = n->next)
            if (n->data && CoopItemInst(n->data) == "ITFOAPPLE") {
                if (CoopItemAmount(n->data) > 1)
                    CT.chest->Remove(n->data, 1);
                else
                    CT.chest->Remove(n->data);
                return true;
            }
        why = "kein Apfel in der Truhe";
        return false;
    }
    static bool J15(const json& pre, const json& post, std::string& d) { return CTJudgeChest(pre, post, -1, d); }

    // 16 Skript legt einen Apfel in die Welt (Wld_InsertItem) - bekannte Luecke: laeuft bisher nur lokal
    static bool P16(std::string& why) {
        if (CT.wpName.empty() || !DialogExternal("WLD_INSERTITEM")) {
            why = "kein Wegpunkt / WLD_INSERTITEM";
            return false;
        }
        CT.itemsBefore = CTCountItems("ITFOAPPLE", player->GetPositionWorld(), 2500.f);
        return true;
    }
    static bool A16(std::string&) {
        zSTRING wpName(CT.wpName.c_str());
        DialogPushInt(parser->GetIndex(zSTRING("ITFOAPPLE")));
        DialogPushString(&wpName);
        DialogExternal("WLD_INSERTITEM")();
        return true;
    }
    static bool J16(const json& pre, const json& post, std::string& d) {
        const int a = CTInt(pre, "items"), b = CTInt(post, "items");
        char buf[96];
        _snprintf(buf, sizeof(buf), "Aepfel am Boden beim Partner %d -> %d (soll +1)", a, b);
        d = buf;
        return a >= 0 && b == a + 1;
    }
    static void C16() {
        // nur hier eingefuegt (Luecke) -> hier wieder weg, sonst unterscheiden sich die Welten
        const int now = CTCountItems("ITFOAPPLE", player->GetPositionWorld(), 2500.f);
        if (CT.itemsBefore >= 0 && now > CT.itemsBefore)
            if (oCItem* it = FindLooseItem("ITFOAPPLE", player->GetPositionWorld(), 2500.f))
                RemoveWorldItem(it);
    }

    // 17 Lage der eigenen Figur beim Partner
    static bool J17(const json&, const json& post, std::string& d) {
        if (CTInt(post, "fig", "found") != 1) {
            d = "Partner findet die Figur nicht";
            return false;
        }
        const float dist = CoopDist(CTVecOf(post["fig"]["p"]), player->GetPositionWorld());
        char buf[64];
        _snprintf(buf, sizeof(buf), "Abstand %.0f (soll < 300)", dist);
        d = buf;
        return dist < 300.f;
    }

    // 18/19 Waffenmodus
    static bool A18(std::string&) { player->SetWeaponMode2(NPC_WEAPON_FIST); return true; }
    static bool A19(std::string&) { player->SetWeaponMode2(NPC_WEAPON_NONE); return true; }
    static bool CTJudgeWm(const json& post, int want, std::string& d) {
        const int wm = CTInt(post, "fig", "wm");
        char buf[64];
        _snprintf(buf, sizeof(buf), "Figur beim Partner Modus %d (soll %d)", wm, want);
        d = buf;
        return wm == want;
    }
    static bool J18(const json&, const json& post, std::string& d) { return CTJudgeWm(post, NPC_WEAPON_FIST, d); }
    static bool J19(const json&, const json& post, std::string& d) { return CTJudgeWm(post, NPC_WEAPON_NONE, d); }

    // 20-23 Nahkampfwaffe und Ruestung anlegen / ablegen (wie die Wiederherstellung des eigenen Helden: erzeugen, PutInInv,
    // EquipItem; ablegen: UnequipItem, RemoveFromInv, RemoveVob)
    static oCItem* CTEquipNew(const char* inst, std::string& why) {
        oCItem* it = CreateCoopItem(parser->GetIndex(zSTRING(inst)));
        oCItem* inv = it ? player->PutInInv(it) : NULL;
        if (!inv) {
            why = std::string(inst) + " nicht erzeugt";
            return NULL;
        }
        player->EquipItem(inv);
        return inv;
    }
    static void CTDropOwn(oCItem*& it) {
        if (!it)
            return;
        player->UnequipItem(it);
        if (oCItem* r = player->RemoveFromInv(it, 1))
            ogame->GetGameWorld()->RemoveVob(r);
        it = NULL;
    }
    static bool P20(std::string& why) {
        if (player->GetEquippedMeleeWeapon()) {
            why = "Held hat schon eine Nahkampfwaffe angelegt";
            return false;
        }
        if (parser->GetIndex(zSTRING("ALTESSCHWERT")) < 0) {
            why = "ALTESSCHWERT fehlt";
            return false;
        }
        return true;
    }
    static bool A20(std::string& why) {
        CT.weapon = CTEquipNew("ALTESSCHWERT", why);
        if (!CT.weapon)
            return false;
        if (player->GetEquippedMeleeWeapon() != CT.weapon) {   // z. B. Staerke-Anforderung nicht erfuellt
            CTDropOwn(CT.weapon);
            why = "Held kann das Schwert nicht anlegen (Anforderungen)";
            return false;
        }
        return true;
    }
    static bool J20(const json&, const json& post, std::string& d) {
        const std::string m = CTStr(post, "fig", "melee");
        d = "Figur beim Partner: " + (m.empty() ? std::string("keine Waffe") : m);
        return m == "ALTESSCHWERT";
    }
    static bool P21(std::string& why) {
        if (!CT.weapon || player->GetEquippedMeleeWeapon() != CT.weapon) {
            why = "kein Schwert aus Schritt 20";
            return false;
        }
        return true;
    }
    static bool A21(std::string&) { CTDropOwn(CT.weapon); return true; }
    static bool J21(const json&, const json& post, std::string& d) {
        const std::string m = CTStr(post, "fig", "melee");
        d = "Figur beim Partner: " + (m.empty() ? std::string("keine Waffe") : m);
        return m.empty();
    }
    static bool P22(std::string& why) {
        if (player->GetEquippedArmor()) {
            why = "Held traegt schon eine Ruestung";
            return false;
        }
        if (parser->GetIndex(zSTRING("VLK_ARMOR_L")) < 0) {
            why = "VLK_ARMOR_L fehlt";
            return false;
        }
        return true;
    }
    static bool A22(std::string& why) {
        CT.armor = CTEquipNew("VLK_ARMOR_L", why);
        if (!CT.armor)
            return false;
        if (player->GetEquippedArmor() != CT.armor) {
            CTDropOwn(CT.armor);
            why = "Held kann die Ruestung nicht anlegen (Anforderungen)";
            return false;
        }
        return true;
    }
    static bool J22(const json&, const json& post, std::string& d) {
        const std::string a = CTStr(post, "fig", "armor");
        d = "Figur beim Partner: " + (a.empty() ? std::string("keine Ruestung") : a);
        return a == "VLK_ARMOR_L";
    }
    static bool P23(std::string& why) {
        if (!CT.armor || player->GetEquippedArmor() != CT.armor) {
            why = "keine Ruestung aus Schritt 22";
            return false;
        }
        return true;
    }
    static bool A23(std::string&) { CTDropOwn(CT.armor); return true; }
    static bool J23(const json&, const json& post, std::string& d) {
        const std::string a = CTStr(post, "fig", "armor");
        d = "Figur beim Partner: " + (a.empty() ? std::string("keine Ruestung") : a);
        return a.empty();
    }

    // 24 Lebenspunkte des eigenen Helden (HP-Abgleich der Figur)
    static bool A24(std::string& why) {
        CT.hpOld = player->GetAttribute(NPC_ATR_HITPOINTS);
        if (CT.hpOld <= 2) {
            why = "zu wenig Lebenspunkte";
            return false;
        }
        player->SetAttribute(NPC_ATR_HITPOINTS, CT.hpOld - 1);
        return true;
    }
    static bool J24(const json&, const json& post, std::string& d) {
        const int hp = CTInt(post, "fig", "hp");
        char buf[64];
        _snprintf(buf, sizeof(buf), "Figur beim Partner %d HP (soll %d)", hp, CT.hpOld - 1);
        d = buf;
        return hp == CT.hpOld - 1;
    }
    static void C24() { player->SetAttribute(NPC_ATR_HITPOINTS, CT.hpOld); }

    // 25 Heldenwerte bleiben beim eigenen Helden (D10/F2)
    static bool A25(std::string&) {
        CT.strOld = player->GetAttribute(NPC_ATR_STRENGTH);
        player->SetAttribute(NPC_ATR_STRENGTH, CT.strOld + 1);
        return true;
    }
    static bool J25(const json& pre, const json& post, std::string& d) {
        const int a = CTInt(pre, "hero", "str"), b = CTInt(post, "hero", "str");
        char buf[96];
        _snprintf(buf, sizeof(buf), "Staerke des Partner-Helden %d -> %d (soll gleich)", a, b);
        d = buf;
        return a != -999 && a == b && CTInt(pre, "hero", "dex") == CTInt(post, "hero", "dex");
    }
    static void C25() { player->SetAttribute(NPC_ATR_STRENGTH, CT.strOld); }

    // 26 Uhrzeit +30 min (nur Host; GT schickt sonst alle 60 s)
    static bool A26(std::string&) {
        int day = 0, h = 0, m = 0;
        ogame->GetTime(day, h, m);
        const int t = (h * 60 + m + 30) % (24 * 60);
        ogame->GetWorldTimer()->SetTime(t / 60, t % 60);
        if (Myself)
            Myself->SyncTime();
        return true;
    }
    static bool J26(const json&, const json& post, std::string& d) {
        int day = 0, h = 0, m = 0;
        ogame->GetTime(day, h, m);
        const int p = CTInt(post, "time", "min");
        int diff = abs(p - (h * 60 + m));
        if (diff > 12 * 60)
            diff = 24 * 60 - diff;
        char buf[96];
        _snprintf(buf, sizeof(buf), "Partner %02d:%02d, hier %02d:%02d", p / 60, p % 60, h, m);
        d = buf;
        return p >= 0 && diff <= 5;
    }

    // 27 Fassung, Welt und Tag gleich
    static bool J27(const json&, const json& post, std::string& d) {
        int day = 0, h = 0, m = 0;
        ogame->GetTime(day, h, m);
        const std::string ver = CTStr(post, "time", "ver"), world = CTStr(post, "time", "world");
        const std::string myWorld = ogame->GetGameWorld()->GetWorldFilename().ToChar();
        char buf[200];
        _snprintf(buf, sizeof(buf), "Partner %s, %s, Tag %d - hier %s, %s, Tag %d", ver.c_str(), world.c_str(),
            CTInt(post, "time", "day"), COOP_OTHELLO_FASSUNG, myWorld.c_str(), day);
        d = buf;
        return ver == COOP_OTHELLO_FASSUNG && world == myWorld && CTInt(post, "time", "day") == day;
    }

    // 28 Log-Versand: eine Marke im eigenen Log kommt in der Log-Datei beim Partner an (G4/G4b)
    static bool A28(std::string&) {
        CT.marker = "Coop-Test-Marke " + CT.tag;
        CTLog("%s", CT.marker.c_str());
        return true;
    }
    static bool J28(const json&, const json& post, std::string& d) {
        d = CTInt(post, "logmark") == 1 ? "Marke in der Log-Datei des Partners" : "Marke beim Partner nicht gefunden";
        return CTInt(post, "logmark") == 1;
    }

    static const CoopTestDef CoopTests[] = {
        { "Verfolgungspunkt beim Partner",    "I1",    1500, 0, NULL,       QFar,      AFar,       JFar,       NULL },
        { "Engine-Aenderungen wie beim Partner", "H1",  300, 0, NULL,       QAudit,    ANone,      JAudit,     NULL },
        { "NPCs um mich beim Partner",        "I5",     300, 0, NULL,       QNear,     ANone,      JNear,      NULL },
        { "Tagebuch: Thema + Eintrag",        "D4/D8", 2000, 0, NULL,       QTopic,    A01,        J01,        NULL },
        { "Erfahrung +10 wie aus Dialog",     "D11",   2000, 0, NULL,       QExp,      A02,        J02,        NULL },
        { "Themenstatus erfolgreich",         "D7",    2500, 0, NULL,       QTopic,    A03,        J03,        NULL },
        { "Themenstatus laufend",             "D7",    2500, 0, NULL,       QTopic,    A04,        J04,        NULL },
        { "Gildenhaltung Troll/Golem",        "D9",    2500, 0, PGuild,     QGuild,    AGuild,     JGuild,     NULL },
        { "Gildenhaltung zurueck",            "D9",    2500, 0, PGuildBack, QGuild,    AGuildBack, JGuildBack, NULL },
        { "Test-NPC einfuegen (Molerat)",     "D13",   3500, 0, P07,        QCountMol, A07,        J07,        NULL },
        { "Test-NPC beim Partner gefuehrt",   "I1",    2500, 0, PMol,       QMolX,     ANone,      JMolSync,   NULL },
        { "Test-NPC: Lage beim Partner",      "I1",     300, 0, PMol,       QMolX,     ANone,      JMolPos,    NULL },
        { "Test-NPC treffen",                 "C1",    2500, 0, PMol,       QMol,      A10,        J10,        NULL },
        { "Test-NPC toeten + EP-Regel",       "C1/I2", 3000, 0, PMol,       QMolExp,   A11,        J11,        NULL },
        { "Test-NPC bleibt tot (5 s)",        "I3",    5000, 0, PMolDead,   QMol,      ANone,      JMolStillDead, NULL },
        { "Zweiter Test-NPC (Fleischwanze)",  "D13",   3500, 0, P12,        QCountBug, A12,        J12,        NULL },
        { "Fleischwanze Haltung freundlich",  "D9",    2500, 0, PBugAlive,  QBug,      A08,        J08,        NULL },
        { "Fleischwanze Haltung neutral",     "D9",    2500, 0, PBugAlive,  QBug,      A09,        J09,        NULL },
        { "Fleischwanze: ein Schlag toetet",  "C1",    3000, 0, PBugAlive,  QBug,      A13,        J13,        NULL },
        { "Truhe: Apfel hinein",              "E1",    2500, 0, P14,        QChest,    A14,        J14,        NULL },
        { "Truhe: Apfel heraus",              "E1",    2500, 0, P15,        QChest,    A15,        J15,        NULL },
        { "Skript: Wld_InsertItem",           "offen", 2500, 2, P16,        QItems,    A16,        J16,        C16 },
        { "Lage der Figur",                   "GT",     300, 0, NULL,       QFig,      ANone,      J17,        NULL },
        { "Faustkampf-Modus",                 "GT",    2000, 0, NULL,       QFig,      A18,        J18,        NULL },
        { "Waffe weg",                        "GT",    2000, 0, NULL,       QFig,      A19,        J19,        NULL },
        { "Nahkampfwaffe anlegen",            "GT",    2000, 0, P20,        QFig,      A20,        J20,        NULL },
        { "Nahkampfwaffe ablegen",            "GT",    2000, 0, P21,        QFig,      A21,        J21,        NULL },
        { "Ruestung anlegen",                 "GT",    2000, 0, P22,        QFig,      A22,        J22,        NULL },
        { "Ruestung ablegen",                 "GT",    2000, 0, P23,        QFig,      A23,        J23,        NULL },
        { "Lebenspunkte des Helden",          "GT",    2000, 0, NULL,       QFig,      A24,        J24,        C24 },
        { "Heldenwerte bleiben lokal",        "D10",   2000, 0, NULL,       QHero,     A25,        J25,        C25 },
        { "Uhrzeit +30 min",                  "GT",    2000, 1, NULL,       QTime,     A26,        J26,        NULL },
        { "Zuschau-Fenster",                  "K2",    1500, 0, PWin,       QTime,     AWin,       JWin,       NULL },
        { "Zuschau-Fenster aus",              "K2",    1000, 0, PWinOff,    QTime,     AWinOff,    JWinOff,    NULL },
        { "Fassung, Welt, Tag",               "G/F",    300, 0, NULL,       QTime,     ANone,      J27,        NULL },
        { "Log-Versand",                      "G4",    3500, 0, NULL,       QLog,      A28,        J28,        NULL },
    };
    static const size_t CoopTestCount = sizeof(CoopTests) / sizeof(CoopTests[0]);

    // --- Ablauf --------------------------------------------------------------------------------------------------------

    static void CTSend(const char* f, const json& extra) {
        json j = extra;
        j["type"] = SYNC_SELFTEST;
        j["id"] = MyselfId.ToChar();
        j["f"] = f;
        CoopTraceCount(TRACE_SENT, SYNC_SELFTEST);
        ReadyToSendJsons.enqueue(j);
    }

    static void CTAsk(const char* ph) {
        json x;
        x["run"] = CT.tag;
        x["n"] = (int)CT.idx;
        x["ph"] = ph;
        x["from"] = DialogMe();
        x["q"] = CoopTests[CT.idx].query();
        CTSend("q", x);
    }

    static void CTResult(const char* what, const std::string& d) {
        const CoopTestDef& t = CoopTests[CT.idx];
        CTSay("Coop-Test %02u/%02u %s  %s (%s): %s", (unsigned)(CT.idx + 1), (unsigned)CoopTestCount, what, t.name, t.ref,
            d.c_str());
    }

    static void CTFinish(bool aborted) {
        CT.on = false;
        if (CoopCamWindowOn())
            CoopCamStop("Coop-Test beendet");   // v44/v48: abgebrochen waehrend der Fenster-Schritte
        CTSay("Coop-Testlauf %s %s: %d OK, %d FEHLER, %d uebersprungen, %d bekannte Luecke", CT.tag.c_str(),
            aborted ? "abgebrochen" : "fertig", CT.ok, CT.fail, CT.skip, CT.gap);
        for (auto& f : CT.fails)
            CTLog("  FEHLER: %s", f.c_str());
        CTSay("Von Hand pruefen: Zielwechsel (C3), Wiederbeleben F4 (C5), Dialoge (D3-D6), Speichern/Laden (F), Fallenlassen/Aufheben (E8/E2)");
    }

    void CoopSelfTestToggle() {
        if (CT.on) {
            CTFinish(true);
            return;
        }
        if (!parser || zCParser::GetParser() != parser || !player || !ogame) {
            CTSay("Coop-Test: Spiel nicht bereit");
            return;
        }
        if (PlayerNpcs.empty()) {
            CTSay("Coop-Test: kein Mitspieler verbunden");
            return;
        }
        SYSTEMTIME st;
        GetLocalTime(&st);
        char tag[64];
        _snprintf(tag, sizeof(tag), "T%02d%02d%02d-%s", st.wHour, st.wMinute, st.wSecond, MyselfId.ToChar());
        tag[sizeof(tag) - 1] = 0;
        CT = CoopTestRun();
        CT.on = true;
        CT.tag = tag;
        CTSay("Coop-Testlauf %s startet: %u Schritte, etwa %u s - Shift+F11 bricht ab (nur Test-Spielstand!)", CT.tag.c_str(),
            (unsigned)CoopTestCount, (unsigned)(CoopTestCount * 4));
        const float pd = CTPartnerDist();   // v42
        CTSay("Abstand zur Figur des Partners: %.0f m (%s; Reichweite %d m)", pd / 100.f,
            pd > (float)BROADCAST_DISTANCE ? "fern - Verfolgungspunkt wird mitgeprueft" : "nah", BROADCAST_DISTANCE / 100);
    }

    void CoopSelfTestPulse() {
        if (!CT.on)
            return;
        if (!player || !ogame || IsLoadingLevel) {
            CTFinish(true);
            return;
        }
        try {
            const CoopTestDef& t = CoopTests[CT.idx < CoopTestCount ? CT.idx : 0];
            switch (CT.phase) {
            case CT_START:
            {
                if (CT.idx >= CoopTestCount) {
                    CTFinish(false);
                    return;
                }
                std::string why;
                if ((t.flags & 1) && !ServerThread) {
                    CT.skip++;
                    CTResult("--", "nur am Host");
                    CT.phase = CT_GAP;
                    CT.deadline = CurrentMs + 200;
                    break;
                }
                if (t.prep && !t.prep(why)) {
                    CT.skip++;
                    CTResult("--", "uebersprungen: " + why);
                    CT.phase = CT_GAP;
                    CT.deadline = CurrentMs + 200;
                    break;
                }
                CT.havePre = CT.havePost = false;
                CTAsk("pre");
                CT.phase = CT_PRE;
                CT.deadline = CurrentMs + 6000;
                break;
            }
            case CT_PRE:
                if (CT.havePre) {
                    std::string why;
                    CTLog("Schritt %02u: %s", (unsigned)(CT.idx + 1), t.name);   // vor der Handlung - zeigt bei Absturz den Schritt
                    if (!t.act(why)) {
                        CT.skip++;
                        CTResult("--", "uebersprungen: " + why);
                        CT.phase = CT_GAP;
                        CT.deadline = CurrentMs + 200;
                        break;
                    }
                    CT.phase = CT_WAIT;
                    CT.deadline = CurrentMs + t.waitMs;
                }
                else if (CurrentMs > CT.deadline) {
                    CT.fail++;
                    CT.fails.push_back(std::string(t.name) + ": Partner antwortet nicht");
                    CTResult("FEHLER", "Partner antwortet nicht (vorher)");
                    CT.phase = CT_GAP;
                    CT.deadline = CurrentMs + 200;
                }
                break;
            case CT_WAIT:
                if (CurrentMs >= CT.deadline) {
                    CTAsk("post");
                    CT.phase = CT_POST;
                    CT.deadline = CurrentMs + 6000;
                }
                break;
            case CT_POST:
                if (CT.havePost || CurrentMs > CT.deadline) {
                    std::string d;
                    const bool ok = CT.havePost && t.judge(CT.pre, CT.post, d);
                    if (!CT.havePost)
                        d = "Partner antwortet nicht (nachher)";
                    if (ok) {
                        CT.ok++;
                        CTResult(t.flags & 2 ? "OK (Luecke geschlossen?)" : "OK", d);
                    }
                    else if (d.compare(0, 3, "-- ") == 0) {   // v43: Urteil "nicht bewertbar"
                        CT.skip++;
                        CTResult("--", d.substr(3));
                    }
                    else if (t.flags & 2) {
                        CT.gap++;
                        CTResult("BEKANNT", d + " - bekannte Luecke");
                    }
                    else {
                        CT.fail++;
                        CT.fails.push_back(std::string(t.name) + ": " + d);
                        CTResult("FEHLER", d);
                    }
                    if (t.cleanup)
                        t.cleanup();
                    CT.phase = CT_GAP;
                    CT.deadline = CurrentMs + 500;
                }
                break;
            case CT_GAP:
                if (CurrentMs >= CT.deadline) {
                    CT.idx++;
                    CT.phase = CT_START;
                }
                break;
            }
        }
        catch (std::exception& ex) {
            CoopErrorLog("Coop-Test", ex.what(), CT.idx < CoopTestCount ? CoopTests[CT.idx].name : "-");
            CT.fail++;
            CT.phase = CT_GAP;
            CT.deadline = CurrentMs + 500;
        }
    }

    bool CoopSelfTestHandles(int type) {
        return type == SYNC_SELFTEST;
    }

    void CoopSelfTestReceive(json& e, const std::string& id) {
        try {
            const std::string f = e.value("f", std::string());
            if (f == "q") {
                const std::string from = e.value("from", id);
                if (from == DialogMe())
                    return;
                json a;
                a["run"] = e["run"];
                a["n"] = e["n"];
                a["ph"] = e["ph"];
                a["to"] = from;
                a["o"] = CoopTestObserve(e["q"], from);
                CTSend("a", a);
                if (e["ph"] == "pre" && e["n"].get<int>() == 0)
                    CTLog("Testlauf %s von %s - ich melde, was ich sehe", e["run"].get<std::string>().c_str(), from.c_str());
            }
            else if (f == "a") {
                if (!CT.on || e.value("to", std::string()) != DialogMe() || e.value("run", std::string()) != CT.tag
                    || e.value("n", -1) != (int)CT.idx)
                    return;
                if (e["ph"] == "pre" && CT.phase == CT_PRE && !CT.havePre) {
                    CT.pre = e["o"];
                    CT.havePre = true;
                }
                else if (e["ph"] == "post" && CT.phase == CT_POST && !CT.havePost) {
                    CT.post = e["o"];
                    CT.havePost = true;
                }
            }
        }
        catch (std::exception& ex) {
            CoopErrorLog("Coop-Test", ex.what(), id.c_str());
        }
    }

#else
    void CoopSelfTestToggle() {}
    void CoopSelfTestPulse() {}
    bool CoopSelfTestHandles(int) { return false; }
    void CoopSelfTestReceive(json&, const std::string&) {}
#endif
}
