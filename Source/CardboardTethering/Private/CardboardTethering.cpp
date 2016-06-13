// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "CardboardTetheringPrivatePCH.h"
#include "CardboardTethering.h"
#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "SceneViewport.h"
#include "IPluginManager.h"
#include "PostProcess/PostProcessHMD.h"
#include "EndianUtils.h"
#include "MagicSparklesStyle.h"

#if WITH_EDITOR
#include "Editor/UnrealEd/Classes/Editor/EditorEngine.h"
#endif

//---------------------------------------------------
// CardboardTethering Plugin Implementation
//---------------------------------------------------

#define LOCTEXT_NAMESPACE "FCardboardTethering"

/** Helper function for acquiring the appropriate FSceneViewport */
FSceneViewport* FindSceneViewport() {
  if (!GIsEditor) {
    UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);
    return GameEngine->SceneViewport.Get();
  }
#if WITH_EDITOR
  else {
    UEditorEngine* EditorEngine = CastChecked<UEditorEngine>(GEngine);
    FSceneViewport* PIEViewport = (FSceneViewport*) EditorEngine->GetPIEViewport();
    if (PIEViewport != nullptr && PIEViewport->IsStereoRenderingAllowed()) {
      // PIE is setup for stereo rendering
      return PIEViewport;
    } else {
      // Check to see if the active editor viewport is drawing in stereo mode
      // @todo vreditor: Should work with even non-active viewport!
      FSceneViewport* EditorViewport = (FSceneViewport*) EditorEngine->GetActiveViewport();
      if (EditorViewport != nullptr && EditorViewport->IsStereoRenderingAllowed()) {
        return EditorViewport;
      }
    }
  }
#endif
  return nullptr;
}

class FCardboardTetheringPlugin : public ICardboardTetheringPlugin {
  /** IHeadMountedDisplayModule implementation */
  virtual TSharedPtr< class IHeadMountedDisplay, ESPMode::ThreadSafe > CreateHeadMountedDisplay() override;

  FString GetModulePriorityKeyName() const override {
    return FString(TEXT("CardboardTethering"));
  }
};

IMPLEMENT_MODULE(FCardboardTetheringPlugin, CardboardTethering)

TSharedPtr< class IHeadMountedDisplay, ESPMode::ThreadSafe > FCardboardTetheringPlugin::CreateHeadMountedDisplay() {
  TSharedPtr< FCardboardTethering, ESPMode::ThreadSafe > CardboardTethering(new FCardboardTethering());
  if (CardboardTethering->IsInitialized()) {
    return CardboardTethering;
  }
  return NULL;
}


//---------------------------------------------------
// CardboardTethering IHeadMountedDisplay Implementation
//---------------------------------------------------

bool FCardboardTethering::IsHMDEnabled() const {
  return true;
}

void FCardboardTethering::EnableHMD(bool enable) {}

EHMDDeviceType::Type FCardboardTethering::GetHMDDeviceType() const {
  return EHMDDeviceType::DT_ES2GenericStereoMesh;
}

bool FCardboardTethering::GetHMDMonitorInfo(MonitorInfo& MonitorDesc) {
  MonitorDesc.MonitorName = "Google Cardboard Tethering";
  MonitorDesc.MonitorId = 0;
  MonitorDesc.DesktopX = MonitorDesc.DesktopY = 0;
  MonitorDesc.ResolutionX = ViewerWidth;
  MonitorDesc.ResolutionY = ViewerHeight;
  return false;
}

void FCardboardTethering::GetFieldOfView(float& OutHFOVInDegrees, float& OutVFOVInDegrees) const {
  OutHFOVInDegrees = 0.0f;
  OutVFOVInDegrees = 0.0f;
}

bool FCardboardTethering::DoesSupportPositionalTracking() const {
  return false;
}

bool FCardboardTethering::HasValidTrackingPosition() {
  return false;
}

