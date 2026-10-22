#include "resource.h"

namespace GOTHIC_ENGINE {
    TSaveLoadGameInfo& SaveLoadGameInfo = UnionCore::SaveLoadGameInfo;

    void _Game_Entry() {
        Game_Entry();
    }

    void _Game_Init() {
        Game_Init();
    }

    void _Game_Loop() {
        try {
            static bool sabotageChecked = false;   // v79: Rueckhalt fuer Othellos Sabotage (Plugin.cpp)
            if (!sabotageChecked) {
                sabotageChecked = true;
                CoopOthelloSabotageCheck("erstes Spielbild", false);
            }
            CoopMenuMouseGameFrame();   // v57: ohne offenes Menue Zeiger weg
            CoopLangRefresh();          // v95: Sprache der Coop-Anzeigen (Gothic.ini coopLang)
            Game_Loop();
            CoopDiffPulse();            // v67: Schwierigkeitsgrad (Host sendet, Kampf-EP-Konstante)
            CoopNewGamePulse();         // v91: gemeinsames neues Spiel (Intro-Abgleich aus, Platz neben dem Host, Diego-Sperre)
        }
        catch (std::exception& e) {
            CoopErrorLog("Game_Loop", e.what(), PluginState.c_str());
            if (CoopErrorSaveDetails()) SaveErrorDetails();
        }
        catch (...) {
            CoopErrorLog("Game_Loop", "Zugriffsfehler (SEH) oder unbekannte Ausnahme", PluginState.c_str());
            if (CoopErrorSaveDetails()) SaveErrorDetails();
        }
    }

    void _Game_Exit() {
    }

    void _Game_PreLoop() {
    }

    void _Game_PostLoop() {
    }

    void _Game_MenuLoop() {
        // v40: Mehrspieler im Hauptmenue - Union ruft das aus dem Render-Platz (+0x1C) jeder Menue-vtable (SHW32+0x59C00)
        try {
            static bool sabotageChecked = false;   // v79: Rueckhalt fuer Othellos Sabotage (Plugin.cpp)
            if (!sabotageChecked) {
                sabotageChecked = true;
                CoopOthelloSabotageCheck("erstes Menuebild", false);
            }
            CoopLangRefresh();        // v95: Sprache der Coop-Anzeigen (Gothic.ini coopLang)
            CoopLobbyPulse();
            CoopSteamPulse();         // v87: Steam - auch im Hauptmenue (Rueckrufe, Tunnel, Beitreten aus Steam)
        }
        catch (std::exception& e) {
            CoopErrorLog("Game_MenuLoop", e.what(), PluginState.c_str());
        }
        catch (...) {
            CoopErrorLog("Game_MenuLoop", "Zugriffsfehler (SEH) oder unbekannte Ausnahme", PluginState.c_str());
        }
        try {
            CoopTitleMenuPulse();     // v84: "Hauptmenue" im Esc-Menue - Menues im naechsten Menuebild schliessen
            CoopMainMenuEscGuard();   // v59: Esc im Hauptmenue ohne Spiel = schwarzer Bildschirm - abfangen
            CoopVersionMenuPulse();   // v58: Fassung in den Versionszeilen (auch im Esc-Menue)
            CoopLangMenuPulse();      // v95: Menue-Texte Deutsch/English (COOP_MENU.D userString[2]/[3])
            CoopSteamMenuPulse();     // v87: Steam-Menue (Freunde, Status)
            CoopIngameLoadListPulse(); // v91: Spielstand-Liste im Esc-Menue
            CoopMenuMousePulse();     // v57: Maus im Menue
            CoopPreviewPulse();       // v60: 3D-Vorschau im COOP-Menue (zeichnet im naechsten Bild ueber DrawItems des Rahmens)
            CoopDiffMenuPulse();      // v67: Schwierigkeitsgrad - Stufe/Einzelwerte, Hinweiszeile
            CoopDiffPulse();          // v67: auch im Menue (Host sendet die Regeln schon in der Wartephase)
        }
        catch (std::exception& e) {
            CoopErrorLog("Menue-Maus", e.what(), PluginState.c_str());
        }
        catch (...) {
            CoopErrorLog("Menue-Maus", "Zugriffsfehler (SEH) oder unbekannte Ausnahme", PluginState.c_str());
        }
    }

