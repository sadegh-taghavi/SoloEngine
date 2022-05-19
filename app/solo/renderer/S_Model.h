#pragma once

#include "tiny_gltf.h"

#include "solo/stl/S_Vector.h"
#include "solo/stl/S_String.h"
#include "solo/math/S_Mat4x4.h"
#include "solo/math/S_Quat.h"
#include "solo/math/S_Vec4.h"
#include "solo/math/S_Vec3.h"
#include "solo/math/S_Vec2.h"

namespace solo
{

class S_Texture;

class S_Model{

public:
    S_Model(const S_String &model);
    virtual ~S_Model();
    S_String path() const;

private:
    tinygltf::FsCallbacks m_fsCallbacks;
    S_String m_path;

    struct Node;

    struct BoundingBox
    {
        S_Vec3 Min;
        S_Vec3 Max;
        bool Valid = false;
        BoundingBox();
        BoundingBox(S_Vec3 Min, S_Vec3 Max);
        BoundingBox aABB(S_Mat4x4 m);
    };

    enum class AlphaMode
    {
        Opaque,
        Mask,
        Blend
    };

    struct Material
    {
        AlphaMode alphaMode = AlphaMode::Opaque;
        float AlphaCutoff = 1.0f;
        float MetallicFactor = 1.0f;
        float RoughnessFactor = 1.0f;
        S_Vec4 BaseColorFactor = S_Vec4(1.0f, 1.0f, 1.0f, 1.0f);
        S_Vec4 EmissiveFactor = S_Vec4(1.0f, 1.0f, 1.0f, 1.0f);
        S_Texture *BaseColorTexture;
        S_Texture *MetallicRoughnessTexture;
        S_Texture *NormalTexture;
        S_Texture *OcclusionTexture;
        S_Texture *EmissiveTexture;

        struct TexCoordSets
        {
            uint8_t BaseColor = 0;
            uint8_t MetallicRoughness = 0;
            uint8_t SpecularGlossiness = 0;
            uint8_t Normal = 0;
            uint8_t Occlusion = 0;
            uint8_t Emissive = 0;
        } TexCoordSets;

        struct Extension
        {
            S_Texture *SpecularGlossinessTexture;
            S_Texture *DiffuseTexture;
            S_Vec4 DiffuseFactor = S_Vec4(1.0f, 1.0f, 1.0f, 1.0f);
            S_Vec3 SpecularFactor = S_Vec3(0.0f, 0.0f, 0.0f);
        } Extension;

        struct PbrWorkflows
        {
            bool MetallicRoughness = true;
            bool SpecularGlossiness = false;
        } PbrWorkflows;

    };

    struct Primitive
    {
        uint32_t FirstIndex;
        uint32_t IndexCount;
        uint32_t VertexCount;
        Material MaterialData;
        bool HasIndices;
        struct BoundingBox BoundingBox;
        Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount, struct Material& material);
        void setBoundingBox(S_Vec3 min, S_Vec3 max);
    };

    struct Mesh
    {
        S_Vector<Primitive*> Primitives;
        struct BoundingBox BoundingBox;
        struct BoundingBox AABoundingBox;
        struct _UniformBlock
        {
            static const uint8_t m_MAX_NUM_JOINTS = 128;
            S_Mat4x4 Matrix;
            S_Mat4x4 JointMatrix[m_MAX_NUM_JOINTS]{};
            float Jointcount{ 0 };
        } UniformBlock;
        Mesh(S_Mat4x4 matrix);
        ~Mesh();
        void setBoundingBox(S_Vec3 min, S_Vec3 max);
    };

    struct Skin
    {
        S_String Name;
        Node *SkeletonRoot = nullptr;
        S_Vector<S_Mat4x4> InverseBindMatrices;
        S_Vector<Node*> Joints;
    };

    struct Node
    {
        Node *Parent;
        uint32_t Index;
        S_Vector<Node*> Children;
        S_Mat4x4 Matrix;
        S_String Name;
        Mesh *MeshData;
        struct Skin *Skin;
        int32_t SkinIndex = -1;
        S_Vec3 Translation;
        S_Vec3 Scale{ 1.0f, 1.0f, 1.0f};
        S_Quat Rotation;
        BoundingBox BoundingBoxvh;
        BoundingBox AABoundingBox;
        S_Mat4x4 localMatrix();
        S_Mat4x4 matrix();
        void update();
        ~Node();
    };

    enum class PathType
    {
        TRANSLATION,
        ROTATION,
        SCALE
    };

    struct AnimationChannel
    {
        PathType Path;
        Node *NodeData;
        uint32_t SamplerIndex;
    };

    enum InterpolationType
    {
        LINEAR,
        STEP,
        CUBICSPLINE
    };

    struct AnimationSampler
    {

        InterpolationType Interpolation;
        S_Vector<float> Inputs;
        S_Vector<S_Vec4> OutputsVec4;
    };

    struct Animation
    {
        S_String Name;
        S_Vector<AnimationSampler> Samplers;
        S_Vector<AnimationChannel> Channels;
        float Start = std::numeric_limits<float>::max();
        float End = std::numeric_limits<float>::min();
    };


    struct Vertex
    {
        S_Vec3 Pos;
        S_Vec3 Normal;
        S_Vec2 UV0;
        S_Vec2 UV1;
        S_Vec4 Joint0;
        S_Vec4 Weight0;
    };

    int m_indices;
    S_Mat4x4 m_aABoundingBox;
    S_Vector<Node*> m_nodes;
    S_Vector<Node*> m_linearNodes;
    S_Vector<Skin*> m_skins;
    S_Vector<S_Texture *> m_textures;
    //                    S_Vector<S_Texture> m_textureSamplers;
    S_Vector<Material> m_materials;
    S_Vector<Animation> m_animations;
    S_Vector<S_String> m_extensions;

    struct Dimensions
    {
        S_Vec3 Min = S_Vec3( FLT_MAX, FLT_MAX, FLT_MAX );
        S_Vec3 Max = S_Vec3( -FLT_MAX, -FLT_MAX, -FLT_MAX );
    } m_dimensions;

    //        void destroy(VkDevice device);
    void loadNode(Node* parent, const tinygltf::Node& node, uint32_t nodeIndex, const tinygltf::Model& model, S_Vector<uint32_t>& indexBuffer, S_Vector<Vertex>& vertexBuffer, float globalscale);
    void loadSkins(tinygltf::Model& gltfModel);
    void loadTextures(tinygltf::Model& gltfModel);
    //        VkSamplerAddressMode getVkWrapMode(int32_t wrapMode);
    //        VkFilter getVkFilterMode(int32_t filterMode);
    void loadTextureSamplers(tinygltf::Model& gltfModel);
    void loadMaterials(tinygltf::Model& gltfModel);
    void loadAnimations(tinygltf::Model& gltfModel);
    void loadFrom(S_String filename, float scale = 1.0f);
    //        void drawNode(Node* node, VkCommandBuffer commandBuffer);
    //        void draw(VkCommandBuffer commandBuffer);
    void calculateBoundingBox(Node* node, Node* parent);
    void getSceneDimensions();
    void updateAnimation(uint32_t index, float time);
    Node* findNode(Node* parent, uint32_t index);
    Node* nodeFromIndex(uint32_t index);

};

}

