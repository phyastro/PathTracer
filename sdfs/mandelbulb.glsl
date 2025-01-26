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

        // f_n(c) = f_n-1(c)^8 + c
        // Something Is Different With The Structure When We Go From 2 To 4 To 8 Times The Angle Than Directly Going To 8 Times The Angle.
        /*
        for (int k = 0; k < 3; k++) {
            float tant2 = z.x / z.z;
            float cosp2 = z.y / r;
            float cosp = 2.0 * cosp2 * cosp2 - 1.0;
            float sinp = 2.0 * sqrt(1.0 - cosp2 * cosp2) * cosp2;
            float cost = (2.0 / (tant2 * tant2 + 1.0)) - 1.0;
            float sint = 2.0 * tant2 / (tant2 * tant2 + 1.0);
            r *= r;
            z = r * vec3(sint * sinp, cosp, cost * sinp);
        }
        z += p;
        */
        float theta = 8.0 * atan(z.x, z.z);
        float phi = 8.0 * acos(z.y / r);
        z = pow(r, 8.0) * vec3(sin(theta) * sin(phi), cos(phi), cos(theta) * sin(phi)) + p;
        //z = vec3(z.x * z.x - z.y * z.y - z.z * z.z, 2.0 * z.y * (z.x), 2.0 * z.x * z.z);
        //z = vec3(z.x * z.x - z.y * z.y - z.z * z.z, 2.0 * z.y * (z.x + z.z), 2.0 * z.x * z.z);
        //z = vec3(z.x * z.x - z.y * z.y - z.z * z.z, 2.0 * z.y * (z.x + z.z), 2.0 * z.x * z.z);
        //z += p;
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
    return 4.0;
}
