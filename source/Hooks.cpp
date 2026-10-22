namespace GOTHIC_ENGINE {
    int LastHpBeforeDamage = -1;
    oCNpc::oSDamageDescriptor* LastIgnoredDamDesc = NULL;
    // v67: Schwierigkeitsgrad (CoopDifficulty.cpp) und HP-Abzug skalieren (CoopDamage.cpp)
    float CoopDiffPlayerFactor();
    float CoopDiffEnemyFactor();
    void CoopScaleBegin(oCNpc* target, float factor);
    void CoopScaleEnd();

#if ENGINE == Engine_G1
    // v36: Treffer einer Coop-Figur beim Host wie einen Treffer des Spielers behandeln. Othello/G1 B_CombatReactToDamage (die
    // Schadensreaktion in ZS_ATTACK) wechselt das Ziel nur bei Npc_IsPlayer(other):
    //   if ((Npc_GetPermAttitude(self, other) == ATT_FRIENDLY || self.npctype == NPCTYPE_FRIEND) && Npc_IsPlayer(other)) return;
    //   if (Npc_IsPlayer(other)) { Npc_SetTempAttitude(self, ATT_HOSTILE); if (Npc_GetAttitude(self, other) <= ATT_ANGRY) Npc_SetTarget(self, other); }
    // Fuer die Coop-Figur ist Npc_IsPlayer falsch - ein NPC, der den Host-Helden angriff, blieb bei ihm. Hier dasselbe fuer die
    // Figur: Haltung zum Spieler (Perm/Typ) statt der Gildenhaltung zur Figur (gil_none, neutral). Monster brauchen das nicht
    // (B_MM_ReactToDamage setzt das Ziel bei jedem Angreifer). Engine: SetEnemy 0x00691A80, GetPermAttitude 0x0068D3C0
    // (Othello liest dort nur das verlegte Gildenfeld), SetTmpAttitude 0x0068D460 - nicht virtuell, Eingaenge unveraendert.
    // v49 (Nutzer: "beim Skelett und NPCs ... entweder nicht reagiert oder den Helden gesucht"; Log Host: ORCBITER und FAKON_LURKER
    // von FRIEND_1 getroffen, Gegner blieb PC_HERO): Monster brauchen es doch. Im Kampf laeuft B_MM_ReactToCombatDamage
    // (G1-Quelle ZS_MM_Master.d: "if (other == hero) Npc_SetTarget(self, other)"; kompiliert vergleicht == bei Instanzen nur die
    // Symbolnummern, Othello-DAT: PUSHINT 1459 / PUSHINT 1456 / == - also nie wahr) - ein Monster, das den Host-Helden angreift,
    // blieb bei ihm. B_MM_ReactToDamage (noch nicht im Kampf) setzt das Ziel dagegen bei jedem Angreifer. Jetzt auch fuer
    // Monster- und Ork-Kampfzustaende (ZS_MM_ATTACK*, ZS_ORC_ATTACK): Ziel = Figur, ausser bei Partymitgliedern/Beschwoerungen
    // des Spielers (AIV_PARTYMEMBER, AIV_MM_PARTYMEMBER).
    static int CoopAivIndex(const char* name, int fallback) {
        zCPar_Symbol* s = parser ? parser->GetSymbol(name) : NULL;
        int v = fallback;
        if (s)
            s->GetValue(v, 0);
        return (v >= 0 && v < 50) ? v : -1;
    }

    static void CoopCombatRetarget(oCNpc* npc, oCNpc* figure) {
        static CoopDiagBudget lines = { 0, 0 };   // v50: je Minute statt je Sitzung
        static int aivParty = -2, aivMmParty = -2;
        if (!npc || !figure || npc == player || PlayerNpcs.count(npc) || npc->enemy == figure || npc->IsDead() || npc->IsUnconscious())
            return;
        const char* st = npc->state.curState.name.ToChar();
        const bool human = strcmp(st, "ZS_ATTACK") == 0;
        const bool monster = strncmp(st, "ZS_MM_ATTACK", 12) == 0 || strcmp(st, "ZS_ORC_ATTACK") == 0;   // v49
        if (!human && !monster)
            return;
        if (aivParty == -2) {
            aivParty = CoopAivIndex("AIV_PARTYMEMBER", 36);
            aivMmParty = CoopAivIndex("AIV_MM_PARTYMEMBER", 23);
        }
        if ((aivParty >= 0 && npc->aiscriptvars[aivParty]) || (aivMmParty >= 0 && npc->aiscriptvars[aivMmParty]))
            return;   // Begleiter/Beschwoerungen des Spielers nicht auf den Mitspieler hetzen
        if (human && (npc->GetPermAttitude(player) == NPC_ATT_FRIENDLY || npc->npcType == 3 /* NPCTYPE_FRIEND */))
            return;
        oCNpc* before = npc->enemy;
        if (human)
            npc->SetTmpAttitude(NPC_ATT_HOSTILE);
        npc->SetEnemy(figure);
        if (CoopDiagAllow(lines, 60))
            OthelloCompatLog("[%lld s, %s] Zielwechsel: %s greift jetzt %s an (vorher %s)", DiagSeconds(), DiagRole(),
                npc->GetObjectName().ToChar(), figure->GetObjectName().ToChar(),
                before == player ? "eigener Held" : (before ? before->GetObjectName().ToChar() : "-"));
    }

    // v98 (C20): Nutzer (Test v97): "Manchmal machen die NPCs diese Animation, wenn sie das erste Mal angegriffen werden, aber diese
    // kommt, wenn der Mitspieler sie angreift ... die NPCs sollten normal direkt wechseln". Log Host: NON_1503 (ZS_STANDAROUND) von
    // FRIEND_1 getroffen -> ZS_REACTTODAMAGE (B_WhirlAround: Umdrehen T_SURPRISE_*), dann ZS_ASSESSENEMY, ZS_ASSESSFIGHTER mit dem
    // Host-Helden als Gegner und nach 6 s zurueck in ZS_STANDAROUND - den Mitspieler griff er nie an. Ursache im Skript
    // (G1/Othello ZS_ReactToDamage): nur bei Npc_IsPlayer(other) wird die Haltung feindlich; fuer die Figur (ein NSC) geht es ohne
    // Haltungswechsel nach ZS_AssessEnemy, das sie (gil_none, neutral) nicht angreift. C3 half nur in ZS_ATTACK, denn
    // OnDamage_Hit laeuft vor der Wahrnehmung. Jetzt beim Host: startet ein Mensch ZS_REACTTODAMAGE hoechstens 1 s nach dem Treffer
    // einer Figur (ohne Treffer des Host-Helden dazwischen), startet er stattdessen ZS_ATTACK gegen sie - nach derselben Regel wie
    // C3: nicht bei Begleitern/Beschwoerungen des Spielers, nicht, wenn er dem Spieler freundlich ist (dort sagt auch beim
    // Spieler das Skript erst "Was soll das?"), nicht im Dialog. Engine wie C3/C13 (SetTmpAttitude, SetEnemy, StartAIState).
    struct CoopFigureHitNote { oCNpc* figure; long long ms; };
    static std::map<oCNpc*, CoopFigureHitNote> CoopFigureHits;

    static void CoopNoteFigureHit(oCNpc* npc, oCNpc* figure) {
        if (CoopFigureHits.size() > 256)
            CoopFigureHits.clear();   // nur kurzlebige Merker
        CoopFigureHits[npc] = { figure, CurrentMs };
    }

    static bool CoopFigureHitTurnsHostile(oCNpc* npc) {
        static int aivParty = -2, aivMmParty = -2;
        if (!npc || npc == player || PlayerNpcs.count(npc) || npc->IsDead() || npc->IsUnconscious() || !npc->IsHuman())
            return false;
        if (aivParty == -2) {
            aivParty = CoopAivIndex("AIV_PARTYMEMBER", 36);
            aivMmParty = CoopAivIndex("AIV_MM_PARTYMEMBER", 23);
        }
        if ((aivParty >= 0 && npc->aiscriptvars[aivParty]) || (aivMmParty >= 0 && npc->aiscriptvars[aivMmParty]))
            return false;
        if (npc->GetPermAttitude(player) == NPC_ATT_FRIENDLY || npc->npcType == 3 /* NPCTYPE_FRIEND */)
            return false;
        return !IsPlayerTalkingWithNpc(npc) && !CoopNpcInDialogWithPlayer(npc);
    }

    // v50 (C10): Nutzer (Test v49): "der Mitspieler war ausserhalb der Reichweite und sobald ein NPC angegriffen wurde, haben die
    // anderen nicht mehr reagiert, wenn sie angegriffen wurden". Log Host: VLK_10024_BUDDLER (saegt, ZS_SAW) von FRIEND_1 getroffen -
    // Zustand blieb ZS_SAW, kein Gegner; Bandit, Gardist und Reis-Buddler daneben reagierten. Ursache in Othellos Skript: ZS_SAW
    // baut keine Wahrnehmungen auf (kein B_SetPerception - als einziger Tagesablauf-Zustand, alle ZS_ des DAT geprueft), und
    // oCNpc_States::DoAIState loescht sie bei jedem Zustandswechsel (0x006C5DE8; Othello ersetzt den Aufruf durch
    // OTHELLO_ABI+0x79A40, das zuerst ClearPerception 0x006B78E0 ruft). Ohne PERC_ASSESSDAMAGE tut AssessDamage_S (0x006B5960,
    // aus OnDamage_Script ueber Othellos Vorbau OTHELLO_ABI+0x7E290, der es am Ende immer ruft) nichts - auch im Einzelspiel.
    // Jetzt beim Host: trifft ein Spieler (Host-Held oder Coop-Figur) einen Menschen in einem Tagesablauf-Zustand ohne
    // Schadens-Wahrnehmung, bekommt er sie wie in jedem anderen Arbeitszustand (ObservingPerception: PERC_ASSESSDAMAGE ->
    // ZS_ReactToDamage). OnDamage_Hit laeuft vor OnDamage_Script (oCNpc::OnDamage 0x0073138F/0x007313B0). Zurueck in ZS_SAW
    // ist er wieder taub, bis zum naechsten Treffer. Engine: HasPerception 0x006B7AA0, EnablePerception 0x006B7900,
    // oCNpc_States::IsInRoutine 0x006C6360 - nicht virtuell, unveraendert; IsHuman virtuell +114h -> OTHELLO_ABI+0x89530
    // (gleiche Signatur, liest das verlegte Gildenfeld +0xA34).
    static void CoopEnsureDamagePerception(oCNpc* npc) {
        static int reactFunc = -2;
        static CoopDiagBudget budget = { 0, 0 };
        if (!npc || npc == player || PlayerNpcs.count(npc) || npc->IsDead() || npc->IsUnconscious())
            return;
        if (npc->HasPerception(NPC_PERC_ASSESSDAMAGE) || !npc->state.IsInRoutine() || !npc->IsHuman())
            return;
        if (reactFunc == -2)
            reactFunc = parser ? parser->GetIndex("ZS_REACTTODAMAGE") : -1;
        if (reactFunc < 0)
            return;
        npc->EnablePerception(NPC_PERC_ASSESSDAMAGE, reactFunc);
        if (CoopDiagAllow(budget, 20))
            OthelloCompatLog("[%lld s, %s] Schadens-Wahrnehmung ergaenzt: %s (Zustand %s hatte keine) - reagiert jetzt auf den Treffer",
                DiagSeconds(), DiagRole(), npc->GetObjectName().ToChar(), npc->state.curState.name.ToChar());
    }

    // v100 (C21): Nutzer (Test v99): "es gab immer noch die Animation wie beim ersten Schlag auf einen NPC bei Fokuswechsel mit dem
    // Mitspieler". Log Host (Sarus, 721-754 s): alle 2 s "Kampf-Uebergabe an FRIEND_2" und "zurueck (anderes Ziel; Ziel HOST) -
    // Kampf geht beim Host weiter". Jede Uebergabe startet ZS_ATTACK neu (beim Client LeadMineGive, beim Host LeadHostBack, die
    // Umlenkung C13 ebenso), und Othellos ZS_Attack() ruft zu Beginn B_WhirlAround(self, other): kann der NPC das neue Ziel nicht
    // sehen (es steht hinter ihm - der Mitspieler, der ihn von hinten schlaegt), AI_WhirlAround = Ueberraschungs-Sprung T_SURPRISE_*,
    // sonst AI_TurnToNpc. Im Einzelspiel wechselt ein kaempfender NPC das Ziel ohne Neustart (B_CombatReactToDamage: Npc_SetTarget)
    // und dreht sich einfach. Jetzt: wer ZS_ATTACK wegen einer Uebergabe/Umlenkung neu startet, wird 2 s vorgemerkt; seine naechste
    // EV_WHIRLAROUND-Nachricht wird vor dem Start zu EV_TURNTOVOB (gleiches Ziel - normale Drehung). Der erste Treffer auf einen
    // ruhigen NPC (ZS_ReactToDamage, C20) bleibt wie im Einzelspiel. Engine (Speicherabbild geprueft): oCNpc::EV_WhirlAround
    // 0x0074ED50 und EV_TurnToVob 0x00750210 unveraendert, beide nur aus dem Nachrichten-Verteiler oCNpc::OnMessage (0x006A7463 /
    // 0x006A743C, push msg, ecx = NPC). Allgemein (GT).
    static std::map<oCNpc*, long long> CoopNoWhirlUntil;

    void CoopNoWhirl(oCNpc* npc) {
        if (!npc)
            return;
        if (CoopNoWhirlUntil.size() > 256)
            CoopNoWhirlUntil.clear();   // nur kurzlebige Merker
        CoopNoWhirlUntil[npc] = CurrentMs + 2000;
    }

    int __fastcall oCNpc_EV_WhirlAround_Coop(oCNpc* _this, void* vtable, oCMsgMovement* msg);
    CInvoke<int(__thiscall*)(oCNpc*, oCMsgMovement*)> Ivk_oCNpc_EV_WhirlAround_Coop(0x0074ED50, &oCNpc_EV_WhirlAround_Coop);
    int __fastcall oCNpc_EV_WhirlAround_Coop(oCNpc* _this, void* vtable, oCMsgMovement* msg) {
        if (_this && msg && !CoopNoWhirlUntil.empty() && msg->subType == oCMsgMovement::EV_WHIRLAROUND && !msg->IsInUse()) {
            auto it = CoopNoWhirlUntil.find(_this);
            if (it != CoopNoWhirlUntil.end()) {
                const bool fresh = it->second >= CurrentMs;
                CoopNoWhirlUntil.erase(it);
                if (fresh && msg->targetVob) {
                    static CoopDiagBudget budget = { 0, 0 };
                    if (CoopDiagAllow(budget, 20))
                        OthelloCompatLog("[%lld s, %s] Zielwechsel ohne Ueberraschungs-Sprung: %s dreht sich zu %s (ZS_ATTACK-Neustart nach "
                            "Uebergabe/Umlenkung)", DiagSeconds(), DiagRole(), _this->GetObjectName().ToChar(),
                            msg->targetVob == player ? "eigenem Helden" : msg->targetVob->GetObjectName().ToChar());
                    msg->subType = oCMsgMovement::EV_TURNTOVOB;
                    return _this->EV_TurnToVob(msg);
                }
            }
        }
        return Ivk_oCNpc_EV_WhirlAround_Coop(_this, msg);
    }

    // v98 (A16): Ladebeginn (Plugin.cpp) - Merker zeigen auf NPCs der alten Welt
    void CoopFigureHitsWorldGone() {
        CoopFigureHits.clear();
        CoopNoWhirlUntil.clear();   // v100 (C21)
    }
#else
    void CoopFigureHitsWorldGone() {}
    void CoopNoWhirl(oCNpc*) {}
#endif

#if ENGINE == Engine_G1
    // v53: Schaden an der Figur (Puppe) eines Clients fuehrt der Host nicht aus - bei einem NPC als Angreifer meldet er nur den
    // Schlag, entscheiden und rechnen tut der Client (CoopHitAuthority.cpp). Haken am Eingang oCNpc::OnDamage(oSDamageDescriptor&)
    // 0x00730FE0: kein Direktaufrufer; der vtable-Platz 0x007DDFEC zeigt unter Othello auf zNoFriendlyFire+0x3720, das nach
    // seiner Freund-Pruefung hierher weiterreicht. Damit laufen fuer die Puppe weder HP-Abzug noch Zucken, Ton, Blut oder
    // Bewusstlos/Tod - die zeigt sie nur, wenn der Client sie meldet (SYNC_HIT_RESULT). Ohne NPC-Angreifer (Sturz, Umgebung)
    // ebenso nichts: die HP des Helden fuehrt der Client.
    void __fastcall oCNpc_OnDamage_Coop(oCNpc* _this, void* vtable, oCNpc::oSDamageDescriptor& d);
    CInvoke<void(__thiscall*)(oCNpc*, oCNpc::oSDamageDescriptor&)> Ivk_oCNpc_OnDamage_Coop(0x00730FE0, &oCNpc_OnDamage_Coop);
    void __fastcall oCNpc_OnDamage_Coop(oCNpc* _this, void* vtable, oCNpc::oSDamageDescriptor& d) {
        if (ServerThread && !IsCoopPaused && _this && _this != player && PlayerNpcs.count(_this) && CoopClientHitAuthority()) {
            oCNpc* a = d.pNpcAttacker;
            // v72: ein NPC, den ein Client im Kampf fuehrt, ist hier Puppe - seine Treffer rechnet der Client selbst
            if (a && a != player && !PlayerNpcs.count(a) && !CoopLeadIsRemote(a))
                CoopSwingReport(a, _this, d);
            return;
        }
        Ivk_oCNpc_OnDamage_Coop(_this, d);
    }
#endif

#if ENGINE == Engine_G1
    // v68 (A10): Absturz beim Beenden (Nutzer: "wenn der Host/Client das Game schliesst, kommt es manchmal zu dieser Access
    // Violation" - zwei Bildschirmfotos, beide EIP 0x0062F1D4, Lesen von 0x00000088, EAX/EBX/ECX/EDI/EBP = 0).
    // 0x0062F1D4 = oCAniCtrl_Human::GetWaterLevel mit this = NULL (cmp [esi+0x88], 1). Von 16 Aufrufstellen passt nur
    // oCNpc::DropAllInHand (0x00694362): bei HP > 0 liest es Welt (+0xB8) und Animationssteuerung (+0x9B4, human_ai - wie
    // GetAnictrl 0x00691A00); ohne Welt geht es ueber 0x0069435E (xor edi, edi; mov ecx, ebx) in GetWaterLevel - ohne
    // Pruefung. DropAllInHand rufen nur DoDie und DropUnconscious; GT ruft DropUnconscious an Figuren/NPCs mit HP > 0 (RemoteNpc,
    // CoopHitAuthority), auch wenn die Figur gerade aus der Welt genommen ist (Beenden, Trennen). Dieselbe Luecke hat
    // oCNpc::CheckModelOverlays (0x0068DA20, aus jedem SetAttribute/ChangeAttribute): Mensch mit 1-2 HP -> GetWaterLevel ueber
    // +0x9B4 (0x0068DA8E). Allgemein (Engine-Luecke, von GT ausgeloest). Beide Haken ueberspringen nur genau den Fall, der sonst
    // abstuerzte (keine Animationssteuerung); Speicherabbild: weder Othello noch andere Plugins aendern die drei Funktionen.
    static void CoopCrashGuardLog(const char* where, oCNpc* npc) {
        static CoopDiagBudget budget = { 0, 0 };
        if (CoopDiagAllow(budget, 10))
            OthelloCompatLog("[%lld s, %s] Absturzschutz: %s an %s ohne Animationssteuerung (HP %d, Welt %s) uebersprungen",
                DiagSeconds(), DiagRole(), where, npc == player ? "eigenem Helden" : npc->GetObjectName().ToChar(),
                npc->attribute[NPC_ATR_HITPOINTS], npc->GetHomeWorld() ? "ja" : "nein");
    }

    void __fastcall oCNpc_DropAllInHand_Coop(oCNpc* _this, void* vtable);
    CInvoke<void(__thiscall*)(oCNpc*)> Ivk_oCNpc_DropAllInHand_Coop(0x00694230, &oCNpc_DropAllInHand_Coop);
    void __fastcall oCNpc_DropAllInHand_Coop(oCNpc* _this, void* vtable) {
        if (_this && !_this->human_ai && _this->attribute[NPC_ATR_HITPOINTS] > 0) {
            CoopCrashGuardLog("DropAllInHand", _this);
            return;
        }
        Ivk_oCNpc_DropAllInHand_Coop(_this);
    }

    void __fastcall oCNpc_CheckModelOverlays_Coop(oCNpc* _this, void* vtable);
    CInvoke<void(__thiscall*)(oCNpc*)> Ivk_oCNpc_CheckModelOverlays_Coop(0x0068DA20, &oCNpc_CheckModelOverlays_Coop);
    void __fastcall oCNpc_CheckModelOverlays_Coop(oCNpc* _this, void* vtable) {
        if (_this && !_this->human_ai) {
            const int hp = _this->attribute[NPC_ATR_HITPOINTS];
            if (hp > 0 && hp <= 2) {
                CoopCrashGuardLog("CheckModelOverlays", _this);
                return;
            }
        }
        Ivk_oCNpc_CheckModelOverlays_Coop(_this);
    }

    // v101 (A17): Absturz beim Host im Kampf (Test v100, Bildschirmfoto: Access Violation 0x005652E6 = zCModel::GetProgressPercent
    // +0x26, Lesen von 0x00000008, aufgerufen aus OTHELLO_ABI+0x87F29). OTHELLO_ABI+0x87890 ist Othellos Ersatz fuer
    // oCNpc::EV_Strafe (Aufruf aus oCNpc::OnMessage 0x006A7497, Bewegungs-Untertyp 13): beim ersten Durchlauf
    // zCModel::StartAni(ani, 0) (0x005612A0), danach sofort GetProgressPercent(ani). GetProgressPercent(int) (0x005652C0, unveraendert)
    // sucht die laufende Animation und liest ohne Pruefung aus ihr - laeuft sie nicht, Absturz. GT sperrt eigene Animationen
    // einer Puppe (Host: NPC, den ein Client fuehrt; Client: NPC, den der Host fuehrt - zCModel_StartAni unten), die Kampf-KI des
    // NPCs (oCAIHuman::FightMelee 0x00611872, EV_AttackForward/-Left/-Right) legt aber weiter Seitenschritte an. Allgemein (GT +
    // Engine-Luecke; Othello prueft hier ebenso wenig wie Gothic). Jetzt: laeuft die Animation nicht, liefert GetProgressPercent
    // 0 - Othellos Seitenschritt endet dann im naechsten Durchlauf von selbst (0x10088159: GetActiveAni == NULL -> fertig).
    // Ebenso die Fassung mit Namen (0x00565210, unveraendert). GetActiveAni(int) 0x00560D90 unveraendert.
    static void CoopProgressGuardLog(zCModel* model, const char* ani) {
        static CoopDiagBudget budget = { 0, 0 };
        if (!CoopDiagAllow(budget, 10))
            return;
        zCVob* v = model ? model->homeVob : NULL;
        oCNpc* npc = (v && v->GetCharacterClass() == 2) ? (oCNpc*)v : NULL;
        OthelloCompatLog("[%lld s, %s] Absturzschutz: GetProgressPercent(%s) ohne laufende Animation an %s%s - 0 statt Absturz",
            DiagSeconds(), DiagRole(), ani, npc == player ? "eigenem Helden" : (v ? v->GetObjectName().ToChar() : "?"),
            npc && CoopLeadIsRemote(npc) ? " (Puppe, ein Client fuehrt)" : "");
    }

    float __fastcall zCModel_GetProgressPercent_Coop(zCModel* _this, void* vtable, int aniID);
    CInvoke<float(__thiscall*)(zCModel*, int)> Ivk_zCModel_GetProgressPercent_Coop(0x005652C0, &zCModel_GetProgressPercent_Coop);
    float __fastcall zCModel_GetProgressPercent_Coop(zCModel* _this, void* vtable, int aniID) {
        if (_this && !_this->GetActiveAni(aniID)) {
            char id[24];
            _snprintf(id, sizeof(id), "%d", aniID);
            id[sizeof(id) - 1] = 0;
            CoopProgressGuardLog(_this, id);
            return 0.f;
        }
        return Ivk_zCModel_GetProgressPercent_Coop(_this, aniID);
    }

    float __fastcall zCModel_GetProgressPercentName_Coop(zCModel* _this, void* vtable, zSTRING const& name);
    CInvoke<float(__thiscall*)(zCModel*, zSTRING const&)> Ivk_zCModel_GetProgressPercentName_Coop(0x00565210, &zCModel_GetProgressPercentName_Coop);
    float __fastcall zCModel_GetProgressPercentName_Coop(zCModel* _this, void* vtable, zSTRING const& name) {
        if (_this) {
            const int id = _this->GetAniIDFromAniName(name);
            if (id < 0 || !_this->GetActiveAni(id)) {
                CoopProgressGuardLog(_this, name.ToChar());
                return 0.f;
            }
        }
        return Ivk_zCModel_GetProgressPercentName_Coop(_this, name);
    }

    // v105 (A19): Absturz beim Client kurz nach dem Laden (Test v104, "beim ersten Start gab es einen Crash"; Windows-Ereignis
    // 22.10. 22:11:01: GothicMod.exe c0000005 bei +0x1A50F, kein Eintrag in den Coop-Logs). 0x0041A50F = zCCSManager::
    // CreateNewCutscene(zSTRING&) mit this = NULL (mov eax, [ebx]), einziger Aufrufer zCCSPlayer::StartCutscene(zSTRING&, zCVob*)
    // (0x0041CD00, +0xE3: ownerManager +0x60 ohne Pruefung). StartCutscene ruft ueber npc->homeWorld->csPlayer (+0x64) und vtable
    // +0x1C oCNpc_States::DoAIState (0x006C612C, Tagesablauf-Zustand mit Cutscene) und oCNpc_States::StartCutscene (0x006C6450).
    // Ohne Manager ist der Player einer, den zCCSManager::RemovePlayerFromList (0x0041A2F0: SetOwnerManager(NULL)) abgemeldet hat,
    // waehrend die Welt ihn noch haelt - wer ihn abmeldet, ist offen (die Zeile unten zeigt die Umstaende). Ebenso ohne Pruefung:
    // StartOutputUnit (0x0041D2E0, Dialogzeilen) und CreateNewOutputUnit (0x0041D000). Alle drei unveraendert (Speicherabbild).
    // Jetzt: fehlt der Manager, haengt GT den Player der aktuellen Welt wieder an den Cutscene-Manager der Sitzung
    // (zCSession::csMan +0x04, SetOwnerManager 0x0041C840 - nur das Feld) und laesst den Aufruf laufen; geht das nicht (anderer
    // Player, kein Manager), liefert er NULL - wie die Engine, wenn Cutscenes gesperrt sind (DoAIState prueft auf NULL).
    // Allgemein G1 (Engine-Luecke; Ausloeser offen).
    static bool CoopCsOwnerCheck(zCCSPlayer* p, const char* what, const char* name, zCVob* vob) {
        if (!p || p->ownerManager)
            return true;
        zCCSManager* man = ogame ? ogame->csMan : NULL;
        zCWorld* w = ogame ? ogame->GetGameWorld() : NULL;
        const bool worldPlayer = w && w->csPlayer == p;
        int listed = 0;
        if (man)
            for (int i = 0; i < man->csPlayerList.GetNum(); i++)
                if (man->csPlayerList[i] == p)
                    listed = 1;
        if (man && worldPlayer)
            p->SetOwnerManager(man);
        static CoopDiagBudget budget = { 0, 0 };
        if (CoopDiagAllow(budget, 10))
            OthelloCompatLog("[%lld s, %s] Absturzschutz: %s(%s) an %s - Cutscene-Player %p ohne Manager (Welt-Player %s, Manager der "
                "Sitzung %p mit %d Playern, darin %s) - %s", DiagSeconds(), DiagRole(), what, name,
                vob ? (vob == player ? "eigenem Helden" : vob->GetObjectName().ToChar()) : "-", (void*)p, worldPlayer ? "ja" : "nein",
                (void*)man, man ? man->csPlayerList.GetNum() : 0, listed ? "ja" : "nein",
                p->ownerManager ? "wieder angehaengt, laeuft" : "nicht gestartet");
        return p->ownerManager != NULL;
    }

    zCCSCutsceneContext* __fastcall zCCSPlayer_StartCutscene_Coop(zCCSPlayer* _this, void* vtable, zSTRING& name, zCVob* vob);
    CInvoke<zCCSCutsceneContext*(__thiscall*)(zCCSPlayer*, zSTRING&, zCVob*)> Ivk_zCCSPlayer_StartCutscene_Coop(0x0041CD00, &zCCSPlayer_StartCutscene_Coop);
    zCCSCutsceneContext* __fastcall zCCSPlayer_StartCutscene_Coop(zCCSPlayer* _this, void* vtable, zSTRING& name, zCVob* vob) {
        if (!CoopCsOwnerCheck(_this, "StartCutscene", name.ToChar(), vob))
            return NULL;
        return Ivk_zCCSPlayer_StartCutscene_Coop(_this, name, vob);
    }

    zCCSCutsceneContext* __fastcall zCCSPlayer_StartOutputUnit_Coop(zCCSPlayer* _this, void* vtable, int ou, zCVob* vob);
    CInvoke<zCCSCutsceneContext*(__thiscall*)(zCCSPlayer*, int, zCVob*)> Ivk_zCCSPlayer_StartOutputUnit_Coop(0x0041D2E0, &zCCSPlayer_StartOutputUnit_Coop);
    zCCSCutsceneContext* __fastcall zCCSPlayer_StartOutputUnit_Coop(zCCSPlayer* _this, void* vtable, int ou, zCVob* vob) {
        char n[16];
        _snprintf(n, sizeof(n), "%d", ou);
        n[sizeof(n) - 1] = 0;
        if (!CoopCsOwnerCheck(_this, "StartOutputUnit", n, vob))
            return NULL;
        return Ivk_zCCSPlayer_StartOutputUnit_Coop(_this, ou, vob);
    }

    zCCSCutsceneContext* __fastcall zCCSPlayer_CreateNewOutputUnit_Coop(zCCSPlayer* _this, void* vtable, int ou, zCVob* vob);
    CInvoke<zCCSCutsceneContext*(__thiscall*)(zCCSPlayer*, int, zCVob*)> Ivk_zCCSPlayer_CreateNewOutputUnit_Coop(0x0041D000, &zCCSPlayer_CreateNewOutputUnit_Coop);
    zCCSCutsceneContext* __fastcall zCCSPlayer_CreateNewOutputUnit_Coop(zCCSPlayer* _this, void* vtable, int ou, zCVob* vob) {
        char n[16];
        _snprintf(n, sizeof(n), "%d", ou);
        n[sizeof(n) - 1] = 0;
        if (!CoopCsOwnerCheck(_this, "CreateNewOutputUnit", n, vob))
            return NULL;
        return Ivk_zCCSPlayer_CreateNewOutputUnit_Coop(_this, ou, vob);
    }

    // v69 (C16): beim Client keine eigene Kampf-KI fuer NPCs, die der Host fuehrt. Test v68: "der NPC hatte ja schon, waehrend
    // ich auf ihn geschlagen habe, dieses Problem" (dreht sich). Log Client: getroffene NPCs gingen hier in ZS_ASSESSENEMY,
    // ZS_ATTACK, ZS_PROCLAIMANDPUNISH, ZS_WATCHFIGHT (eigene Wahrnehmungen auf den eigenen Treffer und das Kampfgeraeusch), bis zu
    // 228 blockierte Animationen in 4 s, "bewegt 619" - die Kampf-KI dreht den NPC direkt (oCAniCtrl_Human, ohne Animation)
    // gegen die Blickrichtung vom Host; ihre Paraden liessen eigene Treffer hier mit Schaden 0 durchgehen. Beim Host kaempft
    // derselbe NPC schon gegen die Figur (Bestrafung, C9/C13); seine Schlaege auf den Client meldet der Host (C11).
    // Jetzt: oCNpc_States::StartAIState(int, ...) (0x006C5490; Othello hat darin nur den Aufruf von CloseCutscenes bei
    // +0x61 umgelegt, Eingang unveraendert) startet beim Client fuer einen benannten NPC (nicht Held/Figur, nicht tot, nicht im
    // Dialog mit dem Helden) keine Kampf-/Reaktionszustaende aus der Liste - der Auftrag gilt als erledigt (1), der NPC bleibt
    // im alten Zustand, Menschen werden dem Helden gegenueber feindlich (wie B_AssessDamage es setzen wuerde). Tagesablauf
    // (isRtnState), Dialog, Bewusstlos/Tod (Programmzustaende < 0) und alles andere bleiben. Abschaltbar:
    // GothicCoopConfig.json "coopClientNpcCombat": true. Allgemein (GT).
    static bool CoopClientCombatStateName(const char* n) {
        static const char* list[] = { "ZS_ATTACK", "ZS_ASSESSENEMY", "ZS_ASSESSFIGHTER", "ZS_ASSESSFIGHTERWAIT", "ZS_REACTTODAMAGE",
            "ZS_PROCLAIMANDPUNISH", "ZS_FLEE", "ZS_GOTOFIGHT", "ZS_WATCHFIGHT", "ZS_ORC_WATCHFIGHT", "ZS_FRIENDLYATTACK", "ZS_BERZERK",
            "ZS_ASSESSDEFEAT", "ZS_ASSESSMURDER", "ZS_ORCSLAVE_ASSESSDAMAGE", "ZS_BABE_FLEE", "ZS_MM_ATTACK", "ZS_MM_ATTACKMAGE",
            "ZS_MM_ASSESSENEMY", "ZS_MM_FLEE", "ZS_ORC_ATTACK" };
        for (const char* s : list)
            if (_stricmp(n, s) == 0)
                return true;
        return false;
    }

    int __fastcall oCNpc_States_StartAIState_Coop(oCNpc_States* _this, void* vtable, int index, int endOld, int timeBehaviour, float timed, int isRtnState);
    CInvoke<int(__thiscall*)(oCNpc_States*, int, int, int, float, int)> Ivk_oCNpc_States_StartAIState_Coop(0x006C5490, &oCNpc_States_StartAIState_Coop);
    int __fastcall oCNpc_States_StartAIState_Coop(oCNpc_States* _this, void* vtable, int index, int endOld, int timeBehaviour, float timed, int isRtnState) {
        static int allowed = -1;
        if (allowed < 0)
            allowed = (CoopConfig.contains("coopClientNpcCombat") && CoopConfig["coopClientNpcCombat"].is_boolean() &&
                CoopConfig["coopClientNpcCombat"].get<bool>()) ? 1 : 0;
        oCNpc* npc = _this ? _this->npc : NULL;
        // v72: beim Host fuer einen NPC, den ein Client im Kampf fuehrt (Puppe), dieselbe Sperre (CoopCombatLead.cpp)
        if (ServerThread && !WorldEditMode && !IsLoadingLevel && !isRtnState && index >= 0 && npc && CoopLeadIsRemote(npc) &&
            !npc->IsDead() && parser) {
            zCPar_Symbol* sym = parser->GetSymbol(index);
            const char* name = sym ? sym->name.ToChar() : NULL;
            if (name && CoopClientCombatStateName(name)) {
                static CoopDiagBudget hostBudget = { 0, 0 };
                if (CoopDiagAllow(hostBudget, 20))
                    OthelloCompatLog("[%lld s, %s] Kampf-KI hier aus: %s startet %s nicht (ein Client fuehrt den NPC im Kampf)",
                        DiagSeconds(), DiagRole(), npc->GetObjectName().ToChar(), name);
                return 1;
            }
        }
        // v98 (C20): erster Treffer einer Figur - direkt angreifen statt ZS_REACTTODAMAGE (Umdrehen, dann oft gar nichts)
        if (ServerThread && !WorldEditMode && !IsLoadingLevel && !isRtnState && index >= 0 && npc && parser && !CoopFigureHits.empty()) {
            auto fh = CoopFigureHits.find(npc);
            if (fh != CoopFigureHits.end()) {
                oCNpc* figure = fh->second.figure;
                const bool fresh = CurrentMs - fh->second.ms <= 1000;
                zCPar_Symbol* sym = parser->GetSymbol(index);
                const char* name = sym ? sym->name.ToChar() : NULL;
                if (!fresh)
                    CoopFigureHits.erase(fh);
                else if (name && _stricmp(name, "ZS_REACTTODAMAGE") == 0) {
                    CoopFigureHits.erase(fh);
                    static int attackIndex = -2;
                    if (attackIndex == -2)
                        attackIndex = parser->GetIndex("ZS_ATTACK");
                    if (attackIndex >= 0 && PlayerNpcs.count(figure) && !figure->IsDead() && CoopFigureHitTurnsHostile(npc)) {
                        npc->SetTmpAttitude(NPC_ATT_HOSTILE);
                        npc->SetEnemy(figure);
                        static CoopDiagBudget c20Budget = { 0, 0 };
                        if (CoopDiagAllow(c20Budget, 30))
                            OthelloCompatLog("[%lld s, %s] Erster Treffer: %s greift %s direkt an (statt ZS_REACTTODAMAGE)", DiagSeconds(),
                                DiagRole(), npc->GetObjectName().ToChar(), figure->GetObjectName().ToChar());
                        return Ivk_oCNpc_States_StartAIState_Coop(_this, attackIndex, endOld, timeBehaviour, timed, isRtnState);
                    }
                }
            }
        }
        // v72: den selbst gefuehrten NPC (Kampf-Uebergabe) nicht sperren
        if (!allowed && ClientThread && !ServerThread && !IsCoopPaused && !WorldEditMode && !IsLoadingLevel && !isRtnState && index >= 0 &&
            npc && npc != player && !PlayerNpcs.count(npc) && !npc->IsDead() && parser && NpcToUniqueNameList.count(npc) &&
            !IsPlayerTalkingWithNpc(npc) && !CoopNpcInDialogWithPlayer(npc) && !CoopLeadIsMine(npc)) {
            zCPar_Symbol* sym = parser->GetSymbol(index);
            const char* name = sym ? sym->name.ToChar() : NULL;
            // Begleiter und Beschwoerungen (AIV_PARTYMEMBER / AIV_MM_PARTYMEMBER) kaempfen weiter selbst
            static int aivParty = -2, aivMmParty = -2;
            if (aivParty == -2) {
                aivParty = CoopAivIndex("AIV_PARTYMEMBER", 36);
                aivMmParty = CoopAivIndex("AIV_MM_PARTYMEMBER", 23);
            }
            const bool party = (aivParty >= 0 && npc->aiscriptvars[aivParty]) || (aivMmParty >= 0 && npc->aiscriptvars[aivMmParty]);
            if (name && !party && CoopClientCombatStateName(name)) {
                if (npc->IsHuman() && player)
                    npc->SetTmpAttitude(NPC_ATT_HOSTILE);
                static CoopDiagBudget budget = { 0, 0 };
                if (CoopDiagAllow(budget, 30))
                    OthelloCompatLog("[%lld s, %s] Kampf-KI hier aus: %s startet %s nicht (der Host fuehrt den NPC; bleibt in %s)",
                        DiagSeconds(), DiagRole(), npc->GetObjectName().ToChar(), name, _this->curState.name.ToChar());
                return 1;
            }
        }
        return Ivk_oCNpc_States_StartAIState_Coop(_this, index, endOld, timeBehaviour, timed, isRtnState);
    }
#endif

    void __fastcall oCNpc_OnDamage_Hit(oCNpc*, void*, oCNpc::oSDamageDescriptor&);
#if ENGINE >= Engine_G2
    CInvoke<void(__thiscall*)(oCNpc*, oCNpc::oSDamageDescriptor&)> Ivk_oCNpc_OnDamage_Hit(0x00666610, &oCNpc_OnDamage_Hit);
#else
    CInvoke<void(__thiscall*)(oCNpc*, oCNpc::oSDamageDescriptor&)> Ivk_oCNpc_OnDamage_Hit(0x00731410, &oCNpc_OnDamage_Hit);
#endif
    void __fastcall oCNpc_OnDamage_Hit(oCNpc* _this, void* vtable, oCNpc::oSDamageDescriptor& damdesc) {
        if (damdesc.pNpcAttacker == player && IsCoopPlayer(_this->GetObjectName())) {
            return;
        }

        if (IsCoopPaused) {
            Ivk_oCNpc_OnDamage_Hit(_this, damdesc);
            return;
        }

#if ENGINE == Engine_G1
        if (ServerThread && damdesc.pNpcAttacker && damdesc.pNpcAttacker == player) {
            CoopPunishHostHit(_this);   // v54 (C13): Bestrafung durch dieses Opfer gilt dem Host zu Recht
            CoopFigureHits.erase(_this);   // v98 (C20): die Reaktion gilt jetzt dem Host-Helden
            if (_this->GetAttitude(damdesc.pNpcAttacker) == NPC_ATT_HOSTILE) {
                auto randValue = GetRandVal(0, 100);
                if (randValue > 33) {
                    _this->SetEnemy(damdesc.pNpcAttacker);
                }
            }
        }
        // v36: Coop-Figur wie der Spieler (vorher derselbe Zufallszweig - die Haltung zur Figur ist aber neutral, er griff nie)
        if (ServerThread && damdesc.pNpcAttacker && PlayerNpcs.count(damdesc.pNpcAttacker)) {
            CoopCombatRetarget(_this, damdesc.pNpcAttacker);
            CoopPunishMark(damdesc.pNpcAttacker, _this);   // v54 (C13): auch ohne Kampfgeraeusch (z. B. Zauber)
            CoopNoteFigureHit(_this, damdesc.pNpcAttacker);   // v98 (C20): gleich folgendes ZS_REACTTODAMAGE -> Angriff
        }
        // v50 (C10): Treffer eines Spielers auf einen NPC ohne Schadens-Wahrnehmung (Othellos ZS_SAW)
        if (ServerThread && damdesc.pNpcAttacker && (damdesc.pNpcAttacker == player || PlayerNpcs.count(damdesc.pNpcAttacker)))
            CoopEnsureDamagePerception(_this);
        // v53: vom Client bestaetigter Schlag eines NPCs auf den eigenen Helden (CoopHitAuthority.cpp) - hier rechnet die eigene
        // Engine den Schaden (Ruestung, Werte, Othellos Abzug). Nur mit Erlaubnis, sonst wie bisher verworfen (unten).
        // NpcsDamageMultipler gilt hierfuer nicht (Standard 100). v67: "Schaden der Gegner" (Host gibt vor) schon - den Schaden
        // rechnet hier der Client selbst und meldet ihn so an den Host.
        if (ClientThread && !ServerThread && _this == player && damdesc.pNpcAttacker && damdesc.pNpcAttacker != player &&
            damdesc.fDamageTotal != COOP_MAGIC_NUMBER && CoopSwingPermitTake(damdesc.pNpcAttacker)) {
            CoopSwingHitBegin(damdesc, _this->GetAttribute(NPC_ATR_HITPOINTS));
            CoopScaleBegin(_this, CoopDiffEnemyFactor());
            Ivk_oCNpc_OnDamage_Hit(_this, damdesc);
            CoopScaleEnd();
            return;
        }
        // v72 (Kampf-Uebergabe): den Schlag eines selbst gefuehrten NPCs rechnet die eigene Engine wie einen bestaetigten Schlag -
        // Ergebnis an den Host wie bisher (CoopSwingPulse)
        if (ClientThread && !ServerThread && _this == player && damdesc.pNpcAttacker && damdesc.pNpcAttacker != player &&
            damdesc.fDamageTotal != COOP_MAGIC_NUMBER && CoopLeadIsMine(damdesc.pNpcAttacker)) {
            CoopSwingHitBegin(damdesc, _this->GetAttribute(NPC_ATR_HITPOINTS));
            CoopScaleBegin(_this, CoopDiffEnemyFactor());
            Ivk_oCNpc_OnDamage_Hit(_this, damdesc);
            CoopScaleEnd();
            return;
        }
        // v72: eine Figur trifft den selbst gefuehrten NPC (nachgespielt) - ggf. Zielwechsel wie beim Host
        if (ClientThread && !ServerThread && damdesc.pNpcAttacker && PlayerNpcs.count(damdesc.pNpcAttacker) && CoopLeadIsMine(_this))
            CoopLeadHitBy(_this, damdesc.pNpcAttacker);
#endif
        // Blocking animations on clients for NPC is not preventing attacking sometimes (so do not call unless attack from the coop engine)
        if (ClientThread && 
            damdesc.pNpcAttacker != player && 
            (_this == player || IsCoopPlayer(_this->GetObjectName())) && 
            damdesc.fDamageTotal != COOP_MAGIC_NUMBER &&
            damdesc.enuModeDamage != oETypeDamage::oEDamageType_Fall) {
            return;
        }

        if (ClientThread && damdesc.pNpcAttacker != player) {
            Ivk_oCNpc_OnDamage_Hit(_this, damdesc);
            return;
        }

        if (ServerThread && PlayerNpcs.count(damdesc.pNpcAttacker)) {
            Ivk_oCNpc_OnDamage_Hit(_this, damdesc);
            return;
        }

        if (damdesc.pNpcAttacker == player && damdesc.fDamageTotal == COOP_MAGIC_NUMBER) {
            LastIgnoredDamDesc = &damdesc;
            Ivk_oCNpc_OnDamage_Hit(_this, damdesc);
            return;
        }

        float HpMultipler = 1.0;
        if (damdesc.pNpcAttacker == player) {
            HpMultipler = PlayersDamageMultipler / 100.0;
#if ENGINE == Engine_G1
            HpMultipler *= CoopDiffPlayerFactor();   // v67: Schaden der Spieler / Leben der Gegner
#endif
        }
        else if (_this == player) {
            HpMultipler = NpcsDamageMultipler / 100.0;
#if ENGINE == Engine_G1
            if (damdesc.pNpcAttacker && !PlayerNpcs.count(damdesc.pNpcAttacker))
                HpMultipler *= CoopDiffEnemyFactor();   // v67: Schaden der Gegner
#endif
        }

        int HpBeforeOnDamage = _this->GetAttribute(NPC_ATR_HITPOINTS);
        int MaxHpBeforeOnDamage = _this->GetAttribute(NPC_ATR_HITPOINTSMAX);
        LastHpBeforeDamage = HpBeforeOnDamage;
        CoopDamageBegin(&damdesc, _this, HpBeforeOnDamage);   // pro Treffer, nicht nur global

        if (HpMultipler == 1.0) {
            Ivk_oCNpc_OnDamage_Hit(_this, damdesc);
            return;
        }

#if ENGINE == Engine_G1
        // v67: den HP-Abzug selbst skalieren (ChangeAttribute-Haken, CoopDamage.cpp). Bisher: HP und Maximum auf 100000, danach
        // HP = vorher - Schaden x Faktor - dabei sah die Engine nie HP 0 (kein Tod/Bewusstlos im Treffer, zUtilities zeigte den
        // ungeteilten Schaden). So entscheidet die Engine wie sonst, mit dem skalierten Abzug.
        CoopScaleBegin(_this, (float)HpMultipler);
        Ivk_oCNpc_OnDamage_Hit(_this, damdesc);
        CoopScaleEnd();
        return;
#endif

        _this->SetAttribute(NPC_ATR_HITPOINTS, 100000);
        _this->SetAttribute(NPC_ATR_HITPOINTSMAX, 100000);

        Ivk_oCNpc_OnDamage_Hit(_this, damdesc);

        int RealDamage = (100000 - _this->GetAttribute(NPC_ATR_HITPOINTS));
        int HpDamage = RealDamage * HpMultipler;

        _this->SetAttribute(NPC_ATR_HITPOINTS, HpBeforeOnDamage - HpDamage);
        _this->SetAttribute(NPC_ATR_HITPOINTSMAX, MaxHpBeforeOnDamage);
    }

    void __fastcall oCNpc_OnDamage_Sound(oCNpc*, void*, oCNpc::oSDamageDescriptor&);
#if ENGINE >= Engine_G2
    CInvoke<void(__thiscall*)(oCNpc*, oCNpc::oSDamageDescriptor&)> Ivk_oCNpc_OnDamage_Sound(0x0067A8A0, &oCNpc_OnDamage_Sound);
#else
    CInvoke<void(__thiscall*)(oCNpc*, oCNpc::oSDamageDescriptor&)> Ivk_oCNpc_OnDamage_Sound(0x00746660, &oCNpc_OnDamage_Sound);
#endif
    void __fastcall oCNpc_OnDamage_Sound(oCNpc* _this, void* vtable, oCNpc::oSDamageDescriptor& damdesc) {
        if (damdesc.pNpcAttacker == player && IsCoopPlayer(_this->GetObjectName())) {
            return;
        }

#if ENGINE == Engine_G1
        // v53: bestaetigter Schlag auf den eigenen Helden (Client) - HP beim Ton merken, Ergebnis meldet CoopSwingPulse
        if (ClientThread && !ServerThread && _this == player && CoopSwingDescIs(&damdesc)) {
            CoopSwingHitSound(&damdesc, _this->GetAttribute(NPC_ATR_HITPOINTS));
            Ivk_oCNpc_OnDamage_Sound(_this, damdesc);
            return;
        }
#endif

        if (LastIgnoredDamDesc == &damdesc) {
            Ivk_oCNpc_OnDamage_Sound(_this, damdesc);
            LastIgnoredDamDesc = NULL;
            return;
        }

        if (IsCoopPaused) {
            Ivk_oCNpc_OnDamage_Sound(_this, damdesc);
            return;
        }

        bool IsFinishUnconsciousHitWithOnlySoundFunctionCall = !damdesc.pNpcAttacker && damdesc.bIsDead;
        if (IsFinishUnconsciousHitWithOnlySoundFunctionCall) {
            Ivk_oCNpc_OnDamage_Sound(_this, damdesc);
            return;
        }

        if (ClientThread && damdesc.pNpcAttacker != player) {
            Ivk_oCNpc_OnDamage_Sound(_this, damdesc);
            return;
        }

        if (ServerThread && PlayerNpcs.count(damdesc.pNpcAttacker)) {
            Ivk_oCNpc_OnDamage_Sound(_this, damdesc);
            return;
        }

        if (damdesc.pNpcAttacker == player && damdesc.fDamageTotal == COOP_MAGIC_NUMBER) {
            Ivk_oCNpc_OnDamage_Sound(_this, damdesc);
            return;
        }

        PlayerHit hit;
        hit.hpBefore = CoopDamageHpBefore(&damdesc, _this, LastHpBeforeDamage);
        hit.hpAtSound = _this->GetAttribute(NPC_ATR_HITPOINTS);
        hit.damage = (float)(hit.hpBefore - hit.hpAtSound);
        hit.attacker = damdesc.pNpcAttacker;
        hit.npc = _this;
        hit.isDead = _this->IsDead();
        hit.isUnconscious = _this->IsUnconscious();
        hit.damageMode = damdesc.enuModeDamage;
        hit.dmgTotal = damdesc.fDamageTotal;
        hit.dmgEffective = damdesc.fDamageEffective;
        hit.dmgReal = damdesc.fDamageReal;
        CoopDamageDefer(hit);   // endgueltig gemessen im naechsten Durchlauf (CoopDamage.cpp)

        Ivk_oCNpc_OnDamage_Sound(_this, damdesc);
    }

    void __fastcall oCNpc_EV_AttackFinish(oCNpc*, void*, oCMsgAttack*);
#if ENGINE >= Engine_G2
    CInvoke<void(__thiscall*)(oCNpc*, oCMsgAttack*)> Ivk_oCNpc_EV_AttackFinish(0x00751AF0, &oCNpc_EV_AttackFinish);
#else
    CInvoke<void(__thiscall*)(oCNpc*, oCMsgAttack*)> Ivk_oCNpc_EV_AttackFinish(0x006AC180, &oCNpc_EV_AttackFinish);
#endif
    void __fastcall oCNpc_EV_AttackFinish(oCNpc* _this, void* vtable, oCMsgAttack* attack) {
        if (!ClientThread || _this != player) {
            Ivk_oCNpc_EV_AttackFinish(_this, attack);
            return;
        }

        auto model = _this->GetModel();
        auto hitAni = model->GetAniFromAniID(attack->hitAni);

        if (model && hitAni && model->IsAniActive(hitAni)) {
            float progress = model->GetProgressPercent(attack->hitAni);
            if (progress >= 0.5 && attack->target) {
                oCNpc* enemy = zDYNAMIC_CAST<oCNpc>(attack->target);
                if (enemy && enemy->IsUnconscious())
                {
                    PlayerHit hit;
                    hit.damage = 1;
                    hit.attacker = player;
                    hit.npc = enemy;
                    hit.isDead = true;
                    hit.isUnconscious = 0;
                    hit.damageMode = oETypeDamage::oEDamageType_Edge;
                    ReadyToSyncDamages.enqueue(hit);
                }
            }
        }

        Ivk_oCNpc_EV_AttackFinish(_this, attack);
    }

    int __fastcall oCAIHuman_StandActions(oCAIHuman*, void*);
#if ENGINE >= Engine_G2
    CInvoke<int(__thiscall*)(oCAIHuman*)> Ivk_oCAIHuman_StandActions(0x00698EA0, &oCAIHuman_StandActions);
#else
    CInvoke<int(__thiscall*)(oCAIHuman*)> Ivk_oCAIHuman_StandActions(0x00612840, &oCAIHuman_StandActions);
#endif
    int __fastcall oCAIHuman_StandActions(oCAIHuman* _this, void* vtable) {
        // 18.10.2026: nur der Coop-CLIENT braucht das Freiraeumen des anvisierten NPCs (seine NPCs sind Marionetten des Hosts).
        // Bisher lief es auch im Einzelspieler (nur der Host war ausgenommen) und leerte die Warteschlange des Gespraechspartners
        // waehrend des Dialogstarts -> EV_PROCESSINFOS weg, Dialog ohne Zeilen, Held haengt (Dialog-Waechter-Log: Aufrufer
        // GOTHIC1COOP.DLL+0x3C0E4 = dieses ClearEM, Wegelagerer 15x, Cayden 4x in einer Sekunde).
        if (ServerThread || !ClientThread) {
            return Ivk_oCAIHuman_StandActions(_this);
        }

        if (_this->npc != player) {
            return Ivk_oCAIHuman_StandActions(_this);
        }

        auto focusedNpc = player->GetFocusNpc();
        if (!focusedNpc) {
            return Ivk_oCAIHuman_StandActions(_this);
        }

        // v72: den selbst gefuehrten NPC (Kampf-Uebergabe) nicht anfassen - seine KI laeuft hier
        if (CoopLeadIsMine(focusedNpc)) {
            return Ivk_oCAIHuman_StandActions(_this);
        }

        if (IsCoopPlayer(focusedNpc->GetObjectName())) {
            return 0;
        }

        if (focusedNpc->IsDead() || focusedNpc->IsUnconscious() || focusedNpc->GetWeaponMode() != NPC_WEAPON_NONE) {
            return Ivk_oCAIHuman_StandActions(_this);
        }

        // v26: spricht ein Mitspieler mit ihm, stehen dessen abgespielte Zeilen in der Warteschlange - nicht leeren
        // (AssessTalk_S im Original lehnt das Ansprechen dann ab).
        if (CoopDialogNpcBusy(focusedNpc)) {
            return Ivk_oCAIHuman_StandActions(_this);
        }

        // Laeuft schon ein Dialog mit genau diesem NPC, nichts loeschen (auch auf dem Client haengt sonst der Dialog).
        {
            oCInformationManager& im = oCInformationManager::GetInformationManager();
            if (im.Npc == focusedNpc && !im.IsDone) {
                return Ivk_oCAIHuman_StandActions(_this);
            }
            // ... ebenso, wenn das Gespraech schon angestossen ist (EV_PROCESSINFOS steht in seiner Warteschlange,
            // der Dialog-Manager hat ihn aber noch nicht uebernommen)
            if (zCEventManager* em = focusedNpc->GetEM(0)) {
                for (int i = 0; i < em->GetNumMessages(); i++) {
                    oCMsgConversation* m = zDYNAMIC_CAST<oCMsgConversation>(em->GetEventMessage(i));
                    if (m && m->subType == oCMsgConversation::EV_PROCESSINFOS) {
                        return Ivk_oCAIHuman_StandActions(_this);
                    }
                }
            }
        }

        auto activeAnims = GetCurrentAni(focusedNpc);
        for (auto ani : activeAnims) {
            focusedNpc->GetModel()->StopAnimation(ani->aniName);
        }

        focusedNpc->GetEM()->KillMessages();
        focusedNpc->ClearEM();

        return Ivk_oCAIHuman_StandActions(_this);
    }

    void __fastcall oCMag_Book_Spell_Cast(oCMag_Book*, void*);
#if ENGINE >= Engine_G2
    CInvoke<void(__thiscall*)(oCMag_Book*)> Ivk_oCMag_Book_Spell_Cast(0x004767A0, &oCMag_Book_Spell_Cast);
#else
    CInvoke<void(__thiscall*)(oCMag_Book*)> Ivk_oCMag_Book_Spell_Cast(0x0046FC00, &oCMag_Book_Spell_Cast);
#endif
    void __fastcall oCMag_Book_Spell_Cast(oCMag_Book* _this, void* vtable) {
        auto castingNpc = (oCNpc*)_this->owner;

        if (IsCoopPaused) {
            Ivk_oCMag_Book_Spell_Cast(_this);
            return;
        }

        if (!castingNpc) {
            Ivk_oCMag_Book_Spell_Cast(_this);
            return;
        }

        oCMag_Book* book = castingNpc->GetSpellBook();
        if (!book) {
            Ivk_oCMag_Book_Spell_Cast(_this);
            return;
        }

        if (IsCoopPlayer(castingNpc->GetObjectName())) {
            int spellID = book->GetSelectedSpellNr();
            if (spellID >= 0)
            {
                oCItem* item = book->GetSpellItem(spellID);
                if (item)
                {
                    auto itemName = item->GetInstanceName();
                    int insIndex = parser->GetIndex(itemName);
                    if (insIndex > 0) {
                        auto spellItem = CreateCoopItem(insIndex);
                        if (spellItem) {
                            castingNpc->DoPutInInventory(spellItem);
                        }
                    }
                }
            }
        }

        Ivk_oCMag_Book_Spell_Cast(_this);

        SpellCast sc;
        sc.npc = (oCNpc*)_this->owner;
        sc.targetNpc = sc.npc->GetFocusNpc();
        ReadyToSyncSpellCasts.enqueue(sc);
    }

    void __fastcall oCNpc_OpenDeadNpc(oCNpc*, void*);
#if ENGINE >= Engine_G2
    CInvoke<void(__thiscall*)(oCNpc*)> Ivk_oCNpc_OpenDeadNpc(0x00762970, &oCNpc_OpenDeadNpc);
#else
    CInvoke<void(__thiscall*)(oCNpc*)> Ivk_oCNpc_OpenDeadNpc(0x006BB890, &oCNpc_OpenDeadNpc);
#endif
    void __fastcall oCNpc_OpenDeadNpc(oCNpc* _this, void* vtable) {
        auto focusNpc = _this->GetFocusNpc();

        if (focusNpc && IsCoopPlayer(focusNpc->GetObjectName())) {
            return;
        }

        Ivk_oCNpc_OpenDeadNpc(_this);
    }

    int __fastcall oCNpc_CanUse(oCNpc*, void*, oCItem*);
#if ENGINE >= Engine_G2
    CInvoke<int(__thiscall*)(oCNpc*, oCItem*)> Ivk_oCNpc_CanUse(0x007319B0, &oCNpc_CanUse);
#else
    CInvoke<int(__thiscall*)(oCNpc*, oCItem*)> Ivk_oCNpc_CanUse(0x0068EF00, &oCNpc_CanUse);
#endif
    int __fastcall oCNpc_CanUse(oCNpc* _this, void* vtable, oCItem* n) {
        if (IsCoopPlayer(_this->GetObjectName())) {
            return true;
        }

        return Ivk_oCNpc_CanUse(_this, n);
    }

    void __fastcall oCWorld_RemoveVob(oCWorld*, void*, zCVob*);
#if ENGINE >= Engine_G2
    CInvoke<void(__thiscall*)(oCWorld*, zCVob*)> Ivk_oCWorld_RemoveVob(0x007800C0, &oCWorld_RemoveVob);
#else
    CInvoke<void(__thiscall*)(oCWorld*, zCVob*)> Ivk_oCWorld_RemoveVob(0x006D6EF0, &oCWorld_RemoveVob);
#endif
    void __fastcall oCWorld_RemoveVob(oCWorld* _this, void* vtable, zCVob* vob) {
        if (vob && vob->GetCharacterClass() == 2)
            CoopCamNpcGone((oCNpc*)vob);   // v44: folgt die Kamera dieser Figur - vorher zurueck zum Helden

        if (IsSavingGame) {
            return Ivk_oCWorld_RemoveVob(_this, vob);
        }

        CoopWorldSyncItemLeavesWorld(vob, _ReturnAddress(), "RemoveVob");   // v14: Gegenstand verlaesst die Welt (Aufheben, Quickloot)

        if (vob->GetCharacterClass() == 2) {
            auto npc = (oCNpc*)vob;
            if (npc && !IsCoopPlayer(npc->GetObjectName())) {
                if (NpcToUniqueNameList.count(npc) > 0) {
                    auto uniqueName = NpcToUniqueNameList[npc];
                    if (BroadcastNpcs.count(uniqueName) > 0) {
                        auto player = BroadcastNpcs[uniqueName];
                        if (player) {
                            player->destroyed = true;
                            BroadcastNpcs.erase(BroadcastNpcs.find(uniqueName));
                        }
                    }
                    if (SyncNpcs.count(uniqueName) > 0) {
                        auto remoteNpc = SyncNpcs[uniqueName];
                        if (remoteNpc) {
                            remoteNpc->destroyed = true;
                            SyncNpcs.erase(SyncNpcs.find(uniqueName));
                        }
                    }

                    UniqueNameToNpcList.erase(uniqueName);
                    NpcToUniqueNameList.erase(npc);
                }
            }
        }

        return Ivk_oCWorld_RemoveVob(_this, vob);
    }

    void __fastcall zCModel_StartAni(zCModel*, void*, zCModelAni*, int);
#if ENGINE >= Engine_G2
    CInvoke<void(__thiscall*)(zCModel*, zCModelAni*, int)> Ivk_zCModel_StartAni(0x0057B0C0, &zCModel_StartAni);
#else
    CInvoke<void(__thiscall*)(zCModel*, zCModelAni*, int)> Ivk_zCModel_StartAni(0x005612F0, &zCModel_StartAni);
#endif
    void __fastcall zCModel_StartAni(zCModel* _this, void* vtable, zCModelAni* a, int b) {
        if (IsLoadingLevel) {
            Ivk_zCModel_StartAni(_this, a, b);
            return;
        }

        if (zinput->KeyPressed(KEY_F2) && !WorldEditMode) {
            Ivk_zCModel_StartAni(_this, a, b);
            return;
        }

        if (IsCoopPaused) {
            Ivk_zCModel_StartAni(_this, a, b);
            return;
        }

#if ENGINE == Engine_G1
        // v72 (Kampf-Uebergabe, CoopCombatLead.cpp): Host - ein NPC, den ein Client fuehrt, ist Puppe: nur Animationen vom Client,
        // dazu eigene Fall-/Liege-/Todesanimationen (Treffer des Host-Helden). Client - den selbst gefuehrten NPC spielt die eigene
        // Engine wie den Helden.
        if (_this->homeVob && _this->homeVob->GetCharacterClass() == 2) {
            oCNpc* led = (oCNpc*)_this->homeVob;
            if (ServerThread && CoopLeadIsRemote(led)) {
                if (b == COOP_MAGIC_NUMBER)
                    Ivk_zCModel_StartAni(_this, a, 0);
                else if (led->IsDead() || led->IsUnconscious())
                    Ivk_zCModel_StartAni(_this, a, b);
                else
                    CoopDiagAniBlocked(_this->homeVob, a);
                return;
            }
            if (ClientThread && !ServerThread && CoopLeadIsMine(led)) {
                Ivk_zCModel_StartAni(_this, a, b == COOP_MAGIC_NUMBER ? 0 : b);
                return;
            }
        }
#endif

        if (ClientThread && b != COOP_MAGIC_NUMBER && _this->homeVob && IgnoredSyncNpc(_this->homeVob)) {
            Ivk_zCModel_StartAni(_this, a, 0);
            return;
        }

        if (ClientThread && b == COOP_MAGIC_NUMBER && _this->homeVob && IgnoredSyncNpc(_this->homeVob)) {
            return;
        }

        if (!ClientThread || _this->homeVob == player) {
            Ivk_zCModel_StartAni(_this, a, b);
            return;
        }

        if (ClientThread && b == COOP_MAGIC_NUMBER) {
            Ivk_zCModel_StartAni(_this, a, 0);
            return;
        }

        // v70 (C17): Treffer auf eine Figur, den der Client gerade nachspielt (RemoteNpc::UpdateAttacks) - die Treffer-Reaktion,
        // die Gothics OnDamage_Anim dafuer startet, hier zulassen (vorher blockiert: der Treffer brach nur die laufende Animation ab)
        if (ClientThread && CoopFigureHitAniNpc && _this->homeVob == CoopFigureHitAniNpc) {
            Ivk_zCModel_StartAni(_this, a, b);
            return;
        }

        // Drehungen des anvisierten NPCs immer zulassen: Bevor ein Dialog die Kamera umschaltet,
        // muss sich der NPC zum Spieler drehen (AI_TurnToNpc); die Engine wartet, bis die Drehung
        // fertig ist. Welche Dreh-Animation gespielt wird, haengt von Waffen-/Laufmodus und Mod ab
        // (T_WALKTURNL, T_RUNTURNR, T_WALKWTURNL, ...), die feste Liste reichte nicht.
        if (!WorldEditMode && player && player->GetFocusNpc() == _this->homeVob && a && a->aniName &&
           (a->aniName == "S_RUN" || a->aniName == "S_WALK" || a->aniName == "T_LOOK" || a->aniName == "T_WALKTURNR" || a->aniName == "T_WALKTURNL" ||
            strstr(a->aniName.ToChar(), "TURN") != NULL))
        {
            Ivk_zCModel_StartAni(_this, a, b);
            return;
        }

        if (ClientThread && _this->homeVob && _this->homeVob->GetCharacterClass() != 2) {
            Ivk_zCModel_StartAni(_this, a, b);
            return;
        }

        if (ClientThread && (IsPlayerTalkingWithNpc(_this->homeVob) || CoopNpcInDialogWithPlayer(_this->homeVob))) {
            Ivk_zCModel_StartAni(_this, a, b);
            return;
        }

        auto npc = (oCNpc*)_this->homeVob;
        if (ClientThread && npc && npc->IsDead()) {
            Ivk_zCModel_StartAni(_this, a, b);
            return;
        }

        // v69 (C15): bewusstlos (K.o. hier - eigener Treffer oder vom Host nachgespielt): Fall- und Liegeanimationen laufen hier
        // sofort. Test v68: "wenn man jemanden niederschlaegt, faellt er erst nach 2 Sekunden zu Boden und es sieht seltsam
        // aus" - Log Client: T_STAND_2_WOUNDEDB blockiert (Zustand ZS_UNCONSCIOUS), gefallen ist er erst mit der Animation des
        // Hosts; Gothics oCNpc::CheckUnconscious (0x00692F20, je KI-Durchlauf) erwartet T_STAND_2_WOUNDED[B] am Modell. Die
        // Fall-/Liegeanimationen des Hosts laesst RemoteNpc::UpdateAnimation dann aus (sonst stand er auf und fiel erneut).
        if (ClientThread && npc && npc->IsUnconscious()) {
            Ivk_zCModel_StartAni(_this, a, b);
            return;
        }

        // TMP: Use the logic to start/stop anim only for NB, it works by just not calling the function in g1/g2
        if (FriendInstanceId.Compare("pc_heromul") && a && a->aniName && (a->aniName == "S_RUN" || a->aniName == "S_FISTRUN")) {
            auto activeBefore = _this->IsAnimationActive(a->aniName);
            Ivk_zCModel_StartAni(_this, a, b);

            if (activeBefore == 0) {
                _this->StopAnimation(a->aniName);
            }
            return;
        }

        CoopDiagAniBlocked(_this->homeVob, a);   // blockiert: eigene Animation eines NPCs auf dem Client
    }

    // 
    // The body of this hook is copied from SaveErrorDetails because I cannot call SaveErrorDetails here no idea why.
    // TODO: Refactor :)
    // 
    // v101 (A18): Test v100 - "erstmal Game-Freeze am Anfang": Windows-Ereignisprotokoll 20:43:37, GothicMod.exe c0000005 an
    // Adresse 0 (Aufruf ueber einen leeren Zeiger), kein Eintrag in den Coop-Logs. Die Fehlerfenster (GTs Meldung und Gothics
    // "Access Violation") lagen im Vollbild unsichtbar hinter dem Spiel - es sah aus wie ein Haenger. Jetzt schreibt der
    // Absturz-Haken zuerst ins Log (Ausnahme, Stelle als Modul+Versatz, Register, Rueckspruenge vom Stapel) und legt das
    // Spielfenster auf die Taskleiste, damit die Fenster zu sehen sind. Nur Windows-Aufrufe, keine Engine-Stelle.
    static bool CoopCrashIsCode(DWORD a) {
        MEMORY_BASIC_INFORMATION mbi;
        if (a < 0x10000 || !VirtualQuery((LPCVOID)a, &mbi, sizeof(mbi)))
            return false;
        if (mbi.State != MEM_COMMIT || mbi.Type != MEM_IMAGE)
            return false;
        return (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0;
    }

    // Rueckspruenge vom Stapel: Werte, die in Code zeigen und hinter einem call stehen (E8 rel32, FF /2)
    static int CoopCrashScanStack(DWORD esp, DWORD* out, int max) {
        int n = 0;
        __try {
            const NT_TIB* tib = (const NT_TIB*)NtCurrentTeb();
            DWORD top = (DWORD)tib->StackBase;
            if (esp < (DWORD)tib->StackLimit || esp >= top)
                return 0;
            if (top > esp + 0x4000)
                top = esp + 0x4000;
            for (DWORD p = esp; p + 4 <= top && n < max; p += 4) {
                const DWORD a = *(const DWORD*)p;
                if (!CoopCrashIsCode(a) || !CoopCrashIsCode(a - 7))
                    continue;
                const unsigned char* c = (const unsigned char*)a;
                if (c[-5] == 0xE8 || (c[-6] == 0xFF && (c[-5] & 0x38) == 0x10) || (c[-3] == 0xFF && (c[-2] & 0x38) == 0x10) ||
                    (c[-2] == 0xFF && (c[-1] & 0x38) == 0x10))
                    out[n++] = a;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
        return n;
    }

    static void CoopCrashWhere(DWORD addr, char* out, size_t len) {
        HMODULE m = NULL;
        char path[MAX_PATH] = "";
        if (addr && GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)addr, &m) &&
            m && GetModuleFileNameA(m, path, MAX_PATH)) {
            const char* file = strrchr(path, '\\');
            _snprintf(out, len, "%s+0x%X", file ? file + 1 : path, addr - (DWORD)m);
        }
        else
            _snprintf(out, len, "0x%08X", addr);
        out[len - 1] = 0;
    }

    static void CoopCrashLog(struct _EXCEPTION_POINTERS* ep, const char* title = "ABSTURZ: Ausnahme") {
        if (!ep || !ep->ExceptionRecord || !ep->ContextRecord)
            return;
        const EXCEPTION_RECORD* er = ep->ExceptionRecord;
        const CONTEXT* c = ep->ContextRecord;
        char where[160], access[64] = "";
        CoopCrashWhere(c->Eip, where, sizeof(where));
        if (er->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && er->NumberParameters >= 2)
            _snprintf(access, sizeof(access), " (%s 0x%08X)", er->ExceptionInformation[0] == 0 ? "Lesen von" :
                (er->ExceptionInformation[0] == 8 ? "Ausfuehren bei" : "Schreiben nach"), (DWORD)er->ExceptionInformation[1]);
        access[sizeof(access) - 1] = 0;
        OthelloCompatLog("[%lld s, %s] %s %08X bei %s%s | Faden %lu | Zustand %s", DiagSeconds(), DiagRole(), title,
            er->ExceptionCode, where, access, GetCurrentThreadId(), PluginState.c_str());
        OthelloCompatLog("  Register: EAX=%08X EBX=%08X ECX=%08X EDX=%08X ESI=%08X EDI=%08X EBP=%08X ESP=%08X", c->Eax, c->Ebx, c->Ecx,
            c->Edx, c->Esi, c->Edi, c->Ebp, c->Esp);
        DWORD ret[24];
        const int n = CoopCrashScanStack(c->Esp, ret, 24);
        std::string chain;
        for (int i = 0; i < n; i++) {
            char w[160];
            CoopCrashWhere(ret[i], w, sizeof(w));
            chain += i ? " <- " : "";
            chain += w;
        }
        OthelloCompatLog("  Rueckspruenge vom Stapel (neueste zuerst): %s", n ? chain.c_str() : "-");
    }

    // Einmal je Absturz: ins Log, dann das Spielfenster auf die Taskleiste (Fehlerfenster sichtbar)
    static volatile LONG CoopCrashLogged = 0;

    static void CoopCrashReport(struct _EXCEPTION_POINTERS* ep) {
        if (InterlockedExchange(&CoopCrashLogged, 1))
            return;
        CoopCrashLog(ep);
#if ENGINE == Engine_G1
        HWND gameWnd = hWndApp ? hWndApp : *(HWND*)0x0086D30C;   // wie CoopMenuMouse.cpp
#else
        HWND gameWnd = hWndApp;
#endif
        if (gameWnd)
            ShowWindowAsync(gameWnd, SW_MINIMIZE);
    }

    // v101 (A18b): Othello stellt einen eigenen Absturz-Filter (OTHELLO_ABI+0x70810 ueber SetUnhandledExceptionFilter, gesetzt von
    // seinem Ersatz fuer zCExceptionHandler::SetIsActive OTHELLO_ABI+0x70960 und beim Start +0x70B40; er schreibt
    // system\Othello_Crash_Reports.txt und zeigt "== application crashed") und reicht NICHT an Gothics Filter weiter - GTs Haken
    // auf zCExceptionHandler::UnhandledExceptionFilter (0x004BF560) lief unter Othello nie. Jetzt legt sich GT selbst davor (beim
    // Start und in jedem Spiel- und Menuebild, weil Othello seinen Filter bei SetIsActive neu setzt) und reicht danach an den
    // vorigen Filter weiter - Othellos Bericht und Fenster bleiben. Allgemein: ohne Othello ist der vorige Gothics Filter.
    static LPTOP_LEVEL_EXCEPTION_FILTER CoopNextCrashFilter = NULL;
    static volatile LONG CoopCrashFilterDepth = 0;

    static LONG WINAPI CoopTopCrashFilter(struct _EXCEPTION_POINTERS* ep) {
        if (InterlockedIncrement(&CoopCrashFilterDepth) > 1) {   // Absturz im Absturz-Filter oder Kreis - nicht noch einmal
            InterlockedDecrement(&CoopCrashFilterDepth);
            return EXCEPTION_CONTINUE_SEARCH;
        }
        CoopCrashReport(ep);
        LPTOP_LEVEL_EXCEPTION_FILTER next = CoopNextCrashFilter;
        const LONG r = next ? next(ep) : EXCEPTION_CONTINUE_SEARCH;
        InterlockedDecrement(&CoopCrashFilterDepth);
        return r;
    }

    // v105 (A20): Test v104 - der Absturz beim Client (A19) lief an GTs und Othellos Filter vorbei (kein "ABSTURZ:" im Log,
    // Othello_Crash_Reports.txt unveraendert, Windows meldete ihn selbst). Warum, ist offen (im Prozess sind u. a. Steams
    // CSERHelper.dll und gameoverlayrenderer.dll; bei jedem Start meldet Windows ausserdem c0000409 Code 21 aus KERNELBASE -
    // "ungueltige Ausnahme-Kette" - ohne Folgen). Darum jetzt zusaetzlich eine Erfassung vor allen Filtern: ein Vectored Exception
    // Handler (AddVectoredExceptionHandler, als erster; alle 10 s neu vorn, falls sich ein anderer davorlegt) schreibt die ersten
    // drei Zugriffsfehler/ungueltigen Befehle/Divisionen durch 0 je Sitzung (je Stelle einmal) mit Registern und Ruecksprungen ins
    // Log - noch bevor entschieden ist, ob jemand sie abfaengt ("erste Stufe"). Er aendert nichts am Ablauf
    // (EXCEPTION_CONTINUE_SEARCH). Allgemein (Windows).
    static PVOID CoopVehHandle = NULL;
    static volatile LONG CoopVehInside = 0;
    static volatile LONG CoopVehCount = 0;
    static DWORD CoopVehSeen[3] = { 0, 0, 0 };
    static long long CoopVehRenewMs = 0;

    static LONG CALLBACK CoopFirstChanceHandler(struct _EXCEPTION_POINTERS* ep) {
        if (!ep || !ep->ExceptionRecord || !ep->ContextRecord)
            return EXCEPTION_CONTINUE_SEARCH;
        const DWORD code = ep->ExceptionRecord->ExceptionCode;
        if (code != EXCEPTION_ACCESS_VIOLATION && code != EXCEPTION_ILLEGAL_INSTRUCTION && code != EXCEPTION_INT_DIVIDE_BY_ZERO &&
            code != EXCEPTION_PRIV_INSTRUCTION)
            return EXCEPTION_CONTINUE_SEARCH;
        if (CoopVehCount >= 3)
            return EXCEPTION_CONTINUE_SEARCH;
        if (InterlockedIncrement(&CoopVehInside) > 1) {   // Ausnahme beim Schreiben (Stapel lesen) - nicht noch einmal
            InterlockedDecrement(&CoopVehInside);
            return EXCEPTION_CONTINUE_SEARCH;
        }
        const DWORD eip = ep->ContextRecord->Eip;
        bool seen = false;
        for (int i = 0; i < 3; i++)
            if (CoopVehSeen[i] == eip)
                seen = true;
        if (!seen) {
            const LONG n = InterlockedIncrement(&CoopVehCount) - 1;
            if (n >= 0 && n < 3) {
                CoopVehSeen[n] = eip;
                CoopCrashLog(ep, "Ausnahme (erste Stufe - faengt sie niemand ab, ist es der Absturz):");
            }
        }
        InterlockedDecrement(&CoopVehInside);
        return EXCEPTION_CONTINUE_SEARCH;
    }

    static void CoopVehPulse() {
        const long long now = GetCurrentMs();
        if (CoopVehHandle && now - CoopVehRenewMs < 10000)
            return;
        CoopVehRenewMs = now;
        PVOID h = AddVectoredExceptionHandler(1, &CoopFirstChanceHandler);
        if (CoopVehHandle)
            RemoveVectoredExceptionHandler(CoopVehHandle);
        else if (h)
            OthelloCompatLog("Absturz-Erfassung: Vectored Exception Handler gesetzt (erste Stufe, max. 3 Stellen je Sitzung)");
        CoopVehHandle = h;
    }

    void CoopCrashFilterPulse() {
        CoopVehPulse();
        LPTOP_LEVEL_EXCEPTION_FILTER cur = SetUnhandledExceptionFilter(&CoopTopCrashFilter);
        if (cur == &CoopTopCrashFilter || cur == CoopNextCrashFilter)
            return;
        CoopNextCrashFilter = cur;
        static int logged = 0;
        if (logged++ < 10) {
            char w[160];
            CoopCrashWhere((DWORD)cur, w, sizeof(w));
            OthelloCompatLog("Absturz-Filter: GT liegt vorn, danach %s", cur ? w : "kein weiterer (Windows)");
        }
    }

    static void __cdecl zCExceptionHandlerUnhandledExceptionFilter(struct _EXCEPTION_POINTERS*);
#if ENGINE >= Engine_G2
    CInvoke<void(*)(struct _EXCEPTION_POINTERS*)> Ivk_zCExceptionHandlerUnhandledExceptionFilter(0x004C88C0, &zCExceptionHandlerUnhandledExceptionFilter);
#else
    CInvoke<void(*)(struct _EXCEPTION_POINTERS*)> Ivk_zCExceptionHandlerUnhandledExceptionFilter(0x004BF560, &zCExceptionHandlerUnhandledExceptionFilter);
#endif
    void zCExceptionHandlerUnhandledExceptionFilter(struct _EXCEPTION_POINTERS* pointers) {
        TrackLastExecutedFunctions = false;
        CoopCrashReport(pointers);   // v101 (A18): zuerst ins Log, Fenster minimieren (einmal - auch aus CoopTopCrashFilter)

        HANDLE process;
        process = GetCurrentProcess();
        DWORD64 dllBase = (DWORD64)GetModuleHandleA("GothicCoop.dll");
        std::vector<std::string> lastMethodCalls;
        std::vector<std::string> lastCoreMethodCalls;

        for (int i = 1; i <= LastExecutedFunctionAddressesMaxLimit; i++) {
            int currentFuncIndex = LastExecutedFunctionAddressesIndex + i;
            if (currentFuncIndex > LastExecutedFunctionAddressesMaxLimit - 1) {
                currentFuncIndex = LastExecutedFunctionAddressesMaxLimit - (i + LastExecutedFunctionAddressesIndex);
                if (currentFuncIndex < 0) {
                    currentFuncIndex = -currentFuncIndex;
                }
            }

            DWORD64 dwDisplacement = 0;
            DWORD64 dwAddress = (DWORD64)(LastExecutedFunctionAddresses[currentFuncIndex]);

            char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
            PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
            pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            pSymbol->MaxNameLen = MAX_SYM_NAME;

            if (SymFromAddr(process, dwAddress, &dwDisplacement, pSymbol))
            {
                if (i >= LastExecutedFunctionAddressesMaxLimit - 11 && i != LastExecutedFunctionAddressesMaxLimit) {
                    lastMethodCalls.push_back(pSymbol->Name);
                }

                auto symbolName = std::string(pSymbol->Name);
                if (symbolName.rfind("Gothic_", 0) == 0 && i != LastExecutedFunctionAddressesMaxLimit) {
                    lastCoreMethodCalls.push_back(symbolName);
                }
            }
            else
            {
                auto log = string::Combine("GothicCoop.dll+%i\n", dwAddress - dllBase).ToChar();
                if (i >= LastExecutedFunctionAddressesMaxLimit - 11 && i != LastExecutedFunctionAddressesMaxLimit) {
                    lastMethodCalls.push_back(log);
                }
            }
        }

        // v97 (A12): wie A1 in Debugger.cpp - unter Othello bleibt die Liste leer, end() - 10 war dann ein ungueltiger Zeiger und
        // der Absturz-Haken stuerzte selbst ab, bevor er die Meldung zeigen konnte. Upstream: Gothic-Workshop a05c6b6.
        const size_t coreCallCount = lastCoreMethodCalls.size() < 10 ? lastCoreMethodCalls.size() : 10;
        std::vector<std::string> last10CoreMethodCalls(lastCoreMethodCalls.end() - coreCallCount, lastCoreMethodCalls.end());
        std::string errorMessage = "[" + CoopVersionName() + ", " + CoopBuildStamp() + "] Fehler:\n";   // v58: Fassung statt COOP_VERSION
        std::string errorLog = "";

        if (PluginState.compare("") != 0) {
            errorMessage += "State:\n";
            errorMessage += PluginState;
            errorMessage += "\n";
        }

        errorMessage += "Packages:\n";
        for (auto data : lastProcessedPackages) {
            errorMessage += data;
            errorMessage += "\n";
        }

        errorMessage += "Calls:\n";
        for (const auto& piece : lastMethodCalls) {
            errorMessage += piece;
            errorMessage += "\n";
        }

        errorMessage += "Core calls:\n";
        for (const auto& piece : last10CoreMethodCalls) {
            errorMessage += piece;
            errorMessage += "\n";
        }

        Message::Error(errorMessage.c_str());
        Ivk_zCExceptionHandlerUnhandledExceptionFilter(pointers);
    }

    int __fastcall oCNpc_DoTakeVob(oCNpc*, void*, zCVob*);
#if ENGINE >= Engine_G2
    CInvoke<int(__thiscall*)(oCNpc*, zCVob*)> Ivk_oCNpc_DoTakeVob(0x007449C0, &oCNpc_DoTakeVob);
#else
    CInvoke<int(__thiscall*)(oCNpc*, zCVob*)> Ivk_oCNpc_DoTakeVob(0x006A0D10, &oCNpc_DoTakeVob);
#endif
    int __fastcall oCNpc_DoTakeVob(oCNpc* _this, void* vtable, zCVob* vob) {
        if (Myself && _this && _this->IsAPlayer() && vob)
        {
            if (oCItem* item = zDYNAMIC_CAST<oCItem>(vob))
            {
                if (item->GetObjectName().HasWord("RX_DROPPED_ITEM_"))
                {
                    Myself->pItemTaken = zfactory->CreateItem(item->GetInstance());
                    Myself->pItemTaken->flags = item->flags;
                    Myself->pItemTaken->amount = item->amount;
                    Myself->pItemTaken->SetObjectName(item->GetObjectName());
                    Myself->pItemTakenPos = item->GetPositionWorld();
                    Myself->SyncOnTakeItem();
                }
            }
        }

        // Gegenstaende aus der Welt meldet seit v14 CoopWorldSync (RemoveVob -> Inventar), wegunabhaengig.
        return Ivk_oCNpc_DoTakeVob(_this, vob);
    }

    int __fastcall oCNpc_DoDropVob(oCNpc*, void*, zCVob*);
#if ENGINE >= Engine_G2
    CInvoke<int(__thiscall*)(oCNpc*, zCVob*)> Ivk_oCNpc_DoDropVob(0x00744DD0, &oCNpc_DoDropVob);
#else
    CInvoke<int(__thiscall*)(oCNpc*, zCVob*)> Ivk_oCNpc_DoDropVob(0x006A10F0, &oCNpc_DoDropVob);
#endif
    int __fastcall oCNpc_DoDropVob(oCNpc* _this, void* vtable, zCVob* vob) {
        if (Myself && _this && _this->IsAPlayer() && vob && (!_this->IsDead() && !_this->IsUnconscious()))
        {
            if (auto pItem = vob->CastTo<oCItem>())
            {
                int randVal = GetRandVal(0, 2e9);
                pItem->SetObjectName("RX_DROPPED_ITEM_" + Z randVal);
                Myself->pItemDropped = pItem;
                Myself->itemDropReady = true;
                Myself->SyncOnDropItem();

                return Ivk_oCNpc_DoDropVob(_this, vob);
            }
        }

        auto dropResult = Ivk_oCNpc_DoDropVob(_this, vob);

        if (_this && vob && IsCoopPlayer(_this->GetObjectName())) {
            if (auto pItem = vob->CastTo<oCItem>())
            {
                if (!pItem->GetObjectName().StartWith("RX_DROPPED_ITEM_")) {
                    pItem->RemoveVobFromWorld();
                }
            }
        }

        return dropResult;
    }
}