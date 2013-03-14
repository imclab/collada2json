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

var global = window;
(function (root, factory) {
    if (typeof exports === 'object') {
        // Node. Does not work with strict CommonJS, but
        // only CommonJS-like enviroments that support module.exports,
        // like Node.
      
        module.exports = factory(global);
        module.exports.RuntimeTFLoader = module.exports;
    } else if (typeof define === 'function' && define.amd) {
        // AMD. Register as an anonymous module.
        define([], function () {
            return factory(root);
        });
    } else {
        // Browser globals
        factory(root);
    }
}(this, function (root) {
    var WebGLTFLoader,ResourceDescription,Technique,ProgramPass,Pass,ScenePass,
        GLSLProgram,Material,Mesh,Node,Primitive,Projection,Camera,Scene;
    if (typeof exports === 'object') {
        require("dependencies/gl-matrix");
        WebGLTFLoader = require("c2j/webgl-tf-loader").WebGLTFLoader;
        ResourceDescription = require("resource-description").ResourceDescription;
        Technique = require("technique").Technique;
        ProgramPass = require("pass").ProgramPass;
        Pass = require("pass").Pass;
        ScenePass = require("pass").ScenePass;
        GLSLProgram = require("glsl-program").GLSLProgram;
        Material = require("material").Material;
        Mesh = require("mesh").Mesh;
        Node = require("node").Node;
        Primitive = require("primitive").Primitive;
        Projection = require("projection").Projection;
        Camera = require("camera").Camera;
        Scene = require("scene").Scene;
    } else {        
        WebGLTFLoader = global.WebGLTFLoader;
        ResourceDescription = global.ResourceDescription;
        Technique = global.Technique;
        ProgramPass = global.ProgramPass;
        Pass = global.Pass;
        ScenePass = global.ScenePass;
        GLSLProgram = global.GLSLProgram;
        Material = global.Material;
        Mesh = global.Mesh;
        Node = global.Node;
        Primitive = global.Primitive;
        Projection = global.Projection;
        Camera = global.Camera;
        Scene = global.Scene;
    }

    var RuntimeTFLoader = Object.create(WebGLTFLoader, {

        _scenes: { writable:true, value: null },

        //----- implements WebGLTFLoader ----------------------------

        totalBufferSize: { value: 0, writable: true },

        handleBuffer: {
            value: function(entryID, description, userInfo) {
                var buffer = Object.create(ResourceDescription).init(entryID, description);
                buffer.id = entryID;
                this.storeEntry(entryID, buffer, description);
                this.totalBufferSize += description.byteLength;
                return true;
            }
        },

        handleBufferView: {
            value: function(entryID, description, userInfo) {

                var bufferView = Object.create(ResourceDescription).init(entryID, description);
                bufferView.id = entryID;

                var buffer = this.getEntry(bufferView.description.buffer);

                bufferView.buffer = buffer;
                this.storeEntry(entryID, bufferView, description);

                return true;
            }
        },

        handleShader: {
            value: function(entryID, description, userInfo) {
                var shader = Object.create(ResourceDescription).init(entryID, description);
                shader.id = entryID;
                shader.type = "shader"; //FIXME should I pass directly the description ? add a type property in resource-description ? (probably first solution...)
                this.storeEntry(entryID, shader, description);
                return true;
            }
        },

        handleImage: {
            value: function(entryID, description, userInfo) {
                var imagePath = description.path;
                var imageResource = Object.create(ResourceDescription).init(imagePath, { "path": imagePath });
                imageResource.type = "image";
                this.storeEntry(entryID, imageResource, description);
                return true;
            }
        },


        handleTechnique: {
            value: function(entryID, description, userInfo) {
                var technique = Object.create(Technique);

                technique.id = entryID;
                this.storeEntry(entryID, technique, description);

                var rootPassID = description.pass;
                technique.passName = rootPassID;    

                var passesDescriptions = description.passes;
                if (!passesDescriptions) {
                    console.log("ERROR: technique does not contain pass");
                    return false;
                }

                var passes = {};
                var allPassesNames = Object.keys(description.passes);
                allPassesNames.forEach( function(passName) {
                    var passDescription = passesDescriptions[passName];
                    if (passDescription.type === Pass.PROGRAM) {
                        var program = passDescription.program; 
                        if (program) {
                            var pass = Object.create(ProgramPass).init();
                            //it is necessary to add an id that is composited using the techniqueID for pass, 
                            //so that we can uniquely identify them when adding primitives per passes.
                            pass.id = entryID+"_"+rootPassID;
                            var vsShaderEntry = this.getEntry(program[GLSLProgram.VERTEX_SHADER]);
                            var fsShaderEntry = this.getEntry(program[GLSLProgram.FRAGMENT_SHADER]);

                            var progInfo = {};
                            progInfo[GLSLProgram.VERTEX_SHADER] = vsShaderEntry.entry;
                            progInfo[GLSLProgram.FRAGMENT_SHADER] = fsShaderEntry.entry;
                            progInfo["uniforms"] = program.uniforms;
                            progInfo["attributes"]= program.attributes;

                            pass.program = Object.create(ResourceDescription).init(entryID+"_"+rootPassID+"_program", progInfo);
                            pass.program.type = "program"; //add this here this program object is not defined in the JSON format, we need to set the type manually.

                            pass.states = passDescription.states;
                            passes[passName] = pass;
                        } else {
                            console.log("ERROR: A Pass with type=program must have a program property");
                            return false;
                        }
                    }
                    
                }, this);

                technique.passes = passes;

                return true;
            }
        },

        handleMaterial: {
            value: function(entryID, description, userInfo) {
                var material = Object.create(Material).init(entryID);
                this.storeEntry(entryID, material, description);

                //Simplification - Just take the selected technique
                material.name = description.name;
                var techniqueEntry = this.getEntry(description.technique);
                if (techniqueEntry) {
                    material.technique = techniqueEntry.entry;
                } else {
                    console.log("ERROR: invalid file, cannot find referenced technique:"+description.technique);
                    return false;
                }

                //then check if that technique contains overrides for paraemeteres
                var techniques = description.techniques;
                if (techniques) {
                    var technique = techniques[description.technique];
                    if (technique) {
                        var parameters = Object.keys(technique.parameters);
                        parameters.forEach( function(parameter) {
                            if (parameter === "diffuseTexture") {
                                var param = technique.parameters.diffuseTexture;
                                if (param) {
                                    var sampler2D = Object.create(ResourceDescription).init(entryID+sampler2D, param);
                                    sampler2D.type = "SAMPLER_2D"
                                    if (param.image) {
                                        technique.parameters.diffuseTexture = sampler2D;
                                        param.image = this.getEntry(param.image).entry;               
                                    }    
                                }
                            }
                            material.parameters[parameter] = technique.parameters[parameter];
                        }, this)

                    }
                }

                return true;
            }
        },

        handleLight: {
            value: function(entryID, description, userInfo) {
                //no lights yet.
                return true;
            }
        },

        handleMesh: {
            value: function(entryID, description, userInfo) {

                var mesh = Object.create(Mesh).init();
                mesh.id = entryID;
                mesh.name = description.name;

                this.storeEntry(entryID, mesh, description);

                var primitivesDescription = description[Mesh.PRIMITIVES];
                if (!primitivesDescription) {
                    //FIXME: not implemented in delegate
                    console.log("MISSING_PRIMITIVES for mesh:"+ entryID);
                    return false;
                }

                for (var i = 0 ; i < primitivesDescription.length ; i++) {
                    var primitiveDescription = primitivesDescription[i];

                    if (primitiveDescription.primitive === "TRIANGLES") {
                        var primitive = Object.create(Primitive).init();

                        //read material
                        var materialEntry = this.getEntry(primitiveDescription.material);
                        primitive.material = materialEntry.entry;

                        mesh.primitives.push(primitive);

                        var vertexAttributesDescription = primitiveDescription.vertexAttributes;

                        vertexAttributesDescription.forEach( function(vertexAttributeDescription) {
                            var accessorID = vertexAttributeDescription.accessor;
                            var accessorEntry = this.getEntry(accessorID);
                            if (!accessorEntry) {
                                //let's just use an anonymous object for the accessor
                                var accessor = description.accessors[accessorID];
                                accessor.type = "accessor";
                                accessor.id = accessorID;
                                this.storeEntry(accessorID, accessor, accessor);

                                var bufferEntry = this.getEntry(accessor.bufferView);
                                accessor.bufferView = bufferEntry.entry;
                                accessorEntry = this.getEntry(accessorID);
                                if (!accessor.byteOffset)
                                    accessor.byteOffset = 0;
                                //this is a set ID, it has to stay a string
                            }
                            var set = vertexAttributeDescription.set ? vertexAttributeDescription.set : "0";

                            primitive.addVertexAttribute( { "semantic" :  vertexAttributeDescription.semantic,
                                                            "set" : set,
                                                            "accessor" : accessorEntry.entry });

                        }, this);

                        //set indices
                        var indicesID = entryID + "_indices"+"_"+i;
                        var indicesEntry = this.getEntry(indicesID);
                        if (!indicesEntry) {
                            indices = primitiveDescription.indices;
                            indices.id = indicesID;
                            var bufferEntry = this.getEntry(indices.bufferView);
                            indices.bufferView = bufferEntry.entry;
                            this.storeEntry(indicesID, indices, indices);
                            indicesEntry = this.getEntry(indicesID);
                        }
                        primitive.indices = indicesEntry.entry;
                    }
                }
                return true;
            }
        },

        handleCamera: {
            value: function(entryID, description, userInfo) {
                //Do not handle camera for now.
                /*
                var camera = Object.create(Camera).init();
                camera.id = entryID;
                this.storeEntry(entryID, camera, description);

                var projection = Object.create(Projection);
                projection.initWithDescription(description);
                camera.projection = projection;
                */
                return true;
            }
        },

        handleLight: {
            value: function(entryID, description, userInfo) {
                return true;
            }
        },

        buildNodeHirerachy: {
            value: function(parentEntry) {
                var parentNode = parentEntry.entry;
                var children = parentEntry.description.children;
                if (children) {
                    children.forEach( function(childID) {
                        var nodeEntry = this.getEntry(childID);
                        parentNode.children.push(nodeEntry.entry);
                        this.buildNodeHirerachy(nodeEntry);
                    }, this);
                }
            }
        },

        handleScene: {
            value: function(entryID, description, userInfo) {

                if (!this._scenes) {
                    this._scenes = [];
                }

                if (!description.node) {
                    console.log("ERROR: invalid file required node property is missing from scene");
                    return false;
                }

                var scene = Object.create(Scene).init();
                scene.id = entryID;
                scene.name = description.name;
                this.storeEntry(entryID, scene, description);

                var nodeEntry = this.getEntry(description.node);

                scene.rootNode = nodeEntry.entry;
                this._scenes.push(scene);
                //now build the hirerarchy
                this.buildNodeHirerachy(nodeEntry);

                return true;
            }
        },

        handleNode: {
            value: function(entryID, description, userInfo) {
                var childIndex = 0;
                var self = this;

                var node = Object.create(Node).init();
                node.id = entryID;
                node.name = description.name;

                this.storeEntry(entryID, node, description);

                node.transform = description.matrix ? mat4.create(description.matrix) : mat4.identity();

                //FIXME: decision needs to be made between these 2 ways, probably meshes will be discarded.
                var meshEntry;
                if (description.mesh) {
                    meshEntry = this.getEntry(description.mesh);
                    node.meshes.push(meshEntry.entry);
                }

                if (description.meshes) {
                    description.meshes.forEach( function(meshID) {
                        meshEntry = this.getEntry(meshID);
                        if (meshEntry)
                            node.meshes.push(meshEntry.entry);
                    }, this);
                }

                if (description.camera) {
                    var cameraEntry = this.getEntry(description.camera);
                    if (cameraEntry)
                        node.cameras.push(cameraEntry.entry);
                }

                return true;
            }
        },

        handleLoadCompleted: {
            value: function(success) {
                if (this._scenes && this.delegate) {
                    if (this._scenes.length > 0) {
                        this.delegate.loadCompleted(this._scenes[0]);
                    }
                }
            }
        },

        handleError: {
            value: function(reason) {
                //TODO: propagate in the delegate
            }
        },

        //----- store model values

        _delegate: {
            value: null,
            writable: true
        },

        delegate: {
            enumerable: true,
            get: function() {
                return this._delegate;
            },
            set: function(value) {
                this._delegate = value;
            }
        },

        _entries: {
            enumerable: false,
            value: null,
            writable: true
        },

        removeAllEntries: {
            value: function() {
                this._entries = {};
            }
        },

        containsEntry: {
            enumerable: false,
            value: function(entryID) {
                if (!this._entries)
                    return false;
                return this._entries[entryID] ? true : false;
            }
        },

        storeEntry: {
            enumerable: false,
            value: function(id, entry, description) {
                if (!this._entries) {
                    this._entries = {};
                }

                if (!id) {
                    console.log("ERROR: not id provided, cannot store");
                    return;
                }

                if (this.containsEntry[id]) {
                    console.log("WARNING: entry:"+id+" is already stored, overriding");
                }

                this._entries[id] = { "id" : id, "entry" : entry, "description" : description };
            }
        },

        getEntry: {
            enumerable: false,
            value: function(entryID) {
                return this._entries ? this._entries[entryID] : null;
            }
        }


    });

    if(root) {
        root.RuntimeTFLoader = RuntimeTFLoader;
    }

    return RuntimeTFLoader;

}));
