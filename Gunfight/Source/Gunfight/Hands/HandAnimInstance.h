// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Gunfight/GunfightTypes/HandState.h"
#include "HandAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class GUNFIGHT_API UHandAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EHandState HandState = EHandState::EHS_Idle;
	
	
};
