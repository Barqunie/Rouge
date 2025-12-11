// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/ProjectileAbility.h"
#include "Data/ProjectileInfo.h"
#include "Libraries/RougeAbilitySystemLibrary.h"
#include "Interfaces/RougeAbilitySystemInterface.h"
#include "Projectiles/ProjectileBase.h"


UProjectileAbility::UProjectileAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UProjectileAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);
	
	AvatarActorFromInfo = GetAvatarActorFromActorInfo();

	if (!ProjectileToSpawnTag.IsValid() || !IsValid(AvatarActorFromInfo)) return;

	if(UProjectileInfo* ProjectileInfo = URougeAbilitySystemLibrary::GetProjectileInfo(GetAvatarActorFromActorInfo()))
	{
		CurrentProjectileParams = *ProjectileInfo->ProjectileInfoMap.Find(ProjectileToSpawnTag);
	}
}

void UProjectileAbility::SpawnProjectile()
{
    if (!IsValid(CurrentProjectileParams.ProjectileClass)) return;
    if (!IsValid(AvatarActorFromInfo)) return;


    const USceneComponent* SpawnPointComp =
        IRougeAbilitySystemInterface::Execute_GetDynamicSpawnPoint(AvatarActorFromInfo);
    if (!IsValid(SpawnPointComp)) return;

    const FVector SpawnLocation = SpawnPointComp->GetComponentLocation();

    FVector ShootDir = AvatarActorFromInfo->GetActorForwardVector();

    ShootDir.Z = 0.f;
    ShootDir.Normalize();

    const FRotator SpawnRotation = ShootDir.Rotation();

    FTransform SpawnTransform;
    SpawnTransform.SetLocation(SpawnLocation);
    SpawnTransform.SetRotation(SpawnRotation.Quaternion());

    if (AProjectileBase* SpawnedProjectile =
        GetWorld()->SpawnActorDeferred<AProjectileBase>(
            CurrentProjectileParams.ProjectileClass,
            SpawnTransform))
    {
        SpawnedProjectile->SetProjectileParams(CurrentProjectileParams);
        SpawnedProjectile->FinishSpawning(SpawnTransform);
    }

}

