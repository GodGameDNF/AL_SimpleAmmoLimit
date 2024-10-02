ScriptName AL_AmmoSwitchActiScript Extends ObjectReference
import ObjectMod

Actor Property PlayerRef Auto
Actor Property AmmoSeller Auto
Spell Property AmmoSellerPenaltySpell Auto

Event OnActivate(ObjectReference akActionRef)
	BlockActivation(True, True)
	AL_SimpleAmmoLimit.injectAmmo()

	PlayerRef.AddSpell(AmmoSellerPenaltySpell, false)
	AmmoSeller.ShowBarterMenu()
	Utility.Wait(0.5)

	PlayerRef.RemoveSpell(AmmoSellerPenaltySpell)
	BlockActivation(False)
EndEvent