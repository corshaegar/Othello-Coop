// Supported with union (c) 2020 Union team
// Union SOURCE file

#include <set>   // v34: CoopHostActiveNames

namespace GOTHIC_ENGINE {

    // --- Diagnose der NPC-Zuordnung zwischen Host und Client ------------------------------
    //
    // Host und Client bilden die NPC-Namen unabhaengig voneinander (BuildGlobalNpcList). Ein
    // NPC wird nur synchronisiert, wenn beide Seiten fuer ihn denselben Namen erzeugen; Pakete
    // mit einem unbekannten Namen verwirft ProcessCoopPacket kommentarlos.
    // Ausgabe:
    //   system\GothicCoopOthello.log   Zusammenfassung alle 30 s, unbekannte Paket-Namen
    //   system\GothicCoopNpcNames.log  jeder vergebene Name mit Instanz, Wegpunkt, Position

    enum { TRACE_SENT, TRACE_RECV, TRACE_APPLIED, TRACE_SKIPPED, TRACE_KINDS };

#if ENGINE == Engine_G1

    static long long DiagStartMs = 0;
    static long long DiagNextSummaryMs = 0;
    static unsigned long DiagPktPlayer = 0;
    static unsigned long DiagPktKnown = 0;
    static unsigned long DiagPktUnknown = 0;
    static std::map<std::string, unsigned long> DiagUnknownIds;
    static unsigned long DiagNamesLogged = 0;
    static unsigned long DiagListRuns = 0;
    static size_t DiagLastNamedCount = 0;

    static const char* DiagRole() {
        return ServerThread ? "Host" : (ClientThread ? "Client" : "Einzel");
    }

    static long long DiagSeconds() {
        if (!DiagStartMs)
            DiagStartMs = CurrentMs;
        return (CurrentMs - DiagStartMs) / 1000;
    }

    // v50: Diagnosezeilen je Art hoechstens n pro Minute. Vorher n je Sitzung - Test v49: Trefferzeilen endeten beim Host nach
    // 2772 s, die spaeteren Kaempfe standen nicht mehr im Log.
    struct CoopDiagBudget {
        long long windowMs;
        unsigned long used;
    };

    static bool CoopDiagAllow(CoopDiagBudget& b, unsigned long perMinute) {
        if (!b.windowMs || CurrentMs - b.windowMs >= 60000) {
            b.windowMs = CurrentMs;
            b.used = 0;
        }
        return b.used++ < perMinute;
    }

    static void DiagNameLog(const char* fmt, ...) {
        CoopLogArchiveOnce();   // v35
        FILE* f = fopen("system\\GothicCoopNpcNames.log", "a");
        if (!f)
            return;
        va_list ap;
        va_start(ap, fmt);
        vfprintf(f, fmt, ap);
        va_end(ap);
        fputc('\n', f);
        fclose(f);
    }

    // Aus BuildGlobalNpcList: jeder neu vergebene Name.
    void CoopDiagNameAssigned(string uniqueName, oCNpc* npc, bool firstRun) {
        if (!npc || DiagNamesLogged >= 6000)
            return;
        DiagNamesLogged++;
        zVEC3 p = npc->GetPositionWorld();
        DiagNameLog("%lld\t%s\t%s\t%s\twp=%s\t%.0f %.0f %.0f\t%s",
            DiagSeconds(), DiagRole(), zSTRING(uniqueName).ToChar(),
            npc->GetInstanceName().ToChar(), npc->wpname.ToChar(),
            p.n[0], p.n[1], p.n[2], firstRun ? "erster-Lauf" : "spaeter");
    }

