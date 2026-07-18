#pragma once

#include <SDL3/SDL_stdinc.h>

typedef Uint64 ecs_id_t;

extern ecs_id_t EcsAssets;
extern ecs_id_t EcsMetadata;
extern ecs_id_t EcsInit;
extern ecs_id_t EcsWindowConfig;
extern ecs_id_t EcsWindow;
extern ecs_id_t EcsGpuDevice;
extern ecs_id_t EcsGpuGraphicsPipeline;
extern ecs_id_t EcsDepthTexture;
extern ecs_id_t EcsGpuCommandBuffer;
extern ecs_id_t EcsGpuRenderPass;
extern ecs_id_t EcsSwapchainTexture;
extern ecs_id_t EcsSwapchainTextureSize;
extern ecs_id_t EcsCamera;
extern ecs_id_t EcsPhysicsConfig;
extern ecs_id_t EcsPhysicsEngine;
extern ecs_id_t EcsModel;
extern ecs_id_t EcsPhysicsBody;
extern ecs_id_t EcsRotation;
extern ecs_id_t EcsPosition;
extern ecs_id_t EcsScale;
extern ecs_id_t EcsProjection;
extern ecs_id_t EcsImGuiContext;
extern ecs_id_t EcsImGuiDrawData;
extern ecs_id_t EcsVertexShader;
extern ecs_id_t EcsFragmentShader;
extern ecs_id_t EcsClearColor;
extern ecs_id_t EcsViewProjection;
extern ecs_id_t EcsWorldTransform;
extern ecs_id_t EcsError;
extern ecs_id_t EcsScriptEngine;
extern ecs_id_t EcsArgs;
extern ecs_id_t EcsModelInstance;

// Input

extern ecs_id_t EcsInputState;
extern ecs_id_t EcsKeycode;
extern ecs_id_t EcsMouseButtonFlags;
