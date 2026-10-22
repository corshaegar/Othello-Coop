// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

    // --- Engine-Funktionen, die Mods an festen Adressen umwidmen ---------------------------
    //
    // GT bindet Engine-Funktionen ueber feste Adressen (zCall). "Gothic: Othello" (OTHELLO_ABI.DLL)
    // behandelt Engine-Funktionen ohne direkte Aufrufer als freien Platz und legt dort eigenen
    // Code ab. Drei davon ruft GT auf (Speicherabbild Othello 4.4.2.1 gegen GothicMod.exe):
    //
    //   0x006B86A0  oCNpc::MakeSpellBook      erste 13 Byte -> "this->visual = [esp+4]; ret 4"
    //   0x006B8B40  oCNpc::UseStandAI         -> Rumpf von oCSpawnManager::DeleteNpc (verschoben)
    //   0x006D0DE0  oCSpawnManager::DeleteNpc -> 64-Byte-Matrixkopie (AVX bzw. SSE)
    //
    // Die Engine selbst merkt davon nichts: ihre beiden DeleteNpc-Aufrufe (0x00650AFB,
    // 0x00693AF8) zeigen jetzt auf 0x006B8B40, MakeSpellBook und UseStandAI ruft sie nie direkt.
    // GT dagegen schrieb mit MakeSpellBook Stapelmuell in npc->visual und verschob ESP um 4 Byte,
    // UseStandAI lief Othellos Listencode auf einem oCNpc, und DeleteNpc ueberschrieb die ersten
    // 64 Byte des Spawn-Managers mit denen des NPCs.
    //
    // Abhilfe je Funktion: liegt am festen Ort noch das Original (oder ein Detour darauf), wie
    // bisher aufrufen. Sonst eine originalgetreue Nachbildung verwenden - Befehl fuer Befehl aus
    // GothicMod.exe uebernommen, nur die relativen Aufrufe sind absolut. Die Nachbildungen rufen
    // ausschliesslich Engine-Helfer, deren Anfang vorher geprueft wird. Bei DeleteNpc hat die
    // Adresse Vorrang, an der die Engine selbst ihre Aufrufe bedient.

    // --- Log-Archiv (v35) ------------------------------------------------------------------------
    //
    // Nutzer: "keine Logs loeschen, sondern immer archivieren". Vor dem ersten Log-Schreiben dieses Starts verschiebt GT alle
    // system\GothicCoop*.log der vorigen Sitzung nach CoopLogs\<Fassung>\<Start der Sitzung>\ (Spielordner, auf jedem PC).
    // Fassung und Start stehen ab v35 in der ersten Zeile von GothicCoopOthello.log ("GothicCoop Othello-Fassung v35, Start
    // ..."); aeltere Logs ohne diese Zeile kommen nach CoopLogs\vor-v35\<letzte Aenderung>\. Geloescht wird nichts. Laesst sich eine Datei nicht verschieben (z. B. von einem Editor gesperrt), bleibt sie liegen und
    // wird weiter angehaengt; fuer GothicCoopOthello.log merkt GT sich dann die alte Groesse, damit der Log-Versand erst dahinter
    // beginnt. Jeder Log-Schreiber ruft CoopLogArchiveOnce() vor seinem ersten fopen. Danach steht die Fassung im neuen Log.
    // Nur Windows-Aufrufe, keine Engine-Stelle.

