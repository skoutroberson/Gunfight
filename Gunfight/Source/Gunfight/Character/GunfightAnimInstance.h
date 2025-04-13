// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Gunfight/GunfightTypes/HandState.h"
#include "GunfightAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class GUNFIGHT_API UGunfightAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaTime) override;

	/**
	* UBIK Upper Body IK
	*/

	UPROPERTY(BlueprintReadOnly, Category = Character)
	FTransform HeadTransform;

	UPROPERTY(BlueprintReadOnly, Category = Character)
	FTransform LeftHandTransform;

	UPROPERTY(BlueprintReadOnly, Category = Character)
	FTransform RightHandTransform;

	UPROPERTY(BlueprintReadOnly, Category = Character)
	USceneComponent* LeftHandComponent;

	UPROPERTY(BlueprintReadOnly, Category = Character)
	USceneComponent* RightHandComponent;

	/**
	* Leg IK
	*/

	UPROPERTY(BlueprintReadOnly, Category = Character)
	FVector RightFootLocation;

	UPROPERTY(BlueprintReadOnly, Category = Character)
	FVector LeftFootLocation;

	UPROPERTY(BlueprintReadOnly, Category = Character)
	FVector TraceRightFootLocation;

	UPROPERTY(BlueprintReadOnly, Category = Character)
	FVector TraceLeftFootLocation;

	// feet need to be reset here so the foot traces work in AGunfightCharacter::UpdateAnimInstanceIK()
	void UpdateFeetLocation();

	UPROPERTY(BlueprintReadOnly, Category = Character)
	bool bElimmed = false;

private:

	UPROPERTY(BlueprintReadOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
	class AGunfightCharacter* GunfightCharacter;

	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float Speed;

	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bIsAccelerating;

	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float YawOffset;

	UPROPERTY(BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
	bool bWeaponEquipped;

	UPROPERTY(BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bLocallyControlled = false;

	class AWeapon* LeftEquippedWeapon;
	AWeapon* RightEquippedWeapon;

	FVector TraceFootLocation(bool bLeft);
	
	UPROPERTY()
	UWorld* World;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Hands, meta = (AllowPrivateAccess = "true"))
	EHandState LeftHandState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Hands, meta = (AllowPrivateAccess = "true"))
	EHandState RightHandState;

public:
	AGunfightCharacter* GetCharacter() { return GunfightCharacter; }
	void SetCharacter(AGunfightCharacter* Char) { GunfightCharacter = Char; }
	FORCEINLINE void SetLocallyControlled(bool bNewLocallyControlled) { bLocallyControlled = bNewLocallyControlled; }
};
