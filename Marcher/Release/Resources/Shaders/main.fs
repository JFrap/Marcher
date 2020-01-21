#version 330 core
out vec4 FragColor;

#define MAX_MARCHING_STEPS 512
#define MAX_DISTANCE 200.f

#define SHADOW_MAX_MARCHING_STEPS 128
#define SHADOW_MAX_DISTANCE 30.f

#define EPSILON 0.005f

struct Camera {
    vec3 Position, Target;
    vec3 TopLeft, TopRight, BottomLeft, BottomRight;
};

struct Ray {
    vec3 Origin, Direction;
};

uniform vec2 ScreenSize;
uniform Camera MainCamera;
uniform float Time;

uniform sampler3D Model;

Ray CalculateFragRay() {
    vec2 RelScreenPos = gl_FragCoord.xy / ScreenSize;

    vec3 TopPos = mix(MainCamera.TopLeft, MainCamera.TopRight, RelScreenPos.x);
    vec3 BottomPos = mix(MainCamera.BottomLeft, MainCamera.BottomRight, RelScreenPos.x);
    vec3 FinalPos = mix(TopPos, BottomPos, RelScreenPos.y);
    return Ray(FinalPos, normalize(FinalPos-MainCamera.Position));
}

vec4 permute(vec4 x){return mod(((x*34.0)+1.0)*x, 289.0);}
vec4 taylorInvSqrt(vec4 r){return 1.79284291400159 - 0.85373472095314 * r;}
vec3 fade(vec3 t) {return t*t*t*(t*(t*6.0-15.0)+10.0);}

float cnoise(vec3 P){
  vec3 Pi0 = floor(P); // Integer part for indexing
  vec3 Pi1 = Pi0 + vec3(1.0); // Integer part + 1
  Pi0 = mod(Pi0, 289.0);
  Pi1 = mod(Pi1, 289.0);
  vec3 Pf0 = fract(P); // Fractional part for interpolation
  vec3 Pf1 = Pf0 - vec3(1.0); // Fractional part - 1.0
  vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
  vec4 iy = vec4(Pi0.yy, Pi1.yy);
  vec4 iz0 = Pi0.zzzz;
  vec4 iz1 = Pi1.zzzz;

  vec4 ixy = permute(permute(ix) + iy);
  vec4 ixy0 = permute(ixy + iz0);
  vec4 ixy1 = permute(ixy + iz1);

  vec4 gx0 = ixy0 / 7.0;
  vec4 gy0 = fract(floor(gx0) / 7.0) - 0.5;
  gx0 = fract(gx0);
  vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
  vec4 sz0 = step(gz0, vec4(0.0));
  gx0 -= sz0 * (step(0.0, gx0) - 0.5);
  gy0 -= sz0 * (step(0.0, gy0) - 0.5);

  vec4 gx1 = ixy1 / 7.0;
  vec4 gy1 = fract(floor(gx1) / 7.0) - 0.5;
  gx1 = fract(gx1);
  vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
  vec4 sz1 = step(gz1, vec4(0.0));
  gx1 -= sz1 * (step(0.0, gx1) - 0.5);
  gy1 -= sz1 * (step(0.0, gy1) - 0.5);

  vec3 g000 = vec3(gx0.x,gy0.x,gz0.x);
  vec3 g100 = vec3(gx0.y,gy0.y,gz0.y);
  vec3 g010 = vec3(gx0.z,gy0.z,gz0.z);
  vec3 g110 = vec3(gx0.w,gy0.w,gz0.w);
  vec3 g001 = vec3(gx1.x,gy1.x,gz1.x);
  vec3 g101 = vec3(gx1.y,gy1.y,gz1.y);
  vec3 g011 = vec3(gx1.z,gy1.z,gz1.z);
  vec3 g111 = vec3(gx1.w,gy1.w,gz1.w);

  vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
  g000 *= norm0.x;
  g010 *= norm0.y;
  g100 *= norm0.z;
  g110 *= norm0.w;
  vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
  g001 *= norm1.x;
  g011 *= norm1.y;
  g101 *= norm1.z;
  g111 *= norm1.w;

  float n000 = dot(g000, Pf0);
  float n100 = dot(g100, vec3(Pf1.x, Pf0.yz));
  float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z));
  float n110 = dot(g110, vec3(Pf1.xy, Pf0.z));
  float n001 = dot(g001, vec3(Pf0.xy, Pf1.z));
  float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z));
  float n011 = dot(g011, vec3(Pf0.x, Pf1.yz));
  float n111 = dot(g111, Pf1);

  vec3 fade_xyz = fade(Pf0);
  vec4 n_z = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z);
  vec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
  float n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x); 
  return 2.2 * n_xyz;
}

float sphereSDF(in vec3 rPos, in vec3 sPos) {
    return distance(rPos, sPos) - 1;
}

vec3 SDFRepitition(in vec3 p, in vec3 c) {
    vec3 q = mod(p,c)-0.5*c;
    return q;
}

float opSmoothUnion(float d1, float d2, float k) {
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) - k*h*(1.0-h); 
}

float sdBox( vec3 p, vec3 b ) {
  vec3 d = abs(p) - b;
  return length(max(d,0.0))
         + min(max(d.x,max(d.y,d.z)),0.0); // remove this line for an only partially signed sdf 
}

