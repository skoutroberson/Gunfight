// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "HitscanWeapon.generated.h"

/**
 * 
 */
UCLASS()
class GUNFIGHT_API AHitscanWeapon : public AWeapon
{
	GENERATED_BODY()
	
public:
	virtual void Fire(const FVector& HitTarget, bool bLeft) override;

protected:

	void WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit);
	void WeaponTraceHits(const FVector& TraceStart, const FVector& HitTarget, TArray<FHitResult>& OutHits);

	UPROPERTY(EditAnywhere)
	class UParticleSystem* ImpactParticles;
	
	UPROPERTY(EditAnywhere)
	USoundCue* HitSound;

private:

	UPROPERTY(EditAnywhere)
	UParticleSystem* BeamParticles;

	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlash;

	UPROPERTY(EditAnywhere)
	USoundCue* FireSound;
	
};
