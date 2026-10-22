// Supported with union (c) 2020 Union team
// Union SOURCE file

#include <deque>
#include <functional>

namespace GOTHIC_ENGINE {

    // v75: Abgleich-Regeln aus dem Menue, im Coop die des Hosts (CoopDifficulty.cpp)
    bool CoopRuleSaves();
    int CoopRuleAutoSaveMinutes();
    bool CoopRuleTakeOnConnect();
    void CoopRulesSendSoon();
    void CoopResyncRequest(const char* why);   // weiter unten (v39)

    // --- Gemeinsames Speichern und Laden (v17) + Stand uebernehmen (v19), 18.10.2026 -------------
    //
    // Ziel (Nutzer): beide PCs sind immer auf demselben Stand; niemand laedt allein und zerschiesst
    // damit den gemeinsamen Fortschritt. Dazu ein Archiv aller Sicherungen auf beiden PCs.
    //
    // v17:
    // - Speichert einer (Menue, Schnellspeichern von zUtilities/Othello), speichert der andere sofort
    //   in denselben Slot mit. Beide Staende bekommen dieselbe Kennung (COOPSYNC.TXT im Slot) und
    //   werden ins Archiv saves_...\coop_archiv\<Kennung>\ kopiert. Archiviertes wird nie geloescht.
    // - Der Host speichert alle coopAutoSaveMinutes (Standard 10) automatisch, reihum in die
    //   reservierten Slots coopSaveSlots (Standard 6-9); der Client speichert mit.
    // - Liste im Spiel (coopBackupKey, Standard F9): alle archivierten Staende, F8/Shift+F8 blaettern,
    //   zweimal Enter laedt (der Partner laedt mit). Enter wird seit v20 selbst aus dem Tastenzustand erkannt.
    // - Bevor ein Slot auf Wunsch des Partners ueberschrieben wird und darin ein Stand ohne Kennung
    //   liegt, wird er als "vorher_..." archiviert.
    // v19 (Stufe 2):
    // - Wer laedt, gibt die WELT vor: der andere uebernimmt die Welt des Ladenden (Story, Quests,
    //   Tagebuch, Truhen, NPCs) und behaelt seinen eigenen HELDEN - so, wie der zu dieser Sicherung war
    //   (COOPHERO.TXT, bei jedem Speichern mitgeschrieben). Fehlen ihm die Dateien, holt er sie ueber das
    //   Netz (FILE_REQ/FILE_PART/FILE_END) nach coop_archiv\<Kennung>__von_<Absender>\.
    // - Beim Verbinden (coopTakeHostStateOnConnect) und per Taste (coopSyncKey, Standard F6) speichern
    //   beide gemeinsam, danach uebernimmt der Client die Host-Welt mit seinem Helden.
    // - Sicherung: die eigene Welt jedes PCs bleibt je Kennung archiviert; vor jeder Uebernahme wird der
    //   aktuelle Stand zusaetzlich gesichert (coopBackupBeforeLoad). Ist der Host-Stand kaputt, laedt der
    //   Client seine eigene Sicherung (F9) - dann uebernimmt der Host diese Welt.
    //
    // Erkennung ueber die Union-Rueckrufe: Union leitet alle Engine-Aufrufe von oCGame::WriteSavegame
    // (0x00428A16, 0x0063CEEF, 0x006435E7) und LoadSavegame (0x00428D04, 0x00643660) ueber SHW32.DLL
    // und meldet Game_SaveBegin/End bzw. Game_LoadBegin_SaveGame mit SaveLoadGameInfo.slotID - auch fuer
    // das Schnellspeichern. Selbst ausgeloest wird ueber CGameManager::Write_Savegame/Read_Savegame
    // (0x00428790/0x00428B50, wie zUtilities). Die Archivkopie entsteht einen Frame nach Game_SaveEnd,
    // weil Write_Savegame SAVEINFO.SAV und THUMB.SAV erst nach WriteSavegame schreibt.
    //
    // v76 "wasserdicht I" (Bericht gt-fix\SPIELSTAND-BERICHT.md, 21.10.2026). Am Speicherabbild geprueft: Gothic leert den Slot
    // bei jedem Speichern selbst (oCGame::WriteSavegame 0x0063AD80 -> CopyFromCurrent 0x00435F70 -> Dir_Copy 0x00444780 ->
    // Dir_Clear(slot, "*.*") 0x004445D0, alle unveraendert) - GTs Dateien im Slot verschwinden dabei mit.
    // - Feste PC-Kennung (coop_archiv\COOPPC.TXT) statt der Rolle HOST/FRIEND_n: srcpc/bypc in COOPSYNC.TXT, "pc" in jedem Paket,
    //   Partnerkopien <Kennung>__von_<PC>. Nach Neuverbinden (FRIEND_2) oder Rollentausch passte die Rolle nicht mehr (S1).
    // - Manifest je Stand (COOPFILES.TXT: Groesse und CRC32 jeder Spieldatei): Archiv erst in <Kennung>.part, pruefen, dann
    //   umbenennen, nie ueberschreiben; Uebertragung prueft Reihenfolge, Groesse, CRC und Manifest; zwischengespeicherte Kopien
    //   werden geprueft (S2, S13). Textdateien ueber .tmp mit Endmarke (S11).
    // - Slot sicher ersetzen: nach savegameN.neu kopieren und pruefen, dann tauschen - nichts Altes bleibt liegen (S4).
    // - Pause (Wunsch des Nutzers): Shift+F2 -> erst gemeinsame Sicherung, dann Pause. In Pause und bei Verbindungsstoerung
    //   speichert und laedt der Client nur fuer sich ("dann werden auch keine Sicherungen fuer den gemeinsamen Coop angelegt").
    //   Fortsetzen, Wiederverbinden: der Client uebernimmt die AKTUELLE Host-Welt, sein Held wird auf den Stand bei Pausenbeginn
    //   bzw. Verbindungsabbruch gesetzt (Variante b) - nichts doppelt, der Host verliert nichts. Danach speichert der Client
    //   seine eigene Kopie dieser Kennung (Host-Welt + eigener Held). Bricht die Verbindung ab, sichert der Host sofort.
    // - Kleinkram: Warteschlange fuer SAVE_REQ (S8), Speicherzeit in COOPSYNC.TXT (S9), Merker bei Verbindungsende und Neuem
    //   Spiel leeren (S10), gescheiterte Uebernahme mit Warnung im Bild (S5, Teil).

    // v93: Eintrag der Liste "Spielstand waehlen" (CoopMenuLobby.cpp) - ein gemeinsamer Stand aus dem Coop-Archiv bzw. einem Slot
    struct CoopListEntry {
        std::string sid, name, time, kind, run;   // sid leer = lokaler Stand ohne Kennung (Slot mit COOPRUN.TXT)
        bool both = false;                        // Liste: der Partner hat ihn auch
        std::string dir;        // eigene Kopie (Slot- oder Archivordner, mit \ am Ende) - Bild und Angaben fuer die Vorschau
        int slot = 0;           // Slot mit dieser Kennung (eigene Kopie), 0 = nur im Archiv
        std::string key;        // Sortierung "JJJJMMTThhmm" + Unix-Zeit
    };
    void CoopSaveSyncListJoint(std::vector<CoopListEntry>& out);
    int CoopSaveSyncPrepareSlot(const std::string& sid, std::string& why);
    bool CoopSaveSyncAskLoadSid(const std::string& sid);
    std::string CoopSaveSyncCurrentRun();
    void CoopSaveSyncSetRun(const std::string& run, const char* why);
    std::string CoopSaveSyncNewRun(const std::string& partnerName);
    void CoopSaveSyncNotePartnerNick(const std::string& nick);
    void CoopSaveSyncKnownRuns(std::set<std::string>& out);

#if ENGINE == Engine_G1

    struct CoopSaveMeta {
        std::string sid, kind, name, by, world, time, src;   // src: wessen Welt in diesem Stand steckt
        std::string srcpc, bypc, srcname;                     // v76: feste PC-Kennung der Welt / des Ausloesers, Spielername
        std::string mh;                                       // v76: Pruefsumme des Manifests (Paket: welcher Inhalt dazugehoert)
        int slot = -1;
        bool take = false;                                    // SAVE_REQ: danach Welt des Absenders uebernehmen
        bool list = false;                                    // LOAD_REQ (v29): bewusst aus der F9-Liste - gewaehlte Welt gilt
        int ask = 0;                                          // LOAD_REQ (v90): 1 Ladewunsch eines Clients, 2 Host hat keine Kopie
        long long stime = 0;                                  // Speicherzeit (Unix-Sekunden); v76 auch in COOPSYNC.TXT
        std::string run;                                      // v93: Durchlauf (JJJJ-MM-TT_hh-mm-ss_Host_Mitspieler), leer = aelter
    };

    // Eigener Held (v19): alles, was beim Uebernehmen einer fremden Welt erhalten bleibt.
    struct CoopHeroItem {
        std::string inst;
        int amount = 1;
        bool active = false;
    };
    struct CoopHero {
        bool valid = false;
        int atr[NPC_ATR_MAX] = {};
        int level = 0;
        unsigned long exp = 0, expNext = 0, lp = 0;
        int guild = 0, trueGuild = 0;
        int talSkill[NPC_TAL_MAX] = {}, talValue[NPC_TAL_MAX] = {};
        int aiv[50] = {};
        std::vector<CoopHeroItem> items;
        // v22: eigene Lage - sonst steht man nach der Uebernahme dort, wo der Held des anderen stand
        bool hasPos = false;
        float pos[3] = {}, at[3] = {};
        std::string world;
        bool exact = false;   // v89: Lage gehoert zu genau diesem Stand (sonst neben den Helden des Stands, CoopPlaceBesideHost)
    };

    // Uebernahme einer fremden Welt (laeuft ueber mehrere Frames)
    struct CoopTakeJob {
        bool active = false;
        std::string sid, src, name;
        std::string srcpc, srcKey;   // v76: PC der Welt; Schluessel fuer Partnerkopie/Empfang (PC, bei alten Paketen die Rolle)
        std::string mh;              // v76: Manifest-Pruefsumme laut Auftrag (leer = unbekannt)
        bool targeted = false;       // v76: Ladeauftrag nur fuer mich (Verbinden, Fortsetzen, Hauptmenue) - danach eigene Kopie
        int slot = -1;
        int step = 0;                // 0 Sicherung, 1 Dateien, 2 laden
        long long since = 0;
        bool backupWanted = true;
        bool requested = false;
        CoopHero hero;
        std::string run;             // v93: Durchlauf - die Partnerkopie kommt in dessen Archiv-Ordner
    };

    static std::vector<int> CoopSaveSlots;
    static int CoopAutoSaveMinutes = 10;
    static int CoopBackupKey = 0, CoopBackupNextKey = 0, CoopSyncKey = 0;
    static bool CoopTakeOnConnect = true, CoopBackupBeforeLoad = true;
    static int CoopPeerCount = 0;
    static long long CoopLastSaveMs = 0;
    static long long CoopNextAutoTryMs = 0;
    static long long CoopFirstActiveMs = 0;
    static bool CoopConnectTakeSent = false;
    static bool CoopSavingForAuto = false;
    static std::string CoopSavingKind;                // "abgleich" bei Host-Sicherung fuer eine Uebernahme
    static bool CoopSavingTake = false;
    static bool CoopHostSyncPending = false;          // Host: Abgleichs-Sicherung steht an
    static long long CoopHostSyncSince = 0;
    static bool CoopBackupSavePending = false;        // Sicherung vor Uebernahme laeuft
    static std::string CoopActiveRemoteSid;           // gesetzt, waehrend wir auf Wunsch des Partners speichern/laden
    static std::string CoopActiveRemoteKind;

    static int PendingArchiveSlot = -999;
    static long long PendingArchiveMs = 0;
    static std::string PendingArchiveSid, PendingArchiveKind, PendingArchiveTakeSrc;
    static std::string PendingArchiveBy, PendingArchiveByPc, PendingArchiveTakeSrcPc, PendingArchiveTakeMh;   // v76
    static bool PendingArchiveRemote = false, PendingArchiveTake = false;
    static std::string CoopActiveRemoteBy, CoopActiveRemoteByPc;   // v76: Ausloeser beim Mitspeichern (frueher aus RemoteSave)
    static std::string CoopActiveRemoteRun, PendingArchiveRun;     // v93: Durchlauf beim Mitspeichern bzw. des zu archivierenden Stands

    static bool RemoteLoadPending = false;
    static bool RemoteLoadTargeted = false;           // v76: Ladeauftrag mit "to" (nur fuer mich)
    static std::deque<CoopSaveMeta> RemoteSaveQueue;  // v76 (S8): mitzuspeichernde Staende - vorher ein Platz, ein zweiter verdraengte den ersten
    static CoopSaveMeta RemoteSave, RemoteLoad;

    // v76: Pause, Stoerung, Abbruch, Uebernahme beim Verbinden/Fortsetzen (Variante b)
    static bool PausePending = false;                 // Client: Shift+F2 gedrueckt, gemeinsame Sicherung laeuft
    static long long PauseSince = 0;
    static bool Disturbed = false;                    // Client: seit 5 s nichts vom Host (ENet, auch keine Pings)
    static long long DisturbSince = 0;
    static CoopHero DisturbHero;                      // eigener Held bei Beginn der Stoerung
    static CoopHero DivergedHero;                     // eigener Held bei Pausenbeginn bzw. Verbindungsabbruch - gilt bei der naechsten Uebernahme
    static std::string DivergedWhy;
    static bool JoinPending = false;                  // Client: wartet auf den Ladeauftrag des Hosts (Verbinden/Fortsetzen)
    static std::string JoinWhy;
    static long long JoinSince = 0;
    static int JoinTries = 0;
    static bool TakeLoadTargeted = false;             // der laufende Ladevorgang kommt aus einem gezielten Auftrag
    static bool JoinLoadNow = false;                  // Ladevorgang auf Wunsch des Partners laeuft
    static std::string PostTakeSidNext, PostTakeSid;  // nach gezielter Uebernahme: eigene Kopie dieser Kennung speichern
    static int PostTakeSlotNext = -1, PostTakeSlot = -1;
    static std::string PostTakeByNext, PostTakeBy, PostTakeByPcNext, PostTakeByPc, PostTakeNameNext, PostTakeName;
    static long long PostTakeAt = 0;
    static bool CoopDesyncWarn = false;               // Uebernahme gescheitert - Staende weichen ab (Anzeige im Bild)
    static long long CoopTakeErrorMs = 0;
    static bool HostPauseSavePending = false, HostAbortSavePending = false;   // Host: Sicherung fuer Pause / nach Abbruch
    static long long HostPauseSaveUntil = 0, HostAbortSaveUntil = 0;
    static std::string HostPauseSaveFor, HostAbortSaveFor;
    static bool RemoteLoadHostWins = false;   // v29: Client laedt gemeinsamen Stand -> Host laedt seine Kopie, Client uebernimmt sie
    static bool CoopLoadingFromList = false;  // v29: Laden aus der F9-Liste (Read_Savegame ruft Game_LoadBegin synchron)
    static long long RemoteSaveSince = 0;

    static CoopTakeJob TakeJob;
    static std::string CoopTakeError;                 // v40: letzter Fehlschlag einer Uebernahme (Anzeige im Hauptmenue)
    static bool PendingHeroApply = false;
    static bool PendingMoveReset = false;             // v25: Bewegung im ersten Spielbild zuruecksetzen
    static bool PendingHeroLocalOk = false;           // F9-Liste: eigener Ladevorgang mit fremder Welt
    static long long PendingHeroSetMs = 0;            // angewendet wird erst nach einem Ladevorgang danach
    static CoopHero PendingHero;
    static std::string PendingHeroSrc;
    static long long CoopLoadEndMs = 0;

    struct CoopFileIn {
        std::string dir;     // Zielordner (.part)
        int files = 0;       // v71: fertige Dateien (vorher: begonnene - Test v69 zeigte 48 s lang "10 Dateien" bei halber 11.)
        int filesTotal = 0;                        // v71: laut Absender
        long long bytes = 0, total = 0, raw = 0;   // v71: empfangen / gesamt wie gesendet (ggf. gepackt) / entpackt
        long long startMs = 0, lastMs = 0, logMs = 0;
        bool failed = false;
        std::string why;                           // v76: Grund des Fehlschlags
        std::string curFile;                       // v76: laufende Datei - Stuecke muessen lueckenlos der Reihe nach kommen
        int nextIdx = 0;
        bool hasCrc = false;
        unsigned crc = 0;                          // v76: CRC32 und Groesse der Datei laut Absender
        long long fileSize = -1;
    };
    static std::map<std::string, CoopFileIn> FilesIn;   // Schluessel sid|src
    static std::deque<json> FileOutQueue;

    // v71: Clients im Hauptmenue, die den laufenden Stand des Hosts laden (Host im Spiel). Der Host legt dafuer eine
    // gemeinsame Sicherung an und schickt jedem dieser Clients danach einen Ladeauftrag ("to"). Nur fuer sie ausgeloest,
    // speichern die Spielenden nur mit und laden nicht neu (CoopHostSyncMenuOnly).
    // v76: ebenso Clients, die im Spiel verbinden oder nach der Pause fortsetzen (STATE_REQ "connect"/"resume") - ohne Vorab-Frage.
    static std::set<std::string> TakePeers;
    static bool CoopHostSyncMenuOnly = false;   // v76: gilt allgemein "nur gezielt" (Hauptmenue, Verbinden, Fortsetzen)
    static bool CoopSyncSaveMenuOnly = false;   // gilt fuer die gerade geschriebene Sicherung (FinishPendingArchive)
    static long long CoopTakeChatMs = 0;        // v71: Fortschritt im Chat (Uebernahme im Spiel)

    // --- v71: Spielstand-Dateien gepackt uebertragen (Windows-Kompression, cabinet.dll ab Windows 8) ---------------------
    // Test v69 (20.10.): Ein Mitspieler ueber das Internet holte 8,6 MB mit gut 100 KB/s, sah 48 s lang nur "(10 Dateien)"
    // und trennte kurz vor dem Ende. Gemessen an savegame8: XPRESS-Huffman 8,6 -> 3,1 MB (36 %), 29 ms packen, 13 ms entpacken.
    // cabinet.dll wird erst zur Laufzeit geladen (fehlt sie, geht alles ungepackt); gepackt wird nur, wenn der Anfordernde in
    // FILE_REQ "z" meldet.
    typedef BOOL(WINAPI* CoopZipCreateFn)(DWORD, void*, void**);
    typedef BOOL(WINAPI* CoopZipRunFn)(void*, const void*, SIZE_T, void*, SIZE_T, SIZE_T*);
    typedef BOOL(WINAPI* CoopZipCloseFn)(void*);
    struct CoopZipApi {
        bool tried = false;
        CoopZipCreateFn createC = NULL, createD = NULL;
        CoopZipRunFn compress = NULL, decompress = NULL;
        CoopZipCloseFn closeC = NULL, closeD = NULL;
    };
    static CoopZipApi CoopZip;
    static const DWORD COOP_ZIP_ALG = 4;   // COMPRESS_ALGORITHM_XPRESS_HUFF

    static bool CoopZipReady() {
        if (!CoopZip.tried) {
            CoopZip.tried = true;
            HMODULE m = LoadLibraryA("cabinet.dll");
            if (m) {
                CoopZip.createC = (CoopZipCreateFn)GetProcAddress(m, "CreateCompressor");
                CoopZip.createD = (CoopZipCreateFn)GetProcAddress(m, "CreateDecompressor");
                CoopZip.compress = (CoopZipRunFn)GetProcAddress(m, "Compress");
                CoopZip.decompress = (CoopZipRunFn)GetProcAddress(m, "Decompress");
                CoopZip.closeC = (CoopZipCloseFn)GetProcAddress(m, "CloseCompressor");
                CoopZip.closeD = (CoopZipCloseFn)GetProcAddress(m, "CloseDecompressor");
            }
        }
        return CoopZip.createC && CoopZip.createD && CoopZip.compress && CoopZip.decompress && CoopZip.closeC && CoopZip.closeD;
    }

    // Packt nur, wenn es kleiner wird.
    static bool CoopZipPack(const std::vector<std::uint8_t>& in, std::vector<std::uint8_t>& out) {
        if (in.empty() || !CoopZipReady())
            return false;
        void* h = NULL;
        if (!CoopZip.createC(COOP_ZIP_ALG, NULL, &h) || !h)
            return false;
        SIZE_T need = 0;
        CoopZip.compress(h, &in[0], in.size(), NULL, 0, &need);   // Groesse erfragen (FALSE, ERROR_INSUFFICIENT_BUFFER)
        bool ok = false;
        if (need > 0) {
            out.resize(need);
            SIZE_T got = 0;
            ok = CoopZip.compress(h, &in[0], in.size(), &out[0], need, &got) && got > 0 && got < in.size();
            if (ok)
                out.resize(got);
        }
        CoopZip.closeC(h);
        return ok;
    }

    static bool CoopZipUnpack(const std::vector<std::uint8_t>& in, size_t rawSize, std::vector<std::uint8_t>& out) {
        if (in.empty() || !rawSize || !CoopZipReady())
            return false;
        void* h = NULL;
        if (!CoopZip.createD(COOP_ZIP_ALG, NULL, &h) || !h)
            return false;
        out.resize(rawSize);
        SIZE_T got = 0;
        const bool ok = CoopZip.decompress(h, &in[0], in.size(), &out[0], rawSize, &got) && got == rawSize;
        CoopZip.closeD(h);
        return ok;
    }

    static bool CoopReadWholeFile(const std::string& path, std::vector<std::uint8_t>& data) {
        data.clear();
        FILE* f = fopen(path.c_str(), "rb");
        if (!f)
            return false;
        fseek(f, 0, SEEK_END);
        const long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        bool ok = size >= 0;
        if (size > 0) {
            data.resize(size);
            ok = fread(&data[0], 1, size, f) == (size_t)size;
        }
        fclose(f);
        return ok;
    }

    static bool BrowserOpen = false;
    static std::vector<CoopSaveMeta> BrowserList;
    static int BrowserSel = 0;
    static long long BrowserConfirmMs = 0;

    static void SaveSyncLog(const char* fmt, ...) {
        char buf[1024];
        va_list ap;
        va_start(ap, fmt);
        _vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        buf[sizeof(buf) - 1] = 0;
        OthelloCompatLog("[%lld s, %s] Speicher-Sync: %s", DiagSeconds(), DiagRole(), buf);
    }

    // v40: Wartephase im Hauptmenue (CoopMenuLobby.cpp) - dort gibt es noch keine Spielsitzung (ogame = NULL, erst
    // CGameManager::Read_Savegame legt sie an), Ladeauftrag und Dateiuebertragung muessen aber schon laufen.
    static bool CoopSaveSyncInMenu = false;

    // v75: "Spielstaende getrennt" (Einstellungen -> COOP -> Abgleich, Host gibt vor) schaltet alles hier ab - jeder speichert und
    // laedt selbst, nichts wird mitgespeichert, uebernommen, archiviert oder uebertragen.
    static bool SaveSyncAktiv() {
        return (ServerThread || ClientThread) && !WorldEditMode && (ogame || CoopSaveSyncInMenu) && gameMan && !(MyselfId == "_player_")
            && CoopRuleSaves();
    }

    static std::string CoopMe() {
        return std::string(MyselfId.ToChar());
    }

    // --- Pfade und Dateien -----------------------------------------------------------------------

    static std::string CoopSavesDir() {
        static std::string dir;
        if (!dir.empty())
            return dir;
        std::string s = zoptions ? std::string(zoptions->GetDirString(DIR_SAVEGAMES).ToChar()) : std::string();
        if (s.find(':') == std::string::npos) {   // relativ: zum Spielordner (Elternordner von system)
            std::string root = GothicExeFolderPath;
            const size_t p = root.find_last_of("\\/");
            if (p != std::string::npos)
                root = root.substr(0, p);
            s = (!s.empty() && (s[0] == '\\' || s[0] == '/')) ? root + s : root + "\\" + s;
        }
        if (!s.empty() && s[s.size() - 1] != '\\' && s[s.size() - 1] != '/')
            s += '\\';
        dir = s;
        SaveSyncLog("Spielstandordner %s (Archiv: coop_archiv)", dir.c_str());
        return dir;
    }

    static std::string CoopSlotDir(int slot) {
        return CoopSavesDir() + std::string(TSaveLoadGameInfo::GetSaveSlotName(slot).ToChar()) + "\\";
    }

    static std::string CoopArchiveRoot() {
        const std::string a = CoopSavesDir() + "coop_archiv\\";
        CreateDirectoryA(a.c_str(), NULL);
        return a;
    }

    static bool CoopDirExists(const std::string& d) {
        const DWORD a = GetFileAttributesA(d.c_str());
        return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
    }

    static bool CoopFileExists(const std::string& f) {
        const DWORD a = GetFileAttributesA(f.c_str());
        return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
    }

    // --- v93: Durchlauf-Ordner im Archiv -----------------------------------------------------------------------------------
    // Nutzer: "nur die Spielstaende, die bei beiden vorhanden sind ... auch lokale Sicherungen, die aus diesem Playthrough
    // stammten ... am besten Unterordner anlegen mit genauer Bezeichnung, z. B. Timestamp_Name_NameDesMitspielers" (Plan
    // gt-fix\PLAN-SPIELSTAENDE-DURCHLAUF.md). Neue Staende liegen in coop_archiv\<Durchlauf>\<Kennung>\ bzw.
    // ...\<Kennung>__von_<PC>\; ein Durchlauf-Ordner traegt COOPRUN.TXT. Aeltere Staende bleiben direkt in coop_archiv\ (nichts
    // verschoben). Technische Sicherungen (vorUebernahme_, vorher_, rest_) bleiben im Hauptordner. CoopArchiveFind sucht einen
    // Stand ueberall (Verzeichnis gemerkt; fehlt ein Name, hoechstens alle 500 ms neu gelesen), CoopArchivePath bildet den Ort
    // fuer einen neuen. Allgemein GT.
    static std::map<std::string, std::string> CoopArchiveIndex;   // GROSS(Ordnername) -> Ordner mit "\"
    static std::vector<std::string> CoopArchiveRunDirs;            // Durchlauf-Ordner mit "\"
    static bool CoopArchiveIndexValid = false;
    static long long CoopArchiveScanMs = 0;

    static std::string CoopUpperName(const std::string& s) {
        std::string r = s;
        for (auto& c : r)
            c = (char)toupper((unsigned char)c);
        return r;
    }

