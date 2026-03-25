#include "common.h"
#include "hmodel.h"

// SSR adapted from ReShade SSR
#include "ssr_settings.h"
#include "ssr_es_check.h"

struct Trace
{
  float3 origin;
  float  step;
  float3 dir;
  bool   hit;
  float3 pos;
  int    blocked;

  float2 uv;
  float2 pos2d;
};

Trace trace_ss_ray(float3 pos, float3 dir)
{
  Trace trace;

  trace.hit = false;
  trace.blocked = 0;

  if (dir.z < 0) return trace; // don't trace backwards

  trace.origin = pos;
  trace.dir = dir;
  // TODO: add jitter
  trace.step = SSR_STEP_SIZE * sqrt(trace.origin.z) * rcp(saturate(1.1 - dot(trace.dir, normalize(pos))));;
  trace.step = clamp(trace.step, max(screen_res.z, screen_res.w) * 5, 1000.f); // step at least 5 pixels
  trace.pos = trace.origin + trace.dir * trace.step;

  int j = 0;
  int k = 0;
  bool uv_inside_screen;
  float4 gP;
  float3 rP;
  float3 error;

  const float2 p2uv_mul = rcp(pos_decompression_params.zw * screen_res.xy);
  [unroll(SSR_NUM_STEPS)] while (j++ < SSR_NUM_STEPS) {
    trace.uv = (trace.pos.xy / trace.pos.z + pos_decompression_params.xy) * p2uv_mul;
    trace.pos2d = trace.uv * screen_res.xy;
#ifndef USE_MSAA
    gP = s_position.Sample(smp_nofilter, trace.uv);
#else
    gP = s_position.Load(int3(trace.pos2d, 0), 0);
#endif
    rP = float3(gP.z * (trace.pos2d * pos_decompression_params.zw - pos_decompression_params.xy), gP.z);
    error = rP - trace.pos;

    bool blocked = error.z < 0;
    trace.blocked += blocked;
    if (blocked && (rP.z == 0 || (error.z > -SSR_ACCEPT_RANGE * trace.step && rP.z > trace.origin.z))) {
      if (k < SSR_NUM_REFINES) {
        trace.step /= SSR_STEP_INC;
        trace.pos -= trace.dir * trace.step;
        trace.step *= SSR_STEP_INC * rcp(SSR_NUM_REFINES);
        j = 0;
      } else {
        j = SSR_NUM_STEPS;
      }
      k++;
    }
    trace.pos += trace.dir * trace.step;
    trace.step *= SSR_STEP_INC;

    uv_inside_screen = all(saturate(-trace.uv.y * trace.uv.y + trace.uv.y));
    bool behind = saturate((trace.blocked > SSR_PASS_BEHIND) - k);
    j += SSR_NUM_STEPS * (!uv_inside_screen + behind);
  }

  trace.hit = k > 0;
  if (trace.hit && rP.z == 0) {
    // We hit the sky, extrapolate position to "infinity" to fix stepping edges
    // TODO: Check if the surface we extrapolated is still sky etc
    trace.pos = trace.origin + trace.dir * 1000.f;
    trace.uv = (trace.pos.xy / trace.pos.z + pos_decompression_params.xy) * p2uv_mul;
    trace.pos2d = trace.uv * screen_res.xy;
    trace.pos = float3(trace.uv, 0.f);
  }

  return trace;
}

float4 calc_fresnel_refract(float3 V, float3 N, float ior, bool need_refract = true)
{
  float cosi = dot(V, N);
  float etai = 1, etat = ior;
  float n = N;
  if (cosi < 0) {
    cosi = -cosi;
  } else {
    etai = ior;
    etat = 1;
    n = -N;
  }
  float eta = etai / etat;
  float icos_sq = 1 - cosi * cosi;
  float sint = eta * sqrt(max(0.f, icos_sq));
  if (sint >= 1) {
    return float4(0, 0, 0, 1); // Total internal reflection
  }
  float cost = sqrt(max(0.f, 1 - sint * sint));
  float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
  float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
  float amount = (Rs * Rs + Rp * Rp) / 2;
  float3 dir = float3(0, 0, 0);
  if (need_refract) {
    float k = 1 - eta * eta * icos_sq;
    dir = normalize(eta * V + (eta * cosi - sqrt(k)) * n);
  }
  return float4(dir, amount);
}

float calc_fresnel_quint(float3 V, float3 N)
{
  float raydir  = reflect(V, N);

  // Van Damme between physically correct and total artistic nonsense
  float schlick = lerp(SSR_FRESNEL_K, 1, pow(saturate(1 - dot(-V, N)), SSR_FRESNEL_EXP));
  float fade 	  = saturate(dot(V, raydir)) * saturate(1 - dot(-V, N));
  float amount  = saturate(schlick * fade);

  return amount;
}

