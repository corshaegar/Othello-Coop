// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

    // --- v55: Name, Adresse und eigenes Aussehen aus dem COOP-Menue (20.10.2026) ---------------------------------------------
    //
    // Nutzer: "im Hauptmenue der Reiter COOP ... IP eingeben, IP wird fuer das naechste Mal gemerkt, Port 9955 ist auch
    // einstellbar. Name, HeadModel, BodyModel" - "das Aussehen, das man waehlt, ist fuer sich selbst. Das Aussehen, das der
    // Mitspieler waehlt, wird dann auch dementsprechend angezeigt. Ausserdem wenn neu geladen wird, soll das Aussehen erhalten
    // bleiben, das ist gerade nicht so" - "mit einer Suche, z. B. wenn man Gomez eingibt, kriegt man das Modell".
    //
    // Werte: Gothic.ini [GOTHICCOOP] (schreibt Gothic selbst aus COOP_MENU.D - Eingabefelder zCMenuItemInput::Run 0x004D53A0,
    // Auswahl zCMenuItemChoice): coopServer, coopPort, coopName, coopLookNpc, coopHead, coopFace, coopBodyMesh, coopBody,
    // coopSkin. Fehlt ein Eintrag, gilt GothicCoopConfig.json (server, port, nickname, headVarNr, bodyTextVarNr) und wird
    // einmal nach Gothic.ini geschrieben, damit das Menue ihn zeigt.
    // Aussehen: oCNpc::SetAdditionalVisuals(Koerper, Koerper-Textur, Hautton, Kopf, Gesicht, Zaehne, Ruestung) - wie bisher, nur
    // Kopf, Statur und Hautton waehlbar statt fest (HUM_HEAD_PONY, hum_body_Naked0, 1). Bisher legte GT es dem eigenen Helden nur
    // beim Start von Host/Client und bei Uebernahmen an - nach einem normalen Laden galt wieder das Aussehen aus dem Spielstand.
    // Jetzt nach jedem Laden (Coop an) und sofort bei einer Aenderung im Menue; die Figur beim Partner bekommt es ueber INIT_NPC
    // (headMesh, bodyMesh, skin neu) auch, wenn sie schon steht.
    // Gesichter 0-164 und Koerper 0-14 und ihre Hauttoene: Texturen in Othellos VDFs (HUM_HEAD_V*_C*, HUM_BODY_NAKED_V*_C*) -
    // nur Othello; andere Mods haben andere Bereiche (fuer das allgemeine Coop zur Laufzeit pruefen).
    // NPC-Suche: GT liest den Skriptcode der NPC-Instanzen (nur lesen, nichts ausfuehren) und nimmt die Werte aus deren
    // Aufruf Mdl_SetVisualBody(self, Koerper, Koerper-Textur, Hautton, Kopf, Gesicht, Zaehne, Ruestung) sowie den Namen aus
    // "name = ..." - allgemein fuer G1-Skripte (Instanzen setzen das Aussehen dort direkt).

    std::string CoopServerIp = "127.0.0.1";       // Ziel fuer "Spiel beitreten" (Client.cpp liest nur diese Kopie)
    std::string MyHeadMesh = "HUM_HEAD_PONY";
    std::string MyBodyMesh = "hum_body_Naked0";
    int MySkin = 1;
    static bool CoopLookChanged = false;          // eigenes Aussehen/Name geaendert - INIT_NPC neu senden (Plugin.cpp)

    void CoopLookApplyTo(oCNpc* npc, const std::string& headMesh, int face, const std::string& bodyMesh, int bodyTex, int skin) {
        if (!npc)
            return;
        npc->SetAdditionalVisuals(zSTRING(bodyMesh.c_str()), bodyTex, skin, zSTRING(headMesh.c_str()), face, 0, -1);
    }

