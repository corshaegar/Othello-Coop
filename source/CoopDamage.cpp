// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

    // --- Schadensmessung fuer die Uebertragung ------------------------------------------------
    //
    // GT ueberträgt einen Treffer mit dem Schaden "HP vorher - HP beim OnDamage_Sound-Hook". Das
    // "vorher" stand in EINER globalen Variablen (LastHpBeforeDamage), gesetzt im OnDamage_Hit-Hook.
    // Unter Othello wird der Schaden in OnDamage_Hit durch eine eigene Funktion abgezogen
    // (0x007377E7: ChangeAttribute -> OTHELLO_ABI+0x7E1E0, mit Mindest- und Ueberschreibwert).
    // Gemessen: Treffer des Clients kamen beim Host mit Schaden 0 an (HP blieb 800), Treffer von
    // NPCs auf den Client ebenso. Zwei Schwachstellen, die wir beide schliessen:
    //   1. Die globale Variable gilt fuer den zuletzt begonnenen Treffer - loest der Ablauf
    //      dazwischen einen weiteren aus, misst GT am falschen Objekt. -> pro Deskriptor merken.
    //   2. Sinken die HP erst nach dem Sound-Hook, misst GT 0. -> einen Durchlauf spaeter erneut
    //      messen und den groesseren Wert uebertragen.

    struct CoopDamageStart {
        const void* desc;
        oCNpc* npc;
        int hp;
    };
    static CoopDamageStart CoopDamageStarts[16];
    static unsigned CoopDamageStartNext = 0;
    static std::vector<PlayerHit> CoopPendingHits;
    static unsigned long CoopDamageFrame = 0;

    // Aus dem OnDamage_Hit-Hook, an der Stelle, an der GT LastHpBeforeDamage setzt.
    void CoopDamageBegin(const void* desc, oCNpc* npc, int hp) {
        CoopDamageStart& s = CoopDamageStarts[CoopDamageStartNext++ % 16];
        s.desc = desc;
        s.npc = npc;
        s.hp = hp;
    }

    // Aus dem OnDamage_Sound-Hook: HP vor genau diesem Treffer (neuester passender Eintrag).
    int CoopDamageHpBefore(const void* desc, oCNpc* npc, int fallback) {
        for (unsigned k = 0; k < 16; k++) {
            const CoopDamageStart& s = CoopDamageStarts[(CoopDamageStartNext - 1 - k) % 16];
            if (s.desc == desc && s.npc == npc)
                return s.hp;
        }
        return fallback;
    }

    // Statt ReadyToSyncDamages.enqueue: erst im naechsten Durchlauf endgueltig messen.
    void CoopDamageDefer(PlayerHit hit) {
        hit.frame = CoopDamageFrame;
        CoopPendingHits.push_back(hit);
    }

    // v98 (A16): Ladebeginn (Plugin.cpp) - Treffer des letzten Bilds vor dem Laden nicht in der neuen Welt messen
    void CoopDamageWorldGone() {
        CoopPendingHits.clear();
        memset(CoopDamageStarts, 0, sizeof(CoopDamageStarts));
    }

    static bool CoopDamageNpcAlive(oCNpc* n) {
        return n && (n == player || PlayerNpcs.count(n) > 0 || NpcToUniqueNameList.count(n) > 0);
    }

    static void CoopDamageLabel(oCNpc* n, char* out, size_t len) {
        if (!n) { strncpy(out, "-", len); return; }
        if (n == player) { strncpy(out, "eigener Held", len); return; }
        _snprintf(out, len, "%s", n->GetObjectName().ToChar());
        out[len - 1] = 0;
    }

    // Aus DamageProcessorLoop, vor der eigentlichen Verarbeitung.
    void CoopDamageFinalizePending() {
        CoopDamageFrame++;
        for (size_t i = 0; i < CoopPendingHits.size();) {
            PlayerHit h = CoopPendingHits[i];
            if (h.frame + 1 >= CoopDamageFrame) {      // im selben Durchlauf entstanden: warten
                i++;
                continue;
            }
            CoopPendingHits.erase(CoopPendingHits.begin() + i);

            int hpNow = -1;
            if (CoopDamageNpcAlive(h.npc)) {
                hpNow = h.npc->GetAttribute(NPC_ATR_HITPOINTS);
                const int later = h.hpBefore - hpNow;
                if (later > h.damage)
                    h.damage = (float)later;
                if (h.npc->IsDead())
                    h.isDead = true;
                if (h.npc->IsUnconscious())
                    h.isUnconscious = 1;
            }
            if (h.damage < 0)
                h.damage = 0;

#if ENGINE == Engine_G1
            const bool coop = h.attacker == player || h.npc == player ||
                PlayerNpcs.count(h.attacker) > 0 || PlayerNpcs.count(h.npc) > 0;
            static CoopDiagBudget logged = { 0, 0 };   // v50: je Minute statt je Sitzung
            if (coop && CoopDiagAllow(logged, 40)) {
                char victim[64], attacker[64];
                CoopDamageLabel(h.npc, victim, sizeof(victim));
                CoopDamageLabel(h.attacker, attacker, sizeof(attacker));
                OthelloCompatLog("[%lld s, %s] Schaden gemessen: %s trifft %s | HP vorher %d, beim Sound %d,"
                    " danach %d -> uebertragen %.0f | total %.0f effektiv %.0f real %.0f Modus %lu tot %d bewusstlos %d",
                    DiagSeconds(), DiagRole(), attacker, victim, h.hpBefore, h.hpAtSound, hpNow, h.damage,
                    h.dmgTotal, h.dmgEffective, h.dmgReal, h.damageMode, h.isDead ? 1 : 0, h.isUnconscious);
            }
#endif
            ReadyToSyncDamages.enqueue(h);
        }
    }

    // Aus RemoteNpc::UpdateAttacks: ein Treffer auf eine Coop-Figur kommt an - bevor GT entscheidet.
    void CoopDamageIncoming(const char* attacker, const char* target, const char* myselfId, float damage,
                            int isUnconscious, bool stillAlive, const char* myAni) {
#if ENGINE == Engine_G1
        static CoopDiagBudget logged = { 0, 0 };   // v50: je Minute statt je Sitzung
        if (!CoopDiagAllow(logged, 40))
            return;
        OthelloCompatLog("[%lld s, %s] Treffer kommt an: %s trifft %s (ich bin %s%s) | Schaden %.0f bewusstlos %d tot %d"
            " | meine Animation: %s",
            DiagSeconds(), DiagRole(), attacker, target, myselfId,
            strcmp(target, myselfId) == 0 ? ", passt" : ", passt NICHT", damage, isUnconscious, stillAlive ? 0 : 1,
            myAni ? myAni : "-");
#endif
    }

    // Aus RemoteNpc::UpdateAttacks: was kommt beim Empfaenger an?
    void CoopDamageReceived(const char* attacker, const char* victim, float damage, int hpBefore, int hpAfter) {
#if ENGINE == Engine_G1
        static CoopDiagBudget logged = { 0, 0 };   // v50: je Minute statt je Sitzung
        if (!CoopDiagAllow(logged, 40))
            return;
        OthelloCompatLog("[%lld s, %s] Schaden empfangen: %s trifft %s | Schaden %.0f, HP %d -> %d",
            DiagSeconds(), DiagRole(), attacker, victim, damage, hpBefore, hpAfter);
#endif
    }

    // v41: > 0, solange der Client einen Tod nachspielt, den der Host schon ausgefuehrt hat (ZS_Dead laeuft dann hier noch
    // einmal). Spawns/Trigger aus diesem Skript schickt D13 (CoopDialogSync.cpp) dann nicht noch einmal an den Host.
    int CoopReplayDeathDepth = 0;

    // --- v40 (C8): Treffer aus dem Spiel des anderen nachspielen - Anzeige = echter Schaden ---------------------------------
    //
    // Nutzer (nach v36 weiter, Host und Client): "Wenn der Mitspieler Schaden macht, tauchen die grossen gruenen Zahlen auf statt
    // des eigentlichen Schadens." Ursache: RemoteNpc::UpdateAttacks spielt einen fremden Treffer mit EM->OnDamage nach (Reaktion,
    // Ton, Animation) und setzte vorher nur die HP auf 999999, damit der Treffer hier nicht toetet. oCNpc::ChangeAttribute
    // (0x0068D7C0) kappt die HP beim Abzug aber auf das Maximum (0x0068D831: HP > HPMAX -> HP = HPMAX). Die Zahl zeichnet
    // zUtilities (DamagePopupMode, Farbe nach Schadensart): sein Haken auf oCNpc::OnDamage_Hit (ZUTILITIES+0x22550, liegt unter
    // dem GT-Haken) zeigt "HP vorher - HP nachher" = 999999 - Max-HP. v36 hatte nur das Zuruecksetzen danach beschleunigt.
    // Ausserdem rechnet die Engine mit der Figur des Mitspielers (Staerke/Geschick 1337, RemoteNpc.cpp) - nie der echte Schaden.
    // Jetzt: HP und Maximum hoch, und der erste HP-Abzug am Ziel waehrend des Nachspielens wird durch den Schaden aus dem Paket
    // ersetzt. Othellos Abzug in OnDamage_Hit+0x63D7 (OTHELLO_ABI+0x7E1E0) ruft ChangeAttribute ueber den Eingang
    // ("mov eax, 0x68d7c0; call eax"), der Haken greift also. zUtilities zeigt damit genau den Schaden des Angreifers.
    // ChangeAttribute ist von Othello und den anderen Plugins unberuehrt (Speicherabbild), nicht virtuell.
