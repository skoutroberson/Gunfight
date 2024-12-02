// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Gunfight/GunfightTypes/Hitbox.h"
#include "LagCompensationComponent.generated.h"

USTRUCT(BlueprintType)
struct FCapsuleInformation
{
	GENERATED_BODY()

	UPROPERTY()
	FVector A;

	UPROPERTY()
	FVector B;

	UPROPERTY()
	float Radius;

	UPROPERTY()
	float Length;

	UPROPERTY()
	EHitbox HitboxType = EHitbox::EH_None;
};




UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GUNFIGHT_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULagCompensationComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(Server, Reliable)
	void ServerScoreRequest(
		class AGunfightCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation,
		float HitTime,
		class AWeapon* DamageCauser
	);

	void ServerScoreRequest_Implementation(
		class AGunfightCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation,
		float HitTime,
		class AWeapon* DamageCauser
	);

protected:
	virtual void BeginPlay() override;

private:


public:	
	
	
};
