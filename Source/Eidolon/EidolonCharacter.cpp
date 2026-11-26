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
#include "EidolonArenaEnemy.h"
#include "EidolonGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AEidolonCharacter::AEidolonCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    PrototypeBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeBody"));
    PrototypeBody->SetupAttachment(RootComponent);
    PrototypeBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PrototypeBody->SetRelativeScale3D(FVector(0.65f, 0.65f, 1.6f));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (BodyMesh.Succeeded()) PrototypeBody->SetStaticMesh(BodyMesh.Object);
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
    LightAction = MakeAction(EInputActionValueType::Boolean);
    HeavyAction = MakeAction(EInputActionValueType::Boolean);
    DodgeAction = MakeAction(EInputActionValueType::Boolean);
    BlockAction = MakeAction(EInputActionValueType::Boolean);
    RestartAction = MakeAction(EInputActionValueType::Boolean);
    MemoryAction = MakeAction(EInputActionValueType::Boolean);
    InteractAction = MakeAction(EInputActionValueType::Boolean);
    EndingOne = MakeAction(EInputActionValueType::Boolean);
    EndingTwo = MakeAction(EInputActionValueType::Boolean);
    EndingThree = MakeAction(EInputActionValueType::Boolean);
    EndingFour = MakeAction(EInputActionValueType::Boolean);
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
    Map(LightAction, EKeys::LeftMouseButton);
    Map(LightAction, EKeys::Gamepad_RightShoulder);
    Map(HeavyAction, EKeys::RightMouseButton);
    Map(HeavyAction, EKeys::Gamepad_RightTrigger);
    Map(DodgeAction, EKeys::Q);
    Map(DodgeAction, EKeys::Gamepad_FaceButton_Right);
    Map(BlockAction, EKeys::F);
    Map(BlockAction, EKeys::Gamepad_LeftShoulder);
    Map(RestartAction, EKeys::R);
    Map(RestartAction, EKeys::Gamepad_FaceButton_Top);
    Map(MemoryAction, EKeys::M);
    Map(MemoryAction, EKeys::Gamepad_RightThumbstick);
    Map(InteractAction, EKeys::E);
    Map(InteractAction, EKeys::Gamepad_FaceButton_Left);
    Map(EndingOne, EKeys::One);
    Map(EndingTwo, EKeys::Two);
    Map(EndingThree, EKeys::Three);
    Map(EndingFour, EKeys::Four);
    Map(EndingOne, EKeys::Gamepad_DPad_Up);
    Map(EndingTwo, EKeys::Gamepad_DPad_Right);
    Map(EndingThree, EKeys::Gamepad_DPad_Down);
    Map(EndingFour, EKeys::Gamepad_DPad_Left);
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
    Input->BindAction(LightAction, ETriggerEvent::Started, this, &AEidolonCharacter::LightAttack);
    Input->BindAction(HeavyAction, ETriggerEvent::Started, this, &AEidolonCharacter::HeavyAttack);
    Input->BindAction(DodgeAction, ETriggerEvent::Started, this, &AEidolonCharacter::Dodge);
    Input->BindAction(BlockAction, ETriggerEvent::Started, this, &AEidolonCharacter::StartBlock);
    Input->BindAction(BlockAction, ETriggerEvent::Completed, this, &AEidolonCharacter::EndBlock);
    Input->BindAction(BlockAction, ETriggerEvent::Canceled, this, &AEidolonCharacter::EndBlock);
    Input->BindAction(RestartAction, ETriggerEvent::Started, this, &AEidolonCharacter::RestartArena);
    Input->BindAction(MemoryAction, ETriggerEvent::Started, this, &AEidolonCharacter::RecoverMemory);
    Input->BindAction(InteractAction, ETriggerEvent::Started, this, &AEidolonCharacter::Interact);
    Input->BindAction(EndingOne, ETriggerEvent::Started, this, &AEidolonCharacter::ChooseOne);
    Input->BindAction(EndingTwo, ETriggerEvent::Started, this, &AEidolonCharacter::ChooseTwo);
    Input->BindAction(EndingThree, ETriggerEvent::Started, this, &AEidolonCharacter::ChooseThree);
    Input->BindAction(EndingFour, ETriggerEvent::Started, this, &AEidolonCharacter::ChooseFour);
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
    if (!Controller || IsDefeated()) return;
    const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
    AddMovementInput(Yaw.Vector(), Value.Get<float>());
}
void AEidolonCharacter::MoveRight(const FInputActionValue& Value)
{
    if (!Controller || IsDefeated()) return;
    const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
    AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Value.Get<float>());
}
void AEidolonCharacter::MouseYaw(const FInputActionValue& Value) { AddControllerYawInput(Value.Get<float>()); }
void AEidolonCharacter::MousePitch(const FInputActionValue& Value) { AddControllerPitchInput(Value.Get<float>()); }
void AEidolonCharacter::StickYaw(const FInputActionValue& Value) { AddControllerYawInput(Value.Get<float>() * StickLookDegreesPerSecond * GetWorld()->GetDeltaSeconds()); }
void AEidolonCharacter::StickPitch(const FInputActionValue& Value) { AddControllerPitchInput(Value.Get<float>() * StickLookDegreesPerSecond * GetWorld()->GetDeltaSeconds()); }
void AEidolonCharacter::StartJump() { if (!IsDefeated()) Jump(); }
void AEidolonCharacter::EndJump() { StopJumping(); }
void AEidolonCharacter::StartSprint() { if (!IsDefeated() && !bBlocking) GetCharacterMovement()->MaxWalkSpeed = SprintSpeed; }
void AEidolonCharacter::StopSprint() { GetCharacterMovement()->MaxWalkSpeed = WalkSpeed; }

void AEidolonCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    AttackCooldown = FMath::Max(0.f, AttackCooldown - DeltaSeconds);
    InvulnerableTime = FMath::Max(0.f, InvulnerableTime - DeltaSeconds);
    if (!IsDefeated() && !bBlocking) Stamina = FMath::Min(100.f, Stamina + 18.f * DeltaSeconds);
}
float AEidolonCharacter::TakeDamage(float Amount, const FDamageEvent& Event, AController* Instigator, AActor* Causer)
{
    if (IsDefeated() || InvulnerableTime > 0.f || Amount <= 0.f) return 0.f;
    if (bBlocking && Stamina >= 12.f) { Stamina -= 12.f; Amount *= 0.25f; }
    const float Applied = FMath::Min(Health, Amount);
    Health -= Applied;
    if (IsDefeated()) { GetCharacterMovement()->DisableMovement(); bBlocking = false; }
    return Applied;
}
void AEidolonCharacter::Attack(float Damage, float Cost, float Cooldown)
{
    if (IsDefeated() || bBlocking || AttackCooldown > 0.f || Stamina < Cost) return;
    Stamina -= Cost;
    AttackCooldown = Cooldown;
    AEidolonArenaEnemy* Target = nullptr;
    float Best = 260.f;
    for (TActorIterator<AEidolonArenaEnemy> It(GetWorld()); It; ++It)
    {
        if (It->IsActorBeingDestroyed()) continue;
        const FVector Offset = It->GetActorLocation() - GetActorLocation();
        const float Distance = Offset.Size();
        if (Distance >= Best || FVector::DotProduct(GetActorForwardVector(), Offset.GetSafeNormal2D()) < 0.25f) continue;
        FHitResult Hit;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(this);
        if (GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), It->GetActorLocation(), ECC_Visibility, Params) && Hit.GetActor() != *It) continue;
        Best = Distance;
        Target = *It;
    }
    if (Target) UGameplayStatics::ApplyDamage(Target, Damage, GetController(), this, nullptr);
}
void AEidolonCharacter::LightAttack() { Attack(20.f, 12.f, 0.45f); }
void AEidolonCharacter::HeavyAttack() { Attack(40.f, 28.f, 0.95f); }
void AEidolonCharacter::Dodge()
{
    if (IsDefeated() || bBlocking || Stamina < 25.f || InvulnerableTime > 0.f || !GetCharacterMovement()->IsMovingOnGround()) return;
    Stamina -= 25.f;
    InvulnerableTime = 0.3f;
    FVector Direction = GetLastMovementInputVector().GetSafeNormal2D();
    if (Direction.IsNearlyZero()) Direction = GetActorForwardVector();
    LaunchCharacter(Direction * 850.f + FVector(0.f,0.f,100.f), true, true);
}
void AEidolonCharacter::StartBlock() { if (!IsDefeated()) { bBlocking = true; StopSprint(); } }
void AEidolonCharacter::EndBlock() { bBlocking = false; }
void AEidolonCharacter::RestartArena() { if (auto* GM = GetWorld()->GetAuthGameMode<AEidolonGameMode>()) GM->ReloadCheckpoint(); }
void AEidolonCharacter::Interact() { if (!IsDefeated()) if (auto* GM = GetWorld()->GetAuthGameMode<AEidolonGameMode>()) GM->Interact(this); }
void AEidolonCharacter::ChooseOne() { if (!IsDefeated()) if (auto* GM = GetWorld()->GetAuthGameMode<AEidolonGameMode>()) GM->SelectEnding(1); }
void AEidolonCharacter::ChooseTwo() { if (!IsDefeated()) if (auto* GM = GetWorld()->GetAuthGameMode<AEidolonGameMode>()) GM->SelectEnding(2); }
void AEidolonCharacter::ChooseThree() { if (!IsDefeated()) if (auto* GM = GetWorld()->GetAuthGameMode<AEidolonGameMode>()) GM->SelectEnding(3); }
void AEidolonCharacter::ChooseFour() { if (!IsDefeated()) if (auto* GM = GetWorld()->GetAuthGameMode<AEidolonGameMode>()) GM->SelectEnding(4); }

void AEidolonCharacter::RecoverMemory() { if (!IsDefeated()) if (auto* GM = GetWorld()->GetAuthGameMode<AEidolonGameMode>()) GM->RecoverMemory(this); }
