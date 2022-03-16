// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Runtime/Engine/Classes/Components/SphereComponent.h"
#include "Obstacle.generated.h"

UCLASS()
class BOUNTYDASH_API AObstacle : public AActor
{
	GENERATED_BODY()

	float KillPoint;

public:
	// �� ������ �Ӽ� �⺻���� ������
	AObstacle();

	// ������ ���۵ǰų� �����Ǿ��� �� ȣ���  
	virtual void BeginPlay() override;

	// �� ������ ȣ���
	virtual void Tick(float DeltaSeconds) override;

	void SetkillPoint(float point);
	float GetkillPoint();

protected:
	UFUNCTION()
		virtual void MyOnActorOverlap(AActor* OverlappedActor, AActor* otherActor);

	UFUNCTION()
		virtual void MyOnActorEndOverlap(AActor* OverlappedActor, AActor* otherActor);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		USphereComponent* Collider;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		UStaticMeshComponent* Mesh;
};