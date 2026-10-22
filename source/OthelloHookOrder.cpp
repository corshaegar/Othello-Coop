// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

    // --- Reihenfolge auf Engine-Funktionen, die GT und ein Mod beide hooken ---------------------
    //
    // GT haengt sich per Union (CInvoke, Hooks.cpp) an 12 Engine-Funktionen. Othello
    // (OTHELLO_ABI.DLL) ersetzt eine davon vollstaendig: oCNpc::CanUse (0x0068EF00) springt nach
    // OTHELLO_ABI+0x78560, eine eigene Neuimplementierung, die nichts weiterreicht. Othello patcht
    // nach GT, GTs Hook ("Coop-Spieler duerfen alles benutzen") lief damit nie.
    //
    // Abhilfe: ein Vorbau vor dem, was gerade am Eingang steht. Coop-Spieler -> TRUE, alle
    // anderen laufen unveraendert in die vorhandene Kette, Othellos Regeln bleiben also erhalten.
    // Fuer die uebrigen GT-Hooks wird nur geprueft und protokolliert, ob ein fremder Hook vorn steht.

#if ENGINE == Engine_G1

#include "CoopHookTable.h"   // v43: alle GT-Haken + Funktionsgrenzen (tools\hooktable.py), auch fuer CoopAudit.cpp

    static size_t CanUseNext = 0;

    static int __fastcall CanUse_Front(oCNpc* npc, void* /*edx*/, oCItem* item) {
        if (npc && IsCoopPlayer(npc->GetObjectName()))
            return TRUE;
        typedef int(__thiscall* CanUseFn)(oCNpc*, oCItem*);
        return ((CanUseFn)CanUseNext)(npc, item);
    }

    static const void* ModuleOf(const void* p) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(p, &mbi, sizeof(mbi)) != sizeof(mbi))
            return NULL;
        return mbi.AllocationBase;
    }

    static void ModuleName(const void* p, char* out, size_t len) {
        out[0] = 0;
        HMODULE m = NULL;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCSTR)p, &m) && m) {
            char path[MAX_PATH];
            if (GetModuleFileNameA(m, path, MAX_PATH)) {
                const char* s = strrchr(path, '\\');
                strncpy(out, s ? s + 1 : path, len - 1);
                out[len - 1] = 0;
                return;
            }
        }
        strncpy(out, "kein Modul", len - 1);
        out[len - 1] = 0;
    }

    // Ziel eines E9 am Funktionseingang, sonst 0.
    static size_t EntryJumpTarget(size_t entry) {
        const unsigned char* p = (const unsigned char*)entry;
        if (!IsReadableMemory(p, 5) || p[0] != 0xE9)
            return 0;
        return entry + 5 + *(const int*)(p + 1);
    }

    static void EnsureCanUseFront() {
        const size_t entry = 0x0068EF00;
        const size_t target = EntryJumpTarget(entry);
        if (!target || target == (size_t)&CanUse_Front)
            return;
        if (ModuleOf((const void*)target) == ModuleOf((const void*)&CanUse_Front))
            return;                                     // GTs eigener Hook steht vorn

        char who[MAX_PATH];
        ModuleName((const void*)target, who, sizeof(who));
        CanUseNext = target;
        unsigned char* p = (unsigned char*)entry;
        DWORD old = 0;
        if (!VirtualProtect(p, 5, PAGE_EXECUTE_READWRITE, &old)) {
            OthelloCompatLog("oCNpc::CanUse: fremder Hook vorn (%s), VirtualProtect fehlgeschlagen", who);
            return;
        }
        *(int*)(p + 1) = (int)((size_t)&CanUse_Front - (entry + 5));
        VirtualProtect(p, 5, old, &old);
        FlushInstructionCache(GetCurrentProcess(), p, 5);
        OthelloCompatLog("oCNpc::CanUse: %08X (%s) stand vorn und reicht nicht weiter - Coop-Vorbau davor gesetzt",
            (unsigned)target, who);
    }

    static void ReportForeignHooks() {
        // v43: alle GT-Haken aus CoopHookTable.h (vorher eine feste Liste mit 12 von inzwischen 33)
        const void* gt = ModuleOf((const void*)&CanUse_Front);
        int foreign = 0, total = 0;
        for (auto& h : CoopFnNames) {
            if (!h.hook)
                continue;
            total++;
            const size_t target = EntryJumpTarget(h.a);
            if (!target) {
                OthelloCompatLog("Hook %s (%08X): kein Sprung am Eingang - GT-Hook nicht aktiv?",
                    h.n, (unsigned)h.a);
                foreign++;
                continue;
            }
            if (target == (size_t)&CanUse_Front || ModuleOf((const void*)target) == gt)
                continue;
            char who[MAX_PATH];
            ModuleName((const void*)target, who, sizeof(who));
            OthelloCompatLog("Hook %s (%08X): vorn steht %s (%08X), nicht GT", h.n, (unsigned)h.a,
                who, (unsigned)target);
            foreign++;
        }
        OthelloCompatLog("GT-Hooks geprueft: %d von %d nicht vorn", foreign, total);
    }

    // Aus Game_Loop: alle 5 s pruefen (Mods koennen spaet patchen), Bericht einmal.
    void CoopEnsureHookOrder() {
        static long long next = 0;
        static bool reported = false;
        if (CurrentMs < next)
            return;
        next = CurrentMs + 5000;
        EnsureCanUseFront();
        if (!reported) {
            reported = true;
            ReportForeignHooks();
        }
    }

#else

    void CoopEnsureHookOrder() {}

#endif
}