void FCardboardTethering::GetPositionalTrackingCameraProperties(FVector& OutOrigin, FQuat& OutOrientation, float& OutHFOV, float& OutVFOV, float& OutCameraDistance, float& OutNearPlane, float& OutFarPlane) const {}

void FCardboardTethering::RebaseObjectOrientationAndPosition(FVector& OutPosition, FQuat& OutOrientation) const {}

void FCardboardTethering::SetInterpupillaryDistance(float NewInterpupillaryDistance) {}

float FCardboardTethering::GetInterpupillaryDistance() const {
  return ViewerInterpupillary.load();
}

void FCardboardTethering::GetCurrentPose(FQuat& CurrentOrientation) {
  /*
  // very basic.  no head model, no prediction, using debuglocalplayer
  ULocalPlayer* Player = GEngine->GetDebugLocalPlayer();

  if (Player != NULL && Player->PlayerController != NULL)
  {
    FVector RotationRate = Player->PlayerController->GetInputVectorKeyState(EKeys::RotationRate);

    double CurrentTime = FApp::GetCurrentTime();
    double DeltaTime = 0.0;

    if (LastSensorTime >= 0.0)
    {
      DeltaTime = CurrentTime - LastSensorTime;
    }

    LastSensorTime = CurrentTime;

    // mostly incorrect, but we just want some sensor input for testing
    RotationRate *= DeltaTime;
    CurrentOrientation *= FQuat(FRotator(FMath::RadiansToDegrees(-RotationRate.X), FMath::RadiansToDegrees(-RotationRate.Y), FMath::RadiansToDegrees(-RotationRate.Z)));
  }
  else
  {
    CurrentOrientation = FQuat(FRotator(0.0f, 0.0f, 0.0f));
  }
  */
  CurrentOrientation = FQuat(FeedbackOrientationX.load(), FeedbackOrientationY.load(), FeedbackOrientationZ.load(), FeedbackOrientationW.load());
}

void FCardboardTethering::GetCurrentOrientationAndPosition(FQuat& CurrentOrientation, FVector& CurrentPosition) {
  CurrentPosition = FVector(0.0f, 0.0f, 0.0f);

  GetCurrentPose(CurrentOrientation);
  CurHmdOrientation = LastHmdOrientation = CurrentOrientation;
}

TSharedPtr<ISceneViewExtension, ESPMode::ThreadSafe> FCardboardTethering::GetViewExtension() {
  TSharedPtr<FCardboardTethering, ESPMode::ThreadSafe> ptr(AsShared());
  return StaticCastSharedPtr<ISceneViewExtension>(ptr);
}

void FCardboardTethering::ApplyHmdRotation(APlayerController* PC, FRotator& ViewRotation) {
  ViewRotation.Normalize();

  GetCurrentPose(CurHmdOrientation);
  LastHmdOrientation = CurHmdOrientation;

  const FRotator DeltaRot = ViewRotation - PC->GetControlRotation();
  DeltaControlRotation = (DeltaControlRotation + DeltaRot).GetNormalized();

  // Pitch from other sources is never good, because there is an absolute up and down that must be respected to avoid motion sickness.
  // Same with roll.
  DeltaControlRotation.Pitch = 0;
  DeltaControlRotation.Roll = 0;
  DeltaControlOrientation = DeltaControlRotation.Quaternion();

  ViewRotation = FRotator(DeltaControlOrientation * CurHmdOrientation);
}

bool FCardboardTethering::UpdatePlayerCamera(FQuat& CurrentOrientation, FVector& CurrentPosition) {
  GetCurrentPose(CurHmdOrientation);
  CurrentOrientation = LastHmdOrientation = CurHmdOrientation;

  return true;
}

bool FCardboardTethering::IsChromaAbCorrectionEnabled() const {
  return false;
}

