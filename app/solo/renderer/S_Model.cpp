
#include "solo/file/S_File.h"
#include "solo/stl/S_Vector.h"
#include "solo/debug/S_Debug.h"
#include "solo/math/S_Math.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_USE_RAPIDJSON
#define TINYGLTF_NO_FS

#include "S_Model.h"

using namespace solo;

S_Model::BoundingBox::BoundingBox() {
};

S_Model::BoundingBox::BoundingBox(S_Vec3 min, S_Vec3 max) : Min(min), Max(max)
{
};

S_Model::BoundingBox S_Model::BoundingBox::aABB(S_Mat4x4 m)
{
    S_Vec3 min = m[3];
    S_Vec3 max = min;
    S_Vec3 v0, v1;

    S_Vec3 right = m[0];
    v0 = right * Min.x();
    v1 = right * Max.x();
    min += solo::min(v0, v1);
    max += solo::max(v0, v1);

    S_Vec3 up = m[1];
    v0 = up * Min.y();
    v1 = up * Max.y();
    min += solo::min(v0, v1);
    max += solo::max(v0, v1);

    S_Vec3 back = m[2];
    v0 = back * this->Min.z();
    v1 = back * this->Max.z();
    min += solo::min(v0, v1);
    max += solo::max(v0, v1);

    return BoundingBox(min, max);
}

S_Model::Primitive::Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount, S_Model::Material &material) :
    FirstIndex(firstIndex), IndexCount(indexCount), VertexCount(vertexCount),MaterialData(material)
{
    HasIndices = indexCount > 0;
};

void S_Model::Primitive::setBoundingBox(S_Vec3 min, S_Vec3 max)
{
    BoundingBox.Min = min;
    BoundingBox.Max = max;
    BoundingBox.Valid = true;
}

// Mesh
S_Model::Mesh::Mesh(S_Mat4x4 matrix)
{
    UniformBlock.Matrix = matrix;
    //            VK_CHECK_RESULT(device->createBuffer(
    //                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    //                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    //                sizeof(uniformBlock),
    //                &uniformBuffer.buffer,
    //                &uniformBuffer.memory,
    //                &uniformBlock));
    //            VK_CHECK_RESULT(vkMapMemory(device->logicalDevice, uniformBuffer.memory, 0, sizeof(uniformBlock), 0, &uniformBuffer.mapped));
    //            uniformBuffer.descriptor = { uniformBuffer.buffer, 0, sizeof(uniformBlock) };
};

S_Model::Mesh::~Mesh()
{
    //            vkDestroyBuffer(device->logicalDevice, uniformBuffer.buffer, nullptr);
    //            vkFreeMemory(device->logicalDevice, uniformBuffer.memory, nullptr);
    for (Primitive* p : Primitives)
        delete p;
}

void S_Model::Mesh::setBoundingBox(S_Vec3 min, S_Vec3 max)
{
    BoundingBox.Min = min;
    BoundingBox.Max = max;
    BoundingBox.Valid = true;
}

// Node
S_Mat4x4 S_Model::Node::localMatrix()
{
    S_Mat4x4 matt;
    return matt.spr( Translation, Rotation, Scale ) * Matrix;
    //            return glm::translate(S_Mat4x4(1.0f), ) * S_Mat4x4() * glm::scale(S_Mat4x4(1.0f), ) * Matrix;
}

S_Mat4x4 S_Model::Node::matrix()
{
    S_Mat4x4 m = localMatrix();
    Node *p = Parent;
    while (p)
    {
        m = p->localMatrix() * m;
        p = p->Parent;
    }
    return m;
}

void S_Model::Node::update()
{
    if (MeshData)
    {
        S_Mat4x4 m = matrix();
        if (Skin)
        {
            MeshData->UniformBlock.Matrix = m;
            // Update join matrices
            S_Mat4x4 inverseTransform;
            m.inverseOut( inverseTransform );
            size_t numJoints = solo::min( static_cast<unsigned int>(Skin->Joints.size()), static_cast<unsigned int>(Mesh::_UniformBlock::m_MAX_NUM_JOINTS) );
            for (size_t i = 0; i < numJoints; i++)
            {
                Node *jointNode = Skin->Joints[i];
                S_Mat4x4 jointMat = jointNode->matrix() * Skin->InverseBindMatrices[i];
                jointMat = inverseTransform * jointMat;
                MeshData->UniformBlock.JointMatrix[i] = jointMat;
            }
            MeshData->UniformBlock.Jointcount = (float)numJoints;
            //memcpy(Mesh->UniformBlock.Mapped, &Mesh->UniformBlock, sizeof(Mesh->UniformBlock));
        } /*else {
                    memcpy(Mesh->UniformBlock.Mapped, &m, sizeof(S_Mat4x4));
                }*/
    }

    for (auto& child : Children)
    {
        child->update();
    }
}

