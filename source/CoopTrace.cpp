// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

    // --- Pruefebene 2 und 3: Schnappschuesse und gefuehrter Testablauf -----------------------
    //
    // Schnappschuss (alle 5 s, sobald verbunden), system\GothicCoopSnapshot.log:
    //   H <ms> <Rolle> <Tag> <hh:mm> <eigene Kennung>                       Kopf
    //   S <ms> <Rolle> <Name> <x> <y> <z> <HP> <HPmax> <Waffenmodus> <tot> <bewusstlos> <KI-Zustand> <Animation Ebene 1>
    // Der Host schreibt alle NPCs, die er sendet, und die Coop-Figuren; der Client alle NPCs, fuer
    // die er Daten empfaengt, und die Figur des Hosts; beide sich selbst unter der eigenen Kennung.
    // Gleicher Name = gleicher NPC (NPC-Namen auf beiden Seiten identisch, siehe CoopDiag.cpp).
    //
    // Testablauf: Taste F7 (Konfiguration "testStepKey") schliesst den aktuellen Schritt ab und
    // zeigt den naechsten im Chat. Je Schritt:
    //   M <ms> <Rolle> <Schritt> <Text>                                     Marker
    //   C <ms> <Rolle> <Schritt> <Art> <Update-Typ> <Anzahl>                Zaehler seit letztem Marker
    //   Z <ms> <Rolle> <Art> <Update-Typ> <Gesamt>                          laufende Zaehler (je Schnappschuss)
    // Auswertung: _othello_backup\compare_sync.py (Host- und Client-Datei nebeneinander).