bool FCardboardTethering::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) {
  if (FParse::Command(&Cmd, TEXT("HMD"))) {
    if (FParse::Command(&Cmd, TEXT("CONNECT"))) {
      ConnectUsb();
      return true;
    } else if (FParse::Command(&Cmd, TEXT("DISCONNECT"))) {
      DisconnectUsb(0);
      return true;
    }
  }
  return false;
}

bool FCardboardTethering::IsPositionalTrackingEnabled() const {
  return false;
}

bool FCardboardTethering::EnablePositionalTracking(bool enable) {
  return false;
}

bool FCardboardTethering::IsHeadTrackingAllowed() const {
  return true;
}

bool FCardboardTethering::IsInLowPersistenceMode() const {
  return false;
}

void FCardboardTethering::EnableLowPersistenceMode(bool Enable) {}

void FCardboardTethering::ResetOrientationAndPosition(float yaw) {
  ResetOrientation(yaw);
  ResetPosition();
}

void FCardboardTethering::ResetOrientation(float Yaw) {}
void FCardboardTethering::ResetPosition() {}

void FCardboardTethering::SetClippingPlanes(float NCP, float FCP) {}

void FCardboardTethering::SetBaseRotation(const FRotator& BaseRot) {}

FRotator FCardboardTethering::GetBaseRotation() const {
  return FRotator::ZeroRotator;
}

void FCardboardTethering::SetBaseOrientation(const FQuat& BaseOrient) {}

FQuat FCardboardTethering::GetBaseOrientation() const {
  return FQuat::Identity;
}

void FCardboardTethering::DrawDistortionMesh_RenderThread(struct FRenderingCompositePassContext& Context, const FIntPoint& TextureSize) {
  float ClipSpaceQuadZ = 0.0f;
  FMatrix QuadTexTransform = FMatrix::Identity;
  FMatrix QuadPosTransform = FMatrix::Identity;
  const FSceneView& View = Context.View;
  const FIntRect SrcRect = View.ViewRect;

  FRHICommandListImmediate& RHICmdList = Context.RHICmdList;
  const FSceneViewFamily& ViewFamily = *(View.Family);
  FIntPoint ViewportSize = ViewFamily.RenderTarget->GetSizeXY();
  RHICmdList.SetViewport(0, 0, 0.0f, ViewportSize.X, ViewportSize.Y, 1.0f);

  static const uint32 NumVerts = 8;
  static const uint32 NumTris = 4;

  static const FDistortionVertex Verts[8] =
  {
    // left eye
    { FVector2D(-0.9f, -0.9f), FVector2D(0.0f, 1.0f), FVector2D(0.0f, 1.0f), FVector2D(0.0f, 1.0f), 1.0f, 0.0f },
    { FVector2D(-0.1f, -0.9f), FVector2D(0.5f, 1.0f), FVector2D(0.5f, 1.0f), FVector2D(0.5f, 1.0f), 1.0f, 0.0f },
    { FVector2D(-0.1f, 0.9f), FVector2D(0.5f, 0.0f), FVector2D(0.5f, 0.0f), FVector2D(0.5f, 0.0f), 1.0f, 0.0f },
    { FVector2D(-0.9f, 0.9f), FVector2D(0.0f, 0.0f), FVector2D(0.0f, 0.0f), FVector2D(0.0f, 0.0f), 1.0f, 0.0f },
    // right eye
    { FVector2D(0.1f, -0.9f), FVector2D(0.5f, 1.0f), FVector2D(0.5f, 1.0f), FVector2D(0.5f, 1.0f), 1.0f, 0.0f },
    { FVector2D(0.9f, -0.9f), FVector2D(1.0f, 1.0f), FVector2D(1.0f, 1.0f), FVector2D(1.0f, 1.0f), 1.0f, 0.0f },
    { FVector2D(0.9f, 0.9f), FVector2D(1.0f, 0.0f), FVector2D(1.0f, 0.0f), FVector2D(1.0f, 0.0f), 1.0f, 0.0f },
    { FVector2D(0.1f, 0.9f), FVector2D(0.5f, 0.0f), FVector2D(0.5f, 0.0f), FVector2D(0.5f, 0.0f), 1.0f, 0.0f },
  };

  static const uint16 Indices[12] = { /*Left*/ 0, 1, 2, 0, 2, 3, /*Right*/ 4, 5, 6, 4, 6, 7 };

  DrawIndexedPrimitiveUP(Context.RHICmdList, PT_TriangleList, 0, NumVerts, NumTris, &Indices,
    sizeof(Indices[0]), &Verts, sizeof(Verts[0]));
}