#define COOP_OTHELLO_FASSUNG "v97"

    // --- v95: Sprache der Coop-Anzeigen ----------------------------------------------------------
    // Nutzer: "dass man die Sprache der Coop-Einstellungen und Anzeigen auf Englisch stellen kann". Gothic.ini [GOTHICCOOP]
    // coopLang (0 Deutsch, 1 English; Auswahlfeld in "Deine Figur" und Einstellungen > COOP), gelesen von CoopLangRefresh
    // (CoopLook.cpp). Jeder Text fuer den Spieler geht ueber CoopTr(deutsch, english); Menue-Texte aus COOP_MENU.D tauscht
    // CoopLangMenuPulse (CoopMenuLobby.cpp). Logs bleiben deutsch.
    static int CoopLangEn = 0;

    // char* statt const char*: Unions string::Combine nimmt als Format nur "const text" (= char* const, Types.h) - die Texte
    // werden nie veraendert
    static inline char* CoopTr(const char* de, const char* en) {
        return const_cast<char*>(CoopLangEn ? en : de);
    }

    // --- v58: Anzeige der Fassung --------------------------------------------------------------
    //
    // Nutzer: "ueberall mal die Version der Coop-Mod anzeigen, im Spiel und auch im Hauptmenue, statt der alten Coop-Version",
    // "vielleicht noch mit Datum/Uhrzeit-Stempel der Version im Hauptmenue". GTs COOP_VERSION (60) bleibt nur Protokollnummer
    // (Pruefung beim Verbinden) und wird nicht mehr angezeigt. Bauzeit = __DATE__/__TIME__ dieser Uebersetzungseinheit
    // (Interface.cpp bindet ueber Sources.h alle Quellen ein - jeder Build uebersetzt sie neu).
    static std::string CoopVersionName() {
        return std::string("GothicCoop ") + COOP_OTHELLO_FASSUNG;
    }

    static std::string CoopBuildStamp() {   // "Oct 20 2026" + "15:42:07" -> "20.10.2026, 15:42 Uhr"
        static const char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
        const char* d = __DATE__;
        const char* hit = strstr(months, std::string(d, 3).c_str());
        const int month = hit ? (int)(hit - months) / 3 + 1 : 0;
        char b[48];
        _snprintf(b, sizeof(b), CoopLangEn ? "%02d.%02d.%.4s, %.5s" : "%02d.%02d.%.4s, %.5s Uhr", atoi(d + 4), month, d + 7, __TIME__);
        b[sizeof(b) - 1] = 0;
        return b;
    }

    void CoopCamNpcGone(oCNpc* npc);   // v44: CoopCamera.cpp - Kamera vor dem Loeschen einer Figur zurueck zum Helden

    static long CoopLogSessionStart = 0;   // ab hier gehoert system\GothicCoopOthello.log zu diesem Start

    static CRITICAL_SECTION* CoopLogArchiveLock() {
        static CRITICAL_SECTION cs;
        static bool ready = (InitializeCriticalSection(&cs), true);
        (void)ready;
        return &cs;
    }

    static void CoopLogArchiveOnce() {
        static volatile LONG done = 0;
        if (done)
            return;
        EnterCriticalSection(CoopLogArchiveLock());
        if (done) {
            LeaveCriticalSection(CoopLogArchiveLock());
            return;
        }
        struct Entry { std::string name; };
        std::vector<Entry> files;
        FILETIME newest = { 0, 0 };
        WIN32_FIND_DATAA fd;
        HANDLE h = FindFirstFileA("system\\GothicCoop*.log", &fd);
        if (h != INVALID_HANDLE_VALUE) {
            do {
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    continue;
                files.push_back({ fd.cFileName });
                if (CompareFileTime(&fd.ftLastWriteTime, &newest) > 0)
                    newest = fd.ftLastWriteTime;
            } while (FindNextFileA(h, &fd));
            FindClose(h);
        }
        char note[600];
        if (files.empty()) {
            _snprintf(note, sizeof(note), "Log-Archiv: keine Logs einer vorigen Sitzung in system\\");
        }
        else {
            // Fassung und Start aus der ersten Zeile des alten Logs, sonst "vor-v35" und die letzte Aenderung.
            char ver[32] = "vor-v35";
            int y = 0, mo = 0, d = 0, hh = 0, mi = 0, ss = 0;
            bool haveStart = false;
            if (FILE* old = fopen("system\\GothicCoopOthello.log", "r")) {
                char line[256] = "";
                char v[32] = "";
                if (fgets(line, sizeof(line), old) &&
                    sscanf(line, "GothicCoop Othello-Fassung %31[^,], Start %d-%d-%d %d:%d:%d", v, &y, &mo, &d, &hh, &mi, &ss) == 7) {
                    bool clean = v[0] != 0;
                    for (const char* p = v; *p; p++)
                        if (!isalnum((unsigned char)*p) && *p != '.' && *p != '-' && *p != '_')
                            clean = false;
                    if (clean) {
                        strncpy(ver, v, sizeof(ver) - 1);
                        haveStart = true;
                    }
                }
                fclose(old);
            }
            if (!haveStart) {
                FILETIME loc;
                SYSTEMTIME st;
                FileTimeToLocalFileTime(&newest, &loc);
                FileTimeToSystemTime(&loc, &st);
                y = st.wYear; mo = st.wMonth; d = st.wDay; hh = st.wHour; mi = st.wMinute; ss = st.wSecond;
            }
            CreateDirectoryA("CoopLogs", NULL);
            const std::string verDir = std::string("CoopLogs\\") + ver;
            CreateDirectoryA(verDir.c_str(), NULL);
            char base[MAX_PATH];
            _snprintf(base, sizeof(base), "%s\\%04d-%02d-%02d_%02d-%02d-%02d", verDir.c_str(), y, mo, d, hh, mi, ss);
            base[sizeof(base) - 1] = 0;
            std::string target = base;
            for (int n = 2; !CreateDirectoryA(target.c_str(), NULL); n++) {
                if (GetLastError() != ERROR_ALREADY_EXISTS || n > 99) {
                    target.clear();
                    break;
                }
                target = std::string(base) + "_" + std::to_string(n);
            }
            int moved = 0, kept = 0;
            for (auto& f : files) {
                const std::string src = "system\\" + f.name;
                if (!target.empty() &&
                    MoveFileExA(src.c_str(), (target + "\\" + f.name).c_str(), MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH)) {
                    moved++;
                    continue;
                }
                kept++;
                if (_stricmp(f.name.c_str(), "GothicCoopOthello.log") == 0) {
                    WIN32_FILE_ATTRIBUTE_DATA a;
                    if (GetFileAttributesExA(src.c_str(), GetFileExInfoStandard, &a))
                        CoopLogSessionStart = (long)a.nFileSizeLow;
                }
            }
            if (kept)
                _snprintf(note, sizeof(note), "Log-Archiv: %d Datei(en) der vorigen Sitzung nach %s\\ verschoben, %d nicht"
                    " verschiebbar (bleiben in system\\ und werden weiter angehaengt)", moved,
                    target.empty() ? "(kein Ordner)" : target.c_str(), kept);
            else
                _snprintf(note, sizeof(note), "Log-Archiv: %d Datei(en) der vorigen Sitzung nach %s\\ verschoben", moved,
                    target.c_str());
        }
        note[sizeof(note) - 1] = 0;
        FILE* f = fopen("system\\GothicCoopOthello.log", "a");
        if (f) {
            SYSTEMTIME now;
            GetLocalTime(&now);
            fprintf(f, "GothicCoop Othello-Fassung %s, Start %04d-%02d-%02d %02d:%02d:%02d\n%s\n", COOP_OTHELLO_FASSUNG,
                now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond, note);
            fclose(f);
        }
        done = 1;
        LeaveCriticalSection(CoopLogArchiveLock());
    }

