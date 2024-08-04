//
// Created by starr on 2024/6/20.
//
#include "LoFiGfx.h"
#include "PfxContext.h"
#include "GfxContext.h"
#include "Message.h"
#include "RenderNode.h"

#include "mimalloc/mimalloc.h"

LoFi::GfxContext* global_gfx = nullptr;

void GfxInit() {
      mi_version();
      mi_option_set(mi_option_verbose, 1);
      mi_option_set(mi_option_show_stats, 1);
      mi_option_set(mi_option_show_errors, 1);
      //mi_option_set(mi_option_arena_eager_commit, 2);
      //mi_option_set(mi_option_reserve_huge_os_pages, 2);
      if (!global_gfx) {
            global_gfx = new LoFi::GfxContext();
            global_gfx->Init();
      }
}

void GfxClose() {
      global_gfx->Shutdown();
      delete global_gfx;
      global_gfx = nullptr;
}

GfxHandle GfxFindResourceByName(const char* resource_name) {
      return {GfxEnumResourceType::INVALID_RESOURCE_TYPE};
}

GfxHandle GfxCreateSwapChain(const GfxParamCreateSwapchain& param) {
      return std::bit_cast<GfxHandle>(global_gfx->CreateSwapChain(param));
}

GfxHandle GfxCreateTexture2D(GfxEnumFormat format, uint32_t w, uint32_t h, const GfxParamCreateTexture2D& param) {
      return std::bit_cast<GfxHandle>(global_gfx->CreateTexture2D((VkFormat)format, w, h, param));
}

GfxHandle GfxCreateTexture2DFromFile(const char* file_path, const GfxParamCreateTexture2D& param) {
      int w, h, channels;
      unsigned char* data = stbi_load(file_path, &w, &h, &channels, 4);
      if (!data) {
            LoFi::MessageManager::Log(LoFi::MessageType::Error, "GfxCreateTexture2DFromFile Err: Load File failed.");
            return GfxHandle{GfxEnumResourceType::INVALID_RESOURCE_TYPE};
      }

      if (channels != 4) {
            stbi_image_free(data);
            LoFi::MessageManager::Log(LoFi::MessageType::Error, "GfxCreateTexture2DFromFile Err: Texture's Channel is not 4.");
            return GfxHandle{GfxEnumResourceType::INVALID_RESOURCE_TYPE};
      }

      auto loaded = GfxCreateTexture2D(GfxEnumFormat::FORMAT_R8G8B8A8_UNORM, w, h, {
            .pResourceName = param.pResourceName,
            .pData = data,
            .DataSize = (size_t)(w * h * channels),
            .MipMapCount = param.MipMapCount
      });

      stbi_image_free(data);
      return loaded;
}

GfxHandle GfxCreateBuffer(const GfxParamCreateBuffer& param) {
      return std::bit_cast<GfxHandle>(global_gfx->CreateBuffer(param));
}

GfxHandle GfxCreateBuffer3F(const GfxParamCreateBuffer3F& param) {
      return std::bit_cast<GfxHandle>(global_gfx->CreateBuffer3F(param));
}

GfxHandle GfxCreateProgram(const GfxParamCreateProgram& param) {
      return std::bit_cast<GfxHandle>(global_gfx->CreateProgram(param));
}

GfxHandle GfxCreateProgramFromFile(const GfxParamCreateProgramFromFile& param) {
      return std::bit_cast<GfxHandle>(global_gfx->CreateProgramFromFile(param));
}

GfxHandle GfxCreateKernel(GfxHandle program, const GfxParamCreateKernel& param) {
      return std::bit_cast<GfxHandle>(global_gfx->CreateKernel(std::bit_cast<LoFi::ResourceHandle>(program), param));
}

GfxHandle GfxCreateRDGNode(const GfxParamCreateRenderNode& param) {
      return std::bit_cast<GfxHandle>(global_gfx->CreateRenderNode(param));
}

void GfxDestroy(GfxHandle resource) {
      return global_gfx->DestroyHandle(std::bit_cast<LoFi::ResourceHandle>(resource));
}

//Modifier

void GfxSetRootRDGNode(GfxHandle node) {
      return global_gfx->SetRootRenderNode(std::bit_cast<LoFi::ResourceHandle>(node));
}

