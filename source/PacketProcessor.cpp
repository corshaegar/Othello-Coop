namespace GOTHIC_ENGINE {
    void CoopLobbyReceiveInGame(json& e, const std::string& id);   // v40: CoopMenuLobby.cpp
    void CoopDiffReceive(json& e, const std::string& id);          // v67: CoopDifficulty.cpp

    void ProcessCoopPacket(json e, ENetEvent packet) {
        auto id = e["id"].get<std::string>();
        auto type = e["type"].get<int>();
        CoopTraceCount(TRACE_RECV, type);
        auto npcToSync = SyncNpcs.count(id.c_str()) ? SyncNpcs[id.c_str()] : NULL;

        if (type == SYNC_PLAYER_NAME) {
            auto connectId = e["connectId"].get<enet_uint32>();

            if (connectId == packet.peer->connectID) {
                std::string name = e["name"].get<std::string>();
                MyselfId = name.c_str();
            }

            return;
        }

        if (type == PLAYER_DISCONNECT) {
            string name = e["name"].get<std::string>().c_str();
            ChatLog(string::Combine(CoopTr("%s hat die Verbindung getrennt.", "%s disconnected."), name));

            CoopWorldSyncPeerGone(std::string(name.ToChar()));
            CoopDialogSyncPeerGone(std::string(name.ToChar()));
            removeSyncedNpc(name);
            return;
        }

        if (type == SYNC_LOBBY) {   // v40: Wartephase im Hauptmenue - im Spiel nur "Host spielt schon" beantworten, nie eine Figur anlegen
            CoopLobbyReceiveInGame(e, id);
            return;
        }
        if (type == SYNC_NPC_LEAD) {   // v72: Kampf-Uebergabe (CoopCombatLead.cpp)
            CoopLeadReceive(e, id);
            return;
        }
        if (CoopLeadDropPacket(id)) {   // v72: Client fuehrt diesen NPC selbst - spaete Pakete des Hosts nicht anwenden
            CoopTraceCount(TRACE_SKIPPED, type);
            return;
        }
        if (type == SYNC_COOP_RULES) {   // v67: Schwierigkeitsgrad des Hosts - auch im Hauptmenue und waehrend einer Pause
            CoopDiffReceive(e, id);
            return;
        }

        if (CoopLogShipHandles(type)) {   // v34/v35: Coop-Log des Partners - auch waehrend "Stop world synchronization"
            CoopLogShipReceive(e, id);
            return;
        }
        if (CoopSelfTestHandles(type)) {   // v37: Coop-Testlauf (Frage/Antwort)
            CoopSelfTestReceive(e, id);
            return;
        }
        if (type == SYNC_RESYNC) {   // v39: Client bittet um den vollen Stand (nach Fortsetzen/Laden)
            CoopResyncReceive(e, id);
            return;
        }
        // v17: gemeinsames Speichern/Laden. v76: auch waehrend der Pause - der Client antwortet dann "pausiert" (die Auto-Sicherung
        // des Hosts haengt nicht), und beim Fortsetzen kommen Ladeauftrag und Dateien durch; alles andere uebergeht er dort selbst.
        if (CoopSaveSyncHandles(type)) {
            CoopSaveSyncReceive(e, id);
            return;
        }

        if (IsCoopPaused) {
            return;
        }

        // Truhen/Gegenstaende gehoeren zu keinem NPC: vor der NPC-Zuordnung abzweigen (sonst legte
        // ein erstes Weltpaket eines Spielers dessen Figur an und loeste Myself->Reinit aus).
        if (CoopWorldSyncHandles(type)) {
            CoopWorldSyncReceive(e, id);
            return;
        }
        if (type == SYNC_NPC_DEATH) {   // v41: Todesmeldung des Hosts (auch ausserhalb der Reichweite)
            CoopFarSyncReceive(e, id);
            return;
        }
        if (CoopDialogSyncHandles(type)) {   // v26: Dialoge (Sperre, Zeilen)
            CoopDialogSyncReceive(e, id);
            return;
        }
        if (type == SYNC_SPELL_CAST) {
            //ChatLog(e.dump().c_str());
        }

        if (npcToSync == NULL) {
            if (IsCoopPlayer(id)) {
                npcToSync = addSyncedNpc(id.c_str());
                if (Myself) {
                    Myself->Reinit();
                }
            }
            else if (UniqueNameToNpcList.count(id.c_str())) {
                npcToSync = addSyncedNpc(id.c_str());
            }
        }
        CoopDiagPacket(id, IsCoopPlayer(id), npcToSync != NULL);

        if (npcToSync) {
            npcToSync->localUpdates.push_back(e);
        }
    }

    void ProcessServerPacket(ENetEvent packet) {
        switch (packet.type) {
        case ENET_EVENT_TYPE_CONNECT:
        {
            auto playerName = string::Combine("FRIEND_%i", GetFreePlayerId());
            ChatLog(string::Combine(CoopTr("(Server) Neue Verbindung: %s", "(Server) We got a new connection %s"), string(playerName)));
#if ENGINE == Engine_G1
            // v71: auch ins Coop-Log (Test v69: Trennen und Neuverbinden im Spiel standen nur im Chat)
            OthelloCompatLog("[%lld s, %s] Netz: %s hat sich verbunden (Host im Spiel, Ping %u ms)", DiagSeconds(), DiagRole(),
                playerName.ToChar(), (unsigned)packet.peer->roundTripTime);
#endif

            json j;
            j["id"] = "HOST";
            j["type"] = SYNC_PLAYER_NAME;
            j["name"] = string(playerName).ToChar();
            j["connectId"] = packet.peer->connectID;
            ReadyToSendJsons.enqueue(j);

            addSyncedNpc(playerName);
            CoopSaveSyncPeerJoined();

            auto d = new PeerData();
            d->name = playerName;
            packet.peer->data = d;

            if (Myself) {
                Myself->Reinit();
            }
            for each (auto n in BroadcastNpcs)
            {
                n.second->Reinit();
            }
            break;
        }
        case ENET_EVENT_TYPE_RECEIVE:
        {
            auto player = (PeerData*)packet.peer->data;
            if (player == NULL) {   // Peer ohne zugeordnete Daten: Paket verwerfen statt abstuerzen
                break;
            }
            auto dataLenght = packet.packet->dataLength;
            const char* data = (const char*)packet.packet->data;

            std::vector<std::uint8_t> bytesVector;
            for (int i = 0; i < dataLenght; i++) {
                bytesVector.push_back(data[i]);
            }

            auto j = json::from_bson(bytesVector);
            j["id"] = player->name.ToChar();
            // v72: Paket eines vom Client gefuehrten NPCs - nur vom Besitzer annehmen, dann mit id = NPC (CoopCombatLead.cpp)
            if (!CoopLeadAcceptPacket(j, std::string(player->name.ToChar())))
                break;

            ReadyToBeDistributedPackets.enqueue(j);
            ProcessCoopPacket(j, packet);
            SaveNetworkPacket(j.dump(-1, ' ', false, json::error_handler_t::ignore).c_str());
            break;
        }
        case ENET_EVENT_TYPE_DISCONNECT:
        {
            auto remoteNpc = (PeerData*)(packet.peer->data);
            if (remoteNpc == NULL) {   // Abbruch, bevor dem Peer Daten zugeordnet waren
                break;
            }
            ChatLog(string::Combine(CoopTr("%s hat die Verbindung getrennt.", "%s disconnected."), remoteNpc->name));
#if ENGINE == Engine_G1
            OthelloCompatLog("[%lld s, %s] Netz: %s hat die Verbindung getrennt (Host im Spiel)", DiagSeconds(), DiagRole(), remoteNpc->name.ToChar());   // v71
#endif
            CoopSaveSyncPeerGone(std::string(remoteNpc->name.ToChar()));   // v71: ausstehende Dateistuecke, Ladeauftrag
            CoopLeadPeerGone(std::string(remoteNpc->name.ToChar()));       // v72: seine NPCs fuehrt wieder der Host

            json j;
            j["id"] = "HOST";
            j["type"] = PLAYER_DISCONNECT;
            j["name"] = string(remoteNpc->name).ToChar();
            ReadyToSendJsons.enqueue(j);

            CoopWorldSyncPeerGone(std::string(remoteNpc->name.ToChar()));
            CoopDialogSyncPeerGone(std::string(remoteNpc->name.ToChar()));
            CoopSaveSyncPeerLeft();
            removeSyncedNpc(remoteNpc->name);
            packet.peer->data = NULL;
            break;
        }
        }
    }

    void ProcessClientPacket(ENetEvent packet) {
        switch (packet.type) {
        case ENET_EVENT_TYPE_RECEIVE:
        {
            auto dataLenght = packet.packet->dataLength;
            const char* data = (const char*)packet.packet->data;
            std::vector<std::uint8_t> bytesVector;
            for (int i = 0; i < dataLenght; i++) {
                bytesVector.push_back(data[i]);
            }

            auto j = json::from_bson(bytesVector);
            ProcessCoopPacket(j, packet);
            SaveNetworkPacket(j.dump(-1, ' ', false, json::error_handler_t::ignore).c_str());

            CurrentPing = packet.peer->roundTripTime;
            enet_packet_destroy(packet.packet);
            break;
        }
        case ENET_EVENT_TYPE_DISCONNECT:
        {
            ChatLog(CoopTr("Verbindung zum Host verloren.", "Connection to the server lost."));
            break;
        }
        }
    }

    void PacketProcessorLoop() {
        PluginState = "PacketProcessorLoop";
        if (ReadyToBeReceivedPackets.isEmpty()) {
            return;
        }
        const unsigned long waiting = (unsigned long)ReadyToBeReceivedPackets.size();   // v52: Messung (CoopDiag.cpp)
        if (waiting > CoopNetRecvQueueMax)
            CoopNetRecvQueueMax = waiting;

        while (!ReadyToBeReceivedPackets.isEmpty()) {
            auto packet = ReadyToBeReceivedPackets.dequeue();

            // v39: Faden endet (getrennt, Rollenwechsel) - Ereignisse verwerfen, ohne den Peer anzufassen
            const bool ending = (ClientThread && !ServerThread && (CoopClientState >= 3 || CoopClientStop))
                || (ServerThread && (CoopServerStop || CoopServerState == 5));
            if (ending) {
                if (packet.type == ENET_EVENT_TYPE_RECEIVE && packet.packet)
                    enet_packet_destroy(packet.packet);
                continue;
            }

            if (ServerThread) {
                ProcessServerPacket(packet);
            }
            else if (ClientThread)
            {
                ProcessClientPacket(packet);
            }
        }
    }
}