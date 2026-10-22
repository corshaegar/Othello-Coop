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
    static void __cdecl zCExceptionHandlerUnhandledExceptionFilter(struct _EXCEPTION_POINTERS*);
#if ENGINE >= Engine_G2
    CInvoke<void(*)(struct _EXCEPTION_POINTERS*)> Ivk_zCExceptionHandlerUnhandledExceptionFilter(0x004C88C0, &zCExceptionHandlerUnhandledExceptionFilter);
#else
    CInvoke<void(*)(struct _EXCEPTION_POINTERS*)> Ivk_zCExceptionHandlerUnhandledExceptionFilter(0x004BF560, &zCExceptionHandlerUnhandledExceptionFilter);
#endif
    void zCExceptionHandlerUnhandledExceptionFilter(struct _EXCEPTION_POINTERS* pointers) {
        TrackLastExecutedFunctions = false;

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