namespace GOTHIC_ENGINE {
    int CoopDiffHpBase(oCNpc* n);   // v70: CoopDifficulty.cpp - Grundwert skalierter Lebenspunkte (sonst -1)

    class LocalNpc
    {
    public:
        oCNpc* npc;
        string name;
        bool initialized = false;
        bool destroyed = false;
        bool hasModel = false;
        std::list<int> pendingUpdates;
        std::list<PlayerHit> hitsToSync;
        std::list<SpellCast> spellCastsToSync;
        std::vector<int> newAnimIds;
        zCModelAni* lastAnimation = NULL;   // v53: vorher nicht vorbelegt (Reinit las sie ggf. ungesetzt)
        std::vector<std::string> newAniNames;              // v53: Namen in Startreihenfolge (Paket "al")
        zCModelAni* lastAniHistory[MAX_ANIHISTORY] = {};   // v53: Stand von zCModel::aniHistoryList beim letzten Bild
        bool aniHistoryInit = false;
        zCArray<int> pArrOverlays;
        zVEC3 lastPosition;
        float lastHeading = 0;
        int lastWeaponMode;
        int lastSyncHp = -1;
        int lastSyncMaxHp;
        int lastProtections[8];
        int lastTalents[4];
        int lastBodyState;
        zSTRING lastSpellInstanceName = "NULL";
        zSTRING lastWeapon1Name;
        zSTRING lastWeapon2Name;
        zSTRING lastArmorName;
        zSTRING lastLeftHandInstanceName;
        zSTRING lastRightHandInstanceName;
        zSTRING revivedFriend = "";
        long long lastTimeSyncTime = 0;
        oCItem* pItemDropped = NULL;
        oCItem* pItemTaken = NULL;
        bool itemDropReady = false;
        zVEC3 pItemTakenPos;
        bool leadSend = false;   // v72: NPC, den dieser Client im Kampf fuehrt (CoopCombatLead.cpp) - Pakete tragen "lead"

        LocalNpc(oCNpc* _npc, string _name) {
            npc = _npc;
            name = _name;
        }

        void Pulse() {
            if (destroyed) {
                return;
            }

            hasModel = npc && npc->GetModel() && npc->vobLeafList.GetNum() > 0;
 
            this->SyncInitialization();
            //this->SyncBodystate();
            this->SyncPosition();
            this->SyncAngle();
            this->SyncOverlays();
            this->SyncAnimation();
            this->SyncWeaponMode();
            this->SyncAttacks();
            this->SyncSpellCasts();
            this->SyncMagicSetup();
            this->SyncHp();
            

            if (npc == player) {
                this->SyncArmor();
                this->SyncWeapons();
                this->SyncProtection();
                this->SyncTalents();
                //this->SyncHand();

                if (ServerThread) {
                    if (CurrentMs > lastTimeSyncTime + 60000) {
                        this->SyncTime();
                        lastTimeSyncTime = CurrentMs;
                    }
                }
            }
        }

        void Reinit() {
            initialized = false;
            lastPosition = NULL;
            lastHeading = 0;
            lastWeaponMode = 0;
            lastSyncHp = -1;
            lastSyncMaxHp = 0;
            lastBodyState = 0;
            lastWeapon1Name = zSTRING();
            lastWeapon2Name = zSTRING();
            lastArmorName = zSTRING();
            lastLeftHandInstanceName = zSTRING();
            lastRightHandInstanceName = zSTRING();
            lastSpellInstanceName = "NULL";
            lastTimeSyncTime = 0;
            lastProtections[0] = -1;
            lastProtections[1] = -1;
            lastProtections[2] = -1;
            lastProtections[3] = -1;
            lastProtections[4] = -1;
            lastProtections[5] = -1;
            lastProtections[6] = -1;
            lastProtections[7] = -1;
            lastTalents[0] = 0;
            lastTalents[1] = 0;
            lastTalents[2] = 0;
            lastTalents[3] = 0;
            pItemDropped = NULL;
            itemDropReady = false;
            pItemTaken = NULL;
            pItemTakenPos = zVEC3(0, 0, 0);
            pArrOverlays.DeleteList();
            
            if (lastAnimation) {
                newAnimIds.push_back(lastAnimation->aniID);
                newAniNames.push_back(std::string(lastAnimation->GetAniName().ToChar()));   // v53
            }
        }

