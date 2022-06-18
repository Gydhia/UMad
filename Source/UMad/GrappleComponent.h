// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "KahnaxGrapple.h"
#include "GrappleComponent.generated.h"


UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UMAD_API UGrappleComponent : public UActorComponent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSubclassOf<AKahnaxGrapple> Grapple;
	
public:	
	// Sets default values for this component's properties
	UGrappleComponent();
	FVector TargetAttach;
	ACharacter* Owner;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private :
	UPROPERTY()
	AKahnaxGrapple* _grapple;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void BeginGrapple(AActor* NearestAttach);
		
};
