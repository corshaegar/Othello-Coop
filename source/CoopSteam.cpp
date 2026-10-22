// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

    // --- v87: Coop ueber Steam (22.10.2026) ---------------------------------------------------------------------------
    //
    // Nutzer: "Implementiere eine Steam Integration aehnlich wie bei Gothic Together. Es soll also eine Verbindung ueber Steam im
    // Spiel moeglich sein." Plan: gt-fix\PLAN-STEAM.md. Allgemein G1 (GT + Gothic 1 auf Steam, App 65540).
    //
    // Geprueft: GTs Release-DLL (nicht unser AGPL-Quelltext) nutzt Steam-Lobbys, ISteamNetworkingSockets, einen Launcher und einen
    // Web-Dienst - davon wird nichts uebernommen. Im Spielordner liegen system\steam_api.dll (Steam Client API 08.63.11.84, flache
    // C-Exporte, SteamAPI_InitFlat) und steam_appid.txt = 65540. zParserExtender laedt steam_api.dll schon (Skriptfunktion
    // Hlp_GetSteamPersonalName) - darum kein SteamAPI_Shutdown und kein ManualDispatch; Rueckrufe nur im Hauptfaden
    // (SteamAPI_RunCallbacks je Bild aus Game_Loop und Game_MenuLoop).
    //
    // Weg: Steam-Tunnel unter ENet. GTs Netzcode bleibt, nur die Strecke zwischen den PCs laeuft ueber Steam-P2P (ISteamNetworking,
    // unzuverlaessig, hoechstens 1200 Byte je Paket; Steam macht NAT/Relay, keine Portfreigabe). ENet macht Zuverlaessigkeit und
    // Reihenfolge selbst; der Steam-Client setzt ENets MTU auf 1200, der Host uebernimmt sie je Verbindung (protocol.c 370-378).
    //  - Host: je Steam-Freund ein lokaler UDP-Socket -> 127.0.0.1:Port des ENet-Hosts (ENet sieht getrennte Mitspieler; Spieler
    //    erkennt GT am Namen, nicht an der Adresse). IP-Mitspieler koennen parallel beitreten. P2P-Anfragen nur von Steam-Freunden.
    //  - Client: lokaler UDP-Socket 127.0.0.1:Tunnel, ENet verbindet sich dorthin (Client.cpp), der Tunnel schickt alles an die
    //    SteamID des Hosts.
    //  - Rich Presence beim Hosten: connect "+gtcoop_join <SteamID>" (Steam-Freundesliste: "Spiel beitreten"/"Einladen"),
    //    gtcoop_host 1, gtcoop_ver (Fassung). Beitreten aus Steam (GameRichPresenceJoinRequested) bei laufendem Spiel: im
    //    Hauptmenue wie "Spiel beitreten", im Spiel wie Shift+F2.
    //  - Menue MENU_COOP_STEAM (COOP_MENU-v87.D): Freunde, die Gothic spielen - hostet -> beitreten, sonst einladen (wenn ich
    //    hoste); erreichbar ueber COOP -> Steam und Einstellungen -> COOP -> Steam-Freunde (auch im Spiel).
    // Gothic.ini [GOTHICCOOP] coopSteam=0 schaltet Steam ab (IP-Coop bleibt).

    void CoopSteamPulse();
    void CoopSteamMenuPulse();
    bool CoopSteamClientPrepare();
    unsigned long long CoopSteamClientHostId();
    int CoopSteamClientLocalPort();
    std::string CoopSteamTargetText();
    bool CoopSteamReconnect();
    void CoopSteamForgetLast();
    bool CoopSteamMenuCommand(const std::string& cmd, zCMenu* menu);
    bool CoopSteamAvailable();

