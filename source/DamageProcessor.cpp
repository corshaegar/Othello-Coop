namespace GOTHIC_ENGINE {
    void DamageProcessorLoop() {
        PluginState = "DamageProcessorLoop";
        CoopDamageFinalizePending();   // Treffer aus dem letzten Durchlauf endgueltig messen
        CoopSwingPulse();              // v53: Ergebnisse bestaetigter NPC-Schlaege an den Host (CoopHitAuthority.cpp)
        CoopPunishPulse();             // v54: Bestrafung wegen eines Client-Treffers geht auf dessen Figur (CoopHitAuthority.cpp)

        if (ReadyToSyncDamages.isEmpty()) {
            return;
        }

        auto hit = ReadyToSyncDamages.dequeue();

        if (!ClientThread && !ServerThread)
        {
            return;
        }

        if (Myself && hit.attacker == Myself->npc) {
            CoopDiagClientHit(hit.npc);
            hit.npcUniqueName = NpcToUniqueNameList[hit.npc];

            if (hit.npc->IsDead()) {
                KilledByPlayerNpcNames[hit.npcUniqueName] = hit.npc;
            }

            Myself->hitsToSync.push_back(hit);
            return;
        }

        if (ServerThread) {
            if (PlayerNpcs.count(hit.attacker)) {
                return;
            }

            auto uniqueNpcName = NpcToUniqueNameList[hit.attacker];
            if (uniqueNpcName && BroadcastNpcs.count(uniqueNpcName)) {
                if (hit.npc == player) {
                    hit.npcUniqueName = "HOST";
                    BroadcastNpcs[uniqueNpcName]->hitsToSync.push_back(hit);
                }
                else if (PlayerNpcs.count(hit.npc)) {
                    hit.npcUniqueName = PlayerNpcs[hit.npc];
                    BroadcastNpcs[uniqueNpcName]->hitsToSync.push_back(hit);
                }
                else if (NpcToUniqueNameList.count(hit.npc)) {
                    hit.npcUniqueName = NpcToUniqueNameList[hit.npc];
                    BroadcastNpcs[uniqueNpcName]->hitsToSync.push_back(hit);
                }
            }
        }
    }
}