    // Aus BuildGlobalNpcList: ein Durchlauf ist fertig.
    void CoopDiagListBuilt(bool firstRun, int worldNpcs, int rtnEntries) {
        DiagListRuns++;
        const size_t named = UniqueNameToNpcList.size();
        if (named == DiagLastNamedCount && DiagListRuns > 3)
            return;
        static unsigned long lines = 0;
        if (lines < 60 || (lines % 100) == 0) {
            OthelloCompatLog("[%lld s, %s] Namensliste Lauf %lu: erster Lauf=%d, NPCs in der Welt=%d,"
                " Tagesablauf-Eintraege=%d, benannt=%u (vorher %u)",
                DiagSeconds(), DiagRole(), DiagListRuns, firstRun ? 1 : 0, worldNpcs, rtnEntries,
                (unsigned)named, (unsigned)DiagLastNamedCount);
        }
        lines++;
        DiagLastNamedCount = named;
    }

    // Aus ProcessCoopPacket: wer hat das Paket geschickt, und kennen wir den Namen?
    // v34: NPC-Namen, fuer die der Host Pakete schickt (gehoeren ihm so) - CoopDialogSync benennt solche nie um
    static std::set<std::string> CoopHostActiveNames;

    void CoopDiagPacket(const std::string& id, bool isPlayer, bool known) {
        if (isPlayer) {
            DiagPktPlayer++;
            return;
        }
        if (known) {
            DiagPktKnown++;
            if (ClientThread && !ServerThread)
                CoopHostActiveNames.insert(id);
            return;
        }
        DiagPktUnknown++;
        unsigned long& n = DiagUnknownIds[id];
        n++;
        if (n == 1 && DiagUnknownIds.size() <= 60) {
            OthelloCompatLog("[%lld s, %s] unbekannter NPC-Name im Paket: %s",
                DiagSeconds(), DiagRole(), id.c_str());
        }
    }

    // --- Treffer-Verfolgung (Host): reagiert der getroffene NPC, und auf wen? -------------------
    // Ein Treffer des Clients kommt als SYNC_ATTACKS beim Host an (RemoteNpc::UpdateAttacks) und
    // wird dort als OnDamage mit der Coop-Figur des Clients als Angreifer ausgefuehrt. Ob der NPC
    // danach angreift (Zustand, Gegner) und sich bewegt, entscheiden Engine und Skripte auf dem Host.
    struct DiagAttackWatch {
        oCNpc* target;
        oCNpc* attacker;
        std::string name;
        long long t0;
        zVEC3 pos0;
        int stage;              // 0 = frei, 1 = 2-s-Pruefung offen, 2 = 6-s-Pruefung offen
    };
    static DiagAttackWatch DiagWatches[8];
    static CoopDiagBudget DiagAttacksLogged = { 0, 0 };   // v50: je Minute

    static void DiagNpcLabel(oCNpc* n, char* out, size_t len) {
        if (!n) {
            strncpy(out, "-", len);
            return;
        }
        _snprintf(out, len, "%s", n->GetObjectName().ToChar());
        out[len - 1] = 0;
    }

    static void DiagAttackState(const char* when, DiagAttackWatch& w) {
        // Nur anfassen, was noch lebt: der Name muss noch auf denselben NPC zeigen (RemoveVob-Hook
        // streicht ihn sonst), der Angreifer muss noch eine Coop-Figur sein.
        if (!UniqueNameToNpcList.count(w.name.c_str()) || UniqueNameToNpcList[w.name.c_str()] != w.target) {
            OthelloCompatLog("[%lld s, %s] Treffer %s %s: NPC nicht mehr in der Welt", DiagSeconds(),
                DiagRole(), w.name.c_str(), when);
            return;
        }
        oCNpc* t = w.target;
        const bool attackerAlive = w.attacker && PlayerNpcs.count(w.attacker) > 0;
        char enemy[64], state[64];
        DiagNpcLabel(t->enemy, enemy, sizeof(enemy));
        _snprintf(state, sizeof(state), "%s", t->state.curState.name.ToChar());
        state[sizeof(state) - 1] = 0;
        zVEC3 p = t->GetPositionWorld();
        const float dx = p.n[0] - w.pos0.n[0], dy = p.n[1] - w.pos0.n[1], dz = p.n[2] - w.pos0.n[2];
        const float moved = sqrtf(dx * dx + dy * dy + dz * dz);
        OthelloCompatLog("[%lld s, %s] Treffer %s %s: Zustand=%s Gegner=%s%s Haltung=%d Waffenmodus=%d"
            " HP=%d bewegt %.0f", DiagSeconds(), DiagRole(), w.name.c_str(), when, state[0] ? state : "-",
            enemy, (t->enemy && t->enemy == w.attacker) ? " (=Coop-Figur)" : (t->enemy == player ? " (=Host-Held)" : ""),
            attackerAlive ? t->GetAttitude(w.attacker) : -1, t->GetWeaponMode(),
            t->GetAttribute(NPC_ATR_HITPOINTS), moved);
    }

