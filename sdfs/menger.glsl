// https://iquilezles.org/articles/menger/
float sdf(in vec3 p)
{
    vec3 q = abs(p) - vec3(1.0);
    float d = length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);

    float s = 1.0;
    for (int m = 0; m < 5; m++)
    {
        vec3 a = mod(p * s, 2.0) - 1.0;
        s *= 3.0;
        vec3 r = abs(1.0 - 3.0 * abs(a));

        float da = max(r.x, r.y);
        float db = max(r.y, r.z);
        float dc = max(r.z, r.x);
        float c = (min(da, min(db, dc)) - 1.0) / s;

        d = max(d, c);
    }

    return d;
}

float sdfmaterial(in vec3 p)
{
    return 0.0;
}