#if ENGINE == Engine_G1

    // FriendGameInfo_t (isteamfriends.h): CGameID (App-ID in den unteren 24 Bit), IP, Ports, Lobby
    struct CoopSteamFriendGame {
        unsigned long long gameId;
        unsigned int gameIp;
        unsigned short gamePort;
        unsigned short queryPort;
        unsigned long long lobby;
    };

    // CCallbackBase der Steamworks-SDK (steam_api_common.h): gleiche Reihenfolge der virtuellen Funktionen und gleiche Felder -
    // MSVC legt die vtable daraus genauso an wie fuer steam_api.dll (Run(Param), Run(Param, IO-Fehler, Aufruf),
    // GetCallbackSizeBytes; Flags +4, Nummer +8). SteamAPI_RegisterCallback traegt Nummer und Flag "registriert" ein.
    class CoopSteamCallbackBase {
    public:
        CoopSteamCallbackBase() : m_nCallbackFlags(0), m_iCallback(0) {}
        virtual void Run(void* pvParam) = 0;
        virtual void Run(void* pvParam, bool bIOFailure, unsigned long long hSteamAPICall) = 0;
        int GetICallback() { return m_iCallback; }
        virtual int GetCallbackSizeBytes() = 0;
    protected:
        unsigned char m_nCallbackFlags;
        int m_iCallback;
    };

    class CoopSteamCallback : public CoopSteamCallbackBase {
    public:
        typedef void(*Handler)(void*);
        CoopSteamCallback(int id, int size, Handler handler) : size(size), handler(handler) { m_iCallback = id; }
        void Run(void* p) override { if (handler && p) handler(p); }
        void Run(void* p, bool, unsigned long long) override { if (handler && p) handler(p); }
        int GetCallbackSizeBytes() override { return size; }
    private:
        int size;
        Handler handler;
    };

    // Flache Exporte der steam_api.dll (S_CALLTYPE = __cdecl, uint64 SteamIDs als Wert)
    struct CoopSteamApi {
        HMODULE dll;
        int(__cdecl* InitFlat)(char* errMsg1024);   // ESteamAPIInitResult: 0 OK, 1 allgemein, 2 kein Steam, 3 Versionen
        void(__cdecl* RunCallbacks)();
        void(__cdecl* RegisterCallback)(void* callback, int id);
        void*(__cdecl* Friends)();
        void*(__cdecl* Networking)();
        void*(__cdecl* User)();
        void*(__cdecl* Utils)();
        unsigned long long(__cdecl* GetSteamID)(void* user);
        bool(__cdecl* LoggedOn)(void* user);
        unsigned int(__cdecl* GetAppID)(void* utils);
        const char*(__cdecl* GetPersonaName)(void* friends);
        int(__cdecl* GetFriendCount)(void* friends, int flags);
        unsigned long long(__cdecl* GetFriendByIndex)(void* friends, int index, int flags);
        bool(__cdecl* GetFriendGamePlayed)(void* friends, unsigned long long id, CoopSteamFriendGame* info);
        const char*(__cdecl* GetFriendPersonaName)(void* friends, unsigned long long id);
        bool(__cdecl* SetRichPresence)(void* friends, const char* key, const char* value);
        void(__cdecl* ClearRichPresence)(void* friends);
        const char*(__cdecl* GetFriendRichPresence)(void* friends, unsigned long long id, const char* key);
        bool(__cdecl* InviteUserToGame)(void* friends, unsigned long long id, const char* connect);
        bool(__cdecl* HasFriend)(void* friends, unsigned long long id, int flags);
        bool(__cdecl* SendP2PPacket)(void* net, unsigned long long to, const void* data, unsigned int size, int sendType, int channel);
        bool(__cdecl* IsP2PPacketAvailable)(void* net, unsigned int* size, int channel);
        bool(__cdecl* ReadP2PPacket)(void* net, void* dest, unsigned int destSize, unsigned int* size, unsigned long long* from, int channel);
        bool(__cdecl* AcceptP2PSessionWithUser)(void* net, unsigned long long id);
        bool(__cdecl* CloseP2PSessionWithUser)(void* net, unsigned long long id);
    };

    static const int STEAM_FRIENDS_IMMEDIATE = 0x04;        // k_EFriendFlagImmediate
    static const int STEAM_P2P_UNRELIABLE = 0;              // k_EP2PSendUnreliable (bis 1200 Byte)
    static const int STEAM_P2P_RELIABLE = 2;                // k_EP2PSendReliable (nur fuer zu grosse Pakete)
    static const unsigned int STEAM_P2P_MAX_UNRELIABLE = 1200;
    static const int STEAM_CHANNEL = 7;                     // eigener P2P-Kanal fuer den Tunnel
    static const int STEAM_ROWS = 6;                        // Zeilen in MENU_COOP_STEAM

    static CoopSteamApi SteamApi = {};
    static bool SteamTried = false;
    static bool SteamReady = false;
    static std::string SteamWhy = "noch nicht gestartet";
    static void* SteamFriends = NULL;
    static void* SteamNet = NULL;
    static void* SteamUserIf = NULL;
    static void* SteamUtilsIf = NULL;
    static unsigned long long SteamMyId = 0;
    static unsigned int SteamAppId = 0;
    static std::string SteamMyName;

    static void SteamLog(const char* fmt, ...) {
        char buf[700];
        va_list ap;
        va_start(ap, fmt);
        _vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        buf[sizeof(buf) - 1] = 0;
        OthelloCompatLog("[%lld s, %s] Steam: %s", DiagSeconds(), DiagRole(), buf);
    }

    // Steam liefert UTF-8, Gothics Schrift ist cp1252
    static std::string SteamText(const char* utf8) {
        if (!utf8 || !*utf8)
            return std::string();
        const int wn = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
        if (wn <= 0)
            return std::string(utf8);
        std::wstring w(wn, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8, -1, &w[0], wn);
        const int an = WideCharToMultiByte(1252, 0, w.c_str(), -1, NULL, 0, "?", NULL);
        if (an <= 0)
            return std::string(utf8);
        std::string a(an, '\0');
        WideCharToMultiByte(1252, 0, w.c_str(), -1, &a[0], an, "?", NULL);
        while (!a.empty() && a[a.size() - 1] == '\0')
            a.erase(a.size() - 1);
        return a;
    }

    static std::string SteamFriendName(unsigned long long id) {
        if (!SteamReady || !id)
            return std::string("?");
        if (id == SteamMyId)
            return SteamMyName;
        const std::string n = SteamText(SteamApi.GetFriendPersonaName(SteamFriends, id));
        return n.empty() ? std::to_string(id) : n;
    }

    // --- Tunnel ---------------------------------------------------------------------------------------------------------

    struct CoopSteamTunnelPeer {
        unsigned long long id;
        SOCKET s;
        DWORD lastMs;
        unsigned long long fromSteam, toSteam;
    };

    static volatile LONG TunnelMode = 0;   // 0 aus, 1 Host, 2 Client
    static volatile LONG TunnelStop = 0;
    static HANDLE TunnelThreadHandle = NULL;
    static unsigned long long TunnelHost = 0;     // Client: SteamID des Hosts
    static int TunnelHostPort = 0;                // Host: Port des ENet-Hosts
    static SOCKET TunnelClientSock = INVALID_SOCKET;
    static int TunnelClientPort = 0;              // Client: hierhin verbindet sich ENet
    static volatile LONG TunnelFromSteam = 0, TunnelToSteam = 0, TunnelOversize = 0, TunnelSendFail = 0, TunnelPeersNow = 0;
    static DWORD TunnelStatsMs = 0;