#ifndef REFLECTIONS_H
TextureCube	s_env0;
TextureCube	s_env1;
#endif

float3 calc_sky(float3 dir)
{
  dir.y = dir.y * 2 - 0.5; // FIXME: This seems to produce somewhat correct results but still off a bit
  float3 sky0 = s_env0.SampleLevel(smp_base, dir, 0).xyz;
  float3 sky1 = s_env1.SampleLevel(smp_base, dir, 0).xyz;
  return lerp(sky0, sky1, L_ambient.w);
}

float3 calc_ss_lighting(Trace trace)
{
  gbuffer_data rgbd = gbuffer_load_data(trace.uv, trace.pos2d, 0);
  if (rgbd.mtl == 0) {
    return rgbd.C.rgb; // Sky color is directly in gbuffer albedo
  }

  float4 rD = float4(rgbd.C.rgb, rgbd.gloss);
#ifndef USE_MSAA
  float4 rL = s_accumulator.Sample(smp_nofilter, trace.uv);
#else
  float4 rL = s_accumulator.Load(int3(trace.uv * screen_res.xy, 0), 0);
#endif

  // TODO: add SSAO for reflection calc?
#ifdef SSR_ENHANCED_SHADERS // We have Enhanced Shaders installed
  if (abs(rgbd.mtl - MAT_FLORA) <= 0.05) {
    float	fAtten = 1 - smoothstep(0, 50, rgbd.P.z);
    rD.a	*= (fAtten * fAtten);
  }
  rL.rgb += rL.a * SRGBToLinear(rD.rgb);

  float3 hdiffuse, hspecular;
  hmodel(hdiffuse, hspecular, rgbd.mtl, rgbd.hemi, rD, rgbd.P, rgbd.N);

  float3 rcolor = rL.rgb + hdiffuse.rgb;
  rcolor = LinearTosRGB(rcolor);
#else // !SSR_ENHANCED_SHADERS
  float3 hdiffuse, hspecular;
  hmodel(hdiffuse, hspecular, rgbd.mtl, rgbd.hemi, rgbd.gloss, rgbd.P, rgbd.N);

  float4 light = float4(rL.rgb + hdiffuse, rL.w);
  float4 C = rD * light;
  float3 spec = C.www * rL.rgb + hspecular * C.rgba;
  float3 rcolor = C.rgb + spec;
#endif // SSR_ENHANCED_SHADERS

  return rcolor;
}

#ifndef MSAA_OPTIMIZATION
float calc_ssr(float2 uv, float2 pos2d, float3 P, float3 N, float gloss, inout float3 color)
#else
float calc_ssr(float2 uv, float2 pos2d, float3 P, float3 N, float gloss, inout float3 color, uint iSample : SV_SAMPLEINDEX )
#endif
{
  // TODO: smooth gloss between nearby positions to reduce sharp edges?
  if (gloss < SSR_GLOSS_THRESHOLD) {
#ifdef SSR_DEBUG_NO_REFLECTION
    color = SSR_DEBUG_NO_REFLECTION;
#endif
    return 0;
  }
#ifdef SSR_MIN_DEPTH
  if (P.z < SSR_MIN_DEPTH) {
#ifdef SSR_DEBUG_MIN_DEPTH
    color = SSR_DEBUG_MIN_DEPTH;
    return 0;
#endif
  }
#endif
#ifdef SSR_DEBUG_REFLECTION
  color = SSR_DEBUG_REFLECTION;
  return 0;
#endif

  float3 V = normalize(P);
#ifdef SSR_USE_FAST_FRESNEL
  float refl_amount = calc_fresnel_quint(V, N) * SSR_REFLECTION_INTENSITY;
#else
  float refl_amount = calc_fresnel_refract(V, N, SSR_SURFACE_IOR, false).w * SSR_REFLECTION_INTENSITY;
#endif
  if (refl_amount < SSR_MIN_REFLECTION) {
#ifdef SSR_DEBUG_MIN_REFLECTION
    color = SSR_DEBUG_MIN_REFLECTION;
#endif
    return 0;
  }

  Trace trace = trace_ss_ray(P, reflect(V, N));

  if (!trace.hit) {
#ifdef SSR_DEBUG_NO_HIT
    color = SSR_DEBUG_NO_HIT;
#endif
    return 0;
  }
#ifdef SSR_DEBUG_HIT
  color = SSR_DEBUG_HIT;
  return 0;
#endif

#ifdef SSR_DEBUG_SKY
  if (trace.pos.z == 0) {
    color = SSR_DEBUG_SKY;
    return 0;
  }
#endif

  float3 rcolor = calc_ss_lighting(trace);
  color = lerp(color, rcolor, refl_amount);
  return refl_amount;
}