    void _Game_SaveBegin() {
        try {
            Game_SaveBegin();
        }
        catch (std::exception& e) {
            CoopErrorLog("Game_SaveBegin", e.what(), PluginState.c_str());
            if (CoopErrorSaveDetails()) SaveErrorDetails();
        }
        catch (...) {
            CoopErrorLog("Game_SaveBegin", "Zugriffsfehler (SEH) oder unbekannte Ausnahme", PluginState.c_str());
            if (CoopErrorSaveDetails()) SaveErrorDetails();
        }
    }

    void _Game_SaveEnd() {
        try {
            Game_SaveEnd();
        }
        catch (std::exception& e) {
            CoopErrorLog("Game_SaveEnd", e.what(), PluginState.c_str());
            if (CoopErrorSaveDetails()) SaveErrorDetails();
        }
        catch (...) {
            CoopErrorLog("Game_SaveEnd", "Zugriffsfehler (SEH) oder unbekannte Ausnahme", PluginState.c_str());
            if (CoopErrorSaveDetails()) SaveErrorDetails();
        }
    }

    void _LoadBegin() {
        try {
            CoopMenuMouseHide();   // v57: kein Zeiger ueber dem Ladebild
            LoadBegin();
        }
        catch (std::exception& e) {
            CoopErrorLog("LoadBegin", e.what(), PluginState.c_str());
            if (CoopErrorSaveDetails()) SaveErrorDetails();
        }
        catch (...) {
            CoopErrorLog("LoadBegin", "Zugriffsfehler (SEH) oder unbekannte Ausnahme", PluginState.c_str());
            if (CoopErrorSaveDetails()) SaveErrorDetails();
        }
    }

    void _LoadEnd() {
        try {
            LoadEnd();
        }
        catch (std::exception& e) {
            CoopErrorLog("LoadEnd", e.what(), PluginState.c_str());
            if (CoopErrorSaveDetails()) SaveErrorDetails();
        }
        catch (...) {
            CoopErrorLog("LoadEnd", "Zugriffsfehler (SEH) oder unbekannte Ausnahme", PluginState.c_str());
            if (CoopErrorSaveDetails()) SaveErrorDetails();
        }
    }

    void _Game_LoadBegin_NewGame() {
        try {
            CoopSaveSyncOnNewGame();   // v76: offene Coop-Auftraege und gemerkter Held gelten nicht fuer ein neues Spiel
        }
        catch (std::exception& e) {
            CoopErrorLog("CoopSaveSyncOnNewGame", e.what(), PluginState.c_str());
        }
        _LoadBegin();
    }

    void _Game_LoadEnd_NewGame() {
        _LoadEnd();
        try {
            CoopNewGameLoadEnd();   // v91: gemeinsames neues Spiel - Client neben den Host, Diego-Sperre
        }
        catch (std::exception& e) {
            CoopErrorLog("CoopNewGameLoadEnd", e.what(), PluginState.c_str());
        }
        catch (...) {
            CoopErrorLog("CoopNewGameLoadEnd", "Zugriffsfehler (SEH) oder unbekannte Ausnahme", PluginState.c_str());
        }
    }

    void _Game_LoadBegin_SaveGame() {
        try {
            CoopSaveSyncOnLoadBegin();   // v17: Partner laedt denselben Stand mit
        }
        catch (std::exception& e) {
            CoopErrorLog("CoopSaveSyncOnLoadBegin", e.what(), PluginState.c_str());
        }
        _LoadBegin();
    }

