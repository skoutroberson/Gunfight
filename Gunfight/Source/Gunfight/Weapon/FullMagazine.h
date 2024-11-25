// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FullMagazine.generated.h"

UCLASS()
class GUNFIGHT_API AFullMagazine : public AActor
{
	GENERATED_BODY()
	
public:	
	AFullMagazine();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

private:

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Magazine Properties", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* MagazineMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Magazine Properties", meta = (AllowPrivateAccess = "true"))
	class USphereComponent* AreaSphere;

	// set when spawned by GunfightCharacter
	UPROPERTY()
	class AGunfightCharacter* CharacterOwner;

	FTimerHandle InitializeCollisionHandle;
	void InitializeCollision();

public:	
	FORCEINLINE void SetCharacterOwner(AGunfightCharacter* Character) { CharacterOwner = Character; }
	
};
