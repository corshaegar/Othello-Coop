// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

    // --- v41: Abgleich ausserhalb der Reichweite - Verfolgungspunkt beim Mitspieler (19.10.2026) -----------------------------
    //
    // Nutzer: "wenn der Mitspieler unseren Radius verlaesst, soll es weiterhin synchronisiert bleiben - NPCs, die er toetet,
    // Quests, die er erledigt, und Monster. Fuer Quests bekommt man die EP, aber nicht fuer die Kills." Idee des Nutzers:
    // "einen Verfolgungspunkt aktivieren wie eine zweite Instanz, die dem Mitspieler folgt".
    //
    // Bisher: Der Host sendet nur NPCs im Umkreis seines eigenen Helden (GetVisibleNpcs, BROADCAST_DISTANCE 4500). Ist die
    // Figur eines Clients weiter weg, loeschte der Host sie (RespawnOrDestroyBasedOnDistance), seine Treffer verwarf
    // UpdateAttacks ("if (!hasModel) return") - Kills weit weg kamen beim Host nie an, die NPCs dort liefen beim Client nur lokal.
    // Und der Spawn-Manager des Hosts nimmt NPCs weit weg vom eigenen Helden ganz aus der Welt.
    //
    // Jetzt (Host und Client - Nutzer: "umgekehrt muss der Client auch den Host mit der Instanz verfolgen"; abschaltbar mit
    // "coopFollowPartner": false):
    // - Verfolgungspunkt: oCSpawnManager::CheckInsertNpcs (0x006CF9A0, je Spawn-Takt 1000 ms; setzt camPos +0x10 auf den Helden
    //   und springt nach CheckInsertNpc 0x006CFDE0) wird abgefangen. Nach jedem Takt laeuft CheckInsertNpc noch einmal mit camPos =
    //   Lage jeder weit entfernten Figur - InsertNpc (0x006D0250) prueft den Abstand selbst gegen camPos (insertRange 4500,
    //   0x00850FB4). Die Wartezeiten der Knoten (+0x10) zaehlt CheckInsertNpc herunter (Hauptschleife und Othellos Anhang ab
    //   +0x409); sie werden um den zusaetzlichen Durchlauf gesichert und zurueckgesetzt.
    // - oCSpawnManager::CheckRemoveNpc (0x006D0A80, je NPC aus der Menschen-KI, 1 = entfernt) wird abgefangen: NPCs im
    //   removeRange (5000, 0x00850FB8) einer Figur und die Figuren selbst bleiben in der Welt.
    // - Eingefuegte NPCs laufen voll (zCVob::DoFrameActivity: Schlafmodus 1 = KI + Animation; nichts schaltet NPCs nach
    //   Kameraabstand auf "nur KI" - gesucht im Speicherabbild). Othellos Eingriff in oCNpc::ProcessNpc gilt nur dem Helden.
    // - Senden: GetVisibleNpcs (Host) nimmt NPCs im Umkreis jeder Figur mit (Helpers.cpp); die Figuren bleiben auf beiden Seiten
    //   immer in der Welt (RemoteNpc.cpp) - damit gelten Treffer, Kaempfe und Tode dort wie in der Naehe. Beim Client halten
    //   dieselben Haken die NPCs um die Host-Figur aktiv, und deren RemoteNpc wird nicht mehr nach 1,5 x Reichweite verworfen.
    // - EP: Kills ausserhalb der Reichweite ohne EP fuer den anderen (CoopKillNoXp: Othellos ZS_Dead gibt EP nur, wenn der Taeter
    //   Spieler ist oder AIV_PARTYMEMBER (36) bzw. AIV_MM_PARTYMEMBER (23) traegt - die Merker des Taeters stehen dabei kurz auf 0).
    //   Quest-EP aus Dialogen bekommen beide wie bisher (D11). In der Naehe bleibt es bei geteilten Kill-EP.
    // - Todesabgleich als Netz: Der Host meldet jeden Tod eines benannten NPCs (Takt 1 s, Paket 51 NPC_DEATH), beim RESYNC die
    //   ganze Liste; der Client toetet ihn ohne EP, falls er bei ihm noch lebt (auch Tode, die er nie gesehen hat).

