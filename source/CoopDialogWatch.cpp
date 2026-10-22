// Supported with union (c) 2020 Union team
// Union SOURCE file

#include <intrin.h>   // _ReturnAddress

namespace GOTHIC_ENGINE {

    // --- Dialog-Waechter (18.10.2026) -----------------------------------------------------------
    //
    // Fehlerbild (Nutzer): Beim Ansprechen eines NPCs oeffnet sich der Dialog, es erscheinen aber keine
    // Zeilen, der Held haengt fest (Cayden, ein Gardist; auch im Einzelspieler).
    //
    // Befund (dialog_sonde.py im haengenden Dialog, nur gelesen): oCInformationManager (0x009D9808)
    // Npc=Cayden, IsDone=0, IsWaitingForScript=1, Info=0; KI-Warteschlangen von Cayden und Held leer,
    // Caydens EventManager "cleared"=1, Zustand ZS_TALK. Den Dialog treibt allein die Nachricht
    // EV_PROCESSINFOS in der Warteschlange des NPCs: oCNpc::EV_ProcessInfos (0x006B3DD0) ruft, solange
    // sie "inUse" ist, je Frame oCInformationManager::Update (0x0072BE90); oCGame::Render ruft Update nur
    // bei IsWaitingForEnd (0x0063DEC0). Leert jemand waehrend des Dialogs die Warteschlange des NPCs
    // (oCNpc::ClearEM 0x006A2610, unter Othello mit umgeleitetem GetEM), ist die Nachricht weg, Update
    // laeuft nie wieder, IsDone bleibt 0 und der Held kommt nicht heraus.
    //
    // Abhilfe: Fehlt die Nachricht bei offenem Dialog 1,5 s lang, eine neue EV_PROCESSINFOS (Ziel = Held,
    // inUse = 1, also ohne neues SetNpc) beim NPC einreihen; der Dialog laeuft dann normal weiter. Nach
    // drei erfolglosen Versuchen oCInformationManager::Exit() (Dialog beenden, Held wieder frei).
    // Dazu Protokoll, wer ClearEM auf den Gespraechspartner eines offenen Dialogs ruft (Ruecksprung +
    // Modul), damit die eigentliche Ursache gefunden werden kann. Ausgabe system\GothicCoopOthello.log.

#if ENGINE == Engine_G1

    static unsigned long DialogWatchLines = 0;

    static bool DialogWatchDarfLoggen() {
        return DialogWatchLines++ < 200;
    }

    static bool DialogWatchHatProcessInfos(oCNpc* npc) {
        zCEventManager* em = npc->GetEM(0);
        if (!em)
            return false;
        const int n = em->GetNumMessages();
        for (int i = 0; i < n; i++) {
            oCMsgConversation* m = zDYNAMIC_CAST<oCMsgConversation>(em->GetEventMessage(i));
            if (m && m->subType == oCMsgConversation::EV_PROCESSINFOS && !m->IsDeleted())
                return true;
        }
        return false;
    }

    void CoopDialogWatchPulse() {
        static oCNpc* watchNpc = NULL;
        static long long since = 0;
        static int tries = 0;

        oCInformationManager& im = oCInformationManager::GetInformationManager();
        if (!im.Npc || !im.Player || im.IsDone || DialogWatchHatProcessInfos(im.Npc)) {
            watchNpc = NULL;
            tries = 0;
            return;
        }
        if (watchNpc != im.Npc) {          // Zustand neu gesehen: erst beobachten
            watchNpc = im.Npc;
            since = CurrentMs;
            tries = 0;
            return;
        }
        if (CurrentMs < since + 1500)
            return;

        char name[64];
        CDiagName(im.Npc, name, sizeof(name));
        char flags[96];
        _snprintf(flags, sizeof(flags), "WartetSkript=%d WartetEnde=%d WartetAuswahl=%d Wichtig=%d/%d",
            im.IsWaitingForScript, im.IsWaitingForEnd, im.IsWaitingForSelection, im.ImportantCurrent, im.ImportantMax);
        flags[sizeof(flags) - 1] = 0;
        if (tries < 3) {
            oCMsgConversation* msg = new oCMsgConversation(oCMsgConversation::EV_PROCESSINFOS, im.Player);
            msg->SetInUse(TRUE);
            im.Npc->GetEM()->OnMessage(msg, im.Player);
            tries++;
            if (DialogWatchDarfLoggen())
                OthelloCompatLog("[%lld s, %s] Dialog-Waechter: Dialog mit %s ohne EV_PROCESSINFOS (%s) - Nachricht neu eingereiht (Versuch %d)",
                    DiagSeconds(), DiagRole(), name, flags, tries);
        }
        else {
            im.Exit();
            if (DialogWatchDarfLoggen())
                OthelloCompatLog("[%lld s, %s] Dialog-Waechter: Dialog mit %s haengt weiter (%s) - beendet (Exit)", DiagSeconds(), DiagRole(), name, flags);
            watchNpc = NULL;
            tries = 0;
        }
        since = CurrentMs;
    }

    // Protokoll: wer leert die Warteschlange des Gespraechspartners waehrend eines offenen Dialogs?
    void __fastcall oCNpc_ClearEM_DialogWatch(oCNpc* _this, void* vtable);
    CInvoke<void(__thiscall*)(oCNpc*)> Ivk_oCNpc_ClearEM_DialogWatch(0x006A2610, &oCNpc_ClearEM_DialogWatch);
    void __fastcall oCNpc_ClearEM_DialogWatch(oCNpc* _this, void* vtable) {
        oCInformationManager& im = oCInformationManager::GetInformationManager();
        if (_this && im.Npc == _this && !im.IsDone) {
            const void* ret = _ReturnAddress();
            char name[64], modul[96];
            CDiagName(_this, name, sizeof(name));
            ModuleName(ret, modul, sizeof(modul));
            if (DialogWatchDarfLoggen())
                OthelloCompatLog("[%lld s, %s] Dialog-Waechter: ClearEM auf Gespraechspartner %s waehrend des Dialogs, Aufrufer %s %p",
                    DiagSeconds(), DiagRole(), name, modul, ret);
        }
        Ivk_oCNpc_ClearEM_DialogWatch(_this);
    }

#else
    void CoopDialogWatchPulse() {}
#endif
}