S_Model::Node::~Node()
{
    if (MeshData)
    {
        delete MeshData;
    }
    for (auto& child : Children)
    {
        delete child;
    }
}

// Model

//        void S_Model::Model::destroy(VkDevice device)
//        {
//            if (vertices.buffer != VK_NULL_HANDLE) {
//                vkDestroyBuffer(device, vertices.buffer, nullptr);
//                vkFreeMemory(device, vertices.memory, nullptr);
//            }
//            if (indices.buffer != VK_NULL_HANDLE) {
//                vkDestroyBuffer(device, indices.buffer, nullptr);
//                vkFreeMemory(device, indices.memory, nullptr);
//            }
//            for (auto texture : textures) {
//                texture.destroy();
//            }
//            textures.resize(0);
//            textureSamplers.resize(0);
//            for (auto node : nodes) {
//                delete node;
//            }
//            materials.resize(0);
//            animations.resize(0);
//            nodes.resize(0);
//            linearNodes.resize(0);
//            extensions.resize(0);
//            for (auto skin : skins) {
//                delete skin;
//            }
//            skins.resize(0);
//        };

void S_Model::loadNode(Node *parent, const tinygltf::Node &node, uint32_t nodeIndex, const tinygltf::Model &model, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer, float globalscale)
{
    /*Node *newNode = new Node{};
    newNode->Index = nodeIndex;
    newNode->Parent = parent;
    newNode->Name = node.name;
    newNode->SkinIndex = node.skin;
    newNode->Matrix.identity();

    // Generate local node matrix
    if (node.translation.size() == 3)
        newNode->Translation = S_Vec3(node.translation.data());

    if (node.rotation.size() == 4)
        newNode->Rotation = S_Quat(node.rotation.data());

    if (node.scale.size() == 3)
        newNode->Scale = S_Vec3(node.scale.data());

    if (node.matrix.size() == 16)
        newNode->Matrix = S_Mat4x4( node.matrix.data() );

    if (node.children.size() > 0)
    {
        for (size_t i = 0; i < node.children.size(); i++)
        {
            loadNode(newNode, model.nodes[static_cast<uint32_t>( node.children[i] )],
                    static_cast<uint32_t>(node.children[i]), model, indexBuffer, vertexBuffer, globalscale);
        }
    }

    // Node contains mesh data
    if (node.mesh > -1)
    {
        const tinygltf::Mesh mesh = model.meshes[static_cast<uint32_t>(node.mesh)];
        Mesh *newMesh = new Mesh(newNode->Matrix);
        for (size_t j = 0; j < mesh.primitives.size(); j++)
        {
            const tinygltf::Primitive &primitive = mesh.primitives[j];
            uint32_t indexStart = static_cast<uint32_t>(indexBuffer.size());
            uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());
            uint32_t indexCount = 0;
            uint32_t vertexCount = 0;
            glm::vec3 posMin{};
            glm::vec3 posMax{};
            bool hasSkin = false;
            bool hasIndices = primitive.indices > -1;
            // Vertices
            {
                const float *bufferPos = nullptr;
                const float *bufferNormals = nullptr;
                const float *bufferTexCoordSet0 = nullptr;
                const float *bufferTexCoordSet1 = nullptr;
                const uint16_t *bufferJoints = nullptr;
                const float *bufferWeights = nullptr;

                int posByteStride;
                int normByteStride;
                int uv0ByteStride;
                int uv1ByteStride;
                int jointByteStride;
                int weightByteStride;

                // Position attribute is required
                assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

                const tinygltf::Accessor &posAccessor = model.accessors[static_cast<uint32_t>(primitive.attributes.find("POSITION")->second)];
                const tinygltf::BufferView &posView = model.bufferViews[static_cast<uint32_t>(posAccessor.bufferView)];
                bufferPos = reinterpret_cast<const float *>(&(model.buffers[static_cast<uint32_t>(posView.buffer)].data[posAccessor.byteOffset + posView.byteOffset]));
                posMin = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
                posMax = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);
                vertexCount = static_cast<uint32_t>(posAccessor.count);
                posByteStride = posAccessor.ByteStride(posView) ? (posAccessor.ByteStride(posView) / sizeof(float)) : tinygltf::GetTypeSizeInBytes(TINYGLTF_TYPE_VEC3);

                if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
                {
                    const tinygltf::Accessor &normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
                    const tinygltf::BufferView &normView = model.bufferViews[normAccessor.bufferView];
                    bufferNormals = reinterpret_cast<const float *>(&(model.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
                    normByteStride = normAccessor.ByteStride(normView) ? (normAccessor.ByteStride(normView) / sizeof(float)) : tinygltf::GetTypeSizeInBytes(TINYGLTF_TYPE_VEC3);
                }

                if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
                {
                    const tinygltf::Accessor &uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
                    const tinygltf::BufferView &uvView = model.bufferViews[uvAccessor.bufferView];
                    bufferTexCoordSet0 = reinterpret_cast<const float *>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
                    uv0ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float)) : tinygltf::GetTypeSizeInBytes(TINYGLTF_TYPE_VEC2);
                }
                if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end())
                {
                    const tinygltf::Accessor &uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_1")->second];
                    const tinygltf::BufferView &uvView = model.bufferViews[uvAccessor.bufferView];
                    bufferTexCoordSet1 = reinterpret_cast<const float *>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
                    uv1ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float)) : tinygltf::GetTypeSizeInBytes(TINYGLTF_TYPE_VEC2);
                }

                // Skinning
                // Joints
                if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
                    const tinygltf::Accessor &jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
                    const tinygltf::BufferView &jointView = model.bufferViews[jointAccessor.bufferView];
                    bufferJoints = reinterpret_cast<const uint16_t *>(&(model.buffers[jointView.buffer].data[jointAccessor.byteOffset + jointView.byteOffset]));
                    jointByteStride = jointAccessor.ByteStride(jointView) ? (jointAccessor.ByteStride(jointView) / sizeof(bufferJoints[0])) : tinygltf::GetTypeSizeInBytes(TINYGLTF_TYPE_VEC4);
                }

                if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
                    const tinygltf::Accessor &weightAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
                    const tinygltf::BufferView &weightView = model.bufferViews[weightAccessor.bufferView];
                    bufferWeights = reinterpret_cast<const float *>(&(model.buffers[weightView.buffer].data[weightAccessor.byteOffset + weightView.byteOffset]));
                    weightByteStride = weightAccessor.ByteStride(weightView) ? (weightAccessor.ByteStride(weightView) / sizeof(float)) : tinygltf::GetTypeSizeInBytes(TINYGLTF_TYPE_VEC4);
                }

                hasSkin = (bufferJoints && bufferWeights);

                for (size_t v = 0; v < posAccessor.count; v++) {
                    Vertex vert{};
                    vert.pos = glm::vec4(glm::make_vec3(&bufferPos[v * posByteStride]), 1.0f);
                    vert.normal = glm::normalize(glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * normByteStride]) : glm::vec3(0.0f)));
                    vert.uv0 = bufferTexCoordSet0 ? glm::make_vec2(&bufferTexCoordSet0[v * uv0ByteStride]) : glm::vec3(0.0f);
                    vert.uv1 = bufferTexCoordSet1 ? glm::make_vec2(&bufferTexCoordSet1[v * uv1ByteStride]) : glm::vec3(0.0f);

                    vert.joint0 = hasSkin ? glm::vec4(glm::make_vec4(&bufferJoints[v * jointByteStride])) : glm::vec4(0.0f);
                    vert.weight0 = hasSkin ? glm::make_vec4(&bufferWeights[v * weightByteStride]) : glm::vec4(0.0f);
                    // Fix for all zero weights
                    if (glm::length(vert.weight0) == 0.0f) {
                        vert.weight0 = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
                    }
                    vertexBuffer.push_back(vert);
                }
            }
            // Indices
            if (hasIndices)
            {
                const tinygltf::Accessor &accessor = model.accessors[primitive.indices > -1 ? primitive.indices : 0];
                const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];

                indexCount = static_cast<uint32_t>(accessor.count);
                const void *dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

                switch (accessor.componentType) {
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                    const uint32_t *buf = static_cast<const uint32_t*>(dataPtr);
                    for (size_t index = 0; index < accessor.count; index++) {
                        indexBuffer.push_back(buf[index] + vertexStart);
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                    const uint16_t *buf = static_cast<const uint16_t*>(dataPtr);
                    for (size_t index = 0; index < accessor.count; index++) {
                        indexBuffer.push_back(buf[index] + vertexStart);
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                    const uint8_t *buf = static_cast<const uint8_t*>(dataPtr);
                    for (size_t index = 0; index < accessor.count; index++) {
                        indexBuffer.push_back(buf[index] + vertexStart);
                    }
                    break;
                }
                default:
                    std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
                    return;
                }
            }
            Primitive *newPrimitive = new Primitive(indexStart, indexCount, vertexCount, primitive.material > -1 ? materials[primitive.material] : materials.back());
            newPrimitive->setBoundingBox(posMin, posMax);
            newMesh->primitives.push_back(newPrimitive);
        }
        // Mesh BB from BBs of primitives
        for (auto p : newMesh->primitives) {
            if (p->bb.valid && !newMesh->bb.valid) {
                newMesh->bb = p->bb;
                newMesh->bb.valid = true;
            }
            newMesh->bb.min = glm::min(newMesh->bb.min, p->bb.min);
            newMesh->bb.max = glm::max(newMesh->bb.max, p->bb.max);
        }
        newNode->mesh = newMesh;
    }
    if (parent) {
        parent->children.push_back(newNode);
    } else {
        nodes.push_back(newNode);
    }
    linearNodes.push_back(newNode);*/
}

