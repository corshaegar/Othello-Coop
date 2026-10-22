// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	

    // interpolation for translation
    zVEC3 Lerp(zVEC3 a, zVEC3 b, float t)
    {
        return a + (b - a) * t;
    }

    // min max limit for a float variable
    void zClamp(float& value, float bl, float bh) {
        if (value < bl)
            value = bl;
        else if (value > bh)
            value = bh;
    }

    // --- Robustheit gegenueber stark gepatchten Mods ---------------------------------
    // Total conversions (hier gemessen an "Gothic: Othello") koennen Engine-Adressen mit
    // anderem Code belegen und Objekte in Zustaenden hinterlassen, die der Vanilla-Code
    // nicht erwartet. Die folgenden Helfer pruefen beides, bevor darauf vertraut wird.

    // Ist der Speicher an p ueber bytes lesbar? Rein ueber die Seitenverwaltung, damit es
    // auch fuer bereits freigegebene Adressen funktioniert.
    bool IsReadableMemory(const void* p, size_t bytes) {
        if (!p)
            return false;
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(p, &mbi, sizeof(mbi)) != sizeof(mbi))
            return false;
        if (mbi.State != MEM_COMMIT)
            return false;
        if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))
            return false;
        const unsigned char* end = (const unsigned char*)mbi.BaseAddress + mbi.RegionSize;
        return (const unsigned char*)p + bytes <= end;
    }

    // Sieht v wie ein lebendes zCObject aus? Ausgerichtet, lesbar, und der Vtable-Zeiger
    // zeigt in das Engine-Image. Messung vom 16.10.2026 an einem frisch erzeugten
    // Coop-NPC unter Othello: visual == 0xA6180FE7, nicht einmal zeigerausgerichtet.
    // KORREKTUR 16.10.2026: Die Pruefung verlangte frueher, dass die Vtable im Engine-Image
    // (0x00400000-0x00900000) liegt. Das ist zu eng und war nachweislich schaedlich:
    // Othello rendert mit GD3D11, und das setzt eigene Visual-Klassen ein, deren Vtables in
    // g1_avx2.dll liegen - weit ausserhalb. Gueltige Visuals wurden so verworfen, der
    // Coop-NPC verlor sein Modell und stand unsichtbar in der Welt (gemessen:
    // visual=0 model=0 homeWorld gueltig, Position korrekt, 363 Einheiten neben dem Helden).
    // Eine Vtable liegt immer in einem GELADENEN MODUL - genau das pruefen wir jetzt, statt
    // einen festen Adressbereich anzunehmen.
    bool LooksLikeLiveVisual(const void* v) {
        if (((size_t)v & 3) != 0)
            return false;
        if (!IsReadableMemory(v, sizeof(void*)))
            return false;
        const void* vtbl = *(const void* const*)v;
        if (((size_t)vtbl & 3) != 0)
            return false;
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(vtbl, &mbi, sizeof(mbi)) != sizeof(mbi))
            return false;
        if (mbi.State != MEM_COMMIT)
            return false;
        if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))
            return false;
        return mbi.Type == MEM_IMAGE;
    }

    // Liegt an addr noch die gemeinte Engine-Funktion? marker ist ein Immediate, das nur
    // in ihrem Rumpf vorkommt (z. B. der Zeiger auf ihren Debug-String).
    bool FunctionLooksIntact(size_t addr, unsigned int marker, size_t window = 0x40) {
        if (!IsReadableMemory((const void*)addr, window))
            return false;
        const unsigned char* p = (const unsigned char*)addr;
        for (size_t i = 0; i + sizeof(unsigned int) <= window; i++) {
            unsigned int v;
            memcpy(&v, p + i, sizeof(v));
            if (v == marker)
                return true;
        }
        return false;
    }

	Common::Map<zSTRING, int> MdsHumansMap;

	void CheckMdsMap() {
		static int check = 0;
		if (!check) {
			check = 1;
			MdsHumansMap.Insert("HUMANS.MDS", 0);
			MdsHumansMap.Insert("HUMANS_RELAXED.MDS", 1);
			MdsHumansMap.Insert("HUMANS_1HST2.MDS", 2);
			MdsHumansMap.Insert("HUMANS_2HST2.MDS", 3);
			MdsHumansMap.Insert("HUMANS_BOWT2.MDS", 4);
			MdsHumansMap.Insert("HUMANS_CBOWT2.MDS", 5);
			MdsHumansMap.Insert("HUMANS_MILITIA.MDS", 6);
			MdsHumansMap.Insert("HUMANS_1HST3.MDS", 7);
			MdsHumansMap.Insert("HUMANS_2HST3.MDS", 8);
			MdsHumansMap.Insert("HUMANS_ARROGANCE.MDS", 9);
			MdsHumansMap.Insert("HUMANS_1HST1.MDS", 10);
			MdsHumansMap.Insert("HUMANS_2HST1.MDS", 11);
			MdsHumansMap.Insert("HUMANS_BOWT1.MDS", 12);
			MdsHumansMap.Insert("HUMANS_CBOWT1.MDS", 13);
			MdsHumansMap.Insert("HUMANS_SPST2.MDS", 14);
			MdsHumansMap.Insert("HUMANS_ACROBATIC.MDS", 15);
			MdsHumansMap.Insert("HUMANS_MAGESPRINT.MDS", 16);
			MdsHumansMap.Insert("HUMANS_TIRED.MDS", 17);
			MdsHumansMap.Insert("HUMANS_MAGE.MDS", 18);
			MdsHumansMap.Insert("HUMANS_SKELETON.MDS", 19);
			MdsHumansMap.Insert("HUMANS_SKELETON_FLY.MDS", 20);
			MdsHumansMap.Insert("HUMANS_BABE.MDS", 21);
			MdsHumansMap.Insert("SHIELD.MDS", 22);
			MdsHumansMap.Insert("HUMANS_PIRATE.MDS", 23);
			MdsHumansMap.Insert("HUMANS_ARR.MDS", 24);
			MdsHumansMap.Insert("SHIELD_ST1.MDS", 25);
			MdsHumansMap.Insert("HUMANS_SPST1.MDS", 26);
			MdsHumansMap.Insert("HUMANS_TRD.MDS", 27);
			MdsHumansMap.Insert("HUMANS_SIT_EAT.MDS", 28);
			MdsHumansMap.Insert("HUMANS_SIT_DRINK.MDS", 29);
			MdsHumansMap.Insert("SHIELD_ST2.MDS", 30);
			MdsHumansMap.Insert("HUMANS_REL.MDS", 31);
			MdsHumansMap.Insert("HUMANS_AXEST2.MDS", 32);
			MdsHumansMap.Insert("HUMANS_NEWTORCH.MDS", 33);

		}
	}

	int GetMdsIndex(zSTRING mds) {
		CheckMdsMap();
		mds.Upper();

		auto arr = MdsHumansMap.GetArray();
		for (int i = 0; i < arr.GetNum(); i++) {
			if (auto pair = arr.GetSafe(i)) {
				if (mds == pair->GetKey()) {
					return pair->GetValue();
				}
			}
		}


		// Die feste Liste oben kennt nur Vanilla-Overlays (ueberwiegend Gothic 2). Unter Gothic 1
		// und Othello fehlen u. a. HUMANS_TORCH, _WOUNDED, _DRUNKEN, _SPRINT, _SWIM, _FLEE sowie
		// Othellos eigene (_BARREL, _SAC, ...). Unbekannte Namen bekommen eine LOKALE Nummer ab 100;
		// uebertragen wird zusaetzlich der Name (LocalNpc SYNC_OVERLAYS), die Gegenseite loest
		// ihn mit ihrer eigenen Nummer auf. Die festen Nummern 0..33 bleiben unveraendert.
		if (mds.IsEmpty())
			return -1;
		static int nextLocalMdsIndex = 100;
		MdsHumansMap.Insert(mds, nextLocalMdsIndex);
		return nextLocalMdsIndex++;
	}

	zSTRING GetMdsByIndex(int index) {
		CheckMdsMap();
		auto arr = MdsHumansMap.GetArray();
		for (int i = 0; i < arr.GetNum(); i++) {
			if (auto pair = arr.GetSafe(i)) {
				if (index == pair->GetValue()) {
					return pair->GetKey();
				}
			}
		}

		return "";
	}

	zCArray<int> GetNpcMds(oCNpc* npc) {
		zCArray<int> arr;
		if (npc && npc->GetModel()) {
			auto proto = npc->GetModel()->modelProtoList;
			// Eintrag 0 ist das Grundmodell (HUMANS.MDS, SCAVENGER.MDS, ...), kein Overlay.
			// Frueher fiel es nur heraus, weil es nicht auf der festen Liste stand; seit
			// GetMdsIndex jeden Namen aufloest, muss es ausdruecklich uebersprungen werden.
			for (int i = 1; i < proto.GetNumInList(); i++) {
				if (auto protoInst = proto.GetSafe(i)) {
					arr.Insert(GetMdsIndex(protoInst->modelProtoFileName));
				}
			}
		}
		return arr;
	}

	void oCNpc::ApplyOverlaysNpc(oCNpc* npc) {
		if (!npc) return;

		zCArray<int> npcMdsList = GetNpcMds(npc);
		ApplyOverlaysArray(npcMdsList);
	}

	void oCNpc::ApplyOverlaysArray(zCArray<int> npcMdsArray) {
		// Ohne Modell gibt es nichts abzugleichen. Frueher fehlte die Pruefung: GetModel() == NULL
		// fuehrte zum Lesen auf 0x60 (modelProtoList bei zCModel+0x58, Anzahl bei +0x08) - genau
		// der Absturz, wegen dessen der Abgleich unter Gothic 1 zeitweise abgeschaltet war.
		zCModel* model = GetModel();
		if (!model)
			return;
		zCArray<zCModelPrototype*> protoList = model->modelProtoList;
		zSTRING baseMds = protoList.GetNumInList() > 0 && protoList.GetSafe(0)
			? protoList.GetSafe(0)->modelProtoFileName : zSTRING("HUMANS.MDS");
		zCArray<zSTRING> mdsList;
		for (int i = 1; i < protoList.GetNumInList(); i++) {   // 0 = Grundmodell, kein Overlay
			if (zCModelPrototype* proto = protoList.GetSafe(i)) {
				if (proto->modelProtoFileName != "HUMANS.MDS") {
					mdsList.Insert(proto->modelProtoFileName);
				}
			}
		}

		for (int i = 0; i < mdsList.GetNumInList(); i++) {
			zSTRING mds = mdsList.GetSafe(i);
			this->RemoveOverlayMds(mds);
		}

		for (int i = 0; i < npcMdsArray.GetNumInList(); i++) {
			int index = npcMdsArray.GetSafe(i);
			zSTRING mds = GetMdsByIndex(index);

			if (!mds.IsEmpty() && mds != "HUMANS.MDS" && mds != baseMds) {
				this->ApplyOverlayMds(mds);
			}
		}
	}

	int oCNpc::CompareOverlaysMds(oCNpc* npc) {
		if (!npc) return 0;
		zCArray<int> npcMdsList = GetNpcMds(npc);
		zCArray<int> npcMdsListMy = GetNpcMds(this);
		if (npcMdsListMy.GetNumInList() != npcMdsList.GetNumInList()) {
			return 0;
		}
		else {
			for (int i = 0; i < npcMdsListMy.GetNumInList(); i++) {
				int mdsMy = npcMdsListMy.GetSafe(i);
				if (!npcMdsList.IsInList(mdsMy)) {
					return 0;
				}
			}
		}


		return 1;
	}

	int oCNpc::CompareOverlaysArray(zCArray<int> npcMdsList) {

		zCArray<int> npcMdsListMy = GetNpcMds(this);

		if (npcMdsListMy.GetNumInList() != npcMdsList.GetNumInList()) {
			return 0;
		}
		else {
			for (int i = 0; i < npcMdsListMy.GetNumInList(); i++) {
				int mdsMy = npcMdsListMy.GetSafe(i);
				if (!npcMdsList.IsInList(mdsMy)) {
					return 0;
				}
			}
		}


		return 1;
	}

	void oCNpc::RemoveOverlayMds(const zSTRING& mds) {
		RemoveOverlay(mds);
		if (GetAnictrl())
			GetAnictrl()->InitAnimations();
	};

	void oCNpc::ApplyOverlayMds(const zSTRING& mds) {
		ApplyOverlay(mds);
		if (GetAnictrl())
			GetAnictrl()->InitAnimations();
	};

	void zCParser::CallFuncByName(zSTRING name) {
		int idx = GetIndex(name);

		if (idx != -1) {
			CallFunc(idx);
		}
	}

	int GetRandVal(int min, int max) {
		return (rand() % (max - min + 1) + min);
	}

	zCArray<zCVob*> CollectVobsInRadius(zVEC3 pos, int radius)
	{
		zTBBox3D box;
		zCArray<zCVob*> vobList;

		if (!player->GetHomeWorld())
		{
			return vobList;
		}

		box.maxs = pos + (zVEC3(1, 1, 1) * radius);
		box.mins = pos - (zVEC3(1, 1, 1) * radius);

		oCNpc* other = NULL;

		player->GetHomeWorld()->bspTree.bspRoot->CollectVobsInBBox3D(vobList, box);
		return vobList;
	}

	ENetPacketFlag PacketFlag(const json& packet)   // v71: Verweis statt Kopie (Dateistuecke sind 48 KB gross)
	{
		auto updateType = (UpdateType)packet["type"].get<int>();

		if (updateType == SYNC_POS || updateType == SYNC_HEADING) {
			return ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT;
		}

		return ENET_PACKET_FLAG_RELIABLE;
	}

	int PacketChannel(const json& packet)
	{
		auto updateType = (UpdateType)packet["type"].get<int>();

		if (updateType == SYNC_POS || updateType == SYNC_HEADING) {
			return 1;
		}

		return 0;
	}
}