    static void CoopArchiveScan() {
        CoopArchiveIndex.clear();
        CoopArchiveRunDirs.clear();
        const std::string root = CoopArchiveRoot();
        auto scan = [&](const std::string& base, bool top) {
            WIN32_FIND_DATAA fd;
            HANDLE h = FindFirstFileA((base + "*").c_str(), &fd);
            if (h == INVALID_HANDLE_VALUE)
                return;
            do {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || fd.cFileName[0] == '.')
                    continue;
                const std::string n = fd.cFileName;
                const std::string d = base + n + "\\";
                if (top && CoopFileExists(d + "COOPRUN.TXT")) {
                    CoopArchiveRunDirs.push_back(d);
                    continue;
                }
                CoopArchiveIndex[CoopUpperName(n)] = d;
            } while (FindNextFileA(h, &fd));
            FindClose(h);
        };
        scan(root, true);
        for (size_t i = 0; i < CoopArchiveRunDirs.size(); i++)
            scan(CoopArchiveRunDirs[i], false);
        CoopArchiveIndexValid = true;
        CoopArchiveScanMs = GetCurrentMs();
    }

    // Ordner eines archivierten Stands (Kennung bzw. Kennung__von_PC), leer = gibt es nicht
    static std::string CoopArchiveFind(const std::string& name) {
        if (!CoopArchiveIndexValid)
            CoopArchiveScan();
        auto it = CoopArchiveIndex.find(CoopUpperName(name));
        if (it != CoopArchiveIndex.end() && CoopDirExists(it->second))
            return it->second;
        if (GetCurrentMs() > CoopArchiveScanMs + 500) {   // neu entstanden oder verschoben?
            CoopArchiveScan();
            it = CoopArchiveIndex.find(CoopUpperName(name));
            if (it != CoopArchiveIndex.end() && CoopDirExists(it->second))
                return it->second;
        }
        return std::string();
    }

    // Ort fuer einen neuen Archivstand: im Ordner des Durchlaufs, ohne Durchlauf bzw. fuer technische Sicherungen im Hauptordner
    static std::string CoopArchivePath(const std::string& name, const std::string& run) {
        const bool technical = !name.compare(0, 14, "vorUebernahme_") || !name.compare(0, 7, "vorher_") || !name.compare(0, 5, "rest_");
        return CoopArchiveRoot() + (run.empty() || technical ? std::string() : run + "\\") + name + "\\";
    }

    // Kopie der Welt eines anderen PCs fuer eine Kennung - vorhandene ueberall, neue im Ordner des Durchlaufs
    static std::string CoopPartnerCopyDir(const std::string& sid, const std::string& src, const std::string& run = std::string()) {
        const std::string name = sid + "__von_" + src;
        const std::string found = CoopArchiveFind(name);
        return !found.empty() ? found : CoopArchivePath(name, run);
    }

    static std::string SaveSyncUpper(const std::string& s) {
        std::string r = s;
        for (auto& c : r)
            c = (char)toupper((unsigned char)c);
        return r;
    }

    // v76 (S11): Text ueber <Datei>.tmp schreiben und dann ersetzen - ein Absturz mitten im Schreiben laesst die alte Datei stehen
    static bool CoopWriteTextAtomic(const std::string& path, const std::string& text) {
        const std::string tmp = path + ".tmp";
        FILE* f = fopen(tmp.c_str(), "wb");
        if (!f)
            return false;
        const bool ok = fwrite(text.data(), 1, text.size(), f) == text.size() && fflush(f) == 0;
        fclose(f);
        if (!ok || !MoveFileExA(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            DeleteFileA(tmp.c_str());
            return false;
        }
        return true;
    }

    static long long CoopUnixNow() {
        return (long long)time(NULL);
    }

    // Alle Dateien (flach) kopieren. Rueckgabe: Anzahl, -1 bei Fehler.
    static int CoopCopyDir(const std::string& from, const std::string& to) {
        CreateDirectoryA(to.c_str(), NULL);
        WIN32_FIND_DATAA fd;
        HANDLE h = FindFirstFileA((from + "*").c_str(), &fd);
        if (h == INVALID_HANDLE_VALUE)
            return -1;
        int n = 0;
        bool ok = true;
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;
            if (CopyFileA((from + fd.cFileName).c_str(), (to + fd.cFileName).c_str(), FALSE))
                n++;
            else
                ok = false;
        } while (FindNextFileA(h, &fd));
        FindClose(h);
        return ok ? n : -1;
    }

    static bool CoopReadMeta(const std::string& dir, CoopSaveMeta& m) {
        FILE* f = fopen((dir + "COOPSYNC.TXT").c_str(), "r");
        if (!f)
            return false;
        char line[512];
        int ver = 1;
        bool end = false;
        while (fgets(line, sizeof(line), f)) {
            std::string l = line;
            while (!l.empty() && (l[l.size() - 1] == '\n' || l[l.size() - 1] == '\r'))
                l.erase(l.size() - 1);
            const size_t eq = l.find('=');
            if (eq == std::string::npos)
                continue;
            const std::string k = l.substr(0, eq), v = l.substr(eq + 1);
            if (k == "sid") m.sid = v;
            else if (k == "kind") m.kind = v;
            else if (k == "name") m.name = v;
            else if (k == "by") m.by = v;
            else if (k == "world") m.world = v;
            else if (k == "time") m.time = v;
            else if (k == "src") m.src = v;
            else if (k == "slot") m.slot = atoi(v.c_str());
            else if (k == "srcpc") m.srcpc = v;      // v76
            else if (k == "bypc") m.bypc = v;
            else if (k == "srcname") m.srcname = v;
            else if (k == "stime") m.stime = _atoi64(v.c_str());
            else if (k == "run" && v.find_first_of("\\/:*?\"<>|") == std::string::npos && v.find("..") == std::string::npos)
                m.run = v;   // v93: wird ein Ordnername - nur sichere Zeichen
            else if (k == "v") ver = atoi(v.c_str());
            else if (k == "end") end = true;
        }
        fclose(f);
        // v17-Staende haben kein src: das ist immer die eigene Welt des PCs, auf dem sie liegen
        // ("by" war der Ausloeser, nicht der Besitzer der Welt).
        // v76: ab Fassung 2 mit Endmarke - eine halb geschriebene Datei gilt als fehlend (der Slot wird dann vorher gesichert)
        if (m.sid.empty() || (ver >= 2 && !end)) {
            const int slot = m.slot;
            m = CoopSaveMeta();
            m.slot = slot;
            return false;
        }
        return true;
    }

    static void CoopWriteMeta(const std::string& dir, const CoopSaveMeta& m) {
        char b[2048];
        _snprintf(b, sizeof(b), "v=2\nsid=%s\nkind=%s\nname=%s\nby=%s\nworld=%s\ntime=%s\nsrc=%s\nslot=%d\nsrcpc=%s\nbypc=%s\nsrcname=%s\nstime=%lld\nrun=%s\nend=1\n",
            m.sid.c_str(), m.kind.c_str(), m.name.c_str(), m.by.c_str(), m.world.c_str(), m.time.c_str(), m.src.c_str(), m.slot,
            m.srcpc.c_str(), m.bypc.c_str(), m.srcname.c_str(), m.stime, m.run.c_str());
        b[sizeof(b) - 1] = 0;
        CoopWriteTextAtomic(dir + "COOPSYNC.TXT", b);
    }

    // v55: letzte Aenderung einer Datei in Unix-Sekunden (0 = fehlt)
    static long long CoopFileUnixTime(const std::string& f) {
        WIN32_FILE_ATTRIBUTE_DATA a;
        if (!GetFileAttributesExA(f.c_str(), GetFileExInfoStandard, &a))
            return 0;
        ULARGE_INTEGER u;
        u.LowPart = a.ftLastWriteTime.dwLowDateTime;
        u.HighPart = a.ftLastWriteTime.dwHighDateTime;
        if (u.QuadPart < 116444736000000000ULL)
            return 0;
        return (long long)((u.QuadPart - 116444736000000000ULL) / 10000000ULL);
    }

    static std::string CoopNowText(const char* fmt) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        char b[64];
        if (!strcmp(fmt, "sid"))
            _snprintf(b, sizeof(b), "%04d-%02d-%02d_%02d-%02d-%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        else
            _snprintf(b, sizeof(b), "%02d.%02d.%04d %02d:%02d", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute);
        b[sizeof(b) - 1] = 0;
        return b;
    }

    static std::string CoopSlotName(int slot) {
        oCSavegameInfo* info = ogame && ogame->savegameManager ? ogame->savegameManager->GetSavegame(slot) : NULL;
        return info ? std::string(info->m_Name.ToChar()) : std::string();
    }

    // --- v76: feste PC-Kennung -----------------------------------------------------------------------------------------
    // Rollen (HOST/FRIEND_n) wechseln mit jedem Verbinden und Rollentausch; gespeichert wird darum die Kennung des PCs. Sie liegt im
    // Archiv (nicht in Gothic.ini - die schreibt Gothic erst beim Beenden, nach einem Absturz waere sie weg).

    static std::string CoopPcIdValue;
    static std::map<std::string, std::string> CoopPcRole;   // PC-Kennung -> aktuelle Spieler-Kennung (aus Speicher-Paketen und hello)

    static bool CoopPcIdValid(const std::string& s) {
        if (s.size() != 12)
            return false;
        for (char c : s)
            if (!isxdigit((unsigned char)c))
                return false;
        return true;
    }

    static std::string CoopMyPc() {
        if (!CoopPcIdValue.empty())
            return CoopPcIdValue;
        const std::string file = CoopArchiveRoot() + "COOPPC.TXT";
        if (FILE* f = fopen(file.c_str(), "r")) {
            char line[64] = {};
            if (fgets(line, sizeof(line), f)) {
                std::string s = line;
                while (!s.empty() && (s[s.size() - 1] == '\n' || s[s.size() - 1] == '\r' || s[s.size() - 1] == ' '))
                    s.erase(s.size() - 1);
                if (CoopPcIdValid(s))
                    CoopPcIdValue = SaveSyncUpper(s);
            }
            fclose(f);
        }
        if (CoopPcIdValue.empty()) {
            unsigned long long h = 1469598103934665603ULL;   // FNV-1a ueber Zeit, Zaehler, Prozess und Rechnername
            auto mix = [&](const void* p, size_t len) {
                const unsigned char* b = (const unsigned char*)p;
                for (size_t i = 0; i < len; i++) {
                    h ^= b[i];
                    h *= 1099511628211ULL;
                }
            };
            LARGE_INTEGER q;
            QueryPerformanceCounter(&q);
            mix(&q, sizeof(q));
            FILETIME ft;
            GetSystemTimeAsFileTime(&ft);
            mix(&ft, sizeof(ft));
            const DWORD pid = GetCurrentProcessId();
            mix(&pid, sizeof(pid));
            char comp[MAX_COMPUTERNAME_LENGTH + 1] = {};
            DWORD n = sizeof(comp);
            if (GetComputerNameA(comp, &n))
                mix(comp, n);
            char b[32];
            _snprintf(b, sizeof(b), "%012llX", h & 0xFFFFFFFFFFFFULL);
            b[sizeof(b) - 1] = 0;
            CoopPcIdValue = b;
            const bool ok = CoopWriteTextAtomic(file, CoopPcIdValue + "\n");
            SaveSyncLog("neue PC-Kennung %s (coop_archiv\\COOPPC.TXT%s)", CoopPcIdValue.c_str(), ok ? "" : " - SCHREIBEN FEHLGESCHLAGEN");
        }
        return CoopPcIdValue;
    }

    static void CoopNotePc(const std::string& pc, const std::string& role) {
        if (pc.empty() || role.empty() || pc == CoopMyPc())
            return;
        auto it = CoopPcRole.find(pc);
        if (it != CoopPcRole.end() && it->second == role)
            return;
        for (auto i = CoopPcRole.begin(); i != CoopPcRole.end();) {   // dieselbe Rolle hatte vorher ein anderer PC
            if (i->second == role && i->first != pc)
                i = CoopPcRole.erase(i);
            else
                ++i;
        }
        CoopPcRole[pc] = role;
        SaveSyncLog("PC %s spielt als %s", pc.c_str(), role.c_str());
    }

    // Aktuelle Spieler-Kennung eines PCs (leer = nicht verbunden bzw. unbekannt)
    static std::string CoopRoleOfPc(const std::string& pc) {
        if (pc.empty())
            return std::string();
        if (pc == CoopMyPc())
            return CoopMe();
        auto it = CoopPcRole.find(pc);
        return it != CoopPcRole.end() ? it->second : std::string();
    }

    // Gehoert die Welt dieses Stands diesem PC? Ab v76 nach PC-Kennung; aeltere Staende (ohne srcpc) wie bisher nach Rolle.
    static bool CoopMetaMine(const CoopSaveMeta& m) {
        if (!m.srcpc.empty())
            return m.srcpc == CoopMyPc();
        return m.src.empty() || m.src == CoopMe();
    }

    // Schluessel der Partnerkopie <Kennung>__von_<Schluessel>: PC-Kennung, bei Staenden vor v76 die Rolle
    static std::string CoopSrcKey(const CoopSaveMeta& m) {
        return !m.srcpc.empty() ? m.srcpc : m.src;
    }

    // --- v76: Manifest (COOPFILES.TXT) - Groesse und CRC32 jeder Spieldatei eines Stands ------------------------------------

    static unsigned CoopCrc32(const std::uint8_t* p, size_t n, unsigned crc) {
        static unsigned table[256];
        static bool init = false;
        if (!init) {
            for (unsigned i = 0; i < 256; i++) {
                unsigned c = i;
                for (int k = 0; k < 8; k++)
                    c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
                table[i] = c;
            }
            init = true;
        }
        crc = ~crc;
        for (size_t i = 0; i < n; i++)
            crc = table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
        return ~crc;
    }

    static bool CoopFileCrc(const std::string& path, long long& size, unsigned& crc) {
        size = 0;
        crc = 0;
        FILE* f = fopen(path.c_str(), "rb");
        if (!f)
            return false;
        std::vector<std::uint8_t> buf(256 * 1024);
        size_t got;
        while ((got = fread(&buf[0], 1, buf.size(), f)) > 0) {
            crc = CoopCrc32(&buf[0], got, crc);
            size += (long long)got;
        }
        const bool ok = !ferror(f);
        fclose(f);
        return ok;
    }

    // Spieldateien: alles ausser GTs eigenen Dateien (COOP*.TXT) und Resten (*.tmp)
    static bool CoopIsGameFile(const std::string& name) {
        const std::string u = SaveSyncUpper(name);
        if (u.compare(0, 4, "COOP") == 0)
            return false;
        return !(u.size() > 4 && u.compare(u.size() - 4, 4, ".TMP") == 0);
    }

    struct CoopManifestFile {
        long long size = 0;
        unsigned crc = 0;
    };
    typedef std::map<std::string, CoopManifestFile> CoopManifest;   // Dateiname (gross) -> Groesse, CRC32

    static bool CoopManifestBuild(const std::string& dir, CoopManifest& m) {
        m.clear();
        WIN32_FIND_DATAA fd;
        HANDLE h = FindFirstFileA((dir + "*").c_str(), &fd);
        if (h == INVALID_HANDLE_VALUE)
            return false;
        bool ok = true;
        do {
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || !CoopIsGameFile(fd.cFileName))
                continue;
            CoopManifestFile e;
            if (!CoopFileCrc(dir + fd.cFileName, e.size, e.crc))
                ok = false;
            m[SaveSyncUpper(fd.cFileName)] = e;
        } while (FindNextFileA(h, &fd));
        FindClose(h);
        return ok && !m.empty();
    }

    static std::string CoopManifestHash(const CoopManifest& m) {
        std::string s;
        for (auto const& e : m) {
            char b[300];
            _snprintf(b, sizeof(b), "%s|%lld|%08X\n", e.first.c_str(), e.second.size, e.second.crc);
            b[sizeof(b) - 1] = 0;
            s += b;
        }
        char h[16];
        _snprintf(h, sizeof(h), "%08X", CoopCrc32((const std::uint8_t*)s.data(), s.size(), 0));
        h[sizeof(h) - 1] = 0;
        return h;
    }

    static bool CoopManifestWrite(const std::string& dir, const CoopManifest& m) {
        std::string s = "v=1\n";
        for (auto const& e : m) {
            char b[300];
            _snprintf(b, sizeof(b), "f=%s|%lld|%08X\n", e.first.c_str(), e.second.size, e.second.crc);
            b[sizeof(b) - 1] = 0;
            s += b;
        }
        s += "end=1\n";
        return CoopWriteTextAtomic(dir + "COOPFILES.TXT", s);
    }

    static bool CoopManifestRead(const std::string& dir, CoopManifest& m) {
        m.clear();
        FILE* f = fopen((dir + "COOPFILES.TXT").c_str(), "r");
        if (!f)
            return false;
        char line[400];
        bool end = false;
        while (fgets(line, sizeof(line), f)) {
            std::string l = line;
            while (!l.empty() && (l[l.size() - 1] == '\n' || l[l.size() - 1] == '\r'))
                l.erase(l.size() - 1);
            if (l == "end=1") {
                end = true;
                continue;
            }
            if (l.compare(0, 2, "f=") != 0)
                continue;
            const size_t a = l.find('|', 2);
            const size_t b = a == std::string::npos ? a : l.find('|', a + 1);
            if (b == std::string::npos)
                continue;
            CoopManifestFile e;
            e.size = _atoi64(l.substr(a + 1, b - a - 1).c_str());
            e.crc = strtoul(l.substr(b + 1).c_str(), NULL, 16);
            m[SaveSyncUpper(l.substr(2, a - 2))] = e;
        }
        fclose(f);
        if (!end || m.empty()) {
            m.clear();
            return false;
        }
        return true;
    }

    // Manifest lesen; fehlt es (Stand vor v76), aus den Dateien bilden und dazuschreiben
    static bool CoopManifestEnsure(const std::string& dir, CoopManifest& m) {
        if (CoopManifestRead(dir, m))
            return true;
        if (!CoopManifestBuild(dir, m))
            return false;
        CoopManifestWrite(dir, m);
        return true;
    }

    // Stimmen die Spieldateien in dir mit dem Manifest ueberein - keine fehlt, keine zu viel, Groesse und CRC gleich?
    static bool CoopManifestCheck(const std::string& dir, const CoopManifest& want, std::string& why) {
        CoopManifest have;
        if (!CoopManifestBuild(dir, have) && have.empty()) {
            why = "keine lesbaren Spieldateien";
            return false;
        }
        for (auto const& e : want) {
            auto it = have.find(e.first);
            if (it == have.end()) {
                why = e.first + " fehlt";
                return false;
            }
            if (it->second.size != e.second.size || it->second.crc != e.second.crc) {
                why = e.first + " weicht ab (Groesse/Pruefsumme)";
                return false;
            }
        }
        for (auto const& e : have)
            if (!want.count(e.first)) {
                why = e.first + " gehoert nicht dazu";
                return false;
            }
        return true;
    }

    // Nur GTs eigene Zwischenordner (*.part, *.neu, *.alt) leeren und entfernen - nie einen Slot oder Archivstand
    static void CoopRemoveTempDir(const std::string& dirWithSlash) {
        std::string d = dirWithSlash;
        if (!d.empty() && d[d.size() - 1] == '\\')
            d.erase(d.size() - 1);
        const std::string u = SaveSyncUpper(d);
        auto ends = [&](const char* x) { const size_t n = strlen(x); return u.size() > n && u.compare(u.size() - n, n, x) == 0; };
        if (!ends(".PART") && !ends(".NEU") && !ends(".ALT"))
            return;
        WIN32_FIND_DATAA fd;
        HANDLE h = FindFirstFileA((d + "\\*").c_str(), &fd);
        if (h != INVALID_HANDLE_VALUE) {
            do {
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    continue;
                const std::string f = d + "\\" + fd.cFileName;
                SetFileAttributesA(f.c_str(), FILE_ATTRIBUTE_NORMAL);
                DeleteFileA(f.c_str());
            } while (FindNextFileA(h, &fd));
            FindClose(h);
        }
        RemoveDirectoryA(d.c_str());
    }

    // Ordner beiseitelegen statt loeschen: <Ordner>_<Grund>_<Zeit>.part (Listen zeigen .part nicht, die Daten bleiben)
    static void CoopSetAside(const std::string& dirWithSlash, const char* why) {
        std::string d = dirWithSlash;
        if (!d.empty() && d[d.size() - 1] == '\\')
            d.erase(d.size() - 1);
        const std::string to = d + "_" + why + "_" + CoopNowText("sid") + ".part";
        const bool ok = MoveFileA(d.c_str(), to.c_str()) != 0;
        CoopArchiveIndexValid = false;   // v93
        SaveSyncLog("%s beiseitegelegt (%s)%s", d.c_str(), why, ok ? "" : " - UMBENENNEN FEHLGESCHLAGEN");
    }

    // v76 (S13): neue eigene Kennung - sekundengenau wie bisher, bei Gleichstand mit Zaehler. v93: auch in Durchlauf-Ordnern pruefen
    static std::string CoopNewSid(const std::string& prefix) {
        const std::string root = CoopArchiveRoot();
        const std::string base = prefix + CoopNowText("sid") + "_" + CoopMe();
        std::string sid = base;
        for (int k = 2; !CoopArchiveFind(sid).empty() || !CoopArchiveFind(sid + ".part").empty() || CoopDirExists(root + sid + "\\"); k++)
            sid = base + "-" + std::to_string(k);
        return sid;
    }

    // --- v93: Durchlauf -------------------------------------------------------------------------------------------------------
    // Kennung JJJJ-MM-TT_hh-mm-ss_<Host>_<Mitspieler> (Namen nur Buchstaben, Ziffern, -). Der Host legt sie beim gemeinsamen
    // neuen Spiel an (CoopNewGame.cpp schickt sie mit LOBBY newgame) bzw. bei der ersten gemeinsamen Sicherung eines Stands ohne
    // Durchlauf. Laden setzt den Durchlauf des Stands, der Mitspieler uebernimmt ihn aus den Paketen des Hosts.
    static std::string CoopCurrentRun;
    static std::string CoopPartnerNick;   // Spielername des Partners (Pakete: srcname, LOBBY hello nick)

    static std::string CoopRunNamePart(const std::string& s) {
        std::string r;
        for (char c : s)
            if (isalnum((unsigned char)c) || c == '-' || (unsigned char)c >= 0xC0)
                r += c;
        if (r.size() > 24)
            r.resize(24);
        return r.empty() ? std::string("Spieler") : r;
    }

    static std::string CoopRunNew(const std::string& hostName, const std::string& partnerName) {
        return CoopNowText("sid") + "_" + CoopRunNamePart(hostName) + "_" + CoopRunNamePart(partnerName);
    }

    static void CoopRunSet(const std::string& run, const char* why) {
        if (run == CoopCurrentRun)
            return;
        SaveSyncLog("Durchlauf %s (%s, vorher %s)", run.empty() ? "-" : run.c_str(), why, CoopCurrentRun.empty() ? "-" : CoopCurrentRun.c_str());
        CoopCurrentRun = run;
    }

    // Ordner eines Durchlaufs mit COOPRUN.TXT anlegen (einmal)
    static std::string CoopRunPcOf(const std::string& role) {   // PC-Kennung einer Rolle (ich selbst oder aus den Paketen)
        if (role == CoopMe())
            return CoopMyPc();
        for (auto const& e : CoopPcRole)
            if (e.second == role)
                return e.first;
        return std::string();
    }

    static void CoopRunFolderEnsure(const std::string& run) {
        if (run.empty())
            return;
        const std::string d = CoopArchiveRoot() + run + "\\";
        if (CoopFileExists(d + "COOPRUN.TXT"))
            return;
        CreateDirectoryA(d.c_str(), NULL);
        std::string host, partner;   // Namen aus der Kennung: Datum_Zeit_Host_Mitspieler
        const size_t a = run.find('_', 11);
        const size_t b = a == std::string::npos ? a : run.find('_', a + 1);
        if (b != std::string::npos) {
            host = run.substr(a + 1, b - a - 1);
            partner = run.substr(b + 1);
        }
        std::string s = "v=1\nrun=" + run + "\nhost=" + host + "\npartner=" + partner + "\nhostpc=" + CoopRunPcOf("HOST") +
            "\nangelegt=" + CoopNowText("time") + "\nvon=" + CoopMyPc() + "\nend=1\n";
        CoopWriteTextAtomic(d + "COOPRUN.TXT", s);
        CoopArchiveIndexValid = false;
        SaveSyncLog("Durchlauf-Ordner angelegt: coop_archiv\\%s", run.c_str());
    }

    // v76 (S2, S13): Archivstand einmal schreiben und pruefen - erst nach <Kennung>.part kopieren, Manifest der Quelle dazu
    // (meta: COOPSYNC.TXT dieses Archivstands), die Kopie gegen das Manifest pruefen, dann umbenennen. Gibt es den Stand schon,
    // bleibt er unveraendert. Rueckgabe: Dateien (vorhandener Stand: seine Dateien), -1 Fehler; mh = Manifest-Pruefsumme.
    // v93: neuer Stand in den Ordner seines Durchlaufs (run aus meta bzw. COOPSYNC.TXT der Quelle; ohne: Hauptordner)
    static int CoopArchiveDir(const std::string& from, const std::string& sid, const CoopSaveMeta* meta, std::string* mh) {
        const std::string existing = CoopArchiveFind(sid);
        if (!existing.empty()) {
            CoopManifest old;
            if (mh && CoopManifestEnsure(existing, old))
                *mh = CoopManifestHash(old);
            SaveSyncLog("Archiv %s gibt es schon - bleibt unveraendert", sid.c_str());
            return (int)old.size() > 0 ? (int)old.size() : 1;
        }
        std::string run = meta ? meta->run : std::string();
        if (!meta) {
            CoopSaveMeta src;
            if (CoopReadMeta(from, src))
                run = src.run;
        }
        std::string target = CoopArchivePath(sid, run);
        target.erase(target.size() - 1);   // ohne "\" (wie bisher)
        if (!run.empty() && target != CoopArchiveRoot() + sid)   // nicht im Hauptordner (technische Sicherung)
            CoopRunFolderEnsure(run);
        CoopManifest m;
        if (!CoopManifestBuild(from, m)) {
            SaveSyncLog("Archiv %s: Spieldateien in %s nicht lesbar", sid.c_str(), from.c_str());
            return -1;
        }
        const std::string part = target + ".part\\";
        CoopRemoveTempDir(part);   // Rest eines frueheren Versuchs
        const int n = CoopCopyDir(from, part);
        std::string why;
        if (n <= 0)
            why = "Kopie fehlgeschlagen";
        else {
            if (meta)
                CoopWriteMeta(part, *meta);
            if (!CoopManifestWrite(part, m))
                why = "Manifest nicht schreibbar";
            else
                CoopManifestCheck(part, m, why);
        }
        if (why.empty() && !MoveFileA((target + ".part").c_str(), target.c_str()))
            why = "Umbenennen fehlgeschlagen";
        CoopArchiveIndexValid = false;   // v93
        if (!why.empty()) {
            SaveSyncLog("Archiv %s FEHLGESCHLAGEN: %s (Rest bleibt in %s.part)", sid.c_str(), why.c_str(), sid.c_str());
            return -1;
        }
        if (!run.empty())
            SaveSyncLog("Archiv %s im Durchlauf %s", sid.c_str(), run.c_str());
        if (mh)
            *mh = CoopManifestHash(m);
        return n;
    }

    // Liegt der Stand eines Slots schon im Archiv (eigene Kopie oder Partnerkopie seiner Kennung)? v93: auch in Durchlauf-Ordnern
    static bool CoopSidArchived(const CoopSaveMeta& m) {
        if (m.sid.empty())
            return false;
        if (CoopMetaMine(m))
            return !CoopArchiveFind(m.sid).empty();
        return !CoopArchiveFind(m.sid + "__von_" + CoopSrcKey(m)).empty();
    }

    // Vor dem Ueberschreiben sichern, was nicht im Archiv liegt: Stand ohne Kennung (z. B. allein gespeichert) und - v76 - auch
    // einen Stand mit Kennung, deren Archiv fehlt (z. B. Archiv fehlgeschlagen).
    static void CoopBackupForeignSlot(int slot) {
        const std::string dir = CoopSlotDir(slot);
        if (!CoopDirExists(dir) || !CoopFileExists(dir + "SAVEINFO.SAV"))
            return;
        CoopSaveMeta m;
        const bool meta = CoopReadMeta(dir, m);
        if (meta && CoopSidArchived(m))
            return;
        CoopSaveMeta b = meta ? m : CoopSaveMeta();
        b.sid = CoopNewSid("vorher_slot" + std::to_string(slot) + "_");
        b.kind = "vorher";
        b.name = CoopSlotName(slot);
        b.by = CoopMe();
        b.bypc = CoopMyPc();
        if (!meta) {
            b.src = CoopMe();
            b.srcpc = CoopMyPc();
            b.srcname = MyNickname.ToChar();
            b.stime = CoopFileUnixTime(dir + "SAVEINFO.SAV");
        }
        b.time = CoopNowText("time");
        b.slot = slot;
        const int n = CoopArchiveDir(dir, b.sid, &b, NULL);
        SaveSyncLog("Slot %d enthielt einen Stand %s - vorher archiviert als %s (%d Dateien)", slot,
            meta ? ("ohne Archiv (" + m.sid + ")").c_str() : "ohne Kennung", b.sid.c_str(), n);
    }

    // Alle Dateien eines Slots entfernen (nur im Rueckfall von CoopSlotReplace - sein Inhalt ist dann im Archiv, wie bei Gothics
    // eigenem Dir_Clear vor dem Speichern)
    static void CoopClearSlotFiles(int slot) {
        const std::string d = CoopSlotDir(slot);
        WIN32_FIND_DATAA fd;
        HANDLE h = FindFirstFileA((d + "*").c_str(), &fd);
        if (h == INVALID_HANDLE_VALUE)
            return;
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;
            const std::string f = d + fd.cFileName;
            SetFileAttributesA(f.c_str(), FILE_ATTRIBUTE_NORMAL);
            DeleteFileA(f.c_str());
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }

    // v76 (S4): Slot sicher ersetzen. Wie Gothic (Dir_Clear vor dem Kopieren) bleibt nichts Altes liegen - etwa die Datei eines Levels,
    // den der neue Stand nie betreten hat -, und ein Absturz mitten im Kopieren laesst den alten Slot stehen. Ablauf: nicht
    // archivierten Inhalt sichern (vorher_...), neue Dateien nach savegameN.neu, gegen das Manifest der Quelle pruefen, eigene
    // Zusaetze (extra: Kennung, Held, Schnellleiste), dann savegameN -> .alt und .neu -> savegameN. Rueckgabe: Dateien, -1 Fehler.
    static int CoopSlotReplace(int slot, const std::string& from, const std::function<void(const std::string&)>& extra, std::string& why) {
        CoopManifest m;
        if (!CoopManifestEnsure(from, m)) {
            why = "Quelle ohne lesbare Spieldateien";
            return -1;
        }
        std::string base = CoopSlotDir(slot);
        base.erase(base.size() - 1);
        const std::string neu = base + ".neu", alt = base + ".alt";
        CoopBackupForeignSlot(slot);
        CoopRemoveTempDir(neu + "\\");
        if (CoopDirExists(alt + "\\")) {   // Rest eines abgebrochenen Tauschs: nie verwerfen, ins Archiv schieben
            const std::string rest = CoopArchiveRoot() + "rest_slot" + std::to_string(slot) + "_" + CoopNowText("sid") + ".part";
            MoveFileA(alt.c_str(), rest.c_str());
            SaveSyncLog("Rest eines frueheren Slot-Tauschs nach %s verschoben", rest.c_str());
        }
        const int n = CoopCopyDir(from, neu + "\\");
        if (n <= 0)
            why = "Kopie fehlgeschlagen";
        else
            CoopManifestCheck(neu + "\\", m, why);
        if (!why.empty()) {
            CoopRemoveTempDir(neu + "\\");
            return -1;
        }
        DeleteFileA((neu + "\\COOPFILES.TXT").c_str());   // im Slot ohne Manifest (extra aendert z. B. die Schnellleiste)
        if (extra)
            extra(neu + "\\");
        const bool hadSlot = CoopDirExists(base + "\\");
        if (hadSlot && !MoveFileA(base.c_str(), alt.c_str())) {
            // Slot laesst sich nicht umbenennen (Datei offen?) - wie Gothic leeren und hineinkopieren
            SaveSyncLog("Slot %d: Tausch nicht moeglich (Fehler %lu) - leeren und kopieren", slot, GetLastError());
            CoopClearSlotFiles(slot);
            const int k = CoopCopyDir(neu + "\\", base + "\\");
            CoopRemoveTempDir(neu + "\\");
            if (k <= 0) {
                why = "Kopie in den Slot fehlgeschlagen";
                return -1;
            }
            return k;
        }
        if (!MoveFileA(neu.c_str(), base.c_str())) {
            why = "Umbenennen fehlgeschlagen";
            if (hadSlot)
                MoveFileA(alt.c_str(), base.c_str());
            return -1;
        }
        if (hadSlot)
            CoopRemoveTempDir(alt + "\\");
        return n;
    }

    // Reservierter Slot fuer die naechste Auto-Sicherung: der am laengsten nicht beschriebene.
    static int CoopNextReservedSlot() {
        int best = CoopSaveSlots.empty() ? 9 : CoopSaveSlots[0];
        FILETIME bestTime = { 0xFFFFFFFF, 0xFFFFFFFF };
        for (int s : CoopSaveSlots) {
            WIN32_FILE_ATTRIBUTE_DATA a;
            if (!GetFileAttributesExA((CoopSlotDir(s) + "SAVEINFO.SAV").c_str(), GetFileExInfoStandard, &a))
                return s;   // leer
            if (CompareFileTime(&a.ftLastWriteTime, &bestTime) < 0) {
                bestTime = a.ftLastWriteTime;
                best = s;
            }
        }
        return best;
    }

    // --- Held sichern / wiederherstellen (v19) -----------------------------------------------------

    static void CoopForEachInvItem(std::vector<oCItem*>& out) {
        out.clear();
        player->inventory2.UnpackAllItems();
        for (int c = 0; c < INV_MAX; c++) {
            for (zCListSort<oCItem>* n = player->inventory2.inventory[c].next; n; n = n->next)
                if (n->data)
                    out.push_back(n->data);
        }
    }

    static CoopHero CoopHeroCapture() {
        CoopHero h;
        if (!player)
            return h;
        for (int a = 0; a < NPC_ATR_MAX; a++)
            h.atr[a] = player->GetAttribute(a);
        h.level = player->level;
        h.exp = player->experience_points;
        h.expNext = player->experience_points_next_level;
        h.lp = player->learn_points;
        h.guild = player->GetGuild();
        h.trueGuild = player->GetTrueGuild();   // Othello verlegt das Feld (0x7A6 -> 0xA34), die Funktion ist angepasst
        for (int t = 1; t < NPC_TAL_MAX; t++) {
            h.talSkill[t] = player->GetTalentSkill(t);
            h.talValue[t] = player->GetTalentValue(t);
        }
        for (int i = 0; i < 50; i++)
            h.aiv[i] = player->aiscriptvars[i];
        const zVEC3 p = player->GetPositionWorld();
        const zVEC3 a = player->GetAtVectorWorld();
        for (int k = 0; k < 3; k++) {
            h.pos[k] = p.n[k];
            h.at[k] = a.n[k];
        }
        h.world = CoopWorldName();
        h.hasPos = !h.world.empty();
        std::vector<oCItem*> inv;
        CoopForEachInvItem(inv);
        for (oCItem* it : inv) {
            CoopHeroItem e;
            e.inst = CoopItemInst(it);
            e.amount = CoopItemAmount(it);
            e.active = it->HasFlag(ITM_FLAG_ACTIVE) != 0;
            h.items.push_back(e);
        }
        h.valid = true;
        h.exact = true;   // v89: aktuelle Lage im laufenden Spiel
        return h;
    }

    // v76 (S11): ueber .tmp geschrieben, Fassung 2 mit Endmarke - ein halber Held (Gegenstaende fehlen) wird nie eingesetzt
    static void CoopHeroWrite(const std::string& dir, const CoopHero& h) {
        if (!h.valid)
            return;
        std::string s = "v=2\natr=";
        char b[400];
        for (int a = 0; a < NPC_ATR_MAX; a++) {
            _snprintf(b, sizeof(b), "%d%s", h.atr[a], a + 1 < NPC_ATR_MAX ? "," : "\n");
            s += b;
        }
        _snprintf(b, sizeof(b), "level=%d\nexp=%lu\nexpnext=%lu\nlp=%lu\nguild=%d\ntrueguild=%d\ntal=", h.level, h.exp, h.expNext, h.lp, h.guild, h.trueGuild);
        s += b;
        for (int t = 1; t < NPC_TAL_MAX; t++) {
            _snprintf(b, sizeof(b), "%d:%d:%d;", t, h.talSkill[t], h.talValue[t]);
            s += b;
        }
        s += "\naiv=";
        for (int i = 0; i < 50; i++) {
            _snprintf(b, sizeof(b), "%d%s", h.aiv[i], i + 1 < 50 ? "," : "\n");
            s += b;
        }
        if (h.hasPos) {
            _snprintf(b, sizeof(b), "pos=%.1f,%.1f,%.1f\nat=%.4f,%.4f,%.4f\nworld=%s\n", h.pos[0], h.pos[1], h.pos[2], h.at[0], h.at[1], h.at[2], h.world.c_str());
            b[sizeof(b) - 1] = 0;
            s += b;
        }
        for (auto const& e : h.items) {
            _snprintf(b, sizeof(b), "item=%s %d %d\n", e.inst.c_str(), e.amount, e.active ? 1 : 0);
            b[sizeof(b) - 1] = 0;
            s += b;
        }
        s += "end=1\n";
        if (!CoopWriteTextAtomic(dir + "COOPHERO.TXT", s))
            SaveSyncLog("COOPHERO.TXT in %s nicht schreibbar", dir.c_str());
    }

    static bool CoopHeroRead(const std::string& dir, CoopHero& h) {
        FILE* f = fopen((dir + "COOPHERO.TXT").c_str(), "r");
        if (!f)
            return false;
        char line[1024];
        bool any = false;
        int ver = 1;
        bool end = false;
        while (fgets(line, sizeof(line), f)) {
            std::string l = line;
            while (!l.empty() && (l[l.size() - 1] == '\n' || l[l.size() - 1] == '\r'))
                l.erase(l.size() - 1);
            const size_t eq = l.find('=');
            if (eq == std::string::npos)
                continue;
            const std::string k = l.substr(0, eq), v = l.substr(eq + 1);
            if (k == "v") ver = atoi(v.c_str());   // v76
            else if (k == "end") end = true;
            else if (k == "atr") {
                int a = 0;
                for (const char* p = v.c_str(); *p && a < NPC_ATR_MAX; a++) {
                    h.atr[a] = atoi(p);
                    p = strchr(p, ',');
                    if (!p) break;
                    p++;
                }
                any = true;
            }
            else if (k == "level") h.level = atoi(v.c_str());
            else if (k == "exp") h.exp = strtoul(v.c_str(), NULL, 10);
            else if (k == "expnext") h.expNext = strtoul(v.c_str(), NULL, 10);
            else if (k == "lp") h.lp = strtoul(v.c_str(), NULL, 10);
            else if (k == "guild") h.guild = atoi(v.c_str());
            else if (k == "trueguild") h.trueGuild = atoi(v.c_str());
            else if (k == "tal") {
                size_t s = 0;
                while (s < v.size()) {
                    size_t e = v.find(';', s);
                    if (e == std::string::npos) e = v.size();
                    int t = 0, sk = 0, va = 0;
                    if (sscanf(v.substr(s, e - s).c_str(), "%d:%d:%d", &t, &sk, &va) == 3 && t > 0 && t < NPC_TAL_MAX) {
                        h.talSkill[t] = sk;
                        h.talValue[t] = va;
                    }
                    s = e + 1;
                }
            }
            else if (k == "aiv") {
                int i = 0;
                for (const char* p = v.c_str(); *p && i < 50; i++) {
                    h.aiv[i] = atoi(p);
                    p = strchr(p, ',');
                    if (!p) break;
                    p++;
                }
            }
            else if (k == "pos") {
                if (sscanf(v.c_str(), "%f,%f,%f", &h.pos[0], &h.pos[1], &h.pos[2]) == 3)
                    h.hasPos = true;
            }
            else if (k == "at") {
                sscanf(v.c_str(), "%f,%f,%f", &h.at[0], &h.at[1], &h.at[2]);
            }
            else if (k == "world") {
                h.world = v;
            }
            else if (k == "item") {
                char inst[128];
                int amount = 1, active = 0;
                if (sscanf(v.c_str(), "%127s %d %d", inst, &amount, &active) >= 2) {
                    CoopHeroItem e;
                    e.inst = inst;
                    e.amount = amount > 0 ? amount : 1;
                    e.active = active != 0;
                    h.items.push_back(e);
                }
            }
        }
        fclose(f);
        if (any && ver >= 2 && !end) {   // v76: abgebrochen geschrieben
            SaveSyncLog("COOPHERO.TXT in %s ist unvollstaendig (ohne Endmarke) - nicht verwendet", dir.c_str());
            any = false;
            h = CoopHero();
        }
        h.valid = any;
        return any;
    }

    // --- v89: Platz neben dem Helden des Stands ---------------------------------------------------------------------------
    // Nutzer (22.10.2026): "man spawnt immer noch manchmal ineinander bzw. kann einer oder beide sich nicht bewegen ... in dem Fall,
    // dass der Mitspieler fuer den Savegame keinen eigenen Standort hat, ... paar Meter neben uns spawnt" - "Der Standort neben
    // unserem Charakter sollte natuerlich legal sein, also nicht in einer Wand oder im Nichts, am besten auf dem Boden".
    // Test v88: kein eigener Held zu dieser Kennung -> zeitlich naechster aus einem anderen Stand samt dessen Lage, 0,5 m neben dem
    // Host (5811/35366 gegen 5825/35317) - beide steckten fest. Ebenso blieb ohne jeden eigenen Helden der Held genau auf dem Host.
    // Nach dem Laden einer fremden Welt steht der Spieler zunaechst dort, wo der Held dieses Stands stand (bei der Uebernahme: der
    // Host). Gesucht wird ringsum (rechts, links, hinten, schraeg, vorn; 2,5 / 3,5 / 1,8 m) eine Stelle, die
    //  - vom Helden des Stands aus in Brusthoehe frei zu sehen ist (keine Wand: zCWorld::TraceRayNearestHit 0x005F46E0, statische
    //    Polygone und Vobs, ohne Figuren),
    //  - Boden hat (Strahl bis 4 m nach unten, kein Wasser) hoechstens 1,2 m ueber/unter dessen Boden,
    //  - fuer einen NPC frei ist (oCVob::SearchNpcPosition 0x006D3B10: Boden ueber GetFloorPosition 0x006D4410, Platzpruefung
    //    0x006D3680; im Speicherabbild unveraendert) und danach noch 1,2-7 m entfernt, in Sicht und auf aehnlicher Hoehe liegt.
    // Allgemein G1.
    static bool CoopFloorBelow(zCWorld* w, const zVEC3& p, float& y, bool& water) {
        water = false;
        const zVEC3 start(p.n[0], p.n[1] + 80.f, p.n[2]);
        const zVEC3 ray(0.f, -480.f, 0.f);
        if (!w->TraceRayNearestHit(start, ray, (zCVob*)player, zTRACERAY_STAT_POLY | zTRACERAY_VOB_IGNORE_CHARACTER | zTRACERAY_POLY_TEST_WATER) ||
            !w->traceRayReport.foundHit)
            return false;
        y = w->traceRayReport.foundIntersection.n[1];
        zCPolygon* poly = w->traceRayReport.foundPoly;
        water = poly && poly->material && poly->material->matGroup == zMAT_GROUP_WATER;
        return true;
    }

    static bool CoopClearLine(zCWorld* w, const zVEC3& a, const zVEC3& b) {
        const zVEC3 ray = b - a;
        return !(w->TraceRayNearestHit(a, ray, (zCVob*)player, zTRACERAY_STAT_POLY | zTRACERAY_VOB_IGNORE_CHARACTER | zTRACERAY_POLY_IGNORE_TRANSP) &&
            w->traceRayReport.foundHit);
    }

    // v102: whom - fuer die Meldung (Teleport-Rune, CoopTeleport.cpp: "den Mitspieler")
    static bool CoopPlaceBesideHost(const zVEC3& host, const zVEC3& hostAt, std::string& how, const char* whom = "den Helden des Stands") {
        zCWorld* w = ogame ? ogame->GetGameWorld() : NULL;
        if (!w || !player) {
            how = "keine Welt - Lage unveraendert";
            return false;
        }
        float ax = hostAt.n[0], az = hostAt.n[2];
        const float al = sqrtf(ax * ax + az * az);
        if (al < 0.01f) { ax = 0.f; az = 1.f; }
        else { ax /= al; az /= al; }
        float hostFloor = host.n[1];
        bool water = false;
        const bool hostHasFloor = CoopFloorBelow(w, host, hostFloor, water);
        const float above = hostHasFloor ? host.n[1] - hostFloor : 0.f;   // Ursprung der Figur ueber dem Boden
        const zVEC3 eye(host.n[0], host.n[1] + 60.f, host.n[2]);
        static const float angles[] = { 90.f, -90.f, 180.f, 135.f, -135.f, 45.f, -45.f, 0.f };
        static const float dists[] = { 250.f, 350.f, 180.f };
        int tried = 0;
        const char* lastWhy = "-";
        for (float d : dists) {
            for (float ang : angles) {
                tried++;
                const float r = ang * 3.14159265f / 180.f;
                const float dx = ax * cosf(r) - az * sinf(r), dz = ax * sinf(r) + az * cosf(r);
                const zVEC3 c(host.n[0] + dx * d, host.n[1], host.n[2] + dz * d);
                if (!CoopClearLine(w, eye, zVEC3(c.n[0], c.n[1] + 60.f, c.n[2]))) { lastWhy = "Wand"; continue; }
                float fy = 0.f;
                if (!CoopFloorBelow(w, c, fy, water)) { lastWhy = "kein Boden"; continue; }
                if (water) { lastWhy = "Wasser"; continue; }
                if (hostHasFloor && fabsf(fy - hostFloor) > 120.f) { lastWhy = "H\xF6he"; continue; }
                zVEC3 p(c.n[0], fy + above, c.n[2]);
                if (!player->SearchNpcPosition(p)) { lastWhy = "kein freier Platz"; continue; }
                const float ex = p.n[0] - host.n[0], ez = p.n[2] - host.n[2];
                const float dist = sqrtf(ex * ex + ez * ez);
                if (dist < 120.f || dist > 700.f || fabsf(p.n[1] - host.n[1]) > 150.f) { lastWhy = "Suche zu weit/zu nah"; continue; }
                if (!CoopClearLine(w, eye, zVEC3(p.n[0], p.n[1] + 60.f, p.n[2]))) { lastWhy = "Wand nach der Suche"; continue; }
                player->SetCollDet(FALSE);
                player->SetPositionWorld(p);
                player->SetCollDet(TRUE);
                player->SetHeadingAtWorld(zVEC3(ax, 0.f, az));
                char b[200];
                _snprintf(b, sizeof(b), "neben %s gesetzt (%.1f m, Richtung %d Grad, %d. Stelle, %.0f/%.0f/%.0f)",
                    whom, dist / 100.f, (int)ang, tried, p.n[0], p.n[1], p.n[2]);
                b[sizeof(b) - 1] = 0;
                how = b;
                return true;
            }
        }
        how = std::string("keine legale Stelle neben ") + whom + " (zuletzt: " + lastWhy + ") - Lage unveraendert";
        return false;
    }

    // Auf den (frisch geladenen) Helden anwenden. Eigene Aenderungen laufen unter WorldSyncApplying,
    // damit die Welt-Sync-Hooks (Inventar, Welt) sie nicht als Spieleraktion melden.
    static void CoopHeroApply(const CoopHero& h) {
        if (!player)
            return;
        // v89: Lage des Helden dieses Stands (bei der Uebernahme der Host) - vor allem anderen merken
        const zVEC3 hostPos = player->GetPositionWorld();
        const zVEC3 hostAt = player->GetAtVectorWorld();
        if (!h.valid) {   // kein eigener Held gefunden: der Held aus dem Stand bleibt - aber nicht genau auf dem Host
            std::string how;
            PendingMoveReset = true;
            CoopPlaceBesideHost(hostPos, hostAt, how);
            SaveSyncLog("kein eigener Held - Held aus dem Stand bleibt, %s", how.c_str());
            return;
        }
        WorldSyncApplying++;
        std::vector<oCItem*> inv;
        CoopForEachInvItem(inv);
        for (oCItem* it : inv)
            if (it->HasFlag(ITM_FLAG_ACTIVE))
                player->UnequipItem(it);
        CoopForEachInvItem(inv);
        int removed = 0;
        for (oCItem* it : inv) {
            oCItem* r = player->RemoveFromInv(it, CoopItemAmount(it));
            if (r) {
                ogame->GetGameWorld()->RemoveVob(r);
                removed++;
            }
        }

        player->level = h.level;
        player->experience_points = h.exp;
        player->experience_points_next_level = h.expNext;
        player->learn_points = h.lp;
        player->SetGuild(h.guild);
        player->SetTrueGuild(h.trueGuild);
        for (int t = 1; t < NPC_TAL_MAX; t++) {
            // v21: GetTalent* liefert -1 fuer Talente, die es nicht gibt (G1: 12-16) - nicht anlegen
            if (h.talSkill[t] >= 0)
                player->SetTalentSkill(t, h.talSkill[t]);
            if (h.talValue[t] >= 0)
                player->SetTalentValue(t, h.talValue[t]);
        }
        for (int i = 0; i < 50; i++)
            player->aiscriptvars[i] = h.aiv[i];

        int added = 0, equipped = 0;
        for (auto const& e : h.items) {
            const int idx = parser->GetIndex(zSTRING(e.inst.c_str()));
            if (idx < 0)
                continue;
            int left = e.amount;
            while (left > 0) {
                oCItem* it = CreateCoopItem(idx);
                if (!it)
                    break;
                const bool multi = (it->flags & ITM_FLAG_MULTI) != 0;
                it->amount = multi ? left : 1;
                left -= it->amount;
                oCItem* inInv = player->PutInInv(it);
                added++;
                if (e.active && inInv && !inInv->HasFlag(ITM_FLAG_ACTIVE)) {
                    player->EquipItem(inInv);
                    equipped++;
                }
                if (!multi && e.active)
                    break;   // nur ein Stueck ist ausgeruestet, Rest kommt als eigener Eintrag
            }
        }
        // Werte zuletzt (Maximalwerte vor den aktuellen)
        player->SetAttribute(NPC_ATR_HITPOINTSMAX, h.atr[NPC_ATR_HITPOINTSMAX]);
        player->SetAttribute(NPC_ATR_MANAMAX, h.atr[NPC_ATR_MANAMAX]);
        for (int a = 0; a < NPC_ATR_MAX; a++)
            player->SetAttribute(a, h.atr[a]);
        WorldSyncApplying--;

        CoopLookApplyOwn("eigener Held wiederhergestellt");   // v55: gewaehltes Aussehen (CoopLook.cpp)

        // v22: eigene Lage wiederherstellen (nur in derselben Welt). v23: NICHT ueber ResetPos - der vtable-Platz
        // ist unter Othello eine andere Funktion mit zwei Argumenten (Absturz), siehe CoopResetPosSafe.
        // v25: hier (evtl. noch im Lade-Rueckruf) nur versetzen; die Bewegung setzt das Original-ResetPos im ersten
        // Spielbild danach an derselben Stelle zurueck (HandleMoveReset) - sonst konnte man sich nicht bewegen (Test v24).
        // v89: nur die eigene Lage GENAU dieses Stands, und nur mit Abstand zum Helden des Stands - sonst daneben (CoopPlaceBesideHost)
        std::string lage;
        PendingMoveReset = true;
        bool placed = false;
        const char* why = "keine eigene Lage";
        if (h.hasPos && h.world == CoopWorldName()) {
            const float ex = h.pos[0] - hostPos.n[0], ey = h.pos[1] - hostPos.n[1], ez = h.pos[2] - hostPos.n[2];
            const float d = sqrtf(ex * ex + ey * ey + ez * ez);
            if (!h.exact)
                why = "Lage aus einem anderen Stand";
            else if (d < 150.f)
                why = "eigene Lage zu nah am Helden des Stands";
            else {
                const zVEC3 p(h.pos[0], h.pos[1], h.pos[2]);
                player->SetCollDet(FALSE);
                player->SetPositionWorld(p);
                player->SetCollDet(TRUE);
                const zVEC3 a(h.at[0], h.at[1], h.at[2]);
                if (a.n[0] != 0.f || a.n[2] != 0.f)
                    player->SetHeadingAtWorld(a);
                lage = "eigene Lage wiederhergestellt";
                placed = true;
            }
        }
        else if (h.hasPos)
            why = "Lage aus anderer Welt";
        if (!placed) {
            std::string how;
            CoopPlaceBesideHost(hostPos, hostAt, how);
            lage = std::string(why) + " - " + how;
        }
        SaveSyncLog("eigener Held wiederhergestellt: Stufe %d, %d Gegenstaende (%d ausgeruestet), vorher %d entfernt, %s", h.level, added, equipped, removed, lage.c_str());
    }

    // --- Pruefen, ob gerade gespeichert werden darf -------------------------------------------------

    static bool CoopCanSaveNow(std::string& why) {
        if (IsLoadingLevel || IsSavingGame) { why = CoopTr("Laden/Speichern l" "\xE4" "uft", "loading/saving"); return false; }
        if (!player || !ogame->savegameManager) { why = CoopTr("kein Held", "no hero"); return false; }
        if (player->IsDead() || player->IsUnconscious()) { why = CoopTr("Held tot/bewusstlos", "hero dead/unconscious"); return false; }
        if (IsPlayerTalkingWithAnybody() || !oCInformationManager::GetInformationManager().IsDone) { why = CoopTr("Dialog", "dialogue"); return false; }
        if (player->inventory2.IsOpen()) { why = CoopTr("Inventar offen", "inventory open"); return false; }
        const int bs = player->GetBodyState();
        if (bs > BS_SPRINT) { why = CoopTr("Held besch" "\xE4" "ftigt (K" "\xF6" "rperzustand ", "hero busy (body state ") + std::to_string(bs) + ")"; return false; }
        return true;
    }

    static void CoopDoSave(int slot, const std::string& name) {
        oCSavegameInfo* info = ogame->savegameManager->GetSavegame(slot);
        if (info)
            info->m_Name = zSTRING(name.c_str());
        gameMan->Write_Savegame(slot);
    }

    // v40: im Hauptmenue nicht direkt laden - CoopLobbyLoadFromMenu (CoopMenuLobby.cpp) laesst das Menue mit
    // OTHELLO_SAVEGAME_LOAD schliessen, CGameManager::Menu laedt dann. CoopActiveRemoteSid bleibt bis Game_LoadBegin gesetzt.
    bool CoopLobbyLoadFromMenu(int slot);
    static bool CoopMenuLoadDeferred = false;

    // v80: im Spiel nur laden, wenn kein Menue laeuft. Game_Loop (und damit dieser Takt) laeuft auch bei offenem Spielmenue
    // (Esc-Menue, Tagebuch, Ladefenster - Sicherung 1408 s im Lademenue, Test 21.10.). Test v79: im offenen Tagebuch aus der
    // F9-Liste geladen -> Read_Savegame lief in der Menue-Schleife, das Tagebuch blieb danach aktiv, das Spiel pausiert (Spielzeit
    // stand), beim Client standen alle NPCs. Gothic selbst laedt erst, nachdem das Menue zu ist (CGameManager::Menu). Allgemein.
    // Im Hauptmenue ohne Spiel (player NULL) laedt CoopLobbyLoadFromMenu ueber das Schliessen des Menues - unveraendert.
    static long long CoopLoadMenuWaitMs = 0;
    // v90 (Test v89: "das eine Mal war ich im Dialog und dann war wieder dieser Bug, wo die Kamera so komisch ist und nicht mehr dem
    // Helden folgt"): eine Uebernahme lud mitten in einem Dialog (CS_ESKORTE2, Kamera CAMMODDIALOG). Jetzt wie beim Menue warten,
    // bis kein Dialog mehr laeuft (oCInformationManager::IsDone, IsPlayerTalkingWithAnybody); nach 15 s beendet GT den Dialog
    // (oCInformationManager::Exit wie der Dialog-Waechter) und laedt 0,7 s spaeter (Kamera zurueck). Allgemein.
    static long long CoopLoadDialogWaitMs = 0;
    static long long CoopLoadDialogExitMs = 0;
    // v101 (Durchsicht v90): ein Ladevorgang, der waehrend des Wartens wegfiel (Verbindung weg, Auftrag ueberholt), liess die
    // Zeiten stehen - der naechste Ladevorgang in einem Dialog haette ihn sofort beendet statt 15 s zu warten. Jeder Ladevorgang
    // fragt hier in jedem Bild; laenger als 2 s keine Frage = neuer Ladevorgang, Zeiten zuruecksetzen.
    static long long CoopLoadBlockAskMs = 0;

    static bool CoopLoadBlockedByMenu(const char* what) {
        if (CurrentMs - CoopLoadBlockAskMs > 2000)
            CoopLoadMenuWaitMs = CoopLoadDialogWaitMs = CoopLoadDialogExitMs = 0;
        CoopLoadBlockAskMs = CurrentMs;
        zCMenu* menu = player ? zCMenu::GetActive() : NULL;
        if (!menu) {
            if (CoopLoadMenuWaitMs) {
                SaveSyncLog("%s: Menue zu - lade jetzt (nach %lld s Warten)", what, (CurrentMs - CoopLoadMenuWaitMs) / 1000);
                CoopLoadMenuWaitMs = 0;
            }
        }
        else {
            if (!CoopLoadMenuWaitMs) {
                CoopLoadMenuWaitMs = CurrentMs;
                SaveSyncLog("%s wartet: Menue %s ist offen - geladen wird, sobald es zu ist", what, menu->name.ToChar());
            }
            return true;
        }
        if (!player)
            return false;
        oCInformationManager& im = oCInformationManager::GetInformationManager();
        if (!im.IsDone || IsPlayerTalkingWithAnybody()) {
            if (!CoopLoadDialogWaitMs) {
                CoopLoadDialogWaitMs = CurrentMs;
                SaveSyncLog("%s wartet: Dialog l\xE4uft - geladen wird nach dem Dialog (sp\xE4testens in 15 s)", what);
                ChatLog(CoopTr("Gleich wird geladen - nach dem Dialog (sp\xE4testens in 15 s).", "Loading soon - after the dialogue (within 15 s)."), zCOLOR(255, 200, 80, 255));
            }
            else if (!CoopLoadDialogExitMs && CurrentMs > CoopLoadDialogWaitMs + 15000) {
                im.Exit();
                CoopLoadDialogExitMs = CurrentMs;
                SaveSyncLog("%s: Dialog nach 15 s beendet (Exit) - lade gleich", what);
            }
            return true;
        }
        if (CoopLoadDialogExitMs && CurrentMs < CoopLoadDialogExitMs + 700)
            return true;   // nach dem Beenden die Kamera erst zurueckkehren lassen
        if (CoopLoadDialogWaitMs) {
            SaveSyncLog("%s: Dialog vorbei - lade jetzt (nach %lld s Warten)", what, (CurrentMs - CoopLoadDialogWaitMs) / 1000);
            CoopLoadDialogWaitMs = 0;
        }
        CoopLoadDialogExitMs = 0;
        return false;
    }

    static void CoopDoLoad(int slot) {
        // Slot-Infos neu von der Platte (falls aus dem Archiv kopiert); im Hauptmenue gibt es ogame noch nicht
        oCSavegameManager* sm = (ogame && ogame->savegameManager) ? ogame->savegameManager : (gameMan ? gameMan->savegameManager : NULL);
        if (sm)
            sm->Reinit();
        CoopMenuLoadDeferred = CoopLobbyLoadFromMenu(slot);
        if (CoopMenuLoadDeferred)
            return;
        gameMan->Read_Savegame(slot);
    }

    // --- Senden ------------------------------------------------------------------------------------

    static void SaveSyncSend(json& j, int type) {
        j["type"] = type;
        j["id"] = MyselfId.ToChar();
        if (ServerThread && !j.contains("by"))
            j["by"] = MyselfId.ToChar();
        j["pc"] = CoopMyPc();   // v76: feste PC-Kennung des Absenders (weitergeleitete Pakete behalten die des Urhebers)
        CoopTraceCount(TRACE_SENT, type);
        ReadyToSendJsons.enqueue(j);
    }

    static void SendMetaReq(int type, const CoopSaveMeta& m, const std::string& to = std::string()) {
        json j;
        if (!to.empty())
            j["to"] = to;   // v71: Ladeauftrag nur fuer einen Client im Hauptmenue (Server.cpp sendet ihn nur dorthin); v76 auch Verbinden/Fortsetzen
        j["sid"] = m.sid;
        j["kind"] = m.kind;
        j["name"] = m.name;
        j["slot"] = m.slot;
        j["world"] = m.world;
        j["time"] = m.time;
        j["src"] = m.src.empty() ? CoopMe() : m.src;
        // v76: PC der Welt - bei eigener Welt dieser PC; ein alter Stand mit fremder Rolle und ohne srcpc bleibt ohne
        const std::string srcpc = !m.srcpc.empty() ? m.srcpc : ((m.src.empty() || m.src == CoopMe()) ? CoopMyPc() : std::string());
        if (!srcpc.empty())
            j["srcpc"] = srcpc;
        if (!m.srcname.empty())
            j["srcname"] = m.srcname;
        if (!m.mh.empty())
            j["mh"] = m.mh;
        if (m.take)
            j["take"] = 1;
        if (m.list)
            j["list"] = 1;
        if (m.ask)
            j["ask"] = m.ask;   // v90
        if (m.stime > 0)
            j["stime"] = m.stime;
        if (!m.run.empty())
            j["run"] = m.run;   // v93: Durchlauf
        SaveSyncSend(j, type);
    }

    // v76: Stand fuer die spaetere Vorschau-Liste beschreiben (COOPINFO.TXT, je PC): Kapitel, Spieltag/Uhrzeit, Welt, Spielzeit,
    // eigener Held, Aufgaben (laufend/erledigt/gescheitert, bis zu 3 laufende Titel). Liest nur, was GT ohnehin liest.
    static std::string LobbyOthelloStamp();   // CoopMenuLobby.cpp
    static void CoopInfoWrite(const std::string& dir, int slot) {
        std::string s = "v=1\n";
        auto add = [&](const char* k, const std::string& v) {
            s += k;
            s += "=";
            s += v;
            s += "\n";
        };
        add("pc", CoopMyPc());
        add("name", MyNickname.ToChar());
        add("rolle", CoopMe());
        add("gt", COOP_OTHELLO_FASSUNG);
        add("othello", LobbyOthelloStamp());
        int kapitel = -1;
        if (zCPar_Symbol* sym = parser ? parser->GetSymbol("KAPITEL") : NULL)
            sym->GetValue(kapitel, 0);
        add("kapitel", std::to_string(kapitel));
        int day = 0, h = 0, mi = 0;
        if (ogame)
            ogame->GetTime(day, h, mi);
        char b[64];
        _snprintf(b, sizeof(b), "%02d:%02d", h, mi);
        b[sizeof(b) - 1] = 0;
        add("tag", std::to_string(day));
        add("uhrzeit", b);
        add("welt", CoopWorldName());
        oCSavegameInfo* info = ogame && ogame->savegameManager ? ogame->savegameManager->GetSavegame(slot) : NULL;
        if (info)
            add("spielzeit", std::to_string(info->m_PlayTimeSeconds));
        if (player) {
            add("stufe", std::to_string(player->level));
            add("gilde", std::to_string(player->GetGuild()));
            // v77: Gildenname aus der Skript-Tabelle TXT_GUILDS (G1-Standard, auch in Othello) fuer die Vorschau-Liste
            zCPar_Symbol* gs = parser ? parser->GetSymbol("TXT_GUILDS") : NULL;
            const int g = player->GetGuild();
            if (gs && g >= 0 && g < (int)gs->ele) {
                zSTRING gn;
                gs->GetValue(gn, g);
                if (!gn.IsEmpty())
                    add("gildenname", gn.ToChar());
            }
            _snprintf(b, sizeof(b), "%d/%d", player->GetAttribute(NPC_ATR_HITPOINTS), player->GetAttribute(NPC_ATR_HITPOINTSMAX));
            b[sizeof(b) - 1] = 0;
            add("lp", b);
            add("ep", std::to_string(player->experience_points));
        }
        int running = 0, done = 0, failed = 0;
        std::vector<std::string> titles;
        oCLogManager& lm = oCLogManager::GetLogManager();
        for (auto* it = lm.m_lstTopics.GetNextInList(); it; it = it->GetNextInList()) {
            oCLogTopic* t = it->GetData();
            if (!t || t->m_enuSection != oCLogTopic::zELogSection_Missions)
                continue;
            if (t->m_enuStatus == oCLogTopic::zELogTopicStatus_Running) {
                running++;
                if (titles.size() < 3)
                    titles.push_back(t->m_strDescription.ToChar());
            }
            else if (t->m_enuStatus == oCLogTopic::zELogTopicStatus_Success)
                done++;
            else if (t->m_enuStatus == oCLogTopic::zELogTopicStatus_Failure)
                failed++;
        }
        _snprintf(b, sizeof(b), "%d/%d/%d", running, done, failed);
        b[sizeof(b) - 1] = 0;
        add("aufgaben", b);   // laufend/erledigt/gescheitert
        for (auto const& t : titles)
            add("aufgabe", t);
        add("end", "1");
        CoopWriteTextAtomic(dir + "COOPINFO.TXT", s);
    }

    static void SendAck(const std::string& sid, bool ok, const std::string& msg) {
        json j;
        j["sid"] = sid;
        j["ok"] = ok ? 1 : 0;
        j["msg"] = msg;
        SaveSyncSend(j, SYNC_SAVE_ACK);
    }

    // --- Dateiuebertragung (v19) -------------------------------------------------------------------

    // Alle Dateien eines Archivordners in Stuecken zu 48 KB einreihen (gesendet wird im Pulse, gedrosselt).
    // v71: zip = der Anfordernde kann entpacken - dann jede Datei ab 4 KB gepackt, wenn es kleiner wird ("z", "raw");
    // jedes Stueck traegt die Gesamtgroesse ("tb") und Dateizahl ("tf") fuer die Fortschrittsanzeige beim Empfaenger.
    // v76: jedes Stueck traegt CRC32 und Groesse seiner Datei ("crc", "fs"), FILE_END die Manifest-Pruefsumme ("mh"). Weicht eine
    // Spieldatei vom Manifest des Archivstands ab, wird nichts gesendet (bad = Grund) - der Empfaenger laedt keinen kaputten Stand.
    static bool CoopQueueSaveFiles(const std::string& dir, const std::string& sid, const std::string& to, bool zip, std::string& bad) {
        struct OutFile {
            std::string name;
            std::vector<std::uint8_t> data;
            size_t raw = 0;
            unsigned crc = 0;
            bool packed = false;
        };
        std::vector<OutFile> list;
        CoopManifest man;
        const bool haveMan = CoopManifestEnsure(dir, man);
        std::set<std::string> seen;
        WIN32_FIND_DATAA fd;
        HANDLE h = FindFirstFileA((dir + "*").c_str(), &fd);
        if (h == INVALID_HANDLE_VALUE) {
            bad = "Ordner fehlt";
            return false;
        }
        const bool canZip = zip && CoopZipReady();
        long long rawBytes = 0, sendBytes = 0;
        int packedFiles = 0;
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;
            OutFile o;
            o.name = fd.cFileName;
            const std::string up = SaveSyncUpper(o.name);
            if (up == "COOPFILES.TXT" || (up.size() > 4 && up.compare(up.size() - 4, 4, ".TMP") == 0))
                continue;   // v76: das Manifest bildet der Empfaenger selbst; Reste nicht senden
            if (!CoopReadWholeFile(dir + o.name, o.data)) {
                if (CoopIsGameFile(o.name)) {
                    bad = o.name + " nicht lesbar";
                    break;
                }
                continue;
            }
            o.raw = o.data.size();
            o.crc = o.data.empty() ? 0 : CoopCrc32(&o.data[0], o.data.size(), 0);
            if (haveMan && CoopIsGameFile(o.name)) {
                auto it = man.find(up);
                if (it == man.end() || it->second.size != (long long)o.raw || it->second.crc != o.crc) {
                    bad = o.name + " passt nicht zum Manifest";
                    break;
                }
                seen.insert(up);
            }
            std::vector<std::uint8_t> packed;
            if (canZip && o.raw >= 4096 && CoopZipPack(o.data, packed) && packed.size() + 1024 < o.raw) {
                o.data.swap(packed);
                o.packed = true;
                packedFiles++;
            }
            rawBytes += (long long)o.raw;
            sendBytes += (long long)o.data.size();
            list.push_back(std::move(o));
        } while (FindNextFileA(h, &fd));
        FindClose(h);
        if (bad.empty() && haveMan)
            for (auto const& e : man)
                if (!seen.count(e.first)) {
                    bad = e.first + " fehlt";
                    break;
                }
        if (bad.empty() && list.empty())
            bad = "keine Dateien";
        if (!bad.empty()) {
            SaveSyncLog("Stand %s fuer %s NICHT gesendet: %s", sid.c_str(), to.c_str(), bad.c_str());
            return false;
        }
        const size_t CHUNK = 48 * 1024;
        for (const OutFile& o : list) {
            const size_t size = o.data.size();
            const int chunks = size > 0 ? (int)((size + CHUNK - 1) / CHUNK) : 1;
            for (int i = 0; i < chunks; i++) {
                const size_t off = i * CHUNK;
                const size_t len = size > 0 ? (size - off < CHUNK ? size - off : CHUNK) : 0;
                json j;
                j["to"] = to;
                j["sid"] = sid;
                j["src"] = CoopMe();
                j["srcpc"] = CoopMyPc();   // v76
                j["f"] = o.name;
                j["i"] = i;
                j["n"] = chunks;
                j["size"] = (int)size;
                j["tb"] = sendBytes;
                j["tf"] = (int)list.size();
                j["crc"] = o.crc;          // v76: der entpackten Datei
                j["fs"] = (long long)o.raw;
                if (o.packed) {
                    j["z"] = 1;
                    j["raw"] = (long long)o.raw;
                }
                j["b"] = json::binary(std::vector<std::uint8_t>(o.data.begin() + off, o.data.begin() + off + len));
                j["type"] = SYNC_FILE_PART;
                FileOutQueue.push_back(j);
            }
        }
        const int files = (int)list.size();
        json end;
        end["to"] = to;
        end["sid"] = sid;
        end["src"] = CoopMe();
        end["srcpc"] = CoopMyPc();   // v76
        end["files"] = files;
        end["tb"] = sendBytes;
        if (haveMan)
            end["mh"] = CoopManifestHash(man);
        end["type"] = SYNC_FILE_END;
        FileOutQueue.push_back(end);
        char how[120];
        if (packedFiles > 0)
            _snprintf(how, sizeof(how), ", gepackt %.1f MB (%d Dateien gepackt)", sendBytes / 1048576.0, packedFiles);
        else
            _snprintf(how, sizeof(how), ", ungepackt (%s)", !zip ? "Gegenseite ohne Entpacker" : (canZip ? "lohnt nicht" : "cabinet.dll fehlt"));
        how[sizeof(how) - 1] = 0;
        SaveSyncLog("sende Stand %s an %s: %d Dateien, %.1f MB%s", sid.c_str(), to.c_str(), files, rawBytes / 1048576.0, how);
        return files > 0;
    }

    // v71: ein Mitspieler ist weg - seine noch nicht gesendeten Dateistuecke und ein offener Ladeauftrag entfallen.
    void CoopSaveSyncPeerGone(const std::string& id) {
        size_t dropped = 0;
        for (auto it = FileOutQueue.begin(); it != FileOutQueue.end();) {
            if (it->contains("to") && (*it)["to"].is_string() && (*it)["to"].get<std::string>() == id) {
                it = FileOutQueue.erase(it);
                dropped++;
            }
            else
                ++it;
        }
        const bool menu = TakePeers.erase(id) > 0;
        if (dropped || menu)
            SaveSyncLog("%s ist weg - %u Dateistuecke verworfen%s", id.c_str(), (unsigned)dropped, menu ? ", Ladeauftrag entfaellt" : "");
        for (auto it = CoopPcRole.begin(); it != CoopPcRole.end();) {   // v76
            if (it->second == id)
                it = CoopPcRole.erase(it);
            else
                ++it;
        }
        // v76: Abbruch-Sicherung - der Host sichert sofort, sobald ein Mitspieler im Spiel wegfaellt (die uebrigen speichern mit)
        if (ServerThread && player && SaveSyncAktiv()) {
            HostAbortSavePending = true;
            HostAbortSaveUntil = CurrentMs + 60000;
            HostAbortSaveFor = id;
            SaveSyncLog("%s ist weg - Abbruch-Sicherung folgt", id.c_str());
        }
    }

    static void CoopPumpFileOut() {
        for (int k = 0; k < 24 && !FileOutQueue.empty(); k++) {
            json j = FileOutQueue.front();
            FileOutQueue.pop_front();
            const int type = j["type"].get<int>();
            SaveSyncSend(j, type);
        }
    }

    // Eigene Welt einer Kennung: eigener Archivordner (src = ich) oder ein Slot mit dieser Kennung.
    static std::string CoopOwnCopyDir(const std::string& sid) {
        const std::string a = CoopArchiveFind(sid);   // v93: Hauptordner oder Durchlauf-Ordner
        // v40: Den Archivordner <Kennung>\ schreibt nur FinishPendingArchive, immer mit der eigenen Welt dieses PCs (fremde
        // Welten liegen in <Kennung>__von_<Absender>\). src darin ist die Spieler-Kennung von damals (HOST/FRIEND_n) - nach einem
        // Rollentausch der PCs oder einem Neuverbinden (dann FRIEND_2) passte sie nicht mehr, der eigene Held fehlte.
        if (!a.empty() && CoopDirExists(a))
            return a;
        for (int s = 1; s <= 15; s++) {
            CoopSaveMeta sm;
            if (CoopReadMeta(CoopSlotDir(s), sm) && sm.sid == sid && CoopMetaMine(sm))   // v76: nach PC-Kennung
                return CoopSlotDir(s);
        }
        return std::string();
    }

    // v55: eigener Held (COOPHERO.TXT in einem eigenen Archivordner oder Slot), dessen Datei zeitlich am naechsten an stime liegt.
    // Fremde Welten (<Kennung>__von_<Absender>) und halbe Uebertragungen (.part) zaehlen nicht.
    // v76: Zeit aus COOPSYNC.TXT (stime), sonst wie bisher das Dateidatum von COOPHERO.TXT.
    static std::string CoopNearestOwnHeroDir(long long stime, long long& diff) {
        std::string best;
        diff = -1;
        auto consider = [&](const std::string& dir) {
            if (!CoopFileExists(dir + "COOPHERO.TXT"))
                return;
            CoopSaveMeta mm;
            const long long t = (CoopReadMeta(dir, mm) && mm.stime > 0) ? mm.stime : CoopFileUnixTime(dir + "COOPHERO.TXT");
            if (t <= 0)
                return;
            const long long d = t > stime ? t - stime : stime - t;
            if (diff < 0 || d < diff) {
                diff = d;
                best = dir;
            }
        };
        CoopArchiveScan();   // v93: Hauptordner und Durchlauf-Ordner
        for (auto const& e : CoopArchiveIndex) {
            const std::string& n = e.first;   // GROSS
            if (n.find("__VON_") != std::string::npos || n.find(".PART") != std::string::npos)
                continue;
            consider(e.second);
        }
        for (int s = 1; s <= 15; s++) {
            CoopSaveMeta sm;
            if (CoopReadMeta(CoopSlotDir(s), sm) && CoopMetaMine(sm))
                consider(CoopSlotDir(s));
        }
        return best;
    }

    // --- Union-Rueckrufe -----------------------------------------------------------------------------

    // v76: Client in Pause oder bei Verbindungsstoerung - speichert und laedt nur fuer sich (Nutzer: "dann werden auch keine
    // Sicherungen fuer den gemeinsamen Coop angelegt")
    static bool CoopSyncSuspended() {
        return ClientThread && !ServerThread && (IsCoopPaused || Disturbed);
    }

    // v93: eigene Gothic-Speicherung ohne Coop-Kennung waehrend eines Durchlaufs (Pause, Stoerung, allein weitergespielt) -
    // COOPRUN.TXT in den Slot, damit die Liste sie als lokalen Stand dieses Durchlaufs zeigen kann (kein Archiv)
    static void CoopRunTagLocal(int slot) {
        if (slot < 1 || CoopCurrentRun.empty())
            return;
        const std::string d = CoopSlotDir(slot);
        if (!CoopFileExists(d + "SAVEINFO.SAV"))
            return;
        CoopWriteTextAtomic(d + "COOPRUN.TXT", "v=1\nrun=" + CoopCurrentRun + "\ntime=" + CoopNowText("time") + "\nstime=" +
            std::to_string(CoopUnixNow()) + "\npc=" + CoopMyPc() + "\nend=1\n");
        SaveSyncLog("Slot %d: lokaler Stand im Durchlauf %s markiert", slot, CoopCurrentRun.c_str());
    }

    // Durchlauf eines Slots: gemeinsamer Stand (COOPSYNC.TXT run) bzw. lokaler Stand (COOPRUN.TXT)
    static std::string CoopSlotRun(int slot, bool* local = NULL) {
        CoopSaveMeta m;
        if (CoopReadMeta(CoopSlotDir(slot), m)) {
            if (local)
                *local = false;
            return m.run;
        }
        std::vector<std::uint8_t> d;
        std::string run;
        if (CoopReadWholeFile(CoopSlotDir(slot) + "COOPRUN.TXT", d) && !d.empty()) {
            const std::string t((const char*)&d[0], d.size());
            const size_t p = t.find("run=");
            if (p != std::string::npos) {
                run = t.substr(p + 4, t.find_first_of("\r\n", p) - p - 4);
                if (run.find_first_of("\\/:*?\"<>|") != std::string::npos || run.find("..") != std::string::npos)
                    run.clear();
            }
        }
        if (local)
            *local = !run.empty();
        return run;
    }

    void CoopSaveSyncOnSaveEnd() {
        const int slot = UnionCore::SaveLoadGameInfo.slotID;
        if (slot < 0)
            return;   // Weltwechsel (current) o. ae.
        if (!SaveSyncAktiv()) {
            CoopRunTagLocal(slot);   // v93
            return;
        }
        if (CoopSyncSuspended() && !CoopBackupSavePending) {
            SaveSyncLog("Slot %d gespeichert - %s: nur eigener Stand (keine Coop-Kennung, kein Archiv, der Partner speichert nicht mit)", slot,
                IsCoopPaused ? "Pause" : "Verbindung gestoert");
            CoopRunTagLocal(slot);   // v93
            return;
        }
        PendingArchiveSlot = slot;
        PendingArchiveMs = CurrentMs;
        PendingArchiveRemote = !CoopActiveRemoteSid.empty();
        PendingArchiveTake = PendingArchiveRemote && RemoteSave.take && RemoteSave.sid == CoopActiveRemoteSid;
        PendingArchiveTakeSrc = PendingArchiveTake ? RemoteSave.by : std::string();
        PendingArchiveTakeSrcPc = PendingArchiveTake ? RemoteSave.bypc : std::string();
        PendingArchiveTakeMh = PendingArchiveTake ? RemoteSave.mh : std::string();
        PendingArchiveBy = PendingArchiveRemote ? CoopActiveRemoteBy : CoopMe();
        PendingArchiveByPc = PendingArchiveRemote ? CoopActiveRemoteByPc : CoopMyPc();
        if (CoopBackupSavePending) {
            PendingArchiveSid = CoopNewSid("vorUebernahme_");
            PendingArchiveKind = "sicherung";
            PendingArchiveRemote = false;
            PendingArchiveTake = false;
            PendingArchiveBy = CoopMe();
            PendingArchiveByPc = CoopMyPc();
        }
        else {
            PendingArchiveSid = PendingArchiveRemote ? CoopActiveRemoteSid : CoopNewSid("");
            PendingArchiveKind = PendingArchiveRemote ? CoopActiveRemoteKind : (!CoopSavingKind.empty() ? CoopSavingKind : (CoopSavingForAuto ? "auto" : "manuell"));
        }
        // v93: Durchlauf - beim Mitspeichern der des Ausloesers; eigene gemeinsame Sicherung ohne Durchlauf (aelterer Stand
        // weitergespielt, Verbinden im Spiel): jetzt einen anlegen
        if (PendingArchiveRemote)
            PendingArchiveRun = CoopActiveRemoteRun;
        else {
            if (CoopCurrentRun.empty() && !CoopBackupSavePending && (ServerThread || ClientThread)) {
                const std::string me = MyNickname.ToChar();
                const std::string other = CoopPartnerNick.empty() ? std::string("Mitspieler") : CoopPartnerNick;
                CoopRunSet(ServerThread ? CoopRunNew(me, other) : CoopRunNew(other, me), "erste gemeinsame Sicherung ohne Durchlauf");
            }
            PendingArchiveRun = CoopCurrentRun;
        }
        CoopLastSaveMs = CurrentMs;
    }

    void CoopSaveSyncOnLoadBegin() {
        const int slot = UnionCore::SaveLoadGameInfo.slotID;
        CoopMenuLoadDeferred = false;
        if (!CoopActiveRemoteSid.empty()) {   // wir laden auf Wunsch des Partners
            SaveSyncLog("lade Slot %d auf Wunsch des Partners (%s)", slot, CoopActiveRemoteSid.c_str());
            CoopActiveRemoteSid.clear();
            JoinLoadNow = true;           // v76: Ende in CoopSaveSyncWorldLoaded (Fortsetzen, eigene Kopie)
            DivergedHero.valid = false;   // v76: der gemerkte Held ist in diesem Ladevorgang aufgegangen (oder gilt nicht mehr)
            if (!TakeLoadTargeted)
                PostTakeSidNext.clear();
            TakeLoadTargeted = false;
            return;
        }
        // Eigener Ladevorgang: ein noch offener Held gehoert nicht zu diesem Stand (ausser aus der F9-Liste)
        if (!PendingHeroLocalOk)
            PendingHeroApply = false;
        PendingHeroLocalOk = false;
        PostTakeSid.clear();   // v76: eigene Kopie nach einer Uebernahme - der Stand ist jetzt ein anderer
        PostTakeSidNext.clear();
        if (slot >= 1)   // v93: eigener Ladevorgang - es gilt der Durchlauf dieses Stands (auch ohne Coop; leer = aelterer Stand)
            CoopRunSet(CoopSlotRun(slot), "eigener Ladevorgang");
        if (slot < 0 || !SaveSyncAktiv())
            return;
        if (CoopSyncSuspended()) {   // v76: Pause/Stoerung - nur fuer mich
            SaveSyncLog("lade Slot %d - %s: nur fuer mich, der Partner laedt nicht mit", slot, IsCoopPaused ? "Pause" : "Verbindung gestoert");
            RemoteLoadPending = false;
            RemoteSaveQueue.clear();
            TakeJob.active = false;
            BrowserOpen = false;
            return;
        }
        CoopSaveMeta m;
        CoopReadMeta(CoopSlotDir(slot), m);
        m.slot = slot;
        if (m.name.empty())
            m.name = CoopSlotName(slot);
        if (m.src.empty())
            m.src = CoopMe();
        if (m.sid.empty() && slot >= 1 && CoopFileExists(CoopSlotDir(slot) + "SAVEINFO.SAV")) {
            // v19: Stand ohne Kennung (z. B. vor dem Verbinden gespeichert) - jetzt eine vergeben und
            // archivieren, damit der Partner ihn uebernehmen kann.
            m.sid = CoopNewSid("geladen_");
            m.kind = "geladen";
            m.by = CoopMe();
            m.bypc = CoopMyPc();
            m.src = CoopMe();
            m.srcpc = CoopMyPc();
            m.srcname = MyNickname.ToChar();
            m.time = CoopNowText("time");
            m.stime = CoopFileUnixTime(CoopSlotDir(slot) + "SAVEINFO.SAV");
            m.run = CoopCurrentRun;   // v93: lokaler Stand eines Durchlaufs (COOPRUN.TXT) bleibt in seinem Durchlauf
            CoopWriteMeta(CoopSlotDir(slot), m);
            CoopArchiveDir(CoopSlotDir(slot), m.sid, NULL, NULL);
        }
        m.list = CoopLoadingFromList;
        if (m.stime <= 0)   // v55: fuer den zeitlich naechsten Helden beim Partner; v76 bevorzugt aus COOPSYNC.TXT
            m.stime = CoopFileUnixTime(CoopSlotDir(slot) + "SAVEINFO.SAV");
        // v76: Manifest-Pruefsumme des Inhalts, den der Partner zu dieser Kennung bekommt (eigene Kopie bzw. Partnerkopie)
        if (!m.sid.empty()) {
            const std::string d = CoopMetaMine(m) ? CoopOwnCopyDir(m.sid) : CoopArchiveFind(m.sid + "__von_" + CoopSrcKey(m));   // v93
            CoopManifest man;
            if (!d.empty() && CoopDirExists(d) && CoopManifestEnsure(d, man))
                m.mh = CoopManifestHash(man);
        }
        SendMetaReq(SYNC_LOAD_REQ, m);
        SaveSyncLog("lade Slot %d (%s, Kennung %s, Welt von %s%s) - Partner uebernimmt", slot, m.name.c_str(), m.sid.empty() ? "KEINE" : m.sid.c_str(),
            m.src.c_str(), m.list ? ", aus der F9-Liste" : "");
        RemoteLoadPending = false;   // ein eigener Ladevorgang ersetzt einen offenen Auftrag
        RemoteLoadHostWins = false;
        RemoteSaveQueue.clear();
        TakeJob.active = false;
        BrowserOpen = false;
    }

    void CoopSaveSyncWorldLoaded() {
        CoopLoadEndMs = GetCurrentMs();
        if (!JoinLoadNow)
            return;
        // v76: ein Ladevorgang auf Wunsch des Partners ist durch - Stand wieder gemeinsam
        JoinLoadNow = false;
        CoopDesyncWarn = false;
        if (!PostTakeSidNext.empty()) {   // gezielte Uebernahme: gleich danach eigene Kopie dieser Kennung speichern
            PostTakeSid = PostTakeSidNext;
            PostTakeSlot = PostTakeSlotNext;
            PostTakeBy = PostTakeByNext;
            PostTakeByPc = PostTakeByPcNext;
            PostTakeName = PostTakeNameNext;
            PostTakeAt = CoopLoadEndMs + 2500;
            PostTakeSidNext.clear();
        }
        if (JoinPending) {
            JoinPending = false;
            JoinTries = 0;
            SaveSyncLog("Uebernahme nach %s abgeschlossen", JoinWhy == "resume" ? "dem Fortsetzen" : "dem Verbinden");
        }
        if (IsCoopPaused && ClientThread && !ServerThread) {   // Fortsetzen: erst jetzt (die Welt ist die des Hosts)
            IsCoopPaused = false;
            ChatLog(CoopTr("Weltabgleich l\xE4" "uft wieder.", "Restoring world synchronization."));
            SaveSyncLog("Pause beendet - Host-Stand geladen, eigener Held vom Pausenbeginn");
        }
    }

    // v24 (Wunsch des Nutzers: "nicht erst danach"): den eigenen Helden samt Lage gleich im Lade-Rueckruf
    // Game_LoadEnd_SaveGame einsetzen, bevor das erste Bild gezeichnet wird. Die Spielstand-Datei selbst
    // (binaeres Engine-Archiv) bleibt unveraendert - auch die archivierte Kopie. Der verzoegerte Weg im
    // Pulse (HandleHeroApply) bleibt als Rueckfall, falls der Rueckruf nicht kam.
    // v76: ohne SaveSyncAktiv - der Auftrag kam aus einem Coop-Ladevorgang und gilt, auch wenn die Verbindung inzwischen weg ist.
    static void CoopHeroApply(const CoopHero& h);
    void CoopSaveSyncApplyHeroAtLoad() {
        if (!PendingHeroApply || !player)
            return;
        PendingHeroApply = false;
        CoopHeroApply(PendingHero);
        SaveSyncLog("eigener Held schon im Ladevorgang eingesetzt");
        ChatLog(string::Combine(CoopTr("Stand von %s " "\xFC" "bernommen - eigener Held wiederhergestellt.", "Took over %s's state - own hero restored."), string(PendingHeroSrc.c_str())));
    }

    void CoopSaveSyncPeerJoined() {
        CoopRulesSendSoon();   // v75: Schwierigkeit und Abgleich gleich an den Neuen (sonst bis zu 5 s spaeter)
        CoopPeerCount++;
        if (CoopPeerCount == 1)
            CoopLastSaveMs = CurrentMs;   // erste Auto-Sicherung nach dem vollen Intervall
    }

    void CoopSaveSyncPeerLeft() {
        if (CoopPeerCount > 0)
            CoopPeerCount--;
    }

    int CoopSaveSyncPeers() {   // v39: verbundene Clients (Host), fuer den Rollenwechsel in Plugin.cpp
        return CoopPeerCount;
    }

    // --- Uebernahme einer fremden Welt -----------------------------------------------------------

    // v76: Uebernahme gescheitert - Grund ins Log, im Spiel bleibt eine Warnung im Bild, bis ein Laden gelingt (S5)
    static void TakeFail(const std::string& error, const std::string& chat) {
        SaveSyncLog("Uebernahme %s abgebrochen: %s", TakeJob.sid.c_str(), error.c_str());
        if (!chat.empty())
            ChatLog(string(chat.c_str()), zCOLOR(255, 80, 80, 255));
        CoopTakeError = error;
        CoopTakeErrorMs = CurrentMs;
        FilesIn.erase(TakeJob.sid + "|" + TakeJob.srcKey);
        TakeJob.active = false;
        if (player && !JoinPending)
            CoopDesyncWarn = true;
    }

    // v76: srcpc = PC der Welt, mh = Manifest-Pruefsumme laut Auftrag, targeted = Auftrag nur fuer mich (Verbinden, Fortsetzen,
    // Hauptmenue) - dann gilt der beim Pausenbeginn/Abbruch gemerkte Held (Variante b), und danach wird die eigene Kopie gespeichert.
    static void StartTakeJob(const std::string& sid, const std::string& src, const std::string& srcpc, const std::string& mh, int slot,
        const std::string& name, bool backupWanted, bool targeted) {
        TakeJob = CoopTakeJob();
        CoopTakeError.clear();
        TakeJob.active = true;
        TakeJob.sid = sid;
        TakeJob.src = src;
        TakeJob.srcpc = srcpc;
        TakeJob.srcKey = !srcpc.empty() ? srcpc : src;
        TakeJob.mh = mh;
        TakeJob.targeted = targeted;
        TakeJob.slot = slot;
        TakeJob.name = name;
        TakeJob.step = 0;
        TakeJob.since = CurrentMs;
        TakeJob.run = CoopCurrentRun;   // v93: aus dem Lade-/Speicherauftrag des Partners (CoopSaveSyncReceive)
        TakeJob.backupWanted = backupWanted && CoopBackupBeforeLoad;
        // Held: so, wie er zu dieser Kennung war (eigene Sicherung), sonst der aktuelle
        const std::string own = CoopOwnCopyDir(sid);
        long long diff = -1;
        std::string nearDir;
        if (targeted && DivergedHero.valid) {
            TakeJob.hero = DivergedHero;
            SaveSyncLog("Uebernahme %s von %s: eigener Held vom %s (Stufe %d, %u Gegenstaende)", sid.c_str(), src.c_str(), DivergedWhy.c_str(),
                DivergedHero.level, (unsigned)DivergedHero.items.size());
        }
        else if (!own.empty() && CoopHeroRead(own, TakeJob.hero)) {
            TakeJob.hero.exact = true;   // v89: Lage gehoert zu diesem Stand
            SaveSyncLog("Uebernahme %s von %s: eigener Held aus der eigenen Sicherung", sid.c_str(), src.c_str());
        }
        else if (!player && RemoteLoad.stime > 0 && !(nearDir = CoopNearestOwnHeroDir(RemoteLoad.stime, diff)).empty() &&
            CoopHeroRead(nearDir, TakeJob.hero)) {
            // v55 (Nutzer: "der Held, der von der Zeit am naechsten zu dem geladenen Save ist"): normaler Spielstand des Hosts aus
            // dem Hauptmenue - hier gibt es noch keinen aktuellen Helden.
            SaveSyncLog("Uebernahme %s von %s: kein eigener Held zu dieser Kennung - zeitlich naechster aus %s (%.1f h Abstand zur Speicherzeit)",
                sid.c_str(), src.c_str(), nearDir.c_str(), diff / 3600.0);
        }
        else if (!player) {
            SaveSyncLog("Uebernahme %s von %s: kein eigener Held gefunden - es bleibt der Held aus dem Stand des Hosts", sid.c_str(), src.c_str());
        }
        else {
            TakeJob.hero = CoopHeroCapture();
            SaveSyncLog("Uebernahme %s von %s: eigener Held = aktueller Stand", sid.c_str(), src.c_str());
        }
        ChatLog(string::Combine(CoopTr("\xDC" "bernehme den Stand von %s - eigener Held bleibt erhalten.", "Taking over %s's state - your own hero is kept."), string(src.c_str())));
    }

    static void TakeJobLoad() {
        const std::string from = CoopPartnerCopyDir(TakeJob.sid, TakeJob.srcKey);
        const int slot = (TakeJob.slot >= 1 && TakeJob.slot <= 15) ? TakeJob.slot : CoopNextReservedSlot();
        const std::string own = CoopOwnCopyDir(TakeJob.sid);
        CoopSaveMeta m;
        CoopReadMeta(from, m);
        m.sid = TakeJob.sid;
        m.src = TakeJob.src;
        if (!TakeJob.srcpc.empty())
            m.srcpc = TakeJob.srcpc;
        m.kind = "uebernommen";
        m.slot = slot;
        // v76 (S4): Slot sicher ersetzen und pruefen; Schnellleiste, Kennung und Held kommen vor dem Tausch dazu
        std::string why;
        const int n = CoopSlotReplace(slot, from, [&](const std::string& d) {
            if (!own.empty() && CoopFileExists(own + "HOTBARINSTANCES.SAV"))   // eigene Schnellleiste passt zum eigenen Inventar
                CopyFileA((own + "HOTBARINSTANCES.SAV").c_str(), (d + "HOTBARINSTANCES.SAV").c_str(), FALSE);
            CoopWriteMeta(d, m);
            CoopHero stored = TakeJob.hero;
            if (!stored.exact)
                stored.hasPos = false;   // v89: fremde Lage (anderer Stand) nicht mit diesem Stand speichern
            CoopHeroWrite(d, stored);
        }, why);
        if (n <= 0) {
            TakeFail(CoopTr("Der Stand lie\xDF sich nicht in Slot ", "The save could not be copied to slot ") + std::to_string(slot) + CoopTr(" kopieren (", " (") + why + ").", std::string(CoopTr("\xDC" "bernahme fehlgeschlagen (Dateien).", "Takeover failed (files).")));
            return;
        }
        SaveSyncLog("Uebernahme %s: Welt von %s in Slot %d (%d Dateien, geprueft), lade", TakeJob.sid.c_str(), TakeJob.src.c_str(), slot, n);
        PendingHeroApply = true;
        PendingHeroSetMs = GetCurrentMs();
        PendingHero = TakeJob.hero;
        PendingHeroSrc = TakeJob.src;
        TakeJob.active = false;
        TakeLoadTargeted = TakeJob.targeted;   // v76: danach eigene Kopie speichern (CoopSaveSyncWorldLoaded)
        if (TakeJob.targeted) {
            PostTakeSidNext = TakeJob.sid;
            PostTakeSlotNext = slot;
            PostTakeByNext = TakeJob.src;
            PostTakeByPcNext = TakeJob.srcpc;
            PostTakeNameNext = m.name.empty() ? "Coop " + CoopNowText("time") : m.name;
        }
        CoopActiveRemoteSid = m.sid;
        CoopDoLoad(slot);
        if (!CoopMenuLoadDeferred)   // v40: im Hauptmenue laedt CGameManager::Menu erst danach - Kennung bis Game_LoadBegin halten
            CoopActiveRemoteSid.clear();
    }

    // Schritt fuer Schritt: Sicherung des aktuellen Stands -> Dateien beschaffen -> laden.
    static bool TakeJobPulse() {
        if (!TakeJob.active)
            return false;
        if (TakeJob.step == 0) {
            if (TakeJob.backupWanted && !CoopBackupSavePending) {
                std::string why;
                if (CoopCanSaveNow(why)) {
                    const int slot = CoopNextReservedSlot();
                    CoopBackupForeignSlot(slot);
                    CoopBackupSavePending = true;
                    CoopDoSave(slot, "Coop Sicherung " + CoopNowText("time"));
                    CoopBackupSavePending = false;
                    SaveSyncLog("vor der Uebernahme den aktuellen Stand gesichert (Slot %d)", slot);
                }
                else
                    SaveSyncLog("Sicherung vor der Uebernahme nicht moeglich (%s) - uebersprungen", why.c_str());
            }
            TakeJob.step = 1;
            return true;
        }
        if (TakeJob.step == 1) {
            const std::string cache = CoopPartnerCopyDir(TakeJob.sid, TakeJob.srcKey);
            if (!TakeJob.requested && CoopDirExists(cache)) {
                // v76 (S2): eine schon vorhandene Kopie nur nehmen, wenn sie vollstaendig ist und zum Auftrag passt
                CoopManifest man;
                std::string why;
                bool ok = CoopManifestEnsure(cache, man) && CoopManifestCheck(cache, man, why);
                if (ok && !TakeJob.mh.empty() && CoopManifestHash(man) != TakeJob.mh) {
                    ok = false;
                    why = "anderer Inhalt als beim Absender";
                }
                if (ok) {
                    TakeJob.step = 2;
                    return true;
                }
                SaveSyncLog("vorhandene Kopie %s unbrauchbar (%s) - hole neu", cache.c_str(), why.empty() ? "nicht lesbar" : why.c_str());
                CoopSetAside(cache, "unbrauchbar");
            }
            if (TakeJob.requested && CoopDirExists(cache)) {   // Empfang fertig und geprueft (ReceiveFileEnd)
                TakeJob.step = 2;
                return true;
            }
            const std::string key = TakeJob.sid + "|" + TakeJob.srcKey;
            if (!TakeJob.requested) {
                // v76: an den Spieler, der diesen PC gerade hat (Rolle kann sich seit dem Speichern geaendert haben)
                std::string to = CoopRoleOfPc(TakeJob.srcpc);
                if (to.empty())
                    to = TakeJob.src;
                if (to.empty() || to == CoopMe()) {
                    TakeFail(std::string(CoopTr("Der PC mit diesem Stand ist nicht verbunden.", "The PC with this save is not connected.")), CoopTr("Stand ", "Save ") + TakeJob.sid + CoopTr(": der PC mit diesem Stand ist nicht verbunden - NICHT " "\xFC" "bernommen.", ": the PC with this save is not connected - NOT taken over."));
                    return false;
                }
                FilesIn.erase(key);   // v71: Zaehler einer frueheren, abgebrochenen Uebertragung derselben Kennung nicht mitzaehlen
                std::string part = cache;
                part.erase(part.size() - 1);
                CoopRemoveTempDir(part + ".part\\");   // v76: Reste einer abgebrochenen Uebertragung
                json j;
                j["to"] = to;
                j["sid"] = TakeJob.sid;
                if (!TakeJob.srcpc.empty())
                    j["srcpc"] = TakeJob.srcpc;   // v76: welcher PC die Welt hat (der Angefragte prueft es)
                if (CoopZipReady())
                    j["z"] = 1;       // v71: gepackt schicken
                SaveSyncSend(j, SYNC_FILE_REQ);
                TakeJob.requested = true;
                TakeJob.since = CurrentMs;
                CoopTakeChatMs = CurrentMs;
                SaveSyncLog("fordere Stand %s bei %s an%s", TakeJob.sid.c_str(), to.c_str(), CoopZipReady() ? " (gepackt)" : "");
            }
            else {
                // v71: abbrechen erst, wenn 120 s lang NICHTS mehr ankommt - vorher 180 s ab der Anforderung, auch mitten in
                // einer langsamen, aber laufenden Uebertragung (8,6 MB brauchten im Test v69 rund 70 s).
                auto it = FilesIn.find(key);
                const long long last = (it != FilesIn.end() && it->second.lastMs > TakeJob.since) ? it->second.lastMs : TakeJob.since;
                if (CurrentMs > last + 120000) {
                    TakeFail(std::string(CoopTr("Vom Host kam 120 s lang nichts mehr an - nicht geladen.", "Nothing arrived from the host for 120 s - not loaded.")),
                        CoopTr("Stand von ", "State from ") + TakeJob.src + CoopTr(" kam nicht an - NICHT " "\xFC" "bernommen, St" "\xE4" "nde weichen ab!", " did not arrive - NOT taken over, states differ!"));
                }
                else if (player && it != FilesIn.end() && it->second.total > 0 && CurrentMs > CoopTakeChatMs + 10000) {
                    CoopTakeChatMs = CurrentMs;   // im Spiel: alle 10 s Fortschritt im Chat
                    ChatLog(string::Combine(CoopTr("Stand von %s: %i %% (%i von %i KB)", "State from %s: %i %% (%i of %i KB)"), string(TakeJob.src.c_str()),
                        (int)(it->second.bytes * 100 / it->second.total), (int)(it->second.bytes / 1024), (int)(it->second.total / 1024)));
                }
            }
            return false;
        }
        if (PendingArchiveSlot >= 0)
            return false;   // erst die Sicherung fertig archivieren
        if (CoopLoadBlockedByMenu("Uebernahme"))
            return false;   // v80: Stand liegt bereit, geladen wird nach dem Menue
        TakeJobLoad();
        return true;
    }

    // --- Empfang ---------------------------------------------------------------------------------

    // --- v39: vor jeder gemeinsamen Sicherung alle fragen -----------------------------------------------------------
    //
    // Nutzer: "wenn das Speichern nicht moeglich ist, weil einer beschaeftigt ist, dann soll keine Sicherung angelegt werden,
    // sondern erst eine Minute spaeter - so oft, bis es klappt". Bisher speicherte der Host (Auto-Sicherung, Abgleich) sofort
    // und archivierte; ein beschaeftigter Client gab nach 60 s auf ("NICHT mitgespeichert Dialog") - die Sicherung des Hosts
    // stand ohne Gegenstueck. Jetzt fragt der Host erst alle Clients (SAVE_CHECK -> SAVE_READY, 5 s Frist); gespeichert und
    // archiviert wird nur, wenn alle (und er selbst) bereit sind, sonst neuer Versuch nach einer Minute, ohne Ende.
    static int CoopCheckKind = 0;                 // 0 keine Frage offen, 1 Auto-Sicherung, 2 Abgleich
    static long long CoopCheckCid = 0, CoopCheckDeadline = 0;
    static int CoopCheckAnswers = 0;
    static std::map<std::string, std::string> CoopCheckBusy;   // Spieler -> Grund
    static long long CoopHostSyncRetryMs = 0;

    static void CoopCheckStart(int kind) {
        CoopCheckKind = kind;
        CoopCheckCid = CurrentMs;
        CoopCheckDeadline = CurrentMs + 5000;
        CoopCheckAnswers = 0;
        CoopCheckBusy.clear();
        json j;
        j["cid"] = std::to_string(CoopCheckCid);
        SaveSyncSend(j, SYNC_SAVE_CHECK);
    }

    static void CoopCheckAnswer(const std::string& id, const json& e) {
        if (!CoopCheckKind || !e.contains("cid") || !e["cid"].is_string() || e["cid"].get<std::string>() != std::to_string(CoopCheckCid))
            return;
        CoopCheckAnswers++;
        const bool ok = e.contains("ok") && e["ok"].is_number() && e["ok"].get<int>() != 0;
        if (!ok)
            CoopCheckBusy[id] = e.contains("why") && e["why"].is_string() ? e["why"].get<std::string>() : std::string("beschaeftigt");
    }

    // 1 alle bereit, 0 noch offen, -1 jemand beschaeftigt oder keine Antwort (why = Grund)
    static int CoopCheckResult(std::string& why) {
        if (!CoopCheckBusy.empty()) {
            why.clear();
            for (auto& b : CoopCheckBusy)
                why += (why.empty() ? "" : ", ") + b.first + ": " + b.second;
            return -1;
        }
        if (CoopCheckAnswers >= CoopPeerCount)
            return 1;
        if (CurrentMs > CoopCheckDeadline) {
            why = "keine Antwort vom Mitspieler";
            return -1;
        }
        return 0;
    }

    bool CoopSaveSyncHandles(int type) {
        return (type >= SYNC_SAVE_REQ && type <= SYNC_STATE_REQ) || type == SYNC_SAVE_CHECK || type == SYNC_SAVE_READY;
    }

    static CoopSaveMeta MetaFromJson(const json& e) {
        CoopSaveMeta m;
        m.sid = e.contains("sid") && e.at("sid").is_string() ? e.at("sid").get<std::string>() : std::string();
        m.kind = e.contains("kind") && e.at("kind").is_string() ? e.at("kind").get<std::string>() : std::string();
        m.name = e.contains("name") && e.at("name").is_string() ? e.at("name").get<std::string>() : std::string();
        m.world = e.contains("world") && e.at("world").is_string() ? e.at("world").get<std::string>() : std::string();
        m.time = e.contains("time") && e.at("time").is_string() ? e.at("time").get<std::string>() : std::string();
        m.src = e.contains("src") && e.at("src").is_string() ? e.at("src").get<std::string>() : std::string();
        m.slot = e.contains("slot") && e.at("slot").is_number() ? e.at("slot").get<int>() : -1;
        m.take = e.contains("take") && e.at("take").is_number() && e.at("take").get<int>() != 0;
        m.list = e.contains("list") && e.at("list").is_number() && e.at("list").get<int>() != 0;
        m.ask = e.contains("ask") && e.at("ask").is_number() ? e.at("ask").get<int>() : 0;   // v90
        m.stime = e.contains("stime") && e.at("stime").is_number() ? e.at("stime").get<long long>() : 0;
        m.srcpc = e.contains("srcpc") && e.at("srcpc").is_string() ? e.at("srcpc").get<std::string>() : std::string();   // v76
        m.srcname = e.contains("srcname") && e.at("srcname").is_string() ? e.at("srcname").get<std::string>() : std::string();
        m.mh = e.contains("mh") && e.at("mh").is_string() ? e.at("mh").get<std::string>() : std::string();
        m.bypc = e.contains("pc") && e.at("pc").is_string() ? e.at("pc").get<std::string>() : std::string();
        m.run = e.contains("run") && e.at("run").is_string() ? e.at("run").get<std::string>() : std::string();   // v93
        if (m.run.find_first_of("\\/:*?\"<>|") != std::string::npos || m.run.find("..") != std::string::npos)
            m.run.clear();   // wird ein Ordnername - nur sichere Zeichen
        return m;
    }

    // v76: nur Stuecke des gerade angeforderten Stands annehmen (Kennung und PC bzw. Rolle des Absenders)
    static bool TakeExpects(const json& e, const std::string& sid) {
        if (!TakeJob.active || !TakeJob.requested || sid != TakeJob.sid)
            return false;
        const std::string pc = e.contains("srcpc") && e["srcpc"].is_string() ? e["srcpc"].get<std::string>() : std::string();
        const std::string src = e.contains("src") && e["src"].is_string() ? e["src"].get<std::string>() : std::string();
        if (!pc.empty() && !TakeJob.srcpc.empty())
            return pc == TakeJob.srcpc;
        return src == TakeJob.src || src == CoopRoleOfPc(TakeJob.srcpc);
    }

    static void ReceiveFilePart(json& e) {
        const std::string sid = e.at("sid").get<std::string>();
        const std::string src = e.at("src").get<std::string>();
        const std::string fname = e.at("f").get<std::string>();
        const int i = e.at("i").get<int>();
        const int n = e.contains("n") && e["n"].is_number() ? e["n"].get<int>() : 1;
        if (fname.find_first_of("\\/:") != std::string::npos || fname.find("..") != std::string::npos)
            return;   // nur flache Dateinamen
        if (!TakeExpects(e, sid)) {
            static CoopDiagBudget budget = { 0, 0 };
            if (CoopDiagAllow(budget, 5))
                SaveSyncLog("Dateistueck %s von %s (%s) nicht angefordert - verworfen", fname.c_str(), src.c_str(), sid.c_str());
            return;
        }
        const std::string key = sid + "|" + TakeJob.srcKey;
        CoopFileIn& in = FilesIn[key];
        const long long now = GetCurrentMs();
        if (in.dir.empty()) {
            // v93: im Ordner des Durchlaufs (fertig benannt wird im selben Ordner, CoopPartnerCopyDir mit TakeJob.run)
            in.dir = CoopArchivePath(sid + "__von_" + TakeJob.srcKey + ".part", TakeJob.run);
            if (!TakeJob.run.empty())
                CoopRunFolderEnsure(TakeJob.run);
            CreateDirectoryA(in.dir.c_str(), NULL);
        }
        if (!in.startMs)
            in.startMs = in.logMs = now;
        in.lastMs = now;
        if (in.failed)
            return;   // v76: nach einem Fehler nichts mehr schreiben (FILE_END bricht ab)
        if (e.contains("tb") && e["tb"].is_number())
            in.total = e["tb"].get<long long>();
        if (e.contains("tf") && e["tf"].is_number())
            in.filesTotal = e["tf"].get<int>();
        // v76 (S2): Stuecke einer Datei lueckenlos der Reihe nach - sonst ist die Datei kaputt
        if (i == 0) {
            in.curFile = fname;
            in.nextIdx = 0;
            in.hasCrc = e.contains("crc") && e["crc"].is_number();
            in.crc = in.hasCrc ? e["crc"].get<unsigned>() : 0;
            in.fileSize = e.contains("fs") && e["fs"].is_number() ? e["fs"].get<long long>() : -1;
        }
        if (fname != in.curFile || i != in.nextIdx) {
            in.failed = true;
            in.why = fname + ": Stueck " + std::to_string(i) + " ausser der Reihe (erwartet " + in.curFile + " " + std::to_string(in.nextIdx) + ")";
            SaveSyncLog("Stand %s von %s: %s", sid.c_str(), src.c_str(), in.why.c_str());
            return;
        }
        in.nextIdx++;
        const bool packed = e.contains("z");   // v71: gepackt - erst nach dem letzten Stueck entpacken
        const std::string target = in.dir + fname + (packed ? ".cz" : "");
        FILE* f = fopen(target.c_str(), i == 0 ? "wb" : "ab");
        if (!f) {
            in.failed = true;
            in.why = fname + " nicht schreibbar";
            return;
        }
        const auto& bin = e.at("b").get_binary();
        const bool written = bin.empty() || fwrite(&bin[0], 1, bin.size(), f) == bin.size();
        if (fclose(f) != 0 || !written) {   // v76: Platte voll o. ae.
            in.failed = true;
            in.why = fname + ": Schreiben fehlgeschlagen (Platte voll?)";
            SaveSyncLog("Stand %s von %s: %s", sid.c_str(), src.c_str(), in.why.c_str());
            return;
        }
        in.bytes += (long long)bin.size();
        if (i >= n - 1) {   // Datei vollstaendig (Stuecke kommen der Reihe nach, ENet zuverlaessig)
            if (packed) {
                const size_t raw = e.contains("raw") && e["raw"].is_number() ? (size_t)e["raw"].get<long long>() : 0;
                std::vector<std::uint8_t> z, out;
                bool ok = CoopReadWholeFile(target, z) && CoopZipUnpack(z, raw, out);
                if (ok) {
                    FILE* o = fopen((in.dir + fname).c_str(), "wb");
                    ok = o && (out.empty() || fwrite(&out[0], 1, out.size(), o) == out.size());
                    if (o && fclose(o) != 0)
                        ok = false;
                }
                DeleteFileA(target.c_str());
                if (!ok) {
                    in.failed = true;
                    in.why = fname + " liess sich nicht entpacken";
                    SaveSyncLog("Stand %s von %s: %s", sid.c_str(), src.c_str(), in.why.c_str());
                }
                in.raw += (long long)raw;
            }
            else
                in.raw += e.contains("size") && e["size"].is_number() ? e["size"].get<long long>() : 0;
            // v76: Groesse und CRC32 der fertigen Datei pruefen
            if (!in.failed && (in.hasCrc || in.fileSize >= 0)) {
                long long size = 0;
                unsigned crc = 0;
                if (!CoopFileCrc(in.dir + fname, size, crc) || (in.fileSize >= 0 && size != in.fileSize) || (in.hasCrc && crc != in.crc)) {
                    in.failed = true;
                    in.why = fname + ": Groesse/Pruefsumme falsch";
                    SaveSyncLog("Stand %s von %s: %s (%lld Bytes, CRC %08X, erwartet %lld, %08X)", sid.c_str(), src.c_str(), in.why.c_str(),
                        size, crc, in.fileSize, in.crc);
                }
            }
            in.files++;
        }
        if (now >= in.logMs + 5000 && in.total > 0) {   // v71: Fortschritt alle 5 s ins Log
            in.logMs = now;
            const double secs = (now - in.startMs) / 1000.0;
            SaveSyncLog("Empfang %s von %s: %.1f von %.1f MB (%d %%), %d von %d Dateien, %.0f KB/s, Ping %d ms", sid.c_str(), src.c_str(),
                in.bytes / 1048576.0, in.total / 1048576.0, (int)(in.bytes * 100 / in.total), in.files, in.filesTotal,
                secs > 0.1 ? in.bytes / 1024.0 / secs : 0.0, (int)CurrentPing);
        }
    }

    static void ReceiveFileEnd(json& e) {
        const std::string sid = e.at("sid").get<std::string>();
        const std::string src = e.at("src").get<std::string>();
        if (!TakeExpects(e, sid))
            return;
        if (e.contains("bad") && e["bad"].is_string()) {   // v76: der Angefragte hat den Stand nicht (oder nur beschaedigt)
            const std::string bad = e["bad"].get<std::string>();
            TakeFail(CoopTr("Stand beim Absender nicht verf" "\xFC" "gbar: ", "State not available at the sender: ") + bad, CoopTr("Stand von ", "State from ") + src + CoopTr(" nicht verf" "\xFC" "gbar (", " not available (") + bad + CoopTr(") - NICHT " "\xFC" "bernommen.", ") - NOT taken over."));
            return;
        }
        const int files = e.contains("files") && e["files"].is_number() ? e["files"].get<int>() : 0;
        const std::string key = sid + "|" + TakeJob.srcKey;
        auto it = FilesIn.find(key);
        if (it == FilesIn.end() || it->second.files != files || files <= 0 || it->second.failed) {
            char b[300];
            _snprintf(b, sizeof(b), CoopTr("Stand unvollst" "\xE4" "ndig (%d von %d Dateien%s%s)", "State incomplete (%d of %d files%s%s)"), it == FilesIn.end() ? 0 : it->second.files, files,
                it != FilesIn.end() && it->second.failed ? ", " : "", it != FilesIn.end() ? it->second.why.c_str() : "");
            b[sizeof(b) - 1] = 0;
            TakeFail(b, CoopTr("Stand von ", "State from ") + src + CoopTr(" kam unvollst" "\xE4" "ndig an - NICHT " "\xFC" "bernommen.", " arrived incomplete - NOT taken over."));
            return;
        }
        const CoopFileIn in = it->second;
        FilesIn.erase(it);
        // v76: Manifest der empfangenen Spieldateien bilden und mit dem Absender vergleichen
        CoopManifest man;
        std::string why;
        if (!CoopManifestBuild(in.dir, man))
            why = "empfangene Dateien nicht lesbar";
        else if (e.contains("mh") && e["mh"].is_string() && CoopManifestHash(man) != e["mh"].get<std::string>())
            why = "Pruefsumme des Stands passt nicht zum Absender";
        else if (!CoopManifestWrite(in.dir, man))
            why = "Manifest nicht schreibbar";
        if (!why.empty()) {
            TakeFail(why, CoopTr("Stand von ", "State from ") + src + CoopTr(" besch" "\xE4" "digt angekommen - NICHT " "\xFC" "bernommen.", " arrived damaged - NOT taken over."));
            return;
        }
        std::string part = in.dir;
        std::string done = CoopPartnerCopyDir(sid, TakeJob.srcKey, TakeJob.run);   // v93: neu im Ordner des Durchlaufs
        if (CoopDirExists(done))
            CoopSetAside(done, "alt");
        CoopArchiveIndexValid = false;
        part.erase(part.size() - 1);
        done.erase(done.size() - 1);
        if (!MoveFileA(part.c_str(), done.c_str())) {
            TakeFail("empfangener Stand nicht umbenennbar (Fehler " + std::to_string(GetLastError()) + ")", CoopTr("Stand von ", "State from ") + src + CoopTr(" konnte nicht abgelegt werden.", " could not be stored."));
            return;
        }
        const double secs = in.startMs ? (GetCurrentMs() - in.startMs) / 1000.0 : 0.0;
        SaveSyncLog("Stand %s von %s empfangen und geprueft (%d Dateien, %.1f MB, uebertragen %.1f MB in %.1f s = %.0f KB/s)", sid.c_str(), src.c_str(), files,
            in.raw / 1048576.0, in.bytes / 1048576.0, secs, secs > 0.1 ? in.bytes / 1024.0 / secs : 0.0);
    }

    static void JoinStart(const char* why, bool retry);   // weiter unten (v76)
    static void JoinEnd(const char* why);
    static void PauseActivate(const char* how);

    // --- v90: Client laedt im Spiel - erst den Host fragen ---------------------------------------------------------------
    // Test v89: "es hat irgendwie zweimal geladen beim Client". Laedt ein Client selbst einen gemeinsamen Stand, laedt er ihn
    // sofort; der Host laedt dann seine eigene Kopie dieser Kennung (v29 "Host-Kopie gilt") und schickt einen Ladeauftrag - der
    // Client laedt ein zweites Mal (Log: Slot 5 und Slot 2, beim zweiten Mal mitten im Dialog). Jetzt schickt der Client im Spiel
    // aus dem Ladefenster nur einen Ladewunsch (LOAD_REQ mit ask 1, CoopMenuLobby.cpp schliesst die Menues ohne zu laden):
    //  - Host hat eine eigene Kopie der Kennung: er laedt sie wie selbst gewaehlt, sein Ladeauftrag laesst den Client EINMAL laden.
    //  - sonst Antwort ask 2: der Client laedt selbst wie bisher (der Host uebernimmt seine Welt). Ohne Antwort nach 8 s ebenso.
    // Allgemein (GT-Speicher-Abgleich).
    static int AskLoadSlot = -1;           // Client: eigener Ladewunsch offen
    static std::string AskLoadSid;
    static long long AskLoadSince = 0;
    static bool AskLoadSelf = false;       // Client: Host hat keine Kopie - selbst laden
    static int HostAskSlot = -1;           // Host: Ladewunsch eines Clients, laedt im naechsten freien Takt
    static std::string HostAskSid, HostAskBy;

    static void HostLoadAsk(const std::string& from, const CoopSaveMeta& q) {
        std::string why;
        const std::string own = q.sid.empty() ? std::string() : CoopOwnCopyDir(q.sid);
        int slot = -1;
        if (own.empty())
            why = CoopTr("keine eigene Kopie dieser Kennung", "no own copy of this id");
        else if (HostAskSlot >= 0 || TakeJob.active || RemoteLoadPending || IsLoadingLevel)
            why = CoopTr("Host l\xE4" "dt gerade", "host is loading");
        else   // v93: liegt sie schon in einem eigenen Slot, diesen; sonst aus dem Archiv in einen Coop-Slot (nicht den Slot des Clients)
            slot = CoopSaveSyncPrepareSlot(q.sid, why);
        if (slot < 0) {
            CoopSaveMeta n = q;
            n.ask = 2;
            SendMetaReq(SYNC_LOAD_REQ, n, from);
            SaveSyncLog("Ladewunsch von %s (%s): %s - er l\xE4" "dt selbst, der Host \xFC" "bernimmt", from.c_str(), q.sid.c_str(), why.c_str());
            return;
        }
        HostAskSlot = slot;
        HostAskSid = q.sid;
        HostAskBy = from;
        SaveSyncLog("Ladewunsch von %s: %s - der Host l\xE4" "dt seine Kopie (Slot %d), %s l\xE4" "dt einmal mit", from.c_str(), q.sid.c_str(), slot, from.c_str());
        ChatLog(string::Combine(CoopTr("%s m\xF6" "chte einen Stand laden - lade ihn f\xFC" "r beide.", "%s wants to load a save - loading it for both."), string(from.c_str())));
    }

    // true = geladen (in diesem Bild danach nichts mehr)
    static bool HandleHostAsk() {
        if (HostAskSlot < 0)
            return false;
        if (!ServerThread) {
            HostAskSlot = -1;
            return false;
        }
        if (CoopLoadBlockedByMenu("Ladewunsch"))
            return false;
        const int slot = HostAskSlot;
        HostAskSlot = -1;
        SaveSyncLog("Ladewunsch von %s: lade Slot %d (%s)", HostAskBy.c_str(), slot, HostAskSid.c_str());
        CoopDoLoad(slot);   // eigener Ladevorgang: CoopSaveSyncOnLoadBegin schickt den Ladeauftrag an alle
        return true;
    }

    // v91: Spielstand-Liste im Esc-Menue (CoopMenuLobby.cpp, LobbyIngameRow) - Host bzw. ohne Coop: laden, sobald die Menues zu sind
    // (und kein Dialog laeuft); der Host meldet es wie jedes eigene Laden dem Mitspieler (CoopSaveSyncOnLoadBegin)
    static int OwnListLoadSlot = -1;

    void CoopSaveSyncLoadSoon(int slot) {
        OwnListLoadSlot = slot;
        SaveSyncLog("Liste im Esc-Men\xFC: Slot %d gew\xE4hlt - lade, sobald die Men\xFC" "s zu sind", slot);
    }

    static bool HandleOwnListLoad() {
        if (OwnListLoadSlot < 0 || !player || IsLoadingLevel)
            return false;
        if (CoopLoadBlockedByMenu("Liste im Esc-Men\xFC"))
            return false;
        const int slot = OwnListLoadSlot;
        OwnListLoadSlot = -1;
        SaveSyncLog("Liste im Esc-Men\xFC: lade Slot %d%s", slot, ServerThread ? " - der Mitspieler l\xE4" "dt mit" : "");
        CoopDoLoad(slot);
        return true;
    }

    // CoopMenuLobby.cpp: Client waehlt im Spiel im Ladefenster einen Stand. true = Wunsch an den Host geschickt (nicht laden).
    bool CoopSaveSyncAskLoad(int slot) {
        if (!player || !ClientThread || ServerThread || CoopClientState != 2 || !SaveSyncAktiv() || CoopSyncSuspended() || slot < 1)
            return false;
        CoopSaveMeta m;
        if (!CoopReadMeta(CoopSlotDir(slot), m) || m.sid.empty())
            return false;   // ohne Coop-Kennung: wie bisher selbst laden (der Host bekommt dann den Hinweis)
        m.slot = slot;
        if (m.name.empty())
            m.name = CoopSlotName(slot);
        if (m.src.empty())
            m.src = CoopMe();
        m.ask = 1;
        SendMetaReq(SYNC_LOAD_REQ, m);
        AskLoadSlot = slot;
        AskLoadSid = m.sid;
        AskLoadSince = CurrentMs;
        AskLoadSelf = false;
        SaveSyncLog("Ladewunsch an den Host: Slot %d (%s, Welt von %s) - erst der Host, dann einmal hier", slot, m.sid.c_str(), m.src.c_str());
        ChatLog(CoopTr("Ladewunsch an den Host geschickt - es wird gleich einmal f\xFC" "r beide geladen.", "Load request sent to the host - it will load once for both."));
        return true;
    }

    // v93: Liste "Spielstand waehlen" im Spiel - Mitspieler waehlt einen gemeinsamen Stand ueber die Kennung (er muss in keinem Slot
    // liegen). AskLoadSlot 0 = erst beim Selbstladen aus dem Archiv in einen Slot legen.
    bool CoopSaveSyncAskLoadSid(const std::string& sid) {
        if (!player || !ClientThread || ServerThread || CoopClientState != 2 || !SaveSyncAktiv() || CoopSyncSuspended() || sid.empty())
            return false;
        const std::string own = CoopOwnCopyDir(sid);
        CoopSaveMeta m;
        if (own.empty() || !CoopReadMeta(own, m) || m.sid != sid)
            return false;
        m.slot = 0;   // der Host waehlt seinen Slot selbst
        if (m.src.empty())
            m.src = CoopMe();
        m.ask = 1;
        SendMetaReq(SYNC_LOAD_REQ, m);
        AskLoadSlot = 0;
        AskLoadSid = sid;
        AskLoadSince = CurrentMs;
        AskLoadSelf = false;
        SaveSyncLog("Ladewunsch an den Host: %s (Liste, Durchlauf %s) - erst der Host, dann einmal hier", sid.c_str(), m.run.empty() ? "-" : m.run.c_str());
        ChatLog(CoopTr("Ladewunsch an den Host geschickt - es wird gleich einmal f\xFC" "r beide geladen.", "Load request sent to the host - it will load once for both."));
        return true;
    }

    static void ClientAskAnswered(const CoopSaveMeta& q) {
        if (AskLoadSlot < 0 || q.sid != AskLoadSid)
            return;
        AskLoadSelf = true;
        SaveSyncLog("Ladewunsch %s: der Host hat keine Kopie - lade selbst (der Host \xFC" "bernimmt)", q.sid.c_str());
    }

    static bool HandleClientAsk() {
        if (AskLoadSlot < 0)
            return false;
        if (ServerThread || !ClientThread) {
            AskLoadSlot = -1;
            return false;
        }
        if (!AskLoadSelf && CurrentMs < AskLoadSince + 8000)
            return false;   // Ladeauftrag des Hosts abwarten
        if (CoopLoadBlockedByMenu("eigener Ladewunsch"))
            return false;
        int slot = AskLoadSlot;
        AskLoadSlot = -1;
        if (!AskLoadSelf)
            SaveSyncLog("Ladewunsch %s: keine Antwort des Hosts in 8 s - lade selbst", AskLoadSid.c_str());
        AskLoadSelf = false;
        if (slot <= 0) {   // v93: aus der Liste (Kennung) - jetzt in einen Slot legen
            std::string why;
            slot = CoopSaveSyncPrepareSlot(AskLoadSid, why);
            if (slot < 0) {
                SaveSyncLog("Ladewunsch %s: nicht ladbar (%s)", AskLoadSid.c_str(), why.c_str());
                ChatLog(string::Combine(CoopTr("Stand nicht ladbar (%s).", "Cannot load save (%s)."), string(why.c_str())), zCOLOR(255, 80, 80, 255));
                return false;
            }
        }
        CoopDoLoad(slot);   // wie bisher: CoopSaveSyncOnLoadBegin meldet es dem Host
        return true;
    }

    void CoopSaveSyncReceive(json& e, const std::string& id) {
        const int type = e["type"].get<int>();
        try {
            if (!SaveSyncAktiv())
                return;
            // v76: PC-Kennung des Absenders merken (weitergeleitete Pakete: der Urheber steht in "by")
            {
                const std::string pc = e.contains("pc") && e["pc"].is_string() ? e["pc"].get<std::string>() : std::string();
                const std::string who = e.contains("by") && e["by"].is_string() ? e["by"].get<std::string>() : id;
                CoopNotePc(pc, who);
            }
            // v76: Speicher-Pakete kommen auch in der Pause an (PacketProcessor.cpp) - in der Pause macht der Client nur, was zu
            // seiner eigenen Pause/Fortsetzung gehoert; alles andere uebergeht er.
            const bool paused = !ServerThread && IsCoopPaused;
            // Dateien und Abgleichsanfragen sind gezielt ("to"), sie werden nicht verteilt.
            if (type == SYNC_FILE_REQ || type == SYNC_FILE_PART || type == SYNC_FILE_END) {
                const std::string to = e.contains("to") && e["to"].is_string() ? e["to"].get<std::string>() : std::string();
                if (to != CoopMe())
                    return;
                if (type == SYNC_FILE_PART)
                    ReceiveFilePart(e);
                else if (type == SYNC_FILE_END)
                    ReceiveFileEnd(e);
                else {
                    const std::string sid = e.at("sid").get<std::string>();
                    const std::string want = e.contains("srcpc") && e["srcpc"].is_string() ? e["srcpc"].get<std::string>() : std::string();
                    const bool mine = want.empty() || want == CoopMyPc();
                    const std::string dir = mine ? CoopOwnCopyDir(sid) : std::string();
                    const bool zip = e.contains("z") && e["z"].is_number() && e["z"].get<int>() != 0;
                    std::string bad;
                    if (dir.empty())
                        bad = mine ? "hier nicht vorhanden" : "gehoert einem anderen PC";
                    else
                        CoopQueueSaveFiles(dir, sid, id, zip, bad);
                    if (!bad.empty()) {   // v76: Absage, damit der Anfordernde nicht 120 s wartet
                        SaveSyncLog("%s fordert Stand %s an - %s", id.c_str(), sid.c_str(), bad.c_str());
                        json j;
                        j["to"] = id;
                        j["sid"] = sid;
                        j["src"] = CoopMe();
                        j["srcpc"] = CoopMyPc();
                        j["files"] = 0;
                        j["bad"] = bad;
                        j["type"] = SYNC_FILE_END;
                        FileOutQueue.push_back(j);
                    }
                }
                return;
            }
            if (type == SYNC_STATE_REQ) {
                if (ServerThread) {
                    const std::string why = e.contains("why") && e["why"].is_string() ? e["why"].get<std::string>() : std::string();
                    if (why == "pause") {   // v76: Client will pausieren - sofort gemeinsame Sicherung, ohne Vorab-Frage
                        HostPauseSavePending = true;
                        HostPauseSaveUntil = CurrentMs + 20000;
                        HostPauseSaveFor = id;
                        SaveSyncLog("%s pausiert - gemeinsame Sicherung jetzt", id.c_str());
                        return;
                    }
                    const bool join = why == "connect" || why == "resume";
                    // v75: "Beim Verbinden: eigenen Stand behalten" - die Anfrage nach dem Verbinden gilt dann nicht (F6 schon)
                    if (join && !CoopRuleTakeOnConnect()) {
                        SaveSyncLog("%s %s - Host-Stand wird nicht uebernommen (Einstellung: eigenen Stand behalten)", id.c_str(),
                            why == "resume" ? "setzt fort" : "ist verbunden");
                        json j;   // v76: Absage, damit der Client nicht wartet
                        j["to"] = id;
                        j["nojoin"] = 1;
                        j["sid"] = "";
                        SaveSyncSend(j, SYNC_LOAD_REQ);
                        return;
                    }
                    if (join) {   // v76: nur er uebernimmt (gezielter Ladeauftrag nach der gemeinsamen Sicherung)
                        TakePeers.insert(id);
                        if (!CoopHostSyncPending) {
                            CoopHostSyncPending = true;
                            CoopHostSyncSince = CurrentMs;
                            CoopHostSyncMenuOnly = true;
                        }
                        CoopHostSyncRetryMs = 0;
                        SaveSyncLog("%s %s - gemeinsame Sicherung, danach uebernimmt er den Host-Stand", id.c_str(),
                            why == "resume" ? "setzt fort" : "ist verbunden");
                        return;
                    }
                    CoopHostSyncPending = true;
                    CoopHostSyncSince = CurrentMs;
                    CoopHostSyncMenuOnly = false;   // v71: F6 beim Client - alle uebernehmen
                    SaveSyncLog("%s moechte den Host-Stand uebernehmen - gemeinsame Sicherung folgt", id.c_str());
                }
                return;
            }
            if (type == SYNC_SAVE_CHECK) {   // v39: Host fragt vor einer gemeinsamen Sicherung, ob gerade gespeichert werden kann
                if (!ServerThread) {
                    std::string why;
                    bool ok = CoopCanSaveNow(why);
                    if (!player) {   // v71: im Hauptmenue speichert der Client nicht mit - kein Grund, die Sicherung aufzuhalten
                        ok = true;   // (bis v70: "kein Held" -> der Host verschob jede Sicherung, solange er verbunden war)
                        why = "Hauptmenue";
                    }
                    else if (paused || JoinPending) {   // v76: speichert nicht mit - den Host nicht aufhalten
                        ok = true;
                        why = paused ? "Pause" : "uebernimmt gleich den Host-Stand";
                    }
                    else if (ok && (RemoteLoadPending || PendingHeroApply)) {
                        ok = false;
                        why = "Laden laeuft";
                    }
                    json j;
                    j["cid"] = e.contains("cid") ? e["cid"] : json(0);
                    j["ok"] = ok ? 1 : 0;
                    j["why"] = why;
                    SaveSyncSend(j, SYNC_SAVE_READY);
                }
                return;
            }
            if (type == SYNC_SAVE_READY) {
                if (ServerThread && !e.contains("by"))
                    CoopCheckAnswer(id, e);
                return;
            }

            // Client-Pakete kommen ohne "by"; der Host handelt und verteilt mit "by". Die automatisch
            // weitergeleitete Client-Fassung (ohne "by") ignorieren die anderen Clients.
            const bool relay = e.contains("by") && e["by"].is_string();
            const std::string by = relay ? e["by"].get<std::string>() : id;
            if (!ServerThread && !relay)
                return;
            if (by == MyselfId.ToChar())
                return;
            if (type == SYNC_LOAD_REQ && e.contains("to") && e["to"].is_string() && e["to"].get<std::string>() != CoopMe())
                return;   // v71: Ladeauftrag des Hosts fuer einen Client im Hauptmenue
            if (type == SYNC_LOAD_REQ && e.contains("ask")) {   // v90: Ladewunsch bzw. Antwort - nicht weiterleiten, hier nicht laden
                const CoopSaveMeta q = MetaFromJson(e);
                if (q.ask == 1 && ServerThread && !relay)
                    HostLoadAsk(id, q);
                else if (q.ask == 2 && !ServerThread)
                    ClientAskAnswered(q);
                return;
            }
            // v29 (Nutzer: "Eintrag verschwindet auch beim Host"): Laedt ein Client einen gemeinsamen Stand, gilt die
            // Host-Kopie dieser Kennung - der Host laedt sie und schickt dann selbst die Aufforderung (Welt von HOST).
            // Die Client-Fassung wird dann nicht weiterverteilt. Ohne eigene Kopie beim Host: wie bisher.
            bool hostWins = false;
            if (ServerThread && !relay && type == SYNC_LOAD_REQ) {
                const CoopSaveMeta q = MetaFromJson(e);
                // Ausnahme: bewusst aus der F9-Liste gewaehlt (Notfallweg F2: Client stellt mit seiner Sicherung wieder her)
                // v76: "fremde Welt" nach PC-Kennung (CoopMetaMine), alte Pakete ohne srcpc wie bisher nach Rolle
                hostWins = !q.list && !q.sid.empty() && !q.src.empty() && !CoopMetaMine(q) && !CoopOwnCopyDir(q.sid).empty();
            }
            if (ServerThread && !relay && type != SYNC_SAVE_ACK && !hostWins) {
                json j = e;
                j["by"] = id;
                j["id"] = MyselfId.ToChar();
                CoopTraceCount(TRACE_SENT, type);
                ReadyToSendJsons.enqueue(j);
            }

            switch (type) {
            case SYNC_SAVE_REQ:
            {
                if (!player) {   // v71: im Hauptmenue gibt es nichts mitzuspeichern (sonst speicherte er nach dem Laden nach)
                    SaveSyncLog("%s hat gespeichert (%s) - im Hauptmenue, nicht mitgespeichert", by.c_str(), MetaFromJson(e).sid.c_str());
                    break;
                }
                CoopSaveMeta r = MetaFromJson(e);
                r.by = by;
                if (r.sid.empty() || r.slot < 0)
                    break;
                if (!r.srcname.empty())
                    CoopPartnerNick = r.srcname;   // v93: Name fuer die Durchlauf-Kennung
                if (paused || JoinPending) {   // v76: Pause bzw. gleich Uebernahme - der eigene Stand gehoert nicht zu dieser Kennung
                    SaveSyncLog("%s hat gespeichert (%s) - %s, nicht mitgespeichert", by.c_str(), r.sid.c_str(),
                        paused ? "Pause" : "uebernehme gleich den Host-Stand");
                    break;
                }
                bool dup = false;   // v76 (S8): Warteschlange, jede Kennung einmal
                for (auto const& q : RemoteSaveQueue)
                    if (q.sid == r.sid)
                        dup = true;
                if (dup)
                    break;
                if (RemoteSaveQueue.empty())
                    RemoteSaveSince = CurrentMs;
                RemoteSaveQueue.push_back(r);
                SaveSyncLog("%s hat gespeichert (Slot %d, %s, %s) - speichere mit%s%s", by.c_str(), r.slot, r.kind.c_str(), r.sid.c_str(),
                    r.take ? ", danach Welt uebernehmen" : "",
                    RemoteSaveQueue.size() > 1 ? (", " + std::to_string(RemoteSaveQueue.size()) + ". in der Warteschlange").c_str() : "");
                break;
            }
            case SYNC_LOAD_REQ:
            {
                const bool targeted = e.contains("to");   // v71/v76: nur fuer mich (Hauptmenue, Verbinden, Fortsetzen)
                if (e.contains("nojoin")) {   // v76: Host uebernimmt nicht (Einstellung "eigenen Stand behalten")
                    JoinEnd("der Host gibt seinen Stand nicht vor (Einstellung: eigenen Stand behalten)");
                    break;
                }
                if (paused && !(targeted && JoinPending)) {
                    SaveSyncLog("%s laedt (%s) - Pause, nicht mitgeladen", by.c_str(), MetaFromJson(e).sid.c_str());
                    break;
                }
                RemoteLoad = MetaFromJson(e);
                RemoteLoad.by = by;
                CoopRunSet(RemoteLoad.run, "Ladeauftrag des Partners");   // v93: geladen wird der Durchlauf dieses Stands
                if (!RemoteLoad.srcname.empty() && RemoteLoad.srcpc != CoopMyPc())
                    CoopPartnerNick = RemoteLoad.srcname;
                if (RemoteLoad.src.empty())
                    RemoteLoad.src = CoopMe();   // v20: Partner mit v17/v18 (kein src, keine Dateiuebertragung) -> eigene Kopie laden
                if (!RemoteLoad.srcpc.empty()) {   // v76: Rolle, die dieser PC jetzt hat
                    const std::string role = CoopRoleOfPc(RemoteLoad.srcpc);
                    if (!role.empty())
                        RemoteLoad.src = role;
                }
                RemoteLoadHostWins = hostWins;
                if (hostWins) {
                    RemoteLoad.src = CoopMe();
                    RemoteLoad.srcpc = CoopMyPc();
                }
                RemoteLoadTargeted = targeted;
                RemoteLoadPending = true;
                RemoteSaveQueue.clear();
                if (AskLoadSlot >= 0) {   // v90: der Host laedt (auf meinen Wunsch oder selbst) - eigener Wunsch erledigt
                    SaveSyncLog("Ladewunsch %s: der Host l\xE4" "dt %s - lade nur einmal mit", AskLoadSid.c_str(), RemoteLoad.sid.c_str());
                    AskLoadSlot = -1;
                    AskLoadSelf = false;
                }
                SaveSyncLog("%s laedt Slot %d (%s, Welt von %s)%s%s", by.c_str(), RemoteLoad.slot,
                    RemoteLoad.sid.empty() ? "ohne Kennung" : RemoteLoad.sid.c_str(), RemoteLoad.src.c_str(),
                    hostWins ? " - Host-Kopie gilt, der Client uebernimmt sie danach" : "", targeted ? " - Auftrag nur fuer mich" : "");
                break;
            }
            case SYNC_SAVE_ACK:
            {
                const bool ok = e.contains("ok") && e["ok"].is_number() && e["ok"].get<int>() != 0;
                const std::string msg = e.contains("msg") && e["msg"].is_string() ? e["msg"].get<std::string>() : std::string();
                ChatLog(string::Combine(ok ? CoopTr("%s hat mitgespeichert.", "%s saved along.") : CoopTr("%s konnte NICHT mitspeichern: %s", "%s could NOT save along: %s"), string(by.c_str()), string(msg.c_str())),
                    ok ? zCOLOR(255, 255, 255, 255) : zCOLOR(255, 80, 80, 255));
                SaveSyncLog("%s: %s %s", by.c_str(), ok ? "mitgespeichert" : "NICHT mitgespeichert", msg.c_str());
                break;
            }
            }
        }
        catch (std::exception& ex) {
            CoopErrorLog("Speicher-Sync", ex.what(), CoopUpdateTypeName(type));
        }
    }

    // --- Ablauf je Frame -------------------------------------------------------------------------

    static void FinishPendingArchive() {
        if (PendingArchiveSlot < 0 || CurrentMs <= PendingArchiveMs)
            return;
        const int slot = PendingArchiveSlot;
        PendingArchiveSlot = -999;
        const std::string dir = CoopSlotDir(slot);
        CoopSaveMeta m;
        m.sid = PendingArchiveSid;
        m.kind = PendingArchiveKind;
        m.name = CoopSlotName(slot);
        m.by = PendingArchiveBy;
        m.bypc = PendingArchiveByPc;
        m.src = CoopMe();   // gespeichert wird immer die eigene Welt
        m.srcpc = CoopMyPc();   // v76
        m.srcname = MyNickname.ToChar();
        m.world = ogame->GetGameWorld() ? std::string(ogame->GetGameWorld()->GetWorldFilename().ToChar()) : std::string();
        m.time = CoopNowText("time");
        m.stime = CoopUnixNow();   // v76 (S9): Speicherzeit selbst merken - Dateidaten aendern sich beim Kopieren/Empfangen
        m.slot = slot;
        m.run = PendingArchiveRun;   // v93: Archiv im Ordner des Durchlaufs, reist mit SAVE_REQ zum Partner
        CoopWriteMeta(dir, m);
        CoopHeroWrite(dir, CoopHeroCapture());   // v19: eigener Held zu dieser Kennung
        CoopInfoWrite(dir, slot);                 // v76: Beschreibung fuer die Vorschau-Liste
        // v76 (S2): Archiv pruefen; schlaegt es fehl, gibt es keinen Ladeauftrag/keine Uebernahme auf diesen Stand
        const int n = CoopArchiveDir(dir, m.sid, NULL, &m.mh);
        SaveSyncLog("Slot %d gespeichert und archiviert: %s (%s, %d Dateien%s)", slot, m.sid.c_str(), m.kind.c_str(), n, n < 0 ? ", ARCHIV FEHLGESCHLAGEN" : ", geprueft");
        if (n < 0)
            ChatLog(CoopTr("Sicherung konnte nicht archiviert werden - siehe Coop-Log (Platte voll?).", "Backup could not be archived - see coop log (disk full?)."), zCOLOR(255, 80, 80, 255));
        if (m.kind == "sicherung")
            return;   // Sicherung vor einer Uebernahme: nur lokal
        if (PendingArchiveRemote) {
            if (m.kind == "pause" && PausePending)   // v76: Pausen-Sicherung des Hosts mitgespeichert - jetzt pausieren
                PauseActivate("nach gemeinsamer Sicherung");
            if (m.kind != "uebernommen")   // eigene Kopie nach einer Uebernahme (HandlePostTakeSave) braucht keine Antwort
                SendAck(m.sid, n > 0, n > 0 ? std::string("Slot ") + std::to_string(slot) : std::string("Archiv fehlgeschlagen"));
            if (PendingArchiveTake && !PendingArchiveTakeSrc.empty())
                StartTakeJob(m.sid, PendingArchiveTakeSrc, PendingArchiveTakeSrcPc, PendingArchiveTakeMh, slot, m.name, false, false);   // gerade gespeichert = gesichert
        }
        else {
            // v71: nur fuer Clients im Hauptmenue ausgeloest - die Spielenden speichern mit, laden aber nicht neu
            // v76: ebenso fuer Clients, die verbinden oder fortsetzen (TakePeers)
            m.take = m.kind == "abgleich" && !CoopSyncSaveMenuOnly && n > 0;
            SendMetaReq(SYNC_SAVE_REQ, m);
            if (m.kind == "abgleich" && ServerThread && !TakePeers.empty()) {
                if (n > 0) {
                    CoopSaveMeta l = m;
                    l.take = false;   // stime: Speicherzeit (fuer den zeitlich naechsten eigenen Helden beim Client im Hauptmenue)
                    for (const std::string& p : TakePeers) {
                        SendMetaReq(SYNC_LOAD_REQ, l, p);
                        SaveSyncLog("%s: Ladeauftrag fuer %s (Slot %d) geschickt", p.c_str(), m.sid.c_str(), slot);
                    }
                    TakePeers.clear();
                }
                else {   // Archiv fehlgeschlagen - in einer Minute noch einmal, die Clients warten weiter
                    CoopHostSyncPending = true;
                    CoopHostSyncMenuOnly = true;
                    CoopHostSyncRetryMs = CurrentMs + 60000;
                    SaveSyncLog("Ladeauftrag fuer %u Clients verschoben - Archiv fehlgeschlagen", (unsigned)TakePeers.size());
                }
            }
            CoopSyncSaveMenuOnly = false;
            if (m.kind == "auto" || m.kind == "abgleich")
                ChatLog(string::Combine(CoopTr("Gemeinsame Sicherung (Slot %i) erstellt.", "Shared backup created (slot %i)."), slot));
            else if (m.kind == "pause")
                ChatLog(string::Combine(CoopTr("%s pausiert - gemeinsame Sicherung (Slot %i) erstellt.", "%s paused - shared backup created (slot %i)."), string(HostPauseSaveFor.c_str()), slot));
            else if (m.kind == "abbruch")
                ChatLog(string::Combine(CoopTr("Verbindung zu %s weg - Sicherung (Slot %i) erstellt.", "Connection to %s lost - backup created (slot %i)."), string(HostAbortSaveFor.c_str()), slot));
        }
    }

    static void HandleRemoteLoad() {
        RemoteLoadPending = false;
        const CoopSaveMeta r = RemoteLoad;
        const bool targeted = RemoteLoadTargeted;
        RemoteLoadTargeted = false;
        if (r.sid.empty()) {
            ChatLog(string::Combine(CoopTr("%s l" "\xE4" "dt einen Stand ohne Coop-Kennung (Slot %i) - bitte denselben Stand laden!", "%s loads a save without coop id (slot %i) - please load the same save!"), string(r.by.c_str()), r.slot), zCOLOR(255, 80, 80, 255));
            CoopTakeError = CoopTr("Der Host l\xE4" "dt einen Stand ohne Coop-Kennung (Slot ", "The host loads a save without coop id (slot ") + std::to_string(r.slot) + ").";
            CoopTakeErrorMs = CurrentMs;
            return;
        }
        if (!CoopMetaMine(r)) {   // v76: nach PC-Kennung
            // v19: Welt des Ladenden uebernehmen, eigenen Helden behalten (v40: im Hauptmenue gibt es nichts zu sichern)
            StartTakeJob(r.sid, r.src, r.srcpc, r.mh, r.slot, r.name, player != NULL, targeted);
            return;
        }
        // Der Partner laedt einen Stand mit MEINER Welt (z. B. meine Sicherung): eigenen Stand laden.
        int slot = -1;
        CoopSaveMeta here;
        if (r.slot >= 1 && CoopReadMeta(CoopSlotDir(r.slot), here) && here.sid == r.sid && CoopMetaMine(here))
            slot = r.slot;
        else {
            const std::string own = CoopOwnCopyDir(r.sid);
            if (own.empty()) {
                ChatLog(string::Combine(CoopTr("Passender Stand %s fehlt hier - nicht geladen!", "Matching save %s is missing here - not loaded!"), string(r.sid.c_str())), zCOLOR(255, 80, 80, 255));
                SaveSyncLog("Laden %s: eigene Welt zu dieser Kennung fehlt", r.sid.c_str());
                CoopTakeError = CoopTr("Der passende Stand ", "The matching save ") + r.sid + CoopTr(" fehlt hier - nicht geladen.", " is missing here - not loaded.");
                CoopTakeErrorMs = CurrentMs;
                if (player)
                    CoopDesyncWarn = true;
                return;
            }
            slot = (r.slot >= 1 && r.slot <= 15) ? r.slot : CoopNextReservedSlot();
            std::string why;
            const int n = CoopSlotReplace(slot, own, nullptr, why);   // v76 (S4)
            if (n <= 0) {
                ChatLog(string::Combine(CoopTr("Stand %s lie" "\xDF" " sich nicht in Slot %i kopieren - nicht geladen!", "Save %s could not be copied to slot %i - not loaded!"), string(r.sid.c_str()), slot), zCOLOR(255, 80, 80, 255));
                SaveSyncLog("Laden %s: eigene Welt nicht in Slot %d (%s)", r.sid.c_str(), slot, why.c_str());
                CoopTakeError = CoopTr("Der Stand ", "The save ") + r.sid + CoopTr(" lie\xDF sich nicht in Slot ", " could not be copied to slot ") + std::to_string(slot) + CoopTr(" kopieren.", ".");
                CoopTakeErrorMs = CurrentMs;
                if (player)
                    CoopDesyncWarn = true;
                return;
            }
            SaveSyncLog("Laden %s: eigene Welt in Slot %d kopiert (%d Dateien, geprueft)", r.sid.c_str(), slot, n);
        }
        if (RemoteLoadHostWins) {   // v29: Clients sollen die Host-Kopie uebernehmen (eigener Held bleibt)
            RemoteLoadHostWins = false;
            CoopSaveMeta m = r;
            m.src = CoopMe();
            m.srcpc = CoopMyPc();
            m.by = CoopMe();
            m.slot = slot;
            const std::string own = CoopOwnCopyDir(r.sid);
            CoopManifest man;
            m.mh = (!own.empty() && CoopManifestEnsure(own, man)) ? CoopManifestHash(man) : std::string();
            SendMetaReq(SYNC_LOAD_REQ, m);
            SaveSyncLog("Laden %s: Host-Kopie geladen, Partner uebernimmt sie", r.sid.c_str());
        }
        CoopActiveRemoteSid = r.sid;
        CoopDoLoad(slot);
        if (!CoopMenuLoadDeferred)   // v40: siehe TakeJobLoad
            CoopActiveRemoteSid.clear();
    }

    // v76 (S8): Warteschlange - je Bild hoechstens ein Mitspeichern, erst nach dem Archiv des vorigen
    static void HandleRemoteSave() {
        if (RemoteSaveQueue.empty() || PendingArchiveSlot >= 0)
            return;
        if (IsCoopPaused && !ServerThread) {   // in der Pause speichert der Client nicht mit
            RemoteSaveQueue.clear();
            return;
        }
        const CoopSaveMeta& next = RemoteSaveQueue.front();
        std::string why;
        if (!CoopCanSaveNow(why)) {
            // v39: nicht mehr nach 60 s aufgeben ("Staende weichen ab") - jede Minute melden und weiter versuchen, bis es klappt
            if (CurrentMs > RemoteSaveSince + 60000) {
                RemoteSaveSince = CurrentMs;
                ChatLog(string::Combine(CoopTr("Mitspeichern wartet (%s) - neuer Versuch, sobald es geht.", "Saving along waits (%s) - retrying when possible."), string(why.c_str())), zCOLOR(255, 200, 80, 255));
                SaveSyncLog("Mitspeichern %s wartet: %s", next.sid.c_str(), why.c_str());
            }
            return;
        }
        RemoteSave = next;
        RemoteSaveQueue.pop_front();
        RemoteSaveSince = CurrentMs;
        CoopBackupForeignSlot(RemoteSave.slot);
        CoopActiveRemoteSid = RemoteSave.sid;
        CoopActiveRemoteKind = RemoteSave.kind;
        CoopActiveRemoteBy = RemoteSave.by;
        CoopActiveRemoteByPc = RemoteSave.bypc;
        CoopActiveRemoteRun = RemoteSave.run;   // v93: Mitspeichern gehoert zum Durchlauf des Ausloesers
        if (!RemoteSave.run.empty())
            CoopRunSet(RemoteSave.run, "Mitspeichern");
        CoopDoSave(RemoteSave.slot, RemoteSave.name.empty() ? "Coop " + RemoteSave.time : RemoteSave.name);
        CoopActiveRemoteSid.clear();
    }

    // --- v76: Pause, Fortsetzen, Stoerung, Abbruch (Variante b) ------------------------------------------------------------

    // Client: Pause beginnt - nach der gemeinsamen Sicherung (oder ohne, wenn der Host nicht rechtzeitig antwortet). Der Held von
    // jetzt gilt beim Fortsetzen (er uebernimmt dann die aktuelle Host-Welt), was in der Pause geschieht, bleibt beim Client.
    static void PauseActivate(const char* how) {
        PausePending = false;
        if (IsCoopPaused)
            return;
        IsCoopPaused = true;
        rtnMan->RestartRoutines();   // wie beim Umschalten in Plugin.cpp
        RemoteSaveQueue.clear();
        if (player) {
            DivergedHero = CoopHeroCapture();
            DivergedWhy = "Pausenbeginn";
        }
        ChatLog(CoopTr("Weltabgleich angehalten.", "Stop world synchronization."));
        ChatLog(string::Combine(CoopTr("Pause (%s): Speichern und Laden gelten jetzt nur f" "\xFC" "r dich. Shift+F2 setzt fort.", "Pause (%s): saving and loading now only affect you. Shift+F2 resumes."), string(how)));
        SaveSyncLog("Pause beginnt (%s) - eigener Held gemerkt (Stufe %d, %u Gegenstaende)", how, DivergedHero.level, (unsigned)DivergedHero.items.size());
    }

    // Client: Host-Stand uebernehmen (why = "connect" oder "resume") - der Host sichert und schickt einen Ladeauftrag nur an mich
    static void JoinStart(const char* why, bool retry) {
        JoinPending = true;
        JoinWhy = why;
        JoinSince = CurrentMs;
        if (!retry)
            JoinTries = 0;
        json j;
        j["why"] = why;
        SaveSyncSend(j, SYNC_STATE_REQ);
        SaveSyncLog("%s - fordere den Host-Stand an%s%s", !strcmp(why, "resume") ? "Fortsetzen" : "verbunden",
            DivergedHero.valid ? (", eigener Held vom " + DivergedWhy).c_str() : ", eigener Held = aktueller Stand",
            retry ? (" (Versuch " + std::to_string(JoinTries + 1) + ")").c_str() : "");
    }

    // Client: keine Uebernahme (Host gibt nicht vor) - Pause wie bisher beenden, gemerkten Helden verwerfen
    static void JoinEnd(const char* why) {
        const bool resume = JoinPending && JoinWhy == "resume";
        JoinPending = false;
        DivergedHero.valid = false;
        SaveSyncLog("keine Uebernahme: %s", why);
        if (resume && IsCoopPaused) {
            IsCoopPaused = false;
            rtnMan->RestartRoutines();
            ChatLog(CoopTr("Weltabgleich l\xE4" "uft wieder.", "Restoring world synchronization."));
            CoopResyncRequest("Fortsetzen ohne Uebernahme");
            CoopWorldSyncRequestAgain();
        }
    }

    static void HandlePausePending() {
        if (PausePending && CurrentMs > PauseSince + 20000)
            PauseActivate("ohne gemeinsame Sicherung - der Host hat nicht rechtzeitig gespeichert");
    }

    // Client: Uebernahme beim Verbinden/Fortsetzen gescheitert oder zu lange ohne Ladeauftrag - bis zu 3 neue Versuche (S5)
    static void HandleJoin() {
        if (!JoinPending || TakeJob.active || RemoteLoadPending || IsLoadingLevel)
            return;
        const bool failed = CoopTakeErrorMs > JoinSince;
        const bool timeout = CurrentMs > JoinSince + 150000;
        if (!failed && !timeout)
            return;
        if (JoinTries < 3) {
            JoinTries++;
            ChatLog(string::Combine(CoopTr("\xDC" "bernahme des Host-Stands fehlgeschlagen (%s) - neuer Versuch %i von 3.", "Taking over the host's state failed (%s) - retry %i of 3."),
                string(failed ? CoopTakeError.c_str() : CoopTr("keine Antwort", "no answer")), JoinTries), zCOLOR(255, 200, 80, 255));
            JoinStart(JoinWhy.c_str(), true);
            return;
        }
        JoinPending = false;
        if (JoinWhy == "resume")
            ChatLog(CoopTr("Fortsetzen fehlgeschlagen - die Synchronisation bleibt angehalten. Shift+F2 versucht es erneut.", "Resume failed - synchronization stays paused. Shift+F2 tries again."), zCOLOR(255, 80, 80, 255));
        else {
            CoopDesyncWarn = true;
            ChatLog(CoopTr("Stand des Hosts NICHT " "\xFC" "bernommen - die St" "\xE4" "nde weichen ab! F6 gleicht ab.", "Host's state NOT taken over - states differ! F6 syncs."), zCOLOR(255, 80, 80, 255));
        }
        SaveSyncLog("Uebernahme nach %s endgueltig fehlgeschlagen", JoinWhy == "resume" ? "dem Fortsetzen" : "dem Verbinden");
    }

    // Client: Verbindungsstoerung - 5 s nichts vom Host (auch keine ENet-Pings, die laufen alle 0,5 s auch waehrend Ladebildschirmen)
    static void HandleDisturbance() {
        if (!ClientThread || ServerThread || CoopClientState != 2) {
            Disturbed = false;
            return;
        }
        const LONG silence = CoopNetSilenceMs;
        if (!Disturbed && silence > 5000) {
            Disturbed = true;
            DisturbSince = CurrentMs;
            DisturbHero = player && !IsCoopPaused ? CoopHeroCapture() : CoopHero();
            ChatLog(CoopTr("Verbindung gest" "\xF6" "rt (keine Antwort vom Host) - Speichern und Laden gelten jetzt nur f" "\xFC" "r dich.", "Connection disturbed (no answer from host) - saving and loading now only affect you."), zCOLOR(255, 200, 80, 255));
            SaveSyncLog("Verbindung gestoert: %ld ms ohne Antwort vom Host - Speichern/Laden nur lokal, eigener Held gemerkt", (long)silence);
        }
        else if (Disturbed && silence < 1000) {
            Disturbed = false;
            DisturbHero.valid = false;
            ChatLog(CoopTr("Verbindung wieder da - voller Abgleich.", "Connection back - full sync."));
            SaveSyncLog("Verbindung wieder da nach %lld s", (CurrentMs - DisturbSince) / 1000);
            if (!IsCoopPaused) {
                CoopResyncRequest("Verbindungsstoerung vorbei");
                CoopWorldSyncRequestAgain();
            }
        }
    }

    // Client: nach einer gezielten Uebernahme die eigene Kopie dieser Kennung speichern (Host-Welt + eigener Held) - dann gibt es
    // zu jeder Kennung, die der Client uebernommen hat, auch seinen Helden (Liste im Hauptmenue, spaeteres Laden durch den Host)
    static void HandlePostTakeSave() {
        if (PostTakeSid.empty() || IsCoopPaused || PendingHeroApply || PendingArchiveSlot >= 0 || CurrentMs < PostTakeAt)
            return;
        std::string why;
        if (!CoopCanSaveNow(why)) {
            if (CurrentMs > PostTakeAt + 300000) {
                SaveSyncLog("eigene Kopie von %s nicht angelegt (%s)", PostTakeSid.c_str(), why.c_str());
                PostTakeSid.clear();
            }
            return;
        }
        const std::string sid = PostTakeSid;
        PostTakeSid.clear();
        if (!CoopArchiveFind(sid).empty())
            return;   // gibt es schon (v93: auch im Durchlauf-Ordner)
        CoopActiveRemoteSid = sid;
        CoopActiveRemoteKind = "uebernommen";
        CoopActiveRemoteBy = PostTakeBy;
        CoopActiveRemoteByPc = PostTakeByPc;
        CoopActiveRemoteRun = CoopCurrentRun;   // v93: Durchlauf des uebernommenen Stands (aus dem Ladeauftrag)
        CoopDoSave(PostTakeSlot, PostTakeName);
        CoopActiveRemoteSid.clear();
        SaveSyncLog("nach der Uebernahme gespeichert: eigene Kopie von %s in Slot %d (Host-Welt, eigener Held)", sid.c_str(), PostTakeSlot);
    }

    // Host: Sicherung fuer die Pause eines Clients bzw. nach einem Verbindungsabbruch - sofort, ohne Vorab-Frage
    static void HandleHostQuickSave(bool& pending, long long until, const char* kind, const std::string& who) {
        if (!ServerThread || !pending || PendingArchiveSlot >= 0)
            return;
        std::string why;
        if (!CoopCanSaveNow(why)) {
            if (CurrentMs > until) {
                pending = false;
                SaveSyncLog("Sicherung (%s, %s) nicht moeglich: %s", kind, who.c_str(), why.c_str());
            }
            return;
        }
        pending = false;
        const int slot = CoopNextReservedSlot();
        CoopBackupForeignSlot(slot);
        CoopSavingKind = kind;
        CoopDoSave(slot, std::string(!strcmp(kind, "pause") ? "Coop Pause " : "Coop Abbruch ") + CoopNowText("time"));
        CoopSavingKind.clear();
    }

    // Im Bild, solange eine Uebernahme gescheitert ist (S5)
    static void DrawDesyncWarn() {
        if (!CoopDesyncWarn || !player || !screen)
            return;
        auto font = screen->GetFontName();
        auto color = screen->fontColor;
        screen->SetFont(zSTRING("Font_Old_10_White_Hi.TGA"));
        screen->SetFontColor(GFX_RED);
        screen->Print(50, 250, zSTRING(CoopTr("Coop: St" "\xE4" "nde weichen ab - F6 gleicht mit dem Host ab", "Coop: states differ - F6 syncs with the host")));
        screen->SetFont(font);
        screen->SetColor(color);
    }

    // v71: wartende Clients im Hauptmenue erfahren, warum es dauert (Anzeige im COOP-Menue)
    static void CoopNotifyMenuPeersWait(const std::string& why) {
        if (TakePeers.empty())
            return;
        json j;
        j["type"] = SYNC_LOBBY;
        j["id"] = MyselfId.ToChar();
        j["op"] = "wait";
        j["why"] = why;
        CoopTraceCount(TRACE_SENT, SYNC_LOBBY);
        ReadyToSendJsons.enqueue(j);
    }

    // Host: gemeinsame Sicherung, nach der die Clients die Host-Welt uebernehmen
    static void HandleHostSync() {
        if (!ServerThread || !CoopHostSyncPending) {
            if (CoopCheckKind == 2)
                CoopCheckKind = 0;
            return;
        }
        if (PendingArchiveSlot >= 0)
            return;
        std::string why;
        // v76: nur fuer bestimmte Clients (Hauptmenue, Verbinden, Fortsetzen) - ohne Vorab-Frage, die anderen speichern mit, wenn
        // sie koennen; ist der Host beschaeftigt, neuer Versuch nach 5 s (der Wartende soll nicht minutenlang stehen)
        if (CoopHostSyncMenuOnly && CoopCheckKind != 2) {
            if (CurrentMs < CoopHostSyncRetryMs)
                return;
            if (!CoopCanSaveNow(why)) {
                CoopHostSyncRetryMs = CurrentMs + 5000;
                static CoopDiagBudget budget = { 0, 0 };
                if (CoopDiagAllow(budget, 6))
                    SaveSyncLog("Sicherung fuer %u wartende Clients verschoben: Host beschaeftigt (%s) - neuer Versuch in 5 s", (unsigned)TakePeers.size(), why.c_str());
                CoopNotifyMenuPeersWait("Host: " + why);
                return;
            }
            CoopHostSyncPending = false;
            CoopSyncSaveMenuOnly = true;
            CoopHostSyncMenuOnly = false;
            const int slot = CoopNextReservedSlot();
            CoopBackupForeignSlot(slot);
            CoopSavingKind = "abgleich";
            CoopDoSave(slot, "Coop Abgleich " + CoopNowText("time"));
            CoopSavingKind.clear();
            return;
        }
        if (CoopCheckKind == 2) {   // v39: Antworten der Clients abwarten
            const int r = CoopCheckResult(why);
            if (r == 0)
                return;
            CoopCheckKind = 0;
            if (r < 0 || !CoopCanSaveNow(why)) {   // jemand beschaeftigt (oder der Host inzwischen selbst)
                CoopHostSyncRetryMs = CurrentMs + 60000;
                ChatLog(string::Combine(CoopTr("Abgleich verschoben (%s) - neuer Versuch in 1 Minute.", "Sync postponed (%s) - retrying in 1 minute."), string(why.c_str())), zCOLOR(255, 200, 80, 255));
                SaveSyncLog("Abgleich verschoben: %s - neuer Versuch in 1 Minute", why.c_str());
                CoopNotifyMenuPeersWait(why);
                return;
            }
        }
        else {
            if (CurrentMs < CoopHostSyncRetryMs)
                return;
            if (!CoopCanSaveNow(why)) {   // v39: nicht mehr nach 60 s aufgeben - jede Minute neu
                CoopHostSyncRetryMs = CurrentMs + 60000;
                ChatLog(string::Combine(CoopTr("Abgleich verschoben: Host besch" "\xE4" "ftigt (%s) - neuer Versuch in 1 Minute.", "Sync postponed: host busy (%s) - retrying in 1 minute."), string(why.c_str())), zCOLOR(255, 200, 80, 255));
                SaveSyncLog("Abgleich verschoben: Host beschaeftigt (%s)", why.c_str());
                CoopNotifyMenuPeersWait("Host: " + why);
                return;
            }
            if (CoopPeerCount > 0) {
                CoopCheckStart(2);
                return;
            }
        }
        CoopHostSyncPending = false;
        CoopSyncSaveMenuOnly = CoopHostSyncMenuOnly;   // v71
        CoopHostSyncMenuOnly = false;
        const int slot = CoopNextReservedSlot();
        CoopBackupForeignSlot(slot);
        CoopSavingKind = "abgleich";
        CoopDoSave(slot, "Coop Abgleich " + CoopNowText("time"));
        CoopSavingKind.clear();
    }

    // v71 (Test v69: "der Client konnte nicht joinen, weil er den Save nicht hatte - ich dachte, wir geben ihm ja sowieso den
    // Save"): ein Client meldet sich aus dem Hauptmenue, waehrend der Host spielt. Bis v70 hiess es nur "Host spielt schon" -
    // er musste trennen, irgendeinen Stand laden und im Spiel neu verbinden. Jetzt legt der Host eine gemeinsame Sicherung an
    // und schickt ihm danach den Ladeauftrag (FinishPendingArchive); der Client holt die Dateien und laedt aus dem Menue mit
    // seinem zuletzt gespeicherten eigenen Helden (wie beim gemeinsamen Laden aus dem Menue, v55).
    void CoopSaveSyncMenuPeerWantsState(const std::string& id) {
        if (!ServerThread || !player || !SaveSyncAktiv())
            return;
        TakePeers.insert(id);
        if (!CoopHostSyncPending) {
            CoopHostSyncPending = true;
            CoopHostSyncSince = CurrentMs;
            CoopHostSyncMenuOnly = true;
        }
        CoopHostSyncRetryMs = 0;   // gleich versuchen (Rueckzug nur, wenn jemand beschaeftigt ist)
        SaveSyncLog("%s meldet sich aus dem Hauptmenue - gemeinsame Sicherung, danach laedt er sie", id.c_str());
        ChatLog(string::Combine(CoopTr("%s verbindet sich aus dem Hauptmen" "\xFC" " - gemeinsame Sicherung, danach l" "\xE4" "dt er den Stand.", "%s connects from the main menu - shared backup, then they load it."), string(id.c_str())));
    }

    // v71: Client-Verbindung beendet (Plugin.cpp, CoopClientEnded) - offene Auftraege dieser Verbindung verwerfen
    // v76: alle Merker leeren (S10); bei Verbindungsabbruch den eigenen Helden merken - vom Beginn der Pause bzw. Stoerung, sonst
    // von jetzt. Beim Wiederverbinden uebernimmt der Client die aktuelle Host-Welt mit diesem Helden (Variante b).
    // v76: Client hat die Verbindung verloren (Plugin.cpp, CoopNetPulse - Zustand 4, nicht beim gewollten Trennen)
    void CoopSaveSyncConnectionLost() {
        if (!player || MyselfId == "_player_" || !CoopRuleSaves() || !CoopRuleTakeOnConnect())
            return;
        if (!(IsCoopPaused && DivergedHero.valid)) {
            DivergedHero = (Disturbed && DisturbHero.valid) ? DisturbHero : CoopHeroCapture();
            DivergedWhy = Disturbed ? "Beginn der Verbindungsstoerung" : "Verbindungsabbruch";
        }
        SaveSyncLog("Verbindung verloren - eigener Held vom %s gemerkt (Stufe %d) - beim Wiederverbinden gilt er", DivergedWhy.c_str(), DivergedHero.level);
    }

    void CoopSaveSyncConnectionEnded() {
        if (TakeJob.active)
            SaveSyncLog("Verbindung beendet - Uebernahme %s abgebrochen", TakeJob.sid.c_str());
        TakeJob.active = false;
        RemoteLoadPending = false;
        RemoteLoadTargeted = false;
        RemoteSaveQueue.clear();
        FilesIn.clear();
        CoopActiveRemoteSid.clear();
        CoopActiveRemoteKind.clear();
        if (PendingHeroApply && CoopLoadEndMs < PendingHeroSetMs)   // Ladevorgang kam nicht mehr zustande
            PendingHeroApply = false;
        PausePending = false;
        JoinPending = false;
        Disturbed = false;
        DisturbHero.valid = false;
        PostTakeSidNext.clear();
        CoopPcRole.clear();
        // v76: beim naechsten Verbinden wieder den Host-Stand uebernehmen - bis v75 blieb der Merker nach dem ersten Verbinden
        // gesetzt, ein Wiederverbinden im selben Spiel (Shift+F2) holte den Host-Stand nicht mehr
        CoopConnectTakeSent = false;
        CoopFirstActiveMs = 0;
    }

    // v91: gemeinsames neues Spiel - beide beginnen an Othellos Startpunkt; der Mitspieler tritt im ersten Spielbild an eine legale
    // Stelle daneben (wie v89 beim Laden ohne eigene Lage). Die Bewegung setzt HandleMoveReset im naechsten Bild zurueck.
    void CoopSaveSyncPlaceBesideStart() {
        if (!player)
            return;
        const zVEC3 at = player->GetPositionWorld();
        const zVEC3 dir = player->GetAtVectorWorld();
        std::string how;
        PendingMoveReset = true;
        CoopPlaceBesideHost(at, dir, how);
        SaveSyncLog("Neues Spiel gemeinsam: Startpunkt wie beim Host - %s", how.c_str());
    }

    // v76: Neues Spiel - nichts Offenes aus dem Coop gilt dafuer (Plugin: Game_LoadBegin_NewGame)
    void CoopSaveSyncOnNewGame() {
        CoopActiveRemoteSid.clear();
        CoopActiveRemoteKind.clear();
        PendingHeroApply = false;
        PendingHeroLocalOk = false;
        TakeJob.active = false;
        RemoteLoadPending = false;
        RemoteSaveQueue.clear();
        PostTakeSid.clear();
        PostTakeSidNext.clear();
        DivergedHero.valid = false;
        JoinPending = false;
        CoopRunSet(std::string(), "neues Spiel");   // v93: beim gemeinsamen Start setzt CoopNewGame.cpp den neuen Durchlauf
        SaveSyncLog("Neues Spiel - offene Auftraege und gemerkter Held verworfen");
    }

    // v76: Host gestartet (Plugin.cpp) - der gemerkte Held eines Clients gilt nicht mehr
    void CoopSaveSyncRoleStart(bool host) {
        if (!host)
            return;
        DivergedHero.valid = false;
        JoinPending = false;
        PausePending = false;
        Disturbed = false;
    }

    static void HandleAutoSave() {
        const int autoMin = CoopRuleAutoSaveMinutes();   // v75: aus dem Menue (Abgleich -> Auto-Sicherung)
        if (!ServerThread || CoopPeerCount <= 0 || autoMin <= 0 || !RemoteSaveQueue.empty() || CoopHostSyncPending ||
            HostPauseSavePending || HostAbortSavePending || PendingArchiveSlot >= 0) {
            if (CoopCheckKind == 1)
                CoopCheckKind = 0;
            return;
        }
        std::string why;
        if (CoopCheckKind == 1) {   // v39: Antworten der Clients abwarten
            const int r = CoopCheckResult(why);
            if (r == 0)
                return;
            CoopCheckKind = 0;
            if (r < 0 || !CoopCanSaveNow(why)) {
                CoopNextAutoTryMs = CurrentMs + 60000;
                ChatLog(string::Combine(CoopTr("Gemeinsame Sicherung verschoben (%s) - neuer Versuch in 1 Minute.", "Shared backup postponed (%s) - retrying in 1 minute."), string(why.c_str())), zCOLOR(255, 200, 80, 255));
                SaveSyncLog("Auto-Sicherung verschoben: %s - neuer Versuch in 1 Minute", why.c_str());
                return;
            }
        }
        else {
            if (CurrentMs < CoopLastSaveMs + (long long)autoMin * 60000 || CurrentMs < CoopNextAutoTryMs)
                return;
            if (!CoopCanSaveNow(why)) {   // v39: eine Minute statt 2 s
                CoopNextAutoTryMs = CurrentMs + 60000;
                SaveSyncLog("Auto-Sicherung verschoben: Host beschaeftigt (%s) - neuer Versuch in 1 Minute", why.c_str());
                return;
            }
            CoopCheckStart(1);   // erst alle Clients fragen
            return;
        }
        const int slot = CoopNextReservedSlot();
        CoopBackupForeignSlot(slot);
        CoopSavingForAuto = true;
        CoopDoSave(slot, "Coop " + CoopNowText("time"));
        CoopSavingForAuto = false;
    }

    // v25: nach dem Einsetzen des Helden die Bewegung zuruecksetzen - Original oCNpc::ResetPos an der aktuellen
    // Stelle (OthelloCompat.cpp, CoopResetPosSafe), Blickrichtung danach wieder setzen (ResetPos dreht zurueck).
    static void HandleMoveReset() {
        if (!PendingMoveReset || !player || IsLoadingLevel || PendingHeroApply)
            return;
        PendingMoveReset = false;
        const zVEC3 at = player->GetAtVectorWorld();
        CoopResetPosSafe(player, player->GetPositionWorld());
        player->SetHeadingAtWorld(at);
        SaveSyncLog("Bewegung des eigenen Helden zurueckgesetzt (ResetPos an der eigenen Stelle)");
    }

    static void HandleHeroApply() {
        if (!PendingHeroApply || !player || IsLoadingLevel)
            return;
        if (CoopLoadEndMs < PendingHeroSetMs) {
            if (GetCurrentMs() > PendingHeroSetMs + 30000) {   // es wurde nie geladen
                PendingHeroApply = false;
                SaveSyncLog("Held nicht wiederhergestellt: kein Ladevorgang nach der Uebernahme");
            }
            return;
        }
        if (GetCurrentMs() < CoopLoadEndMs + 500)
            return;
        PendingHeroApply = false;
        CoopHeroApply(PendingHero);
        ChatLog(string::Combine(CoopTr("Stand von %s " "\xFC" "bernommen - eigener Held wiederhergestellt.", "Took over %s's state - own hero restored."), string(PendingHeroSrc.c_str())));
    }

    static void HandleSyncTriggers() {
        // Beim Verbinden: Client uebernimmt einmal die Host-Welt
        if (!CoopFirstActiveMs)
            CoopFirstActiveMs = CurrentMs;
        // v21: direkt nach dem Verbinden und mit Ansage - im Test v19 lief der Abgleich 5 s spaeter,
        // genau nach einem eigenen Speichern, und sah wie "laedt direkt nach dem Speichern" aus.
        // v75: ob uebernommen wird, gibt der Host vor (Abgleich -> Beim Verbinden; er prueft die Anfrage auch selbst)
        // v76: der Client speichert dabei nicht mehr mit (sein Stand von vor dem Verbinden gehoert nicht zur gemeinsamen Kennung) -
        // er uebernimmt die Host-Sicherung ueber einen Ladeauftrag nur fuer ihn und speichert danach seine eigene Kopie
        if (ClientThread && !ServerThread && CoopRuleTakeOnConnect() && !CoopConnectTakeSent && CurrentMs > CoopFirstActiveMs + 1500) {
            CoopConnectTakeSent = true;
            JoinStart("connect", false);
            ChatLog(DivergedHero.valid ? CoopTr("Verbunden - der Stand des Hosts wird " "\xFC" "bernommen, eigener Held vom Verbindungsabbruch.", "Connected - taking over the host's state, own hero from the disconnect.")
                                       : CoopTr("Verbunden - der Stand des Hosts wird " "\xFC" "bernommen (eigener Held bleibt).", "Connected - taking over the host's state (own hero kept)."));
        }
        // Taste: Client fordert an, Host loest fuer alle aus
        if (CoopSyncKey && zinput->KeyToggled(CoopSyncKey) && !IsPlayerTalkingWithAnybody() && !BrowserOpen) {
            if (!ServerThread && (IsCoopPaused || JoinPending || PausePending)) {   // v76
                ChatLog(IsCoopPaused ? CoopTr("Erst die Pause beenden (Shift+F2).", "End the pause first (Shift+F2).") : CoopTr("Abgleich l" "\xE4" "uft schon - bitte warten.", "Sync already running - please wait."), zCOLOR(255, 200, 80, 255));
            }
            else if (ServerThread) {
                CoopHostSyncPending = true;
                CoopHostSyncSince = CurrentMs;
                CoopHostSyncMenuOnly = false;   // v71
                ChatLog(CoopTr("Abgleich: gemeinsame Sicherung, danach " "\xFC" "bernehmen alle die Host-Welt.", "Sync: shared backup, then everyone takes over the host's world."));
            }
            else {
                json j;
                SaveSyncSend(j, SYNC_STATE_REQ);
                ChatLog(CoopTr("Abgleich beim Host angefordert.", "Sync requested from the host."));
            }
        }
    }

    static void BrowserScan() {
        BrowserList.clear();
        const std::string root = CoopArchiveRoot();
        CoopArchiveScan();   // v93: Hauptordner und Durchlauf-Ordner (die Ordner selbst sind keine Staende)
        for (auto const& a : CoopArchiveIndex) {
            if (a.first.size() > 5 && a.first.substr(a.first.size() - 5) == ".PART")
                continue;   // unvollstaendige Uebertragung
            const std::string rel = a.second.substr(root.size(), a.second.size() - root.size() - 1);   // [Durchlauf\]Ordner
            const std::string folder = rel.substr(rel.find_last_of('\\') == std::string::npos ? 0 : rel.find_last_of('\\') + 1);
            CoopSaveMeta m;
            if (!CoopReadMeta(a.second, m))
                m.sid = folder;
            m.name = rel;   // Ordner relativ zum Archiv (eindeutig, zeigt "__von_..." und den Durchlauf)
            BrowserList.push_back(m);
        }
        auto key = [](const CoopSaveMeta& m) {
            const size_t s = m.name.find_last_of('\\');
            const std::string f = s == std::string::npos ? m.name : m.name.substr(s + 1);
            const size_t p = f.find("20");
            return p == std::string::npos ? f : f.substr(p);
        };
        std::sort(BrowserList.begin(), BrowserList.end(), [&](const CoopSaveMeta& a, const CoopSaveMeta& b) { return key(a) > key(b); });
        BrowserSel = 0;
        BrowserConfirmMs = 0;
    }

    static void BrowserLoadSelected() {
        if (BrowserSel < 0 || BrowserSel >= (int)BrowserList.size())
            return;
        const CoopSaveMeta m = BrowserList[BrowserSel];
        const int slot = CoopNextReservedSlot();
        std::string why;
        const int n = CoopSlotReplace(slot, CoopArchiveRoot() + m.name + "\\", nullptr, why);   // v76 (S4): geprueft, nichts Altes bleibt
        SaveSyncLog("Liste: lade %s ueber Slot %d (%d Dateien%s%s)", m.name.c_str(), slot, n, n <= 0 ? ", FEHLER: " : ", geprueft", n <= 0 ? why.c_str() : "");
        BrowserOpen = false;
        if (n <= 0) {
            ChatLog(string::Combine(CoopTr("Sicherung konnte nicht kopiert werden (%s).", "Backup could not be copied (%s)."), string(why.c_str())), zCOLOR(255, 80, 80, 255));
            return;
        }
        // Fremde Welt mit eigenem Helden? Dann nach dem Laden den Helden dieses Standes wiederherstellen.
        CoopSaveMeta meta;
        if (CoopReadMeta(CoopSlotDir(slot), meta) && !meta.src.empty() && !CoopMetaMine(meta)) {
            CoopHero h;
            const std::string own = CoopOwnCopyDir(meta.sid);
            if ((!own.empty() && CoopHeroRead(own, h)) || CoopHeroRead(CoopSlotDir(slot), h)) {
                h.exact = true;   // v89: eigene Kopie bzw. Held dieses Stands (fremde Lagen speichert die Uebernahme ab v89 nicht mehr mit)
                PendingHeroApply = true;
                PendingHeroLocalOk = true;
                PendingHeroSetMs = GetCurrentMs();
                PendingHero = h;
                PendingHeroSrc = meta.src;
            }
        }
        CoopLoadingFromList = true;   // v29: gewaehlte Welt gilt (kein "Host-Kopie gilt")
        CoopDoLoad(slot);   // Game_LoadBegin_SaveGame meldet den Partner
        CoopLoadingFromList = false;
    }

    // v20: Enter kam per KeyToggled nie an (Test 18.10.: "kann nicht mit Enter geladen werden") - das
    // Gedrueckt-Ereignis verbraucht vorher die Engine bzw. ein Plugin, die F-Tasten dagegen kommen an.
    // Darum die Flanke selbst aus dem Tastenzustand bilden (zinput->KeyPressed, dazu GetAsyncKeyState,
    // nur wenn das Spielfenster vorn ist).
    static bool CoopKeyEdge(int zkey, int vk, bool& was) {
        bool down = zinput->KeyPressed(zkey) != 0;
        if (!down && vk) {
            HWND fg = GetForegroundWindow();
            DWORD pid = 0;
            if (fg)
                GetWindowThreadProcessId(fg, &pid);
            if (pid == GetCurrentProcessId() && (GetAsyncKeyState(vk) & 0x8000))
                down = true;
        }
        const bool edge = down && !was;
        was = down;
        return edge;
    }

    static bool BrowserEnterWas = true;
    static bool BrowserNextWas = true;

    static void BrowserPulse() {
        bool& enterWas = BrowserEnterWas;
        if (!CoopBackupKey)
            return;
        // v80: bei offenem Spielmenue gesperrt (Laden aus der Menue-Schleife liess im Test v79 das Tagebuch aktiv und das Spiel
        // pausiert). F9 wird hier verbraucht, damit die Liste nicht nach dem Schliessen des Menues aufgeht.
        if (player && zCMenu::GetActive()) {
            zinput->KeyToggled(CoopBackupKey);
            enterWas = true;
            BrowserNextWas = true;
            if (BrowserOpen) {
                BrowserOpen = false;
                SaveSyncLog("Liste: geschlossen - Menue %s offen (Laden aus der Liste nur ohne Menue)", zCMenu::GetActive()->name.ToChar());
            }
            return;
        }
        if (zinput->KeyToggled(CoopBackupKey) && !IsPlayerTalkingWithAnybody()) {
            BrowserOpen = !BrowserOpen;
            if (BrowserOpen) {
                BrowserScan();
                enterWas = true;   // beim Oeffnen gehaltene Tasten nicht als Druck werten
                BrowserNextWas = true;
            }
        }
        if (!BrowserOpen)
            return;
        const int n = (int)BrowserList.size();
        // v22: Shift+F8 kam nicht an (Test 18.10.) - F8 und Shift wie Enter selbst aus dem Tastenzustand bilden.
        const int nextVk = CoopBackupNextKey == KEY_F8 ? VK_F8 : 0;
        if (CoopBackupNextKey && CoopKeyEdge(CoopBackupNextKey, nextVk, BrowserNextWas)) {
            bool up = zinput->KeyPressed(KEY_LSHIFT) || zinput->KeyPressed(KEY_RSHIFT);
            if (!up) {
                HWND fg = GetForegroundWindow();
                DWORD pid = 0;
                if (fg)
                    GetWindowThreadProcessId(fg, &pid);
                up = pid == GetCurrentProcessId() && (GetAsyncKeyState(VK_SHIFT) & 0x8000);
            }
            BrowserSel = up ? (BrowserSel > 0 ? BrowserSel - 1 : 0) : (BrowserSel < n - 1 ? BrowserSel + 1 : BrowserSel);
            BrowserConfirmMs = 0;
        }
        const bool enter = CoopKeyEdge(KEY_RETURN, VK_RETURN, enterWas) || zinput->KeyToggled(KEY_NUMPADENTER);
        if (enter && n > 0) {
            if (BrowserConfirmMs && CurrentMs < BrowserConfirmMs + 4000) {
                SaveSyncLog("Liste: Enter bestaetigt - lade Eintrag %d", BrowserSel);
                BrowserLoadSelected();
                return;
            }
            BrowserConfirmMs = CurrentMs;
            SaveSyncLog("Liste: Enter - noch einmal zum Laden");
        }

        auto font = screen->GetFontName();
        screen->SetFont(zSTRING("Font_Old_10_White_Hi.TGA"));
        int y = 1200;
        screen->Print(300, y, zSTRING(CoopTr("Coop-Sicherungen ", "Coop backups ")) + zSTRING(n > 0 ? BrowserSel + 1 : 0) + zSTRING("/") + zSTRING(n) +
            zSTRING(CoopTr(" - F8 " "\xE4" "lter, Shift+F8 neuer, 2x Enter laden, F9 schlie" "\xDF" "en", " - F8 older, Shift+F8 newer, 2x Enter load, F9 close")));
        y += 250;
        if (BrowserConfirmMs && CurrentMs < BrowserConfirmMs + 4000) {
            screen->Print(300, y, zSTRING(CoopTr("Enter noch einmal: Stand laden (der Partner " "\xFC" "bernimmt ihn)", "Enter again: load the save (your partner takes it over)")));
            y += 250;
        }
        const int first = BrowserSel > 4 ? BrowserSel - 4 : 0;
        for (int i = first; i < n && i < first + 10; i++) {
            const CoopSaveMeta& m = BrowserList[i];
            char line[300];
            _snprintf(line, sizeof(line), CoopTr("%s %s  [%s]  Welt: %s  %s", "%s %s  [%s]  world: %s  %s"), i == BrowserSel ? ">" : " ", m.time.empty() ? "?" : m.time.c_str(),
                m.kind.empty() ? "?" : m.kind.c_str(), m.src.empty() ? "?" : m.src.c_str(), m.name.c_str());
            line[sizeof(line) - 1] = 0;
            screen->Print(300, y, zSTRING(line));
            y += 220;
        }
        screen->SetFont(font);
    }

    void CoopSaveSyncInit() {
        CoopSaveSlots.clear();
        if (CoopConfig.contains("coopSaveSlots") && CoopConfig["coopSaveSlots"].is_array()) {
            for (auto const& v : CoopConfig["coopSaveSlots"])
                if (v.is_number() && v.get<int>() >= 1 && v.get<int>() <= 15)
                    CoopSaveSlots.push_back(v.get<int>());
        }
        if (CoopSaveSlots.empty())
            CoopSaveSlots = { 6, 7, 8, 9 };
        if (CoopConfig.contains("coopAutoSaveMinutes") && CoopConfig["coopAutoSaveMinutes"].is_number())
            CoopAutoSaveMinutes = CoopConfig["coopAutoSaveMinutes"].get<int>();
        if (CoopConfig.contains("coopTakeHostStateOnConnect") && CoopConfig["coopTakeHostStateOnConnect"].is_boolean())
            CoopTakeOnConnect = CoopConfig["coopTakeHostStateOnConnect"].get<bool>();
        if (CoopConfig.contains("coopBackupBeforeLoad") && CoopConfig["coopBackupBeforeLoad"].is_boolean())
            CoopBackupBeforeLoad = CoopConfig["coopBackupBeforeLoad"].get<bool>();
        CoopBackupKey = ReadConfigKey("coopBackupKey", "KEY_F9");
        CoopBackupNextKey = ReadConfigKey("coopBackupNextKey", "KEY_F8");
        CoopSyncKey = ReadConfigKey("coopSyncKey", "KEY_F6");
    }

    // Am Ende von Game_Loop: kann einen Ladevorgang ausloesen, danach laeuft in diesem Frame nichts mehr.
    void CoopSaveSyncPulse() {
        HandleHeroApply();   // v76: auch ohne Verbindung zu Ende bringen - der Auftrag kam aus einem Coop-Ladevorgang
        HandleMoveReset();
        if (HandleOwnListLoad())   // v91: auch ohne Coop (Liste im Esc-Menue)
            return;
        if (!SaveSyncAktiv() || IsLoadingLevel)
            return;
        CoopPumpFileOut();
        FinishPendingArchive();
        HandleDisturbance();
        HandlePausePending();
        if (RemoteLoadPending) {
            // v80: eigene Welt wird sofort geladen - bei offenem Menue warten; fremde Welt startet nur die Uebernahme
            // (Dateien kommen auch bei offenem Menue, geladen wird in TakeJobPulse)
            if (!RemoteLoad.sid.empty() && CoopMetaMine(RemoteLoad) && CoopLoadBlockedByMenu("Ladeauftrag"))
                return;
            HandleRemoteLoad();
            return;
        }
        if (TakeJobPulse())
            return;
        if (HandleHostAsk())     // v90: Ladewunsch eines Clients (Host laedt seine Kopie)
            return;
        if (HandleClientAsk())   // v90: eigener Ladewunsch - Host hat keine Kopie bzw. keine Antwort
            return;
        HandleJoin();
        HandlePostTakeSave();
        HandleRemoteSave();
        HandleHostQuickSave(HostPauseSavePending, HostPauseSaveUntil, "pause", HostPauseSaveFor);
        HandleHostQuickSave(HostAbortSavePending, HostAbortSaveUntil, "abbruch", HostAbortSaveFor);
        HandleHostSync();
        HandleAutoSave();
        HandleSyncTriggers();
        BrowserPulse();
        DrawDesyncWarn();
    }

    // v76: Shift+F2 beim verbundenen Client (Plugin.cpp, CoopKeyClient). Nutzer: "sobald Stop world synchronization gemacht wird,
    // speichert der Host direkt". Pause: erst die gemeinsame Sicherung, dann pausieren. Fortsetzen: Host-Stand uebernehmen, eigener
    // Held vom Pausenbeginn (Variante b). Rueckgabe true = hier erledigt; false = wie bisher umschalten (Spielstaende getrennt,
    // Fortsetzen ohne Uebernahme).
    bool CoopSaveSyncPauseKey() {
        if (!ClientThread || ServerThread || !SaveSyncAktiv() || !player)
            return false;
        if (PausePending) {
            ChatLog(CoopTr("Pause: warte noch auf die gemeinsame Sicherung ...", "Pause: waiting for the shared backup ..."));
            return true;
        }
        if (JoinPending || TakeJob.active || RemoteLoadPending || IsLoadingLevel) {
            ChatLog(IsCoopPaused ? CoopTr("Fortsetzen l" "\xE4" "uft schon - bitte warten.", "Resume already running - please wait.") : CoopTr("Abgleich l" "\xE4" "uft - Pause erst danach.", "Sync running - pause afterwards."), zCOLOR(255, 200, 80, 255));
            return true;
        }
        if (!IsCoopPaused) {
            PausePending = true;
            PauseSince = CurrentMs;
            json j;
            j["why"] = "pause";
            SaveSyncSend(j, SYNC_STATE_REQ);
            ChatLog(CoopTr("Pause: gemeinsame Sicherung, danach wird die Synchronisation angehalten.", "Pause: shared backup, then synchronization is paused."));
            SaveSyncLog("Pause gewuenscht - der Host legt eine gemeinsame Sicherung an");
            return true;
        }
        if (!CoopRuleTakeOnConnect()) {   // Einstellung des Hosts "eigenen Stand behalten": wie bisher fortsetzen
            DivergedHero.valid = false;
            return false;
        }
        JoinStart("resume", false);
        ChatLog(CoopTr("Fortsetzen: der Stand des Hosts wird " "\xFC" "bernommen, eigener Held wie bei Pausenbeginn.", "Resume: taking over the host's state, own hero as at pause start."));
        return true;
    }

    // v76: PC-Kennung fuer das Hauptmenue (hello) und Zuordnung PC -> Rolle aus dem hello des Partners
    std::string CoopSaveSyncPcId() {
        return CoopMyPc();
    }

    void CoopSaveSyncNotePc(const std::string& pc, const std::string& role) {
        CoopNotePc(pc, role);
    }

    // --- v40: Mehrspieler im Hauptmenue (CoopMenuLobby.cpp) ----------------------------------------------------------------

    void CoopSaveSyncSetMenu(bool on) {
        if (CoopSaveSyncInMenu != on)
            SaveSyncLog(on ? "Wartephase im Hauptmenue - Ladeauftrag und Dateien schon ohne Spielsitzung" : "Wartephase beendet");
        CoopSaveSyncInMenu = on;
    }

    // Im Hauptmenue: nur der Ladeauftrag des Hosts (Uebernahme, Dateien holen) - der Rest braucht eine Welt.
    void CoopSaveSyncMenuPulse() {
        if (!SaveSyncAktiv())
            return;
        CoopPumpFileOut();
        if (RemoteLoadPending) {
            HandleRemoteLoad();
            return;
        }
        TakeJobPulse();
    }

    // Nach dem gemeinsamen Laden aus dem Menue haben beide denselben Stand: kein "Stand des Hosts uebernehmen" beim Verbinden.
    void CoopSaveSyncSkipConnectTake() {
        CoopConnectTakeSent = true;
        SaveSyncLog("gemeinsam aus dem Hauptmenue geladen - keine Uebernahme des Host-Stands nach dem Verbinden");
    }

    // Host beendet: ENet trennt alle Peers ohne DISCONNECT-Ereignis an den Hauptfaden (im Menue auch mit Mitspieler moeglich).
    void CoopSaveSyncPeersReset() {
        CoopPeerCount = 0;
    }

    std::string CoopSaveSyncSlotSid(int slot) {
        CoopSaveMeta m;
        return CoopReadMeta(CoopSlotDir(slot), m) ? m.sid : std::string();
    }

    // v55: Kennung, Name und Zeit eines Slots (Spielstand-Liste im COOP-Menue). Zeit als "TT.MM.JJJJ hh:mm".
    bool CoopSaveSyncSlotInfo(int slot, std::string& sid, std::string& name, std::string& time) {
        CoopSaveMeta m;
        if (!CoopReadMeta(CoopSlotDir(slot), m))
            return false;
        sid = m.sid;
        name = m.name;
        time = m.time;
        return true;
    }

    // Gemeinsamer Stand fuer diesen PC: eigene Kopie der Kennung mit eigenem Helden (COOPHERO.TXT).
    bool CoopSaveSyncHasOwnHero(const std::string& sid) {
        const std::string own = CoopOwnCopyDir(sid);
        return !own.empty() && CoopFileExists(own + "COOPHERO.TXT");
    }

    // --- v93: Liste "Spielstand waehlen" aus dem Coop-Archiv (CoopMenuLobby.cpp) ------------------------------------------------

    std::string CoopSaveSyncCurrentRun() {
        return CoopCurrentRun;
    }

    void CoopSaveSyncSetRun(const std::string& run, const char* why) {
        CoopRunSet(run, why);
    }

    // Host beim gemeinsamen neuen Spiel: neuer Durchlauf "JJJJ-MM-TT_hh-mm-ss_<ich>_<Mitspieler>"
    std::string CoopSaveSyncNewRun(const std::string& partnerName) {
        if (!partnerName.empty())
            CoopPartnerNick = partnerName;
        return CoopRunNew(MyNickname.ToChar(), CoopPartnerNick.empty() ? std::string("Mitspieler") : CoopPartnerNick);
    }

    void CoopSaveSyncNotePartnerNick(const std::string& nick) {
        if (!nick.empty())
            CoopPartnerNick = nick;
    }

    // Durchlaeufe, die dieser PC kennt (Archiv-Ordner, Staende in Slots) - der Partner zeigt damit "lokale Staende" nur aus
    // gemeinsamen Durchlaeufen
    void CoopSaveSyncKnownRuns(std::set<std::string>& out) {
        out.clear();
        CoopArchiveScan();
        for (auto const& d : CoopArchiveRunDirs) {
            std::string n = d.substr(0, d.size() - 1);
            n = n.substr(n.find_last_of('\\') + 1);
            out.insert(n);
        }
        for (int s = 1; s <= 15; s++) {
            const std::string r = CoopSlotRun(s);
            if (!r.empty())
                out.insert(r);
        }
        if (!CoopCurrentRun.empty())
            out.insert(CoopCurrentRun);
    }

    static bool CoopListTechnical(const CoopSaveMeta& m) {
        return !m.sid.compare(0, 14, "vorUebernahme_") || !m.sid.compare(0, 7, "vorher_") || !m.sid.compare(0, 5, "rest_") ||
            m.kind == "sicherung" || m.kind == "vorher";
    }

    // Sortierschluessel: Speicherzeit (Unix), sonst aus "TT.MM.JJJJ hh:mm"
    static std::string CoopListKey(long long stime, const std::string& time) {
        long long t = stime;
        if (t <= 0 && time.size() >= 16) {
            struct tm tmv = {};
            tmv.tm_mday = atoi(time.substr(0, 2).c_str());
            tmv.tm_mon = atoi(time.substr(3, 2).c_str()) - 1;
            tmv.tm_year = atoi(time.substr(6, 4).c_str()) - 1900;
            tmv.tm_hour = atoi(time.substr(11, 2).c_str());
            tmv.tm_min = atoi(time.substr(14, 2).c_str());
            tmv.tm_isdst = -1;
            t = (long long)mktime(&tmv);
        }
        char b[32];
        _snprintf(b, sizeof(b), "%015lld", t > 0 ? t : 0);
        b[sizeof(b) - 1] = 0;
        return b;
    }

    // Eigene Staende, je Kennung einmal, neueste zuerst: gemeinsame Staende (eigene Kopie in einem Slot oder im Archiv, Hauptordner
    // und Durchlauf-Ordner) und lokale Staende eines Durchlaufs (Slots mit COOPRUN.TXT, ohne Kennung). Technische Sicherungen nicht.
    void CoopSaveSyncListJoint(std::vector<CoopListEntry>& out) {
        out.clear();
        std::set<std::string> seen;
        for (int s = 1; s <= 15; s++) {
            const std::string d = CoopSlotDir(s);
            if (!CoopFileExists(d + "SAVEINFO.SAV"))
                continue;
            CoopSaveMeta m;
            if (CoopReadMeta(d, m)) {
                if (!CoopMetaMine(m) || CoopListTechnical(m) || seen.count(m.sid))
                    continue;
                seen.insert(m.sid);
                CoopListEntry e;
                e.sid = m.sid;
                e.run = m.run;
                e.name = m.name;
                e.time = m.time;
                e.kind = m.kind;
                e.dir = d;
                e.slot = s;
                e.key = CoopListKey(m.stime, m.time);
                out.push_back(e);
                continue;
            }
            bool local = false;
            const std::string run = CoopSlotRun(s, &local);
            if (!local)
                continue;
            CoopListEntry e;   // lokaler Stand eines Durchlaufs (eigene Gothic-Speicherung ohne Kennung)
            e.run = run;
            e.name = CoopSlotName(s);
            e.kind = "lokal";
            e.dir = d;
            e.slot = s;
            std::vector<std::uint8_t> raw;
            long long st = 0;
            if (CoopReadWholeFile(d + "COOPRUN.TXT", raw) && !raw.empty()) {
                const std::string t((const char*)&raw[0], raw.size());
                const size_t p = t.find("stime="), q = t.find("time=");
                if (p != std::string::npos)
                    st = _atoi64(t.c_str() + p + 6);
                if (q != std::string::npos && (p == std::string::npos || q < p))
                    e.time = t.substr(q + 5, t.find_first_of("\r\n", q) - q - 5);
            }
            e.key = CoopListKey(st > 0 ? st : CoopFileUnixTime(d + "SAVEINFO.SAV"), e.time);
            out.push_back(e);
        }
        CoopArchiveScan();
        for (auto const& a : CoopArchiveIndex) {
            if (a.first.find("__VON_") != std::string::npos || a.first.find(".PART") != std::string::npos)
                continue;
            CoopSaveMeta m;
            if (!CoopReadMeta(a.second, m) || CoopUpperName(m.sid) != a.first || CoopListTechnical(m) || seen.count(m.sid))
                continue;
            seen.insert(m.sid);
            CoopListEntry e;
            e.sid = m.sid;
            e.run = m.run;
            e.name = m.name;
            e.time = m.time;
            e.kind = m.kind;
            e.dir = a.second;
            e.key = CoopListKey(m.stime, m.time);
            out.push_back(e);
        }
        std::sort(out.begin(), out.end(), [](const CoopListEntry& x, const CoopListEntry& y) { return x.key > y.key; });
    }

    // Stand einer Kennung ladebereit machen: liegt die eigene Kopie in einem Slot, dieser; sonst aus dem Archiv in den naechsten
    // Coop-Slot (CoopNextReservedSlot, sicher ersetzt wie F9-Liste und Ladewunsch). -1 = nicht moeglich (why).
    int CoopSaveSyncPrepareSlot(const std::string& sid, std::string& why) {
        if (sid.empty()) {
            why = CoopTr("ohne Kennung", "no id");
            return -1;
        }
        for (int s = 1; s <= 15; s++) {
            CoopSaveMeta sm;
            if (CoopReadMeta(CoopSlotDir(s), sm) && sm.sid == sid && CoopMetaMine(sm))
                return s;
        }
        const std::string own = CoopOwnCopyDir(sid);
        if (own.empty()) {
            why = CoopTr("keine eigene Kopie", "no own copy");
            return -1;
        }
        const int slot = CoopNextReservedSlot();
        const int n = CoopSlotReplace(slot, own, nullptr, why);
        SaveSyncLog("Liste: %s aus dem Archiv in Slot %d gelegt (%d Dateien%s%s)", sid.c_str(), slot, n, n <= 0 ? ", FEHLER: " : ", geprueft",
            n <= 0 ? why.c_str() : "");
        return n > 0 ? slot : -1;
    }

    // Status fuer das COOP-Menue. Gleich bleibend, solange sich nichts Wesentliches aendert (er wird bei jeder Aenderung
    // geloggt) - den laufenden Fortschritt liefert CoopSaveSyncMenuProgress.
    std::string CoopSaveSyncMenuStatus() {
        if (RemoteLoadPending)
            return CoopTr("Der Host l\xE4" "dt - bereite den Stand vor ...", "The host is loading - preparing the save ...");
        if (TakeJob.active) {
            if (TakeJob.step <= 1) {
                char b[300];
                _snprintf(b, sizeof(b), CoopTr("Hole den Stand \"%s\" vom Host", "Fetching the save \"%s\" from the host"), TakeJob.name.c_str());
                b[sizeof(b) - 1] = 0;
                return b;
            }
            return CoopTr("Stand ist da - lade ...", "Save is here - loading ...");
        }
        return CoopTakeError;
    }

    // v71: Fortschritt der laufenden Uebertragung fuer die Anzeige (nicht fuers Log), hoechstens zweimal je Sekunde neu
    std::string CoopSaveSyncMenuProgress() {
        static std::string text;
        static long long ms = 0;
        const long long now = GetCurrentMs();
        if (now < ms + 500)
            return text;
        ms = now;
        text.clear();
        if (!TakeJob.active || TakeJob.step > 1)
            return text;
        auto it = FilesIn.find(TakeJob.sid + "|" + TakeJob.src);
        char b[200];
        if (it == FilesIn.end() || it->second.total <= 0)
            _snprintf(b, sizeof(b), CoopTr(": warte auf die ersten Daten (%d s) ...", ": waiting for the first data (%d s) ..."), TakeJob.requested ? (int)((now - TakeJob.since) / 1000) : 0);
        else {
            const CoopFileIn& in = it->second;
            const double secs = (now - in.startMs) / 1000.0;
            _snprintf(b, sizeof(b), CoopTr(": %.1f von %.1f MB (%d %%, %.0f KB/s) - bitte warten", ": %.1f of %.1f MB (%d %%, %.0f KB/s) - please wait"), in.bytes / 1048576.0, in.total / 1048576.0,
                (int)(in.bytes * 100 / in.total), secs > 0.5 ? in.bytes / 1024.0 / secs : 0.0);
        }
        b[sizeof(b) - 1] = 0;
        text = b;
        return text;
    }

    // --- v39: voller Abgleich nach "Restoring world synchronization" und nach jedem Laden ------------------------------
    //
    // Nutzer: "wenn man disconnected und dann reconnected, dann sind die Sachen nicht mehr synchron, z. B. Position und Leben
    // des Mitspielers". Die Verbinden-Taste beim Client schaltet nur IsCoopPaused um (Plugin.cpp); in der Pause verwirft der
    // Client alle Pakete (ProcessCoopPacket). GT schickt nur Aenderungen (Lage ab 5 Einheiten, HP bei Aenderung) - was sich in
    // der Pause beim Partner geaendert hat und seitdem ruht, blieb alt. Beim Verbinden macht GT dagegen alles neu (Host:
    // LocalNpc::Reinit fuer den eigenen Helden und alle gesendeten NPCs; Client: Stand des Hosts uebernehmen, Weltbestand).
    // Jetzt: Fortsetzen = wie Verbinden; ausserdem fordert der Client nach jedem Laden den vollen Stand an (Paket 47 RESYNC).
    void CoopResyncRequest(const char* why) {
        if (!ClientThread || ServerThread || MyselfId == "_player_")
            return;
        if (Myself)
            Myself->Reinit();   // eigener Stand voll an den Host
        json j;
        j["type"] = SYNC_RESYNC;
        j["id"] = MyselfId.ToChar();
        j["why"] = why;
        CoopTraceCount(TRACE_SENT, SYNC_RESYNC);
        ReadyToSendJsons.enqueue(j);
        SaveSyncLog("voller Stand beim Host angefordert (%s), eigener Stand neu gesendet", why);
    }

    void CoopResyncResumed() {
        if (!ClientThread || ServerThread)
            return;
        CoopResyncRequest("Synchronisation fortgesetzt");
        CoopWorldSyncRequestAgain();   // Truhen und Gegenstaende neu vom Host
        if (CoopRuleTakeOnConnect()) { // wie beim Verbinden: Stand des Hosts uebernehmen, eigener Held bleibt (v75: Regel des Hosts)
            CoopConnectTakeSent = false;
            CoopFirstActiveMs = CurrentMs;
            ChatLog(CoopTr("Synchronisation fortgesetzt - der Stand des Hosts wird wie beim Verbinden " "\xFC" "bernommen.", "Synchronization resumed - taking over the host's state as when connecting."));
        }
    }

    void CoopFarSyncSendAllDead();   // v41: CoopFarSync.cpp

    void CoopResyncReceive(json& e, const std::string& id) {
        if (!ServerThread || e.contains("by"))
            return;   // nur der Host antwortet; andere Clients bekommen die weitergeleitete Fassung
        CoopFarSyncSendAllDead();   // v41: alle Toten - auch die, die der Client ausserhalb der Reichweite oder in der Pause verpasst hat
        if (Myself)
            Myself->Reinit();
        int n = 0;
        for (auto& b : BroadcastNpcs) {
            b.second->Reinit();
            n++;
        }
        const std::string why = e.contains("why") && e["why"].is_string() ? e["why"].get<std::string>() : std::string("-");
        SaveSyncLog("%s bittet um den vollen Stand (%s) - eigener Held und %d NPCs neu gesendet", id.c_str(), why.c_str(), n);
    }