        void SyncInitialization() {
            if (!initialized) {
                addUpdate(INIT_NPC);
                initialized = true;
            }
        }

        // v55: Name/Aussehen im COOP-Menue geaendert - der Partner uebernimmt sie aus INIT_NPC auch an der stehenden Figur.
        void ResendInit() {
            if (npc == player)
                addUpdate(INIT_NPC);
        }

        // v52: Lage und Blickrichtung hoechstens alle 33 ms (eigener Held) bzw. 50 ms (NPCs) senden. Vorher bei jeder
        // Aenderung, also jedes Bild: ein laufender NPC erzeugte bei 60 Bildern/s 60 Lage-Pakete je Sekunde. Der Empfaenger
        // glaettet und rechnet zwischen den Paketen weiter (RemoteNpc::UpdateNpcPosition). Haelt der NPC an, geht die
        // letzte Lage spaetestens nach einem Takt hinaus (lastPosition bleibt bis zum Senden stehen).
        // Konfiguration: coopSendIntervalHeroMs (33), coopSendIntervalNpcMs (50); 0 = wie bisher jedes Bild.
        long long lastPosSentMs = 0;
        long long lastHeadingSentMs = 0;

        static long long SendIntervalMs(bool hero) {
            static int heroMs = -1, npcMs = -1;
            if (heroMs < 0) {
                heroMs = 33;
                npcMs = 50;
                if (CoopConfig.contains("coopSendIntervalHeroMs") && CoopConfig["coopSendIntervalHeroMs"].is_number_integer())
                    heroMs = CoopConfig["coopSendIntervalHeroMs"].get<int>();
                if (CoopConfig.contains("coopSendIntervalNpcMs") && CoopConfig["coopSendIntervalNpcMs"].is_number_integer())
                    npcMs = CoopConfig["coopSendIntervalNpcMs"].get<int>();
                if (heroMs < 0 || heroMs > 500) heroMs = 33;
                if (npcMs < 0 || npcMs > 500) npcMs = 50;
            }
            return hero ? heroMs : npcMs;
        }

        void SyncPosition()
        {
            zVEC3 playerPos = npc->GetPositionWorld();

            int dist = lastPosition != NULL ?
                GetDistance3D(playerPos.n[0], playerPos.n[1], playerPos.n[2], lastPosition.n[0], lastPosition.n[1], lastPosition.n[2]) :
                999;

            if (dist > 5 && (dist > 300 || CurrentMs - lastPosSentMs >= SendIntervalMs(npc == player)))
            {
                addUpdate(SYNC_POS);
                lastPosition = playerPos;
                lastPosSentMs = CurrentMs;
            }
        };

        void SyncAngle() {
            float currentHeading = GetHeading(npc);
            if (abs(currentHeading - lastHeading) > 3 && CurrentMs - lastHeadingSentMs >= SendIntervalMs(npc == player))
            {
                addUpdate(SYNC_HEADING);
                lastHeading = currentHeading;
                lastHeadingSentMs = CurrentMs;
            }
        }

