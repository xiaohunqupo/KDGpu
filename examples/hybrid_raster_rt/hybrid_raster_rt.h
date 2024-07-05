/*
  This file is part of KDGpu.

  SPDX-FileCopyrightText: 2024 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>

  SPDX-License-Identifier: MIT

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#pragma once

#include <KDGpuExample/simple_example_engine_layer.h>

#include <KDGpu/acceleration_structure.h>
#include <KDGpu/bind_group.h>
#include <KDGpu/buffer.h>
#include <KDGpu/graphics_pipeline.h>
#include <KDGpu/render_pass_command_recorder_options.h>
#include <KDGpu/raytracing_shader_binding_table.h>
#include <KDGpu/raytracing_pipeline.h>

#include <glm/glm.hpp>

using namespace KDGpuExample;

class HybridRasterRt : public SimpleExampleEngineLayer
{
public:
protected:
    void initializeScene() override;
    void cleanupScene() override;
    void updateScene() override;
    void render() override;
    void resize() override;

private:
    void initializeGlobal();
    void initializeParticles();
    void initializeGBuffer();
    void initializeAlpha();
    void initializeShadows();
    void initializeCompositing();
    void initializeMeshes();
    void initializeAccelerationStructures();

    struct Particles {
        Buffer particleDataBuffer;
        Buffer blasTransformBuffer;
        PipelineLayout computePipelineLayout;
        ComputePipeline computePipeline;
        BindGroup particleBindGroup;
    } m_particles;

    struct GBuffer {
        Texture posTexture;
        Texture normalTexture;
        Texture colorTexture;
        Texture depthTexture;
        Texture fragmentHeadsPointer;
        Texture shadowTexture;
        Texture reflectionTexture;

        TextureView posTextureView;
        TextureView normalTextureView;
        TextureView colorTextureView;
        TextureView depthTextureView;
        TextureView fragmentHeadsPointerView;
        TextureView shadowTextureView;
        TextureView reflectionTextureView;

        Buffer fragmentLinkedListBuffer;
        size_t fragmentLinkedListBufferByteSize;
        TextureLayout fragmentHeadsPointerLayout{ TextureLayout::Undefined };
        TextureLayout shadowTextureLayout{ TextureLayout::Undefined };

        BindGroupLayout opaqueNormalDepthBindGroupLayout;
        BindGroupLayout alphaBindGroupLayout;
        BindGroupLayout shadowBindGroupLayout;
        BindGroupLayout reflectionBindGroupLayout;

        BindGroup opaqueNormalDepthBindGroup;
        BindGroup alphaLinkedListBindGroup;
        BindGroup shadowBindGroup;
        BindGroup reflectionBindGroup;

        Sampler sampler;

        void initialize(Device *device);
        void resize(Device *device, Extent2D extent);
        void cleanup();

    } m_gbuffer;

    struct DepthFillPass {
        RenderPassCommandRecorderOptions renderPassOptions;
    } m_zfillPass;

    struct AlphaFillPass {
        RenderPassCommandRecorderOptions renderPassOptions;
    } m_alphaPass;

    struct OpaquePass {
        RenderPassCommandRecorderOptions renderPassOptions;
    } m_opaquePass;

    struct ShadowPass {
        RayTracingPassCommandRecorderOptions rtPassOptions;
        PipelineLayout pipelineLayout;
        RayTracingPipeline pipeline;
        RayTracingShaderBindingTable sbt;
    } m_shadowPass;

    struct ReflectionPass {
        RayTracingPassCommandRecorderOptions rtPassOptions;
        RayTracingPipeline m_pipeline;
        RayTracingShaderBindingTable sbt;
    } m_reflectionPass;

    struct Compositing {
        RenderPassCommandRecorderOptions renderPassOptions;
        PipelineLayout graphicsPipelineLayout;
        GraphicsPipeline graphicsPipeline;
    } m_compositing;

    struct PlaneMesh {
        PipelineLayout zFillGraphicsPipelineLayout;
        PipelineLayout opaqueFillGraphicsPipelineLayout;
        GraphicsPipeline zFillGraphicsPipeline;
        GraphicsPipeline opaqueFillGraphicsPipeline;
        Buffer vertexBuffer;
        size_t vertexCount;
    } m_planeMesh;

    struct SphereMesh {
        PipelineLayout zFillGraphicsPipelineLayout;
        PipelineLayout alphaFillGraphicsPipelineLayout;
        PipelineLayout opaqueFillGraphicsPipelineLayout;
        GraphicsPipeline zFillGraphicsPipeline;
        GraphicsPipeline alphaFillGraphicsPipeline;
        GraphicsPipeline opaqueFillGraphicsPipeline;
        Buffer vertexBuffer;
        size_t vertexCount;
    } m_sphereMesh;

    struct AccelerationStructures {
        AccelerationStructure opaqueSpheresBlas;
        AccelerationStructure alphaSpheresBlas;
        AccelerationStructure opaquePlaneBlas;
        AccelerationStructure tBlas;

        BuildAccelerationStructureOptions opaqueSpheresASBuildOptions;
        BuildAccelerationStructureOptions alphaSpheresASBuildOptions;
        BuildAccelerationStructureOptions opaquePlaneASBuildOptions;
        BuildAccelerationStructureOptions tlASBuildOptions;

        BindGroupLayout tsASBindGroupLayout;
        BindGroup tsASBindGroup;

        bool hasBuiltStaticBlas = false;
    } m_as;

    struct Global {
        Buffer cameraDataBuffer;
        BindGroupLayout cameraBindGroupLayout;
        BindGroup cameraBindGroup;

        Buffer lightsDataBuffer;
        BindGroupLayout lightsBindGroupLayout;
        BindGroup lightBindGroup;

        CommandBuffer commandBuffer;
    } m_global;
};
