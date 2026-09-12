#include <Windows.h>
#include <cstdio>
#include <cstdlib>

#include <atlbase.h>
#include <d3d11.h>
#include <dxgi1_6.h>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "dxguid.lib")

int main (int argc, char** argv)
{
  float max_cll = 0.0f;

  // Output is otherwise lost when stdout is redirected
  setvbuf (stdout, nullptr, _IONBF, 0);

  if (argc == 2)
  {
    char*  end    = nullptr;
    double parsed = strtod (argv [1], &end);

    if (end == argv [1] || *end != '\0' || parsed <= 0.0 || parsed > 10000.0)
    {
      printf ("Usage: %s [MaxCLL in nits, 0 < value <= 10000]\n", argv [0]);
      return 1;
    }

    max_cll = static_cast <float> (parsed);
  }

  CComPtr <IDXGIFactory>                         pFactory;
  CreateDXGIFactory (IID_IDXGIFactory, (void **)&pFactory.p);

  // ... wtf?
  if (! pFactory)
    return 0;

  int                    AdapterIdx = 0;
  CComPtr <IDXGIAdapter> pAdapter;

  while (SUCCEEDED (pFactory->EnumAdapters (AdapterIdx++, &pAdapter.p)))
  {
    int                   OutputIdx = 0;
    CComPtr <IDXGIOutput> pOutput;

    while (SUCCEEDED (pAdapter->EnumOutputs (OutputIdx++, &pOutput.p)))
    {
      CComQIPtr <IDXGIOutput6>
          pOutput6 ( pOutput );
      if (pOutput6 != nullptr)
      {
        DXGI_OUTPUT_DESC1    outDesc1 = { };
        if (FAILED (pOutput6->GetDesc1 (&outDesc1)))
        {
          printf ("Skipped Display: GetDesc1 failed\n");
          pOutput = nullptr;
          continue;
        }

        UINT Width  = outDesc1.DesktopCoordinates.right  -
                      outDesc1.DesktopCoordinates.left;
        UINT Height = outDesc1.DesktopCoordinates.bottom -
                      outDesc1.DesktopCoordinates.top;

        if (outDesc1.ColorSpace != DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
        {
          printf ("Skipped Display: %ws (ColorSpace=%d, not HDR10)\n",
                  outDesc1.DeviceName, outDesc1.ColorSpace);
        }

        else if (max_cll == 0.0f && outDesc1.MaxLuminance <= 0.0f)
        {
          printf ("Skipped Display: %ws (reports MaxLuminance=%f; pass a "
                  "MaxCLL override on the command line)\n",
                  outDesc1.DeviceName, outDesc1.MaxLuminance);
        }

        else
        {
          HWND hWnd =
            CreateWindow (
              L"static", L"HDR10", WS_VISIBLE | WS_POPUP        |
                  WS_MINIMIZEBOX | WS_SYSMENU | WS_CLIPCHILDREN |
                  WS_CLIPSIBLINGS, outDesc1.DesktopCoordinates.left,
                                   outDesc1.DesktopCoordinates.top,
                                     Width, Height,
                                      0, 0, 0, 0
            );

          if (hWnd == nullptr)
          {
            printf ("Skipped Display: %ws (CreateWindow failed: %lu)\n",
                    outDesc1.DeviceName, GetLastError ());
            pOutput = nullptr;
            continue;
          }

          DXGI_SWAP_CHAIN_DESC
            swapDesc                        = { };
            swapDesc.BufferDesc.Width       = Width;
            swapDesc.BufferDesc.Height      = Height;
            swapDesc.BufferDesc.Format      = DXGI_FORMAT_R10G10B10A2_UNORM;
            swapDesc.BufferDesc.RefreshRate = { 0, 0 };
            swapDesc.BufferCount            = 3;
            swapDesc.Windowed               = TRUE;
            swapDesc.OutputWindow           = hWnd;
            swapDesc.SampleDesc             = { 1, 0 };
            swapDesc.SwapEffect             = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            swapDesc.BufferUsage            = DXGI_USAGE_BACK_BUFFER;
            swapDesc.Flags                  = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

          CComPtr <IDXGISwapChain> pSwapChain;

          if ( SUCCEEDED (
                 D3D11CreateDeviceAndSwapChain (
                    nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0x0, nullptr,
                      0, D3D11_SDK_VERSION, &swapDesc, &pSwapChain.p,
                        nullptr, nullptr, nullptr ) )
             )
          {
            CComQIPtr <IDXGISwapChain4>
                pSwapChain4 (pSwapChain);
            if (pSwapChain4 != nullptr)
            {
              if (max_cll != 0.0f)
              {
                outDesc1.MaxLuminance = max_cll;
              }

              // Mastering luminance is in units of 0.0001 nits; MaxCLL and
              //   MaxFALL are in whole nits and only 16 bits wide.
              float  peak_nits = outDesc1.MaxLuminance;
              UINT16 peak_u16  = static_cast <UINT16> (
                peak_nits > 65535.0f ? 65535.0f : peak_nits + 0.5f );

              DXGI_HDR_METADATA_HDR10
                metadata                           = { };
                metadata.MinMasteringLuminance     =  0;
                metadata.MaxMasteringLuminance     =
                  static_cast <UINT>   (peak_nits * 10000.0f);
                metadata.MaxFrameAverageLightLevel = peak_u16;
                metadata.MaxContentLightLevel      = peak_u16;
                metadata.WhitePoint   [0]          =
                  static_cast <UINT16> (outDesc1.WhitePoint   [0] * 50000.0F);
                metadata.WhitePoint   [1]          =
                  static_cast <UINT16> (outDesc1.WhitePoint   [1] * 50000.0F);
                metadata.BluePrimary  [0]          =
                  static_cast <UINT16> (outDesc1.BluePrimary  [0] * 50000.0F);
                metadata.BluePrimary  [1]          =
                  static_cast <UINT16> (outDesc1.BluePrimary  [1] * 50000.0F);
                metadata.RedPrimary   [0]          =
                  static_cast <UINT16> (outDesc1.RedPrimary   [0] * 50000.0F);
                metadata.RedPrimary   [1]          =
                  static_cast <UINT16> (outDesc1.RedPrimary   [1] * 50000.0F);
                metadata.GreenPrimary [0]          =
                  static_cast <UINT16> (outDesc1.GreenPrimary [0] * 50000.0F);
                metadata.GreenPrimary [1]          =
                  static_cast <UINT16> (outDesc1.GreenPrimary [1] * 50000.0F);

              if (FAILED (pSwapChain4->SetColorSpace1 (
                            DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 )))
              {
                printf (" Warning: SetColorSpace1 failed on %ws\n",
                        outDesc1.DeviceName);
              }

              if ( SUCCEEDED (
                     pSwapChain4->SetFullscreenState (TRUE, nullptr))
                 )
              {
                // Ensure that the display mode is changed
                DXGI_MODE_DESC
                  modeDesc        = {   };
                  modeDesc.Width  = Width;
                  modeDesc.Height = Height;
                  modeDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;

                HRESULT hrMode =
                  pOutput6->FindClosestMatchingMode (&modeDesc,
                                                     &modeDesc, nullptr);
                if (SUCCEEDED (hrMode))
                {
                  modeDesc.RefreshRate =
                    { modeDesc.RefreshRate.Numerator/3,
                      modeDesc.RefreshRate.Denominator };

                  hrMode =
                    pOutput6->FindClosestMatchingMode (&modeDesc,
                                                       &modeDesc, nullptr);
                }

                if (SUCCEEDED (hrMode))
                  hrMode = pSwapChain4->ResizeTarget (&modeDesc);

                if (FAILED (hrMode))
                {
                  printf (" Warning: display mode switch failed on %ws "
                          "(0x%08X)\n", outDesc1.DeviceName, hrMode);
                }

                Sleep                (  25);
                pSwapChain4->Present (1, 0);
              }

              else
              {
                printf (" Warning: SetFullscreenState failed on %ws\n",
                        outDesc1.DeviceName);
              }

              if ( SUCCEEDED (
                     pSwapChain4->SetHDRMetaData (
                       DXGI_HDR_METADATA_TYPE_HDR10, sizeof (metadata),
                                                            &metadata ) )
                 )
              {
                printf ("Sanitized Display: %ws\n", outDesc1.DeviceName);
                printf (" MaxCLL=%u nits\n\n",      peak_u16);

                pSwapChain4->Present (1, 0);
                               Sleep ( 250);
                pSwapChain4->Present (1, 0);
              }

              else
              {
                printf ("Failed to set HDR metadata on %ws\n",
                        outDesc1.DeviceName);
              }

              // Releasing a swap chain that is still fullscreen terminates
              //   the process, so no further displays would be processed.
              pSwapChain4->SetFullscreenState (FALSE, nullptr);
            }
          }

          DestroyWindow (hWnd);
        }
      }

      pOutput = nullptr;
    }

    pAdapter = nullptr;
  }

  return 0;
}
