// Mandelbulb 3D
// http://blog.hvidtfeldts.net/index.php/2011/09/distance-estimated-3d-fractals-v-the-mandelbulb-different-de-approximations/
float sdf(in vec3 p)
{
    vec3 z = p;
    float r = length(z);
    float dr = 1.0;

    for (int i = 0; i < 4; i++) {
        // Differentiate f_n(c) w.r.t c
        // Scalar Derivative
        dr = 8.0 * pow(r, 7.0) * dr + 1.0;

        float invr = 1.0 / r;
        float invl = inversesqrt(dot(vec2(z.x, z.z), vec2(z.x, z.z)));
        r *= r;
        r *= r;
        r *= r;

        // f_n(c) = f_n-1(c)^8 + c
        // Using Chebyshev Polynomials
        // Faster Than Inverse Trigonometric Functions
        float cost = z.z * invl;
        float cosp = z.y * invr;
        float sint = z.x * invl;
        float sinp = invr / invl;
        float cost2 = cost * cost;
        float cosp2 = cosp * cosp;
        float cost8 = fma(fma(fma(fma(128.0, cost2, -256.0), cost2, 160.0), cost2, -32.0), cost2, 1.0);
        float cosp8 = fma(fma(fma(fma(128.0, cosp2, -256.0), cosp2, 160.0), cosp2, -32.0), cosp2, 1.0);
        float sint8 = fma(fma(fma(128.0, cost2, -192.0), cost2, 80.0), cost2, -8.0) * sint * cost;
        float sinp8 = fma(fma(fma(128.0, cosp2, -192.0), cosp2, 80.0), cosp2, -8.0) * sinp * cosp;
        z = r * vec3(sint8 * sinp8, cosp8, cost8 * sinp8);
        z += p;

        // f_n(c) = f_n-1(c)^8 + c
        // Using Inverse Trigonometric Functions
        // For Some Reason acos Is Much Faster Than atan For Me
        //float theta = 8.0 * acos(z.z * invl);
        //float phi = 8.0 * acos(z.y * invr);
        //z = r * vec3(sin(theta) * sin(phi), cos(phi), cos(theta) * sin(phi)) + p;
        r = length(z);

        // This Is Placed At Bottom Otherwise We Waste The Last Iteration Calculation
        if (r > 16.0) {
            break;
        }
    }

    // DE Approximation
    return 0.5 * log(r) * r / dr;
}

float sdfmaterial(in vec3 p)
{
    float factor = dot(p, p);
    return mix(4.0, 3.0, factor / (0.8 + factor));
}