#if ENGINE == Engine_G1
    static oCNpc* CoopReplayTarget = NULL;
    static int CoopReplayDamage = 0;
    static bool CoopReplayApplied = false;

    // v67 (Schwierigkeitsgrad, CoopDifficulty.cpp): waehrend oCNpc::OnDamage_Hit jeden HP-Abzug am Ziel mit dem Faktor
    // skalieren (gerundet, mindestens 1). Tod/Bewusstlos entscheidet danach die Engine mit den echten HP.
    static oCNpc* CoopScaleTarget = NULL;
    static float CoopScaleFactor = 1.f;

    void CoopScaleBegin(oCNpc* target, float factor) {
        CoopScaleTarget = (factor > 0.f && (factor < 0.999f || factor > 1.001f)) ? target : NULL;
        CoopScaleFactor = factor;
    }

    void CoopScaleEnd() {
        CoopScaleTarget = NULL;
        CoopScaleFactor = 1.f;
    }

    void __fastcall oCNpc_ChangeAttribute_Coop(oCNpc* _this, void* vtable, int nr, int value);
    CInvoke<void(__thiscall*)(oCNpc*, int, int)> Ivk_oCNpc_ChangeAttribute_Coop(0x0068D7C0, &oCNpc_ChangeAttribute_Coop);
    void __fastcall oCNpc_ChangeAttribute_Coop(oCNpc* _this, void* vtable, int nr, int value) {
        if (CoopReplayTarget && _this == CoopReplayTarget && nr == NPC_ATR_HITPOINTS && value < 0 && !CoopReplayApplied) {
            CoopReplayApplied = true;
            value = -CoopReplayDamage;
        }
        else if (CoopScaleTarget && _this == CoopScaleTarget && nr == NPC_ATR_HITPOINTS && value < 0) {
            int v = (int)floorf(-value * CoopScaleFactor + 0.5f);
            if (v < 1)
                v = 1;
            static CoopDiagBudget logged = { 0, 0 };
            if (CoopDiagAllow(logged, 20))
                OthelloCompatLog("[%lld s, %s] Schwierigkeit: Treffer an %s - HP-Abzug %d x %.2f = %d", DiagSeconds(), DiagRole(),
                    _this == player ? "eigenem Helden" : _this->GetObjectName().ToChar(), -value, CoopScaleFactor, v);
            value = -v;
        }
        Ivk_oCNpc_ChangeAttribute_Coop(_this, nr, value);
    }

    // Erster HP-Abzug an target bis CoopReplayEnd = genau damage.
    void CoopReplayBegin(oCNpc* target, float damage) {
        CoopReplayTarget = target;
        CoopReplayDamage = damage > 0 ? (int)(damage + 0.5f) : 0;
        CoopReplayApplied = false;
    }

    void CoopReplayEnd() {
        CoopReplayTarget = NULL;
    }

    // Treffer am Ziel nachspielen; die HP setzt danach der Aufrufer (Host-Wert bzw. Schaden aus dem Paket).
    void CoopReplayHit(oCNpc* target, oCNpc* attacker, float damage, float dmgArg, int damageMode) {
        static CoopDiagBudget logged = { 0, 0 };   // v50: je Minute statt je Sitzung
        const int hpMax = target->GetAttribute(NPC_ATR_HITPOINTSMAX);
        target->SetAttribute(NPC_ATR_HITPOINTSMAX, 999999);
        target->SetAttribute(NPC_ATR_HITPOINTS, 999999);
        CoopReplayBegin(target, damage);
        target->GetEM(false)->OnDamage(target, attacker, dmgArg, damageMode, target->GetPositionWorld());
        const bool applied = CoopReplayApplied;
        const int hpAfter = target->GetAttribute(NPC_ATR_HITPOINTS);
        CoopReplayEnd();
        target->SetAttribute(NPC_ATR_HITPOINTSMAX, hpMax);
        if (CoopDiagAllow(logged, 40)) {
            char a[64], t[64];
            CoopDamageLabel(attacker, a, sizeof(a));
            CoopDamageLabel(target, t, sizeof(t));
            OthelloCompatLog("[%lld s, %s] Treffer nachgespielt: %s -> %s, Schaden %.0f, %s (Anzeige %d)", DiagSeconds(), DiagRole(),
                a, t, damage, applied ? "Abzug ersetzt" : "kein HP-Abzug in OnDamage", 999999 - hpAfter);
        }
    }
#else
    void CoopScaleBegin(oCNpc*, float) {}
    void CoopScaleEnd() {}
    void CoopReplayBegin(oCNpc*, float) {}
    void CoopReplayEnd() {}
    void CoopReplayHit(oCNpc* target, oCNpc* attacker, float damage, float dmgArg, int damageMode) {
        target->SetAttribute(NPC_ATR_HITPOINTS, 999999);
        target->GetEM(false)->OnDamage(target, attacker, dmgArg, damageMode, target->GetPositionWorld());
    }
#endif
}