    // Aus RemoteNpc::UpdateAttacks, Zweig "Welt-NPC", direkt nach OnDamage.
    void CoopDiagAttack(const std::string& targetName, oCNpc* target, oCNpc* attacker, int attitudeBefore) {
        if (!target || !CoopDiagAllow(DiagAttacksLogged, 40))
            return;
        char att[64];
        DiagNpcLabel(attacker, att, sizeof(att));
        OthelloCompatLog("[%lld s, %s] Treffer %s von %s (Instanz %s): Haltung vorher=%d",
            DiagSeconds(), DiagRole(), targetName.c_str(), att,
            attacker ? attacker->GetInstanceName().ToChar() : "-", attitudeBefore);
        for (auto& w : DiagWatches) {
            if (w.stage == 0 || (w.target == target && w.name == targetName)) {
                w.target = target;
                w.attacker = attacker;
                w.name = targetName;
                w.t0 = CurrentMs;
                w.pos0 = target->GetPositionWorld();
                w.stage = 1;
                DiagAttackState("sofort", w);
                return;
            }
        }
    }

    static void DiagAttackPulse() {
        for (auto& w : DiagWatches) {
            if (w.stage == 1 && CurrentMs >= w.t0 + 2000) {
                DiagAttackState("nach 2 s", w);
                w.stage = 2;
            }
            else if (w.stage == 2 && CurrentMs >= w.t0 + 6000) {
                DiagAttackState("nach 6 s", w);
                w.stage = 0;
            }
        }
    }

    // v52: Netz-Messung je 30 s - Raten aus den ENet-Summen des Netzfadens (Global.cpp), groesste Warteschlangen, Glaetten.
    static void DiagNetSummary() {
        static LONG prevSentB = 0, prevRecvB = 0, prevSentP = 0, prevRecvP = 0;
        static long long prevMs = 0;
        static unsigned long prevSteps = 0, prevSnaps = 0;
        const LONG sb = CoopNetSentBytes, rb = CoopNetRecvBytes, sp = CoopNetSentPkts, rp = CoopNetRecvPkts;
        const float secs = prevMs ? (float)(CurrentMs - prevMs) / 1000.f : 0.f;
        if (secs > 0.f) {
            OthelloCompatLog("[%lld s, %s] Netz: gesendet %.0f Pakete/s (%.1f KB/s), empfangen %.0f Pakete/s (%.1f KB/s) |"
                " warteten zum Senden max %ld, je Durchlauf gesendet max %ld, abgeholt max %ld, im Spiel je Bild max %lu |"
                " Glaetten: %lu Bilder, %lu harte Spruenge | Verbindung: Ping max %ld ms, Verlust max %.1f %%, ENet-Drossel min %ld/32,"
                " unterwegs max %ld KB",
                DiagSeconds(), DiagRole(),
                (float)(unsigned long)(sp - prevSentP) / secs, (float)(unsigned long)(sb - prevSentB) / 1024.f / secs,
                (float)(unsigned long)(rp - prevRecvP) / secs, (float)(unsigned long)(rb - prevRecvB) / 1024.f / secs,
                (long)InterlockedExchange(&CoopNetSendQueueMax, 0), (long)InterlockedExchange(&CoopNetSendBatchMax, 0),
                (long)InterlockedExchange(&CoopNetRecvBatchMax, 0), CoopNetRecvQueueMax,
                CoopSmoothSteps - prevSteps, CoopSmoothSnaps - prevSnaps,
                (long)InterlockedExchange(&CoopNetRttMax, 0), (float)InterlockedExchange(&CoopNetLossMax, 0) * 100.f / 65536.f,   // v71
                (long)InterlockedExchange(&CoopNetThrottleMin, 32), (long)InterlockedExchange(&CoopNetInTransitMax, 0) / 1024);
            CoopNetRecvQueueMax = 0;
        }
        prevSentB = sb; prevRecvB = rb; prevSentP = sp; prevRecvP = rp;
        prevSteps = CoopSmoothSteps; prevSnaps = CoopSmoothSnaps;
        prevMs = CurrentMs;
    }