#if ENGINE == Engine_G1

    static const char* CoopHeadMeshes[] = { "HUM_HEAD_PONY", "HUM_HEAD_STD_PONY", "HUM_HEAD_FIGHTER", "HUM_HEAD_BALD",
        "HUM_HEAD_FATBALD", "HUM_HEAD_THIEF", "HUM_HEAD_PSIONIC", "HUM_HEAD_FLAIL", "HUM_HEAD_FLEX", "HUM_HEAD_LUTTER",
        "HUM_HEAD_PFEIFFER", "HUM_HEAD_PYMONTE", "HUM_HEAD_THOMAS", "HUM_HEAD_UNICORN" };
    static const int CoopHeadMeshCount = 14;
    static const char* CoopBodyMeshes[] = { "hum_body_Naked0", "Hum_Body_CookSmith" };
    static const int CoopBodyMeshCount = 2;
    static const int CoopFaceMax = 164, CoopBodyMax = 14;

    static void LookLog(const char* fmt, ...) {
        char buf[700];
        va_list ap;
        va_start(ap, fmt);
        _vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        buf[sizeof(buf) - 1] = 0;
        OthelloCompatLog("[%lld s, %s] Aussehen: %s", DiagSeconds(), DiagRole(), buf);
    }

    static std::string LookUpper(std::string s) {
        for (auto& c : s)
            c = (char)toupper((unsigned char)c);
        return s;
    }

    static std::string LookTrim(const std::string& s) {
        size_t a = 0, b = s.size();
        while (a < b && (unsigned char)s[a] <= ' ')
            a++;
        while (b > a && (unsigned char)s[b - 1] <= ' ')
            b--;
        return s.substr(a, b - a);
    }

    // Hauttoene (Bitmaske C0..C3), fuer die es zu einem Gesicht bzw. Koerper eine ECHTE Textur gibt. v65 (Test v64: "manchmal
    // erscheinen die Modelle rot ... Textur z. B. Kopf oder Koerper komplett rot"): Gothics textures.VDF (2001, allgemein G1)
    // enthaelt fuer manche Varianten nur rote 4x4-Platzhalter (DXT1, 44 Bytes): HUM_HEAD_V21-117_C0, HUM_HEAD_V96_C1,
    // HUM_BODY_COOKSMITH_V0-1_C0 - die Tabelle bis v64 zaehlte nur Namen. Neu gezaehlt nach Groesse (neueste VDF gewinnt):
    // Gesichter 0-13: C0-C3, 14-16: C0-C2, 17-20: C0-C1, 21-95/97-117: nur C1, 96: keine, 118-164 (Othello): nur C0;
    // hum_body_Naked0 0-3: C0-C3, 4: C0/C1/C3, 5-14 (Othello): nur C0; Hum_Body_CookSmith nur 0-1 mit C1.
    static int LookFaceSkins(int face) {
        if (face <= 13) return 0xF;
        if (face <= 16) return 0x7;
        if (face <= 20) return 0x3;
        if (face == 96) return 0;
        if (face <= 117) return 0x2;
        return 0x1;
    }

    static int LookBodySkins(int bodyMesh, int body) {
        if (bodyMesh == 1)   // Hum_Body_CookSmith
            return body <= 1 ? 0x2 : 0;
        if (body <= 3) return 0xF;
        if (body == 4) return 0xB;
        return 0x1;
    }

    // naechster Hautton aus der Maske, bei Gleichstand der hellere; -1 = keiner
    static int LookNearestSkin(int mask, int want) {
        for (int d = 0; d <= 3; d++) {
            if (want - d >= 0 && (mask >> (want - d) & 1)) return want - d;
            if (want + d <= 3 && (mask >> (want + d) & 1)) return want + d;
        }
        return -1;
    }

    static int LookNearestFace(int face, int skinBits) {
        for (int d = 0; d <= CoopFaceMax; d++) {
            if (face - d >= 0 && (LookFaceSkins(face - d) & skinBits)) return face - d;
            if (face + d <= CoopFaceMax && (LookFaceSkins(face + d) & skinBits)) return face + d;
        }
        return face;
    }

    static int LookNearestBody(int bodyMesh, int body, int skinBits) {
        for (int d = 0; d <= CoopBodyMax; d++) {
            if (body - d >= 0 && (LookBodySkins(bodyMesh, body - d) & skinBits)) return body - d;
            if (body + d <= CoopBodyMax && (LookBodySkins(bodyMesh, body + d) & skinBits)) return body + d;
        }
        return -1;
    }

    // Gewaehltes Aussehen auf vorhandene Texturen bringen. Kopf und Koerper tragen denselben Hautton (SetAdditionalVisuals).
    // Gesicht und Statur bleiben; zuerst weicht der Hautton aus (der naechste, den Gesicht und Koerper beide haben), dann die
    // Koerper-Textur, zuletzt das Gesicht (nur Statur kraeftig mit einem Gesicht ohne C1)
    static void LookResolve(int bodyMesh, int& face, int& body, int& skin) {
        face = LookNearestFace(face, 0xF);
        const int b = LookNearestBody(bodyMesh, body, 0xF);
        if (b >= 0)
            body = b;
        const int common = LookFaceSkins(face) & LookBodySkins(bodyMesh, body);
        if (common) {
            skin = LookNearestSkin(common, skin);
            return;
        }
        skin = LookNearestSkin(LookFaceSkins(face), skin);
        const int b2 = LookNearestBody(bodyMesh, body, 1 << skin);
        if (b2 >= 0) {
            body = b2;
            return;
        }
        skin = LookNearestSkin(LookBodySkins(bodyMesh, body), skin);
        face = LookNearestFace(face, 1 << skin);
    }

    static const zSTRING& LookSection() {
        static zSTRING sec("GOTHICCOOP");
        return sec;
    }

    static bool IniHas(const char* key) {
        return zoptions && zoptions->EntryExists(LookSection(), key);
    }

    static int IniInt(const char* key, int def) {
        return IniHas(key) ? zoptions->ReadInt(LookSection(), key, def) : def;
    }

    static std::string IniStr(const char* key, const std::string& def) {
        if (!IniHas(key))
            return def;
        zSTRING v = zoptions->ReadString(LookSection(), key, def.c_str());
        return std::string(v.ToChar());
    }

    static void IniSetInt(const char* key, int v) {
        if (zoptions)
            zoptions->WriteInt(LookSection(), key, v, 0);
    }

    static void IniSetStr(const char* key, const std::string& v) {
        if (zoptions)
            zoptions->WriteString(LookSection(), key, zSTRING(v.c_str()), 0);
    }

    // v95: Sprache der Coop-Anzeigen aus Gothic.ini [GOTHICCOOP] coopLang (0 Deutsch, 1 English) - Game_Loop und Game_MenuLoop,
    // hoechstens alle 0,5 s (das Auswahlfeld schreibt Gothic.ini sofort; CoopTr in OthelloCompat.cpp)
    void CoopLangRefresh() {
        static DWORD last = 0;
        const DWORD now = GetTickCount();
        if (last && now - last < 500)
            return;
        last = now;
        const int en = IniInt("coopLang", 0) == 1 ? 1 : 0;
        if (en != CoopLangEn) {
            CoopLangEn = en;
            OthelloCompatLog("Sprache der Coop-Anzeigen: %s (Gothic.ini coopLang=%d)", en ? "English" : "Deutsch", en);
        }
    }

    static std::string JsonStr(const char* key, const std::string& def) {
        return CoopConfig.contains(key) && CoopConfig[key].is_string() ? CoopConfig[key].get<std::string>() : def;
    }

    static int JsonInt(const char* key, int def) {
        return CoopConfig.contains(key) && CoopConfig[key].is_number_integer() ? CoopConfig[key].get<int>() : def;
    }

    static int LookIndexOf(const char* const* list, int count, const std::string& name) {
        const std::string u = LookUpper(name);
        for (int i = 0; i < count; i++)
            if (LookUpper(list[i]) == u)
                return i;
        return -1;
    }

    // Einmal je Start: fehlende Eintraege aus der json vorbelegen (das Menue zeigt sonst leere Felder bzw. die erste Wahl).
    void CoopCamDefaults();   // CoopCamera.cpp (v66): Kamera-Eintraege vorbelegen, damit das Menue sie schon im Hauptmenue zeigt
    void CoopDiffDefaults();  // CoopDifficulty.cpp (v67): Schwierigkeitsgrad ebenso

    static void CoopSettingsDefaults() {
        static bool done = false;
        if (done || !zoptions)
            return;
        done = true;
        int written = 0;
        if (!IniHas("coopServer")) { IniSetStr("coopServer", JsonStr("server", "127.0.0.1")); written++; }
        if (!IniHas("coopPort")) { IniSetStr("coopPort", std::to_string(JsonInt("port", 9955))); written++; }
        if (!IniHas("coopName")) {
            std::string n = JsonStr("nickname", "Spieler");
            if (n.empty() || n == "Your nick")
                n = "Spieler";
            IniSetStr("coopName", n);
            written++;
        }
        if (!IniHas("coopLookNpc")) { IniSetStr("coopLookNpc", ""); written++; }
        if (!IniHas("coopHead")) { IniSetInt("coopHead", 0); written++; }
        if (!IniHas("coopFace")) { IniSetInt("coopFace", JsonInt("headVarNr", MyHeadVarNr)); written++; }
        if (!IniHas("coopBodyMesh")) { IniSetInt("coopBodyMesh", 0); written++; }
        if (!IniHas("coopBody")) { IniSetInt("coopBody", JsonInt("bodyTextVarNr", MyBodyTextVarNr)); written++; }
        if (!IniHas("coopSkin")) { IniSetInt("coopSkin", DefaultBodyTexColorNr); written++; }
        CoopCamDefaults();
        CoopDiffDefaults();
        if (written)
            LookLog("%d Coop-Eintraege in Gothic.ini [GOTHICCOOP] aus GothicCoopConfig.json vorbelegt", written);
    }

    static std::string CoopLookKey() {
        return MyHeadMesh + "|" + std::to_string(MyHeadVarNr) + "|" + MyBodyMesh + "|" + std::to_string(MyBodyTextVarNr) + "|" +
            std::to_string(MySkin) + "|" + std::string(MyNickname.ToChar());
    }

    // Aus Gothic.ini lesen (Menue, Start, vor Host/Client). Rueckgabe: Aussehen oder Name hat sich geaendert.
    bool CoopSettingsRefresh(bool log) {
        if (!zoptions)
            return false;
        CoopSettingsDefaults();
        const std::string before = CoopLookKey();
        CoopServerIp = LookTrim(IniStr("coopServer", JsonStr("server", "127.0.0.1")));
        const int port = atoi(IniStr("coopPort", std::to_string(JsonInt("port", 9955))).c_str());
        if (port > 0 && port < 65536)
            ConnectionPort = port;
        const std::string name = LookTrim(IniStr("coopName", JsonStr("nickname", "Spieler")));
        MyNickname = name.empty() ? "Spieler" : name.c_str();
        int head = IniInt("coopHead", 0);
        int face = IniInt("coopFace", MyHeadVarNr);
        int bodyMesh = IniInt("coopBodyMesh", 0);
        int body = IniInt("coopBody", MyBodyTextVarNr);
        int skin = IniInt("coopSkin", DefaultBodyTexColorNr);
        if (head < 0 || head >= CoopHeadMeshCount) head = 0;
        if (face < 0 || face > CoopFaceMax) face = 0;
        if (bodyMesh < 0 || bodyMesh >= CoopBodyMeshCount) bodyMesh = 0;
        if (body < 0 || body > CoopBodyMax) body = 0;
        if (skin < 0 || skin > 3) skin = 0;
        MyHeadMesh = CoopHeadMeshes[head];
        MyBodyMesh = CoopBodyMeshes[bodyMesh];
        const int wantFace = face, wantBody = body, wantSkin = skin;
        LookResolve(bodyMesh, face, body, skin);
        MyHeadVarNr = face;
        MyBodyTextVarNr = body;
        MySkin = skin;
        const bool changed = CoopLookKey() != before;
        if (changed && log) {
            std::string note;
            if (face != wantFace)
                note += ", Gesicht " + std::to_string(wantFace) + " -> " + std::to_string(face);
            if (body != wantBody)
                note += ", Koerper " + std::to_string(wantBody) + " -> " + std::to_string(body);
            if (skin != wantSkin)
                note += ", Hautton " + std::to_string(wantSkin + 1) + " -> " + std::to_string(skin + 1);
            if (!note.empty())
                note = " (gewaehlt ohne echte Textur" + note + ")";
            LookLog("eingestellt: Name %s, Kopf %s, Gesicht %d, Statur %s, Koerper %d, Hautton %d%s | Host-Adresse %s:%d",
                MyNickname.ToChar(), MyHeadMesh.c_str(), MyHeadVarNr, MyBodyMesh.c_str(), MyBodyTextVarNr, MySkin + 1,
                note.c_str(), CoopServerIp.c_str(), ConnectionPort);
        }
        return changed;
    }

    // Eigenes Aussehen am Helden - nur mit laufendem Coop (Host oder Client), sonst bleibt Othellos Aussehen.
    void CoopLookApplyOwn(const char* why) {
        if (!player || (!ServerThread && !ClientThread) || WorldEditMode)
            return;
        CoopSettingsRefresh(false);
        CoopLookApplyTo(player, MyHeadMesh, MyHeadVarNr, MyBodyMesh, MyBodyTextVarNr, MySkin);
        static CoopDiagBudget budget = { 0, 0 };
        if (CoopDiagAllow(budget, 10))
            LookLog("eigener Held (%s): Kopf %s, Gesicht %d, Statur %s, Koerper %d, Hautton %d", why, MyHeadMesh.c_str(), MyHeadVarNr,
                MyBodyMesh.c_str(), MyBodyTextVarNr, MySkin + 1);
    }

    // Plugin.cpp: Aussehen/Name im Menue geaendert -> INIT_NPC neu senden.
    bool CoopLookTakeChanged() {
        const bool c = CoopLookChanged;
        CoopLookChanged = false;
        return c;
    }

    // Im Spiel und im Menue (alle 500 ms): Aenderungen aus dem Menue uebernehmen.
    void CoopLookPulse() {
        static long long next = 0;
        const long long now = CurrentMs;   // im Spiel (Game_Loop) und im Menue (CoopLobbyPulse) je Takt gesetzt
        if (now < next)
            return;
        next = now + 500;
        if (!CoopSettingsRefresh(true))
            return;
        if (player && (ServerThread || ClientThread)) {
            CoopLookApplyOwn("im Menue geaendert");
            CoopLookChanged = true;
        }
    }

    // --- Suche: Aussehen eines NPCs aus dem Skriptcode ------------------------------------------------------------------

    struct CoopLookFound {
        std::string inst, name, headMesh, bodyMesh;
        int bodyTex = -1, skin = -1, face = -1;
    };

    struct CoopLookVal {
        int kind;              // 0 Zahl, 1 Symbol (PUSHVAR/ARRAYVAR), 2 Instanz
        int i;
        zCPar_Symbol* s;
    };

    static int LookValInt(const CoopLookVal& v) {
        if (v.kind == 0)
            return v.i;
        if (v.kind == 1 && v.s && v.s->type == zPAR_TYPE_INT && (v.s->flags & zPAR_FLAG_CONST)) {
            int r = -1;
            v.s->GetValue(r, 0);
            return r;
        }
        return -1;
    }

    static std::string LookValStr(const CoopLookVal& v) {
        if (v.kind == 1 && v.s && v.s->type == zPAR_TYPE_STRING && (v.s->flags & zPAR_FLAG_CONST)) {
            zSTRING r;
            v.s->GetValue(r, 0);
            return std::string(r.ToChar());
        }
        return std::string();
    }

    static bool LookIsNpcInstance(zCPar_Symbol* s) {
        zCPar_Symbol* p = s ? s->parent : NULL;
        for (int k = 0; k < 6 && p; k++) {
            if (p->type == zPAR_TYPE_CLASS)
                return strcmp(p->name.ToChar(), "C_NPC") == 0;
            p = p->parent;
        }
        return false;
    }

    // Instanz-Code von vorn bis RET lesen (Befehlslaengen wie tools\datdis.py: 1 Byte, 5 Bytes fuer CALL/CALLEXTERN/PUSHINT/
    // PUSHVAR/PUSHINST/JUMP/JUMPF/SETINSTANCE, 6 Bytes fuer PUSHVAR mit Index 245).
    static bool LookDecode(zCPar_Symbol* inst, CoopLookFound& f) {
        const unsigned char* code = parser->stack.stack;
        const int size = parser->stack.stacksize;
        int pos = inst->single_intdata;
        if (!code || size <= 0 || pos < 0 || pos >= size)
            return false;
        std::vector<CoopLookVal> st;
        bool visual = false;
        for (int steps = 0; steps < 3000 && pos < size; steps++) {
            const unsigned char op = code[pos];
            if (op == zPAR_TOK_RET)
                break;
            if (op == zPAR_TOK_CALL || op == zPAR_TOK_CALLEXTERN || op == zPAR_TOK_PUSHINT || op == zPAR_TOK_PUSHVAR ||
                op == zPAR_TOK_PUSHINST || op == zPAR_TOK_JUMP || op == zPAR_TOK_JUMPF || op == zPAR_TOK_SETINSTANCE) {
                if (pos + 5 > size)
                    break;
                int arg;
                memcpy(&arg, code + pos + 1, 4);
                pos += 5;
                if (op == zPAR_TOK_PUSHINT)
                    st.push_back({ 0, arg, NULL });
                else if (op == zPAR_TOK_PUSHVAR || op == zPAR_TOK_PUSHINST)
                    st.push_back({ op == zPAR_TOK_PUSHVAR ? 1 : 2, 0, parser->GetSymbol(arg) });
                else {
                    if (op == zPAR_TOK_CALLEXTERN && !visual && st.size() >= 8) {
                        zCPar_Symbol* fn = parser->GetSymbol(arg);
                        if (fn && strcmp(fn->name.ToChar(), "MDL_SETVISUALBODY") == 0) {
                            const size_t b = st.size() - 8;   // self, Koerper, Koerper-Textur, Hautton, Kopf, Gesicht, Zaehne, Ruestung
                            f.bodyMesh = LookValStr(st[b + 1]);
                            f.bodyTex = LookValInt(st[b + 2]);
                            f.skin = LookValInt(st[b + 3]);
                            f.headMesh = LookValStr(st[b + 4]);
                            f.face = LookValInt(st[b + 5]);
                            visual = !f.headMesh.empty();
                        }
                    }
                    st.clear();
                }
                continue;
            }
            if (op == 245) {   // PUSHVAR mit Feldindex
                if (pos + 6 > size)
                    break;
                int arg;
                memcpy(&arg, code + pos + 1, 4);
                pos += 6;
                st.push_back({ 1, 0, parser->GetSymbol(arg) });
                continue;
            }
            pos += 1;
            if (op == zPAR_OP_UN_MINUS) {
                if (!st.empty() && st.back().kind == 0)
                    st.back().i = -st.back().i;
                continue;
            }
            if (op == zPAR_TOK_ASSIGNSTR && st.size() >= 2 && f.name.empty()) {
                const CoopLookVal& target = st.back();
                if (target.kind == 1 && target.s && strcmp(target.s->name.ToChar(), "C_NPC.NAME") == 0)
                    f.name = LookValStr(st[st.size() - 2]);
            }
            st.clear();
        }
        return visual;
    }

    // Beste Uebereinstimmung: Anzeigename gleich > Instanzname gleich > Anzeigename enthaelt > Instanzname enthaelt.
    static bool LookSearch(const std::string& text, CoopLookFound& out, int& matches) {
        matches = 0;
        const std::string q = LookUpper(LookTrim(text));
        if (q.empty() || !parser)
            return false;
        const int n = parser->symtab.GetNumInList();
        int best = 0;
        for (int i = 0; i < n; i++) {
            zCPar_Symbol* s = parser->GetSymbol(i);
            if (!s || s->type != zPAR_TYPE_INSTANCE || !LookIsNpcInstance(s))
                continue;
            if (strchr(s->name.ToChar(), '.'))
                continue;   // v55b: lokale Variablen in Funktionen (z. B. B_STORY_GORNJOINS.GORN) - kein eigener Code
            CoopLookFound f;
            f.inst = s->name.ToChar();
            if (!LookDecode(s, f))
                continue;
            const std::string disp = LookUpper(f.name), inst = LookUpper(f.inst);
            int score = 0;
            if (disp == q) score = 4;
            else if (inst == q) score = 3;
            else if (!disp.empty() && disp.find(q) != std::string::npos) score = 2;
            else if (inst.find(q) != std::string::npos) score = 1;
            if (!score)
                continue;
            matches++;
            if (score > best) {
                best = score;
                out = f;
            }
        }
        return best > 0;
    }

    // Auswahlfelder im offenen Menue neu aus Gothic.ini lesen (zCMenuItemChoice::InitMenuItem 0x004D7F70, unveraendert laut
    // Speicherabbild; vtable 0x007D2C54 = zCMenuItemChoice). v55b (Nutzer: "man muss zweimal bestaetigen, also nach der Suche
    // nochmal Enter"): InitMenuItem liest nur den Wert (+0x3D8), die Anzeige erneuert erst der naechste Tastendruck. Danach
    // DrawFront ueber den vtable-Platz +0x58 (= zCMenuItemChoice::DrawFront 0x004D79E0, .rdata im Speicher wie in der Datei) -
    // so macht es auch ToggleValue am Ende (0x004D7116).
    static int LookRefreshChoices() {
        zCMenu* menu = zCMenu::GetActive();
        if (!menu)
            return 0;
        static const char* ids[] = { "MENUITEM_COOP_HEAD_CHOICE", "MENUITEM_COOP_FACE_CHOICE", "MENUITEM_COOP_BODYMESH_CHOICE",
            "MENUITEM_COOP_BODY_CHOICE", "MENUITEM_COOP_SKIN_CHOICE" };
        typedef void(__thiscall* VoidFn)(zCMenuItem*);
        int n = 0;
        for (int i = 0; i < menu->m_listItems.GetNum(); i++) {
            zCMenuItem* it = menu->m_listItems[i];
            if (!it || *(DWORD*)it != 0x007D2C54)
                continue;
            const std::string id = LookUpper(std::string(it->id.ToChar()));
            for (const char* want : ids) {
                if (id != want)
                    continue;
                ((VoidFn)0x004D7F70)(it);                          // InitMenuItem
                ((VoidFn)(*(DWORD**)it)[0x58 / 4])(it);            // DrawFront
                n++;
            }
        }
        return n;
    }

    // CoopMenuLobby.cpp: nach dem Eingabefeld "Aussehen wie NPC". Rueckgabe: Hinweis fuer die Statuszeile.
    std::string CoopLookSearchFromMenu() {
        const std::string text = LookTrim(IniStr("coopLookNpc", ""));
        if (text.empty())
            return std::string();
        CoopLookFound f;
        int matches = 0;
        if (!LookSearch(text, f, matches)) {
            LookLog("Suche \"%s\": kein NPC mit Aussehen gefunden", text.c_str());
            return CoopTr("Kein NPC \"", "No NPC \"") + text + CoopTr("\" gefunden.", "\" found.");
        }
        const int head = LookIndexOf(CoopHeadMeshes, CoopHeadMeshCount, f.headMesh);
        const int body = LookIndexOf(CoopBodyMeshes, CoopBodyMeshCount, f.bodyMesh);
        if (head >= 0) IniSetInt("coopHead", head);
        if (body >= 0) IniSetInt("coopBodyMesh", body);
        if (f.face >= 0 && f.face <= CoopFaceMax) IniSetInt("coopFace", f.face);
        if (f.bodyTex >= 0 && f.bodyTex <= CoopBodyMax) IniSetInt("coopBody", f.bodyTex);
        if (f.skin >= 0 && f.skin <= 3) IniSetInt("coopSkin", f.skin);
        LookLog("Suche \"%s\": %d Treffer, uebernommen von %s (%s): Kopf %s%s, Gesicht %d, Statur %s%s, Koerper %d, Hautton %d",
            text.c_str(), matches, f.inst.c_str(), f.name.empty() ? "-" : f.name.c_str(), f.headMesh.c_str(), head < 0 ? " (nicht in der Liste)" : "",
            f.face, f.bodyMesh.c_str(), body < 0 ? " (nicht in der Liste)" : "", f.bodyTex, f.skin + 1);
        const int shown = LookRefreshChoices();
        if (shown < 5)
            LookLog("Auswahlfelder neu gezeichnet: %d von 5 (Menue %s)", shown, zCMenu::GetActive() ? zCMenu::GetActive()->name.ToChar() : "-");
        CoopSettingsRefresh(true);
        if (player && (ServerThread || ClientThread)) {
            CoopLookApplyOwn("wie NPC");
            CoopLookChanged = true;
        }
        std::string who = f.name.empty() ? f.inst : f.name + " (" + f.inst + ")";
        if (CoopLangEn)
            return "Look of " + who + " applied" + (matches > 1 ? " - " + std::to_string(matches) + " matches, be more specific for others." : ".");
        return "Aussehen von " + who + " " + "\xFC" "bernommen" + (matches > 1 ? " - " + std::to_string(matches) + " Treffer, genauer eingeben f\xFC" "r andere." : ".");
    }

#else
    bool CoopSettingsRefresh(bool) {
        CoopServerIp = CoopConfig.contains("server") && CoopConfig["server"].is_string() ? CoopConfig["server"].get<std::string>() : std::string("127.0.0.1");
        return false;
    }
    void CoopLookApplyOwn(const char*) {
        if (player && (ServerThread || ClientThread))
            CoopLookApplyTo(player, MyHeadMesh, MyHeadVarNr, MyBodyMesh, MyBodyTextVarNr, DefaultBodyTexColorNr);
    }
    bool CoopLookTakeChanged() { return false; }
    void CoopLookPulse() {}
    std::string CoopLookSearchFromMenu() { return std::string(); }
    void CoopLangRefresh() {}
#endif
}
