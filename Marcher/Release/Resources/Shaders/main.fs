#version 330 core
out vec4 FragColor;

#define MAX_MARCHING_STEPS 512
#define MAX_DISTANCE 50.f

#define SHADOW_MAX_MARCHING_STEPS 128
#define SHADOW_MAX_DISTANCE 50.f

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

float sphereSDF(in vec3 rPos, in vec3 sPos) {
    return distance(rPos, sPos) - 1.2;
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

float SceneSDF(in vec3 p) {
    if (sdBox(p - vec3(1), vec3(1)) <= EPSILON) {
        return min(texture(Model, p * vec3(1.f/2)).r, p.y);
    }
    return min(sdBox(p - vec3(1), vec3(1)), p.y);

    // return min(p.y, sphereSDF(p, vec3(0,1,0)));
}

vec3 EstimateNormal(in vec3 p) {
    const vec3 small_step = vec3(0.001, 0.0, 0.0);

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
};

MarchInfo March(in Ray ray) {
    float depth = 0.f;
    float dist, minDist = MAX_DISTANCE;
    for (int i = 0; i < MAX_MARCHING_STEPS; i++) {
        dist = SceneSDF(ray.Origin + (ray.Direction * depth));
        minDist = min(dist, minDist);
        if (dist < EPSILON) {

            return MarchInfo(true, depth, dist, ray.Origin + (ray.Direction * depth), EstimateNormal(ray.Origin + (ray.Direction * depth)));
        }
        depth += dist;
        if (depth >= MAX_DISTANCE) {
            return MarchInfo(false, depth, minDist, ray.Origin + (ray.Direction * depth), vec3(0));
        }
    }
    return MarchInfo(false, depth, minDist, ray.Origin + (ray.Direction * depth), vec3(0));
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

vec3 Render(in Ray ray) {
    MarchInfo info = March(ray);

    if (info.Hit) {
        float shadow = 1.f-SoftShadow(Ray(info.Position + info.Normal * EPSILON, normalize(vec3(1))), 8);

        float ret = max(dot(info.Normal, normalize(vec3(1))), 0.1f);
        ret -= shadow * (ret-0.1f);
        return mix(ret * vec3(1.f), vec3(0.5f), distance(ray.Origin, info.Position)/MAX_DISTANCE);
    }
    return vec3(0.5f);
}

void main() {
    Ray CamRay = CalculateFragRay();

    FragColor = vec4(Render(CamRay), 1.f);
}