// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NSCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "Kismet/GameplayStatics.h"

#include "Net/UnrealNetwork.h"
#include "NSPlayerState.h"

#include "Particles/ParticleSystemComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// ANSCharacter

ANSCharacter::ANSCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->RelativeLocation = FVector(-39.56f, 1.75f, 64.f); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	FP_Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterFP_Mesh"));
	FP_Mesh->SetOnlyOwnerSee(true);
	FP_Mesh->SetupAttachment(FirstPersonCameraComponent);
	FP_Mesh->bCastDynamicShadow = false;
	FP_Mesh->CastShadow = false;
	FP_Mesh->RelativeRotation = FRotator(1.9f, -19.19f, 5.2f);
	FP_Mesh->RelativeLocation = FVector(-0.5f, -4.4f, -155.7f);

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	FP_Gun->SetupAttachment(FP_Mesh, TEXT("GripPoint"));
	
	// �� �޽� ������Ʈ �����ϱ�
	TP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("TP_Gun"));
	TP_Gun->SetOwnerNoSee(true);
	TP_Gun->AttachTo(GetMesh(), TEXT("hand_rSocket"), EAttachLocation::SnapToTargetIncludingScale, true);
	
	GetMesh()->SetOwnerNoSee(true); //���ܹ߻�

	// ��ƼŬ ����
	TP_GunShotParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleSysTP"));
	TP_GunShotParticle->bAutoActivate = false;
	TP_GunShotParticle->AttachTo(TP_Gun);
	TP_GunShotParticle->SetOwnerNoSee(true);

	// ��ƼŬ ����
	FP_GunShotParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("FP_GunShotParticle"));
	FP_GunShotParticle->bAutoActivate = false;
	FP_GunShotParticle->AttachTo(FP_Gun);
	FP_GunShotParticle->SetOnlyOwnerSee(true);

	BulletParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("BulletSysTP"));
	BulletParticle->bAutoActivate = false;
	BulletParticle->AttachTo(FirstPersonCameraComponent);
}

void ANSCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
	
	if (Role != ROLE_Authority)
	{
		SetTeam(CurrentTeam);
	}
}

void ANSCharacter::SetTeam_Implementation(ETeam NewTeam)
{
	FLinearColor outColour;
	if (NewTeam == ETeam::BLUE_TEAM)
	{
		outColour = FLinearColor(0.0f, 0.0f, 0.5f);
	}
	else
	{
		outColour = FLinearColor(0.5f, 0.0f, 0.0f);
	}
	if (DynamicMat == nullptr)
	{
		DynamicMat = UMaterialInstanceDynamic::Create(GetMesh()->GetMaterial(0), this);
		DynamicMat->SetVectorParameterValue(TEXT("BodyColor"), outColour);
		GetMesh()->SetMaterial(0, DynamicMat);
		FP_Mesh->SetMaterial(0, DynamicMat);
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void ANSCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind fire event
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ANSCharacter::OnFire);
	
	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &ANSCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ANSCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ANSCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ANSCharacter::LookUpAtRate);
}

void ANSCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANSCharacter, CurrentTeam);
}

void ANSCharacter::OnFire()
{
	// �����Ǿ��ٸ� �߻� �ִϸ��̼� ����� �õ��Ѵ�
	if (FP_FireAnimation != NULL)
	{
		// �� �޽��� �ִϸ��̼� ������Ʈ�� ��´�
		UAnimInstance* AnimInstance = FP_Mesh->GetAnimInstance();
		if (AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(FP_FireAnimation, 1.f);
		}
	}

	// �����Ǿ��ٸ� FP ��ƼŬ ����Ʈ�� ����Ѵ�
	if (FP_GunShotParticle != nullptr)
	{
		FP_GunShotParticle->Activate(true);
	}

	FVector mousePos;
	FVector mouseDir;

	APlayerController* pController = Cast<APlayerController>(GetController());
	FVector2D ScreenPos = GEngine->GameViewport->Viewport->GetSizeXY();

	pController->DeprojectScreenPositionToWorld(ScreenPos.X / 2.0f, ScreenPos.Y / 2.0f, mousePos, mouseDir);
	mouseDir *= 10000000.0f;

	ServerFire(mousePos, mouseDir);

}

bool ANSCharacter::ServerFire_Validate(const FVector pos, const FVector dir)
{
	if (pos != FVector(ForceInit) && dir != FVector(ForceInit))
	{
		return true;
	}
	else
	{
		return false;
	}
}

