// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "CustomHMDPrivatePCH.h"
#include "CustomHMD.h"
#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "IPluginManager.h"
#include "PostProcess/PostProcessHMD.h"

//---------------------------------------------------
// CustomHMD Plugin Implementation
//---------------------------------------------------

#define LOCTEXT_NAMESPACE "FCustomHMD"

class FCustomHMDPlugin : public ICustomHMDPlugin
{
	/** IHeadMountedDisplayModule implementation */
	virtual TSharedPtr< class IHeadMountedDisplay, ESPMode::ThreadSafe > CreateHeadMountedDisplay() override;

	FString GetModulePriorityKeyName() const override
	{
		return FString(TEXT("CustomHMD"));
	}
};

IMPLEMENT_MODULE( FCustomHMDPlugin, CustomHMD )

TSharedPtr< class IHeadMountedDisplay, ESPMode::ThreadSafe > FCustomHMDPlugin::CreateHeadMountedDisplay()
{
	TSharedPtr< FCustomHMD, ESPMode::ThreadSafe > CustomHMD( new FCustomHMD() );
	if( CustomHMD->IsInitialized() )
	{
		return CustomHMD;
	}
	return NULL;
}


//---------------------------------------------------
// CustomHMD IHeadMountedDisplay Implementation
//---------------------------------------------------

bool FCustomHMD::IsHMDEnabled() const
{
	return true;
}

void FCustomHMD::EnableHMD(bool enable)
{
}

EHMDDeviceType::Type FCustomHMD::GetHMDDeviceType() const
{
	return EHMDDeviceType::DT_ES2GenericStereoMesh;
}

bool FCustomHMD::GetHMDMonitorInfo(MonitorInfo& MonitorDesc)
{
	MonitorDesc.MonitorName = "";
	MonitorDesc.MonitorId = 0;
	MonitorDesc.DesktopX = MonitorDesc.DesktopY = MonitorDesc.ResolutionX = MonitorDesc.ResolutionY = 0;
	return false;
}

void FCustomHMD::GetFieldOfView(float& OutHFOVInDegrees, float& OutVFOVInDegrees) const
{
	OutHFOVInDegrees = 0.0f;
	OutVFOVInDegrees = 0.0f;
}

bool FCustomHMD::DoesSupportPositionalTracking() const
{
	return false;
}

bool FCustomHMD::HasValidTrackingPosition()
{
	return false;
}

void FCustomHMD::GetPositionalTrackingCameraProperties(FVector& OutOrigin, FQuat& OutOrientation, float& OutHFOV, float& OutVFOV, float& OutCameraDistance, float& OutNearPlane, float& OutFarPlane) const
{
}

void FCustomHMD::RebaseObjectOrientationAndPosition(FVector& OutPosition, FQuat& OutOrientation) const
{
}

void FCustomHMD::SetInterpupillaryDistance(float NewInterpupillaryDistance)
{
}

float FCustomHMD::GetInterpupillaryDistance() const
{
	return 0.064f;
}

void FCustomHMD::GetCurrentPose(FQuat& CurrentOrientation)
{
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
}

void FCustomHMD::GetCurrentOrientationAndPosition(FQuat& CurrentOrientation, FVector& CurrentPosition)
{
	CurrentPosition = FVector(0.0f, 0.0f, 0.0f);

	GetCurrentPose(CurrentOrientation);
	CurHmdOrientation = LastHmdOrientation = CurrentOrientation;
}

TSharedPtr<ISceneViewExtension, ESPMode::ThreadSafe> FCustomHMD::GetViewExtension()
{
	TSharedPtr<FCustomHMD, ESPMode::ThreadSafe> ptr(AsShared());
	return StaticCastSharedPtr<ISceneViewExtension>(ptr);
}

void FCustomHMD::ApplyHmdRotation(APlayerController* PC, FRotator& ViewRotation)
{
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

bool FCustomHMD::UpdatePlayerCamera(FQuat& CurrentOrientation, FVector& CurrentPosition)
{
	return false;
}

bool FCustomHMD::IsChromaAbCorrectionEnabled() const
{
	return false;
}

bool FCustomHMD::Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar )
{
  if (FParse::Command(&Cmd, TEXT("HMD"))) {
    if (FParse::Command(&Cmd, TEXT("CONNECT"))) {
      ConnectUsb();
      return true;
    } else if (FParse::Command(&Cmd, TEXT("DISCONNECT"))) {
      DisconnectUsb();
      return true;
    }
  }
	return false;
}