#if ENGINE == Engine_G1

    // Auch aus den Netzwerk-Faeden aufgerufen (CoopErrorLog) - darum mit Sperre.
    static CRITICAL_SECTION* OthelloCompatLogLock() {
        static CRITICAL_SECTION cs;
        static bool ready = (InitializeCriticalSection(&cs), true);
        (void)ready;
        return &cs;
    }

    static void OthelloCompatLog(const char* fmt, ...) {
        EnterCriticalSection(OthelloCompatLogLock());
        CoopLogArchiveOnce();   // v35: vorige Sitzung zuerst archivieren
        FILE* f = fopen("system\\GothicCoopOthello.log", "a");
        if (f) {
            va_list ap;
            va_start(ap, fmt);
            vfprintf(f, fmt, ap);
            va_end(ap);
            fputc('\n', f);
            fclose(f);
        }
        LeaveCriticalSection(OthelloCompatLogLock());
    }

    // v46: Othellos Absturzbericht (system\Othello_Crash_Reports.txt, bei jedem Absturz ueberschrieben) einmal je neuem Bericht ins
    // GT-Log schreiben - das Log geht per Log-Versand zum Partner, so liegt auch der Bericht des anderen PCs beim Host vor
    // (Test v45: der Client stuerzte ab, sein Bericht lag nur auf seinem PC). Merker: system\GothicCoopCrashSeen.txt (Dateizeit).
    void CoopLogCrashReportOnce() {
        static bool done = false;
        if (done)
            return;
        done = true;
        WIN32_FILE_ATTRIBUTE_DATA fa;
        if (!GetFileAttributesExA("system\\Othello_Crash_Reports.txt", GetFileExInfoStandard, &fa))
            return;
        const unsigned long long t = ((unsigned long long)fa.ftLastWriteTime.dwHighDateTime << 32) | fa.ftLastWriteTime.dwLowDateTime;
        unsigned long long seen = 0;
        if (FILE* f = fopen("system\\GothicCoopCrashSeen.txt", "r")) {
            fscanf(f, "%llu", &seen);
            fclose(f);
        }
        if (t <= seen)
            return;
        std::string body;
        if (FILE* f = fopen("system\\Othello_Crash_Reports.txt", "rb")) {
            char buf[2048];
            size_t got;
            while (body.size() < 8000 && (got = fread(buf, 1, sizeof(buf), f)) > 0)
                body.append(buf, got);
            fclose(f);
        }
        FILETIME lt;
        SYSTEMTIME st = {};
        FileTimeToLocalFileTime(&fa.ftLastWriteTime, &lt);
        FileTimeToSystemTime(&lt, &st);
        OthelloCompatLog("Othello-Absturzbericht vom %02d.%02d.%04d %02d:%02d:%02d (Othello_Crash_Reports.txt, %u Bytes):",
            st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond, (unsigned)body.size());
        size_t a = 0;
        while (a < body.size()) {
            size_t e = body.find('\n', a);
            if (e == std::string::npos)
                e = body.size();
            std::string line = body.substr(a, e - a);
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            if (!line.empty())
                OthelloCompatLog("  AB %s", line.c_str());
            a = e + 1;
        }
        if (FILE* f = fopen("system\\GothicCoopCrashSeen.txt", "w")) {
            fprintf(f, "%llu\n", t);
            fclose(f);
        }
    }

    static bool EngineBytesEqual(size_t addr, const unsigned char* expect, size_t len) {
        if (!IsReadableMemory((const void*)addr, len))
            return false;
        return memcmp((const void*)addr, expect, len) == 0;
    }

    // Original oder Detour (E9 rel32) eines anderen Plugins - beides heisst: die Funktion ist da.
    static bool EngineFunctionPresent(size_t addr, const unsigned char* head, size_t len) {
        if (EngineBytesEqual(addr, head, len))
            return true;
        return IsReadableMemory((const void*)addr, 5) && *(const unsigned char*)addr == 0xE9;
    }

    // Funktionskoepfe aus GothicMod.exe (G1 Classic 1.08k).
    static const unsigned char HEAD_MAKESPELLBOOK[] = { 0x6A, 0xFF, 0x68, 0xCC, 0x0D, 0x7C, 0x00, 0x64 };
    static const unsigned char HEAD_USESTANDAI[]    = { 0x6A, 0xFF, 0x68, 0x2C, 0x0E, 0x7C, 0x00, 0x64 };
    static const unsigned char HEAD_DELETENPC[]     = { 0x55, 0x57, 0x8B, 0xF9, 0x8B, 0x47, 0x08, 0x33,
                                                        0xED, 0x85, 0xC0, 0x7E, 0x69 };
    static const unsigned char HEAD_ALLOC[]         = { 0x8B, 0x44, 0x24, 0x04, 0x50, 0xE8 };  // 0x0054EBE0
    static const unsigned char HEAD_FREE[]          = { 0xE9, 0xBA, 0xC1, 0x20, 0x00 };        // 0x0054EBB0
    static const unsigned char HEAD_OBJCREATED[]    = { 0xA1, 0x2C, 0x8E, 0x85, 0x00, 0x85 };  // 0x0075A250
    static const unsigned char HEAD_AISTAND_CTOR[]  = { 0x6A, 0xFF, 0x68, 0x1B, 0x38, 0x7B };  // 0x006168D0
    static const unsigned char HEAD_SETAI[]         = { 0x56, 0x8B, 0xF1, 0x8B, 0x8E, 0xB0 };  // 0x005D3730
    static const unsigned char HEAD_MAGBOOK_CTOR[]  = { 0x6A, 0xFF, 0x68, 0x8A, 0x8C, 0x79 };  // 0x0046E910
    static const unsigned char HEAD_SETOWNER[]      = { 0x8B, 0x44, 0x24, 0x04, 0x56, 0x8B };  // 0x0046EE50

    // --- oCNpc::MakeSpellBook (0x006B86A0..0x006B8727) ----------------------------------------
    //   if (!mag_book) mag_book = zNEW(oCMag_Book);   mag_book->SetOwner(this);   [+0x7A9] = 1
    // Ausnahmerahmen und Stapelaufbau wie im Original, damit Othellos unveraenderter
    // Handler 0x007C0DCC im Fehlerfall dieselben Slots findet.
    static void __declspec(naked) __fastcall MakeSpellBook_Replica(oCNpc* /*ecx*/) {
        __asm {
            push    -1
            push    0x007C0DCC
            mov     eax, dword ptr fs:[0]
            push    eax
            mov     dword ptr fs:[0], esp
            push    ecx
            push    esi
            mov     esi, ecx
            mov     eax, dword ptr [esi + 0x954]        // mag_book
            test    eax, eax
            jne     msb_have_book
            push    0x3970
            push    0x0084E2EC
            push    0x0084E3E4
            push    0x7C
            mov     eax, 0x0054EBE0                     // Speicher anfordern
            call    eax
            add     esp, 0x10
            mov     dword ptr [esp + 4], eax
            test    eax, eax
            mov     dword ptr [esp + 0x10], 0
            je      msb_no_memory
            mov     ecx, eax
            mov     eax, 0x0046E910                     // oCMag_Book::oCMag_Book
            call    eax
            jmp     msb_store_book
        msb_no_memory:
            xor     eax, eax
        msb_store_book:
            mov     dword ptr [esp + 0x10], 0xFFFFFFFF
            mov     dword ptr [esi + 0x954], eax
        msb_have_book:
            mov     ecx, dword ptr [esi + 0x954]
            push    esi
            mov     eax, 0x0046EE50                     // oCMag_Book::SetOwner
            call    eax
            mov     ecx, dword ptr [esp + 8]
            mov     byte ptr [esi + 0x7A9], 1
            pop     esi
            mov     dword ptr fs:[0], ecx
            add     esp, 0x10
            ret
        }
    }

    // --- oCNpc::UseStandAI (0x006B8B40..0x006B8BE0) -------------------------------------------
    //   ai = zNEW(oCAIHuman_Stand)(this);   SetAI(ai);   zRELEASE(callback_ai)
    static void __declspec(naked) __fastcall UseStandAI_Replica(oCNpc* /*ecx*/) {
        __asm {
            push    -1
            push    0x007C0E2C
            mov     eax, dword ptr fs:[0]
            push    eax
            mov     dword ptr fs:[0], esp
            push    ecx
            push    esi
            push    edi
            push    0x3A15
            push    0x0084E2EC
            push    0x0084FD88                          // "oCAIHuman_Stand(this)"
            push    0x24
            mov     edi, ecx
            mov     eax, 0x0054EBE0                     // Speicher anfordern
            call    eax
            push    0x3A15
            push    0x0084E2EC
            mov     esi, eax
            push    0x008D8788
            push    esi
            mov     eax, 0x0075A250                     // Objekt beim Klassensystem melden
            call    eax
            add     esp, 0x20
            mov     dword ptr [esp + 8], esi
            test    esi, esi
            mov     dword ptr [esp + 0x14], 0
            je      usa_no_ai
            push    edi
            mov     ecx, esi
            mov     eax, 0x006168D0                     // oCAIHuman_Stand::oCAIHuman_Stand(this)
            call    eax
            jmp     usa_set_ai
        usa_no_ai:
            xor     eax, eax
        usa_set_ai:
            push    eax
            mov     ecx, edi
            mov     dword ptr [esp + 0x18], 0xFFFFFFFF
            mov     eax, 0x005D3730                     // zCVob::SetAI
            call    eax
            mov     ecx, dword ptr [edi + 0xB0]         // callback_ai
            mov     edx, dword ptr [ecx + 4]
            dec     edx
            mov     eax, edx
            test    eax, eax
            pop     edi
            mov     dword ptr [ecx + 4], edx
            pop     esi
            jg      usa_done
            mov     eax, dword ptr [ecx]
            push    1
            call    dword ptr [eax + 0x0C]
        usa_done:
            mov     ecx, dword ptr [esp + 4]
            mov     dword ptr fs:[0], ecx
            add     esp, 0x10
            ret
        }
    }

    // --- oCSpawnManager::DeleteNpc (0x006D0DE0..0x006D0E69) -----------------------------------
    // Nur Rueckfall, falls die Engine-eigene Fassung nicht auffindbar ist.
    // __fastcall mit Platzhalter in EDX = __thiscall mit einem Stapelargument; das abschliessende
    // jmp in zCWorld::RemoveVob raeumt es ab (ret 4), genau wie im Original.
    static void __declspec(naked) __fastcall DeleteNpc_Replica(oCSpawnManager* /*ecx*/, int /*edx*/, oCNpc* /*npc*/) {
        __asm {
            push    ebp
            push    edi
            mov     edi, ecx
            mov     eax, dword ptr [edi + 8]            // spawnList.numInArray
            xor     ebp, ebp
            test    eax, eax
            jle     dn_remove_vob
            push    ebx
            push    esi
        dn_next_node:
            mov     eax, dword ptr [edi]
            mov     esi, dword ptr [eax + ebp * 4]
            mov     ecx, dword ptr [esi]
            cmp     ecx, dword ptr [esp + 0x14]
            jne     dn_continue_loop
            test    esi, esi
            je      dn_unlink
            mov     edx, dword ptr [ecx + 4]
            dec     edx
            mov     eax, edx
            test    eax, eax
            mov     dword ptr [ecx + 4], edx
            jg      dn_unlink
            mov     edx, dword ptr [ecx]
            push    1
            call    dword ptr [edx + 0x0C]
        dn_unlink:
            mov     ecx, dword ptr [edi + 8]
            test    ecx, ecx
            je      dn_free_node
            xor     eax, eax
            test    ecx, ecx
            jle     dn_free_node
            mov     ebx, dword ptr [edi]
            mov     edx, ebx
        dn_find_node:
            cmp     dword ptr [edx], esi
            je      dn_found_node
            inc     eax
            add     edx, 4
            cmp     eax, ecx
            jl      dn_find_node
            jmp     dn_free_node
        dn_found_node:
            dec     ecx
            cmp     eax, ecx
            mov     dword ptr [edi + 8], ecx
            je      dn_free_node
            mov     ecx, dword ptr [ebx + ecx * 4]
            mov     dword ptr [ebx + eax * 4], ecx
        dn_free_node:
            push    esi
            mov     eax, 0x0054EBB0                     // Speicher freigeben
            call    eax
            add     esp, 4
            dec     ebp
        dn_continue_loop:
            mov     eax, dword ptr [edi + 8]
            inc     ebp
            cmp     ebp, eax
            jl      dn_next_node
            pop     esi
            pop     ebx
        dn_remove_vob:
            mov     ecx, dword ptr ds:[0x008DA6BC]      // ogame
            mov     edx, dword ptr [ecx]
            call    dword ptr [edx + 0x34]              // GetGameWorld()
            mov     edx, dword ptr [eax]
            pop     edi
            pop     ebp
            mov     ecx, eax
            jmp     dword ptr [edx + 0x30]              // RemoveVob(npc)
        }
    }

    // Sind alle Engine-Helfer, die die Nachbildungen rufen, noch dort, wo sie hingehoeren?
    static bool ReplicaHelpersPresent(const char* what) {
        struct { size_t addr; const unsigned char* head; size_t len; const char* name; } helpers[] = {
            { 0x0054EBE0, HEAD_ALLOC,        sizeof(HEAD_ALLOC),        "Speicher anfordern" },
            { 0x0054EBB0, HEAD_FREE,         sizeof(HEAD_FREE),         "Speicher freigeben" },
            { 0x0075A250, HEAD_OBJCREATED,   sizeof(HEAD_OBJCREATED),   "Objekt melden" },
            { 0x006168D0, HEAD_AISTAND_CTOR, sizeof(HEAD_AISTAND_CTOR), "oCAIHuman_Stand-Konstruktor" },
            { 0x005D3730, HEAD_SETAI,        sizeof(HEAD_SETAI),        "zCVob::SetAI" },
            { 0x0046E910, HEAD_MAGBOOK_CTOR, sizeof(HEAD_MAGBOOK_CTOR), "oCMag_Book-Konstruktor" },
            { 0x0046EE50, HEAD_SETOWNER,     sizeof(HEAD_SETOWNER),     "oCMag_Book::SetOwner" },
        };
        for (auto& h : helpers) {
            if (!EngineFunctionPresent(h.addr, h.head, h.len)) {
                OthelloCompatLog("%s: Engine-Helfer %s bei %08X ist veraendert - Aufruf entfaellt",
                    what, h.name, (unsigned)h.addr);
                return false;
            }
        }
        return true;
    }

    enum EngineCallMode { CALL_ORIGINAL, CALL_REPLICA, CALL_SKIP };

    static EngineCallMode DecideMode(const char* what, size_t addr, const unsigned char* head, size_t len) {
        if (EngineFunctionPresent(addr, head, len)) {
            OthelloCompatLog("%s: Original bei %08X vorhanden", what, (unsigned)addr);
            return CALL_ORIGINAL;
        }
        if (ReplicaHelpersPresent(what)) {
            unsigned char p[4] = { 0, 0, 0, 0 };
            if (IsReadableMemory((const void*)addr, sizeof(p)))
                memcpy(p, (const void*)addr, sizeof(p));
            OthelloCompatLog("%s: %08X ist umgewidmet (%02X %02X %02X %02X) - Nachbildung",
                what, (unsigned)addr, p[0], p[1], p[2], p[3]);
            return CALL_REPLICA;
        }
        return CALL_SKIP;
    }

    void CoopMakeSpellBook(oCNpc* npc) {
        if (!npc)
            return;
        static const EngineCallMode mode = DecideMode("MakeSpellBook", 0x006B86A0,
            HEAD_MAKESPELLBOOK, sizeof(HEAD_MAKESPELLBOOK));
        if (mode == CALL_ORIGINAL)
            npc->MakeSpellBook();
        else if (mode == CALL_REPLICA)
            MakeSpellBook_Replica(npc);
    }

    void CoopUseStandAI(oCNpc* npc) {
        if (!npc)
            return;
        static const EngineCallMode mode = DecideMode("UseStandAI", 0x006B8B40,
            HEAD_USESTANDAI, sizeof(HEAD_USESTANDAI));
        if (mode == CALL_ORIGINAL)
            npc->UseStandAI();
        else if (mode == CALL_REPLICA)
            UseStandAI_Replica(npc);
    }

    // Wo bedient die Engine selbst ihre DeleteNpc-Aufrufe? Erst der Stammplatz, dann die Ziele
    // ihrer beiden Aufrufstellen - Othello hat genau diese auf die verschobene Fassung umgebogen.
    static size_t ResolveEngineDeleteNpc() {
        if (EngineBytesEqual(0x006D0DE0, HEAD_DELETENPC, sizeof(HEAD_DELETENPC)))
            return 0x006D0DE0;
        const size_t callSites[] = { 0x00693AF8, 0x00650AFB };
        for (size_t site : callSites) {
            if (!IsReadableMemory((const void*)site, 5) || *(const unsigned char*)site != 0xE8)
                continue;
            const size_t target = site + 5 + *(const int*)(site + 1);
            if (EngineBytesEqual(target, HEAD_DELETENPC, sizeof(HEAD_DELETENPC)))
                return target;
        }
        return 0;
    }

    void CoopDeleteNpc(oCSpawnManager* spawnman, oCNpc* npc) {
        if (!spawnman || !npc)
            return;
        CoopCamNpcGone(npc);   // v44
        static size_t resolved = (size_t)-1;
        static bool replicaOk = false;
        if (resolved == (size_t)-1) {
            resolved = ResolveEngineDeleteNpc();
            if (resolved == 0x006D0DE0) {
                OthelloCompatLog("DeleteNpc: Original bei 006D0DE0 vorhanden");
            }
            else if (resolved) {
                OthelloCompatLog("DeleteNpc: 006D0DE0 ist umgewidmet, die Engine nutzt %08X - wird mitbenutzt",
                    (unsigned)resolved);
            }
            else {
                replicaOk = ReplicaHelpersPresent("DeleteNpc");
                OthelloCompatLog("DeleteNpc: keine Engine-Fassung gefunden - %s",
                    replicaOk ? "Nachbildung" : "Aufruf entfaellt");
            }
        }

        if (resolved == 0x006D0DE0) {
            spawnman->DeleteNpc(npc);
        }
        else if (resolved) {
            typedef void(__thiscall* DeleteNpcFn)(oCSpawnManager*, oCNpc*);
            ((DeleteNpcFn)resolved)(spawnman, npc);
        }
        else if (replicaOk) {
            DeleteNpc_Replica(spawnman, 0, npc);
        }
    }

    // --- oCAniCtrl_Human::CanParade (0x006284C0) ----------------------------------------------
    //
    // Othello ersetzt die Funktion durch einen Sprung nach OTHELLO_ABI+0xC4610:
    //     B8 01 00 00 00   mov eax, 1
    //     C2 04 00         ret 4
    // Die Engine ruft CanParade nirgends direkt auf (0 Aufrufer, Original wie Othello); Othellos
    // eigene Parier-Logik sitzt im Schadensablauf (u. a. OTHELLO_ABI+0xC4620: Parade-Animation des
    // Verteidigers UND Blickwinkel zum Angreifer). GT fragt auf dem Client bei jedem NPC-Treffer
    // "if (player->GetAnictrl()->CanParade(npc)) break;" - mit dem Platzhalter wurde JEDER Treffer
    // verworfen.
    //
    // v8 nutzte eine Nachbildung des Vanilla-Originals (layer-1-Animation mit "PARADE" im Namen,
    // Parade-Fenster +0x1A8/+0x1AC; unter Othello 0/0 = die ganze Animation). Gemessen: der eine
    // NPC-Treffer des Tests wurde damit als "pariert" verworfen. Grundsaetzlich doppelt: den
    // Treffer erzeugt der HOST, wo die Coop-Figur die Parade-Animationen des Clients gespiegelt
    // abspielt und Othellos Parier-Logik bereits entschieden hat. Eine zweite, groebere und
    // zeitversetzte Pruefung beim Client kann dem nur widersprechen.
    // Darum unter Othello: der Host entscheidet, ein ankommender Treffer gilt als nicht pariert.
    // Ohne Othello bleibt alles wie in GT (Original-Funktion).
    static const unsigned char HEAD_CANPARADE[]  = { 0x8B, 0x44, 0x24, 0x04, 0x85, 0xC0, 0x55, 0x56 };
    static const unsigned char STUB_RETURN_ONE[] = { 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC2, 0x04, 0x00 };

    int CoopCanParade(oCAniCtrl_Human* aniCtrl, oCNpc* attacker) {
        if (!aniCtrl)
            return FALSE;
        enum { CP_UNKNOWN, CP_ORIGINAL, CP_HOST_DECIDES };
        static int mode = CP_UNKNOWN;
        if (mode == CP_UNKNOWN) {
            mode = CP_ORIGINAL;
            const unsigned char* p = (const unsigned char*)0x006284C0;
            if (!EngineBytesEqual(0x006284C0, HEAD_CANPARADE, sizeof(HEAD_CANPARADE)) &&
                IsReadableMemory(p, 5) && p[0] == 0xE9) {
                const size_t target = 0x006284C0 + 5 + *(const int*)(p + 1);
                if (EngineBytesEqual(target, STUB_RETURN_ONE, sizeof(STUB_RETURN_ONE)))
                    mode = CP_HOST_DECIDES;
                OthelloCompatLog("CanParade: Eingang springt nach %08X%s", (unsigned)target,
                    mode == CP_HOST_DECIDES ? " = Platzhalter 'immer 1' - Parieren entscheidet der Host"
                                            : " - wird benutzt");
            }
            else {
                OthelloCompatLog("CanParade: Original bei 006284C0 vorhanden");
            }
        }
        if (mode == CP_HOST_DECIDES)
            return FALSE;
        return aniCtrl->CanParade(attacker);
    }

    // Name der Animation auf Ebene 1 (Grundebene) - fuer das Protokoll ankommender Treffer.
    const char* CoopBaseLayerAni(oCNpc* npc) {
        zCModel* model = npc ? npc->GetModel() : NULL;
        if (!model)
            return "-";
        for (int i = 0; i < model->numActiveAnis && i < zMDL_MAX_ANIS_PARALLEL; i++) {
            zCModelAniActive* a = model->aniChannels[i];
            if (a && a->protoAni && a->protoAni->layer == 1)
                return a->protoAni->aniName.ToChar();
        }
        return "-";
    }

    // --- oCWorld: verschobene Objektlisten ----------------------------------------------------
    //
    // Othello vergroessert oCWorld (oCWorld::_CreateNewInstance ruft OTHELLO_ABI+0x62C90) und legt
    // die drei Objektlisten um:  voblist 0x6268 -> 0x627C,  voblist_npcs 0x626C -> 0x6284,
    // voblist_items 0x6270 -> 0x6288. Alle Engine-Zugriffe darauf (rund 70, in oCWorld, oCGame,
    // oCNpc, oCRtnManager ...) sind umgeschrieben, z. B. in oCWorld::InsertInLists:
    //     006D76A1  8B 8F 68 62 00 00   mov ecx,[edi+6268h]   ->  [edi+627Ch]
    //     006D76F9  8B 8F 6C 62 00 00   mov ecx,[edi+626Ch]   ->  [edi+6284h]
    //     006D7722  8B 8F 70 62 00 00   mov ecx,[edi+6270h]   ->  [edi+6288h]
    // GT las ueber das SDK am alten Ort und fand keinen einzigen NPC: der Host sendete keine
    // NPCs, der Client konnte keine zuordnen. Wir lesen den Offset aus genau diesen Befehlen.
    static size_t EngineWorldListOffset(size_t insnAddr, size_t sdkOffset, const char* what) {
        size_t offset = sdkOffset;
        const unsigned char* p = (const unsigned char*)insnAddr;
        if (IsReadableMemory(p, 6) && p[0] == 0x8B && p[1] == 0x8F) {
            const size_t disp = *(const unsigned int*)(p + 2);
            if (disp >= 0x6200 && disp < 0x6400 && (disp & 3) == 0)
                offset = disp;
        }
        OthelloCompatLog("oCWorld::%s: Offset %04X (SDK %04X)%s", what, (unsigned)offset,
            (unsigned)sdkOffset, offset != sdkOffset ? " - verschoben, Engine-Wert wird benutzt" : "");
        return offset;
    }

    zCListSort<oCNpc>* CoopWorldNpcListFirst(oCWorld* world) {
        if (!world)
            return NULL;
        static const size_t offset = EngineWorldListOffset(0x006D76F9, 0x626C, "voblist_npcs");
        zCListSort<oCNpc>* head = *(zCListSort<oCNpc>**)((unsigned char*)world + offset);
        return head ? head->next : NULL;
    }

    zCListSort<oCItem>* CoopWorldItemListFirst(oCWorld* world) {
        if (!world)
            return NULL;
        static const size_t offset = EngineWorldListOffset(0x006D7722, 0x6270, "voblist_items");
        zCListSort<oCItem>* head = *(zCListSort<oCItem>**)((unsigned char*)world + offset);
        return head ? head->next : NULL;
    }

    // --- oCNpc::ResetPos: vtable-Platz umgewidmet (v23) -----------------------------------------
    //
    // Unter Othello zeigt der vtable-Platz von oCNpc::ResetPos(zVEC3&) (0x007DDFF0, Original 0x0074CED0,
    // ret 4) auf OTHELLO_ABI+0x79360 - eine andere Funktion mit ZWEI Argumenten (endet mit ret 8; beendet
    // Dialoge, Interrupt, versetzt). Ein Aufruf wie das Original raeumt 4 Byte zu viel vom Stack: Absturz
    // (Test v22: Client stuerzte nach dem Abgleich beim Wiederherstellen der eigenen Lage ab). Das Original
    // hat keine direkten Aufrufer mehr.
    // v23 versetzte deshalb nur (Kollision aus, SetPositionWorld, an) - Test v24: Lage stimmte, aber der Spieler
    // konnte sich danach NICHT bewegen. Das Original setzt zusaetzlich die Bewegung zurueck: Interrupt,
    // oCAniCtrl_Human::Reset, Stehanimation, SetActionMode, ClearEM, SetBodyState, SetSleeping(0),
    // SetMovLock(0), Kamera-/Fokusmodus. v25 ruft darum das ORIGINAL 0x0074CED0 direkt auf (__thiscall, ret 4) -
    // nicht ueber den umgewidmeten vtable-Platz. Geprueft: Kopf unveraendert; darin zwei umgeleitete Aufrufe
    // (ResetRotationsWorld -> Engine 0x005EEB3C, SetFocusMode -> OTHELLO_ABI+0x7A9B0) und die virtuellen
    // Aufrufe IsSelfPlayer (+104h, unveraendert) und IsMonster (+10Ch -> OTHELLO_ABI+0x895B0, gleiche
    // Signatur, liest nur das verlegte Gildenfeld +0xA34). Rueckfall, falls der Kopf nicht passt: nur versetzen.
    static const unsigned char HEAD_RESETPOS[] = { 0x64, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x6A, 0xFF, 0x68, 0xD8, 0xE3, 0x7C };

    void CoopResetPosSafe(oCNpc* npc, const zVEC3& pos) {
        if (!npc)
            return;
        static const bool orig = EngineFunctionPresent(0x0074CED0, HEAD_RESETPOS, sizeof(HEAD_RESETPOS));
        static bool logged = false;
        if (!logged) {
            logged = true;
            OthelloCompatLog("ResetPos: %s", orig ? "Original 0x0074CED0 wird direkt aufgerufen (vtable-Platz umgewidmet)"
                                                  : "Original nicht erkannt - nur versetzen (SetPositionWorld)");
        }
        if (orig) {
            typedef void(__thiscall* ResetPosFn)(oCNpc*, zVEC3&);
            zVEC3 p = pos;
            ((ResetPosFn)0x0074CED0)(npc, p);
            return;
        }
        npc->SetCollDet(FALSE);
        npc->SetPositionWorld(pos);
        npc->SetCollDet(TRUE);
    }

