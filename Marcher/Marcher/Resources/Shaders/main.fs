#version 330 core
out vec4 FragColor;

struct Camera {
    vec3 Position, Target;
    vec3 TopLeft, TopRight, BottomLeft, BottomRight;
};

struct Ray {
    vec3 Origin, Direction;
};

uniform float EPSILON;
uniform float MAX_DISTANCE;
uniform int MAX_MARCHING_STEPS;

uniform bool ShadowsEnabled;
uniform float ShadowStrength;
uniform float AOStrength;

uniform vec3 AmbientColor;
uniform vec3 LightColor;
uniform vec3 LightDir;

uniform vec2 ScreenSize;
uniform Camera MainCamera;
uniform float Time;

//uniform sampler3D Model;

Ray CalculateFragRay() {
    vec2 RelScreenPos = gl_FragCoord.xy / ScreenSize;

    vec3 TopPos = mix(MainCamera.TopLeft, MainCamera.TopRight, RelScreenPos.x);
    vec3 BottomPos = mix(MainCamera.BottomLeft, MainCamera.BottomRight, RelScreenPos.x);
    vec3 FinalPos = mix(TopPos, BottomPos, RelScreenPos.y);
    return Ray(FinalPos, normalize(FinalPos-MainCamera.Position));
}

float SceneSDF(in vec3 p);

// float s = 100000;
// if (sdBox(p - vec3(1), vec3(1)) <= EPSILON) {
//     s = texture(Model, p * vec3(1.f/2)).r;
// }
// else {
//     s = sdBox(p - vec3(1), vec3(1));
// }
// float f = sphereSDF(SDFRepitition(p, vec3(6,0,6)), vec3(0));
// if (f <= EPSILON) {
//     //f = max(f, cnoise(p+vec3(Time)));
// }

// return min(min(s, f), p.y);

/*float SceneSDFAO(in vec3 p) {
    // float s = 100000;
    // if (sdBox(p - vec3(1), vec3(1)) <= EPSILON) {
    //     s = texture(Model, p * vec3(1.f/2)).r;
    // }
    // else if (sdBox(p - vec3(1), vec3(1)) <= 0.5) {
    //     s = sdBox(p - vec3(1), vec3(1)) + texture(Model, p * vec3(1.f/2)).r;
    // }
    // float f = sphereSDF(SDFRepitition(p, vec3(6,0,6)), vec3(0));
    // if (f <= EPSILON) {
    //     //f = max(f, cnoise(p+vec3(Time)));
    // }

    // return min(min(s, f), p.y);
    return min(sphereSDF(SDFRepitition(p, vec3(6,6,6)), vec3(0)), p.y);
}*/

vec3 EstimateNormal(in vec3 p) {
    vec3 small_step = vec3(EPSILON, 0.0, 0.0);

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

float Shadow(in Ray ray) {
    for(float t=EPSILON; t<MAX_DISTANCE;) {
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
        float dd = SceneSDF(aopos);
        float ao = clamp(-(dd - hr), 0.0, 1.0);
        totao += ao * sca * vec4(1.0, 1.0, 1.0, 1.0);
        sca *= 0.75;
    }
    
    return 1.0 - clamp(AOStrength * totao.w, 0.0, 1.0);
}

vec3 Render(in Ray ray) {
    MarchInfo info = March(ray);

    if (info.Hit) {
        float shadow = 0.f;
        if (ShadowsEnabled) {
            shadow = 1.f-Shadow(Ray(info.Position + info.Normal * EPSILON*2, normalize(-LightDir)));
        }
        
        vec3 ret = LightColor * max(dot(info.Normal, normalize(-LightDir)), 0.f);
        ret -= vec3(shadow * ShadowStrength) * ret;
        ret += AmbientColor * (vec3(1)-ret);

        ret *= genAmbientOcclusion(info.Position + info.Normal * EPSILON, info.Normal);
        return mix(ret * vec3(1.f), AmbientColor, distance(ray.Origin, info.Position)/MAX_DISTANCE);
    }
    return AmbientColor;
}

void SetDiffuse(in vec3 col) {

}

void main() {
    Ray CamRay = CalculateFragRay();

    if (SceneSDF(CamRay.Origin) < EPSILON) {
        FragColor = vec4(0,0,0,1);
    }
    else {
        FragColor = vec4(Render(CamRay), 1.f);
    }
}

float SceneSDF(in vec3 p) {
    float sphere = distance(p + vec3(sin(p.x*20)*0.01), vec3(0, 1, 0)) - 1;
    float plane = p.y;

    return min(sphere, plane);
}