void FCustomHMD::OnScreenModeChange(EWindowMode::Type WindowMode)
{
}

bool FCustomHMD::IsPositionalTrackingEnabled() const
{
	return false;
}

bool FCustomHMD::EnablePositionalTracking(bool enable)
{
	return false;
}

bool FCustomHMD::IsHeadTrackingAllowed() const
{
	return true;
}

bool FCustomHMD::IsInLowPersistenceMode() const
{
	return false;
}

void FCustomHMD::EnableLowPersistenceMode(bool Enable)
{
}

void FCustomHMD::ResetOrientationAndPosition(float yaw)
{
	ResetOrientation(yaw);
	ResetPosition();
}

void FCustomHMD::ResetOrientation(float Yaw)
{
}
void FCustomHMD::ResetPosition()
{
}

void FCustomHMD::SetClippingPlanes(float NCP, float FCP)
{
}

void FCustomHMD::SetBaseRotation(const FRotator& BaseRot)
{
}

FRotator FCustomHMD::GetBaseRotation() const
{
	return FRotator::ZeroRotator;
}

void FCustomHMD::SetBaseOrientation(const FQuat& BaseOrient)
{
}

FQuat FCustomHMD::GetBaseOrientation() const
{
	return FQuat::Identity;
}

void FCustomHMD::DrawDistortionMesh_RenderThread(struct FRenderingCompositePassContext& Context, const FIntPoint& TextureSize)
{
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

bool FCustomHMD::IsStereoEnabled() const
{
	return true;
}

bool FCustomHMD::EnableStereo(bool stereo)
{
	return true;
}

void FCustomHMD::AdjustViewRect(EStereoscopicPass StereoPass, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const
{
	SizeX = SizeX / 2;
	if( StereoPass == eSSP_RIGHT_EYE )
	{
		X += SizeX;
	}
}

void FCustomHMD::CalculateStereoViewOffset(const enum EStereoscopicPass StereoPassType, const FRotator& ViewRotation, const float WorldToMeters, FVector& ViewLocation)
{
	if( StereoPassType != eSSP_FULL)
	{
		float EyeOffset = 3.20000005f;
		const float PassOffset = (StereoPassType == eSSP_LEFT_EYE) ? EyeOffset : -EyeOffset;
		ViewLocation += ViewRotation.Quaternion().RotateVector(FVector(0,PassOffset,0));
	}
}

FMatrix FCustomHMD::GetStereoProjectionMatrix(const enum EStereoscopicPass StereoPassType, const float FOV) const
{
	const float ProjectionCenterOffset = 0.151976421f;
	const float PassProjectionOffset = (StereoPassType == eSSP_LEFT_EYE) ? ProjectionCenterOffset : -ProjectionCenterOffset;

	const float HalfFov = 2.19686294f / 2.f;
	const float InWidth = 640.f;
	const float InHeight = 480.f;
	const float XS = 1.0f / tan(HalfFov);
	const float YS = InWidth / tan(HalfFov) / InHeight;

	const float InNearZ = GNearClippingPlane;
	return FMatrix(
		FPlane(XS,                      0.0f,								    0.0f,							0.0f),
		FPlane(0.0f,					YS,	                                    0.0f,							0.0f),
		FPlane(0.0f,	                0.0f,								    0.0f,							1.0f),
		FPlane(0.0f,					0.0f,								    InNearZ,						0.0f))

		* FTranslationMatrix(FVector(PassProjectionOffset,0,0));
}

void FCustomHMD::InitCanvasFromView(FSceneView* InView, UCanvas* Canvas)
{
}

void FCustomHMD::GetEyeRenderParams_RenderThread(const FRenderingCompositePassContext& Context, FVector2D& EyeToSrcUVScaleValue, FVector2D& EyeToSrcUVOffsetValue) const
{
	EyeToSrcUVOffsetValue = FVector2D::ZeroVector;
	EyeToSrcUVScaleValue = FVector2D(1.0f, 1.0f);
}


void FCustomHMD::SetupViewFamily(FSceneViewFamily& InViewFamily)
{
	InViewFamily.EngineShowFlags.MotionBlur = 0;
	InViewFamily.EngineShowFlags.HMDDistortion = true;
	InViewFamily.EngineShowFlags.SetScreenPercentage(true);
	InViewFamily.EngineShowFlags.StereoRendering = IsStereoEnabled();
}

void FCustomHMD::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
	InView.BaseHmdOrientation = FQuat(FRotator(0.0f,0.0f,0.0f));
	InView.BaseHmdLocation = FVector(0.f);
//	WorldToMetersScale = InView.WorldToMetersScale;
	InViewFamily.bUseSeparateRenderTarget = false;
}

void FCustomHMD::PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView)
{
	check(IsInRenderingThread());
}