#ifndef SIO_UDP_CONNRESET
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
#endif

    // UDP-Socket auf 127.0.0.1 (Port 0 = beliebig), nicht blockierend, ohne WSAECONNRESET nach ICMP "Port nicht erreichbar"
    static SOCKET TunnelUdpSocket(int port, int& bound) {
        bound = 0;
        SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (s == INVALID_SOCKET)
            return s;
        sockaddr_in a = {};
        a.sin_family = AF_INET;
        a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        a.sin_port = htons((u_short)port);
        if (bind(s, (sockaddr*)&a, sizeof(a)) != 0) {
            closesocket(s);
            return INVALID_SOCKET;
        }
        u_long nb = 1;
        ioctlsocket(s, FIONBIO, &nb);
        BOOL off = FALSE;
        DWORD ret = 0;
        WSAIoctl(s, SIO_UDP_CONNRESET, &off, sizeof(off), NULL, 0, &ret, NULL, NULL);
        int size = 1 << 20;
        setsockopt(s, SOL_SOCKET, SO_RCVBUF, (const char*)&size, sizeof(size));
        setsockopt(s, SOL_SOCKET, SO_SNDBUF, (const char*)&size, sizeof(size));
        int len = sizeof(a);
        if (getsockname(s, (sockaddr*)&a, &len) == 0)
            bound = ntohs(a.sin_port);
        return s;
    }

    static void TunnelSend(unsigned long long to, const char* data, int size) {
        int type = STEAM_P2P_UNRELIABLE;
        if ((unsigned int)size > STEAM_P2P_MAX_UNRELIABLE) {   // sollte bei MTU 1200 nicht vorkommen
            type = STEAM_P2P_RELIABLE;
            if (InterlockedIncrement(&TunnelOversize) <= 5)
                SteamLog("Tunnel: Paket mit %d Byte - geht zuverl" "\xE4" "ssig statt unzuverl" "\xE4" "ssig", size);
        }
        if (SteamApi.SendP2PPacket(SteamNet, to, data, (unsigned int)size, type, STEAM_CHANNEL))
            InterlockedIncrement(&TunnelToSteam);
        else if (InterlockedIncrement(&TunnelSendFail) <= 5)
            SteamLog("Tunnel: Senden an %llu fehlgeschlagen (%d Byte)", to, size);
    }

    static DWORD WINAPI CoopSteamTunnelThread(LPVOID) {
        const LONG mode = TunnelMode;
        std::vector<CoopSteamTunnelPeer> peers;
        std::vector<char> buf(64 * 1024);
        sockaddr_in enetAddr = {};
        bool enetKnown = false;
        while (!TunnelStop) {
            bool busy = false;
            const DWORD now = GetTickCount();
            // Steam -> lokal
            unsigned int size = 0;
            for (int k = 0; k < 512 && SteamApi.IsP2PPacketAvailable(SteamNet, &size, STEAM_CHANNEL); k++) {
                if (size > buf.size())
                    buf.resize(size);
                unsigned int got = 0;
                unsigned long long from = 0;
                if (!SteamApi.ReadP2PPacket(SteamNet, &buf[0], (unsigned int)buf.size(), &got, &from, STEAM_CHANNEL))
                    break;
                busy = true;
                InterlockedIncrement(&TunnelFromSteam);
                if (mode == 1) {
                    CoopSteamTunnelPeer* p = NULL;
                    for (auto& q : peers)
                        if (q.id == from)
                            p = &q;
                    if (!p) {
                        int local = 0;
                        SOCKET s = TunnelUdpSocket(0, local);
                        sockaddr_in to = {};
                        to.sin_family = AF_INET;
                        to.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
                        to.sin_port = htons((u_short)TunnelHostPort);
                        if (s == INVALID_SOCKET || connect(s, (sockaddr*)&to, sizeof(to)) != 0) {
                            if (s != INVALID_SOCKET)
                                closesocket(s);
                            SteamLog("Tunnel: kein lokaler Socket f" "\xFC" "r %s (%llu) - Paket verworfen", SteamFriendName(from).c_str(), from);
                            continue;
                        }
                        peers.push_back({ from, s, now, 0, 0 });
                        p = &peers.back();
                        InterlockedExchange(&TunnelPeersNow, (LONG)peers.size());
                        SteamLog("Tunnel: Mitspieler %s (%llu) - lokal 127.0.0.1:%d -> ENet-Host :%d", SteamFriendName(from).c_str(), from,
                            local, TunnelHostPort);
                    }
                    send(p->s, &buf[0], (int)got, 0);
                    p->lastMs = now;
                    p->fromSteam++;
                }
                else if (mode == 2 && from == TunnelHost && enetKnown) {
                    sendto(TunnelClientSock, &buf[0], (int)got, 0, (sockaddr*)&enetAddr, sizeof(enetAddr));
                }
            }
            // lokal -> Steam
            if (mode == 1) {
                for (auto& p : peers) {
                    for (int k = 0; k < 512; k++) {
                        const int n = recv(p.s, &buf[0], (int)buf.size(), 0);
                        if (n <= 0)
                            break;
                        busy = true;
                        TunnelSend(p.id, &buf[0], n);
                        p.lastMs = now;
                        p.toSteam++;
                    }
                }
                for (size_t i = 0; i < peers.size();) {   // 2 min ohne Pakete: Gegenstelle abbauen
                    if (now - peers[i].lastMs > 120000) {
                        SteamLog("Tunnel: %s seit 2 min still - abgebaut (von Steam %llu, nach Steam %llu Pakete)",
                            SteamFriendName(peers[i].id).c_str(), peers[i].fromSteam, peers[i].toSteam);
                        closesocket(peers[i].s);
                        SteamApi.CloseP2PSessionWithUser(SteamNet, peers[i].id);
                        peers.erase(peers.begin() + i);
                        InterlockedExchange(&TunnelPeersNow, (LONG)peers.size());
                    }
                    else
                        i++;
                }
            }
            else if (mode == 2) {
                for (int k = 0; k < 512; k++) {
                    sockaddr_in from = {};
                    int fl = sizeof(from);
                    const int n = recvfrom(TunnelClientSock, &buf[0], (int)buf.size(), 0, (sockaddr*)&from, &fl);
                    if (n <= 0)
                        break;
                    busy = true;
                    enetAddr = from;
                    enetKnown = true;
                    TunnelSend(TunnelHost, &buf[0], n);
                }
            }
            if (!busy) {   // bis 1 ms auf lokale Pakete warten (Steam-Pakete werden je Durchlauf abgefragt)
                fd_set rd;
                FD_ZERO(&rd);
                if (mode == 1)
                    for (auto& p : peers)
                        if (rd.fd_count < FD_SETSIZE)
                            FD_SET(p.s, &rd);
                if (mode == 2 && TunnelClientSock != INVALID_SOCKET)
                    FD_SET(TunnelClientSock, &rd);
                if (rd.fd_count > 0) {
                    timeval tv = { 0, 1000 };
                    select(0, &rd, NULL, NULL, &tv);
                }
                else
                    Sleep(1);
            }
        }
        for (auto& p : peers) {
            closesocket(p.s);
            SteamApi.CloseP2PSessionWithUser(SteamNet, p.id);
        }
        InterlockedExchange(&TunnelPeersNow, 0);
        if (mode == 2) {
            if (TunnelClientSock != INVALID_SOCKET)
                closesocket(TunnelClientSock);
            TunnelClientSock = INVALID_SOCKET;
            SteamApi.CloseP2PSessionWithUser(SteamNet, TunnelHost);
        }
        return 0;
    }

    static void TunnelStatsLog(const char* why) {
        SteamLog("Tunnel %s: von Steam %ld, nach Steam %ld Pakete, zu gro" "\xDF" " %ld, Sendefehler %ld, Gegenstellen %ld", why,
            (long)TunnelFromSteam, (long)TunnelToSteam, (long)TunnelOversize, (long)TunnelSendFail, (long)TunnelPeersNow);
    }

    static void TunnelStopNow(const char* why) {
        if (!TunnelThreadHandle) {
            TunnelMode = 0;
            return;
        }
        TunnelStop = 1;
        WaitForSingleObject(TunnelThreadHandle, 3000);
        CloseHandle(TunnelThreadHandle);
        TunnelThreadHandle = NULL;
        TunnelStatsLog(why);
        TunnelMode = 0;
    }

    static bool TunnelStart(LONG mode) {
        if (TunnelThreadHandle)
            TunnelStopNow("neu gestartet");
        TunnelStop = 0;
        TunnelMode = mode;
        TunnelFromSteam = TunnelToSteam = TunnelOversize = TunnelSendFail = TunnelPeersNow = 0;
        TunnelStatsMs = GetTickCount();
        TunnelThreadHandle = CreateThread(NULL, 0, CoopSteamTunnelThread, NULL, 0, NULL);
        if (!TunnelThreadHandle) {
            TunnelMode = 0;
            SteamLog("Tunnel: Faden lie" "\xDF" " sich nicht starten (%lu)", GetLastError());
            return false;
        }
        return true;
    }

    // --- Rueckrufe (Hauptfaden, aus SteamAPI_RunCallbacks) -----------------------------------------------------------

    static unsigned long long SteamJoinRequest = 0;   // aus Steam ("Spiel beitreten"/Einladung) - im naechsten Takt
    static int SteamCallbackLogs = 0;
    static std::string SteamClientFail;               // letzter P2P-Fehler zum Host (Anzeige)

    static void SteamOnSessionRequest(void* p) {   // P2PSessionRequest_t (1202): { CSteamID }
        const unsigned long long id = *(const unsigned long long*)p;
        bool accept = false;
        const char* why = "";
        if (TunnelMode == 1) {
            accept = SteamApi.HasFriend(SteamFriends, id, STEAM_FRIENDS_IMMEDIATE);
            why = "kein Steam-Freund";
        }
        else if (TunnelMode == 2) {
            accept = id == TunnelHost;
            why = "nicht der Host dieser Verbindung";
        }
        else
            why = "weder Host noch Steam-Verbindung aktiv";
        if (accept)
            SteamApi.AcceptP2PSessionWithUser(SteamNet, id);
        if (SteamCallbackLogs++ < 40)
            SteamLog("P2P-Anfrage von %s (%llu) %s%s%s", SteamFriendName(id).c_str(), id, accept ? "angenommen" : "abgelehnt (",
                accept ? "" : why, accept ? "" : ")");
    }

    static void SteamOnSessionFail(void* p) {   // P2PSessionConnectFail_t (1203): { CSteamID, uint8 Fehler }
        const unsigned long long id = *(const unsigned long long*)p;
        const int err = *((const unsigned char*)p + 8);
        static const char* const texts[] = { "kein Fehler", "die Gegenstelle spielt nicht Gothic", "keine Rechte an Gothic (App)",
            "die Gegenstelle ist nicht bei Steam angemeldet", "Zeit" "\xFC" "berschreitung" };
        const char* text = err >= 0 && err < 5 ? texts[err] : "unbekannt";
        if (SteamCallbackLogs++ < 40)
            SteamLog("P2P-Verbindung zu %s (%llu) gescheitert: %s (%d)", SteamFriendName(id).c_str(), id, text, err);
        if (TunnelMode == 2 && id == TunnelHost)
            SteamClientFail = std::string("Steam: ") + text + ".";
    }

    static void SteamOnJoinRequested(void* p) {   // GameRichPresenceJoinRequested_t (337): { CSteamID, char connect[256] }
        const unsigned long long fr = *(const unsigned long long*)p;
        char connect[257];
        memcpy(connect, (const char*)p + 8, 256);
        connect[256] = 0;
        unsigned long long host = 0;
        const char* tag = strstr(connect, "+gtcoop_join ");
        if (tag)
            host = _strtoui64(tag + 13, NULL, 10);
        if (!host)
            host = fr;
        SteamLog("Beitreten aus Steam: %s (%llu), Text \"%s\" -> Host %llu", SteamFriendName(fr).c_str(), fr, connect, host);
        SteamJoinRequest = host;
    }

    static CoopSteamCallback SteamCbSession(1202, 8, &SteamOnSessionRequest);
    static CoopSteamCallback SteamCbFail(1203, 16, &SteamOnSessionFail);
    static CoopSteamCallback SteamCbJoin(337, 264, &SteamOnJoinRequested);

    // --- Start ------------------------------------------------------------------------------------------------------------

    template<class T> static bool SteamProc(T& fn, const char* name, std::string& missing) {
        fn = (T)GetProcAddress(SteamApi.dll, name);
        if (!fn && missing.empty())
            missing = name;
        return fn != NULL;
    }

    static void CoopSteamInitOnce() {
        if (SteamTried)
            return;
        SteamTried = true;
        if (IniInt("coopSteam", 1) == 0) {
            SteamWhy = CoopTr("abgeschaltet (Gothic.ini [GOTHICCOOP] coopSteam=0)", "switched off (Gothic.ini [GOTHICCOOP] coopSteam=0)");
            SteamLog("%s", SteamWhy.c_str());
            return;
        }
        SteamApi.dll = GetModuleHandleA("steam_api.dll");
        const bool already = SteamApi.dll != NULL;
        if (!SteamApi.dll)
            SteamApi.dll = LoadLibraryA("steam_api.dll");
        if (!SteamApi.dll) {
            SteamWhy = CoopTr("steam_api.dll fehlt im system-Ordner", "steam_api.dll missing in the system folder");
            SteamLog("nicht verf" "\xFC" "gbar: %s", SteamWhy.c_str());
            return;
        }
        std::string missing;
        SteamProc(SteamApi.InitFlat, "SteamAPI_InitFlat", missing);
        SteamProc(SteamApi.RunCallbacks, "SteamAPI_RunCallbacks", missing);
        SteamProc(SteamApi.RegisterCallback, "SteamAPI_RegisterCallback", missing);
        SteamProc(SteamApi.Friends, "SteamAPI_SteamFriends_v017", missing);
        SteamProc(SteamApi.Networking, "SteamAPI_SteamNetworking_v006", missing);
        SteamProc(SteamApi.User, "SteamAPI_SteamUser_v023", missing);
        SteamProc(SteamApi.Utils, "SteamAPI_SteamUtils_v010", missing);
        SteamProc(SteamApi.GetSteamID, "SteamAPI_ISteamUser_GetSteamID", missing);
        SteamProc(SteamApi.LoggedOn, "SteamAPI_ISteamUser_BLoggedOn", missing);
        SteamProc(SteamApi.GetAppID, "SteamAPI_ISteamUtils_GetAppID", missing);
        SteamProc(SteamApi.GetPersonaName, "SteamAPI_ISteamFriends_GetPersonaName", missing);
        SteamProc(SteamApi.GetFriendCount, "SteamAPI_ISteamFriends_GetFriendCount", missing);
        SteamProc(SteamApi.GetFriendByIndex, "SteamAPI_ISteamFriends_GetFriendByIndex", missing);
        SteamProc(SteamApi.GetFriendGamePlayed, "SteamAPI_ISteamFriends_GetFriendGamePlayed", missing);
        SteamProc(SteamApi.GetFriendPersonaName, "SteamAPI_ISteamFriends_GetFriendPersonaName", missing);
        SteamProc(SteamApi.SetRichPresence, "SteamAPI_ISteamFriends_SetRichPresence", missing);
        SteamProc(SteamApi.ClearRichPresence, "SteamAPI_ISteamFriends_ClearRichPresence", missing);
        SteamProc(SteamApi.GetFriendRichPresence, "SteamAPI_ISteamFriends_GetFriendRichPresence", missing);
        SteamProc(SteamApi.InviteUserToGame, "SteamAPI_ISteamFriends_InviteUserToGame", missing);
        SteamProc(SteamApi.HasFriend, "SteamAPI_ISteamFriends_HasFriend", missing);
        SteamProc(SteamApi.SendP2PPacket, "SteamAPI_ISteamNetworking_SendP2PPacket", missing);
        SteamProc(SteamApi.IsP2PPacketAvailable, "SteamAPI_ISteamNetworking_IsP2PPacketAvailable", missing);
        SteamProc(SteamApi.ReadP2PPacket, "SteamAPI_ISteamNetworking_ReadP2PPacket", missing);
        SteamProc(SteamApi.AcceptP2PSessionWithUser, "SteamAPI_ISteamNetworking_AcceptP2PSessionWithUser", missing);
        SteamProc(SteamApi.CloseP2PSessionWithUser, "SteamAPI_ISteamNetworking_CloseP2PSessionWithUser", missing);
        if (!missing.empty()) {
            SteamWhy = CoopTr("steam_api.dll zu alt (fehlt ", "steam_api.dll too old (missing ") + missing + ")";
            SteamLog("nicht verf" "\xFC" "gbar: %s", SteamWhy.c_str());
            return;
        }
        char err[1024] = { 0 };
        const int r = SteamApi.InitFlat(err);
        if (r != 0) {
            static const char* const codes[] = { "ok", "allgemeiner Fehler", "Steam l" "\xE4" "uft nicht", "Versionen passen nicht" };
            SteamWhy = std::string(r >= 0 && r < 4 ? codes[r] : "Fehler") + (err[0] ? std::string(" - ") + err : std::string());
            SteamLog("nicht verf" "\xFC" "gbar: SteamAPI_InitFlat %d (%s)", r, SteamWhy.c_str());
            return;
        }
        SteamFriends = SteamApi.Friends();
        SteamNet = SteamApi.Networking();
        SteamUserIf = SteamApi.User();
        SteamUtilsIf = SteamApi.Utils();
        if (!SteamFriends || !SteamNet || !SteamUserIf || !SteamUtilsIf) {
            SteamWhy = CoopTr("Steam-Schnittstellen fehlen", "Steam interfaces missing");
            SteamLog("nicht verf" "\xFC" "gbar: %s (Freunde %p, Netz %p, Konto %p, Werkzeuge %p)", SteamWhy.c_str(), SteamFriends, SteamNet,
                SteamUserIf, SteamUtilsIf);
            return;
        }
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);   // ENet tut das erst im Netzfaden
        SteamMyId = SteamApi.GetSteamID(SteamUserIf);
        SteamAppId = SteamApi.GetAppID(SteamUtilsIf);
        SteamMyName = SteamText(SteamApi.GetPersonaName(SteamFriends));
        SteamApi.RegisterCallback(static_cast<CoopSteamCallbackBase*>(&SteamCbSession), 1202);
        SteamApi.RegisterCallback(static_cast<CoopSteamCallbackBase*>(&SteamCbFail), 1203);
        SteamApi.RegisterCallback(static_cast<CoopSteamCallbackBase*>(&SteamCbJoin), 337);
        SteamReady = true;
        SteamWhy.clear();
        SteamLog("angemeldet als %s (SteamID %llu, App %u%s)%s", SteamMyName.c_str(), SteamMyId, SteamAppId,
            SteamApi.LoggedOn(SteamUserIf) ? "" : ", OFFLINE", already ? " - steam_api.dll war schon geladen (zParserExtender)" : "");
    }

    // --- Rich Presence --------------------------------------------------------------------------------------------------

    static int SteamPresence = -1;   // 0 nichts, 1 Host, 2 verbundener Client

    static void SteamSetPresence(int state) {
        if (state == SteamPresence)
            return;
        SteamPresence = state;
        SteamApi.ClearRichPresence(SteamFriends);
        if (state == 1) {
            char connect[64];
            _snprintf(connect, sizeof(connect), "+gtcoop_join %llu", SteamMyId);
            connect[sizeof(connect) - 1] = 0;
            SteamApi.SetRichPresence(SteamFriends, "connect", connect);
            SteamApi.SetRichPresence(SteamFriends, "gtcoop_host", "1");
            SteamApi.SetRichPresence(SteamFriends, "gtcoop_ver", COOP_OTHELLO_FASSUNG);
            SteamApi.SetRichPresence(SteamFriends, "status", "Hostet Gothic Coop");
        }
        else if (state == 2) {
            SteamApi.SetRichPresence(SteamFriends, "gtcoop_ver", COOP_OTHELLO_FASSUNG);
            SteamApi.SetRichPresence(SteamFriends, "status", "Spielt Gothic Coop");
        }
        SteamLog("Rich Presence: %s", state == 1 ? "Host (Freunde k" "\xF6" "nnen in Steam beitreten)" : state == 2 ? "Mitspieler" : "aus");
    }

    // --- Beitreten ------------------------------------------------------------------------------------------------------

    static unsigned long long SteamJoinTarget = 0;     // fuer den naechsten CoopStartClient
    static unsigned long long SteamClientHost = 0;     // laufende Client-Verbindung ueber Steam
    static unsigned long long SteamLastHost = 0;       // Shift+F2 verbindet wieder dorthin
    static std::string SteamLastHostName;
    static bool SteamLastWasSteam = false;             // letzter Verbindungsversuch ueber Steam (Anzeige)

    static void SteamNote(const std::string& text, zCMenu* menu) {
        LobbyNote(text, menu);
        if (!menu && player)
            ChatLog(string(text.c_str()), zCOLOR(255, 200, 80, 255));
    }

    static void CoopSteamJoin(unsigned long long host, const char* why, zCMenu* menu) {
        const std::string name = SteamFriendName(host);
        if (!SteamReady) {
            SteamNote(CoopTr("Steam nicht verf" "\xFC" "gbar: ", "Steam not available: ") + SteamWhy, menu);
            return;
        }
        if (host == SteamMyId) {
            SteamNote(CoopTr("Das bist du selbst.", "That's you."), menu);
            return;
        }
        if (ServerThread) {
            SteamNote(CoopTr("Du hostest gerade - erst \"Verbindung trennen\", dann bei ", "You are hosting - \"Disconnect\" first, then join ") + name + CoopTr(" beitreten.", "."), menu);
            return;
        }
        if (ClientThread) {
            SteamNote(SteamClientHost == host ? CoopTr("Mit ", "Already connecting/connected to ") + name + CoopTr(" wird schon verbunden bzw. ist verbunden.", ".")
                                              : std::string(CoopTr("Erst \"Verbindung trennen\".", "\"Disconnect\" first.")), menu);
            return;
        }
        SteamJoinTarget = host;
        SteamClientFail.clear();
        SteamLog("Beitreten bei %s (%llu) - %s, %s", name.c_str(), host, why, player ? "im Spiel" : "im Hauptmen" "\xFC");
        if (!player) {   // wie "Spiel beitreten" im COOP-Menue
            Lobby.active = true;
            Lobby.role = 2;
            CoopSaveSyncSetMenu(true);
            LobbyResetClient();
        }
        CoopStartClient();
        if (menu)
            SteamNote(CoopTr("Verbinde \xFC" "ber Steam mit ", "Connecting via Steam to ") + name + " ...", menu);
    }

    // Plugin.cpp, CoopStartClient (Hauptfaden, vor dem Start des ENet-Fadens): Steam-Ziel uebernehmen und Tunnel starten
    bool CoopSteamClientPrepare() {
        SteamClientHost = 0;
        const unsigned long long host = SteamJoinTarget;
        SteamJoinTarget = 0;
        SteamLastWasSteam = host != 0;
        if (!host || !SteamReady)
            return false;
        if (TunnelThreadHandle)
            TunnelStopNow("neuer Client");
        int port = 0;
        SOCKET s = TunnelUdpSocket(0, port);
        if (s == INVALID_SOCKET || !port) {
            if (s != INVALID_SOCKET)
                closesocket(s);
            SteamLog("Tunnel: lokaler Socket fehlgeschlagen (%d) - Verbindung \xFC" "ber IP", WSAGetLastError());
            SteamLastWasSteam = false;
            return false;
        }
        TunnelClientSock = s;
        TunnelClientPort = port;
        TunnelHost = host;
        if (!TunnelStart(2)) {
            closesocket(s);
            TunnelClientSock = INVALID_SOCKET;
            SteamLastWasSteam = false;
            return false;
        }
        SteamClientHost = host;
        SteamLastHost = host;
        SteamLastHostName = SteamFriendName(host);
        SteamLog("Client \xFC" "ber Steam: Host %s (%llu), ENet verbindet sich mit 127.0.0.1:%d (MTU 1200)", SteamLastHostName.c_str(), host, port);
        return true;
    }

    unsigned long long CoopSteamClientHostId() {
        return SteamClientHost;
    }

    // v88: Anzeige (Chat beim Hoststart, Lobby-Status, Zeile oben links)
    bool CoopSteamAvailable() {
        return SteamReady;
    }

    int CoopSteamClientLocalPort() {
        return TunnelClientPort;
    }

    // Anzeige (Lobby-Status, Chat): leer = IP
    std::string CoopSteamTargetText() {
        if (!SteamLastWasSteam)
            return std::string();
        return CoopTr("Steam-Freund ", "Steam friend ") + (SteamLastHostName.empty() ? std::string("?") : SteamLastHostName);
    }

    // Plugin.cpp, CoopKeyClient (Shift+F2 ohne Verbindung): wie zuletzt - war das Steam, wieder ueber Steam
    bool CoopSteamReconnect() {
        if (!SteamReady || !SteamLastHost)
            return false;
        CoopSteamJoin(SteamLastHost, "Shift+F2 (wie zuletzt \xFC" "ber Steam)", NULL);
        return true;
    }

    // CoopMenuLobby.cpp: "Spiel beitreten" im COOP-Menue ist IP
    void CoopSteamForgetLast() {
        SteamLastHost = 0;
        SteamJoinTarget = 0;
    }

    // --- Takt -------------------------------------------------------------------------------------------------------------

    // Game_Loop und Game_MenuLoop (Hauptfaden)
    void CoopSteamPulse() {
        if (!SteamTried)
            CoopSteamInitOnce();
        if (!SteamReady)
            return;
        SteamApi.RunCallbacks();
        const bool hosting = ServerThread && CoopServerState == 1;
        if (hosting && TunnelMode != 1) {
            if (TunnelMode)
                TunnelStopNow("Wechsel zum Host");
            TunnelHostPort = ConnectionPort;
            if (TunnelStart(1))
                SteamLog("Host \xFC" "ber Steam erreichbar (ENet-Port %d) - Freunde treten \xFC" "ber COOP > Steam oder die Steam-Freundesliste bei",
                    TunnelHostPort);
        }
        else if (!hosting && TunnelMode == 1)
            TunnelStopNow("Host beendet");
        if (TunnelMode == 2 && !ClientThread) {
            TunnelStopNow("Client beendet");
            SteamClientHost = 0;
        }
        SteamSetPresence(hosting ? 1 : (ClientThread && CoopClientState == 2) ? 2 : 0);
        if (TunnelMode && GetTickCount() - TunnelStatsMs > 60000) {
            TunnelStatsMs = GetTickCount();
            TunnelStatsLog("l" "\xE4" "uft");
        }
        if (SteamJoinRequest) {
            const unsigned long long host = SteamJoinRequest;
            SteamJoinRequest = 0;
            CoopSteamJoin(host, "Steam-Freundesliste/Einladung", zCMenu::GetActive());
        }
    }

    // --- Menue MENU_COOP_STEAM ------------------------------------------------------------------------------------------

    struct CoopSteamFriendRow {
        unsigned long long id;
        std::string name, ver;
        bool hosting;
    };
    static std::vector<CoopSteamFriendRow> SteamRows;
    static long long SteamRowsMs = 0;
    static long long SteamMenuShownMs = 0;
    static zCMenu* SteamMenuShown = NULL;

    static void SteamScanFriends() {
        SteamRows.clear();
        const int n = SteamApi.GetFriendCount(SteamFriends, STEAM_FRIENDS_IMMEDIATE);
        for (int i = 0; i < n && i < 2000; i++) {
            const unsigned long long id = SteamApi.GetFriendByIndex(SteamFriends, i, STEAM_FRIENDS_IMMEDIATE);
            CoopSteamFriendGame g = {};
            if (!id || !SteamApi.GetFriendGamePlayed(SteamFriends, id, &g) || (unsigned int)(g.gameId & 0xFFFFFF) != SteamAppId)
                continue;
            const char* host = SteamApi.GetFriendRichPresence(SteamFriends, id, "gtcoop_host");
            const char* ver = SteamApi.GetFriendRichPresence(SteamFriends, id, "gtcoop_ver");
            SteamRows.push_back({ id, SteamFriendName(id), ver ? std::string(ver) : std::string(), host && strcmp(host, "1") == 0 });
        }
        std::sort(SteamRows.begin(), SteamRows.end(), [](const CoopSteamFriendRow& a, const CoopSteamFriendRow& b) {
            return a.hosting != b.hosting ? a.hosting : a.name < b.name;
        });
    }

    // v94 (Test v93: Beitreten "nur klein in der Mitte"): Zeilen in grosser Schrift (COOP_MENU-v94.D), was ein Klick tut gross
    static std::string SteamRowText(const CoopSteamFriendRow& r) {
        if (r.hosting) {
            if (!r.ver.empty() && r.ver != COOP_OTHELLO_FASSUNG)
                return ">> " + r.name + CoopTr(" hostet - BEITRETEN (Fassung ", " hosts - JOIN (version ") + r.ver + CoopTr(", hier ", ", here ") + COOP_OTHELLO_FASSUNG + ")";
            return ">> " + r.name + CoopTr(" hostet - BEITRETEN", " hosts - JOIN");
        }
        return r.name + (ServerThread ? CoopTr(" spielt Gothic - EINLADEN", " plays Gothic - INVITE") : CoopTr(" spielt Gothic (hostet nicht)", " plays Gothic (not hosting)"));
    }

    // v94: Freundesliste auch fuer die COOP-Startseite (hoechstens alle 2 s, beim Klick sofort)
    static void SteamQuickScan(bool force) {
        const long long now = GetCurrentMs();
        if (!force && now < SteamRowsMs + 2000)
            return;
        SteamRowsMs = now;
        SteamScanFriends();
    }

    // Freund fuer den Steam-Knopf: wer hostet, gleiche Fassung zuerst (SteamRows: Hosts vorn, nach Namen)
    static const CoopSteamFriendRow* SteamQuickHost(int& hosts) {
        hosts = 0;
        const CoopSteamFriendRow* best = NULL;
        for (auto& r : SteamRows) {
            if (!r.hosting)
                continue;
            hosts++;
            const bool same = r.ver.empty() || r.ver == COOP_OTHELLO_FASSUNG;
            if (!best || (same && !(best->ver.empty() || best->ver == COOP_OTHELLO_FASSUNG)))
                best = &r;
        }
        return best;
    }

    // v94: Steam-Knopf der COOP-Startseite (MENUITEM_COOP_STEAMJOIN, CoopMenuLobby.cpp LobbyShowStatus) - Text je nach Lage
    std::string CoopSteamQuickText() {
        // v95 (Test v94: "dieser Text 'Kein Steam-Freund hostet gerade' ist zu lang"): kurz, der Knopf ist schmaler
        if (!SteamReady)
            return CoopTr("Steam nicht verf" "\xFC" "gbar", "Steam not available");
        if (ServerThread)
            return CoopTr("Steam: Freunde einladen ...", "Steam: invite friends ...");
        if (ClientThread) {
            if (CoopClientState == 2)
                return SteamClientHost ? CoopTr("Steam: verbunden mit ", "Steam: connected to ") + SteamFriendName(SteamClientHost)
                                       : std::string(CoopTr("Verbunden \xFC" "ber IP", "Connected via IP"));
            return SteamClientHost ? CoopTr("Steam: verbinde mit ", "Steam: connecting to ") + SteamFriendName(SteamClientHost) + " ..."
                                   : std::string(CoopTr("Verbinde ...", "Connecting ..."));
        }
        SteamQuickScan(false);
        int hosts = 0;
        const CoopSteamFriendRow* r = SteamQuickHost(hosts);
        if (!r)
            return CoopTr("Steam: niemand hostet", "Steam: nobody hosting");
        std::string t = ">> " + r->name + CoopTr(" hostet: beitreten", " hosts: join");
        if (!r->ver.empty() && r->ver != COOP_OTHELLO_FASSUNG)
            t = ">> " + r->name + " (" + r->ver + CoopTr("): beitreten", "): join");
        if (hosts > 1)
            t += " (+" + std::to_string(hosts - 1) + ")";
        return t;
    }

    // v94: Klick auf den Steam-Knopf (COOP_STEAM_QUICK)
    static void CoopSteamQuick(zCMenu* menu) {
        if (!SteamReady) {
            SteamNote(CoopTr("Steam nicht verf" "\xFC" "gbar (", "Steam not available (") + SteamWhy + CoopTr(") - \"\xDC" "ber IP beitreten\".", ") - \"Join via IP\"."), menu);
            return;
        }
        if (ServerThread) {   // Freunde einladen: Steam-Freunde verschachtelt oeffnen (zCMenu_HandleSelAction_Coop)
            LobbyOpenSteamMenu = !player;
            if (player)
                SteamNote(CoopTr("Einladen: Einstellungen > COOP > Steam-Freunde.", "Invite: Settings > COOP > Steam friends."), menu);
            return;
        }
        if (ClientThread) {
            SteamNote(CoopClientState == 2 ? std::string(CoopTr("Schon verbunden - f" "\xFC" "r einen anderen Host erst \"Verbindung trennen\".", "Already connected - \"Disconnect\" first for another host."))
                                           : std::string(CoopTr("Verbindung wird aufgebaut ...", "Connecting ...")), menu);
            return;
        }
        SteamQuickScan(true);
        int hosts = 0;
        const CoopSteamFriendRow* r = SteamQuickHost(hosts);
        if (!r) {
            SteamNote(CoopTr("Kein Steam-Freund hostet gerade - einer w" "\xE4" "hlt \"Spiel hosten\", der andere dann diesen Knopf.", "No Steam friend is hosting - one picks \"Host game\", the other this button."), menu);
            return;
        }
        CoopSteamJoin(r->id, "Steam-Knopf der COOP-Startseite", menu);
    }

    // Verbindungsstatus fuer das Steam-Menue (im Hauptmenue der Lobby-Status, im Spiel kurz)
    static void SteamNetStatus(std::string& l1, std::string& l2) {
        l1.clear();
        l2.clear();
        if (!player) {
            LobbyStatus(l1, l2);
        }
        else if (ServerThread) {
            l1 = CoopTr("Host l" "\xE4" "uft - ", "Hosting - ") + std::to_string(PlayerNpcs.size()) + CoopTr(" Mitspieler im Spiel.", " player(s) in game.");
        }
        else if (ClientThread) {
            l1 = CoopClientState == 2 ? CoopTr("Verbunden", "Connected") + std::string(SteamClientHost ? CoopTr(" \xFC" "ber Steam mit ", " via Steam to ") + SteamFriendName(SteamClientHost) : "") + "."
                                      : CoopTr("Verbinde ...", "Connecting ...");
        }
        else
            l1 = CoopTr("Nicht verbunden.", "Not connected.");
        if (!SteamClientFail.empty() && !ClientThread)
            l2 = SteamClientFail;
    }

    void CoopSteamMenuPulse() {
        zCMenu* menu = zCMenu::GetActive();
        if (!menu || LobbyUpper(menu->name) != "MENU_COOP_STEAM")
            return;
        const long long now = GetCurrentMs();
        if (menu == SteamMenuShown && now < SteamMenuShownMs + 1000)
            return;
        SteamMenuShown = menu;
        SteamMenuShownMs = now;
        std::string s1, s2;
        std::vector<std::string> rows(STEAM_ROWS);
        if (!SteamReady) {
            s1 = CoopTr("Steam nicht verf" "\xFC" "gbar: ", "Steam not available: ") + SteamWhy;
            s2 = CoopTr("Coop \xFC" "ber IP geht weiter (COOP > Spiel hosten / \xDC" "ber IP beitreten).", "Coop via IP still works (COOP > Host game / Join via IP).");   // v94
            SteamRows.clear();
        }
        else {
            if (now >= SteamRowsMs + 2000) {
                SteamRowsMs = now;
                SteamScanFriends();
            }
            s1 = CoopTr("Steam: angemeldet als ", "Steam: signed in as ") + SteamMyName + (SteamApi.LoggedOn(SteamUserIf) ? "" : " (OFFLINE)") + ".";
            if (ServerThread)
                s2 = CoopTr("Du hostest - Freunde sehen dich hier und in der Steam-Freundesliste (Spiel beitreten).", "You are hosting - friends see you here and in the Steam friends list (Join game).");
            else if (ClientThread)
                s2 = SteamClientHost ? CoopTr("Verbindung \xFC" "ber Steam mit ", "Connected via Steam to ") + SteamFriendName(SteamClientHost) + "." : std::string(CoopTr("Verbindung \xFC" "ber IP.", "Connected via IP."));
            else
                s2 = CoopTr("Freund w" "\xE4" "hlen, der hostet - oder selbst \"Spiel hosten\" und Freunde einladen.", "Pick a friend who hosts - or \"Host game\" yourself and invite friends.");
            for (int i = 0; i < STEAM_ROWS && i < (int)SteamRows.size(); i++)
                rows[i] = SteamRowText(SteamRows[i]);
            if (SteamRows.empty())
                rows[0] = CoopTr("Kein Steam-Freund spielt gerade Gothic.", "No Steam friend is playing Gothic.");
            else if ((int)SteamRows.size() > STEAM_ROWS)
                rows[STEAM_ROWS - 1] += "  (+" + std::to_string(SteamRows.size() - STEAM_ROWS + 1) + CoopTr(" weitere)", " more)");
        }
        std::string n1, n2;
        SteamNetStatus(n1, n2);
        if (!Lobby.note.empty() && now < Lobby.noteUntil)
            n2 = Lobby.note;
        LobbySetItemText(LobbyFindItem(menu, "MENUITEM_COOPSTEAM_STATUS1"), s1);
        LobbySetItemText(LobbyFindItem(menu, "MENUITEM_COOPSTEAM_STATUS2"), s2);
        for (int i = 0; i < STEAM_ROWS; i++) {
            char id[40];
            _snprintf(id, sizeof(id), "MENUITEM_COOPSTEAM_ROW%d", i);
            id[sizeof(id) - 1] = 0;
            LobbySetItemText(LobbyFindItem(menu, id), rows[i]);
        }
        LobbySetItemText(LobbyFindItem(menu, "MENUITEM_COOPSTEAM_NET1"), n1);
        LobbySetItemText(LobbyFindItem(menu, "MENUITEM_COOPSTEAM_NET2"), n2);
    }

    // CoopMenuLobby.cpp (LobbyHandleSelAction): COOP_STEAM_* - auch im Spiel
    bool CoopSteamMenuCommand(const std::string& cmd, zCMenu* menu) {
        if (cmd.compare(0, 11, "COOP_STEAM_") != 0)
            return false;
        SteamLog("Befehl %s (%s, Host %s, Client %s)", cmd.c_str(), player ? "im Spiel" : "im Hauptmen" "\xFC", ServerThread ? "an" : "aus",
            ClientThread ? "an" : "aus");
        SteamMenuShownMs = 0;   // Anzeige gleich neu
        if (cmd == "COOP_STEAM_QUICK") {   // v94: Steam-Knopf der COOP-Startseite
            CoopSteamQuick(menu);
            return true;
        }
        if (cmd == "COOP_STEAM_HOST") {
            if (!player)
                LobbyCommand("COOP_HOST", menu);
            else if (ServerThread)
                SteamNote(CoopTr("Der Host l" "\xE4" "uft schon.", "The host is already running."), menu);
            else
                CoopKeyHost();
            return true;
        }
        if (cmd == "COOP_STEAM_STOP") {
            if (!player) {
                LobbyCommand("COOP_STOP", menu);
                return true;
            }
            if (ClientThread) {
                CoopPendingRole = 0;
                CoopClientStop = 1;
                SteamNote(CoopTr("Verbindung getrennt.", "Disconnected."), menu);
            }
            else if (ServerThread) {
                if (!PlayerNpcs.empty() || CoopSaveSyncPeers() > 0)
                    SteamNote(CoopTr("Als Host sind Spieler verbunden - im Spiel nicht trennbar.", "Players are connected to your host - cannot disconnect in game."), menu);
                else {
                    CoopPendingRole = 0;
                    CoopServerStop = 1;
                    SteamNote(CoopTr("Host beendet.", "Host stopped."), menu);
                }
            }
            else
                SteamNote(CoopTr("Nicht verbunden.", "Not connected."), menu);
            return true;
        }
        if (!SteamReady) {
            SteamNote(CoopTr("Steam nicht verf" "\xFC" "gbar: ", "Steam not available: ") + SteamWhy, menu);
            return true;
        }
        if (cmd.compare(0, 15, "COOP_STEAM_ROW_") == 0) {
            const int n = atoi(cmd.c_str() + 15);
            if (n < 0 || n >= (int)SteamRows.size() || n >= STEAM_ROWS) {
                SteamNote(CoopTr("Leere Zeile.", "Empty row."), menu);
                return true;
            }
            const CoopSteamFriendRow r = SteamRows[n];
            if (r.hosting) {
                CoopSteamJoin(r.id, "Steam-Men" "\xFC", menu);
            }
            else if (ServerThread) {
                char connect[64];
                _snprintf(connect, sizeof(connect), "+gtcoop_join %llu", SteamMyId);
                connect[sizeof(connect) - 1] = 0;
                const bool ok = SteamApi.InviteUserToGame(SteamFriends, r.id, connect);
                SteamLog("Einladung an %s (%llu): %s", r.name.c_str(), r.id, ok ? "geschickt" : "FEHLGESCHLAGEN");
                SteamNote(ok ? CoopTr("Einladung an ", "Invitation sent to ") + r.name + CoopTr(" geschickt - er nimmt sie in Steam an (Gothic muss laufen).", " - they accept it in Steam (Gothic must be running).")
                             : CoopTr("Einladung an ", "Invitation to ") + r.name + CoopTr(" ging nicht.", " failed."), menu);
            }
            else
                SteamNote(r.name + CoopTr(" hostet nicht - einer von euch w" "\xE4" "hlt erst \"Spiel hosten\".", " is not hosting - one of you picks \"Host game\" first."), menu);
            return true;
        }
        SteamNote(CoopTr("Unbekannter Befehl ", "Unknown command ") + cmd, menu);
        return true;
    }

#else
    void CoopSteamPulse() {}
    void CoopSteamMenuPulse() {}
    bool CoopSteamClientPrepare() { return false; }
    unsigned long long CoopSteamClientHostId() { return 0; }
    int CoopSteamClientLocalPort() { return 0; }
    std::string CoopSteamTargetText() { return std::string(); }
    bool CoopSteamReconnect() { return false; }
    void CoopSteamForgetLast() {}
    bool CoopSteamMenuCommand(const std::string&, zCMenu*) { return false; }
    bool CoopSteamAvailable() { return false; }
    std::string CoopSteamQuickText() { return std::string(); }
#endif
}
