
#ifndef _SHADER_DEFINITION_H
#define _SHADER_DEFINITION_H


//----------------------------------------------------------------------------------------------------------
// Reverse Z
//----------------------------------------------------------------------------------------------------------
#define USE_REVERSE_Z 1

#define Z_UPPER_BOUND 50000.0f
#define Z_LOWER_BOUND 1.0f
#define Z_UPPER_BOUND_NORM 1.0f
#define Z_LOWER_BOUND_NORM 0.0f

#if USE_REVERSE_Z

#define FAR_Z Z_LOWER_BOUND
#define NEAR_Z Z_UPPER_BOUND
#define FAR_Z_NORM Z_LOWER_BOUND_NORM
#define NEAR_Z_NORM Z_UPPER_BOUND_NORM

#else

#define FAR_Z Z_UPPER_BOUND
#define NEAR_Z Z_LOWER_BOUND
#define FAR_Z_NORM Z_UPPER_BOUND_NORM
#define NEAR_Z_NORM Z_LOWER_BOUND_NORM

#endif


//----------------------------------------------------------------------------------------------------------
// Depth readback
//----------------------------------------------------------------------------------------------------------
#define DEPTH_READBACK_BUFFER_SIZE_X 256
#define DEPTH_READBACK_BUFFER_SIZE_Y 128

#define DEPTH_READBACK_BUFFER_SIZE (DEPTH_READBACK_BUFFER_SIZE_X * DEPTH_READBACK_BUFFER_SIZE_Y)

#define DEPTH_DOWNSAMPLE_THREAD_NUM_X 8
#define DEPTH_DOWNSAMPLE_THREAD_NUM_Y 8


//----------------------------------------------------------------------------------------------------------
// TBDR
//----------------------------------------------------------------------------------------------------------
#define TILE_SIZE_X 16
#define TILE_SIZE_Y 16

#define TILE_THREAD_NUM_X 8
#define TILE_THREAD_NUM_Y 8

#define COMPUTE_SHADER_TILE_GROUP_SIZE (TILE_THREAD_NUM_X * TILE_THREAD_NUM_Y)


//----------------------------------------------------------------------------------------------------------
// CBDR
//----------------------------------------------------------------------------------------------------------
#define CLUSTER_SIZE_X 64
#define CLUSTER_SIZE_Y 64
#define CLUSTER_NUM_Z 16

#define CLUSTER_THREAD_NUM_X 8
#define CLUSTER_THREAD_NUM_Y 8

#define COMPUTE_SHADER_CLUSTER_GROUP_SIZE (CLUSTER_THREAD_NUM_X * CLUSTER_THREAD_NUM_Y)


static const float DepthSlicing_16[17] = {
	1.0f, 20.0f, 29.7f, 44.0f, 65.3f,
	96.9f, 143.7f, 213.2f, 316.2f, 469.1f,
	695.9f, 1032.4f, 1531.5f, 2272.0f, 3370.5f,
	5000.0f, 50000.0f
};


//----------------------------------------------------------------------------------------------------------
// Light
//----------------------------------------------------------------------------------------------------------
#define MAX_GRID_POINT_LIGHT_NUM 80
#define MAX_GRID_SPOTLIGHT_NUM 20


//----------------------------------------------------------------------------------------------------------
// Shadow
//----------------------------------------------------------------------------------------------------------
#define SHADOW_CASCADE_NUM 2

#define SHADOW_MAP_RESOLUTION 2048

#define SHADOW_MAP_PREFILTER_GROUP_SIZE 256

#define USE_SHADOW_MAP_PREFILTER 0

#define LIGHT_Z_UPPER_BOUND 50000.0f
#define LIGHT_Z_LOWER_BOUND 1.0f
#define LIGHT_Z_UPPER_BOUND_NORM 1.0f
#define LIGHT_Z_LOWER_BOUND_NORM 0.0f

#if USE_REVERSE_Z

#define LIGHT_FAR_Z LIGHT_Z_LOWER_BOUND
#define LIGHT_NEAR_Z LIGHT_Z_UPPER_BOUND
#define LIGHT_FAR_Z_NORM LIGHT_Z_LOWER_BOUND_NORM
#define LIGHT_NEAR_Z_NORM LIGHT_Z_UPPER_BOUND_NORM

#else

#define LIGHT_FAR_Z LIGHT_Z_UPPER_BOUND
#define LIGHT_NEAR_Z LIGHT_Z_LOWER_BOUND
#define LIGHT_FAR_Z_NORM LIGHT_Z_UPPER_BOUND_NORM
#define LIGHT_NEAR_Z_NORM LIGHT_Z_LOWER_BOUND_NORM

#endif

#define USE_PCSS_TEMPORAL 0

#define SDF_GRID_NUM 64

#define SDF_TILE_THREAD_NUM_X 32
#define SDF_TILE_THREAD_NUM_Y 32

#define MAX_GRID_SDF_NUM 50

#define SDF_SHADOW_DISTANCE 15000.0f

#define SDF_DISTANCE_RANGE_SCALE 2.0f

#define USE_FIXED_POINT_SDF_TEXTURE 1

#define SDF_OUT_OF_BOX_RANGE 100.0f


//----------------------------------------------------------------------------------------------------------
// Motion Blur.
//----------------------------------------------------------------------------------------------------------

#define MOTION_BLUR_SHUTTER_ANGLE 720
#define MOTION_BLUR_SAMPLE_COUNT 16
#define MOTION_BLUR_MAX_RADIUS_SCALE 20.0f

// Derivated parameters.
#define MOTION_BLUR_MAX_RADIUS (int)(MOTION_BLUR_MAX_RADIUS_SCALE * gRenderTargetSize.y / 100)
#define MOTION_BLUR_RCP_MAX_RADIUS  (1.0f / MOTION_BLUR_MAX_RADIUS)
#define MOTION_BLUR_VELOCITY_SCALE (MOTION_BLUR_SHUTTER_ANGLE / 360.0f)
#define MOTION_BLUR_TILE_SIZE (((MOTION_BLUR_MAX_RADIUS - 1) / 8 + 1) * 8)
#define MOTION_BLUR_TILE_MAX_OFFSET_XY ((MOTION_BLUR_TILE_SIZE / 8.0f - 1.0f) * -0.5f)


//----------------------------------------------------------------------------------------------------------
// Bloom.
//----------------------------------------------------------------------------------------------------------

#define BLOOM_USE_13_TAP_FILTER 1
#define BLOOM_USE_TENT_UPSAMPLE 1

#define BLOOM_THRESHOLD 0.25f
#define BLOOM_CLAMP 65000.0f
#define BLOOM_SOFT_KNEE 0.5f
#define BLOOM_DIFFUSION 7.0f
#define BLOOM_INTENSITY 0.5f



#endif 