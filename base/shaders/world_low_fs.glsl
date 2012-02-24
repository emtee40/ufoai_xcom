/**
 * @file world_low_fs.glsl
 * @brief Low quality battlescape fragment shader.
 */

#if r_postprocess
	/*
	 * Indicates that gl_FragData is written to, not gl_FragColor.
	 * #extension needs to be placed before all non preprocessor code.
	 */
	#extension GL_ARB_draw_buffers : enable
#endif

uniform int BUMPMAP;
uniform float BUMP;

/** Diffuse texture.*/
uniform sampler2D SAMPLER_DIFFUSE;
/** Lightmap.*/
uniform sampler2D SAMPLER_LIGHTMAP;
/** Deluxemap.*/
uniform sampler2D SAMPLER_DELUXEMAP;
/** Normalmap.*/
uniform sampler2D SAMPLER_NORMALMAP;

const vec3 two = vec3(2.0);
const vec3 negHalf = vec3(-0.5);

#include "fog_fs.glsl"
#include "write_fragment_fs.glsl"

/**
 * @brief main
 */
void main(void) {
	vec4 finalColor = vec4(0.0);
	vec3 light = vec3(0.0);
	/* These two should be declared in this scope for developer tools to work */
	vec3 deluxemap = vec3(0.0);
	vec4 normalmap = vec4(0.0);

	/* lightmap contains pre-computed incoming light color */
	light = texture2D(SAMPLER_LIGHTMAP, gl_TexCoord[1].st).rgb;

#if r_bumpmap
	if (BUMPMAP > 0) {
		/* Sample deluxemap and normalmap.*/
		normalmap = texture2D(SAMPLER_NORMALMAP, gl_TexCoord[0].st);
		normalmap.rgb = normalize(two * (normalmap.rgb + negHalf));

		/* deluxemap contains pre-computed incoming light vectors in object tangent space */
		deluxemap = texture2D(SAMPLER_DELUXEMAP, gl_TexCoord[1].st).rgb;
		deluxemap = normalize(two * (deluxemap + negHalf));

		/* Bump mapping.*/
		light *= clamp(dot(deluxemap, vec3(normalmap.x * BUMP, normalmap.y * BUMP, normalmap.z)), 0.0, 1.0);
	}
#endif

	/* Sample the diffuse texture, honoring the parallax offset.*/
	vec4 diffuse = texture2D(SAMPLER_DIFFUSE, gl_TexCoord[0].st);

	finalColor.rgb = diffuse.rgb * light;
	finalColor.a = diffuse.a;

#if r_fog
	/* Add fog.*/
	finalColor = FogFragment(finalColor);
#endif

/* Developer tools */

#if r_normalmap
	vec3 n = normalize(2.0 * (texture2D(SAMPLER_NORMALMAP, gl_TexCoord[0].st).rgb - 0.5));
	finalColor.rgb = finalColor.rgb * 0.01 + (1.0 - dot(n, normalize(lightDirs[0].xyz))) * 0.5 * vec3(1.0);
	finalColor.a = 1.0;
#endif

#if r_lightmap
	finalColor.rgb = finalColor.rgb * 0.01 + lightmap;
	finalColor.a = 1.0;
#endif

#if r_deluxemap
	if (BUMPMAP > 0) {
		finalColor.rgb = finalColor.rgb * 0.01 + deluxemap;
		finalColor.a = 1.0;
	}
#endif

	writeFragment(finalColor);
}