#if ENGINE == Engine_G1

    static const char* CoopTestSteps[] = {
        "Beide verbunden und in Sichtweite?",
        "Client: einen NPC ansprechen und den Dialog ganz durchspielen.",
        "Client: mit einem Haendler handeln (etwas kaufen oder verkaufen).",
        "Client: Waffe ziehen und einen NPC dreimal treffen.",
        "Client: sich von einem NPC oder Monster treffen lassen.",
        "Client: mit Bogen oder Armbrust auf ein Ziel schiessen.",
        "Client: einen Zauber wirken.",
        "Client: einen Gegenstand fallen lassen. Host: ihn aufheben.",
        "Client: Ruestung und Nahkampfwaffe wechseln.",
        "Client: einen Trank trinken.",
        "Host: ein Monster oder einen NPC toeten. Client: die Leiche pluendern.",
        "Host: schlafen und Zeit vorspulen.",
    };
    static const int CoopTestStepCount = sizeof(CoopTestSteps) / sizeof(CoopTestSteps[0]);
    static int CoopTestStep = -1;                        // -1: noch nicht gestartet
    static unsigned long CoopTraceLast[TRACE_KINDS][TRACE_TYPES];
    static long long CoopNextSnapshotMs = 0;
    static unsigned long CoopSnapshotLines = 0;
    static bool CoopSnapshotSessionWritten = false;

    static void SnapLog(const char* fmt, ...) {
        CoopLogArchiveOnce();   // v35
        FILE* f = fopen("system\\GothicCoopSnapshot.log", "a");
        if (!f)
            return;
        va_list ap;
        va_start(ap, fmt);
        vfprintf(f, fmt, ap);
        va_end(ap);
        fputc('\n', f);
        fclose(f);
        CoopSnapshotLines++;
    }

    static void SnapNpc(long long now, const char* role, const char* name, oCNpc* npc) {
        if (!npc)
            return;
        zVEC3 p = npc->GetPositionWorld();
        const char* state = npc->state.curState.name.ToChar();
        SnapLog("S\t%lld\t%s\t%s\t%.0f\t%.0f\t%.0f\t%d\t%d\t%d\t%d\t%d\t%s\t%s",
            now, role, name, p.n[0], p.n[1], p.n[2],
            npc->GetAttribute(NPC_ATR_HITPOINTS), npc->GetAttribute(NPC_ATR_HITPOINTSMAX),
            npc->GetWeaponMode(), npc->IsDead() ? 1 : 0, npc->IsUnconscious() ? 1 : 0,
            (state && state[0]) ? state : "-", CoopBaseLayerAni(npc));
    }

    static void CoopWriteSnapshot() {
        const long long now = GetCurrentMs();
        const char* role = DiagRole();
        if (!CoopSnapshotSessionWritten) {
            CoopSnapshotSessionWritten = true;
            SYSTEMTIME st;
            GetLocalTime(&st);
            SnapLog("# Sitzung %04d-%02d-%02d %02d:%02d:%02d, Rolle %s, Kennung %s", st.wYear, st.wMonth,
                st.wDay, st.wHour, st.wMinute, st.wSecond, role, zSTRING(MyselfId).ToChar());
        }
        int day = 0, h = 0, m = 0;
        ogame->GetTime(day, h, m);
        SnapLog("H\t%lld\t%s\t%d\t%02d:%02d\t%s", now, role, day, h, m, zSTRING(MyselfId).ToChar());
        if (player)
            SnapNpc(now, role, zSTRING(MyselfId).ToChar(), player);
        if (ServerThread) {
            for (auto& p : BroadcastNpcs)
                if (p.second && !p.second->destroyed)
                    SnapNpc(now, role, zSTRING(p.first).ToChar(), p.second->npc);
            for (auto& p : PlayerNpcs)
                SnapNpc(now, role, zSTRING(p.second).ToChar(), p.first);
        }
        else if (ClientThread) {
            for (auto& p : SyncNpcs)
                if (p.second && !p.second->destroyed)
                    SnapNpc(now, role, zSTRING(p.first).ToChar(), p.second->npc);
        }
        // Laufende Gesamtzaehler je Art und Update-Typ - damit lassen sich fuer jeden Zeitraum
        // (z. B. einen Testschritt, egal auf welchem PC F7 gedrueckt wurde) beide Seiten vergleichen.
        static const char* kinds[] = { "gesendet", "empfangen", "angewendet", "uebersprungen" };
        for (int k = 0; k < TRACE_KINDS; k++)
            for (int t = 0; t < TRACE_TYPES; t++)
                if (CoopTraceCounts[k][t])
                    SnapLog("Z\t%lld\t%s\t%s\t%s\t%lu", now, role, kinds[k], CoopUpdateTypeName(t), CoopTraceCounts[k][t]);
    }

    static void CoopTraceMarker() {
        const long long now = GetCurrentMs();
        const char* role = DiagRole();
        if (CoopTestStep >= 0 && CoopTestStep < CoopTestStepCount) {
            SnapLog("M\t%lld\t%s\t%d\t%s", now, role, CoopTestStep + 1, CoopTestSteps[CoopTestStep]);
            std::string summary;
            static const char* kinds[] = { "gesendet", "empfangen", "angewendet", "uebersprungen" };
            for (int k = 0; k < TRACE_KINDS; k++) {
                std::string part;
                for (int t = 0; t < TRACE_TYPES; t++) {
                    const unsigned long d = CoopTraceCounts[k][t] - CoopTraceLast[k][t];
                    if (!d)
                        continue;
                    SnapLog("C\t%lld\t%s\t%d\t%s\t%s\t%lu", now, role, CoopTestStep + 1, kinds[k],
                        CoopUpdateTypeName(t), d);
                    char item[48];
                    _snprintf(item, sizeof(item), "%s%s %lu", part.empty() ? "" : ", ", CoopUpdateTypeName(t), d);
                    item[sizeof(item) - 1] = 0;
                    part += item;
                }
                if (!part.empty())
                    summary += std::string(summary.empty() ? "" : " | ") + kinds[k] + ": " + part;
            }
            OthelloCompatLog("[%lld s, %s] TEST Schritt %d fertig: %s | %s", DiagSeconds(), role,
                CoopTestStep + 1, CoopTestSteps[CoopTestStep], summary.empty() ? "kein Sync-Verkehr" : summary.c_str());
        }
        memcpy(CoopTraceLast, CoopTraceCounts, sizeof(CoopTraceLast));
        CoopTestStep++;
        char msg[256];
        if (CoopTestStep < CoopTestStepCount)
            _snprintf(msg, sizeof(msg), "[Test] Schritt %d/%d: %s  (danach F7)", CoopTestStep + 1,
                CoopTestStepCount, CoopTestSteps[CoopTestStep]);
        else
            _snprintf(msg, sizeof(msg), "[Test] Testablauf beendet - Spiel schliessen, Logs von beiden PCs schicken.");
        msg[sizeof(msg) - 1] = 0;
        ChatLog(string(msg), zCOLOR(120, 255, 120, 255));
        CoopNextSnapshotMs = 0;                          // sofort einen Schnappschuss
    }

    // Aus Game_Loop.
    void CoopTracePulse() {
        if (!ServerThread && !ClientThread)
            return;
        static int stepKey = ReadConfigKey("testStepKey", "KEY_F7");
        if (zinput->KeyToggled(stepKey))
            CoopTraceMarker();
        if (CurrentMs >= CoopNextSnapshotMs && CoopSnapshotLines < 80000) {
            CoopNextSnapshotMs = CurrentMs + 5000;
            CoopWriteSnapshot();
        }
    }

#else

    void CoopTracePulse() {}

#endif
}