#if ENGINE == Engine_G1

    static bool FarCfgRead = false;
    static bool FarFollow = true;
    static long long FarNextScanMs = 0, FarNextLogMs = 0;
    static std::map<oCNpc*, std::string> FarAlive;      // Host: zuletzt lebend gesehene benannte NPCs
    static bool FarScanReady = false;
    static std::set<std::string> FarKnownDead;          // Client: vom Host als tot gemeldet
    static std::map<std::string, long long> FarPending; // Client: Name -> bis wann noch versuchen
    static unsigned long FarInsertedSinceLog = 0, FarKeptSinceLog = 0, FarDeathsSent = 0, FarDeathsApplied = 0;
    static unsigned long FarLedInsertedSinceLog = 0;    // v43: Client - vom Host gefuehrte NPCs an seiner Lage eingesetzt
    static std::map<oCNpc*, zVEC3> FarMoveProbe;        // Lage der NPCs um die Figur beim letzten Bericht
    static std::map<oCNpc*, zVEC3> FarMoveProbeMe;      // v43: dasselbe um den eigenen Helden (Vergleichswert)

    static void FarLog(const char* fmt, ...) {
        char buf[1024];
        va_list ap;
        va_start(ap, fmt);
        _vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        buf[sizeof(buf) - 1] = 0;
        OthelloCompatLog("[%lld s, %s] Fern-Abgleich: %s", DiagSeconds(), DiagRole(), buf);
    }

    static void FarReadConfig() {
        if (FarCfgRead)
            return;
        FarCfgRead = true;
        if (CoopConfig.contains("coopFollowPartner") && CoopConfig["coopFollowPartner"].is_boolean())
            FarFollow = CoopConfig["coopFollowPartner"].get<bool>();
        FarLog("Verfolgungspunkt beim Mitspieler %s (coopFollowPartner)", FarFollow ? "an" : "aus");
    }

    // Verfolgungspunkt an (Host und Client - Nutzer: "umgekehrt muss der Client auch den Host mit der Instanz verfolgen"),
    // verbunden, Welt geladen.
    bool CoopFollowActive() {
        FarReadConfig();
        return FarFollow && (ServerThread || (ClientThread && !IsCoopPaused)) && !(MyselfId == "_player_") && !IsLoadingLevel
            && !WorldEditMode && player && ogame && ogame->GetGameWorld();
    }

    static float FarDist(const zVEC3& a, const zVEC3& b) {
        const float dx = a.n[0] - b.n[0], dy = a.n[1] - b.n[1], dz = a.n[2] - b.n[2];
        return sqrtf(dx * dx + dy * dy + dz * dz);
    }

    // Lage der Figuren der anderen Spieler (Host: alle Clients; Client: Host und weitere Clients). Bevorzugt die zuletzt
    // gemeldete Lage.
    int CoopFollowPoints(zVEC3* out, int max) {
        if (!CoopFollowActive())
            return 0;
        int n = 0;
        for (auto& kv : PlayerNpcs) {
            if (n >= max)
                break;
            oCNpc* f = kv.first;
            if (!f)
                continue;
            auto it = SyncNpcs.find(kv.second);
            if (it != SyncNpcs.end() && it->second && it->second->lastPositionFromServer)
                out[n++] = *it->second->lastPositionFromServer;
            else
                out[n++] = f->GetPositionWorld();
        }
        return n;
    }

    bool CoopFollowNear(const zVEC3& p, float range) {
        zVEC3 pts[8];
        const int n = CoopFollowPoints(pts, 8);
        for (int i = 0; i < n; i++)
            if (FarDist(pts[i], p) < range)
                return true;
        return false;
    }

    // Figur/NPC ausserhalb der eigenen Reichweite (fuer die EP-Regel bei Kills). v67: Reichweite aus dem Schwierigkeitsgrad
    // (CoopDifficulty.cpp, Vorgabe 45 m = BROADCAST_DISTANCE wie bisher; "ueberall" = nie zu weit).
    float CoopDiffXpRangeCm();
    bool CoopFarFromMe(oCNpc* npc) {
        if (!npc || !player)
            return false;
        const float range = CoopDiffXpRangeCm();
        if (range < 0.f)
            return false;
        return FarDist(npc->GetPositionWorld(), player->GetPositionWorld()) > range;
    }

    static bool FarActiveInWorld(oCNpc* npc) {
        return npc && npc->GetHomeWorld() && npc->vobLeafList.GetNum() > 0 && npc->GetModel();
    }

    // Figur an pos in die Welt setzen, auch weit weg vom eigenen Helden. InsertNpc (0x006D0250) prueft den Abstand gegen camPos
    // und setzt nur ein, wenn oCNpc::spawnInRange (+0x79C Bit 0x2000, 0x006D0301) gesetzt ist - sonst setzt es das Bit und
    // liefert 0. Darum camPos = pos und das Bit vorher setzen.
    bool CoopSpawnInsertAt(oCNpc* npc, const zVEC3& pos) {
        oCSpawnManager* sm = ogame ? ogame->spawnman : NULL;
        if (!sm || !npc)
            return false;
        const zVEC3 cam = sm->camPos;
        sm->camPos = pos;
        *(DWORD*)((BYTE*)npc + 0x79C) |= 0x2000;
        const int ok = sm->InsertNpc(npc, pos);
        sm->camPos = cam;
        return ok != 0;
    }

    // Toeten ohne EP fuer den eigenen Helden: Taeter = killer (oder das Opfer selbst), seine Partei-Merker kurz 0.
    void CoopKillNoXp(oCNpc* npc, oCNpc* killer, const char* why) {
        if (!npc || npc->IsDead() || npc == player)
            return;
        static int pm = -2, mmpm = -2;
        if (pm == -2) {
            pm = GetPartyMemberID();
            zCPar_Symbol* s = parser->GetSymbol("AIV_MM_PARTYMEMBER");
            mmpm = -1;
            if (s)
                s->GetValue(mmpm, 0);
            FarLog("EP-Merker: AIV_PARTYMEMBER %d, AIV_MM_PARTYMEMBER %d", pm, mmpm);
        }
        oCNpc* k = killer ? killer : npc;
        const int savePm = (pm >= 0 && pm < 50) ? k->aiscriptvars[pm] : 0;
        const int saveMm = (mmpm >= 0 && mmpm < 50) ? k->aiscriptvars[mmpm] : 0;
        if (pm >= 0 && pm < 50)
            k->aiscriptvars[pm] = 0;
        if (mmpm >= 0 && mmpm < 50)
            k->aiscriptvars[mmpm] = 0;
        const bool active = FarActiveInWorld(npc);
        const bool replay = ClientThread && !ServerThread;   // der Host hat diesen Tod (und sein ZS_Dead) schon ausgefuehrt
        npc->SetAttribute(NPC_ATR_HITPOINTS, 0);
        if (replay)
            CoopReplayDeathDepth++;
        if (active)
            npc->DoDie(k);   // vtable -> OTHELLO_ABI+0x7C3F0 (wie C7); ohne aktive Welt reicht HP 0 (CheckInsertNpc setzt Tote nicht ein)
        if (replay)
            CoopReplayDeathDepth--;
        if (pm >= 0 && pm < 50)
            k->aiscriptvars[pm] = savePm;
        if (mmpm >= 0 && mmpm < 50)
            k->aiscriptvars[mmpm] = saveMm;
        FarDeathsApplied++;
        static unsigned long logged = 0;
        if (logged++ < 60) {
            auto it = NpcToUniqueNameList.find(npc);
            FarLog("%s tot ohne EP (%s, %s)", it != NpcToUniqueNameList.end() ? it->second.ToChar() : npc->GetObjectName().ToChar(),
                why, active ? "in der Welt: DoDie" : "nicht in der Welt: HP 0");
        }
    }

    bool CoopFarKnownDead(const std::string& name) {
        return FarKnownDead.count(name) != 0;
    }

    // v43: steht der NPC in der Spawn-Liste (dann setzt der Spawn-Manager ihn selbst ein)?
    bool CoopSpawnListHas(oCNpc* npc) {
        oCSpawnManager* sm = ogame ? ogame->spawnman : NULL;
        if (!sm || !npc)
            return false;
        for (int k = 0; k < sm->spawnList.GetNum(); k++)
            if (oCSpawnManager::oSSpawnNode* node = sm->spawnList[k])
                if (node->npc == npc)
                    return true;
        return false;
    }

    void CoopFarCountLedInsert() {
        FarLedInsertedSinceLog++;
    }

    // --- Haken auf den Spawn-Manager (Host) -----------------------------------------------------------------------------------

    void __fastcall oCSpawnManager_CheckInsertNpcs_Coop(oCSpawnManager* _this, void* vtable);
    CInvoke<void(__thiscall*)(oCSpawnManager*)> Ivk_oCSpawnManager_CheckInsertNpcs_Coop(0x006CF9A0, &oCSpawnManager_CheckInsertNpcs_Coop);
    void __fastcall oCSpawnManager_CheckInsertNpcs_Coop(oCSpawnManager* _this, void* vtable) {
        const float before = _this->insertTime;
        Ivk_oCSpawnManager_CheckInsertNpcs_Coop(_this);
        if (!_this->spawningEnabled || !(_this->insertTime < before))
            return;   // kein Spawn-Takt in diesem Bild
        try {
            zVEC3 pts[9];
            int n = CoopFollowPoints(pts, 8);
            // v44: Kamera beim Mitspieler - der Spawn-Manager nimmt die Kamera als Mittelpunkt (camVob, +0x36), also den
            // eigenen Helden als weiteren Punkt (CoopCamera.cpp)
            if (CoopCamWatching() && player)
                pts[n++] = player->GetPositionWorld();
            if (n <= 0)
                return;
            const zVEC3 cam = _this->camPos;
            const float ins = oCSpawnManager::GetInsertRange();
            for (int i = 0; i < n; i++) {
                if (FarDist(pts[i], cam) < ins)
                    continue;   // Figur in der Naehe: der normale Durchlauf deckt sie ab
                std::map<oCSpawnManager::oSSpawnNode*, float> timers;
                for (int k = 0; k < _this->spawnList.GetNum(); k++)
                    if (oCSpawnManager::oSSpawnNode* node = _this->spawnList[k])
                        timers[node] = node->timer;
                const int nodesBefore = _this->spawnList.GetNum();
                _this->camPos = pts[i];
                _this->CheckInsertNpc();
                _this->camPos = cam;
                for (int k = 0; k < _this->spawnList.GetNum(); k++) {
                    oCSpawnManager::oSSpawnNode* node = _this->spawnList[k];
                    auto t = node ? timers.find(node) : timers.end();
                    if (t != timers.end())
                        node->timer = t->second;
                }
                if (nodesBefore > _this->spawnList.GetNum())
                    FarInsertedSinceLog += nodesBefore - _this->spawnList.GetNum();
            }
        }
        catch (...) {
            CoopErrorLog("Fern-Abgleich", "Ausnahme", "CheckInsertNpcs");
        }
    }

    int __fastcall oCSpawnManager_CheckRemoveNpc_Coop(oCSpawnManager* _this, void* vtable, oCNpc* npc);
    CInvoke<int(__thiscall*)(oCSpawnManager*, oCNpc*)> Ivk_oCSpawnManager_CheckRemoveNpc_Coop(0x006D0A80, &oCSpawnManager_CheckRemoveNpc_Coop);
    int __fastcall oCSpawnManager_CheckRemoveNpc_Coop(oCSpawnManager* _this, void* vtable, oCNpc* npc) {
        // v44: Kamera beim Mitspieler - der Mittelpunkt ist dann die Kamera (camVob, CheckRemoveNpc +0x64); NPCs um den eigenen
        // Helden bleiben trotzdem
        if (npc && npc != player && player && CoopCamWatching() && !npc->IsDead() &&
            FarDist(npc->GetPositionWorld(), player->GetPositionWorld()) < oCSpawnManager::GetRemoveRange()) {
            FarKeptSinceLog++;
            return 0;
        }
        if (npc && npc != player && CoopFollowActive()) {
            if (PlayerNpcs.count(npc))
                return 0;   // Figur eines Mitspielers: immer in der Welt lassen
            if (!npc->IsDead() && CoopFollowNear(npc->GetPositionWorld(), oCSpawnManager::GetRemoveRange())) {
                FarKeptSinceLog++;
                return 0;   // im Umkreis eines Mitspielers: nicht entfernen
            }
        }
        return Ivk_oCSpawnManager_CheckRemoveNpc_Coop(_this, npc);
    }

    // --- Todesabgleich ---------------------------------------------------------------------------------------------------

    static void FarSendDeaths(const std::vector<std::string>& names, bool all) {
        if (names.empty() && !all)
            return;
        json j;
        j["type"] = SYNC_NPC_DEATH;
        j["id"] = MyselfId.ToChar();
        j["w"] = CoopWorldName();
        j["d"] = names;
        if (all)
            j["all"] = 1;
        CoopTraceCount(TRACE_SENT, SYNC_NPC_DEATH);
        ReadyToSendJsons.enqueue(j);
        FarDeathsSent += (unsigned long)names.size();
    }

    // Host: beim RESYNC eines Clients alle toten benannten NPCs.
    void CoopFarSyncSendAllDead() {
        if (!ServerThread || !ogame || !ogame->GetGameWorld())
            return;
        std::vector<std::string> dead;
        for (auto* l = CoopWorldNpcListFirst(ogame->GetGameWorld()); l; l = l->next) {
            oCNpc* npc = l->data;
            if (!npc || npc == player || PlayerNpcs.count(npc) || !npc->IsDead())
                continue;
            auto it = NpcToUniqueNameList.find(npc);
            if (it != NpcToUniqueNameList.end())
                dead.push_back(std::string(it->second.ToChar()));
        }
        FarSendDeaths(dead, true);
        FarLog("Host: volle Todesliste an die Clients (%u NPCs)", (unsigned)dead.size());
    }

    static void FarScanDeaths() {
        std::map<oCNpc*, std::string> alive;
        std::vector<std::string> died;
        for (auto* l = CoopWorldNpcListFirst(ogame->GetGameWorld()); l; l = l->next) {
            oCNpc* npc = l->data;
            if (!npc || npc == player || PlayerNpcs.count(npc))
                continue;
            auto it = NpcToUniqueNameList.find(npc);
            if (it == NpcToUniqueNameList.end())
                continue;
            const std::string name = it->second.ToChar();
            if (!npc->IsDead())
                alive[npc] = name;
            else if (FarScanReady) {
                auto was = FarAlive.find(npc);
                if (was != FarAlive.end() && was->second == name)
                    died.push_back(name);
            }
        }
        FarAlive.swap(alive);
        FarScanReady = true;
        if (!died.empty()) {
            FarSendDeaths(died, false);
            FarLog("Host: %u Tod(e) an die Clients gemeldet (z. B. %s)", (unsigned)died.size(), died[0].c_str());
        }
    }

    static void FarApplyPending() {
        const long long now = GetCurrentMs();
        for (auto it = FarPending.begin(); it != FarPending.end();) {
            auto n = UniqueNameToNpcList.find(string(it->first.c_str()));
            oCNpc* npc = n != UniqueNameToNpcList.end() ? n->second : NULL;
            if (npc && npc->IsDead()) {
                it = FarPending.erase(it);
                continue;
            }
            if (npc && !IsPlayerTalkingWithNpc(npc)) {
                CoopKillNoXp(npc, NULL, "vom Host gemeldet");
                it = FarPending.erase(it);
                continue;
            }
            if (now > it->second) {
                FarLog("Client: gemeldeten Tod von %s nicht angewendet (%s)", it->first.c_str(), npc ? "Dialog" : "hier unbekannt");
                it = FarPending.erase(it);
                continue;
            }
            ++it;
        }
    }

    void CoopFarSyncReceive(json& e, const std::string& id) {
        CoopTraceCount(TRACE_RECV, SYNC_NPC_DEATH);
        if (ServerThread || !ClientThread || IsLoadingLevel)
            return;   // nur der Host meldet; Clients wenden an
        const std::string w = e.contains("w") && e["w"].is_string() ? e["w"].get<std::string>() : std::string();
        if (w != CoopWorldName())
            return;
        if (!e.contains("d") || !e["d"].is_array())
            return;
        const long long until = GetCurrentMs() + 15000;
        int n = 0;
        for (auto& d : e["d"]) {
            if (!d.is_string())
                continue;
            const std::string name = d.get<std::string>();
            FarKnownDead.insert(name);
            FarPending[name] = until;
            n++;
        }
        FarApplyPending();
        if (!e.contains("all") || n)
            FarLog("Client: %d Tod(e) vom Host%s", n, e.contains("all") ? " (volle Liste)" : "");
    }

    // Gerade abgeglichen? Host: wird gesendet. Client: vom Host gefuehrt (RemoteNpc mit Modell) - sonst sperrt der
    // StartAni-Haken (Hooks.cpp) beim Client jede eigene Animation, der NPC steht dann still.
    static bool FarIsLed(oCNpc* npc) {
        auto nm = NpcToUniqueNameList.find(npc);
        if (nm == NpcToUniqueNameList.end())
            return false;
        if (ServerThread)
            return BroadcastNpcs.count(nm->second) != 0;
        auto it = SyncNpcs.find(nm->second);
        return it != SyncNpcs.end() && it->second && !it->second->destroyed && it->second->hasModel;
    }

    // Aktive NPCs um c: wie viele, wie viele seit dem letzten Bericht bewegt (> 50 cm), wie viele ohne Abgleich.
    static void FarProbe(const zVEC3& c, std::map<oCNpc*, zVEC3>& last, int& active, int& moved, int& notLed) {
        active = moved = notLed = 0;
        std::map<oCNpc*, zVEC3> probe;
        for (auto* l = CoopWorldNpcListFirst(ogame->GetGameWorld()); l; l = l->next) {
            oCNpc* npc = l->data;
            if (!npc || npc == player || PlayerNpcs.count(npc) || npc->vobLeafList.GetNum() == 0 || npc->IsDead())
                continue;
            const zVEC3 p = npc->GetPositionWorld();
            if (FarDist(p, c) >= (float)BROADCAST_DISTANCE)
                continue;
            active++;
            auto was = last.find(npc);
            if (was != last.end() && FarDist(was->second, p) > 50.f)
                moved++;
            if (!FarIsLed(npc))
                notLed++;
            probe[npc] = p;
        }
        last.swap(probe);
    }

    static void FarReport() {
        if (!CoopFollowActive())
            return;
        zVEC3 pts[8];
        const int n = CoopFollowPoints(pts, 8);
        const zVEC3 me = player->GetPositionWorld();
        int meAct = 0, meMoved = 0, meNotLed = 0;   // v43: Vergleichswert um den eigenen Helden
        FarProbe(me, FarMoveProbeMe, meAct, meMoved, meNotLed);
        for (int i = 0; i < n; i++) {
            const float d = FarDist(pts[i], me);
            int active = 0, moved = 0, notLed = 0;
            FarProbe(pts[i], FarMoveProbe, active, moved, notLed);
            int synced = 0;   // Client: vom Host gefuehrte NPCs (RemoteNpc) - auch um die Host-Figur
            for (auto& s : SyncNpcs)
                if (s.second && !s.second->destroyed && s.second->hasModel)
                    synced++;
            FarLog("Verfolgungspunkt %d bei %.0f/%.0f/%.0f, %.0f m von mir - %d NPCs aktiv in seinem Umkreis, %d davon bewegt seit dem"
                " letzten Bericht, %d ohne Abgleich (%s); bei mir %d aktiv, %d bewegt, %d ohne Abgleich; eingesetzt %lu, behalten %lu,"
                " vom Host gefuehrte nachgesetzt %lu; gesendet %u, vom Host gefuehrt %d NPCs; Tode gemeldet %lu, angewendet %lu",
                i + 1, pts[i].n[0], pts[i].n[1], pts[i].n[2], d / 100.f, active, moved, notLed,
                ServerThread ? "nicht gesendet" : "nicht vom Host gefuehrt - stehen hier still", meAct, meMoved, meNotLed,
                FarInsertedSinceLog, FarKeptSinceLog, FarLedInsertedSinceLog, (unsigned)BroadcastNpcs.size(), synced,
                FarDeathsSent, FarDeathsApplied);
        }
        FarInsertedSinceLog = FarKeptSinceLog = FarLedInsertedSinceLog = 0;
    }

    // Aus Game_Loop (nach BuildGlobalNpcList).
    void CoopFarSyncPulse() {
        FarReadConfig();
        if (!(ServerThread || ClientThread) || IsLoadingLevel || !player || !ogame || !ogame->GetGameWorld() || MyselfId == "_player_")
            return;
        const long long now = GetCurrentMs();
        if (now >= FarNextScanMs) {
            FarNextScanMs = now + 1000;
            if (ServerThread)
                FarScanDeaths();
            else if (!FarPending.empty())
                FarApplyPending();
        }
        if (now >= FarNextLogMs) {
            FarNextLogMs = now + 30000;
            if (!PlayerNpcs.empty())
                FarReport();
        }
    }

    // Aus LoadEnd: neue Welt.
    void CoopFarSyncWorldLoaded() {
        FarAlive.clear();
        FarScanReady = false;
        FarKnownDead.clear();
        FarPending.clear();
        FarMoveProbe.clear();
        FarMoveProbeMe.clear();
    }

#else
    bool CoopFollowActive() { return false; }
    int CoopFollowPoints(zVEC3*, int) { return 0; }
    bool CoopFollowNear(const zVEC3&, float) { return false; }
    bool CoopFarFromMe(oCNpc*) { return false; }
    bool CoopSpawnInsertAt(oCNpc*, const zVEC3&) { return false; }
    void CoopKillNoXp(oCNpc* npc, oCNpc*, const char*) { if (npc) npc->SetAttribute(NPC_ATR_HITPOINTS, 0); }
    bool CoopFarKnownDead(const std::string&) { return false; }
    bool CoopSpawnListHas(oCNpc*) { return false; }
    void CoopFarCountLedInsert() {}
    void CoopFarSyncSendAllDead() {}
    void CoopFarSyncReceive(json&, const std::string&) {}
    void CoopFarSyncPulse() {}
    void CoopFarSyncWorldLoaded() {}
#endif
}