#else

    void CoopLogCrashReportOnce() {}

    void CoopResetPosSafe(oCNpc* npc, const zVEC3& pos) {
        if (npc) {
            zVEC3 p = pos;
            npc->ResetPos(p);
        }
    }

    void CoopMakeSpellBook(oCNpc* npc) {
        if (npc)
            npc->MakeSpellBook();
    }

    void CoopUseStandAI(oCNpc* npc) {
        if (npc)
            npc->UseStandAI();
    }

    void CoopDeleteNpc(oCSpawnManager* spawnman, oCNpc* npc) {
        CoopCamNpcGone(npc);   // v44
        if (spawnman && npc)
            spawnman->DeleteNpc(npc);
    }

    zCListSort<oCNpc>* CoopWorldNpcListFirst(oCWorld* world) {
        return (world && world->voblist_npcs) ? world->voblist_npcs->next : NULL;
    }

    int CoopCanParade(oCAniCtrl_Human* aniCtrl, oCNpc* attacker) {
        return aniCtrl ? aniCtrl->CanParade(attacker) : FALSE;
    }

    const char* CoopBaseLayerAni(oCNpc*) {
        return "-";
    }

    zCListSort<oCItem>* CoopWorldItemListFirst(oCWorld* world) {
        return (world && world->voblist_items) ? world->voblist_items->next : NULL;
    }

#endif
}