bool FCardboardTethering::IsStereoEnabled() const {
  return true;
}

bool FCardboardTethering::EnableStereo(bool stereo) {
  if (ViewerWidth.load() <= 0 || ViewerHeight.load() <= 0 || !stereo) {
    return true;
  }

  // Closest resolution to what we want probably.
  FSystemResolution::RequestResolutionChange(1280, 720, EWindowMode::Windowed);

  // Set the viewport to match that of the HMD display
  FSceneViewport* SceneVP = FindSceneViewport();
  if (SceneVP) {
    SceneVP->SetViewportSize(ViewerWidth.load() * 2, ViewerHeight.load()); // for two cardboard eyes
  }

  return true;
}

void FCardboardTethering::AdjustViewRect(EStereoscopicPass StereoPass, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const {
  SizeX = SizeX / 2;
  if (StereoPass == eSSP_RIGHT_EYE) {
    X += SizeX;
  }
}

void FCardboardTethering::CalculateStereoViewOffset(const enum EStereoscopicPass StereoPassType, const FRotator& ViewRotation, const float WorldToMeters, FVector& ViewLocation) {
  if (StereoPassType != eSSP_FULL) {
    float EyeOffset = 3.20000005f;
    const float PassOffset = (StereoPassType == eSSP_LEFT_EYE) ? EyeOffset : -EyeOffset;
    ViewLocation += ViewRotation.Quaternion().RotateVector(FVector(0, PassOffset, 0));
  }
}

FMatrix FCardboardTethering::GetStereoProjectionMatrix(const enum EStereoscopicPass StereoPassType, const float FOV) const {
  const float ProjectionCenterOffset = 0.151976421f;
  const float PassProjectionOffset = (StereoPassType == eSSP_LEFT_EYE) ? ProjectionCenterOffset : -ProjectionCenterOffset;

  const float HalfFov = 2.19686294f / 2.f;
  const float InWidth = ViewerWidth; // 640.f;
  const float InHeight = ViewerHeight; // 480.f;
  const float XS = 1.0f / tan(HalfFov);
  const float YS = InWidth / tan(HalfFov) / InHeight;

  const float InNearZ = GNearClippingPlane;
  return FMatrix(
    FPlane(XS, 0.0f, 0.0f, 0.0f),
    FPlane(0.0f, YS, 0.0f, 0.0f),
    FPlane(0.0f, 0.0f, 0.0f, 1.0f),
    FPlane(0.0f, 0.0f, InNearZ, 0.0f))

    * FTranslationMatrix(FVector(PassProjectionOffset, 0, 0));
}

void FCardboardTethering::InitCanvasFromView(FSceneView* InView, UCanvas* Canvas) {}

void FCardboardTethering::GetEyeRenderParams_RenderThread(const FRenderingCompositePassContext& Context, FVector2D& EyeToSrcUVScaleValue, FVector2D& EyeToSrcUVOffsetValue) const {
  EyeToSrcUVOffsetValue = FVector2D::ZeroVector;
  EyeToSrcUVScaleValue = FVector2D(1.0f, 1.0f);
}


void FCardboardTethering::SetupViewFamily(FSceneViewFamily& InViewFamily) {
  InViewFamily.EngineShowFlags.MotionBlur = 0;
  InViewFamily.EngineShowFlags.HMDDistortion = true;
  InViewFamily.EngineShowFlags.SetScreenPercentage(true);
  InViewFamily.EngineShowFlags.StereoRendering = IsStereoEnabled();
}

void FCardboardTethering::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) {
  InView.BaseHmdOrientation = FQuat(FRotator(0.0f, 0.0f, 0.0f));
  InView.BaseHmdLocation = FVector(0.f);
  //	WorldToMetersScale = InView.WorldToMetersScale;
  InViewFamily.bUseSeparateRenderTarget = false;
}

