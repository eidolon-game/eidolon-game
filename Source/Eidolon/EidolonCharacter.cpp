#include "EidolonCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputCoreTypes.h"

AEidolonCharacter::AEidolonCharacter()
{
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;
    auto* Movement = GetCharacterMovement();
    Movement->bOrientRotationToMovement = true;
    Movement->RotationRate = FRotator(0.f, 540.f, 0.f);
    Movement->MaxWalkSpeed = WalkSpeed;
    Movement->JumpZVelocity = 500.f;
    Movement->AirControl = 0.25f;
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 350.f;
    CameraBoom->SocketOffset = FVector(0.f, 45.f, 60.f);
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->bDoCollisionTest = true;
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
}

void AEidolonCharacter::BuildInputMapping()
{
    if (Mapping) return;
    Mapping = NewObject<UInputMappingContext>(this);
    const auto MakeAction = [this](EInputActionValueType Type)
    {
        auto* Action = NewObject<UInputAction>(this);
        Action->ValueType = Type;
        return Action;
    };
    const auto Axis = EInputActionValueType::Axis1D;
    ForwardAction = MakeAction(Axis);
    RightAction = MakeAction(Axis);
    MouseYawAction = MakeAction(Axis);
    MousePitchAction = MakeAction(Axis);
    StickYawAction = MakeAction(Axis);
    StickPitchAction = MakeAction(Axis);
    JumpAction = MakeAction(EInputActionValueType::Boolean);
    SprintAction = MakeAction(EInputActionValueType::Boolean);
    const auto Map = [this](UInputAction* Action, FKey Key, bool bNegate = false, bool bDeadZone = false)
    {
        auto& Binding = Mapping->MapKey(Action, Key);
        if (bDeadZone)
        {
            auto* Modifier = NewObject<UInputModifierDeadZone>(Mapping);
            Modifier->LowerThreshold = 0.15f;
            Modifier->UpperThreshold = 1.f;
            Binding.Modifiers.Add(Modifier);
        }
        if (bNegate) Binding.Modifiers.Add(NewObject<UInputModifierNegate>(Mapping));
    };
    Map(ForwardAction, EKeys::W);
    Map(ForwardAction, EKeys::S, true);
    Map(RightAction, EKeys::D);
    Map(RightAction, EKeys::A, true);
    Map(ForwardAction, EKeys::Gamepad_LeftY, false, true);
    Map(RightAction, EKeys::Gamepad_LeftX, false, true);
    Map(MouseYawAction, EKeys::MouseX);
    Map(MousePitchAction, EKeys::MouseY, true);
    Map(StickYawAction, EKeys::Gamepad_RightX, false, true);
    Map(StickPitchAction, EKeys::Gamepad_RightY, true, true);
    Map(JumpAction, EKeys::SpaceBar);
    Map(JumpAction, EKeys::Gamepad_FaceButton_Bottom);
    Map(SprintAction, EKeys::LeftShift);
    Map(SprintAction, EKeys::Gamepad_LeftThumbstick);
}

void AEidolonCharacter::PawnClientRestart()
{
    Super::PawnClientRestart();
    StopSprint();
    BuildInputMapping();
    if (auto* PC = Cast<APlayerController>(GetController()))
    {
        if (auto* LP = PC->GetLocalPlayer())
        {
            if (auto* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            {
                Subsystem->RemoveMappingContext(Mapping);
                Subsystem->AddMappingContext(Mapping, 0);
            }
        }
    }
}

void AEidolonCharacter::RemoveInputMapping()
{
    if (auto* PC = Cast<APlayerController>(GetController()))
        if (auto* LP = PC->GetLocalPlayer())
            if (auto* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
                if (Mapping) Subsystem->RemoveMappingContext(Mapping);
}

void AEidolonCharacter::UnPossessed()
{
    RemoveInputMapping();
    StopSprint();
    Super::UnPossessed();
}

void AEidolonCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    BuildInputMapping();
    auto* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (!ensure(Input)) return;
    Input->BindAction(ForwardAction, ETriggerEvent::Triggered, this, &AEidolonCharacter::MoveForward);
    Input->BindAction(RightAction, ETriggerEvent::Triggered, this, &AEidolonCharacter::MoveRight);
    Input->BindAction(MouseYawAction, ETriggerEvent::Triggered, this, &AEidolonCharacter::MouseYaw);
    Input->BindAction(MousePitchAction, ETriggerEvent::Triggered, this, &AEidolonCharacter::MousePitch);
    Input->BindAction(StickYawAction, ETriggerEvent::Triggered, this, &AEidolonCharacter::StickYaw);
    Input->BindAction(StickPitchAction, ETriggerEvent::Triggered, this, &AEidolonCharacter::StickPitch);
    Input->BindAction(JumpAction, ETriggerEvent::Started, this, &AEidolonCharacter::StartJump);
    Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &AEidolonCharacter::EndJump);
    Input->BindAction(JumpAction, ETriggerEvent::Canceled, this, &AEidolonCharacter::EndJump);
    Input->BindAction(SprintAction, ETriggerEvent::Started, this, &AEidolonCharacter::StartSprint);
    Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &AEidolonCharacter::StopSprint);
    Input->BindAction(SprintAction, ETriggerEvent::Canceled, this, &AEidolonCharacter::StopSprint);
}

void AEidolonCharacter::MoveForward(const FInputActionValue& Value)
{
    if (!Controller) return;
    const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
    AddMovementInput(Yaw.Vector(), Value.Get<float>());
}
void AEidolonCharacter::MoveRight(const FInputActionValue& Value)
{
    if (!Controller) return;
    const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
    AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Value.Get<float>());
}
void AEidolonCharacter::MouseYaw(const FInputActionValue& Value) { AddControllerYawInput(Value.Get<float>()); }
void AEidolonCharacter::MousePitch(const FInputActionValue& Value) { AddControllerPitchInput(Value.Get<float>()); }
void AEidolonCharacter::StickYaw(const FInputActionValue& Value) { AddControllerYawInput(Value.Get<float>() * StickLookDegreesPerSecond * GetWorld()->GetDeltaSeconds()); }
void AEidolonCharacter::StickPitch(const FInputActionValue& Value) { AddControllerPitchInput(Value.Get<float>() * StickLookDegreesPerSecond * GetWorld()->GetDeltaSeconds()); }
void AEidolonCharacter::StartJump() { Jump(); }
void AEidolonCharacter::EndJump() { StopJumping(); }
void AEidolonCharacter::StartSprint() { GetCharacterMovement()->MaxWalkSpeed = SprintSpeed; }
void AEidolonCharacter::StopSprint() { GetCharacterMovement()->MaxWalkSpeed = WalkSpeed; }
