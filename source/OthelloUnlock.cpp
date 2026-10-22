// Supported with union (c) 2020 Union team
// Union SOURCE file

// --- v79: Othellos Plugin-Sperre zur Laufzeit abschalten (nur Othello 4.4.2.1), 21.10.2026 -------------------------------
//
// Nutzer: "Hilft uns diese Mod (ModRestrictRemove = zNonToxicUnfaker), das Spiel ohne die Modeinschraenkungen zu starten, ohne die
// DLL direkt zu bearbeiten? Koennen wir das in unsere Coop-Mod einbauen?" - Die Mod ist fuer Othello 4.3.5 gebaut (Offsets 0x4BF79,
// 0x5FF8A - hier anderer Code), ihr Weg passt aber: OTHELLO_ABI.DLL im Speicher aendern, sobald sie geladen ist. Bisher (16.10.,
// BEFUND.md) waren 102 bedingte Spruenge direkt in Data\ScriptsOthello4.0_de.vdf geaendert.
//
// Othello 4.4.2.1 (OTHELLO_ABI.orig.dll, Basis 0x10000000, im Spiel verschoben - Speicherabbild 0x70670000) haengt seine Sperre in
// drei Engine-Aufrufe (verschleierte Ziele: Adresse = ~ecx ^ edx, Funktion 0x1008A6F0):
//  - HandledWinMain +0x206 (0x004F4166) und oCGame::Init +0x2B8 (0x00637208, ueber 0x100B0430 -> 0x1000BD90): Verteiler 0x1004C570
//    waehlt per Zufall eine von 100 gleichen Pruef-Varianten (Tabelle 0x100DB278, nur von hier erreichbar) und ruft sie mit
//    "call eax" (0x1004C5F9) - unbekanntes Plugin -> MessageBoxW "Remove the following unsupported plugins" + quick_exit.
//    Abschalten: FF D0 -> 31 C0 (xor eax,eax; alle 100 Varianten auf einmal). Nicht 90 90 wie die Mod bei 4.3.5: an
//    0x004F4166 stand Gothics "call [CreateMenu]" (Othello schreibt dort "call 0x1004C570; nop"), und 0x004F416C legt eax als
//    Menue-Griff nach 0x86F4DC (DeleteMenu-Schleife, DestroyMenu beim Beenden). 0 = "kein Menue" (DestroyMenu wird dann
//    uebersprungen); mit 90 90 stuende dort die Adresse der Pruef-Variante. Der Init-Weg (0x100B0BF1) wertet eax nicht aus.
//  - CGameManager::GameInit +0x191 (0x00425A91 -> 0x10054F30): stille Sabotage - unbekanntes Plugin -> 0x5D in den Epilog von fuenf
//    Engine-Funktionen (0x432EE1, 0x5F36E4, 0x5F7C2A, 0x64025A, 0x640995), spaeter Absturz an scheinbar zufaelliger Stelle.
//    Abschalten: die beiden Whitelist-Abfragen 0x100553CD/0x100558B5 "0F 85 rel32" -> "E9 rel32+1, 90" (Plugin gilt als erlaubt),
//    dieselben Stellen wie der VDF-Patch.
// Jede Stelle wird erst geaendert, wenn der Befehl davor mit der (verschobenen) Adresse uebereinstimmt - eine andere Othello-Fassung
// bleibt unberuehrt. Steht schon die Ersetzung da (VDF noch gepatcht), bleibt es dabei. Eingegriffen wird in DllMain (ist die DLL
// schon geladen) und sonst per LdrRegisterDllNotification beim Laden - beides vor HandledWinMain+0x206, denn Othello prueft dort die
// schon geladenen Plugins. Als Rueckhalt prueft GT nach dem Start die fuenf Engine-Bytes und stellt sabotierte wieder her.
// Nur Windows-Aufrufe, kein Engine- oder Union-Code (laeuft im Lader).

#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

namespace {

    struct UnlockSite {
        DWORD rva;               // Patchstelle
        BYTE size;
        BYTE expect[6];
        BYTE repl[6];
        DWORD sigRva;            // Befehl davor mit Adress-Operand
        BYTE sig[5];
        BYTE sigLen;
        DWORD sigTarget;         // Adresse im Operanden minus Wunschbasis 0x10000000
        const char* what;
    };

