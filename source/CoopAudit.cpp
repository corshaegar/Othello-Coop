// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

    // --- Pruefebene 1: Selbstpruefung beim Start ----------------------------------------------
    //
    // Einmal je Start, sobald die Welt geladen ist (alle Plugins haben dann gepatcht):
    //  1. .text von GothicMod.exe im Speicher gegen die Datei auf der Platte - JEDE Aenderung,
    //     egal von wem (Othello, Union, GD3D11, GT selbst), mit Ziel-Modul bei Sprung/Aufruf.
    //  2. Jede Engine-Funktion, die GT fest verdrahtet aufruft (aus GTs eigenem Code gelesen:
    //     mov r32,imm32 / call|jmp r32) und jeder GT-Hook: unveraendert, umgeleitet (wohin),
    //     GT-Hook, PLATZHALTER (Ziel gibt nur eine Konstante zurueck / springt sofort zurueck)
    //     oder ERSETZT (Eingang ueberschrieben, kein Sprung). Bekannte, behandelte Faelle sind
    //     markiert.
    //  3. Die pro Start gewuerfelten bzw. verlegten Felder (oCWorld-Listen, oCNpc::guildTrue).
    // Ausgabe: system\GothicCoopAudit.log (je Start neu), Zusammenfassung + Alarme zusaetzlich
    // in system\GothicCoopOthello.log.
    //
    // v43 (Nutzer: "alle Hooks in einem Testlauf durchgehen, dann sehen wir die veraenderten Funktionen"):
    //  - jede Stelle mit Funktion (CoopHookTable.h aus tools\hooktable.py: Grenzen aller 8329 SDK-Funktionen, Namen der
    //    GT-Haken und der von GT aufgerufenen Funktionen; sonst Anfangsadresse + Abstand),
    //  - alle GT-Haken aus den Quellen (33 statt einer festen Liste mit 12),
    //  - je von GT benutzter Funktion auch die Aenderungen im Rumpf (wer hat hineingepatcht),
    //  - 4. .rdata (vtable-Plaetze und Konstanten; Importtabelle ausgenommen),
    //  - Fingerabdruck je Verursacher-Modul: der Coop-Testlauf (Shift+F11) vergleicht ihn mit dem Partner; weicht er ab,
    //    schreiben beide ihre ganze Liste ins GT-Log (das geht ohnehin zum Partner) - Auswertung mit tools\auditnames.py.

