// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/RougeMainGameMode.h"

UCharacterClassInfo* ARougeMainGameMode::GetCharacterClassDefaultInfo() const
{
	return ClassDefaults;
}

UProjectileInfo* ARougeMainGameMode::GetProjectileInfo() const
{
	return ProjectileInfo;
}
