namespace GOTHIC_ENGINE {
    bool CoopSteamAvailable();   // v88: CoopSteam.cpp - Steam angemeldet (der Host ist dann auch ueber Steam erreichbar)
    void CoopIntroSkipPeek(const ENetEvent& ev);   // v91: CoopNewGame.cpp - Intro-Abbruch des Partners schon im Netzfaden

    // v71: Spielstand-Dateien (FILE_REQ/PART/END) und gezielte Ladeauftraege tragen "to" - nur an diesen Empfaenger senden.
    // Bis v70 gingen sie an alle Verbundenen, Client-Dateien wurden zusaetzlich an die anderen Clients weitergeleitet (Test v69:
    // 8,6 MB fuer FRIEND_1 liefen auch zu jedem, der sich waehrenddessen verband).
    static bool CoopTargetedPacket(const json& j, std::string& to) {
        if (!j.contains("to") || !j["to"].is_string() || !j.contains("type") || !j["type"].is_number())
            return false;
        const int t = j["type"].get<int>();
        if (t != SYNC_FILE_REQ && t != SYNC_FILE_PART && t != SYNC_FILE_END && t != SYNC_LOAD_REQ && t != SYNC_NPC_LEAD)   // v72: NPC_LEAD
            return false;
        to = j["to"].get<std::string>();
        return true;
    }

    static void CoopSendToPeer(ENetHost* server, const std::string& to, ENetPacket* packet, int channel) {
        for (size_t i = 0; i < server->peerCount; i++) {
            ENetPeer* peer = &server->peers[i];
            PeerData* pd = (PeerData*)peer->data;
            if (pd && pd->name.Compare(to.c_str())) {
                if (enet_peer_send(peer, channel, packet) == 0)
                    return;
                break;
            }
        }
        enet_packet_destroy(packet);   // Empfaenger nicht (mehr) verbunden
    }