#if ENGINE == Engine_G1

    struct CoopAuditKnown { DWORD addr; const char* note; };
    static const CoopAuditKnown CoopAuditHandled[] = {
        { 0x006B86A0, "MakeSpellBook - Nachbildung (OthelloCompat.cpp)" },
        { 0x006B8B40, "UseStandAI - Nachbildung (OthelloCompat.cpp)" },
        { 0x006D0DE0, "DeleteNpc - Engine-eigene Fassung (OthelloCompat.cpp)" },
        { 0x006284C0, "CanParade - Host entscheidet (OthelloCompat.cpp)" },
        { 0x0068EF00, "CanUse - Coop-Vorbau (OthelloHookOrder.cpp)" },
        { 0x0068DFC0, "GetTrueGuild - von Othello auf das verlegte Feld +0xA34 umgeschrieben (gewollt, v19)" },
        { 0x0068DFD0, "SetTrueGuild - von Othello auf das verlegte Feld +0xA34 umgeschrieben (gewollt, v19)" },
        { 0x00663340, "oCGuilds::SetAttitude - Othello ersetzt nur die Grenzpruefungen (jge +0x7/+0x10 -> nop), Aufruf sicher (v35)" },
    };

    static const char* CoopAuditHandledNote(DWORD a) {
        for (auto& k : CoopAuditHandled)
            if (k.addr == a)
                return k.note;
        return NULL;
    }

    static FILE* CoopAuditFile = NULL;

    static void AuditOut(const char* fmt, ...) {
        if (!CoopAuditFile)
            return;
        va_list ap;
        va_start(ap, fmt);
        vfprintf(CoopAuditFile, fmt, ap);
        va_end(ap);
        fputc('\n', CoopAuditFile);
    }

    static bool AuditReadFile(const char* path, std::vector<unsigned char>& out) {
        HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (h == INVALID_HANDLE_VALUE)
            return false;
        const DWORD size = GetFileSize(h, NULL);
        out.resize(size);
        DWORD got = 0;
        const BOOL ok = size && ReadFile(h, &out[0], size, &got, NULL) && got == size;
        CloseHandle(h);
        return ok != FALSE;
    }

    // Sektion einer geladenen PE-Datei (Speicherabbild) nach Namen.
    static const IMAGE_SECTION_HEADER* AuditSection(const unsigned char* base, const char* name) {
        const IMAGE_DOS_HEADER* dos = (const IMAGE_DOS_HEADER*)base;
        const IMAGE_NT_HEADERS32* nt = (const IMAGE_NT_HEADERS32*)(base + dos->e_lfanew);
        const IMAGE_SECTION_HEADER* s = IMAGE_FIRST_SECTION(nt);
        for (int i = 0; i < nt->FileHeader.NumberOfSections; i++, s++)
            if (strncmp((const char*)s->Name, name, 8) == 0)
                return s;
        return NULL;
    }

    // Gibt die Ziel-Funktion nur eine Konstante zurueck oder kehrt sofort zurueck?
    static bool AuditStubKind(DWORD a, char* out, size_t len) {
        const unsigned char* p = (const unsigned char*)a;
        if (!IsReadableMemory(p, 8))
            return false;
        auto isRet = [](const unsigned char* q) { return q[0] == 0xC3 || (q[0] == 0xC2 && q[2] == 0x00); };
        if (isRet(p)) { _snprintf(out, len, "springt sofort zurueck"); return true; }
        if (p[0] == 0xB8 && isRet(p + 5)) { _snprintf(out, len, "gibt immer %d zurueck", *(const int*)(p + 1)); return true; }
        if ((p[0] == 0x33 || p[0] == 0x31) && p[1] == 0xC0 && isRet(p + 2)) { _snprintf(out, len, "gibt immer 0 zurueck"); return true; }
        if (p[0] == 0xB0 && isRet(p + 2)) { _snprintf(out, len, "gibt immer %d zurueck (8 Bit)", p[1]); return true; }
        return false;
    }

    static DWORD AuditFollowJumps(DWORD a, int* hops) {
        *hops = 0;
        for (int i = 0; i < 4; i++) {
            const unsigned char* p = (const unsigned char*)a;
            if (!IsReadableMemory(p, 6))
                break;
            if (p[0] == 0xE9) a = a + 5 + *(const int*)(p + 1);
            else if (p[0] == 0x68 && p[5] == 0xC3) a = *(const DWORD*)(p + 1);
            else if (p[0] == 0xFF && p[1] == 0x25) {
                const DWORD* slot = *(const DWORD* const*)(p + 2);
                if (!IsReadableMemory(slot, 4)) break;
                a = *slot;
            }
            else break;
            (*hops)++;
        }
        return a;
    }

    static void AuditCollectGtTargets(DWORD textLo, DWORD textHi, std::vector<DWORD>& out) {
        HMODULE self = NULL;
        if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCSTR)&AuditCollectGtTargets, &self) || !self)
            return;
        const unsigned char* base = (const unsigned char*)self;
        const IMAGE_DOS_HEADER* dos = (const IMAGE_DOS_HEADER*)base;
        const IMAGE_NT_HEADERS32* nt = (const IMAGE_NT_HEADERS32*)(base + dos->e_lfanew);
        const IMAGE_SECTION_HEADER* s = IMAGE_FIRST_SECTION(nt);
        for (int i = 0; i < nt->FileHeader.NumberOfSections; i++, s++) {
            if (!(s->Characteristics & IMAGE_SCN_MEM_EXECUTE))
                continue;
            const unsigned char* b = base + s->VirtualAddress;
            const DWORD n = s->Misc.VirtualSize;
            for (DWORD k = 0; k + 7 <= n; k++) {
                if (b[k] < 0xB8 || b[k] > 0xBF || b[k + 5] != 0xFF)
                    continue;
                const unsigned char m = b[k + 6];
                if (!((m >= 0xD0 && m <= 0xD7) || (m >= 0xE0 && m <= 0xE7)) || (m & 7) != (b[k] - 0xB8))
                    continue;
                const DWORD t = *(const DWORD*)(b + k + 1);
                if (t >= textLo && t < textHi)
                    out.push_back(t);
            }
        }
        std::sort(out.begin(), out.end());
        out.erase(std::unique(out.begin(), out.end()), out.end());
    }

    // --- v43: Funktionen zu Adressen (CoopHookTable.h) -------------------------------------------

    static DWORD AuditFnStart(DWORD a) {
        const DWORD* end = CoopFnStarts + sizeof(CoopFnStarts) / sizeof(CoopFnStarts[0]);
        const DWORD* it = std::upper_bound(CoopFnStarts, end, a);
        if (it == CoopFnStarts)
            return 0;
        const DWORD s = *(it - 1);
        return a - s <= 0x2000 ? s : 0;
    }

    // naechster Funktionsanfang nach a (Ende des Rumpfs von a)
    static DWORD AuditFnEnd(DWORD a) {
        const DWORD* end = CoopFnStarts + sizeof(CoopFnStarts) / sizeof(CoopFnStarts[0]);
        const DWORD* it = std::upper_bound(CoopFnStarts, end, a);
        const DWORD e = it != end ? *it : a + 0x400;
        return e - a > 0x2000 ? a + 0x2000 : e;
    }

    static const CoopFnName* AuditFnInfo(DWORD start) {
        const CoopFnName* end = CoopFnNames + sizeof(CoopFnNames) / sizeof(CoopFnNames[0]);
        const CoopFnName* it = std::lower_bound(CoopFnNames, end, start,
            [](const CoopFnName& n, DWORD v) { return n.a < v; });
        return it != end && it->a == start ? it : NULL;
    }

    // "oCNpc::DoDie+0x12" bzw. "006A1234+0x12" (Name nur fuer GT-Haken und von GT aufgerufene Funktionen - die anderen
    // benennt tools\auditnames.py)
    static void AuditLabel(DWORD a, char* out, size_t len) {
        const DWORD s = AuditFnStart(a);
        if (!s) {
            _snprintf(out, len, "?");
        }
        else {
            const CoopFnName* f = AuditFnInfo(s);
            if (f)
                _snprintf(out, len, "%s+0x%X%s", f->n, a - s, f->hook ? " [GT-Haken]" : " [GT ruft auf]");
            else
                _snprintf(out, len, "%08X+0x%X", s, a - s);
        }
        out[len - 1] = 0;
    }

    // --- v43: Abtastung, Fingerabdruck, Liste fuer den Partner -----------------------------------

    struct AuditEntry {
        DWORD va, len, target, off;   // off = Ziel - Modulanfang (0 ohne Modul)
        char kind;                    // 'B' Bytes, 'C' Aufruf, 'J' Sprung, 'R' .rdata-Eintrag
        DWORD oldv, newv;             // nur 'R'
        std::string owner;
    };
    static std::vector<AuditEntry> AuditList;
    static unsigned long AuditHash = 0, AuditRdHash = 0, AuditHooksActive = 0, AuditHooksTotal = 0;
    static std::map<std::string, unsigned long> AuditOwners, AuditRdOwners;
    static bool AuditHave = false, AuditDumped = false;
    static std::string AuditError;

    static unsigned long AuditFnv(unsigned long h, const char* s) {
        for (; *s; s++) {
            h ^= (unsigned char)*s;
            h *= 16777619UL;
        }
        return h;
    }

    // v44: liegt a in ausfuehrbarem Speicher? Sonst ist "E8/E9 + 4 Byte" kein Aufruf/Sprung, sondern ein Wert, den Othello je
    // Start neu setzt (Test v43: CollectInfos+0x19E zeigte beim Client "nach AMDXX32.DLL", beim Host "kein Modul").
    static bool AuditIsCode(DWORD a) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery((const void*)a, &mbi, sizeof(mbi)) != sizeof(mbi) || mbi.State != MEM_COMMIT)
            return false;
        return (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0;
    }

    static void AuditOwnerOf(DWORD target, std::string& owner, DWORD& off) {
        char name[MAX_PATH];
        ModuleName((const void*)target, name, sizeof(name));
        owner = name;
        const void* base = ModuleOf((const void*)target);
        off = (base && strcmp(name, "kein Modul") != 0) ? target - (DWORD)base : 0;
    }

    // Speicher gegen Datei: .text (Bereiche) und .rdata (DWORDs, ohne Importtabelle). false = Datei/Sektion nicht lesbar.
    static bool AuditScan(std::vector<unsigned char>& file, DWORD& textVa, DWORD& textLen) {
        AuditList.clear();
        AuditOwners.clear();
        AuditRdOwners.clear();
        AuditHash = AuditRdHash = 2166136261UL;
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        const unsigned char* image = (const unsigned char*)GetModuleHandleA(NULL);
        const IMAGE_SECTION_HEADER* text = AuditSection(image, ".text");
        if (!text || !AuditReadFile(path, file) || text->PointerToRawData + text->SizeOfRawData > file.size()) {
            AuditError = std::string(path) + " bzw. .text nicht lesbar";
            return false;
        }
        textVa = (DWORD)image + text->VirtualAddress;
        textLen = text->Misc.VirtualSize < text->SizeOfRawData ? text->Misc.VirtualSize : text->SizeOfRawData;
        const unsigned char* mem = (const unsigned char*)textVa;
        const unsigned char* fil = &file[text->PointerToRawData];
        // v44: Fingerabdruck je Funktion (Anfang, Art, Ziel-Modul+Abstand) statt je Bytebereich - Othello wuerfelt verlegte
        // Feld-Abstaende und Trampolin-Adressen je Start neu, die Bereichsgrenzen schwanken dann (Test v43: SetScriptValues
        // +0x51 4 Byte gegen +0x52 3 Byte), obwohl beide PCs dieselben Eingriffe haben
        std::set<std::string> textKeys;
        for (DWORD i = 0; i < textLen;) {
            if (mem[i] == fil[i]) { i++; continue; }
            DWORD j = i, last = i;
            while (j < textLen && j - last <= 4) {
                if (mem[j] != fil[j]) last = j;
                j++;
            }
            AuditEntry e = {};
            e.va = textVa + i;
            e.len = last + 1 - i;
            e.kind = 'B';
            for (DWORD s = e.va >= 4 ? e.va - 4 : e.va; s <= e.va; s++) {
                const unsigned char* p = (const unsigned char*)s;
                if ((p[0] == 0xE8 || p[0] == 0xE9) && s + 5 > e.va && s >= textVa) {
                    const DWORD t = s + 5 + *(const int*)(p + 1);
                    if (!AuditIsCode(t))
                        continue;   // v44: kein Code am Ziel - geaenderter Wert, kein Sprung
                    e.target = t;
                    e.kind = p[0] == 0xE8 ? 'C' : 'J';
                    break;
                }
            }
            if (e.target)
                AuditOwnerOf(e.target, e.owner, e.off);
            else
                e.owner = "(Bytes)";
            AuditOwners[e.owner]++;
            char key[160];
            const DWORD fs = AuditFnStart(e.va);
            if (e.kind == 'B')
                _snprintf(key, sizeof(key), "%08X:B", fs ? fs : e.va);
            else
                _snprintf(key, sizeof(key), "%08X:%c:%s:%X", fs ? fs : e.va, e.kind, e.owner.c_str(),
                    e.owner == "kein Modul" ? 0 : e.off);
            key[sizeof(key) - 1] = 0;
            textKeys.insert(key);
            AuditList.push_back(e);
            i = last + 1;
        }
        for (auto& k : textKeys)
            AuditHash = AuditFnv(AuditFnv(AuditHash, k.c_str()), ";");

        // .rdata: vtable-Plaetze und Konstanten (Importtabelle ausgenommen - die fuellt der Lader)
        const IMAGE_SECTION_HEADER* rd = AuditSection(image, ".rdata");
        if (rd && rd->PointerToRawData + rd->SizeOfRawData <= file.size()) {
            const IMAGE_DOS_HEADER* dos = (const IMAGE_DOS_HEADER*)image;
            const IMAGE_NT_HEADERS32* nt = (const IMAGE_NT_HEADERS32*)(image + dos->e_lfanew);
            const IMAGE_DATA_DIRECTORY& iat = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT];
            const DWORD iatLo = (DWORD)image + iat.VirtualAddress, iatHi = iatLo + iat.Size;
            const DWORD rdVa = (DWORD)image + rd->VirtualAddress;
            const DWORD rdLen = (rd->Misc.VirtualSize < rd->SizeOfRawData ? rd->Misc.VirtualSize : rd->SizeOfRawData) & ~3u;
            const DWORD* rm = (const DWORD*)rdVa;
            const DWORD* rf = (const DWORD*)&file[rd->PointerToRawData];
            for (DWORD k = 0; k < rdLen / 4; k++) {
                const DWORD va = rdVa + k * 4;
                if (rm[k] == rf[k] || (va >= iatLo && va < iatHi))
                    continue;
                AuditEntry e = {};
                e.va = va;
                e.len = 4;
                e.kind = 'R';
                e.oldv = rf[k];
                e.newv = rm[k];
                if (IsReadableMemory((const void*)e.newv, 1))
                    AuditOwnerOf(e.newv, e.owner, e.off);
                else
                    e.owner = "(Wert)";
                AuditRdOwners[e.owner]++;
                char key[160];
                _snprintf(key, sizeof(key), "%08X:%08X:%s:%X;", e.va, e.oldv, e.owner.c_str(), e.off);
                key[sizeof(key) - 1] = 0;
                AuditRdHash = AuditFnv(AuditRdHash, key);
                AuditList.push_back(e);
            }
        }

        // GT-Haken aktiv? (Sprung am Eingang ins GT-Modul, auch ueber Union-Trampoline)
        AuditHooksActive = AuditHooksTotal = 0;
        const void* gtModule = ModuleOf((const void*)&AuditScan);
        for (auto& h : CoopFnNames) {
            if (!h.hook)
                continue;
            AuditHooksTotal++;
            int hops = 0;
            const DWORD dest = AuditFollowJumps(h.a, &hops);
            if (hops > 0 && ModuleOf((const void*)dest) == gtModule)
                AuditHooksActive++;
        }
        AuditHave = true;
        AuditError.clear();
        return true;
    }

    static void AuditEntryLine(const AuditEntry& e, char* out, size_t len) {
        char fn[160];
        AuditLabel(e.va, fn, sizeof(fn));
        if (e.kind == 'R') {
            char was[160];
            AuditLabel(e.oldv, was, sizeof(was));
            _snprintf(out, len, "%08X .rdata %08X -> %08X %s+0x%X (vorher %s)", e.va, e.oldv, e.newv, e.owner.c_str(), e.off,
                AuditFnStart(e.oldv) ? was : "Wert");
        }
        else if (e.kind == 'B')
            _snprintf(out, len, "%08X %4u Bytes  %s", e.va, e.len, fn);
        else
            _snprintf(out, len, "%08X %4u %-6s -> %s+0x%X  %s", e.va, e.len, e.kind == 'C' ? "Aufruf" : "Sprung", e.owner.c_str(),
                e.off, fn);
        out[len - 1] = 0;
    }

    static std::string AuditOwnerList(const std::map<std::string, unsigned long>& m) {
        std::string s;
        for (auto& o : m) {
            char b[96];
            _snprintf(b, sizeof(b), "%s%s %lu", s.empty() ? "" : ", ", o.first.c_str(), o.second);
            b[sizeof(b) - 1] = 0;
            s += b;
        }
        return s;
    }

    // Fuer den Coop-Testlauf: Zusammenfassung (refresh = neu abtasten, z. B. weil Mods spaeter patchen).
    json CoopAuditSummaryJson(bool refresh) {
        json j;
        if (refresh || !AuditHave) {
            std::vector<unsigned char> file;
            DWORD tva = 0, tlen = 0;
            AuditScan(file, tva, tlen);
        }
        j["ok"] = AuditHave && AuditError.empty() ? 1 : 0;
        if (!AuditError.empty())
            j["err"] = AuditError;
        unsigned long nText = 0, nRd = 0;
        for (auto& e : AuditList)
            (e.kind == 'R' ? nRd : nText)++;
        char h[16];
        _snprintf(h, sizeof(h), "%08lX", AuditHash);
        h[sizeof(h) - 1] = 0;
        j["n"] = nText;
        j["h"] = std::string(h);
        _snprintf(h, sizeof(h), "%08lX", AuditRdHash);
        h[sizeof(h) - 1] = 0;
        j["rn"] = nRd;
        j["rh"] = std::string(h);
        j["ha"] = AuditHooksActive;
        j["ht"] = AuditHooksTotal;
        json o = json::object(), ro = json::object();
        for (auto& x : AuditOwners)
            o[x.first] = x.second;
        for (auto& x : AuditRdOwners)
            ro[x.first] = x.second;
        j["o"] = o;
        j["ro"] = ro;
        return j;
    }

    // Ganze Liste ins GT-Log (einmal je Start) - das Log geht zum Partner, beide Listen lassen sich dann vergleichen.
    void CoopAuditDumpToLog(const char* why) {
        if (AuditDumped || !AuditHave)
            return;
        AuditDumped = true;
        OthelloCompatLog("[%lld s, %s] Haken-Liste (%s): %u Eintraege - Spalten Adresse, Laenge, Art, Ziel-Modul+Abstand, Funktion",
            DiagSeconds(), DiagRole(), why, (unsigned)AuditList.size());
        // in einem Zug schreiben (OthelloCompatLog oeffnet die Datei je Zeile - bei ~2100 Zeilen ein spuerbarer Ruckler)
        EnterCriticalSection(OthelloCompatLogLock());
        if (FILE* f = fopen("system\\GothicCoopOthello.log", "a")) {
            for (auto& e : AuditList) {
                char line[400];
                AuditEntryLine(e, line, sizeof(line));
                fprintf(f, "  HL %s\n", line);
            }
            fclose(f);
        }
        LeaveCriticalSection(OthelloCompatLogLock());
        OthelloCompatLog("[%lld s, %s] Haken-Liste Ende", DiagSeconds(), DiagRole());
    }

    void CoopAuditOnce() {
        static bool done = false;
        if (done || !player || !ogame || !ogame->GetGameWorld())
            return;
        done = true;

        CoopLogArchiveOnce();   // v35: altes Audit-Log wird archiviert statt ueberschrieben
        CoopAuditFile = fopen("system\\GothicCoopAudit.log", "w");
        SYSTEMTIME st;
        GetLocalTime(&st);
        AuditOut("Selbstpruefung GothicCoop gegen die Engine, %04d-%02d-%02d %02d:%02d:%02d (Fassung %s)",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, COOP_OTHELLO_FASSUNG);

        // --- 1. Speicher gegen Datei ---------------------------------------------------------
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        std::vector<unsigned char> file;
        DWORD textVa = 0, textLen = 0;
        if (!AuditScan(file, textVa, textLen)) {
            AuditOut("FEHLER: %s - Vergleich entfaellt", AuditError.c_str());
            OthelloCompatLog("Selbstpruefung: %s - Vergleich entfaellt", AuditError.c_str());
            if (CoopAuditFile) fclose(CoopAuditFile);
            CoopAuditFile = NULL;
            return;
        }
        const unsigned char* image = (const unsigned char*)GetModuleHandleA(NULL);
        const IMAGE_SECTION_HEADER* text = AuditSection(image, ".text");
        const unsigned char* fil = &file[text->PointerToRawData];

        unsigned long nText = 0, nRd = 0;
        for (auto& e : AuditList)
            (e.kind == 'R' ? nRd : nText)++;
        AuditOut("\n== 1. Geaenderte Bereiche in %s .text: %lu (Spalten: Adresse, Laenge, Art, Ziel, Funktion)", path, nText);
        for (auto& e : AuditList) {
            if (e.kind == 'R')
                continue;
            char fn[160];
            AuditLabel(e.va, fn, sizeof(fn));
            const unsigned char* m = (const unsigned char*)e.va;
            const unsigned char* f = fil + (e.va - textVa);
            if (e.target)
                AuditOut("%08X %4u %-6s -> %08X %s  | %s", e.va, e.len, e.kind == 'C' ? "Aufruf" : "Sprung", e.target,
                    e.owner.c_str(), fn);
            else
                AuditOut("%08X %4u Bytes  Speicher %02X %02X %02X %02X %02X %02X | Datei %02X %02X %02X %02X %02X %02X  | %s",
                    e.va, e.len, m[0], m[1], m[2], m[3], m[4], m[5], f[0], f[1], f[2], f[3], f[4], f[5], fn);
        }
        AuditOut("Nach Ziel-Modul:");
        for (auto& o : AuditOwners)
            AuditOut("  %6lu  %s", o.second, o.first.c_str());

        // --- 2. Engine-Funktionen, die GT aufruft, und GT-Hooks --------------------------------
        std::vector<DWORD> targets;
        AuditCollectGtTargets(textVa, textVa + textLen, targets);
        for (auto& h : CoopFnNames)   // v43: alle GT-Haken (CoopHookTable.h)
            if (h.hook)
                targets.push_back(h.a);
        std::sort(targets.begin(), targets.end());
        targets.erase(std::unique(targets.begin(), targets.end()), targets.end());

        const void* gtModule = ModuleOf((const void*)&CoopAuditOnce);
        unsigned long nSame = 0, nRedir = 0, nGt = 0, nStub = 0, nRepl = 0, nHandled = 0, nInner = 0, nFnInner = 0;
        std::vector<std::string> alarms;
        AuditOut("\n== 2. Von GT benutzte Engine-Funktionen und GT-Hooks: %u (je Funktion: Eingang, dann Aenderungen im Rumpf)",
            (unsigned)targets.size());
        for (DWORD t : targets) {
            const unsigned char* m = (const unsigned char*)t;
            const unsigned char* f = fil + (t - textVa);
            const CoopFnName* info = AuditFnInfo(t);
            const bool isHook = info && info->hook;
            const char* fname = info ? info->n : "?";
            const char* handled = CoopAuditHandledNote(t);
            char line[512];
            if (memcmp(m, f, 16) == 0) {
                nSame++;
                _snprintf(line, sizeof(line), "%08X %s unveraendert%s", t, fname, isHook ? "  (GT-Hook NICHT aktiv?)" : "");
            }
            else if (m[0] == 0xE9 || (m[0] == 0x68 && m[5] == 0xC3) || (m[0] == 0xFF && m[1] == 0x25)) {
                int hops = 0;
                const DWORD dest = AuditFollowJumps(t, &hops);
                char owner[MAX_PATH], stub[96];
                ModuleName((const void*)dest, owner, sizeof(owner));
                if (ModuleOf((const void*)dest) == gtModule) {
                    nGt++;
                    _snprintf(line, sizeof(line), "%08X %s GT-Hook vorn%s", t, fname, isHook ? "" : "  (?)");
                }
                else if (AuditStubKind(dest, stub, sizeof(stub))) {
                    nStub++;
                    _snprintf(line, sizeof(line), "%08X %s PLATZHALTER: Sprung nach %08X %s - %s%s%s", t, fname, dest, owner, stub,
                        handled ? " | behandelt: " : "", handled ? handled : "");
                }
                else {
                    nRedir++;
                    _snprintf(line, sizeof(line), "%08X %s umgeleitet nach %08X %s (%d Sprung/Spruenge)%s%s%s", t, fname, dest, owner,
                        hops, isHook ? "  FREMDER HOOK VOR GT" : "", handled ? " | behandelt: " : "", handled ? handled : "");
                }
            }
            else {
                nRepl++;
                _snprintf(line, sizeof(line), "%08X %s ERSETZT: Speicher %02X %02X %02X %02X %02X %02X, Datei %02X %02X %02X %02X %02X %02X%s%s",
                    t, fname, m[0], m[1], m[2], m[3], m[4], m[5], f[0], f[1], f[2], f[3], f[4], f[5],
                    handled ? " | behandelt: " : "", handled ? handled : "");
            }
            line[sizeof(line) - 1] = 0;
            AuditOut("  %s", line);
            // v43: Aenderungen im Rumpf (nach den ersten 16 Bytes bis zum naechsten Funktionsanfang)
            const DWORD end = AuditFnEnd(t);
            unsigned long inner = 0;
            for (auto& e : AuditList) {
                if (e.kind == 'R' || e.va < t + 16 || e.va >= end)
                    continue;
                inner++;
                if (e.target)
                    AuditOut("      +0x%X %s -> %s+0x%X", e.va - t, e.kind == 'C' ? "Aufruf" : "Sprung", e.owner.c_str(), e.off);
                else
                    AuditOut("      +0x%X %u Bytes geaendert", e.va - t, e.len);
            }
            if (inner) {
                nInner += inner;
                nFnInner++;
            }
            if (handled)
                nHandled++;
            const bool alarm = (strstr(line, "PLATZHALTER") || strstr(line, "ERSETZT") ||
                strstr(line, "FREMDER HOOK") || strstr(line, "NICHT aktiv")) && !handled;
            if (alarm)
                alarms.push_back(line);
        }

        // --- 3. Verlegte Felder ---------------------------------------------------------------
        AuditOut("\n== 3. Felder, die Othello verlegt (Engine-Befehle lesen)");
        static const struct { DWORD insn; const char* name; } fields[] = {
            { 0x006D76A1, "oCWorld::voblist" }, { 0x006D76F9, "oCWorld::voblist_npcs" },
            { 0x006D7722, "oCWorld::voblist_items" } };
        for (auto& fd : fields) {
            const unsigned char* p = (const unsigned char*)fd.insn;
            const unsigned char* q = fil + (fd.insn - textVa);
            AuditOut("  %-24s Datei +%04X, Speicher +%04X", fd.name, *(const DWORD*)(q + 2), *(const DWORD*)(p + 2));
        }
        {
            const unsigned char* p = (const unsigned char*)0x0068B9B2;
            const unsigned char* q = fil + (0x0068B9B2 - textVa);
            AuditOut("  %-24s Datei +%04X, Speicher +%04X (0x0068B9B2)", "oCNpc::guildTrue",
                *(const DWORD*)(q + 2), *(const DWORD*)(p + 2));
        }

        // --- 4. .rdata (v43): vtable-Plaetze und Konstanten -----------------------------------
        AuditOut("\n== 4. Geaenderte Eintraege in .rdata (ohne Importtabelle): %lu", nRd);
        for (auto& e : AuditList) {
            if (e.kind != 'R')
                continue;
            char line[400];
            AuditEntryLine(e, line, sizeof(line));
            AuditOut("  %s", line);
        }
        AuditOut("Nach Ziel-Modul:");
        for (auto& o : AuditRdOwners)
            AuditOut("  %6lu  %s", o.second, o.first.c_str());
        AuditOut("\nFingerabdruck: .text %08lX, .rdata %08lX; GT-Haken aktiv %lu von %lu", AuditHash, AuditRdHash,
            AuditHooksActive, AuditHooksTotal);

        if (CoopAuditFile) fclose(CoopAuditFile);
        CoopAuditFile = NULL;

        OthelloCompatLog("Selbstpruefung: %lu geaenderte Bereiche in der Engine; GT nutzt %u Engine-Funktionen/Hooks:"
            " unveraendert %lu, umgeleitet %lu, GT-Hook vorn %lu, PLATZHALTER %lu, ERSETZT %lu (behandelt %lu)"
            " - Details system\\GothicCoopAudit.log", nText, (unsigned)targets.size(),
            nSame, nRedir, nGt, nStub, nRepl, nHandled);
        OthelloCompatLog("Selbstpruefung (v43): .text nach Verursacher: %s", AuditOwnerList(AuditOwners).c_str());
        OthelloCompatLog("Selbstpruefung (v43): .rdata %lu Eintraege: %s", nRd, AuditOwnerList(AuditRdOwners).c_str());
        OthelloCompatLog("Selbstpruefung (v43): GT-Haken aktiv %lu von %lu; %lu der von GT benutzten Funktionen haben Aenderungen im"
            " Rumpf (%lu Stellen); Fingerabdruck .text %08lX, .rdata %08lX", AuditHooksActive, AuditHooksTotal, nFnInner, nInner,
            AuditHash, AuditRdHash);
        for (auto& a : alarms)
            OthelloCompatLog("  ALARM (unbehandelt): %s", a.c_str());
        if (alarms.empty())
            OthelloCompatLog("  keine unbehandelten Platzhalter, Ersetzungen oder fremden Hooks vor GT");
    }

#else

    void CoopAuditOnce() {}
    json CoopAuditSummaryJson(bool) { return json::object(); }
    void CoopAuditDumpToLog(const char*) {}

#endif
}