    void _Game_LoadEnd_SaveGame() {
        try {
            Game_LoadEnd_SaveGame();
        }
        catch (std::exception& e) {
            CoopErrorLog("Game_LoadEnd_SaveGame", e.what(), PluginState.c_str());
            if (CoopErrorSaveDetails()) SaveErrorDetails();
        }
        catch (...) {
            CoopErrorLog("Game_LoadEnd_SaveGame", "Zugriffsfehler (SEH) oder unbekannte Ausnahme", PluginState.c_str());
            if (CoopErrorSaveDetails()) SaveErrorDetails();
        }
    }

    void _Game_LoadBegin_ChangeLevel() {
        _LoadBegin();
    }

    void _Game_LoadEnd_ChangeLevel() {
        _LoadEnd();
    }

    void _Game_LoadBegin_Trigger() {
        try {
            Game_LoadBegin_Trigger();
        }
        catch (std::exception& e) {
            CoopErrorLog("Game_LoadBegin_Trigger", e.what(), PluginState.c_str());
            if (CoopErrorSaveDetails()) SaveErrorDetails();
        }
        catch (...) {
            CoopErrorLog("Game_LoadBegin_Trigger", "Zugriffsfehler (SEH) oder unbekannte Ausnahme", PluginState.c_str());
            if (CoopErrorSaveDetails()) SaveErrorDetails();
        }
    }

    void _Game_LoadEnd_Trigger() {
    }

    void _Game_Pause() {
    }

    void _Game_Unpause() {
    }

    void _Game_DefineExternals() {
    }

    void _Game_ApplyOptions() {
    }

    /*
    Functions call order on Game initialization:
      - Game_Entry           * Gothic entry point
      - Game_DefineExternals * Define external script functions
      - Game_Init            * After DAT files init

    Functions call order on Change level:
      - Game_LoadBegin_Trigger     * Entry in trigger
      - Game_LoadEnd_Trigger       *
      - Game_Loop                  * Frame call window
      - Game_LoadBegin_ChangeLevel * Load begin
      - Game_SaveBegin             * Save previous level information
      - Game_SaveEnd               *
      - Game_LoadEnd_ChangeLevel   *

    Functions call order on Save game:
      - Game_Pause     * Open menu
      - Game_Unpause   * Click on save
      - Game_Loop      * Frame call window
      - Game_SaveBegin * Save begin
      - Game_SaveEnd   *

    Functions call order on Load game:
      - Game_Pause              * Open menu
      - Game_Unpause            * Click on load
      - Game_LoadBegin_SaveGame * Load begin
      - Game_LoadEnd_SaveGame   *
    */


#define AppDefault True
    CApplication* lpApplication = !CHECK_THIS_ENGINE ? Null : CApplication::CreateRefApplication(
        Enabled(AppDefault) _Game_Entry,
        Enabled(AppDefault) _Game_Init,
        Enabled(AppDefault) _Game_Exit,
        Enabled(AppDefault) _Game_PreLoop,
        Enabled(AppDefault) _Game_Loop,
        Enabled(AppDefault) _Game_PostLoop,
        Enabled(AppDefault) _Game_MenuLoop,
        Enabled(AppDefault) _Game_SaveBegin,
        Enabled(AppDefault) _Game_SaveEnd,
        Enabled(AppDefault) _Game_LoadBegin_NewGame,
        Enabled(AppDefault) _Game_LoadEnd_NewGame,
        Enabled(AppDefault) _Game_LoadBegin_SaveGame,
        Enabled(AppDefault) _Game_LoadEnd_SaveGame,
        Enabled(AppDefault) _Game_LoadBegin_ChangeLevel,
        Enabled(AppDefault) _Game_LoadEnd_ChangeLevel,
        Enabled(AppDefault) _Game_LoadBegin_Trigger,
        Enabled(AppDefault) _Game_LoadEnd_Trigger,
        Enabled(AppDefault) _Game_Pause,
        Enabled(AppDefault) _Game_Unpause,
        Enabled(AppDefault) _Game_DefineExternals,
        Enabled(AppDefault) _Game_ApplyOptions
    );
}