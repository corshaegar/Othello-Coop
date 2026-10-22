// Coop-Figur des Mitspielers (GothicCoop, Othello 4.4.2.1). Ab v36 - nur zusammen mit GothicCoop v36 oder neuer einsetzen.
// Bis v35 (Datei PC_MULTIPLAYER_HERO.autorun-bis-v35.D): aivar[49] = 3000 und ein Schwert im Inventar.
instance pc_multiplayer_hero(npc_default) {
    name = "Player";
    guild = gil_none;
    id = 0;
    voice = 15;
    level = 0;
    npctype = npctype_main;
    exp = 0;
    exp_next = 500;
    lp = 0;

    // aivar[49] = AIV_BOSSBEHAVIOR (Bitfeld, wird in OTHELLO_ABI ausgewertet):
    //   16  BOSSBEHAVIOR_SUMMONEDBYPC   wie eine Beschwoerung des Spielers: im Kampfmodus nicht anvisierbar,
    //                                   eigene Pfeile/Zauber treffen sie nicht (CanThisCollideWith)
    //    8  BOSSBEHAVIOR_DISABLEAUTOAIM die Nahkampf-Ausrichtung des Spielers dreht nicht auf sie
    //  128  BOSSBEHAVIOR_CANNOTPLUNDER  liegend nicht pluenderbar
    // Frueher 3000 = zusaetzlich 32 Boss-Lebensbalken, 256 Magie- und 512 Feuer-Immunitaet (der Mitspieler nahm dadurch
    // keinen Magie-/Feuerschaden) und 2048 (von Othello nirgends abgefragt).
    aivar[49] = 152;

    // Kein Schwert mehr noetig: GothicCoop v36 belebt auch ohne Fokus die naechste liegende Coop-Figur in 2 m wieder
    // (Othello laesst Tote nur mit Inventar anvisieren).

	attribute[ATR_STRENGTH] 		= 10;
	attribute[ATR_DEXTERITY] 		= 10;
	attribute[ATR_MANA_MAX] 		= 10;
	attribute[ATR_MANA] 			= 10;
	attribute[ATR_HITPOINTS_MAX]	= 40;
	attribute[ATR_HITPOINTS] 		= 40;

    mdl_setvisual(self, "HUMANS.MDS");
	mdl_setvisualbody(self, "hum_body_Naked0", 9, 0, "Hum_Head_Pony", 18, 0, -1);
};
