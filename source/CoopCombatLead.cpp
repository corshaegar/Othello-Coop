// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

    // --- v72: Kampf-Uebergabe - der Client rechnet NPCs, die gegen ihn kaempfen (21.10.2026) --------------------------------
    //
    // Nutzer: "Als naechstes implementiere die Client-Trefferlogik. Also der Client soll den Kampf selbst berechnen, unabhaengig
    // vom Host. Somit haetten wir die beste Trefferlogik." Gewaehlt: Kampf-Uebergabe. Und: "es gab schon vorher Bugs, bei denen
    // der Client gegen Monster kaempfen musste, die beim Host nicht waren - deren Kampflogik wurde bereits vom Client komplett
    // uebernommen. Jetzt muss man das quasi uebertragen auf NPCs/Monster, die wirklich beim Host sind."
    //
    // Bis v71 fuehrte der Host jeden NPC: seine KI entschied beim Host, wann er zuschlaegt, getroffen wurde dort die (verzoegerte)
    // Figur des Clients; erst dann pruefte der Client (v53, CoopHitAuthority.cpp). Beim Client lief fuer die NPC-Kopie keine
    // Kampf-KI (C16), Animationen und Lage kamen vom Host.
    //
    // Jetzt, allgemein (GT): Greift ein vom Host gefuehrter NPC die Figur eines Clients an (Angriffszustand ZS_ATTACK, ZS_MM_ATTACK,
    // ZS_MM_ATTACKMAGE oder ZS_ORC_ATTACK, Gegner = Figur, hoechstens 30 m entfernt), gibt der Host ihn an diesen Client ab:
    // - Host: NPC aus BroadcastNpcs (er sendet ihn nicht mehr), KI angehalten (Auftraege und Zustand geloescht; Kampfzustaende
    //   und eigene Animationen wie beim Client gesperrt - Hooks.cpp), er wird Puppe: RemoteNpc wendet Lage, Blickrichtung,
    //   Animationen, Waffenmodus und HP vom Client an. Paket NPC_LEAD "give" an den Client.
    // - Client: RemoteNpc weg, eigener LocalNpc sendet den NPC (Pakete mit "lead"; der Host prueft den Absender und verteilt sie mit
    //   id = NPC an die anderen). Die eigene KI laeuft (Gegner = eigener Held, Kampfzustand wie beim Host) - Schlaege, Paraden,
    //   Treffer und Schaden rechnet seine Engine, ohne Umweg ueber den Host. Treffer am eigenen Helden meldet er wie bisher
    //   (SYNC_HIT_RESULT); eigene Treffer auf den NPC ebenso (Host spielt sie nach - EP, Bewusstlos, Tod wie bisher).
    // - Rueckgabe ("back" mit Lage, HP, Zustand und Ziel): NPC tot oder bewusstlos (nach 0,6 s, damit die Treffer vorher beim Host
    //   sind), Ziel ist nicht mehr der eigene Held (z. B. vom Host getroffen - wie beim Host mit 2/3 Wahrscheinlichkeit), 2 s kein
    //   Kampfzustand, weiter als 45 m, Pause, oder der NPC ist weg. Der Host setzt die KI mit diesem Ziel fort; ist es die Figur
    //   eines anderen Clients, geht der NPC dorthin weiter. Trennt der Client oder spricht der Host-Held mit dem NPC, holt der
    //   Host ihn zurueck ("take").
    // Abschaltbar: GothicCoopConfig.json "coopCombatLead": false (dann wie v71).

    // Aus Hooks.cpp / PacketProcessor.cpp / Server.cpp / Plugin.cpp / VisibleNpcsUpdater.cpp / CoopDifficulty.cpp
    bool CoopLeadIsRemote(oCNpc* npc);
    bool CoopLeadIsMine(oCNpc* npc);

