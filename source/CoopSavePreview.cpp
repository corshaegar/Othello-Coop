// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

    // --- v77: Vorschau in der Liste "Spielstand waehlen" (MENU_COOP_LOAD), 21.10.2026 -----------------------------------------
    //
    // Nutzer: "Vorschaubild der Savegames direkt anzeigen, wenn man mit der Maus rueber geht, und am besten die Bilder von beiden
    // Savegames, also von Host und Client, mit Namen, wenn man ueber unsere Coop-Sicherungen geht. Wir sollten auch eine genauere
    // Beschreibung haben mit Kapitel, Uhrzeit, Tag im Spiel und kurzem Questlog."
    // - Gothics Ladefenster: siehe CoopMenuMouse.cpp (MouseSelect ruft wie HandleEvent HandleSlotChange auf).
    // - COOP-Liste (nur der Host waehlt): rechts zwei Bilder (Host, Mitspieler) mit Namen und je 8 Zeilen (COOP_MENU-v77.D).
    //   Bild wie oCMenuSavegame::HandleSlotChange (0x0042C420): eine oCSavegameInfo, ReloadResources (0x00434380) laedt
    //   <Spielstaende><m_Dir>THUMB.SAV in m_ThumbTex, zCView::InsertBack(zCTexture*) (0x006FC600) legt es auf den Eintrag.
    //   Angelegt ueber zCObject::CreateNewInstance("oCSavegameInfo") - nur Othello: OTHELLO_ABI vergroessert die Klasse (0xA0 statt
    //   0x90 Bytes, Schwierigkeit in SAVEINFO.SAV; Aufruf in _CreateNewInstance +0x2B umgeleitet), "new" mit SDK-Groesse waere zu klein.
    //   Angaben: COOPINFO.TXT (ab v76: Kapitel, Tag/Uhrzeit, Spielzeit, Held, Aufgaben), sonst SAVEINFO.SAV (ASCII-Archiv: Titel,
    //   Welt, Tag, Uhrzeit, Speicherdatum, Spielzeit).
    // - Bild und Angaben des Mitspielers: der Host fragt im Hauptmenue je Kennung (LOBBY "pv"), der Client antwortet aus seiner
    //   eigenen Kopie ("pvd": THUMB.SAV gepackt, COOPINFO.TXT, SAVEINFO.SAV) - abgelegt in <Spielstaende>\coop_vorschau\<Kennung>__<PC>\
    //   (ausserhalb von coop_archiv: das Archiv bleibt unveraendert, die F9-Liste zeigt den Ordner nicht).

