Scriptname RelentlessBash_Script extends activemagiceffect  

Event OnEffectStart(Actor akTarget, Actor akCaster)
    Form right = akTarget.GetEquippedObject(1)
    If right
        akTarget.DropObject(right)
    EndIf
    Form left = akTarget.GetEquippedObject(0)
    If left
        akTarget.DropObject(left)
    EndIf
    Form shield = akTarget.GetEquippedShield()
    If shield
        akTarget.DropObject(shield)
    EndIf
EndEvent