#if ENGINE == Engine_G1

    struct CoopLeadHostEntry {
        std::string owner;   // FRIEND_n
        oCNpc* npc;
        long long since;
    };
    struct CoopLeadMineEntry {
        oCNpc* npc;
        LocalNpc* local;
        long long since;
        long long downSince;   // tot/bewusstlos seit
        long long idleSince;   // ohne Kampfzustand seit
    };
    static std::map<std::string, CoopLeadHostEntry> CoopLeadsHost;   // Host: NPC-Name -> fuehrender Client
    static std::map<std::string, long long> CoopLeadCooldown;        // Host: NPC-Name -> nicht vor ... wieder abgeben
    static std::map<std::string, CoopLeadMineEntry> CoopLeadsMine;   // Client: selbst gefuehrte NPCs
    static std::set<oCNpc*> CoopLeadRemoteSet, CoopLeadMineSet;       // schnelle Abfrage in den Haken
    static bool CoopLeadCfgRead = false, CoopLeadOn = true;
    static long long CoopLeadNextHostMs = 0, CoopLeadNextMineMs = 0;
    static unsigned long CoopLeadGiven = 0, CoopLeadReturned = 0;
    static const float CoopLeadGiveRange = 3000.f;   // Abstand NPC - Figur beim Abgeben
    static const float CoopLeadKeepRange = 4500.f;   // Abstand NPC - eigener Held, bis der Client zurueckgibt
    static const long long CoopLeadHoldMs = 1500;    // so lange mindestens behalten (ausser tot/bewusstlos/weg)

    static void CoopLeadReadConfig() {
        if (CoopLeadCfgRead)
            return;
        CoopLeadCfgRead = true;
        if (CoopConfig.contains("coopCombatLead") && CoopConfig["coopCombatLead"].is_boolean())
            CoopLeadOn = CoopConfig["coopCombatLead"].get<bool>();
        OthelloCompatLog("[%lld s, %s] Kampf-Uebergabe: %s (coopCombatLead)", DiagSeconds(), DiagRole(), CoopLeadOn ? "an" : "aus");
    }

    static void LeadLog(const char* fmt, ...) {
        static CoopDiagBudget budget = { 0, 0 };
        if (!CoopDiagAllow(budget, 60))
            return;
        char buf[700];
        va_list ap;
        va_start(ap, fmt);
        _vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        buf[sizeof(buf) - 1] = 0;
        OthelloCompatLog("[%lld s, %s] Kampf-Uebergabe: %s", DiagSeconds(), DiagRole(), buf);
    }

    static void LeadSend(json& j) {
        j["type"] = SYNC_NPC_LEAD;
        j["id"] = MyselfId.ToChar();
        CoopTraceCount(TRACE_SENT, SYNC_NPC_LEAD);
        ReadyToSendJsons.enqueue(j);
    }

    // Angriffszustaende, in denen der Host abgibt
    static bool LeadAttackState(const char* n) {
        return n && (_stricmp(n, "ZS_ATTACK") == 0 || _stricmp(n, "ZS_MM_ATTACK") == 0 || _stricmp(n, "ZS_MM_ATTACKMAGE") == 0 ||
            _stricmp(n, "ZS_ORC_ATTACK") == 0);
    }

    // Kampf- und Reaktionszustaende (wie C16, Hooks.cpp) - so lange behaelt der Client den NPC
    static bool LeadCombatState(const char* n) {
        static const char* list[] = { "ZS_ATTACK", "ZS_ASSESSENEMY", "ZS_ASSESSFIGHTER", "ZS_ASSESSFIGHTERWAIT", "ZS_REACTTODAMAGE",
            "ZS_PROCLAIMANDPUNISH", "ZS_FLEE", "ZS_GOTOFIGHT", "ZS_WATCHFIGHT", "ZS_ORC_WATCHFIGHT", "ZS_BERZERK",
            "ZS_MM_ATTACK", "ZS_MM_ATTACKMAGE", "ZS_MM_ASSESSENEMY", "ZS_MM_FLEE", "ZS_ORC_ATTACK" };
        if (!n)
            return false;
        for (const char* s : list)
            if (_stricmp(n, s) == 0)
                return true;
        return false;
    }

    static int LeadAivIndex(const char* name, int fallback) {
        zCPar_Symbol* s = parser ? parser->GetSymbol(name) : NULL;
        int v = fallback;
        if (s)
            s->GetValue(v, 0);
        return (v >= 0 && v < 50) ? v : -1;
    }

    // Begleiter und Beschwoerungen des Spielers (AIV_PARTYMEMBER / AIV_MM_PARTYMEMBER) bleiben beim Host
    static bool LeadIsParty(oCNpc* npc) {
        static int aivParty = -2, aivMmParty = -2;
        if (aivParty == -2) {
            aivParty = LeadAivIndex("AIV_PARTYMEMBER", 36);
            aivMmParty = LeadAivIndex("AIV_MM_PARTYMEMBER", 23);
        }
        return (aivParty >= 0 && npc->aiscriptvars[aivParty]) || (aivMmParty >= 0 && npc->aiscriptvars[aivMmParty]);
    }

    static oCNpc* LeadNpcByName(const std::string& n) {
        auto it = UniqueNameToNpcList.find(string(n.c_str()));
        return it != UniqueNameToNpcList.end() ? it->second : NULL;
    }

    // Name fuer das Paket: eigener Held = eigene Kennung, Figuren = Spielername, sonst GT-Name des NPCs
    static std::string LeadNameOf(oCNpc* n) {
        if (!n)
            return std::string();
        if (n == player)
            return std::string(MyselfId.ToChar());
        auto p = PlayerNpcs.find(n);
        if (p != PlayerNpcs.end())
            return std::string(p->second.ToChar());
        auto u = NpcToUniqueNameList.find(n);
        return u != NpcToUniqueNameList.end() ? std::string(u->second.ToChar()) : std::string();
    }

    // Gegenstueck: Spielername/GT-Name -> NPC hier ("HOST" ist beim Host der eigene Held)
    static oCNpc* LeadTargetByName(const std::string& n) {
        if (n.empty())
            return NULL;
        if (n == MyselfId.ToChar())
            return player;
        auto p = PlayerNameToNpc.find(string(n.c_str()));
        if (p != PlayerNameToNpc.end())
            return p->second;
        return LeadNpcByName(n);
    }

    static bool LeadInWorld(oCNpc* npc) {
        return npc && npc->GetHomeWorld() && npc->vobLeafList.GetNum() > 0 && npc->GetModel();
    }

    // RemoteNpc eines NPCs entfernen, ohne den NPC zu loeschen (DESTROY_NPC wuerde ihn aus der Welt nehmen)
    static void LeadDropRemote(const std::string& n) {
        auto s = SyncNpcs.find(string(n.c_str()));
        if (s == SyncNpcs.end())
            return;
        RemoteNpc* r = s->second;
        SyncNpcs.erase(s);
        delete r;
    }

    // Lage hart setzen wie RemoteNpc::UpdateNpcPosition bei grossem Abstand
    static void LeadSetPos(oCNpc* npc, const zVEC3& pos) {
        if ((pos - npc->GetPositionWorld()).LengthApprox() < 50.f)
            return;
        const bool inMove = npc->isInMovementMode != 0;
        if (inMove)
            npc->EndMovement();
        npc->SetCollDet(FALSE);
        npc->trafoObjToWorld.SetTranslation(pos);
        npc->SetCollDet(TRUE);
        if (inMove)
            npc->BeginMovement();
    }

    bool CoopLeadIsRemote(oCNpc* npc) {
        return npc && ServerThread && CoopLeadRemoteSet.count(npc) != 0;
    }

    bool CoopLeadIsMine(oCNpc* npc) {
        return npc && ClientThread && !ServerThread && CoopLeadMineSet.count(npc) != 0;
    }

    // ===== Host =====================================================================================================

    static void LeadGive(oCNpc* npc, const std::string& n, const std::string& owner, const std::string& st, float dist) {
        CoopLeadsHost[n] = { owner, npc, CurrentMs };
        CoopLeadRemoteSet.insert(npc);
        auto b = BroadcastNpcs.find(string(n.c_str()));
        if (b != BroadcastNpcs.end()) {
            if (b->second)
                b->second->destroyed = true;
            BroadcastNpcs.erase(b);
        }
        if (!SyncNpcs.count(string(n.c_str())) || !SyncNpcs[string(n.c_str())])
            addSyncedNpc(string(n.c_str()));
        json j;
        j["op"] = "give";
        j["to"] = owner;
        j["n"] = n;
        j["st"] = st;
        j["hp"] = npc->GetAttribute(NPC_ATR_HITPOINTS);
        j["hpmax"] = npc->GetAttribute(NPC_ATR_HITPOINTSMAX);
        j["wm"] = npc->GetWeaponMode();
        LeadSend(j);
        // ab jetzt Puppe: eigene KI anhalten (Kampfzustaende und eigene Animationen sperrt Hooks.cpp)
        npc->GetEM()->KillMessages();
        npc->ClearEM();
        npc->state.ClearAIState();
        CoopLeadGiven++;
        LeadLog("%s an %s uebergeben (%s, %.1f m) - dessen PC rechnet jetzt KI, Schlaege und Treffer", n.c_str(), owner.c_str(),
            st.c_str(), dist / 100.f);
    }

    // Host nimmt den NPC zurueck (ohne Daten vom Client): KI frei, Tagesablauf - die Wahrnehmungen holen ihn ggf. wieder in den Kampf
    static void LeadHostReclaim(std::map<std::string, CoopLeadHostEntry>::iterator it, const char* why, bool tell) {
        const std::string n = it->first;
        const std::string owner = it->second.owner;
        oCNpc* npc = it->second.npc;
        CoopLeadRemoteSet.erase(npc);
        CoopLeadsHost.erase(it);
        CoopLeadCooldown[n] = CurrentMs + 3000;
        LeadDropRemote(n);
        if (tell) {
            json j;
            j["op"] = "take";
            j["to"] = owner;
            j["n"] = n;
            LeadSend(j);
        }
        if (npc && LeadNpcByName(n) == npc && !npc->IsDead() && !npc->IsUnconscious()) {
            npc->GetEM()->KillMessages();
            npc->ClearEM();
            npc->state.ClearAIState();
        }
        LeadLog("%s von %s zurueckgeholt (%s)", n.c_str(), owner.c_str(), why);
    }

    static void LeadHostPulse() {
        if (CurrentMs < CoopLeadNextHostMs)
            return;
        CoopLeadNextHostMs = CurrentMs + 200;
        // bestehende: NPC noch da, Besitzer noch verbunden, Host-Held spricht nicht mit ihm?
        for (auto it = CoopLeadsHost.begin(); it != CoopLeadsHost.end();) {
            oCNpc* npc = it->second.npc;
            if (LeadNpcByName(it->first) != npc) {
                CoopLeadRemoteSet.erase(npc);
                LeadLog("%s nicht mehr in der Welt - Fuehrung durch %s endet", it->first.c_str(), it->second.owner.c_str());
                json j;
                j["op"] = "take";
                j["to"] = it->second.owner;
                j["n"] = it->first;
                LeadSend(j);
                LeadDropRemote(it->first);
                it = CoopLeadsHost.erase(it);
                continue;
            }
            if (!PlayerNameToNpc.count(string(it->second.owner.c_str()))) {
                auto next = std::next(it);
                LeadHostReclaim(it, "Figur des Besitzers fehlt", true);
                it = next;
                continue;
            }
            if (IsPlayerTalkingWithNpc(npc) || CoopNpcInDialogWithPlayer(npc)) {
                auto next = std::next(it);
                LeadHostReclaim(it, "Host-Held spricht mit ihm", true);
                it = next;
                continue;
            }
            ++it;
        }
        if (!CoopLeadOn)
            return;
        // neue: vom Host gefuehrte NPCs im Angriff auf eine Figur
        struct Cand { oCNpc* npc; std::string n, owner, st; float dist; };
        std::vector<Cand> cands;
        for (auto& kv : BroadcastNpcs) {
            LocalNpc* ln = kv.second;
            oCNpc* npc = ln ? ln->npc : NULL;
            if (!npc || ln->destroyed || npc->IsDead() || npc->IsUnconscious() || !LeadInWorld(npc))
                continue;
            oCNpc* en = npc->enemy;
            if (!en || !PlayerNpcs.count(en) || en->IsDead() || en->IsUnconscious())
                continue;
            const char* st = npc->state.curState.name.ToChar();
            if (!LeadAttackState(st) || LeadIsParty(npc) || IsPlayerTalkingWithNpc(npc) || CoopNpcInDialogWithPlayer(npc))
                continue;
            string key = kv.first;   // Schluessel ist const, ToChar nicht
            const std::string n = key.ToChar();
            auto cd = CoopLeadCooldown.find(n);
            if (cd != CoopLeadCooldown.end() && cd->second > CurrentMs)
                continue;
            const float dist = (npc->GetPositionWorld() - en->GetPositionWorld()).LengthApprox();
            if (dist > CoopLeadGiveRange)
                continue;
            cands.push_back({ npc, n, std::string(PlayerNpcs[en].ToChar()), std::string(st), dist });
        }
        for (auto& c : cands)
            LeadGive(c.npc, c.n, c.owner, c.st, c.dist);
    }

    static void LeadHostBack(json& e, const std::string& id, const std::string& n) {
        auto it = CoopLeadsHost.find(n);
        if (it == CoopLeadsHost.end() || it->second.owner != id)
            return;
        oCNpc* npc = it->second.npc;
        const long long held = CurrentMs - it->second.since;
        CoopLeadsHost.erase(it);
        CoopLeadRemoteSet.erase(npc);
        LeadDropRemote(n);
        CoopLeadReturned++;
        const std::string why = e.contains("why") && e["why"].is_string() ? e["why"].get<std::string>() : std::string("-");
        const bool notHere = why == "nicht hier";
        CoopLeadCooldown[n] = CurrentMs + (notHere ? 5000 : 1500);
        if (!npc || LeadNpcByName(n) != npc) {
            LeadLog("%s von %s zurueck (%s) - hier nicht mehr in der Welt", n.c_str(), id.c_str(), why.c_str());
            return;
        }
        const bool dead = e.contains("dead") && e["dead"].is_number() && e["dead"].get<int>() != 0;
        const bool unc = e.contains("unc") && e["unc"].is_number() && e["unc"].get<int>() != 0;
        if (e.contains("x") && e["x"].is_number() && e.contains("y") && e.contains("z"))
            LeadSetPos(npc, zVEC3(e["x"].get<float>(), e["y"].get<float>(), e["z"].get<float>()));
        if (e.contains("h") && e["h"].is_number()) {
            npc->ResetRotationsWorld();
            npc->RotateWorldY(e["h"].get<float>());
        }
        if (!npc->IsDead() && e.contains("hp") && e["hp"].is_number() && e.contains("hpmax") && e["hpmax"].is_number()) {
            const int hp = e["hp"].get<int>(), hpmax = e["hpmax"].get<int>();
            if (hpmax > 0)
                npc->SetAttribute(NPC_ATR_HITPOINTSMAX, hpmax);
            if (hp > 0)
                npc->SetAttribute(NPC_ATR_HITPOINTS, hp);
        }
        const std::string st = e.contains("st") && e["st"].is_string() ? e["st"].get<std::string>() : std::string();
        const std::string en = e.contains("en") && e["en"].is_string() ? e["en"].get<std::string>() : std::string();
        const char* how = "Tagesablauf";
        if (dead) {
            if (!npc->IsDead())
                CoopKillNoXp(npc, NULL, "beim fuehrenden Client gestorben");   // Treffer mit EP kamen vorher als Nachspielen
            how = "tot";
        }
        else if (unc) {
            if (!npc->IsUnconscious() && !npc->IsDead()) {
                npc->SetWeaponMode2(NPC_WEAPON_NONE);
                CoopStopMobUse(npc, "vor dem K.o. (Rueckgabe)");   // v74 (RemoteNpc.cpp)
                npc->DropUnconscious(1, npc);   // ohne Taeter - EP kamen mit dem nachgespielten Treffer
            }
            how = "bewusstlos";
        }
        else if (!npc->IsDead() && !npc->IsUnconscious()) {
            npc->GetEM()->KillMessages();
            npc->ClearEM();
            oCNpc* target = LeadTargetByName(en);
            if (target && target != npc && !target->IsDead() && LeadCombatState(st.c_str())) {
                if (npc->IsHuman())
                    npc->SetTmpAttitude(NPC_ATT_HOSTILE);
                npc->SetEnemy(target);
                npc->state.StartAIState(zSTRING(LeadAttackState(st.c_str()) ? st.c_str() : (npc->IsHuman() ? "ZS_ATTACK" : "ZS_MM_ATTACK")),
                    FALSE, 0, 0.f, FALSE);
                how = "Kampf geht beim Host weiter";
            }
            else
                npc->state.ClearAIState();
        }
        LeadLog("%s von %s zurueck nach %.1f s (%s; Ziel %s, %s) - %s", n.c_str(), id.c_str(), held / 1000.f, why.c_str(),
            en.empty() ? "-" : en.c_str(), st.empty() ? "-" : st.c_str(), how);
    }

    // Host, aus ProcessServerPacket: Paket eines Clients mit "lead" - nur vom Besitzer, dann mit id = NPC weiterverteilen.
    // false = verwerfen.
    bool CoopLeadAcceptPacket(json& j, const std::string& sender) {
        if (!j.contains("lead"))
            return true;
        if (!j["lead"].is_string())
            return false;
        const std::string n = j["lead"].get<std::string>();
        auto it = CoopLeadsHost.find(n);
        if (it == CoopLeadsHost.end() || it->second.owner != sender)
            return false;   // (noch/nicht mehr) nicht von ihm gefuehrt - z. B. letzte Pakete nach der Rueckgabe
        j["id"] = n;
        j["lb"] = sender;   // Server.cpp: nicht an den Besitzer zurueck
        return true;
    }

    // Host: ein Client ist weg - seine NPCs zurueck
    void CoopLeadPeerGone(const std::string& id) {
        for (auto it = CoopLeadsHost.begin(); it != CoopLeadsHost.end();) {
            if (it->second.owner == id) {
                auto next = std::next(it);
                LeadHostReclaim(it, "Verbindung getrennt", false);
                it = next;
            }
            else
                ++it;
        }
    }

    // ===== Client ===================================================================================================

    static void LeadMineRelease(const std::string& n, const char* why, bool send) {
        auto it = CoopLeadsMine.find(n);
        if (it == CoopLeadsMine.end())
            return;
        CoopLeadMineEntry m = it->second;
        CoopLeadsMine.erase(it);
        CoopLeadMineSet.erase(m.npc);
        oCNpc* npc = m.npc;
        const bool here = npc && LeadNpcByName(n) == npc;
        if (send) {
            if (here && m.local) {   // letzte Lage/HP/Animation vor der Rueckgabe hinaus (gleicher Kanal, gleiche Reihenfolge)
                m.local->Pulse();
                m.local->PackUpdate();
            }
            json j;
            j["op"] = "back";
            j["n"] = n;
            j["why"] = why;
            if (here) {
                const zVEC3 p = npc->GetPositionWorld();
                j["x"] = p.n[0];
                j["y"] = p.n[1];
                j["z"] = p.n[2];
                j["h"] = GetHeading(npc);
                j["hp"] = npc->GetAttribute(NPC_ATR_HITPOINTS);
                j["hpmax"] = npc->GetAttribute(NPC_ATR_HITPOINTSMAX);
                j["wm"] = npc->GetWeaponMode();
                j["st"] = npc->state.curState.name.ToChar();
                j["en"] = LeadNameOf(npc->enemy);
                j["dead"] = npc->IsDead() ? 1 : 0;
                j["unc"] = npc->IsUnconscious() ? 1 : 0;
            }
            LeadSend(j);
        }
        if (m.local) {
            m.local->destroyed = true;
            delete m.local;
        }
        // wieder vom Host gefuehrt: eigene Kampf-KI aus (C16 sperrt sie danach wieder), Lage/Animationen kommen vom Host
        if (here && !npc->IsDead() && !npc->IsUnconscious()) {
            npc->GetEM()->KillMessages();
            npc->ClearEM();
            npc->state.ClearAIState();
        }
        LeadLog("%s an den Host zurueck nach %.1f s (%s)", n.c_str(), (CurrentMs - m.since) / 1000.f, why);
    }

    static void LeadMineGive(json& e, const std::string& n) {
        oCNpc* npc = LeadNpcByName(n);
        if (CoopLeadsMine.count(n))
            return;
        const char* refuse = NULL;
        if (!CoopLeadOn)
            refuse = "aus";
        else if (!player || IsCoopPaused || IsLoadingLevel)
            refuse = "Pause";   // auch im Hauptmenue/beim Laden - der Host behaelt ihn dann
        else if (!npc || !LeadInWorld(npc))
            refuse = "nicht hier";
        else if (npc->IsDead() || npc->IsUnconscious())
            refuse = "liegt schon";
        if (refuse) {
            json b;
            b["op"] = "back";
            b["n"] = n;
            b["why"] = refuse;
            LeadSend(b);
            LeadLog("%s: Uebergabe abgelehnt (%s)", n.c_str(), refuse);
            return;
        }
        LeadDropRemote(n);   // der Host sendet ihn nicht mehr
        LocalNpc* ln = new LocalNpc(npc, string(n.c_str()));
        ln->Reinit();             // Vorbelegung: Lage, HP und Waffenmodus gehen gleich hinaus
        ln->leadSend = true;
        ln->initialized = true;   // kein INIT_NPC (der NPC steht bei allen schon in der Welt)
        CoopLeadsMine[n] = { npc, ln, CurrentMs, 0, 0 };
        CoopLeadMineSet.insert(npc);
        // eigene KI: Ziel = eigener Held, Angriffszustand wie beim Host
        npc->GetEM()->KillMessages();
        npc->ClearEM();
        if (npc->IsHuman())
            npc->SetTmpAttitude(NPC_ATT_HOSTILE);
        npc->SetEnemy(player);
        std::string st = e.contains("st") && e["st"].is_string() ? e["st"].get<std::string>() : std::string();
        if (!LeadAttackState(st.c_str()))
            st = npc->IsHuman() ? "ZS_ATTACK" : "ZS_MM_ATTACK";
        npc->state.StartAIState(zSTRING(st.c_str()), FALSE, 0, 0.f, FALSE);
        LeadLog("fuehre %s jetzt selbst (%s, %.1f m, HP %d) - KI, Schlaege und Treffer rechnet dieser PC", n.c_str(), st.c_str(),
            (npc->GetPositionWorld() - player->GetPositionWorld()).LengthApprox() / 100.f, npc->GetAttribute(NPC_ATR_HITPOINTS));
    }

    static void LeadMinePulse() {
        // jedes Bild senden - wie der Host seine NPCs (LocalNpc drosselt Lage/Blickrichtung selbst)
        for (auto& kv : CoopLeadsMine) {
            if (kv.second.local && LeadNpcByName(kv.first) == kv.second.npc) {
                kv.second.local->Pulse();
                kv.second.local->PackUpdate();
            }
        }
        if (CurrentMs < CoopLeadNextMineMs)
            return;
        CoopLeadNextMineMs = CurrentMs + 200;
        std::vector<std::pair<std::string, const char*>> back;
        for (auto& kv : CoopLeadsMine) {
            CoopLeadMineEntry& m = kv.second;
            oCNpc* npc = m.npc;
            if (LeadNpcByName(kv.first) != npc || !LeadInWorld(npc)) {
                back.push_back({ kv.first, "nicht mehr in der Welt" });
                continue;
            }
            if (IsCoopPaused || !player) {
                back.push_back({ kv.first, "Pause" });
                continue;
            }
            if (npc->IsDead() || npc->IsUnconscious()) {
                if (!m.downSince)
                    m.downSince = CurrentMs;
                if (CurrentMs - m.downSince >= 600)   // erst die Treffer hinaus (DamageProcessorLoop misst einen Durchlauf spaeter)
                    back.push_back({ kv.first, npc->IsDead() ? "tot" : "bewusstlos" });
                continue;
            }
            const bool hold = CurrentMs - m.since < CoopLeadHoldMs;
            if (!hold && npc->enemy && npc->enemy != player) {
                back.push_back({ kv.first, "anderes Ziel" });
                continue;
            }
            if (!LeadCombatState(npc->state.curState.name.ToChar())) {
                if (!m.idleSince)
                    m.idleSince = CurrentMs;
                if (CurrentMs - m.idleSince >= 2000) {
                    back.push_back({ kv.first, "Kampf vorbei" });
                    continue;
                }
            }
            else
                m.idleSince = 0;
            if (!hold && (npc->GetPositionWorld() - player->GetPositionWorld()).LengthApprox() > CoopLeadKeepRange)
                back.push_back({ kv.first, "zu weit" });
        }
        for (auto& b : back)
            LeadMineRelease(b.first, b.second, true);
    }

    // Client: ein Paket des Hosts fuer einen NPC, den ich fuehre - verwerfen (z. B. letzte Pakete vor der Uebergabe)
    bool CoopLeadDropPacket(const std::string& id) {
        return ClientThread && !ServerThread && CoopLeadsMine.count(id) != 0;
    }

    // Client (Besitzer): eine Figur trifft meinen NPC (nachgespielt). Wie beim Host (Hooks.cpp, Treffer des Host-Helden): mit 2/3
    // Wahrscheinlichkeit wechselt er das Ziel - dann gibt der Client ihn zurueck, und es kaempft, wen er jetzt angreift.
    void CoopLeadHitBy(oCNpc* npc, oCNpc* attacker) {
        if (!CoopLeadIsMine(npc) || !attacker || !PlayerNpcs.count(attacker) || npc->enemy == attacker || npc->IsDead() || npc->IsUnconscious())
            return;
        if (GetRandVal(0, 100) <= 33)
            return;
        if (npc->IsHuman())
            npc->SetTmpAttitude(NPC_ATT_HOSTILE);
        npc->SetEnemy(attacker);
        LeadLog("%s wechselt das Ziel auf %s (von dort getroffen)", LeadNameOf(npc).c_str(), LeadNameOf(attacker).c_str());
    }

    // ===== beide =====================================================================================================

    void CoopLeadReceive(json& e, const std::string& id) {
        CoopTraceCount(TRACE_RECV, SYNC_NPC_LEAD);
        CoopLeadReadConfig();
        const std::string op = e.contains("op") && e["op"].is_string() ? e["op"].get<std::string>() : std::string();
        const std::string n = e.contains("n") && e["n"].is_string() ? e["n"].get<std::string>() : std::string();
        if (n.empty())
            return;
        if (ServerThread) {
            if (op == "back" && !e.contains("by") && player && !IsLoadingLevel)
                LeadHostBack(e, id, n);
            return;
        }
        if (!ClientThread)
            return;
        const std::string to = e.contains("to") && e["to"].is_string() ? e["to"].get<std::string>() : std::string();
        if (to != MyselfId.ToChar())
            return;
        if (op == "give")
            LeadMineGive(e, n);
        else if (op == "take")
            LeadMineRelease(n, "vom Host zurueckgeholt", false);
    }

    // Game_Loop (Plugin.cpp), nach dem Senden der eigenen Pakete
    void CoopLeadPulse() {
        CoopLeadReadConfig();
        if (IsLoadingLevel || !player || WorldEditMode)
            return;
        if (ServerThread) {
            if (!IsCoopPaused)
                LeadHostPulse();
        }
        else if (ClientThread)
            LeadMinePulse();
    }

    // Ladebeginn, Verbindungs-/Host-Ende: alles vergessen (keine Engine-Aufrufe - die Welt kann schon weg sein)
    void CoopLeadClear(const char* why) {
        if (!CoopLeadsHost.empty() || !CoopLeadsMine.empty())
            OthelloCompatLog("[%lld s, %s] Kampf-Uebergabe: %u abgegebene / %u selbst gefuehrte NPCs vergessen (%s)", DiagSeconds(), DiagRole(),
                (unsigned)CoopLeadsHost.size(), (unsigned)CoopLeadsMine.size(), why);
        for (auto& kv : CoopLeadsMine)
            if (kv.second.local) {
                kv.second.local->destroyed = true;
                delete kv.second.local;
            }
        CoopLeadsHost.clear();
        CoopLeadsMine.clear();
        CoopLeadRemoteSet.clear();
        CoopLeadMineSet.clear();
        CoopLeadCooldown.clear();
    }

    // fuer die Netzzeile alle 30 s (CoopDiag.cpp)
    void CoopLeadDiag(std::string& out) {
        char b[160];
        _snprintf(b, sizeof(b), " | Kampf-Uebergabe: %u abgegeben, %u zurueck, jetzt %u beim Client / %u selbst gefuehrt", CoopLeadGiven,
            CoopLeadReturned, (unsigned)CoopLeadsHost.size(), (unsigned)CoopLeadsMine.size());
        b[sizeof(b) - 1] = 0;
        out = b;
    }

#else
    bool CoopLeadIsRemote(oCNpc*) { return false; }
    bool CoopLeadIsMine(oCNpc*) { return false; }
    bool CoopLeadAcceptPacket(json&, const std::string&) { return true; }
    void CoopLeadPeerGone(const std::string&) {}
    bool CoopLeadDropPacket(const std::string&) { return false; }
    void CoopLeadHitBy(oCNpc*, oCNpc*) {}
    void CoopLeadReceive(json&, const std::string&) {}
    void CoopLeadPulse() {}
    void CoopLeadClear(const char*) {}
    void CoopLeadDiag(std::string& out) { out.clear(); }
#endif
}
