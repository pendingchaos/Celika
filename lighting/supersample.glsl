uniform sampler2D uTex;
uniform vec2 uTex_dim;
uniform float uSSAA;

varying vec2 vfUv;

vec4 celika_main() {
    vec4 res = vec4(0.0);
    vec2 base = gl_FragCoord.xy * vec2(uSSAA);
    vec2 off;
    for (off.x = 0.0; off.x < uSSAA; off.x++)
        for (off.y = 0; off.y < uSSAA; off.y++)
            res += TEXELFETCH(uTex, uTex_dim, base+off);
    return res / (uSSAA*uSSAA);
}
