uniform sampler2D tex;
uniform vec2 tex_dim;

varying vec2 vfUv;

vec4 celika_main() {
    return TEXELFETCH(tex, tex_dim, gl_FragCoord.xy);
}
