ScriptName AL_AmmoActiPerk Extends Perk

GlobalVariable Property gAmmoLimitMult Auto
Sound Property AL_MachineSound01 Auto

Message Property AL_IncreaseAmmoMessage Auto

GlobalVariable Property gIncreaseLimitBasePrice Auto
GlobalVariable Property gIncreaseLimitAddedPrice Auto
MiscObject Property Caps001 Auto

Function Entry_10(ObjectReference akTargetRef, Actor akActor)
	InputEnableLayer GachaLayer = InputEnableLayer.Create()
	GachaLayer.DisablePlayerControls(True, False, True, True, False, True, True, True, True, True, True)

	int basePrice = gIncreaseLimitBasePrice.GetValueInt()
	float iLimitMult = gAmmoLimitMult.GetValue()

	int i = AL_IncreaseAmmoMessage.Show(basePrice, akActor.GetItemCount(caps001), iLimitMult, iLimitMult + 0.1) 
	if i == 0
		AL_MachineSound01.Play(akTargetRef)
		akActor.RemoveItem(caps001, basePrice, true)
		gIncreaseLimitBasePrice.SetValue(basePrice + gIncreaseLimitAddedPrice.GetValue())
		gAmmoLimitMult.SetValue(iLimitMult + 0.1)
	Endif

	GachaLayer.Reset()
	GachaLayer.Delete()
EndFunction