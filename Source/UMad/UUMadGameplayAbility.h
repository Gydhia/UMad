// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "UMad.h"
#include "UUMadGameplayAbility.generated.h"

/**
 * 
 */
UCLASS()
class UMAD_API UUMadGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UUMadGameplayAbility();
	 
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Ability")
	EUMadAbilityInputID AbilityInputID = EUMadAbilityInputID::None;
	
};
