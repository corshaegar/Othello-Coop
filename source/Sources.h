// Supported with union (c) 2018-2021 Union team
// Add your sources this

// Automatically generated block
#pragma region Includes
#include "SafeQueue.cpp"
#include "CustomTypes.cpp"
#include "Chat.cpp"
#include "Utils.cpp"
#include "OthelloCompat.cpp"
#include "Global.cpp"
#include "CoopDiag.cpp"
#include "CoopLook.cpp"        // v55: Name, Adresse, eigenes Aussehen aus dem COOP-Menue (Gothic.ini), NPC-Suche
#include "Helpers.cpp"
#include "CoopDamage.cpp"
#include "CoopDiagClient.cpp"
#include "Debugger.cpp"
#include "PlayerAnimation.cpp"
#include "LocalNpc.cpp"
#include "RemoteNpc.cpp"
#include "SyncHelpers.cpp"
#include "CoopTrace.cpp"
#include "CoopWorldSync.cpp"   // v13: Truhen und Gegenstaende (Host fuehrt den Bestand)
#include "CoopSaveSync.cpp"    // v17: gemeinsames Speichern/Laden, Archiv, Liste im Spiel
#include "CoopCamera.cpp"      // v44: Kamera beim Mitspieler (Shift+F5)
#include "CoopFarSync.cpp"     // v41: Verfolgungspunkt beim Mitspieler, Todesabgleich ausserhalb der Reichweite
#include "CoopHitAuthority.cpp" // v53: Treffer auf den Helden eines Clients entscheidet der Client
#include "CoopDialogSync.cpp"  // v26: Dialoge - einer je NPC, von aussen beobachtbar
#include "CoopLogShip.cpp"     // v34/v35: Coop-Logs zum Partner (GothicCoopOthelloClient.log / GothicCoopOthelloHost.log)
#include "CoopSelfTest.cpp"    // v37/v38: Coop-Testlauf (Shift+F11), v38 mit 28 Schritten und eigenen Test-NPCs
#include "CoopCombatLead.cpp"  // v72: Kampf-Uebergabe - NPCs, die gegen einen Client kaempfen, rechnet dessen PC
#include "Client.cpp"
#include "Server.cpp"
#include "MappedPort.cpp"
#include "GameStats.cpp"
#include "PacketProcessor.cpp"
#include "DamageProcessor.cpp"
#include "VisibleNpcsUpdater.cpp"
#include "SpellCastProcessor.cpp"
#include "ReviveFriendLoop.cpp"
#include "Hooks.cpp"
#include "OthelloHookOrder.cpp"
#include "CoopDialogWatch.cpp"   // 18.10.2026: Dialog-Waechter (EV_PROCESSINFOS verloren)
#include "CoopAudit.cpp"
#include "Plugin.cpp"
#include "CoopMenuLobby.cpp"   // v40: Mehrspieler im Hauptmenue (COOP_MENU.D, Haken auf zCMenu::HandleSelAction)
#include "CoopMenuInject.cpp"  // v81: COOP-Eintrag zur Laufzeit in Othellos MENU_MAIN / MENU_OPTIONS (vtable-Platz SetByScript)
#include "CoopSavePreview.cpp" // v77: Vorschau in der Liste "Spielstand waehlen" (Bilder beider PCs, Kapitel, Tag, Aufgaben)
#include "CoopMenuMouse.cpp"   // v57: Maus in Gothics Menues (Zeiger, Ueberfahren, Klick; GRawInput RegisterWrapCheck)
#include "CoopTitle.cpp"       // v82: "Hauptmenue" im Esc-Menue - zurueck zum Startbildschirm ohne Neustart
#include "CoopSteam.cpp"       // v87: Coop ueber Steam (Tunnel unter ENet, Rich Presence, Steam-Menue)
#include "CoopPreview.cpp"     // v60: 3D-Vorschau der eigenen Figur im COOP-Menue (Renderer coop6, GDX_CoopPreviewDraw)
#include "CoopDifficulty.cpp"  // v67: Schwierigkeitsgrad (Schaden, Leben der Gegner, Kampf-EP; Host gibt vor)
#include "CoopNewGame.cpp"     // v91: gemeinsames neues Spiel (Intro fuer beide ueberspringen, Platz neben dem Host, Diego)
#include "UnionHooks.cpp"
#pragma endregion

// ...