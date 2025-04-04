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

	UPROPERTY()
	int32 BoneIndex;
};

USTRUCT(BlueprintType)
struct FSphereInfo
{
	GENERATED_BODY()

	FVector Center;
	float Radius;
};

// info for the weapon fire trace hit
USTRUCT(BlueprintType)
struct FHitInfo
{
	GENERATED_BODY()

	// used for spawning blood impact particles
	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FVector Normal;

	// used to determine how much damage to apply to player
	UPROPERTY()
	EHitbox HitType = EHitbox::EH_None;

	UPROPERTY()
	int32 HitBoneIndex;
};

USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()

	UPROPERTY()
	float Time;

	UPROPERTY()
	TArray<FCapsuleInformation> HitCapsulesInfo;

	UPROPERTY()
	AGunfightCharacter* Character;
};

USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	EHitbox HitType = EHitbox::EH_None;

	UPROPERTY()
	FVector HitLocation;

	UPROPERTY()
	FVector HitNormal;

	UPROPERTY()
	int32 HitBoneIndex;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GUNFIGHT_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULagCompensationComponent();
	friend class AGunfightCharacter;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void ShowFramePackage(const FFramePackage& Package, const FColor& Color);

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

	FServerSideRewindResult ServerSideRewind(
		AGunfightCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation,
		float HitTime
	);

protected:
	virtual void BeginPlay() override;

	FFramePackage GetFrameToCheck(AGunfightCharacter* HitCharacter, float HitTime);

	FServerSideRewindResult ConfirmHit(
		const FFramePackage& Package,
		AGunfightCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation
	);

	FFramePackage InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime);
	void SaveFramePackage();
	void SaveFramePackage(FFramePackage& Package);

private:

	UPROPERTY()
	AGunfightCharacter* Character;

	UPROPERTY()
	class AGunfightPlayerController* Controller;

	UPROPERTY()
	class AWeapon* DamageCauserWeapon;

	TDoubleLinkedList<FFramePackage> FrameHistory;

	UPROPERTY(EditAnywhere)
	float MaxRecordTime = 2.f;

	void EnableCharacterMeshCollision(AGunfightCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled);
	FHitInfo TraceAgainstCapsules(const FFramePackage& Package, const AGunfightCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector& TraceEnd);
	void CapsuleToSpheres(const FVector& A, const FVector& B, const float Radius, const float Length, TArray<FSphereInfo>& OutSpheres);
	// copied from Vector.h to avoid having to cast FVector to FVector3f every time we call this
	inline bool LineSphereIntersection(const FVector& Start, const FVector& Dir, float Length, const FVector& Origin, float Radius);
	// Returns first intersection point from a line intersecting with a sphere. Assumes that the line does intersect with the sphere.
	inline FVector const FirstIntersectionPoint(const FVector& LineStart, const FVector& LineEnd, const FVector& Center, const float Radius);

	// Maximum number of hits to check in TraceAgainstCapsules
	UPROPERTY(EditDefaultsOnly)
	uint8 MaxSpheresHit = 4;


public:	
	
	
};