    const UnlockSite UnlockSites[] = {
        { 0x4C5F9, 2, { 0xFF, 0xD0 }, { 0x31, 0xC0 }, 0x4C5F2, { 0x8B, 0x04, 0x95 }, 3, 0xDB278,
          "Pruef-Verteiler (Meldung 'unsupported plugins', 100 Varianten)" },
        { 0x553CD, 6, { 0x0F, 0x85, 0xCA, 0x00, 0x00, 0x00 }, { 0xE9, 0xCB, 0x00, 0x00, 0x00, 0x90 }, 0x553C4, { 0x8B, 0x45, 0xE0, 0x3B, 0x05 }, 5,
          0x14FDE8, "Sabotage-Pruefung 1" },
        { 0x558B5, 6, { 0x0F, 0x85, 0xCD, 0x00, 0x00, 0x00 }, { 0xE9, 0xCE, 0x00, 0x00, 0x00, 0x90 }, 0x558AC, { 0x8B, 0x45, 0xE0, 0x3B, 0x05 }, 5,
          0x14FDE8, "Sabotage-Pruefung 2" },
    };

    // GothicMod.exe: Epilog-Bytes, die Othellos Sabotage auf 0x5D (pop ebp) setzt
    struct EpilogByte {
        DWORD va;
        BYTE orig;
    };
    const EpilogByte SabotageBytes[] = {
        { 0x00432EE1, 0x5B }, { 0x005F36E4, 0x5B }, { 0x005F7C2A, 0x5B }, { 0x0064025A, 0x5E }, { 0x00640995, 0x5E },
    };

    char UnlockLog[2048] = "";
    bool UnlockDone = false;
    PVOID UnlockCookie = NULL;

    void UnlockAppend(const char* fmt, ...) {
        char line[300];
        va_list ap;
        va_start(ap, fmt);
        _vsnprintf(line, sizeof(line), fmt, ap);
        va_end(ap);
        line[sizeof(line) - 1] = 0;
        const size_t have = strlen(UnlockLog);
        if (have + strlen(line) + 2 < sizeof(UnlockLog)) {
            strcat(UnlockLog, line);
            strcat(UnlockLog, "\n");
        }
    }

    void UnlockApply(BYTE* base, DWORD imageSize, const char* how) {
        if (UnlockDone || !base)
            return;
        UnlockDone = true;
        UnlockAppend("Othello-Sperre: OTHELLO_ABI.DLL bei %08X (%s)", (unsigned)(UINT_PTR)base, how);
        for (const UnlockSite& s : UnlockSites) {
            if (s.rva + s.size > imageSize || s.sigRva + s.sigLen + 4 > imageSize) {
                UnlockAppend("  %05X %s: ausserhalb der DLL - andere Othello-Fassung, nicht geaendert", (unsigned)s.rva, s.what);
                continue;
            }
            BYTE* p = base + s.rva;
            const BYTE* sig = base + s.sigRva;
            const DWORD want = (DWORD)(UINT_PTR)base + s.sigTarget;
            if (memcmp(sig, s.sig, s.sigLen) != 0 || *(const DWORD*)(sig + s.sigLen) != want) {
                UnlockAppend("  %05X %s: Muster passt nicht - andere Othello-Fassung, nicht geaendert", (unsigned)s.rva, s.what);
                continue;
            }
            if (memcmp(p, s.repl, s.size) == 0) {
                UnlockAppend("  %05X %s: schon abgeschaltet (VDF-Patch)", (unsigned)s.rva, s.what);
                continue;
            }
            if (memcmp(p, s.expect, s.size) != 0) {
                UnlockAppend("  %05X %s: unerwartete Bytes %02X %02X - nicht geaendert", (unsigned)s.rva, s.what, p[0], p[1]);
                continue;
            }
            DWORD old = 0;
            if (!VirtualProtect(p, s.size, PAGE_EXECUTE_READWRITE, &old)) {
                UnlockAppend("  %05X %s: VirtualProtect fehlgeschlagen (%lu)", (unsigned)s.rva, s.what, GetLastError());
                continue;
            }
            memcpy(p, s.repl, s.size);
            VirtualProtect(p, s.size, old, &old);
            FlushInstructionCache(GetCurrentProcess(), p, s.size);
            UnlockAppend("  %05X %s: abgeschaltet", (unsigned)s.rva, s.what);
        }
    }