void ANSCharacter::ServerFire_Implementation(const FVector pos, const FVector dir)
{
	Fire(pos, dir);
	MultiCastShootEffects();
}

void ANSCharacter::MultiCastShootEffects_Implementation()
{
	// �����Ǿ��ٸ� �߻� �ִϸ��̼� ����� �õ��Ѵ�
	if (TP_FireAnimation != NULL)
	{
		// �� �޽��� �ִϸ��̼� ������Ʈ�� ��´�
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(TP_FireAnimation, 1.f);
		}
	}

	// ������ ��� ���� ����� �õ��Ѵ�.
	if (FireSound != NULL)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	if (TP_GunShotParticle != nullptr)
	{
		TP_GunShotParticle->Activate(true);
	}

	if (BulletParticle != nullptr)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BulletParticle->Template, BulletParticle->GetComponentLocation(), BulletParticle->GetComponentRotation());
	}
}

void ANSCharacter::Fire(const FVector pos, const FVector dir)
{
	// ����ĳ��Ʈ ����
	FCollisionObjectQueryParams ObjQuery;
	ObjQuery.AddObjectTypesToQuery(ECC_GameTraceChannel1);

	FCollisionQueryParams ColQuery;
	ColQuery.AddIgnoredActor(this);

	FHitResult HitRes;
	GetWorld()->LineTraceSingleByObjectType(HitRes, pos, dir, ObjQuery, ColQuery);

	DrawDebugLine(GetWorld(), pos, dir, FColor::Red, true, 100, 0, 5.0f);

	if (HitRes.bBlockingHit)
	{
		ANSCharacter* OtherChar = Cast<ANSCharacter>(HitRes.GetActor());
		if (OtherChar != nullptr && OtherChar->GetNSPlayerState()->Team != this->GetNSPlayerState()->Team)
		{
			FDamageEvent thisEvent(UDamageType::StaticClass());
			OtherChar->TakeDamage(10.0f, thisEvent, this->GetController(), this);

			APlayerController* thisPC = Cast<APlayerController>(GetController());
			thisPC->ClientPlayForceFeedback(HitSuccessFeedback, false, NAME_None);
		}
	}
}

float ANSCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

	if (Role == ROLE_Authority && DamageCauser != this && NSPlayerState->Health > 0)
	{
		NSPlayerState->Health -= Damage;
		PlayPain();

		if (NSPlayerState->Health <= 0)
		{
			NSPlayerState->Deaths++;
			// �÷��̾ �������� �ð����� �״´�
			MultiCastRagdoll();
			ANSCharacter * OtherChar = Cast<ANSCharacter>(DamageCauser);

			if (OtherChar)
			{
				OtherChar->NSPlayerState->Score += 1.0f;
			}

			// 3�� �ڿ� �������ȴ�
			FTimerHandle thisTimer;

			GetWorldTimerManager().SetTimer<ANSCharacter>(thisTimer, this, &ANSCharacter::Respawn, 3.0f, false);
		}
	}
	return Damage;
}

void ANSCharacter::PlayPain_Implementation()
{
	if (Role == ROLE_AutonomousProxy)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PainSound, GetActorLocation());
	}
}

void ANSCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void ANSCharacter::MultiCastRagdoll_Implementation()
{
	GetMesh()->SetPhysicsBlendWeight(1.0f);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName("Ragdoll");
}

void ANSCharacter::Respawn()
{
	if (Role == ROLE_Authority)
	{
		// ���� ���κ��� ��ġ ���
		NSPlayerState->Health = 100.0f;
		Cast<ANSGameMode>(GetWorld()->GetAuthGameMode())->Respawn(this);

		Destroy(true, true);
	}
}

void ANSCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	NSPlayerState = Cast<ANSPlayerState>(PlayerState);

	if (Role == ROLE_Authority && NSPlayerState != nullptr)
	{
		NSPlayerState->Health = 100.0f;
	}
}

ANSPlayerState* ANSCharacter::GetNSPlayerState()
{
	if (NSPlayerState)
	{
		return NSPlayerState;
	}
	else
	{
		NSPlayerState = Cast<ANSPlayerState>(PlayerState);
		return NSPlayerState;
	}
}

void ANSCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void ANSCharacter::SetNSPlayerState(ANSPlayerState* newPS)
{
	// PS�� ��ȿ�ϰ� �������� �����Ǿ����� Ȯ���Ѵ�
	if (newPS && Role == ROLE_Authority)
	{
		NSPlayerState = newPS;
		PlayerState = newPS;
	}
}

void ANSCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ANSCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}