// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "ICardboardTetheringPlugin.h"
#include "HeadMountedDisplay.h"
#include "IHeadMountedDisplay.h"
#include "SceneViewExtension.h"
#include "LibraryInitParams.h"
#include "UsbDevice.h"
#include <atomic>
#include <cstdint>

#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#include <d3d11.h>
#include "HideWindowsPlatformTypes.h"
#endif

/**
 * Simple Head Mounted Display
 */
class FCardboardTethering : public IHeadMountedDisplay, public ISceneViewExtension, public TSharedFromThis<FCardboardTethering, ESPMode::ThreadSafe> {
public:
  /** IHeadMountedDisplay interface */
  virtual bool IsHMDConnected() override { return true; }
  virtual bool IsHMDEnabled() const override;
  virtual void EnableHMD(bool allow = true) override;
  virtual EHMDDeviceType::Type GetHMDDeviceType() const override;
  virtual bool GetHMDMonitorInfo(MonitorInfo&) override;

  virtual void GetFieldOfView(float& OutHFOVInDegrees, float& OutVFOVInDegrees) const override;

  virtual bool DoesSupportPositionalTracking() const override;
  virtual bool HasValidTrackingPosition() override;
  virtual void GetPositionalTrackingCameraProperties(FVector& OutOrigin, FQuat& OutOrientation, float& OutHFOV, float& OutVFOV, float& OutCameraDistance, float& OutNearPlane, float& OutFarPlane) const override;
  virtual void RebaseObjectOrientationAndPosition(FVector& OutPosition, FQuat& OutOrientation) const override;

  virtual void SetInterpupillaryDistance(float NewInterpupillaryDistance) override;
  virtual float GetInterpupillaryDistance() const override;

  virtual void GetCurrentOrientationAndPosition(FQuat& CurrentOrientation, FVector& CurrentPosition) override;
  virtual TSharedPtr<class ISceneViewExtension, ESPMode::ThreadSafe> GetViewExtension() override;
  virtual void ApplyHmdRotation(APlayerController* PC, FRotator& ViewRotation) override;
  virtual bool UpdatePlayerCamera(FQuat& CurrentOrientation, FVector& CurrentPosition) override;

  virtual bool IsChromaAbCorrectionEnabled() const override;

  virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;

  virtual bool IsPositionalTrackingEnabled() const override;
  virtual bool EnablePositionalTracking(bool enable) override;

  virtual bool IsHeadTrackingAllowed() const override;

  virtual bool IsInLowPersistenceMode() const override;
  virtual void EnableLowPersistenceMode(bool Enable = true) override;

  virtual void ResetOrientationAndPosition(float yaw = 0.f) override;
  virtual void ResetOrientation(float Yaw = 0.f) override;
  virtual void ResetPosition() override;

  virtual void SetClippingPlanes(float NCP, float FCP) override;

  virtual void SetBaseRotation(const FRotator& BaseRot) override;
  virtual FRotator GetBaseRotation() const override;

  virtual void SetBaseOrientation(const FQuat& BaseOrient) override;
  virtual FQuat GetBaseOrientation() const override;

  virtual void DrawDistortionMesh_RenderThread(struct FRenderingCompositePassContext& Context, const FIntPoint& TextureSize) override;

  /** IStereoRendering interface */
  virtual bool IsStereoEnabled() const override;
  virtual bool EnableStereo(bool stereo = true) override;
  virtual void AdjustViewRect(EStereoscopicPass StereoPass, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const override;
  virtual void CalculateStereoViewOffset(const EStereoscopicPass StereoPassType, const FRotator& ViewRotation,
    const float MetersToWorld, FVector& ViewLocation) override;
  virtual FMatrix GetStereoProjectionMatrix(const EStereoscopicPass StereoPassType, const float FOV) const override;
  virtual void InitCanvasFromView(FSceneView* InView, UCanvas* Canvas) override;
  virtual void RenderTexture_RenderThread(FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef BackBuffer, FTexture2DRHIParamRef SrcTexture) const override;
  virtual void GetEyeRenderParams_RenderThread(const struct FRenderingCompositePassContext& Context, FVector2D& EyeToSrcUVScaleValue, FVector2D& EyeToSrcUVOffsetValue) const override;
  virtual void CalculateRenderTargetSize(const class FViewport& Viewport, uint32& InOutSizeX, uint32& InOutSizeY) override;
  virtual bool NeedReAllocateViewportRenderTarget(const FViewport& Viewport) override;
  virtual bool ShouldUseSeparateRenderTarget() const override {
    check(IsInGameThread());
    return IsStereoEnabled();
  }
  virtual void UpdateViewport(bool bUseSeparateRenderTarget, const FViewport& Viewport, SViewport*) override;

  /** ISceneViewExtension interface */
  virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override;
  virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override;
  virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) {}
  virtual void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override;
  virtual void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override;