        // v53: alle seit dem letzten Bild gestarteten Animationen in Startreihenfolge (vorher nur die zuletzt gestartete, und
        // mehrere wurden von hinten verschickt - die aeltere kam zuletzt an und blieb stehen). Treffer-Reaktionen und Drehungen
        // gehen jetzt mit (PlayerAnimation.cpp). Dieselbe Zustands-Animation (S_...) direkt hintereinander nur einmal.
        // v54 (L7, Nutzer nach Test v53: "es sieht so aus, als wuerde sich der Mitspieler drehen, obwohl er fest steht, ausserdem
        // sind die Animationen gar nicht mehr synchron"): Drehungen wieder nicht (wie bis v52). Die Dreh-Animationen laufen in
        // sich selbst weiter (naechste Animation = sie selbst), die Engine des Senders beendet sie mit StopTurnAnis
        // (0x00625A20) - ein Ende steht nicht in aniHistoryList, beim Empfaenger lief die Drehung also endlos weiter und
        // ueberlagerte jede folgende Animation. Die Blickrichtung kommt weiter ueber SYNC_HEADING.
        void SyncAnimation() {
            if (!hasModel) {
                return;
            }
            zCModel* model = npc->GetModel();
            if (!model || !model->aniHistoryList) {
                return;
            }

            zCModelAni* cur[MAX_ANIHISTORY];
            for (int i = 0; i < MAX_ANIHISTORY; i++)
                cur[i] = model->aniHistoryList[i];
            int fresh = CoopAniHistoryNew(lastAniHistory, cur);
            if (!aniHistoryInit) {
                aniHistoryInit = true;
                fresh = 1;   // beim ersten Bild nur die neueste
            }
            if (fresh > 4)
                fresh = 4;
            for (int k = MAX_ANIHISTORY - fresh; k < MAX_ANIHISTORY; k++) {
                zCModelAni* ani = cur[k];
                if (!ani || IsAniBlockedForAll(ani) || IsAnimationTurning(ani))
                    continue;
                const zSTRING aniName = ani->GetAniName();
                if (ani == lastAnimation && aniName.Length() > 1 && aniName.ToChar()[0] == 'S' && aniName.ToChar()[1] == '_')
                    continue;
                newAnimIds.push_back(ani->aniID);
                newAniNames.push_back(std::string(aniName.ToChar()));
                lastAnimation = ani;
            }
            for (int i = 0; i < MAX_ANIHISTORY; i++)
                lastAniHistory[i] = cur[i];

            if (newAnimIds.size() > 0)
            {
                addUpdate(SYNC_ANIMATION);
            }
        }

        void SyncWeaponMode() {
            int currentWeaponMode = npc->GetWeaponMode();
            if (currentWeaponMode != lastWeaponMode) {
                addUpdate(SYNC_WEAPON_MODE);
                lastWeaponMode = currentWeaponMode;
            }
        }

        void SyncAttacks() {
            if (hitsToSync.size() > 0) {
                addUpdate(SYNC_ATTACKS);
            }
        }

        void SyncSpellCasts() {
            if (spellCastsToSync.size() > 0) {
                addUpdate(SYNC_SPELL_CAST);
            }
        }

        void SyncHand() {
            auto leftHand = npc->GetLeftHand();
            auto rightHand = npc->GetRightHand();

            auto leftHandInstanceName = leftHand ? leftHand->GetInstanceName() : "NULL";
            auto rightHandInstanceName = rightHand ? rightHand->GetInstanceName() : "NULL";

            if (!leftHandInstanceName.Compare(lastLeftHandInstanceName) || !lastRightHandInstanceName.Compare(rightHandInstanceName)) {
                addUpdate(SYNC_HAND);
                lastLeftHandInstanceName = leftHandInstanceName;
                lastRightHandInstanceName = rightHandInstanceName;
            }
        }

        void SyncArmor() {
            auto armor = npc->GetEquippedArmor();
            auto armorName = armor ? armor->GetInstanceName().ToChar() : "NULL";

            if (!lastArmorName.Compare(armorName)) {
                addUpdate(SYNC_ARMOR);
                lastArmorName = armorName;
            }
        }

        void SyncOnDropItem()
        {
            if (itemDropReady)
            {
                addUpdate(SYNC_DROPITEM);
            }
        }

