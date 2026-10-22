namespace GOTHIC_ENGINE {
    string ReviveState = "READY";
    oCNpc* ReviveNpc;
    long long ReviveStartTime = 0;

    void ReviveFriendLoop() {
        PluginState = "ReviveFriendLoop";

        if (WorldEditMode) {
            return;
        }

        if (!ServerThread && !ClientThread) {
            return;
        }

        if (ReviveStartTime + 10000 < CurrentMs && ReviveState.Compare("HEALING")) {
            if (player->IsDead() || player->IsUnconscious()) {
                ReviveStartTime = 0;
                ReviveState = "READY";
                return;
            }

            if (ReviveNpc && Myself)   // v98 (A16): vorher ohne Pruefung vor dem if unten
                Myself->SyncRevived(ReviveNpc->GetObjectName());

            if (ReviveNpc) {
                ReviveNpc->StopFaceAni("T_HURT");
                ReviveNpc->SetWeaponMode2(NPC_WEAPON_NONE);
            }

            ReviveStartTime = 0;
            ReviveState = "READY";
            ReviveNpc = NULL;
            ChatLog(CoopTr("Dein Mitspieler lebt wieder!", "Your friend is back alive!"));
            return;
        }

        if (ReviveState.Compare("HEALING") && ReviveNpc && ReviveNpc->GetPositionWorld().Distance(player->GetPositionWorld()) > 200) {
            ReviveStartTime = 0;
            ReviveState = "READY";

            ChatLog(CoopTr("Beim Wiederbeleben kannst du dich nicht bewegen.", "You cannot move while reviving your friend."));
            return;
        }

        if (CoopShiftDown() && zinput->KeyToggled(RevivePlayerKey)) {   // v39: nur mit Shift
            auto focusedNpc = player->GetFocusNpc();
            if (!focusedNpc || !IsCoopPlayer(focusedNpc->GetObjectName()))
            {
                // v36: ohne Fokus die naechste liegende Coop-Figur in 2 m. Othello (CollectFocusVob -> OTHELLO_ABI+0x8A730) laesst
                // Bewusstlose gar nicht und Tote nur mit Inventar anvisieren - dafuer stand bisher ein Schwert in
                // PC_MULTIPLAYER_HERO.D.
                focusedNpc = NULL;
                float best = 200.0f;
                for (auto& p : PlayerNpcs) {
                    oCNpc* n = p.first;
                    if (!n || n == player || n->GetAttribute(NPC_ATR_HITPOINTS) != 0)
                        continue;
                    const float dist = n->GetPositionWorld().Distance(player->GetPositionWorld());
                    if (dist <= best) {
                        best = dist;
                        focusedNpc = n;
                    }
                }
                if (!focusedNpc)
                    return;
            }

            ReviveNpc = focusedNpc;

            if (ReviveState.Compare("READY")) {
                auto frinedHp = ReviveNpc->GetAttribute(NPC_ATR_HITPOINTS);
                if (frinedHp != 0) {
                    ChatLog(CoopTr("Dein Mitspieler lebt - nichts wiederzubeleben.", "You cannot revive your friend when alive."));
                    ReviveNpc = NULL;
                    return;
                }

                if (ReviveNpc->GetPositionWorld().Distance(player->GetPositionWorld()) > 200) {
                    ChatLog(CoopTr("Zu weit weg zum Wiederbeleben.", "You are too far away to revive your friend."));
                    ReviveNpc = NULL;
                    return;
                }

                ChatLog(CoopTr("Du belebst deinen Mitspieler wieder - 10 Sekunden nicht bewegen.", "You are reviving your friend, please do not move for 10 seconds."));
                ReviveState = "HEALING";
                ReviveStartTime = CurrentMs;
                player->GetModel()->StartAni("T_PLUNDER", 1);
            }
            else if (ReviveState.Compare("HEALING")) {
                ChatLog(CoopTr("Wiederbelebung l" "\xE4" "uft schon - bitte warten.", "You are already reviving your friend, please wait."));
            }
        }
    }
}