void FCustomHMD::PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& ViewFamily)
{
	check(IsInRenderingThread());
}

FCustomHMD::BridgeBaseImpl* FCustomHMD::GetActiveRHIBridgeImpl() {
  return pD3D11Bridge;
}

FCustomHMD::FCustomHMD() :
  CurHmdOrientation(FQuat::Identity),
  LastHmdOrientation(FQuat::Identity),
  DeltaControlRotation(FRotator::ZeroRotator),
  DeltaControlOrientation(FQuat::Identity),
  LastSensorTime(-1.0),
  WindowMirrorMode(2),
  TurboJpegLibraryHandle(0)
{
  static const FName RendererModuleName("Renderer");
  RendererModule = FModuleManager::GetModulePtr<IRendererModule>(RendererModuleName);

#if PLATFORM_WINDOWS
  if (IsPCPlatform(GMaxRHIShaderPlatform) && !IsOpenGLPlatform(GMaxRHIShaderPlatform)) {
    pD3D11Bridge = new D3D11Bridge(this);
  }
#endif

  FString BaseDir = IPluginManager::Get().FindPlugin("CustomHMD")->GetBaseDir();

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

FCustomHMD::~FCustomHMD()
{
  FPlatformProcess::FreeDllHandle(TurboJpegLibraryHandle);
  TurboJpegLibraryHandle = nullptr;
  FPlatformProcess::FreeDllHandle(LibUsbLibraryHandle);
  TurboJpegLibraryHandle = nullptr;
}

bool FCustomHMD::IsInitialized() const
{
	return true;
}

void FCustomHMD::UpdateViewport(bool bUseSeparateRenderTarget, const FViewport& InViewport, SViewport* ViewportWidget) {
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

void FCustomHMD::CalculateRenderTargetSize(const class FViewport& Viewport, uint32& InOutSizeX, uint32& InOutSizeY) {
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

bool FCustomHMD::NeedReAllocateViewportRenderTarget(const FViewport& Viewport) {
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

void FCustomHMD::ConnectUsb() {
  // Try to find the non-accessory device (hardcoded Nexus 4).
  TSharedPtr<MayaUsbDevice> tempDevice;
  int status = MayaUsbDevice::create(&tempDevice, SharedLibraryInitParams, 0x18d1, 0x4ee2);
  if (!status) {
    tempDevice->convertToAccessory();

    // Wait for device re-renumeration.
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  TSharedPtr<MayaUsbDevice> realDevice;
  status = MayaUsbDevice::create(&realDevice, SharedLibraryInitParams);
  if (!status) {
    UE_LOG(LogTemp, Warning, TEXT("CONNECTED!"));

    FScopeLock lock(&ActiveUsbDeviceMutex);
    ActiveUsbDevice = realDevice;
    ActiveUsbDevice->waitHandshakeAsync([this](bool success) {
      if (success) {
        FinishHandshake();
      } else {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("UsbHandshakeFailed", "USB handshake failure."));
      }
    });
    return;
  }

  FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("UsbNoDevice", "Could not find the USB device."));
}

void FCustomHMD::DisconnectUsb() {
  FScopeLock lock(&ActiveUsbDeviceMutex);
  ActiveUsbDevice = nullptr;
}

void FCustomHMD::FinishHandshake() {
  FScopeLock lock(&ActiveUsbDeviceMutex);

  // Set up the send loop.
  ActiveUsbDevice->beginSendLoop([this](int reason) {
    FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("UsbConnectionFailed", "The USB connection failed ({0})."), FText::AsNumber(reason)));
    ActiveUsbDevice = nullptr;
  });

  // Set up the receive loop.
  ActiveUsbDevice->beginReadLoop([this](const unsigned char* data) {
    // Don't do anything right now.
  }, 4 * sizeof(float));
}
