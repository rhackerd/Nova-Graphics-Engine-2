#pragma once

#include "core.h"
#include "system.h"
#include <Nova/Core/base.h>
#include <string>
#include "buffer.h"
#include "types.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>


namespace Nova::GE {
    // Too simple and high level for createInfo

    class Mesh {
        public:
            Mesh() = default;
            ~Mesh() = default;

        public:
            bool init(const std::string& path, VmaAllocator allocator);
            void shutdown();
        public:
            vk::Buffer  getVertexBuffer()   const { return m_vertexBuffer.getBuffer(); }
            vk::Buffer  getIndexBuffer()    const { return m_indexBuffer.getBuffer(); }
            u32         getIndexCount()     const { return m_indexCount; }
            bool        isValid()           const { return m_vertexBuffer.isValid(); }

        public:
            std::vector<GE::Vertex>&    getVerticies()  { return vertices; }
            std::vector<u32>&       getIndices()    { return indices; }

        private:
            void loadNode(aiNode* node, const aiScene* scene, std::vector<Vertex>& vertices, std::vector<u32>& indices);
            void loadMesh(aiMesh* mesh, const aiScene* scene, std::vector<Vertex>& vertices, std::vector<u32>& indices);


        private:
            Buffer  m_vertexBuffer;
            Buffer  m_indexBuffer;

            std::vector<GE::Vertex> vertices;
            std::vector<u32>    indices;

            u32     m_indexCount = 0;
    };
};