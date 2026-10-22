namespace GOTHIC_ENGINE {
    const int COOP_VERSION = 60;
    const int COOP_MAGIC_NUMBER = 1337;
    int BROADCAST_DISTANCE = 4500;

    DWORD MainThreadId;
    std::string PluginState = "";
    Chat* GameChat = new Chat();

    char GothicExeFolderPath[MAX_PATH];
    std::string GothicCoopLogPath;

    string MyselfId = "_player_";
    static LocalNpc* Myself = NULL;
    int LastFreePlayerId = 0;
    long long CurrentMs = 0;
    json CoopConfig;
    static bool IsLoadingLevel = false;
    static bool IsSavingGame = false;
    static bool IsCoopPaused = false;
    long long LastUpdateListOfVisibleNpcs = 0;
    long long LastNpcListRefreshTime = 0;
    zVEC3* CurrentWorldTOTPosition;
    int CurrentPing = -1;

    string FriendInstanceId = "ch";
    string MyNickname = "";
    bool WorldEditMode = false;

    int PlayersDamageMultipler = 50;
    int NpcsDamageMultipler = 100;
    int ToggleGameLogKey;
    int ToggleGameStatsKey;
    int StartServerKey;
    int StartConnectionKey;
    int ReinitPlayersKey;
    int RevivePlayerKey;

#if ENGINE >= Engine_G2
    int MyBodyTextVarNr = 9;
    int MyHeadVarNr = 18;
    int DefaultBodyTexColorNr = 0;
#else
    int MyBodyTextVarNr = 4;
    int MyHeadVarNr = 9;
    int DefaultBodyTexColorNr = 1;
#endif

    int ConnectionPort = 1234;

    static Thread* ServerThread = NULL;
    static Thread* ClientThread = NULL;

    // v39: Zustand der Netz-Faeden (Client: 0 aus, 1 verbindet, 2 verbunden, 3 fehlgeschlagen, 4 getrennt, 5 beendet;
    // Server: 0 aus, 1 laeuft, 5 beendet) und Stoppwunsch - damit Verbinden ohne Neustart wiederholt und die Rolle
    // gewechselt werden kann (Plugin.cpp, CoopNetPulse).
    static volatile LONG CoopClientState = 0;
    static volatile LONG CoopServerState = 0;
    static volatile LONG CoopClientStop = 0;
    static volatile LONG CoopServerStop = 0;

    // v52: Netz-Messung. Der Netzfaden schreibt die ENet-Summen (ENetHost::totalSentData/-ReceivedData/-SentPackets/
    // -ReceivedPackets) und die groessten Werte seit der letzten Ausgabe, der Hauptfaden liest sie alle 30 s (CoopDiag.cpp).
    static volatile LONG CoopNetSentBytes = 0;
    static volatile LONG CoopNetRecvBytes = 0;
    static volatile LONG CoopNetSentPkts = 0;
    static volatile LONG CoopNetRecvPkts = 0;
    static volatile LONG CoopNetSendQueueMax = 0;   // Pakete in ReadyToSendJsons vor dem Senden
    static volatile LONG CoopNetSendBatchMax = 0;   // Pakete in einem Durchlauf gesendet
    static volatile LONG CoopNetRecvBatchMax = 0;   // Ereignisse in einem Durchlauf abgeholt

    // v52: Glaetten beim Empfaenger (RemoteNpc.cpp, nur Hauptfaden): Bilder mit geglaetteter Lage, harte Spruenge
    static unsigned long CoopSmoothSteps = 0;
    static unsigned long CoopSmoothSnaps = 0;
    static unsigned long CoopNetRecvQueueMax = 0;   // v52: empfangene Ereignisse, die auf den Hauptfaden warteten (je Bild)

    static void CoopNetMax(volatile LONG& slot, LONG value) {
        LONG cur = slot;
        while (value > cur) {
            const LONG prev = InterlockedCompareExchange(&slot, value, cur);
            if (prev == cur)
                break;
            cur = prev;
        }
    }

    // v71: Zustand der Verbindungen seit der letzten Ausgabe (Test v69: 8,6 MB liefen einmal mit gut 100 KB/s, spaeter mit
    // 1-4 MB/s - ohne diese Werte war nicht zu sagen, ob Ping, Verlust oder ENets Drosselung schuld war)
    static volatile LONG CoopNetRttMax = 0;          // ms, groesster geglaetteter Ping
    static volatile LONG CoopNetThrottleMin = 32;    // ENet-Drossel (32 = offen), kleinster Wert
    static volatile LONG CoopNetLossMax = 0;         // Paketverlust in 1/65536
    static volatile LONG CoopNetInTransitMax = 0;    // Bytes zuverlaessig unterwegs (ENet-Fenster)
    // v76: ms seit dem letzten Empfang von der Gegenseite (ENet lastReceiveTime - auch Pings, alle 0,5 s, auch waehrend ein
    // Ladebildschirm den Hauptfaden aufhaelt). Beim Client = Stille vom Host (CoopSaveSync.cpp, Verbindungsstoerung ab 5 s).
    static volatile LONG CoopNetSilenceMs = 0;

    static void CoopNetTotals(const ENetHost* host) {
        if (!host)
            return;
        InterlockedExchange(&CoopNetSentBytes, (LONG)host->totalSentData);
        InterlockedExchange(&CoopNetRecvBytes, (LONG)host->totalReceivedData);
        InterlockedExchange(&CoopNetSentPkts, (LONG)host->totalSentPackets);
        InterlockedExchange(&CoopNetRecvPkts, (LONG)host->totalReceivedPackets);
        LONG silence = 0;
        for (size_t i = 0; i < host->peerCount; i++) {
            const ENetPeer& p = host->peers[i];
            if (p.state != ENET_PEER_STATE_CONNECTED)
                continue;
            const enet_uint32 a = host->serviceTime, b = p.lastReceiveTime;   // wie ENET_TIME_DIFFERENCE (enet/time.h)
            const LONG s = (LONG)(a - b >= 86400000u ? b - a : a - b);
            if (s > silence)
                silence = s;
            CoopNetMax(CoopNetRttMax, (LONG)p.roundTripTime);
            CoopNetMax(CoopNetLossMax, (LONG)p.packetLoss);
            CoopNetMax(CoopNetInTransitMax, (LONG)p.reliableDataInTransit);
            LONG cur = CoopNetThrottleMin;
            while ((LONG)p.packetThrottle < cur) {
                const LONG prev = InterlockedCompareExchange(&CoopNetThrottleMin, (LONG)p.packetThrottle, cur);
                if (prev == cur)
                    break;
                cur = prev;
            }
        }
        InterlockedExchange(&CoopNetSilenceMs, silence);
    }

    std::map<string, LocalNpc*> BroadcastNpcs;
    std::map<string, RemoteNpc*> SyncNpcs;

    static std::map<string, oCNpc*> UniqueNameToNpcList;
    static std::map<oCNpc*, string> NpcToUniqueNameList;

    static std::map<string, int> NamesCounter;
    static std::map<oCNpc*, string> NpcToFirstRoutineWp;

    static std::map<oCNpc*, string> PlayerNpcs;
    static std::map<string, oCNpc*> PlayerNameToNpc;

    static std::map<string, oCNpc*> KilledByPlayerNpcNames;

    static SafeQueue<json> ReadyToSendJsons;
    static SafeQueue<json> ReadyToBeDistributedPackets;
    static SafeQueue<ENetEvent> ReadyToBeReceivedPackets;
    static SafeQueue<PlayerHit> ReadyToSyncDamages;
    static SafeQueue<SpellCast> ReadyToSyncSpellCasts;

    static const int IgnoredSyncNpcsCount = 9;
    static const char IgnoredSyncNpcs[IgnoredSyncNpcsCount][30] =
    {
        {"AD_OLDGHOSTRIDDLE1AD_LAST"},
        {"AD_OLDGHOSTRIDDLE2AD_LAST"},
        {"AD_OLDGHOSTRIDDLE3AD_LAST"},
        {"AD_OLDGHOSTRIDDLE4AD_LAST"},
        {"AD_OLDGHOSTRIDDLE5AD_LAST"},
        {"AD_OLDGHOSTRIDDLE6AD_LAST"},
        {"AD_OLDGHOSTRIDDLE7AD_LAST"},
        {"AD_OLDGHOSTRIDDLE8AD_LAST"},
        {"WISP_DETECTOR"},
    };
}