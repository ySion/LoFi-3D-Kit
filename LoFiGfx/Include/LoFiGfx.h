//
// Created by starr on 2024/6/20.
//

#ifndef LOFIGFX_H
#define LOFIGFX_H
//
#ifdef LOFI_GFX_DLL_EXPORT
#define LOFI_API __declspec(dllexport)
#else
#define LOFI_API __declspec(dllimport)
#endif

#include "LoFiGfxDefines.h"
#include <vector>
#include <string_view>


extern "C" {
      void GfxInit();

      void GfxClose();

      //Resource Create or Destroy

      LOFI_API GfxHandle GfxFindResourceByName(const char* resource_name);

      LOFI_API GfxHandle GfxCreateSwapChain(const GfxParamCreateSwapchain& param = {});

      LOFI_API GfxHandle GfxCreateTexture2D(GfxEnumFormat format, uint32_t w, uint32_t h, const GfxParamCreateTexture2D& param = {});

      LOFI_API GfxHandle GfxCreateTexture2DFromFile(const char* file_path, const GfxParamCreateTexture2D& param = {});

      LOFI_API GfxHandle GfxCreateBuffer(const GfxParamCreateBuffer& param = {});

      LOFI_API GfxHandle GfxCreateBuffer3F(const GfxParamCreateBuffer3F& param = {});

      LOFI_API GfxHandle GfxCreateProgram(const GfxParamCreateProgram& param);

      LOFI_API GfxHandle GfxCreateProgramFromFile(const GfxParamCreateProgramFromFile& param);

      LOFI_API GfxHandle GfxCreateKernel(GfxHandle program, const GfxParamCreateKernel& param = {});

      LOFI_API GfxHandle GfxCreateRDGNode(const GfxParamCreateRenderNode& param);

      LOFI_API void GfxDestroy(GfxHandle resource);

      //Modifier
      LOFI_API void GfxSetRootRDGNode(GfxHandle node);

      LOFI_API bool GfxSetRDGNodeWaitFor(GfxHandle node, const GfxInfoRenderNodeWait& param);

      LOFI_API bool GfxSetKernelConstant(GfxHandle kernel, const char* name, const void* data);

      LOFI_API bool GfxFillKernelConstant(GfxHandle kernel, const void* data, size_t size);

      LOFI_API bool GfxUploadBuffer(GfxHandle buffer, const void* data, uint64_t size);

      LOFI_API bool GfxResizeBuffer(GfxHandle buffer, uint64_t size);

      LOFI_API bool GfxUploadTexture2D(GfxHandle texture, const void* data, uint64_t size);

      LOFI_API bool GfxResizeTexture2D(GfxHandle texture, uint32_t w, uint32_t h);

      //Info Get

      LOFI_API GfxRDGNodeCore GfxGetRDGNodeCore(GfxHandle node);

      LOFI_API GfxInfoKernelLayout GfxGetKernelLayout(GfxHandle kernel);

      LOFI_API uint32_t GfxGetTextureBindlessIndex(GfxHandle texture);

      LOFI_API uint64_t GfxGetBufferBindlessAddress(GfxHandle buffer);

      LOFI_API void* GfxGetBufferMappedAddress(GfxHandle buffer);

      //Commands

      LOFI_API void GfxGenFrame();

      LOFI_API void GfxCmdBeginComputePass(GfxRDGNodeCore nodec);

      LOFI_API void GfxCmdEndComputePass(GfxRDGNodeCore nodec);

      LOFI_API void GfxCmdComputeDispatch(GfxRDGNodeCore nodec, uint32_t x, uint32_t y, uint32_t z);

      LOFI_API void GfxCmdBeginRenderPass(GfxRDGNodeCore nodec, const GfxParamBeginRenderPass& textures);

      LOFI_API void GfxCmdEndRenderPass(GfxRDGNodeCore nodec);

      LOFI_API void GfxCmdBindKernel(GfxRDGNodeCore nodec, GfxHandle kernel);

      LOFI_API void GfxCmdBindVertexBuffer(GfxRDGNodeCore nodec, GfxHandle vertex_buffer, uint32_t first_binding = 0, uint32_t binding_count = 1, size_t offset = 0);

      LOFI_API void GfxCmdBindIndexBuffer(GfxRDGNodeCore nodec, GfxHandle index_buffer, size_t offset = 0);

      LOFI_API void GfxCmdDrawIndexedIndirect(GfxRDGNodeCore nodec, GfxHandle indirect_bufer, size_t offset, uint32_t draw_count, uint32_t stride);

      LOFI_API void GfxCmdDrawIndex(GfxRDGNodeCore nodec, uint32_t index_count, uint32_t instance_count = 1, uint32_t first_index = 0, int32_t vertex_offset = 0, uint32_t first_instance = 0);

      LOFI_API void GfxCmdDraw(GfxRDGNodeCore nodec, uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex = 0, uint32_t first_instance = 0);

      LOFI_API void GfxCmdSetViewport(GfxRDGNodeCore nodec, float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f);

      LOFI_API void GfxCmdSetScissor(GfxRDGNodeCore nodec, uint32_t x, uint32_t y, uint32_t width, uint32_t height);

      LOFI_API void GfxCmdSetViewportAuto(GfxRDGNodeCore nodec, bool invert_y = true);

      LOFI_API void GfxCmdSetScissorAuto(GfxRDGNodeCore nodec);

      LOFI_API void GfxCmdPushConstant(GfxRDGNodeCore nodec, GfxInfoKernelLayout kernel_layout, const void* data, size_t data_size);

      LOFI_API void GfxCmdAsSampledTexure(GfxRDGNodeCore nodec, GfxHandle texture, GfxEnumKernelType which_kernel_use = GfxEnumKernelType::OUT_OF_KERNEL);

      LOFI_API void GfxCmdAsReadTexure(GfxRDGNodeCore nodec, GfxHandle texture, GfxEnumKernelType which_kernel_use = GfxEnumKernelType::OUT_OF_KERNEL);

      LOFI_API void GfxCmdAsWriteTexture(GfxRDGNodeCore nodec, GfxHandle texture, GfxEnumKernelType which_kernel_use = GfxEnumKernelType::OUT_OF_KERNEL);

      LOFI_API void GfxCmdAsWriteBuffer(GfxRDGNodeCore nodec, GfxHandle buffer, GfxEnumKernelType which_kernel_use = GfxEnumKernelType::OUT_OF_KERNEL);

      LOFI_API void GfxCmdAsReadBuffer(GfxRDGNodeCore nodec, GfxHandle buffer, GfxEnumKernelType which_kernel_use = GfxEnumKernelType::OUT_OF_KERNEL);

      //2D

      LOFI_API Gfx2DCanvas Gfx2DCreateCanvas();

      LOFI_API void Gfx2DDestroy(Gfx2DCanvas canvas);

      LOFI_API bool Gfx2DLoadFont(Gfx2DCanvas canvas, const char* font_path);

      LOFI_API void Gfx2DCmdPushCanvasSize(Gfx2DCanvas canvas, uint16_t w, uint16_t h);

      LOFI_API void Gfx2DCmdPopCanvasSize(Gfx2DCanvas canvas);

      LOFI_API void Gfx2DCmdPushScissor(Gfx2DCanvas canvas, uint16_t x, uint16_t y, uint16_t w, uint16_t h);

      LOFI_API void Gfx2DCmdPopScissor(Gfx2DCanvas canvas); //new draw call

      // void Gfx2DCmdPushShadow(Gfx2DCanvas canvas,const ShadowParameter& shadow_parameter);
      //
      // void Gfx2DCmdPopShadow(Gfx2DCanvas canvas);
      //
      LOFI_API void Gfx2DCmdPushStrock(Gfx2DCanvas canvas, const Gfx2DParamStrockType& strock_parameter);

      LOFI_API void Gfx2DCmdPopStrock(Gfx2DCanvas canvas);

      LOFI_API void Gfx2DCmdDrawBox(Gfx2DCanvas canvas, GfxVec2 start, Gfx2DParamBox param, float rotation = 0, GfxColor color = {255, 255, 255, 255});

      LOFI_API void Gfx2DCmdDrawRoundBox(Gfx2DCanvas canvas, GfxVec2 start, Gfx2DParamRoundBox param, float rotation = 0, GfxColor color = {255, 255, 255, 255});

      LOFI_API void Gfx2DCmdDrawNGon(Gfx2DCanvas canvas, GfxVec2 center, Gfx2DParamRoundNGon param, float rotation = 0, GfxColor color = {255, 255, 255, 255});

      LOFI_API void Gfx2DCmdDrawCircle(Gfx2DCanvas canvas, GfxVec2 center, Gfx2DParamCircle param, float rotation = 0, GfxColor color = {255, 255, 255, 255});

      LOFI_API void Gfx2DCmdDrawText(Gfx2DCanvas canvas, GfxVec2 start, const wchar_t* text, Gfx2DParamText param, GfxColor color);

      LOFI_API void Gfx2DReset(Gfx2DCanvas canvas);

      LOFI_API void Gfx2DEmitDrawCommand(Gfx2DCanvas canvas, GfxRDGNodeCore nodec);

      LOFI_API void Gfx2DDispatchGenerateCommands(Gfx2DCanvas canvas);
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