/*
float scene1(in vec3 p) {
    float s = 100000;
    if (sdBox(p - vec3(1), vec3(1)) <= EPSILON) {
        s = min(texture(Model, p * vec3(1.f/2)).r, p.y);
    }
    else {
        s =  min(sdBox(p - vec3(1), vec3(1)), p.y);
    }
    float f = sphereSDF(SDFRepitition(p, vec3(6,0,6)), vec3(0));
    if (f <= EPSILON) {
        //f = max(f, cnoise(p+vec3(Time)));
    }

    return min(s, f);
}

float scene2(in vec3 p) {
    float f = 100000;

    f = sphereSDF(p, vec3(0,2,0));

    if (f <= 1) {
        f = sphereSDF(p, vec3(0,2,0)) + (cnoise(p + vec3(Time)) * 0.5 + 0.5) * 0.3;
    }

    return opSmoothUnion(p.y - (sin(length(p.xz) + Time * 4) * 0.5 + 0.5) / max(length(p.xz), 1), f, 1);
}
*/

float SceneSDF(in vec3 p) {
    float s = 100000;
    if (sdBox(p - vec3(1), vec3(1)) <= EPSILON) {
        s = texture(Model, p * vec3(1.f/2)).r;
    }
    else {
        s = sdBox(p - vec3(1), vec3(1));
    }
    float f = sphereSDF(SDFRepitition(p, vec3(6,0,6)), vec3(0));
    if (f <= EPSILON) {
        //f = max(f, cnoise(p+vec3(Time)));
    }

    return min(min(s, f), p.y);
}

float SceneSDFAO(in vec3 p) {
    float s = 100000;
    if (sdBox(p - vec3(1), vec3(1)) <= EPSILON) {
        s = texture(Model, p * vec3(1.f/2)).r;
    }
    else if (sdBox(p - vec3(1), vec3(1)) <= 0.5) {
        s = sdBox(p - vec3(1), vec3(1)) + texture(Model, p * vec3(1.f/2)).r;
    }
    float f = sphereSDF(SDFRepitition(p, vec3(6,0,6)), vec3(0));
    if (f <= EPSILON) {
        //f = max(f, cnoise(p+vec3(Time)));
    }

    return min(min(s, f), p.y);
}

vec3 EstimateNormal(in vec3 p) {
    const vec3 small_step = vec3(0.03125, 0.0, 0.0);

    float gradient_x = SceneSDF(p + small_step.xyy) - SceneSDF(p - small_step.xyy);
    float gradient_y = SceneSDF(p + small_step.yxy) - SceneSDF(p - small_step.yxy);
    float gradient_z = SceneSDF(p + small_step.yyx) - SceneSDF(p - small_step.yyx);

    vec3 normal = vec3(gradient_x, gradient_y, gradient_z);

    return normalize(normal);
}

struct MarchInfo {
    bool Hit;
    float Depth, MinDistance;
    vec3 Position, Normal;
    int Steps;
};

MarchInfo March(in Ray ray) {
    float depth = 0.f;
    float dist, minDist = MAX_DISTANCE;
    int i = 0;
    for (; i < MAX_MARCHING_STEPS; i++) {
        dist = SceneSDF(ray.Origin + (ray.Direction * depth));
        minDist = min(dist, minDist);
        if (dist < EPSILON) {
            if (dist < 0) {
                depth += dist; depth += dist;
            }
            return MarchInfo(true, depth, dist, ray.Origin + (ray.Direction * depth), EstimateNormal(ray.Origin + (ray.Direction * depth)), i);
        }
        depth += dist;
        if (depth >= MAX_DISTANCE) {
            return MarchInfo(false, depth, minDist, ray.Origin + (ray.Direction * depth), vec3(0), i);
        }
    }
    return MarchInfo(false, depth, minDist, ray.Origin + (ray.Direction * depth), vec3(0), i);
}

float SoftShadow(in Ray ray, in float k) {
    for(float t=EPSILON; t<SHADOW_MAX_DISTANCE;) {
        float h = SceneSDF(ray.Origin + ray.Direction*t);
        if(h<EPSILON)
            return 0;
        t += h;
    }
    return 1;
}

float genAmbientOcclusion(vec3 ro, vec3 rd) {
    vec4 totao = vec4(0.0);
    float sca = 1.0;

    for (int aoi = 0; aoi < 5; aoi++) {
        float hr = 0.01 + 0.02 * float(aoi * aoi);
        vec3 aopos = ro + rd * hr;
        float dd = SceneSDFAO(aopos);
        float ao = clamp(-(dd - hr), 0.0, 1.0);
        totao += ao * sca * vec4(1.0, 1.0, 1.0, 1.0);
        sca *= 0.75;
    }

    const float aoCoef = 1;
    return 1.0 - clamp(aoCoef * totao.w, 0.0, 1.0);
}

vec3 bgColor = vec3(0.6, 0.6, 1);

vec3 Render(in Ray ray) {
    MarchInfo info = March(ray);

    if (info.Hit) {
        float shadow = 1.f-SoftShadow(Ray(info.Position + info.Normal * EPSILON*2, normalize(vec3(1))), 8);
        
        float ret = max(dot(info.Normal, normalize(vec3(1))), 0.2f);
        ret -= shadow * (ret-0.2f);
        ret *= genAmbientOcclusion(info.Position + info.Normal * EPSILON, info.Normal);
        return mix(ret * vec3(1.f), bgColor, distance(ray.Origin, info.Position)/MAX_DISTANCE);
    }
    return bgColor;
}

void main() {
    Ray CamRay = CalculateFragRay();

    FragColor = vec4(Render(CamRay), 1.f);
}