#else
    void CoopSaveSyncSetMenu(bool) {}
    void CoopSaveSyncMenuPulse() {}
    void CoopSaveSyncSkipConnectTake() {}
    void CoopSaveSyncPeersReset() {}
    std::string CoopSaveSyncSlotSid(int) { return std::string(); }
    bool CoopSaveSyncHasOwnHero(const std::string&) { return false; }
    bool CoopSaveSyncSlotInfo(int, std::string&, std::string&, std::string&) { return false; }
    std::string CoopSaveSyncMenuStatus() { return std::string(); }
    std::string CoopSaveSyncMenuProgress() { return std::string(); }
    void CoopSaveSyncMenuPeerWantsState(const std::string&) {}
    void CoopSaveSyncConnectionEnded() {}
    void CoopSaveSyncConnectionLost() {}
    void CoopSaveSyncOnNewGame() {}
    void CoopSaveSyncPlaceBesideStart() {}
    void CoopSaveSyncRoleStart(bool) {}
    bool CoopSaveSyncPauseKey() { return false; }
    std::string CoopSaveSyncPcId() { return std::string(); }
    void CoopSaveSyncNotePc(const std::string&, const std::string&) {}
    void CoopSaveSyncPeerGone(const std::string&) {}
    void CoopResyncRequest(const char*) {}
    void CoopResyncResumed() {}
    void CoopResyncReceive(json&, const std::string&) {}
    void CoopSaveSyncOnSaveEnd() {}
    void CoopSaveSyncOnLoadBegin() {}
    void CoopSaveSyncWorldLoaded() {}
    void CoopSaveSyncApplyHeroAtLoad() {}
    void CoopSaveSyncPeerJoined() {}
    void CoopSaveSyncPeerLeft() {}
    int CoopSaveSyncPeers() { return 0; }
    bool CoopSaveSyncHandles(int) { return false; }
    void CoopSaveSyncReceive(json&, const std::string&) {}
    bool CoopSaveSyncAskLoad(int) { return false; }
    void CoopSaveSyncLoadSoon(int) {}
    void CoopSaveSyncInit() {}
    void CoopSaveSyncPulse() {}
    void CoopSaveSyncListJoint(std::vector<CoopListEntry>& out) { out.clear(); }
    int CoopSaveSyncPrepareSlot(const std::string&, std::string& why) { why = "nicht G1"; return -1; }
    bool CoopSaveSyncAskLoadSid(const std::string&) { return false; }
    std::string CoopSaveSyncCurrentRun() { return std::string(); }
    void CoopSaveSyncSetRun(const std::string&, const char*) {}
    std::string CoopSaveSyncNewRun(const std::string&) { return std::string(); }
    void CoopSaveSyncNotePartnerNick(const std::string&) {}
    void CoopSaveSyncKnownRuns(std::set<std::string>& out) { out.clear(); }
#endif
}
