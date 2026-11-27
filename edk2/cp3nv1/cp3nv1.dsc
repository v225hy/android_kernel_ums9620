[Defines]
  VENDOR_NAME                    = Coolpad
  PLATFORM_NAME                  = cp3nv1
  PLATFORM_GUID                  = ad0b2e51-2aa1-4b2f-856c-5204e4e4c294
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010019
  OUTPUT_DIRECTORY               = Build/$(PLATFORM_NAME)
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platform/Spreadtrum/ums9620_t157/ums9620_t157.fdf
  DEVICE_DXE_FV_COMPONENTS       = Platform/Coolpad/ums9620_t157/cp3nv1/cp3nv1.fdf.inc
  DEFINE USE_CUSTOM_FBDXE        = TRUE

!include Platform/Spreadtrum/ums9620_t157/ums9620_t157.dsc

[BuildOptions.common]
  GCC:*_*_AARCH64_CC_FLAGS = -DENABLE_SIMPLE_INIT -DLOW_RAM

[PcdsFixedAtBuild.common]
  # CP3NV1: 1080x2408, U-Boot leaves the panel scanning a 32-bit buffer at
  # 0xB0000000. Keep the Y60 RGB565->32bpp shadow GOP path for the first test.
  gSprdTokenSpaceGuid.PcdMipiFrameBufferWidth|1080
  gSprdTokenSpaceGuid.PcdMipiFrameBufferHeight|2408
  gSprdTokenSpaceGuid.PcdMipiFrameBufferAddress|0xB0000000
  gSprdTokenSpaceGuid.PcdMipiDisplayInitialized|TRUE

  gRenegadePkgTokenSpaceGuid.PcdDeviceVendor|"Coolpad"
  gRenegadePkgTokenSpaceGuid.PcdDeviceProduct|"CP3NV1"
  gRenegadePkgTokenSpaceGuid.PcdDeviceCodeName|"CP3NV1"

[PcdsPatchableInModule.common]
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutColumn|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutRow|0