bool GfxSetRDGNodeWaitFor(GfxHandle node, const GfxInfoRenderNodeWait& param) {
      return global_gfx->SetRenderNodeWait(std::bit_cast<LoFi::ResourceHandle>(node), param);
}

bool GfxSetKernelConstant(GfxHandle kernel, const char* name, const void* data) {
      return global_gfx->SetKernelConstant(std::bit_cast<LoFi::ResourceHandle>(kernel), name, data);
}

bool GfxFillKernelConstant(GfxHandle kernel, const void* data, size_t size) {
      return global_gfx->FillKernelConstant(std::bit_cast<LoFi::ResourceHandle>(kernel), data, size);
}

bool GfxUploadBuffer(GfxHandle buffer, const void* data, uint64_t size) {
      return global_gfx->SetBuffer(std::bit_cast<LoFi::ResourceHandle>(buffer), data, size);
}

bool GfxResizeBuffer(GfxHandle buffer, uint64_t size) {
      return global_gfx->ResizeBuffer(std::bit_cast<LoFi::ResourceHandle>(buffer), size);
}

bool GfxUploadTexture2D(GfxHandle texture, const void* data, uint64_t size) {
      return global_gfx->SetTexture2D(std::bit_cast<LoFi::ResourceHandle>(texture), data, size);
}

bool GfxResizeTexture2D(GfxHandle texture, uint32_t w, uint32_t h) {
      return global_gfx->ResizeTexture2D(std::bit_cast<LoFi::ResourceHandle>(texture), w, h);
}

//Infos
GfxRDGNodeCore GfxGetRDGNodeCore(GfxHandle node) {
      return std::bit_cast<GfxRDGNodeCore>(global_gfx->GetRenderGraphNodePtr(std::bit_cast<LoFi::ResourceHandle>(node)));
}

GfxInfoKernelLayout GfxGetKernelLayout(GfxHandle kernel) {
      return global_gfx->GetKernelLayout(std::bit_cast<LoFi::ResourceHandle>(kernel));
}

uint32_t GfxGetTextureBindlessIndex(GfxHandle texture) {
      return global_gfx->GetTextureBindlessIndex(std::bit_cast<LoFi::ResourceHandle>(texture));
}

uint64_t GfxGetBufferBindlessAddress(GfxHandle buffer) {
      return global_gfx->GetBufferBindlessAddress(std::bit_cast<LoFi::ResourceHandle>(buffer));
}

void* GfxGetBufferMappedAddress(GfxHandle buffer) {
      return global_gfx->GetBufferMappedAddress(std::bit_cast<LoFi::ResourceHandle>(buffer));
}

//Command

void GfxGenFrame() {
      return global_gfx->GenFrame();
}

void GfxCmdBeginComputePass(GfxRDGNodeCore nodec) {
      return std::bit_cast<LoFi::RenderNode*>(nodec)->CmdBeginComputePass();
}

void GfxCmdEndComputePass(GfxRDGNodeCore nodec) {
      return std::bit_cast<LoFi::RenderNode*>(nodec)->CmdEndComputePass();
}

void GfxCmdComputeDispatch(GfxRDGNodeCore nodec, uint32_t x, uint32_t y, uint32_t z) {
      return std::bit_cast<LoFi::RenderNode*>(nodec)->CmdComputeDispatch(x, y, z);
}

void GfxCmdBeginRenderPass(GfxRDGNodeCore nodec, const GfxParamBeginRenderPass& textures) {
      return std::bit_cast<LoFi::RenderNode*>(nodec)->CmdBeginRenderPass(textures);
}

void GfxCmdEndRenderPass(GfxRDGNodeCore nodec) {
      return std::bit_cast<LoFi::RenderNode*>(nodec)->CmdEndRenderPass();
}

void GfxCmdBindKernel(GfxRDGNodeCore nodec, GfxHandle kernel) {
      return std::bit_cast<LoFi::RenderNode*>(nodec)->CmdBindKernel(std::bit_cast<LoFi::ResourceHandle>(kernel));
}

void GfxCmdBindVertexBuffer(GfxRDGNodeCore nodec, GfxHandle vertex_buffer, uint32_t first_binding, uint32_t binding_count, size_t offset) {
      return std::bit_cast<LoFi::RenderNode*>(nodec)->CmdBindVertexBuffer(std::bit_cast<LoFi::ResourceHandle>(vertex_buffer), first_binding, binding_count, offset);
}