        void SyncOnTakeItem()
        {
            if (pItemTaken)
            {
                addUpdate(SYNC_TAKEITEM);
            }
        }

        void SyncWeapons() {
            if (!npc->IsHuman()) {
                return;
            }

            auto weapon1 = npc->GetEquippedMeleeWeapon();
            auto weapon1Name = weapon1 ? weapon1->GetInstanceName() : "NULL";

            auto weapon2 = npc->GetEquippedRangedWeapon();
            auto weapon2Name = weapon2 ? weapon2->GetInstanceName() : "NULL";

            if (!lastWeapon1Name.Compare(weapon1Name) || !lastWeapon2Name.Compare(weapon2Name)) {
                addUpdate(SYNC_WEAPONS);
                
                lastWeapon1Name = weapon1Name;
                lastWeapon2Name = weapon2Name;
            }

        }

        void SyncBodystate() {
            auto bs = npc->GetBodyState();

            if (bs != lastBodyState)
            {
                addUpdate(SYNC_BODYSTATE);
                lastBodyState = bs;
            }
        }

        void SyncOverlays() {
            if (!npc->CompareOverlaysArray(pArrOverlays))
            {
                addUpdate(SYNC_OVERLAYS);
                pArrOverlays = GetNpcMds(npc);
            }
        }

        void SyncHp() {
            auto currentHp = npc->GetAttribute(NPC_ATR_HITPOINTS);
            auto currentMaxHp = npc->GetAttribute(NPC_ATR_HITPOINTSMAX);

            if (npc && npc->GetObjectName().Compare("BDT_50011_Addon_Raven") && currentHp == 100000) {
                return; // BDT_50011_Addon_Raven souls attack cutscene hack
            }

            if (currentHp != lastSyncHp || currentMaxHp != lastSyncMaxHp) {
                addUpdate(SYNC_HP);

                lastSyncHp = currentHp;
                lastSyncMaxHp = currentMaxHp;
            }
        }

        void SyncTalents() {
            int currentTalents[4];

            currentTalents[0] = npc->GetTalentSkill(oCNpcTalent::NPC_TAL_1H);
            currentTalents[1] = npc->GetTalentSkill(oCNpcTalent::NPC_TAL_2H);
            currentTalents[2] = npc->GetTalentSkill(oCNpcTalent::NPC_TAL_BOW);
            currentTalents[3] = npc->GetTalentSkill(oCNpcTalent::NPC_TAL_CROSSBOW);

            for (int i = 0; i < 4; i++) {
                if (currentTalents[i] != lastTalents[i]) {
                    addUpdate(SYNC_TALENTS);
                    break;
                }
            }

            for (int i = 0; i < 4; i++) {
                lastTalents[i] = currentTalents[i];
            }
        }

        void SyncProtection() {
            int currentProtections[8];
            for (int i = 0; i < 8; i++) {
                currentProtections[i] = npc->GetProtectionByIndex(static_cast<oEIndexDamage>(i));
            }

            for (int i = 0; i < 8; i++) {
                if (currentProtections[i] != lastProtections[i]) {
                    addUpdate(SYNC_PROTECTIONS);
                    break;
                }
            }

            for (int i = 0; i < 8; i++) {
                lastProtections[i] = currentProtections[i];
            }
        }

        void SyncMagicSetup() {
            if (!hasModel || lastSyncHp == 0) {
                return;
            }

            if (npc->GetWeaponMode() != NPC_WEAPON_MAG && !lastSpellInstanceName.Compare("NULL")) {
                addUpdate(SYNC_MAGIC_SETUP);
                lastSpellInstanceName = "NULL";
            }

            if (npc->GetWeaponMode() == NPC_WEAPON_MAG)
            {
                oCMag_Book* book = npc->GetSpellBook();
                if (book)
                {
                    int spellID = book->GetSelectedSpellNr();
                    if (spellID >= 0)
                    {
                        oCItem* item = book->GetSpellItem(spellID);
                        if (item)
                        {
                            auto itemName = item->GetInstanceName();
                            if (!itemName.Compare(lastSpellInstanceName)) {
                                addUpdate(SYNC_MAGIC_SETUP);
                                lastSpellInstanceName = itemName;
                            }
                        }
                    }
                }
            }
        }

