// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

    // --- Diagnose auf dem Client: blockierte NPC-Animationen und Dialogzustand ----------------
    //
    // Auf dem Client startet GT keine eigene Animation eines NPCs (zCModel::StartAni-Hook), nur
    // die vom Host gesendeten. Ausnahmen: Dialogpartner, Tote, ein paar Lauf-/Drehanimationen des
    // anvisierten NPCs. Beobachtet: NPCs bewegen sich nach einem Treffer des Clients nicht,
    // Dialoge haengen. Hier wird protokolliert, was fuer den anvisierten/angesprochenen/
    // getroffenen NPC blockiert wird und wie Kamera und Dialogverwaltung dabei stehen.

#if ENGINE == Engine_G1

    static unsigned long CDiagAniLines = 0;
    static std::map<std::string, unsigned long> CDiagAniSeen;
    static oCNpc* CDiagWatchNpc = NULL;
    static long long CDiagWatchT0 = 0;
    static zVEC3 CDiagWatchPos;
    static int CDiagWatchStep = 0;
    static unsigned long CDiagWatchBlocked = 0;
    static long long CDiagNextDialogCheck = 0;
    static std::string CDiagLastDialogState;
    static unsigned long CDiagDialogLines = 0;

    static void CDiagName(oCNpc* n, char* out, size_t len) {
        if (!n) { strncpy(out, "-", len); return; }
        if (n == player) { strncpy(out, "eigener Held", len); return; }
        _snprintf(out, len, "%s", n->GetObjectName().ToChar());
        out[len - 1] = 0;
    }

    // Aus dem zCModel::StartAni-Hook, wenn GT die Animation NICHT startet.
    void CoopDiagAniBlocked(zCVob* vob, zCModelAni* ani) {
        if (!ClientThread || !player || !vob || vob->GetCharacterClass() != 2)
            return;
        oCNpc* npc = (oCNpc*)vob;
        if (npc == CDiagWatchNpc)
            CDiagWatchBlocked++;
        const bool focus = npc == player->GetFocusNpc();
        const bool dialog = CoopNpcInDialogWithPlayer(npc);
        if (!(npc == CDiagWatchNpc || focus || dialog) || CDiagAniLines >= 80)
            return;
        char name[64];
        CDiagName(npc, name, sizeof(name));
        std::string key = std::string(name) + "|" + (ani ? ani->aniName.ToChar() : "?");
        if (CDiagAniSeen[key]++ > 0)
            return;
        CDiagAniLines++;
        OthelloCompatLog("[%lld s, %s] Animation blockiert: %s %s | Fokus=%d Dialog=%d Kamera=%s Zustand=%s",
            DiagSeconds(), DiagRole(), name, ani ? ani->aniName.ToChar() : "?", focus ? 1 : 0, dialog ? 1 : 0,
            ogame->GetCameraAI()->GetMode().ToChar(), npc->state.curState.name.ToChar());
    }

    // Aus RemoteNpc::Update: Host-Daten fuer diesen NPC wurden wegen eines Dialogs zurueckgestellt.
    void CoopDiagUpdateSkipped(const char* name, int pending) {
        static std::string last;
        static unsigned long count = 0, lines = 0;
        count++;
        if (lines >= 40 || (last == name && (count % 200) != 0))
            return;
        last = name;
        lines++;
        oCInformationManager& im = oCInformationManager::GetInformationManager();
        OthelloCompatLog("[%lld s, %s] Host-Daten fuer %s zurueckgestellt (Dialog), wartend %d | Kamera=%s IsDone=%d",
            DiagSeconds(), DiagRole(), name, pending, ogame->GetCameraAI()->GetMode().ToChar(), im.IsDone);
    }

    // Aus DamageProcessorLoop: der Client-Spieler hat diesen NPC getroffen.
    void CoopDiagClientHit(oCNpc* npc) {
        if (!ClientThread || !npc)
            return;
        CDiagWatchNpc = npc;
        CDiagWatchT0 = CurrentMs;
        CDiagWatchPos = npc->GetPositionWorld();
        CDiagWatchStep = 0;
        CDiagWatchBlocked = 0;
    }

    // Aus Game_Loop.
    void CoopDiagClientPulse() {
        if (!ClientThread || !player)
            return;

        if (CDiagWatchNpc) {
            static const int marks[] = { 1000, 2000, 4000, 8000 };
            if (CDiagWatchStep < 4 && CurrentMs >= CDiagWatchT0 + marks[CDiagWatchStep]) {
                char name[64];
                if (NpcToUniqueNameList.count(CDiagWatchNpc)) {
                    oCNpc* n = CDiagWatchNpc;
                    CDiagName(n, name, sizeof(name));
                    char enemy[64];
                    CDiagName(n->enemy, enemy, sizeof(enemy));
                    zVEC3 p = n->GetPositionWorld();
                    const float dx = p.n[0] - CDiagWatchPos.n[0], dy = p.n[1] - CDiagWatchPos.n[1],
                        dz = p.n[2] - CDiagWatchPos.n[2];
                    OthelloCompatLog("[%lld s, %s] getroffener NPC %s nach %d s: bewegt %.0f, Zustand=%s,"
                        " Gegner=%s, Waffenmodus=%d, HP=%d, blockierte Animationen bisher %lu",
                        DiagSeconds(), DiagRole(), name, marks[CDiagWatchStep] / 1000,
                        sqrtf(dx * dx + dy * dy + dz * dz), n->state.curState.name.ToChar(), enemy,
                        n->GetWeaponMode(), n->GetAttribute(NPC_ATR_HITPOINTS), CDiagWatchBlocked);
                }
                else {
                    OthelloCompatLog("[%lld s, %s] getroffener NPC nicht mehr in der Welt", DiagSeconds(), DiagRole());
                    CDiagWatchNpc = NULL;
                }
                if (++CDiagWatchStep >= 4)
                    CDiagWatchNpc = NULL;
            }
        }

        if (CurrentMs >= CDiagNextDialogCheck && CDiagDialogLines < 80) {
            CDiagNextDialogCheck = CurrentMs + 250;
            oCInformationManager& im = oCInformationManager::GetInformationManager();
            char infoNpc[64], talkOther[64], focus[64], buf[400];
            CDiagName(im.Npc, infoNpc, sizeof(infoNpc));
            CDiagName(player->talkOther, talkOther, sizeof(talkOther));
            CDiagName(player->GetFocusNpc(), focus, sizeof(focus));
            _snprintf(buf, sizeof(buf), "Kamera=%s InfoNpc=%s IsDone=%d WartetEnde=%d WartetSkript=%d"
                " WartetAuswahl=%d talkOther=%s Fokus=%s",
                ogame->GetCameraAI()->GetMode().ToChar(), infoNpc, im.IsDone, im.IsWaitingForEnd,
                im.IsWaitingForScript, im.IsWaitingForSelection, talkOther, focus);
            buf[sizeof(buf) - 1] = 0;
            // Nur Zustaende mit Dialogbezug protokollieren, sonst flutet der Fokuswechsel das Log.
            const bool relevant = strstr(buf, "CAMMODDIALOG") || im.Npc;   // talkOther allein ist kein Dialog
            if (relevant && CDiagLastDialogState != buf) {
                CDiagLastDialogState = buf;
                CDiagDialogLines++;
                OthelloCompatLog("[%lld s, %s] Dialog: %s", DiagSeconds(), DiagRole(), buf);
            }
        }
    }

#else

    void CoopDiagAniBlocked(zCVob*, zCModelAni*) {}
    void CoopDiagUpdateSkipped(const char*, int) {}
    void CoopDiagClientHit(oCNpc*) {}
    void CoopDiagClientPulse() {}

#endif
}
