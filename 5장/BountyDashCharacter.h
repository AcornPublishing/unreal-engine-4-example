// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Runtime/Engine/Classes/Components/AudioComponent.h"
#include "BountyDashCharacter.generated.h"

UCLASS()
class BOUNTYDASH_API ABountyDashCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// ĳ������ �Ӽ� �⺻ �� ����
	ABountyDashCharacter();

protected:
	// ������ ���۵ǰų� �����Ǿ��� �� ȣ���  
	virtual void BeginPlay() override;

public:
	// �� ������ ȣ��� 
	virtual void Tick(float DeltaSeconds) override;

	// �Է��ϴ� ����� ���ε��ϱ� ���� ȣ��
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


	//�Լ�
public:
	void ScoreUp();

protected:
	// �÷��̾� �¿��� Ÿ�� ��ġ �̵��� �����Ѵ� 
	void MoveRight();
	void MoveLeft();

	// ������Ʈ �ݸ��� ĸ���� ���Ǵ� ������ �Լ� 
	UFUNCTION()
	void MyOnComponentOverlap(class UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void MyOnComponentEndOverlap(class UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);


	//����
public:
	// �̵� ���� �迭
	UPROPERTY(EditAnywhere, Category = Logic)
	TArray<class ATargetPoint*> TargetArray;

	// ĳ���� �� �ٲ� �ӵ� 
	UPROPERTY(EditAnywhere, Category = Logic)
	float CharSpeed;

	// �浹 ���带 ���� ����� ������Ʈ
	UPROPERTY(EditAnywhere, Category = Sound)
	UAudioComponent* hitObstacleSound;

	// ���� ȹ�� ���带 ���� ����� ������Ʈ
	UPROPERTY(EditAnywhere, Category = Sound)
	UAudioComponent* dingSound;
	
protected:
	// �÷��̾�� ī�޶��� �Ÿ��� �����ϱ� ���� ī�޶� �� 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	class USpringArmComponent* CameraBoom;

	// Ÿ�� ��� ����� ī�޶� 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	class UCameraComponent* FollowCamera;

	// �÷��̾� ���� 
	UPROPERTY(BlueprintReadOnly)
	int32 Score;

private:
	// ĳ���� �� ��ġ ������ 
	short CurrentLocation;
	FVector DesiredLocation;
	bool bBeingPushed;
};
