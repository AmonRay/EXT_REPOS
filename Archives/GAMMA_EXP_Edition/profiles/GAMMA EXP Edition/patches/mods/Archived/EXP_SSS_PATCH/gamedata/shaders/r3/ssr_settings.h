// Change this is you want stronger reflections
#define SSR_REFLECTION_INTENSITY 2.0f

// This controls how much surface should "gloss" to enable reflections
#define SSR_GLOSS_THRESHOLD 0.2f	// 0.2

// You can try to tweak these to get better performance or picture quality
#define SSR_NUM_STEPS 4	//	20
#define SSR_NUM_REFINES 0	//	3
#define SSR_STEP_SIZE 0.07f	//	0.01f

// Set this to zero if you want crystal clear water
#define SSR_WATER_INTENSITY_MUL 0.05f	//	0.5f

// Uncomment this if reflections on weapons etc bother you (will also disable surface reflections closer than ~1m)
#define SSR_MIN_DEPTH 1.0f

// Uncomment this if you want to limit the water shader to either refraction or reflection (better performance)
// #define SSR_WATER_SINGLE_RAY

// Uncomment this if you want to use faster Van Damme fresnel from ReShade (doesn't apply to water)
// #define SSR_USE_FAST_FRESNEL
// NOTE: You'll probably also need to reduce the SSR_MIN_REFLECTION since it relies on correct Fresnel result

// If the effect doesn't seem to be working uncomment one or more of these
// #define SSR_DEBUG_NO_REFLECTION float3(0, 0, 1)
// #define SSR_DEBUG_REFLECTION float3(0, 1, 0)
// #define SSR_DEBUG_MIN_DEPTH float3(P.z / SSR_MIN_DEPTH, P.z / SSR_MIN_DEPTH, P.z / SSR_MIN_DEPTH)
// #define SSR_DEBUG_NO_HIT float3(1, 0, 0)
// #define SSR_DEBUG_HIT float3(abs(trace.dir.xy), trace.dir.z)
// #define SSR_DEBUG_HIT calc_ss_lighting(trace)
// #define SSR_DEBUG_SKY float3(0, 1, 1)
// #define SSR_DEBUG_MIN_REFLECTION float3(1, 0, 0)

// For debugging the water surface, you shouldn't hopefully need these _ever_
// #define SSR_WDEBUG_MIN_REFLECTION float3(1, 0, 0)
// #define SSR_WDEBUG_MIN_REFRACTION float3(0, 1, 0)
// #define SSR_WDEBUG_REFRACTION_MAX_MURKYNESS float3(1, 0, 1)
// #define SSR_WDEBUG_DFOG float3(0, 0.5, 1)
// #define SSR_WDEBUG_REFLECTION_MISS env.rgb
// #define SSR_WDEBUG_REFRACTION_MISS float3(1, 0, 1)
// #define SSR_WDEBUG_SPECULAR spec
// #define SSR_WDEBUG_SHADOWED float(trace.blocked / SSR_PASS_BEHIND).xxx

// -- EDIT BELOW AT YOUR OWN RISK --
#define SSR_ACCEPT_RANGE 2.5f	//	2.5f
#define SSR_STEP_INC 1.6f		//	1.6f

// These are for Van Damme Fresnel from qUINT
#define SSR_FRESNEL_EXP 5.0f
#define SSR_FRESNEL_K 0.04f

// How many times should we try to pass behind/under an object before failing
#define SSR_PASS_BEHIND 3

// Anything below this shouldn't really be noticeable
#define SSR_MIN_REFLECTION 0.008f

// Assume we'll mostly apply this to water or similar objects
#define SSR_SURFACE_IOR 1.33f

// Edit this if you want unrealistic water
#define SSR_WATER_IOR 1.33f

// This controls how much the water gets blended near the shore/objects (to avoid sharp transition)
#define SSR_WATER_SHORE_FADE_MUL 5

// Disable effects if shore fade would be stronger than this, not really noticeable anyways
#define SSR_WATER_SHORE_FADE_LIMIT 0.33

// Don't refract or reflect if distance fog would overpower the effect
#define SSR_WATER_MAX_DFOG 0.33f

// Don't refract if most of the color comes from reflection
#define SSR_WATER_MIN_REFRACTION 0.25f

// Don't refract if can't see much of the bottom anyways
#define SSR_WATER_REFRACTION_MAX_MURKYNESS 0.95

// FIXME: Where does this weird tilt come from, baked into the maps?
#define SSR_WATER_NORMAL_OFFSET float3(0.1, 0.1, 0)

// This is used to control fading out water reflections hitting top of the screen
#define SSR_WATER_EDGE_FADE_MUL 10.f

// These control the specular hilights from the sun on the water
#define SSR_WATER_SPECULAR_FLATTEN 0.2f
#define SSR_WATER_SPECULAR_SHARPNESS 500.f
#define SSR_WATER_SPECULAR_INTENSITY 8.f