    DWORD UnlockImageSize(BYTE* base) {
        const IMAGE_DOS_HEADER* dos = (const IMAGE_DOS_HEADER*)base;
        if (dos->e_magic != IMAGE_DOS_SIGNATURE)
            return 0;
        const IMAGE_NT_HEADERS* nt = (const IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
        return nt->Signature == IMAGE_NT_SIGNATURE ? nt->OptionalHeader.SizeOfImage : 0;
    }

    // ntdll: LdrRegisterDllNotification (ab Windows Vista, nicht dokumentiert, Aufbau wie in ntdll)
    struct UnlockUStr {
        USHORT Length, MaximumLength;
        PWSTR Buffer;
    };
    struct UnlockNotifyData {
        ULONG Flags;
        const UnlockUStr* FullDllName;
        const UnlockUStr* BaseDllName;
        PVOID DllBase;
        ULONG SizeOfImage;
    };
    typedef VOID(NTAPI* UnlockNotifyFn)(ULONG reason, const UnlockNotifyData* data, PVOID ctx);
    typedef LONG(NTAPI* UnlockRegisterFn)(ULONG flags, UnlockNotifyFn fn, PVOID ctx, PVOID* cookie);

    VOID NTAPI UnlockNotify(ULONG reason, const UnlockNotifyData* data, PVOID) {
        if (reason != 1 || !data || !data->BaseDllName || !data->BaseDllName->Buffer || UnlockDone)
            return;   // 1 = geladen
        wchar_t name[64] = {};
        const size_t n = data->BaseDllName->Length / sizeof(wchar_t);
        if (n == 0 || n >= 64)
            return;
        memcpy(name, data->BaseDllName->Buffer, n * sizeof(wchar_t));
        if (_wcsicmp(name, L"OTHELLO_ABI.DLL") == 0)
            UnlockApply((BYTE*)data->DllBase, data->SizeOfImage, "beim Laden");
    }
}

// Aus DllMain (DLL_PROCESS_ATTACH)
extern "C" void CoopOthelloUnlockInstall() {
    if (HMODULE m = GetModuleHandleW(L"OTHELLO_ABI.DLL")) {
        UnlockApply((BYTE*)m, UnlockImageSize((BYTE*)m), "schon geladen");
        return;
    }
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    UnlockRegisterFn reg = ntdll ? (UnlockRegisterFn)GetProcAddress(ntdll, "LdrRegisterDllNotification") : NULL;
    if (!reg || reg(0, UnlockNotify, NULL, &UnlockCookie) != 0)
        UnlockAppend("Othello-Sperre: LdrRegisterDllNotification nicht verfuegbar - nicht abgeschaltet");
}

// Fuer das Coop-Log (Game_Init): was beim Start geschah; ohne Othello leer
extern "C" const char* CoopOthelloUnlockLog() {
    if (!UnlockDone && UnlockLog[0] == 0 && !GetModuleHandleW(L"OTHELLO_ABI.DLL"))
        return "";
    if (!UnlockDone && UnlockLog[0] == 0)
        return "Othello-Sperre: OTHELLO_ABI.DLL wurde ohne Benachrichtigung geladen - nicht abgeschaltet";
    return UnlockLog;
}

// Rueckhalt: sabotierte Engine-Bytes (0x5D) wieder herstellen. Rueckgabe: Anzahl wiederhergestellt; text = Bericht (leer = alles gut).
extern "C" int CoopOthelloSabotageRepair(char* text, int size) {
    int fixed = 0, odd = 0;
    char buf[400] = "";
    for (const EpilogByte& e : SabotageBytes) {
        BYTE* p = (BYTE*)(UINT_PTR)e.va;
        if (*p == e.orig)
            continue;
        char part[80];
        if (*p == 0x5D) {
            DWORD old = 0;
            if (VirtualProtect(p, 1, PAGE_EXECUTE_READWRITE, &old)) {
                *p = e.orig;
                VirtualProtect(p, 1, old, &old);
                FlushInstructionCache(GetCurrentProcess(), p, 1);
                fixed++;
                _snprintf(part, sizeof(part), " %08X 5D->%02X", (unsigned)e.va, e.orig);
            }
            else
                _snprintf(part, sizeof(part), " %08X 5D (nicht beschreibbar)", (unsigned)e.va);
        }
        else {
            odd++;
            _snprintf(part, sizeof(part), " %08X %02X (erwartet %02X, nicht angefasst)", (unsigned)e.va, *p, e.orig);
        }
        part[sizeof(part) - 1] = 0;
        if (strlen(buf) + strlen(part) + 1 < sizeof(buf))
            strcat(buf, part);
    }
    if (text && size > 0) {
        if (fixed || odd)
            _snprintf(text, size, "Othello-Sabotage: %d Engine-Bytes wiederhergestellt, %d unbekannt:%s", fixed, odd, buf);
        else
            text[0] = 0;
        text[size - 1] = 0;
    }
    return fixed;
}
