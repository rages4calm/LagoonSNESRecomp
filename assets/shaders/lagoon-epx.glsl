// Title-owned EPX/Scale2x decision rules, independently implemented.
// https://www.scale2x.it/algorithm . Palette-based reconstruction, followed by
// a one-output-pixel fractional edge footprint; no frame history or full-image blur.
#ifdef VERTEX
in vec2 VertexCoord;
in vec2 TexCoord;
out vec2 vTexCoord;
void main() {
    vTexCoord = TexCoord;
    gl_Position = vec4(VertexCoord * 2.0 - 1.0, 0.0, 1.0);
}
#endif
#ifdef FRAGMENT
uniform sampler2D Texture;
uniform vec2 TextureSize;
uniform vec2 OutputSize;
in vec2 vTexCoord;
out vec4 FragColor;
vec4 sourcePixel(vec2 cell) {
    cell = clamp(cell, vec2(0.0), TextureSize - vec2(1.0));
    return texture(Texture, (cell + vec2(0.5)) / TextureSize);
}
vec4 epxPixel(vec2 halfCell) {
    halfCell = clamp(halfCell, vec2(0.0), vec2(TextureSize.x * 2.0 - 1.0, (TextureSize.y - 34.0) * 2.0 - 1.0));
    vec2 position = (halfCell + vec2(0.5)) * 0.5;
    vec2 cell = floor(position);
    vec4 e = sourcePixel(cell);
    // Uploaded texture row zero is the first CPU field row. The vertex
    // transform handles screen orientation, so this remains the bottom HUD.
    float hud = max(TextureSize.y - 34.0, 0.0);
    if (cell.y >= hud) return e;
    vec4 b = sourcePixel(vec2(cell.x, max(cell.y - 1.0, 0.0)));
    vec4 h = sourcePixel(vec2(cell.x, min(cell.y + 1.0, hud - 1.0)));
    vec4 d = sourcePixel(cell - vec2(1.0, 0.0));
    vec4 f = sourcePixel(cell + vec2(1.0, 0.0));
    vec2 quadrant = fract(position);
    vec4 result = e;
    if (any(notEqual(b,h)) && any(notEqual(d,f))) {
        if (quadrant.y < 0.5) {
            if (quadrant.x < 0.5 && all(equal(d,b))) result=d;
            if (quadrant.x >= 0.5 && all(equal(b,f))) result=f;
        } else {
            if (quadrant.x < 0.5 && all(equal(d,h))) result=d;
            if (quadrant.x >= 0.5 && all(equal(h,f))) result=f;
        }
    }
    return result;
}
void main() {
    vec2 p = clamp(vTexCoord * TextureSize, vec2(0.0), TextureSize - vec2(0.0001));
    vec2 grid = p * 2.0;
    vec2 span = max(TextureSize * 2.0 / OutputSize, vec2(0.00001));
    if (p.y >= TextureSize.y - 34.0) { FragColor = sourcePixel(floor(p + vec2(0.00001))); return; }
    // Sharp fractional scaling of the reconstructed 2x lattice. Only the
    // one-output-pixel edge footprint blends; interiors stay solid.
    vec2 center = floor(grid) + vec2(0.5);
    vec2 at = center + clamp((grid-center)/span, vec2(-0.5), vec2(0.5)) - vec2(0.5);
    vec2 lo = floor(at), blend = fract(at);
    FragColor = mix(mix(epxPixel(lo),epxPixel(lo+vec2(1,0)),blend.x),
                    mix(epxPixel(lo+vec2(0,1)),epxPixel(lo+vec2(1,1)),blend.x),blend.y);
}
#endif
