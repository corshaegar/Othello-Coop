// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

    // --- v53: Treffer auf den Helden eines Clients entscheidet der Client (20.10.2026) ----------------------------------------
    //
    // Nutzer: "Die Version vom Client dominiert fuer ihn: wenn Schaden entsteht, dann entsteht dieser beim Host. Wenn kein Schaden
    // entsteht, dann entsteht auch kein Schaden beim Host. Der Host entscheidet nicht ueber den Schaden, den der Client bekommt, und
    // berechnet diesen auch nicht, er bekommt lediglich die Informationen vom Client." Anlass: Test v52 - mehrere Treffer kamen an,
    // waehrend der Client einen Ausweichsprung machte (T_FISTPARADEJUMPB); frueher zeigte der Host eine Todesanimation der Figur,
    // obwohl der Client keinen Schaden hatte (die Puppe beim Host entschied mit ihren eigenen, verspaeteten HP selbst).
    //
    // Ein Nahkampfschlag entsteht nur, wo die KI des Angreifers laeuft: EV_AttackForward/Left/Right/Run -> StartHitCombo
    // (0x00627560) -> HitCombo/CheckHitTarget -> CreateHit (0x00627C80). Die NPC-Kopien beim Client bekommen vom Host nur
    // Animationen, nie diese Befehle - der Client kann einen Schlag nicht von sich aus bemerken. Darum:
    // - Host: Schaden an einer Figur (Puppe) eines Clients durch einen NPC wird nicht ausgefuehrt (Haken am Eingang von
    //   oCNpc::OnDamage 0x00730FE0, hinter zNoFriendlyFire, das den vtable-Platz 0x007DDFEC belegt). Stattdessen geht nur die
    //   Meldung "NPC X schlaegt nach dir" an die Clients (SYNC_ATTACKS mit "swing"). Die Puppe nimmt keinen Schaden, zuckt nicht,
    //   faellt nicht von selbst.
    // - Client: prueft in seiner Sicht (Abstand zur Kampfreichweite des Angreifers, Blickrichtung, eigene Parade/Ausweichsprung;
    //   Fernkampf/Magie: Reichweite und freie Sicht) und laesst den Schlag nur dann die eigene Engine ausfuehren: CreateHit der
    //   NPC-Kopie (ueber die Aufrufstelle in HitCombo 0x0062791F - unter Othello dessen Vorbau OTHELLO_ABI+0xC4B70, der das
    //   Original ruft und nachbehandelt) bzw. bei Fernkampf/Magie OnDamage mit dem Rohschaden des Hosts. Den Schaden rechnet
    //   damit das Client-Spiel mit seiner Ruestung, seinen Werten und Othellos Regeln; bewusstlos/tot entscheidet seine Engine.
    // - Client meldet das Ergebnis (SYNC_HIT_RESULT: Angreifer, Schaden, HP, bewusstlos, tot; "wake" beim Aufstehen). Der Host
    //   spiegelt es an der Puppe (DropUnconscious/DoDie mit dem Angreifer); HP kommen wie bisher ueber SYNC_HP.
    // - Treffer des Clients auf NPCs zaehlten schon so (Client misst, Host spielt nach). Neu: der Host meldet sie als Kampfgeraeusch
    //   an die Umgebung (oCNpc::AssessFightSound_S 0x006B5B80, wie CreateHit beim Host-Helden 0x0062804C) - sonst bemerkten
    //   Umstehende beim Host die Kaempfe des Clients nie.
    // Konfiguration: coopClientHitAuthority (true), coopHitReachExtraCm (150), coopHitAngleDeg (75), coopHitRangedMaxCm (5000).

    void CoopNoWhirl(oCNpc* npc);   // v100 (C21, Hooks.cpp): ZS_ATTACK-Neustart ohne Ueberraschungs-Sprung

