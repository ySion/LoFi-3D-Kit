#pragma once

#include "GfxContext.h"
#include "taskflow/taskflow.hpp"

namespace LoFi {
      struct FontUV {
            uint32_t x, y, w, h;
      };

      struct GenDrawVertexData {
            glm::vec2 Pos;
            glm::vec2 UV;
      };

      enum class StrockFillType : uint16_t {
            Soild = 0,
            Texture = 1,
            Linear1 = 2,
            Linear2,
            Linear3,
            Linear4,
            Linear5,
            Radial1,
            Radial2,
            Radial3,
            Radial4,
            Radial5,
      };

      struct GradientData {
            glm::vec2 Offset;
            uint16_t Direction;
            uint16_t Pos1;
            uint16_t Pos2;
            uint16_t Pos3;
            uint16_t Pos4;
            uint16_t Pos5;
            glm::u8vec4 Color1;
            glm::u8vec4 Color2;
            glm::u8vec4 Color3;
            glm::u8vec4 Color4;
            glm::u8vec4 Color5;
      };

      enum class DrawPrimitveType : uint16_t {
            Box = 0,
            RoundBox = 1,
            RoundNGon = 2,
            Circle = 3,
      };

      struct PParamBox {
            glm::vec2 Size;
      };

      struct PParamRoundBox {
            glm::vec2 Size;
            float RoundnessTopRight;
            float RoundnessBottomRight;
            float RoundnessTopLeft;
            float RoundnessBottomLeft;
      };

      struct PParamRoundNGon {
            float Radius;
            float SegmentCount;
            float Roundness;
      };

      struct PParamRoundPie {
            float Radius;
            float SegmentCount;
            float Roundness;
            float StartAngle;
            float EndAngle;
      };

      struct PParamCircle {
            float Radius;
      };


      struct PrimitiveParameterAglier {
            glm::vec4 A;
            glm::vec4 B;
      };

      union PrimitiveParameter {
            PParamBox Box;
            PParamRoundBox RoundRect;
            PParamRoundNGon RoundNGon;
            PParamCircle Circle;
            PrimitiveParameterAglier Aglier;
      };

      struct GenInstanceData {
            glm::u16vec4 Scissor;
            glm::u16vec2 CanvasSize;
            glm::u16vec2 Size;
            glm::vec2 Center;
            float CenterRotate;
            glm::u8vec4 Color;
            uint32_t TextureBindlessIndex_or_GradientDataOffset;
            StrockFillType StrockType;
            DrawPrimitveType PrimitiveType;
            PrimitiveParameter PrimitiveParameter;
      };

      struct ShadowParameter {
            uint16_t Distance;
            uint16_t Hardness;
            glm::u8vec4 color;
      };

      struct StrockTypeParameter {
            StrockFillType Type;

            union {
                  struct {
                        uint16_t GradientDirection;
                        glm::u8vec4 SoildColor;
                        entt::entity ImageHandle;
                  } Solid;

                  struct {
                        uint16_t GradientDirection;

                        uint16_t Pos1;
                        uint16_t Pos2;
                        uint16_t Pos3;
                        uint16_t Pos4;
                        uint16_t Pos5;
                        glm::u8vec4 Color1;
                        glm::u8vec4 Color2;
                        glm::u8vec4 Color3;
                        glm::u8vec4 Color4;
                        glm::u8vec4 Color5;
                  } Linear;

                  struct {
                        uint16_t Pos1;
                        uint16_t Pos2;
                        uint16_t Pos3;
                        uint16_t Pos4;
                        uint16_t Pos5;
                        glm::u8vec4 Color1;
                        glm::u8vec4 Color2;
                        glm::u8vec4 Color3;
                        glm::u8vec4 Color4;
                        glm::u8vec4 Color5;
                  } Radial;

                  struct {
                        entt::entity ImageHandle;
                  } Texture;
            };
      };


      //painter library

      class PfxContext {
            static inline PfxContext* _instance;

      public:
            static PfxContext* Get() { return _instance; }

            NO_COPY_MOVE_CONS(PfxContext);

            ~PfxContext();

            explicit PfxContext();

            bool LoadFont(const char* path);

            [[nodiscard]] entt::entity GetAtlas() const { return _fontAtlas; }

            void PushCanvasSize(glm::u16vec2 size);

            void PopCanvasSize();

            void PushScissor(glm::u16vec4 scissor);

            void PopScissor(); //new draw call

            void PushShadow(const ShadowParameter& shadow_parameter);

            void PopShadow();

            void PushStrock(const StrockTypeParameter& strock_parameter);

            void PopStrock();

            //不规则
            //规则

            void DrawBox(glm::vec2 start, PParamBox param, float rotation = 0, glm::u8vec4 color = {255, 255, 255, 255});

            void DrawRoundBox(glm::vec2 start, PParamRoundBox param, float rotation = 0, glm::u8vec4 color = {255, 255, 255, 255});

            void DrawNGon(glm::vec2 center, PParamRoundNGon param, float rotation = 0, glm::u8vec4 color = {255, 255, 255, 255});

            void DrawCircle(glm::vec2 center, PParamCircle param,float rotation = 0, glm::u8vec4 color = {255, 255, 255, 255});

            void DrawRoundRect();

            void DrawCircle();

            void DrawPie();

            //规则 Filled

            void FillRect(); // GPU

            void FillRoundRect(); // GPU

            void FillCircle(); // GPU

            void FillPolygon(); // GPU

            //特殊 规则

            //void DrawPoints(); // CS  CPoint

            //void DrawText(const wchar_t* text, float x, float y, float size, glm::vec4 color); //CPU

            void Reset();

            void EmitDrawCommand();

            void DispatchGenerateCommands();

      private:
            GenInstanceData& NewDrawInstanceData();

      private:
            GfxContext* _gfx;

            entt::entity _fontAtlas;

            VkExtent2D _fontAtlasSize;

            entt::dense_map<wchar_t, FontUV> _fontUVs{};

            glm::u16vec4 _currentScissor{};
            glm::u16vec2 _currentCanvasSize{};
            StrockTypeParameter _currentStrock{};

      private:
            std::vector<glm::u16vec4> _scissorStack{};
            std::vector<glm::u16vec2> _canvasSizeStack{};
            std::vector<StrockTypeParameter> _strockStack{};
            std::vector<ShadowParameter> _shadowStack{};


            std::vector<GenDrawVertexData> _vertexData{};
            std::vector<uint32_t> _indexData{};

            std::vector<GradientData> _gradientData{};

            std::vector<GenInstanceData> _instanceData{};
            std::vector<VkDrawIndexedIndirectCommand> _indirectData{};

            entt::entity _bufferVertex[3];
            entt::entity _bufferIndex[3];
            entt::entity _bufferInstance[3];
            entt::entity _bufferIndirect[3];
            entt::entity _bufferGradient[3];

            entt::dense_set<entt::entity> _sampledImageReference[3]{};

      private:
            entt::entity _programDraw;
            entt::entity _kernelDraw;

      private:
            tf::Executor _taskExecutor{};
            tf::Taskflow _taskFlowDispatch{};

            const float _pixelExpand = 20;
      };
};
