uniform sampler2D tex;
uniform vec2 tex_dim;

void main() {
    gl_FragColor = GET_TEXEL(tex, gl_FragCoord.xy);
}
