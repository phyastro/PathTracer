float sdf(in vec3 p)
{
    float s1 = length(p) - 1.0;
    float s2 = length(vec3(p.x, p.y - 1.0, p.z)) - 0.7;
    float s3 = length(vec3(p.x - 0.25, p.y - 1.25, p.z - 0.5)) - 0.25;
    float s4 = length(vec3(p.x + 0.25, p.y - 1.25, p.z - 0.5)) - 0.25;
    return smin(smin(smin(s1, s2), s3), s4);
}

float sdfmaterial(in vec3 p)
{
    vec2 s1 = vec2(length(p) - 1.0, 4.0);
    vec2 s2 = vec2(length(vec3(p.x, p.y - 1.0, p.z)) - 0.7, 5.0);
    vec2 s3 = vec2(length(vec3(p.x - 0.25, p.y - 1.25, p.z - 0.5)) - 0.25, 6.0);
    vec2 s4 = vec2(length(vec3(p.x + 0.25, p.y - 1.25, p.z - 0.5)) - 0.25, 6.0);
    return smin(smin(smin(s1, s2), s3), s4).y;
}