  class BridgeBaseImpl : public FRHICustomPresent {
  public:
    BridgeBaseImpl(FCardboardTethering* plugin) :
      FRHICustomPresent(nullptr),
      Plugin(plugin),
      bNeedReinitRendererAPI(true),
      bInitialized(false) {}

    bool IsInitialized() const { return bInitialized; }

    virtual void BeginRendering() = 0;
    virtual void UpdateViewport(const FViewport& Viewport, FRHIViewport* InViewportRHI) = 0;
    virtual void SetNeedReinitRendererAPI() { bNeedReinitRendererAPI = true; }

    virtual void Reset() = 0;
    virtual void Shutdown() = 0;

  protected:
    FCardboardTethering* Plugin;
    bool bNeedReinitRendererAPI;
    bool bInitialized;
  };

#if PLATFORM_WINDOWS
  class D3D11Bridge : public BridgeBaseImpl {
  public:
    D3D11Bridge(FCardboardTethering* plugin);

    virtual void OnBackBufferResize() override;
    virtual bool Present(int& SyncInterval) override;

    virtual void BeginRendering() override;
    void FinishRendering();
    virtual void UpdateViewport(const FViewport& Viewport, FRHIViewport* InViewportRHI) override;
    virtual void Reset() override;
    virtual void Shutdown() override {
      Reset();
    }

  protected:
    ID3D11Texture2D* RenderTargetTexture = NULL;
  };
#endif // PLATFORM_WINDOWS

  BridgeBaseImpl* GetActiveRHIBridgeImpl();

public:
  /** Constructor */
  FCardboardTethering();

  /** Destructor */
  virtual ~FCardboardTethering();

  /** @return	True if the HMD was initialized OK */
  bool IsInitialized() const;

  TSharedPtr<LibraryInitParams> SharedLibraryInitParams;
  FCriticalSection ActiveUsbDeviceMutex;
  TSharedPtr<UsbDevice> ActiveUsbDevice;

  bool GetCachedConnectionState() const;
  void ShowConnectUsbDialog();
  void DisconnectUsb();

private:
  FQuat CurHmdOrientation;
  FQuat LastHmdOrientation;

  FRotator DeltaControlRotation;    // same as DeltaControlOrientation but as rotator
  FQuat DeltaControlOrientation; // same as DeltaControlRotation but as quat

  double LastSensorTime;
  int32 WindowMirrorMode;
  IRendererModule* RendererModule;
  void* TurboJpegLibraryHandle;
  void* LibUsbLibraryHandle;

  std::atomic<float> FeedbackOrientationX;
  std::atomic<float> FeedbackOrientationY;
  std::atomic<float> FeedbackOrientationZ;
  std::atomic<float> FeedbackOrientationW;

  std::atomic<int32_t> ViewerWidth;
  std::atomic<int32_t> ViewerHeight;
  std::atomic<float> ViewerInterpupillary;

  FCriticalSection StatusWindowMutex;
  TSharedPtr<SWindow> StatusWindow;

  struct ConnectDialogState {
    std::vector<UsbDeviceDesc> list;
    int selectedItem;
    ConnectDialogState() : selectedItem(-1) {}
    ConnectDialogState(std::vector<UsbDeviceDesc>& newList) {
      list = newList;
      if (list.size() > 0) {
        selectedItem = 0;
      } else {
        selectedItem = -1;
      }
    }
  };

  FCriticalSection ConnectDialogMutex;
  TSharedPtr<SWindow> ConnectDialog;
  ConnectDialogState DialogState;

  bool CachedConnectionState;

#if PLATFORM_WINDOWS
  TRefCountPtr<D3D11Bridge>	pD3D11Bridge;
#endif

  void GetCurrentPose(FQuat& CurrentOrientation);
  void ConnectUsb(uint16_t vid = 0x18d1, uint16_t pid = 0x4ee2);
  void DisconnectUsb(int reason);
  void FinishHandshake();

  void OpenDialogOnGameThread(FText msg);
  void OpenStatusWindowOnGameThread(FText msg, std::function<FReply(void)> cancelHandler);
  void CloseStatusWindowOnGameThread();

  static FText GetDeviceLabel(const UsbDeviceDesc& device);
  static FText GetDeviceTooltip(const UsbDeviceDesc& device);
};

DEFINE_LOG_CATEGORY_STATIC(LogHMD, Log, All);
