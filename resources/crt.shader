[VERTEX]
attribute vec3 a_vertex;
attribute vec3 a_normal;
attribute vec2 a_uv;

uniform mat4 projection_matrix;
uniform mat4 camera_matrix;
uniform mat4 object_matrix;
uniform vec3 object_scale;

varying vec2 v_uv;

void main()
{
    gl_Position = projection_matrix * camera_matrix * object_matrix * vec4(a_vertex.xyz * object_scale, 1.0);
    v_uv = a_uv.xy;
}

[FRAGMENT]

// Simple scanlines with curvature and mask effects lifted from crt-lottes
// by hunterk

////////////////////////////////////////////////////////////////////
////////////////////////////  SETTINGS  ////////////////////////////
/////  comment these lines to disable effects and gain speed  //////
////////////////////////////////////////////////////////////////////

#define MASK // fancy, expensive phosphor mask effect
#define CURVATURE // applies barrel distortion to the screen
#define SCANLINES  // applies horizontal scanline effect
//#define ROTATE_SCANLINES // for TATE games; also disables the mask effects, which look bad with it
#define EXTRA_MASKS // disable these if you need extra registers freed up

////////////////////////////////////////////////////////////////////
//////////////////////////  END SETTINGS  //////////////////////////
////////////////////////////////////////////////////////////////////

/*
layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float shadowMask;
	float SCANLINE_SINE_COMP_B;
	float warpX;
	float warpY;
	float maskDark;
	float maskLight;
	float monitor_gamma;
	float crt_gamma;
	float SCANLINE_SINE_COMP_A;
	float SCANLINE_BASE_BRIGHTNESS;
} params;

#pragma parameter shadowMask "shadowMask" 1.0 0.0 4.0 1.0
#pragma parameter SCANLINE_SINE_COMP_B "Scanline Intensity" 0.40 0.0 1.0 0.05
#pragma parameter warpX "warpX" 0.031 0.0 0.125 0.01
#pragma parameter warpY "warpY" 0.041 0.0 0.125 0.01
#pragma parameter maskDark "maskDark" 0.5 0.0 2.0 0.1
#pragma parameter maskLight "maskLight" 1.5 0.0 2.0 0.1
#pragma parameter crt_gamma "CRT Gamma" 2.5 1.0 4.0 0.05
#pragma parameter monitor_gamma "Monitor Gamma" 2.2 1.0 4.0 0.05
#pragma parameter SCANLINE_SINE_COMP_A "Scanline Sine Comp A" 0.0 0.0 0.10 0.01
#pragma parameter SCANLINE_BASE_BRIGHTNESS "Scanline Base Brightness" 0.95 0.0 1.0 0.01
*/
#define params_SCANLINE_SINE_COMP_B 0.40
#define params_shadowMask 1.0
#define params_warpX 0.031
#define params_warpY 0.041
#define params_maskDark 0.5
#define params_maskLight 1.5
#define params_crt_gamma 2.5
#define params_monitor_gamma 2.2
#define params_SCANLINE_SINE_COMP_A 0.0
#define params_SCANLINE_BASE_BRIGHTNESS 0.95

uniform vec2 output_size;
uniform sampler2D texture_map;
uniform vec4 color;

varying vec2 v_uv;

const vec4 params_SourceSize = vec4(256.0, 224.0, 1.0/256.0, 1.0/224.0);

vec4 scanline(vec2 coord, vec4 frame)
{
#if defined SCANLINES
	vec2 omega = vec2(3.1415 * output_size.x, 2.0 * 3.1415 * params_SourceSize.y);
	vec2 sine_comp = vec2(params_SCANLINE_SINE_COMP_A, params_SCANLINE_SINE_COMP_B);
	vec3 res = frame.xyz;
	#ifdef ROTATE_SCANLINES
		sine_comp = sine_comp.yx;
		omega = omega.yx;
	#endif
	vec3 scanline = res * (params_SCANLINE_BASE_BRIGHTNESS + dot(sine_comp * sin(coord * omega), vec2(1.0, 1.0)));

	return vec4(scanline.x, scanline.y, scanline.z, 1.0);
#else
	return frame;
#endif
}

