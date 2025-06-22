
struct PSInput
{
    float4 p: SV_POSITION;
    float4 d: COLOR;
};

PSInput main_vs(float4 position : POSITION, float4 color : COLOR)
{
    PSInput o;

    o.p = position;
    o.d = color;

    return o;
}

float4 main_ps(PSInput vi) : SV_TARGET0
{
	return vi.d;
}
