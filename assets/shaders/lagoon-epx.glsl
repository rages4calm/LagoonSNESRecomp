// Title-owned EPX/Scale2x decision rules, independently implemented.
// https://www.scale2x.it/algorithm . Keeps source colors; no frame history.
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
in vec2 vTexCoord;
out vec4 FragColor;
vec4 sourcePixel(vec2 cell) {
    cell = clamp(cell, vec2(0.0), TextureSize - vec2(1.0));
    return texture(Texture, (cell + vec2(0.5)) / TextureSize);
}
void main() {
    vec2 position = clamp(vTexCoord * TextureSize, vec2(0.0), TextureSize - vec2(0.0001));
    vec2 cell = floor(position);
    vec4 e = sourcePixel(cell);
    // Uploaded texture row zero is the first CPU field row. The vertex
    // transform handles screen orientation, so this remains the bottom HUD.
    float hud = max(TextureSize.y - 32.0, 0.0);
    if (cell.y >= hud) { FragColor = e; return; }
    vec4 b = sourcePixel(vec2(cell.x, max(cell.y - 1.0, 0.0)));
    vec4 h = sourcePixel(vec2(cell.x, min(cell.y + 1.0, hud - 1.0)));
    vec4 d = sourcePixel(cell - vec2(1.0, 0.0));
    vec4 f = sourcePixel(cell + vec2(1.0, 0.0));
    vec2 quadrant = fract(position);
    FragColor = e;
    if (any(notEqual(b,h)) && any(notEqual(d,f))) {
        if (quadrant.y < 0.5) {
            if (quadrant.x < 0.5 && all(equal(d,b))) FragColor=d;
            if (quadrant.x >= 0.5 && all(equal(b,f))) FragColor=f;
        } else {
            if (quadrant.x < 0.5 && all(equal(d,h))) FragColor=d;
            if (quadrant.x >= 0.5 && all(equal(h,f))) FragColor=f;
        }
    }
}
#endif
