//
// Created by starr on 2024/6/20.
//

#ifndef LOFIGFX_H
#define LOFIGFX_H
//
// #ifdef LOFI_GFX_DLL_EXPORT
// #define LOFI_API __declspec(dllexport)
// #else
// #define LOFI_API __declspec(dllimport)
// #endif

#include "LoFiGfxDefines.h"
#include <vector>
#include <string_view>

namespace LoFi {
      struct PParamRoundNGon;
}

extern "C" {
      void GfxInit();

      void GfxClose();

      //Resource Create or Destroy

      GfxHandle GfxFindResourceByName(const char* resource_name);

      GfxHandle GfxCreateSwapChain(const GfxParamCreateSwapchain& param = {});

      GfxHandle GfxCreateTexture2D(GfxEnumFormat format, uint32_t w, uint32_t h, const GfxParamCreateTexture2D& param = {});

      GfxHandle GfxCreateTexture2DFromFile(const char* file_path, const GfxParamCreateTexture2D& param = {});

      GfxHandle GfxCreateBuffer(const GfxParamCreateBuffer& param = {});

      GfxHandle GfxCreateBuffer3F(const GfxParamCreateBuffer3F& param = {});

      GfxHandle GfxCreateProgram(const GfxParamCreateProgram& param);

      GfxHandle GfxCreateProgramFromFile(const GfxParamCreateProgramFromFile& param);

      GfxHandle GfxCreateKernel(GfxHandle program, const GfxParamCreateKernel& param = {});

      GfxHandle GfxCreateRDGNode(const GfxParamCreateRenderNode& param);

      void GfxDestroy(GfxHandle resource);

      //Modifier
      void GfxSetRootRDGNode(GfxHandle node);

      bool GfxSetRDGNodeWaitFor(GfxHandle node, const GfxInfoRenderNodeWait& param);

      bool GfxSetKernelConstant(GfxHandle kernel, const char* name, const void* data);

      bool GfxFillKernelConstant(GfxHandle kernel, const void* data, size_t size);

      bool GfxUploadBuffer(GfxHandle buffer, const void* data, uint64_t size);

      bool GfxResizeBuffer(GfxHandle buffer, uint64_t size);

      bool GfxUploadTexture2D(GfxHandle texture, const void* data, uint64_t size);

      bool GfxResizeTexture2D(GfxHandle texture, uint32_t w, uint32_t h);

      //Info Get

      GfxRDGNodeCore GfxGetRDGNodeCore(GfxHandle node);

      GfxInfoKernelLayout GfxGetKernelLayout(GfxHandle kernel);

      uint32_t GfxGetTextureBindlessIndex(GfxHandle texture);

      uint64_t GfxGetBufferBindlessAddress(GfxHandle buffer);

      void* GfxGetBufferMappedAddress(GfxHandle buffer);

      //Commands

      void GfxGenFrame();

      void GfxCmdBeginComputePass(GfxRDGNodeCore nodec);

      void GfxCmdEndComputePass(GfxRDGNodeCore nodec);

      void GfxCmdComputeDispatch(GfxRDGNodeCore nodec, uint32_t x, uint32_t y, uint32_t z);

      void GfxCmdBeginRenderPass(GfxRDGNodeCore nodec, const GfxParamBeginRenderPass& textures);

      void GfxCmdEndRenderPass(GfxRDGNodeCore nodec);

      void GfxCmdBindKernel(GfxRDGNodeCore nodec, GfxHandle kernel);

      void GfxCmdBindVertexBuffer(GfxRDGNodeCore nodec, GfxHandle vertex_buffer, uint32_t first_binding = 0, uint32_t binding_count = 1, size_t offset = 0);

      void GfxCmdBindIndexBuffer(GfxRDGNodeCore nodec, GfxHandle index_buffer, size_t offset = 0);

      void GfxCmdDrawIndexedIndirect(GfxRDGNodeCore nodec, GfxHandle indirect_bufer, size_t offset, uint32_t draw_count, uint32_t stride);

      void GfxCmdDrawIndex(GfxRDGNodeCore nodec, uint32_t index_count, uint32_t instance_count = 1, uint32_t first_index = 0, int32_t vertex_offset = 0, uint32_t first_instance = 0);

      void GfxCmdDraw(GfxRDGNodeCore nodec, uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex = 0, uint32_t first_instance = 0);

      void GfxCmdSetViewport(GfxRDGNodeCore nodec, float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f);

      void GfxCmdSetScissor(GfxRDGNodeCore nodec, uint32_t x, uint32_t y, uint32_t width, uint32_t height);

      void GfxCmdSetViewportAuto(GfxRDGNodeCore nodec, bool invert_y = true);

      void GfxCmdSetScissorAuto(GfxRDGNodeCore nodec);

      void GfxCmdPushConstant(GfxRDGNodeCore nodec, GfxInfoKernelLayout kernel_layout, const void* data, size_t data_size);

      void GfxCmdAsSampledTexure(GfxRDGNodeCore nodec, GfxHandle texture, GfxEnumKernelType which_kernel_use = GfxEnumKernelType::OUT_OF_KERNEL);

      void GfxCmdAsReadTexure(GfxRDGNodeCore nodec, GfxHandle texture, GfxEnumKernelType which_kernel_use = GfxEnumKernelType::OUT_OF_KERNEL);

      void GfxCmdAsWriteTexture(GfxRDGNodeCore nodec, GfxHandle texture, GfxEnumKernelType which_kernel_use = GfxEnumKernelType::OUT_OF_KERNEL);

      void GfxCmdAsWriteBuffer(GfxRDGNodeCore nodec, GfxHandle buffer, GfxEnumKernelType which_kernel_use = GfxEnumKernelType::OUT_OF_KERNEL);

      void GfxCmdAsReadBuffer(GfxRDGNodeCore nodec, GfxHandle buffer, GfxEnumKernelType which_kernel_use = GfxEnumKernelType::OUT_OF_KERNEL);

      //2D

      Gfx2DCanvas Gfx2DCreateCanvas();

      void Gfx2DDestroy(Gfx2DCanvas canvas);

      bool Gfx2DLoadFont(Gfx2DCanvas canvas, const char* font_path);

      void Gfx2DCmdPushCanvasSize(Gfx2DCanvas canvas, uint16_t w, uint16_t h);

      void Gfx2DCmdPopCanvasSize(Gfx2DCanvas canvas);

      void Gfx2DCmdPushScissor(Gfx2DCanvas canvas, uint16_t x, uint16_t y, uint16_t w, uint16_t h);

      void Gfx2DCmdPopScissor(Gfx2DCanvas canvas); //new draw call

      // void Gfx2DCmdPushShadow(Gfx2DCanvas canvas,const ShadowParameter& shadow_parameter);
      //
      // void Gfx2DCmdPopShadow(Gfx2DCanvas canvas);
      //
      void Gfx2DCmdPushStrock(Gfx2DCanvas canvas, const Gfx2DParamStrockType& strock_parameter);

      void Gfx2DCmdPopStrock(Gfx2DCanvas canvas);

      void Gfx2DCmdDrawBox(Gfx2DCanvas canvas, GfxVec2 start, Gfx2DParamBox param, float rotation = 0, GfxColor color = {255, 255, 255, 255});

      void Gfx2DCmdDrawRoundBox(Gfx2DCanvas canvas, GfxVec2 start, Gfx2DParamRoundBox param, float rotation = 0, GfxColor color = {255, 255, 255, 255});

      void Gfx2DCmdDrawNGon(Gfx2DCanvas canvas, GfxVec2 center, Gfx2DParamRoundNGon param, float rotation = 0, GfxColor color = {255, 255, 255, 255});

      void Gfx2DCmdDrawCircle(Gfx2DCanvas canvas, GfxVec2 center, Gfx2DParamCircle param, float rotation = 0, GfxColor color = {255, 255, 255, 255});

      void Gfx2DCmdDrawText(Gfx2DCanvas canvas, GfxVec2 start, const wchar_t* text, Gfx2DParamText param, GfxColor color);

      void Gfx2DReset(Gfx2DCanvas canvas);

      void Gfx2DEmitDrawCommand(Gfx2DCanvas canvas, GfxRDGNodeCore nodec);

      void Gfx2DDispatchGenerateCommands(Gfx2DCanvas canvas);
}

