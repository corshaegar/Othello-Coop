namespace GOTHIC_ENGINE {
    enum UpdateType
    {
        SYNC_POS,
        SYNC_HEADING,
        SYNC_ANIMATION,
        SYNC_WEAPON_MODE,
        INIT_NPC,
        DESTROY_NPC,
        SYNC_ATTACKS,
        SYNC_ARMOR,
        SYNC_WEAPONS,
        SYNC_HP,
        SYNC_TIME,
        SYNC_HAND,
        SYNC_MAGIC_SETUP,
        SYNC_SPELL_CAST,
        SYNC_REVIVED,
        SYNC_PROTECTIONS,
        SYNC_PLAYER_NAME,
        PLAYER_DISCONNECT,
        SYNC_TALENTS,
        SYNC_BODYSTATE,
        SYNC_OVERLAYS,
        SYNC_DROPITEM,
        SYNC_TAKEITEM,
        // v13 Welt-Abgleich (CoopWorldSync.cpp), Reihenfolge = Bereich in CoopWorldSyncHandles
        SYNC_CHEST_OPEN,
        SYNC_CHEST_CLOSE,
        SYNC_CHEST_STATE,
        SYNC_CHEST_BULK,
        SYNC_ITEM_TAKE,
        SYNC_ITEM_GONE,
        SYNC_ITEM_BULK,
        SYNC_WORLD_REQUEST,
        SYNC_NPC_LOOT,       // v15: Leiche/Bewusstloser gepluendert
        // v17 gemeinsames Speichern/Laden (CoopSaveSync.cpp), Reihenfolge = Bereich in CoopSaveSyncHandles
        SYNC_SAVE_REQ,
        SYNC_LOAD_REQ,
        SYNC_SAVE_ACK,
        // v19 Stand uebernehmen (Reihenfolge = Bereich in CoopSaveSyncHandles bis SYNC_STATE_REQ)
        SYNC_FILE_REQ,
        SYNC_FILE_PART,
        SYNC_FILE_END,
        SYNC_STATE_REQ,
        // v26 Dialoge (CoopDialogSync.cpp), Reihenfolge = Bereich in CoopDialogSyncHandles
        SYNC_DIALOG_START,
        SYNC_DIALOG_LINE,
        SYNC_DIALOG_END,
        // v27 Ueberspringen, Fortschritt (Bereich in CoopDialogSyncHandles bis SYNC_DIALOG_STATE)
        SYNC_DIALOG_SKIP,
        SYNC_DIALOG_STATE,
        // v33 Host-Ereignisse: Spawns, Trigger (Bereich in CoopDialogSyncHandles bis SYNC_HOST_EVENT)
        SYNC_HOST_EVENT,
        // v34 Client-Log zum Host (CoopLogShip.cpp)
        SYNC_CLIENT_LOG,
        // v37 Coop-Testlauf (CoopSelfTest.cpp)
        SYNC_SELFTEST,
        // v39 voller Stand nach Fortsetzen/Laden (CoopSaveSync.cpp), Vorab-Frage vor gemeinsamer Sicherung
        SYNC_RESYNC,
        SYNC_SAVE_CHECK,
        SYNC_SAVE_READY,
        // v40 Mehrspieler im Hauptmenue (CoopMenuLobby.cpp): hello / slots / have
        SYNC_LOBBY,
        // v41 Todesmeldung des Hosts (CoopFarSync.cpp): w Welt, d Namen, all = volle Liste
        SYNC_NPC_DEATH,
        // v53 Ergebnis eines NPC-Schlags auf den Helden eines Clients (CoopHitAuthority.cpp): att, dmg, hp, unc, dead / wake
        SYNC_HIT_RESULT,
        // v67 Schwierigkeitsgrad des Hosts (CoopDifficulty.cpp): p Stufe, dp/de/hp Kampfwerte, xs EP-Regel, xr EP-Reichweite
        SYNC_COOP_RULES,
        // v72 Kampf-Uebergabe (CoopCombatLead.cpp): op give (Host->Client, to), back (Client->Host), n NPC-Name, st, en, hp ...
        SYNC_NPC_LEAD,
    };

    struct PlayerHit
    {
        string npcUniqueName;
        float damage;
        oCNpc* npc;
        oCNpc* attacker;
        int isUnconscious;
        bool isDead;
        unsigned long damageMode;

        // Schadensmessung (CoopDamage.cpp): HP vor dem Treffer, HP beim Sound-Hook, Durchlauf.
        int hpBefore = -1;
        int hpAtSound = -1;
        unsigned long frame = 0;
        float dmgTotal = 0;
        float dmgEffective = 0;
        float dmgReal = 0;
    };

    struct SpellCast
    {
        oCNpc* npc;
        oCNpc* targetNpc;
        string npcUniqueName;
        string targetNpcUniqueName;
    };

    class PeerData
    {
    public:
        string name;
        PeerData() {}
    };
}