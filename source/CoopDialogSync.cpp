// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

    // --- Dialoge: nur einer je NPC, von aussen beobachtbar (v26), Fortschritt + Ueberspringen (v27) -----
    //
    // Wunsch des Nutzers: immer nur ein Spieler spricht mit einem NPC; der Dialog ist von aussen beobachtbar -
    // der NPC spricht seine Zeilen, die Coop-Figur die gewaehlten Heldenzeilen. v27: der Dialog-Fortschritt gilt
    // danach fuer beide, und Ueberspringen (Rechtsklick) laesst die Zeile bei beiden enden.
    //
    // Sprecher-Seite:
    // - Eigener Dialog wird im Pulse an oCInformationManager erkannt (Npc gesetzt, Player == Held, IsDone == 0)
    //   -> DIALOG_START {n = GT-Name des NPCs}, beim Ende DIALOG_END.
    // - Zeilen: Hook hinter oCNpc::EV_Output (0x006B1A70) bzw. EV_OutputSVM (0x006B15A0) -> DIALOG_LINE {n, s = h
    //   (Held) | n (NPC), o = Ausgabename, svm}. EV_Output ist sofort "fertig" (Rueckgabe 1 nach StartOutputUnit);
    //   gesprochen wird danach von der Ton-Nachricht der Ausgabe-Einheit (oCMsgConversation EV_PLAYANISOUND /
    //   EV_PLAYSOUND) in der Warteschlange des Sprechers, Restzeit in ms im Feld +0x90 (SDK-Name f_no; Othello zaehlt
    //   es in OTHELLO_ABI+0x7EA40 herunter, die Nachricht endet bei <= 0).
    // - Ueberspringen: je Sprecher wird diese Ton-Nachricht verfolgt. Verschwindet sie mit mehr als
    //   DIALOG_SKIP_REST_MS Restzeit, wurde die Zeile abgebrochen -> DIALOG_SKIP {n, s, o}. Unabhaengig davon, wie
    //   das Ueberspringen ausgeloest wird (Rechtsklick, Taste, anderes Plugin).
    // - Fortschritt: Hooks um oCInformationManager::OnInfo(oCInfo*) (0x0072D560) und OnChoice(oCInfoChoice*)
    //   (0x0072DC70), beide __fastcall. Darin laeuft das Dialogskript synchron (oCInfo::Info -> CallFunc, told = 1;
    //   Othello setzt told schon vor dem Aufruf). Vorher/nachher Schnappschuss, der Unterschied geht als
    //   DIALOG_STATE {i = Info, v = Skript-Ganzzahlen [Name, Index, Wert], l = Tagebuch, p = NPC-Zustand}.
    //   Skript-Ganzzahlen: alle globalen int-Symbole (nicht const/Klassenfeld/extern, ohne '.' = keine lokalen).
    //   Othello benutzt kein Ikarus/LeGo (GOTHIC.DAT geprueft) - es gibt keine Zeiger in Skriptvariablen.
    //   NPC-Zustand (alle NPCs der Welt ausser Spielern): aivar[50], Haltung (+0x824/+0x828, Felder unter Othello
    //   unveraendert), Tagesablauf (daily_routine = Symbolindex, als Name uebertragen).
    // Zuschauer-Seite (Host und Clients):
    // - Sperre fuer NPC n: oCNpc::AssessTalk_S (0x006B5F50) liefert 0, Chat-Hinweis. GTs Client-Freiraeumen im
    //   StandActions-Hook laesst solche NPCs in Ruhe.
    // - NPC n anhalten (v27): Warteschlange leeren, zur Coop-Figur drehen; danach unterdrueckt ein Hook auf
    //   oCNpc_States::DoAIState (0x006C5C60, Aufrufer oCAIHuman::DoAI ignoriert die Rueckgabe) die ZS-Schleife -
    //   der Tagesablauf laeuft nicht weiter, die Warteschlange (Zeilen) aber schon. v26 schob dafuer EV_WAIT nach;
    //   die standen vor den Ton-Nachrichten und verzoegerten jede Zeile.
    // - Zeile abspielen: neue oCMsgConversation(EV_OUTPUT bzw. EV_OUTPUTSVM, Name) OHNE Ziel in die Warteschlange des
    //   Sprechers, nur in Hoerweite (DIALOG_HEAR_RANGE). Ohne Ziel nimmt EV_Output den Monolog-Weg, und EV_PlaySound
    //   zeigt den Untertitel wie bei Leuten in der Umgebung (v27, Wunsch des Nutzers: rot). Untertitel-Arten in
    //   EV_PlaySound (msg+0x94): 1 eigener Dialog (Held weiss 0x8DB548, NPC gelb 0x8DB478; Spieler beteiligt UND
    //   ZS_TALK), 5 Umgebung/Zuruf (orange-rot 0x8DB480, ogame+0x44; Spieler beteiligt ohne ZS_TALK oder kein
    //   Partner), 3 Zwischensequenz, 2 NPC-NPC (weiss, PrintMessageCXY ogame+0x38). "Spieler" = vtable+0x104
    //   oCNpc::IsSelfPlayer. Die Farben setzt das Spiel zur Laufzeit. Die Reihenfolge NPC/Held stellt ohne Ziel
    //   DialogLineQueue her (naechste Zeile erst, wenn beide still sind).
    // - SKIP: wartet die Zeile noch, wird sie ausgelassen; sonst Ton-Nachrichten des Sprechers auf Restzeit 0 (reines
    //   Datenfeld), Stimme aus (oCNpc::StopAllVoices, nicht virtuell).
    // - STATE: Ganzzahlen ueber zCPar_Symbol::SetValue, told direkt, Tagebuch ueber die Skript-Externals
    //   LOG_CREATETOPIC / LOG_SETTOPICSTATUS / LOG_ADDENTRY (Funktionszeiger aus dem Symbol, zCPar_Symbol::offset -
    //   so wie DefineExternal ihn ablegt; Parameter auf den Datenstapel: int = Wert, 0x40; string = zSTRING*, 0x41),
    //   NPCs ueber aivar, SetAttitude/SetTmpAttitude, oCNpc_States::ChangeRoutine(int).
    // - Ende: Sperre sofort frei; NPC erst freigeben, wenn er nicht mehr spricht (hoechstens 15 s).
    // Alle benutzten Engine-Stellen und ihre Aufrufer unter Othello geprueft (Speicherabbild): unveraendert bis auf
    // oCInfo::Info (Othello: told vor dem Skript) - deshalb wird dort nicht gehookt.

    bool CoopRuleQuests();   // v75: CoopDifficulty.cpp - Abgleich "Quests" / "Tagebuch" (Regeln des Hosts)
    bool CoopRuleLog();
    bool CoopNewGameInfoBlocked();   // v91: CoopNewGame.cpp - gemeinsames neues Spiel: der Host spricht zuerst (Diego)
    void CoopNewGameDialogEnded(const std::string& by, const std::string& npc);

