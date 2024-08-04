#pragma once
#include "GfxContext.h"

namespace LoFi {
      struct FontUV {
            float x, y, w, h;
      };

      struct GenDrawVertexData {
            glm::vec2 Pos;
            glm::vec2 UV;
      };

      enum class StrockFillType : uint16_t {
            Soild = 0,
            Texture = 1,

            Linear2 = 10,
            Linear3 = 11,
            Linear4 = 12,
            Linear5 = 13,
            Linear6 = 14,
            Linear7 = 15,


            Radial2 = 20,
            Radial3 = 21,
            Radial4 = 22,
            Radial5 = 23,
            Radial6 = 24,
            Radial7 = 25,
      };

      struct GradientData {
            glm::vec2 Offset;
            float DirectionAngle;
            float Pos1;
            float Pos2;
            float Pos3;
            float Pos4;
            float Pos5;
            float Pos6;
            glm::u8vec4 Color1;
            glm::u8vec4 Color2;
            glm::u8vec4 Color3;
            glm::u8vec4 Color4;
            glm::u8vec4 Color5;
            glm::u8vec4 Color6;
      };

      enum class DrawPrimitveType : uint16_t {
            Box = 0,
            RoundBox = 1,
            RoundNGon = 2,
            Circle = 3,

            Text = 50,
      };

      struct PParamText {
            float Size;
            float Space;
            float PxRange;
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
            float padd[8];
      };

      union PrimitiveParameter {
            PParamText Text;
            PParamBox Box;
            PParamRoundBox RoundRect;
            PParamRoundNGon RoundNGon;
            PParamCircle Circle;
            PrimitiveParameterAglier Aglier;
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
                        ResourceHandle ImageHandle;
                  } Solid;

                  struct {
                        float DirectionAngle;
                        float Pos1;
                        float Pos2;
                        float Pos3;
                        float Pos4;
                        float Pos5;
                        float Pos6;
                        glm::u8vec4 Color1;
                        glm::u8vec4 Color2;
                        glm::u8vec4 Color3;
                        glm::u8vec4 Color4;
                        glm::u8vec4 Color5;
                        glm::u8vec4 Color6;
                  } Linear;

                  struct {
                        float Pos1;
                        float Pos2;
                        float Pos3;
                        float Pos4;
                        float Pos5;
                        float Pos6;
                        glm::u8vec4 Color1;
                        glm::u8vec4 Color2;
                        glm::u8vec4 Color3;
                        glm::u8vec4 Color4;
                        glm::u8vec4 Color5;
                        glm::u8vec4 Color6;
                  } Radial;

                  struct {
                        ResourceHandle ImageHandle;
                  } Texture;
            };
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

      //painter library

      class PfxFontLibrary {
      public:
            entt::dense_map<wchar_t, FontUV> _fontUVs{};

      };

      class PfxContext {
            static inline PfxContext* _instance;

      public:

            NO_COPY_MOVE_CONS(PfxContext);

            ~PfxContext();

            explicit PfxContext();

            bool GenAndLoadFont(const char* path);

            [[nodiscard]] ResourceHandle GetAtlas() const { return _fontAtlas; }

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

            void DrawText(glm::vec2 start, const wchar_t* text, PParamText param, glm::u8vec4 color); //CPU

            //特殊 规则

            //void DrawPoints(); // CS  CPoint


            void Reset();

            void EmitDrawCommand(RenderNode* node);

            void DispatchGenerateCommands() const;

            ResourceHandle _fontAtlas {};

            uint32_t _fontAtlasTextureBindlessIndex {};
      private:
            GenInstanceData& NewDrawInstanceData();

      private:
            GfxContext* _gfx {};

            VkExtent2D _fontAtlasSize {};

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

            ResourceHandle _bufferVertex[3] {};
            ResourceHandle _bufferIndex[3] {};
            ResourceHandle _bufferInstance[3] {};
            ResourceHandle _bufferIndirect[3] {};
            ResourceHandle _bufferGradient[3] {};

            entt::dense_set<ResourceHandle, Internal::HashResourceHandle, Internal::EqualResourceHandle> _sampledImageReference[3]{};

      private:
            ResourceHandle _programDraw {};
            ResourceHandle _kernelDraw {};

            ResourceHandle _renderNodeResource {};

      private:
            //tf::Executor _taskExecutor{};
            //tf::Taskflow _taskFlowDispatch{};

            const float _pixelExpand = 20;

            entt::dense_set<wchar_t> _fontDOT{};
      };
};
