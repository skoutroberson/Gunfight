// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VRCharacter.h"
#include "Gunfight/GunfightTypes/HandState.h"
#include "GunfightCharacter.generated.h"

/**
 * 
 */
UCLASS()
class GUNFIGHT_API AGunfightCharacter : public AVRCharacter
{
	GENERATED_BODY()
	
public:
	
	AGunfightCharacter();
	virtual void PostInitializeComponents() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable)
	void SetVREnabled(bool bEnabled);

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

protected:
	virtual void BeginPlay() override;

	// input
	void MoveForward(float Throttle);
	void MoveRight(float Throttle);
	void TurnRight(float Throttle);
	void LookUp(float Throttle);

	DECLARE_DELEGATE_OneParam(FLeftRightButtonDelegate, const bool);

	void GripPressed(bool bLeftController);
	void GripReleased(bool bLeftController);
	void TriggerPressed(bool bLeftController);
	void TriggerReleased(bool bLeftController);
	void AButtonPressed(bool bLeftController);
	void AButtonReleased(bool bLeftController);
	void BButtonPressed(bool bLeftController);
	void BButtonReleased(bool bLeftController);

private:

	/**
	* Gunfight components
	*/

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UCombatComponent> Combat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UVRComponent> VRComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class ULagCompensationComponent> LagCompensation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Gun, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UChildActorComponent> DefaultWeapon;

	UPROPERTY()
	class AWeapon* WeaponActor;

	UPROPERTY()
	UWorld* World;

	class UGunfightAnimInstance* GunfightAnimInstance;

	void UpdateAnimInstanceIK();
	FVector TraceFootLocationIK(bool bLeft);
	FCollisionQueryParams IKQueryParams;

	void MoveMeshToCamera();

	void UpdateAnimation();

	UPROPERTY(VisibleAnywhere);
	bool bIsInVR = true;

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressedLeft();
	void ServerEquipButtonPressedLeft_Implementation();
	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressedRight();
	void ServerEquipButtonPressedRight_Implementation();

	UPROPERTY()
	class AWeapon* OverlappingWeapon;

	UPROPERTY(EditAnywhere, Category = Hands, meta = (AllowPrivateAccess = "true"))
	EHandState LeftHandState;

	UPROPERTY(EditAnywhere, Category = Hands, meta = (AllowPrivateAccess = "true"))
	EHandState RightHandState;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	bool bRightHolsterPreferred = true;

	void PollInit();

	bool bLocallyGrippingWeapon = false;

public:
	// returns true if right holster is preferred
	inline bool GetRightHolsterPreferred() { return bRightHolsterPreferred; }
	inline AWeapon* GetWeapon() { return WeaponActor; }
	inline bool GetLocallyGrippingWeapon() { return bLocallyGrippingWeapon; }
	void SetHandState(bool bLeftHand, EHandState NewState);
	inline EHandState GetHandState(bool bLeftHand) { return bLeftHand ? LeftHandState : RightHandState; }
};