#if ENGINE == Engine_G1

    static bool CoopHitCfgRead = false;
    static bool CoopHitAuthorityOn = true;
    static float CoopHitReachExtra = 150.f;
    static float CoopHitAngleCos = 0.2588f;   // cos 75 Grad
    static float CoopHitRangedMax = 5000.f;

    static void CoopHitReadConfig() {
        if (CoopHitCfgRead)
            return;
        CoopHitCfgRead = true;
        if (CoopConfig.contains("coopClientHitAuthority") && CoopConfig["coopClientHitAuthority"].is_boolean())
            CoopHitAuthorityOn = CoopConfig["coopClientHitAuthority"].get<bool>();
        if (CoopConfig.contains("coopHitReachExtraCm") && CoopConfig["coopHitReachExtraCm"].is_number())
            CoopHitReachExtra = CoopConfig["coopHitReachExtraCm"].get<float>();
        if (CoopConfig.contains("coopHitAngleDeg") && CoopConfig["coopHitAngleDeg"].is_number()) {
            float deg = CoopConfig["coopHitAngleDeg"].get<float>();
            if (deg < 10.f) deg = 10.f;
            if (deg > 180.f) deg = 180.f;
            CoopHitAngleCos = cosf(deg * 3.14159265f / 180.f);
        }
        if (CoopConfig.contains("coopHitRangedMaxCm") && CoopConfig["coopHitRangedMaxCm"].is_number())
            CoopHitRangedMax = CoopConfig["coopHitRangedMaxCm"].get<float>();
    }

    bool CoopClientHitAuthority() {
        CoopHitReadConfig();
        return CoopHitAuthorityOn;
    }

    static void SwingLog(const char* fmt, ...) {
        static CoopDiagBudget budget = { 0, 0 };
        if (!CoopDiagAllow(budget, 60))
            return;
        char buf[600];
        va_list ap;
        va_start(ap, fmt);
        _vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        buf[sizeof(buf) - 1] = 0;
        OthelloCompatLog("[%lld s, %s] Schlag: %s", DiagSeconds(), DiagRole(), buf);
    }

    static std::string CoopHitNpcName(oCNpc* n) {
        if (!n)
            return "-";
        auto it = NpcToUniqueNameList.find(n);
        if (it != NpcToUniqueNameList.end())
            return std::string(it->second.ToChar());
        return std::string(n->GetObjectName().ToChar());
    }

    // --- v54 (C13): Wer einen Client bestrafen will, bestraft beim Host nicht den Host-Helden ---------------------------------
    //
    // Nutzer (Test v53): "Die Gegner greifen ausserdem den Host an und nicht den Client." Log Host: FRIEND_1 trifft CS_ESKORTE2
    // (104 s, Kampfgeraeusch C12), danach schlaegt CS_ESKORTE1 den Host-Helden nieder (111 s) - der Host hatte niemanden
    // angegriffen. Ursache (allgemein G1, Othello ebenso): B_AssessFightSound schickt eine Wache, deren Schuetzling angegriffen
    // wird, nach ZS_ProclaimAndPunish, und das zielt fest auf 'hero' (G1-Quelle: "zu Bestrafender muss der Spieler sein ('hero'
    // wird verwendet!)"; Othello-DAT: Npc_SetTarget(self, HERO) in ZS_PROCLAIMANDPUNISH und _END, tools\settarget_hero.py).
    // Dorthin fuehren auch ZS_AssessEnemy, ZS_AssessDefeat und ZS_AssessMurder. Beim Host ist hero immer der Host-Held.
    // Jetzt beim Host: das Opfer eines Treffers einer Coop-Figur und alle Menschen in der Reichweite des Kampfgeraeuschs
    // (HAI_DIST_HELPATTACKEDCHARGES aus dem Skript, gemessen am Opfer, + 5 m) werden 30 s vorgemerkt - ausser wer da schon mit dem
    // Host-Helden kaempft. Will ein vorgemerkter NPC den Host-Helden bestrafen (ZS_PROCLAIMANDPUNISH) oder greift ihn an
    // (ZS_ATTACK, Gegner = Host-Held), und der Host hat in den letzten 20 s (HAI_TIME_PURSUIT) niemanden getroffen, geht er auf
    // die Figur: Auftraege geloescht (wie Npc_ClearAIQueue), Gegner = Figur, ZS_ATTACK - wie am Ende von ZS_ProclaimAndPunish,
    // nur ohne den Gang zum Host-Helden. Liegt die Figur, endet die Bestrafung (Tagesablauf). Trifft der Host-Held den NPC selbst,
    // entfaellt die Vormerkung.
    // Engine (Speicherabbild geprueft): oCNpc_States::StartAIState(zSTRING const&, ...) 0x006C5350 unveraendert, ruft
    // StartAIState(int, ...) 0x006C5490 - darin hat Othello nur den Aufruf von CloseCutscenes (0x006C54F1) auf ein ret gelegt;
    // SetEnemy 0x00691A80 unveraendert; oCNpc::ClearEM 0x006A2610 (Othello ersetzt darin nur zCVob::GetEM durch
    // OTHELLO_ABI+0x7B610, wie bei jedem Npc_ClearAIQueue der Skripte). Nicht virtuell.
    struct CoopPunishMarkEntry {
        oCNpc* figure;
        long long until;
        long long holdUntil;   // nach einem Umlenken 1 s Ruhe (der Zustandswechsel greift erst im naechsten KI-Durchlauf)
    };
    static std::map<std::string, CoopPunishMarkEntry> CoopPunishMarks;   // eindeutiger NPC-Name -> Figur, gueltig bis
    static long long CoopHostHeroLastHitMs = -1000000;
    static const long long CoopPunishMarkMs = 30000;
    static const long long CoopPunishHostQuietMs = 20000;

    static void PunishLog(const char* fmt, ...) {
        static CoopDiagBudget budget = { 0, 0 };
        if (!CoopDiagAllow(budget, 30))
            return;
        char buf[600];
        va_list ap;
        va_start(ap, fmt);
        _vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        buf[sizeof(buf) - 1] = 0;
        OthelloCompatLog("[%lld s, %s] Bestrafung: %s", DiagSeconds(), DiagRole(), buf);
    }

    // v100 (C13b): Nutzer (Test v99): "Sarus ist rot, toetet uns aber nicht". Log Host: nach INFO_SARUS_GOAWAY (Othello:
    // Npc_SetPermAttitude(self, ATT_HOSTILE), Angriff auf den Helden) schlug FRIEND_2 ihn staendig - Sarus war also vorgemerkt.
    // 819 s lenkte die Umlenkung seinen Angriff auf die Figur, 862 s und 915 s schickte sie ihn "zurueck in den Tagesablauf", weil
    // die Figur lag - mitten im eigenen Kampf gegen den Host-Helden. Wer dem Host-Helden selbst dauerhaft feindlich ist (feste
    // Haltung zum Spieler, z. B. nach so einem Dialog oder eine feindliche Gilde), greift ihn aus eigenem Grund an - das ist keine
    // Bestrafung fuer die Tat des Clients. Solche NPCs werden nicht vorgemerkt und nicht umgelenkt. GetPermAttitude 0x0068D3C0
    // (Hooks.cpp: unveraendert, liest bei Othello das verlegte Gildenfeld).
    static bool CoopPunishOwnGrudge(oCNpc* npc) {
        return player && npc->GetPermAttitude(player) == NPC_ATT_HOSTILE;
    }

    static bool CoopPunishFightsHost(oCNpc* npc) {
        return npc->enemy == player && strcmp(npc->state.curState.name.ToChar(), "ZS_ATTACK") == 0;
    }

    static bool CoopPunishMarkOne(oCNpc* npc, oCNpc* figure) {
        if (!npc || npc == player || npc == figure || PlayerNpcs.count(npc) || npc->IsDead() || npc->IsUnconscious() || !npc->IsHuman())
            return false;
        auto it = NpcToUniqueNameList.find(npc);
        if (it == NpcToUniqueNameList.end())
            return false;
        const std::string key(it->second.ToChar());
        if (CoopPunishFightsHost(npc) || CoopPunishOwnGrudge(npc)) {   // kaempft schon mit dem Host-Helden / ist ihm selbst feind - bleibt dabei
            CoopPunishMarks.erase(key);
            return false;
        }
        auto old = CoopPunishMarks.find(key);
        const long long hold = old != CoopPunishMarks.end() ? old->second.holdUntil : 0;
        CoopPunishMarks[key] = { figure, CurrentMs + CoopPunishMarkMs, hold };
        return true;
    }

    // Host: Treffer einer Coop-Figur auf victim - Opfer und Umstehende vormerken. Rueckgabe: Zahl der Vorgemerkten.
    int CoopPunishMark(oCNpc* figure, oCNpc* victim) {
        static float range = -1.f;
        if (!ServerThread || !figure || !victim || !ogame)
            return 0;
        if (range < 0.f) {
            int v = 2000;
            zCPar_Symbol* s = parser ? parser->GetSymbol("HAI_DIST_HELPATTACKEDCHARGES") : NULL;
            if (s)
                s->GetValue(v, 0);
            if (v < 500 || v > 10000)
                v = 2000;
            range = (float)v + 500.f;
        }
        int n = CoopPunishMarkOne(victim, figure) ? 1 : 0;
        const zVEC3 at = victim->GetPositionWorld();
        for (zCListSort<oCNpc>* l = CoopWorldNpcListFirst(ogame->GetGameWorld()); l; l = l->next) {
            oCNpc* npc = l->data;
            if (!npc || npc == victim)
                continue;
            if ((npc->GetPositionWorld() - at).LengthApprox() > range)
                continue;
            if (CoopPunishMarkOne(npc, figure))
                n++;
        }
        return n;
    }

    // Host: der Host-Held trifft npc - seine Bestrafung gilt dann dem Host zu Recht.
    void CoopPunishHostHit(oCNpc* npc) {
        CoopHostHeroLastHitMs = CurrentMs;
        if (!npc)
            return;
        auto it = NpcToUniqueNameList.find(npc);
        if (it != NpcToUniqueNameList.end())
            CoopPunishMarks.erase(std::string(it->second.ToChar()));
    }

    // Host, jedes Bild (DamageProcessorLoop), wirksam alle 100 ms.
    void CoopPunishPulse() {
        static long long next = 0;
        if (!ServerThread || IsCoopPaused || !player || CoopPunishMarks.empty() || CurrentMs < next)
            return;
        next = CurrentMs + 100;
        const bool hostQuiet = CurrentMs - CoopHostHeroLastHitMs >= CoopPunishHostQuietMs;
        for (auto it = CoopPunishMarks.begin(); it != CoopPunishMarks.end();) {
            auto un = UniqueNameToNpcList.find(string(it->first.c_str()));
            oCNpc* npc = un != UniqueNameToNpcList.end() ? un->second : NULL;
            oCNpc* figure = it->second.figure;
            if (it->second.until < CurrentMs || !npc || npc->IsDead() || npc->IsUnconscious() || !PlayerNpcs.count(figure)) {
                it = CoopPunishMarks.erase(it);
                continue;
            }
            // v55: Kopie - StartAIState/ClearAIState unten ersetzen den Zustandsnamen (Test v54: Log zeigte "SELF", "ITEM" = freigegeben)
            const std::string st = npc->state.curState.name.ToChar();
            const bool punish = st == "ZS_PROCLAIMANDPUNISH";
            const bool attack = st == "ZS_ATTACK" && npc->enemy == player;
            if (!hostQuiet || (!punish && !attack) || it->second.holdUntil > CurrentMs) {
                ++it;
                continue;
            }
            if (CoopPunishOwnGrudge(npc)) {   // v100 (C13b): Haltung erst nach dem Vormerken feindlich geworden (Dialog)
                PunishLog("%s greift den Host-Helden an (%s) - ihm selbst feindlich (feste Haltung), keine Bestrafung: bleibt dabei",
                    it->first.c_str(), st.c_str());
                it = CoopPunishMarks.erase(it);
                continue;
            }
            it->second.holdUntil = CurrentMs + 1000;
            const std::string fig = CoopHitNpcName(figure);
            if (figure->IsDead() || figure->IsUnconscious()) {
                npc->ClearEM();
                npc->state.ClearAIState();
                PunishLog("%s wollte den Host-Helden bestrafen (%s), Anlass %s liegt - zurueck in den Tagesablauf",
                    it->first.c_str(), st.c_str(), fig.c_str());
                it = CoopPunishMarks.erase(it);
                continue;
            }
            if (punish) {
                npc->ClearEM();
                npc->SetEnemy(figure);
                CoopNoWhirl(npc);   // v100 (C21)
                npc->state.StartAIState(zSTRING("ZS_ATTACK"), FALSE, 0, 0.f, FALSE);
            }
            else
                npc->SetEnemy(figure);
            PunishLog("%s wollte den Host-Helden %s (%s) - Anlass war %s, greift jetzt %s an",
                it->first.c_str(), punish ? "bestrafen" : "angreifen", st.c_str(), fig.c_str(), fig.c_str());
            ++it;
        }
    }

    // ===== Host =====================================================================================================

    // Aus dem Haken auf oCNpc::OnDamage (Hooks.cpp): ein NPC trifft die Puppe eines Clients - nur melden.
    void CoopSwingReport(oCNpc* attacker, oCNpc* puppet, oCNpc::oSDamageDescriptor& d) {
        auto an = NpcToUniqueNameList.find(attacker);
        auto pn = PlayerNpcs.find(puppet);
        if (an == NpcToUniqueNameList.end() || pn == PlayerNpcs.end()) {
            SwingLog("%s -> %s: Angreifer ohne Namen, nicht gemeldet (kein Schaden)", attacker ? attacker->GetObjectName().ToChar() : "-",
                puppet ? puppet->GetObjectName().ToChar() : "-");
            return;
        }
        // enuModeWeapon (CreateHit 0x00627D40..0x00627DCA): 1 Faust, 2 Nahkampfwaffe, 4 Bogen/Armbrust, 8 Magie
        const bool ranged = (d.enuModeWeapon & 4) != 0 || (d.enuModeWeapon & 8) != 0 || d.pFXHit != NULL;
        json att;
        att["target"] = pn->second.ToChar();
        att["damage"] = -1.0f;
        att["isUnconscious"] = 0;
        att["isDead"] = false;
        att["damageMode"] = d.enuModeDamage;
        att["swing"] = 1;
        att["raw"] = d.fDamageTotal;
        att["wm"] = d.enuModeWeapon;
        att["rng"] = ranged ? 1 : 0;
        json j;
        j["type"] = SYNC_ATTACKS;
        j["id"] = an->second.ToChar();
        j["att"] = json::array();
        j["att"].push_back(att);
        CoopTraceCount(TRACE_SENT, SYNC_ATTACKS);
        ReadyToSendJsons.enqueue(j);
        SwingLog("%s -> %s gemeldet (%s, roh %.0f) - Puppe nimmt keinen Schaden, der Client entscheidet",
            an->second.ToChar(), pn->second.ToChar(), ranged ? "Fernkampf/Magie" : "Nahkampf", d.fDamageTotal);
    }

    // Aus RemoteNpc::UpdateAttacks (Host): Treffer des Clients auf einen NPC als Kampfgeraeusch an die Umgebung.
    void CoopFightSound(oCNpc* attacker, oCNpc* victim) {
        if (!ServerThread || !attacker || !victim)
            return;
        const zVEC3 pos = attacker->GetPositionWorld();
        oCNpc::AssessFightSound_S(attacker, pos, victim);
        const int marked = PlayerNpcs.count(attacker) ? CoopPunishMark(attacker, victim) : 0;   // v54 (C13)
        static CoopDiagBudget budget = { 0, 0 };
        if (CoopDiagAllow(budget, 20))
            OthelloCompatLog("[%lld s, %s] Kampfgeraeusch: %s trifft %s - Umstehende bemerken es (%d vorgemerkt: bestrafen sie, dann %s)",
                DiagSeconds(), DiagRole(), CoopHitNpcName(attacker).c_str(), CoopHitNpcName(victim).c_str(), marked,
                CoopHitNpcName(attacker).c_str());
    }

    // SYNC_HIT_RESULT vom Client (Host und andere Clients): Puppe spiegelt den Zustand des Helden.
    void CoopApplyHitResult(oCNpc* puppet, json& u, const std::string& who) {
        if (!puppet)
            return;
        if (u.contains("wake")) {
            if (puppet->IsUnconscious() && !puppet->IsDead()) {
                // wie RemoteNpc::UpdateWeaponMode bei bewusstloser Figur: Zustand neu anfangen
                puppet->GetEM()->KillMessages();
                puppet->ClearEM();
                puppet->state.ClearAIState();
                SwingLog("%s steht wieder auf - Figur hier ebenso", who.c_str());
            }
            return;
        }
        const std::string an = u.contains("att") && u["att"].is_string() ? u["att"].get<std::string>() : std::string("-");
        auto it = UniqueNameToNpcList.find(string(an.c_str()));
        oCNpc* attacker = it != UniqueNameToNpcList.end() ? it->second : NULL;
        const int dmg = u.contains("dmg") && u["dmg"].is_number() ? u["dmg"].get<int>() : 0;
        const int hp = u.contains("hp") && u["hp"].is_number() ? u["hp"].get<int>() : -1;
        const bool unc = u.contains("unc") && u["unc"].is_number() && u["unc"].get<int>() != 0;
        const bool dead = u.contains("dead") && u["dead"].is_number() && u["dead"].get<int>() != 0;
        const char* what = "";
        if (dead && !puppet->IsDead()) {
            puppet->SetAttribute(NPC_ATR_HITPOINTS, 0);
            puppet->DoDie(attacker ? attacker : puppet);   // vtable -> OTHELLO_ABI+0x7C3F0 (wie C7)
            what = " - Figur stirbt";
        }
        else if (unc && !dead && !puppet->IsUnconscious() && !puppet->IsDead()) {
            puppet->SetWeaponMode2(NPC_WEAPON_NONE);
            puppet->DropUnconscious(1, attacker ? attacker : puppet);
            what = " - Figur bewusstlos";
        }
        SwingLog("Ergebnis von %s: %s traf, Schaden %d, HP %d%s", who.c_str(), an.c_str(), dmg, hp, what);
    }

    // ===== Client ===================================================================================================

    struct CoopSwingPermit {
        oCNpc* attacker;
        std::string name;
        long long until;
        bool used;
    };
    static std::vector<CoopSwingPermit> CoopSwingPermits;

    struct CoopSwingPending {
        const void* desc;
        oCNpc* attacker;
        std::string name;
        int hpBefore;
        int hpAtSound;
        unsigned long mode;
        unsigned long frame;
        bool sounded;
    };
    static std::vector<CoopSwingPending> CoopSwingPendings;
    static unsigned long CoopSwingFrame = 0;

    typedef void(__thiscall* CoopCreateHitFn)(oCAniCtrl_Human*, zCVob*);

    // CreateHit wie die Engine sie aus HitCombo ruft (0x0062791F): unter Othello dessen Vorbau (ruft das Original 0x00627C80,
    // behandelt danach nach, ret 4), sonst das Original. Kopf beider Fassungen geprueft.
    static CoopCreateHitFn CoopResolveCreateHit() {
        static bool done = false;
        static CoopCreateHitFn fn = NULL;
        if (done)
            return fn;
        done = true;
        static const unsigned char HEAD_ORIG[] = { 0x64, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x6A, 0xFF, 0x68, 0xC3, 0x52, 0x7B, 0x00 };
        static const unsigned char HEAD_OTHELLO[] = { 0xFF, 0x74, 0x24, 0x04, 0xB8, 0x80, 0x7C, 0x62, 0x00 };
        const size_t site = 0x0062791F;
        size_t target = 0;
        const char* kind = "nicht gefunden";
        if (IsReadableMemory((const void*)site, 5) && *(const unsigned char*)site == 0xE8) {
            const size_t t = site + 5 + *(const int*)(site + 1);
            if (t == 0x00627C80 && EngineBytesEqual(t, HEAD_ORIG, sizeof(HEAD_ORIG))) {
                target = t;
                kind = "Original";
            }
            else if (EngineBytesEqual(t, HEAD_OTHELLO, sizeof(HEAD_OTHELLO))) {
                target = t;
                kind = "Vorbau einer Mod (ruft das Original)";
            }
        }
        if (!target && EngineBytesEqual(0x00627C80, HEAD_ORIG, sizeof(HEAD_ORIG))) {
            target = 0x00627C80;
            kind = "Original (Aufrufstelle unbekannt)";
        }
        OthelloCompatLog("CreateHit fuer bestaetigte Schlaege: %08X (%s)", (unsigned)target, kind);
        fn = (CoopCreateHitFn)target;
        return fn;
    }

    // Spielt der eigene Held gerade eine Parade oder einen Ausweichsprung (T_*PARADE*, T_*PARADEJUMPB)?
    static const char* CoopHeroParadeAni(zSTRING& keep) {
        zCModel* m = player ? player->GetModel() : NULL;
        if (!m)
            return NULL;
        for (int i = 0; i < m->numActiveAnis && i < 16; i++) {
            const int id = DANGERGetCurrentAni(m, i);
            if (id < 0)
                continue;
            zCModelAni* ani = m->GetAniFromAniID(id);
            if (!ani)
                continue;
            keep = ani->GetAniName();
            if (strstr(keep.ToChar(), "PARADE"))
                return keep.ToChar();
        }
        return NULL;
    }

    // Aus RemoteNpc::UpdateAttacks (Client): ein NPC schlaegt nach dem eigenen Helden. attacker = NULL: hier nicht in der Welt.
    void CoopClientSwing(oCNpc* attacker, const char* attackerName, json& a) {
        CoopHitReadConfig();
        const char* an = attackerName ? attackerName : "?";
        const unsigned long mode = a.contains("damageMode") && a["damageMode"].is_number() ? a["damageMode"].get<unsigned long>() : 0;
        const float raw = a.contains("raw") && a["raw"].is_number() ? a["raw"].get<float>() : 0.f;
        const bool ranged = a.contains("rng") && a["rng"].is_number() && a["rng"].get<int>() != 0;
        if (!player)
            return;
        if (!attacker || !attacker->GetHomeWorld() || attacker->vobLeafList.GetNum() == 0 || !attacker->GetModel()) {
            SwingLog("%s: hier nicht in der Welt - zaehlt nicht", an);
            return;
        }
        if (player->IsDead() || player->IsUnconscious()) {
            SwingLog("%s: eigener Held liegt schon - zaehlt nicht", an);
            return;
        }
        const zVEC3 pa = attacker->GetPositionWorld();
        const zVEC3 ph = player->GetPositionWorld();
        const float dist = (ph - pa).LengthApprox();
        char why[160] = "";
        if (ranged) {
            if (dist > CoopHitRangedMax) {
                SwingLog("%s (Fernkampf/Magie): ausgewichen - zu weit (%.1f m > %.1f m)", an, dist / 100.f, CoopHitRangedMax / 100.f);
                return;
            }
            if (!attacker->FreeLineOfSight(player)) {
                SwingLog("%s (Fernkampf/Magie): ausgewichen - keine freie Sicht (%.1f m)", an, dist / 100.f);
                return;
            }
            _snprintf(why, sizeof(why), "Fernkampf/Magie, %.1f m, roh %.0f", dist / 100.f, raw);
        }
        else {
            float range = (float)attacker->GetFightRange();
            if (range < 50.f || range > 1500.f)
                range = 250.f;   // unplausibel: Standard-Nahkampfweite
            const float reach = range + CoopHitReachExtra;
            if (dist > reach) {
                SwingLog("%s: ausgewichen - zu weit (%.1f m, Reichweite %.1f m + %.1f m)", an, dist / 100.f, range / 100.f,
                    CoopHitReachExtra / 100.f);
                return;
            }
            zVEC3 at = attacker->GetAtVectorWorld();
            zVEC3 to = ph - pa;
            const float la = sqrtf(at.n[0] * at.n[0] + at.n[2] * at.n[2]);
            const float lt = sqrtf(to.n[0] * to.n[0] + to.n[2] * to.n[2]);
            float angle = 0.f;
            if (la > 0.01f && lt > 30.f) {
                const float c = (at.n[0] * to.n[0] + at.n[2] * to.n[2]) / (la * lt);
                angle = acosf(c < -1.f ? -1.f : (c > 1.f ? 1.f : c)) * 180.f / 3.14159265f;
                if (c < CoopHitAngleCos) {
                    SwingLog("%s: ausgewichen - steht nicht zu mir (%.0f Grad, %.1f m)", an, angle, dist / 100.f);
                    return;
                }
            }
            zSTRING keep;
            if (const char* par = CoopHeroParadeAni(keep)) {
                SwingLog("%s: ausgewichen - eigene Parade/Ausweichsprung (%s, %.1f m)", an, par, dist / 100.f);
                return;
            }
            _snprintf(why, sizeof(why), "Nahkampf, %.1f m von %.1f m, %.0f Grad", dist / 100.f, reach / 100.f, angle);
        }
        why[sizeof(why) - 1] = 0;

        // Angenommen: die eigene Engine fuehrt den Schlag aus und rechnet den Schaden. zNoFriendlyFire (vtable oCNpc::OnDamage)
        // laesst einen NPC-Treffer auf den Helden nur durch, wenn der NPC ihn als Gegner hat oder feindlich ist; CreateHit
        // verlangt fuer NPC-Angreifer enemy == Ziel (0x00627CE1). Darum vorher SetEnemy.
        CoopSwingPermit p;
        p.attacker = attacker;
        p.name = an;
        p.until = CurrentMs + 1500;
        p.used = false;
        CoopSwingPermits.push_back(p);
        attacker->SetEnemy(player);
        CoopCreateHitFn createHit = ranged ? NULL : CoopResolveCreateHit();
        oCAniCtrl_Human* ctrl = attacker->GetAnictrl();
        if (createHit && ctrl) {
            createHit(ctrl, player);
            SwingLog("%s: getroffen (%s) - eigene Engine rechnet den Schaden", an, why);
        }
        else {
            player->GetEM(false)->OnDamage(player, attacker, raw > 0.f ? raw : 1.f, mode, ph);
            SwingLog("%s: getroffen (%s) - eigene Engine rechnet den Schaden aus dem Rohwert", an, why);
        }
    }

    // Aus oCNpc_OnDamage_Hit (Client): Schaden am eigenen Helden - von einem bestaetigten Schlag?
    bool CoopSwingPermitTake(oCNpc* attacker) {
        for (auto& p : CoopSwingPermits) {
            if (!p.used && p.attacker == attacker && p.until >= CurrentMs) {
                p.used = true;
                return true;
            }
        }
        return false;
    }

    void CoopSwingHitBegin(oCNpc::oSDamageDescriptor& d, int hpBefore) {
        CoopSwingPending s;
        s.desc = &d;
        s.attacker = d.pNpcAttacker;
        s.name = CoopHitNpcName(d.pNpcAttacker);
        s.hpBefore = hpBefore;
        s.hpAtSound = -1;
        s.mode = d.enuModeDamage;
        s.frame = CoopSwingFrame;
        s.sounded = false;
        CoopSwingPendings.push_back(s);
    }

    bool CoopSwingDescIs(const void* desc) {
        for (auto& s : CoopSwingPendings)
            if (s.desc == desc && !s.sounded)
                return true;
        return false;
    }

    void CoopSwingHitSound(const void* desc, int hpAtSound) {
        for (auto& s : CoopSwingPendings) {
            if (s.desc == desc && !s.sounded) {
                s.sounded = true;
                s.hpAtSound = hpAtSound;
                return;
            }
        }
    }

    static void CoopSendHitResult(json& j) {
        j["type"] = SYNC_HIT_RESULT;
        j["id"] = MyselfId.ToChar();
        CoopTraceCount(TRACE_SENT, SYNC_HIT_RESULT);
        ReadyToSendJsons.enqueue(j);
    }

    // Jedes Bild (DamageProcessorLoop): Ergebnisse einen Durchlauf spaeter melden (Tod/Bewusstlosigkeit setzt die Engine ggf.
    // erst danach), abgelaufene Erlaubnisse aufraeumen, Aufstehen melden.
    void CoopSwingPulse() {
        CoopSwingFrame++;
        if (!ClientThread || ServerThread || !player)
            return;
        for (size_t i = 0; i < CoopSwingPendings.size();) {
            CoopSwingPending s = CoopSwingPendings[i];
            if (s.frame + 1 >= CoopSwingFrame) {
                i++;
                continue;
            }
            CoopSwingPendings.erase(CoopSwingPendings.begin() + i);
            const int hp = player->GetAttribute(NPC_ATR_HITPOINTS);
            int dmg = s.hpBefore - hp;
            if (dmg < 0)
                dmg = 0;
            const bool dead = player->IsDead() != 0;
            const bool unc = player->IsUnconscious() != 0;
            json j;
            j["att"] = s.name;
            j["dmg"] = dmg;
            j["hp"] = hp;
            j["hpmax"] = player->GetAttribute(NPC_ATR_HITPOINTSMAX);
            j["unc"] = unc ? 1 : 0;
            j["dead"] = dead ? 1 : 0;
            j["mode"] = s.mode;
            CoopSendHitResult(j);
            SwingLog("Ergebnis: %s traf den eigenen Helden, Schaden %d, HP %d -> %d%s%s - an den Host", s.name.c_str(), dmg,
                s.hpBefore, hp, unc ? ", bewusstlos" : "", dead ? ", tot" : "");
        }
        for (size_t i = 0; i < CoopSwingPermits.size();) {
            const CoopSwingPermit& p = CoopSwingPermits[i];
            if (p.used || p.until < CurrentMs) {
                if (!p.used)
                    SwingLog("%s: angenommen, aber die Engine hat keinen Schaden ausgeloest (kein Treffer am Helden)", p.name.c_str());
                CoopSwingPermits.erase(CoopSwingPermits.begin() + i);
                continue;
            }
            i++;
        }
        static bool wasDown = false;
        const bool down = player->IsUnconscious() != 0;
        if (wasDown && !down && !player->IsDead()) {
            json j;
            j["wake"] = 1;
            CoopSendHitResult(j);
            SwingLog("eigener Held steht wieder auf - an den Host");
        }
        wasDown = down;
    }

    // v98 (A16): Ladebeginn (Plugin.cpp) - Bestrafungs-Merker gelten fuer NPCs der alten Welt (nach dem Laden gleiche Namen,
    // andere Geschichte), Schlag-Erlaubnisse und -Ergebnisse halten Zeiger auf NPCs der alten Welt
    void CoopHitWorldGone() {
        CoopPunishMarks.clear();
        CoopSwingPermits.clear();
        CoopSwingPendings.clear();
    }

#else
    void CoopHitWorldGone() {}
    bool CoopClientHitAuthority() { return false; }
    void CoopSwingReport(oCNpc*, oCNpc*, oCNpc::oSDamageDescriptor&) {}
    void CoopFightSound(oCNpc*, oCNpc*) {}
    void CoopApplyHitResult(oCNpc*, json&, const std::string&) {}
    void CoopClientSwing(oCNpc*, const char*, json&) {}
    bool CoopSwingPermitTake(oCNpc*) { return false; }
    void CoopSwingHitBegin(oCNpc::oSDamageDescriptor&, int) {}
    bool CoopSwingDescIs(const void*) { return false; }
    void CoopSwingHitSound(const void*, int) {}
    void CoopSwingPulse() {}
    void CoopPunishPulse() {}
#endif
}
