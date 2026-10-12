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

    LoadProjectileParams();
}

void UProjectileAbility::SetProjectileToSpawn(const FGameplayTag& InProjectileTag)
{
    ProjectileToSpawnTag = InProjectileTag;
    LoadProjectileParams();
}

void UProjectileAbility::LoadProjectileParams()
{
    if (!ProjectileToSpawnTag.IsValid() || !IsValid(AvatarActorFromInfo)) return;

    // ProjectileInfo GameMode'dan geliyor, clientta null doner (spawn zaten server'da).
    if (UProjectileInfo* ProjectileInfo = URougeAbilitySystemLibrary::GetProjectileInfo(AvatarActorFromInfo))
    {
        if (const FProjectileParams* Params = ProjectileInfo->ProjectileInfoMap.Find(ProjectileToSpawnTag))
        {
            CurrentProjectileParams = *Params;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("ProjectileAbility: '%s' not found in ProjectileInfo map."), *ProjectileToSpawnTag.ToString());
        }
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
            SpawnTransform, AvatarActorFromInfo))
    {
        SpawnedProjectile->SetProjectileParams(CurrentProjectileParams);

        FDamageEffectInfo DamageEffectInfo;
        CaptureDamageEffectInfo(nullptr, DamageEffectInfo);

        SpawnedProjectile->DamageEffectInfo = DamageEffectInfo;

        SpawnedProjectile->FinishSpawning(SpawnTransform);
    }

}