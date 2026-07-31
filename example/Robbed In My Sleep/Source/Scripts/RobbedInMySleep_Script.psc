Scriptname RobbedInMySleep_Script extends ReferenceAlias  

MiscObject Property Gold001 Auto
Message Property MessageToShow Auto

Event OnInit()
    Init()
EndEvent

Event OnPlayerLoadGame()
    Init()
EndEvent

Function Init()
    ; "CarelessSleep" is defined in .json file located in /SKSE/Plugins/State Globals/:
    ;
    ; "actions" : [
    ;     {
    ;         "type" : "modEvent",
    ;         "value" : 3,
    ;         "eventName" : "CarelessSleep"
    ;     }
    ; ]
    ; so: when the global's value is set to 3, call a Mod Event named "CarelessSleep"

    RegisterForModEvent("CarelessSleep", "OnCarelessSleep")
EndFunction

Event OnCarelessSleep(String eventName, String strArg, Float numArg, Form sender)
    GetActorReference().RemoveItem(Gold001, 50)
    MessageToShow.Show()
EndEvent