    void CoopLeadDiag(std::string& out);   // v72: CoopCombatLead.cpp

    // Aus Game_Loop.
    void CoopDiagPulse() {
        if (!ServerThread && !ClientThread)
            return;
        DiagAttackPulse();
        if (CurrentMs < DiagNextSummaryMs)
            return;
        DiagNextSummaryMs = CurrentMs + 30000;
        std::string lead;
        CoopLeadDiag(lead);
        OthelloCompatLog("[%lld s, %s] Stand: gesendet werden %u NPCs, empfangen fuer %u, Namen %u |"
            " Pakete Spieler %lu, NPC bekannt %lu, NPC unbekannt %lu (%u verschiedene) | Pause=%d%s",
            DiagSeconds(), DiagRole(), (unsigned)BroadcastNpcs.size(), (unsigned)SyncNpcs.size(),
            (unsigned)UniqueNameToNpcList.size(), DiagPktPlayer, DiagPktKnown, DiagPktUnknown,
            (unsigned)DiagUnknownIds.size(), IsCoopPaused ? 1 : 0, lead.c_str());
        DiagNetSummary();
    }

    // --- Pruefebene 3: Sync-Zaehler je Update-Typ --------------------------------------------
    // gesendet (LocalNpc::PackUpdate), empfangen (ProcessCoopPacket), angewendet / uebersprungen
    // (RemoteNpc::Update). Bei jedem Testschritt-Marker wird die Differenz protokolliert
    // (CoopTrace.cpp) - so sieht man je Handlung: gesendet? angekommen? angewendet?
    static const int TRACE_TYPES = 64;   // v39: Paket 47-49 (vorher 48); v40: 50 LOBBY
    static unsigned long CoopTraceCounts[TRACE_KINDS][TRACE_TYPES];

    void CoopTraceCount(int kind, int type) {
        if (kind >= 0 && kind < TRACE_KINDS && type >= 0 && type < TRACE_TYPES)
            CoopTraceCounts[kind][type]++;
    }

    static const char* CoopUpdateTypeName(int type) {
        static const char* names[] = {
            "POS", "HEADING", "ANIMATION", "WEAPON_MODE", "INIT_NPC", "DESTROY_NPC", "ATTACKS",
            "ARMOR", "WEAPONS", "HP", "TIME", "HAND", "MAGIC_SETUP", "SPELL_CAST", "REVIVED",
            "PROTECTIONS", "PLAYER_NAME", "PLAYER_DISCONNECT", "TALENTS", "BODYSTATE", "OVERLAYS",
            "DROPITEM", "TAKEITEM", "CHEST_OPEN", "CHEST_CLOSE", "CHEST_STATE", "CHEST_BULK",
            "ITEM_TAKE", "ITEM_GONE", "ITEM_BULK", "WORLD_REQUEST", "NPC_LOOT", "SAVE_REQ", "LOAD_REQ", "SAVE_ACK", "FILE_REQ", "FILE_PART", "FILE_END", "STATE_REQ",
            "DIALOG_START", "DIALOG_LINE", "DIALOG_END", "DIALOG_SKIP", "DIALOG_STATE", "HOST_EVENT", "CLIENT_LOG", "SELFTEST", "RESYNC", "SAVE_CHECK", "SAVE_READY", "LOBBY", "NPC_DEATH", "HIT_RESULT", "COOP_RULES", "NPC_LEAD" };
        return (type >= 0 && type < (int)(sizeof(names) / sizeof(names[0]))) ? names[type] : "?";
    }

