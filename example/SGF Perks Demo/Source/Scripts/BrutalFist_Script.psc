Scriptname BrutalFist_Script extends activemagiceffect  

GlobalVariable Property counter  Auto
GlobalVariable Property PushStrength Auto
Explosion Property ExplosionEffect Auto

Event OnEffectStart(Actor akTarget, Actor akCaster)
    Game.GetPlayer().PlaceAtMe(ExplosionEffect)
    Game.GetPlayer().PushActorAway(akTarget, PushStrength.value)
EndEvent