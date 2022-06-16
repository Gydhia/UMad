// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Niagara/Public/NiagaraComponent.h"
#include "KahnaxGrapple.generated.h"


UCLASS()
class UMAD_API AKahnaxGrapple : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = GrapplingHook, meta = (AllowPrivateAccess = "true"))
	UNiagaraComponent* GrappleLine;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = GrapplingHook, meta = (AllowPrivateAccess = "true"))
	float TimeToReach = 0.5f;
	
public:	
	// Sets default values for this actor's properties
	AKahnaxGrapple();
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	FVector _startingLocation;
	FVector _endingLocation;
	FVector _direction;

	ACharacter* Owner;

	float _progress = -1;
	bool _goBackToOwner = false;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void StartGrapple(FVector Start, FVector Target, ACharacter* GrappleOwner);
	void EndGrapple();
};