void S_Model::loadSkins(tinygltf::Model &gltfModel)
{
    /*for (tinygltf::Skin &source : gltfModel.skins) {
        Skin *newSkin = new Skin{};
        newSkin->name = source.name;

        // Find skeleton root node
        if (source.skeleton > -1) {
            newSkin->skeletonRoot = nodeFromIndex(source.skeleton);
        }

        // Find joint nodes
        for (int jointIndex : source.joints) {
            Node* node = nodeFromIndex(jointIndex);
            if (node) {
                newSkin->joints.push_back(nodeFromIndex(jointIndex));
            }
        }

        // Get inverse bind matrices from buffer
        if (source.inverseBindMatrices > -1) {
            const tinygltf::Accessor &accessor = gltfModel.accessors[source.inverseBindMatrices];
            const tinygltf::BufferView &bufferView = gltfModel.bufferViews[accessor.bufferView];
            const tinygltf::Buffer &buffer = gltfModel.buffers[bufferView.buffer];
            newSkin->inverseBindMatrices.resize(accessor.count);
            memcpy(newSkin->inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(glm::mat4));
        }

        skins.push_back(newSkin);
    }*/
}


S_Model::S_Model(const S_String &model)
{
    m_path = model;
    S_File file( model );
    if( !file.open() )
    {
        s_debugLayer("Model not found!", model );
        return;
    }

    S_Vector<std::byte> fileData(file.size());
    file.read( reinterpret_cast<char *>( fileData.data() ), fileData.size() );
    file.close();

    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF gltfContext;
    std::string error;
    std::string warning;
    m_fsCallbacks.FileExists = [](const std::string &abs_filename, void *)->bool
    {
        S_File file( abs_filename );
        return file.exist();
    };

    m_fsCallbacks.ExpandFilePath = [](const std::string &filepath, void *userData)->std::string
    {
        S_Model* model = reinterpret_cast<S_Model *>( userData );

        S_String outPath;
        auto lastSlashPos = model->path().find_last_of("/");
        if( lastSlashPos == S_String::npos )
            outPath = filepath;
        else
            outPath = model->path().substr( 0, lastSlashPos + 1 ) + filepath;
        return outPath;
    };

    m_fsCallbacks.ReadWholeFile = [](std::vector<unsigned char> *out, std::string *err, const std::string &filepath, void *)->bool
    {
        S_File file( filepath );
        if( !file.open() )
            return false;
        out->resize(file.size());
        file.read( reinterpret_cast<char *>( out->data() ), out->size() );
        file.close();

        return true;
    };

    m_fsCallbacks.WriteWholeFile = [](std::string *err, const std::string &, const std::vector<unsigned char> &, void *)->bool
    {
        return false;
    };

    m_fsCallbacks.user_data = this;

    gltfContext.SetFsCallbacks(m_fsCallbacks);

    gltfContext.LoadASCIIFromString( &gltfModel, &error, &warning, reinterpret_cast<const char *>( fileData.data() ),
                                     static_cast<unsigned int>(fileData.size()), "" );
    s_debug( gltfModel.nodes.size() );
}

S_Model::~S_Model()
{

}

S_String S_Model::path() const
{
    return m_path;
}
