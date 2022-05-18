// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EUMadAbilityInputID : uint8 {
	None,
	Confirm,
	Cancel,
	Launch,
	Grapple,
	Jump,
	Wall
};