void GfxCmdBindIndexBuffer(GfxRDGNodeCore nodec, GfxHandle index_buffer, size_t offset) {
      return std::bit_cast<LoFi::RenderNode*>(nodec)->CmdBindIndexBuffer(std::bit_cast<LoFi::ResourceHandle>(index_buffer), offset);
}

void GfxCmdDrawIndexedIndirect(GfxRDGNodeCore nodec, GfxHandle indirect_bufer, size_t offset, uint32_t draw_count, uint32_t stride) {
      return std::bit_cast<LoFi::RenderNode*>(nodec)->CmdDrawIndexedIndirect(std::bit_cast<LoFi::ResourceHandle>(indirect_bufer), offset, draw_count, stride);
}

void GfxCmdDrawIndex(GfxRDGNodeCore nodec, uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance) {
      return std::bit_cast<LoFi::RenderNode*>(nodec)->CmdDrawIndex(index_count, instance_count, first_index, vertex_offset, first_instance);
}

void GfxCmdDraw(GfxRDGNodeCore nodec, uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) {
      return std::bit_cast<LoFi::RenderNode*>(nodec)->CmdDraw(vertex_count, instance_count, first_vertex, first_instance);
}

void GfxCmdSetViewport(GfxRDGNodeCore nodec, float x, float y, float width, float height, float minDepth, float maxDepth) {
      return std::bit_cast<LoFi::RenderNode*>(nodec)->CmdSetViewport({x, y, width, height, minDepth, maxDepth});
}

void GfxCmdSetScissor(GfxRDGNodeCore nodec, int x, int y, uint32_t width, uint32_t height) {
      return std::bit_cast<LoFi::RenderNode*>(nodec)->CmdSetScissor(VkRect2D{x, y, width, height});
}

void GfxCmdSetViewportAuto(GfxRDGNodeCore nodec, bool invert_y) {
      return std::bit_cast<LoFi::RenderNode*>(nodec)->CmdSetViewportAuto(invert_y);
}

void GfxCmdSetScissorAuto(GfxRDGNodeCore nodec) {
      return std::bit_cast<LoFi::RenderNode*>(nodec)->CmdSetScissorAuto();
}

void GfxCmdPushConstant(GfxRDGNodeCore nodec, GfxInfoKernelLayout kernel_layout, const void* data, size_t data_size) {
      return std::bit_cast<LoFi::RenderNode*>(nodec)->CmdPushConstant(kernel_layout, data, data_size);
}

void GfxCmdAsSampledTexure(GfxRDGNodeCore nodec, GfxHandle texture, GfxEnumKernelType which_kernel_use) {
      return std::bit_cast<LoFi::RenderNode*>(nodec)->CmdAsSampledTexure(std::bit_cast<LoFi::ResourceHandle>(texture), which_kernel_use);
}

void GfxCmdAsReadTexure(GfxRDGNodeCore nodec, GfxHandle texture, GfxEnumKernelType which_kernel_use) {
      return std::bit_cast<LoFi::RenderNode*>(nodec)->CmdAsReadTexure(std::bit_cast<LoFi::ResourceHandle>(texture), which_kernel_use);
}

void GfxCmdAsWriteTexture(GfxRDGNodeCore nodec, GfxHandle texture, GfxEnumKernelType which_kernel_use) {
      return std::bit_cast<LoFi::RenderNode*>(nodec)->CmdAsWriteTexture(std::bit_cast<LoFi::ResourceHandle>(texture), which_kernel_use);
}

void GfxCmdAsWriteBuffer(GfxRDGNodeCore nodec, GfxHandle buffer, GfxEnumKernelType which_kernel_use) {
      return std::bit_cast<LoFi::RenderNode*>(nodec)->CmdAsWriteBuffer(std::bit_cast<LoFi::ResourceHandle>(buffer), which_kernel_use);
}

void GfxCmdAsReadBuffer(GfxRDGNodeCore nodec, GfxHandle buffer, GfxEnumKernelType which_kernel_use) {
      return std::bit_cast<LoFi::RenderNode*>(nodec)->CmdAsReadBuffer(std::bit_cast<LoFi::ResourceHandle>(buffer), which_kernel_use);
}

Gfx2DCanvas Gfx2DCreateCanvas() {
      return std::bit_cast<Gfx2DCanvas>(global_gfx->Create2DCanvas());
}

