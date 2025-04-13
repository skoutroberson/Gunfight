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
	virtual void Tick(float DeltaTime) override;

	void Dropped();
	void Equipped();

	bool bEquipped = false;

	// Skins

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UMaterialInterface*> WeaponSkins;

	void SetWeaponSkin(int32 SkinIndex);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FTransform GrabOffsetRight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	FTransform GrabOffsetLeft;

protected:
	virtual void BeginPlay() override;

	// check for overlaps on spawn because the OnComponentBeginOverlap doesn't register if they are inside eachother on spawn.
	void GetSpawnOverlaps();

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

	bool bLeftControllerOverlap = false;
	bool bRightControllerOverlap = false;

	void ShouldAttachToHolster();

	FTimerHandle ShouldAttachToHolsterHandle;
	FTimerHandle InsertIntoMagHandle;

	void CanInsertIntoMagwell();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magazine Properties", meta = (AllowPrivateAccess = "true"));
	FVector HandSocketOffset = FVector::ZeroVector;

	bool bLerpToMagwellStart = false;

	void LerpToMagwellStart(float DeltaTime);

	FVector MagwellRelativeTargetLocation;

	float MagwellDistance = 0.f;

	UPROPERTY();
	USceneComponent* MagwellEnd;
	UPROPERTY();
	USceneComponent* MagwellStart;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magazine Properties", meta = (AllowPrivateAccess = "true"));
	float MagInsertSpeed = 5.f;

	UPROPERTY()
	class AWeapon* WeaponOwner;

public:	
	FORCEINLINE void SetCharacterOwner(AGunfightCharacter* Character) { CharacterOwner = Character; }
	FORCEINLINE bool CheckHandOverlap(bool bLeft) const { return bLeft ? bLeftControllerOverlap : bRightControllerOverlap; }
	FORCEINLINE UStaticMeshComponent* GetMagazineMesh() const { return MagazineMesh; }
	FORCEINLINE FVector GetHandSocketOffset() const { return HandSocketOffset; }
	FORCEINLINE void SetWeaponOwner(AWeapon* NewWeaponOwner) { WeaponOwner = NewWeaponOwner; }
	FORCEINLINE AWeapon* GetWeaponOwner() { return WeaponOwner; }
};