void FCardboardTethering::PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) {
  check(IsInRenderingThread());
}

void FCardboardTethering::PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& ViewFamily) {
  check(IsInRenderingThread());
}

FCardboardTethering::BridgeBaseImpl* FCardboardTethering::GetActiveRHIBridgeImpl() {
  return pD3D11Bridge;
}

FCardboardTethering::FCardboardTethering() :
  CurHmdOrientation(FQuat::Identity),
  LastHmdOrientation(FQuat::Identity),
  DeltaControlRotation(FRotator::ZeroRotator),
  DeltaControlOrientation(FQuat::Identity),
  LastSensorTime(-1.0),
  WindowMirrorMode(2),
  TurboJpegLibraryHandle(0) {
  static const FName RendererModuleName("Renderer");
  RendererModule = FModuleManager::GetModulePtr<IRendererModule>(RendererModuleName);

  FQuat zero(FRotator(0.0f, 0.0f, 0.0f));
  FeedbackOrientationX.store(zero.X);
  FeedbackOrientationY.store(zero.Y);
  FeedbackOrientationZ.store(zero.Z);
  FeedbackOrientationW.store(zero.W);

#if PLATFORM_WINDOWS
  if (IsPCPlatform(GMaxRHIShaderPlatform) && !IsOpenGLPlatform(GMaxRHIShaderPlatform)) {
    pD3D11Bridge = new D3D11Bridge(this);
  }
#endif

  FString BaseDir = IPluginManager::Get().FindPlugin("CardboardTethering")->GetBaseDir();

  // Add on the relative location of the third party dll and load it
  FString LibraryPath;
#if PLATFORM_WINDOWS
  LibraryPath = FPaths::Combine(*BaseDir, TEXT("Binaries/ThirdParty/turbojpeg/Win64/turbojpeg.dll"));
  TurboJpegLibraryHandle = !LibraryPath.IsEmpty() ? FPlatformProcess::GetDllHandle(*LibraryPath) : nullptr;

  LibraryPath = FPaths::Combine(*BaseDir, TEXT("Binaries/ThirdParty/libusb/Win64/libusb-1.0.dll"));
  LibUsbLibraryHandle = !LibraryPath.IsEmpty() ? FPlatformProcess::GetDllHandle(*LibraryPath) : nullptr;

  if (TurboJpegLibraryHandle && LibUsbLibraryHandle) {
    UE_LOG(LogTemp, Warning, TEXT("Found them!"));
    SharedLibraryInitParams = TSharedPtr<LibraryInitParams>(new LibraryInitParams());
  } else
#endif // PLATFORM_WINDOWS
  {
    FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ThirdPartyLibraryError", "Failed to load example third party library"));
  }

  UE_LOG(LogTemp, Warning, TEXT("DONE CREATING!"));
}

FCardboardTethering::~FCardboardTethering() {
  FPlatformProcess::FreeDllHandle(TurboJpegLibraryHandle);
  TurboJpegLibraryHandle = nullptr;
  FPlatformProcess::FreeDllHandle(LibUsbLibraryHandle);
  TurboJpegLibraryHandle = nullptr;
}

bool FCardboardTethering::IsInitialized() const {
  return true;
}

