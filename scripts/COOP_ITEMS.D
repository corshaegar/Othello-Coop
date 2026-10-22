// Coop-Gegenstaende (GothicCoop, Othello 4.4.2.1). Ab v102 - nur zusammen mit GothicCoop v102 oder neuer einsetzen.
// Gehoert nach system\Autorun (zParserExtender, Game-Parser wie PC_MULTIPLAYER_HERO.D). Beide PCs brauchen dieselbe Datei.

// v106: Aussehen der Coop-Rune. Vorbelegt mit Othellos Teleport-Runen-Modell; GothicCoop v106 oder neuer legt beim Start ein
// eigenes Modell an (ITRU_COOP_TELEPORT.3DS, Bild "F") und setzt diese Konstante darauf, sobald Gothic es laden kann.
// Aeltere GothicCoop-Fassungen lassen sie unveraendert.
const string coop_rune_visual = "ItRn_TP04.3ds";

// Teleport-Rune zum Mitspieler. Sie nutzt Gothics Teleport-Zauber SPL_TELEPORT5 (Aufladen, Effekt spellFX_Teleport und
// Animation T_HEASHOOT_2_STAND wie jede Teleport-Rune). GothicCoop setzt beim Wirken das Ziel auf den Mitspieler (statt des
// Wegpunkts aus Spell_Logic_Teleport5) und zieht dafuer kein Mana ab. Jeder Held bekommt sie nach dem Laden, falls er sie
// nicht hat (GothicCoop). Wie jede Teleport-Rune gesperrt, solange man in Xardas' altem Turm ist (Othellos Spell_Logic).
instance itru_coop_teleport(c_item) {
    name = "Teleport zum Mitspieler";
    mainflag = item_kat_rune;
    flags = 0;
    value = 0;
    visual = coop_rune_visual;
    material = mat_stone;
    spell = spl_teleport5;
    description = name;
    text[0] = "Bringt dich zu deinem Mitspieler (Coop).";
    text[1] = name_mana_needed;
    count[1] = 0;
};
