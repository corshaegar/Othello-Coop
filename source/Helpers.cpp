namespace GOTHIC_ENGINE {
    void CoopLog(std::string l)
    {
        CoopLogArchiveOnce();   // v35
        std::ofstream CoopLog(GothicCoopLogPath, std::ios_base::app | std::ios_base::out);
        CoopLog << l;
    }

    void ChatLog(string text, zCOLOR color = zCOLOR(255, 255, 255, 255)) {
        GameChat->AddLine(text, color);
    };

    int GetFreePlayerId() {
        LastFreePlayerId += 1;
        return LastFreePlayerId;
    }

    bool IsCoopPlayer(std::string name) {
        string cStringName = name.c_str();
        return cStringName == "HOST" || cStringName.StartWith("FRIEND_");
    }

    bool IsCoopPlayer(string name) {
        return name == "HOST" || name.StartWith("FRIEND_");
    }

    oCItem* CreateCoopItem(int insIndex) {
        return zfactory->CreateItem(insIndex);
    }

    // v39 (Nutzer: "nur noch mit Shift+F1, Shift+F2, Shift+F3, Shift+F4, damit man nicht aus Versehen disconnected"):
    // Shift gedrueckt? Wie CoopSimShiftDown (CoopDialogSync.cpp): zinput, sonst Windows-Taste, wenn das Spielfenster vorn ist.
    bool CoopShiftDown() {
        if (zinput->KeyPressed(KEY_LSHIFT) || zinput->KeyPressed(KEY_RSHIFT))
            return true;
        HWND fg = GetForegroundWindow();
        DWORD pid = 0;
        if (fg)
            GetWindowThreadProcessId(fg, &pid);
        return pid == GetCurrentProcessId() && (GetAsyncKeyState(VK_SHIFT) & 0x8000);
    }

    // v51 (O1): Test v50 - beim Client wurde die Host-Figur in 5 min viermal geloescht und neu erzeugt (neue Adresse, gleiche
    // Lage), das Zuschau-Fenster ging jedes Mal aus. Ursache: Othellos Test-Modus legt Noclip auf Shift+F3 (OTHELLO_ABI:
    // "noclip (SHIFT + F3)", "deactivate (F3)", angezeigt nur bei ogame->game_testmode = oCGame+0xB0, wie im SDK) - dieselbe
    // Taste wie GTs "Figuren neu" (reinitPlayersKey, Standard F3). Jedes Ein-/Ausschalten von Noclip erzeugte alle Coop-Figuren
    // neu (Schnappschuss des Clients: S_NOCLIP genau zu diesen Zeiten). Allgemein: im Test-Modus belegt schon die Engine F2-F12
    // (oCGame::TestKeys 0x00660000, Schalter ueber 0x006625D0/0x00662538; Othello ersetzt den F3-Zweig 0x00660DF1 durch
    // OTHELLO_ABI+0x89180). Jetzt: liegt "Figuren neu" auf F2-F12 und ist der Test-Modus an, tut GT nichts. Beides wird
    // protokolliert, damit ein Neuaufbau der Figuren im Log zu sehen ist.
    bool CoopReinitKeyAllowed(int key) {
        const bool fkey = (key >= KEY_F2 && key <= KEY_F10) || key == KEY_F11 || key == KEY_F12;
        const bool testmode = ogame && ogame->game_testmode != 0;
        const bool allowed = !(fkey && testmode);
#if ENGINE == Engine_G1
        static CoopDiagBudget logged = { 0, 0 };
        if (CoopDiagAllow(logged, 10)) {
            if (allowed)
                OthelloCompatLog("[%lld s, %s] Figuren neu (Taste reinitPlayersKey mit Shift): %u Figur(en) werden neu erzeugt",
                    DiagSeconds(), DiagRole(), (unsigned)PlayerNpcs.size());
            else
                OthelloCompatLog("[%lld s, %s] Shift+F-Taste im Test-Modus gehoert dem Spiel (Othello: Shift+F3 = Noclip) -"
                    " Figuren neu nicht ausgeloest", DiagSeconds(), DiagRole());
        }
#endif
        return allowed;
    }

    int CoopFollowPoints(zVEC3* out, int max);   // v41: CoopFarSync.cpp

    std::vector<oCNpc*> GetVisibleNpcs() {
        std::vector<oCNpc*> npcs;
        auto* list = CoopWorldNpcListFirst(ogame->GetGameWorld());
        // v41: auch NPCs im Umkreis der Figuren (Verfolgungspunkt beim Mitspieler, nur Host)
        zVEC3 follow[8];
        const int followCount = CoopFollowPoints(follow, 8);

        while (list) {
            auto npc = list->data;
            auto npcPosition = npc->GetPositionWorld();
            auto playerPosition = player->GetPositionWorld();

            if (npc->vobLeafList.GetNum() == 0) {
                list = list->next;
                continue;
            }

            bool nearFigure = false;
            for (int f = 0; f < followCount && !nearFigure; f++)
                nearFigure = follow[f].Distance(npcPosition) < BROADCAST_DISTANCE;

            if (playerPosition.Distance(npcPosition) < BROADCAST_DISTANCE || nearFigure) {
                if (!npc->IsAPlayer() && !IsCoopPlayer(npc->GetObjectName())) {
                    npcs.push_back(npc);
                }
            }
            list = list->next;
        }

        return npcs;
    }

    float GetHeading(oCNpc* npc)
    {
        float x = *(float*)((DWORD)npc + 0x44);
        float rotx = asin(x) * 180.0f / 3.14f;
        float y = *(float*)((DWORD)npc + 0x64);
        if (y > 0)
        {
            if (x < 0)
                rotx = 360 + rotx;
        }
        else
        {
            if (rotx > 0)
                rotx = 180 - rotx;
            else
            {
                rotx = 180 + rotx;
                rotx = 360 - rotx;
            }
        }
        return rotx;
    };

    int ReadConfigKey(std::string key, string _default) {
        auto stringKey = CoopConfig.contains(key) ?
            string(CoopConfig[key].get<std::string>().c_str()).ToChar() :
            _default;

        return GetEmulationKeyCode(stringKey);
    }

    bool IsPlayerTalkingWithAnybody() {
        return ogame->GetCameraAI()->GetMode().Compare("CAMMODDIALOG");
    }

    bool IsPlayerTalkingWithNpc(zCVob* npc) {
        if (!ogame->GetCameraAI()->GetMode().Compare("CAMMODDIALOG")) {
            return false;
        }

        if (player->talkOther == npc) {
            return true;
        }

        if (ogame->GetCameraAI()->targetVobList.GetNum() > 0) {
            for (int i = 0; i < ogame->GetCameraAI()->targetVobList.GetNum(); i++) {
                auto vob = ogame->GetCameraAI()->targetVobList[i];
                if (npc == vob) {
                    return true;
                }
            }
        }

        return false;
    }

    // Fuehrt Gothics Dialogverwaltung diesen NPC gerade als Gespraechspartner des Spielers?
    // IsPlayerTalkingWithNpc erkennt den Dialog erst am Kameramodus CAMMODDIALOG; der springt
    // aber erst an, wenn sich der NPC schon zum Spieler gedreht hat. Vorher blockierte GT auf dem
    // Client die Dreh-Animationen - die Drehung wurde nie fertig, der Dialog hing.
    //
    // Bewusst NICHT ueber talkOther: Gothic setzt es auch bei Drohung/Kampfansage und loescht es
    // danach nicht (gemessen: talkOther=CS_ESKORTE2 bei IsDone=1). In v6 fuehrte das dazu, dass
    // der Client alle Host-Daten dieses NPCs verwarf - Position, Animation und Treffer.
    bool CoopNpcInDialogWithPlayer(zCVob* vob) {
        if (!vob || !player || vob->GetCharacterClass() != 2)
            return false;
        oCInformationManager& im = oCInformationManager::GetInformationManager();
        return im.Npc == (oCNpc*)vob && !im.IsDone;
    }

    float GetDistance3D(float aX, float aY, float aZ, float bX, float bY, float bZ)
    {
        float distX = aX - bX;
        float distY = aY - bY;
        float distZ = aZ - bZ;
        return sqrt(distX * distX + distY * distY + distZ * distZ);
    };

    bool IgnoredSyncNpc(zCVob* npc) {
        auto name = npc->GetObjectName();

        for (unsigned int i = 0; i < IgnoredSyncNpcsCount; i++)
        {
            if (strcmp(name.ToChar(), IgnoredSyncNpcs[i]) == 0)
                return true;
        }

        return false;
    }

    void BuildGlobalNpcList() {
        PluginState = "BuildGlobalNpcList";

        auto* list = CoopWorldNpcListFirst(ogame->GetGameWorld());
        auto firstRun = NpcToFirstRoutineWp.size() == 0;
        int diagRtnEntries = 0;
        int diagWorldNpcs = 0;

        if (firstRun) {
            auto* rtnList = rtnMan->rtnList.next;
            while (rtnList) {
                auto rtn = rtnList->data;
                diagRtnEntries++;
                if (NpcToFirstRoutineWp.count(rtn->npc) == 0) {
                    NpcToFirstRoutineWp[rtn->npc] = rtn->wpname.ToChar();
                }
                rtnList = rtnList->next;
            }

            while (list) {
                auto npc = list->data;
                NamesCounter[string(npc->GetObjectName())] += 1;
                list = list->next;
            }
        }

        list = CoopWorldNpcListFirst(ogame->GetGameWorld());
        while (list) {
            auto npc = list->data;
            diagWorldNpcs++;

            if (npc->IsAPlayer() || IsCoopPlayer(npc->GetObjectName()) || IgnoredSyncNpc(npc) || NpcToUniqueNameList.count(npc)) {
                list = list->next;
                continue;
            }

            auto objectName = string(npc->GetObjectName());
            CStringA name = "";
            if (NamesCounter[objectName] == 1) {
                name = objectName;
            }
            else {
                auto secondUniquePart = npc->wpname ? string(npc->wpname) : string("UNKNOW");
                if (NpcToFirstRoutineWp.count(npc) > 0 && !NpcToFirstRoutineWp[npc].IsEmpty()) {
                    secondUniquePart = NpcToFirstRoutineWp[npc];
                }
                if (!firstRun) {
                    secondUniquePart = npc->wpname ? string::Combine("DYNAMIC-%s", string(npc->wpname)) : string("DYNAMIC");
                }
                name = string::Combine("%s-%s", string(npc->GetObjectName()), secondUniquePart);
                NamesCounter[name] += 1;
            }

            auto uniqueName = string::Combine("%s-%i", name, NamesCounter[name]);
            UniqueNameToNpcList[uniqueName] = npc;
            NpcToUniqueNameList[npc] = uniqueName;
            CoopDiagNameAssigned(uniqueName, npc, firstRun);

            list = list->next;
        }

        CoopDiagListBuilt(firstRun, diagWorldNpcs, diagRtnEntries);
    }

    long long GetCurrentMs() {
        std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
            );

        return ms.count();
    }

    static int GetPartyMemberID() {
        zCPar_Symbol* sym = parser->GetSymbol("AIV_PARTYMEMBER");
        if (!sym)
#if ENGINE >= Engine_G2
            return 15;
#else
            return 36;
#endif
        int id;
        sym->GetValue(id, 0);
        return id;
    }

    static int GetFriendDefaultInstanceId() {
        auto instId = parser->GetIndex(FriendInstanceId);

        return instId;
    }
}