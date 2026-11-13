// MMPX reconstruction, MIT: licenses/MMPX.txt. Pinned C99 rules adapted for GPU.
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
uint mmpxLoad(ivec2 p){p=clamp(p,ivec2(0),ivec2(int(TextureSize.x)-1,int(TextureSize.y)-35));uvec3 c=uvec3(round(sourcePixel(vec2(p)).rgb*255.0));return(c.r<<16)|(c.g<<8)|c.b;}
uint luma(uint color)
{
    return (((color & 0x00FF0000u) >> 16) + ((color & 0x0000FF00u) >> 8) + (color & 0x000000FFu) + 1u);
}

bool all_eq2(uint B, uint A0, uint A1)
{
    return ((B ^ A0) | (B ^ A1)) == 0u;
}

bool all_eq3(uint B, uint A0, uint A1, uint A2)
{
    return ((B ^ A0) | (B ^ A1) | (B ^ A2)) == 0u;
}

bool all_eq4(uint B, uint A0, uint A1, uint A2, uint A3)
{
    return ((B ^ A0) | (B ^ A1) | (B ^ A2) | (B ^ A3)) == 0u;
}

bool any_eq3(uint B, uint A0, uint A1, uint A2)
{
    return B == A0 || B == A1 || B == A2;
}

bool none_eq2(uint B, uint A0, uint A1)
{
    return (B != A0) && (B != A1);
}

bool none_eq4(uint B, uint A0, uint A1, uint A2, uint A3)
{
    return B != A0 && B != A1 && B != A2 && B != A3;
}

vec3 mmpx(ivec2 halfcell){
 halfcell=clamp(halfcell,ivec2(0,0),ivec2(int(TextureSize.x)*2-1,(int(TextureSize.y)-34)*2-1));
 int srcX=halfcell.x/2,srcY=halfcell.y/2;

        uint A = mmpxLoad(ivec2(srcX - 1,srcY - 1));
        uint B = mmpxLoad(ivec2(srcX,srcY - 1));
        uint C = mmpxLoad(ivec2(srcX + 1,srcY - 1));

        uint D = mmpxLoad(ivec2(srcX - 1,srcY));
        uint E = mmpxLoad(ivec2(srcX,srcY));
        uint F = mmpxLoad(ivec2(srcX + 1,srcY));

        uint G = mmpxLoad(ivec2(srcX - 1,srcY + 1));
        uint H = mmpxLoad(ivec2(srcX,srcY + 1));
        uint I = mmpxLoad(ivec2(srcX + 1,srcY + 1));

        uint Q = mmpxLoad(ivec2(srcX - 2,srcY));
        uint R = mmpxLoad(ivec2(srcX + 2,srcY));

 uint J=E,K=E,L=E,M=E;
            if (((A ^ E) | (B ^ E) | (C ^ E) | (D ^ E) | (F ^ E) | (G ^ E) | (H ^ E) | (I ^ E)) != 0u)
            {
                uint P = mmpxLoad(ivec2(srcX,srcY - 2)), S = mmpxLoad(ivec2(srcX,srcY + 2));
                uint Bl = luma(B), Dl = luma(D), El = luma(E), Fl = luma(F), Hl = luma(H);

                // 1:1 slope rules
                {
                    if ((D == B && D != H && D != F) && (El >= Dl || E == A) && any_eq3(E, A, C, G) && ((El < Dl) || A != D || E != P || E != Q)) J = D;
                    if ((B == F && B != D && B != H) && (El >= Bl || E == C) && any_eq3(E, A, C, I) && ((El < Bl) || C != B || E != P || E != R)) K = B;
                    if ((H == D && H != F && H != B) && (El >= Hl || E == G) && any_eq3(E, A, G, I) && ((El < Hl) || G != H || E != S || E != Q)) L = H;
                    if ((F == H && F != B && F != D) && (El >= Fl || E == I) && any_eq3(E, C, G, I) && ((El < Fl) || I != H || E != R || E != S)) M = F;
                }

                // Intersection rules
                {
                    if ((E != F && all_eq4(E, C, I, D, Q) && all_eq2(F, B, H)) && (F != mmpxLoad(ivec2(srcX + 3,srcY)))) K = M = F;
                    if ((E != D && all_eq4(E, A, G, F, R) && all_eq2(D, B, H)) && (D != mmpxLoad(ivec2(srcX - 3,srcY)))) J = L = D;
                    if ((E != H && all_eq4(E, G, I, B, P) && all_eq2(H, D, F)) && (H != mmpxLoad(ivec2(srcX,srcY + 3)))) L = M = H;
                    if ((E != B && all_eq4(E, A, C, H, S) && all_eq2(B, D, F)) && (B != mmpxLoad(ivec2(srcX,srcY - 3)))) J = K = B;
                    if (Bl < El && all_eq4(E, G, H, I, S) && none_eq4(E, A, D, C, F)) J = K = B;
                    if (Hl < El && all_eq4(E, A, B, C, P) && none_eq4(E, D, G, I, F)) L = M = H;
                    if (Fl < El && all_eq4(E, A, D, G, Q) && none_eq4(E, B, C, I, H)) K = M = F;
                    if (Dl < El && all_eq4(E, C, F, I, R) && none_eq4(E, B, A, G, H)) J = L = D;
                }

                // 2:1 slope rules
                {
                    if (H != B)
                    {
                        if (H != A && H != E && H != C)
                        {
                            if (all_eq3(H, G, F, R) && none_eq2(H, D, mmpxLoad(ivec2(srcX + 2,srcY - 1)))) L = M;
                            if (all_eq3(H, I, D, Q) && none_eq2(H, F, mmpxLoad(ivec2(srcX - 2,srcY - 1)))) M = L;
                        }

                        if (B != I && B != G && B != E)
                        {
                            if (all_eq3(B, A, F, R) && none_eq2(B, D, mmpxLoad(ivec2(srcX + 2,srcY + 1)))) J = K;
                            if (all_eq3(B, C, D, Q) && none_eq2(B, F, mmpxLoad(ivec2(srcX - 2,srcY + 1)))) K = J;
                        }
                    } // H !== B

                    if (F != D)
                    {
                        if (D != I && D != E && D != C)
                        {
                            if (all_eq3(D, A, H, S) && none_eq2(D, B, mmpxLoad(ivec2(srcX + 1,srcY + 2)))) J = L;
                            if (all_eq3(D, G, B, P) && none_eq2(D, H, mmpxLoad(ivec2(srcX + 1,srcY - 2)))) L = J;
                        }

                        if (F != E && F != A && F != G)
                        {
                            if (all_eq3(F, C, H, S) && none_eq2(F, B, mmpxLoad(ivec2(srcX - 1,srcY + 2)))) K = M;
                            if (all_eq3(F, I, B, P) && none_eq2(F, H, mmpxLoad(ivec2(srcX - 1,srcY - 2)))) M = K;
                        }
                    } // F !== D
                } // 2:1 slope
            }


 uint color=(halfcell.y&1)==0?((halfcell.x&1)==0?J:K):((halfcell.x&1)==0?L:M);
 return vec3(float((color>>16)&255u),float((color>>8)&255u),float(color&255u))/255.0;
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
    FragColor = mix(mix(vec4(mmpx(ivec2(lo)),1.0),vec4(mmpx(ivec2(lo)+ivec2(1,0)),1.0),blend.x),
                    mix(vec4(mmpx(ivec2(lo)+ivec2(0,1)),1.0),vec4(mmpx(ivec2(lo)+ivec2(1,1)),1.0),blend.x),blend.y);
}
#endif
