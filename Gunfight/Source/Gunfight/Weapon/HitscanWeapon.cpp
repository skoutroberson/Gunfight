// Fill out your copyright notice in the Description page of Project Settings.


#include "HitscanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Gunfight/Character/GunfightCharacter.h"
#include "Gunfight/GunfightTypes/Hitbox.h"

void AHitscanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();

		FHitResult FireHit;
		WeaponTraceHit(Start, HitTarget, FireHit);

		AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(FireHit.GetActor());
		if (GunfightCharacter && InstigatorController)
		{
			bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
			if (HasAuthority() && bCauseAuthDamage)
			{
				
				UGameplayStatics::ApplyDamage(
					GunfightCharacter,
					FHitbox::GetDamage(FireHit.BoneName, Damage),
					InstigatorController,
					this,
					UDamageType::StaticClass()
				);
				
			}
			if (!HasAuthority() && bUseServerSideRewind)
			{
				// TO DO
			}
		}
	}
}

void AHitscanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
{

}
