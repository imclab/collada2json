#include "JSONExport.h"
#include <stdio.h>
#include <stdlib.h>
#include "geometryHelpers.h"

using namespace rapidjson;
using namespace std::tr1;
using namespace std;

namespace JSONExport
{
    unsigned int* createTrianglesFromPolylist(unsigned int *verticesCount /* array containing the count for each array of indices per face */,
                                              unsigned int *polylist /* array containing the indices of a face */,
                                              unsigned int count /* count of entries within the verticesCount array */,
                                              unsigned int *triangulatedIndicesCount /* number of indices in returned array */) {
        //destination buffer size
        unsigned int indicesCount = 0;
        for (unsigned int i = 0 ; i < count ; i++) {
            indicesCount += (verticesCount[i] - 2) * 3;
        }

        if (triangulatedIndicesCount) {
            *triangulatedIndicesCount = indicesCount;
        }

        unsigned int *triangleIndices = (unsigned int*)malloc(sizeof(unsigned int) * indicesCount);
        unsigned int offsetDestination = 0;
        unsigned int offsetSource = 0;
        
        for (unsigned int i = 0 ; i < count ; i++) {
            unsigned int trianglesCount = verticesCount[i] - 2;
            
            unsigned int firstIndex = polylist[0];
            offsetSource = 1;
            
            for (unsigned k = 0 ; k < trianglesCount ; k++) {
                triangleIndices[offsetDestination] = firstIndex;
                triangleIndices[offsetDestination + 1] = polylist[offsetSource];
                triangleIndices[offsetDestination + 2] = polylist[offsetSource + 1];
                //printf("%d %d %d\n",triangleIndices[offsetDestination], triangleIndices[offsetDestination +1], triangleIndices[offsetDestination +2]);
                offsetSource += 1;
                offsetDestination += 3;
            }
            offsetSource += 1;
            
            polylist += verticesCount[i];
        }
        
        
        return triangleIndices;
    }
    
    std::string _KeyWithSemanticAndSet(JSONExport::Semantic semantic, unsigned int indexSet)
    {
        std::string semanticIndexSetKey = "";
        semanticIndexSetKey += "semantic";
        semanticIndexSetKey += JSONUtils::toString(semantic);
        semanticIndexSetKey += ":indexSet";
        semanticIndexSetKey += JSONUtils::toString(indexSet);
        
        return semanticIndexSetKey;
    }
    
    //---- JSONPrimitiveRemapInfos -------------------------------------------------------------
    
    typedef unordered_map<unsigned int* ,unsigned int /* index of existing n-uplet of indices */, RemappedMeshIndexesHash, RemappedMeshIndexesEq> RemappedMeshIndexesHashmap;
    
    //FIXME: this could be just an intermediate anonymous(no id) JSONBuffer
    class JSONPrimitiveRemapInfos
    {
    public:
        JSONPrimitiveRemapInfos(unsigned int* generatedIndices, unsigned int generatedIndicesCount, unsigned int *originalCountAndIndexes);
        virtual ~JSONPrimitiveRemapInfos();
        
        unsigned int generatedIndicesCount();
        unsigned int* generatedIndices();
        unsigned int* originalCountAndIndexes();
        
    private:
        unsigned int _generatedIndicesCount;
        unsigned int* _generatedIndices;
        unsigned int* _originalCountAndIndexes;
    };
    
    //---- JSONPrimitiveRemapInfos -------------------------------------------------------------
    JSONPrimitiveRemapInfos::JSONPrimitiveRemapInfos(unsigned int* generatedIndices, unsigned int generatedIndicesCount, unsigned int *originalCountAndIndexes):
    _generatedIndicesCount(generatedIndicesCount),
    _generatedIndices(generatedIndices),
    _originalCountAndIndexes(originalCountAndIndexes)
    {
    }
    
    JSONPrimitiveRemapInfos::~JSONPrimitiveRemapInfos()
    {
        if (this->_generatedIndices)
            free(this->_generatedIndices);
        if (this->_originalCountAndIndexes)
            free(this->_originalCountAndIndexes);
    }
    
