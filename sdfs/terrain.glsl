
float terrian(in float x) {
    return sin(0.0625 * x) + sin(0.125 * (x + 10.0)) + 0.25 * sin(0.25 * x) + 0.125 * sin(0.5 * x) + 0.25 * sin(x) + 0.25 * sin(2.0 * x + 1.0) + 0.125 * sin(4.0 * x + 2.0) + 0.0625 * sin(8.0 * x + 1.0) + 0.03125 * sin(16.0 * x + 5.0) + 0.015625 * sin(32.0 * x) + 0.0078125 * sin(64.0 * x);
}

float sdf(in vec3 p) {
    return p.y - (terrian(p.x) + terrian(p.z));
}

float sdfmaterial(in vec3 p)
{
    return 0.0;
}