void FCardboardTethering::UpdateViewport(bool bUseSeparateRenderTarget, const FViewport& InViewport, SViewport* ViewportWidget) {
  check(IsInGameThread());
  //UE_LOG(LogTemp, Warning, TEXT("jpeg handle %d"), TurboJpegLibraryHandle);
  FRHIViewport* const ViewportRHI = InViewport.GetViewportRHI().GetReference();

  if (!IsStereoEnabled()) {
    if (!bUseSeparateRenderTarget) {
      ViewportRHI->SetCustomPresent(nullptr);
    }
    return;
  }

  GetActiveRHIBridgeImpl()->UpdateViewport(InViewport, ViewportRHI);
}

void FCardboardTethering::CalculateRenderTargetSize(const class FViewport& Viewport, uint32& InOutSizeX, uint32& InOutSizeY) {
  check(IsInGameThread());

  //	if (Flags.bScreenPercentageEnabled)
  {
    static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.ScreenPercentage"));
    float value = CVar->GetValueOnGameThread();
    if (value > 0.0f) {
      InOutSizeX = FMath::CeilToInt(InOutSizeX * value / 100.f);
      InOutSizeY = FMath::CeilToInt(InOutSizeY * value / 100.f);
    }
  }
}

bool FCardboardTethering::NeedReAllocateViewportRenderTarget(const FViewport& Viewport) {
  check(IsInGameThread());

  if (IsStereoEnabled()) {
    const uint32 InSizeX = Viewport.GetSizeXY().X;
    const uint32 InSizeY = Viewport.GetSizeXY().Y;
    FIntPoint RenderTargetSize;
    RenderTargetSize.X = Viewport.GetRenderTargetTexture()->GetSizeX();
    RenderTargetSize.Y = Viewport.GetRenderTargetTexture()->GetSizeY();

    uint32 NewSizeX = InSizeX, NewSizeY = InSizeY;
    CalculateRenderTargetSize(Viewport, NewSizeX, NewSizeY);
    if (NewSizeX != RenderTargetSize.X || NewSizeY != RenderTargetSize.Y) {
      return true;
    }
  }
  return false;
}