#if ENGINE == Engine_G1

    static void PvLog(const char* fmt, ...) {
        char buf[1024];
        va_list ap;
        va_start(ap, fmt);
        _vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        buf[sizeof(buf) - 1] = 0;
        OthelloCompatLog("[%lld s, %s] Vorschau-Liste: %s", DiagSeconds(), DiagRole(), buf);
    }

    struct CoopPvInfo {
        std::map<std::string, std::string> kv;   // COOPINFO.TXT
        std::vector<std::string> quests;
        std::string title, world, saveDate;      // SAVEINFO.SAV
        int day = -1, hour = 0, min = 0, play = -1;
        bool hasInfo = false, hasSaveInfo = false;
    };

    struct CoopPvAsk {
        long long askMs = 0;
        bool none = false;        // Mitspieler hat keine eigene Kopie dieser Kennung
    };

    static std::map<std::string, oCSavegameInfo*> PvTex;   // Ordner relativ zum Spielstandordner -> Info mit Vorschaubild
    static std::map<std::string, CoopPvAsk> PvAsk;         // Kennung -> Anfrage beim Mitspieler
    static int PvShownRow = -2;
    static zCMenu* PvShownMenu = NULL;
    static std::string PvShownKey;
    static long long PvShownMs = 0;
    static bool PvDirty = false;

    static std::string PvRoot() {
        const std::string r = CoopSavesDir() + "coop_vorschau\\";
        CreateDirectoryA(r.c_str(), NULL);
        return r;
    }

    static std::string PvPartnerDir(const std::string& sid, const std::string& pc) {
        return PvRoot() + sid + "__" + pc + "\\";
    }

    static std::string PvPcOfRole(const std::string& role) {
        for (auto const& e : CoopPcRole)
            if (e.second == role)
                return e.first;
        return std::string();
    }

    // Vorschaubild eines Ordners (absolut, im Spielstandordner) - wie HandleSlotChange: oCSavegameInfo::ReloadResources
    static zCTexture* PvThumb(const std::string& absDir) {
        const std::string root = CoopSavesDir();
        if (absDir.size() <= root.size() || _strnicmp(absDir.c_str(), root.c_str(), root.size()) != 0)
            return NULL;
        const std::string rel = absDir.substr(root.size());
        oCSavegameInfo*& info = PvTex[SaveSyncUpper(rel)];
        if (!info) {
            info = zDYNAMIC_CAST<oCSavegameInfo>(zCObject::CreateNewInstance(zSTRING("oCSavegameInfo")));
            if (!info) {
                PvLog("oCSavegameInfo nicht angelegt");
                return NULL;
            }
            info->m_Dir = zSTRING(rel.c_str());
        }
        if (!info->m_ThumbTex && CoopFileExists(absDir + "THUMB.SAV"))
            info->ReloadResources();
        return info->m_ThumbTex;
    }

    // Bild eines Ordners neu laden (Daten des Mitspielers sind angekommen)
    static void PvThumbForget(const std::string& absDir) {
        const std::string root = CoopSavesDir();
        if (absDir.size() <= root.size())
            return;
        auto it = PvTex.find(SaveSyncUpper(absDir.substr(root.size())));
        if (it != PvTex.end() && it->second)
            it->second->CleanResources();
    }

    static std::string PvReadText(const std::string& path) {
        std::vector<std::uint8_t> d;
        if (!CoopReadWholeFile(path, d) || d.empty())
            return std::string();
        return std::string((const char*)&d[0], d.size());
    }

    static void PvParse(const std::string& coopInfo, const std::string& saveInfo, CoopPvInfo& out) {
        out = CoopPvInfo();
        size_t s = 0;
        while (s < coopInfo.size()) {
            size_t e = coopInfo.find('\n', s);
            if (e == std::string::npos)
                e = coopInfo.size();
            std::string l = coopInfo.substr(s, e - s);
            s = e + 1;
            while (!l.empty() && (l[l.size() - 1] == '\r'))
                l.erase(l.size() - 1);
            const size_t eq = l.find('=');
            if (eq == std::string::npos)
                continue;
            const std::string k = l.substr(0, eq), v = l.substr(eq + 1);
            if (k == "aufgabe")
                out.quests.push_back(v);
            else
                out.kv[k] = v;
        }
        out.hasInfo = out.kv.count("end") > 0;
        // SAVEINFO.SAV: zCArchiverGeneric ASCII, Zeilen "Name=typ:wert"
        s = 0;
        while (s < saveInfo.size()) {
            size_t e = saveInfo.find('\n', s);
            if (e == std::string::npos)
                e = saveInfo.size();
            std::string l = saveInfo.substr(s, e - s);
            s = e + 1;
            while (!l.empty() && (l[l.size() - 1] == '\r' || l[l.size() - 1] == ' '))
                l.erase(l.size() - 1);
            size_t b = 0;
            while (b < l.size() && (l[b] == '\t' || l[b] == ' '))
                b++;
            const size_t eq = l.find('=', b), colon = eq == std::string::npos ? eq : l.find(':', eq);
            if (colon == std::string::npos)
                continue;
            const std::string k = l.substr(b, eq - b), v = l.substr(colon + 1);
            out.hasSaveInfo = true;
            if (k == "Title") out.title = v;
            else if (k == "WorldName") out.world = v;
            else if (k == "TimeDay") out.day = atoi(v.c_str());
            else if (k == "TimeHour") out.hour = atoi(v.c_str());
            else if (k == "TimeMin") out.min = atoi(v.c_str());
            else if (k == "SaveDate") out.saveDate = v;
            else if (k == "PlayTimeSeconds") out.play = atoi(v.c_str());
        }
    }

    static std::string PvGet(const CoopPvInfo& in, const char* k) {
        auto it = in.kv.find(k);
        return it != in.kv.end() ? it->second : std::string();
    }

    // v78: Spieltag und Uhrzeit wie Gothics Ladefenster aus SAVEINFO.SAV (COOPINFO.TXT "tag" zaehlt ab 0)
    static std::string PvWhen(const CoopPvInfo& in) {
        char b[200];
        const std::string kap = PvGet(in, "kapitel");
        std::string tag, zeit;
        if (in.day >= 0) {
            tag = std::to_string(in.day);
            _snprintf(b, sizeof(b), "%02d:%02d", in.hour, in.min);
            b[sizeof(b) - 1] = 0;
            zeit = b;
        }
        else {
            const std::string t = PvGet(in, "tag");
            tag = t.empty() ? std::string("?") : std::to_string(atoi(t.c_str()) + 1);
            zeit = PvGet(in, "uhrzeit");
        }
        if (!kap.empty() && kap != "-1")
            _snprintf(b, sizeof(b), CoopTr("Kapitel %s, Tag %s, %s Uhr", "Chapter %s, day %s, %s"), kap.c_str(), tag.c_str(), zeit.empty() ? "?" : zeit.c_str());
        else
            _snprintf(b, sizeof(b), CoopTr("Tag %s, %s Uhr", "Day %s, %s"), tag.c_str(), zeit.empty() ? "?" : zeit.c_str());
        b[sizeof(b) - 1] = 0;
        return b;
    }

    static std::string PvPlay(const CoopPvInfo& in) {
        int play = in.play;   // SAVEINFO.SAV (Gothic), sonst COOPINFO.TXT
        if (play < 0 && !PvGet(in, "spielzeit").empty())
            play = atoi(PvGet(in, "spielzeit").c_str());
        if (play < 0)
            return std::string();
        char b[100];   // v95: kurz ("Spielzeit 1:05 h") - die Zeile fasst etwa 30 Zeichen
        if (play < 3600)
            _snprintf(b, sizeof(b), CoopTr("Spielzeit %d min", "Played %d min"), play / 60);
        else
            _snprintf(b, sizeof(b), CoopTr("Spielzeit %d:%02d h", "Played %d:%02d h"), play / 3600, (play / 60) % 60);
        b[sizeof(b) - 1] = 0;
        std::string s = b;
        std::string welt = !PvGet(in, "welt").empty() ? PvGet(in, "welt") : in.world;
        const size_t dot = welt.find_last_of('.');
        if (dot != std::string::npos)
            welt = welt.substr(0, dot);
        if (!welt.empty() && SaveSyncUpper(welt) != "WORLD")   // nur andere Welten nennen (Hauptwelt ist der Normalfall)
            s += ", " + welt;
        return s;
    }

    // "Name: Stufe 3, Gardist" - der Held eines PCs
    static std::string PvHero(const std::string& name, const CoopPvInfo& in) {
        if (!in.hasInfo)
            return name + CoopTr(": " COOP_AE "lterer Stand", ": older save");
        std::string gilde = PvGet(in, "gildenname");   // Othellos Gildenname (deutsch, aus dem Spielstand)
        if (gilde.empty() || gilde == "(keine)")
            gilde = PvGet(in, "gilde") == "0" ? std::string(CoopTr("ohne Gilde", "no guild")) : CoopTr("Gilde ", "guild ") + PvGet(in, "gilde");
        return name + CoopTr(": Stufe ", ": level ") + PvGet(in, "stufe") + ", " + gilde;
    }

    // v78: Gothic rechnet Menues in der Aufloesung aus Gothic.ini (hier 640x480), die kleine Schrift (font_old_10_white) fasst in
    // 1500 Menue-Einheiten nur etwa 15 Zeichen (Test v77: "Kapitel 1 T"). Darum unter den beiden Bildern 8 Zeilen ueber die volle
    // Breite (3150, etwa 32 Zeichen): Welt-Angaben (bei einem gemeinsamen Stand gleich) einmal, dazu je eine Zeile je Held, die
    // Aufgaben des Hosts. LobbySetItemText kuerzt jede Zeile auf die Breite des Eintrags.
    static void PvSetPic(zCMenu* menu, const char* id, zCTexture* tex, const std::string& alt) {
        zCMenuItem* it = LobbyFindItem(menu, id);
        if (!it)
            return;
        it->InsertBack(tex);   // zCView::InsertBack(zCTexture*) - wie HandleSlotChange; NULL nimmt das Bild weg
        LobbySetItemText(it, tex ? std::string() : alt);
    }

    static void PvSetLines(zCMenu* menu, const std::vector<std::string>& lines) {
        for (int i = 0; i < 8; i++) {
            char id[64];
            _snprintf(id, sizeof(id), "MENUITEM_COOPLOAD_INFO%d", i);
            id[sizeof(id) - 1] = 0;
            LobbySetItemText(LobbyFindItem(menu, id), i < (int)lines.size() ? lines[i] : std::string());
        }
    }

    // Host: Daten des Mitspielers zu einer Kennung anfragen (hoechstens alle 15 s je Kennung)
    static void PvRequest(const std::string& sid, bool force) {
        CoopPvAsk& a = PvAsk[sid];
        const long long now = GetCurrentMs();
        if (!force && a.askMs && now < a.askMs + 15000)
            return;
        a.askMs = now;
        json j;
        j["op"] = "pv";
        j["sid"] = sid;
        LobbySend(j);
    }

    // Nach dem Aufbau der Liste: Daten des Mitspielers fuer alle Zeilen vorab holen
    void CoopPreviewPrefetch() {
        if (!ServerThread || Lobby.peers.empty())
            return;
        const auto& peer = *Lobby.peers.begin();
        const std::string pc = PvPcOfRole(peer.first);
        int n = 0;
        for (auto const& e : Lobby.rows) {   // v93: Staende aus dem Coop-Archiv
            const std::string& sid = e.sid;
            if (sid.empty() || !peer.second.have.count(sid))
                continue;
            if (!pc.empty() && CoopFileExists(PvPartnerDir(sid, pc) + "COOPPV.TXT"))
                continue;
            PvRequest(sid, false);
            n++;
        }
        if (n)
            PvLog("%d Vorschauen beim Mitspieler angefragt", n);
    }

    // v93: Durchlauf "JJJJ-MM-TT_hh-mm-ss_Host_Mitspieler" lesbar
    // v95 (Test v94: "dieses 'Durchlauf aelterer Stand' ... zu lang und passt nicht rein"): kurz "Durchlauf vom 22.10. 12:20";
    // aeltere Staende ohne Durchlauf bekommen keine Zeile (leer)
    static std::string PvRunLabel(const std::string& run) {
        if (run.size() < 19)
            return std::string();
        return CoopTr("Durchlauf vom ", "Playthrough ") + run.substr(8, 2) + "." + run.substr(5, 2) + ". " + run.substr(11, 2) + ":" + run.substr(14, 2);
    }

    // v92 (Test v91: "Spielstand waehlen ... mit den Vorschaubildern - diese Version brauchen wir" im Esc-Menue): im Spiel gibt es
    // keine Wartephase (Lobby.peers, have-Liste). Partner ist dort beim Host der erste Mitspieler mit bekannter PC-Kennung
    // (CoopPcRole), beim Mitspieler der Host; ob er eine eigene Kopie hat, sagt erst seine Antwort ("pvd" mit none). Die Anfragen
    // laufen im Spiel ueber CoopLobbyReceiveInGame (CoopMenuLobby.cpp), beide Richtungen.
    static std::string PvIngamePartner() {
        if (ClientThread && !ServerThread)
            return "HOST";
        for (auto const& e : CoopPcRole)
            if (e.second != "HOST" && !e.second.empty())
                return e.second;
        return std::string();
    }

    // Jedes Menuebild in MENU_COOP_LOAD (CoopMenuLobby.cpp): markierte Zeile -> Bilder und Angaben beider Seiten
    void CoopSavePreviewPulse(zCMenu* menu) {
        const bool ingame = player != NULL;   // v92: auch im Esc-Menue, fuer Host, Mitspieler und allein
        if (!menu || (!ingame && !ServerThread))
            return;
        if (menu != PvShownMenu) {   // anderes (neu angelegtes) Menue: alles neu setzen
            PvShownMenu = menu;
            PvShownRow = -2;
            PvShownKey.clear();
        }
        int row = PvShownRow;
        const int sel = menu->m_mainSel;
        if (sel >= 0 && sel < menu->m_listItems.GetNum() && menu->m_listItems[sel]) {
            const std::string id = LobbyUpper(menu->m_listItems[sel]->id);
            if (id.compare(0, 21, "MENUITEM_COOPLOAD_ROW") == 0)
                row = atoi(id.c_str() + 21);
        }
        if (row < 0 || row >= (int)Lobby.rows.size()) {
            if (PvShownRow != -1) {   // keine Zeile (leere Liste): Vorschau leeren
                PvShownRow = -1;
                PvShownKey.clear();
                PvSetPic(menu, "MENUITEM_COOPLOAD_PIC_HOST", NULL, " ");
                PvSetPic(menu, "MENUITEM_COOPLOAD_PIC_PARTNER", NULL, " ");
                LobbySetItemText(LobbyFindItem(menu, "MENUITEM_COOPLOAD_NAME_HOST"), std::string());
                LobbySetItemText(LobbyFindItem(menu, "MENUITEM_COOPLOAD_NAME_PARTNER"), std::string());
                PvSetLines(menu, std::vector<std::string>());
            }
            return;
        }
        // v93: Zeile = Stand aus dem Coop-Archiv bzw. Slot (eigene Daten aus e.dir); "both" = der Partner hat ihn auch
        const CoopListEntry& entry = Lobby.rows[row];
        const std::string sid = entry.sid;
        const bool coop = ServerThread || ClientThread;
        const std::string peerRole = ingame ? (coop ? PvIngamePartner() : std::string())
            : (Lobby.peers.empty() ? std::string() : Lobby.peers.begin()->first);
        const bool peerHas = !peerRole.empty() && !sid.empty() && entry.both;
        const std::string pc = PvPcOfRole(peerRole);
        const std::string pdir = pc.empty() || sid.empty() ? std::string() : PvPartnerDir(sid, pc);
        const bool pdata = !pdir.empty() && CoopFileExists(pdir + "COOPPV.TXT");
        const std::string key = sid + "|" + entry.dir + "|" + (pdata ? "1" : "0") + "|" + (peerHas ? "1" : "0");
        const long long now = GetCurrentMs();
        // jede Sekunde neu setzen - oeffnet Gothic dasselbe Menue erneut, setzt es den Hintergrund der Eintraege zurueck
        if (row == PvShownRow && key == PvShownKey && !PvDirty && now < PvShownMs + 1000)
            return;
        PvShownRow = row;
        PvShownKey = key;
        PvShownMs = now;
        PvDirty = false;

        // eigener Stand (links; beim Host "(Host)")
        const std::string hdir = entry.dir;
        CoopPvInfo h;
        PvParse(PvReadText(hdir + "COOPINFO.TXT"), PvReadText(hdir + "SAVEINFO.SAV"), h);
        const std::string hname = MyNickname.ToChar();
        PvSetPic(menu, "MENUITEM_COOPLOAD_PIC_HOST", PvThumb(hdir), CoopTr("kein Bild", "no picture"));
        LobbySetItemText(LobbyFindItem(menu, "MENUITEM_COOPLOAD_NAME_HOST"), ServerThread ? hname + " (Host)" : hname);

        // Partner (rechts) - v95 (Test v94: "Noch kein Mitspieler verbunden ... zu lang"): kurze Zeilen
        std::string pname = peerRole.empty() ? std::string(coop ? CoopTr("kein Partner", "no partner") : CoopTr("ohne Coop", "no coop")) : peerRole;
        std::string pline;
        zCTexture* ptex = NULL;
        if (peerRole.empty())
            pline = coop ? CoopTr("Kein Mitspieler", "No partner") : CoopTr("Ohne Coop", "No coop");
        else if (!peerHas)   // v93: lokaler Stand - der Partner uebernimmt beim Laden die Welt
            pline = pname + (sid.empty() ? CoopTr(": nur hier gespeichert", ": saved only here") : CoopTr(": hat ihn nicht", ": does not have it"));
        else if (pdata) {
            CoopPvInfo p;
            PvParse(PvReadText(pdir + "COOPINFO.TXT"), PvReadText(pdir + "SAVEINFO.SAV"), p);
            const std::string meta = PvReadText(pdir + "COOPPV.TXT");
            const size_t n = meta.find("name=");
            if (n != std::string::npos) {
                std::string v = meta.substr(n + 5);
                const size_t e = v.find_first_of("\r\n");
                if (e != std::string::npos)
                    v = v.substr(0, e);
                if (!v.empty())
                    pname = v;
            }
            if (meta.find("none=1") != std::string::npos)
                pline = pname + CoopTr(": eigene Kopie fehlt", ": own copy missing");
            else {
                pline = PvHero(pname, p);
                ptex = PvThumb(pdir);
            }
        }
        else {
            PvRequest(sid, false);
            pline = pname + CoopTr(": wird geholt ...", ": fetching ...");
        }
        PvSetPic(menu, "MENUITEM_COOPLOAD_PIC_PARTNER", ptex, peerHas && !pdata ? "..." : CoopTr("kein Bild", "no picture"));
        LobbySetItemText(LobbyFindItem(menu, "MENUITEM_COOPLOAD_NAME_PARTNER"), pname);

        // Zeilen: Welt-Angaben (Host), beide Helden, v93 Durchlauf, Aufgaben des Hosts
        // v95 (Test v94: "die Quests sind auch zu lang und passen nicht rein"): Aufgaben auf die freien Zeilen umbrechen (Woerter)
        // statt mitten im Wort mit "..." abzuschneiden; leere Zeilen (z. B. ohne Durchlauf) fallen weg
        std::vector<std::string> lines;
        for (const std::string& l : { PvWhen(h), PvPlay(h), PvHero(hname, h), pline, PvRunLabel(entry.run) })
            if (!l.empty())
                lines.push_back(l);
        if (h.hasInfo) {
            int run = 0, done = 0, fail = 0;
            sscanf(PvGet(h, "aufgaben").c_str(), "%d/%d/%d", &run, &done, &fail);
            std::string a = CoopTr("Aufgaben: ", "Quests: ") + std::to_string(run) + CoopTr(" offen, ", " open, ") + std::to_string(done) +
                CoopTr(" erledigt", " done");
            if (fail > 0)
                a += ", " + std::to_string(fail) + CoopTr(" gesch.", " failed");
            lines.push_back(a);
            zCMenuItem* infoItem = LobbyFindItem(menu, "MENUITEM_COOPLOAD_INFO0");
            for (size_t i = 0; i < h.quests.size() && lines.size() < 8; i++)
                for (const std::string& q : LobbyWrapText(infoItem, "- " + h.quests[i], (int)(8 - lines.size()) < 2 ? 1 : 2))
                    if (lines.size() < 8)
                        lines.push_back(q);
        }
        else if (!h.saveDate.empty())
            lines.push_back(CoopTr("gespeichert ", "saved ") + h.saveDate);
        PvSetLines(menu, lines);
    }

    // LOBBY "pv" (Client) und "pvd" (Host) - aus LobbyReceive (CoopMenuLobby.cpp)
    void CoopPreviewReceive(json& e, const std::string& from, const std::string& op) {
        const std::string sid = e.contains("sid") && e["sid"].is_string() ? e["sid"].get<std::string>() : std::string();
        if (sid.empty() || sid.find_first_of("\\/:") != std::string::npos || sid.find("..") != std::string::npos)
            return;
        if (op == "pv") {   // v92: jede Rolle antwortet (im Spiel fragt auch der Mitspieler den Host)
            json j;
            j["op"] = "pvd";
            j["sid"] = sid;
            j["pc"] = CoopMyPc();
            j["name"] = MyNickname.ToChar();
            const std::string dir = CoopOwnCopyDir(sid);
            if (dir.empty()) {
                j["none"] = 1;
            }
            else {
                std::vector<std::uint8_t> th, packed;
                if (CoopReadWholeFile(dir + "THUMB.SAV", th) && !th.empty()) {
                    if (CoopZipPack(th, packed) && packed.size() + 1024 < th.size()) {
                        j["z"] = 1;
                        j["raw"] = (long long)th.size();
                        j["th"] = json::binary(packed);
                    }
                    else
                        j["th"] = json::binary(th);
                }
                j["info"] = PvReadText(dir + "COOPINFO.TXT");
                j["sinfo"] = PvReadText(dir + "SAVEINFO.SAV");
            }
            LobbySend(j);
            return;
        }
        if (op == "pvd") {   // v92: jede Rolle nimmt Antworten an
            const std::string pc = e.contains("pc") && e["pc"].is_string() ? e["pc"].get<std::string>() : std::string();
            if (!CoopPcIdValid(pc))
                return;
            CoopSaveSyncNotePc(pc, from);   // v92: im Spiel kennt der Mitspieler die PC-Kennung des Hosts sonst evtl. nicht
            const std::string dir = PvPartnerDir(sid, pc);
            CreateDirectoryA(dir.substr(0, dir.size() - 1).c_str(), NULL);
            const bool none = e.contains("none");
            bool thumb = false;
            if (!none && e.contains("th") && e["th"].is_binary()) {
                std::vector<std::uint8_t> data = e["th"].get_binary(), raw;
                if (e.contains("z")) {
                    const size_t rs = e.contains("raw") && e["raw"].is_number() ? (size_t)e["raw"].get<long long>() : 0;
                    if (CoopZipUnpack(data, rs, raw))
                        data.swap(raw);
                    else
                        data.clear();
                }
                if (!data.empty()) {
                    FILE* f = fopen((dir + "THUMB.SAV.tmp").c_str(), "wb");
                    bool ok = f && fwrite(&data[0], 1, data.size(), f) == data.size();
                    if (f && fclose(f) != 0)
                        ok = false;
                    thumb = ok && MoveFileExA((dir + "THUMB.SAV.tmp").c_str(), (dir + "THUMB.SAV").c_str(), MOVEFILE_REPLACE_EXISTING);
                }
            }
            if (!none) {
                if (e.contains("info") && e["info"].is_string())
                    CoopWriteTextAtomic(dir + "COOPINFO.TXT", e["info"].get<std::string>());
                if (e.contains("sinfo") && e["sinfo"].is_string())
                    CoopWriteTextAtomic(dir + "SAVEINFO.SAV", e["sinfo"].get<std::string>());
            }
            const std::string name = e.contains("name") && e["name"].is_string() ? e["name"].get<std::string>() : from;
            CoopWriteTextAtomic(dir + "COOPPV.TXT", "name=" + name + "\n" + (none ? "none=1\n" : "") + "end=1\n");
            PvThumbForget(dir);
            PvAsk[sid].none = none;
            PvDirty = true;
            PvLog("Vorschau von %s zu %s: %s", from.c_str(), sid.c_str(), none ? "keine eigene Kopie" : (thumb ? "Bild und Angaben" : "nur Angaben"));
        }
    }

    // Wartephase vorbei (CoopLobbyAfterLoad): Bilder freigeben
    void CoopPreviewReset() {
        for (auto& e : PvTex) {
            if (!e.second)
                continue;
            e.second->CleanResources();
            e.second->Release();
        }
        PvTex.clear();
        PvAsk.clear();
        PvShownRow = -2;
        PvShownMenu = NULL;
        PvShownKey.clear();
    }

#else
    void CoopPreviewPrefetch() {}
    void CoopSavePreviewPulse(zCMenu*) {}
    void CoopPreviewReceive(json&, const std::string&, const std::string&) {}
    void CoopPreviewReset() {}
#endif
}
