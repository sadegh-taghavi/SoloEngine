#pragma once

#include "tiny_gltf.h"

#include "solo/stl/S_Vector.h"
#include "solo/stl/S_String.h"
#include "solo/math/S_Math.h"

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
        glm::vec3 Min;
        glm::vec3 Max;
        bool Valid = false;
        BoundingBox();
        BoundingBox(glm::vec3 Min, glm::vec3 Max);
        BoundingBox aABB(glm::mat4 m);
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
        glm::vec4 BaseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        glm::vec4 EmissiveFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
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
            glm::vec4 DiffuseFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            glm::vec3 SpecularFactor = glm::vec3(0.0f, 0.0f, 0.0f);
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
        void setBoundingBox(glm::vec3 min, glm::vec3 max);
    };

    struct Mesh
    {
        S_Vector<Primitive*> Primitives;
        struct BoundingBox BoundingBox;
        struct BoundingBox AABoundingBox;
        struct _UniformBlock
        {
            static const uint8_t m_MAX_NUM_JOINTS = 128;
            glm::mat4 Matrix;
            glm::mat4 JointMatrix[m_MAX_NUM_JOINTS]{};
            float Jointcount{ 0 };
        } UniformBlock;
        Mesh(glm::mat4 matrix);
        ~Mesh();
        void setBoundingBox(glm::vec3 min, glm::vec3 max);
    };

    struct Skin
    {
        S_String Name;
        Node *SkeletonRoot = nullptr;
        S_Vector<glm::mat4> InverseBindMatrices;
        S_Vector<Node*> Joints;
    };

    struct Node
    {
        Node *Parent;
        uint32_t Index;
        S_Vector<Node*> Children;
        glm::mat4 Matrix;
        S_String Name;
        Mesh *MeshData;
        struct Skin *Skin;
        int32_t SkinIndex = -1;
        glm::vec3 Translation;
        glm::vec3 Scale{ 1.0f, 1.0f, 1.0f};
        glm::quat Rotation;
        BoundingBox BoundingBoxvh;
        BoundingBox AABoundingBox;
        glm::mat4 localMatrix();
        glm::mat4 matrix();
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
        S_Vector<glm::vec4> OutputsVec4;
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
        glm::vec3 Pos;
        glm::vec3 Normal;
        glm::vec2 UV0;
        glm::vec2 UV1;
        glm::vec4 Joint0;
        glm::vec4 Weight0;
    };

    int m_indices;
    glm::mat4 m_aABoundingBox;
    S_Vector<Node*> m_nodes;
    S_Vector<Node*> m_linearNodes;
    S_Vector<Skin*> m_skins;
    S_Vector<S_Texture *> m_textures;
    S_Vector<Material> m_materials;
    S_Vector<Animation> m_animations;
    S_Vector<S_String> m_extensions;

    struct Dimensions
    {
        glm::vec3 Min = glm::vec3( FLT_MAX, FLT_MAX, FLT_MAX );
        glm::vec3 Max = glm::vec3( -FLT_MAX, -FLT_MAX, -FLT_MAX );
    } m_dimensions;

    void loadNode(Node* parent, const tinygltf::Node& node, uint32_t nodeIndex, const tinygltf::Model& model, S_Vector<uint32_t>& indexBuffer, S_Vector<Vertex>& vertexBuffer, float globalscale);
    void loadSkins(tinygltf::Model& gltfModel);
    void loadTextures(tinygltf::Model& gltfModel);
    void loadTextureSamplers(tinygltf::Model& gltfModel);
    void loadMaterials(tinygltf::Model& gltfModel);
    void loadAnimations(tinygltf::Model& gltfModel);
    void loadFrom(S_String filename, float scale = 1.0f);
    void calculateBoundingBox(Node* node, Node* parent);
    void getSceneDimensions();
    void updateAnimation(uint32_t index, float time);
    Node* findNode(Node* parent, uint32_t index);
    Node* nodeFromIndex(uint32_t index);

};

}
