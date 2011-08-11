/** 
 * @file postDeferredV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
attribute vec3 position;

varying vec2 vary_fragcoord;
varying vec2 vary_tc;

uniform vec2 tc_scale;

uniform vec2 screen_res;

void main()
{
	//transform vertex
	vec4 pos = gl_ModelViewProjectionMatrix * vec4(position.xyz, 1.0);
	gl_Position = pos;	
	vary_tc = (pos.xy*0.5+0.5)*tc_scale;
	vary_fragcoord = (pos.xy*0.5+0.5)*screen_res;
}
