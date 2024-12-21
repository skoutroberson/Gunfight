// Fill out your copyright notice in the Description page of Project Settings.


#include "HitscanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Gunfight/Character/GunfightCharacter.h"
#include "Gunfight/GunfightTypes/Hitbox.h"
#include "Particles/ParticleSystemComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Gunfight/GunfightComponents/LagCompensationComponent.h"
#include "Gunfight/PlayerController/GunfightPlayerController.h"

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

				const FVector BloodSpawnLocation = ((Start - FireHit.ImpactPoint).GetSafeNormal() * 10.f) + FireHit.ImpactPoint;
				GunfightCharacter->MulticastSpawnBlood(BloodSpawnLocation);
			}
			if (!HasAuthority() && bUseServerSideRewind)
			{
				// TO DO
				CharacterOwner = CharacterOwner == nullptr ? Cast<AGunfightCharacter>(OwnerPawn) : CharacterOwner;
				GunfightOwnerController = GunfightOwnerController == nullptr ? Cast<AGunfightPlayerController>(InstigatorController) : GunfightOwnerController;
				if (GunfightOwnerController && CharacterOwner && CharacterOwner->GetLagCompensation() && CharacterOwner->IsLocallyControlled())
				{
					CharacterOwner->GetLagCompensation()->ServerScoreRequest(
						GunfightCharacter,
						Start,
						HitTarget,
						GunfightOwnerController->GetServerTime() - GunfightOwnerController->SingleTripTime,
						this
					);
				}
			}
		}

		/*
		* Multi hit bullets. I'm not using this until I know this won't be too expensive on mobile
		* 
		float CurrentBulletDamage = Damage; // will get smaller as it hits stuff during travel
		TMap<AActor*, bool> HitActors;

		TArray<FHitResult> FireHits;
		WeaponTraceHits(Start, HitTarget, FireHits);

		const int n = FireHits.Num() > 5 ? 5 : FireHits.Num();

		for (int i = 0; i < n; ++i)
		{
			FHitResult& FireHit = FireHits[i];
			AActor* HitActor = FireHit.GetActor();

			// skip iteration if actor was already hit
			if (HitActor)
			{
				if (HitActors.Contains(HitActor))
				{
					continue;
				}
				HitActors.Add(HitActor);
			}

			AGunfightCharacter* GunfightCharacter = Cast<AGunfightCharacter>(HitActor);
			if (GunfightCharacter && InstigatorController)
			{
				bool bCauseAuthDamage = !bUseServerSideRewind || OwnerPawn->IsLocallyControlled();
				if (HasAuthority() && bCauseAuthDamage)
				{

					UGameplayStatics::ApplyDamage(
						GunfightCharacter,
						FHitbox::GetDamage(FireHit.BoneName, CurrentBulletDamage),
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

			// check surface type and update bullet damage
			if (FireHit.PhysMaterial.IsValid())
			{
				uint32 SurfaceType = FireHit.PhysMaterial->SurfaceType;

				if (SurfaceType == SurfaceType1) // Hardwall
				{
					return;
				}
				else if (SurfaceType == SurfaceType2) // Softwall
				{
					CurrentBulletDamage *= 0.66f;
				}
				else if (SurfaceType == SurfaceType3) // Human
				{
					CurrentBulletDamage *= 0.9f;
				}
			}
		}*/
	}
}

void AHitscanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
{
	UWorld* World = GetWorld();
	if (World)
	{
		FVector End = TraceStart + (HitTarget - TraceStart) * 1.25f;

		World->LineTraceSingleByChannel(
			OutHit,
			TraceStart,
			End,
			ECollisionChannel::ECC_Visibility
		);
		FVector BeamEnd = End;
		if (OutHit.bBlockingHit)
		{
			BeamEnd = OutHit.ImpactPoint;

			if (BeamParticles)
			{
				UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
					World,
					BeamParticles,
					TraceStart,
					FRotator::ZeroRotator,
					true
				);
				if (Beam)
				{
					Beam->SetVectorParameter(FName("Target"), BeamEnd);
				}
			}
		}
	}
}

void AHitscanWeapon::WeaponTraceHits(const FVector& TraceStart, const FVector& HitTarget, TArray<FHitResult>& OutHits)
{
	UWorld* World = GetWorld();
	if (World)
	{
		FVector End = TraceStart + (HitTarget - TraceStart) * 1.25f;

		World->LineTraceMultiByChannel(
			OutHits,
			TraceStart,
			End,
			ECollisionChannel::ECC_Visibility,
			BulletQueryParams
		);
	}
}