    // --- Pruefebene 4: verschluckte Fehler ------------------------------------------------------
    // GT faengt Fehler an zehn Stellen ab (Union-Einstiege, Netzwerk-Faeden) und laeuft still
    // weiter; mit /EHa auch Zugriffsfehler. Hier wird jede Fangstelle mit Meldung und Zustand
    // protokolliert. Auch aus den Netzwerk-Faeden aufgerufen - OthelloCompatLog sperrt.
    static std::map<std::string, unsigned long>* CoopErrorCounts() {
        static std::map<std::string, unsigned long> m;
        return &m;
    }

    void CoopErrorLog(const char* where, const char* what, const char* state) {
        unsigned long n;
        EnterCriticalSection(OthelloCompatLogLock());
        n = ++(*CoopErrorCounts())[where];
        LeaveCriticalSection(OthelloCompatLogLock());
        if (n > 10 && (n % 100) != 0)
            return;
        SYSTEMTIME st;
        GetLocalTime(&st);
        OthelloCompatLog("[%02d:%02d:%02d.%03d] FEHLER abgefangen in %s (#%lu): %s | Zustand: %s",
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, where, n, what ? what : "-",
            state ? state : "-");
    }

    // SaveErrorDetails loest tausende Symbole auf und ist sehr teuer - nur die ersten 3 Male.
    bool CoopErrorSaveDetails() {
        static long calls = 0;
        return InterlockedIncrement(&calls) <= 3;
    }

    // v53: harter Sprung beim Glaetten (RemoteNpc::UpdateNpcPosition) - welcher NPC? Test v52: 709 Spruenge in 30 s, ohne Namen.
    void CoopDiagSnap(const char* name, float err, const zVEC3& target, const zVEC3& shown) {
        static CoopDiagBudget budget = { 0, 0 };
        if (!CoopDiagAllow(budget, 10))
            return;
        OthelloCompatLog("[%lld s, %s] Glaetten: harter Sprung bei %s - %.1f m Abweichung (Ziel %.0f/%.0f/%.0f, war %.0f/%.0f/%.0f)",
            DiagSeconds(), DiagRole(), name ? name : "?", err / 100.f, target.n[0], target.n[1], target.n[2],
            shown.n[0], shown.n[1], shown.n[2]);
    }

    // v53: Animation per Name empfangen, im eigenen Modell aber unbekannt (anderes Overlay/Modell)
    void CoopDiagAniUnknown(const char* name, const char* ani) {
        static CoopDiagBudget budget = { 0, 0 };
        if (!CoopDiagAllow(budget, 10))
            return;
        OthelloCompatLog("[%lld s, %s] Animation %s fuer %s hier unbekannt - nicht gespielt", DiagSeconds(), DiagRole(),
            ani ? ani : "?", name ? name : "?");
    }

#else

    void CoopTraceCount(int, int) {}
    void CoopErrorLog(const char*, const char*, const char*) {}
    bool CoopErrorSaveDetails() { return true; }
    void CoopDiagNameAssigned(string, oCNpc*, bool) {}
    void CoopDiagListBuilt(bool, int, int) {}
    void CoopDiagPacket(const std::string&, bool, bool) {}
    void CoopDiagAttack(const std::string&, oCNpc*, oCNpc*, int) {}
    void CoopDiagPulse() {}
    void CoopDiagSnap(const char*, float, const zVEC3&, const zVEC3&) {}
    void CoopDiagAniUnknown(const char*, const char*) {}

#endif
}
