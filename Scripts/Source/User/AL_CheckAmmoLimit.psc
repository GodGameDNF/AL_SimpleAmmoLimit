ScriptName AL_CheckAmmoLimit Extends ReferenceAlias

Perk Property AL_ActiPerk Auto
Keyword Property ObjectTypeAmmo Auto
GlobalVariable Property gAmmoDropHeight Auto

Actor PlayerRef

Event OnAliasInit()
	AddInventoryEventFilter(ObjectTypeAmmo)
	PlayerRef = GetRef() as Actor
	PlayerRef.AddPerk(AL_ActiPerk)
EndEvent

Event OnItemAdded(Form akBaseItem, int aiItemCount, \
ObjectReference akItemReference, ObjectReference akSourceContainer)

	int dropCount = AL_SimpleAmmoLimit.CheckDrop(akBaseItem)

	if dropCount > 0
		ObjectReference ammoRef = PlayerRef.PlaceAtme(akBaseItem, dropCount, false, true, false)
		float[] locateArray = AL_SimpleAmmoLimit.getDropPos()

		ammoRef.Moveto(PlayerRef, locateArray[0], locateArray[1], gAmmoDropHeight.GetValue())
		ammoRef.Enable()
	Endif
EndEvent