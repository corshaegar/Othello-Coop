namespace GOTHIC_ENGINE {
	RemoteNpc* addSyncedNpc(string uniqueName) {
		auto myFriend = new RemoteNpc(uniqueName);
		SyncNpcs[uniqueName] = myFriend;
		return myFriend;
	}

	void removeSyncedNpc(string uniqueName) {
		// v40: Der Eintrag kann fehlen - LoadEnd behaelt nur Spieler mit Figur, und seit v40 meldet sich ein Mitspieler schon im
		// Hauptmenue (noch ohne Figur). SyncNpcs[...] legte dann NULL an, der Zugriff darauf stuerzte ab (Trennen danach).
		auto it = SyncNpcs.find(uniqueName);
		if (it == SyncNpcs.end() || !it->second) {
			if (it != SyncNpcs.end())
				SyncNpcs.erase(it);
			return;
		}
		auto removedNpc = it->second;
		json j;
		j["type"] = DESTROY_NPC;
		removedNpc->localUpdates.push_back(j);
	}
}