    unsigned int JSONPrimitiveRemapInfos::generatedIndicesCount()
    {
        return _generatedIndicesCount;
    }
    
    unsigned int* JSONPrimitiveRemapInfos::generatedIndices()
    {
        return _generatedIndices;
    }
    
    unsigned int* JSONPrimitiveRemapInfos::originalCountAndIndexes()
    {
        return _originalCountAndIndexes;
    }
    
    typedef struct {
        unsigned char* remappedBufferData;
        //size_t remappedAccessorByteOffset;
        size_t remappedAccessorByteStride;
        
        unsigned char* originalBufferData;
        //size_t originalAccessorByteOffset;
        size_t originalAccessorByteStride;
        
        size_t elementByteLength;
        
    } AccessorsBufferInfos;
    
    static AccessorsBufferInfos* createAccessorsBuffersInfos(AccessorVector allOriginalAccessors ,AccessorVector allRemappedAccessors, unsigned int*indicesInRemapping, unsigned int count)
    {
        AccessorsBufferInfos* allBufferInfos = (AccessorsBufferInfos*)malloc(count * sizeof(AccessorsBufferInfos));
        for (size_t accessorIndex = 0 ; accessorIndex < count; accessorIndex++) {
            AccessorsBufferInfos *bufferInfos = &allBufferInfos[accessorIndex];
            
            shared_ptr <JSONExport::JSONAccessor> remappedAccessor = allRemappedAccessors[indicesInRemapping[accessorIndex]];
            shared_ptr <JSONExport::JSONAccessor> originalAccessor = allOriginalAccessors[indicesInRemapping[accessorIndex]];
            
            if (originalAccessor->getElementByteLength() != remappedAccessor->getElementByteLength()) {
                // FIXME : report error
                free(allBufferInfos);
                return 0;
            }
            
            shared_ptr <JSONExport::JSONBuffer> remappedBuffer = remappedAccessor->getBuffer();
            shared_ptr <JSONExport::JSONBuffer> originalBuffer = originalAccessor->getBuffer();
            
            bufferInfos->remappedBufferData = (unsigned char*)static_pointer_cast <JSONDataBuffer> (remappedBuffer)->getData() + remappedAccessor->getByteOffset();
            bufferInfos->remappedAccessorByteStride = remappedAccessor->getByteStride();
            
            bufferInfos->originalBufferData = (unsigned char*)static_pointer_cast <JSONDataBuffer> (originalBuffer)->getData() + originalAccessor->getByteOffset();
            bufferInfos->originalAccessorByteStride = originalAccessor->getByteStride();
            
            bufferInfos->elementByteLength = remappedAccessor->getElementByteLength();
        }
        
        return allBufferInfos;
    }
    
    bool __RemapPrimitiveVertices(shared_ptr<JSONExport::JSONPrimitive> primitive,
                                  std::vector< shared_ptr<JSONExport::JSONIndices> > allIndices,
                                  AccessorVector allOriginalAccessors,
                                  AccessorVector allRemappedAccessors,
                                  unsigned int* indicesInRemapping,
                                  shared_ptr<JSONExport::JSONPrimitiveRemapInfos> primitiveRemapInfos)
    {
        size_t indicesSize = allIndices.size();
        if (allOriginalAccessors.size() < indicesSize) {
            //TODO: assert & inconsistency check
        }
        
        unsigned int vertexAttributesCount = (unsigned int)indicesSize;
        
        //get the primitive infos to know where we need to "go" for remap
        unsigned int count = primitiveRemapInfos->generatedIndicesCount();
        unsigned int* indices = primitiveRemapInfos->generatedIndices();
        
        AccessorsBufferInfos *allBufferInfos = createAccessorsBuffersInfos(allOriginalAccessors , allRemappedAccessors, indicesInRemapping, vertexAttributesCount);
        
        unsigned int* uniqueIndicesBuffer = (unsigned int*) (static_pointer_cast <JSONDataBuffer> (primitive->getIndices()->getBuffer())->getData());
        
        unsigned int *originalCountAndIndexes = primitiveRemapInfos->originalCountAndIndexes();
        
        for (unsigned int k = 0 ; k < count ; k++) {
            unsigned int idx = indices[k];
            unsigned int* remappedIndex = &originalCountAndIndexes[idx * (allOriginalAccessors.size() + 1)];
            
            for (size_t accessorIndex = 0 ; accessorIndex < vertexAttributesCount  ; accessorIndex++) {
                AccessorsBufferInfos *bufferInfos = &allBufferInfos[accessorIndex];
                void *ptrDst = bufferInfos->remappedBufferData + (uniqueIndicesBuffer[idx] * bufferInfos->remappedAccessorByteStride);
                void *ptrSrc = (unsigned char*)bufferInfos->originalBufferData + (remappedIndex[1 /* skip count */ + indicesInRemapping[accessorIndex]] * bufferInfos->originalAccessorByteStride);
                //FIXME: optimize / secure this a bit, too many indirections without testing for invalid pointers
                /* copy the vertex attributes at the right offset and right indice (using the generated uniqueIndexes table */
                memcpy(ptrDst, ptrSrc , bufferInfos->elementByteLength);
            }
        }
        
        if (allBufferInfos)
            free(allBufferInfos);
        
        return true;
    }
    
    
    
