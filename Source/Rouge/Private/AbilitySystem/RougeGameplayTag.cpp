// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/RougeGameplayTag.h"

namespace RougeGameplayTags::Combat
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage, "Combat.Data.Damage", "Set By Caller Data Tag For Combat");
	//UE_DEFINE_GAMEPLAY_TAG_STATIC(Projectile_Basic, "Projectile.Basic");

}

namespace RougeGameplayTags::Event
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Melee_WindowOpen, "Event.Melee.WindowOpen", "Sent by AnimNotifyState_MeleeDamageWindow when the melee hit window opens");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Melee_WindowClose, "Event.Melee.WindowClose", "Sent by AnimNotifyState_MeleeDamageWindow when the melee hit window closes");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Melee_Hit, "Event.Melee.Hit", "Sent by AnimNotify_MeleeHit for a single instant melee hit check");
}

namespace RougeGameplayTags::State
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dead, "State.Dead", "Character is dead (player: waiting for revive / respawn timer). Blocks abilities and damage");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Invulnerable, "State.Invulnerable", "Character takes no damage (e.g. briefly after being revived)");
}
