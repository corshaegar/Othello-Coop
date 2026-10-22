namespace GOTHIC_ENGINE {
    unsigned long long CoopSteamClientHostId();   // v87: CoopSteam.cpp - 0 = Verbindung ueber IP
    int CoopSteamClientLocalPort();
    std::string CoopSteamTargetText();
    void CoopIntroSkipPeek(const ENetEvent& ev);   // v91: CoopNewGame.cpp - Intro-Abbruch des Partners schon im Netzfaden

    int CoopClientThread()
    {
        CoopClientState = 1;   // v39: verbindet
        static bool enetExitRegistered = false;
        if (enet_initialize() != 0)
        {
            ChatLog("An error occurred while initializing ENet.");
            CoopClientState = 3;
            return EXIT_FAILURE;
        }
        if (!enetExitRegistered) {   // v39: nur einmal - der Faden kann jetzt mehrmals gestartet werden
            enetExitRegistered = true;
            atexit(enet_deinitialize);
        }

        ENetHost* client;
        client = enet_host_create(NULL, 1, 2, 0, 0);
        if (client == NULL)
        {
            ChatLog("An error occurred while trying to create an ENet client host.");
            CoopClientState = 3;
            return EXIT_FAILURE;
        }

        // v87: Steam - ENet verbindet sich mit dem lokalen Steam-Tunnel (CoopSteam.cpp, vom Hauptfaden vor dem Start gestartet);
        // ein Steam-P2P-Paket (unzuverlaessig) fasst hoechstens 1200 Byte - der Host uebernimmt die MTU des Clients je Verbindung.
        // v88: auch die MTU der Peers - enet_host_create setzt sie schon beim Anlegen (enet_peer_reset, peer.c 415: peer->mtu =
        // host->mtu = 1400), enet_host_connect schickt peer->mtu (host.c 238). Test v87: 432 bzw. 52 Pakete mit 1400 Byte.
        const bool steam = CoopSteamClientHostId() != 0;
        if (steam) {
            client->mtu = 1200;
            for (size_t i = 0; i < client->peerCount; i++)
                client->peers[i].mtu = 1200;
        }
        ENetAddress address;
        ENetEvent event;
        ENetPeer* peer;
        auto serverIp = steam ? std::string("127.0.0.1") : CoopServerIp;   // v55: aus dem COOP-Menue (Gothic.ini coopServer), vom Hauptfaden vor dem Start gesetzt
        const std::string target = steam ? CoopSteamTargetText() : serverIp;
        enet_address_set_host(&address, serverIp.c_str());
        address.port = steam ? CoopSteamClientLocalPort() : ConnectionPort;
        peer = enet_host_connect(client, &address, 2, 0);
        if (peer == NULL)
        {
            ChatLog("No available peers for initiating an ENet connection.");
            enet_host_destroy(client);
            CoopClientState = 3;
            return EXIT_FAILURE;
        }

        if (
            enet_host_service(client, &event, steam ? 15000 : 5000) > 0   // v87: Steam braucht fuer NAT/Relay oft einige Sekunden
            && event.type == ENET_EVENT_TYPE_CONNECT)
        {
            // v58: Fassung statt GTs Protokollnummer (COOP_VERSION) anzeigen
            if (steam)
                ChatLog(string::Combine(CoopTr("Verbunden \xFC" "ber Steam mit %s (%s).", "Connected via Steam to %s (%s)."), string(target.c_str()), string(CoopVersionName().c_str())));
            else
                ChatLog(string::Combine(CoopTr("Verbunden mit %s (%s, Port %i).", "Connected to %s (%s, port %i)."), string(serverIp.c_str()), string(CoopVersionName().c_str()), address.port));
        }
        else
        {
            // v39: vorher lief der Faden ohne Verbindung endlos weiter, und die Verbinden-Taste schaltete nur noch die Pause um
            // ("verbunden, obwohl der Server nicht an war" - nur Neustart half). Jetzt beenden; Shift+F2 versucht es erneut.
            enet_peer_reset(peer);
            enet_host_destroy(client);
            if (steam)
                ChatLog(string::Combine(CoopTr("Verbindung \xFC" "ber Steam mit %s fehlgeschlagen (%s).", "Connection via Steam to %s failed (%s)."), string(target.c_str()), string(CoopVersionName().c_str())));
            else
                ChatLog(string::Combine(CoopTr("Verbindung zu %s fehlgeschlagen (%s, Port %i).", "Connection to %s failed (%s, port %i)."), string(serverIp.c_str()), string(CoopVersionName().c_str()), address.port));
            CoopClientState = 3;
            return EXIT_FAILURE;
        }
        CoopClientState = 2;   // verbunden

        unsigned long ClientThreadErrors = 0;
        while (true) {
            if (CoopClientStop) {   // v39: Rolle wechseln / neu verbinden
                enet_peer_disconnect_now(peer, 0);
                enet_host_flush(client);
                enet_host_destroy(client);
                CoopClientState = 5;
                return 0;
            }
            try {
                if (CoopClientState == 4) {   // v39: getrennt - warten, bis der Hauptfaden das Ende bestaetigt (CoopClientStop)
                    Sleep(5);
                    continue;
                }
                // v52: Bis 1 ms auf das Netz warten, dann ALLE schon angekommenen Ereignisse abholen und ALLE wartenden Pakete
                // senden. Vorher je Durchlauf genau ein Ereignis und ein Paket - bei vielen Paketen (laufende NPCs, Kampf)
                // staute sich die Warteschlange, jedes weitere Paket kam spaeter an (Bewegung ruckelte, Treffer verzoegert).
                ENetEvent event;
                auto eventStatus = enet_host_service(client, &event, 1);
                bool hostGone = false;
                LONG received = 0;
                while (eventStatus > 0) {
                    if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
                        // v39: Host weg. Den ENet-Host erst nach der Bestaetigung zerstoeren - in der Warteschlange koennen
                        // noch Ereignisse mit Zeigern auf diesen Peer stehen (ProcessClientPacket liest peer->roundTripTime).
                        CoopClientState = 4;
                        hostGone = true;
                        break;
                    }
                    CoopIntroSkipPeek(event);   // v91: der Hauptfaden steht waehrend des Intros im Video
                    ReadyToBeReceivedPackets.enqueue(event);
                    received++;
                    eventStatus = enet_host_check_events(client, &event);
                }
                CoopNetMax(CoopNetRecvBatchMax, received);
                if (hostGone)
                    continue;

                CoopNetMax(CoopNetSendQueueMax, (LONG)ReadyToSendJsons.size());
                LONG sent = 0;
                while (sent < 512 && !ReadyToSendJsons.isEmpty()) {
                    auto rawJson = ReadyToSendJsons.dequeue();
                    auto bjson = json::to_bson(rawJson);

                    ENetPacket* packet = enet_packet_create(&bjson[0], bjson.size(), PacketFlag(rawJson));
                    enet_peer_send(peer, PacketChannel(rawJson), packet);
                    sent++;
                }
                if (sent > 0)
                    enet_host_flush(client);   // gleich hinaus, nicht erst beim naechsten enet_host_service
                CoopNetMax(CoopNetSendBatchMax, sent);
                CoopNetTotals(client);
            }
            // Siehe Server.cpp: ein gescheiterter Durchlauf darf den Faden nicht beenden.
            catch (std::exception& e) {
                ClientThreadErrors++;
                CoopErrorLog("Client-Faden", e.what(), "Pakete senden/empfangen");
                continue;
            }
            catch (...) {
                ClientThreadErrors++;
                CoopErrorLog("Client-Faden", "Zugriffsfehler (SEH) oder unbekannte Ausnahme", "Pakete senden/empfangen");
                continue;
            }
        }
    }
}