    shared_ptr<JSONExport::JSONPrimitiveRemapInfos> __BuildPrimitiveUniqueIndexes(shared_ptr<JSONExport::JSONPrimitive> primitive,
                                                                                  std::vector< shared_ptr<JSONExport::JSONIndices> > allIndices,
                                                                                  RemappedMeshIndexesHashmap& remappedMeshIndexesMap,
                                                                                  unsigned int* indicesInRemapping,
                                                                                  unsigned int startIndex,
                                                                                  unsigned int accessorsCount,
                                                                                  unsigned int &endIndex)
    {
        size_t allIndicesSize = allIndices.size();
        size_t vertexAttributeCount = allIndices[0]->getCount();
        
        unsigned int generatedIndicesCount = 0;
        unsigned int vertexAttributesCount = accessorsCount;
        size_t sizeOfRemappedIndex = (vertexAttributesCount + 1) * sizeof(unsigned int);
        
        unsigned int* originalCountAndIndexes = (unsigned int*)calloc( (vertexAttributeCount * sizeOfRemappedIndex), sizeof(unsigned char));
        //this is useful for debugging.
        
        unsigned int *uniqueIndexes = (unsigned int*)calloc( vertexAttributeCount * sizeof(unsigned int), 1);
        unsigned int *generatedIndices = (unsigned int*) calloc (vertexAttributeCount * sizeof(unsigned int) , 1); //owned by PrimitiveRemapInfos
        unsigned int currentIndex = startIndex;
        
        for (size_t k = 0 ; k < vertexAttributeCount ; k++) {
            unsigned int* remappedIndex = &originalCountAndIndexes[k * (vertexAttributesCount + 1)];
            
            remappedIndex[0] = vertexAttributesCount;
            for (unsigned int i = 0 ; i < allIndicesSize ; i++) {
                unsigned int idx = indicesInRemapping[i];
                remappedIndex[1 + idx] = ((unsigned int*)(static_pointer_cast <JSONDataBuffer> (allIndices[i]->getBuffer())->getData()))[k];
            }
            
            unsigned int index = remappedMeshIndexesMap[remappedIndex];
            if (index == 0) {
                index = currentIndex++;
                generatedIndices[generatedIndicesCount++] = (unsigned int)k;
                remappedMeshIndexesMap[remappedIndex] = index;
            }
            
            uniqueIndexes[k] = index - 1;
        }
        
        endIndex = currentIndex;
        shared_ptr <JSONExport::JSONPrimitiveRemapInfos> primitiveRemapInfos(new JSONExport::JSONPrimitiveRemapInfos(generatedIndices, generatedIndicesCount, originalCountAndIndexes));
        shared_ptr <JSONExport::JSONDataBuffer> indicesBuffer(new JSONExport::JSONDataBuffer(uniqueIndexes, vertexAttributeCount * sizeof(unsigned int), true));
        
        shared_ptr <JSONExport::JSONIndices> indices = shared_ptr <JSONExport::JSONIndices> (new JSONExport::JSONIndices(indicesBuffer, vertexAttributeCount, JSONExport::VERTEX, 0));
        
        primitive->setIndices(indices);
        
        return primitiveRemapInfos;
    }
    
    
    shared_ptr <JSONMesh> CreateUnifiedIndexesMeshFromMesh(JSONMesh *sourceMesh, std::vector< shared_ptr<IndicesVector> > &vectorOfIndicesVector)
    {
        AccessorVector originalAccessors;
        AccessorVector remappedAccessors;
        shared_ptr <JSONMesh> targetMesh(new JSONMesh(*sourceMesh));
        
        //targetMesh->setID(sourceMesh->getID());
        //targetMesh->setName(sourceMesh->getName());
        
        PrimitiveVector sourcePrimitives = sourceMesh->getPrimitives();
        PrimitiveVector targetPrimitives = targetMesh->getPrimitives();
        
        size_t startIndex = 1; // begin at 1 because the hashtable will return 0 when the element is not present
        unsigned endIndex = 0;
        size_t primitiveCount = sourcePrimitives.size();
        unsigned int maxVertexAttributes = 0;
        
        if (primitiveCount == 0) {
            // FIXME: report error
            //return 0;
        }
        
        //in originalAccessors we'll get the flattened list of all the accessors as a vector.
        //fill semanticAndSetToIndex with key: (semantic, indexSet) value: index in originalAccessors vector.
        vector <JSONExport::Semantic> allSemantics = sourceMesh->allSemantics();
        std::map<string, unsigned int> semanticAndSetToIndex;
        
        for (unsigned int i = 0 ; i < allSemantics.size() ; i++) {
            IndexSetToAccessorHashmap& indexSetToAccessor = sourceMesh->getAccessorsForSemantic(allSemantics[i]);
            IndexSetToAccessorHashmap::const_iterator accessorIterator;
            for (accessorIterator = indexSetToAccessor.begin() ; accessorIterator != indexSetToAccessor.end() ; accessorIterator++) {
                //(*it).first;             // the key value (of type Key)
                //(*it).second;            // the mapped value (of type T)
                shared_ptr <JSONExport::JSONAccessor> selectedAccessor = (*accessorIterator).second;
                unsigned int indexSet = (*accessorIterator).first;
                JSONExport::Semantic semantic = allSemantics[i];
                std::string semanticIndexSetKey = _KeyWithSemanticAndSet(semantic, indexSet);
                unsigned int size = (unsigned int)originalAccessors.size();
                semanticAndSetToIndex[semanticIndexSetKey] = size;
                
                originalAccessors.push_back(selectedAccessor);
            }
        }
        
        maxVertexAttributes = (unsigned int)originalAccessors.size();
        
        vector <shared_ptr<JSONExport::JSONPrimitiveRemapInfos> > allPrimitiveRemapInfos;
        
        //build a array that maps the accessors that the indices points to with the index of the indice.
        JSONExport::RemappedMeshIndexesHashmap remappedMeshIndexesMap;
        for (unsigned int i = 0 ; i < primitiveCount ; i++) {
            shared_ptr<IndicesVector>  allIndicesSharedPtr = vectorOfIndicesVector[i];
            IndicesVector *allIndices = allIndicesSharedPtr.get();
            unsigned int* indicesInRemapping = (unsigned int*)malloc(sizeof(unsigned int) * allIndices->size());
            
            for (unsigned int k = 0 ; k < allIndices->size() ; k++) {
                JSONExport::Semantic semantic = (*allIndices)[k]->getSemantic();
                unsigned int indexSet = (*allIndices)[k]->getIndexOfSet();
                std::string semanticIndexSetKey = _KeyWithSemanticAndSet(semantic, indexSet);
                unsigned int idx = semanticAndSetToIndex[semanticIndexSetKey];
                indicesInRemapping[k] = idx;
            }
            
            shared_ptr<JSONExport::JSONPrimitiveRemapInfos> primitiveRemapInfos = __BuildPrimitiveUniqueIndexes(targetPrimitives[i], *allIndices, remappedMeshIndexesMap, indicesInRemapping, startIndex, maxVertexAttributes, endIndex);
            
            free(indicesInRemapping);
            
            if (primitiveRemapInfos.get()) {
                startIndex = endIndex;
                allPrimitiveRemapInfos.push_back(primitiveRemapInfos);
            } else {
                // FIXME: report error
                //return NULL;
            }
        }
        
        // we are using WebGL for rendering, this involve OpenGL/ES where only float are supported.
        // now we got not only the uniqueIndexes but also the number of different indexes, i.e the number of vertex attributes count
        // we can allocate the buffer to hold vertex attributes
        unsigned int vertexCount = endIndex - 1;
        
        for (unsigned int i = 0 ; i < allSemantics.size() ; i++) {
            IndexSetToAccessorHashmap& indexSetToAccessor = sourceMesh->getAccessorsForSemantic(allSemantics[i]);
            IndexSetToAccessorHashmap& destinationIndexSetToAccessor = targetMesh->getAccessorsForSemantic(allSemantics[i]);
            IndexSetToAccessorHashmap::const_iterator accessorIterator;
            
            //FIXME: consider turn this search into a method for mesh
            for (accessorIterator = indexSetToAccessor.begin() ; accessorIterator != indexSetToAccessor.end() ; accessorIterator++) {
                //(*it).first;             // the key value (of type Key)
                //(*it).second;            // the mapped value (of type T)
                shared_ptr <JSONExport::JSONAccessor> selectedAccessor = (*accessorIterator).second;
                
                size_t sourceSize = vertexCount * selectedAccessor->getElementByteLength();
                void* sourceData = malloc(sourceSize);
                
                shared_ptr <JSONExport::JSONBuffer> referenceBuffer = selectedAccessor->getBuffer();
                shared_ptr <JSONExport::JSONDataBuffer> remappedBuffer(new JSONExport::JSONDataBuffer(referenceBuffer->getID(), sourceData, sourceSize, true));
                shared_ptr <JSONExport::JSONAccessor> remappedAccessor(new JSONExport::JSONAccessor(selectedAccessor.get()));
                remappedAccessor->setBuffer(remappedBuffer);
                remappedAccessor->setCount(vertexCount);
                
                destinationIndexSetToAccessor[(*accessorIterator).first] = remappedAccessor;
                
                remappedAccessors.push_back(remappedAccessor);
            }
        }
        
        /*
         if (_allOriginalAccessors.size() != allIndices.size()) {
         // FIXME: report error
         return false;
         }
         */
        for (unsigned int i = 0 ; i < primitiveCount ; i++) {
            shared_ptr<IndicesVector>  allIndicesSharedPtr = vectorOfIndicesVector[i];
            IndicesVector *allIndices = allIndicesSharedPtr.get();
            unsigned int* indicesInRemapping = (unsigned int*)calloc(sizeof(unsigned int) * (*allIndices).size(), 1);
            
            for (unsigned int k = 0 ; k < (*allIndices).size() ; k++) {
                JSONExport::Semantic semantic = (*allIndices)[k]->getSemantic();
                unsigned int indexSet = (*allIndices)[k]->getIndexOfSet();
                std::string semanticIndexSetKey = _KeyWithSemanticAndSet(semantic, indexSet);
                unsigned int idx = semanticAndSetToIndex[semanticIndexSetKey];
                indicesInRemapping[k] = idx;
            }
            
            bool status = __RemapPrimitiveVertices(targetPrimitives[i],
                                                   (*allIndices),
                                                   originalAccessors ,
                                                   remappedAccessors,
                                                   indicesInRemapping,
                                                   allPrimitiveRemapInfos[i]);
            free(indicesInRemapping);
            
            if (!status) {
                // FIXME: report error
                //return NULL;
            }
            
        }
        
        if (endIndex > 65535) {
            //The mesh should be split but we do not handle this feature yet
            printf("WARNING: unsupported (yet) feature - mesh has more than 65535 vertex, splitting has to be done for GL/ES \n");
            //delete targetMesh;
            //return 0; //for now return false as splitting is not implemented
        }
        
        return targetMesh;
    }
    

}