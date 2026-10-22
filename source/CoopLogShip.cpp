// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

    // --- Coop-Logs zum Partner (v34 Client -> Host, v35 auch Host -> Client) -------------------------
    //
    // Jeder PC schickt sein system\GothicCoopOthello.log fortlaufend an den Partner, damit nach einem Test beide Logs auf beiden
    // PCs liegen: der Host schreibt system\GothicCoopOthelloClient.log (FRIEND_1; weitere Clients ..._<Name>.log), der Client
    // system\GothicCoopOthelloHost.log (v35, Wunsch des Nutzers). Je Verbindung eine Kopfzeile mit Name, Fassung, Zeit und Stelle.
    // Gelesen wird unter derselben Sperre wie OthelloCompatLog (OthelloCompat.cpp), in Stuecken bis 24 KB je Sekunde ab der
    // zuletzt gesendeten Stelle; wird die Datei kleiner (neu angelegt), beginnt es wieder bei 0.
    // v35: Start ab CoopLogSessionStart (das Log der vorigen Sitzung ist archiviert, siehe CoopLogArchiveOnce - v34 schickte
    // bei einem nie umbenannten Log den ganzen alten Verlauf). Der Host sendet erst, wenn ein Client sein erstes Stueck
    // ("first") geschickt hat, und beginnt dann wieder am Sitzungsanfang - so bekommt auch ein spaeter verbundener Client das
    // ganze Host-Log (ein schon verbundener Client bekommt es dann ein zweites Mal, mit eigener Kopfzeile).
    // Kennzeichen "host" = vom Host. GTs Server leitet Client-Pakete an die anderen Clients weiter; die ignorieren sie.

    static long long LogShipNextMs = 0;
    static long LogShipOffset = 0;
    static bool LogShipStarted = false;
    static bool LogShipHostWanted = false;    // Host: mindestens ein Client hat sein Log begonnen
    static bool LogShipHostRestart = false;   // Host: neuer Client - eigenes Log noch einmal ab Sitzungsanfang
    static std::string LogShipConnId;         // v71: Client - Kennung der Verbindung, fuer die zuletzt gesendet wurde

    bool CoopLogShipHandles(int type) {
        return type == SYNC_CLIENT_LOG;
    }

    void CoopLogShipPulse() {
        const bool host = ServerThread != NULL;
        const bool client = ClientThread != NULL && !host && MyselfId != "_player_";
        if ((!host && !client) || (host && !LogShipHostWanted) || CurrentMs < LogShipNextMs)
            return;
        LogShipNextMs = CurrentMs + 1000;
        if (!LogShipStarted || (host && LogShipHostRestart))
            LogShipOffset = CoopLogSessionStart;
        // v71: nach einem Neuverbinden (neue Kennung, z. B. FRIEND_2) beginnt beim Host eine neue Datei - mit Kopfzeile ab der
        // aktuellen Stelle (Test v69: GothicCoopOthelloClient_FRIEND_2..5.log fingen ohne Kopf mitten im Verlauf an)
        const bool newConn = client && LogShipConnId != MyselfId.ToChar();
        const bool first = !LogShipStarted || (host && LogShipHostRestart) || newConn;
        std::string chunk;
        bool restart = false;
        EnterCriticalSection(OthelloCompatLogLock());
        FILE* f = fopen("system\\GothicCoopOthello.log", "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            const long size = ftell(f);
            if (size < LogShipOffset) {   // neu angelegt
                LogShipOffset = 0;
                restart = true;
            }
            const long n = size - LogShipOffset > 24000 ? 24000 : size - LogShipOffset;
            if (n > 0) {
                chunk.resize(n);
                fseek(f, LogShipOffset, SEEK_SET);
                const size_t got = fread(&chunk[0], 1, n, f);
                chunk.resize(got);
            }
            fclose(f);
        }
        LeaveCriticalSection(OthelloCompatLogLock());
        if (chunk.empty())
            return;
        json j;
        j["type"] = SYNC_CLIENT_LOG;
        j["id"] = MyselfId.ToChar();
        j["off"] = LogShipOffset;
        j["ver"] = COOP_OTHELLO_FASSUNG;
        if (host)
            j["host"] = 1;
        if (first || restart)
            j["first"] = 1;
        j["d"] = chunk;
        CoopTraceCount(TRACE_SENT, SYNC_CLIENT_LOG);
        ReadyToSendJsons.enqueue(j);
        LogShipOffset += (long)chunk.size();
        LogShipStarted = true;
        LogShipHostRestart = false;
        if (client)
            LogShipConnId = MyselfId.ToChar();
    }

    void CoopLogShipReceive(json& e, const std::string& id) {
        if (e.contains("by"))
            return;
        const bool fromHost = e.contains("host");
        std::string path;
        if (ServerThread) {
            if (fromHost)
                return;
            std::string safe;
            for (char c : id)
                safe += (isalnum((unsigned char)c) || c == '_' || c == '-') ? c : '_';
            path = safe == "FRIEND_1" ? "system\\GothicCoopOthelloClient.log" : "system\\GothicCoopOthelloClient_" + safe + ".log";
            if (e.contains("first")) {   // ein Client beginnt: ab jetzt (wieder) das Host-Log ab Sitzungsanfang schicken
                LogShipHostWanted = true;
                LogShipHostRestart = true;
            }
        }
        else {
            if (!fromHost)
                return;   // weitergeleitetes Log eines anderen Clients
            path = "system\\GothicCoopOthelloHost.log";
        }
        try {
            const std::string d = e.at("d").get<std::string>();
            CoopLogArchiveOnce();
            FILE* f = fopen(path.c_str(), "ab");
            if (!f)
                return;
            if (e.contains("first")) {
                SYSTEMTIME t;
                GetLocalTime(&t);
                const std::string ver = e.contains("ver") && e["ver"].is_string() ? e["ver"].get<std::string>() : std::string("v34");
                fprintf(f, "\r\n===== Log von %s (Fassung %s), empfangen ab %04d-%02d-%02d %02d:%02d:%02d (Stelle %ld) =====\r\n",
                    id.c_str(), ver.c_str(), t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond,
                    e.contains("off") ? e["off"].get<long>() : 0L);
            }
            fwrite(d.data(), 1, d.size(), f);
            fclose(f);
        }
        catch (std::exception& ex) {
            CoopErrorLog("Coop-Log", ex.what(), id.c_str());
        }
    }
}
