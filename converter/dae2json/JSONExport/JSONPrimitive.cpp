// Copyright (c) 2012, Motorola Mobility, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of the Motorola Mobility, Inc. nor the names of its
//    contributors may be used to endorse or promote products derived from this
//    software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "JSONExport.h"

using namespace rapidjson;

namespace JSONExport 
{    
    //---- JSONPrimitiveRemapInfos ------------------------------------------------------------------------------------------

    JSONPrimitive::JSONPrimitive()
    {
    }
    
    JSONPrimitive::~JSONPrimitive()
    {
    }
    
    JSONPrimitive::JSONPrimitive(const JSONPrimitive &primitive)
    {
        this->_allVertexAttributes = primitive._allVertexAttributes;
        this->_type = primitive._type;
        this->_materialID = primitive._materialID;
        this->_materialObjectID = primitive._materialObjectID;
        this->_uniqueIndices = primitive._uniqueIndices;
    }

    JSONExport::Semantic JSONPrimitive::getSemanticAtIndex(unsigned int index)
    {
        return this->_allVertexAttributes[index]->getSemantic();
    }
    
    unsigned int JSONPrimitive::getIndexOfSetAtIndex(unsigned int index)
    {
        return this->_allVertexAttributes[index]->getIndexOfSet();
    }

    unsigned int JSONPrimitive::getIndicesInfosCount()
    {
        return this->_allVertexAttributes.size();
    }
    
    VertexAttributeVector JSONPrimitive::getVertexAttributes()
    {
        return this->_allVertexAttributes;
    }
    
    void JSONPrimitive::appendVertexAttribute(shared_ptr <JSONVertexAttribute> VertexAttribute)
    {
        this->_allVertexAttributes.push_back(VertexAttribute);
    }    
    
    shared_ptr <JSONExport::JSONIndices> JSONPrimitive::getUniqueIndices()
    {
        return this->_uniqueIndices;
    }
    
    std::string JSONPrimitive::getType()
    {
        return _type;
    }
    
    void JSONPrimitive::setType(std::string type)
    {
        this->_type = type;
    }
    
    std::string JSONPrimitive::getMaterialID()
    {
        return this->_materialID;
    }
    
    void JSONPrimitive::setMaterialID(std::string materialID)
    {
        this->_materialID = materialID;
    }
    
    unsigned int JSONPrimitive::getMaterialObjectID()
    {
        return this->_materialObjectID;
    }
    
    void JSONPrimitive::setMaterialObjectID(unsigned int materialID)
    {
        this->_materialObjectID = materialID;
    }

    shared_ptr <JSONExport::JSONIndices>  JSONPrimitive::getIndices()
    {
        return this->_uniqueIndices;
    }
    
    void JSONPrimitive::setIndices(shared_ptr <JSONExport::JSONIndices> indices)
    {
        this->_uniqueIndices = indices;
    }
    
    
};