#ifdef CURVATURE
// Distortion of scanlines, and end of screen alpha.
vec2 Warp(vec2 pos)
{
    pos  = pos*2.0-1.0;    
    pos *= vec2(1.0 + (pos.y*pos.y)*params_warpX, 1.0 + (pos.x*pos.x)*params_warpY);
    
    return pos*0.5 + 0.5;
}
#endif

#if defined MASK && !defined ROTATE_SCANLINES
	// Shadow mask.
	vec4 Mask(vec2 pos)
	{
		vec3 mask = vec3(params_maskDark, params_maskDark, params_maskDark);
	  
		// Very compressed TV style shadow mask.
		if (params_shadowMask == 1.0) 
		{
			float line = params_maskLight;
			float odd = 0.0;
			
			if (fract(pos.x*0.166666666) < 0.5) odd = 1.0;
			if (fract((pos.y + odd) * 0.5) < 0.5) line = params_maskDark;  
			
			pos.x = fract(pos.x*0.333333333);

			if      (pos.x < 0.333) mask.r = params_maskLight;
			else if (pos.x < 0.666) mask.g = params_maskLight;
			else                    mask.b = params_maskLight;
			mask*=line;  
		} 

		// Aperture-grille.
		else if (params_shadowMask == 2.0) 
		{
			pos.x = fract(pos.x*0.333333333);

			if      (pos.x < 0.333) mask.r = params_maskLight;
			else if (pos.x < 0.666) mask.g = params_maskLight;
			else                    mask.b = params_maskLight;
		} 
	#ifdef EXTRA_MASKS
		// These can cause moire with curvature and scanlines
		// so they're an easy target for freeing up registers
		
		// Stretched VGA style shadow mask (same as prior shaders).
		else if (params_shadowMask == 3.0) 
		{
			pos.x += pos.y*3.0;
			pos.x  = fract(pos.x*0.166666666);

			if      (pos.x < 0.333) mask.r = params_maskLight;
			else if (pos.x < 0.666) mask.g = params_maskLight;
			else                    mask.b = params_maskLight;
		}

		// VGA style shadow mask.
		else if (params_shadowMask == 4.0) 
		{
			pos.xy  = floor(pos.xy*vec2(1.0, 0.5));
			pos.x  += pos.y*3.0;
			pos.x   = fract(pos.x*0.166666666);

			if      (pos.x < 0.333) mask.r = params_maskLight;
			else if (pos.x < 0.666) mask.g = params_maskLight;
			else                    mask.b = params_maskLight;
		}
	#endif
		
		else mask = vec3(1.,1.,1.);

		return vec4(mask, 1.0);
	}
#endif

void main()
{
#ifdef CURVATURE
	vec2 pos = Warp(v_uv.xy);
#else
	vec2 pos = v_uv.xy;
#endif

#if defined MASK && !defined ROTATE_SCANLINES
	// mask effects look bad unless applied in linear gamma space
	vec4 in_gamma = vec4(params_crt_gamma, params_crt_gamma, params_crt_gamma, 1.0);
	vec4 out_gamma = vec4(1.0 / params_monitor_gamma, 1.0 / params_monitor_gamma, 1.0 / params_monitor_gamma, 1.0);
	vec4 res = pow(texture2D(texture_map, pos), in_gamma);
#else
	vec4 res = texture2D(texture_map, pos);
#endif

#if defined MASK && !defined ROTATE_SCANLINES
	// apply the mask; looks bad with vert scanlines so make them mutually exclusive
	res *= Mask(v_uv * output_size.xy * 1.0001);
#endif

#if defined MASK && !defined ROTATE_SCANLINES
	// re-apply the gamma curve for the mask path
    gl_FragColor = pow(scanline(pos-vec2(0.0,0.25*params_SourceSize.w), res), out_gamma);
#else
	gl_FragColor = scanline(pos, res);
#endif
    if (pos.x < 0.0 || pos.y < 0.0 || pos.x > 1.0 || pos.y > 1.0)
        gl_FragColor.rgb = vec3(0.0, 0.0, 0.0);
}