void Gfx2DDestroy(Gfx2DCanvas canvas) {
      return global_gfx->Destroy2DCanvas(canvas);
}

bool Gfx2DLoadFont(Gfx2DCanvas canvas, const char* font_path) {
      return std::bit_cast<LoFi::PfxContext*>(canvas)->GenAndLoadFont(font_path);
}

void Gfx2DCmdPushCanvasSize(Gfx2DCanvas canvas, uint16_t w, uint16_t h) {
      return std::bit_cast<LoFi::PfxContext*>(canvas)->PushCanvasSize({w, h});
}

void Gfx2DCmdPopCanvasSize(Gfx2DCanvas canvas) {
      return std::bit_cast<LoFi::PfxContext*>(canvas)->PopCanvasSize();
}

void Gfx2DCmdPushScissor(Gfx2DCanvas canvas, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
      return std::bit_cast<LoFi::PfxContext*>(canvas)->PushScissor({x, y, w, h});
}

void Gfx2DCmdPopScissor(Gfx2DCanvas canvas) {
      return std::bit_cast<LoFi::PfxContext*>(canvas)->PopScissor();
}

void Gfx2DReset(Gfx2DCanvas canvas) {
      return std::bit_cast<LoFi::PfxContext*>(canvas)->Reset();
}

void Gfx2DEmitDrawCommand(Gfx2DCanvas canvas, GfxRDGNodeCore nodec) {
      return std::bit_cast<LoFi::PfxContext*>(canvas)->EmitDrawCommand(std::bit_cast<LoFi::RenderNode*>(nodec));
}

void Gfx2DDispatchGenerateCommands(Gfx2DCanvas canvas) {
      return std::bit_cast<LoFi::PfxContext*>(canvas)->DispatchGenerateCommands();
}

void Gfx2DCmdPushStrock(Gfx2DCanvas canvas, const Gfx2DParamStrockType& strock_parameter) {
      return std::bit_cast<LoFi::PfxContext*>(canvas)->PushStrock(std::bit_cast<LoFi::StrockTypeParameter>(strock_parameter));
}

void Gfx2DCmdPopStrock(Gfx2DCanvas canvas) {
      return std::bit_cast<LoFi::PfxContext*>(canvas)->PopStrock();
}

void Gfx2DCmdDrawBox(Gfx2DCanvas canvas, GfxVec2 start, Gfx2DParamBox param, float rotation, GfxColor color) {
      return std::bit_cast<LoFi::PfxContext*>(canvas)->DrawBox(std::bit_cast<glm::vec2>(start), std::bit_cast<LoFi::PParamBox>(param), rotation,
            std::bit_cast<glm::u8vec4>(color));
}

void Gfx2DCmdDrawRoundBox(Gfx2DCanvas canvas, GfxVec2 start, Gfx2DParamRoundBox param, float rotation, GfxColor color) {
      return std::bit_cast<LoFi::PfxContext*>(canvas)->DrawRoundBox(std::bit_cast<glm::vec2>(start),
            std::bit_cast<LoFi::PParamRoundBox>(param), rotation, std::bit_cast<glm::u8vec4>(color));
}

void Gfx2DCmdDrawNGon(Gfx2DCanvas canvas, GfxVec2 center, Gfx2DParamRoundNGon param, float rotation, GfxColor color) {
      return std::bit_cast<LoFi::PfxContext*>(canvas)->DrawNGon(std::bit_cast<glm::vec2>(center), std::bit_cast<LoFi::PParamRoundNGon>(param), rotation,
            std::bit_cast<glm::u8vec4>(color));
}

void Gfx2DCmdDrawCircle(Gfx2DCanvas canvas, GfxVec2 center, Gfx2DParamCircle param, float rotation, GfxColor color) {
      return std::bit_cast<LoFi::PfxContext*>(canvas)->DrawCircle(std::bit_cast<glm::vec2>(center), std::bit_cast<LoFi::PParamCircle>(param), rotation,
            std::bit_cast<glm::u8vec4>(color));
}

void Gfx2DCmdDrawText(Gfx2DCanvas canvas, GfxVec2 start, const wchar_t* text, Gfx2DParamText param, GfxColor color) {
      return std::bit_cast<LoFi::PfxContext*>(canvas)->DrawText(std::bit_cast<glm::vec2>(start), text, std::bit_cast<LoFi::PParamText>(param),
            std::bit_cast<glm::u8vec4>(color));
}