        void SyncTime() {
            addUpdate(SYNC_TIME);
        }

        void SyncRevived(zSTRING friendName) {
            revivedFriend = friendName;
            addUpdate(SYNC_REVIVED);
        }

        void addUpdate(int type) {
            bool found = std::find(pendingUpdates.begin(), pendingUpdates.end(), type) != pendingUpdates.end();
            if (!found) {
                pendingUpdates.push_back(type);
            }
        }

        void PackUpdate() {
            for each (auto type in pendingUpdates)
            {
                CoopTraceCount(TRACE_SENT, type);
                json j;
                j["type"] = type;
                j["id"] = name;
                if (leadSend)
                    j["lead"] = name;   // v72: der Host setzt "id" sonst auf den Absender (FRIEND_n)
                this->AddUpdatePayload(type, j);
                ReadyToSendJsons.enqueue(j);
            }

            pendingUpdates.clear();
        }

        void AddUpdatePayload(int type, json& j) {
            switch (type)
            {
                case INIT_NPC:
                {
                    j["instanceId"] = parser->GetIndex(npc->GetInstanceName());
                    j["nickname"] = MyNickname;
                    j["x"] = lastPosition.n[0];
                    j["y"] = lastPosition.n[1];
                    j["z"] = lastPosition.n[2];
                    j["bodyTextVarNr"] = MyBodyTextVarNr;
                    j["headVarNr"] = MyHeadVarNr;
                    j["headMesh"] = MyHeadMesh;   // v55: Kopf, Statur, Hautton aus dem COOP-Menue (CoopLook.cpp)
                    j["bodyMesh"] = MyBodyMesh;
                    j["skin"] = MySkin;
                    break;
                }
                case SYNC_POS:
                {
                    j["x"] = lastPosition.n[0];
                    j["y"] = lastPosition.n[1];
                    j["z"] = lastPosition.n[2];
                    break;
                }
                case SYNC_HEADING:
                {
                    j["h"] = lastHeading;
                    break;
                }
                case SYNC_ANIMATION:
                {
                    // v53: alle neuen Animationen in Reihenfolge per Name ("al"), "a" = letzte Nummer wie bisher
                    j["a"] = newAnimIds.empty() ? -1 : newAnimIds.back();
                    auto names = nlohmann::json::array();
                    for (auto& n : newAniNames)
                        names.push_back(n);
                    j["al"] = names;
                    newAnimIds.clear();
                    newAniNames.clear();
                    break;
                }
                case SYNC_WEAPON_MODE:
                {
                    j["wm"] = lastWeaponMode;
                    break;
                }
                case SYNC_MAGIC_SETUP:
                {
                    j["spell"] = lastSpellInstanceName;
                    break;
                }
                case SYNC_SPELL_CAST:
                {
                    auto casts = nlohmann::json::array();
                    for each (auto sc in spellCastsToSync) {
                        nlohmann::json cast;
                        cast["target"] = sc.targetNpcUniqueName;
                        casts.push_back(cast);
                    }

                    j["casts"] = casts;
                    spellCastsToSync.clear();
                    break;
                }
                case SYNC_ARMOR:
                {
                    j["armor"] = lastArmorName;
                    break;
                }
                case SYNC_WEAPONS:
                {
                    j["w1"] = lastWeapon1Name;
                    j["w2"] = lastWeapon2Name;
                    break;
                }
                case SYNC_HP:
                {
                    j["hp"] = lastSyncHp;
                    j["hp_max"] = lastSyncMaxHp;
                    {
                        const int hb = CoopDiffHpBase(npc);   // v70: Grundwert, wenn "Leben der Gegner" ihn skaliert hat
                        if (hb > 0)
                            j["hb"] = hb;
                    }
                    break;
                }
                case SYNC_BODYSTATE:
                {
                    j["bs"] = lastBodyState;
                    break;
                }
                case SYNC_OVERLAYS:
                {
                    auto overlays = nlohmann::json::array();

                    for (int i = 0; i < pArrOverlays.GetNumInList(); i++)
                    {
                        nlohmann::json overlay;
                        overlay["over"] = pArrOverlays.GetSafe(i);
                        // Name zusaetzlich zur Nummer: Nummern ab 100 sind nur lokal gueltig
                        // (GetMdsIndex), der Empfaenger loest den Namen selbst auf.
                        overlay["name"] = GetMdsByIndex(pArrOverlays.GetSafe(i)).ToChar();
                        overlays.push_back(overlay);
                    }

                    j["overlays"] = overlays;
                    break;
                }
                case SYNC_PROTECTIONS:
                {
                    j["p0"] = lastProtections[0];
                    j["p1"] = lastProtections[1];
                    j["p2"] = lastProtections[2];
                    j["p3"] = lastProtections[3];
                    j["p4"] = lastProtections[4];
                    j["p5"] = lastProtections[5];
                    j["p6"] = lastProtections[6];
                    j["p7"] = lastProtections[7];
                    break;
                }
                case SYNC_TALENTS:
                {
                    j["t0"] = lastTalents[0];
                    j["t1"] = lastTalents[1];
                    j["t2"] = lastTalents[2];
                    j["t3"] = lastTalents[3];
                    break;
                }
                case SYNC_HAND:
                {
                    j["left"] = lastLeftHandInstanceName;
                    j["right"] = lastRightHandInstanceName;
                    break;
                }
                case SYNC_TIME:
                {
                    int _a, h, m;
                    ogame->GetTime(_a, h, m);

                    j["h"] = h;
                    j["m"] = m;
                    break;
                }
                case SYNC_REVIVED:
                {
                    j["name"] = revivedFriend;
                    revivedFriend = "";
                    break;
                }
                case SYNC_ATTACKS:
                {
                    auto attacks = nlohmann::json::array();
                    for each (auto at in hitsToSync) {
                        nlohmann::json att;
                        att["target"] = at.npcUniqueName;
                        att["damage"] = at.damage;
                        att["isUnconscious"] = at.isUnconscious;
                        att["isDead"] = at.isDead;
                        att["damageMode"] = at.damageMode;
                        attacks.push_back(att);
                    }

                    j["att"] = attacks;
                    hitsToSync.clear();
                    break;
                }
                case SYNC_DROPITEM:
                {
                    if (pItemDropped && itemDropReady)
                    {
                        j["itemDropped"] = pItemDropped->GetInstanceName();
                        j["count"] = pItemDropped->amount;
                        j["flags"] = pItemDropped->flags;
                        j["itemUniqName"] = pItemDropped->GetObjectName();

                        itemDropReady = false;
                    }
                    break;
                }
                case SYNC_TAKEITEM:
                {
                    if (pItemTaken)
                    {
                        j["itemDropped"] = pItemTaken->GetInstanceName();
                        j["count"] = pItemTaken->amount;
                        j["flags"] = pItemTaken->flags;
                        j["uniqName"] = pItemTaken->GetObjectName();
                        j["x"] = pItemTakenPos.n[0];
                        j["y"] = pItemTakenPos.n[1];
                        j["z"] = pItemTakenPos.n[2];

                        pItemTaken->RemoveVobFromWorld();
                        pItemTaken = NULL;
                    }
                    break;
                }
                case DESTROY_NPC:
                {
                    break;
                }
            } 
        }
    };
}