void FCardboardTethering::ConnectUsb(uint16_t vid, uint16_t pid) {
  // Try to find the non-accessory device (hardcoded Nexus 4).
  TSharedPtr<UsbDevice> tempDevice;
  int status = UsbDevice::create(&tempDevice, SharedLibraryInitParams, vid, pid);
  if (!status) {
    tempDevice->convertToAccessory();

    // Wait for device re-renumeration.
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  TSharedPtr<UsbDevice> realDevice;
  status = UsbDevice::create(&realDevice, SharedLibraryInitParams);
  if (!status) {
    UE_LOG(LogTemp, Warning, TEXT("CONNECTED!"));

    OpenStatusWindowOnGameThread(
      LOCTEXT("HandshakeWaitMessage", "Waiting for Android device..."),
      [=]() {
        DisconnectUsb(0);
        return FReply::Handled();
      }
    );

    FScopeLock lock(&ActiveUsbDeviceMutex);
    ActiveUsbDevice = realDevice;
    ActiveUsbDevice->waitHandshakeAsync([this](bool success) {
      if (success) {
        FinishHandshake();
      } else {
        DisconnectUsb(0);
        OpenDialogOnGameThread(LOCTEXT("UsbHandshakeFailed", "USB handshake failure."));
      }
    });
    return;
  }

  OpenDialogOnGameThread(LOCTEXT("UsbNoDevice", "Could not find the USB device."));
}

void FCardboardTethering::DisconnectUsb(int reason) {
  CloseStatusWindowOnGameThread();

  FScopeLock lock(&ActiveUsbDeviceMutex);
  if (!ActiveUsbDevice.IsValid()) {
    UE_LOG(LogTemp, Warning, TEXT("Already disconnected"));
    return;
  }

  ActiveUsbDevice = nullptr;
  UE_LOG(LogTemp, Warning, TEXT("DISCONNECTED!"));

  if (reason != 0) {
    OpenDialogOnGameThread(FText::Format(LOCTEXT("UsbConnectionFailed",
      "The USB connection failed ({0}). Most likely the physical connection failed, "
      "e.g. the cable was disconnected."), FText::AsNumber(reason)));
  }
}

void FCardboardTethering::FinishHandshake() {
  CloseStatusWindowOnGameThread();

  FScopeLock lock(&ActiveUsbDeviceMutex);

  // Record the params.
  int32_t w, h;
  float ip;
  ActiveUsbDevice->getViewerParams(&w, &h, &ip);
  ViewerWidth.store(w);
  ViewerHeight.store(h);
  ViewerInterpupillary.store(ip);
  UE_LOG(LogTemp, Warning, TEXT("w=%d, h=%d, ip=%f"), w, h, ip);

  // Set up the send loop.
  ActiveUsbDevice->beginSendLoop([this](int reason) {
    DisconnectUsb(reason);
  });

  // Set up the receive loop.
  ActiveUsbDevice->beginReadLoop([this](const unsigned char* data, int reason) {
    if (reason) {
      DisconnectUsb(reason);
    } else {
      // Don't do anything right now.
      float floatData[4];
      std::memcpy(floatData, data, 4 * sizeof(float));
      for (int i = 0; i < 4; ++i) {
        floatData[i] = EndianUtils::bigToNativeFloat(floatData[i]);
      }

      // Determined by empirical testing, this seems to convert the coord space correctly!
      FeedbackOrientationX.store(floatData[0]);
      FeedbackOrientationY.store(floatData[2]);
      FeedbackOrientationZ.store(floatData[3]);
      FeedbackOrientationW.store(floatData[1]);
    }
  }, 4 * sizeof(float));
}

void FCardboardTethering::OpenDialogOnGameThread(FText msg) {
  FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([=]() {
    FMessageDialog::Open(EAppMsgType::Ok, msg);
  }, TStatId(), nullptr, ENamedThreads::GameThread);
}

void FCardboardTethering::OpenStatusWindowOnGameThread(FText msg,
    std::function<FReply(void)> cancelHandler) {
  FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([=]() {
    FMagicSparklesStyle::Initialize();

    FScopeLock lock(&StatusWindowMutex);
    if (StatusWindow.IsValid()) {
      StatusWindow->RequestDestroyWindow();
    }

    StatusWindow = SNew(SWindow)
      .MinHeight(0)
      .SizingRule(ESizingRule::FixedSize)
      .ClientSize(FVector2D(400, 50))
      .CreateTitleBar(false)
      .IsTopmostWindow(true)
      .SupportsMaximize(false)
      .SupportsMinimize(false)
      [
        SNew(SBox).VAlign(EVerticalAlignment::VAlign_Center)
        [
          SNew(SGridPanel)
            .FillColumn(0, 1.0f)
            + SGridPanel::Slot(0, 0)
              .Padding(2.0f)
              [
                SNew(STextBlock)
                  .Text(msg)
                  .TextStyle(&FMagicSparklesStyle::Get().GetWidgetStyle<FTextBlockStyle>(
                    TEXT("MagicSparkles.StatusTitle")))
              ]
            + SGridPanel::Slot(1, 0)
              .Padding(2.0f)
              [
                SNew(SButton)
                  .Text(LOCTEXT("CancelButtonLabel", "Cancel"))
                  .OnClicked_Lambda(cancelHandler)
              ]
        ]
      ];
    FSlateApplication::Get().AddWindow(StatusWindow.ToSharedRef());
  }, TStatId(), nullptr, ENamedThreads::GameThread);
}

void FCardboardTethering::CloseStatusWindowOnGameThread() {
  FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([=]() {
    FScopeLock lock(&StatusWindowMutex);
    if (StatusWindow.IsValid()) {
      StatusWindow->RequestDestroyWindow();
    }
  }, TStatId(), nullptr, ENamedThreads::GameThread);
}