//C++ Method
template <class T>
GfxHandle GfxCreateTexture2D(const std::vector<T>& data, GfxEnumFormat format, uint32_t w, uint32_t h, uint32_t mipMapCounts = 1, const char* resource_name = nullptr) {
      return GfxCreateTexture2D(format, w, h, {
            .pResourceName = resource_name,
            .pData = data.data(),
            .DataSize = data.size() * sizeof(T),
            .MipMapCount = mipMapCounts
      });
}

template <class T>
GfxHandle GfxCreateBuffer(const std::vector<T>& data, const char* resource_name = nullptr, bool cpu_access = true, bool single_upload = false) {
      return GfxCreateBuffer({.pResourceName = resource_name, .pData = data.data(), .DataSize = data.size() * sizeof(T), .bSingleUpload = cpu_access, .bCpuAccess = single_upload});
}

template <class T, size_t L>
GfxHandle GfxCreateBuffer(const std::array<T, L>& data, const char* resource_name = nullptr, bool cpu_access = true, bool single_upload = false) {
      return GfxCreateBuffer({.pResourceName = resource_name, .pData = data.data(), .DataSize = data.size() * sizeof(T), .bSingleUpload = cpu_access, .bCpuAccess = single_upload});
}

inline GfxHandle GfxCreateProgram(const std::vector<const char*>& codes, const char* config, const char* resource_name = nullptr) {
      return GfxCreateProgram(GfxParamCreateProgram{
            .pResourceName = resource_name,
            .pConfig = config,
            .pSourceCodes = codes.data(),
            .countSourceCode = codes.size()
      });
}

inline GfxHandle GfxCreateProgramFromFile(const std::vector<const char*>& files_path, const char* config, const char* resource_name = nullptr) {
      return GfxCreateProgramFromFile(GfxParamCreateProgramFromFile{
            .pResourceName = resource_name,
            .pConfig = config,
            .pSourceCodeFileNames = files_path.data(),
            .countSourceCodeFileName = files_path.size()
      });
}

template <class T> requires (std::is_trivially_copyable_v<T> && (!std::is_pointer_v<T>))
bool GfxSetKernelConstant(GfxHandle kernel, const char* name, T data) { return GfxSetKernelConstant(kernel, name, (const void*)&data); }

inline void GfxCmdBeginRenderPass(GfxRDGNodeCore nodec, const std::vector<GfxInfoRenderPassaAttachment>& textures) {
      return GfxCmdBeginRenderPass(nodec, GfxParamBeginRenderPass{
            .pAttachments = textures.data(),
            .countAttachments = textures.size()
      });
}


#endif //LOFIGFX_H