#if ENGINE == Engine_G1

    static const float DIALOG_HEAR_RANGE = 3000.f;   // cm
    static const int DIALOG_SKIP_REST_MS = 350;

    struct CoopDialogLock {
        std::string owner;
        long long since;
    };

    struct CoopDialogEnd {
        std::string n;
        std::string by;
        long long since;
    };

    struct CoopDialogQueuedLine {
        std::string by;
        std::string s;
        std::string o;
        bool svm;
    };

    struct CoopDialogLineTrack {
        void* msg = NULL;        // verfolgte Ton-Nachricht (nur Vergleich, nie dereferenziert)
        int lastRest = 0;        // zuletzt gesehene Restzeit in ms
        std::string line;        // zuletzt gemeldete Zeile dieses Sprechers
        long long lineMs = 0;
        bool soundSeen = false;
        bool ran = false;        // v80: Restzeit seit dem ersten Sehen gesunken - die Nachricht lief wirklich
    };

    static std::map<std::string, CoopDialogLock> DialogLocks;    // GT-Name des NPCs -> wer spricht
    static std::vector<CoopDialogEnd> DialogEnds;                 // beendet, NPC spricht evtl. noch die letzte Zeile
    static std::set<oCNpc*> DialogFrozen;                         // Tagesablauf angehalten (nur Vergleich)
    static std::map<std::string, std::deque<CoopDialogQueuedLine>> DialogLineQueue;   // je NPC: Zeilen, die noch warten
    static std::map<std::string, long long> DialogLastInjectMs;
    static oCNpc* MyDialogNpcPtr = NULL;
    static std::string MyDialogNpc;
    static std::map<oCNpc*, std::pair<void*, std::string>> LastLineMsg;
    static CoopDialogLineTrack TrackHero, TrackNpc;
    static long long DialogBusyMsgMs = 0;
    static int DialogInjecting = 0;
    static unsigned long DialogLogLines = 0;
    static int DialogSoundDiag = 0;

    static void DialogLog(const char* fmt, ...) {
        if (DialogLogLines >= 1500)
            return;
        DialogLogLines++;
        char buf[600];
        va_list ap;
        va_start(ap, fmt);
        _vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        buf[sizeof(buf) - 1] = 0;
        OthelloCompatLog("[%lld s, %s] Dialog-Sync: %s", DiagSeconds(), DiagRole(), buf);
    }

    static bool DialogSyncAktiv() {
        return (ServerThread || ClientThread) && !IsCoopPaused && !IsLoadingLevel && !WorldEditMode && ogame && player
            && !(MyselfId == "_player_");
    }

    static std::string DialogMe() {
        return std::string(MyselfId.ToChar());
    }

    static std::string DialogNpcName(oCNpc* npc) {
        auto f = NpcToUniqueNameList.find(npc);
        return f == NpcToUniqueNameList.end() ? std::string() : std::string(f->second.ToChar());
    }

    static oCNpc* DialogNpcByName(const std::string& n) {
        auto f = UniqueNameToNpcList.find(string(n.c_str()));
        return f == UniqueNameToNpcList.end() ? NULL : f->second;
    }

    static oCNpc* DialogFigureOf(const std::string& who) {
        auto f = PlayerNameToNpc.find(string(who.c_str()));
        return f == PlayerNameToNpc.end() ? NULL : f->second;
    }

    static void DialogSend(json& j, int type) {
        j["type"] = type;
        j["id"] = MyselfId.ToChar();
        j["w"] = CoopWorldName();
        if (ServerThread && !j.contains("by"))
            j["by"] = MyselfId.ToChar();
        CoopTraceCount(TRACE_SENT, type);
        ReadyToSendJsons.enqueue(j);
    }

    // Fuer den StandActions-Hook: steht dieser NPC in einem Dialog eines Mitspielers?
    bool CoopDialogNpcBusy(oCNpc* npc) {
        if (!npc || DialogLocks.empty())
            return false;
        auto f = DialogLocks.find(DialogNpcName(npc));
        return f != DialogLocks.end() && f->second.owner != DialogMe();
    }

    // --- Nachrichten in Warteschlangen --------------------------------------------------------------

    static bool DialogIsSoundMsg(oCMsgConversation* m) {
        return m && (m->subType == oCMsgConversation::EV_PLAYANISOUND || m->subType == oCMsgConversation::EV_PLAYSOUND);
    }

    // laufende Ton-Nachricht: Restzeit gueltig (v28). Eine noch nicht gestartete Nachricht traegt in +0x90 anderes
    // (beim Helden gesehen: 0x3F800000 = 1.0f); EV_PlaySound setzt dort beim Start die Dauer in ms.
    static oCMsgConversation* DialogFindSound(oCNpc* npc) {
        zCEventManager* em = npc ? npc->GetEM(0) : NULL;
        if (!em)
            return NULL;
        const int cnt = em->GetNumMessages();
        for (int i = 0; i < cnt; i++) {
            oCMsgConversation* m = zDYNAMIC_CAST<oCMsgConversation>(em->GetEventMessage(i));
            if (DialogIsSoundMsg(m) && !m->IsDeleted() && m->f_no > 0 && m->f_no <= 120000)
                return m;
        }
        return NULL;
    }

    // spricht der NPC noch (Zeile wartet oder Ton laeuft)?
    static bool DialogSpeaking(oCNpc* npc) {
        zCEventManager* em = npc ? npc->GetEM(0) : NULL;
        if (!em)
            return false;
        const int cnt = em->GetNumMessages();
        for (int i = 0; i < cnt; i++) {
            oCMsgConversation* m = zDYNAMIC_CAST<oCMsgConversation>(em->GetEventMessage(i));
            if (m && !m->IsDeleted() && (DialogIsSoundMsg(m) || m->subType == oCMsgConversation::EV_OUTPUT
                || m->subType == oCMsgConversation::EV_OUTPUTSVM))
                return true;
        }
        return false;
    }

    // --- Sprecher-Seite: Ueberspringen erkennen -----------------------------------------------------

    static void DialogCheckSkip(oCNpc* speaker, CoopDialogLineTrack& t, const char* s) {
        oCMsgConversation* cur = speaker ? DialogFindSound(speaker) : NULL;
        const int rest = cur ? cur->f_no : 0;
        // v80: dieselbe Nachricht, Restzeit gestiegen, ohne vorher gesunken zu sein - sie startet erst jetzt. Eine neue Ton-Nachricht
        // im Speicher einer alten traegt in +0x90 bis zum Start noch deren Rest (Test v79 nach Tod + Laden: 561, 2521, 1202 ms wie vor
        // dem Tod) - das war kein Ueberspringen, der Partner brach die Zeilen trotzdem ab.
        if (t.msg && cur == (void*)t.msg && rest > t.lastRest + 50 && !t.ran) {
            static int restartLogged = 0;
            if (restartLogged < 20) {
                restartLogged++;
                DialogLog("Ton der Zeile %s (%s) startet erst jetzt (%d ms, vorher %d ms aus altem Speicher) - kein Ueberspringen",
                    t.line.c_str(), s, rest, t.lastRest);
            }
            t.lastRest = rest;
            return;
        }
        // die verfolgte Ton-Nachricht ist weg - oder an derselben Adresse steht schon die naechste (Restzeit stieg)
        if (t.msg && (cur != (void*)t.msg || rest > t.lastRest + 50)) {
            if (t.ran && t.lastRest > DIALOG_SKIP_REST_MS && !t.line.empty() && !MyDialogNpc.empty()) {
                json j;
                j["n"] = MyDialogNpc;
                j["s"] = s;
                j["o"] = t.line;
                j["r"] = t.lastRest;
                DialogSend(j, SYNC_DIALOG_SKIP);
                DialogLog("Zeile %s uebersprungen (Rest %d ms) - Partner ueberspringt mit", t.line.c_str(), t.lastRest);
            }
            t.msg = NULL;
            t.lastRest = 0;
            t.ran = false;
        }
        if (cur && !t.msg) {
            t.msg = cur;
            t.soundSeen = true;
            t.ran = false;
            t.lastRest = rest;   // v80: Ausgangswert - "lief" erst, wenn die Restzeit darunter sinkt
            if (DialogSoundDiag < 30) {
                DialogSoundDiag++;
                DialogLog("Ton der Zeile %s (%s): Art %d, Restzeit %d ms", t.line.c_str(), s, (int)cur->subType, rest);
            }
        }
        if (cur) {
            if (rest < t.lastRest)
                t.ran = true;
            t.lastRest = rest;
        }
        else if (!t.soundSeen && !t.line.empty() && t.lineMs && CurrentMs > t.lineMs + 2000) {
            DialogLog("Zeile %s (%s): keine Ton-Nachricht in der Warteschlange gefunden - Ueberspringen hier nicht erkennbar",
                t.line.c_str(), s);
            t.lineMs = 0;
        }
    }

    static void DialogCheckSkips() {
        DialogCheckSkip(player, TrackHero, "h");
        DialogCheckSkip(MyDialogNpcPtr, TrackNpc, "n");
    }

    // --- Sprecher-Seite: Beginn, Ende, Zeilen --------------------------------------------------------

    static void TrackOwnDialog() {
        oCInformationManager& im = oCInformationManager::GetInformationManager();
        oCNpc* now = (im.Npc && im.Player == player && !im.IsDone) ? im.Npc : NULL;
        if (now == MyDialogNpcPtr)
            return;
        if (MyDialogNpcPtr && !MyDialogNpc.empty()) {
            DialogCheckSkips();   // letzte Zeile uebersprungen und gleich beendet?
            json j;
            j["n"] = MyDialogNpc;
            DialogSend(j, SYNC_DIALOG_END);
            DialogLog("eigener Dialog mit %s beendet", MyDialogNpc.c_str());
        }
        MyDialogNpcPtr = now;
        MyDialogNpc = now ? DialogNpcName(now) : std::string();
        LastLineMsg.clear();
        TrackHero = CoopDialogLineTrack();
        TrackNpc = CoopDialogLineTrack();
        if (!now)
            return;
        if (MyDialogNpc.empty()) {
            DialogLog("eigener Dialog mit %s - NPC hat keinen GT-Namen, nicht beobachtbar", now->GetInstanceName().ToChar());
            return;
        }
        auto f = DialogLocks.find(MyDialogNpc);
        if (f != DialogLocks.end() && f->second.owner != DialogMe())
            DialogLog("eigener Dialog mit %s, obwohl %s mit ihm spricht (gleichzeitig angesprochen oder vom NPC gestartet)",
                MyDialogNpc.c_str(), f->second.owner.c_str());
        json j;
        j["n"] = MyDialogNpc;
        DialogSend(j, SYNC_DIALOG_START);
        DialogLog("eigener Dialog mit %s beginnt", MyDialogNpc.c_str());
    }

    static void ReportLine(oCNpc* speaker, oCMsgConversation* msg, bool svm) {
        if (DialogInjecting || !msg || !speaker || !DialogSyncAktiv())
            return;
        if (!MyDialogNpcPtr)
            TrackOwnDialog();   // Zeile im selben Bild wie der Dialogbeginn
        if (!MyDialogNpcPtr || MyDialogNpc.empty())
            return;
        const bool hero = speaker == player;
        if (!hero && speaker != MyDialogNpcPtr)
            return;
        const std::string name = msg->name.ToChar();
        if (name.empty())
            return;
        auto& last = LastLineMsg[speaker];
        if (last.first == (void*)msg && last.second == name)
            return;   // dieselbe Nachricht, schon gemeldet
        last = std::make_pair((void*)msg, name);
        DialogCheckSkips();   // vorige Zeile vorzeitig zu Ende? Erst SKIP, dann die neue Zeile
        CoopDialogLineTrack& t = hero ? TrackHero : TrackNpc;
        t.line = name;
        t.lineMs = CurrentMs;
        t.soundSeen = false;
        json j;
        j["n"] = MyDialogNpc;
        j["s"] = hero ? "h" : "n";
        j["o"] = name;
        j["svm"] = svm ? 1 : 0;
        DialogSend(j, SYNC_DIALOG_LINE);
    }

    // --- Sprecher-Seite: Fortschritt (Skript-Zustand vor/nach Info bzw. Auswahl) --------------------

    struct CoopDlgTopic {
        std::string t;
        int sec = 0;
        int st = 0;
        std::vector<std::string> e;
    };

    struct CoopDlgNpc {
        oCNpc* npc = NULL;
        int aiv[50];
        int att = 0;
        int tatt = 0;
        int rtn = 0;
    };

    struct CoopDlgSnap {
        std::vector<int> ints;
        std::vector<CoopDlgTopic> log;
        std::vector<CoopDlgNpc> npcs;
        long long exp = 0;   // v31: Erfahrung des eigenen Helden (Quest-EP fuer beide)
    };

    struct CoopDlgIntSlot {
        zCPar_Symbol* sym;
        int idx;
    };

    static std::vector<CoopDlgIntSlot> DlgIntSlots;
    static std::map<std::pair<zCPar_Symbol*, int>, int> DlgSlotIndex;   // (Symbol, Feld) -> Platz in DlgIntSlots
    static int DlgIntSymCount = -1;

    // --- Story-Abgleich (v28): Stand, gegen den jede Sekunde verglichen wird ---------------------------
    // Was vom Partner kommt, wird auch hier eingetragen (kein Echo). Nach dem Laden neu aufgenommen.
    static bool StoryReady = false;
    static bool StorySyncOn = true;
    static bool StoryCfgRead = false;
    static long long StoryNextMs = 0;
    static std::vector<int> StoryBaseInts;
    static std::map<std::string, CoopDlgTopic> StoryBaseLog;
    static std::vector<std::deque<long long>> StoryChangeTimes;
    static std::vector<char> StoryNoisy;
    static std::set<std::string> StoryIgnore;

    // v31: NPC-Tagesablauf (daily_routine) und dauerhafte Haltung (attitude) aller Welt-NPCs, Gildenhaltungs-Tabelle
    struct CoopStoryNpcBase {
        int rtn;
        int att;
    };
    static std::map<std::string, CoopStoryNpcBase> StoryBaseNpc;   // GT-Name -> Stand
    static std::map<std::string, std::deque<long long>> StoryNpcTimes;
    static std::set<std::string> StoryNpcNoisy;
    static std::vector<unsigned char> StoryBaseGuilds;
    static int StoryGuildSize = 0;

    // Story-Werte sind kleine Zahlen (Merker, Zaehler, Tage, LOG_*). Grosse Werte koennten Zeiger/Handles von
    // Plugin-Skripten sein (zParserExtender) - nie uebertragen.
    static bool StoryValueOk(int v) {
        return v > -0x400000 && v < 0x400000;
    }

    // --- Heldenwerte (v31) -------------------------------------------------------------------------
    //
    // Othello legt Ausruestungs- und Bonuszustaende des Helden in GLOBALE Skriptvariablen (HERO_*_EQ, GET_ITAM_*, MODREGENHP,
    // HPPERKILL ...; Analyse tools\herovars.py: 135 nur von Helden-Funktionen geschrieben, 15 gemischt). Die duerfen weder der
    // Story-Abgleich noch D4 uebertragen, sonst bekaeme der Held des Partners fremde Boni. Einordnung zur Laufzeit aus dem
    // Bytecode (parser->stack: stack = Code, stacksize = Laenge, siehe zCPar_Stack::Load 0x006FB450):
    // - Wurzeln: alle Funktionen, die einem C_ITEM.ON_EQUIP/ON_UNEQUIP zugewiesen werden (PUSHINT|PUSHVAR f; PUSHVAR feld;
    //   ASSIGNFUNC), dazu B_GIVEXP, B_ADDBONUS, B_BUYATTRIBUTEPOINTS, B_TEACH*, die Instanz PC_HERO;
    // - Helden-Kontext = Wurzeln + alles, was sie aufrufen (CALL);
    // - Heldenwert = globale int-Variable, die irgendeine Funktion im Helden-Kontext schreibt (PUSHVAR|ARRAYVAR v; = += -= *= /=).
    static std::set<zCPar_Symbol*> StoryHeroVars;
    static bool StoryHeroVarsBuilt = false;

    static void StoryBuildHeroVars() {
        StoryHeroVarsBuilt = true;
        if (!parser)
            return;
        const unsigned char* code = parser->stack.stack;
        const int size = parser->stack.stacksize;
        const int cnt = parser->symtab.table.GetNum();
        if (!code || size <= 0 || cnt <= 0) {
            DialogLog("Heldenwerte: kein Skript-Code (%p, %d) - nicht eingeordnet", code, size);
            return;
        }
        std::vector<std::pair<int, int>> owners;   // Code-Anfang -> Symbol (Funktion, Prototyp, Instanz)
        std::map<int, int> funcAt;                  // Code-Anfang -> Funktionssymbol
        for (int i = 0; i < cnt; i++) {
            zCPar_Symbol* s = parser->symtab.table[i];
            if (!s || (s->flags & (zPAR_FLAG_EXTERNAL | zPAR_FLAG_CLASSVAR)))
                continue;
            if (s->type != zPAR_TYPE_FUNC && s->type != zPAR_TYPE_PROTOTYPE && s->type != zPAR_TYPE_INSTANCE)
                continue;
            const int st = s->single_intdata;
            if (st < 0 || st >= size)
                continue;
            owners.push_back(std::make_pair(st, i));
            if (s->type == zPAR_TYPE_FUNC)
                funcAt[st] = i;
        }
        std::sort(owners.begin(), owners.end());
        auto ownerOf = [&](int q) {
            auto it = std::upper_bound(owners.begin(), owners.end(), std::make_pair(q, 0x7FFFFFFF));
            return it == owners.begin() ? -1 : (it - 1)->second;
        };
        auto isGlobalInt = [&](int i) {
            if (i < 0 || i >= cnt)
                return false;
            zCPar_Symbol* s = parser->symtab.table[i];
            return s && s->type == zPAR_TYPE_INT && !(s->flags & (zPAR_FLAG_CONST | zPAR_FLAG_CLASSVAR)) && !strchr(s->name.ToChar(), '.');
        };
        const int onEq = parser->GetIndex(zSTRING("C_ITEM.ON_EQUIP"));
        const int onUneq = parser->GetIndex(zSTRING("C_ITEM.ON_UNEQUIP"));
        std::map<int, std::set<int>> calls, writers;
        std::set<int> roots;
        int p1op = -1, p1arg = 0, p2op = -1, p2arg = 0;
        for (int q = 0; q < size;) {
            const unsigned char op = code[q];
            int arg = 0, len = 1;
            if (op == 61 || op == 62 || op == 64 || op == 65 || op == 67 || op == 75 || op == 76 || op == 80) {
                if (q + 5 > size)
                    break;
                arg = *(const int*)(code + q + 1);
                len = 5;
            }
            else if (op == 245) {
                if (q + 6 > size)
                    break;
                arg = *(const int*)(code + q + 1);
                len = 6;
            }
            const int o = ownerOf(q);
            if (op == 61) {
                auto f = funcAt.find(arg);
                if (f != funcAt.end())
                    calls[o].insert(f->second);
            }
            else if ((op == 9 || (op >= 19 && op <= 22)) && (p1op == 65 || p1op == 245) && isGlobalInt(p1arg))
                writers[p1arg].insert(o);
            else if (op == 72 && p1op == 65 && (p1arg == onEq || p1arg == onUneq) && (p2op == 64 || p2op == 65)
                && p2arg >= 0 && p2arg < cnt && parser->symtab.table[p2arg] && parser->symtab.table[p2arg]->type == zPAR_TYPE_FUNC)
                roots.insert(p2arg);
            p2op = p1op; p2arg = p1arg;
            p1op = op; p1arg = arg;
            q += len;
        }
        const int equipRoots = (int)roots.size();
        for (const char* nm : { "B_GIVEXP", "B_ADDBONUS", "B_BUYATTRIBUTEPOINTS", "PC_HERO" }) {
            const int i = parser->GetIndex(zSTRING(nm));
            if (i >= 0)
                roots.insert(i);
        }
        for (int i = 0; i < cnt; i++) {
            zCPar_Symbol* s = parser->symtab.table[i];
            if (s && s->type == zPAR_TYPE_FUNC && !strncmp(s->name.ToChar(), "B_TEACH", 7))
                roots.insert(i);
        }
        std::set<int> hero;
        std::vector<int> work(roots.begin(), roots.end());
        while (!work.empty()) {
            const int f = work.back();
            work.pop_back();
            if (!hero.insert(f).second)
                continue;
            auto c = calls.find(f);
            if (c != calls.end())
                work.insert(work.end(), c->second.begin(), c->second.end());
        }
        std::string sample;
        for (auto& w : writers)
            for (int o : w.second)
                if (hero.count(o)) {
                    zCPar_Symbol* v = parser->symtab.table[w.first];
                    StoryHeroVars.insert(v);
                    if (sample.size() < 300)
                        sample += (sample.empty() ? "" : ", ") + std::string(v->name.ToChar());
                    break;
                }
        DialogLog("Heldenwerte: %u globale Variablen gehoeren zum Helden und werden nicht abgeglichen (%d Anlege-/Ablege-Funktionen, "
            "%u im Helden-Kontext), z. B. %s", (unsigned)StoryHeroVars.size(), equipRoots, (unsigned)hero.size(), sample.c_str());
    }

    static bool StoryIsHeroVar(zCPar_Symbol* s) {
        if (!StoryHeroVarsBuilt)
            StoryBuildHeroVars();
        return s && StoryHeroVars.count(s) != 0;
    }

    static void DlgBuildIntSlots() {
        const int cnt = parser ? parser->symtab.table.GetNum() : 0;
        if (cnt == DlgIntSymCount)
            return;
        DlgIntSlots.clear();
        DlgSlotIndex.clear();
        StoryReady = false;   // Platzliste neu -> Vergleichsstand neu aufnehmen
        for (int i = 0; i < cnt; i++) {
            zCPar_Symbol* sym = parser->symtab.table[i];
            if (!sym || sym->type != zPAR_TYPE_INT || (sym->flags & (zPAR_FLAG_CONST | zPAR_FLAG_CLASSVAR | zPAR_FLAG_EXTERNAL)))
                continue;
            const char* nm = sym->name.ToChar();
            if (!nm || !*nm || strchr(nm, '.'))
                continue;   // lokale Variablen (FUNKTION.NAME)
            if (StoryIsHeroVar(sym))
                continue;   // v31: Zustand des eigenen Helden (Ausruestung, Boni)
            const int ele = sym->ele > 1 ? (int)sym->ele : 1;
            for (int k = 0; k < ele; k++) {
                DlgSlotIndex[std::make_pair(sym, k)] = (int)DlgIntSlots.size();
                DlgIntSlots.push_back({ sym, k });
            }
        }
        DlgIntSymCount = cnt;
        DialogLog("%u Skript-Ganzzahlen werden fuer den Dialog-Fortschritt verglichen", (unsigned)DlgIntSlots.size());
    }

    static void DlgSnapLog(std::vector<CoopDlgTopic>& out) {
        oCLogManager& lm = oCLogManager::GetLogManager();
        for (auto* it = lm.m_lstTopics.GetNextInList(); it; it = it->GetNextInList()) {
            oCLogTopic* t = it->GetData();
            if (!t)
                continue;
            CoopDlgTopic s;
            s.t = t->m_strDescription.ToChar();
            s.sec = (int)t->m_enuSection;
            s.st = (int)t->m_enuStatus;
            for (auto* e = t->m_lstEntries.GetNextInList(); e; e = e->GetNextInList())
                if (oCLogEntry* le = e->GetData())
                    s.e.push_back(le->m_strDescription.ToChar());
            out.push_back(s);
        }
    }

    static void DlgSnapshot(CoopDlgSnap& s) {
        DlgBuildIntSlots();
        s.ints.resize(DlgIntSlots.size());
        for (size_t i = 0; i < DlgIntSlots.size(); i++) {
            int v = 0;
            DlgIntSlots[i].sym->GetValue(v, DlgIntSlots[i].idx);
            s.ints[i] = v;
        }
        DlgSnapLog(s.log);
        s.exp = player ? (long long)player->experience_points : 0;
        for (auto* l = CoopWorldNpcListFirst(ogame->GetGameWorld()); l; l = l->next) {
            oCNpc* npc = l->data;
            if (!npc || npc == player || IsCoopPlayer(npc->GetObjectName()) || !NpcToUniqueNameList.count(npc))
                continue;
            CoopDlgNpc n;
            n.npc = npc;
            memcpy(n.aiv, npc->aiscriptvars, sizeof(n.aiv));
            n.att = npc->attitude;
            n.tatt = npc->tmp_attitude;
            n.rtn = npc->daily_routine;
            s.npcs.push_back(n);
        }
    }

    // Tagebuch-Thema vorher (oder NULL = neu) gegen jetzt; Unterschied als {t, c, s?, e[]}. AddEntry haengt hinten an:
    // Listenfolge = zeitliche Folge.
    static bool DlgTopicDiff(const CoopDlgTopic* before, const CoopDlgTopic& now, json& jt) {
        jt = json();
        jt["t"] = now.t;
        jt["c"] = now.sec;
        bool changed = !before;
        std::set<std::string> old;
        if (before)
            old.insert(before->e.begin(), before->e.end());
        if (!before || before->st != now.st) {
            jt["s"] = now.st;
            changed = true;
        }
        json e = json::array();
        for (auto& s : now.e)
            if (!old.count(s)) {
                e.push_back(s);
                changed = true;
            }
        jt["e"] = e;
        return changed;
    }

    static void DlgSendDiff(const CoopDlgSnap& a, const CoopDlgSnap& b, const std::string& info, const char* what) {
        json j;
        int nv = 0, nl = 0, np = 0;
        json v = json::array();
        for (size_t i = 0; i < b.ints.size() && i < a.ints.size(); i++)
            if (a.ints[i] != b.ints[i] && StoryValueOk(a.ints[i]) && StoryValueOk(b.ints[i])) {
                v.push_back({ DlgIntSlots[i].sym->name.ToChar(), DlgIntSlots[i].idx, b.ints[i] });
                nv++;
            }
        json l = json::array();
        std::map<std::string, const CoopDlgTopic*> before;
        for (auto& t : a.log)
            before[t.t] = &t;
        for (auto& t : b.log) {
            auto f = before.find(t.t);
            json jt;
            if (!DlgTopicDiff(f == before.end() ? NULL : f->second, t, jt))
                continue;
            l.push_back(jt);
            nl++;
        }
        json p = json::array();
        std::map<oCNpc*, const CoopDlgNpc*> nb;
        for (auto& n : a.npcs)
            nb[n.npc] = &n;
        for (auto& n : b.npcs) {
            auto f = nb.find(n.npc);
            if (f == nb.end())
                continue;
            const CoopDlgNpc& o = *f->second;
            json jn;
            json av = json::array();
            for (int i = 0; i < 50; i++)
                if (o.aiv[i] != n.aiv[i])
                    av.push_back({ i, n.aiv[i] });
            if (!av.empty())
                jn["a"] = av;
            if (o.att != n.att)
                jn["at"] = n.att;
            if (o.tatt != n.tatt)
                jn["tt"] = n.tatt;
            if (o.rtn != n.rtn) {
                zCPar_Symbol* rs = parser->GetSymbol(n.rtn);
                if (rs)
                    jn["r"] = rs->name.ToChar();
            }
            if (jn.empty())
                continue;
            jn["n"] = DialogNpcName(n.npc);
            p.push_back(jn);
            np++;
        }
        // v31: Quest-EP - was der eigene Held waehrend des Dialogskripts an Erfahrung bekam (Kampf-EP laufen nicht hier)
        const long long xp = b.exp > a.exp ? b.exp - a.exp : 0;
        if (info.empty() && !nv && !nl && !np && !xp)
            return;
        if (!info.empty())
            j["i"] = info;
        j["n"] = MyDialogNpc;
        if (nv)
            j["v"] = v;
        if (nl)
            j["l"] = l;
        if (np)
            j["p"] = p;
        if (xp > 0 && xp < 1000000)
            j["xp"] = (int)xp;
        DialogSend(j, SYNC_DIALOG_STATE);
        DialogLog("Fortschritt (%s %s): %d Variablen, %d Tagebuch-Themen, %d NPCs, %d EP -> Partner", what,
            info.empty() ? "-" : info.c_str(), nv, nl, np, (int)xp);
    }

    static bool StoryNpcSkip(oCNpc* npc) {
        return !npc || npc == player || IsCoopPlayer(npc->GetObjectName());
    }

    static void StorySnapNpcs() {
        StoryBaseNpc.clear();
        for (auto* l = CoopWorldNpcListFirst(ogame->GetGameWorld()); l; l = l->next) {
            oCNpc* npc = l->data;
            if (StoryNpcSkip(npc))
                continue;
            const std::string n = DialogNpcName(npc);
            if (!n.empty())
                StoryBaseNpc[n] = { npc->daily_routine, npc->attitude };
        }
    }

    // oCGuilds: Tabelle size x size Bytes, Eintrag (g1, g2) bei attitude[g2 * size + g1] (oCGuilds::SetAttitude 0x00663340;
    // Othello hat dort nur die Grenzpruefung "< 42" entfernt - mehr Gilden).
    static void StorySnapGuilds() {
        oCGuilds* g = ogame ? ogame->GetGuilds() : NULL;
        if (!g || !g->attitude || g->size <= 0 || g->size > 256) {
            StoryGuildSize = 0;
            StoryBaseGuilds.clear();
            return;
        }
        StoryGuildSize = g->size;
        StoryBaseGuilds.assign(g->attitude, g->attitude + g->size * g->size);
    }

    // Was der Dialog geaendert und schon gesendet hat, in den Story-Vergleichsstand uebernehmen (sonst schickte der
    // naechste Story-Takt es noch einmal). Nur die Aenderungen a -> b, damit Aenderungen davor weiter gesendet werden.
    static void StoryAbsorb(const CoopDlgSnap& a, const CoopDlgSnap& b) {
        if (!StoryReady || StoryBaseInts.size() != b.ints.size() || a.ints.size() != b.ints.size())
            return;
        std::map<oCNpc*, const CoopDlgNpc*> na;   // v31: NPC-Tagesablauf/Haltung aus dem Dialog
        for (auto& n : a.npcs)
            na[n.npc] = &n;
        for (auto& n : b.npcs) {
            auto f = na.find(n.npc);
            if (f == na.end() || (f->second->rtn == n.rtn && f->second->att == n.att))
                continue;
            const std::string nm = DialogNpcName(n.npc);
            if (!nm.empty())
                StoryBaseNpc[nm] = { n.rtn, n.att };
        }
        for (size_t i = 0; i < b.ints.size(); i++)
            if (a.ints[i] != b.ints[i])
                StoryBaseInts[i] = b.ints[i];
        std::map<std::string, const CoopDlgTopic*> before;
        for (auto& t : a.log)
            before[t.t] = &t;
        for (auto& t : b.log) {
            auto f = before.find(t.t);
            json jt;
            if (!DlgTopicDiff(f == before.end() ? NULL : f->second, t, jt))
                continue;
            auto bt = StoryBaseLog.find(t.t);
            if (bt == StoryBaseLog.end()) {
                StoryBaseLog[t.t] = t;
                continue;
            }
            if (jt.contains("s"))
                bt->second.st = t.st;
            std::set<std::string> have(bt->second.e.begin(), bt->second.e.end());
            for (auto& s : jt["e"])
                if (!have.count(s.get<std::string>()))
                    bt->second.e.push_back(s.get<std::string>());
        }
    }

    static bool DlgOwnDialog(oCInformationManager* im) {
        return im && im->Player == player && DialogSyncAktiv() && parser;
    }

    // --- Zuschauer-Seite ---------------------------------------------------------------------------

    static bool DialogDarfAnhalten(oCNpc* npc) {
        return npc && npc != MyDialogNpcPtr && npc != player && !npc->IsDead() && npc->GetWeaponMode() == NPC_WEAPON_NONE;
    }

    static void DialogFreezeStart(const std::string& n, const std::string& by) {
        oCNpc* npc = DialogNpcByName(n);
        if (!npc) {
            DialogLog("NPC %s hier nicht vorhanden - nur gesperrt", n.c_str());
            return;
        }
        if (!DialogDarfAnhalten(npc)) {
            DialogLog("NPC %s nicht angehalten (eigener Dialog, tot oder Waffe gezogen)", n.c_str());
            return;
        }
        oCNpc* fig = DialogFigureOf(by);
        DialogInjecting++;
        npc->ClearEM();
        // kein Gespraechspartner: EV_PlaySound waehlt sonst den NPC-NPC-Untertitel (Art 2) statt Umgebung (Art 5)
        npc->StopTalkingWith();
        if (fig && fig->GetTalkingWith() != player)
            fig->StopTalkingWith();
        if (fig)
            npc->GetEM()->OnMessage(new oCMsgMovement(oCMsgMovement::EV_TURNTOVOB, (zCVob*)fig), npc);
        DialogInjecting--;
        DialogFrozen.insert(npc);
    }

    // Zeile abspielen. Ziel bewusst NULL: EV_Output nimmt dann den Monolog-Weg (kein StartTalkingWith - der eigene
    // Held bleibt unberuehrt), und EV_PlaySound zeigt den Untertitel wie bei Umgebungsgespraechen (Art 5, Ansicht
    // ogame+0x44, Farbe orange-rot 0x8DB480) statt als NPC-NPC-Text (Art 2, weiss). Die Reihenfolge NPC/Held, die
    // sonst EV_Output ueber das Ziel herstellt, uebernimmt DialogLineQueue.
    static bool DialogInjectLine(const std::string& n, const CoopDialogQueuedLine& l) {
        oCNpc* npc = DialogNpcByName(n);
        oCNpc* fig = DialogFigureOf(l.by);
        oCNpc* speaker = l.s == "h" ? fig : npc;
        if (!speaker) {
            DialogLog("Zeile %s von %s: %s hier nicht vorhanden", l.o.c_str(), l.by.c_str(), !fig ? "Coop-Figur" : n.c_str());
            return false;
        }
        const zVEC3 d = speaker->GetPositionWorld() - player->GetPositionWorld();
        if (d.Length() > DIALOG_HEAR_RANGE)
            return false;
        oCMsgConversation* msg = new oCMsgConversation(l.svm ? oCMsgConversation::EV_OUTPUTSVM : oCMsgConversation::EV_OUTPUT, zSTRING(l.o.c_str()));
        msg->target = NULL;
        DialogInjecting++;
        speaker->GetEM()->OnMessage(msg, speaker);
        DialogInjecting--;
        return true;
    }

    // naechste wartende Zeile starten, sobald NPC und Figur still sind (und die vorige mindestens 300 ms lief -
    // die Ton-Nachricht der Ausgabe-Einheit erscheint erst nach EV_Output)
    static void DialogPumpLines(const std::string& n) {
        auto q = DialogLineQueue.find(n);
        if (q == DialogLineQueue.end() || q->second.empty())
            return;
        if (CurrentMs < DialogLastInjectMs[n] + 300)
            return;
        const CoopDialogQueuedLine& l = q->second.front();
        if (DialogSpeaking(DialogNpcByName(n)) || DialogSpeaking(DialogFigureOf(l.by)))
            return;
        const CoopDialogQueuedLine line = l;
        q->second.pop_front();
        if (DialogInjectLine(n, line))
            DialogLastInjectMs[n] = CurrentMs;
    }

    static void DialogPlayLine(const std::string& n, const std::string& by, const std::string& s, const std::string& o, bool svm) {
        DialogLineQueue[n].push_back({ by, s, o, svm });
        DialogPumpLines(n);
    }

    static void DialogCutLine(const std::string& n, const std::string& by, const std::string& s, const std::string& o) {
        // noch nicht gestartet? Dann einfach auslassen.
        auto q = DialogLineQueue.find(n);
        if (q != DialogLineQueue.end())
            for (auto it = q->second.begin(); it != q->second.end(); ++it)
                if (it->o == o && it->s == s) {
                    q->second.erase(it);
                    DialogLog("%s ueberspringt %s - hier ausgelassen (wartete noch)", by.c_str(), o.c_str());
                    return;
                }
        oCNpc* speaker = s == "h" ? DialogFigureOf(by) : DialogNpcByName(n);
        zCEventManager* em = speaker ? speaker->GetEM(0) : NULL;
        if (!em)
            return;
        int cut = 0;
        const int cnt = em->GetNumMessages();
        for (int i = 0; i < cnt; i++) {
            oCMsgConversation* m = zDYNAMIC_CAST<oCMsgConversation>(em->GetEventMessage(i));
            if (m && !m->IsDeleted() && DialogIsSoundMsg(m) && m->f_no > 0) {
                m->f_no = 0;   // Restzeit: die Nachricht endet im naechsten Bild
                cut++;
            }
        }
        if (cut)
            speaker->StopAllVoices();
        DialogLog("%s ueberspringt %s - hier %s", by.c_str(), o.c_str(), cut ? "abgebrochen" : "lief nicht (mehr)");
    }

    static void DialogRelease(const std::string& n, const std::string& by) {
        oCNpc* npc = DialogNpcByName(n);
        oCNpc* fig = DialogFigureOf(by);
        if (npc && DialogLocks.find(n) == DialogLocks.end()) {
            DialogFrozen.erase(npc);
            if (npc != MyDialogNpcPtr)
                npc->StopTalkingWith();
        }
        if (fig)
            fig->StopTalkingWith();
    }

    static void DialogDropEnd(const std::string& n) {
        for (auto it = DialogEnds.begin(); it != DialogEnds.end();) {
            if (it->n == n)
                it = DialogEnds.erase(it);
            else
                ++it;
        }
    }

    // --- Zuschauer-Seite: Fortschritt anwenden ------------------------------------------------------

    typedef int(__cdecl* CoopParserExternal)();

    // zCParser::DefineExternal (0x006F6840): SetOffset(Rueckgabetyp) - bei void also 0! - und SetStackPos(Funktion, 0)
    // (0x006F8520: mov [ecx+18h], Funktion). Der Funktionszeiger steht im Inhaltsfeld (+0x18), nicht in offset (v28;
    // v27 las offset und fand die LOG_*-Externals deshalb nie).
    static CoopParserExternal DialogExternal(const char* name) {
        zCPar_Symbol* s = parser ? parser->GetSymbol(zSTRING(name)) : NULL;
        if (!s || s->type != zPAR_TYPE_FUNC || !(s->flags & zPAR_FLAG_EXTERNAL) || !s->single_intdata)
            return NULL;
        return (CoopParserExternal)s->single_intdata;
    }

    static void DialogPushInt(int v) {
        parser->datastack.Push(v);
        parser->datastack.Push(0x40);   // zPAR_TOK_PUSHINT
    }

    static void DialogPushString(zSTRING* s) {
        parser->datastack.Push((int)s);
        parser->datastack.Push(0x41);   // zPAR_TOK_PUSHVAR (PopString nimmt nur den Zeiger)
    }

    // v30 (Wunsch des Nutzers): die Meldung "Neuer Tagebucheintrag" auch beim Partner. Sie steht nicht in der Engine, sondern
    // im Skript B_LogEntry(topic, entry) - Othello: Log_AddEntry + PrintScreen("Neuer Tagebucheintrag (<Thema>)", -1,
    // _YPOS_MESSAGE_LOGENTRY, "font_old_10_white.tga", 3) + Snd_Play("LogEntry"). Also das Skript selbst ausfuehren, wie
    // zCParser::CallFunc (0x006E9690) es tut: Parameter auf den Datenstapel ([zSTRING*, 0x41], das Skript holt sie im Prolog
    // selbst ab: PUSHVAR param; ASSIGNSTR), curfuncnr setzen, DoStack(Codeadresse = Inhaltsfeld des Symbols). CallFunc selbst
    // geht nicht: es erwartet Zeichenketten als ganze zSTRING-Objekte (0x14 Byte) in den variablen Argumenten.
    // B_LogEntry ruft PrintDebugNpc, das self.id/self.name liest -> self fuer den Aufruf auf den eigenen Helden setzen
    // (Instanzzeiger = offset des Symbols SELF, wie zCParser::SetInstance), danach zuruecksetzen.
    // Skriptfunktion ohne Rueckgabe ausfuehren (v31 verallgemeinert): Parameter in Skript-Reihenfolge, jeweils int oder string.
    // Prueft Typ, Parameterzahl und -typen (Symbole Index+1 ...), setzt self = eigener Held (wie oben begruendet).
    struct CoopScriptArg {
        bool str;
        int i;
        zSTRING* s;
    };

    // v92: auch Funktionen mit int-Rueckgabe (Othellos B_IncreaseRep) - der Wert bleibt nach DoStack auf dem Datenstapel; wie
    // zCParser::CallFunc (0x006E9957: Rueckgabetyp im Symbol +0x1C, bei int/float PopDataValue 0x006E8550) abholen
    static bool DialogRunScript(int idx, const std::vector<CoopScriptArg>& args) {
        zCPar_Symbol* f = (idx >= 0 && parser) ? parser->GetSymbol(idx) : NULL;
        if (!f || !player || f->type != zPAR_TYPE_FUNC || (f->flags & zPAR_FLAG_EXTERNAL)
            || ((f->flags & zPAR_FLAG_RETURN) && f->offset != zPAR_TYPE_INT && f->offset != zPAR_TYPE_FLOAT)
            || (int)f->ele != (int)args.size() || f->single_intdata <= 0 || zCParser::GetParser() != parser)
            return false;
        for (size_t k = 0; k < args.size(); k++) {
            zCPar_Symbol* p = parser->GetSymbol(idx + 1 + (int)k);
            if (!p || (int)p->type != (args[k].str ? zPAR_TYPE_STRING : zPAR_TYPE_INT))
                return false;
        }
        zCPar_Symbol* selfSym = parser->GetSymbol(zSTRING("SELF"));
        void* oldSelf = selfSym ? (void*)selfSym->offset : NULL;
        parser->SetInstance(zSTRING("SELF"), player);
        for (auto& a : args) {
            if (a.str)
                DialogPushString(a.s);
            else
                DialogPushInt(a.i);
        }
        const int oldFunc = parser->curfuncnr;
        parser->curfuncnr = idx;
        parser->DoStack(f->single_intdata);
        if (f->flags & zPAR_FLAG_RETURN)
            parser->PopDataValue();
        parser->curfuncnr = oldFunc;
        parser->SetInstance(zSTRING("SELF"), oldSelf);
        return true;
    }

    // --- v92 (nur Othello): Ruf und Spielstart-Bonus beim Partner --------------------------------------------------------
    //
    // Test v91: "Ruf bei Altem Lager nicht synchronisiert beim Client". Der Wert kam an (Spielstaende beider PCs: REPUTATIONOC = 1),
    // es fehlte aber Othellos Meldung "Ruf in dem Alten Lager + 1 = 1": B_IncreaseRep(lager, punkte) (REPUTATIONOC/NC/SC +=,
    // PrintScreenColored) laeuft nur im Dialogskript des Sprechers. Steigt ein Ruf-Wert beim Uebernehmen, ruft GT B_IncreaseRep
    // mit dem Unterschied auf (gleicher Wert, gleiche Meldung) und setzt danach den Wert des Partners.
    static int DialogRepCamp(zCPar_Symbol* sym) {
        if (!sym)
            return 0;
        const char* n = sym->name.ToChar();
        return !strcmp(n, "REPUTATIONOC") ? 1 : !strcmp(n, "REPUTATIONNC") ? 2 : !strcmp(n, "REPUTATIONSC") ? 3 : 0;
    }

    static bool DialogScriptIncreaseRep(int camp, int points) {
        static int idx = -2;
        if (idx == -2) {
            idx = parser->GetIndex(zSTRING("B_INCREASEREP"));
            DialogLog("Ruf: Skript B_INCREASEREP Symbol %d (Othello)", idx);
        }
        return idx >= 0 && DialogRunScript(idx, { { false, camp, NULL }, { false, points, NULL } });
    }

    // Othellos INFO_DIEGO_GAMESTART_INFO gibt dem Helden zuerst den Spielstart-Bonus: je nach DIFFICULTY_LEVEL B_AddBonus(hero, ...)
    // auf Staerke, Geschick, Intelligenz und Regeneration (+5 bzw. +10), HEROSTR... +=, Krit-Multiplikatoren 200, Sprint-Kosten,
    // Player_SetCurrentStamina, Ausdauer-Regeneration. Erst danach folgen die Dialogzeilen (AI_Output mit SELF) und
    // B_Kapitelwechsel(1). Das Gespraech fuehrt nur einer - der andere Held bekam den Bonus nie (Werte des Helden gleicht GT
    // absichtlich nicht ab, v31). Beim Partner laeuft deshalb genau dieser erste Teil: ein RET an den ersten "PUSHINST SELF" in
    // den Bytecode (parser->stack), DoStack ab Funktionsanfang, danach das Byte zurueck. Im Teil davor duerfen nur Spruenge
    // innerhalb des Teils, keine AI_*-Externals und als Skriptaufruf nur B_ADDBONUS stehen - sonst wird nichts ausgefuehrt.
    static bool DialogRunHeroPart(int funcIdx, const std::string& what) {
        zCPar_Symbol* f = (parser && funcIdx >= 0) ? parser->GetSymbol(funcIdx) : NULL;
        unsigned char* code = parser ? parser->stack.stack : NULL;
        const int size = parser ? parser->stack.stacksize : 0;
        if (!f || !player || !code || f->type != zPAR_TYPE_FUNC || (f->flags & (zPAR_FLAG_EXTERNAL | zPAR_FLAG_RETURN)) || f->ele != 0
            || f->single_intdata <= 0 || f->single_intdata >= size || zCParser::GetParser() != parser) {
            DialogLog("Spielstart-Bonus (%s): Funktion nicht nutzbar - nicht ausgefuehrt", what.c_str());
            return false;
        }
        const int start = f->single_intdata;
        const int selfIdx = parser->GetIndex(zSTRING("SELF"));
        zCPar_Symbol* addBonus = parser->GetSymbol(zSTRING("B_ADDBONUS"));
        int q = start, stop = -1, maxJump = start, bonusCalls = 0;
        const char* why = NULL;
        for (int n = 0; q < size && n < 4000 && !why; n++) {
            const unsigned char op = code[q];
            int arg = 0, len = 1;
            if (op == 61 || op == 62 || op == 64 || op == 65 || op == 67 || op == 75 || op == 76 || op == 80) {
                if (q + 5 > size) { why = "Code-Ende"; break; }
                arg = *(const int*)(code + q + 1);
                len = 5;
            }
            else if (op == 245)
                len = 6;
            if ((op == 67 && arg == selfIdx) || op == 60) {   // PUSHINST SELF (Dialogzeilen) bzw. RET
                stop = q;
                break;
            }
            if (op == 62) {
                zCPar_Symbol* x = parser->GetSymbol(arg);
                if (!x || !strncmp(x->name.ToChar(), "AI_", 3) || !strcmp(x->name.ToChar(), "INTRODUCECHAPTER"))
                    why = "Dialog-External vor den Dialogzeilen";
            }
            else if (op == 61) {
                if (!addBonus || addBonus->single_intdata != arg)
                    why = "anderer Skriptaufruf als B_ADDBONUS";
                else
                    bonusCalls++;
            }
            else if (op == 75 || op == 76)
                maxJump = arg > maxJump ? arg : maxJump;
            q += len;
        }
        if (!why && stop < 0)
            why = "kein Ende gefunden";
        if (!why && stop <= start)
            why = "kein Teil vor den Dialogzeilen";
        if (!why && maxJump > stop)
            why = "Sprung hinter den Teil";
        if (why) {
            DialogLog("Spielstart-Bonus (%s): %s - nicht ausgefuehrt", what.c_str(), why);
            return false;
        }
        const unsigned char saved = code[stop];
        code[stop] = 60;   // RET
        const int oldFunc = parser->curfuncnr;
        parser->curfuncnr = funcIdx;
        parser->DoStack(start);
        parser->curfuncnr = oldFunc;
        code[stop] = saved;
        DialogLog("Spielstart-Bonus (%s): Teil vor den Dialogzeilen fuer den eigenen Helden ausgefuehrt (%d Bytes, %d x B_AddBonus) - "
            "Staerke %d, Geschick %d", what.c_str(), stop - start, bonusCalls, player->GetAttribute(NPC_ATR_STRENGTH),
            player->GetAttribute(NPC_ATR_DEXTERITY));
        return true;
    }

    static int DialogLogEntryFunc = -2;

    static bool DialogScriptLogEntry(zSTRING& topic, zSTRING& entry) {
        if (DialogLogEntryFunc == -2) {
            DialogLogEntryFunc = parser->GetIndex(zSTRING("B_LOGENTRY"));
            DialogLog("Tagebuch: Skript B_LOGENTRY Symbol %d - wird fuer Meldung + Ton benutzt (Rueckfall: nur Eintrag)", DialogLogEntryFunc);
        }
        return DialogRunScript(DialogLogEntryFunc, { { true, 0, &topic }, { true, 0, &entry } });
    }

    // v31 (Nutzer: "Beide bekommen EP"): Quest-Erfahrung beim Partner ueber das Spielskript B_GiveXP - mit Meldung
    // "Erfahrung +X", Stufenaufstieg, Lernpunkten, Boni. Othello: B_GiveXP(add_xp, printColor), Vanilla G1: B_GiveXP(add_xp).
    // Erster Parameter = EP, weitere int-Parameter 0 (Othello: 0 = weisse Meldung wie im Normalfall).
    static bool DialogScriptGiveXP(int xp) {
        static int idx = -2;
        if (idx == -2) {
            idx = parser->GetIndex(zSTRING("B_GIVEXP"));
            DialogLog("Erfahrung: Skript B_GIVEXP Symbol %d", idx);
        }
        zCPar_Symbol* f = idx >= 0 ? parser->GetSymbol(idx) : NULL;
        if (!f || f->ele < 1 || f->ele > 4)
            return false;
        std::vector<CoopScriptArg> args;
        for (int k = 0; k < (int)f->ele; k++)
            args.push_back({ false, k == 0 ? xp : 0, NULL });
        return DialogRunScript(idx, args);
    }

    static oCLogTopic* DialogFindTopic(const std::string& t) {
        oCLogManager& lm = oCLogManager::GetLogManager();
        for (auto* it = lm.m_lstTopics.GetNextInList(); it; it = it->GetNextInList()) {
            oCLogTopic* tp = it->GetData();
            if (tp && t == tp->m_strDescription.ToChar())
                return tp;
        }
        return NULL;
    }

    static int DialogApplyLog(json& l) {
        static CoopParserExternal create = NULL, status = NULL, add = NULL;
        if (!create || !status || !add) {   // bei Fehlschlag beim naechsten Mal neu suchen
            create = DialogExternal("LOG_CREATETOPIC");
            status = DialogExternal("LOG_SETTOPICSTATUS");
            add = DialogExternal("LOG_ADDENTRY");
            DialogLog("Tagebuch-Externals: LOG_CREATETOPIC %p, LOG_SETTOPICSTATUS %p, LOG_ADDENTRY %p", create, status, add);
        }
        if (!create || !status || !add) {
            DialogLog("Tagebuch-Externals nicht gefunden - Tagebuch nicht abgeglichen");
            return 0;
        }
        // Die Externals holen ihre Parameter ueber zCParser::GetParser() (cur_parser 0x008DDFD4); wir legen sie auf den
        // Stapel von parser (0x008DCE08). Im Spiel sind beide gleich (Speicherabbild) - sonst lieber nichts aufrufen.
        if (zCParser::GetParser() != parser) {
            DialogLog("Tagebuch: aktueller Parser %p ist nicht der Spiel-Parser %p - nicht abgeglichen", zCParser::GetParser(), parser);
            return 0;
        }
        int n = 0;
        for (auto& jt : l) {
            const std::string t = jt.at("t").get<std::string>();
            zSTRING topic(t.c_str());
            oCLogTopic* tp = DialogFindTopic(t);
            if (!tp) {
                DialogPushString(&topic);
                DialogPushInt(jt.contains("c") ? jt["c"].get<int>() : 0);
                create();
                tp = DialogFindTopic(t);
                n++;
            }
            if (!tp)
                continue;
            if (jt.contains("s") && jt["s"].get<int>() != (int)tp->m_enuStatus) {
                DialogPushString(&topic);
                DialogPushInt(jt["s"].get<int>());
                status();
                n++;
            }
            if (jt.contains("e")) {
                std::set<std::string> have;
                for (auto* e = tp->m_lstEntries.GetNextInList(); e; e = e->GetNextInList())
                    if (oCLogEntry* le = e->GetData())
                        have.insert(le->m_strDescription.ToChar());
                for (auto& je : jt["e"]) {
                    const std::string s = je.get<std::string>();
                    if (have.count(s))
                        continue;
                    zSTRING entry(s.c_str());
                    if (!DialogScriptLogEntry(topic, entry)) {   // v30: wie im Spiel mit Meldung + Ton
                        DialogPushString(&topic);
                        DialogPushString(&entry);
                        add();
                    }
                    have.insert(s);
                    n++;
                }
            }
        }
        return n;
    }

    static CoopDlgTopic StoryTopicNow(oCLogTopic* tp) {
        CoopDlgTopic s;
        s.t = tp->m_strDescription.ToChar();
        s.sec = (int)tp->m_enuSection;
        s.st = (int)tp->m_enuStatus;
        for (auto* e = tp->m_lstEntries.GetNextInList(); e; e = e->GetNextInList())
            if (oCLogEntry* le = e->GetData())
                s.e.push_back(le->m_strDescription.ToChar());
        return s;
    }

    // v94: Kapitel-Bildschirm des Partners erst nach dessen Gespraech. Test v93: "ch" kam mitten in Diegos Begruessung
    // (INFO_DIEGO_GAMESTART laeuft nur beim Host) - beim Host zeigt Gothic ihn erst nach dem Dialog (CGameManager::Run 0x00424F7A
    // nur bei MenuEnabled 0x00429140, im Dialog aus), beim Mitspieler lief kein Dialog, er kam sofort. Darum warten, solange der
    // Absender hier eine Gespraechssperre hat (DialogLocks) oder seine letzte Zeile noch laeuft (DialogEnds); hoechstens 5 min.
    // Allgemein G1.
    struct CoopChapterPending {
        std::string by;
        json ch;
        long long since;
    };
    static std::vector<CoopChapterPending> ChapterPending;

    static bool DialogPartnerTalking(const std::string& by) {
        for (auto& l : DialogLocks)
            if (l.second.owner == by)
                return true;
        for (auto& d : DialogEnds)
            if (d.by == by)
                return true;
        return false;
    }

    // CGameManager::IntroduceChapter (0x00429A20, Eingang unveraendert) merkt Titel, Untertitel, Bild, Ton und Dauer nur vor;
    // Gothic zeigt ihn im naechsten Bild ohne Dialog
    static void DialogShowChapter(json& c, const std::string& by, const char* how) {
        if (!gameMan || !c.is_array() || c.size() != 5)
            return;
        const std::string t = c[0].is_string() ? c[0].get<std::string>() : std::string();
        gameMan->IntroduceChapter(zSTRING(t.c_str()), zSTRING(c[1].is_string() ? c[1].get<std::string>().c_str() : ""),
            zSTRING(c[2].is_string() ? c[2].get<std::string>().c_str() : ""), zSTRING(c[3].is_string() ? c[3].get<std::string>().c_str() : ""),
            c[4].is_number() ? c[4].get<int>() : 6000);
        DialogLog("Kapitel-Bildschirm von %s: %s%s", by.c_str(), t.c_str(), how);
    }

    static void DialogChapterPulse() {
        for (auto it = ChapterPending.begin(); it != ChapterPending.end();) {
            const bool late = CurrentMs > it->since + 300000;
            if (!late && DialogPartnerTalking(it->by)) {
                ++it;
                continue;
            }
            CoopChapterPending p = *it;
            it = ChapterPending.erase(it);
            DialogShowChapter(p.ch, p.by, late ? " (nach 5 min ohne Gespr\xE4" "chsende)" : " (nach seinem Gespr\xE4" "ch)");
        }
    }

    static void DialogApplyState(json& ein, const std::string& by, bool sameWorld) {
        if (!parser)
            return;
        json e = ein;   // v75: Kopie - wird je nach Abgleich-Regel gefiltert, das Paket selbst bleibt (Weiterleiten)
        DlgBuildIntSlots();
        int told = 0, vars = 0, logs = 0, npcs = 0;
        // v75: Abgleich-Regeln des Hosts - "Quests" (erzaehlte Dialoge, Skript-Variablen, NPC-Zustaende, Gildenhaltungen, Quest-EP)
        // und "Tagebuch" einzeln. Was aus ist, wird hier nicht uebernommen.
        const bool quests = CoopRuleQuests(), diary = CoopRuleLog();
        if (!quests || !diary) {
            json f = json::object();
            for (auto it = e.begin(); it != e.end(); ++it) {
                const std::string k = it.key();
                if ((!quests && (k == "i" || k == "v" || k == "p" || k == "g" || k == "xp" || k == "ch")) || (!diary && k == "l"))
                    continue;
                f[k] = it.value();
            }
            static CoopDiagBudget budget = { 0, 0 };
            if (f.size() != e.size() && CoopDiagAllow(budget, 20))
                DialogLog("Fortschritt von %s: %s%s nicht uebernommen (Einstellung Abgleich)", by.c_str(), quests ? "" : "Quest-Teil ",
                    diary ? "" : "Tagebuch");
            e = f;
        }
        oCInfo* toldInfo = NULL;
        if (e.contains("i") && e["i"].is_string()) {
            const int idx = parser->GetIndex(zSTRING(e["i"].get<std::string>().c_str()));
            oCInfo* info = (idx >= 0 && ogame->GetInfoManager()) ? ogame->GetInfoManager()->GetInformation(idx) : NULL;
            if (info && !info->told) {
                info->told = TRUE;
                told = 1;
                toldInfo = info;
            }
        }
        std::string varNames;   // v92: Namen im Log (Test v91: welche Werte kamen an?)
        if (e.contains("v"))
            for (auto& v : e["v"]) {
                zCPar_Symbol* sym = parser->GetSymbol(zSTRING(v.at(0).get<std::string>().c_str()));
                const int k = v.at(1).get<int>();
                const int ele = sym && sym->ele > 1 ? (int)sym->ele : 1;
                const int val = v.at(2).get<int>();
                if (!sym || sym->type != zPAR_TYPE_INT || (sym->flags & (zPAR_FLAG_CONST | zPAR_FLAG_CLASSVAR)) || k < 0 || k >= ele
                    || !StoryValueOk(val) || StoryIsHeroVar(sym))   // v31: Heldenwerte nie uebernehmen (auch von aelteren Fassungen)
                    continue;
                int old = 0;
                sym->GetValue(old, k);
                const int camp = DialogRepCamp(sym);   // v92 (nur Othello): Ruf mit Othellos Meldung
                if (camp && k == 0 && val > old && DialogScriptIncreaseRep(camp, val - old))
                    DialogLog("Ruf: %s %d -> %d ueber B_IncreaseRep (Meldung wie beim Partner)", sym->name.ToChar(), old, val);
                sym->SetValue(val, k);
                vars++;
                if (varNames.size() < 300)
                    varNames += (varNames.empty() ? "" : ", ") + std::string(sym->name.ToChar()) + (ele > 1 ? "[" + std::to_string(k) + "]" : "")
                        + "=" + std::to_string(val);
                auto slot = DlgSlotIndex.find(std::make_pair(sym, k));   // kein Echo im Story-Takt
                if (StoryReady && slot != DlgSlotIndex.end() && slot->second < (int)StoryBaseInts.size())
                    StoryBaseInts[slot->second] = val;
            }
        // v92: Kapitel-Bildschirm des Partners (IntroduceChapter, siehe CoopExtIntroduceChapter)
        // v94 (Test v93: "Kapitel 1 hat beim Client direkt getriggert"): erst, wenn das Gespraech des Absenders hier zu Ende ist
        if (e.contains("ch") && e["ch"].is_array() && e["ch"].size() == 5 && gameMan) {
            if (DialogPartnerTalking(by)) {
                ChapterPending.push_back({ by, e["ch"], CurrentMs });
                DialogLog("Kapitel-Bildschirm von %s: %s - erst nach seinem Gespr\xE4" "ch", by.c_str(),
                    e["ch"][0].is_string() ? e["ch"][0].get<std::string>().c_str() : "?");
            }
            else
                DialogShowChapter(e["ch"], by, "");
        }
        if (e.contains("l")) {
            logs = DialogApplyLog(e["l"]);
            if (StoryReady)
                for (auto& jt : e["l"]) {
                    const std::string t = jt.at("t").get<std::string>();
                    if (oCLogTopic* tp = DialogFindTopic(t))
                        StoryBaseLog[t] = StoryTopicNow(tp);
                }
        }
        if (e.contains("p") && sameWorld)
            for (auto& jn : e["p"]) {
                oCNpc* npc = DialogNpcByName(jn.at("n").get<std::string>());
                if (!npc || npc == player)
                    continue;
                if (jn.contains("a"))
                    for (auto& a : jn["a"]) {
                        const int i = a.at(0).get<int>();
                        if (i >= 0 && i < 50)
                            npc->aiscriptvars[i] = a.at(1).get<int>();
                    }
                if (jn.contains("at"))
                    npc->SetAttitude(jn["at"].get<int>());
                if (jn.contains("tt"))
                    npc->SetTmpAttitude(jn["tt"].get<int>());
                int rtnNow = npc->daily_routine;
                if (jn.contains("r")) {
                    const int idx = parser->GetIndex(zSTRING(jn["r"].get<std::string>().c_str()));
                    if (idx > 0 && npc->daily_routine != idx)
                        npc->state.ChangeRoutine(idx);
                    if (idx > 0)
                        rtnNow = idx;
                }
                if (StoryReady)   // v31: kein Echo im Story-Takt
                    StoryBaseNpc[jn.at("n").get<std::string>()] = { rtnNow, npc->attitude };
                npcs++;
            }
        int xpGiven = 0;
        if (e.contains("xp") && e["xp"].is_number()) {   // v31: Quest-EP auch fuer den eigenen Helden
            const int xp = e["xp"].get<int>();
            if (xp > 0 && xp < 1000000) {
                if (DialogScriptGiveXP(xp))
                    xpGiven = xp;
                else if (player) {   // Rueckfall ohne Skript: nur die Erfahrung, ohne Stufenaufstieg/Meldung
                    player->experience_points += xp;
                    xpGiven = xp;
                    DialogLog("Erfahrung: B_GIVEXP nicht nutzbar - %d EP direkt gutgeschrieben (ohne Stufenaufstieg)", xp);
                }
            }
        }
        int guilds = 0;
        if (e.contains("g")) {   // v31: Gildenhaltungen (global)
            oCGuilds* g = ogame->GetGuilds();
            for (auto& x : e["g"]) {
                const int g1 = x.at(0).get<int>(), g2 = x.at(1).get<int>(), v = x.at(2).get<int>();
                if (!g || !g->attitude || g1 < 0 || g2 < 0 || g1 >= g->size || g2 >= g->size || v < 0 || v > 255)
                    continue;
                g->SetAttitude(g1, g2, (unsigned char)v);
                if (StoryReady && g->size == StoryGuildSize && (size_t)(g2 * g->size + g1) < StoryBaseGuilds.size())
                    StoryBaseGuilds[g2 * g->size + g1] = (unsigned char)v;
                guilds++;
            }
        }
        DialogLog("%s von %s uebernommen (%s): Info %s, %d Variablen, %d Tagebuch-Aenderungen, %d NPCs, %d Gildenhaltungen, %d EP%s%s",
            e.contains("q") ? "Story" : "Fortschritt", by.c_str(),
            e.contains("i") ? e["i"].get<std::string>().c_str() : "-", told ? "neu" : "-", vars, logs, npcs, guilds, xpGiven,
            varNames.empty() ? "" : " - ", varNames.c_str());
        // v92 (nur Othello): Diegos Spielstart - der eigene Held bekam den Bonus nicht (das Gespraech fuehrte der Partner)
        if (toldInfo && !_stricmp(toldInfo->name.ToChar(), "INFO_DIEGO_GAMESTART"))
            DialogRunHeroPart(toldInfo->pd.information, "INFO_DIEGO_GAMESTART");
    }

    // --- Story-Abgleich ausserhalb von Dialogen (v28) ---------------------------------------------
    //
    // Jede Sekunde: globale Skript-Ganzzahlen und Tagebuch gegen den Vergleichsstand. Neue Aenderungen (Buch gelesen,
    // Trigger, Quest-NPC tot, Skript ...) gehen als DIALOG_STATE {q = 1, v, l} an den Partner. Variablen, die sich mehr als
    // STORY_NOISY_MAX mal in 30 s aendern (Hilfsvariablen der KI), sind fuer die Sitzung ausgeschlossen; ebenso Werte, die
    // wie Zeiger aussehen, und coopStoryIgnore aus der Konfiguration.

    static const int STORY_NOISY_MAX = 12;

    static void StoryReadConfig() {
        if (StoryCfgRead)
            return;
        StoryCfgRead = true;
        // fest ausgeschlossen: beschreibt den eigenen Helden, nicht die Geschichte (Log v28: TALKING bei jedem Dialog)
        StoryIgnore.insert("TALKING");
        StoryIgnore.insert("MOVE_ENTRY");   // v30: Anzeige-Umschalter in B_LogEntry (wechselt bei jedem Eintrag, auch beim Anwenden)
        if (CoopConfig.contains("coopStorySync") && CoopConfig["coopStorySync"].is_boolean())
            StorySyncOn = CoopConfig["coopStorySync"].get<bool>();
        if (CoopConfig.contains("coopStoryIgnore") && CoopConfig["coopStoryIgnore"].is_array())
            for (auto const& v : CoopConfig["coopStoryIgnore"])
                if (v.is_string()) {
                    zSTRING s(v.get<std::string>().c_str());
                    s.Upper();
                    StoryIgnore.insert(s.ToChar());
                }
        DialogLog("Story-Abgleich %s, %u Variablen ausgeschlossen (coopStoryIgnore)", StorySyncOn ? "an" : "AUS",
            (unsigned)StoryIgnore.size());
    }

    static void StoryTakeBaseline() {
        DlgBuildIntSlots();
        StoryBaseInts.resize(DlgIntSlots.size());
        for (size_t i = 0; i < DlgIntSlots.size(); i++) {
            int v = 0;
            DlgIntSlots[i].sym->GetValue(v, DlgIntSlots[i].idx);
            StoryBaseInts[i] = v;
        }
        if (StoryNoisy.size() != DlgIntSlots.size()) {
            StoryNoisy.assign(DlgIntSlots.size(), 0);
            StoryChangeTimes.assign(DlgIntSlots.size(), std::deque<long long>());
        }
        StoryBaseLog.clear();
        std::vector<CoopDlgTopic> log;
        DlgSnapLog(log);
        for (auto& t : log)
            StoryBaseLog[t.t] = t;
        StorySnapNpcs();
        StorySnapGuilds();
        StoryReady = true;
        DialogLog("Story-Abgleich: Vergleichsstand aufgenommen (%u Variablen, %u Tagebuch-Themen)",
            (unsigned)StoryBaseInts.size(), (unsigned)StoryBaseLog.size());
    }

    static void StoryTick() {
        if (!StoryReady) {
            StoryTakeBaseline();
            return;
        }
        DlgBuildIntSlots();
        if (!StoryReady || StoryBaseInts.size() != DlgIntSlots.size()) {
            StoryTakeBaseline();
            return;
        }
        json v = json::array();
        int nv = 0;
        for (size_t i = 0; i < DlgIntSlots.size(); i++) {
            int cur = 0;
            DlgIntSlots[i].sym->GetValue(cur, DlgIntSlots[i].idx);
            const int old = StoryBaseInts[i];
            if (cur == old)
                continue;
            StoryBaseInts[i] = cur;
            if (StoryNoisy[i])
                continue;
            const char* nm = DlgIntSlots[i].sym->name.ToChar();
            if (StoryIgnore.count(nm))
                continue;
            auto& times = StoryChangeTimes[i];
            times.push_back(CurrentMs);
            while (!times.empty() && times.front() < CurrentMs - 30000)
                times.pop_front();
            if ((int)times.size() > STORY_NOISY_MAX) {
                StoryNoisy[i] = 1;
                DialogLog("Story-Abgleich: %s aendert sich staendig - fuer diese Sitzung ausgeschlossen", nm);
                continue;
            }
            if (!StoryValueOk(cur) || !StoryValueOk(old)) {
                StoryNoisy[i] = 1;
                DialogLog("Story-Abgleich: %s haelt grosse Werte (%d) - vermutlich Zeiger, ausgeschlossen", nm, cur);
                continue;
            }
            v.push_back({ nm, DlgIntSlots[i].idx, cur });
            nv++;
        }
        json l = json::array();
        int nl = 0;
        oCLogManager& lm = oCLogManager::GetLogManager();
        for (auto* it = lm.m_lstTopics.GetNextInList(); it; it = it->GetNextInList()) {
            oCLogTopic* tp = it->GetData();
            if (!tp)
                continue;
            const std::string name = tp->m_strDescription.ToChar();
            auto f = StoryBaseLog.find(name);
            if (f != StoryBaseLog.end() && f->second.st == (int)tp->m_enuStatus
                && (int)f->second.e.size() == tp->m_lstEntries.GetNumInList())
                continue;   // unveraendert (Status und Anzahl) - Eintraege nicht einzeln kopieren
            const CoopDlgTopic t = StoryTopicNow(tp);
            json jt;
            if (DlgTopicDiff(f == StoryBaseLog.end() ? NULL : &f->second, t, jt)) {
                l.push_back(jt);
                nl++;
            }
            StoryBaseLog[name] = t;
        }
        // v31: Tagesablauf und dauerhafte Haltung der Welt-NPCs (aendern nur Skripte: Npc_ExchangeRoutine, Npc_SetPermAttitude
        // - auch ausserhalb von Dialogen, z. B. B_Kapitelwechsel, B_CheckDeadMissionNPCs, Story-Funktionen)
        json p = json::array();
        int np = 0;
        std::string npcNames;
        for (auto* li = CoopWorldNpcListFirst(ogame->GetGameWorld()); li; li = li->next) {
            oCNpc* npc = li->data;
            if (StoryNpcSkip(npc))
                continue;
            const std::string name = DialogNpcName(npc);
            if (name.empty())
                continue;
            const int rtn = npc->daily_routine, att = npc->attitude;
            auto f = StoryBaseNpc.find(name);
            if (f == StoryBaseNpc.end()) {   // neu in der Liste (z. B. gerade eingefuegt) - nur merken
                StoryBaseNpc[name] = { rtn, att };
                continue;
            }
            if (f->second.rtn == rtn && f->second.att == att)
                continue;
            json jn;
            if (f->second.rtn != rtn)
                if (zCPar_Symbol* rs = parser->GetSymbol(rtn))
                    jn["r"] = rs->name.ToChar();
            if (f->second.att != att)
                jn["at"] = att;
            f->second = { rtn, att };
            if (jn.empty() || StoryNpcNoisy.count(name))
                continue;
            auto& times = StoryNpcTimes[name];
            times.push_back(CurrentMs);
            while (!times.empty() && times.front() < CurrentMs - 30000)
                times.pop_front();
            if (times.size() > 6) {
                StoryNpcNoisy.insert(name);
                DialogLog("Story-Abgleich: NPC %s wechselt Tagesablauf/Haltung staendig - fuer diese Sitzung ausgeschlossen", name.c_str());
                continue;
            }
            jn["n"] = name;
            p.push_back(jn);
            np++;
            if (npcNames.size() < 200)
                npcNames += (npcNames.empty() ? "" : ", ") + name;
        }
        // v31: Gildenhaltungen (B_SetGuildAttitude, B_InitGuildAttitudes bei Kapitelwechseln)
        json gl = json::array();
        int ng = 0;
        oCGuilds* gu = ogame->GetGuilds();
        if (gu && gu->attitude && gu->size == StoryGuildSize && StoryGuildSize > 0
            && StoryBaseGuilds.size() == (size_t)(StoryGuildSize * StoryGuildSize)) {
            for (int i = 0; i < StoryGuildSize * StoryGuildSize; i++)
                if (gu->attitude[i] != StoryBaseGuilds[i]) {
                    gl.push_back({ i % StoryGuildSize, i / StoryGuildSize, (int)gu->attitude[i] });
                    StoryBaseGuilds[i] = gu->attitude[i];
                    ng++;
                }
        }
        else
            StorySnapGuilds();
        if (!nv && !nl && !np && !ng)
            return;
        json j;
        j["q"] = 1;
        if (nv)
            j["v"] = v;
        if (nl)
            j["l"] = l;
        if (np)
            j["p"] = p;
        if (ng)
            j["g"] = gl;
        DialogSend(j, SYNC_DIALOG_STATE);
        std::string names;
        for (auto& x : v) {
            if (names.size() > 200)
                break;
            names += (names.empty() ? "" : ", ") + x.at(0).get<std::string>();
        }
        DialogLog("Story-Abgleich: %d Variablen (%s), %d Tagebuch-Themen, %d NPCs (%s), %d Gildenhaltungen -> Partner",
            nv, names.c_str(), nl, np, npcNames.c_str(), ng);
    }

    // --- Host-Ereignisse (v33): Spawns und Trigger laufen ueber den Host ----------------------------
    //
    // Nutzer: "ueber den Host natuerlich, aber wenn der Client solche Events durch Dialoge triggert, sollte es ueber den Host
    // laufen". Test v32: eine Fleischwanze des Hosts erschien beim Client nicht - GT gleicht nur NPCs ab, die auf beiden PCs mit
    // demselben Namen existieren; spaeter erzeugte heissen beim Host "<Objekt>-DYNAMIC-<Wegpunkt>-n" (Helpers.cpp) und waren
    // beim Client unbekannt (Log "unbekannter NPC-Name im Paket").
    // - Skript-Externals WLD_INSERTNPC, WLD_SENDTRIGGER, WLD_SENDUNTRIGGER: Funktionszeiger im Symbol (Inhaltsfeld +0x18)
    //   durch eigene ersetzt - zCParser::DoStack liest ihn bei jedem CALLEXTERN neu (GetStackPos -> call; 0x006E91ED).
    //   Client: Parameter wie das Original vom Stapel holen (GetParameter, letzter zuerst), Anfrage an den Host, NICHT selbst
    //   ausfuehren. Host: ausfuehren; Trigger gehen danach an alle Clients ("trigger"/"untrigger", dort Original ausfuehren).
    // - Neue NPCs beim Host (jede Quelle: Skript, Kapitel, Beschwoerung): GT-Namen, die nach dem Erstlauf von
    //   BuildGlobalNpcList entstehen, gehen als "spawn" {n, inst, Position} an die Clients; der Client legt den NPC an
    //   (CreateVob + spawnman->InsertNpc, wie GT bei der Coop-Figur) und gibt ihm genau diesen Namen. Alle 30 s wiederholt der
    //   Host die Liste (lebende): ein Client, der inzwischen einen Host-Spielstand geladen hat, hat denselben NPC unter einem
    //   anderen (Erstlauf-)Namen - er wird dann umbenannt (gleiche Instanz, hoechstens 3 m), sonst neu angelegt.
    typedef int(__cdecl* CoopParserExternal2)();
    static CoopParserExternal2 OrigWldInsertNpc = NULL, OrigWldSendTrigger = NULL, OrigWldSendUntrigger = NULL;
    static bool HostEvInstalled = false;
    static int HostEvApplying = 0;
    struct CoopSpawnInfo {
        oCNpc* npc;
        std::string inst;
    };
    static std::map<std::string, CoopSpawnInfo> SpawnDynamic;   // Host: nach dem Laden erzeugte NPCs
    static std::set<std::string> SpawnKnown;
    static std::set<std::string> SpawnFromHost;                 // Client: vom Host vergebene Namen
    static bool SpawnBaselineDone = false;
    static long long SpawnNextMs = 0, SpawnResendMs = 0;

    static bool HostEvForward() {
        return ClientThread && !ServerThread && !HostEvApplying && DialogSyncAktiv();
    }

    static void HostEvSend(json& j) {
        DialogSend(j, SYNC_HOST_EVENT);
    }

    // v41: Laeuft gerade ein Todesskript (SELF ist ein toter NPC, z. B. ZS_Dead)? Jeder Tod beim Client wird beim Host mit
    // eigenem ZS_Dead nachvollzogen (Angriffe, Verfolgungspunkt, Todesmeldung) - Spawns/Trigger daraus nicht noch einmal anfragen.
    static bool HostEvInDeathScript() {
        if (CoopReplayDeathDepth > 0)
            return true;
        zCPar_Symbol* s = parser ? parser->GetSymbol(zSTRING("SELF")) : NULL;
        oCNpc* self = s ? (oCNpc*)s->offset : NULL;
        return self && self != player && self->IsDead();
    }

    static int __cdecl CoopExtWldInsertNpc() {
        if (!HostEvForward())
            return OrigWldInsertNpc ? OrigWldInsertNpc() : 0;
        zCParser* p = zCParser::GetParser();
        zSTRING wp;
        int inst = 0;
        p->GetParameter(wp);   // letzter Parameter zuerst
        p->GetParameter(inst);
        zCPar_Symbol* s = p->GetSymbol(inst);
        if (HostEvInDeathScript()) {   // v41: Todesskript - der Host fuehrt diesen Spawn mit seinem eigenen ZS_Dead aus und meldet ihn
            DialogLog("Host-Ereignis: Wld_InsertNpc(%s, %s) aus einem Todesskript - nicht angefragt (der Host spawnt selbst)",
                s ? s->name.ToChar() : "?", wp.ToChar());
            return 0;
        }
        json j;
        j["f"] = "spawnreq";
        j["inst"] = s ? s->name.ToChar() : "";
        j["wp"] = wp.ToChar();
        HostEvSend(j);
        DialogLog("Host-Ereignis: Wld_InsertNpc(%s, %s) -> Anfrage an den Host (Client erzeugt nicht selbst)",
            s ? s->name.ToChar() : "?", wp.ToChar());
        return 0;
    }

    static int CoopHostEvTrigger(CoopParserExternal2 orig, const char* f) {
        if (HostEvForward()) {
            zSTRING v;
            zCParser::GetParser()->GetParameter(v);
            if (HostEvInDeathScript()) {   // v41: Todesskript - der Host loest den Trigger mit seinem eigenen ZS_Dead aus und verteilt ihn
                DialogLog("Host-Ereignis: Wld_Send%s(%s) aus einem Todesskript - nicht angefragt (der Host loest selbst aus)", f[0] == 't' ? "Trigger" : "Untrigger", v.ToChar());
                return 0;
            }
            json j;
            j["f"] = std::string(f) + "req";
            j["s"] = v.ToChar();
            HostEvSend(j);
            DialogLog("Host-Ereignis: Wld_Send%s(%s) -> Anfrage an den Host", f[0] == 't' ? "Trigger" : "Untrigger", v.ToChar());
            return 0;
        }
        if (ServerThread && !HostEvApplying && DialogSyncAktiv() && zCParser::GetParser() == parser) {
            zSTRING v;
            parser->GetParameter(v);
            DialogPushString(&v);   // wieder hinlegen, das Original holt ihn selbst
            const int r = orig ? orig() : 0;
            json j;
            j["f"] = f;
            j["s"] = v.ToChar();
            HostEvSend(j);
            DialogLog("Host-Ereignis: Wld_Send%s(%s) ausgefuehrt und an die Clients", f[0] == 't' ? "Trigger" : "Untrigger", v.ToChar());
            return r;
        }
        return orig ? orig() : 0;
    }

    static int __cdecl CoopExtWldSendTrigger() {
        return CoopHostEvTrigger(OrigWldSendTrigger, "trigger");
    }

    static int __cdecl CoopExtWldSendUntrigger() {
        return CoopHostEvTrigger(OrigWldSendUntrigger, "untrigger");
    }

    // v92 (Test v91: "Kapitel 1 wird nicht angezeigt" beim Client): Kapitel-Bildschirme kommen aus Skripten - B_Kapitelwechsel ruft
    // IntroduceChapter(titel, untertitel, bild, ton, ms); Kapitel 1 z. B. aus Diegos Begruessung (INFO_DIEGO_GAMESTART_INFO), die nur
    // beim Host laeuft. Das External (Engine 0x0065E610 -> CGameManager::IntroduceChapter 0x00429A20, merkt nur vor) wird umgeleitet:
    // Parameter ansehen (letzter zuerst holen, wieder hinlegen), Original aufrufen, als DIALOG_STATE "ch" an den Partner (der Host
    // gibt es wie jeden Fortschritt weiter). Allgemein G1 (IntroduceChapter ist Gothics External).
    static CoopParserExternal2 OrigIntroduceChapter = NULL;

    static int __cdecl CoopExtIntroduceChapter() {
        zCParser* p = zCParser::GetParser();
        if (!p || p != parser || !DialogSyncAktiv() || !CoopRuleQuests())
            return OrigIntroduceChapter ? OrigIntroduceChapter() : 0;
        int ms = 0;
        zSTRING snd, tex, sub, title;
        p->GetParameter(ms);   // letzter Parameter zuerst
        p->GetParameter(snd);
        p->GetParameter(tex);
        p->GetParameter(sub);
        p->GetParameter(title);
        DialogPushString(&title);   // wieder hinlegen, das Original holt sie selbst
        DialogPushString(&sub);
        DialogPushString(&tex);
        DialogPushString(&snd);
        DialogPushInt(ms);
        const int r = OrigIntroduceChapter ? OrigIntroduceChapter() : 0;
        json j;
        j["ch"] = json::array({ title.ToChar(), sub.ToChar(), tex.ToChar(), snd.ToChar(), ms });
        DialogSend(j, SYNC_DIALOG_STATE);
        DialogLog("Kapitel-Bildschirm %s (%s) - auch beim Partner", title.ToChar(), sub.ToChar());
        return r;
    }

    // v49: Npc_CheckInfo(self, important) aus einer Wahrnehmung, die von der Figur eines Mitspielers kommt (OTHER = Figur), meldet
    // "keine Info". Othellos B_InterceptAssessDamage (PERC_ASSESSDAMAGE in vielen Tagesablaeufen) prueft zuerst
    // Npc_CheckInfo(self, 1) und startet bei einer wichtigen Info sofort das Gespraech mit hero (hero.aivar[22] = TRUE,
    // AI_StartState(self, ZS_Talk, 1, "")) statt ZS_ReactToDamage. Trifft der Mitspieler so einen NPC, lief der zum eigenen
    // Helden, um zu reden ("sucht den Helden") - oder brach ab und machte im Tagesablauf weiter ("reagiert nicht"; Log Host:
    // VLK_10024_BUDDLER blieb in ZS_SAW). Mit "keine Info" geht er wie bei jedem menschlichen Angreifer in ZS_ReactToDamage ->
    // ZS_AssessEnemy -> Npc_SetTarget(self, other). Parameter wie das Original vom Stapel (letzter zuerst), Rueckgabe 0.
    static CoopParserExternal2 OrigNpcCheckInfo = NULL;
    static zCPar_Symbol* CheckInfoOtherSym = NULL;
    static zCParser* CheckInfoOtherParser = NULL;

    // v91: Parameter von Npc_CheckInfo(npc, important) ansehen, ohne sie vom Stapel zu nehmen. Datenstapel zCParser +0x58:
    // int[0x400], Stapelzeiger (Index) +0x1000 (zCPar_DataStack::Pop 0x006FB510). Oben liegt important als [Wert, 0x40] bzw.
    // [Adresse, 0x41] (PopDataValue 0x006E8550 ueber GetParameter 0x006F6DF0), darunter die Symbolnummer der Instanz
    // (GetInstance 0x006F6F80: ein Wert, Symbol +0x1C = Instanzzeiger). Im Speicherabbild alle unveraendert.
    static bool CheckInfoPeek(zCParser* p, oCNpc*& self, int& important) {
        const int* st = (const int*)((const char*)p + 0x58);
        const int sp = st[0x400];
        if (sp < 3 || sp > 0x400)
            return false;
        const int tok = st[sp - 1], v = st[sp - 2];
        if (tok == 0x40)
            important = v;
        else if (tok == 0x41 && v)
            important = *(const int*)v;
        else
            return false;
        const int idx = st[sp - 3];
        zCPar_Symbol* s = idx > 0 ? p->GetSymbol(idx) : NULL;
        self = s ? (oCNpc*)s->offset : NULL;
        return true;
    }

    static int __cdecl CoopExtNpcCheckInfo() {
        zCParser* p = zCParser::GetParser();
        if (p != parser || !p)
            return OrigNpcCheckInfo ? OrigNpcCheckInfo() : 0;
        if (CheckInfoOtherParser != parser) {
            CheckInfoOtherParser = parser;
            CheckInfoOtherSym = parser->GetSymbol(zSTRING("OTHER"));
        }
        oCNpc* other = CheckInfoOtherSym ? (oCNpc*)CheckInfoOtherSym->offset : NULL;
        // v91: eigener Held - keine wichtige Info, wenn der NPC gerade mit einem Mitspieler spricht (allgemein, "einer je NPC") oder
        // nach einem gemeinsamen neuen Spiel der Host noch nicht gesprochen hat (Diego geht zum Host, der Client sieht zu)
        if (other && other == player) {
            oCNpc* self = NULL;
            int important = 0;
            if (CheckInfoPeek(p, self, important) && important) {
                const bool busy = self && CoopDialogNpcBusy(self);
                if (busy || CoopNewGameInfoBlocked()) {
                    p->GetParameter(important);   // wie das Original vom Stapel nehmen (letzter zuerst)
                    p->GetInstance();
                    p->SetReturn(0);
                    static unsigned long ownLines = 0;
                    if (ownLines++ < 40)
                        DialogLog("Npc_CheckInfo(%s, 1) -> keine Info (%s)", self ? self->GetObjectName().ToChar() : "?",
                            busy ? "spricht gerade mit einem Mitspieler" : "neues Spiel: der Host spricht zuerst");
                    return 0;
                }
            }
            return OrigNpcCheckInfo ? OrigNpcCheckInfo() : 0;
        }
        if (!other || !PlayerNpcs.count(other))
            return OrigNpcCheckInfo ? OrigNpcCheckInfo() : 0;
        int important = 0;
        p->GetParameter(important);   // letzter Parameter zuerst
        oCNpc* self = (oCNpc*)p->GetInstance();
        p->SetReturn(0);
        static unsigned long lines = 0;
        if (lines++ < 40)
            DialogLog("Npc_CheckInfo(%s, %d) bei Wahrnehmung von %s -> keine Info (sonst ginge der NPC zum eigenen Helden reden)",
                self ? self->GetObjectName().ToChar() : "?", important, other->GetObjectName().ToChar());
        return 0;
    }

    static void HostEvInstallOne(const char* name, CoopParserExternal2& orig, CoopParserExternal2 mine) {
        zCPar_Symbol* s = parser->GetSymbol(zSTRING(name));
        if (!s || s->type != zPAR_TYPE_FUNC || !(s->flags & zPAR_FLAG_EXTERNAL) || !s->single_intdata) {
            DialogLog("Host-Ereignis: External %s nicht gefunden - nicht umgeleitet", name);
            return;
        }
        if ((CoopParserExternal2)s->single_intdata == mine)
            return;
        orig = (CoopParserExternal2)s->single_intdata;
        s->single_intdata = (int)mine;
        DialogLog("Host-Ereignis: External %s umgeleitet (Original %p)", name, orig);
    }

    static void HostEvInstall() {
        if (HostEvInstalled || !parser)
            return;
        HostEvInstalled = true;
        HostEvInstallOne("WLD_INSERTNPC", OrigWldInsertNpc, &CoopExtWldInsertNpc);
        HostEvInstallOne("WLD_SENDTRIGGER", OrigWldSendTrigger, &CoopExtWldSendTrigger);
        HostEvInstallOne("WLD_SENDUNTRIGGER", OrigWldSendUntrigger, &CoopExtWldSendUntrigger);
        HostEvInstallOne("NPC_CHECKINFO", OrigNpcCheckInfo, &CoopExtNpcCheckInfo);   // v49
        HostEvInstallOne("INTRODUCECHAPTER", OrigIntroduceChapter, &CoopExtIntroduceChapter);   // v92
    }

    // Host: ein Original-External mit Parametern ausfuehren (Anfrage eines Clients)
    static void HostEvRunInsertNpc(const std::string& inst, const std::string& wp) {
        const int idx = parser->GetIndex(zSTRING(inst.c_str()));
        if (idx < 0 || !OrigWldInsertNpc || zCParser::GetParser() != parser) {
            DialogLog("Host-Ereignis: Spawn %s an %s nicht moeglich", inst.c_str(), wp.c_str());
            return;
        }
        zSTRING w(wp.c_str());
        DialogPushInt(idx);
        DialogPushString(&w);
        HostEvApplying++;
        OrigWldInsertNpc();
        HostEvApplying--;
    }

    static void HostEvRunTrigger(CoopParserExternal2 orig, const std::string& s) {
        if (!orig || zCParser::GetParser() != parser)
            return;
        zSTRING v(s.c_str());
        DialogPushString(&v);
        HostEvApplying++;
        orig();
        HostEvApplying--;
    }

    static void SpawnSend(const std::string& n, oCNpc* npc, const std::string& inst, bool repeat) {
        const zVEC3 pos = npc->GetPositionWorld();
        json j;
        j["f"] = "spawn";
        j["n"] = n;
        j["inst"] = inst;
        j["x"] = pos[0];
        j["y"] = pos[1];
        j["z"] = pos[2];
        if (repeat)
            j["r"] = 1;
        HostEvSend(j);
    }

    // Host, je Sekunde: neue GT-Namen = neue NPCs; alle 30 s die Liste wiederholen
    static void SpawnPulse() {
        if (!ServerThread || CurrentMs < SpawnNextMs)
            return;
        SpawnNextMs = CurrentMs + 1000;
        if (!SpawnBaselineDone) {
            if (NpcToUniqueNameList.empty())
                return;
            for (auto& p : NpcToUniqueNameList)
                SpawnKnown.insert(p.second.ToChar());
            SpawnBaselineDone = true;
            SpawnResendMs = CurrentMs + 30000;
            DialogLog("Host-Ereignis: %u NPCs beim Laden - spaeter erzeugte gehen an die Clients", (unsigned)SpawnKnown.size());
            return;
        }
        for (auto& p : NpcToUniqueNameList) {
            const std::string n = p.second.ToChar();
            if (!SpawnKnown.insert(n).second)
                continue;
            oCNpc* npc = p.first;
            if (!npc || npc == player || IsCoopPlayer(npc->GetObjectName()))
                continue;
            const std::string inst = npc->GetInstanceName().ToChar();
            SpawnDynamic[n] = { npc, inst };
            SpawnSend(n, npc, inst, false);
            DialogLog("Host-Ereignis: neuer NPC %s (%s) -> Clients", n.c_str(), inst.c_str());
        }
        if (CurrentMs >= SpawnResendMs && !SpawnDynamic.empty()) {
            SpawnResendMs = CurrentMs + 30000;
            for (auto it = SpawnDynamic.begin(); it != SpawnDynamic.end();) {
                auto u = UniqueNameToNpcList.find(string(it->first.c_str()));
                if (u == UniqueNameToNpcList.end() || u->second != it->second.npc) {   // nicht mehr da
                    it = SpawnDynamic.erase(it);
                    continue;
                }
                if (!it->second.npc->IsDead())
                    SpawnSend(it->first, it->second.npc, it->second.inst, true);
                ++it;
            }
        }
    }

    // Client: NPC des Hosts anlegen oder einen vorhandenen passenden umbenennen
    static void SpawnApply(json& e) {
        const std::string n = e.at("n").get<std::string>();
        const std::string inst = e.at("inst").get<std::string>();
        const zVEC3 pos(e.at("x").get<float>(), e.at("y").get<float>(), e.at("z").get<float>());
        if (UniqueNameToNpcList.count(string(n.c_str()))) {
            SpawnFromHost.insert(n);
            return;   // schon da (gleicher Name)
        }
        // Nur bei der Wiederholung (r): vorhandener NPC gleicher Instanz in der Naehe, der nach dem Laden eines Host-Stands hier
        // anders heisst. v34: Test v33 zeigte, dass eine frische Meldung eine ANDERE, schon vorhandene Fleischwanze umbenannte
        // ("MEATBUG-OCR_LADDER_TOP_01-1 heisst hier jetzt ...") - frische Meldungen legen deshalb immer neu an, und NPCs, fuer
        // die der Host Pakete schickt (CoopHostActiveNames, CoopDiag.cpp), gehoeren ihm unter ihrem Namen und sind tabu.
        oCNpc* best = NULL;
        float bestD = 300.f;
        for (auto* l = e.contains("r") ? CoopWorldNpcListFirst(ogame->GetGameWorld()) : NULL; l; l = l->next) {
            oCNpc* npc = l->data;
            if (StoryNpcSkip(npc) || npc->IsDead() || inst != npc->GetInstanceName().ToChar())
                continue;
            auto nm = NpcToUniqueNameList.find(npc);
            if (nm != NpcToUniqueNameList.end() && (SpawnFromHost.count(nm->second.ToChar()) || CoopHostActiveNames.count(nm->second.ToChar())))
                continue;
            const zVEC3 d = npc->GetPositionWorld() - pos;
            if (d.Length() < bestD) {
                bestD = d.Length();
                best = npc;
            }
        }
        if (best) {
            auto old = NpcToUniqueNameList.find(best);
            const std::string oldName = old != NpcToUniqueNameList.end() ? old->second.ToChar() : std::string();
            if (!oldName.empty())
                UniqueNameToNpcList.erase(string(oldName.c_str()));
            NpcToUniqueNameList[best] = string(n.c_str());
            UniqueNameToNpcList[string(n.c_str())] = best;
            SpawnFromHost.insert(n);
            DialogLog("Host-Ereignis: NPC %s heisst hier jetzt %s (wie beim Host)", oldName.empty() ? "?" : oldName.c_str(), n.c_str());
            return;
        }
        if (e.contains("r"))
            return;   // Wiederholung: nur umbenennen, nicht nachtraeglich erzeugen (z. B. hier gestorben/entfernt)
        const int idx = parser->GetIndex(zSTRING(inst.c_str()));
        if (idx < 0) {
            DialogLog("Host-Ereignis: Instanz %s unbekannt - %s nicht angelegt", inst.c_str(), n.c_str());
            return;
        }
        oCNpc* npc = dynamic_cast<oCNpc*>(ogame->GetGameWorld()->CreateVob(zTVobType::zVOB_TYPE_NSC, idx));
        if (!npc) {
            DialogLog("Host-Ereignis: CreateVob(%s) lieferte keinen NPC", inst.c_str());
            return;
        }
        zVEC3 p = pos;
        // v43: InsertNpc prueft den Abstand zu camPos (eigener Held) und setzt weit weg nichts ein - Test v42: Molerat 80 m vom
        // Client "da, nicht in der Welt", danach nie gefuehrt. Im Umkreis eines Mitspielers wie die Figur einsetzen
        // (CoopSpawnInsertAt), sonst in die Spawn-Liste (SpawnNpc 0x006D0710, GD3D11 ruft das Original zuerst) - dann erscheint
        // er, sobald jemand in die Naehe kommt.
        const char* how = "eingesetzt";
        bool inWorld = CoopFollowActive() && CoopFollowNear(p, (float)BROADCAST_DISTANCE) && CoopSpawnInsertAt(npc, p);
        if (inWorld)
            how = "beim Mitspieler eingesetzt";
        else
            inWorld = ogame->spawnman->InsertNpc(npc, p) != 0;
        if (!inWorld) {
            ogame->spawnman->SpawnNpc(npc, p, 0.f);
            how = "weit weg - in der Spawn-Liste";
        }
        NpcToUniqueNameList[npc] = string(n.c_str());
        UniqueNameToNpcList[string(n.c_str())] = npc;
        SpawnFromHost.insert(n);
        DialogLog("Host-Ereignis: NPC %s (%s) vom Host angelegt (%s)", n.c_str(), inst.c_str(), how);
    }

    static void HostEvReceive(json& e, const std::string& by, bool relay) {
        const std::string f = e.contains("f") && e["f"].is_string() ? e["f"].get<std::string>() : std::string();
        if (ServerThread && !relay) {   // Anfrage eines Clients
            if (f == "spawnreq")
                HostEvRunInsertNpc(e.at("inst").get<std::string>(), e.at("wp").get<std::string>());
            else if (f == "triggerreq" || f == "untriggerreq") {
                const std::string s = e.at("s").get<std::string>();
                const bool trig = f == "triggerreq";
                HostEvRunTrigger(trig ? OrigWldSendTrigger : OrigWldSendUntrigger, s);
                json j;
                j["f"] = trig ? "trigger" : "untrigger";
                j["s"] = s;
                HostEvSend(j);
            }
            DialogLog("Host-Ereignis: Anfrage %s von %s ausgefuehrt", f.c_str(), by.c_str());
            return;
        }
        if (ServerThread)
            return;
        if (f == "spawn")
            SpawnApply(e);
        else if (f == "trigger")
            HostEvRunTrigger(OrigWldSendTrigger, e.at("s").get<std::string>());
        else if (f == "untrigger")
            HostEvRunTrigger(OrigWldSendUntrigger, e.at("s").get<std::string>());
    }

    // --- Pakete ------------------------------------------------------------------------------------

    bool CoopDialogSyncHandles(int type) {
        return type >= SYNC_DIALOG_START && type <= SYNC_HOST_EVENT;
    }

    void CoopDialogSyncReceive(json& e, const std::string& id) {
        const int type = e["type"].get<int>();
        try {
            if (!DialogSyncAktiv())
                return;
            // Client-Pakete kommen ohne "by"; der Host verteilt sie mit "by". Die automatisch weitergeleitete
            // Client-Fassung (ohne "by") ignorieren die anderen Clients.
            const bool relay = e.contains("by") && e["by"].is_string();
            const std::string by = relay ? e["by"].get<std::string>() : id;
            if (!ServerThread && !relay)
                return;
            if (by == DialogMe())
                return;
            const bool sameWorld = !(e.contains("w") && e["w"].is_string() && e["w"].get<std::string>() != CoopWorldName());
            if (type == SYNC_HOST_EVENT) {   // v33: Anfragen nicht weiterverteilen - der Host handelt und verteilt das Ergebnis
                if (sameWorld)
                    HostEvReceive(e, by, relay);
                return;
            }
            if (ServerThread && !relay) {
                json j = e;
                j["by"] = id;
                j["id"] = MyselfId.ToChar();
                CoopTraceCount(TRACE_SENT, type);
                ReadyToSendJsons.enqueue(j);
            }
            // Partner in einer anderen Welt - gleichnamige NPCs waeren andere. Das Ende gilt immer (es hebt nur eine
            // vorhandene Sperre auf), der Fortschritt bis auf den NPC-Teil auch (Skriptvariablen/Tagebuch sind global).
            if (!sameWorld && type != SYNC_DIALOG_END && type != SYNC_DIALOG_STATE)
                return;
            const std::string n = e.contains("n") && e["n"].is_string() ? e["n"].get<std::string>() : std::string();
            switch (type) {
            case SYNC_DIALOG_START:
            {
                DialogLocks[n] = { by, CurrentMs };
                DialogDropEnd(n);
                DialogLineQueue.erase(n);
                DialogFreezeStart(n, by);
                DialogLog("%s spricht mit %s - gesperrt, NPC angehalten", by.c_str(), n.c_str());
                break;
            }
            case SYNC_DIALOG_LINE:
            {
                const std::string s = e.at("s").get<std::string>();
                const std::string o = e.at("o").get<std::string>();
                const bool svm = e.contains("svm") && e["svm"].is_number() && e["svm"].get<int>() != 0;
                DialogPlayLine(n, by, s, o, svm);
                break;
            }
            case SYNC_DIALOG_SKIP:
            {
                DialogCutLine(n, by, e.at("s").get<std::string>(), e.at("o").get<std::string>());
                break;
            }
            case SYNC_DIALOG_END:
            {
                auto f = DialogLocks.find(n);
                if (f == DialogLocks.end() || f->second.owner != by)
                    break;
                DialogLocks.erase(f);
                DialogDropEnd(n);
                DialogEnds.push_back({ n, by, CurrentMs });
                DialogLog("%s beendet den Dialog mit %s - frei", by.c_str(), n.c_str());
                CoopNewGameDialogEnded(by, n);   // v91: erstes Gespraech des Hosts nach dem gemeinsamen Start
                break;
            }
            case SYNC_DIALOG_STATE:
            {
                DialogApplyState(e, by, sameWorld);
                break;
            }
            }
        }
        catch (std::exception& ex) {
            CoopErrorLog("Dialog-Sync", ex.what(), CoopUpdateTypeName(type));
        }
    }

    void CoopDialogSyncPeerGone(const std::string& who) {
        for (auto it = DialogLocks.begin(); it != DialogLocks.end();) {
            if (it->second.owner == who) {
                const std::string n = it->first;
                it = DialogLocks.erase(it);
                DialogRelease(n, who);
                DialogLog("%s getrennt - Sperre fuer %s aufgehoben", who.c_str(), n.c_str());
            }
            else
                ++it;
        }
    }

    void CoopDialogSyncWorldLoaded() {
        if (!MyDialogNpc.empty() && (ServerThread || ClientThread)) {   // mitten im Dialog geladen: Sperre beim Partner aufheben
            json j;
            j["n"] = MyDialogNpc;
            DialogSend(j, SYNC_DIALOG_END);
        }
        DialogLocks.clear();
        DialogEnds.clear();
        DialogFrozen.clear();
        ChapterPending.clear();   // v94: nach dem Laden gilt der Bildschirm nicht mehr
        DialogLineQueue.clear();
        DialogLastInjectMs.clear();
        MyDialogNpcPtr = NULL;
        MyDialogNpc.clear();
        LastLineMsg.clear();
        TrackHero = CoopDialogLineTrack();
        TrackNpc = CoopDialogLineTrack();
        StoryReady = false;   // neuer Vergleichsstand nach dem Laden (Init-Skripte abwarten)
        StoryNextMs = GetCurrentMs() + 3000;
        SpawnKnown.clear();   // v33: neue Bestandsaufnahme nach dem Erstlauf von BuildGlobalNpcList
        SpawnDynamic.clear();
        SpawnFromHost.clear();
        CoopHostActiveNames.clear();
        SpawnBaselineDone = false;
        SpawnNextMs = GetCurrentMs() + 2500;
    }

    // --- Coop-Test per Taste (v32, Wunsch des Nutzers: "Simulation mit einem Hotkey") ---------------------
    //
    // Shift + CoopSimKey (Standard F11; F10/F12 = Schnellspeichern/-laden). v32-v36: vier Einzelschritte reihum. Seit v37 ein
    // ganzer Testlauf mit 26 Schritten und Rueckmeldung des Partners (CoopSelfTest.cpp, D12b).
    void CoopSelfTestToggle();   // CoopSelfTest.cpp
    static int CoopSimKey = 0;
    static bool CoopSimKeyWas = true;
    static int CoopSimStep = 0;

    static bool CoopSimShiftDown() {
        if (zinput->KeyPressed(KEY_LSHIFT) || zinput->KeyPressed(KEY_RSHIFT))
            return true;
        HWND fg = GetForegroundWindow();
        DWORD pid = 0;
        if (fg)
            GetWindowThreadProcessId(fg, &pid);
        return pid == GetCurrentProcessId() && (GetAsyncKeyState(VK_SHIFT) & 0x8000);
    }

    void CoopDialogSyncPulse() {
        if (!DialogSyncAktiv())
            return;
        if (!CoopSimKey)
            CoopSimKey = ReadConfigKey("CoopSimKey", "KEY_F11");
        if (CoopSimKey && CoopKeyEdge(CoopSimKey, CoopSimKey == KEY_F11 ? VK_F11 : 0, CoopSimKeyWas) && CoopSimShiftDown()
            && !MyDialogNpcPtr) {
            try {
                CoopSelfTestToggle();
            }
            catch (std::exception& ex) {
                CoopErrorLog("Coop-Test", ex.what(), "Schritt");
            }
        }
        HostEvInstall();   // v33
        try {
            SpawnPulse();
        }
        catch (std::exception& ex) {
            CoopErrorLog("Host-Ereignis", ex.what(), "Spawn");
        }
        StoryReadConfig();
        // v75: Quests und Tagebuch einzeln (Einstellungen -> COOP -> Abgleich, Host gibt vor); coopStorySync aus der json belegt
        // die beiden Menue-Werte nur beim ersten Start vor
        if ((CoopRuleQuests() || CoopRuleLog()) && parser && CurrentMs >= StoryNextMs) {
            StoryNextMs = CurrentMs + 1000;
            try {
                StoryTick();
            }
            catch (std::exception& ex) {
                CoopErrorLog("Story-Abgleich", ex.what(), "Takt");
            }
        }
        TrackOwnDialog();
        if (MyDialogNpcPtr && !MyDialogNpc.empty())
            DialogCheckSkips();
        for (auto& q : DialogLineQueue)
            DialogPumpLines(q.first);
        // beendete Dialoge: NPC freigeben, sobald er (und die Figur) nicht mehr spricht und keine Zeile mehr wartet
        for (auto it = DialogEnds.begin(); it != DialogEnds.end();) {
            oCNpc* npc = DialogNpcByName(it->n);
            oCNpc* fig = DialogFigureOf(it->by);
            auto q = DialogLineQueue.find(it->n);
            const bool waiting = q != DialogLineQueue.end() && !q->second.empty();
            if (CurrentMs > it->since + 15000 || (!waiting && !DialogSpeaking(npc) && !DialogSpeaking(fig))) {
                const CoopDialogEnd end = *it;
                it = DialogEnds.erase(it);
                DialogLineQueue.erase(end.n);
                DialogRelease(end.n, end.by);
            }
            else
                ++it;
        }
        // veraltete Sperren (10 min ohne Ende) aufheben
        for (auto it = DialogLocks.begin(); it != DialogLocks.end();) {
            if (CurrentMs > it->second.since + 600000) {
                const std::string n = it->first, owner = it->second.owner;
                DialogLog("Sperre fuer %s (von %s) veraltet - aufgehoben", n.c_str(), owner.c_str());
                it = DialogLocks.erase(it);
                DialogRelease(n, owner);
                continue;
            }
            ++it;
        }
        if (DialogLocks.empty() && DialogEnds.empty() && !DialogFrozen.empty())
            DialogFrozen.clear();
        DialogChapterPulse();   // v94
    }

    // --- Hooks -------------------------------------------------------------------------------------

    int __fastcall oCNpc_EV_Output_Coop(oCNpc* _this, void* vtable, oCMsgConversation* msg);
    CInvoke<int(__thiscall*)(oCNpc*, oCMsgConversation*)> Ivk_oCNpc_EV_Output_Coop(0x006B1A70, &oCNpc_EV_Output_Coop);
    int __fastcall oCNpc_EV_Output_Coop(oCNpc* _this, void* vtable, oCMsgConversation* msg) {
        const int r = Ivk_oCNpc_EV_Output_Coop(_this, msg);
        if (r)   // v28: erst wenn die Zeile startet - solange das Gegenueber noch spricht, liefert EV_Output 0 (wartet)
            ReportLine(_this, msg, false);
        return r;
    }

    int __fastcall oCNpc_EV_OutputSVM_Coop(oCNpc* _this, void* vtable, oCMsgConversation* msg);
    CInvoke<int(__thiscall*)(oCNpc*, oCMsgConversation*)> Ivk_oCNpc_EV_OutputSVM_Coop(0x006B15A0, &oCNpc_EV_OutputSVM_Coop);
    int __fastcall oCNpc_EV_OutputSVM_Coop(oCNpc* _this, void* vtable, oCMsgConversation* msg) {
        const int r = Ivk_oCNpc_EV_OutputSVM_Coop(_this, msg);
        if (r)
            ReportLine(_this, msg, true);
        return r;
    }

    // Spricht ein Mitspieler gerade mit diesem NPC, darf der eigene Held ihn nicht ansprechen.
    int __fastcall oCNpc_AssessTalk_S_Coop(oCNpc* _this, void* vtable, oCNpc* other);
    CInvoke<int(__thiscall*)(oCNpc*, oCNpc*)> Ivk_oCNpc_AssessTalk_S_Coop(0x006B5F50, &oCNpc_AssessTalk_S_Coop);
    int __fastcall oCNpc_AssessTalk_S_Coop(oCNpc* _this, void* vtable, oCNpc* other) {
        if (other && other == player && _this && !DialogLocks.empty() && DialogSyncAktiv()) {
            auto f = DialogLocks.find(DialogNpcName(_this));
            if (f != DialogLocks.end() && f->second.owner != DialogMe()) {
                if (CurrentMs > DialogBusyMsgMs + 3000) {
                    DialogBusyMsgMs = CurrentMs;
                    ChatLog(string::Combine(CoopTr("%s spricht gerade mit %s.", "%s is talking to %s."), string(f->second.owner.c_str()), string(f->first.c_str())));
                }
                return 0;
            }
        }
        return Ivk_oCNpc_AssessTalk_S_Coop(_this, other);
    }

    // NPC im Dialog eines Mitspielers: ZS-Schleife (Tagesablauf) aussetzen, Warteschlange laeuft weiter.
    int __fastcall oCNpc_States_DoAIState_Coop(oCNpc_States* _this, void* vtable);
    CInvoke<int(__thiscall*)(oCNpc_States*)> Ivk_oCNpc_States_DoAIState_Coop(0x006C5C60, &oCNpc_States_DoAIState_Coop);
    int __fastcall oCNpc_States_DoAIState_Coop(oCNpc_States* _this, void* vtable) {
        if (!DialogFrozen.empty() && _this) {
            oCNpc* npc = _this->npc;
            if (npc && DialogFrozen.count(npc) && !npc->IsDead() && npc->GetWeaponMode() == NPC_WEAPON_NONE)
                return 0;
        }
        return Ivk_oCNpc_States_DoAIState_Coop(_this);
    }

    // Fortschritt: das Dialogskript laeuft synchron in OnInfo / OnChoice.
    void __fastcall oCInformationManager_OnInfo_Coop(oCInformationManager* _this, oCInfo* info);
    CInvoke<void(__fastcall*)(oCInformationManager*, oCInfo*)> Ivk_oCInformationManager_OnInfo_Coop(0x0072D560, &oCInformationManager_OnInfo_Coop);
    void __fastcall oCInformationManager_OnInfo_Coop(oCInformationManager* _this, oCInfo* info) {
        if (!info || !DlgOwnDialog(_this)) {
            Ivk_oCInformationManager_OnInfo_Coop(_this, info);
            return;
        }
        if (!MyDialogNpcPtr)
            TrackOwnDialog();
        CoopDlgSnap a, b;
        DlgSnapshot(a);
        const std::string name = info->name.ToChar();
        Ivk_oCInformationManager_OnInfo_Coop(_this, info);
        try {
            DlgSnapshot(b);
            DlgSendDiff(a, b, name, "Info");
            StoryAbsorb(a, b);
        }
        catch (std::exception& ex) {
            CoopErrorLog("Dialog-Fortschritt", ex.what(), name.c_str());
        }
    }

    void __fastcall oCInformationManager_OnChoice_Coop(oCInformationManager* _this, oCInfoChoice* choice);
    CInvoke<void(__fastcall*)(oCInformationManager*, oCInfoChoice*)> Ivk_oCInformationManager_OnChoice_Coop(0x0072DC70, &oCInformationManager_OnChoice_Coop);
    void __fastcall oCInformationManager_OnChoice_Coop(oCInformationManager* _this, oCInfoChoice* choice) {
        if (!choice || !DlgOwnDialog(_this)) {
            Ivk_oCInformationManager_OnChoice_Coop(_this, choice);
            return;
        }
        CoopDlgSnap a, b;
        DlgSnapshot(a);
        Ivk_oCInformationManager_OnChoice_Coop(_this, choice);
        try {
            DlgSnapshot(b);
            DlgSendDiff(a, b, std::string(), "Auswahl");
            StoryAbsorb(a, b);
        }
        catch (std::exception& ex) {
            CoopErrorLog("Dialog-Fortschritt", ex.what(), "Auswahl");
        }
    }

#else
    bool CoopDialogNpcBusy(oCNpc*) { return false; }
    bool CoopDialogSyncHandles(int) { return false; }
    void CoopDialogSyncReceive(json&, const std::string&) {}
    void CoopDialogSyncPeerGone(const std::string&) {}
    void CoopDialogSyncWorldLoaded() {}
    void CoopDialogSyncPulse() {}
#endif
}
