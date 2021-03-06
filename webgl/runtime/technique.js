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
        module.exports.Technique = module.exports;
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
    var Base;
    if (typeof exports === 'object') {
        require("dependencies/gl-matrix");
        Base = require("base").Base;
    } else {
        Base = global.Base;
    }

    var Technique = Object.create(Base, {

        _parameters: { value: null, writable: true },

        _passName: { value: null, writable: true },

        _passes: { value: null, writable: true },

        init: {
            value: function() {
                this.__Base_init();
                this.passes = {};
                return this;
            }
        },

        parameters: {
            get: function() {
                return this._parameters;
            },
            set: function(value) {
                this._parameters = value;
            }
        },

        passName: {
            get: function() {
                return this._passName;
            },
            set: function(value) {
                if (this._passName != value) {
                    this._passName = value;
                }
            }
        },

        rootPass: {
            get: function() {
                return this._passes[this.passName];
            }
        },

        passesDidChange: {
            value: function() {
                //update the @pass when passes is changed. 
                //For convenience set to null if there are multiple passes or to the only pass contained when there is just a single one.
                var passesNames = Object.keys(this.passes);
                this.passName = (passesNames.length == 1) ? passesNames[0] : null;
            }
        },

        passes: {
            get: function() {
                return this._passes;
            },
            set: function(value) {
                if (this._passes != value) {
                    this._passes = value;
                    this.passesDidChange();
                }
            }
        },

        execute: {
            value: function(renderer) {
                renderer.resetStates();
                this.rootPass.execute(renderer);
            }
        }

    });

    if(root) {
        root.Technique = Technique;
    }

    return Technique;

}));