    int CoopServerThread()
    {
        static bool enetExitRegistered = false;
        if (enet_initialize() != 0)
        {
            ChatLog("An error occurred while initializing ENet.");
            CoopServerState = 5;
            return EXIT_FAILURE;
        }
        if (!enetExitRegistered) {   // v39: nur einmal - der Faden kann jetzt mehrmals gestartet werden
            enetExitRegistered = true;
            atexit(enet_deinitialize);
        }

        ENetAddress address;
        ENetHost* server;
        enet_address_set_host(&address, "0.0.0.0");
        address.port = ConnectionPort;
        server = enet_host_create(&address, 32, 2, 0, 0);
        if (server == NULL)
        {
            ChatLog("An error occurred while trying to create an ENet server host.");
            CoopServerState = 5;
            return EXIT_FAILURE;
        }

        CoopServerState = 1;   // v39: laeuft
        // v58: Fassung statt COOP_VERSION. v88: mit Steam nimmt der Host beides an - Test v87: "Oben stand immer noch etwas mit
        // Port, sollte da nicht Steam stehen?"
        if (CoopSteamAvailable())
            ChatLog(string::Combine(CoopTr("(Server) Bereit (%s) - Steam-Freunde treten \xFC" "ber Steam bei, IP-Mitspieler \xFC" "ber Port %i.", "(Server) Ready (%s) - Steam friends join via Steam, IP players via port %i."),
                string(CoopVersionName().c_str()), address.port));
        else
            ChatLog(string::Combine(CoopTr("(Server) Bereit (%s, Port %i).", "(Server) Ready (%s, port %i)."), string(CoopVersionName().c_str()), address.port));
        unsigned long ServerThreadErrors = 0;
        while (true) {
            if (CoopServerStop) {   // v39: Rolle wechseln (nur ohne verbundene Spieler, siehe Plugin.cpp)
                enet_host_flush(server);
                enet_host_destroy(server);
                CoopServerState = 5;
                return 0;
            }
            try {
                // v52: wie Client.cpp - alle angekommenen Ereignisse abholen, alles Wartende weiterleiten und senden (vorher je
                // Durchlauf eines; unter Last staute es sich).
                ENetEvent event;
                auto eventStatus = enet_host_service(server, &event, 1);
                LONG received = 0;
                while (eventStatus > 0) {
                    CoopIntroSkipPeek(event);   // v91: der Hauptfaden steht waehrend des Intros im Video
                    ReadyToBeReceivedPackets.enqueue(event);
                    received++;
                    eventStatus = enet_host_check_events(server, &event);
                }
                CoopNetMax(CoopNetRecvBatchMax, received);

                LONG sent = 0;
                while (sent < 512 && !ReadyToBeDistributedPackets.isEmpty()) {
                    sent++;
                    auto jsonPacket = ReadyToBeDistributedPackets.dequeue();
                    auto playerId = jsonPacket["id"].get<std::string>();
                    std::string to;
                    const bool targeted = CoopTargetedPacket(jsonPacket, to);
                    if (targeted && to == "HOST")
                        continue;   // v71: an den Host gerichtet - nicht an die anderen Clients weiterleiten
                    // v72: Rueckgabe eines NPCs ("back") geht nur den Host an; Pakete eines vom Client gefuehrten NPCs (id = NPC)
                    // gehen an alle ausser an den Besitzer ("lb")
                    if (jsonPacket.contains("type") && jsonPacket["type"].is_number() && jsonPacket["type"].get<int>() == SYNC_NPC_LEAD)
                        continue;
                    const std::string leadBy = jsonPacket.contains("lb") && jsonPacket["lb"].is_string() ? jsonPacket["lb"].get<std::string>() : std::string();

                    for (int i = 0; i < server->peerCount; i++) {
                        auto peer = &server->peers[i];
                        auto player = (PeerData*)peer->data;

                        if (!peer || !player) {
                            continue;
                        }

                        if (!player->name.Compare(playerId.c_str()) && (!targeted || player->name.Compare(to.c_str())) &&
                            (leadBy.empty() || !player->name.Compare(leadBy.c_str()))) {
                            auto bson = json::to_bson(jsonPacket);

                            ENetPacket* packet = enet_packet_create(&bson[0], bson.size(), PacketFlag(jsonPacket));
                            enet_peer_send(peer, PacketChannel(jsonPacket), packet);
                        }
                    }
                }

                CoopNetMax(CoopNetSendQueueMax, (LONG)ReadyToSendJsons.size());
                LONG own = 0;
                while (own < 512 && !ReadyToSendJsons.isEmpty()) {
                    auto rawJson = ReadyToSendJsons.dequeue();
                    auto bson = json::to_bson(rawJson);

                    ENetPacket* packet = enet_packet_create(&bson[0], bson.size(), PacketFlag(rawJson));
                    std::string to;
                    if (CoopTargetedPacket(rawJson, to))
                        CoopSendToPeer(server, to, packet, PacketChannel(rawJson));   // v71
                    else
                        enet_host_broadcast(server, PacketChannel(rawJson), packet);
                    own++;
                }
                sent += own;
                if (sent > 0)
                    enet_host_flush(server);   // gleich hinaus, nicht erst beim naechsten enet_host_service
                CoopNetMax(CoopNetSendBatchMax, sent);
                CoopNetTotals(server);
            }
            // Frueher stand hier "return EXIT_FAILURE": eine einzige Ausnahme beendete den
            // Server-Faden endgueltig, danach kam kein Paket mehr an. Gemessen unter Othello:
            // genau ein Zugriffsfehler, dann Stille. Ein Durchlauf darf scheitern, der Faden
            // nicht. Kein Message::Error - das oeffnet einen Dialog aus einem Nebenfaden.
            catch (std::exception& e) {
                ServerThreadErrors++;
                CoopErrorLog("Server-Faden", e.what(), "Pakete senden/empfangen");
                continue;
            }
            catch (...) {
                ServerThreadErrors++;
                CoopErrorLog("Server-Faden", "Zugriffsfehler (SEH) oder unbekannte Ausnahme", "Pakete senden/empfangen");
                continue;
            }
        }
    }
}
