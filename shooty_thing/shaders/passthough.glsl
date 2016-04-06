uniform sampler2D tex;
uniform vec2 tex_dim;

varying vec2 vfUV;

void main() {
    gl_FragColor = GET_TEXEL(tex, tex_dim, gl_FragCoord.xy);
}
