float SceneSDF(in vec3 p) {
    float sphere = distance(p + vec3(sin(p.x*20)*0.01), vec3(0, 1, 0)) - 1;
    float plane = p.y;

    return min(sphere, plane);
}