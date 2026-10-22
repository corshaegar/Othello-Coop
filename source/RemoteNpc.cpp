namespace GOTHIC_ENGINE {
    // v41: Abgleich ausserhalb der Reichweite (CoopFarSync.cpp)
    bool CoopFollowActive();
    bool CoopFollowNear(const zVEC3& p, float range);
    bool CoopFarFromMe(oCNpc* npc);
    bool CoopDiffXpNoShare();            // v67/v68: CoopDifficulty.cpp
    void CoopDiffPartnerXpBegin();
    void CoopDiffPartnerXpEnd();
    void CoopDiffHpFromHost(oCNpc* n, int hpMax, int base);   // v70
    // v70 (C17): beim Client spielt die Figur die Treffer-Reaktion des nachgespielten Treffers selbst (Hooks.cpp, StartAni)
    oCNpc* CoopFigureHitAniNpc = NULL;
    bool CoopSpawnInsertAt(oCNpc* npc, const zVEC3& pos);
    void CoopKillNoXp(oCNpc* npc, oCNpc* killer, const char* why);
    bool CoopFarKnownDead(const std::string& name);
    bool CoopSpawnListHas(oCNpc* npc);   // v43
    void CoopFarCountLedInsert();        // v43
    // v53: Treffer-Entscheidung beim getroffenen Client (CoopHitAuthority.cpp)
    void CoopClientSwing(oCNpc* attacker, const char* attackerName, json& a);
    void CoopFightSound(oCNpc* attacker, oCNpc* victim);
    void CoopApplyHitResult(oCNpc* puppet, json& u, const std::string& who);
    bool CoopLeadIsRemote(oCNpc* npc);   // v72/v73: CoopCombatLead.cpp - NPC fuehrt gerade ein Client im Kampf

    // v74 (Nutzer: "wenn ein NPC etwas macht und der Client ihn niederschlaegt, dann macht beim Host der NPC weiter, z. B. schleifen,
    // kochen"; Test v73: NON_1501 in ZS_COOK und NON_1502 in ZS_SMITH_SHARP nach dem K.o. beim Host mit HP 1 weiter am Mob).
    // Allgemein (G1-Engine): ein echter Treffer beendet die Benutzung eines Mobs/Gegenstands in oCNpc::OnDamage_Anim (0x007443BC:
    // ClearEM 0x006A2610, dann oCNpc::Interrupt(0) 0x00692830 - gibt interactMob/interactItem frei). Das Nachspielen eines K.o.
    // ruft nur DropUnconscious (0x00692C10), und dessen StartAIState unterbricht einen NPC am Mob absichtlich nicht (0x006C5974:
    // nur ohne interactMob/interactItem Interrupt + ClearEM) - der Bewusstlos-Zustand griff nicht, die Mob-Schleife lief weiter.
    // Jetzt vorher wie die Engine beim Treffer. Speicherabbild: Othello legt in Interrupt nur den Aufruf bei +0x64 um
    // (OTHELLO_ABI+0x80D20), Eingang unveraendert; DoDie (0x006934A0) ruft Interrupt selbst - der Tod hatte das Problem nicht.
    static void CoopStopMobUse(oCNpc* n, const char* why) {
        if (!n || (!n->GetInteractMob() && !n->interactItem))
            return;
        const std::string st = n->state.curState.name.ToChar();
        n->ClearEM();
        n->Interrupt(0);
#if ENGINE == Engine_G1
        static CoopDiagBudget budget = { 0, 0 };
        if (CoopDiagAllow(budget, 30))
            OthelloCompatLog("[%lld s, %s] Mob-Benutzung beendet: %s (%s) - %s", DiagSeconds(), DiagRole(), n->GetObjectName().ToChar(),
                st.c_str(), why);
#endif
    }

    class RemoteNpc
    {
    public:
        string name;
        string playerNickname = "";
        int playerHeadVarNr = 9;       // v55: vorbelegt (vorher ungesetzt bis INIT_NPC)
        int playerBodyTextVarNr = 4;
        std::string playerHeadMesh = "HUM_HEAD_PONY";    // v55: vom Mitspieler gewaehlt (INIT_NPC headMesh/bodyMesh/skin)
        std::string playerBodyMesh = "hum_body_Naked0";
        int playerSkin = -1;                             // -1: Partner ohne v55 - wie bisher DefaultBodyTexColorNr
        oCNpc* npc = NULL;
        bool destroyed = false;
        bool isSpawned = false;
        bool hasNpc = false;
        bool hasModel = false;
        std::vector<json> localUpdates;

        zVEC3* lastPositionFromServer = NULL;
        float lastHeadingFromServer = -1;
        int lastHpFromServer = -1;
        int lastMaxHpFromServer = -1;
        int lastWeaponMode = -1;
        int lastWeapon1 = -1;
        int lastWeapon2 = -1;
        int lastArmor = -1;
        zSTRING lastSpellInstanceName;
        oCItem* spellItem;
        long long nextFollowSpawnMs = 0;   // v41: Figur beim Host wieder einsetzen (Verfolgungspunkt)
        std::map<oCVob*, bool> syncedNpcItems;

        RemoteNpc(string playerName) {
            name = playerName;
        }

        // v52: Glaetten beim Empfaenger. Vorher setzte UpdateNpcPosition jedes Bild hart die zuletzt empfangene Lage: zwischen
        // zwei Paketen stand der NPC, waehrend seine Lauf-Animation ihn vorwaerts schob - beim naechsten Bild zurueck, beim
        // naechsten Paket ein Sprung (Ruckeln, "Gummiband"). Jetzt: Geschwindigkeit aus den letzten beiden Paketen, Ziel = letzte
        // Lage + Geschwindigkeit x Alter (hoechstens 120 ms weiter), angezeigte Lage folgt dem Ziel weich (Zeitkonstante ~65 ms).
        // Grosse Abweichungen (> 3 m, Teleport) weiter hart. Blickrichtung ebenso ueber den kuerzesten Winkel.
        // Konfiguration coopSmoothRemote (true); false = wie bisher.
        zVEC3 velocityFromServer = zVEC3(0, 0, 0);
        long long lastPosRecvMs = 0;
        long long lastSmoothMs = 0;
        float shownHeading = -1;

        static bool SmoothOn() {
            static int on = -1;
            if (on < 0) {
                on = 1;
                if (CoopConfig.contains("coopSmoothRemote") && CoopConfig["coopSmoothRemote"].is_boolean())
                    on = CoopConfig["coopSmoothRemote"].get<bool>() ? 1 : 0;
            }
            return on == 1;
        }

        // v51 (O3): Lage vom Host merken - einmal anlegen, danach ueberschreiben. Vorher legte jedes Positionspaket ein neues
        // zVEC3 an und gab das alte nie frei (Speicher wuchs mit Spielzeit und NPC-Zahl). Den Zeiger haelt niemand fest: alle
        // Leser kopieren den Wert, oCSpawnManager::InsertNpc legt die Lage als Wert im Spawn-Knoten ab (oSSpawnNode::spawnPos).
        void SetLastPositionFromServer(float x, float y, float z) {
            const zVEC3 p(x, y, z);
            const long long now = CurrentMs;
            if (lastPositionFromServer && lastPosRecvMs > 0) {   // v52: Geschwindigkeit fuer das Weiterrechnen
                const long long dt = now - lastPosRecvMs;
                if (dt > 300) {
                    velocityFromServer = zVEC3(0, 0, 0);   // lange nichts gekommen: stand still
                }
                else if (dt >= 10) {
                    // Ankunftszeiten haengen am Bildtakt (Pakete werden je Bild abgearbeitet) - darum mit der vorigen
                    // Geschwindigkeit mitteln. > 15 m/s: Sprung, nicht weiterrechnen.
                    zVEC3 v = (p - *lastPositionFromServer) * (1000.f / (float)dt);
                    if (v.LengthApprox() > 1500.f)
                        velocityFromServer = zVEC3(0, 0, 0);
                    else
                        velocityFromServer = velocityFromServer * 0.4f + v * 0.6f;
                }
                // dt < 10: mehrere Pakete im selben Bild - Geschwindigkeit behalten
            }
            if (lastPositionFromServer)
                *lastPositionFromServer = p;
            else
                lastPositionFromServer = new zVEC3(p);
            lastPosRecvMs = now;
        }

        void Update() {
            if (destroyed) {
                return;
            }

            UpdateHasNpcAndHasModel();
            //RemoveCoopItemsFromGround();
            RespawnOrDestroyBasedOnDistance();

            if (npc == NULL && UniqueNameToNpcList.count(name) > 0) {
                npc = UniqueNameToNpcList[name];
            }

            // Waehrend der Spieler mit diesem NPC spricht, keine Host-Daten anwenden. Erkennung auch
            // ueber die Dialogverwaltung, nicht nur ueber den Kameramodus (siehe CoopNpcInDialogWithPlayer).
            if (npc && (IsPlayerTalkingWithNpc(npc) || CoopNpcInDialogWithPlayer(npc))) {
                CoopDiagUpdateSkipped(zSTRING(name).ToChar(), (int)localUpdates.size());
                return;
            }

            if (IsNpcInTot()) {
                return;
            }

            // Original: for (i = 0; i < localUpdates.size(); i++) - verarbeitet nur die Haelfte,
            // weil size() mit jedem erase() sinkt. Und update["type"].get<int>() wirft
            // json::type_error 302, sobald ein Eintrag beschaedigt ist; die Ausnahme brach die
            // gesamte Aktualisierung des NPCs ab. Beschaedigte Eintraege ueberspringen wir.
            while (!localUpdates.empty())
            {
                auto update = localUpdates.front();
                localUpdates.erase(localUpdates.begin());

                if (!update.is_object() || !update.contains("type") || !update["type"].is_number()) {
                    continue;
                }

                auto type = update["type"].get<int>();
                PluginState = "Updating NPC " + name + " TYPE: " + type;

                // Der oCNpc entsteht erst in UpdateInitialization. Jeder andere Zweig
                // dereferenziert ihn ungeprueft - gemessen stuerzte UpdateOverlays so ab
                // (ApplyOverlaysArray auf this=NULL, Lesen auf 0x60). Ohne npc also
                // ueberspringen; das deckt alle Zweige des switch auf einmal ab.
                if (npc == NULL && type != INIT_NPC) {
                    CoopTraceCount(TRACE_SKIPPED, type);
                    continue;
                }
                CoopTraceCount(TRACE_APPLIED, type);

                // Ein einzelnes fehlerhaftes Update (z. B. fehlendes JSON-Feld -> json::type_error)
                // brach frueher den gesamten restlichen Game_Loop dieses Frames ab, still
                // (catch(...) in UnionHooks.cpp). Jetzt: protokollieren, dieses Update verwerfen,
                // weitermachen. Zugriffsfehler (SEH, /EHa) werden protokolliert und weitergereicht.
                try {
                switch (type) {
                case INIT_NPC:
                {
                    UpdateInitialization(update);
                    break;
                }
                case SYNC_POS:
                {
                    UpdatePosition(update);
                    break;
                }
                case SYNC_HEADING:
                {
                    UpdateAngle(update);
                    break;
                }
                case SYNC_ANIMATION:
                {
                    UpdateAnimation(update);
                    break;
                }
                case SYNC_WEAPON_MODE:
                {
                    UpdateWeaponMode(update);
                    break;
                }
                case SYNC_HP:
                {
                    UpdateHp(update);
                    break;
                }
                case SYNC_TALENTS:
                {
                    UpdateTalents(update);
                    break;
                }
                case SYNC_PROTECTIONS:
                {
                    UpdateProtection(update);
                    break;
                }
                case SYNC_ARMOR:
                {
                    UpdateArmor(update);
                    break;
                }
                case SYNC_MAGIC_SETUP:
                {
                    UpdateMagicSetup(update);
                    break;
                }
                case SYNC_HAND: {
                    UpdateHand(update);
                    break;
                }
                case SYNC_WEAPONS:
                {
                    UpdateWeapons(update);
                    break;
                }
                case SYNC_SPELL_CAST:
                {
                    UpdateSpellCasts(update);
                    break;
                }
                case SYNC_ATTACKS:
                {
                    UpdateAttacks(update);
                    break;
                }
                case SYNC_HIT_RESULT:   // v53
                {
                    UpdateHitResult(update);
                    break;
                }
                case SYNC_TIME: {
                    UpdateTime(update);
                    break;
                }
                case SYNC_REVIVED: {
                    UpdateRevived(update);
                    break;
                }
                case DESTROY_NPC:
                {
                    DestroyNpc();
                    break;
                }
                case SYNC_BODYSTATE:
                {
                    UpdateBodystate(update);
                    break;
                }
                case SYNC_OVERLAYS:
                {
                    UpdateOverlays(update);
                    break;
                }
                case SYNC_DROPITEM:
                {
                    UpdateDropItem(update);
                    break;
                }
                case SYNC_TAKEITEM:
                {
                    UpdateTakeItem(update);
                    break;
                }

                }
                }
                catch (std::exception& e) {
                    CoopErrorLog("RemoteNpc::Update", e.what(), PluginState.c_str());
                }
                catch (...) {
                    CoopErrorLog("RemoteNpc::Update", "Zugriffsfehler (SEH)", PluginState.c_str());
                    throw;
                }
            }

            PluginState = "UpdateSyncNpcs";
            UpdateNpcBasedOnLastDataFromServer();
        }

        // v55: Kopf, Statur, Hautton (fehlen bei einem Partner vor v55 - dann wie bisher)
        bool ReadLook(json& update) {
            const std::string head = update.contains("headMesh") && update["headMesh"].is_string() ? update["headMesh"].get<std::string>() : std::string("HUM_HEAD_PONY");
            const std::string body = update.contains("bodyMesh") && update["bodyMesh"].is_string() ? update["bodyMesh"].get<std::string>() : std::string("hum_body_Naked0");
            const int skin = update.contains("skin") && update["skin"].is_number_integer() ? update["skin"].get<int>() : -1;
            const int headNr = update.contains("headVarNr") && update["headVarNr"].is_number_integer() ? update["headVarNr"].get<int>() : playerHeadVarNr;
            const int bodyNr = update.contains("bodyTextVarNr") && update["bodyTextVarNr"].is_number_integer() ? update["bodyTextVarNr"].get<int>() : playerBodyTextVarNr;
            const bool changed = head != playerHeadMesh || body != playerBodyMesh || skin != playerSkin || headNr != playerHeadVarNr || bodyNr != playerBodyTextVarNr;
            playerHeadMesh = head;
            playerBodyMesh = body;
            playerSkin = skin;
            playerHeadVarNr = headNr;
            playerBodyTextVarNr = bodyNr;
            return changed;
        }

        void ApplyLook() {
            CoopLookApplyTo(npc, playerHeadMesh, playerHeadVarNr, playerBodyMesh, playerBodyTextVarNr,
                playerSkin >= 0 ? playerSkin : DefaultBodyTexColorNr);
        }

        void UpdateInitialization(json update) {
            if (npc != NULL) {
                // v55 (Nutzer: "das Aussehen, das der Mitspieler waehlt, wird dann auch dementsprechend angezeigt"): Name und
                // Aussehen auch an der schon stehenden Figur uebernehmen (neu gesendet nach dem Laden und nach Aenderungen im Menue).
                if (!IsCoopPlayer(name))
                    return;
                const std::string nick = update.contains("nickname") && update["nickname"].is_string() ? update["nickname"].get<std::string>() : std::string();
                const bool look = ReadLook(update);
                if (!nick.empty() && !(playerNickname == nick.c_str())) {
                    playerNickname = nick.c_str();
                    npc->name[0] = zSTRING(playerNickname.ToChar());
                }
                if (look) {
                    ApplyLook();
                    OthelloCompatLog("[%lld s, %s] Aussehen: Figur %s neu - Kopf %s, Gesicht %d, Statur %s, Koerper %d, Hautton %d",
                        DiagSeconds(), DiagRole(), zSTRING(name).ToChar(), playerHeadMesh.c_str(), playerHeadVarNr, playerBodyMesh.c_str(),
                        playerBodyTextVarNr, (playerSkin >= 0 ? playerSkin : DefaultBodyTexColorNr) + 1);
                }
                return;
            }
            {
                auto x = update["x"].get<float>();
                auto y = update["y"].get<float>();
                auto z = update["z"].get<float>();
                auto nickname = update["nickname"].get<std::string>();

                playerNickname = nickname.c_str();
                ReadLook(update);
                SetLastPositionFromServer(x, y, z);   // v51 (O3)

                if (IsCoopPlayer(name)) {
                    InitCoopFriendNpc();
                    UpdateHasNpcAndHasModel();
                }
                else if (npc) {
                    ogame->spawnman->InsertNpc(npc, *lastPositionFromServer);
                }
            }
        }

        void UpdatePosition(json update) {
            auto x = update["x"].get<float>();
            auto y = update["y"].get<float>();
            auto z = update["z"].get<float>();

            if (CurrentWorldTOTPosition) {
                auto newPosition = zVEC3(x, y, z);
                auto totPos = *CurrentWorldTOTPosition;
                int dist = GetDistance3D(newPosition.n[0], newPosition.n[1], newPosition.n[2], totPos.n[0], totPos.n[1], totPos.n[2]);
                if (dist < 500) {
                    destroyed = true;
                    return;
                }
            }

            SetLastPositionFromServer(x, y, z);   // v51 (O3)
        }

        void UpdateAngle(json update) {
            auto h = update["h"].get<float>();
            lastHeadingFromServer = h;
            if (hasModel && npc && !SmoothOn()) {   // v52: mit Glaetten setzt UpdateNpcBasedOnLastDataFromServer je Bild
                npc->ResetRotationsWorld();
                npc->RotateWorldY(h);
            }
        }

        // v69 (C15): liegt der NPC hier bewusstlos (Fall/Liegen spielt die eigene Engine, Hooks.cpp), Fall- und Liegeanimationen
        // des Hosts nicht noch einmal starten - er stand sonst kurz auf und fiel erneut. Aufstehen (..._2_STAND) kommt weiter.
        static bool CoopSkipHostLyingAni(oCNpc* n, const char* an) {
            return n && an && !IsCoopPlayer(n->GetObjectName()) && n->IsUnconscious() && strstr(an, "WOUNDED") && !strstr(an, "_2_STAND");
        }

        void UpdateAnimation(json update) {
            if (hasModel && npc) {
                zCModel* model = npc->GetModel();
                // v53: alle seit dem letzten Paket gestarteten Animationen in Reihenfolge, per Name - so passt auch ein anderes
                // Overlay (gleicher Name, eigene Nummer im eigenen Modell). Vorher nur eine Nummer je Paket.
                if (update.contains("al") && update["al"].is_array() && !update["al"].empty()) {
                    for (auto& n : update["al"]) {
                        if (!n.is_string())
                            continue;
                        const std::string an = n.get<std::string>();
                        if (CoopSkipHostLyingAni(npc, an.c_str()))
                            continue;
                        // v70 (C17): Treffer-Reaktion der Figur, die hier schon laeuft (nachgespielter Treffer), nicht neu starten
                        if (IsCoopPlayer(name) && (an.find("STUMBLE") != std::string::npos || an.find("GOTHIT") != std::string::npos) &&
                            model->IsAnimationActive(zSTRING(an.c_str())))
                            continue;
                        const int id = model->GetAniIDFromAniName(zSTRING(an.c_str()));
                        if (id >= 0)
                            model->StartAni(id, COOP_MAGIC_NUMBER);
                        else
                            CoopDiagAniUnknown(zSTRING(name).ToChar(), an.c_str());
                    }
                    return;
                }
                auto a = update["a"].get<int>();
                if (a >= 0) {
                    zCModelAni* ani = model->GetAniFromAniID(a);
                    if (ani && CoopSkipHostLyingAni(npc, ani->aniName.ToChar()))
                        return;
                    model->StartAni(a, COOP_MAGIC_NUMBER);
                }
            }
        }

        void UpdateWeaponMode(json update) {
            if (hasModel && npc) {
                auto wm = update["wm"].get<int>();
                lastWeaponMode = wm;
                npc->SetWeaponMode2(wm);

                // v68 (C14): bewusstlose/tote NPCs nicht zuruecksetzen. Test v67: "die Animation des einen Gardisten, der
                // angegriffen wurde, war nicht komplett richtig, er hatte sich irgendwie rotiert" - Log Client: GRD_214 nach dem
                // K.o. (HP 1, beim Host ZS_UNCONSCIOUS) hier in ZS_ASSESSENEMY, 191 blockierte Animationen in 4 s. Der Host steckt
                // beim K.o. die Waffe weg (Waffenmodus 0) - dieses Paket loeschte hier den Bewusstlos-Zustand, die eigene KI
                // wollte aufstehen und sich drehen, GT zeigte die Liegeanimation des Hosts.
                if (!IsCoopPlayer(name) && !npc->IsUnconscious() && !npc->IsDead()) {
                    npc->GetEM()->KillMessages();
                    npc->ClearEM();
                    npc->state.ClearAIState();
                }
                else {
                    // if coop friend is changing weapon mode while the game thinks IsUnconscious -> restart AI state
                    if (npc->IsUnconscious()) {
                        npc->GetEM()->KillMessages();
                        npc->ClearEM();
                        npc->state.ClearAIState();
                    }
                }
            }
        }

        void UpdateHp(json update) {
            auto hp = update["hp"].get<int>();
            auto hpMax = update["hp_max"].get<int>();

            if (!IsCoopPlayer(name) && hp == 0) {
                if (hasNpc && npc) {
                    if (!npc->IsDead() && CoopFarKnownDead(std::string(name.ToChar()))) {
                        // v41: der Host hat diesen Tod gemeldet (z. B. ausserhalb der Reichweite) - nicht mit HP 1 stehen lassen
                        CoopKillNoXp(npc, NULL, "HP 0 vom Host, als tot gemeldet");
                    } else if (!npc->IsDead() && KilledByPlayerNpcNames.count(name) == 0) {
                        npc->SetAttribute(NPC_ATR_HITPOINTS, 1);
                    } else {
                        npc->SetAttribute(NPC_ATR_HITPOINTS, 0);
                    }
                }
                lastHpFromServer = -1;
                return;
            }

            lastHpFromServer = hp;
            lastMaxHpFromServer = hpMax;
            if (hasNpc && npc) {
                npc->SetAttribute(NPC_ATR_HITPOINTS, hp);
                npc->SetAttribute(NPC_ATR_HITPOINTSMAX, hpMax);
                // v70: vom Host skalierte Lebenspunkte ("Leben der Gegner") - Grundwert merken (vor dem Speichern zurueck)
                if (!IsCoopPlayer(name))
                    CoopDiffHpFromHost(npc, hpMax, update.contains("hb") && update["hb"].is_number_integer() ? update["hb"].get<int>() : -1);
            }
        }

        void UpdateTalents(json update) {
            auto t0 = update["t0"].get<int>();
            auto t1 = update["t1"].get<int>();
            auto t2 = update["t2"].get<int>();
            auto t3 = update["t3"].get<int>();

            if (hasNpc && npc) {
                npc->SetTalentSkill(oCNpcTalent::NPC_TAL_1H, t0);
                npc->SetTalentSkill(oCNpcTalent::NPC_TAL_2H, t1);
                npc->SetTalentSkill(oCNpcTalent::NPC_TAL_BOW, t2);
                npc->SetTalentSkill(oCNpcTalent::NPC_TAL_CROSSBOW, t3);
            }
        }

        void UpdateProtection(json update) {
            auto p0 = update["p0"].get<int>();
            auto p1 = update["p1"].get<int>();
            auto p2 = update["p2"].get<int>();
            auto p3 = update["p3"].get<int>();
            auto p4 = update["p4"].get<int>();
            auto p5 = update["p5"].get<int>();
            auto p6 = update["p6"].get<int>();
            auto p7 = update["p7"].get<int>();

            if (hasNpc && npc) {
                npc->SetProtectionByIndex(static_cast<oEIndexDamage>(0), p0);
                npc->SetProtectionByIndex(static_cast<oEIndexDamage>(1), p1);
                npc->SetProtectionByIndex(static_cast<oEIndexDamage>(2), p2);
                npc->SetProtectionByIndex(static_cast<oEIndexDamage>(3), p3);
                npc->SetProtectionByIndex(static_cast<oEIndexDamage>(4), p4);
                npc->SetProtectionByIndex(static_cast<oEIndexDamage>(5), p5);
                npc->SetProtectionByIndex(static_cast<oEIndexDamage>(6), p6);
                npc->SetProtectionByIndex(static_cast<oEIndexDamage>(7), p7);
            }
        }

        void UpdateArmor(json update) {
            if (hasNpc && npc) {
                auto armor = update["armor"].get<std::string>();

                auto currentArmor = npc->GetEquippedArmor();
                if (currentArmor) {
                    lastArmor = -1;
                    npc->UnequipItem(currentArmor);
                }

                if (armor != "NULL") {
                    int insIndex = parser->GetIndex(armor.c_str());
                    if (insIndex > 0) {
                        auto newArmor = CreateCoopItem(insIndex);
                        if (newArmor) {
                            lastArmor = insIndex;
                            npc->Equip(newArmor);
                        }
                    }

                }
            }
        }

        void UpdateBodystate(json update) {
            if (hasNpc && npc) {
                auto bs = update["bs"].get<int>();
                npc->SetBodyState(bs);
            }
        }

        void UpdateOverlays(json update) {
            // War unter Gothic 1 zeitweise abgeschaltet ("Lesen auf 0x60" in ApplyOverlaysArray).
            // Das war zCModel::modelProtoList (+0x58) eines NULL-Modells: der Coop-NPC hatte damals
            // wegen Othellos MakeSpellBook-Slot kein Modell (OthelloCompat.cpp). Die Engine-
            // Funktionen dieses Pfads sind unter Othello unveraendert (GetModel 0x695300,
            // ApplyOverlay 0x68AD40, RemoveOverlay 0x68B040). Jetzt mit Modellpruefung aktiv.
            if (hasNpc && hasModel && npc && npc->GetModel()) {
                if (!update.contains("overlays") || !update["overlays"].is_array())
                    return;
                zCArray<int> overlaysNew;

                for (auto& a : update["overlays"]) {
                    // Der Name hat Vorrang: die feste Nummernliste kennt nur Vanilla-Overlays,
                    // Nummern ab 100 vergibt jede Seite selbst (GetMdsIndex).
                    int overlayId = -1;
                    if (a.contains("name") && a["name"].is_string()) {
                        overlayId = GetMdsIndex(zSTRING(a["name"].get<std::string>().c_str()));
                    }
                    else if (a.contains("over") && a["over"].is_number()) {
                        overlayId = a["over"].get<int>();
                    }
                    if (overlayId >= 0)
                        overlaysNew.InsertEnd(overlayId);
                }

                if (!npc->CompareOverlaysArray(overlaysNew))
                {
                    npc->ApplyOverlaysArray(overlaysNew);
                }
            }
        }

        void UpdateMagicSetup(json update) {
            if (hasNpc && hasModel && npc) {
                auto spellInstanceName = update["spell"].get<std::string>();
                oCMag_Book* book = npc->GetSpellBook();
                if (book)
                {
                    auto selectedSpell = book->GetSelectedSpell();
                    if (selectedSpell) {
                        auto selectedSpellItem = book->GetSpellItem(selectedSpell);
                        if (selectedSpellItem) {   // v97 (A14): kann NULL sein (Upstream: Gothic-Workshop 0a5bb2f)
                            npc->DoDropVob(selectedSpellItem);
                            selectedSpellItem->RemoveVobFromWorld();
                        }
                        selectedSpell->Kill();
                    }

                    book->spellitems.EmptyList();
                    book->spells.EmptyList();
                }

                if (spellInstanceName.compare("NULL") != 0) {
                    int insIndex = parser->GetIndex(spellInstanceName.c_str());
                    if (insIndex > 0) {
                        auto spellItem = CreateCoopItem(insIndex);
                        if (spellItem) {
                            npc->DoPutInInventory(spellItem);
                            npc->Equip(spellItem);

                            oCMag_Book* book = npc->GetSpellBook();
                            if (book) {
                                book->Open(0);
                            }
                        }
                    }
                }
            }
        }

        void UpdateHand(json update) {
            if (!hasModel) {
                return;
            }
            auto leftItem = update["left"].get<std::string>();
            auto rightItem = update["right"].get<std::string>();

            auto leftHandItem = npc->GetLeftHand();
            if (leftHandItem)
            {
                npc->DoDropVob(leftHandItem);
                leftHandItem->RemoveVobFromWorld();
                syncedNpcItems.erase(leftHandItem);
            }

            if (leftItem != "NULL") {
                int insIndex = parser->GetIndex(leftItem.c_str());
                if (insIndex > 0) {
                    auto newItem = CreateCoopItem(insIndex);
                    if (newItem) {
                        syncedNpcItems[newItem] = true;
                        npc->SetLeftHand(newItem);
                    }
                }
            }

            auto rightHandItem = npc->GetRightHand();
            if (rightHandItem)
            {
                npc->DoDropVob(rightHandItem);
                rightHandItem->RemoveVobFromWorld();
                syncedNpcItems.erase(rightHandItem);
            }

            if (rightItem != "NULL") {
                int insIndex = parser->GetIndex(rightItem.c_str());
                if (insIndex > 0) {
                    auto newItem = CreateCoopItem(insIndex);
                    if (newItem) {
                        syncedNpcItems[newItem] = true;
                        npc->SetRightHand(newItem);
                    }
                }
            }
        }

        void UpdateWeapons(json update) {
            if (hasModel && npc) {
                auto weapon1 = update["w1"].get<std::string>();
                auto weapon2 = update["w2"].get<std::string>();

                auto currentWeapon1 = npc->GetEquippedMeleeWeapon();
                auto currentWeapon2 = npc->GetEquippedRangedWeapon();

                if (currentWeapon1) {
                    npc->UnequipItem(currentWeapon1);
                    lastWeapon1 = -1;
                }

                if (currentWeapon2) {
                    npc->UnequipItem(currentWeapon2);
                    lastWeapon2 = -1;
                }

                if (weapon1 != "NULL") {
                    int insIndex = parser->GetIndex(weapon1.c_str());
                    if (insIndex > 0) {
                        auto newWeapon = CreateCoopItem(insIndex);
                        if (newWeapon) {
                            lastWeapon1 = insIndex;
                            npc->Equip(newWeapon);
                            syncedNpcItems[newWeapon] = true;
                        }
                    }

                }

                if (weapon2 != "NULL") {
                    int insIndex = parser->GetIndex(weapon2.c_str());
                    if (insIndex > 0) {
                        auto newWeapon = CreateCoopItem(insIndex);
                        if (newWeapon) {
                            lastWeapon2 = insIndex;
                            npc->Equip(newWeapon);
                            syncedNpcItems[newWeapon] = true;
                        }
                    }

                }
            }
        }

        void UpdateSpellCasts(json update) {
            if (!hasModel) {
                return;
            }

            oCMag_Book* book = npc->GetSpellBook();
            if (book)
            {
                auto selectedSpell = book->GetSelectedSpell();
                if (selectedSpell) {
                    std::vector<json> casts = update["casts"];
                    for each (auto c in casts) {
                        auto target = c["target"].get<std::string>();

                        if (!target.empty() && UniqueNameToNpcList.count(target.c_str()) > 0) {
                            book->Spell_Setup(0, npc, UniqueNameToNpcList[target.c_str()]);
                        }
                        else {
                            zCVob* nullVob = NULL;
                            book->Spell_Setup(0, npc, nullVob);
                        }

                        book->Spell_Invest();
                        book->Spell_Cast();
                    }
                }
            }
        }

        void UpdateAttacks(json update) {
            std::vector<json> atts = update["att"];
            if (!hasModel) {
                // v53: Schlag eines hier nicht sichtbaren NPCs auf den eigenen Helden - zaehlt nicht (der Client entscheidet)
                for each (auto a in atts) {
                    if (a.contains("swing") && a.contains("target") && a["target"].is_string() &&
                        a["target"].get<std::string>().compare(MyselfId) == 0)
                        CoopClientSwing(NULL, zSTRING(name).ToChar(), a);
                }
                return;
            }

            for each (auto a in atts) {
                auto target = a["target"].get<std::string>();

                // v53: Schlag eines NPCs auf den Helden eines Spielers (Host meldet nur, CoopHitAuthority.cpp). Entscheiden und rechnen
                // tut der getroffene Spieler; Schlaege auf andere Clients gehen hier nichts an (deren Ergebnis kommt als
                // SYNC_HIT_RESULT).
                if (a.contains("swing")) {
                    if (target.compare(MyselfId) == 0)
                        CoopClientSwing(npc, zSTRING(name).ToChar(), a);
                    continue;
                }
                auto damage = a["damage"].get<float>();
                auto isUnconscious = a["isUnconscious"].get<int>();
                auto stillAlive = !a["isDead"].get<bool>();
                auto damageMode = a["damageMode"].get<unsigned long>();

                if (IsCoopPlayer(target) || target.compare(MyselfId) == 0) {
                    CoopDamageIncoming(zSTRING(name).ToChar(), target.c_str(), zSTRING(MyselfId).ToChar(),
                        damage, isUnconscious, stillAlive, CoopBaseLayerAni(player));
                }

                // attack player (client only, eg. wolf attacks player)
                if (target.compare(MyselfId) == 0) {
                    auto targetNpc = player;
                    int health = targetNpc->GetAttribute(NPC_ATR_HITPOINTS);

                    // Othello ersetzt CanParade durch "immer 1" - siehe CoopCanParade (OthelloCompat.cpp).
                    if (CoopCanParade(player->GetAnictrl(), npc)) {
                        CoopDamageReceived(zSTRING(name).ToChar(), "eigener Held (pariert)", 0, health, health);
                        break;
                    }

                    if (isUnconscious && stillAlive) {
                        targetNpc->SetWeaponMode2(NPC_WEAPON_NONE);
                        targetNpc->DropUnconscious(1, npc);
                    }

                    if (stillAlive) {
                        // v40 (C8): HP-Maximum mit hoch, Abzug = Schaden aus dem Paket (CoopDamage.cpp) - sonst zeigte zUtilities
                        // 999999 - Max-HP an
                        CoopReplayHit(targetNpc, npc, damage, COOP_MAGIC_NUMBER, damageMode);
                        targetNpc->SetAttribute(NPC_ATR_HITPOINTS, health - damage);
                    }
                    else if (!targetNpc->IsDead()) {
                        // v39: sofort sterben wie bei der Figur des anderen Spielers (Zweig darunter: DoDie). Vorher HP 1 +
                        // OnDamage ueber die Ereignis-Warteschlange des Helden - die kam waehrend eines Laufs verzoegert an
                        // (Test v38: "Schaden 1, HP 1 -> 1" viermal, erst der fuenfte Treffer toetete), und beim Host starb die
                        // Figur bei jedem Treffer neu (Todeston). DoDie ist unter Othello umgebogen (OTHELLO_ABI+0x7C3F0, prueft
                        // BOSSBEHAVIOR_CANNOTDIE), GT ruft es fuer Figuren schon so auf.
                        targetNpc->SetAttribute(NPC_ATR_HITPOINTS, 0);
                        targetNpc->DoDie(npc);
                    }
                    CoopDamageReceived(zSTRING(name).ToChar(), "eigener Held", damage, health,
                        targetNpc->GetAttribute(NPC_ATR_HITPOINTS));

                    break;
                }

                // attack other coop player (client only, eg. wolf attacks host)
                if (IsCoopPlayer(target) && PlayerNameToNpc.count(target.c_str())) {
                    auto targetNpc = PlayerNameToNpc[target.c_str()];
                    int health = targetNpc->GetAttribute(NPC_ATR_HITPOINTS);

                    if (isUnconscious && stillAlive) {
                        targetNpc->SetWeaponMode2(NPC_WEAPON_NONE);
                        targetNpc->DropUnconscious(1, npc);
                    }

                    if (stillAlive) {
                        // v70 (C17): Test v69: "die Animationen des Mitspielers sind noch nicht perfekt, z. B. wenn dieser angegriffen
                        // wird, sieht man nicht die Animation davon". Gothics OnDamage_Anim unterbricht hier die laufende Animation der
                        // Figur und startet die Treffer-Reaktion - die blockierte GT (eigene Animation einer Figur beim Client), so
                        // brach der Treffer die vom Host geschickte Reaktion ab, statt sie zu zeigen. Jetzt spielt die Figur sie selbst
                        // (sofort, von der Engine passend gewaehlt); dieselbe Reaktion vom Host startet UpdateAnimation nicht noch einmal.
                        targetNpc->SetAttribute(NPC_ATR_HITPOINTS, 999999);
                        CoopFigureHitAniNpc = targetNpc;
                        targetNpc->GetEM(false)->OnDamage(targetNpc, npc, 1, damageMode, targetNpc->GetPositionWorld());
                        CoopFigureHitAniNpc = NULL;
                        targetNpc->SetAttribute(NPC_ATR_HITPOINTS, health);
                    }
                    else {
                        targetNpc->SetAttribute(NPC_ATR_HITPOINTS, 0);
                        targetNpc->DoDie(npc);
                    }

                    break;
                }

                // attack any world npc (eg. client attacks Moe, Cavalorn attacks goblin, wolf attacks sheep)
                auto targetNpc = UniqueNameToNpcList[target.c_str()];
                if (targetNpc) {
                    int health = targetNpc->GetAttribute(NPC_ATR_HITPOINTS);
                    auto isTalkingWith = IsPlayerTalkingWithNpc(targetNpc);
                    static int AIV_PARTYMEMBER = GetPartyMemberID();
                    const bool partyAttacker = IsCoopPlayer(npc->GetObjectName()) || npc->aiscriptvars[AIV_PARTYMEMBER] == True;
                    // v41: Ziel gerade in der Welt? (ausserhalb aller Umkreise nimmt der Spawn-Manager NPCs heraus)
                    const bool targetActive = targetNpc->GetHomeWorld() && targetNpc->vobLeafList.GetNum() > 0;
                    // v41 (Nutzer: "fuer die Kills keine EP"): Angreifer ausserhalb der eigenen Reichweite - ohne EP fuer den eigenen Helden.
                    // v67: Reichweite aus dem Schwierigkeitsgrad (CoopFarFromMe); v68: EP-Anteil Partner 0 % = Kills des Partners nie
                    // mit EP, sonst gilt beim Nachspielen allgemein x Partner-Anteil (CoopDiffPartnerXpBegin)
                    const bool noShare = partyAttacker && CoopDiffXpNoShare();
                    const bool farAttacker = partyAttacker && (noShare || CoopFarFromMe(npc) || !targetActive);
                    // v73 (Test v72: "ein paar Gardisten sind direkt gestorben"): fuehrt ein Client den NPC (Kampf-Uebergabe), hat er
                    // den Treffer schon selbst abgezogen und die HP geschickt - oft vor diesem Treffer-Paket. Beim Host zog das
                    // Nachspielen den Schaden ein zweites Mal ab (Log: "HP 52 -> 0" bei 274 Schaden, beim Client HP 52, nicht tot;
                    // Brueckenwache bewusstlos mit HP 1 -> 0). Der Host meldete den Tod, der Client toetete den NPC dann ohne EP.
                    // Jetzt: HP nicht anfassen; Bewusstlos und Tod (EP, Zeugen, ZS_Dead) spielt der Host weiter nach.
                    const bool clientLed = ServerThread && CoopLeadIsRemote(targetNpc);

                    if (isUnconscious && stillAlive && targetActive) {
                        targetNpc->SetWeaponMode2(NPC_WEAPON_NONE);
                        CoopStopMobUse(targetNpc, "vor dem nachgespielten K.o.");   // v74: sonst bleibt er am Mob (kochen, schleifen)

                        if (partyAttacker && !farAttacker) {
                            // v49: Taeter = Figur (vorher der eigene Held). Othellos ZS_Unconscious gibt die EP auch bei
                            // "Partymember besiegt" (other.aivar[AIV_PARTYMEMBER] && C_NpcIsHuman(other)) - die Figur hat den
                            // Merker. Mit dem eigenen Helden als Taeter merkten sich Opfer und Zeugen ihn (NEWS_DEFEAT) und
                            // gingen auf ihn los statt auf den Mitspieler.
                            CoopDiffPartnerXpBegin();   // v68: B_UnconciousXP mit EP-Anteil Partner
                            targetNpc->DropUnconscious(1, npc);
                            CoopDiffPartnerXpEnd();
                        }
                        else if (farAttacker) {
                            // Taeter = Figur, aber ohne Partei-Merker - sonst gaebe Othellos Skript die EP dem eigenen Helden
                            const int pm = npc->aiscriptvars[AIV_PARTYMEMBER];
                            npc->aiscriptvars[AIV_PARTYMEMBER] = 0;
                            targetNpc->DropUnconscious(1, npc);
                            npc->aiscriptvars[AIV_PARTYMEMBER] = pm;
                        }
                        else {
                            targetNpc->DropUnconscious(1, npc);
                        }
                    }

                    if (stillAlive) {
                        const int attitudeBefore = (npc && IsCoopPlayer(name)) ? targetNpc->GetAttitude(npc) : -1;
                        if (!isTalkingWith && !isUnconscious && targetActive && !clientLed) {
                            // v40 (C8): die gruene ~999999 kam von zUtilities (HP vorher - nachher, Abzug auf das Maximum gekappt) -
                            // jetzt HP-Maximum mit hoch und Abzug = Schaden aus dem Paket (CoopDamage.cpp)
                            CoopReplayHit(targetNpc, npc, damage, COOP_MAGIC_NUMBER, damageMode);
                            // v36: beim Client sofort zurueck. Vorher blieben die HP bis zum naechsten HP-Paket des Hosts bei ~999999.
                            // Der Host schickt danach den echten Wert.
                            if (!ServerThread)
                                targetNpc->SetAttribute(NPC_ATR_HITPOINTS, health - damage > 1 ? (int)(health - damage) : 1);
                        }
                        if (ServerThread) {
                            if (!clientLed)   // v73: sonst hat der fuehrende Client die HP schon geschickt
                                targetNpc->SetAttribute(NPC_ATR_HITPOINTS, health - damage > 0 ? health - damage : 0);
                            if (IsCoopPlayer(name)) {
                                CoopDamageReceived(zSTRING(name).ToChar(), target.c_str(), damage, health,
                                    targetNpc->GetAttribute(NPC_ATR_HITPOINTS));
                                CoopDiagAttack(target, targetNpc, npc, attitudeBefore);
                            }
                        }
                    }
                    else if (targetNpc->IsDead()) {
                        // v41: schon tot (z. B. Todesmeldung des Hosts kam zuerst) - nicht noch einmal toeten
                    }
                    else if (farAttacker) {
                        // v41: Kill ausserhalb der Reichweite - der NPC stirbt auch hier, aber ohne EP fuer den eigenen Helden
                        CoopKillNoXp(targetNpc, npc, noShare ? "Kill des Mitspielers - Kampf-EP nur fuer den, der toetet" :
                            "Kill des Mitspielers ausserhalb der Reichweite");
                        if (SyncNpcs.count(target.c_str()) > 0 && SyncNpcs[target.c_str()])
                            SyncNpcs[target.c_str()]->lastHpFromServer = -1;
                    }
                    else {
                        // v40 (C8): toedlicher Treffer - HP = Schaden, Abzug = Schaden: zUtilities zeigt den echten Schaden (vorher HP 1)
                        const float killDamage = damage >= 1 ? damage : 1;
                        targetNpc->SetAttribute(NPC_ATR_HITPOINTS, (int)(killDamage + 0.5f));

                        if (partyAttacker) {
                            // v41: beim Client nachgespielter Tod - Spawns/Trigger aus ZS_Dead nicht noch einmal an den Host (D13)
                            const bool replay = ClientThread && !ServerThread;
                            if (replay)
                                CoopReplayDeathDepth++;
                            CoopReplayBegin(targetNpc, killDamage);
                            // v49: Taeter = Figur (vorher der eigene Held). ZS_Dead gibt die EP auch fuer menschliche Partymitglieder
                            // (other.aivar[AIV_PARTYMEMBER] && C_NpcIsHuman(other) -> B_DeathXP an hero); Zeugen (ASSESSMURDER,
                            // ASSESSOTHERSDAMAGE) sehen so den Mitspieler als Taeter statt des eigenen Helden.
                            CoopDiffPartnerXpBegin();   // v68: B_DeathXP mit EP-Anteil Partner
                            targetNpc->OnDamage(targetNpc, npc, COOP_MAGIC_NUMBER, damageMode, targetNpc->GetPositionWorld());
                            CoopDiffPartnerXpEnd();
                            CoopReplayEnd();
                            if (replay)
                                CoopReplayDeathDepth--;
                        }

                        if (SyncNpcs.count(target.c_str()) > 0) {
                            auto syncedKilledNpc = SyncNpcs[target.c_str()];
                            syncedKilledNpc->lastHpFromServer = -1;
                        }

                        targetNpc->SetAttribute(NPC_ATR_HITPOINTS, 0);
                    }

                    // v53: Treffer des Clients beim Host als Kampfgeraeusch an die Umgebung (CoopHitAuthority.cpp) - die Engine
                    // erzeugt es nur in CreateHit des Angreifers, das Nachspielen ueber OnDamage hatte keins.
                    if (ServerThread && IsCoopPlayer(name) && targetActive)
                        CoopFightSound(npc, targetNpc);
                }
            }
        }

        // v53: Ergebnis eines NPC-Schlags auf den Helden dieses Clients - die Figur spiegelt es (CoopHitAuthority.cpp)
        void UpdateHitResult(json update) {
            if (npc && IsCoopPlayer(name))
                CoopApplyHitResult(npc, update, std::string(zSTRING(name).ToChar()));
        }

        void UpdateTime(json update) {
            if (!IsPlayerTalkingWithAnybody()) {
                auto h = update["h"].get<int>();
                auto m = update["m"].get<int>();
                ogame->GetWorldTimer()->SetTime(h, m);
            }
        }

        void UpdateRevived(json update) {
            auto name = update["name"].get<std::string>();

            if (player->IsDead() && name.compare(MyselfId) == 0) {
                player->StopFaceAni("T_HURT");
                player->SetWeaponMode2(NPC_WEAPON_NONE);
                CoopResetPosSafe(player, player->GetPositionWorld());   // v23: ResetPos-Platz unter Othello umgewidmet (OthelloCompat.cpp)
                player->SetAttribute(NPC_ATR_HITPOINTS, 1);
                parser->CallFuncByName("RX_Mult_ReviveHero");
            }
        }

        void UpdateDropItem(json update) {
            if (!hasModel) {
                return;
            }

            auto itemName = update["itemDropped"].get<std::string>();
            auto count = update["count"].get<int>();
            auto flags = update["flags"].get<int>();
            auto itemUniqName = update["itemUniqName"].get<std::string>();

            int index = parser->GetIndex(itemName.c_str());

            if (index != -1)
            {
                oCItem* item = CreateCoopItem(index);
                
                if (item)
                {
                    item->amount = count;
                    item->SetObjectName(itemUniqName.c_str());

                    npc->DoPutInInventory(item);
                    npc->DoDropVob(item);
                }
            }

        }

        void UpdateTakeItem(json update) {
            if (!hasModel) {
                return;
            }

            auto itemName = update["itemDropped"].get<std::string>();
            auto count = update["count"].get<int>();
            auto flags = update["flags"].get<int>();
            auto x = update["x"].get<float>();
            auto y = update["y"].get<float>();
            auto z = update["z"].get<float>();
            auto uniqName = update["uniqName"].get<std::string>();
            auto itemPos = zVEC3(x, y, z);

            auto pList = CollectVobsInRadius(itemPos, 2500);
            int index = parser->GetIndex(itemName.c_str());

            if (index == -1) {
                return;
            }

            for (int i = 0; i < pList.GetNumInList(); i++)
            {
                if (auto pVob = pList.GetSafe(i))
                {
                    if (auto pItem = pVob->CastTo<oCItem>())
                    {
                        if (pItem->GetInstance() == index && pItem->GetObjectName() == uniqName.c_str())
                        {
                            pItem->RemoveVobFromWorld();
                            break;
                        }
                    }
                }
            }
        }

        void DestroyNpc() {
            if (npc != NULL) {
                PlayerNpcs.erase(npc);
                PlayerNameToNpc.erase(name);

                CoopDeleteNpc(ogame->spawnman, npc);
                destroyed = true;
                npc = NULL;
            }
        }

        // v52: Blickrichtung weich nachfuehren (kuerzester Winkel, Zeitkonstante ~50 ms); > 150 Grad sofort.
        float SmoothedHeading(float dt) {
            const float target = lastHeadingFromServer;
            if (shownHeading < 0.f || dt <= 0.f) {
                if (shownHeading < 0.f)
                    shownHeading = target;
                return shownHeading;
            }
            const float d = fmodf(target - shownHeading + 540.f, 360.f) - 180.f;
            if (fabsf(d) > 150.f)
                shownHeading = target;
            else
                shownHeading += d * (1.f - expf(-dt * 20.f));
            if (shownHeading < 0.f)
                shownHeading += 360.f;
            if (shownHeading >= 360.f)
                shownHeading -= 360.f;
            return shownHeading;
        }

        void UpdateNpcBasedOnLastDataFromServer() {
            if (npc && hasModel) {
                // v52: Zeit seit dem letzten Bild dieses NPCs (fuer das Glaetten), hoechstens 0,1 s
                const long long now = CurrentMs;
                float dt = lastSmoothMs ? (float)(now - lastSmoothMs) / 1000.f : 0.f;
                if (dt > 0.1f)
                    dt = 0.1f;
                lastSmoothMs = now;

                if (lastPositionFromServer) {
                    UpdateNpcPosition(dt);
                }

                if (lastHpFromServer != -1 && lastHpFromServer != npc->GetAttribute(NPC_ATR_HITPOINTS)) {
                    if (!npc->IsDead() || IsCoopPlayer(name)) {
                        npc->SetAttribute(NPC_ATR_HITPOINTS, lastHpFromServer);
                    }
                }

                if (lastWeaponMode != -1 && lastWeaponMode != npc->GetWeaponMode()) {
                    npc->SetWeaponMode2(lastWeaponMode);
                }

                if (lastHeadingFromServer != -1) {
                    npc->ResetRotationsWorld();
                    npc->RotateWorldY(SmoothOn() ? SmoothedHeading(dt) : lastHeadingFromServer);
                }

                if (IsCoopPlayer(name)) {
                    static int AIV_PARTYMEMBER = GetPartyMemberID();
                    npc->aiscriptvars[AIV_PARTYMEMBER] = True;
                }
            }
        }

        void UpdateNpcPosition(float dt) {
            auto currentPosition = npc->GetPositionWorld();
            auto pos = *lastPositionFromServer;
            if (SmoothOn()) {
                // v52: Ziel = letzte Lage + Geschwindigkeit x Alter. Pakete kommen beim Laufen alle 33-50 ms; ist laenger als
                // 0,15 s keins gekommen, steht der NPC (der Sender schickt jede Bewegung ueber 5 cm) - dann nicht weiterrechnen.
                const float age = (float)(CurrentMs - lastPosRecvMs) / 1000.f;
                zVEC3 target = pos;
                if (age >= 0.f && age <= 0.15f)
                    target = pos + velocityFromServer * (age > 0.1f ? 0.1f : age);
                const float err = (target - currentPosition).LengthApprox();
                if (dt > 0.f && err < 300.f) {
                    pos = currentPosition + (target - currentPosition) * (1.f - expf(-dt * 15.f));
                    CoopSmoothSteps++;
                }
                else {
                    pos = target;
                    if (err >= 300.f) {
                        CoopSmoothSnaps++;
                        CoopDiagSnap(zSTRING(name).ToChar(), err, target, currentPosition);   // v53: welcher NPC?
                    }
                }
            }
            auto dist = (int)(pos - currentPosition).LengthApprox();

            if (dist < 200) {
                npc->SetCollDet(FALSE);
                npc->SetPositionWorld(pos);
                npc->SetCollDet(TRUE);

                return;
            }

            bool inMove = npc->isInMovementMode;
            if (inMove) {
#if ENGINE >= Engine_G2
                npc->EndMovement(false);
#else
                npc->EndMovement();
#endif
            }

            npc->SetCollDet(FALSE);
            npc->trafoObjToWorld.SetTranslation(pos);
            npc->SetCollDet(TRUE);

            if (inMove) {
                npc->BeginMovement();
            }
        }

        void UpdateHasNpcAndHasModel() {
            hasNpc = npc != NULL;
            // GetModel() prueft dasselbe Feld wie InitModel und wuerde an einem
            // unbrauchbaren visual ebenso abstuerzen.
            SanitizeStaleVisual();
            hasModel = npc && npc->GetModel() && npc->vobLeafList.GetNum() > 0;
        }

        void ReinitCoopFriendNpc() {
            if (npc) {
                PlayerNpcs.erase(npc);
                PlayerNameToNpc.erase(name);
                CoopDeleteNpc(ogame->spawnman, npc);
                npc = NULL;
                InitCoopFriendNpc();
            }
        }

        // oCNpc::InitModel (G1: 0x00695020) liest this->visual bedingungslos, sobald es
        // ungleich NULL ist, und ruft dessen erste virtuelle Methode auf:
        //     mov edi,[esi+0xB8] / cmp edi,ebx / je <neu anlegen> / mov eax,[edi] / call [eax]
        // NULL ist also der sichere Wert - dann baut InitModel das Modell frisch auf.
        // Unter stark gepatchten Mods kann ein frisch erzeugter NPC hier aber Muell tragen.
        // Gemessen (Othello 4.4.2.1, 16.10.2026) direkt nach InsertNpc: visual = 0xA6180FE7,
        // nicht zeigerausgerichtet, waehrend die oCNpc-Vtable intakt war - der Wert stand
        // also von Anfang an im Feld und wurde von niemandem geschrieben.
        void SanitizeStaleVisual() {
            if (!npc)
                return;
            if (npc->visual && !LooksLikeLiveVisual(npc->visual)) {
                // Bewusst direkt und nicht ueber SetVisual(NULL): der Zeiger ist bereits
                // unbrauchbar, ein Release darauf wuerde genau den Absturz ausloesen, den
                // wir verhindern wollen. Es geht dabei nichts verloren.
#if ENGINE == Engine_G1
                // Seit OthelloCompat.cpp sollte das nicht mehr vorkommen - die bekannte
                // Quelle des Muells war MakeSpellBook. Tritt es doch auf, wollen wir es wissen.
                OthelloCompatLog("SanitizeStaleVisual: npc=%08X visual=%08X unbrauchbar -> NULL",
                    (unsigned)npc, (unsigned)npc->visual);
#endif
                npc->visual = NULL;
            }
        }

        // Kurzbefund nach den ersten Spawns: hat der Coop-NPC ein Modell, und wo steht er?
        void ReportSpawnState(int instanceId) {
#if ENGINE == Engine_G1
            static int reported = 0;
            if (reported >= 5 || !npc)
                return;
            reported++;
            zVEC3 ist = npc->GetPositionWorld();
            zVEC3 soll = lastPositionFromServer ? *lastPositionFromServer : ist;
            OthelloCompatLog("Spawn %s: Instanz %d, npc=%08X visual=%08X model=%08X magbook=%08X,"
                " Soll %.0f %.0f %.0f, Ist %.0f %.0f %.0f",
                zSTRING(name).ToChar(), instanceId, (unsigned)npc, (unsigned)npc->visual,
                (unsigned)npc->GetModel(), (unsigned)npc->GetSpellBook(),
                soll.n[0], soll.n[1], soll.n[2], ist.n[0], ist.n[1], ist.n[2]);
            // Ein Instanz-Symbol zeigt auf das zuletzt damit erzeugte Objekt. Ist die Freund-Instanz
            // PC_HERO, meinen Skripte, die "PC_HERO" ansprechen, danach die Coop-Figur statt des Helden.
            if (zCPar_Symbol* sym = parser->GetSymbol(instanceId)) {
                const unsigned target = (unsigned)sym->offset;
                OthelloCompatLog("  Symbol %s zeigt jetzt auf %08X%s (Coop-Figur %08X, eigener Held %08X)",
                    sym->name.ToChar(), target,
                    target == (unsigned)npc ? " = COOP-FIGUR" : (target == (unsigned)player ? " = eigener Held" : ""),
                    (unsigned)npc, (unsigned)player);
            }
#endif
        }

        void InitCoopFriendNpc() {
            int instanceId = GetFriendDefaultInstanceId();
            if (instanceId <= 0) {
                ChatLog("Invalid NPC instance id.");
                return;
            }
            if (!npc) {
                npc = dynamic_cast<oCNpc*>(ogame->GetGameWorld()->CreateVob(zTVobType::zVOB_TYPE_NSC, instanceId));
                if (npc == NULL) {   // dynamic_cast fehlgeschlagen: nicht weitermachen
                    ChatLog("CreateVob returned no NPC.");
                    return;
                }
            }

            // v41: beim Host mit Verfolgungspunkt auch weit weg einsetzen (InsertNpc prueft sonst den Abstand zum eigenen Helden)
            if (!CoopFollowActive() || !CoopSpawnInsertAt(npc, *lastPositionFromServer))
                ogame->spawnman->InsertNpc(npc, *lastPositionFromServer);
            isSpawned = true;
            npc->name[0] = playerNickname.IsEmpty() ? zSTRING(name) : playerNickname;

            npc->SetObjectName(name);
            npc->SetVobName(name);
            npc->SetVobPresetName(name);

            // MakeSpellBook und UseStandAI sind an feste Engine-Adressen gebunden, die
            // Othello umwidmet. Die Coop*-Varianten pruefen das (OthelloCompat.cpp).
            CoopMakeSpellBook(npc);

            CoopUseStandAI(npc);
            npc->dontWriteIntoArchive = TRUE;
            npc->idx = 69133769;

            SanitizeStaleVisual();
            ApplyLook();   // v55: Kopf, Statur, Hautton wie vom Mitspieler gewaehlt (vorher fest HUM_HEAD_PONY / hum_body_Naked0)
            ReportSpawnState(instanceId);

#if ENGINE >= Engine_G2
            npc->SetHitChance(1, 100);
            npc->SetHitChance(2, 100);
            npc->SetHitChance(3, 100);
            npc->SetHitChance(4, 100);
#endif
            if (lastMaxHpFromServer != -1) {
                npc->SetAttribute(NPC_ATR_HITPOINTSMAX, lastMaxHpFromServer);
            }
            npc->SetAttribute(NPC_ATR_STRENGTH, COOP_MAGIC_NUMBER);
            npc->SetAttribute(NPC_ATR_DEXTERITY, COOP_MAGIC_NUMBER);
            npc->SetAttribute(NPC_ATR_MANA, 10000);
            npc->SetAttribute(NPC_ATR_MANAMAX, 10000);

            auto armor = npc->GetEquippedArmor();
            if (armor) {
                npc->UnequipItem(armor);
            }

            auto weapon1 = npc->GetEquippedMeleeWeapon();
            if (weapon1) {
                npc->UnequipItem(weapon1);
            }

            auto weapon2 = npc->GetEquippedRangedWeapon();
            if (weapon2) {
                npc->UnequipItem(weapon2);
            }

            if (lastWeapon1 > 0) {
                auto weapon = CreateCoopItem(lastWeapon1);
                npc->Equip(weapon);
            }

            if (lastWeapon2 > 0) {
                auto weapon = CreateCoopItem(lastWeapon2);
                npc->Equip(weapon);
            }

            if (lastArmor > 0) {
                auto armor = CreateCoopItem(lastArmor);
                npc->Equip(armor);
            }

            if (lastWeaponMode > 0) {
                npc->SetWeaponMode2(lastWeaponMode);
            }

            if (IsCoopPlayer(name)) {
                static int AIV_PARTYMEMBER = GetPartyMemberID();
                npc->aiscriptvars[AIV_PARTYMEMBER] = True;
            }

            PlayerNpcs[npc] = name;
            PlayerNameToNpc[name] = npc;
        }

        void RemoveCoopItemsFromGround() {
            if (hasNpc && npc && syncedNpcItems.size() > 0) {
                auto rightItem = npc->GetRightHand();
                auto leftItem = npc->GetLeftHand();
                auto currentWeapon1 = npc->GetEquippedMeleeWeapon();
                auto currentWeapon2 = npc->GetEquippedRangedWeapon();

                for (auto syncedItem : syncedNpcItems) {
                    auto item = syncedItem.first;

                    if (!item) {
                        syncedNpcItems.erase(item);
                    }

                    if (item != leftItem && item != rightItem && item != currentWeapon1 && item != currentWeapon2) {
                        item->RemoveVobFromWorld();
                        syncedNpcItems.erase(item);
                    }
                }
            }
        }

        void RespawnOrDestroyBasedOnDistance() {
            if (hasNpc && npc && lastPositionFromServer) {
                auto dist = (int)(*lastPositionFromServer - player->GetPositionWorld()).LengthApprox();

                if (IsCoopPlayer(name)) {
                    if (CoopFollowActive()) {
                        // v41 (Verfolgungspunkt, Host): die Figur bleibt immer in der Welt - sonst verwarf UpdateAttacks die Treffer
                        // des Mitspielers ausserhalb der Reichweite und kein NPC um ihn sah ihn. Neuer Versuch hoechstens alle 2 s.
                        const long long now = GetCurrentMs();
                        if ((!isSpawned || !hasModel) && now >= nextFollowSpawnMs) {
                            nextFollowSpawnMs = now + 2000;
                            InitCoopFriendNpc();
                            isSpawned = true;
                        }
                        return;
                    }
                    if (dist > BROADCAST_DISTANCE && isSpawned) {
                        CoopDeleteNpc(ogame->spawnman, npc);
                        isSpawned = false;
                    }
                    if (dist < BROADCAST_DISTANCE && (!isSpawned || !hasModel)) {
                        InitCoopFriendNpc();
                        isSpawned = true;
                    }
                }
                else if (dist > BROADCAST_DISTANCE * 1.5 && !CoopFollowNear(*lastPositionFromServer, BROADCAST_DISTANCE * 1.5f)) {
                    // v41: im Umkreis eines anderen Spielers (Verfolgungspunkt) weiter vom Host fuehren lassen - einsetzen/behalten
                    // erledigt der Spawn-Manager-Haken (CoopFarSync.cpp)
                    destroyed = true;
                    return;
                }
                else if (dist < BROADCAST_DISTANCE && !hasModel) {
                    ogame->spawnman->InsertNpc(npc, *lastPositionFromServer);
                }
                else if (!hasModel && !npc->IsDead() && CoopFollowActive() && CoopFollowNear(*lastPositionFromServer, (float)BROADCAST_DISTANCE)) {
                    // v43: der Host fuehrt den NPC im Umkreis einer Figur, hier ist er nicht in der Welt (z. B. folgt er dem Host,
                    // seine Tagesablauf-Stelle liegt hier weit weg - der Spawn-Manager setzt ihn dann nie ein). An der Lage des
                    // Hosts einsetzen, hoechstens alle 2 s, nicht wenn er in der Spawn-Liste steht (die setzt ihn selbst ein).
                    const long long now = GetCurrentMs();
                    if (now >= nextFollowSpawnMs && !CoopSpawnListHas(npc)) {
                        nextFollowSpawnMs = now + 2000;
                        if (CoopSpawnInsertAt(npc, *lastPositionFromServer))
                            CoopFarCountLedInsert();
                    }
                }
            }
        }

        bool IsNpcInTot() {
            if (lastPositionFromServer && CurrentWorldTOTPosition && npc && !IsCoopPlayer(name)) {
                auto newPosition = npc->GetPositionWorld();
                auto totPos = *CurrentWorldTOTPosition;
                int dist = GetDistance3D(newPosition.n[0], newPosition.n[1], newPosition.n[2], totPos.n[0], totPos.n[1], totPos.n[2]);
                if (dist < 500) {
                    destroyed = true;
                    return true;
                }
            }

            return false;
        }
    };
}