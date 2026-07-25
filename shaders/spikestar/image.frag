
out vec4 C;

// vec4 bg = .9*vec4(52./256., 9./256., 38./256., 1.);
// vec4 fg = 1.1*vec4(1.,195./256.,31./256.,1.);

// vec4 fg = vec4(248./256.,73./256.,52./256.,1.);
// vec4 bg = .3*vec4(77./256., 94./256., 95./256., 1.);

float f(float x) {
    x /= 4.;
    return .3*texture(iFreqL, x).r;
}

#define SPIRAL

void main() {
	if (iFrame == 0) {
		C = vec4(0.5, 0.5, 0.5, 1.0);
		return;
	}
	const float PI = 3.141592;
	float threshold = .2;
	float time = iTime/100.;

    // Normalized pixel coordinates (from -0.5 to 0.5)
    // + correct aspect ratio
	vec2 p = gl_FragCoord.xy/iRes*2.-1.;
#ifdef SPIRAL
	p*=1.5;
	p-=vec2(.0,-.4);
#endif
	float aspect = iRes.x/iRes.y;
	p.y /= aspect;
	p *= 1. + fract(max(aspect, .7))/2.;

	float theta = atan(p.y,p.x);
	float len = length(p);

	// Make star fish

	// bass
	float bbump = (f(0.01)+f(0.02)+f(0.03))*.05;

	float bump = f(0.02)*.07;

	vec4 bg = vec4(0.0, (sin(bump * 2.0)+1.0)/2.0, (cos(bbump * 2.0)+1.0)/2.0, 1.0);

	// Set the fishies parameters
	float fish_number_legs = 18.;

	// Make the fish twirl around
	float fish_spin = -time*100.;

	// Make the fish get bigger
	float fish_leg_len = .1+.1*bump;

	// Make the fish move its legs
	float fish_leg_bend = 0.1*sin(40.*time+len)*(4.*PI);

	// Put the fish togeter
	float fish = fish_leg_len*sin(fish_leg_bend + fish_spin + fish_number_legs*theta);

	// Make the fish jump a little
	float fish_jump = .25*bump; // just a soft bump

#ifdef SPIRAL
	float spiral = (theta-PI/2.)/PI;
	float fish_dist = 1.-len*(.8+fish_jump+fish)+spiral;
	float fish_swim = -time*4.;
	float fish_school = fract(fish_dist*.5 + fish_swim);
	fish_school = pow(fish_school, 1.3);
	float v = f(fish_school);
	v = log(v+1.);
	v *= smoothstep(0.,0.08,len*(fish_school));
        // fish is a bit limp on linux so boost it up some
        v *= 50.;
#else
	// Pixel distance to fish
	float fish_dist = 1.-len*(.8+fish_jump+fish);

	// Fishes swim away
	float fish_swim = time/4.;

	// Fishes are packed fin to fin and gill to gill
	float fish_school = abs(fract(.5*fish_dist + fish_swim)-.5);
	fish_school += 0.05; // remove the always zero frequencies.
	fish_school = fract(fish_school*.8);

	// Eat the fish
	float v = f(fish_school);
	v = log(2.*v+1.);
	v *= smoothstep(0.,.07, len*(.8+fish));
        // fish is a bit limp on linux so boost it up some
        v *= 65.;
#endif

    // Position of the center
    vec2 center = vec2(0, 0);

    // Switch the "scene"
    float t = 5.0+bbump*100.0*iTime;
    float tm = mod(t, 24.0);
    if (tm <= 16.0) {
        t *= 0.5;
    } else if (tm <= 18.0) {
        t *= 0.7;
    } else if (tm <= 20.0) {
        t *= 0.5;
	} else {
        t *= 1.5;
    }

    // Formula for the shape, that changes over time
    float d = distance(p, center) * cos(t * log(t) * log(t) * atan(p.x, p.y)) * 10.0;

    //float s = smoothstep(0.2, 0.3, d);
    float s = smoothstep(0., 1.5*fwidth(d), d);

    // Set the color
    float red = 0.0;
    if (iTime < 16.0) {
        red = 1.0;
    }

	const vec3 spiral_color = vec3(1.0, 0.45, 0.0);
	vec3 stripes = vec3(1.0, 1.0, 1.0 - (bg.b*s)/2.0);

	C = vec4(mix(stripes, spiral_color, smoothstep(0.,1.,v)), 1.0);
}
